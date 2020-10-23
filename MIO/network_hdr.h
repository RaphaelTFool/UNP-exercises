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

#endif
