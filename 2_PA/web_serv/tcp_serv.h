#ifndef SERVER_FUNS_H_
#define SERVER_FUNS_H_

#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h> //for threading , link with lpthread
#include <semaphore.h>
#include <signal.h>

#include <wrappers.h>
#include <utilities.h>
#include <status_codes.h>


#define KILO 1024
#define MAX_CLNT_MSG_SZ 2000


typedef struct // arguments for requester threads
{
    int socket_desc;
    sem_t *accept_sem;
    sem_t *socket_sem;
    int thread_id;      
    
} thread_args;


int handle_get(const char *client_req, int sockfd, sem_t *socket_sem);
 
/*
 * Parses the clients request, returns 0 for success.
 * Catches:
 *      405: Method Not Allowed
 *      505: HTTP Version Not Supported
 *      400: Bad Request
 */
int parse_req(const char *client_req, char *filename_buf,
            char * filetype_buf, int * http_v);         
            
/*
 * Grabs the filetype from the filename
 * Catches:
 *      400: Bad Request
 */
int get_filetype(char * filename, char* filetype_buf);
            
/*
 * builds header for server response
 */
int build_header(char * filetype, char* header_buf, int http_v, int filesize);

/*
 * Sends file pointed to by FP to client
 * Catches:
 *      404: File Not Found
 *      405: Forbidden
 */
int send_response(FILE *fp, char * header, int sockfd, sem_t *socket_sem);

/*
 * Sends specified error to client
 */
int send_error(char *header_buf, int error, int http_v, int sockfd, sem_t *socket_sem);


void join_threads(int num_threads, pthread_t *thr_arr);



void *socket_closer(void *vargs);



#endif //SERVER_FUNS_H_