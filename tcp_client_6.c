/*************************************************************************
**    > File Name: tcp_client_6.c
**    > Author: Ral
**    > Mail: 
**    > Created Time: 
*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAXBUF 64000

#define OK       0
#define EINPUT  -1
#define ECALL   -2
#define EOTH    -3

#define ERR_PRINT(fmt) \
    printf("\n%s call failed:\nerror_code:\t%d,\nerror_msg:\t%s.\n", fmt, errno,strerror(errno))

enum
{
    SEND = 0,
    RECV,
    OPT
};
int tcp_communicate(int fd, char *buf, int buflen, int op)
{
    int length = 0;

    if (fd <= 0 || NULL == buf || buflen <= 0 || op >= OPT)
    {
        printf("Input parameter Error\n");
        return EINPUT;
    }

    if (SEND == op)
    {
        if ((length = send(fd, buf, buflen, 0)) < 0)
        {
            printf("\nmessage send failed\n");
            ERR_PRINT("send");
            printf("\nIGNORE this failure, continue next connection\n\n");
            return ECALL;
        }
        else
        {
            printf("message \"%s\" SEND successfully, %d bytes in total\n\n", buf, length);
            return OK;
        }
    }
    else if (RECV == op)
    {
        if ((length = recv(fd, buf, buflen, 0)) < 0)
        {
            printf("\nmessage recv failed\n");
            ERR_PRINT("recv");
            //goto failure
            printf("\nIGNORE this failure, continue next connection\n\n");
            return ECALL;
        }
        else
        {
            printf("message \"%s\" RECV successfully, %d bytes in total\n\n", buf, length);
            return OK;
        }
    }
    else
    {
        printf("\nunknown option\n");
        return EOTH;
    }

    return length;
}

int connect_server_communicate(char *ip, short port, int num)
{
    int sockfd;
    struct sockaddr_in6 server_addr;
    char buf[MAXBUF + 1];
    int ret;
    int i = 0;

    if (NULL == ip || port <= 0 || num < 0)
    {
        printf("Input parameter Error\n");
        ret = EINPUT;
        goto failure;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
    if (inet_pton(AF_INET6, ip, &server_addr.sin6_addr) <= 0)
    {
        ERR_PRINT("inet_pton");
        ret = ECALL;
        goto failure;
    }

    do
    {
        i++;
        printf("\nNO.%d request started from client\n", i);

        if ((sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        {
            ERR_PRINT("socket");
            ret = ECALL;
            //goto failure;
            printf("\nIGNORE this failure, continue next connection\n\n");
            continue;
        }

        if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr_in6)) < 0)
        {
            ERR_PRINT("connect");
            ret = ECALL;
            //goto failure;
            printf("\nIGNORE this failure, continue next connection\n\n");
            close(sockfd);
            continue;
        }

        bzero(buf, MAXBUF + 1);
        if (tcp_communicate(sockfd, buf, MAXBUF, RECV) < 0)
        {
            close(sockfd);
            continue;
        }

        bzero(buf, MAXBUF + 1);
        snprintf(buf, MAXBUF, "%d", i);
        if (tcp_communicate(sockfd, buf, strlen(buf), SEND) < 0)
        {
            close(sockfd);
            continue;
        }

        bzero(buf, MAXBUF + 1);
        if (tcp_communicate(sockfd, buf, MAXBUF, RECV) < 0)
        {
            close(sockfd);
            continue;
        }

        close(sockfd);
    } while(i < num);

    ret = OK;

failure:
    if (sockfd > 0)
        close(sockfd);
    return ret;
}

int main(int argc, char **argv)
{
    int port;
    int num;
    char buf[128];
    char *ip;

    if (argc != 4)
    {
        printf("Usage %s ip6 port connectTimes\n", argv[0]);
        exit(1);
    }
    else
    {
        strcpy(buf, argv[1]);
        port = atoi(argv[2]);
        num = atoi(argv[3]);
        if (strlen(buf) < 3)
            exit(2);
        else
            ip = buf;

        if (connect_server_communicate(ip, port, num) < 0)
        {
            printf("connect server failed\n");
        }
        else
        {
            printf("tcp succ.\n");
        }
    }

    return 0;
}

