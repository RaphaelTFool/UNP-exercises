/*************************************************************************
	> File Name: shm_get.c
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
    int length;

    if (3 != argc)
    {
        printf("Usage: %s <pathname> <length#>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    length = atoi(argv[2]);
    id = shm_create(argv[1], length);
    DBG_PRINT("id = (dec)%d, (hex)0x%08x", id, id);
    if (ERR_ERROR == id)
    {
        DBG_PRINT("failed to create share memory");
        return ERR_ERROR;
    }

    return ERR_SUCCESS;
}
