/*************************************************************************
	> File Name: Mthreads.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Thu 29 Oct 2020 04:55:40 PM CST
 ************************************************************************/

#include <stdio.h>
#include "network_hdr.h"

void* worker(void *arg) {
    int *count = (int*)arg;
    for (int i = 0; i < 2000000; i++) {
        (*count)++;
    }
    return count;
}

int main() {
    pthread_t tid[5];
    pthread_attr_t attr;
    int count = 0;

    Pthread_attr_init(&attr);
    //zesize_t size;
    //zepthread_attr_getstacksize(&attr, &size);
    //zeprintf("default size: %lu\n", size);
    //8388608
    Pthread_attr_setstacksize(&attr, 20480);
    for (int i = 0; i < 5; i++)
        Pthread_create(&tid[i], &attr, worker, &count);
    Pthread_attr_destroy(&attr);

    int *res = NULL;
    for (int i = 0; i < 5; i++) {
        Pthread_join(tid[i], (void **)&res);
        printf("thread %ld get result %d\n", tid[i], *res);
    }

    return 0;
}
