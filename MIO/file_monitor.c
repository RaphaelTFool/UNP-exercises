#include "network_hdr.h"

int running = 1;

void sig_handler(int signo) {
    running = 0;
}

int main(int argc, char **argv) {
    if (SIG_ERR == register_signal(SIGINT, 0, sig_handler)) {
        ERR_PRINT("register signal failed");
        exit(EXIT_FAILURE);
    }

    //注意：epoll不支持通常文件，但可以使用管道
    int fd = open("./testfile", O_CREAT|O_RDWR|O_TRUNC, 0644);
    if (fd < 0) {
        handle_errno(errno);
    }

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    if (epfd < 0) {
        handle_errno(errno);
    }

    struct epoll_event epev;
    epev.data.fd = fd;
    epev.events = EPOLLIN|EPOLLET;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &epev) < 0) {
        ERR_PRINT("epoll_ctl add file fd error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    epev.data.fd = STDIN_FILENO;
    epev.events = EPOLLIN|EPOLLET;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &epev) < 0) {
        ERR_PRINT("epoll_ctl add stdin error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct epoll_event revent[8];

    while (running) {
        int readyn = epoll_wait(epfd, revent, 8, -1);
        if (readyn < 0) {
            ERR_PRINT("epoll_wait error");
            handle_errno(errno);
        } else {
            for (int i = 0; i < readyn; i++) {
                if (revent[i].events & EPOLLERR ||
                    revent[i].events & EPOLLHUP ||
                    !(revent[i].events & EPOLLIN)) {
                        ERR_PRINT("error events for fd: %d", revent[i].data.fd);
                        continue;
                    } else {
                        if (fd == revent[i].data.fd) {
                            char readbuf[4096];
                            memset(readbuf, 0, sizeof(readbuf));
                            ERR_PRINT("file get changed");
                            if (lseek(fd, 0, SEEK_SET) < 0) {
                                ERR_PRINT("set cur_pos to 0 failed");
                            }
                            int rn = read(fd, readbuf, sizeof(readbuf));
                            if (rn < 0) {
                                ERR_PRINT("read failed: %s", strerror(errno));
                            }
                            printf("get message: %s", readbuf);
                        } else if (STDIN_FILENO == revent[i].data.fd) {
                            char stdbuf[4096];
                            if (!fgets(stdbuf, sizeof(stdbuf), stdin)) {
                                ERR_PRINT("fgets get wrong: %s", strerror(errno));
                            }
                            int wn = write(fd, stdbuf, strlen(stdbuf));
                            if (wn < 0) {
                                ERR_PRINT("write failed: %s", strerror(errno));
                            }
                        }
                    }
            }
        }
    }
#if 0
    char buff[] = "hello";
    int wn = write(fd, buff, strlen(buff));
    if (wn < 0) {
        handle_errno(errno);
    }

    off_t ot = lseek(fd, 0, SEEK_SET);
    if (ot < 0) {
        handle_errno(errno);
    }

    char readbuff[1024];
    memset(readbuff, 0, sizeof(readbuff));
    int rn = read(fd, readbuff, sizeof(readbuff));
    if (rn < 0) {
        handle_errno(errno);
    }
    ERR_PRINT("read %d bytes", rn);

    readbuff[rn] = '\0';
    ERR_PRINT("get message: %s", readbuff);
#endif

    close(fd);
    return 0;
}
