#ifndef __SHM_H__
#define __SHM_H__

#include <pthread.h>

extern void init_shm();

typedef struct {
    char ch;
    char pair;
    char pad;
    char ack;
    char msg;
    char pad2[16-5];
} shm_info_t;

typedef struct {
    char en;
    char curr_energy;
    pthread_mutex_t mut;
} shm_common_t;

#define SHM_INFO_INIT {0}

extern shm_info_t *shm_info_arr;
extern shm_common_t *shm_common;
extern int HAS_SHM, MEM_INIT;

extern pthread_mutex_t memlock;

#define SHM_SAFE(op) pthread_mutex_lock(&shm_common->mut); \
            op \
            pthread_mutex_unlock(&shm_common->mut);

#endif // __SHM_H__
