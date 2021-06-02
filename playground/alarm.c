#include "csapp.h"

void handler(int sig)
{
    static int beeps = 0;

    printf("BEEP\n");

    if (++beeps < 5) 
        alarm(1);

    else {
        printf("BOOM!\n");
        exit(0);
    }
}

int main()
{
    signal(SIGALRM, handler);
    alarm(1);  // arange kernel to send alarm to this process 

    while (1) {
        // signal hander will return to here, which is the next instruction
        // after the interupt
    }
    printf("Never reach here. Because program exit in the handler...\n");
    exit(0);
}
