#include <stdio.h>
#include <stdlib.h>

#include "queue.h"
#include "utils.h"

int qput(queue_t *qu, int x, int y) {
    val_t v = {x, y};
    qputv(qu, v);
}

int qputv(queue_t *qu, val_t v) {
    
    node_t *node;
    int pos = 1;

    node_t *next = (node_t *) malloc(sizeof(node_t));
    next->val = v;

    mut_lock(qu->mut);
    node = qu->root;
    if (!node || !VAL_GT(v, node->val)) {
        next->next = qu->root;
        qu->root = next;
        mut_unlock(qu->mut);
        return 0;
    }

    while (node->next && VAL_GT(v, node->next->val)) {
        node = node->next;
        ++pos;
    }
    
    next->next = node->next;
    node->next = next;
    mut_unlock(qu->mut);
    return pos;
}

int qdel(queue_t *qu) {
    node_t *node = qu->root;
    int c = 0;

    while(node) {
        node_t *t = node->next;
        free(node);
        node = t;
        ++c;
    }
    pthread_mutex_destroy(&qu->mut);
    return c;
}

int qrm1(queue_t *qu, int y) {
    mut_lock(qu->mut);
    node_t *node = qu->root;

    if (node && node->val.y == y) {
        node_t *t = qu->root;
        qu->root = node->next;
        mut_unlock(qu->mut);
        free(t);
        return 0;
    }

    int c = 0;

    while(node->next) {
        ++c;
        if (node->next->val.y == y) {
            node_t *t = node->next;
            node->next = node->next->next;
            mut_unlock(qu->mut);
            free(t);
            return c;
        }
        
        node = node->next;
    }
    mut_unlock(qu->mut);
    return -1;
}

int qpop(queue_t *qu) {
    mut_lock(qu->mut);
    int r = -1;
    if (qu->root) {
        r = qu->root->val.y;
        node_t *t = qu->root;
        qu->root = t->next;
        free(t);
    }
    mut_unlock(qu->mut);
    return r;
}

void qprint(queue_t *qu) {

    mut_lock(qu->mut);
    node_t *node = qu->root;

    while(node) {
        printf("(%d, %d) %c ", node->val.x, node->val.y, 
            node->next ? "<>"[VAL_GT(node->val, node->next->val)]
            : '|');
        node = node->next;
    }
    // printf("\n");
    mut_unlock(qu->mut);
}