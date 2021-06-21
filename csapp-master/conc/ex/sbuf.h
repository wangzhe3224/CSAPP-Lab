#ifndef __SBUF_H__
#define __SBUF_H__

#include "csapp.h"

typedef struct
{
    int *buf;
    int n;
    int front;   /* buf[(front+1)%n] is first item */
    int rear;    /* buf[rear%n] is last item */
    sem_t mutex; /* Protect access to buf */
    sem_t slots; /* Counts available slots */
    sem_t items; /* Counts available items */
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int subf_remove(sbuf_t *sp);

#endif