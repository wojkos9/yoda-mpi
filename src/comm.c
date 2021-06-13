#include "main.h"
#include "state.h"
#include "utils.h"
#include "queue.h"
#include "shm.h"
#include "helpers.h"

void comm_th_xy() {

    MPI_Status status;
    packet_t pkt;

    int ts, i, err;

    int fin = 0;

    while (state != ST_FIN && !fin) {
        // val_t own_req2 = own_req;
        err = MPI_Recv( &pkt, 1, PAK_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        // pthread_mutex_lock(&lamut);
        lamport = MAX(lamport, pkt.ts) + 1;
        // pthread_mutex_unlock(&lamut);

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
                } else if (state >= ST_WORK) { // Y
                    inc_ack();
                    if (ack_count == cown - 1) {
                        pthread_mutex_unlock(&can_leave); // Y -> ST_IDLE
                    }
                }
                
                break;
            case REQ: // X
                if (state != ST_PAIR && state != ST_CRIT) {
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
                if ((state != ST_CRIT) || blocked)
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
                if (state == ST_AWAIT && pkt.data == place) {
                    set_pair(pkt.src);
                    pthread_mutex_unlock(&pair_mut); // -> ST_PAIR
                }
                break;
            case FIN: // never used unless internal fail
                debug(0, "FIN==========================================");
                fin = 1;
                break;
            case REL: // Y
                qrm1(&qu, pkt.src);
                offset += 1;
                psend(pkt.src, ACK);
                break;
            case MEM: // shm
                HAS_SHM = pkt.data;
                pthread_mutex_unlock(&memlock);
                break;
            case DEC: // X
                --energy;
                SHM_SAFE(
                    shm_info_arr[rank].en = energy;
                )
                qrm1(&qu, pkt.src);
                offset += 1;
                psend(pkt.src, DACK);
                if (!energy) {
                    blocked = 1;
                    SHM_SAFE(
                        shm_info_arr[rank].x = 'B';
                    )
                    // release_z();
                }
                break;
            case DACK: // X
                ++dack_count;
                try_leave();
                break;
            case END:
                pthread_mutex_unlock(&mut); // -> ST_LEAVE
                break;
            case STA: // Y
                set_pair(pkt.src);
                pthread_mutex_unlock(&start_mut); // Y -> ST_CRIT
                break;
            case INC: // X
                ++energy;
                SHM_SAFE(
                    shm_info_arr[rank].en = energy;
                )
                if (energy == cz) {
                    blocked = 0;
                    SHM_SAFE(
                        shm_info_arr[rank].x = 'U';
                    )
                    debug(10, "UNLOCK");
                    try_enter();
                }
                break;
            case SHM_OK: // shm
                ++ok_count;
                debug(0, "OK %d", ok_count);
                if (ok_count == size) {
                    // pthread_mutex_unlock(&start_mut);
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
        pthread_mutex_lock(&lamut);
        lamport = MAX(lamport, pkt.ts) + 1;
        pthread_mutex_unlock(&lamut);

        debug(10, "RECV %s/%d from %d", mtyp_map[status.MPI_TAG], pkt.data, pkt.src);

        switch (status.MPI_TAG) {
            case WAKE:
                if (state == ST_SLEEP) {
                    pthread_mutex_unlock(&mut);
                }
                break;
            case ACK:
                inc_ack();
                if (ack_count == cx && state == ST_AWAIT) {
                    pthread_mutex_unlock(&start_mut);
                }
                break;
            case MEM:
                HAS_SHM = pkt.data;
                pthread_mutex_unlock(&memlock);
                break;
            case FIN:
                debug(0, "FIN");
                fin = 1;
                break;
            case SHM_OK: // shm
                ++ok_count;
                if (ok_count == size) {
                    // pthread_mutex_unlock(&start_mut);
                }
                break;
        }
    }
}