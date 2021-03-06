#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <pthread.h>

typedef struct {
    int x, y;
} val_t;

#define VAL_GT(v1, v2) (v1.x > v2.x || v1.x == v2.x && v1.y > v2.y)

struct node_t{
    val_t val;
    struct node_t *next;
};

typedef struct node_t node_t;


typedef struct {
node_t *root;
pthread_mutex_t mut;
} queue_t;

#define QUEUE_INIT {0, PTHREAD_MUTEX_INITIALIZER}

int qput(queue_t *qu, int x, int y);
int qputv(queue_t *qu, val_t v);
int qdel(queue_t *qu);
int qrm1(queue_t *qu, int y);
void qprint(queue_t *qu);
int qpop(queue_t *qu);

#endif // __QUEUE_H__
