/*************************************************************************
	> File Name: shmread.c
	> Author: 
	> Mail: 
	> Created Time: Sat 13 Oct 2018 02:27:42 AM EDT
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

struct shm_data
{
    int type;
    int length;
    char data[0];
};

int main(int argc, char *argv[])
{
    int id;
    int token = 't'<<24|'e'<<16|'s'<<8|'t';
    int length = 0;
    int i, ret;
    struct shmid_ds buff;
    struct shm_data *shm_msg, *test;
    unsigned char *ptr;

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

    test = shmat(id, NULL, 0);
    if ((void *)-1 == test)
    {
        DBG_PRINT("shmat error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    shm_msg = shmat(id, NULL, 0);
    if ((void *)-1 == shm_msg)
    {
        DBG_PRINT("shmat error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    ret = shmctl(id, IPC_STAT, &buff);
    if (-1 == ret)
    {
        DBG_PRINT("shmctl error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    DBG_PRINT("size of share memory: %lu Bytes", buff.shm_segsz);
    if (length != 0)
    {
        shm_msg->length = length;
    }
    if (shm_msg->length > buff.shm_segsz - sizeof(struct shm_data))
    {
        DBG_PRINT("out of share memory");
        exit(EXIT_FAILURE);
    }
    if (shm_msg->type != token)
    {
        DBG_PRINT("msg type wrong");
        exit(EXIT_FAILURE);
    }
    ptr = (unsigned char *)shm_msg->data;
    for (i = 0; i < shm_msg->length; i++)
    {
        if (0 != i && i % 3 == 0)
            printf("\n");
        printf("data[%05d] = %03u, ", i, ptr[i]);
    }
    printf("\n");

    printf("shm_msg = %p, test = %p\n", shm_msg, test);

    ret = shmdt(shm_msg);
    if (-1 == ret)
    {
        DBG_PRINT("shmdt error: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    pause();

    return 0;
}
