#include "csapp.h"
#include "sbuf.h"
#define NTHEAD   4
#define SBUFSIZE 16

void echo_cnt(int connfd);
void *thread(void *vargp);

sbuf_t sbuf;  /* buffer for connected fd */

int main(int argc, char **argv)
{
    int i, listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);

    sbuf_init(&sbuf, SBUFSIZE);
    // create threads
    for (i=0; i<NTHEAD; i++)
    {
        Pthread_create(&tid, NULL, thread, NULL);
    }

    while (1) 
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *) &clientaddr, &clientlen);
        sbuf_insert(&sbuf, connfd);
    }
}


void *thread(void *vargp)
{
    Pthread_detach(pthread_self()); // reap by kernel not main thread
    while (1)
    {
        // pick something to serve
        int connfd = sbuf_remove(&sbuf); /* Remove connfd from buffer */ //line:conc:pre:removeconnfd
        echo_cnt(connfd);                                                /* Service client */
        Close(connfd);
    }
}