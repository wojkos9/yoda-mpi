#ifndef __HELPERS_H__
#define __HELPERS_H__

#include "state.h"

extern void inc_ack();
extern void zero_ack();
extern void set_place(int newplace);
extern void change_state(ST newst);
extern void try_reserve_place();
extern void try_enter();
extern void start_order();
extern void start_enter_crit();
extern void notify_enter();
extern void wakeup_z();
extern void release_z();
extern void release_place();
extern void release_queue();
extern void try_leave();
extern void block();
extern void unblock();
extern void set_pair(int newpair);
extern void try_init_shm();

#endif // __HELPERS_H__


