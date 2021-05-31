#include "csapp.h"
#include <sys/wait.h>
#define N 2

int main() 
{
    int status, i;
    pid_t pid[N], retpid;

    // create N child processes 
    for (i=0; i<N; i++) {
        if ((pid[i] = Fork()) == 0) {
            exit(100+i);  // exit immediatly
        }
    }

    // wait all children to finish
    // when all child processes are reaped, waitpid == -1, exit loop
    i = 0;
    while ((retpid=waitpid(pid[i++], &status, 0)) > 0) {
        if (WIFEXITED(status)) 
            printf("child %d terminated normally with exit status=%d\n", 
                    retpid , WEXITSTATUS(status));
        else 
            printf("child %d terminated abnormally\n", pid[i]);
        
    }

    if (errno != ECHILD)
        unix_error("waitpid error");

    exit(0);
}
