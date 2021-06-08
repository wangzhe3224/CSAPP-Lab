/**
 * @file tiny.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-06-08
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include <sys/socket.h>
#include "rio.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_url(char *url, char *filename, char *cgiargs);
void serve_static(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void client_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
}