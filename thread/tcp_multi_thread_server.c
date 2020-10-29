/*************************************************************************
	> File Name: tcp_multi_thread_server.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Thu 29 Oct 2020 05:40:11 PM CST
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    assert(signo != 0);
    running = 0;
}

void echo_loop(int fd) {
    char buff[1024];
    while (running) {
        memset(buff, 0, sizeof(buff));
        int rcn = recv(fd, buff, sizeof(buff), 0);
        if (rcn < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            ERR_PRINT("recv failed: %s", strerror(errno));
            close(fd);
            pthread_exit(0);
        } else if (rcn == 0) {
            ERR_PRINT("peer close connection");
            close(fd);
            pthread_exit(0);
        } else {
            ERR_PRINT("thread %ld get msg: %s", pthread_self(), buff);
            int sdn = send(fd, buff, strlen(buff), 0);
            if (sdn < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                ERR_PRINT("send failed: %s", strerror(errno));
                close(fd);
                pthread_exit(0);
            }
        }
    }
}

void* thread_run(void *arg) {
    pthread_detach(pthread_self());
    int fd = *(int *)arg;
    echo_loop(fd);
    return NULL;
}

int main() {
    if (register_signal(SIGINT, 0, sig_handler) < 0) {
        ERR_PRINT("register sigint failed\n");
        exit(EXIT_FAILURE);
    }

    int sockfd = tcp_server_listen1("127.0.0.1", 11111);
    if (sockfd < 0) {
        exit(EXIT_FAILURE);
    }

    pthread_attr_t attr;
    Pthread_attr_init(&attr);
    Pthread_attr_setstacksize(&attr, 409600);
    struct sockaddr_in client;
    socklen_t len;
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
    pthread_t thread_id;
    while (running) {
        int connfd = accept(sockfd, (struct sockaddr*)&client, &len);
        if (connfd < 0) {
            ERR_PRINT("accept failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        } else {
            Pthread_create(&thread_id, &attr, thread_run, &connfd);
            inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
            port = ntohs(client.sin_port);
            printf("client [%s:%u] dispatch for thread %ld\n",
                   ip, port, thread_id);
        }
    }
    Pthread_attr_destroy(&attr);

    return 0;
}
