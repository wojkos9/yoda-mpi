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
#define colbeg printf("%c[%d;%dm [tid %d ts %d]: ", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, lamport)
#define colend printf("%c[%d;%dm\n", 27, 0, 37)
#define println(FORMAT, ...) printf("%c[%d;%dm [tid %d ts %d st %c b %d a %d E %d R %d] %c: " FORMAT "%c[%d;%dm\n", 27, (1 + (rank / 7)) % 2, 31 + (6 + rank) % 7, rank, lamport, state_map[state][0], blocked, ack_count, energy, own_req.x, "CW"[tid], ##__VA_ARGS__, 27, 0, 37)

#define col(...) colbeg, __VA_ARGS__, colend

extern int DEBUG_LVL;

#define debug(lvl, ...) if (DEBUG_LVL >= lvl) println(__VA_ARGS__)

#define MAX(a, b) ((a)>(b)?(a):(b))

#define SYMB_B '|'
#define SYMB_U '.'
#define NO_PLACE -2

#define MUT_DBG_LVL 31

#define mut_lock2(mut) {debug(MUT_DBG_LVL, "LMUT " #mut); _merr = pthread_mutex_lock(&mut); if (_merr) println("LOCK ERROR %d", _merr);}
#define mut_unlock2(mut) {debug(MUT_DBG_LVL, "UMUT " #mut); pthread_mutex_unlock(&mut);}

#if 1
static int _merr;
#define mut_lock(mut) {debug(MUT_DBG_LVL, "LMUT " #mut " L %d", __LINE__); _merr = sem_wait(&mut); if (_merr) println("LOCK ERROR %d", _merr);}
#define mut_unlock(mut) {debug(MUT_DBG_LVL, "UMUT " #mut " L %d", __LINE__); sem_post(&mut);}
#define mut_init(mut) sem_init(&mut, 0, 0)
#define mut_init2(mut, v) sem_init(&mut, 0, v)
#define mut_decl(...) sem_t __VA_ARGS__

#elif 1
static int _merr;
#define mut_lock(mut) {debug(MUT_DBG_LVL, "LMUT " #mut); _merr = pthread_mutex_lock(&mut); if (_merr) println("LOCK ERROR %d", _merr);}
#define mut_unlock(mut) {debug(MUT_DBG_LVL, "UMUT " #mut); pthread_mutex_unlock(&mut);}
#else
#define mut_lock(mut) pthread_mutex_lock(&mut)
#define mut_unlock(mut) pthread_mutex_unlock(&mut)
#endif

#endif // UTILS_H_