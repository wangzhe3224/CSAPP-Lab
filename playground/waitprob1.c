#include "csapp.h"
#include <sys/wait.h>

int main()
{
    int status;
    pid_t pid; 

    printf("Hello\n");
    pid = Fork();
    printf("%d\n", !pid);  // make it 0 

    if (pid != 0) {
        // only parent process goes here
        if (( pid = waitpid(-1, &status, 0) ) > 0) {
            if (WIFEXITED(status) != 0) {
                printf("%d\n", WEXITSTATUS(status));
            }
        }
    }

    printf("Bye\n");
    exit(2);
}
