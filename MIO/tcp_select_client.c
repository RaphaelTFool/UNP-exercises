/*************************************************************************
	> File Name: tcp_select_client.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Wed 28 Oct 2020 09:13:47 AM CST
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    assert(signo != 0);
    running = 0;
}

int main() {
    if (register_signal(SIGINT, 0, sig_handler) < 0) {
        ERR_PRINT("signal registered failed\n");
        exit(EXIT_FAILURE);
    }

    unsigned short port = 11111;
    int connfd = tcp_client_connect("127.0.0.1", port);
    if (connfd < 0) {
        ERR_PRINT("connect server failed\n");
        exit(EXIT_FAILURE);
    }

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    FD_SET(connfd, &rfds);
    fd_set read_mask;

    while (running) {
        read_mask = rfds;
        int sret = select(connfd + 1, &read_mask, NULL, NULL, NULL); 
        switch (sret) {
        case -1:
            if (errno == EINTR) continue;
            ERR_PRINT("select error: %s", strerror(errno));
            break;
            
        case 0:
            ERR_PRINT("select timeout\n");
            break;

        default:
            if (FD_ISSET(STDIN_FILENO, &read_mask)) {
                printf("get a stdio read fdset\n");
                char buff[4096];
                char content[8192];
                if (fgets(buff, sizeof(buff), stdin)) {
                    int len = strlen(buff);
                    if (buff[len - 1] == '\n') buff[len - 1] = 0;
                    sprintf(content, "%05d%s", (int)strlen(buff), buff);
                    int sdn = send(connfd, content, strlen(content), 0);
                    if (sdn <= 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;
                        ERR_PRINT("send error: %s", strerror(errno));
                    } else {
                        printf("buff len: %d bytes, send %d bytes\n", (int)strlen(buff), sdn);
                    }
                } else {
                    ERR_PRINT("fgets error: %s", strerror(errno));
                }
            }
            if (FD_ISSET(connfd, &read_mask)) {
                printf("get a socket read fdset\n");
                char buff[4096];
                char len_str[6];
                memset(buff, 0, sizeof(buff));
                int rcn = recv(connfd, buff, sizeof(buff), 0);
                if (rcn < 0) {
                    if (errno == EINTR || errno == EAGAIN) continue;
                    ERR_PRINT("read error: %s", strerror(errno));
                    FD_CLR(connfd, &rfds);
                    close(connfd);
                } else if (rcn == 0) {
                    ERR_PRINT("peer close connection");
                    FD_CLR(connfd, &rfds);
                    close(connfd);
                    exit(EXIT_FAILURE);
                } else {
                    printf("read %d bytes\n", rcn);
                    if (rcn >= 5) {
                        memcpy(len_str, buff, 5);
                        len_str[5] = 0;
                        int len = atoi(len_str);
                        printf("get %d bytes message.\n", len);
                        printf("%s\n", buff + 5);
                    }
                }
            }
            break;
        }
    }

    return 0;
}
