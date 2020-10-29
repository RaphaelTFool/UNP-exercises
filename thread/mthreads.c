/*************************************************************************
	> File Name: mthreads.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Thu 29 Oct 2020 03:12:31 PM CST
 ************************************************************************/

#include <stdio.h>
#include "network_hdr.h"

void* worker(void *args) {
    int* count = (int*)args;
    for (int i = 0; i < 200000000; i++) {
        (*count)++;
        //ERR_PRINT("thread %ld: count = %d", pthread_self(), *count);
    }
    return 0;
}

int main() {
    int count = 0;
    pthread_t tid1, tid2; 
    int error;

    //注意pthread_create成功返回0,异常返回错误码
    if ((error = pthread_create(&tid1, 0, worker, &count)) != 0) {
        ERR_PRINT("pthread_create failed: %s", strerror(error));
        exit(EXIT_FAILURE);
    }

    if ((error = pthread_create(&tid2, 0, worker, &count)) != 0) {
        ERR_PRINT("pthread_create failed: %s", strerror(error));
        exit(EXIT_FAILURE);
    }

    printf("count = %d\n", count);
    pthread_join(tid1, 0);
    pthread_join(tid2, 0);
    printf("count = %d\n", count);

    return 0;
}
