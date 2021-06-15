#include "state.h"
#include "main.h"
#include "utils.h"

#include <stdlib.h>
#include <getopt.h>

int parse_args(int argc, char *argv[]) {
    char c;

    energy = -1;

    while ((c = getopt(argc, argv, "e:v:sc:t:")) != -1) {
        switch(c) {
            case 'e':
                energy = atoi(optarg);
                break;
            case 'v':
                DEBUG_LVL = atoi(optarg);
                break;
            //ifndef NOSHM
            case 's':
                HAS_SHM = 1;
                break;
            //endif NOSHM
            case 'c':
                cx = atoi(argv[optind-1]);
                cy = atoi(argv[optind]);
                cz = atoi(argv[optind+1]);
                optind += 2;
                COUNTS_OVR = 1;
                break;
            case 't':
                base_time = atof(optarg) * base_time;
                break;
        }
    }

    if (energy < 0) {
        energy = cz;
    }

}