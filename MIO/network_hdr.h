/*************************************************************************
	> File Name: network_hdr.h
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Fri 23 Oct 2020 04:15:16 PM CST
 ************************************************************************/

#ifndef _NETWORK_HDR_H
#define _NETWORK_HDR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>

#define DEFAULT_TIME_OUT 1800
#define ERR_PRINT(fmt, args...) \
        printf("[%-6.5s:%6.5d]" fmt "\n", __func__, __LINE__, ##args)

typedef void (*sigfunc_t)(int);

sigfunc_t register_signal(int signo, int flags, sigfunc_t sig_handler) {
    struct sigaction act, oact;

    sigemptyset(&act.sa_mask);
    sigaddset(&act.sa_mask, signo);
    act.sa_flags = flags;
    act.sa_handler = sig_handler;

    if (-1 == sigaction(signo, &act, &oact)) {
        ERR_PRINT("register signal %d error: %s",
                 signo, strerror(errno));
        return SIG_ERR;
    }

    return oact.sa_handler;
}

int make_noblocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0) {
        ERR_PRINT("fcntl error: %s", strerror(errno));
        return -1;
    }
    return fd;
}

int tcp_server_listen(const char* ip, unsigned short port) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ERR_PRINT("socket created error: %s", strerror(errno));
        return -1;
    }

    if (make_noblocking(fd) < 0) {
        return -1;
    }

    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0) {
        ERR_PRINT("set socket option failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (ip) {
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr) <= 0) {
            ERR_PRINT("inet_pton failed: %s", strerror(errno));
            return -1;
        }
    } else {
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        ERR_PRINT("bind failed: %s", strerror(errno));
        return -1;
    }

    //listen函数并不返回套接字
    if (listen(fd, 512) < 0) {
        ERR_PRINT("listen failed: %s", strerror(errno));
        return -1;
    }

    return fd;
}

int tcp_client_connect(const char* ip, unsigned short port) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ERR_PRINT("socket created failed: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    if (ip) {
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr.s_addr) <= 0) {
            ERR_PRINT("inet_ntop failed: %s", strerror(errno));
            return -1;
        }
    } else {
        if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr) <= 0) {
            ERR_PRINT("inet_ntop failed: %s", strerror(errno));
            return -1;
        }
    }

    if (connect(fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        ERR_PRINT("connect failed: %s", strerror(errno));
        return -1;
    }

    int flag = fcntl(fd, F_GETFL, 0);
    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0) {
        ERR_PRINT("fcntl set noblock failed: %s", strerror(errno));
        return -1;
    }

    return fd;
}

//ssize_t writen(int fd, const void* data, size_t n) {
ssize_t sendn(int fd, const void* data, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    const char* ptr = (const char *)data;

    while (nleft > 0) {
        //nwritten = write(fd, ptr, nleft);
        nwritten = send(fd, ptr, nleft, 0);
        if (nwritten <= 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                ERR_PRINT("write error happens: %s", strerror(errno));
                //break;
                return -1;
            }
        } else {
            ptr += nwritten;
            nleft -= nwritten;
        }
    }
    return n - nleft;
}

#endif
