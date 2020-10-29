/*************************************************************************
	> File Name: tcp_epoll_server_LT.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Wed 28 Oct 2020 06:07:08 PM CST
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    assert(signo != 0);
    running = 0;
}

#define MAXFDS 128

int main() {
    if (register_signal(SIGINT, 0, sig_handler) < 0) {
        ERR_PRINT("register signal failed\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = tcp_server_listen("127.0.0.1", 11111); 
    if (sockfd < 0) {
        ERR_PRINT("server start failed\n");
        exit(EXIT_FAILURE);
    }
    make_noblocking(sockfd);

    struct epoll_event *events = (struct epoll_event*)calloc(MAXFDS, sizeof(struct epoll_event));
    if (!events) {
        ERR_PRINT("calloc failed: %s", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int efd = epoll_create1(EPOLL_CLOEXEC);

    struct epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &event) < 0) {
        ERR_PRINT("add event failed: %s", strerror(errno));
        close(sockfd);
        close(efd);
        free(events);
        exit(EXIT_FAILURE);
    }

    while (running) {
        int n = epoll_wait(efd, events, MAXFDS, -1);
        if (n < 0) {
            if (errno == EINTR) continue;
            ERR_PRINT("epoll_wait failed: %s", strerror(errno));
            break;
        } else if (n == 0) {
            ERR_PRINT("timeout");
        } else {
            for (int i = 0; i < n; i++) {
                if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                    ERR_PRINT("error happen on fd %d", events[i].data.fd);
                    epoll_ctl(efd, EPOLL_CTL_DEL, events[i].data.fd, &events[i]);
                    close(events[i].data.fd);
                } else if (events[i].data.fd == sockfd) {
                    struct sockaddr_in client;
                    socklen_t len = sizeof(client);
                    char ip[INET_ADDRSTRLEN];
                    int clifd = accept(sockfd, (struct sockaddr*)&client, &len);
                    if (clifd < 0) {
                        ERR_PRINT("accept failed: %s", strerror(errno));
                        continue;
                    } else {
                        make_noblocking(clifd);
                        struct epoll_event event;
                        event.data.fd = clifd;
                        event.events = EPOLLIN;
                        //event.events = EPOLLIN | EPOLLET;
                        if (epoll_ctl(efd, EPOLL_CTL_ADD, clifd, &event) < 0) {
                            ERR_PRINT("epoll_ctl failed: %s", strerror(errno));
                        }
                            printf("client [%s:%u] connected...\n",
                               inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip)),
                               ntohs(client.sin_port));
                    }
                } else {
                    int connfd = events[i].data.fd;
                    printf("epoll get event from fd %d, data:", connfd);
                    char c;
                    int rcn = recv(connfd, &c, 1, 0);
                    if (rcn < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;
                        ERR_PRINT("recv failed: %s", strerror(errno));
                        if (epoll_ctl(efd, EPOLL_CTL_DEL, connfd, &events[i]) < 0) {
                            ERR_PRINT("epoll_ctl delete failed: %s", strerror(errno));
                        }
                        close(connfd);
                    } else if (rcn == 0) {
                        ERR_PRINT("peer connection close");
                        ERR_PRINT("recv failed: %s", strerror(errno));
                        if (epoll_ctl(efd, EPOLL_CTL_DEL, connfd, &events[i]) < 0) {
                            ERR_PRINT("epoll_ctl delete failed: %s", strerror(errno));
                        }
                    } else {
                        if (c != '\n') printf("%c\n", c);
                        else printf("return\n");
                        int sdn = send(connfd, &c, 1, 0);
                        if (sdn < 0) {
                            if (errno == EINTR || errno == EAGAIN) continue;
                            ERR_PRINT("send failed: %s", strerror(errno));
                            if (epoll_ctl(efd, EPOLL_CTL_DEL, connfd, &events[i]) < 0) {
                                ERR_PRINT("epoll_ctl delete failed: %s", strerror(errno));
                            }
                            close(connfd);
                        }
                    }
                }
            }
        }
    }

    close(sockfd);
    close(efd);
    free(events);

    return 0;
}
