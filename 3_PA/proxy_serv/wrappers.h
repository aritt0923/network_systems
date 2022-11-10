#ifndef WRAPPERS_H_
#define WRAPPERS_H_


#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdlib.h>
#include <pthread.h>

/*
 * Declares wrapper functions with error checking
 */

void *calloc_wrap(size_t num, size_t size);

void *malloc_wrap(size_t size);

ssize_t send_wrap(int sockfd, const void *buf, size_t len, int flags);

int pthread_create_wrap(pthread_t *restrict thread,
                   const pthread_attr_t *restrict attr,
                   void *(*start_routine)(void *),
                   void *restrict arg);
                   
                   
int listen_wrap(int sockfd, int backlog);
                   
/*            
    
int accept_wrap(int sockfd, struct sockaddr *restrict addr,
                  socklen_t *restrict addrlen);
                  
*/

#endif // WRAPPERS_H_