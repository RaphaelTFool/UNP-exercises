/*************************************************************************
	> File Name: tcp_multi_process_server.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Thu 29 Oct 2020 01:53:57 PM CST
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
//利用linux接口在父进程退出时确保子进程也退出的方法
#include <sys/prctl.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    assert(signo != 0);
    running = 0;
}

void child_handler(int signo) {
    assert(signo != 0);
    while (waitpid(-1, 0, WNOHANG) > 0);
}

void child_run(int fd) {
#if 0
    if (prctl(PR_SET_PDEATHSIG, SIGINT) < 0) {
        ERR_PRINT("prctl set signal failed: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
#endif
    if (register_signal(SIGCHLD, 0, SIG_IGN) < 0) {
        ERR_PRINT("clear signal failed\n");
        exit(EXIT_FAILURE);
    }
    char buff[1024];
    while (running) {
        memset(buff, 0, sizeof(buff));
        int rdn = recv(fd, buff, sizeof(buff), 0);
        if (rdn < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            ERR_PRINT("read error: %s", strerror(errno));
            ERR_PRINT("child %d exit", getpid());
            close(fd);
            exit(EXIT_FAILURE);
        } else if (rdn == 0) {
            ERR_PRINT("client close connection\n");
            close(fd);
            exit(EXIT_FAILURE);
        } else {
            printf("child %d get message: %s", getpid(), buff);
            int sdn = send(fd, buff, strlen(buff), 0);
            if (sdn < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                ERR_PRINT("send failed: %s", strerror(errno));
                close(fd);
                exit(EXIT_FAILURE);
            }
        }
    }
    printf("process %d exit\n", getpid());
}

int main() {
    if (register_signal(SIGINT, 0, sig_handler) < 0) {
        ERR_PRINT("register sigint failed");
        exit(EXIT_FAILURE);
    }

#if 1
    if (register_signal(SIGCHLD, 0, child_handler) < 0) {
        ERR_PRINT("register sigchld failed");
        exit(EXIT_FAILURE);
    }
#endif

    //不使用多路IO监听事件而采用非阻塞服务套接字会一直报错
    int sockfd = tcp_server_listen1("127.0.0.1", 11111);
    if (sockfd < 0) {
        ERR_PRINT("tcp server start failed\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    char ip[INET_ADDRSTRLEN];
    uint16_t port;
    while (running) {
        int connfd = accept(sockfd, (struct sockaddr*)&client, &len);
        if (connfd < 0) {
            if (errno == EINTR) continue;
            ERR_PRINT("accept failed: %s", strerror(errno));
            exit(EXIT_FAILURE);
        } else {
            inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(ip));
            port = ntohs(client.sin_port);
            printf("client [%s:%d] connected\n", ip, port);
            pid_t pid = fork();
            switch (pid) {
            case -1:
                ERR_PRINT("fork failed: %s", strerror(errno));
                exit(EXIT_FAILURE);
                break;

            case 0:
                close(sockfd);
                child_run(connfd);
                exit(EXIT_SUCCESS);
                break;
                
            default:
                close(connfd);
            }
        }
    }
}
