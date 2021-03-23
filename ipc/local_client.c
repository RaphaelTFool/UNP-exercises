#include "network_hdr.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        ERR_PRINT("usage: %s <local path>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        handle_errno(errno);
    }

    struct sockaddr_un servaddr, cliaddr;
    bzero(&cliaddr, sizeof(cliaddr));
    cliaddr.sun_family = AF_LOCAL;
    strcpy(cliaddr.sun_path, argv[1]);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, argv[1]);

    if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        handle_errno(errno);
    }

    while (1) {
        char send_buf[4096];
        printf("请输入字符串:\n");
        if (fgets(send_buf, sizeof(send_buf), stdin) == NULL) {
            handle_errno(errno);
        }

        if (send(fd, send_buf, strlen(send_buf), 0) < 0) {
            handle_errno(errno);
        }

        char recv_buf[4096];
        bzero(recv_buf, sizeof(recv_buf));
        int rbytes = recv(fd, recv_buf, sizeof(recv_buf), 0);
        if (rbytes < 0) {
            handle_errno(errno);
        } else if (rbytes == 0) {
            printf("server quit\n");
            break;
        }
        printf("receive data: %s\n", recv_buf);
    }

    close(fd);
    return 0;
}