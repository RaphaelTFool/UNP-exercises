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
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/select.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#define DEFAULT_TIME_OUT 1800
#define ERR_PRINT(fmt, args...) \
do \
{ \
    struct timeval etv; \
    struct tm etm; \
    gettimeofday(&etv, NULL); \
    localtime_r(&etv.tv_sec, &etm); \
    char ms[7]; \
    snprintf(ms, sizeof(ms), "%lu", etv.tv_usec); \
    for (int i = strlen(ms); i < 6; i++) ms[i] = '0'; \
    ms[6] = 0; \
    printf("[%04d-%02d-%02d %02d:%02d:%02d.%s][%-6.5s:%6.5d]" fmt "\n", \
           etm.tm_year + 1900, etm.tm_mon + 1, etm.tm_mday, \
           etm.tm_hour, etm.tm_min, etm.tm_sec, ms, \
           __func__, __LINE__, ##args); \
} while (0)

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

void hexdump(const void* buf, size_t len) {
    printf("\nHexDump:\n");
    const uint8_t *data = (const uint8_t*)buf;
    for (size_t i = 0; i < len; i += 16) {
        printf("%08lx | ", i);
        for (int j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%.2x ", data[i + j]);
            } else {
                printf("   ");
            }
        }
        printf(" | ");
        for (int j = 0; j < 16; j++) {
            if (i + j < len) {
                printf("%c", isspace(data[i + j])? '.': data[i + j]);
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    printf("end\n\n");
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
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
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
        if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) {
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
