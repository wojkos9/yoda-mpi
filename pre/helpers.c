#include "main.h"

void release_z() {
    int pid;
    int rel = 0;
    while ((pid = qpop(&qu_z)) != -1) {
        rel = 1;
        psend(pid, ACK);
    }
    if (rel) {
        debug(10, "[[[ RELEASE Z ]]]");
    }
    else {
        debug(10, "[[[ NO Z ]]]");
    }
        
}

void block() {
    blocked = 1;
    // release_z();
    //ifndef NOSHM
    SHM_SAFE(
        shm_info_arr[rank].x = SYMB_B;
    )
    //endif NOSHM
    debug(10, "BLOCK");
}

void unblock() {
    blocked = 0;
    
    //ifndef NOSHM
    SHM_SAFE(
        shm_info_arr[rank].x = SYMB_U;
    )
    //endif NOSHM
    debug(10, "UNBLOCK");
}

void release_queue() {
    int pid;
    while ((pid = qpop(&qu_x)) != -1) {
        psend(pid, ACK);
    }
}

int tot_wake = 0;
void wakeup_z() {
    debug(10, "\t\t\t*WAKING");
    //ifndef NOSHM
    SHM_SAFE(
        shm_info_arr[rank].y = ++tot_wake;
    )
    //endif NOSHM
    psend_to_typ_all(PT_Z, WAKE, 0);
}

void try_leave() {
    if (dack_count == cown - 1) {
        if (!energy) {
            block();

            wakeup_z();
            //ifndef NOSHM
            //ifndef NOSHOWWAKE
            SHM_SAFE(
                shm_info_arr[rank].msg = '!';
            )
            //endif NOSHOWWAKE
            //endif NOSHM
        }
        mut_unlock(can_leave); // X -> ST_IDLE
    }
}

void inc_ack() {
    ++ack_count;
    //ifndef NOSHM
    SHM_SAFE(
        shm_info_arr[rank].a = ack_count+'0';
    )
    //endif NOSHM
}

void zero_ack() {
    ack_count = 0;
    //ifndef NOSHM
    SHM_SAFE(
        shm_info_arr[rank].a = ack_count+'0';
    )
    //endif NOSHM
}

//ifndef NOSHM
void try_init_shm() {
    if (HAS_SHM) {
        
        if (1) {
            init_shm();
            debug(0, "HAS SHM: %d", HAS_SHM);

        }
    }
}
//endif NOSHM

void set_place(int newplace) {
    place = newplace;
    //ifndef NOSHM
    SHM_SAFE(
        shmprintn(pad4, 3, place);
    )
    //endif NOSHM
}

void change_state(ST new) {
    state = new;
    //ifndef NOSHM
    SHM_SAFE(
        shm_info_arr[rank].ch = state_map[state][0];
    )
    //endif NOSHM
    debug(15, "\t\t\t\t\t-> STATE %s", state_map[new]);
}

void set_pair(int new) {
    pair = new;
    //ifndef NOSHM
    SHM_SAFE(
        shm_info_arr[rank].pair = '0' + pair;
    )
    //endif NOSHM
}

void try_reserve_place() {
    if (ack_count == cown - 1) {
        zero_ack();
        if (DEBUG_LVL >= 9) {
            col(printf("RESERVING +%d: ", offset), qprint(&qu));
        }
        set_place(qrm1(&qu, rank) + offset);
        ++offset;
        change_state(ST_AWAIT);
        debug(9, "PLACE %d", place);
        psend_to_typ(otyp, ORD, place);
        if (pair == -1) {
            for(int i = 0; i < copp; i++) {
                if (places[i] == place) {
                    set_pair(i + opp_base);
                    mut_unlock(pair_mut); // -> ST_PAIR
                }
            }
        }
    }
}
//ifndef NOSHM
void notify_enter() {
    ++enter_count;
    SHM_SAFE(
        shmprintn(pad2, 3, enter_count); 
    )
}
//endif NOSHM

void try_enter() { // X
    debug(30, "////// TRY ENTER \\\\\\\\\\\\");
    if (state == ST_PAIR && ack_count >= cown - energy) {
        if (!blocked) {
            zero_ack();
            int err = 0;
            //ifndef NOSHM
            //ifndef NOERR
            SHM_SAFE2(
                shm_common->x_crit += 1;
                if (shm_common->z_crit) {
                    err = ERR_XZ_EXCL;
                }
            )
            //endif NOERR
            //endif NOSHM
            --energy;
            debug(9, "TO_CRIT");
            change_state(ST_CRIT);
            mut_unlock(crit_mut); // -> ST_CRIT

            debug(1, "-------DEC--------");
            //ifndef NOSHM
            SHM_SAFE2(
                shm_common->tot_en += 1;
                shm_common->curr_energy -= 1;
                //ifndef NOERR
                if (shm_common->curr_energy  < 0) {
                    err = 1;
                }
                //endif NOERR
            )
            //ifndef NOERR
            if (err) {
                sync_all_with_msg(FIN, err);
            }
            //endif NOERR

            SHM_SAFE(
                shm_info_arr[rank].c = energy + '0';
            )
            //endif NOSHM
            

        }
    }
}
void start_order() {
    change_state(ST_ORD);
    mut_lock(lamut); // so that it doesn't respond to PAR in the meantime
    int ts = lamport;
    qput(&qu, ts, rank);

    zero_ack();
    psend_to_typ(styp, PAR, ts);
    mut_unlock(lamut);

    if (cown == 1) {
        try_reserve_place();
    }
}

void start_enter_crit() { // X
    dack_count = 0;

    own_req.x = lamport;
    
    change_state(ST_PAIR);

    psend_to_typ(styp, REQ, own_req.x);

    if (cown == 1) {
        try_enter();
        try_leave();
    }
}

void release_place() { // Y
    if (cown == 1) {
        mut_unlock(can_leave);
    } else {
        psend_to_typ(styp, REL, 0);
    }
}