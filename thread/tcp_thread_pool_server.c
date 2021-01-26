/*************************************************************************
	> File Name: tcp_thread_pool_server.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Mon 02 Nov 2020 06:18:19 PM CST
 ************************************************************************/

#include <stdio.h>
#include "network_hdr.h"

#define QSIZE 2

int running = 1;

void sig_handler(int signo) {
    running = 0;
}

void echo_loop(int fd) {
    char buff[1024];
    while (running) {
        memset(buff, 0, sizeof(buff));
        int rcn = recv(fd, buff, sizeof(buff), 0);
        if (rcn < 0) {
            if (errno == EINTR || errno == EAGAIN) continue;
            ERR_PRINT("recv failed: %s", strerror(errno));
            close(fd);
            break;
        } else if (rcn == 0) {
            ERR_PRINT("peer close connection");
            close(fd);
            break;
        } else {
            ERR_PRINT("thread %ld get msg: %s", pthread_self(), buff);
            int sdn = send(fd, buff, strlen(buff), 0);
            if (sdn < 0) {
                if (errno == EINTR || errno == EAGAIN) continue;
                ERR_PRINT("send failed: %s", strerror(errno));
                close(fd);
                break;
            }
        }
    }
}

typedef struct {
    int maxfd;
    int *fd;
    int front;
    int rear;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} block_queue;

void block_queue_init(block_queue *blockQueue, int number) {
    blockQueue->maxfd = number;
    blockQueue->fd = calloc(number, sizeof(int));
    if (!blockQueue->fd) {
        handle_errno(errno);
    }
    blockQueue->front = blockQueue->rear = 0;
    Pthread_mutex_init(&blockQueue->mutex, NULL);
    Pthread_cond_init(&blockQueue->cond, NULL);
}

void block_queue_push(block_queue *blockQueue, int fd) {
    Pthread_mutex_lock(&blockQueue->mutex);
    blockQueue->fd[blockQueue->rear] = fd;
    if (++blockQueue->rear == blockQueue->maxfd) {
        blockQueue->rear = 0;
    }
    ERR_PRINT("push fd %d", fd);
    Pthread_cond_signal(&blockQueue->cond);
    Pthread_mutex_unlock(&blockQueue->mutex);
}

int block_queue_pop(block_queue *blockQueue) {
    Pthread_mutex_lock(&blockQueue->mutex);
    while (blockQueue->front == blockQueue->rear) {
        Pthread_cond_wait(&blockQueue->cond, &blockQueue->mutex);
    }
    int fd = blockQueue->fd[blockQueue->front];
    if (++blockQueue->front == blockQueue->maxfd) {
        blockQueue->front = 0;
    }
    printf("pop fd %d", fd);
    Pthread_mutex_unlock(&blockQueue->mutex);
    return fd;
}

void* thread_run(void *arg) {
    pthread_t tid = pthread_self();
    pthread_detach(tid);

    block_queue *blockQueue = (block_queue *) arg;
    while (running) {
        int fd = block_queue_pop(blockQueue);
        ERR_PRINT("get fd in thread, fd = %d, tid = %ld", fd, tid);
        echo_loop(fd);
    }

    return NULL;
}

int main(int argc, char **argv) {
    int listen_fd = tcp_server_listen1("127.0.0.1", 22333);
    block_queue blockQueue;
    block_queue_init(&blockQueue, QSIZE);

    register_signal(SIGINT, 0, sig_handler);

    pthread_t *thread_array = calloc(QSIZE, sizeof(pthread_t));
    if (!thread_array) {
        handle_errno(errno);
    }
    int i;
    for (i = 0; i < QSIZE; i++) {
        Pthread_create(thread_array + i, NULL, &thread_run, (void *)&blockQueue);
    }

    while (running) {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        int fd = accept(listen_fd, (struct sockaddr *) &ss, &slen);
        if (fd < 0) {
            handle_errno(errno);
        } else {
            block_queue_push(&blockQueue, fd);
        }
    }

    return 0;
}
