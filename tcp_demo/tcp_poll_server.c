/*************************************************************************
	> File Name: tcp_server.c
	> Author: 
	> Mail: 
	> Created Time: Wed 13 Feb 2019 02:06:34 PM CST
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>

#define DEFAULT_TIME_OUT 1800
#define ERR_PRINT(fmt, ...) \
        printf("[%-6.5s:%6.5d]"fmt"\n", __func__, __LINE__, ##__VA_ARGS__)

typedef void (*sigfunc_t)(int);

sigfunc_t register_signal(
    int signo,
    int flags,
    sigfunc_t sig_handler
    )
{
    struct sigaction act, oact;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, SIGINT);
    act.sa_flags = flags;
    act.sa_handler = sig_handler;

    if (-1 == sigaction(signo, &act, &oact))
    {
        ERR_PRINT("register signal %d error: %s",
                 signo, strerror(errno));
        return SIG_ERR;
    }

    return oact.sa_handler;
}

int tcp_server(const char *ip, unsigned short port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    if (NULL == ip)
    {
        ERR_PRINT("input para error");
        return -1;
    }

    memset(&server_addr, 0x0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        ERR_PRINT("ip addr error: %s", strerror(errno));
        return -1;
    }
    server_addr.sin_port = htons(port);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        ERR_PRINT("socket error: %s", strerror(errno));
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ERR_PRINT("socket bind error: %s", strerror(errno));
        return -1;
    }

    return sockfd;
}

int tcp_listen(int sockfd, int backlog)
{
    int ret;
    ret = listen(sockfd, backlog);
    if (ret < 0)
    {
        ERR_PRINT("listen error: %s", strerror(errno));
        return -1;
    }

    return ret;
}

int tcp_accept(int listenfd, struct sockaddr *addr, socklen_t *addrlen)
{
    int connfd;

    connfd = accept(listenfd, addr, addrlen);
    if (connfd < 0)
    {
        ERR_PRINT("socket accept error: %s", strerror(errno));
        return -1;
    }

    return connfd;
}

int tcp_send(int sockfd, char *buff, int bufflen)
{
    char send_buff[1024];
    int send_len = 0;
    int send_ttlen = 0;

    snprintf(send_buff, sizeof(send_buff), "%05d%s", bufflen, buff);
    send_ttlen = strlen(send_buff);
    ERR_PRINT("send_bufflen: %d, send_buff: %s", send_ttlen, send_buff);

    char *sendp = send_buff;
    int already_send = 0;
    while (send_ttlen > 0)
    {
        send_len = send(sockfd, sendp, send_ttlen, 0);
        switch (send_len)
        {
            case -1:
            if (errno == EINTR)
                continue;
            else
            {
                ERR_PRINT("send msg error");
                return -1;
            }
            break;

            case 0:
            ERR_PRINT("send error");
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

int tcp_recv(int sockfd, char *buff, int *buff_len)
{
    char len_str[6];
    int recv_len;
    int msg_len;

    if (NULL == buff || *buff_len <= 0)
    {
        ERR_PRINT("input para invalid");
        return -1;
    }

    memset(len_str, 0x0, sizeof(len_str));
    recv_len = recv(sockfd, len_str, sizeof(len_str) - 1, MSG_WAITALL);
    if (5 != recv_len)
    {
        ERR_PRINT("@@ failed to get msg len, recv_len = %d", recv_len);
        return -1;
    }
    else
    {
        ERR_PRINT("len_str: %s", len_str);
        msg_len = atoi(len_str);
        ERR_PRINT("practical msg length: %d", msg_len);
        /* 如果消息格式不规范，这里会导致下一个消息的解析出错 */
        *buff_len = (msg_len > 0 && msg_len < *buff_len - 1) ? msg_len: *buff_len - 1;
        ERR_PRINT("buff_len = %d", *buff_len);
        recv_len = recv(sockfd, buff, *buff_len, MSG_WAITALL);
        if (recv_len != *buff_len)
        {
            ERR_PRINT("@@ failed to receive message");
            return -1;
        }
        ERR_PRINT("buff: %s", buff);
    }

    return 0;
}

int children_write_msg(char *filename, char *msg, int msg_len)
{
    FILE *fp;
    int write_len;
    time_t now;
    struct tm fnow;
    char time_str[1024];
    int time_len;

    fp = fopen(filename, "a+");
    if (NULL == fp)
    {
        ERR_PRINT("failed to open file \"%s\": %s", filename, strerror(errno));
        return -1;
    }
    memset(time_str, 0x0, sizeof(time_str));
    time(&now);
    localtime_r(&now, &fnow);
    snprintf(time_str, sizeof(time_str), "[%02d:%02d:%02d]", fnow.tm_hour, fnow.tm_min, fnow.tm_sec);
    time_len = strlen(time_str);
    write_len = fwrite(time_str, 1, time_len, fp);
    if (time_len != write_len)
    {
        return -1;
    }
    write_len = fwrite(msg, 1, msg_len, fp);
    if (msg_len != write_len)
    {
        ERR_PRINT("something error happens");
        return -1;
    }

    ERR_PRINT("childpid: %d: write_len = %d", getpid(),  write_len);

    fclose(fp);

    return write_len;
}

void usage(char *name)
{
    printf("Usage: %s <-s ip address> <-p port>\n", name);
}

int main(int argc, char *argv[])
{
    int sockfd, listenfd, connfd;
    struct sockaddr_in client;
    socklen_t addrlen = sizeof(client);
    int opt;
    int timeout = DEFAULT_TIME_OUT;
    char s_ip[INET_ADDRSTRLEN];
    unsigned short s_port;

    int i = 0;

    while (-1 != (opt = getopt(argc, argv, "hp:s:t:")))
    {
        switch (opt)
        {
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
    for (i = 0; i < optind; i++)
    {
        printf("No.%d: %s\n", i, argv[i]);
    }
    for (i = optind; i < argc; i++)
    {
        printf("No.%d: %s\n", i, argv[i]);
    }
    if (argc < 5 || argc > 7)
    {
        fprintf(stderr, "command line option and argument error\n\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (SIG_ERR == register_signal(SIGCHLD, SA_NOCLDWAIT, SIG_IGN))
    {
        exit(EXIT_FAILURE);
    }

    if (-1 == (sockfd = tcp_server(s_ip, s_port)))
    {
        exit(EXIT_FAILURE);
    }

    if (-1 == tcp_listen(sockfd, 5))
    {
        close(sockfd);
        exit(EXIT_FAILURE);
    }

#define INIT_SIZE 1024
    struct pollfd event_set[INIT_SIZE];
    event_set[0].fd = sockfd;
    event_set[0].events = POLLRDNORM;

    for (int i = 1; i < INIT_SIZE; i++) {
        event_set[i].fd = -1;
    }

    while (1)
    {
        static int cli_cnt = 0;
        int ready_number = poll(event_set, INIT_SIZE, -1);
        ERR_PRINT("Get %d events", ready_number);
        if (ready_number < 0) {
                ERR_PRINT("poll error: %s", strerror(errno));
        } else if (ready_number == 0) {
            ERR_PRINT("timeout");
        } else {
            if (event_set[0].revents & POLLRDNORM) {
                ++cli_cnt;
                memset(&client, 0x0, sizeof(client));
                if (-1 == (connfd = tcp_accept(sockfd, (struct sockaddr *)&client, &addrlen)))
                {
                    ERR_PRINT("socket accept error");
                }
                else 
                {
                    char cli_ip[INET_ADDRSTRLEN];
                    memset(cli_ip, 0x0, sizeof(cli_ip));
                    ERR_PRINT("client info: %s:%u",
                             inet_ntop(AF_INET, &client.sin_addr, cli_ip, sizeof(cli_ip)),
                             ntohs(client.sin_port));
                    int i = 0;
                    for (; i < INIT_SIZE; i++)
                    {
                        if (event_set[i].fd < 0) {
                            event_set[i].fd = connfd;
                            event_set[i].events = POLLRDNORM;
                            break;
                        }
                    }
                    if (i == INIT_SIZE) {
                        ERR_PRINT("fd exausted");
                        close(connfd);
                    }
                }
                if (!--ready_number) {
                    continue;
                }
            }
            for (int i = 1; i < INIT_SIZE; i++)
            {
                if (event_set[i].fd < 0)
                {
                    continue;
                }
                if (event_set[i].revents & (POLLRDNORM | POLLERR)) 
                {
                    char buff[1024];
                    char msg_buff[1024];
                    char filename[1024];
                    int recv_len;
                    int buff_tlen = sizeof(buff);
                    memset(buff, 0x0, sizeof(buff));
                    recv_len = tcp_recv(event_set[i].fd, buff, &buff_tlen);
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
                            break;

                            case '0':
                            ERR_PRINT("### receive a tcp heartbeat ###");
                            memset(buff, 0x0, sizeof(buff));
                            buff[0] = '$';
                            int send_len = tcp_send(event_set[i].fd, buff, 1);
                            if (send_len < 0)
                            {
                                ERR_PRINT("send heartbeat ack error: %s", strerror(errno));
                            }
                            break;

                            default:
                            printf("#####\n");  
                            ERR_PRINT("unknown message: %s\n", buff);
                            printf("#####\n");  
                            break;
                        }
                    }
                    else if (recv_len < 0 || errno == ECONNRESET)
                    {
                        ERR_PRINT("some errors happens or client exits ...");
                        close(event_set[i].fd);
                        event_set[i].fd = -1;
                    }
                    if (!--ready_number) 
                    {
                        break;
                    }
                }
            }
        }
    }

    close(listenfd);

    return 0;
}
