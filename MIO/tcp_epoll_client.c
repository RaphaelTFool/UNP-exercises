/*************************************************************************
	> File Name: tcp_epoll_client.c
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

    int efd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event events[10];
    struct epoll_event event;
    event.data.fd = fileno(stdin);
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, fileno(stdin), &event) < 0) {
        ERR_PRINT("add stdin failed\n");
        close(connfd);
        close(efd);
        exit(EXIT_FAILURE);
    }
    event.data.fd = connfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event) < 0) {
        ERR_PRINT("add client connect fd failed\n");
        exit(EXIT_FAILURE);
    }

    while (running) {
        int readyn = epoll_wait(efd, events, sizeof(events), -1);
        if (readyn < 0) {
            if (errno == EINTR) continue;
            ERR_PRINT("epoll_wait error: %s", strerror(errno));
            close(efd);
            exit(EXIT_FAILURE);
        } else if (readyn == 0) {
            ERR_PRINT("timeout\n");
        } else {
            for (int i = 0; i < readyn; i++) {
                if (events[i].data.fd == fileno(stdin)) {
                    char buff[1024];
                    if (fgets(buff, sizeof(buff), stdin)) {
                        int len = strlen(buff);
                        if (buff[len - 1] == '\n') buff[len - 1] = 0;
                        char content[2048];
                        sprintf(content, "%05d%s", (int)strlen(buff), buff);
                        if (send(connfd, content, strlen(content), 0) < 0) {
                            if (errno == EINTR || errno == EAGAIN) continue;
                            ERR_PRINT("send error: %s", strerror(errno));
                            close(connfd);
                            close(efd);
                            exit(EXIT_FAILURE);
                        }
                    }
                } else if (events[i].data.fd == connfd) {
                    char buff[1024];
                    int rdn = recv(connfd, buff, sizeof(buff), MSG_WAITALL);
                    if (rdn < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;
                        ERR_PRINT("read error: %s", strerror(errno));
                        close(connfd);
                        close(efd);
                        exit(EXIT_FAILURE);
                    } else if (rdn == 0) {
                        ERR_PRINT("server close socket\n");
                        close(connfd);
                        close(efd);
                        exit(EXIT_FAILURE);
                    } else {
                        char len_str[6];
                        if (rdn > 5) {
                            strncpy(len_str, buff, 5);
                            len_str[5] = 0;
                            int len = atoi(len_str);
                            printf("received %d bytes message:\n%s\n", len, buff + 5);
                        }
                    }
                }
            }
        }
    }
    close(efd);

    return 0;
}
