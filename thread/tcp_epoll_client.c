/*************************************************************************
	> File Name: tcp_epoll_client.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Fri 06 Nov 2020 04:51:40 PM CST
 ************************************************************************/

#include <stdio.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    running = 0;
}

int main() {
    if (register_signal(SIGINT, 0, sig_handler) < 0) {
        ERR_PRINT("register signal failed");
        exit(EXIT_FAILURE);
    }

    int sockfd = tcp_client_connect("127.0.0.1", 9000);
    if (sockfd < 0) {
        ERR_PRINT("client start failed");
        exit(EXIT_FAILURE);
    }
    make_noblocking(sockfd);

    int efd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev;
    ev.data.fd = STDIN_FILENO;
    ev.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) < 0) {
        handle_errno(errno);
    }
    ev.data.fd = sockfd;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &ev) < 0) {
        handle_errno(errno);
    }

    struct epoll_event* evs = Calloc(10, sizeof(struct epoll_event));
    char heartbeat[] = "helloworld";
    struct timeval tv;
    gettimeofday(&tv, 0);
    srand(tv.tv_usec);
    int count = 0;

    while (running) {
        if (count > 5000) {
            kill(SIGINT, getpid());
        } else {
            sleep(rand() % 10);
            int ret = send(sockfd, heartbeat, strlen(heartbeat), 0);
            if (ret < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                ERR_PRINT("send failed");
            }
            count++;
        }
        int n = epoll_wait(efd, evs, 10, -1);
        switch (n) {
        case -1:
            if (errno == EINTR) continue;
            handle_errno(errno);
            break;

        case 0:
            ERR_PRINT("time out");
            break;

        default:
            for (int i = 0; i < n; i++) {
                if ((evs[i].events & EPOLLERR ||
                     evs[i].events & EPOLLHUP ||
                     !(evs[i].events & EPOLLIN))) {
                    ERR_PRINT("fd %d get error, close it", evs[i].data.fd);
                    continue;
                }
                char buff[4096] = { 0 };
                size_t nbytes;
                int fd = evs[i].data.fd;
                if (fd == STDIN_FILENO) {
                    fgets(buff, 4096, stdin);
                    if (buff[strlen(buff) - 1] == '\n') {
                        buff[strlen(buff) - 1] = 0;
                    }
                    nbytes = send(sockfd, buff, strlen(buff), 0);
                    if (nbytes < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;
                        ERR_PRINT("send failed: %s", strerror(errno));
                        close(sockfd);
                        close(efd);
                        exit(EXIT_FAILURE);
                    }
                } else if (fd == sockfd) {
                    nbytes = recv(sockfd, buff, 4096, 0);
                    if (nbytes < 0) {
                        if (errno == EINTR || errno == EAGAIN) continue;
                        ERR_PRINT("recv failed: %s", strerror(errno));
                        close(sockfd);
                        close(efd);
                        exit(EXIT_FAILURE);
                    } else if (nbytes == 0) {
                        ERR_PRINT("server exit");
                        close(sockfd);
                        close(efd);
                        exit(EXIT_FAILURE);
                    } else {
                        printf("%s\n", buff);
                    }
                }
            }
            break;
        }
    }
    close(efd);

    return 0;
}
