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

#include "shm_util.h"

int main(int argc, char *argv[])
{
    int id;

    if (2 != argc)
    {
        printf("Usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    id = shm_attach(argv[1]);
    DBG_PRINT("id = (dec)%d, (hex)0x%08x", id, id);
    if (ERR_ERROR == id)
    {
        DBG_PRINT("failed to create share memory");
        return ERR_ERROR;
    }

    if (shm_destroy(id) < 0)
    {
        DBG_PRINT("failed to destroy share memory");
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}
