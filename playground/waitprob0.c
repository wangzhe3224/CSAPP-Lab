#include "csapp.h"

int main() {
    if (Fork() == 0) {
        // Only child process goes here.
        printf("a");
    }
    else {
        printf("b");
        // pid = -1 means wait set consists all child processes
        waitpid(-1, NULL, 0);
    }
    printf("c");
    exit(0);
}
