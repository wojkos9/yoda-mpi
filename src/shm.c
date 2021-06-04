
#include "shm.h"

#include "state.h"

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>

#include "utils.h"

#include "main.h"
#include "state.h"

#define SHM_PATH "/yoda_shm"

shm_common_t *shm_common;
shm_info_t *shm_info_arr;

pthread_mutex_t memlock = PTHREAD_MUTEX_INITIALIZER;

void init_shm() {

    HAS_SHM = 1;

    int shm_size = sizeof(shm_common_t) + sizeof(shm_info_t) * size;

    if (rank != 0) {
        pthread_mutex_lock(&memlock);
    }

    int fd = shm_open(SHM_PATH, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd < 0) goto shm_fail;

    if (rank == 0) {
        debug(0, "TRUNC FILE");
        if (-1 == ftruncate(fd, shm_size)) goto shm_fail;
        // close(fd);
        // fd = shm_open(SHM_PATH, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        // if (fd < 0) goto shm_fail;
    }

    shm_common = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_common == MAP_FAILED) goto shm_fail;

    MEM_INIT = 1;

    shm_info_arr = (void *)shm_common + sizeof(shm_common_t);

    if (rank == 0) {
        memset(shm_common, 0, shm_size);
        shm_common->curr = energy;
        for (int i = 0; i < size; i++) {
            shm_info_arr[i].pad = '-';
        }
    }

    

    goto shm_succ;

    shm_fail:
    debug(0, "SHM FAIL");
    HAS_SHM = 0;
    MEM_INIT = 0;
    if (rank != 0) sync_all_with_msg(FIN, 0);
    shm_succ:
    if (rank == 0) sync_all_with_msg(MEM, HAS_SHM);
}