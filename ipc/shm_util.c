/*************************************************************************
	> File Name: shm_util.c
	> Author: 
	> Mail: 
	> Created Time: Sat 13 Oct 2018 03:11:53 AM EDT
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

int shm_create(const char *pathname, int length)
{
    int oflag = SVSHM_MODE | IPC_CREAT | IPC_EXCL;
    int id = -1;
    int token = MAGIC_TOKEN;
    void *shm_base = NULL;

    if (NULL == pathname || length <= 0)
    {
        DBG_PRINT("input para invalid");
        return ERR_ERROR;
    }

    id = shmget(ftok(pathname, token), length, oflag);
    if (-1 == id)
    {
        DBG_PRINT("shmget error: %s", strerror(errno));
        return ERR_ERROR;
    }
    shm_base = shmat(id, NULL, 0);
    if ((void *)-1 == shm_base)
    {
        DBG_PRINT("shmat error: %s", strerror(errno));
        return ERR_ERROR;
    }

    return id;
}

int shm_attach(const char *pathname)
{
    int id;
    int token = MAGIC_TOKEN;

    if (NULL == pathname)
    {
        DBG_PRINT("input para invalid");
        return ERR_ERROR;
    }

    id = shmget(ftok(pathname, token), 0, SVSHM_MODE);
    if (-1 == id)
    {
        DBG_PRINT("shmget error: %s", strerror(errno));
        return ERR_ERROR;
    }

    return id;
}

void *shm_get_base(int id)
{
    void *ptr;

    if (id < 0)
    {
        DBG_PRINT("input para invalid");
        return NULL;
    }
    if ((void *)-1 == (ptr = shmat(id, NULL, 0)))
    {
        DBG_PRINT("shmat error: %s", strerror(errno));
        return NULL;
    }

    return ptr;
}

int shm_detach(int id)
{
    void *shm_base;

    if (id < 0)
    {
        DBG_PRINT("input para invalid");
        return ERR_ERROR;
    }

    if (NULL == (shm_base = shm_get_base(id)))
    {
        DBG_PRINT("failed to detach share memory");
        return ERR_ERROR;
    }

    if (-1 == shmdt(shm_base))
    {
        DBG_PRINT("failed to detach: %s", strerror(errno));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

int shm_destroy(int id)
{
    if (id < 0)
    {
        DBG_PRINT("input para invalid");
        return ERR_ERROR;
    }

    if (-1 == shmctl(id, IPC_RMID, NULL))
    {
        DBG_PRINT("shmctl error: %s", strerror(errno));
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}

int shm_write(int id, void *data, int length)
{
    struct shmid_ds buff;
    char *ptr;

    if (id <= 0 || NULL == data || length <= 0)
    {
        printf("Usage: shmrmid <pathname> [length#]\n");
        return ERR_ERROR;
    }

    ptr = shm_get_base(id);
    if (NULL == ptr)
    {
        DBG_PRINT("failed to get share memory address");
        return ERR_ERROR;
    }
    if (-1 == shmctl(id, IPC_STAT, &buff))
    {
        DBG_PRINT("shmctl error: %s", strerror(errno));
        return ERR_ERROR;
    }
    if (length > buff.shm_segsz)
    {
        DBG_PRINT("out of share memory");
        return ERR_ERROR;
    }
    memcpy(ptr, data, length);

    return ERR_SUCCESS;
}

int shm_read(int id, void **data)
{
    struct shmid_ds buff;

    if (id < 0 || NULL == data)
    {
        DBG_PRINT("input para invalid");
        return ERR_ERROR;
    }

    *data = shm_get_base(id);
    if (NULL == *data)
    {
        DBG_PRINT("failed to get share memory address");
        return ERR_ERROR;
    }

    if (-1 == shmctl(id, IPC_STAT, &buff))
    {
        DBG_PRINT("shmctl error: %s", strerror(errno));
        return ERR_ERROR;
    }

    return buff.shm_segsz;
}
