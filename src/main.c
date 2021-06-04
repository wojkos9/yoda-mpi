#include <mpi.h>

#include "state.h"
#include "utils.h"

#include "main.h"

#include "queue.h"

#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>

#include "shm.h"

int COUNTS_OVR = 0;

int HAS_SHM = 1;
int MEM_INIT = 0;

ST state;

int size, rank, lamport, ack_count;
PTYP styp, otyp;
int pair = -1;

int DEBUG_LVL;

MPI_Datatype PAK_T;

int cx, cy, cz, copp, cown, opp_base;
int offset;
int place = 0, last_place = -1;

val_t own_req;

int *places;

queue_t qu = QUEUE_INIT;
queue_t qu_x = QUEUE_INIT;
queue_t qu_z = QUEUE_INIT;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t start_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t can_leave = PTHREAD_MUTEX_INITIALIZER;

int blocked = 0;
int energy = 5;

int dack_count = 0;

int messenger = 0;

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

void inc_ack() {
    ++ack_count;
    if (MEM_INIT) {
        shm_info_arr[rank].ack = ack_count;
    }
}

void zero_ack() {
    ack_count = 0;
    if (MEM_INIT) {
        shm_info_arr[rank].ack = 0;
    }
}

void try_init_shm() {
    if (HAS_SHM) {
        pthread_mutex_lock(&memlock);
        if (HAS_SHM) {
            init_shm();
            shm_info_arr[rank].st = 1;
        }

        debug(0, "HAS SHM: %d", HAS_SHM);
        
    }

}

void change_state(ST new) {
    state = new;
    if (MEM_INIT) {
        shm_info_arr[rank].ch = state_map[state][0];
    }
    debug(15, "\t\t\t\t\t-> STATE %s", state_map[new]);
}

void try_reserve_place() {
    if (ack_count == cown) {
        zero_ack();
        if (DEBUG_LVL >= 9) {
            col(printf("RESERVING +%d: ", offset), qprint(&qu));
        }
        place = qrm1(&qu, rank) + offset;
        ++offset;
        change_state(ST_AWAIT);
        debug(9, "PLACE %d", place);
        psend_to_typ(otyp, ORD, place);
        for(int i = 0; i < copp; i++) {
            if (places[i] == place) {
                pair = i + opp_base;
                change_state(ST_PAIR);
                pthread_mutex_unlock(&mut); // -> ST_PAIR
            }
        }
    }
}

void try_enter() { // X
    if (ack_count >= cown - energy) {
        if (!blocked) {
            --energy;
            change_state(ST_CRIT);
            debug(9, "TO_CRIT");
            pthread_mutex_unlock(&mut); // -> ST_CRIT

            if (MEM_INIT) {
                debug(10, "-------DEC--------");
                shm_common->en += 1;
                shm_common->curr -= 1;
            }
        }
    }
}

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
            case REQ:
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
                    pair = pkt.src;
                    change_state(ST_PAIR);
                    pthread_mutex_unlock(&mut); // -> ST_PAIR
                }
                break;
            case FIN:
                debug(0, "FIN");
                change_state(ST_FIN);
                break;
            case REL:
                qrm1(&qu, pkt.src);
                offset += 1;
                psend(pkt.src, ACK);
                break;
            case MEM:
                HAS_SHM = pkt.data;
                pthread_mutex_unlock(&memlock);
                break;
            case DEC:
                --energy;
                qrm1(&qu, pkt.src);
                offset += 1;
                psend(pkt.src, DACK);
                if (!energy) {
                    blocked = 1;
                    // release_z();
                }
                break;
            case DACK:
                ++dack_count;
                if (dack_count == cown - 1) {
                    if (!energy) {
                        messenger = 1;
                    }
                    pthread_mutex_unlock(&can_leave); // X -> ST_IDLE
                }
                break;
            case END:
                change_state(ST_LEAVE);
                pthread_mutex_unlock(&mut); // -> ST_LEAVE
                break;
            case STA: // Y
                pair = pkt.src;
                change_state(ST_CRIT);
                pthread_mutex_unlock(&start_mut); // Y -> ST_CRIT
                break;
            case INC:
                ++energy;
                if (energy == cz && blocked) {
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
                change_state(ST_AWAIT);
                pthread_mutex_unlock(&mut);
                break;
            case ACK:
                inc_ack();
                if (ack_count == cx) {
                    change_state(ST_ZCRIT);
                    pthread_mutex_unlock(&mut);
                }
                break;
            case MEM:
                HAS_SHM = pkt.data;
                pthread_mutex_unlock(&memlock);
                break;
        }
    }
}

void *main_th_z(void *p) {

    try_init_shm();

    while (state != ST_FIN) {
        pthread_mutex_lock(&mut);
        zero_ack();
        psend_to_typ_all(PT_X, ZREQ, 0);

        pthread_mutex_lock(&mut);

        if (MEM_INIT) {
            shm_info_arr[rank].st += 1;
        }

        usleep(rand() % 10 * 100000);

        if (MEM_INIT) {
            shm_common->curr += 1;
        }
        psend_to_typ_all(PT_X, INC, 0);
        change_state(ST_SLEEP);
    }
    return 0;
}

void start_order() {
    zero_ack();
    change_state(ST_ORD);
    if (cown > 1 || 1) {
        pthread_mutex_lock(&lamut);
        psend_to_typ_all(styp, PAR, lamport);
        pthread_mutex_unlock(&lamut);
    } else {
        try_reserve_place();
    }
}

void start_enter_crit() {
    if (cown == 1) {
        dack_count = 1;
        pthread_mutex_unlock(&mut);
        pthread_mutex_unlock(&can_leave);
        return;
    }
    dack_count = 0;

    pthread_mutex_lock(&lamut);
    own_req.x = lamport;
    psend_to_typ(styp, REQ, own_req.x);
    pthread_mutex_unlock(&lamut);
}

void release_place() {
    zero_ack();
    psend_to_typ(styp, REL, 0);
}

void* main_th_xy(void *p) {
    debug(20, "MAIN %d %d %d", rank, cown, copp)

    try_init_shm();

    pthread_mutex_lock(&can_leave);
    pthread_mutex_lock(&start_mut);

    while(state != ST_FIN) {
        start_order();
        pthread_mutex_lock(&mut);
        debug(10, "PAIR %d @ %d", pair, place);

        if (styp == PT_X) {
            debug(15, "------TRY ENTER------");
            start_enter_crit(); // ST_PAIR
            pthread_mutex_lock(&mut);
            psend_to_typ(styp, DEC, 0); // ST_CRIT
            
            debug(15, "CRIT");

            int pid;
            while ((pid = qpop(&qu_x)) != -1) {
                psend(pid, ACK);
            }
            change_state(ST_WORK);
            psend(pair, STA); // send START to Y
            
        } else {
            pthread_mutex_lock(&start_mut); // Y wait for START
            change_state(ST_WORK);
            release_place();
            
        }

        

        debug(15, "START %d", pair);

        if (MEM_INIT) {
            shm_info_arr[rank].st += 1;
        }
        

        usleep(rand() % 5 * 50000);

        // usleep(5 * 1000000);

        debug(15, "END %d", pair);

        psend(pair, END);
        pthread_mutex_lock(&mut); // wait for END

        if (messenger) {
            messenger = 0;
            debug(10, "WAKING");
            psend_to_typ_all(PT_Z, WAKE, 0);
        }

        

        place = -1;
        pair = -1;

        usleep(50000);
        if (rank == 0 && DEBUG_LVL >= 9) printf("\n\n\n");
        usleep(50000);

        change_state(ST_LEAVE);

        if (styp == PT_X) {
            release_z();
        }
        
        
        
        pthread_mutex_lock(&can_leave);
        change_state(ST_IDLE);

        
    }
    
    return 0;
}




void init(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    // check_thread_support(provided);

    const int nitems = 3;
    int blocklengths[3] = {1,1,1};
    MPI_Datatype typy[3] = {MPI_INT, MPI_INT, MPI_INT};

    MPI_Aint     offsets[3]; 
    offsets[0] = offsetof(packet_t, ts);
    offsets[1] = offsetof(packet_t, src);
    offsets[2] = offsetof(packet_t, data);

    MPI_Type_create_struct(nitems, blocklengths, offsets, typy, &PAK_T);
    MPI_Type_commit(&PAK_T);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    srand(rank);
    own_req.y = rank;
}

int main(int argc, char **argv)
{
    init(&argc, &argv);

    DEBUG_LVL = 9;

    parse_args(argc, argv);

    // if (argc >= 4) {
    //     cx = atoi(argv[1]);
    //     cy = atoi(argv[2]);
    //     cz = atoi(argv[3]);
    // } else {
    //     cy = cx = size / 2;
    //     // cx = size;
    //     //cz = size - cx - cy;
    // }
    if (!COUNTS_OVR) {
        cy = cx = size / 2;
    }

    if (rank == 0) {
        printf("ENERGY %d, %d-%d-%d\n", energy, cx, cy, cz);
    }
    

    styp = rank < cx ? PT_X : (rank < cx+cy ? PT_Y : PT_Z);
    otyp = styp < PT_Z ? 1-styp : PT_X;
    cown = *cnts[styp];
    copp = *cnts[otyp];
    opp_base = styp == PT_X ? cown : 0;

    places = malloc(sizeof(int) * copp);

    debug(30, "Hello %d %d %d %d %d %c -> %c", argc, size, cx, cy, cz, "XYZ"[styp], "XYZ"[otyp]);

    pthread_mutex_lock(&mut);

    pthread_t th;

    if (styp != PT_Z) {
        pthread_create(&th, NULL, main_th_xy, NULL);
        comm_th_xy();
    } else {
        state = ST_SLEEP;
        pthread_create(&th, NULL, main_th_z, NULL);
        comm_th_z();
    }

   

    pthread_join(th, NULL);

    free(places);
    MPI_Finalize();
    return 0;
}