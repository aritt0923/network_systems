#ifndef SERVER_FUNS_H_
#define SERVER_FUNS_H_

#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdlib.h>
#include <errno.h>

#include <wrappers.h>
#include <utilities.h>
#include <status_codes.h>


#define KILO 1024
#define MAX_CLNT_MSG_SZ 2000


int handle_get(const char *client_req, int sockfd);
 
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
int send_response(FILE *fp, char * header, int sockfd);

/*
 * Sends specified error to client
 */
int send_error(int error, int http_v, int sockfd);


#endif //SERVER_FUNS_H_