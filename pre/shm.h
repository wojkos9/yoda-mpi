#ifndef __SHM_H__
#define __SHM_H__

#include <pthread.h>
#include <semaphore.h>

#include "utils.h"
#include "mut.h"

extern void init_shm();

typedef struct {
    char ch;
    char pair;
    char en;
    char ack;
    char msg;
    char pad2[3];
    char x;
    char a;
    char b;
    char c;
    char y;
    char pad4[3];
} shm_info_t;

typedef struct {
    int tot_en;
    short curr_energy;
    char x_crit;
    char z_crit;
    pthread_mutex_t mut;
} shm_common_t;

#define SHM_INFO_INIT {0}

extern shm_info_t *shm_info_arr;
extern shm_common_t *shm_common;
extern int HAS_SHM, MEM_INIT;

extern mut_decl(memlock);

// #define SHM_SAFE(op) if (MEM_INIT) {\
//             mut_lock(shm_common->mut); \
//             op \
//             mut_unlock(shm_common->mut); \
//             }

#define SHM_SAFE(op) if (MEM_INIT) {\
            op \
            }

#if 1
#define SHM_SAFE2(op) if (MEM_INIT) {\
            mut_lock2(shm_common->mut); \
            op \
            mut_unlock2(shm_common->mut); \
            }
#else
#define SHM_SAFE2 SHM_SAFE
#endif

#endif // __SHM_H__
