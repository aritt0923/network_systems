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
#include <dirent.h>

struct send_rec_args
{
    struct sockaddr_in *clientaddr;
    int sockfd;
    char *buf;
    int *clientlen;
    int n; // size of message sent or recieved
};

// send / rec packets
int send_to_client(struct send_rec_args *args);
int rec_from_client(struct send_rec_args *args);
int rec_from_client_tmout(struct send_rec_args *args);

// switch functions
int rec_file_from_client(char *filename, struct send_rec_args *args);
int send_file_to_client(char *filename, struct send_rec_args *args);
int delete_file(char *filename, struct send_rec_args *args);
int ls(struct send_rec_args *args);
int client_exit(struct send_rec_args *args);

// handle client input
int validate_client_no_file(char *buf);
int validate_client_file_op(char *buf);
int handle_client_cmd(struct send_rec_args *args);
int cmd_switch(int res, char *filename, struct send_rec_args *args);
int parse_rec_filesize(struct send_rec_args *args);
int split_cmd(char *buf, char *cmd, char *filename);
int get_row_num(char *buf, int *row_num, int *total_rows);

// send / rec files
char **calloc_2d(int filesize);
int fill_metadata(char **file_buffer_2d, int num_rows);
int fill_buffer_from_client(char **file_buffer_2d, int filesize, struct send_rec_args *args, int *rows_rec);
int read_file_to_buf(FILE *fileptr, char **file_buffer_2d);
int bin_to_file_2d(char *dest_filename, char **file_buffer_2d, int filesize);
int send_file_size(FILE *fileptr, struct send_rec_args *args);
int send_rows(char **file_buffer_2d, int *rows_to_send, int num_rows, struct send_rec_args *args);
int copy_row(char *src, char *dest);
int listen_for_resend(int *rows_to_send, int num_rows, struct send_rec_args *args);
int free_2d(char **arr, int filesize);
int req_resend(int *rows_rec, int num_rows, struct send_rec_args *args);

// basic utilities
void strip_newline(char *buf);
void str_to_lower(char *buf);
int get_file_size(FILE *fileptr);
int get_num_rows(int filesize);
bool parseLong(const char *str, int *val);

#define BUFSIZE 1024
#define GET 1
#define PUT 2
#define DELETE 3
#define LS 4
#define EXIT 5
#define TIMEOUT 9

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
    int n = 0;                     /* message byte size */

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
    args.n = n;

    /*
     * main loop: wait for a datagram, then echo it
     */
    clientlen = sizeof(clientaddr);
    while (1)
    {

        /*
         * recvfrom: receive a UDP datagram from a client
         */
        memset(args.buf, '\0', BUFSIZE);
        rec_from_client(&args);

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
        // printf("server received datagram from %s (%s)\n", hostp->h_name, hostaddrp);
        // printf("server received %ld/%d bytes: %s\n", strlen(buf), args.n, buf);

        handle_client_cmd(&args);
    }
    return 0;
}

int rec_from_client(struct send_rec_args *args)
{

    // set timeout
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (setsockopt(args->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error");
    }
    bzero(args->buf, BUFSIZE);
    args->n = recvfrom(args->sockfd, args->buf, BUFSIZE, 0, (struct sockaddr *)args->clientaddr, (socklen_t *)args->clientlen);
    if (args->n < 0)
        error("ERROR in recvfrom");
    // printf("Echo from client: %s --END\n", args->buf);
    return 0;
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
    args->n = recvfrom(args->sockfd, args->buf, BUFSIZE, 0, (struct sockaddr *)args->clientaddr, (socklen_t *)args->clientlen);
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
    return 0;
}

int send_to_client(struct send_rec_args *args)
{
    /* send the message to the client */
    struct sockaddr_in *clientaddr = args->clientaddr;
    *args->clientlen = sizeof(*clientaddr);
    args->n = sendto(args->sockfd, args->buf, BUFSIZE, 0, (struct sockaddr *)args->clientaddr, *args->clientlen);
    if (args->n < 0)
        error("ERROR in sendto");
    return *args->clientlen;
}

int send_str(char *str, struct send_rec_args *args)
{
    if (strlen(str) >= BUFSIZE - 1)
    {
        return -1;
    }
    memset(args->buf, '\0', BUFSIZE);
    strncpy(args->buf, str, strlen(str) + 1);
    send_to_client(args);
}

int cmd_switch(int res, char *filename, struct send_rec_args *args)
{
    switch (res)
    {

    case GET:
        send_file_to_client(filename, args);
        break;

    case PUT:
        rec_file_from_client(filename, args);
        break;

    case DELETE:
        delete_file(filename, args);
        break;

    case LS:
        ls(args);
        break;

    case EXIT:
        client_exit(args);
        break;

    /* you can have any number of case statements */
    default: /* Optional */
        fprintf(stderr, "Unrecognized cmd passed to cmd_switch\n");
        return -1;
    }
    return 0;
}

int delete_file(char *filename, struct send_rec_args *args)
{
}

int ls(struct send_rec_args *args)
{
}

int client_exit(struct send_rec_args *args)
{
    printf("Client sent exit. Goodbye client!\n");
    send_str("GOODBYE", args);
    return 0;
}

int send_file_to_client(char *filename, struct send_rec_args *args)
{
    FILE *fileptr = fopen(filename, "rb"); // Open the file in binary mode
    if (fileptr == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", filename);
        send_str("FAIL", args);
        return 1;
    }
    int filesize = get_file_size(fileptr);
    int num_rows = get_num_rows(filesize);

    // before we send our request, we prepare everything we need to send the file
    // read the file to a 2d array (each row will be a packet)
    char **file_buffer_2d = calloc_2d(filesize);
    fill_metadata(file_buffer_2d, num_rows);
    read_file_to_buf(fileptr, file_buffer_2d);
    // initialize an array to keep track of which rows we need to send
    // (will be used to resend dropped packets later)
    int *rows_to_send = calloc(num_rows, sizeof(int));
    // we need to send them all at first
    memset(rows_to_send, 1, num_rows * sizeof(int));
    send_file_size(fileptr, args);

    // looking for SENDFILE
    rec_from_client(args);

    if (strncmp("SENDFILE", args->buf, strlen("SENDFILE")) == 0)
    {
        send_rows(file_buffer_2d, rows_to_send, num_rows, args);
        memset(rows_to_send, 0, num_rows * sizeof(int));
    }
    else
    {
        fprintf(stderr, "Error with ack from client: expected SENDSIZE\n");
        free_2d(file_buffer_2d, filesize);

        return -1;
    }
    while (listen_for_resend(rows_to_send, num_rows, args) > 0)
    {
        send_rows(file_buffer_2d, rows_to_send, num_rows, args);
        memset(rows_to_send, 0, num_rows * sizeof(int));
    }
    // bin_to_file_2d("client_copy.txt", file_buffer_2d, filesize);
    free_2d(file_buffer_2d, filesize);
    return 0;
}

int send_file_size(FILE *fileptr, struct send_rec_args *args)
{
    memset(args->buf, '\0', BUFSIZE);
    int filesize = get_file_size(fileptr);
    strncpy(args->buf, "FILESIZE ", strlen("FILESIZE "));
    // get number of chars needed to hold filesize as string
    int filesize_str_len = snprintf(NULL, 0, "%d", filesize);
    // alloc the string
    char *filesize_buf = calloc(filesize_str_len, sizeof(char));
    // copy it over
    sprintf(filesize_buf, "%d", filesize);
    strncat(args->buf, filesize_buf, strlen(filesize_buf));
    strncat(args->buf, " ", strlen(" ") + 1);
    send_to_client(args);
    memset(filesize_buf, '\0', strlen(filesize_buf));
    free(filesize_buf);
    return 0;
}

int fill_metadata(char **file_buffer_2d, int num_rows)
{

    if (num_rows > 999999999)
    {
        fprintf(stderr, "filesize way to big\n");
        return -1;
    }

    // alloc the string
    char num_rows_str[10];
    // copy it over
    sprintf(num_rows_str, "%d", num_rows);

    for (int i = 0; i < num_rows; i++)
    {
        char *buf = file_buffer_2d[i];
        strncpy(buf, "ROW ", strlen("ROW "));
        // alloc the string
        char row_num_str[10];
        // copy it over
        sprintf(row_num_str, "%d", i);
        strncat(buf, row_num_str, strlen(row_num_str));
        strncat(buf, " ", strlen(" ") + 1);

        strncat(buf, num_rows_str, strlen(num_rows_str));
        strncat(buf, " ", strlen(" ") + 1);
        // printf("buf is: %s\n", buf);
    }
    return 0;
}

int send_rows(char **file_buffer_2d, int *rows_to_send, int num_rows, struct send_rec_args *args)
{
    for (int i = 0; i < num_rows; i++)
    {
        if (rows_to_send[i])
        {
            memset(args->buf, '\0', BUFSIZE);
            copy_row(file_buffer_2d[i], args->buf);
            send_to_client(args);
        }
    }
    return 0;
}

int listen_for_resend(int *rows_to_send, int num_rows, struct send_rec_args *args)
{

    // recieves "resend" packets from the client until they stop coming
    // populates an array called rows_to_send with this information
    int rec_res;
    while ((rec_res = rec_from_client_tmout(args) != TIMEOUT))
    {
        if (strncmp("ALLREC", args->buf, strlen("ALLREC")) == 0)
        {
            return 0;
        }

        if (strncmp("RESEND ", args->buf, strlen("RESEND ")) == 0)
        {
            char *row_num_str = &args->buf[strlen("RESEND ")];
            int *row_num = NULL;
            if (!parseLong(row_num_str, row_num))
            {
                fprintf(stderr, "Error parsing row number from listen_for_resend\n");
                return -1;
            }
            if (*row_num >= num_rows)
            {
                fprintf(stderr, "Error with row number from listen_for_resend (to large)\n");
                return -1;
            }
            rows_to_send[*row_num] = 1;
        }
    }
    return 1;
}

int rec_file_from_client(char *filename, struct send_rec_args *args)
{
    send_str("SENDSIZE", args);
    // expect "FILESIZE [filesize]" in return
    rec_from_client(args);

    if (strncmp("FAIL", args->buf, strlen("FAIL")) == 0)
    {
        fprintf(stderr, "Client could not send file\n");
        return -1;
    }

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

    int num_rows = get_num_rows(filesize);
    int rows_rec[num_rows];
    memset(rows_rec, 0, num_rows * sizeof(int));

    // tell the client to send the file
    send_str("SENDFILE", args);
    // start recieving packets
    fill_buffer_from_client(file_buffer_2d, filesize, args, rows_rec);
    // tell client which ones we missed
    int resend_res;
    while ((resend_res = req_resend(rows_rec, num_rows, args)) != 0)
    {
        if (resend_res < 0)
        {
            fprintf(stderr, "Error resending rows \n");
            free_2d(file_buffer_2d, filesize);
            return -1;
        }
        fill_buffer_from_client(file_buffer_2d, filesize, args, rows_rec);
    }
    send_str("ALLREC", args);
    bin_to_file_2d(filename, file_buffer_2d, filesize);
    free_2d(file_buffer_2d, filesize);
    return 0;
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
        if (get_row_num(&str[strlen("ROW ")], &row_num, &total_rows) < 0)
        {
            fprintf(stderr, "in fill_buffer_from_client - error with parseLong\n");
            return -1;
        }
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

int req_resend(int *rows_rec, int num_rows, struct send_rec_args *args)
{
    bool resend_again = 0;
    for (int i = 0; i < num_rows; i++)
    {
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

int copy_row(char *src, char *dest)
{
    for (int i = 0; i < BUFSIZE; i++)
    {
        dest[i] = src[i];
        // printf("%c", src[i]);
    }
    return 0;
}

int get_row_num(char *buf, int *row_num, int *total_rows)
{
    char total_rows_str[BUFSIZE];
    memset(total_rows_str, '\0', BUFSIZE);
    char row_num_str[BUFSIZE];
    memset(row_num_str, '\0', BUFSIZE);
    if (split_cmd(buf, row_num_str, total_rows_str) < 0)
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
    return 0;
}

int bin_to_file_2d(char *dest_filename, char **file_buffer_2d, int filesize)
{
    // char client_copy[20] = "client_copy.txt";
    // FILE *dest_fileptr = fopen(client_copy, "wb");
    FILE *dest_fileptr = fopen(dest_filename, "wb");

    if (dest_fileptr == NULL)
    {
        fprintf(stderr, "Error opening dest file.\n");
        return -1;
    }
    int rows = get_num_rows(filesize);
    // printf("rows = %d\n",rows );

    int bytes_written = 0;
    int bytes_remaining = filesize;
    for (int i = 0; i < rows; i++)
    {
        int n_written = 0;
        const char *ptr = &file_buffer_2d[i][16];

        if ((BUFSIZE - 16) > bytes_remaining)
        {
            n_written = fwrite(ptr, sizeof(char), bytes_remaining, dest_fileptr);
            // printf("final_row:\n    row: %d, bytes_written: %d\n", i+1, bytes_written+n_written);
            break;
        }
        n_written = fwrite(ptr, sizeof(char), BUFSIZE - 16, dest_fileptr);
        bytes_written += n_written;
        bytes_remaining -= n_written;
    }
    rewind(dest_fileptr); // Jump back to the beginning of the file

    return 0;
}

int free_2d(char **arr, int filesize)
{
    int rows = get_num_rows(filesize);
    for (int i = 0; i < rows; i++)
    {
        free(arr[i]);
    }
    free(arr);
    return 0;
}

char **calloc_2d(int filesize)
{
    int rows = get_num_rows(filesize);

    char **arr = (char **)calloc(rows, sizeof(char *));
    for (int i = 0; i < rows; i++)
        arr[i] = (char *)calloc(BUFSIZE, sizeof(char));

    return arr;
}

int read_file_to_buf(FILE *fileptr, char **file_buffer_2d)
{
    // accepts a 2d array and fills it with the files binary data
    int filesize = get_file_size(fileptr);
    char *file_buffer_1d = (char *)calloc(filesize, sizeof(char));
    fread(file_buffer_1d, filesize, 1, fileptr); // Read in the entire file

    int rows = get_num_rows(filesize);
    // printf("NUM ROWS: %d\n", rows);
    // printf("FILESIZE %d\n", filesize);

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
    return 0;
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
    memset(filesize_str, '\0', BUFSIZE);

    split_cmd(args->buf, msg_from_client, filesize_str);
    int filesize;
    bool scs = parseLong(filesize_str, &filesize);
    if (!scs)
    {
        fprintf(stderr, "parseLong says it messed this up\n");
        return -1;
    }
    if (filesize == 0)
    {
        return -1;
    }
    return filesize;
}

bool parseLong(const char *str, int *val)
{
    char *temp;
    bool rc = true;
    errno = 0;
    *val = (int)strtol(str, &temp, 0);

    if (temp == str || errno == ERANGE)
        rc = false;

    return rc;
}

int get_num_rows(int filesize)
{

    int row_len = BUFSIZE - 16;
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

int split_cmd(char *buf, char *str1, char *str2)
{ /*
   * split the usr command into str1 - str2 by whitespace
   * if no space is found, str1 is set equal to the first string in buf,
   * str2 is set to all null chars, and 0 is returned
   * otherwise str2 is set to the second string, and 1 is returned
   */

    // printf("usr in is: %s\n", buf);
    int split_index = -1;
    for (int i = 0; i < (int)strlen(buf); i++)
    {
        if (buf[i] == ' ')
        {
            split_index = i;
            break;
        }
    }
    if (split_index == -1)
    { // no whitespace was found
        strncpy(str1, buf, strlen(buf));
        memset(str2, '\0', BUFSIZE);
        return 0;
    }
    strncpy(str1, buf, split_index);
    str1[split_index] = '\0';
    // printf("Cmd is: %s\n", str1);

    int filename_ind = 0;
    for (int i = split_index + 1; i < (int)strlen(buf); i++)
    {
        if (buf[i] == '\0' || buf[i] == ' ')
        {
            break;
        }
        str2[filename_ind] = buf[i];
        filename_ind++;
    }
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

int validate_client_file_op(char *cmd)
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

    char cmd[BUFSIZE];
    char filename[BUFSIZE];
    int rec_filename = 0;
    if ((rec_filename = split_cmd(args->buf, cmd, filename)) < 0)
    { // error with split_cmd
        return -1;
    }

    // validate the clients commands
    int res = -1;
    if (rec_filename)
    {
        if ((res = validate_client_file_op(cmd)) < 0)
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
    return cmd_switch(res, filename, args);
    
}
