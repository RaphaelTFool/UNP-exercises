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

#define ERR_PRINT(fmt, ...) \
        printf("[%-6.5s:%6.5d]"fmt"\n", __func__, __LINE__, ##__VA_ARGS__)

int tcp_connect(const char *ip, unsigned short port)
{
    int sockfd, ret;
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

    if (argc < 5 || argc > 7)
    {
        fprintf(stderr, "command line option and argument error\n\n");
        usage(argv[0]);
        exit(EXIT_FAILURE);
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

    if (-1 == (sockfd = tcp_connect(s_ip, s_port)))
    {
        ERR_PRINT("connect server error");
        exit(EXIT_FAILURE);
    }

    while (1)
    {
        char buff[1024];
        snprintf(buff, sizeof(buff), "hello server, my pid is %d\n", getpid());
        int send_len = send(sockfd, buff, strlen(buff), 0);
        printf("send_len: %d\n", send_len);
        sleep(time);
    }

    close(listenfd);

    return 0;
}
