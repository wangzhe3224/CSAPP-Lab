#include "csapp.h"

static readcnt = 0;
sem_t mutex, w;

void reader()
{
    while (1)
    {
        P(&mutex);
        readcnt++;
        if (readcnt == 1) 
            /* 只要有一个reader在等待，就尝试获得写入锁 
             * 这时，如果有正在进行的写入线程获得锁，read阻塞
             */
            P(&w);
        V(&mutex);

        /* Critical Section: read */ 

        P(&mutex);
        readcnt--;
        if (readcnt == 0)
            /* 只有没有reader在等待才会释放写入锁 */
            V(&w);
        V(&mutex);
    }
}

void writer()
{
    while(1)
    {
        P(&w);

        V(&w);
    }
}