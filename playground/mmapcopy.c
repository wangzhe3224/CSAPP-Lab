#include "csapp.h"

void mapcopy(int fd, int size) 
{
    char *bufp;
    // mmap will ask the kernel to link a VM area with given fd.
    bufp = Mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
    // Here we are writing from VM to std 1 
    Write(1, bufp, size);
    return;
}

int main(int argc, char **argv)
{
    struct stat stat;
    int fd;

    if (argc == 1) {
        printf("Use: mapcopy <filename>\n");
        exit(0);
    }

    fd = Open(argv[1], O_RDONLY, 0);
    fstat(fd, &stat);
    mapcopy(fd, stat.st_size);
    exit(0);
}
