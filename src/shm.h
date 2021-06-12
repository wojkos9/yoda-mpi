#ifndef __SHM_H__
#define __SHM_H__

#include <pthread.h>

extern void init_shm();

typedef struct {
    char ch;
    char pair;
    char en;
    char ack;
    char msg;
    char pad2[3];
    char x;
    char pad3[3];
    char y;
    char pad4[3];
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

// #define SHM_SAFE(op) if (MEM_INIT) {\
//             pthread_mutex_lock(&shm_common->mut); \
//             op \
//             pthread_mutex_unlock(&shm_common->mut); \
//             }

#define SHM_SAFE(op) if (MEM_INIT) {\
            op \
            }

#define SHM_SAFE2(op) if (MEM_INIT) {\
            pthread_mutex_lock(&shm_common->mut); \
            op \
            pthread_mutex_unlock(&shm_common->mut); \
            }

#endif // __SHM_H__
