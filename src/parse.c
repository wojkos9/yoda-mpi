#include "main.h"
#include "utils.h"
#include "shm.h"

#include <stdlib.h>
#include <getopt.h>

int parse_args(int argc, char *argv[]) {
    char c;

    while ((c = getopt(argc, argv, "e:v:s")) != -1) {
        switch(c) {
            case 'e':
                energy = atoi(optarg);
                break;
            case 'v':
                DEBUG_LVL = atoi(optarg);
                break;
            case 's':
                HAS_SHM = 1;
                break;
        }
    }
}