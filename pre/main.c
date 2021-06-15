#include <mpi.h>

#include "state.h"
#include "utils.h"

#include "main.h"

#include "queue.h"

#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>

#include "helpers.h"

__thread int tid = 0;

int base_time = 100000;

void random_sleep2(int min, int max) {
    usleep((min + rand() % (max-min)) * base_time);
}
void random_sleep(int max) {
    usleep(rand() % max * base_time);
}

int COUNTS_OVR = 0;

//ifndef NOSHM
int HAS_SHM = 0;
int MEM_INIT = 0;
int enter_count = 0;
//endif NOSHM

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

sem_t mut,
start_mut,
pair_mut,
lamut,
can_leave,
crit_mut;

int blocked = 0;
int energy = 5;

int dack_count = 0;

int messenger = 0;

void* main_th_xy(void *p) {
    tid = 1;
    debug(20, "MAIN %d %d %d", rank, cown, copp);
    //ifndef NOSHM
    try_init_shm();
    //endif NOSHM

    while(state != ST_FIN) {
        start_order();
        mut_lock(pair_mut);
        own_req.x = -1;
        
        debug(10, "PAIR %d @ %d", pair, place);

        if (styp == PT_X) {
            //ifndef NOSHM
            SHM_SAFE(
                shm_info_arr[rank].msg = '#';
            )
            //endif NOSHM
            debug(15, "------START ENTER------");
            start_enter_crit(); // ST_PAIR
            
            mut_lock(crit_mut);
            change_state(ST_CRIT);

            debug(15, "CRIT");

            psend_to_typ(styp, DEC, 0); // ST_CRIT

            //ifndef NOSHM
            SHM_SAFE(
                shm_info_arr[rank].msg = 0;
            )
            //endif NOSHM

            change_state(ST_WORK);

            // release queued X
            release_queue();

            psend(pair, STA); // send START to Y
            
        } else {
            change_state(ST_PAIR);
            mut_lock(start_mut); // Y wait for START
            
            change_state(ST_WORK);
            
        }

        //ifndef NOSHM
        // shm
        notify_enter();
        //endif NOSHM

        debug(15, "START %d", pair);

        random_sleep(10);

        debug(15, "END %d", pair);

        psend(pair, END);
        mut_lock(mut); // wait for END
        change_state(ST_LEAVE);

        set_place(-1);
        set_pair(-1);

        if (styp == PT_X) {
            //ifndef NOSHM
            SHM_SAFE2(
                shm_common->x_crit -= 1;
            )
            //endif NOSHM
            release_z();
        } else {
            dack_count = 0;
            release_place();
        }

        if (cown > 1) {
            mut_lock(can_leave);
        }
        

        
    }
    
    return 0;
}

pthread_mutex_t wake_mut = PTHREAD_MUTEX_INITIALIZER;
int z_awake = 0;

void *main_th_z(void *p) {
    tid = 1;
    //ifndef NOSHM
    try_init_shm();
    //endif NOSHM

    while (state != ST_FIN) {
        z_awake = 0;
        mut_lock(mut);
        
        change_state(ST_AWAIT);

        zero_ack();
        psend_to_typ_all(PT_X, ZREQ, 0);

        mut_lock(start_mut);
        change_state(ST_ZCRIT);

        //ifndef NOSHM
        int err = 0;
        SHM_SAFE2(
            shm_common->z_crit += 1;
            if (shm_common->x_crit > 0) {
                err = ERR_XZ_EXCL;
            }
        )

        if (err) {
            sync_all_with_msg(FIN, err);
        }
        notify_enter();
        //endif NOSHM
        
        random_sleep(10);

        //ifndef NOSHM
        SHM_SAFE2(
            shm_common->curr_energy += 1;
        )
        SHM_SAFE2(
            shm_common->z_crit -= 1;
        )
        //endif NOSHM

        debug(1, "+++++++++INC++++++++++");
        change_state(ST_SLEEP);
        psend_to_typ_all(PT_X, INC, 0);
        
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
    places = malloc(sizeof(*places) * copp);
    memset(places, NO_PLACE, sizeof(*places) * copp);

    debug(30, "Hello %d %d %d %d %d %c -> %c", argc, size, cx, cy, cz, "XYZ"[styp], "XYZ"[otyp]);

    // by default wait for unlock
    mut_init(mut);
    mut_init(can_leave);
    mut_init(start_mut);
    mut_init(pair_mut);
    mut_init(crit_mut);
    mut_init2(lamut, 1);
    //ifndef NOSHM
    mut_init(memlock);
    //endif NOSHM

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