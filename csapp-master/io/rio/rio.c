/**
 * @file rio.c
 * @author zhe wang (wangzhetju@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-06-05
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "rio.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

/**
 * @brief Robustly read n bytes (unbufferred)
 * 
 * @param fd 
 * @param usrbuf 
 * @param n 
 * @return ssize_t 
 */
ssize_t rio_readn(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf; // create a new point to manipulate user buffer

    while (nleft > 0)
    {
        if ((nread = read(fd, bufp, nleft)) < 0)
        {
            // something wrong branch
            if (errno == EINTR)
            {
                // errno will get updated by the kernel
                // get interrupted, reset readn to read again later
                nread = 0;
            }
            else
            {
                // if it is other reason failure, return -1 for failure
                return -1;
            }
        }
        else if (nread == 0)
            break; /* EOF */

        nleft -= nread;
        bufp += nread;
    }

    return (n - nleft);
}

/**
 * @brief Robustly write n bytes (unbuffered)
 * 
 * @param fd 
 * @param usrbuf 
 * @param n 
 * @return ssize_t 
 */
ssize_t rio_writen(int fd, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        if ((nwritten = write(fd, bufp, nleft)) <= 0)
        {
            if (errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }

        nleft -= nwritten;
        bufp += nwritten;
    }

    return n-nleft;
}

/**
 * @brief Associate a file descriptor with a read buffer and reset buffer 
 * 
 * @param rp 
 * @param fd 
 */
void rio_readinitb(rio_t *rp, int fd) 
{
    rp->rio_fd = fd;
    rp->rio_cnt = 0;
    rp->rio_bufptr = rp->rio_buf;
}


/**
 * @brief 
 * 
 * @param rp 
 * @param usrbuf 
 * @param n 
 * @return ssize_t , -1 for failed, 0 for EOF, positive bytes of read
 */
static ssize_t rio_read(rio_t *rp, char *usrbuf, size_t n)
{
    int cnt;

    /* if buffer empty, refill buffer with read */
    while (rp->rio_cnt <= 0)
    {
        rp->rio_cnt = read(rp->rio_fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->rio_cnt < 0) {
            /**
             * read failed, handle interrupt case only
             * if interrupted, do nothing go to next loop
             */
            if (errno != EINTR) 
                return -1;
        }
        else if (rp->rio_cnt == 0) // EOF
            return 0;
        else
            rp->rio_bufptr = rp->rio_buf;  // reset buffer pointer
    }

    // copy min(n, rp->rio_cnt) bytes from internal buffer to user buffer
    cnt = n;
    if (rp->rio_cnt < n)
        cnt = rp->rio_cnt;

    memcpy(usrbuf, rp->rio_bufptr, cnt);
    rp->rio_bufptr += cnt;
    rp->rio_cnt -=cnt;
    return cnt;
}

/**
 * @brief 
 * 
 * @param rp 
 * @param usrbuf 
 * @param n 
 * @return ssize_t 
 */
ssize_t rio_readnb(rio_t *rp, void *usrbuf, size_t n)
{
    size_t nleft = n;
    ssize_t nread;
    char *bufp = usrbuf;

    while (nleft > 0)
    {
        // rio_read handle interrupt and other -1 cases
        if ((nread = rio_read(rp, bufp, nleft) < 0))
            return -1;
        else if (nread == 0)
            break; // EOF

        nleft -= nread;
        bufp += nread;
    }
    return (n - nleft);
}


/**
 * @brief read a text line ( buffered )
 * 
 * @param tp 
 * @param usrbuf 
 * @param maxlen 
 * @return ssize_t 
 */
ssize_t rio_readlineb(rio_t *rp, void *usrbuf, size_t maxlen)
{
    int n, rc;
    char c, *bufp = usrbuf;

    for (n = 1; n < maxlen; n++)
    {
        if ((rc = rio_read(rp, &c, 1)) == 1)
        {
            *bufp++ = c;
            if (c == '\n')
            {
                n++;
                break;
            }
        }
        else if (rc == 0)  // EOF
        {
            if (n == 1)
                return 0;  // no data read yet
            else
                break;   // some data read already
        }
        else 
            return -1;  // error
    }
    *bufp = 0;  // append '\0' at the end of char buffer
    return n-1;
}