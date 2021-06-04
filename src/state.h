#ifndef STATE_H_
#define STATE_H_

#include "main.h"

typedef enum {
    ST_IDLE, ST_FIN, ST_WAIT, ST_ORD, ST_PAIR, ST_CRIT, ST_WORK, ST_LEAVE, ST_POST,
    ST_SLEEP
} ST;

extern ST state;

extern int size, rank, lamport, ack_count;
extern PTYP styp, otyp;
extern int pair;

#endif // STATE_H_
