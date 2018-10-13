/*************************************************************************
	> File Name: shm_util.h
	> Author: 
	> Mail: 
	> Created Time: Sat 13 Oct 2018 03:06:21 AM EDT
 ************************************************************************/

#ifndef _SHM_UTIL_H
#define _SHM_UTIL_H

#include <sys/shm.h>

#define DBG_PRINT(fmt, ...) \
        printf("[%-6.5s: %06d] "fmt"\n", __func__, __LINE__, ##__VA_ARGS__);

#define SVSHM_MODE (SHM_R | SHM_W | SHM_R>>3 | SHM_R>>6)
#define MAGIC_TOKEN ('t'<<24|'e'<<16|'s'<<8|'t')

enum
{
    ERR_ERROR = -1,
    ERR_SUCCESS = 0
};

int shm_create(const char *pathname, int length);
int shm_destroy(int id);
int shm_attach(const char *pathname);
void *shm_get_base(int id);
int shm_detach(int id);
int shm_write(int id, void *data, int length);
int shm_read(int id, void **data);

#endif
