#ifndef STATE_H_
#define STATE_H_

#include "main.h"
#include "macro_expand.h"

#include "queue.h"

#include "types.h"

extern ST state;

extern int size, rank, lamport, ack_count, dack_count, enter_count, ok_count;
extern PTYP styp, otyp;
extern int pair, place;
extern val_t own_req;

extern int messenger;

extern int energy;

extern int HAS_SHM;

#endif // STATE_H_
