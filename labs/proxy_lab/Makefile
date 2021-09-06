CC = gcc
CFLAGS = -O3 -pthread -Wall -Wextra -Werror
LDFLAGS = $(CFLAGS)
OBJECTS = csapp.o echoclient.o echoserver.o echoserver_t.o
PROJECTS = echoclient echoserver echoserver_t

all: $(OBJECTS) $(PROJECTS)

echoclient: csapp.o echoclient.o

echoserver: csapp.o echoserver.o

echoserver_t: csapp.o echoserver_t.o

clean:
	rm -f $(PROJECTS) $(OBJECTS)
