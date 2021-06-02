#ifndef __MAIN_H__
#define __MAIN_H__

#include <mpi.h>

extern MPI_Datatype PAK_T;

typedef enum {
    REQ, ACK, PAR, ORD, FIN
} MTYP;

typedef enum {
    PT_X, PT_Y, PT_Z
} PTYP;

extern int cx, cy, cz, copp;

static int* cnts[] = {&cx, &cy, &cz};

typedef struct {
    int ts;         /* timestamp (Lamport clock) */
    int src;        /* message sender */
    int data;       /* data */
} packet_t;

extern int psend(int dest, MTYP typ);
extern int psend1(int dest, MTYP typ, int data);

#endif // __MAIN_H__
