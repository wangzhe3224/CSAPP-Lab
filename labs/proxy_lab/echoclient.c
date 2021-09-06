/*
 * echoclient.c - An echo client
 */

#include "csapp.h"

int main(int argc, char **argv) {
    int clientfd;
    char *host, *port, buf[MAXLINE];
    rio_t rio;

    if (argc != 3) {
        fprintf(stderr, "usage: %s <host> <port>\n", argv[0]);
        exit(0);
    }

    host = argv[1];
    port = argv[2];

    // Open socket connection to server
    if ((clientfd = open_clientfd(host, port)) < 0) {
        fprintf(stderr, "Error connecting to %s:%s\n", host, port);
        exit(-1);
    }

    // Initialize RIO read structure
    rio_readinitb(&rio, clientfd);

    // Read a line from stdin
    while (fgets(buf, MAXLINE, stdin) != NULL) {
        // Write line to server
        if (rio_writen(clientfd, buf, strlen(buf)) < 0) {
            fprintf(stderr, "Error writing to server\n");
            break;
        }

        // Read back response (up to MAXLINE characters)
        if (rio_readlineb(&rio, buf, MAXLINE) < 0) {
            fprintf(stderr, "Error reading from server\n");
            break;
        }

        // Print response from server
        printf("%s", buf);
    }

    // Close the socket connection when done
    Close(clientfd);
    exit(0);
}
