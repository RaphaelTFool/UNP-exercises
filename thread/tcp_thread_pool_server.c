/*************************************************************************
	> File Name: tcp_thread_pool_server.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Mon 02 Nov 2020 06:18:19 PM CST
 ************************************************************************/

#include <stdio.h>
#include "network_hdr.h"

typedef struct {
    int maxfd;
    int *fd;
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} block_queue;

