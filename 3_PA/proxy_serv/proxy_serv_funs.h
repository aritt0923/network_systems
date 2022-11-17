#ifndef PROXY_SERV_FUNS_H_
#define PROXY_SERV_FUNS_H_

#include <stdio.h>
#include <string.h> //strlen

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h> //inet_addr
#include <sys/stat.h>

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <fcntl.h>
#include <unistd.h>    //write
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h> //for threading , link with lpthread
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>

#include <time.h>
#include <cache.h>


#define KILO 1024
#define MAX_MSG_SZ KILO*2

#define MAX_REQ_TYPE_LEN 5
#define MAX_URL_LEN KILO
#define MAX_HOSTNAME_LEN KILO
#define MAX_PORT_NUM_LEN 6
#define MAX_FILETYPE_SIZE 64
#define EVP_MAX_MD_SIZE 64

#define DEFAULT_PORT "80"


typedef struct // arguments for requester threads
{
    hash_table *cache;
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
    char * port_num;
    char * filepath;
    char * query;
    
    int http_v;
    int dynamic;
} req_params;




/* Source: server_funs.c
 * Entry point for all server_funs.c && proxy_funs.c functions
 * Accepts GET request from client and responds appropriately
 */
int handle_get(hash_table *cache, const char *client_req, int sockfd, sem_t *socket_sem);

 
/*
 * Parses the clients request into req_params struct
 * Returns 0 for success
 * Catches:
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
int build_header(req_params *params, struct cache_node *file, char *header_buf);

 
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


/* Source: proxy_funs.c
 * Accepts GET request from client-side
 * Relays request to remote server
 * Creates cache entry for resulting file  
 * Catches: 
 *      returns server error if any
 */
int fetch_remote(hash_table *cache, const char *client_req, 
        req_params *params, struct cache_node *file, int client_sockfd, sem_t *socket_sem);

int send_file_from_cache(hash_table *cache, struct cache_node *file, req_params *params, int sockfd, sem_t *socket_sem);

int connect_remote(int *serv_sockfd, req_params *params);

int parse_header(char * header_packet, struct cache_node *file);

int get_full_filepath(char *filepath, struct cache_node *file);

int check_ttl(hash_table *cache, struct cache_node *file, int ttl_seconds);

double diff_timespec(const struct timespec *time1, const struct timespec *time0);

int check_dynamic(char *url);

int check_blocklist(char *hostname, char * ip_addr);

#endif //PROXY_SERV_FUNS_H_