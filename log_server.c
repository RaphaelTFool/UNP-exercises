/*************************************************************************
**    > File Name: log_server.c
**    > Author: liuxiang
**    > Mail: lxiangb@ankki.com 
**    > Created Time: 2018??04??28?? ?????? 09ʱ40??29??
*************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <stdarg.h>
#include <string.h>

#define SERVER_IP "172.24.1.152"
#define SERVER_PORT 8888
#define BUF_SIZE 1024

int create_server_and_recv(unsigned short port, char *recv_buf, int len)
{
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    int server_sock_fd;
    socklen_t client_sock_len = sizeof(client_addr);
    int recv_len;

    if (NULL == recv_buf || len <= 0)
    {
        printf("input para error\n");
        return -1;
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if ((server_sock_fd = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Create socket failed:");
        exit(1);
    }

    if ((bind(server_sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr))) < 0)
    {
        perror("Server bind failed:");
        exit(1);
    }

    do
    {
        bzero(recv_buf, len);
        if ((recv_len = recvfrom(server_sock_fd, recv_buf, len, 0, (struct sockaddr *)&client_addr, &client_sock_len)) < 0)
        {
            perror("Receive data failed:");
            exit(1);
        }
        else
        {
            recv_buf[len - 1] = '\0';
            printf("Receive data length: %d\n", recv_len);
            printf("content: %s\n", recv_buf);
        }
    } while(1);

    close(server_sock_fd);
    return 0;
}

int main(int argc, char **argv)
{
    char buf[BUF_SIZE];

    create_server_and_recv(SERVER_PORT, buf, BUF_SIZE);

    return 0;
}

