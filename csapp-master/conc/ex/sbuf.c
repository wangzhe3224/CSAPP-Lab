#include "csapp.h"
#include "sbuf.h"

/**
 * @brief Create an empty, bounded, shared FIFO buffer with n slots
 * 
 * @param sp 
 * @param n 
 */
void sbuf_init(sbuf_t *sp, int n)
{
    // Heap allocation dynamic
    sp->buf = Calloc(n, sizeof(int));
    sp->n = n;
    sp->front = sp->rear = 0;
    // Sem will make sure inc/dec or P/V is atomic action.
    Sem_init(&sp->mutex, 0, 1);
    Sem_init(&sp->items, 0, 0);
    Sem_init(&sp->slots, 0, n);
}

void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots);  // wait for slot, if slots > 0, dec it. else wait
    P(&sp->mutex);  // lock buffer
    sp->buf[(++sp->rear) % (sp->n)] = item;
    V(&sp->mutex);  // release buffer
    V(&sp->items);  // inc item in the buffer
}

int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);  // wait for item
    P(&sp->mutex);  // lock buffer
    item = sp->buf[(++sp->front) % (sp->n)];
    V(&sp->mutex);  // unlock the buffer
    V(&sp->slots);  // update slot
}