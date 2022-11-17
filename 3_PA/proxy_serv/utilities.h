#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <stdio.h>
#include <ctype.h>
#include <string.h> //strlen

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h> //inet_addr

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <unistd.h>    //write
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h> //for threading , link with lpthread
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <proxy_serv_funs.h>


// https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

int str_to_lower(char * str, int len);

int get_file_size(FILE *fileptr);

// https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
str2int_errno str2int(int *out, char *s);

int md5_str(char * url, char * hash_res_buf);

void *get_in_addr(struct sockaddr *sa);

FILE * open_file_from_cache_node_wr(struct cache_node *file);


FILE * open_file_from_cache_node_rd(struct cache_node *file);
 
int get_md5_str(char *md5_str_buf, req_params *params);



/* Source: utilities.c
 * Callocs buffers and initializes data
 * for the passed req_params struct
 */
int init_params(req_params *params);


/* Source: utilities.c
 * Frees allocated memory
 * for the passed params struct
 */
int free_params(req_params *params);


#endif // UTILITIES_H_