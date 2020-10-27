/*************************************************************************
	> File Name: tcp_linger_client.c
	> Author: RaphaelTFool
	> Mail: 1207975808@qq.com
	> Created Time: Tue 27 Oct 2020 02:55:24 PM CST
 ************************************************************************/

#include <stdio.h>
#include "network_hdr.h"

int main() {
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket:");
        exit(EXIT_FAILURE);
    }

    //(errno == EINPROGRESS)
    //make_noblocking(fd);

    struct sockaddr_in serv;
    serv.sin_family = AF_INET;
    serv.sin_port = htons(11111);
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr);

    if (connect(fd, (struct sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect:");
        exit(EXIT_FAILURE);
    }

    struct linger opt;
    opt.l_onoff = 1;
    opt.l_linger = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &opt, sizeof(opt)) < 0) {
        perror("setsockopt:");
        exit(EXIT_FAILURE);
    }

    close(fd);

    return 0;
}
