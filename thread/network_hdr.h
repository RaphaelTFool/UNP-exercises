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
#include <pthread.h>

pthread_mutex_t debug_mutex = PTHREAD_MUTEX_INITIALIZER;

/* 时间打印函数 */
void print_time(void) {
#ifdef _DEBUG_TIME
    struct timeval etv;
    struct tm etm;
    gettimeofday(&etv, NULL);
    localtime_r(&etv.tv_sec, &etm);
    char ms[7];
    snprintf(ms, sizeof(ms), "%lu", etv.tv_usec);
    for (int i = strlen(ms); i < 6; i++) ms[i] = '0';
    ms[6] = 0;
    printf("[%04d-%02d-%02d %02d:%02d:%02d.%s]",
           etm.tm_year + 1900, etm.tm_mon + 1, etm.tm_mday,
           etm.tm_hour, etm.tm_min, etm.tm_sec, ms);
#endif
}

#define ERR_PRINT(fmt, args...) \
do \
{ \
    pthread_mutex_lock(&debug_mutex); \
    print_time(); \
    printf("[%-6.5s:%6.5d]" fmt "\n", __func__, __LINE__, ##args); \
    pthread_mutex_unlock(&debug_mutex); \
} while (0)

#define handle_errno(err) \
do { \
    ERR_PRINT("\"%s\" error: %s", __func__, strerror(err)); \
    exit(EXIT_FAILURE); \
} while (0)

/*===================================================*/
/*===================================================*/
/*===================================================*/

/* 内存分配函数封装 */
void* Malloc(size_t nbytes) {
    void* ret = malloc(nbytes);
    if (!ret)
        handle_errno(errno);
    return ret;
}

void* Calloc(size_t nmemb, size_t nbytes) {
    void* ret = calloc(nmemb, nbytes);
    if (!ret)
        handle_errno(errno);
    return ret;
}

void *Realloc(void* ptr, size_t nbytes) {
    void* ret = realloc(ptr, nbytes);
    if (!ret)
        handle_errno(errno);
    return ret;
}

/*===================================================*/
/*===================================================*/
/*===================================================*/

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

int tcp_server_listen1(const char* ip, unsigned short port) {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        ERR_PRINT("socket created error: %s", strerror(errno));
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

/*===================================================*/
/*===================================================*/
/*===================================================*/

/* Pthread Functions */
void Pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine) (void *), void *arg) {
    int s = pthread_create(thread, attr, start_routine, arg);
    if (s != 0)
        handle_errno(s);
}

void Pthread_join(pthread_t thread_id, void **res) {
    int s = pthread_join(thread_id, res);
    if (s != 0)
        handle_errno(s);
}

void Pthread_cancel(pthread_t thread_id) {
    int s = pthread_cancel(thread_id);
    if (s != 0)
        handle_errno(s);
}

void Pthread_detach(pthread_t thread_id) {
    int s = pthread_detach(thread_id);
    if (s != 0)
        handle_errno(s);
}

/* Pthread Attribute */
void Pthread_attr_init(pthread_attr_t *attr) {
    int s = pthread_attr_init(attr);
    if (s != 0)
        handle_errno(s);
}

void Pthread_attr_destroy(pthread_attr_t *attr) {
    int s = pthread_attr_destroy(attr);
    if (s != 0)
        handle_errno(s);
}

void Pthread_attr_setstacksize(pthread_attr_t *attr, size_t stacksize) {
    int s = pthread_attr_setstacksize(attr, stacksize);
    if (s != 0)
        handle_errno(s);
}

void Pthread_attr_setdetachstate(pthread_attr_t *attr, int detachstate) {
    int s = pthread_attr_setdetachstate(attr, detachstate);
    if (s != 0)
        handle_errno(s);
}

/*===================================================*/
/*===================================================*/
/*===================================================*/

/* pthread mutex */
void Pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr) {
    int s = pthread_mutex_init(mutex, attr);
    if (s != 0)
        handle_errno(s);
}

void Pthread_mutex_destroy(pthread_mutex_t* mutex) {
    int s = pthread_mutex_destroy(mutex);
    if (s != 0)
        handle_errno(s);
}

void Pthread_mutex_lock(pthread_mutex_t* mutex) {
    int s = pthread_mutex_lock(mutex);
    if(s != 0)
        handle_errno(s);
}

void Pthread_mutex_unlock(pthread_mutex_t* mutex) {
    int s = pthread_mutex_unlock(mutex);
    if (s != 0)
        handle_errno(s);
}

/* pthread cond */
void Pthread_cond_init(pthread_cond_t* cond, const pthread_condattr_t* attr) {
    int s = pthread_cond_init(cond, attr);
    if (s != 0)
        handle_errno(s);
}

void Pthread_cond_destroy(pthread_cond_t* cond) {
    int s = pthread_cond_destroy(cond);
    if (s != 0)
        handle_errno(s);
}

void Pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex) {
    int s = pthread_cond_wait(cond, mutex);
    if (s != 0)
        handle_errno(s);
}

void Pthread_cond_signal(pthread_cond_t* cond) {
    int s = pthread_cond_signal(cond);
    if (s != 0)
        handle_errno(s);
}

#endif
