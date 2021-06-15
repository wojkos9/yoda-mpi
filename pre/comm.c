#include "main.h"
#include "state.h"
#include "utils.h"
#include "queue.h"
#include "helpers.h"

int inc_count = 0;

void comm_th_xy() {

    MPI_Status status;
    packet_t pkt;

    int ts, i, err;

    int fin = 0;

    while (state != ST_FIN && !fin) {
        // val_t own_req2 = own_req;
        err = MPI_Recv( &pkt, 1, PAK_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        if (status.MPI_TAG == PAR) mut_lock(lamut);
        lamport = MAX(lamport, pkt.ts) + 1;
       

        if (err != MPI_SUCCESS) {
            debug(0, "RECV ERROR %d", err);
            break;
        }

        debug(20, "RECV %s/%d from %d", mtyp_map[status.MPI_TAG], pkt.data, pkt.src);

        switch (status.MPI_TAG) {
            case PAR:
                qput(&qu, pkt.data, pkt.src);
                if (DEBUG_LVL >= 15) {
                    col(printf("ACKING "), qprint(&qu));
                }
                psend(pkt.src, ACK);
                mut_unlock(lamut);
                break;
            case ACK:
                if (state == ST_ORD) {
                    debug(15, "ACK <- %d", pkt.src);
                    inc_ack();
                    try_reserve_place();
                } else if (styp == PT_X) {
                    if (state == ST_PAIR) {
                        inc_ack();
                        try_enter();
                    }
                }
                
                break;
            case REQ: // X
                if (own_req.x == -1 || state != ST_PAIR && state != ST_CRIT) {
                    psend(pkt.src, ACK);
                } else {
                    val_t req = {pkt.data, pkt.src};
                    if (VAL_GT(own_req, req)) {
                        psend(pkt.src, ACK);
                    } else {
                        qputv(&qu_x, req);
                    }
                }
                break;
            case ZREQ:
                if ((state != ST_CRIT && state != ST_WORK))// || blocked)
                {
                    psend(pkt.src, ACK);
                } else {
                    val_t req = {pkt.data, pkt.src};
                    qputv(&qu_z, req);
                }
                break;
            case ORD:
                i = pkt.src - opp_base;
                places[i] = pkt.data;
                if (pair == -1 && state == ST_AWAIT && pkt.data == place) {
                    set_pair(pkt.src);
                    mut_unlock(pair_mut); // -> ST_PAIR
                }
                break;
            case FIN: // never used unless internal fail
                debug(0, "FIN==========================================");
                fin = 1;
                break;
            case REL: // Y
                qrm1(&qu, pkt.src);
                offset += 1;
                psend(pkt.src, DACK);
                break;
            //ifndef NOSHM
            case MEM: // shm
                HAS_SHM = pkt.data;
                mut_unlock(memlock);
                break;
            //endif NOSHM
            case DEC: // X
                --energy;
                //ifndef NOSHM
                SHM_SAFE(
                    shm_info_arr[rank].c = energy + '0';
                )
                //endif NOSHM
                qrm1(&qu, pkt.src);
                offset += 1;
                psend(pkt.src, DACK);
                if (!energy) {
                    block();
                    // release_z();
                }
                break;
            case DACK: // X
                ++dack_count;
                if (styp == PT_X) {
                    try_leave();
                } else {
                    if (dack_count == cown - 1) {
                        mut_unlock(can_leave); // Y -> ST_IDLE
                    }
                }
                break;
            case END:
                mut_unlock(mut); // -> ST_LEAVE
                break;
            case STA: // Y
                set_pair(pkt.src);
                mut_unlock(start_mut); // Y -> ST_CRIT
                break;
            case INC: // X
                ++energy;
                ++inc_count;
                //ifndef NOSHM
                SHM_SAFE(
                    shm_info_arr[rank].c = energy + '0';
                )
                //endif NOSHM
                if (inc_count == cz) {
                    inc_count = 0;
                    unblock();
                    
                    try_enter();
                }
                break;
        }
    }
}

void comm_th_z() {
    MPI_Status status;
    packet_t pkt;

    int fin = 0;

    while (state != ST_FIN && !fin) {
        MPI_Recv( &pkt, 1, PAK_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        lamport = MAX(lamport, pkt.ts) + 1;

        debug(10, "RECV %s/%d from %d", mtyp_map[status.MPI_TAG], pkt.data, pkt.src);

        switch (status.MPI_TAG) {
            case WAKE:
                if (state == ST_SLEEP) {
                    pthread_mutex_lock(&wake_mut);
                    if (z_awake == 0) {
                        mut_unlock(mut);
                        z_awake = 1;
                    }
                    pthread_mutex_unlock(&wake_mut);
                }
                break;
            case ACK:
                inc_ack();
                if (ack_count == cx && state == ST_AWAIT) {
                    mut_unlock(start_mut);
                }
                break;
            //ifndef NOSHM
            case MEM:
                HAS_SHM = pkt.data;
                mut_unlock(memlock);
                break;
            //endif NOSHM
            case FIN:
                debug(0, "FIN");
                fin = 1;
                break;
        }
    }
}