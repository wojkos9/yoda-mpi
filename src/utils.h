#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include "state.h"

#define log(...) fprintf(stderr, "[%d] " fmt, rank, __VA_ARGS__)

#define P_WHITE printf("%c[%d;%dm", 27, 1, 37);
#define P_BLACK printf("%c[%d;%dm", 27, 1, 30);
#define P_RED printf("%c[%d;%dm", 27, 1, 31);
#define P_GREEN printf("%c[%d;%dm", 27, 1, 33);
#define P_BLUE printf("%c[%d;%dm", 27, 1, 34);
#define P_MAGENTA printf("%c[%d;%dm", 27, 1, 35);
#define P_CYAN printf("%c[%d;%d;%dm", 27, 1, 36);
#define P_SET(X) printf("%c[%d;%dm", 27, 1, 31 + (6 + X) % 7);
#define P_CLR printf("%c[%d;%dm", 27, 0, 37);

/* printf colored based on rank */
#define println(FORMAT, ...) printf("%c[%d;%dm [tid %d ts %d]: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, lamport, ##__VA_ARGS__, 27, 0, 37);

static int DEBUG_LVL = 0;

#define debug(lvl, ...) if (DEBUG_LVL >= lvl) println(__VA_ARGS__);

#define MAX(a, b) ((a)>(b)?(a):(b))

#endif // UTILS_H_