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
    int length;

    if (3 != argc)
    {
        printf("Usage: %s <pathname> <data_count#>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    count = atoi(argv[2]);
    length = sizeof(struct shm_msg) + count * sizeof(unsigned char);

    id = shm_attach(argv[1]);
    DBG_PRINT("id = (dec)%d, (hex)0x%08x", id, id);
    if (ERR_ERROR == id)
    {
        DBG_PRINT("failed to create share memory");
        return ERR_ERROR;
    }
    if (length > shm_read(id, &shmptr))
    {
        DBG_PRINT("data length is bigger than share memory size");
        goto cleanup;
    }

    msg = (struct shm_msg *)malloc(length);
    if (NULL == msg)
    {
        DBG_PRINT("failed to malloc: %s", strerror(errno));
        goto cleanup;
    }
    msg->type = MAGIC_TOKEN;
    msg->length = length;
    for (i = 0; i < count; i++)
    {
        msg->data[i] = i % 256;
    }
    memcpy(shmptr, msg, length);
    DBG_PRINT("write finished");

    shm_detach(id);
    free(msg);

    return ERR_SUCCESS;

cleanup:
    shm_detach(id);
    return ERR_ERROR;
}
