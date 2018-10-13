/*************************************************************************
	> File Name: shmget.c
	> Author: 
	> Mail: 
	> Created Time: Fri 12 Oct 2018 11:10:57 PM EDT
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <errno.h>

#define DBG_PRINT(fmt, ...) \
        printf("[%-6.5s: %06d] "fmt"\n", __func__, __LINE__, ##__VA_ARGS__);

#define SVSHM_MODE (SHM_R | SHM_W | SHM_R>>3 | SHM_R>>6)

int main(int argc, char *argv[])
{
    int c, id, oflag;
    char *ptr;
    size_t length;
    oflag = SVSHM_MODE | IPC_CREAT;

    int token = 't'<<24|'e'<<16|'s'<<8|'t';

    printf("token = (dec)%d, (hex)0x%08x\n", token, token);

    while ((c = getopt(argc, argv, "e")) != -1)
    {
        switch (c)
        {
            case 'e':
            oflag |= IPC_EXCL;
            break;
        }
    }
    if (optind != argc - 2)
    {
        printf("Usage: shmget [-e] <pathname> <length#>");
        exit(EXIT_FAILURE);
    }

    length = atoi(argv[optind + 1]);
    id = shmget(ftok(argv[optind], token), length, oflag);
    printf("id = (dec)%d, (hex)0x%x\n", id, id);
    if (-1 == id)
    {
        DBG_PRINT("shmget error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    ptr = shmat(id, NULL, 0);
    if ((void *)-1 == ptr)
    {
        DBG_PRINT("shmat error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
