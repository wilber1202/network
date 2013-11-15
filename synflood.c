#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
 
#define ETH_P_IP 0x0800
 
typedef struct tagIPHead
{
    unsigned char  ucVerLen;
    unsigned char  ucTos;
    unsigned short usTotalLen;
    unsigned short usIdentity;
    unsigned short usFlags;
    unsigned char  ucTtl;
    unsigned char  ucProtocol;
    unsigned short usCheckSum;
    unsigned int   uiSrcIP;
    unsigned int   uiDstIP;
}IPHEAD_S;
 
typedef struct tagTCPHead
{
    unsigned short usSrcPort;
    unsigned short usDstPort;
    unsigned int   uiSeq;
    unsigned int   uiAck;
    unsigned char  ucLen;
    unsigned char  ucFlag;
    unsigned short usWin;
    unsigned short usCheckSum;
    unsigned short usAgentP;
}TCPHEAD_S;
 
typedef struct tagTCPMSS
{
    unsigned char  ucKind;
    unsigned char  ucLen;
    unsigned short usMSS;
}TCPMSS_S;
 
typedef struct tagTCPSACK
{
    unsigned char ucKind;
    unsigned char ucLen;
}TCPSACK_S;
 
typedef struct tagTCPTS
{
    unsigned char  ucKind;
    unsigned char  ucLen;
    unsigned int   uiTS;
    unsigned int   uiACK;
}TCPTS_S;
 
typedef struct tagTCPWSCALE
{
    unsigned char  ucNop;
    unsigned char  ucKind;
    unsigned char  ucLen;
    unsigned char  ucValue;
}TCPWINSCALE_S;
 
unsigned short in_cksum(unsigned short *addr, int len) 
{
    int sum            = 0; 
    unsigned short res = 0; 
 
    assert(NULL != addr);
 
    while (len > 1)  
    { 
        sum += *addr++; 
        len -=2; 
    } 
     
    if (1 == len) 
    { 
        *((unsigned char *)(&res)) = *((unsigned char *)addr); 
        sum += res; 
    }
  
    sum  = (sum >>16) + (sum & 0xffff); 
    sum += (sum >>16); 
    res  = ~sum; 
 
    return res; 
} 
 
void formatTCP(TCPHEAD_S *pstTCPHead, unsigned short usDstPort)
{
    assert(NULL != pstTCPHead);
 
    pstTCPHead->usSrcPort  = htons(12580);
    pstTCPHead->usDstPort  = htons(usDstPort);
    pstTCPHead->uiSeq      = htonl(0x01020304);
    pstTCPHead->uiAck      = 0;
    pstTCPHead->ucLen      = 80;
    pstTCPHead->ucFlag     = 2;
    pstTCPHead->usWin      = htons(14600);
    pstTCPHead->usAgentP   = 0;
    pstTCPHead->usCheckSum = in_cksum((unsigned short *)pstTCPHead, 20);
}
 
void formatTCPOption(char *pTCPOption)
{
    assert(NULL != pTCPOption);
 
    TCPMSS_S *pstMSS = NULL;
    TCPSACK_S *pstSACK = NULL;
    TCPTS_S   *pstTS = NULL;
    TCPWINSCALE_S *pstWinScale = NULL;
 
    pstMSS = (TCPMSS_S *)pTCPOption;
    pstMSS->ucKind = 0x02;
    pstMSS->ucLen = 0x04;
    pstMSS->usMSS = htons(1460);
 
    pstSACK = (TCPSACK_S *)(pstMSS + 1);
    pstSACK->ucKind = 0x04;
    pstSACK->ucLen = 0x02;
 
    pstTS = (TCPTS_S *)(pstSACK + 1);
    pstTS->ucKind = 0x08;
    pstTS->ucLen = 0x0a;
    pstTS->uiTS = (htonl)((unsigned int)time(NULL));
    pstTS->uiACK = 0;
 
    pstWinScale = (TCPWINSCALE_S *)(pTCPOption + 16);
    pstWinScale->ucNop = 0x01;
    pstWinScale->ucKind = 0x03;
    pstWinScale->ucLen = 0x03;
    pstWinScale->ucValue = 0x06;
 
    return;
}
 
void formatIP(IPHEAD_S *pstIPHead, unsigned uiTotalLen, unsigned uiAttackedIP)
{
    assert(NULL != pstIPHead);
 
    pstIPHead->ucVerLen   = 0x45;
    pstIPHead->ucTos      = 0;
    //pstIPHead->usTotalLen = htons(uiTotalLen);
    pstIPHead->usTotalLen = 0;
    pstIPHead->usIdentity = htons(0xbd66);
    //pstIPHead->usIdentity = 0;
    pstIPHead->usFlags    = 0;
    pstIPHead->ucTtl      = 62;
    pstIPHead->ucProtocol = 6;
    //pstIPHead->uiSrcIP    = inet_addr("10.7.85.56");
    pstIPHead->uiSrcIP    = 0;
    pstIPHead->uiDstIP    = uiAttackedIP;
    //pstIPHead->usCheckSum = in_cksum((unsigned short *)pstIPHead, (int)uiTotalLen);
    pstIPHead->usCheckSum = 0;
 
    return;
}
 
int main(int argc, char *argv[])
{
    int ret = 0;
    int sockfd = 0;
    int optval = 1;
    unsigned short usDstPort = 0;
    unsigned int uiAttackedIP = 0;
    char buffer[60] = {0};
    struct sockaddr_in sin;
 
    if (3 != argc)
    {
        printf("miss args...\r\n");
        return -1;
    }
 
    if (INADDR_NONE == inet_addr(argv[1]))
    {
        printf("dst address invalid...\r\n");
        return -1;
    }
 
    uiAttackedIP = (unsigned int)inet_addr(argv[1]);
 
    usDstPort = atoi(argv[2]);
    if (usDstPort > 65535)
    {
        printf("dst port invalid...\r\n");
        return -1;
    }
 
    sin.sin_family      = AF_INET;
    sin.sin_port        = htons(usDstPort);
    sin.sin_addr.s_addr = uiAttackedIP;
 
    formatIP((IPHEAD_S *)buffer, 40, uiAttackedIP);
    formatTCP((TCPHEAD_S *)(buffer + 20), usDstPort);
    //formatTCPOption(buffer + 40);
 
    sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (-1 == sockfd) {
        perror("socket");
        return -1;
    }
 
    ret = setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &optval, sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }
 
    ret = sendto(sockfd, buffer, 40, 0, (struct sockaddr *)&sin, sizeof (sin));
    if (-1 == ret) {
        perror("send");
        return -1;
    }
 
    close(sockfd);
 
    return 0;
}
