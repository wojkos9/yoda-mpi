#include <stdio.h>
#include <stdlib.h>

#include "queue.h"

#define VAL_GT(v1, v2) (v1.x > v2.x || v1.x == v2.x && v1.y > v2.y)

int qput(queue_t *qu, int x, int y) {
    val_t v = {x, y};
    qputv(qu, v);
}

int qputv(queue_t *qu, val_t v) {
    node_t *node = qu->root;
    int pos = 1;

    node_t *next = (node_t *) malloc(sizeof(node_t));
    next->val = v;

    if (!node || !VAL_GT(v, node->val)) {
        next->next = qu->root;
        qu->root = next;
        return 0;
    }

    while (node->next && VAL_GT(v, node->next->val)) {
        node = node->next;
        ++pos;
    }
    
    next->next = node->next;
    node->next = next;
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
    return c;
}

int qrm1(queue_t *qu, int y) {
    node_t *node = qu->root;

    if (node && node->val.y == y) {
        node_t *t = qu->root;
        qu->root = node->next;
        free(t);
        return 0;
    }

    int c = 0;

    while(node->next) {
        ++c;
        if (node->next->val.y == y) {
            node_t *t = node->next;
            node->next = node->next->next;
            free(t);
            return c;
        }
        
        node = node->next;
    }
    return -1;
}

void qprint(queue_t *qu) {
    node_t *node = qu->root;

    while(node) {
        printf("(%d, %d) %c ", node->val.x, node->val.y, 
            node->next ? "<>"[VAL_GT(node->val, node->next->val)]
            : '|');
        node = node->next;
    }
    printf("\n");
}