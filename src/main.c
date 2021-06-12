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

int enter_count = 0;
int ok_count = 0;

void random_sleep2(int min, int max) {
    usleep((min + rand() % (max-min)) * base_time);
}
void random_sleep(int max) {
    usleep(rand() % max * base_time);
}

int COUNTS_OVR = 0;

int HAS_SHM = 0;
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
pthread_mutex_t crit_mut = PTHREAD_MUTEX_INITIALIZER;
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

void wakeup_z() {
    debug(10, "\t\t\t*WAKING");
    SHM_SAFE(
        shm_info_arr[rank].y += 1;
    )
    psend_to_typ_all(PT_Z, WAKE, 0);
}

void try_leave() {
    if (dack_count == cown - 1) {
        if (!energy) {
            blocked = 1;
            wakeup_z();
            SHM_SAFE(
                shm_info_arr[rank].msg = '!';
            )
        }
        pthread_mutex_unlock(&can_leave); // X -> ST_IDLE
    }
}

void inc_ack() {
    ++ack_count;
    
    SHM_SAFE(
        // shm_info_arr[rank].ack = ack_count;
        snprintf(shm_info_arr[rank].pad3, 3, "%d", ack_count % 1000);
    )
}

void zero_ack() {
    ack_count = 0;
    
    SHM_SAFE(
        // shm_info_arr[rank].ack = 0;
        snprintf(shm_info_arr[rank].pad3, 3, "%d", ack_count  % 1000);
    )
}

void try_init_shm() {
    if (HAS_SHM) {
        pthread_mutex_lock(&memlock);
        if (1) {
            init_shm();
            debug(0, "HAS SHM: %d", HAS_SHM);

            // sync_all_with_msg(SHM_OK, 0);
            // pthread_mutex_lock(&start_mut);

            
            // if (!MEM_INIT) {
            //     sync_all_with_msg(FIN, 0);
            // }
        }
    }

}

void set_place(int newplace) {
    place = newplace;
    SHM_SAFE(
        snprintf(shm_info_arr[rank].pad4, 3, "%d", place % 1000);
    )
}

void change_state(ST new) {
    state = new;
    SHM_SAFE(
        shm_info_arr[rank].ch = state_map[state][0];
    )
    debug(15, "\t\t\t\t\t-> STATE %s", state_map[new]);
}

void set_pair(int new) {
    pair = new;
    SHM_SAFE(
        shm_info_arr[rank].pair = '0' + pair;
    )
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
        for(int i = 0; i < copp; i++) {
            if (places[i] == place) {
                set_pair(i + opp_base);
                pthread_mutex_unlock(&pair_mut); // -> ST_PAIR
            }
        }
    }
}

void notify_enter() {
    ++enter_count;
    SHM_SAFE(
        snprintf(shm_info_arr[rank].pad2, 3, "%d", enter_count % 1000);
    )
}

void try_enter() { // X
    debug(10, "////// TRY ENTER \\\\\\\\\\\\");
    if (state == ST_PAIR && ack_count >= cown - energy) {
        if (!blocked) {
            --energy;
            debug(9, "TO_CRIT");
            change_state(ST_CRIT);
            pthread_mutex_unlock(&crit_mut); // -> ST_CRIT

            debug(1, "-------DEC--------");

            int err = 0;
            SHM_SAFE2(
                shm_common->en += 1;
                shm_common->curr_energy -= 1;
                if (shm_common->curr_energy  < 0) {
                    err = 1;
                }
            )
            if (err) {
                sync_all_with_msg(FIN, 0);
            }
            

            SHM_SAFE(
                shm_info_arr[rank].en = energy;
            )
            

        }
    }
}

void start_order() {
    change_state(ST_ORD);
    // pthread_mutex_lock(&lamut);
    int ts = lamport;
    qput(&qu, ts, rank);
    
    zero_ack();
    psend_to_typ(styp, PAR, ts);
    // pthread_mutex_unlock(&lamut);

    if (cown == 1) {
        try_reserve_place();
    }
}

void start_enter_crit() { // X
    dack_count = 0;

    // pthread_mutex_lock(&lamut);
    own_req.x = lamport;
    psend_to_typ(styp, REQ, own_req.x);
    // pthread_mutex_unlock(&lamut);

    if (cown == 1) {
        try_enter();
        try_leave();
    }
}

void release_place() { // Y
    if (cown == 1) {
        pthread_mutex_unlock(&can_leave);
    } else {
        psend_to_typ(styp, REL, 0);
    }
    
}

void* main_th_xy(void *p) {
    debug(20, "MAIN %d %d %d", rank, cown, copp)

    pthread_mutex_lock(&can_leave);
    pthread_mutex_lock(&start_mut);
    pthread_mutex_lock(&pair_mut);
    pthread_mutex_lock(&crit_mut);

    try_init_shm();

    while(state != ST_FIN) {
        start_order();
        pthread_mutex_lock(&pair_mut);
        change_state(ST_PAIR);
        debug(10, "PAIR %d @ %d", pair, place);

        if (styp == PT_X) {
            SHM_SAFE(
                shm_info_arr[rank].msg = '#';
            )
            debug(15, "------START ENTER------");
            start_enter_crit(); // ST_PAIR
            pthread_mutex_lock(&crit_mut);
            change_state(ST_CRIT);

            debug(15, "CRIT");

            psend_to_typ(styp, DEC, 0); // ST_CRIT

            SHM_SAFE(
                shm_info_arr[rank].msg = 0;
            )

            change_state(ST_WORK);

            // release queued X
            int pid;
            while ((pid = qpop(&qu_x)) != -1) {
                psend(pid, ACK);
            }
            
            psend(pair, STA); // send START to Y
            
        } else {
            pthread_mutex_lock(&start_mut); // Y wait for START
            release_place();
            change_state(ST_WORK);
            
        }

        notify_enter();

        

        debug(15, "START %d", pair);

        random_sleep(10);

        debug(15, "END %d", pair);

        psend(pair, END);
        pthread_mutex_lock(&mut); // wait for END
        change_state(ST_LEAVE);

        set_place(-1);
        set_pair(-1);

        if (styp == PT_X) {
            release_z();
        }

        if (cown > 1) {
            pthread_mutex_lock(&can_leave);
        }
        

        
    }
    
    return 0;
}

void *main_th_z(void *p) {

    pthread_mutex_lock(&start_mut);

    try_init_shm();

    

    while (state != ST_FIN) {
        pthread_mutex_lock(&mut);
        change_state(ST_AWAIT);

        zero_ack();
        psend_to_typ_all(PT_X, ZREQ, 0);

        pthread_mutex_lock(&start_mut);
        change_state(ST_ZCRIT);

        notify_enter();

        random_sleep(10);

        
        SHM_SAFE2(
            shm_common->curr_energy += 1;
        )

        debug(1, "+++++++++INC++++++++++");
        psend_to_typ_all(PT_X, INC, 0);
        change_state(ST_SLEEP);
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

   

    // pthread_join(th, NULL);
    pthread_cancel(th);

    free(places);
    MPI_Finalize();
    return 0;
}