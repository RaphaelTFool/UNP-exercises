#include "network_hdr.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        ERR_PRINT("Usage: %s <local path>", argv[0]);
        exit(EXIT_FAILURE);
    }

    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        handle_errno(errno);
    }

    char *local_path = argv[1];
    unlink(local_path);

    struct sockaddr_un cliaddr, servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sun_family = AF_LOCAL;
    strcpy(servaddr.sun_path, local_path);

    if (bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        handle_errno(errno);
    }

    if (listen(fd, 5) < 0) {
        handle_errno(errno);
    }

    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(fd, (struct sockaddr *)&cliaddr, &clilen);
    if (connfd < 0) {
        handle_errno(errno);
    }

    while (1) {
        char buf[4096];
        bzero(buf, sizeof(buf));
        if (read(connfd, buf, sizeof(buf)) == 0) {
            ERR_PRINT("client quit");
            break;
        }

        char send_line[4096];
        sprintf(send_line, "Hi, %s", buf);

        int nbytes = strlen(send_line);

        if (write(connfd, send_line, nbytes) != nbytes) {
            ERR_PRINT("write failed, %s", strerror(errno));
            break;
        }
    }

    close(fd);
    close(connfd);

    return 0;
}
