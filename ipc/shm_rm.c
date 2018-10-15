/*************************************************************************
	> File Name: shm_rm.c
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
    int ret;

    if (2 != argc)
    {
        printf("Usage: %s <pathname>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    id = shm_attach(argv[1]);
    if (ERR_ERROR == id)
    {
        DBG_PRINT("failed to get share memory base address");
        return ERR_ERROR;
    }
    ret = shm_destroy(id);
    if (ERR_ERROR == ret)
    {
        DBG_PRINT("failed to release share memory");
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}
