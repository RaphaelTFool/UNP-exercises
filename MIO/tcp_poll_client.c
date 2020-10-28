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
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define ERR_PRINT(fmt, ...) \
        printf("[%-6.5s:%6.5d]"fmt"\n", __func__, __LINE__, ##__VA_ARGS__)

int tcp_connect(const char *ip, unsigned short port)
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

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ERR_PRINT("socket connect error: %s", strerror(errno));
        return -1;
    }

    return sockfd;
}

int tcp_send(int sockfd, char *buff, int bufflen)
{
    char send_buff[1024];
    int send_ttlen = 0;
    int send_len = 0;

    memset(send_buff, 0x0, sizeof(send_buff));
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
        if (recv_len <= 0)
        {
            ERR_PRINT("@@ failed to receive message");
            return -1;
        }
        ERR_PRINT("buff: %s", buff);
    }

    return 0;
}

void usage(char *name)
{
    printf("Usage: %s <-s ip address> <-p port>\n", name);
}

int main(int argc, char *argv[])
{
    int sockfd;
    int opt;
    int maxfd;
    char s_ip[INET_ADDRSTRLEN];
    unsigned short s_port;
    int send_times = 0;

    int time = 3;

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
            time = (unsigned short)atoi(optarg);
            if (0 == time)
                time = 3;
            break;

            default:
            fprintf(stderr, "unknown option\n");
            usage(argv[0]);
            exit(EXIT_FAILURE);
            break;
        }
    }

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

    if (-1 == (sockfd = tcp_connect(s_ip, s_port)))
    {
        ERR_PRINT("connect server error");
        exit(EXIT_FAILURE);
    }
    maxfd = sockfd + 1;

    while (1)
    {
        char buff[1024];
        fd_set nreads;
        struct timeval timeout;

        memset(buff, 0x0, sizeof(buff));
        snprintf(buff, sizeof(buff), "1:hello server, my pid is %d.", getpid());
        /* 第一次send */
        int send_len = tcp_send(sockfd, buff, strlen(buff));
        if (0 == send_len)
        {
            send_times++;
            if (send_times >= 3)
            {
                #if 0
                /* 尝试sleep打断两次send */
                usleep(5);
                /* 依然会随机粘包 */
                #endif
                ERR_PRINT("### send tcp heartbeat probe ###");
                buff[0] = '0';
                buff[1] = '\0';
                /* 第二次send */
                /* 连续的两次send导致的tcp流上的粘包，从而让交互逻辑出现了偏差 */
                send_len = tcp_send(sockfd, buff, 1);
                if (send_len == 0)
                {
                    ERR_PRINT("heartbeat send");
                    timeout.tv_sec = 10;
                    timeout.tv_usec = 0;

                    FD_ZERO(&nreads);
                    FD_SET(sockfd, &nreads);
                    maxfd = sockfd + 1;

                    int sret = select(maxfd, &nreads, NULL, NULL, &timeout);
                    switch (sret)
                    {
                        case -1:
                        ERR_PRINT("select error: %s, exit ...", strerror(errno));
                        close(sockfd);
                        exit(EXIT_FAILURE);
                        break;

                        case 0:
                        ERR_PRINT("select timeout, exit ...");
                        close(sockfd);
                        exit(EXIT_FAILURE);
                        break;

                        default:
                        if (FD_ISSET(sockfd, &nreads))
                        {
                            memset(buff, 0x0, sizeof(buff));
                            int buff_tlen = sizeof(buff);
                            int recv_len = tcp_recv(sockfd, buff, &buff_tlen);
                            if (recv_len < 0)
                            {
                                ERR_PRINT("tcp_recv error: %s", strerror(errno));
                                close(sockfd);
                                exit(EXIT_FAILURE);
                            }
                            else
                            {
                                if (buff[0] == '$')
                                {
                                    ERR_PRINT("### tcp connection OK ###");
                                    send_times = 0;
                                }
                                else
                                {
                                    ERR_PRINT("unknown error");
                                    close(sockfd);
                                    exit(EXIT_FAILURE);
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
        //sleep(time);
    }

    close(sockfd);
    return 0;
}
