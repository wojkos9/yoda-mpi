#ifndef STATE_H_
#define STATE_H_

#include "main.h"
#include "macro_expand.h"

#define ALL_STATES \
(IDLE) \
(FIN) \
(AWAIT) \
(ORD) \
(PAIR) \
(CRIT) \
(WORK) \
(LEAVE) \
(SLEEP) \
(ZCRIT) \

typedef enum {
#define EXPAND_FUN(x) PREFIX_EXPAND(ST_, x)
    FUN_APPLY(ALL_STATES)
#undef EXPAND_FUN
} ST;

static char *state_map[] = {
#define EXPAND_FUN QUOTE_EXPAND
    FUN_APPLY(ALL_STATES)
#undef EXPAND_FUN
};

extern ST state;

extern int size, rank, lamport, ack_count;
extern PTYP styp, otyp;
extern int pair;

extern int energy;

#endif // STATE_H_
