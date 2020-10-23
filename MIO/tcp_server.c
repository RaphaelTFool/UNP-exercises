/*************************************************************************
	> File Name: tcp_server.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Fri 23 Oct 2020 04:25:35 PM CST
 ************************************************************************/

#include <stdio.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    running = 0;
}

int tcp_server_listen(const char* ip, unsigned short port) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ERR_PRINT("socket error: %s", strerror(errno));
        return -1;
    }

    int flags = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, O_NONBLOCK | flags) < 0) {
        ERR_PRINT("socket set nonblock failed: %s", strerror(errno));
    }

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (ip) {
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr) <= 0) {
            ERR_PRINT("inet_ntop failed: %s", strerror(errno));
            return -1;
        }
    } else {
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        ERR_PRINT("bind failed: %s", strerror(errno));
        return -1;
    }

    int listenfd;
    if ((listenfd = listen(fd, 5)) < 0) {
        ERR_PRINT("listen failed: %s", strerror(errno));
        return -1;
    }

    return listenfd;
}

struct Buffer {
    int fd;
    char ip[INET_ADDRSTRLEN];
    unsigned short port;
    char buff[4096];
    int rindex;
    int windex;
};

//int main(int argc, char *argv[]) {
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
    int listenfd = 0;
    if ((listenfd = tcp_server_listen(NULL, port)) < 0) {
        ERR_PRINT("server start failed\n");
        exit(EXIT_FAILURE);
    }

    if (register_signal(SIGINT, 0, sig_handler) == SIG_ERR) {
        ERR_PRINT("signal register failed\n");
        exit(EXIT_FAILURE);
    }

    if (register_signal(SIGTERM, 0, sig_handler) == SIG_ERR) {
        ERR_PRINT("signal register failed\n");
        exit(EXIT_FAILURE);
    }

    int maxfd = listenfd;
    fd_set readfd;
    FD_ZERO(&readfd);
    FD_SET(listenfd, &readfd);
    fd_set dupli = readfd;
    int cli[1024];
    struct Buffer sbuff[1024];
    for (int i = 0; i < 1024; i++) cli[i] = -1;

    while (running) {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50 * 1000;
        readfd = dupli;
        int ret = select(maxfd + 1, &readfd, NULL, NULL, &tv);
        if (ret < 0) {
            ERR_PRINT("select error: %s", strerror(errno));
            continue;
        }

        if (ret == 0) {
            continue;
        }

        if (FD_ISSET(listenfd, &readfd)) {
            struct sockaddr_in cli_addr;
            socklen_t clen; 
            int sockfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clen);
            if (sockfd < 0) {
                ERR_PRINT("accept failed: %s", strerror(errno));
            } else {
                char ip[INET_ADDRSTRLEN];
                unsigned short port = ntohs(cli_addr.sin_port);
                inet_ntop(AF_INET, &cli_addr.sin_addr.s_addr, ip, sizeof(cli_addr));
                printf("client: %s:%d connecting...\n", ip, port);
                if (maxfd < sockfd) maxfd = sockfd;
                int id = 0;
                for (; id < 1024 && cli[id] >= 0; id++) {}
                if (id == 1024) {
                    ERR_PRINT("client exceed limit\n");
                    close(sockfd);
                } else {
                    memset(&sbuff[id], 0x0, sizeof(struct Buffer));
                    cli[id] = sockfd;
                    strcpy(sbuff[id].ip, ip);
                    sbuff[id].port = port;
                }
            }
        }

        char tbuff[4096];
        for (int i = 0; i < 1024; i++) {
            if (cli[i] < 0) continue;
            if (FD_ISSET(cli[i], &readfd)) {
                memset(tbuff, 0x0, 4096);
                sbuff[i].fd = cli[i];
                int readn = recv(cli[i], tbuff, 4096, 0);
                if (readn < 0) {
                    if (errno == EWOULDBLOCK || errno == EAGAIN) continue;
                    else {
                        ERR_PRINT("client %s:%d in exception, close socket...", sbuff[i].ip, sbuff[i].port);
                        close(cli[i]);
                        cli[i] = -1;
                    }
                }
                if (readn == 0) {
                    printf("client %s:%d exit, close socket...", sbuff[i].ip, sbuff[i].port);
                    close(cli[i]);
                    cli[i] = -1;
                }
            }
        }
    }

    return 0;
}
