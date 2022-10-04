/*
 * echoservert.c - A concurrent echo server using threads
 */

#include "nethelp.h"

void echo(int connfd);
void *thread(void *vargp);

int main(int argc, char **argv)
{
    int listenfd, *connfdp, port, clientlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientaddr;
    pthread_t tid;

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);

    listenfd = open_listenfd(port);
    while (1)
    {
        connfdp = malloc(sizeof(int));
        *connfdp = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen);
        pthread_create(&tid, NULL, thread, connfdp);
    }
}

/* thread routine */
void *thread(void *vargp)
{
    int connfd = *((int *)vargp);
    pthread_detach(pthread_self());
    free(vargp);
    echo(connfd);
    close(connfd);
    return NULL;
}

/*
 * echo - read and echo text lines until client closes connection
 */
void echo(int connfd)
{
    size_t n;
    char buf[MAXLINE];

    while ((n = readline(connfd, buf, MAXLINE)) != 0)
    {
        printf("server received %d bytes\n", n);
        write(connfd, buf, n);
    }
}
