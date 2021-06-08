#include <stdio.h>

#define RIO_BUFFERSIZE 8192
typedef struct {
    int rio_fd;       /* 打开的文件标识符 */
    int rio_cnt;      /* Unread bytes in internal buf */
    char *rio_bufptr; /* Next undread bytes pointer */
    char rio_buf[RIO_BUFFERSIZE]; /* internal buffer */
} rio_t;

/* RIO APIs */
// unbuffer API
ssize_t rio_readn(int fd, void *usrbuf, size_t n);
ssize_t rio_writen(int fd, void *usrbuf, size_t n);
// buffer read
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n);
ssize_t rio_readlinenb(rio_t *rp, void *usrbuf, size_t n);
