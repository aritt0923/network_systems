#ifndef CLIENT_SERV_COMMON_H_
#define CLIENT_SERV_COMMON_H_

#define LS 10
#define GET 11
#define PUT 12

#define MAX_CMD_LEN 4

#define KILO 1024
#define MAX_FILENAME_LEN 256
#define MAX_FILESIZE_STR 24
#define MAX_IP_LEN KILO
#define MAX_PORT_LEN 5

#define MAX_MSG_SZ KILO*2
#define MAX_CMD_STR_SIZE KILO


#define NUM_SERVERS 4
#define MIN_GET_SERVERS 3
#define MIN_PUT_SERVERS NUM_SERVERS
#define MIN_FILE_LEN 4

/*
 * Server return status codes
 */

#define OK 200          // OK: request is valid
#define BAD_REQ 400     // Bad Request: request could not be parsed or is malformed
#define FBDN 403        // Forbidden: permission issue with file
#define NOTFOUND 404          // Not Found: file not found
#define MNA 405         // Method Not Allowed: a method other than GET was requested
#define HTTP 505        // HTTP Version Not Supported: an http version other than 1.0 or 1.1 was requested
#define BAD_RESP 500    // Response from remote server was wonky 


#include <stdio.h>
#include <string.h> //strlen

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h> //inet_addr
#include <sys/stat.h>

#include <sys/signal.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

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
#include <pwd.h>
#include <time.h>

typedef struct // arguments for server
{
    char * cmd;
    // '1' or '0', do we need a specific version of filename_hr?
    // if this is the first server queried, no
    // otherwise we need the version the first server returned.
    char req_tmstmp;
    char * filename_hr;
    char * filename_md5;
    char * filename_tmstmp_md5;
    // assuming single digit number of servers (always 4 for this assignment)
    char chunk;
    // Name of the chunk as stored
    // i.e <filename_tmstmp_md5>_<chunk>
    char * chunk_name;

    char * offset;
    char * chunk_size;
    char * filesize;
} df_serv_cmd;


#endif //CLIENT_SERVER_COMMON_H_
