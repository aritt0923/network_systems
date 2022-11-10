#ifndef PROXY_SERV_FUNS_H_
#define PROXY_SERV_FUNS_H_

#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h> //for threading , link with lpthread
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <wrappers.h>
#include <utilities.h>
#include <status_codes.h>


#define KILO 1024
#define MAX_CLNT_MSG_SZ KILO*2

#define MAX_REQ_TYPE_LEN 5
#define MAX_URL_LEN KILO
#define MAX_HOSTNAME_LEN KILO
#define MAX_HTTP_V_LEN 10
#define EVP_MAX_MD_SIZE 64


typedef struct // arguments for requester threads
{
    int socket_desc;
    sem_t *accept_sem;
    sem_t *socket_sem;
    int thread_id;      
    
} thread_args;

typedef struct 
{
    char * req_type;
    char * url;
    char * hostname;
    
    int http_v;
    int req_type_len;
    int url_len;
    int hostname_len;
    int http_v_len;
} req_params;

typedef struct 
{
    char * url;
    char * md5_hash;
    unsigned int filesize;
    char * filetype;
    int age_sec;
} cache_entry;

/* Source: server_funs.c
 * Entry point for all server_funs.c && proxy_funs.c functions
 * Accepts GET request from client and responds appropriately
 */
int handle_get(const char *client_req, int sockfd, sem_t *socket_sem);
 
/* Source: server_funs.c
 * Parses the clients request, returns 0 for success.
 * Catches:
 *      405: Method Not Allowed
 *      505: HTTP Version Not Supported
 *      400: Bad Request
 */
int parse_req(const char *client_req, req_params *params);         
            
/* Source: server_funs.c
 * Grabs the filetype from the filename
 * Catches:
 *      400: Bad Request
 */
int get_filetype(char * filename, char* filetype_buf);
            
/* Source: server_funs.c
 * builds header for server response
 */
int build_header(req_params *params, char* header_buf);

/* Source: server_funs.c
 * Sends file pointed to by FP to client
 * Catches:
 *      404: File Not Found
 *      403: Forbidden
 */
int send_file(FILE *fp, char * header, int sockfd, sem_t *socket_sem);

/* Source: server_funs.c
 * Sends specified error to client
 */
int send_error(int error, req_params *params, int sockfd, sem_t *socket_sem);

/* Source: server_funs.c (maybe move to utilities)
 * Joins an array of threads
 */
void join_threads(int num_threads, pthread_t *thr_arr);


/* Source: proxy_serv.c
 * Thread function
 * Closes socket, breaking hanging accept() function 
 * For clean exit
 */
void *socket_closer(void *vargs);


/*
 * Accepts GET request from client-side
 * Relays request to remote server
 * Creates cache entry for resulting file  
 * Catches: 
 *      403: Forbidden
 */
int fetch_remote(const char* url, int http_v, const char *host);

int req_params_init(req_params *params);

int free_params(req_params *params);

int send_from_cache(req_params *params, char * md5_hash, int sockfd, sem_t *socket_sem);

bool check_cache(char * md5_hash);


#endif //PROXY_SERV_FUNS_H_