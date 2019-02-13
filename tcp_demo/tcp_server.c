/*************************************************************************
	> File Name: tcp_server.c
	> Author: 
	> Mail: 
	> Created Time: Wed 13 Feb 2019 02:06:34 PM CST
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/select.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<time.h>
#include<signal.h>

#define ERR_PRINT(fmt, ...) \
        printf("[%-6.5s:%6.5d]"fmt"\n", __func__, __LINE__, ##__VA_ARGS__)

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

    ERR_PRINT("write_len = %d", write_len);

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
    char s_ip[INET_ADDRSTRLEN];
    unsigned short s_port;
    pid_t child;

    int i = 0;

    while (-1 != (opt = getopt(argc, argv, "hp:s:")))
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
    if (argc != 5)
    {
        fprintf(stderr, "command line option and argument error\n\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (-1 == (sockfd = tcp_server(s_ip, s_port)))
    {
        ERR_PRINT("create server error");
        exit(EXIT_FAILURE);
    }

    if (-1 == tcp_listen(sockfd, 5))
    {
        ERR_PRINT("listen sockfd error");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        memset(&client, 0x0, sizeof(client));
        if (-1 == (connfd = tcp_accept(sockfd, (struct sockaddr *)&client, &addrlen)))
        {
            ERR_PRINT("socket accept error");
        }
        else
        {
            child = fork();
            switch (child)
            {
                case -1:
                ERR_PRINT("fork error: %s", strerror(errno));
                break;

                case 0:
                close(sockfd);
                char buff[1024];
                char msg_buff[1024];
                char filename[1024];
                int recv_len;

                snprintf(filename, sizeof(filename), "child.%d", getpid());

                while (1)
                {
                    memset(buff, 0x0, sizeof(buff));
                    recv_len = recv(connfd, buff, sizeof(buff), 0);
                    if (recv_len > 0)
                    {
                        snprintf(msg_buff, sizeof(msg_buff), "mypid: %d, recv len: %d, recv msg: %s\n",
                              getpid(), recv_len, buff);
                        children_write_msg(filename, msg_buff, strlen(msg_buff));
                    }
                    else if (recv_len == 0)
                    {
                        break;
                    }
                }
                close(connfd);
                exit(EXIT_FAILURE);
                break;

                default:
                printf("father pid: %d, wait again\n", getpid());
                break;
            }
        }
        sleep(1);
    }

    close(listenfd);

    return 0;
}
