#ifndef __TYPES_H__
#define __TYPES_H__

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
(DACK) \
(INC) \
(WAKE) \
(ZREQ)

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


#endif // __TYPES_H__
