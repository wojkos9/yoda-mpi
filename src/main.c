#include <mpi.h>

#include "state.h"
#include "utils.h"

#include "main.h"

#include "queue.h"

#include <stdlib.h>

MPI_Datatype PAK_T;

int cx, cy, cz, copp, cown;
int offset;
int place;

queue_t qu = {0};


void try_reserve_place() {
    if (ack_count == cown - 1) {
        printf("%d: ", rank);
        if (DEBUG_LVL >= 30) qprint(&qu);
        place = qrm1(&qu, rank) + offset;
        ++offset;
        state = ST_WAIT;
        debug(10, "PLACE %d", place);
    }
}

void comm_th() {

    MPI_Status status;
    packet_t pkt;

    int cond = 0;

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
        }
    }
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

void start_order() {
    int ts = lamport;
    qput(&qu, ts, rank);
    ack_count = 0;
    psend_to_typ(styp, PAR, ts);
}

int main(int argc, char **argv)
{
    init(&argc, &argv);

    DEBUG_LVL = 10;

    if (argc >= 4) {
        cx = atoi(argv[1]);
        cy = atoi(argv[2]);
        cz = atoi(argv[3]);
    } else {
        //cy = cx = size / 2;
        cx = size;
        //cz = size - cx - cy;
    }

    styp = rank < cx ? PT_X : (rank < cx+cy ? PT_Y : PT_Z);
    otyp = styp < PT_Z ? 1-styp : PT_X;
    cown = *cnts[styp];
    copp = *cnts[otyp];

    debug(30, "Hello %d %d %d %d %d %c -> %c", argc, size, cx, cy, cz, "XYZ"[styp], "XYZ"[otyp]);

    start_order();

    comm_th();

    MPI_Finalize();
    return 0;
}