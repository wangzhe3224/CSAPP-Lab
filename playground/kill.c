#include "csapp.h"

int main()
{
    pid_t pid;

    if ((pid = Fork()) == 0) {
        Pause();  // wait signal, blocking...
        printf("Never reach here.");
        exit(0);
    }

    Kill(pid, SIGKILL);
    exit(0);
}
