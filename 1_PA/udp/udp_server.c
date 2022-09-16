/*
 * udpserver.c - A simple UDP echo client
 * usage: udpclient <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

struct send_rec_args
{
    struct sockadd_in *clientaddr;
    int sockfd;
    char *buf;
    int *clientlen;
    int n; //size of message sent or recieved
};

#define BUFSIZE 1024
#define GET 1
#define PUT 2
#define DELETE 3
#define LS 4
#define EXIT 5

int validate_client_no_file(char *buf);
int validate_client_file_op(char *buf, char *filename);
int handle_client_cmd(struct send_rec_args *args);
void strip_newline(char *buf);
void str_to_lower(char *buf);
int split_client_in(char *buf, char *cmd, char *filename);
int cmd_switch(int res, char *filename, struct send_rec_args *args);
int rec_file_from_client(char *filename, struct send_rec_args *args);

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char **argv)
{
    int sockfd;                    /* socket */
    int portno;                    /* port to listen on */
    int clientlen;                 /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp;         /* client host info */
    char buf[BUFSIZE];             /* message buf */
    char *hostaddrp;               /* dotted decimal host addr string */
    int optval;                    /* flag value for setsockopt */
    int n;                         /* message byte size */

    /*
     * check command line arguments
     */
    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        printf("argc is: %d\n argv[0] is: %s\n", argc, argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /*
     * socket: create the parent socket
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *)&optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    // zero out serveraddr
    bzero((char *)&serveraddr, sizeof(serveraddr));
    // IPv4
    serveraddr.sin_family = AF_INET;
    // INADDR_ANY is a placeholder IP used when you don't know the machines IP?
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // port number, from cmd line
    serveraddr.sin_port = htons((unsigned short)portno);

    /*
     * bind: associate the parent socket with a port
     */
    if (bind(sockfd, (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");

    struct send_rec_args args;
    args.buf = buf;
    args.clientaddr = &clientaddr;
    args.clientlen = &clientlen;
    args.sockfd = sockfd;
    /*
     * main loop: wait for a datagram, then echo it
     */
    clientlen = sizeof(clientaddr);
    while (1)
    {
        
        
        /*
         * recvfrom: receive a UDP datagram from a client
         */
        rec_from_client(&args);
        /*
        bzero(buf, BUFSIZE);
        n = recvfrom(sockfd, buf, BUFSIZE, 0,
                     (struct sockaddr *)&clientaddr, &clientlen);
        if (n < 0)
            error("ERROR in recvfrom");
        */

        /*
         * gethostbyaddr: determine who sent the datagram
         */
        hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                              sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
            error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);
        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");
        printf("server received datagram from %s (%s)\n",
               hostp->h_name, hostaddrp);
        printf("server received %ld/%d bytes: %s\n", strlen(buf), n, buf);

        handle_client_cmd(&args);

        /*
         * sendto: echo the input back to the client
         */
        //send_to_client(&args);
        /*
        n = sendto(sockfd, buf, strlen(buf), 0,
                   (struct sockaddr *)&clientaddr, clientlen);
        if (n < 0)
            error("ERROR in sendto");
        */
    }
}

int rec_from_client(struct send_rec_args *args)
{
    bzero(args->buf, BUFSIZE);
    args->n = recvfrom(args->sockfd, args->buf, BUFSIZ, 0, args->clientaddr, args->clientlen);
    if (args->n < 0)
        error("ERROR in recvfrom");
    //printf("Echo from client: %s --END\n", args->buf);
}

int send_to_client(struct send_rec_args *args)
{
    /* send the message to the client */
    struct sockaddr_in *clientaddr = args->clientaddr;
    *args->clientlen = sizeof(*clientaddr);
    args->n = sendto(args->sockfd, args->buf, strlen(args->buf), 0, args->clientaddr, *args->clientlen);
    if (args->n < 0)
        error("ERROR in sendto");
    return *args->clientlen;
}

int split_client_in(char *buf, char *cmd, char *filename)
{ /*
   * split the client command into cmd - filename
   * if no filename is passed, cmd is set equal to buf
   */

    // printf("client in is: %s\n", buf);
    int split_index = -1;
    for (int i = 0; i < strlen(buf); i++)
    {
        if (buf[i] == ' ')
        {
            split_index = i;
            break;
        }
    }
    if (split_index == -1)
    { // no whitespace was found
        strncpy(cmd, buf, strlen(buf));
        return 0;
    }
    strncpy(cmd, buf, split_index);
    cmd[split_index] = '\0';
    printf("Cmd is: %s\n", cmd);

    int filename_ind = 0;
    for (int i = split_index + 1; i < strlen(buf); i++)
    {
        filename[filename_ind] = buf[i];
        filename_ind++;
    }
    printf("Filename is: %s\n", filename);
    return 1;
}

int validate_client_no_file(char *cmd)
{ // No filename was passed - validate that cmd is either ls or exit
    int res = -1;
    if (strcmp("ls", cmd) == 0)
    {
        // printf("Client says 'ls'\n");
        res = LS;
    }
    else if (strcmp("exit", cmd) == 0)
    {
        // printf("Client says 'exit'\n");
        res = EXIT;
    }
    return res;
}

int validate_client_file_op(char *cmd, char *filename)
{ // Filename was passed - validate that cmd is either get, put, or delete

    int res = -1;

    if (strcmp("get", cmd) == 0)
    {
        // printf("Client says 'get'\n");
        res = GET;
    }
    else if (strcmp("put", cmd) == 0)
    {
        // printf("Client says 'put'\n");
        res = PUT;
    }
    else if (strcmp("delete", cmd) == 0)
    {
        // printf("Client says 'delete'\n");
        res = DELETE;
    }
    return res;
}

int handle_client_cmd(struct send_rec_args *args)
{ // process the clients command
    strip_newline(args->buf);
    str_to_lower(args->buf);

    char *cmd = calloc(strlen(args->buf), sizeof(char));
    char *filename = calloc(strlen(args->buf), sizeof(char));
    int rec_filename = 0;
    if ((rec_filename = split_client_in(args->buf, cmd, filename)) < 0)
    { // error with split_client_in
        return -1;
    }
    // validate the clients commands
    int res = -1;
    if (rec_filename)
    {
        if ((res = validate_client_file_op(cmd, filename)) < 0)
        {
            fprintf(stderr, "Error with validate_client_file_op\n");
            return -1;
        }
    }
    else
    {
        if ((res = validate_client_no_file(cmd)) < 0)
        {
            fprintf(stderr, "Error with validate_client_no_file\n");
            return -1;
        }
    }
    return (cmd_switch(res, filename, args));
}

int cmd_switch(int res, char *filename, struct send_rec_args *args)
{
    switch (res)
    {

    case GET:
        
        break; 

    case PUT:
        rec_file_from_client(filename, args);
        break; 
    
    case DELETE:
        break; 
    
    case LS:
        break; 
        
    case EXIT:
        break; 

    /* you can have any number of case statements */
    default: /* Optional */
        fprintf(stderr, "Unrecognized cmd passed to cmd_switch\n");
        return -1;
    }
}

int rec_file_from_client(char *filename, struct send_rec_args *args)
{
    printf("in rec_from_client\n");
    strncpy(args->buf, "SENDSIZE", strlen("SENDSIZE"));
    send_to_client(args);
    
}

void strip_newline(char *buf)
{
    buf[strcspn(buf, "\n")] = '\0';
}

void str_to_lower(char *buf)
{
    for (int i = 0; buf[i]; i++)
    {
        buf[i] = tolower(buf[i]);
    }
}

