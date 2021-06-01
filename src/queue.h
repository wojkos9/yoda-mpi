#ifndef __QUEUE_H__
#define __QUEUE_H__

typedef struct {
    int x, y;
} val_t;

struct node_t{
    val_t val;
    struct node_t *next;
};

typedef struct node_t node_t;


typedef struct {
node_t *root;
} queue_t;

static queue_t QUEUE_INIT;

int qput(queue_t *qu, int x, int y);
int qputv(queue_t *qu, val_t v);
int qdel(queue_t *qu);
int qrm1(queue_t *qu, int y);
void qprint(queue_t *qu);

#endif // __QUEUE_H__
