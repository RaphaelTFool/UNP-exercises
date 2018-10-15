/*************************************************************************
	> File Name: shm_write.c
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

#include "shm_util.h"

struct shm_msg
{
    int type;
    int length;
    unsigned char data[0];
};

int main(int argc, char *argv[])
{
    int id;
    int i, count;
    struct shm_msg *msg = NULL;
    void *shmptr = NULL;

    if (2 != argc)
    {
        printf("Usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    id = shm_attach(argv[1]);
    DBG_PRINT("id = (dec)%d, (hex)0x%08x", id, id);
    if (ERR_ERROR == id)
    {
        DBG_PRINT("failed to attach share memory");
        return ERR_ERROR;
    }
    if (shm_read(id, &shmptr) < sizeof(struct shm_msg))
    {
        DBG_PRINT("data length is bigger than share memory size");
        goto cleanup;
    }
    msg = (struct shm_msg *)shmptr;

    if (msg->type != MAGIC_TOKEN)
    {
        DBG_PRINT("share message type wrong");
        goto cleanup;
    }
    count = (msg->length - sizeof(struct shm_msg))/sizeof(unsigned char);
    for (i = 0; i < count; i++)
    {
        if (0 != i && i % 3 == 0)
            printf("\n");
        printf("data[%05d] = %03d, ", i, msg->data[i]);
    }
    printf("\n");
    shm_detach(id);

    return ERR_SUCCESS;

cleanup:
    shm_detach(id);
    return ERR_ERROR;
}
