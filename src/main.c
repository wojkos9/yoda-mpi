#include <mpi.h>

#include "state.h"
#include "utils.h"

#include "main.h"

#include "queue.h"

#include <stdlib.h>

#include <pthread.h>
#include <unistd.h>

MPI_Datatype PAK_T;

int cx, cy, cz, copp, cown, opp_base;
int offset;
int place = 0;

int *places;

queue_t qu = QUEUE_INIT;

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;


void try_reserve_place() {
    if (ack_count == cown - 1) {
        ack_count = 0;
        if (DEBUG_LVL >= 30) qprint(&qu);
        place = qrm1(&qu, rank) + offset;
        ++offset;
        state = ST_WAIT;
        debug(10, "PLACE %d", place);
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
                psend(pkt.src, ACK);
                break;
            case ACK:
                ++ack_count;
                try_reserve_place();
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
        }
    }
}

void start_order() {
    int ts = lamport;
    qput(&qu, ts, rank);
    ack_count = 0;
    if (cown > 1) {
        psend_to_typ(styp, PAR, ts);
    } else {
        try_reserve_place();
    }
    
}

void* main_th(void *p) {
    debug(20, "MAIN %d %d %d", rank, cown, copp)
    while(state != ST_FIN) {
        start_order();
        pthread_mutex_lock(&mut);
        debug(5, "PAIR %d @%d", pair, place);
        state = ST_IDLE;
        place = -1;
        pair = -1;
        usleep(500000);
        if (rank == 0) printf("\n\n\n");
        usleep(500000);
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

int psend1(int dest, MTYP typ, int data) {
    packet_t pkt;
    pkt.ts = ++lamport;
    pkt.src = rank;
    pkt.data = data;
    MPI_Send(&pkt, 1, PAK_T, dest, typ, MPI_COMM_WORLD);
    debug(30, "SEND %d to %d", typ, dest);
}

int psend(int dest, MTYP typ) {
    psend1(dest, typ, 0);
}

int psend_to_typ(PTYP ptyp, MTYP mtyp, int data) {
    int c0 = ptyp==PT_X ? 0 : (ptyp == PT_Y ? cx : cx+cy); 
    for (int i = c0; i < c0 + *cnts[ptyp]; i++) {
        if (i != rank) {
            psend1(i, mtyp, data);
        }
    }
}

int main(int argc, char **argv)
{
    init(&argc, &argv);

    DEBUG_LVL = 10;

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