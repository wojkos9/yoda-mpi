#ifndef __MAIN_H__
#define __MAIN_H__

#include <mpi.h>

#include "macro_expand.h"

extern MPI_Datatype PAK_T;

#define PKT_TYPES \
(REQ) \
(ACK) \
(PAR) \
(ORD) \
(FIN) \
(REL) \
(MEM) \
(DEC) \
(STA) \
(END) \
(DACK)

typedef enum {
#define EXPAND_FUN EXPAND
    FUN_APPLY(PKT_TYPES)
#undef EXPAND_FUN
} MTYP;

#define mkmtyp(t) #t

static char *mtyp_map[] = {
#define EXPAND_FUN QUOTE_EXPAND
    FUN_APPLY(PKT_TYPES)
#undef EXPAND_FUN
};

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
extern int psend_to_typ(PTYP ptyp, MTYP mtyp, int data);
extern int psend_to_typ_except(PTYP ptyp, MTYP mtyp, int data, int except);
extern int psend_to_typ_all(PTYP ptyp, MTYP mtyp, int data);
extern int psend_to_all(MTYP mtyp, int data);
extern int sync_all_with_msg(MTYP mtyp, int data);

int parse_args(int argc, char *argv[]);

extern int energy;

extern int COUNTS_OVR;


#endif // __MAIN_H__
