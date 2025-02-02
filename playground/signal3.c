#include "csapp.h"

void handler2(int sig)
{
    pid_t pid;

    while ((pid=waitpid(-1, NULL, 0)) > 0) 
        printf("Handler reaped child %d\n", pid);
    
    if (errno != ECHILD)
        unix_error("waitpid error");
    Sleep(2);
    return;
}

int main(int argc, char **argv) 
{
    int i, n;
    char buf[MAXBUF];
    pid_t pid;

    if (signal(SIGCHLD, handler2) == SIG_ERR)
        unix_error("signal error.");

    for (i=0; i<3; i++) {
        pid = Fork();
        if (pid == 0) {
            printf("Hello from child %d\n", (int)getpid());
            Sleep(1);
            exit(0);
        }
    }

    while ((n = read(STDIN_FILENO, buf, sizeof(buf))) < 0)
        if ( errno != EINTR )
            unix_error("read error");

    printf("Parent processing input:\n");
    while (1)
        ;

    exit(0);
}
