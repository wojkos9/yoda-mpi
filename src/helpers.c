#include "state.h"
#include "utils.h"

int psend1(int dest, MTYP typ, int data) {
    packet_t pkt;
    pkt.ts = ++lamport;
    pkt.src = rank;
    pkt.data = data;
    MPI_Send(&pkt, 1, PAK_T, dest, typ, MPI_COMM_WORLD);
    debug(20, "SEND %d to %d", typ, dest);
}

int psend(int dest, MTYP typ) {
    psend1(dest, typ, 0);
}

int psend_to_typ_except(PTYP ptyp, MTYP mtyp, int data, int except) {
    
    int c0 = ptyp==PT_X ? 0 : (ptyp == PT_Y ? cx : cx+cy); 
    for (int i = c0; i < c0 + *cnts[ptyp]; i++) {
        if (i != except) {
            psend1(i, mtyp, data);
        }
    }
}

int psend_to_typ_all(PTYP ptyp, MTYP mtyp, int data) {
    psend_to_typ_except(ptyp, mtyp, data, -1);
}

int psend_to_typ(PTYP ptyp, MTYP mtyp, int data) {
    psend_to_typ_except(ptyp, mtyp, data, rank);
}