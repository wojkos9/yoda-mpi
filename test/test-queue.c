#include "../src/queue.h"
#include <stdio.h>

int main() {

    queue_t qu = QUEUE_INIT;

    val_t a[]= {{0, 2}, {0, 0}, {0, 1}};

    for (int i = 0; i < sizeof(a)/sizeof(a[0]); i++) {
        val_t v = a[i];
        int r = qputv(&qu, v);
        printf("%d\n", r);
        qprint(&qu);
    }

    printf("RM %d\n", qrm1(&qu, 2));

    qprint(&qu);

    printf("DEL %d\n", qdel(&qu));

    return 0;
}