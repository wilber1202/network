#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "geoip.h"

int cidr(unsigned int uiSmallIP, unsigned int uiBigIP, CIDR_S *pstCIDR)
{
    unsigned int  i              = 0;
    unsigned int  j              = 0;
    unsigned int  k              = 0;
    unsigned int  uiCount        = 0;
    unsigned long ulOrNum        = 0;
    unsigned int  uiSeporatorNum = 0;
    unsigned int  uiLowestOnePos = 32;    /* 0 to 31 */
    /* struct in_addr addr; */

    if ((NULL == pstCIDR) || (uiSmallIP > uiBigIP)) {
        printf("cidr args error...\r\n");
        return -1;
    }

    for (i = uiSmallIP; i < uiBigIP; i++) {
        /* find lowest bit which is 1 */
        for (j = 0; j < 32; j++) {
            if (i & (1 << j)) {
                uiLowestOnePos = j;
                break;
            }
        }

        if (0 == uiLowestOnePos) {
            /* lowest bit is 1, a seporate CIDR */
            /* addr.s_addr = (in_addr_t)htonl(i);
            printf("CIDR: %s/%d\r\n", inet_ntoa(addr), MAX_MASK); */
            pstCIDR[uiCount].uiIP   = i;
            pstCIDR[uiCount].uiMask = MAX_MASK;
            uiCount++;
            if (uiCount >= 32) {
                printf("too many segments...\r\n");
                return 0;
            }
            continue;
        }

        for (k = uiLowestOnePos; 0 != k; k--) {
            ulOrNum        = ((unsigned long)1 << k) - 1;
            uiSeporatorNum = (unsigned int)(i | ulOrNum);

            if (uiSeporatorNum == uiBigIP) {
                /* generate */
                /* addr.s_addr = (in_addr_t)htonl(i);
                printf("CIDR: %s/%d\r\n", inet_ntoa(addr), MAX_MASK - k); */

                pstCIDR[uiCount].uiIP   = i;
                pstCIDR[uiCount].uiMask = MAX_MASK - k;

                return 0;
            } else if (uiSeporatorNum < uiBigIP) {
                /* generate */
                /* addr.s_addr = (in_addr_t)htonl(i);
                printf("CIDR: %s/%d\r\n", inet_ntoa(addr), MAX_MASK - k); */

                pstCIDR[uiCount].uiIP   = i;
                pstCIDR[uiCount].uiMask = MAX_MASK - k;
                uiCount++;

                if (uiCount >= 32) {
                    printf("too many segments...\r\n");
                    return 0;
                }

                i = uiSeporatorNum;
                break;
            } else {
                continue;
            }    
        }

        /* only one IP remain */
        if ((i + 1) == uiBigIP) {
            /* addr.s_addr = (in_addr_t)htonl(uiBigIP);
            printf("CIDR: %s/%d\r\n", inet_ntoa(addr), MAX_MASK); */
            pstCIDR[uiCount].uiIP   = uiBigIP;
            pstCIDR[uiCount].uiMask = MAX_MASK;
        }

        /* do not need to set uiLowestOnePos to 32 */ 
    }

    return 0;
}

#ifdef CIDR_TEST
int main(int argc, char *argv[])
{
    unsigned int i = 0;
    unsigned int iSmallIP;
    unsigned int iBigIP;
    CIDR_S stCIDR[32] = {0};

    if (argc != 3) {
        printf("arg invalid\r\n");
        return -1;
    }

    iSmallIP = ntohl(inet_addr(argv[1]));
    iBigIP = ntohl(inet_addr(argv[2]));

    cidr(iSmallIP, iBigIP, stCIDR);

    for (; 0 != stCIDR[i].uiIP; i++) {
        printf("IP: %u, Mask: %u\r\n", stCIDR[i].uiIP, stCIDR[i].uiMask);
    }

    return 0;    
}
#endif
