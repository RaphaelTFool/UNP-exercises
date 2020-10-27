/*************************************************************************
	> File Name: tcp_block_server.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Tue 27 Oct 2020 02:11:11 PM CST
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
    if (register_signal(SIGINT, 0, sig_handler) == SIG_ERR) {
        printf("signal register failed\n");
        exit(EXIT_FAILURE);
    }

    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket:");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt:");
        exit(EXIT_FAILURE);
    }

    //block
    //make_noblocking(fd);

    struct sockaddr_in serv;
    bzero(&serv, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = htonl(INADDR_ANY);
    serv.sin_port = htons(11111);

    if (bind(fd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("bind:");
        exit(EXIT_FAILURE);
    }

    listen(fd, 1024);

#if 0
    struct sockaddr_in cli;
    socklen_t clen = sizeof(cli);
    sleep(5);
    int cfd = accept(fd, (struct sockaddr*)&cli, &clen);
    char ip[INET_ADDRSTRLEN];
    printf("client [%s:%d] connected\n", 
           inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip)),
           ntohs(cli.sin_port));
    while (running) {
        char buff[1024] = { 0 };
        int rn = recv(cfd, buff, sizeof(buff), 0);
        if (rn <= 0) {
            break;
        }
    }

#else
    int status[1024] = { 0 };
    int maxfd = fd;
    fd_set rfd;

    while (running) {
        FD_ZERO(&rfd);
        FD_SET(fd, &rfd);
        for (int i = 0; i < 1024; i++) {
            if (status[i]) {
                FD_SET(i, &rfd);
            }
        }

        int sret = select(maxfd + 1, &rfd, NULL, NULL, NULL);
        switch (sret) {
        case -1:
            {
                perror("select:");
                continue;
            }
            break;

        case 0:
            {
                printf("timeout\n");
                continue;
            }
            break;

        default:
            {
                if (FD_ISSET(fd, &rfd)) {
                    sleep(10);
                    struct sockaddr_in client;
                    socklen_t clen = sizeof(client);
                    char ip[INET_ADDRSTRLEN];
                    int cli = accept(fd, (struct sockaddr*)&client, &clen);
                    if (cli < 0) {
                        perror("accept:");
                    } else {
                        printf("client [%s:%d] connected\n", 
                               inet_ntop(AF_INET, &client.sin_addr, ip, sizeof(client)), ntohs(client.sin_port));
                        if (cli < 1024) {
                            //make_noblocking(cli);
                            if (cli > maxfd) {
                                maxfd = cli;
                            }
                            status[cli] = 1;
                        } else {
                            close(cli);
                        }
                    }
                }

                for (int i = 0; i < 1024; i++) {
                    if (status[i] && FD_ISSET(i, &rfd)) {
                        char buff[4096] = { 0 };
                        int rdn = recv(i, buff, sizeof(buff), 0);
                        switch (rdn) {
                        case -1:
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                continue;
                            } else {
                                status[i] = 0;
                                close(i);
                            }
                            break;

                        case 0:
                            status[i] = 0;
                            close(i);
                            break;

                        default: 
                            {
                                int wrn = send(i, buff, strlen(buff), 0);
                                if (wrn < 0 && errno != EAGAIN) {
                                    status[i] = 0;
                                    close(i);
                                }
                            }
                            break;
                        }
                    }
                }
            }
            break;
        }
    }
#endif

    return 0;
}
