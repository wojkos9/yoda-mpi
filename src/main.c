#include <mpi.h>

#include "state.h"
#include "utils.h"

#include "main.h"

#include "queue.h"

#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>

ST state;

int size, rank, lamport, ack_count;
PTYP styp, otyp;
int pair = -1;

int DEBUG_LVL;

MPI_Datatype PAK_T;

int cx, cy, cz, copp, cown, opp_base;
int offset;
int place = 0, last_place = -1;

int *places;

queue_t qu = QUEUE_INIT;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lamut = PTHREAD_MUTEX_INITIALIZER;


void try_reserve_place() {
    if (ack_count == cown) {
        ack_count = 0;
        if (DEBUG_LVL >= 9) {
            col(printf("RESERVING +%d: ", offset), qprint(&qu));
        }
        pthread_mutex_lock(&lamut);
        place = qrm1(&qu, rank) + offset;
        ++offset;
        pthread_mutex_unlock(&lamut);
        state = ST_WAIT;
        debug(9, "PLACE %d", place);
        psend_to_typ(otyp, ORD, place);
        for(int i = 0; i < copp; i++) {
            if (places[i] == place) {
                pair = i + opp_base;
                state = ST_PAIR;
                pthread_mutex_unlock(&mut);
            }
        }
    }
}

void comm_th() {

    MPI_Status status;
    packet_t pkt;

    int i;

    while (state != ST_FIN) {
        MPI_Recv( &pkt, 1, PAK_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        lamport = MAX(lamport, pkt.ts) + 1;

        debug(20, "RECV %d from %d", status.MPI_TAG, pkt.src);

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
                    ++ack_count;
                    try_reserve_place();
                } else if (state == ST_POST) {
                    ++ack_count;
                    if (ack_count == cown - 1) {
                        state = ST_IDLE;
                        pthread_mutex_unlock(&mut);
                    }
                }
                
                break;
            case ORD:
                i = pkt.src - opp_base;
                places[i] = pkt.data;
                if (state == ST_WAIT && pkt.data == place) {
                    pair = pkt.src;
                    state = ST_PAIR;
                    pthread_mutex_unlock(&mut);
                }
                break;
            case FIN:
                debug(0, "FIN");
                state = ST_FIN;
                break;
            case REL:
                pthread_mutex_lock(&lamut);
                qrm1(&qu, pkt.src);
                offset += 1;
                pthread_mutex_unlock(&lamut);
                psend(pkt.src, ACK);
                break;
        }
    }
}

void start_order() {
    ack_count = 0;
    state = ST_ORD;
    if (cown > 1) {
        psend_to_typ_all(styp, PAR, lamport);
    } else {
        try_reserve_place();
    }
    
}
void release_place() {
    ack_count = 0;
    state = ST_POST;
    psend_to_typ(styp, REL, 0);
}


void* main_th(void *p) {
    debug(20, "MAIN %d %d %d", rank, cown, copp)
    while(state != ST_FIN) {
        start_order();
        pthread_mutex_lock(&mut);
        debug(10, "PAIR %d @ %d", pair, place);
        place = -1;
        pair = -1;
        usleep(500000);
        if (rank == 0) printf("\n\n\n");
        usleep(500000);

        release_place();
        pthread_mutex_lock(&mut);
        state = ST_IDLE;
    }
    
    return 0;
}

void init(int *argc, char ***argv)
{
    int provided;
    MPI_Init_thread(argc, argv, MPI_THREAD_MULTIPLE, &provided);
    // check_thread_support(provided);

    const int nitems = 3; /* bo packet_t ma trzy pola */
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
}

int main(int argc, char **argv)
{
    init(&argc, &argv);

    DEBUG_LVL = 9;

    if (argc > 1) {
        DEBUG_LVL = atoi(argv[1]);
    }

    if (argc >= 4) {
        cx = atoi(argv[1]);
        cy = atoi(argv[2]);
        cz = atoi(argv[3]);
    } else {
        cy = cx = size / 2;
        // cx = size;
        //cz = size - cx - cy;
    }

    styp = rank < cx ? PT_X : (rank < cx+cy ? PT_Y : PT_Z);
    otyp = styp < PT_Z ? 1-styp : PT_X;
    cown = *cnts[styp];
    copp = *cnts[otyp];
    opp_base = styp == PT_X ? copp : 0;

    places = malloc(sizeof(int) * copp);

    debug(30, "Hello %d %d %d %d %d %c -> %c", argc, size, cx, cy, cz, "XYZ"[styp], "XYZ"[otyp]);

    pthread_mutex_lock(&mut);

    pthread_t th;

    pthread_create(&th, NULL, main_th, NULL);

    ack_count = 0;
    // start_order();

    comm_th();

    pthread_join(th, NULL);

    free(places);
    MPI_Finalize();
    return 0;
}