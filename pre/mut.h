#ifndef __MUT_H__
#define __MUT_H__

#include "utils.h"

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

#endif // __MUT_H__