/*************************************************************************
	> File Name: tcp_client.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Mon 26 Oct 2020 04:35:52 PM CST
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    assert(signo != 0);
    running = 0;
}

int main(void) {
#if 0
    if (argc != 2) {
        printf("\nUsage: %s <#port>\n\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int para = atoi(argv[1]);
    unsigned short port = para & 0xFF;
#else
    unsigned short port = 11111;
#endif

    if (register_signal(SIGINT, 0, sig_handler) < 0) {
        ERR_PRINT("signal register failed\n");
        exit(EXIT_FAILURE);
    }

    if (register_signal(SIGTERM, 0, sig_handler) < 0) {
        ERR_PRINT("signal register failed\n");
        exit(EXIT_FAILURE);
    }

    char tbuff[4096];
    int fd = tcp_client_connect(NULL, port);
    if (fd < 0) {
        ERR_PRINT("tcp connect failed\n");
        exit(EXIT_FAILURE);
    }

    fd_set readfd;
    FD_ZERO(&readfd);
    FD_SET(STDIN_FILENO, &readfd);
    FD_SET(fd, &readfd);
    int maxfd = fd > STDIN_FILENO? fd: STDIN_FILENO;
    fd_set dupli = readfd;

    while(running) {
        readfd = dupli;
        int sret = select(maxfd + 1, &readfd, NULL, NULL, NULL);
        switch (sret) {
        case -1:
            {
                if (errno == EINTR) continue;
                ERR_PRINT("select failed: %s", strerror(errno));
            }
            break;
        case 0:
            {
                ERR_PRINT("time out\n");
            }
            break;
        default:
            {
                if (FD_ISSET(STDIN_FILENO, &readfd)) {
                    memset(tbuff, 0, sizeof(tbuff));
                    fgets(tbuff, sizeof(tbuff), stdin);
                    tbuff[4095] = 0;
                    if (sendn(fd, tbuff, strlen(tbuff)) < 0) {
                        ERR_PRINT("send error, exit...");
                        close(fd);
                        exit(EXIT_FAILURE);
                    }
                }

                if (FD_ISSET(fd, &readfd)) {
                    memset(tbuff, 0, sizeof(tbuff));
                    int readn = recv(fd, tbuff, sizeof(tbuff), 0);
                    if (readn < 0) {
                        if (errno == EAGAIN || errno == EINTR) {
                            continue;
                        } else {
                            ERR_PRINT("read error: %s", strerror(errno));
                            close(fd);
                        }
                    } else if (readn == 0) {
                        printf("peer exit\n");
                        close(fd);
                        exit(EXIT_FAILURE);
                    } else {
                        printf("receive from server: %s\n", tbuff);
                    }
                }
            }
            break;
        }
    }

    return 0;
}
