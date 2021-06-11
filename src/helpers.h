#ifndef __HELPERS_H__
#define __HELPERS_H__

#include "state.h"

extern void inc_ack();
extern void zero_ack();
extern void change_state(ST newst);
extern void try_reserve_place();
extern void try_enter();
extern void set_pair(int newpair);

#endif // __HELPERS_H__


