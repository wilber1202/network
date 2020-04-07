#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define FOUR_HOURS  14400

int main(int argc, char *argv[]) {
    int i;
    int count = 0;
    char cmd[128] = {0};

    (void)daemon(1, 0);

    while (1) {
        for (i = 0; i < 100; i++) {
            memset(cmd, 0, 128);
            sprintf(cmd, "pkill -kill -t tty%d", i);
            //printf("command: %s\r\n", cmd);
            system(cmd);
        }
        
        for (i = 0; i < 100; i++) {
            memset(cmd, 0, 128);
            sprintf(cmd, "pkill -kill -t pts/%d", i);
            //printf("command: %s\r\n", cmd);
            system(cmd);
        }
        
        sleep(1);
        count++;
        if (count >= FOUR_HOURS) {
            return 0;
        }
    }
}
