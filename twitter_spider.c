#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#define SIZE_1K                 1024
#define SIZE_2M                 (2 * 1024 * 1024)
#define RM_HTML                 "rm -rf /tmp/fangongheike.html"
#define RM_MESSAGE              "rm -rf /tmp/message"
#define ACCESS_CMD              "wget -e \"https_proxy=http://127.0.0.1:8118\" https://twitter.com/fangongheike -t 1 -q -O /tmp/fangongheike.html" \
								" --dns-timeout=3 --connect-timeout=4 --read-timeout=240"
#define MAIL_CMD1               "cat /tmp/message | mail -s Twitter_update 13967188560@139.com"
#define MAIL_CMD2               "cat /tmp/message | mail -s Twitter_update 13757109183@139.com"
#define MAIL_CMD3               "cat /tmp/message | mail -s Twitter_update 13656680967@139.com"
#define UPDATE_LABLE            "<div class=\"js-tweet-text-container\">"

char *g_message;

int init(void) {
    g_message = malloc(SIZE_1K);
    if (NULL == g_message) {
        return -1;
    }
    
    memset(g_message, 0, SIZE_1K);

    (void)daemon(1, 0);

    return 0;
}

int scan(void) {
    int size = 0;
    int msg_len = 0;
    FILE *fp = NULL;
    char *p_update = NULL;
    char *p_msg_start = NULL;
    char *p_msg_end = NULL;
    char *p_html = NULL;

    fp = fopen("/tmp/fangongheike.html", "r");
    if (NULL == fp) {
        return -1;
    }

    p_html = malloc(SIZE_2M);
    if (NULL == p_html) {
        fclose(fp);
        return -1;
    }

    memset(p_html, 0, SIZE_2M);

    while (0 == feof(fp)) {
        size = fread(p_html, 1, SIZE_2M, fp);
        if (size <= 0) {
            free(p_html);
            fclose(fp);
            return -1;
        }

        p_update = strstr(p_html, UPDATE_LABLE);
        if (NULL == p_update) {
            continue;
        }

        p_update += strlen(UPDATE_LABLE);

        p_msg_start = strchr(p_update, '>');
        if (NULL == p_msg_start) {
            free(p_html);
            fclose(fp);
            return -1;
        }

        p_msg_start += 1;
        p_msg_end = strchr(p_msg_start, '<');
        if (NULL == p_msg_end) {
            free(p_html);
            fclose(fp);
            return -1;
        }

        msg_len = (p_msg_end - p_msg_start > SIZE_1K) ? SIZE_1K : (p_msg_end - p_msg_start);

        if ('\0' == g_message[0]) {
            syslog(LOG_INFO, "get twitter update\n");
            memcpy(g_message, p_msg_start, msg_len);

            free(p_html);
            fclose(fp);
            return 0;
        } else if (0 == memcmp(g_message, p_msg_start, msg_len)) {
            syslog(LOG_INFO, "twitter do not update\n");
            free(p_html);
            fclose(fp);
            return 0;
        } else {
            memset(g_message, 0, SIZE_1K);
            memcpy(g_message, p_msg_start, msg_len);
            
            syslog(LOG_INFO, "twitter updated\n");

            free(p_html);
            fclose(fp);
            return 1;
        }
    }

    free(p_html);
    fclose(fp);
    return 0;
}

void report(void) {
    int size;
    char *key;
    char key_str[32] = {0};
    FILE *fp = NULL;
    
    fp = fopen("/tmp/message", "w");
    if (NULL == fp) {
        return;
    }
    
    size = fwrite(g_message, strlen(g_message), 1, fp);
    if (1 != size) {
        fclose(fp);
        return;
    }

    fflush(fp);

    fclose(fp);

    (void)system(MAIL_CMD1);
    (void)sleep(3);
    (void)system(MAIL_CMD2);
    (void)sleep(3);
    (void)system(MAIL_CMD3);

    syslog(LOG_INFO, "twitter update has been mailed\n");

    return;
}

int main(int argc, char *argv[]) {
    int ret = 0;

    ret = init();
    if (ret < 0) {
        return -1;
    }
	
    while (1) {
        (void)sleep(60);

        (void)system(RM_HTML);
        (void)system(RM_MESSAGE);
        (void)system(ACCESS_CMD);

        ret = scan();
        if (1 != ret) {
            continue;
        }

        report();
    }
}
