/*************************************************************************
**    > File Name: tcp_server_6.c
**    > Author: Ral
**    > Mail: 
**    > Created Time: 
*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>

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

int create_server(char *ip, short port, int num)
{
    int listen_fd, conn_fd;
    socklen_t len;
    int length;
    //struct sockaddr_in6 server_addr, client_addr;
    struct sockaddr_in server_addr, client_addr;
    char buf[MAXBUF + 1];
    int ret;
    int count = 0;

    if (port <= 0 || num <= 0)
    {
        printf("input parameters error\n");
        ret = EINPUT;
        goto failure;
    }
    //if ((listen_fd = socket(PF_INET6, SOCK_STREAM, 0)) < 0)
    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        ERR_PRINT("socket");
        ret = ECALL;
        goto failure;
    }

    bzero(&server_addr, sizeof(server_addr));
    //server_addr.sin6_family = PF_INET6;
    server_addr.sin_family = PF_INET;
    //server_addr.sin6_port = htons(port);
    server_addr.sin_port = htons(port);
    if (NULL != ip)
    {
        //if (inet_pton(PF_INET6, ip, &server_addr.sin6_addr) <= 0)
        if (inet_pton(PF_INET, ip, &server_addr.sin_addr) <= 0)
        {
            ERR_PRINT("inet_pton");
            ret = ECALL;
            goto failure;
        }
    }
    else
    {
        //server_addr.sin6_addr = in6addr_any;
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ERR_PRINT("bind");
        ret = ECALL;
        goto failure;
    }

    if (listen(listen_fd, num) < 0)
    {
        ERR_PRINT("listen");
        ret = ECALL;
        goto failure;
    }

    while (1)
    {
        len = sizeof(struct sockaddr);
        if ((conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &len)) < 0)
        {
            ERR_PRINT("accept");
            ret = ECALL;
            //goto failure;
            printf("\nIGNORE this failure, continue next connection\n\n");
            continue;
        }
        else
        {
            ++count;
            printf("NO.%d connection COMES from client\n", count);
        }

        //if (NULL == inet_ntop(PF_INET6, &client_addr.sin6_addr, buf, sizeof(buf)))
        if (NULL == inet_ntop(PF_INET, &client_addr.sin_addr, buf, sizeof(buf)))
        {
            ERR_PRINT("inet_ntop");
            ret = ECALL;
            //goto failure;
            printf("\nIGNORE this failure, continue next connection\n\n");
            continue;
        }
        else
        {
            //printf("SERVER:\nGot a connection from %s, port %d, at socket %d\n",
            //        buf, client_addr.sin6_port, conn_fd);
            printf("SERVER:\nGot a connection from %s, port %d, at socket %d\n",
                    buf, client_addr.sin_port, conn_fd);
        }

        printf("\nFirst SEND:\n\n");
        bzero(buf, MAXBUF+1);
        snprintf(buf, MAXBUF, "\nThis is the FIRST message to send as client after accept,the event happens at conn_fd but not listen_fd, listen_fd is a listen fd which could not send or receive message\n");
        if (tcp_communicate(conn_fd, buf, strlen(buf), SEND) < 0)
        {
            close(conn_fd);
            continue;
        }

        printf("\nFirst RECV:\n\n");
        bzero(buf, MAXBUF + 1);
        if (tcp_communicate(conn_fd, buf, MAXBUF, RECV) < 0)
        {
            close(conn_fd);
            continue;
        }

        printf("\nParse request and Second SEND:\n\n");
        length = atoi(buf);
        if (length <= 0)
        {
            printf("receive command Error\n");
            ret = EOTH;
            length = 6;
            //goto failure;
        }
        memset(buf, 'B', length);
        if (tcp_communicate(conn_fd, buf, strlen(buf), SEND) < 0)
        {
            close(conn_fd);
            continue;
        }

        close(conn_fd);
    }
    close(listen_fd);

    return OK;

failure:
    if (listen_fd > 0)
        close(listen_fd);
    if (conn_fd > 0)
        close(conn_fd);

    return ret;
}

int main(int argc, char **argv)
{
    short port;
    int num;
    char buf[128];
    char *ip;

    if (argc != 4)
    {
        //printf("Usage: %s ip6 port listenq\n", argv[0]);
        printf("Usage: %s ip4 port listenq\n", argv[0]);
        exit(1);
    }
    else
    {
        strcpy(buf, argv[1]);
        port = atoi(argv[2]);
        num = atoi(argv[3]);
        if (strlen(buf) < 3)
            ip = NULL;
        else
            ip = buf;
        if (create_server(ip, port, num) < 0)
        {
            printf("server start failed\n");
        }
    }

    return 0;
}

