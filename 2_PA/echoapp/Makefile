CC = gcc
CFLAGS = -Wall -g
LDFLAGS = -lpthread -lnsl -lrt -lsocket

OBJS = nethelp.o echoserver.o echoservert.o echoclient.o 

all: echoserver echoservert echoclient 

nethelp.o: nethelp.c
	$(CC) $(CFLAGS) -c nethelp.c

echoserver.o: echoserver.c
	$(CC) $(CFLAGS) -c echoserver.c

echoservert.o: echoservert.c
	$(CC) $(CFLAGS) -c echoservert.c

echoclient.o: echoclient.c
	$(CC) $(CFLAGS) -c echoclient.c

echoserver: echoserver.o nethelp.o
	$(CC) $(LDFLAGS) -o $@ echoserver.o nethelp.o

echoservert: echoservert.o nethelp.o
	$(CC) $(LDFLAGS) -o $@ echoservert.o nethelp.o

echoclient: echoclient.o nethelp.o
	$(CC) $(LDFLAGS) -o $@ echoclient.o nethelp.o

clean:
	rm -f echoserver echoservert echoclient *.o *~ core
