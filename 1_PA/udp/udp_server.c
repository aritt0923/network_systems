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
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>

struct send_rec_args
{
    struct sockadd_in *clientaddr;
    int sockfd;
    char *buf;
    int *clientlen;
    int n; // size of message sent or recieved
};

#define BUFSIZE 1024
#define GET 1
#define PUT 2
#define DELETE 3
#define LS 4
#define EXIT 5
#define TIMEOUT -3

int validate_client_no_file(char *buf);
int validate_client_file_op(char *buf, char *filename);
int handle_client_cmd(struct send_rec_args *args);
void strip_newline(char *buf);
void str_to_lower(char *buf);
int split_client_in(char *buf, char *cmd, char *filename);
int cmd_switch(int res, char *filename, struct send_rec_args *args);
int rec_file_from_client(char *filename, struct send_rec_args *args);
int parse_rec_filesize(struct send_rec_args *args);
bool parseLong(const char *str, long *val);
char **calloc_2d(int filesize);
int fill_buffer_from_client(char **file_buffer_2d, int filesize, struct send_rec_args *args, int *rows_rec);
int rec_from_client(struct send_rec_args *args);
int get_file_size(FILE *fileptr);
int get_num_rows(int filesize);
int rec_from_client_tmout(struct send_rec_args *args);
int get_row_num(char *buf, int *row_num, int *total_rows);
int copy_row(char *src, char *dest);
int resend_rows(int *rows_rec, int num_rows, struct send_rec_args *args);
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
        // send_to_client(&args);
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
    args->n = recvfrom(args->sockfd, args->buf, BUFSIZE, 0, args->clientaddr, args->clientlen);
    if (args->n < 0)
        error("ERROR in recvfrom");
    // printf("Echo from client: %s --END\n", args->buf);
}

int rec_from_client_tmout(struct send_rec_args *args)
{
    // set timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100000;
    if (setsockopt(args->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error");
    }
    bzero(args->buf, BUFSIZE);
    args->n = recvfrom(args->sockfd, args->buf, BUFSIZE, 0, args->clientaddr, args->clientlen);
    if (args->n < 0)
    {
        if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == ETIMEDOUT))
        {
            return TIMEOUT;
        }
        else
        {
            return -1;
            error("Error in recvfrom");
        }
    }
    // remove timeout
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    if (setsockopt(args->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error");
    }
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
    // printf("Filename is: %s\n", filename);
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
    memset(args->buf, '\0', BUFSIZE);
    strncpy(args->buf, "SENDSIZE", strlen("SENDSIZE"));
    // tell the client to send the size of the file
    send_to_client(args);
    // expect "FILESIZE [filesize]" in return
    rec_from_client(args);
    if (strncmp("FILESIZE", args->buf, strlen("FILESIZE")) != 0)
    {
        fprintf(stderr, "Did not receive filesize from client\n");
        return -1;
    }
    int filesize = parse_rec_filesize(args);
    if (filesize <= 0)
    {
        fprintf(stderr, "Error getting filesize from client\n");
        return -1;
    }
    char **file_buffer_2d = calloc_2d(filesize);
    memset(args->buf, '\0', BUFSIZE);
    strncpy(args->buf, "SENDFILE", strlen("SENDFILE"));

    int num_rows = get_num_rows(filesize);
    int rows_rec[num_rows];
    memset(rows_rec, 0, num_rows * sizeof(int));

    // tell the client to send the file
    send_to_client(args);
    // start recieving packets
    fill_buffer_from_client(file_buffer_2d, filesize, args, rows_rec);
    // tell client which ones we missed
    int resend_res;
    while ((resend_res = resend_rows(rows_rec, num_rows, args)) != 0)
    {
        if (resend_res < 0)
        {
            fprintf(stderr, "Error resending rows \n");
            return -1;
        }
        fill_buffer_from_client(file_buffer_2d, filesize, args, rows_rec);
    }
    memset(args->buf, '\0', BUFSIZE);
    strncpy(args->buf, "SENDSIZE", strlen("SENDSIZE"));
    send_to_client(args);
    bin_to_file_2d("server_rec_file", file_buffer_2d, filesize);
}

int resend_rows(int *rows_rec, int num_rows, struct send_rec_args *args)
{
    bool resend_again = 0;
    for (int i = 0; i < num_rows; i++)
    {
        int send_res;
        if (rows_rec[i] != 1)
        {
            resend_again = 1;
            memset(args->buf, '\0', BUFSIZE);
            strncpy("RESEND ", args->buf, strlen("RESEND "));
            // get number of chars needed to hold row num as string
            // alloc the string
            if (i > 999999999)
            {
                fprintf(stderr, "filesize way to big\n");
                return -1;
            }
            char row_num_str[10];
            // copy it over
            sprintf(row_num_str, "%d", i);
            strncat(args->buf, row_num_str, strlen(row_num_str));
            send_to_client(args);
        }
    }
    return resend_again;
}

int fill_buffer_from_client(char **file_buffer_2d, int filesize, struct send_rec_args *args, int *rows_rec)
{

    int res = 0;
    int num_rows = get_num_rows(filesize);
    while ((res = rec_from_client_tmout(args)) != TIMEOUT)
    {
        if (strncmp("ROW ", args->buf, strlen("ROW ")) != 0)
        {
            fprintf(stderr, "In fill_buffer_from_client - Error with pckt recieved\n");
            return -1;
        }
        char *str = args->buf;
        int row_num = -1;
        int total_rows = -1;
        get_row_num(&str[strlen("ROW ")], &row_num, &total_rows);
        if (row_num == -1 || (row_num > num_rows))
        {
            fprintf(stderr, "in fill_buffer_from_client - error recieving row information\n");
            return -1;
        }
        if (num_rows != total_rows)
        {
            fprintf(stderr, "in fill_buffer_from_client - error with row %d - wrong number of rows\n", row_num);
            return -1;
        }
        rows_rec[row_num] = 1;
        copy_row(args->buf, file_buffer_2d[row_num]);
    }

    return TIMEOUT;
}

int copy_row(char *src, char *dest)
{
    for (int i = 0; i < BUFSIZE; i++)
    {
        dest[i] = src[i];
    }
}

int get_row_num(char *buf, int *row_num, int *total_rows)
{
    char total_rows_str[BUFSIZE];
    char row_num_str[BUFSIZE];
    if (split_client_in(buf, row_num_str, total_rows_str) < 0)
    {
        fprintf(stderr, "In get_row_num: Error splitting client input\n");
        return -1;
    }
    if (!parseLong(row_num_str, row_num))
    {
        fprintf(stderr, "In get_row_num: Error with parseLong\n");
        return -1;
    }
    if (!parseLong(total_rows_str, total_rows))
    {
        fprintf(stderr, "In get_row_num: Error with parseLong\n");
        return -1;
    }
}

int bin_to_file_2d(char *dest_filename, char **file_buffer_2d, int filesize)
{
    FILE *dest_fileptr = fopen(dest_filename, "w");
    if (dest_fileptr == NULL)
    {
        fprintf(stderr, "Error opening dest file.\n");
        return -1;
    }
    int rows = get_num_rows(filesize);

    int bytes_written = 0;
    int bytes_remaining = filesize;
    for (int i = 0; i < rows; i++)
    {
        int n_written = 0;
        const char *ptr = &file_buffer_2d[i][16];

        if ((BUFSIZE - 16) > bytes_remaining)
        {
            n_written = fwrite(ptr, sizeof(char), bytes_remaining, dest_fileptr);
            break;
        }
        n_written = fwrite(ptr, sizeof(char), BUFSIZE - 16, dest_fileptr);
        bytes_written += n_written;
        bytes_remaining -= n_written;
    }
}

int free_2d(char **arr, int filesize)
{
    int rows = get_num_rows(filesize);
    for (int i = 0; i < rows; i++)
    {
        free(arr[i]);
    }
    free(arr);
}

char **calloc_2d(int filesize)
{
    int rows = get_num_rows(filesize);

    char **arr = (char **)calloc(rows, sizeof(char *));
    for (int i = 0; i < rows; i++)
        arr[i] = (char *)calloc(BUFSIZE, sizeof(char));

    return arr;
}

char **read_file_to_buf(FILE *fileptr)
{
    int filesize = get_file_size(fileptr);
    char *file_buffer_1d = (char *)calloc(filesize, sizeof(char));
    fread(file_buffer_1d, filesize, 1, fileptr); // Read in the entire file

    char **file_buffer_2d = calloc_2d(filesize);
    int rows = get_num_rows(filesize);
    printf("NUM ROWS: %d\n", rows);
    printf("FILESIZE %d\n", filesize);

    int ind_1d = 0;
    for (int i = 0; i < rows; i++)
    {
        for (int j = 16; j < BUFSIZE; j++)
        {
            if (ind_1d == filesize)
            {
                // make sure the outer loop breaks
                i = rows;
                break;
            }
            file_buffer_2d[i][j] = file_buffer_1d[ind_1d];
            ind_1d++;
        }
    }
    free(file_buffer_1d);
    return file_buffer_2d;
}

int get_file_size(FILE *fileptr)
{
    fseek(fileptr, 0, SEEK_END);  // Jump to the end of the file
    int filelen = ftell(fileptr); // Get the current byte offset in the file
    rewind(fileptr);              // Jump back to the beginning of the file
    return filelen;
}

int parse_rec_filesize(struct send_rec_args *args)
{
    char filesize_str[BUFSIZE];
    char msg_from_client[10];
    split_client_in(args->buf, msg_from_client, filesize_str);
    int filesize;
    bool scs = parseLong(filesize_str, &filesize);
    if (!scs)
    {
        fprintf(stderr, "parseLong says it messed this up\n");
        return -1;
    }
    if (filesize == 0 || filesize == LONG_MAX || filesize == LONG_MIN)
    {
        return -1;
    }
    printf("filesize is: %d\n", filesize);
    return filesize;
}

bool parseLong(const char *str, long *val)
{
    char *temp;
    bool rc = true;
    errno = 0;
    *val = strtol(str, &temp, 0);

    if (temp == str ||
        ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
        rc = false;

    return rc;
}

int get_num_rows(int filesize)
{

    int row_len = BUFSIZE - 16;
    int filesize_db = filesize;

    int ceil = (filesize + row_len - 1) / row_len;

    return ceil;
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
