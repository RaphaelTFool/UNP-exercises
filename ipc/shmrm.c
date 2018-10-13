/*************************************************************************
	> File Name: shmrm.c
	> Author: 
	> Mail: 
	> Created Time: Fri 12 Oct 2018 11:59:49 PM EDT
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
    int id;
    int token = 't'<<24|'e'<<16|'s'<<8|'t';
    int length = 0;
    int ret;

    if (argc > 3 || argc < 2)
    {
        printf("Usage: shmrmid <pathname> [length#]\n");
        exit(EXIT_FAILURE);
    }
    else if (argc == 3)
    {
        length = atoi(argv[2]);
    }

    printf("ket_t = (hex)0x%x\n", ftok(argv[1], token));
    id = shmget(ftok(argv[1], token), length, SVSHM_MODE);
    printf("id = (dec)%d, (hex)0x%x\n", id, id);
    if (-1 == id)
    {
        DBG_PRINT("shmget error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    ret = shmctl(id, IPC_RMID, NULL);
    if (-1 == ret)
    {
        DBG_PRINT("shmctl error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
