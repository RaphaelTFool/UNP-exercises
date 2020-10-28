/*************************************************************************
	> File Name: tcp_epoll_server.c
	> Author: 
	> Mail: 
	> Created Time: Wed 13 Feb 2019 02:06:34 PM CST
 ************************************************************************/

#include <stdio.h>
#include <getopt.h>
#include <assert.h>
#include "network_hdr.h"

static int running = 1;

void sig_handler(int signo) {
    assert(signo != 0);
    running = 0;
}

int tcp_accept(int listenfd, struct sockaddr *addr, socklen_t *addrlen) {
    int connfd;

    connfd = accept(listenfd, addr, addrlen);
    if (connfd < 0) {
        ERR_PRINT("socket accept error: %s", strerror(errno));
        return -1;
    }

    return connfd;
}

int tcp_send(int sockfd, char *buff, int bufflen) {
    char send_buff[1024];
    int send_len = 0;
    int send_ttlen = 0;

    snprintf(send_buff, sizeof(send_buff), "%05d%s", bufflen, buff);
    send_ttlen = strlen(send_buff);
    ERR_PRINT("send_bufflen: %d, send_buff: %s", send_ttlen, send_buff);

    char *sendp = send_buff;
    int already_send = 0;
    while (send_ttlen > 0) {
        send_len = send(sockfd, sendp, send_ttlen, 0);
        switch (send_len) {
            case -1:
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            else {
                ERR_PRINT("send msg error: %s", strerror(errno));
                return -1;
            }
            break;

            case 0:
            ERR_PRINT("send error: %s", strerror(errno));
            return -1;
            break;

            default:
            already_send += send_len;
            sendp += send_len;
            send_ttlen -= send_len;
            break;
        }
    }

    ERR_PRINT("send len: %d", already_send);
    return 0;
}

int tcp_recv(int sockfd, char *buff, int *buff_len) {
    char len_str[6];
    int recv_len;
    int msg_len;

    if (NULL == buff || *buff_len <= 0) {
        ERR_PRINT("input para invalid");
        return -1;
    }

    memset(len_str, 0x0, sizeof(len_str));
    recv_len = recv(sockfd, len_str, sizeof(len_str) - 1, MSG_WAITALL);
    if (5 != recv_len) {
        ERR_PRINT("@@ failed to get msg len, recv_len = %d", recv_len);
        return -1;
    } else {
        ERR_PRINT("len_str: %s", len_str);
        msg_len = atoi(len_str);
        ERR_PRINT("practical msg length: %d", msg_len);
        /* 如果消息格式不规范，这里会导致下一个消息的解析出错 */
        *buff_len = (msg_len > 0 && msg_len < *buff_len - 1) ? msg_len: *buff_len - 1;
        ERR_PRINT("buff_len = %d", *buff_len);
        recv_len = recv(sockfd, buff, *buff_len, MSG_WAITALL);
        if (recv_len != *buff_len) {
            ERR_PRINT("@@ failed to receive message");
            return -1;
        }
        ERR_PRINT("buff: %s", buff);
    }

    return 0;
}

int children_write_msg(char *filename, char *msg, int msg_len) {
    FILE *fp;
    int write_len;
    time_t now;
    struct tm fnow;
    char time_str[1024];
    int time_len;

    fp = fopen(filename, "a+");
    if (NULL == fp) {
        ERR_PRINT("failed to open file \"%s\": %s", filename, strerror(errno));
        return -1;
    }
    memset(time_str, 0x0, sizeof(time_str));
    time(&now);
    localtime_r(&now, &fnow);
    snprintf(time_str, sizeof(time_str), "[%02d:%02d:%02d]", fnow.tm_hour, fnow.tm_min, fnow.tm_sec);
    time_len = strlen(time_str);
    write_len = fwrite(time_str, 1, time_len, fp);
    if (time_len != write_len) {
        return -1;
    }
    write_len = fwrite(msg, 1, msg_len, fp);
    if (msg_len != write_len) {
        ERR_PRINT("something error happens");
        return -1;
    }

    ERR_PRINT("childpid: %d: write_len = %d", getpid(),  write_len);

    fclose(fp);

    return write_len;
}

void usage(char *name) {
    printf("Usage: %s <-s ip address> <-p port>\n", name);
}

int main(int argc, char *argv[]) {
    int sockfd, connfd;
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    int opt;
    int timeout = DEFAULT_TIME_OUT;
    char s_ip[INET_ADDRSTRLEN];
    unsigned short s_port;

    int i = 0;

    while (-1 != (opt = getopt(argc, argv, "hp:s:t:"))) {
        switch (opt) {
            case 'h':
            usage(argv[0]);
            exit(EXIT_FAILURE);
            break;

            case 'p':
            s_port = (unsigned short)atoi(optarg);
            break;

            case 's':
            memset(s_ip, 0x0, sizeof(s_ip));
            strncpy(s_ip, optarg, sizeof(s_ip));
            break;

            case 't':
            timeout = atoi(optarg);
            if (timeout <= 0)
                timeout = DEFAULT_TIME_OUT;
            break;

            default:
            fprintf(stderr, "unknown option\n");
            usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

    children_write_msg("parent", "hello\n", 6);

    printf("optind = %d, argc = %d\n", optind, argc);
    for (i = 0; i < optind; i++) {
        printf("No.%d: %s\n", i, argv[i]);
    }
    for (i = optind; i < argc; i++) {
        printf("No.%d: %s\n", i, argv[i]);
    }
    if (argc < 5 || argc > 7) {
        fprintf(stderr, "command line option and argument error\n\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (SIG_ERR == register_signal(SIGCHLD, SA_NOCLDWAIT, SIG_IGN)) {
        exit(EXIT_FAILURE);
    }

    if (SIG_ERR == register_signal(SIGINT, 0, sig_handler)) {
        ERR_PRINT("register signal failed\n");
        exit(EXIT_FAILURE);
    }

    if (-1 == (sockfd = tcp_server_listen(s_ip, s_port)))
    {
        exit(EXIT_FAILURE);
    }

    int efd = epoll_create1(EPOLL_CLOEXEC);
    if (efd == -1) {
        ERR_PRINT("epoll_cleate1 failed: %s", strerror(errno));
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event event;
    event.data.fd = sockfd;
    event.events = EPOLLIN;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &event) < 0) {
        ERR_PRINT("add listenfd failed: %s", strerror(errno));
        close(sockfd);
        close(efd);
        exit(EXIT_FAILURE);
    }

#define MAX_EVENTS 102400
    struct epoll_event *events = (struct epoll_event*)calloc(MAX_EVENTS, sizeof(struct epoll_event));
    if (!events) {
        ERR_PRINT("calloc failed: %s", strerror(errno));
        close(sockfd);
        close(efd);
        exit(EXIT_FAILURE);
    }

    while (running)
    {
        static int cli_cnt = 0;
        int ready_number = epoll_wait(efd, events, MAX_EVENTS, -1);
        ERR_PRINT("epoll_wait wakeup, Get %d events", ready_number);
        if (ready_number < 0) {
                ERR_PRINT("epoll error: %s", strerror(errno));
        } else if (ready_number == 0) {
            ERR_PRINT("timeout");
        } else {
            for (int i = 0; i < ready_number; i++) {
                if ((events[i].events & EPOLLERR) ||
                    (events[i].events & EPOLLHUP) ||
                    (!(events[i].events & EPOLLIN))) {
                    ERR_PRINT("epoll error\n");
                    close(events[i].data.fd);
                    continue;
                } else if (events[i].data.fd == sockfd) {
                    ++cli_cnt;
                    memset(&client, 0x0, sizeof(client));
                    if (-1 == (connfd = tcp_accept(sockfd, (struct sockaddr *)&client, &addrlen))) {
                        ERR_PRINT("socket accept error");
                    } else {
                        char cli_ip[INET_ADDRSTRLEN];
                        memset(cli_ip, 0x0, sizeof(cli_ip));
                        ERR_PRINT("client info: %s:%u",
                                 inet_ntop(AF_INET, &client.sin_addr, cli_ip, sizeof(cli_ip)),
                                 ntohs(client.sin_port));
                        struct epoll_event event;
                        make_noblocking(connfd);
                        event.data.fd = connfd;
                        event.events = EPOLLIN;
                        if (epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &event) < 0) {
                            ERR_PRINT("epoll_ctl add connection fd failed: %s", strerror(errno));
                        }
                    }
                    continue;
                } else {
                    int clifd = events[i].data.fd;
                    char buff[1024];
                    char msg_buff[2048];
                    char filename[1024];
                    int recv_len;
                    int buff_tlen = sizeof(buff);
                    memset(buff, 0x0, sizeof(buff));
                    recv_len = tcp_recv(clifd, buff, &buff_tlen);
                    if (recv_len == 0)
                    {
                        ERR_PRINT("buff[0] = %c", buff[0]);
                        switch (buff[0])
                        {
                            case '1':
                            printf("#####\n");  
                            printf("buff: %s\n", buff);
                            printf("#####\n");  
                            snprintf(filename, sizeof(filename), "record.%d", getpid());
                            snprintf(msg_buff, sizeof(msg_buff), "NO.%d Client, msg len: %d, recv msg: %s\n",
                                  i, (int)strlen(buff), buff);
                            children_write_msg(filename, msg_buff, strlen(msg_buff));
                            //tcp_send(clifd, buff, strlen(buff));
                            break;

                            case '0':
                            ERR_PRINT("### receive a tcp heartbeat ###");
                            memset(buff, 0x0, sizeof(buff));
                            buff[0] = '$';
                            int send_len = tcp_send(clifd, buff, 1);
                            if (send_len < 0)
                            {
                                ERR_PRINT("send heartbeat ack error: %s", strerror(errno));
                            }
                            break;

                            default:
                            printf("#####\n");  
                            ERR_PRINT("unknown message: %s\n", buff);
                            tcp_send(clifd, buff, strlen(buff));
                            printf("#####\n");  
                            break;
                        }
                    } else if (recv_len < 0 || errno != EAGAIN) {
                        ERR_PRINT("some errors happens or client exits ...");
#if 0
                        //似乎自动被删除了
                        if (epoll_ctl(efd, EPOLL_CTL_DEL, clifd, &events[i]) < 0) {
                            ERR_PRINT("epoll_ctl delete event failed: %s\n", strerror(errno));
                        }
#endif
                        close(clifd);
                    }
                }
            }
        }
    }

    free(events);
    close(efd);

    return 0;
}
