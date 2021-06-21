#include "csapp.h"

void *thread(void *vargp);

int main()
{
    pthread_t tid;
    Pthread_create(&tid, NULL, thread, NULL);
    Pthread_join(tid, NULL);
    exit(0);
}


void *thread(void *vargp)
{
    printf("Hello thread!\n");
    return NULL;
}