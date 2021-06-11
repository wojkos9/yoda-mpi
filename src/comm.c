#include "main.h"
#include "state.h"
#include "utils.h"
#include "queue.h"
#include "shm.h"
#include "helpers.h"

void comm_th_xy() {

    MPI_Status status;
    packet_t pkt;

    int ts, i;

    while (state != ST_FIN) {
        MPI_Recv( &pkt, 1, PAK_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&lamut);
        lamport = MAX(lamport, pkt.ts) + 1;
        pthread_mutex_unlock(&lamut);

        debug(20 || status.MPI_TAG == ZREQ, "RECV %s/%d from %d", mtyp_map[status.MPI_TAG], pkt.data, pkt.src);

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
                } else if (state >= ST_WORK && styp == PT_Y) { // Y
                    inc_ack();
                    if (ack_count == cown - 1) {
                        pthread_mutex_unlock(&can_leave); // Y -> ST_IDLE
                    }
                } else if (state == ST_PAIR) {
                    inc_ack();
                    try_enter();
                }
                
                break;
            case REQ: // X
                if (state != ST_PAIR) {
                    psend(pkt.src, ACK);
                } else {
                    val_t req = {pkt.data, pkt.src};
                    if (VAL_GT(req, own_req)) {
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
                debug(0, "FIN");
                change_state(ST_FIN);
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
                qrm1(&qu, pkt.src);
                offset += 1;
                psend(pkt.src, DACK);
                if (!energy) {
                    blocked = 1;
                    // release_z();
                }
                break;
            case DACK: // X
                ++dack_count;
                if (dack_count == cown - 1) {
                    if (!energy) {
                        messenger = 1;
                        SHM_SAFE(
                            shm_info_arr[rank].msg = '!';
                        )
                    }
                    pthread_mutex_unlock(&can_leave); // X -> ST_IDLE
                }
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
                if (energy == cz) {
                    blocked = 0;
                    debug(10, "UNLOCK");
                    try_enter();
                }
        }
    }
}

void comm_th_z() {
    MPI_Status status;
    packet_t pkt;

    while (state != ST_FIN) {
        MPI_Recv( &pkt, 1, PAK_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        pthread_mutex_lock(&lamut);
        lamport = MAX(lamport, pkt.ts) + 1;
        pthread_mutex_unlock(&lamut);

        debug(10, "*ZREC %s/%d from %d", mtyp_map[status.MPI_TAG], pkt.data, pkt.src);

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
        }
    }
}