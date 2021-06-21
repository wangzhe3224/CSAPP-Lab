#include "csapp.h"

/* define a pool of connected descriptors */
typedef struct {
    int maxfd;  // largest fd in set
    fd_set read_set;  // all active descriptors
    fd_set ready_set;  // subset of descriptors that ready to be read
    int nready;  // number of ready descriptors from select
    int maxi;    // highwater index into client array
    int clientfd[FD_SETSIZE];     // set of active descriptors, default 1024
    rio_t clientrio[FD_SETSIZE];  // set of active read buffers
} pool;

void init_pool(int listenfd, pool *p);
void add_client(int connfd, pool *p);
void check_clients(pool *p);

int byte_cnt = 0;  // counts total bytes received by server

int main(int argc, char **argv)
{
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;
    static pool pool;  // private variable

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);

    // server loop
    while (1)
    {
        // why get ready set and read_set
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        if (FD_ISSET(listenfd, &pool.ready_set)) 
        {
            clientlen = sizeof(struct sockaddr_storage);
            connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
            add_client(connfd, &pool);  // add client into pool to be handled
        }

        check_clients(&pool);
    }
}


void init_pool(int listenfd, pool *p)
{
    int i;
    // initialized to -1 
    p->maxi = -1;
    for (i=0; i<FD_SETSIZE; i++)
        p->clientfd[i] = -1;

    // initially, listenfd is the only fd of select read set
    p->maxfd = listenfd;
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool *p)
{
    int i;
    p->nready--; // ? we didn't initialize this ...
    for (i = 0; i < FD_SETSIZE; i++)
    {
        if (p->clientfd[i] < 0)
        {
            // assign fd and its buffer
            p->clientfd[i] = connfd;
            Rio_readinitb(&p->clientrio[i], connfd);

            // Turn on fd
            FD_SET(connfd, &p->read_set);

            // update pool status
            if (connfd > p->maxfd)
                p->maxfd = connfd;
            if (i > p->maxi)
                p->maxi = i;
            break;  // find an available slot
        }
    } 

    if (i == FD_SETSIZE)
        app_error("add_client error: Too many clients. Run out of descriptor");
}


void check_clients(pool *p)
{
    int i, connfd, n;
    char buf[MAXLINE];
    rio_t rio;

    for (i=0; (i <= p->maxi) && (p->nready > 0); i++) {
        connfd = p->clientfd[i];
        rio = p->clientrio[i];

        if ((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))) {
            p->nready--;
            if ((n=Rio_readlineb(&rio, buf, MAXLINE)) != 0) {
                byte_cnt += n;
                printf("Server received %d (%d total) bytes on fd %d\n",
                       n, byte_cnt, connfd);
                Rio_writen(connfd, buf, n);  // echo back the received info..
            }
            else
            {
                // EOF detected, remove fd from pool
                Close(connfd);
                FD_CLR(connfd, &p->read_set);
                p->clientfd[i] = -1; // do we need clear related io buffer?
            }
        }
    }

    // report pool
    printf("Pool:\n    maxfd: %d\n    nready: %d\n    maxi: %d\n",
           p->maxfd, p->nready, p->maxi);
}