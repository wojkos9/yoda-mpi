#ifndef STATE_H_
#define STATE_H_

#include "main.h"

typedef enum {
    ST_IDLE, ST_FIN, ST_WAIT, ST_ORD, ST_PAIR
} ST;

static ST state;

static int size, rank, lamport, ack_count;
static PTYP styp, otyp;
static int pair = -1;

#endif // STATE_H_
