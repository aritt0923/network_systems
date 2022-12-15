#include "wrappers.h"

ssize_t send_wrap(int sockfd, const void *buf, size_t len, int flags)
{
    ssize_t res;
    if ((res = send(sockfd, buf, len, flags)) == -1)
    {
        perror("[-]Error in sending header.");
        exit(1);
    }
    return res;
}

void *calloc_wrap(size_t num, size_t size)
{
    void *mem_block = calloc(num, size);
    if (mem_block == NULL)
    {
        perror("[-]Error in allocating memory (calloc).");
        exit(1);
    }
    return mem_block;
}

void *malloc_wrap(size_t size)
{
    void *mem_block = malloc(size);
    if (mem_block == NULL)
    {
        perror("[-]Error in allocating memory (calloc).");
        exit(1);
    }
    return mem_block;
}

int pthread_create_wrap(pthread_t *restrict thread,
                   const pthread_attr_t *restrict attr,
                   void *(*start_routine)(void *),
                   void *restrict arg)
{
    int res;
    if ((res = pthread_create(thread, attr, start_routine, arg)) != 0)
    {
        perror("could not create thread");
	    exit (1);
    }
    return res;
}

 int listen_wrap(int sockfd, int backlog)
 {
    int res;
    if((res = listen(sockfd, backlog)) != 0)
    {
        perror("listen failed");
        exit(1);
    }
    return res;
 }

/* 
 * Accept returns error when sockfd is closed by another thread
 * But we want that behavior, so we need to use the function itself
 * in the main code

int accept_wrap(int sockfd, struct sockaddr *restrict addr,
                  socklen_t *restrict addrlen)
{
    int res;
    if ((res = accept(sockfd, addr, addrlen)) < 0)
    {
        perror("accept failed");
        exit(1);
    }
    return res;
}

*/