/*************************************************************************
	> File Name: tcp_server.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Fri 23 Oct 2020 04:25:35 PM CST
 ************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    assert(signo != 0);
    running = 0;
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

    if (register_signal(SIGINT, 0, sig_handler) == SIG_ERR) {
        ERR_PRINT("signal register failed\n");
        exit(EXIT_FAILURE);
    }

    if (register_signal(SIGTERM, 0, sig_handler) == SIG_ERR) {
        ERR_PRINT("signal register failed\n");
        exit(EXIT_FAILURE);
    }

    //struct sockaddr_in ex;
    //printf("size of sockaddr_in: %u\n", sizeof(struct sockaddr_in));
    //printf("size of sa_family_t: %u\n", sizeof(ex.sin_family));
    //printf("size of sin_addr: %u\n", sizeof(ex.sin_addr));
    //printf("size of sin_port: %u\n", sizeof(ex.sin_port));

    int listenfd = 0;
    if ((listenfd = tcp_server_listen(NULL, port)) < 0) {
        ERR_PRINT("server start failed\n");
        exit(EXIT_FAILURE);
    }

    int maxfd = listenfd;
    ERR_PRINT("listenfd = %d\n", listenfd);
    fd_set readfd, exfd;
    FD_ZERO(&readfd);
    FD_SET(listenfd, &readfd);
    fd_set dupli = readfd;
    int cli[1024];
    struct Buffer sbuff[1024];
    for (int i = 0; i < 1024; i++) cli[i] = -1;

    while (running) {
        hexdump(&readfd, sizeof(readfd));
        //struct timeval tv;
        //tv.tv_sec = 0;
        //tv.tv_usec = 800 * 1000;
        readfd = dupli;
        FD_ZERO(&exfd);
        ERR_PRINT("wait I/O\n");
        //int ret = select(maxfd + 1, &readfd, NULL, NULL, &tv);
        int ret = select(maxfd + 1, &readfd, NULL, &exfd, NULL);
        if (ret < 0) {
            ERR_PRINT("select error: %s", strerror(errno));
        }
        printf("after select\n");
        hexdump(&readfd, sizeof(readfd));

        if (ret == 0) {
            ERR_PRINT("time out");
            continue;
        }

        if (FD_ISSET(listenfd, &readfd)) {
            ERR_PRINT("get accept event, try to accept\n");
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
                int id = 0;
                for (; id < 1024 && cli[id] >= 0; id++) {}
                if (id == 1024) {
                    ERR_PRINT("client exceed limit\n");
                    close(sockfd);
                } else {
                    //skbuff初始化
                    ERR_PRINT("sbuff init\n");
                    memset(&sbuff[id], 0x0, sizeof(struct Buffer));
                    make_noblocking(cli[id]);
                    cli[id] = sockfd;
                    strcpy(sbuff[id].ip, ip);
                    sbuff[id].port = port;
                    FD_SET(sockfd, &dupli);
                    if (sockfd > maxfd) {
                        maxfd = sockfd;
                    }
                }
            }
        }

        ERR_PRINT("check client\n");
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
                        ERR_PRINT("client %s:%d in exception, close socket...\n", sbuff[i].ip, sbuff[i].port);
                        close(cli[i]);
                        FD_CLR(cli[i], &dupli);
                        cli[i] = -1;
                        printf("read error\n");
                        hexdump(&dupli, sizeof(dupli));
                    }
                } else if (readn == 0) {
                    printf("client %s:%d exit, close socket...\n", sbuff[i].ip, sbuff[i].port);
                    close(cli[i]);
                    FD_CLR(cli[i], &dupli);
                    cli[i] = -1;
                    printf("peer close\n");
                    hexdump(&dupli, sizeof(dupli));
                } else {
                    printf("read from [%s:%d]: %s\n", sbuff[i].ip, sbuff[i].port, tbuff);
                    if (readn < 4096 - sbuff[i].rindex - 1) {
                        strncpy(sbuff[i].buff + sbuff[i].rindex, tbuff, strlen(tbuff));
                        sbuff[i].rindex += readn;
                    } else {
                        strncpy(sbuff[i].buff + sbuff[i].rindex, tbuff, 4096 - sbuff[i].rindex - 1);
                        sbuff[i].buff[4095] = 0;
                        sbuff[i].rindex = 4095;
                    }
                    printf("buff status: %s\n", sbuff[i].buff);
                    printf("read index: %d, write index: %d\n", sbuff[i].rindex, sbuff[i].windex);
                    ERR_PRINT("try to echo back(try 3 times):\n");
                    int written = sendn(cli[i], sbuff[i].buff + sbuff[i].windex, sbuff[i].rindex - sbuff[i].windex);
                    if (written > 0) {
                        ERR_PRINT("%d bytes data send\n", written);
                        sbuff[i].windex += written;
                        if (sbuff[i].windex == sbuff[i].rindex) {
                            sbuff[i].windex = sbuff[i].rindex = 0;
                        } else {
                            ERR_PRINT("send to [%s:%d] error\n", sbuff[i].ip, sbuff[i].port);
                            close(cli[i]);
                            FD_CLR(cli[i], &dupli);
                            cli[i] = -1;
                            printf("wirte error\n");
                            hexdump(&dupli, sizeof(dupli));
                        }
                    }
                }
            }
        }
        readfd = dupli;
        printf("after = dupli\n");
        hexdump(&readfd, sizeof(readfd));
        ERR_PRINT("check client end\n");
    }

    return 0;
}
