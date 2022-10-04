/*
 * echoserver.c - A sequential echo server
 */

#include "nethelp.h"

void echo(int connfd);

int main(int argc, char **argv) 
{
    int listenfd, connfd, port, clientlen=sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1) {
	connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clientlen);
	echo(connfd);    /* Service client */
	close(connfd);   /* Close connection with client */
    }
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd) 
{
    size_t n; 
    char buf[MAXLINE]; 

    while((n = readline(connfd, buf, MAXLINE)) != 0) {
	printf("server received %d bytes\n", n);
	write(connfd, buf, n);
    }
}

