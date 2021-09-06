/*
 * echoserver.c - An iterative echo server
 */

#include "csapp.h"

#define HOSTLEN 256
#define SERVLEN 8

// Information about a connected client.
typedef struct {
    struct sockaddr_in addr;    // Socket address
    socklen_t addrlen;          // Socket address length
    int connfd;                 // Client connection file descriptor
    char host[HOSTLEN];         // Client host
    char serv[SERVLEN];         // Client service (port)
} client_info;

void *echo(void *arg) {
    client_info *client = arg;
    ssize_t len;
    char buf[MAXLINE];
    rio_t rio;

    Pthread_detach(pthread_self());

    // Initialize RIO read structure
    rio_readinitb(&rio, client->connfd);

    // Get some extra info about the client (hostname/port)
    // This is optional, but it's nice to know who's connected
    Getnameinfo((SA *) &client->addr, client->addrlen,
            client->host, sizeof(client->host),
            client->serv, sizeof(client->serv),
            0);
    printf("Accepted connection from %s:%s\n", client->host, client->serv);

    // Read line by line from client and echo back
    // NOTE: rio_readlineb returns the number of bytes read, or -1 on error
    while((len = rio_readlineb(&rio, buf, MAXLINE)) > 0) {
        printf("%s:%s sent %ld bytes\n", client->host, client->serv, len);

        // Write message back to client
        if (rio_writen(client->connfd, buf, len) != len) {
            fprintf(stderr, "Error writing to client, disconnecting\n");
            break;
        }
    }

    if (len < 0) {
        fprintf(stderr, "Error reading from client, disconnecting\n");
    }

    // Close the socket connection
    Close(client->connfd);

    printf("Disconnected from %s:%s\n", client->host, client->serv);

    Free(client);
    return NULL;
}

int main(int argc, char **argv) {
    int listenfd;
    char *port;

    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = argv[1];

    // Start listening on the given port number
    listenfd = Open_listenfd(port);

    while (1) {
        // The thread we're about to create
        pthread_t thread;

        // Allocate space on the heap for client info
        client_info *client = Malloc(sizeof(*client));

        // Initialize the length of the address
        client->addrlen = sizeof(client->addr);

        // Accept() will block until a client connects to the port
        client->connfd = Accept(listenfd,
                (SA *) &client->addr, &client->addrlen);

        // Connection is established; echo to client
        pthread_create(&thread, NULL, echo, client);
    }
}

