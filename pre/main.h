#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdio.h>

#include "mut.h"

#include <mpi.h>
#include <pthread.h>
#include <semaphore.h>

#include "queue.h"
#include "state.h"

#include "types.h"
//ifndef NOSHM
#include "shm.h"
//endif NOSHM



extern MPI_Datatype PAK_T;


extern int cx, cy, cz, copp;

static int* cnts[] = {&cx, &cy, &cz};

typedef struct {
    int ts;         /* timestamp (Lamport clock) */
    int src;        /* message sender */
    int data;       /* data */
} packet_t;

extern int psend(int dest, MTYP typ);
extern int psend1(int dest, MTYP typ, int data);
extern int psend_to_typ(PTYP ptyp, MTYP mtyp, int data);
extern int psend_to_typ_except(PTYP ptyp, MTYP mtyp, int data, int except);
extern int psend_to_typ_all(PTYP ptyp, MTYP mtyp, int data);
extern int psend_to_all(MTYP mtyp, int data);
extern int sync_all_with_msg(MTYP mtyp, int data);

int parse_args(int argc, char *argv[]);

extern int COUNTS_OVR;

extern int blocked;
extern int base_time;
extern int cx, cy, cz, copp, cown, opp_base, offset;

extern sem_t mut,
start_mut,
pair_mut,
crit_mut,
lamut,
can_leave;
extern pthread_mutex_t wake_mut;
extern int z_awake;

extern int *places;
extern queue_t qu, qu_x, qu_z;

extern void comm_th_xy();
extern void comm_th_z();

#define ERR_XZ_EXCL 2


#endif // __MAIN_H__
