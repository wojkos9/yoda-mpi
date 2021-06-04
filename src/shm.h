#ifndef __SHM_H__
#define __SHM_H__

#include "pthread.h"

extern void init_shm();

typedef struct {
    char st;
    char ch;
    char pad;
    char ack;
} shm_info_t;

typedef struct {
    char en;
    char curr;
} shm_common_t;

#define SHM_INFO_INIT {0}

extern shm_info_t *shm_info_arr;
extern shm_common_t *shm_common;
extern int HAS_SHM, MEM_INIT;

extern pthread_mutex_t memlock;

#endif // __SHM_H__
