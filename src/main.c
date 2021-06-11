#include <mpi.h>

#include "state.h"
#include "utils.h"

#include "main.h"

#include "queue.h"

#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>

#include "shm.h"

int base_time = 100000;

void random_sleep2(int min, int max) {
    usleep((min + rand() % (max-min)) * base_time);
}
void random_sleep(int max) {
    usleep(rand() % max * base_time);
}

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
int place = 0;

val_t own_req;

int *places;

queue_t qu = QUEUE_INIT;
queue_t qu_x = QUEUE_INIT;
queue_t qu_z = QUEUE_INIT;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t start_mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pair_mut = PTHREAD_MUTEX_INITIALIZER;
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
        pthread_mutex_lock(&shm_common->mut);
        shm_info_arr[rank].ack = ack_count;
        pthread_mutex_unlock(&shm_common->mut);
    }
}

void zero_ack() {
    ack_count = 0;
    if (MEM_INIT) {
        pthread_mutex_lock(&shm_common->mut);
        shm_info_arr[rank].ack = 0;
        pthread_mutex_unlock(&shm_common->mut);
    }
}

void try_init_shm() {
    if (HAS_SHM) {
        pthread_mutex_lock(&memlock);
        if (HAS_SHM) {
            init_shm();
        }

        debug(0, "HAS SHM: %d", HAS_SHM);
        
    }

}

void change_state(ST new) {
    state = new;
    if (MEM_INIT) {
        pthread_mutex_lock(&shm_common->mut);
        shm_info_arr[rank].ch = state_map[state][0];
        pthread_mutex_unlock(&shm_common->mut);
    }
    debug(15, "\t\t\t\t\t-> STATE %s", state_map[new]);
}

void set_pair(int new) {
    pair = new;
    if (MEM_INIT) {
        pthread_mutex_lock(&shm_common->mut);
        shm_info_arr[rank].pair = '0' + pair;
        pthread_mutex_unlock(&shm_common->mut);
    }
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
                set_pair(i + opp_base);
                pthread_mutex_unlock(&pair_mut); // -> ST_PAIR
            }
        }
    }
}

void try_enter() { // X
    if (ack_count >= cown - energy) {
        if (!blocked) {
            --energy;
            debug(5, "ENENENENENENNENENENEN %d\n", energy);
            debug(9, "TO_CRIT");
            pthread_mutex_unlock(&mut); // -> ST_CRIT

            if (MEM_INIT) {
                debug(10, "-------DEC--------");
                pthread_mutex_lock(&shm_common->mut);
                shm_common->en += 1;
                shm_common->curr_energy -= 1;
                pthread_mutex_unlock(&shm_common->mut);
            }
            if (cown == 1 && !energy) {
                messenger = 1;
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

void *main_th_z(void *p) {

    try_init_shm();

    pthread_mutex_lock(&start_mut);

    while (state != ST_FIN) {
        pthread_mutex_lock(&mut);
        change_state(ST_AWAIT);

        zero_ack();
        psend_to_typ_all(PT_X, ZREQ, 0);

        pthread_mutex_lock(&start_mut);
        change_state(ST_ZCRIT);

        random_sleep(10);

        if (MEM_INIT) {
            pthread_mutex_lock(&shm_common->mut);
            shm_common->curr_energy += 1;
            pthread_mutex_unlock(&shm_common->mut);
        }
        debug(15, "+++++++++INC++++++++++");
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
        try_enter();
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
    if (cown == 1) {
        pthread_mutex_unlock(&can_leave);
    } else {
        psend_to_typ(styp, REL, 0);
    }
    
}

void* main_th_xy(void *p) {
    debug(20, "MAIN %d %d %d", rank, cown, copp)

    try_init_shm();

    pthread_mutex_lock(&can_leave);
    pthread_mutex_lock(&start_mut);
    pthread_mutex_lock(&pair_mut);

    while(state != ST_FIN) {
        start_order();
        pthread_mutex_lock(&pair_mut);
        change_state(ST_PAIR);
        debug(10, "PAIR %d @ %d", pair, place);

        if (styp == PT_X) {
            debug(15, "------TRY ENTER------");
            start_enter_crit(); // ST_PAIR
            pthread_mutex_lock(&mut);
            change_state(ST_CRIT);
            psend_to_typ(styp, DEC, 0); // ST_CRIT
            
            debug(15, "CRIT");

            int pid;
            while ((pid = qpop(&qu_x)) != -1) {
                psend(pid, ACK);
            }
            
            psend(pair, STA); // send START to Y
            
        } else {
            pthread_mutex_lock(&start_mut); // Y wait for START
            release_place();
            
        }

        change_state(ST_WORK);

        debug(15, "START %d", pair);

        random_sleep(10);

        debug(15, "END %d", pair);

        psend(pair, END);
        pthread_mutex_lock(&mut); // wait for END
        change_state(ST_LEAVE);

        if (messenger) { // co jak nie dostal DACK ?
            messenger = 0;
            debug(10, "\t\t\t*WAKING");
            psend_to_typ_all(PT_Z, WAKE, 0);
        }

        

        place = -1;
        set_pair(-1);

        if (styp == PT_X) {
            release_z();
        }
        
        
        
        pthread_mutex_lock(&can_leave);

        
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

    base_time = 100000;

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
    
    // 0, 1, ... cx-1 -> X
    // cx, cx+1, ..., cx+cy-1 -> Y
    // cx+cy.. size -> Z
    styp = rank < cx ? PT_X : (rank < cx+cy ? PT_Y : PT_Z);
    otyp = styp < PT_Z ? 1-styp : PT_X;
    cown = *cnts[styp];
    copp = *cnts[otyp];
    opp_base = styp == PT_X ? cown : 0;

    // places received from opposite process type
    places = malloc(sizeof(int) * copp);

    debug(30, "Hello %d %d %d %d %d %c -> %c", argc, size, cx, cy, cz, "XYZ"[styp], "XYZ"[otyp]);

    // by default wait for unlock
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