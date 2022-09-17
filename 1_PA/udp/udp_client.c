/*
 * udpusr.c - A simple UDP usr
 * usage: udpusr <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>

struct send_rec_args
{
    struct sockadd_in *serveraddr;
    int sockfd;
    char *buf;
    int *serverlen;
    int n; // size of message sent or recieved
};

int rec_from_server(struct send_rec_args *args);
int send_to_server(struct send_rec_args *args);

int handle_usr_cmd(struct send_rec_args *args);
int validate_usr_file_op(char *cmd, char *filename);
int validate_usr_no_file(char *cmd);
int split_usr_in(char *buf, char *cmd, char *filename);
int cmd_switch(int res, char *filename, struct send_rec_args *args);
void strip_newline(char *buf);
void str_to_lower(char *buf);
int get_file_size(FILE *fileptr);

int send_file_to_server(char *filename, struct send_rec_args *args);
int send_file_size(FILE *fileptr, struct send_rec_args *args);
int bin_to_file_2d(char *dest_filename, char **file_buffer_2d, int filesize);
int bin_to_file_1d(char *dest_filename, char *file_buffer_1d, FILE *fileptr);
int send_rows(char **file_buffer_2d, int *rows_to_send, int num_rows, struct send_rec_args *args);
int copy_row(char *src, char *dest);
int free_2d(char **arr, int filesize);
int get_num_rows(int filesize);
char **calloc_2d(int filesize);
int read_file_to_buf(FILE *fileptr, char**file_buffer_2d);
int resend_loop(char **file_buffer_2d, int *rows_to_send, int num_rows, struct send_rec_args *args);
int split_server_in(char *buf, char *cmd, char *filename);
bool parseLong(const char *str, long *val);
int fill_metadata(char **file_buffer_2d, int num_rows);
int rec_from_server_tmout(struct send_rec_args *args);

#define BUFSIZE 1024
#define GET 1
#define PUT 2
#define DELETE 3
#define LS 4
#define EXIT 5
#define TIMEOUT -3

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char **argv)
{
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char *buf = calloc(BUFSIZE, sizeof(char));

    /* check command line arguments */
    if (argc != 3)
    {
        fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL)
    {
        fprintf(stderr, "ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    bzero(buf, BUFSIZE);
    printf("Please enter cmd: ");
    fgets(buf, BUFSIZE, stdin);

    /* send the message to the server */
    struct send_rec_args args;
    args.buf = buf;
    args.serveraddr = &serveraddr;
    args.serverlen = &serverlen;
    args.sockfd = sockfd;
    args.n = 0;

    handle_usr_cmd(&args);

    // serverlen = send_to_server(&args);

    // rec_from_server(&args);

    return 0;
}

int rec_from_server(struct send_rec_args *args)
{
    memset(args->buf, '\0', strlen(args->buf));
    args->n = recvfrom(args->sockfd, args->buf, BUFSIZE, 0, args->serveraddr, args->serverlen);
    if (args->n < 0)
        error("ERROR in recvfrom");
    // printf("Echo from server: %s", args->buf);
}

int rec_from_server_tmout(struct send_rec_args *args)
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
    args->n = recvfrom(args->sockfd, args->buf, BUFSIZE, 0, args->serveraddr, args->serverlen);
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

int send_to_server(struct send_rec_args *args)
{
    /* send the message to the server */
    struct sockaddr_in *serveraddr = args->serveraddr;
    *args->serverlen = sizeof(*serveraddr);
    args->n = sendto(args->sockfd, args->buf, BUFSIZE, 0, args->serveraddr, *args->serverlen);
    if (args->n < 0)
        error("ERROR in sendto");
    
    for (int i = 0; i < BUFSIZE; i++)
    {
        printf("%c", args->buf[i]);
    }
    return *args->serverlen;
}

int handle_usr_cmd(struct send_rec_args *args)
{ // process the user's command
    strip_newline(args->buf);
    str_to_lower(args->buf);

    char *cmd = calloc(strlen(args->buf), sizeof(char));
    char *filename = calloc(strlen(args->buf), sizeof(char));
    int rec_filename = 0;
    if ((rec_filename = split_usr_in(args->buf, cmd, filename)) < 0)
    { // error with split_usr_in
        return -1;
    }
    // validate the user's commands
    int res = -1;
    if (rec_filename)
    {
        if ((res = validate_usr_file_op(cmd, filename)) < 0)
        {
            fprintf(stderr, "Error with validate_usr_file_op\n");
            return -1;
        }
    }
    else
    {
        if ((res = validate_usr_no_file(cmd)) < 0)
        {
            fprintf(stderr, "Error with validate_usr_no_file\n");
            return -1;
        }
    }
    return (cmd_switch(res, filename, args));
}

int split_usr_in(char *buf, char *cmd, char *filename)
{ /*
   * split the usr command into cmd - filename
   * if no filename is passed, cmd is set equal to buf
   */

    // printf("usr in is: %s\n", buf);
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

int validate_usr_no_file(char *cmd)
{ // No filename was passed - validate that cmd is either ls or exit
    int res = -1;
    if (strcmp("ls", cmd) == 0)
    {
        // printf("usr says 'ls'\n");
        res = LS;
    }
    else if (strcmp("exit", cmd) == 0)
    {
        // printf("usr says 'exit'\n");
        res = EXIT;
    }
    return res;
}

int validate_usr_file_op(char *cmd, char *filename)
{ // Filename was passed - validate that cmd is either get, put, or delete

    int res = -1;

    if (strcmp("get", cmd) == 0)
    {
        // printf("usr says 'get'\n");
        res = GET;
    }
    else if (strcmp("put", cmd) == 0)
    {
        // printf("usr says 'put'\n");
        res = PUT;
    }
    else if (strcmp("delete", cmd) == 0)
    {
        // printf("usr says 'delete'\n");
        res = DELETE;
    }
    return res;
}

int cmd_switch(int res, char *filename, struct send_rec_args *args)
{
    switch (res)
    {

    case GET:

        break;

    case PUT:
        while ((res = send_file_to_server(filename, args)) < 0)
        {
        }
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

int send_file_to_server(char *filename, struct send_rec_args *args)
{
    FILE *fileptr = fopen(filename, "r"); // Open the file in binary mode
    if (fileptr == NULL)
    {
        fprintf(stderr, "Error opening file: %s\n", filename);
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
    // finally, reply to the server with the filesize

    // zero the buffer and ensure that we're sending the message we want
    memset(args->buf, '\0', BUFSIZE);
    strncpy(args->buf, "put ", strlen("put "));
    strncat(args->buf, filename, strlen(filename));

    // sending "put [filename]"
    send_to_server(args);
    // looking for "SENDSIZE"
    rec_from_server(args);
    if (strncmp("SENDSIZE", args->buf, strlen("SENDSIZE")) == 0)
    {
        send_file_size(fileptr, args);
    }
    else
    {
        fprintf(stderr, "Error with ack from server: expected SENDFILESIZE\n");
        return -1;
    }
    // looking for SENDFILE
    rec_from_server(args);
    if (strncmp("SENDFILE", args->buf, strlen("SENDFILE")) == 0)
    {
        send_rows(file_buffer_2d, rows_to_send, num_rows, args);
        memset(rows_to_send, 0, num_rows * sizeof(int));
    }
    else
    {
        fprintf(stderr, "Error with ack from server: expected SENDSIZE\n");
        return -1;
    }
    while (resend_loop(file_buffer_2d, rows_to_send, num_rows, args) > 0)
    {
        send_rows(file_buffer_2d, rows_to_send, num_rows, args);
        memset(rows_to_send, 0, num_rows * sizeof(int));
    }
    bin_to_file_2d("client_copy.txt", file_buffer_2d, filesize);
}

int resend_loop(char **file_buffer_2d, int *rows_to_send, int num_rows, struct send_rec_args *args)
{
    int rec_res;
    while((rec_res = rec_from_server_tmout(args) != TIMEOUT))
    {
        if (strncmp("ALLREC", args->buf, strlen("ALLREC")) == 0)
        {
            return 0;
        }

        if (strncmp("RESEND ", args->buf, strlen("RESEND ")) == 0)
        {
            char *row_num_str = &args->buf[strlen("RESEND ")];
            int *row_num;
            if(!parseLong(row_num_str, row_num))
            {
                fprintf(stderr, "Error parsing row number from resend_loop\n");
                return -1;
            }
            if(*row_num >= num_rows)
            {
                fprintf(stderr, "Error with row number from resend_loop (to large)\n");
                return -1;
            }
            rows_to_send[*row_num] = 1; 
        }
    }
    return 1;
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
        strncat(buf, " ", strlen(" "));
        
        strncat(buf, num_rows_str, strlen(num_rows_str));
        strncat(buf, " ", strlen(" "));
        //printf("buf is: %s\n", buf);
    }
}

int send_rows(char **file_buffer_2d, int *rows_to_send, int num_rows, struct send_rec_args *args)
{
    for (int i = 0; i < num_rows; i++)
    {
        if (rows_to_send[i])
        {
            memset(args->buf, '\0', BUFSIZE);
            copy_row(file_buffer_2d[i], args->buf);
            /*
            printf("buf is:\n");
            for(int j = 0; j < BUFSIZE; j++)
            {
                printf("%c", args->buf[j]);
            }
            printf("\n");
            */
            //printf("arr row is: %s\n", file_buffer_2d[i]);
            send_to_server(args);
        }
    }
}



int copy_row(char *src, char *dest)
{
    for (int i = 0; i < BUFSIZE; i++)
    {
        dest[i] = src[i];
        //printf("%c", src[i]);
    }
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
    send_to_server(args);
    memset(filesize_buf, '\0', strlen(filesize_buf));
    free(filesize_buf);
}

int read_file_to_buf(FILE *fileptr, char**file_buffer_2d)
{
    // dynamically allocates a 2d array and reads a file into it
    // row by row. The first 16 bytes of each row are reserved for metadata
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

char **calloc_2d(int filesize)
{
    // dynamically allocates a 2d_array based on the size of the file
    int rows = get_num_rows(filesize);

    char **arr = (char **)calloc(rows, sizeof(char *));
    for (int i = 0; i < rows; i++)
        arr[i] = (char *)calloc(BUFSIZE, sizeof(char));

    return arr;
}

int free_2d(char **arr, int filesize)
{
    // frees the dynamically allocated 2d_arr
    int rows = get_num_rows(filesize);
    for (int i = 0; i < rows; i++)
    {
        free(arr[i]);
    }
    free(arr);
}

int get_num_rows(int filesize)
{
    // takes filesize and returns the number of rows we need in a 2d array
    int row_len = BUFSIZE - 16;
    int filesize_db = filesize;

    int ceil = (filesize + row_len - 1) / row_len;

    return ceil;
}

int bin_to_file_2d(char *dest_filename, char **file_buffer_2d, int filesize)
{
    // accepts a 2d char array filled with binary file data
    // and reads that data into a file
    FILE *dest_fileptr = fopen(dest_filename, "wb");
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
        // first 16 chars of each row are reserved for metadata
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

int bin_to_file_1d(char *dest_filename, char *file_buffer_1d, FILE *fileptr)
{
    FILE *dest_fileptr = fopen(dest_filename, "w");
    if (dest_fileptr == NULL)
    {
        fprintf(stderr, "Error opening dest file.\n");
        return -1;
    }
    int filesize = get_file_size(fileptr);
    int n_written = fwrite(file_buffer_1d, sizeof(char), filesize, dest_fileptr);
}

int get_file_size(FILE *fileptr)
{
    fseek(fileptr, 0, SEEK_END);  // Jump to the end of the file
    int filelen = ftell(fileptr); // Get the current byte offset in the file
    rewind(fileptr);              // Jump back to the beginning of the file
    return filelen;
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

int split_server_in(char *buf, char *cmd, char *filename)
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