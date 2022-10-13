#include <tcp_serv.h>

/*
 * Handles GET request from client, builds response, sends file
 * Catches:
 *      404: Not Found
 */
int handle_get(const char *client_req, int sockfd)
{
    // printf("in handle get\n");
    char *filepath_buf = calloc(KILO, sizeof(char));
    char *filetype_buf = calloc(25, sizeof(char));
    char *header_buf = calloc(KILO, sizeof(char));

    int http_v = 0;

    int res = 0;
    if ((res = parse_req(client_req, filepath_buf,
                         filetype_buf, &http_v)) != 0)
    { // exit point for any errors returned by parse_req
        send_error(res, http_v, sockfd);
        fprintf("parse_req returned %d\n", res);
        return res;
    }
    char *true_filepath = calloc(KILO, sizeof(char));
    strncpy(true_filepath, "../www/", strlen("../www/"));
    int idx = strlen(true_filepath);
    strncpy(&true_filepath[idx], filepath_buf, strlen(filepath_buf));
    free(filepath_buf);
    
    // printf("Searching for file\n");
    FILE *fp = fopen(true_filepath, "rb");
    if (fp == NULL)
    {
        // perror("[-]Error in reading file.");
        send_error(NOTFOUND, http_v, sockfd);
        fprintf(stderr, "NOTFOUND in handle_get\n");
        return NOTFOUND;
    }

    int filesize = get_file_size(fp);

    int hdr_size = build_header(filetype_buf, header_buf, http_v, filesize);

    send_response(fp, header_buf, sockfd);

    free(filetype_buf);
    free(header_buf);
}

/*
 * Parses the clients request, returns 0 for success.
 * Catches:
 *      405: Method Not Allowed
 *      505: HTTP Version Not Supported
 *      400: Bad Request
 */
int parse_req(const char *client_req, char *filepath_buf,
              char *filetype_buf, int *http_v)
{
    // printf("in parse_req\n");
    if (strncmp(client_req, "GET ", 4) != 0)
    {
        fprintf(stderr, "METHOD NOT ALLOWED in parse_req\n");
        return MNA;
    }

    // get the filepath
    int fn_end_idx = -1; // idx of whitespace after filepath
    for (int i = 4; i < MAX_CLNT_MSG_SZ; i++)
    { // filepath starts at idx 4
        if (client_req[i] == ' ')
        {
            fn_end_idx = i;
            break;
        }
    }

    if (fn_end_idx == -1)
    { // no whitespace found after filepath
        fprintf(stderr, "BAD REQUEST in parse_req\n");
        return BAD_REQ;
    }

    strncpy(filepath_buf, &client_req[4], fn_end_idx - 4);
    str_to_lower(filepath_buf, strlen(filepath_buf));
    if (strstr(filepath_buf, "..") != NULL)
    { // don't allow the client to navigate up the filetree
        fprintf(stderr, "FORBIDDEN in parse_req\n");
        return FBDN;
    }
    get_filetype(filepath_buf, filetype_buf);
    // printf("Back from get_filetype\n");
    // get http ver
    char http_str[9];
    strncpy(http_str, &client_req[fn_end_idx + 1], 8);
    http_str[8] = '\0';

    if (strncmp(http_str, "HTTP/1.1", 8)==0)
    {
        *http_v = 1;
    }
    else if (strncmp(http_str, "HTTP/1.0", 8)==0)
    {
        *http_v = 0;
    }
    else
    {
        fprintf(stderr, "HTTP in parse_req\n");
        return HTTP;
    }
    return 0;
}

/*
 * Grabs the filetype from the filepath
 * Fills out filetype_buf accordingly
 * Catches:
 *      400: Bad Request
 */
int get_filetype(char *filepath, char *filetype_buf)
{
    // printf("in get_filetype\n");
    
    char *file_ext_buf = calloc(10, sizeof(char));
    int ext_start_idx = -1;
    for (int i = 0; i < strlen(filepath); i++)
    {
        if (filepath[i] == '.')
        {
            ext_start_idx = i + 1;
        }
    }

    if (ext_start_idx == -1)
    { // no file extension found
        fprintf(stderr, "BAD REQ 1 in get_filetype\n");
        return BAD_REQ;
    }

    strncpy(file_ext_buf, &filepath[ext_start_idx], 4);
    file_ext_buf[4] = '\0';
    if (strncmp(file_ext_buf, "html", 4) == 0)
    {
        strncpy(filetype_buf, "text/html", strlen("text/html") + 1);
        return 0;
    }
    if (strncmp(file_ext_buf, "txt", 3) == 0)
    {
        strncpy(filetype_buf, "text/plain", strlen("text/plain") + 1);
        return 0;
    }
    if (strncmp(file_ext_buf, "png", 3) == 0)
    {
        strncpy(filetype_buf, "image/png", strlen("image/png") + 1);
        return 0;
    }
    if (strncmp(file_ext_buf, "gif", 3)== 0)
    {
        strncpy(filetype_buf, "image/gif", strlen("image/gif") + 1);
        return 0;
    }
    if (strncmp(file_ext_buf, "jpg", 3) == 0)
    {
        strncpy(filetype_buf, "image/jpg", strlen("image/jpg") + 1);
        return 0;
    }
    if (strncmp(file_ext_buf, "css", 3) == 0)
    {
        strncpy(filetype_buf, "text/css", strlen("text/css") + 1);
        return 0;
    }
    if (strncmp(file_ext_buf, "js", 2) == 0)
    {
        strncpy(filetype_buf, "application/javascript", strlen("application/javascript") + 1);
        return 0;
    }
    fprintf(stderr, "BAD REQ 2 in get_filetype\n");
    return BAD_REQ;
}

int build_header(char *filetype, char *header_buf, int http_v, int filesize)
{
    // printf("in build_header\n");

    char http_str[10];
    if (http_v == 1)
    {
        strncpy(http_str, "HTTP/1.1 ", strlen("HTTP/1.1 "));
    }
    else
    {
        strncpy(http_str, "HTTP/1.0 ", strlen("HTTP/1.0 "));
    }

    strncpy(header_buf, http_str, strlen("HTTP/1._ "));

    int curr_idx = strlen(header_buf);
    strncpy(&header_buf[curr_idx], "200 OK\r\n", strlen("200 OK\r\n"));

    curr_idx = strlen(header_buf);
    strncpy(&header_buf[curr_idx], "Content-Type: ", strlen("Content-Type: "));

    curr_idx = strlen(header_buf);
    strncpy(&header_buf[curr_idx], filetype, strlen(filetype));

    curr_idx = strlen(header_buf);
    strncpy(&header_buf[curr_idx], "\r\n", strlen("\r\n"));

    curr_idx = strlen(header_buf);
    strncpy(&header_buf[curr_idx], "Content-Length: ", strlen("Content-Length: "));

    int fs_str_len = snprintf(NULL, 0, "%d", filesize);
    char *fs_str = malloc(fs_str_len + 1);
    snprintf(fs_str, fs_str_len + 1, "%d", filesize);
    curr_idx = strlen(header_buf);
    strncpy(&header_buf[curr_idx], fs_str, fs_str_len);
    free(fs_str);
    
    curr_idx = strlen(header_buf);
    strncpy(&header_buf[curr_idx], "\r\n\r\n", strlen("\r\n\r\n"));
    
    printf("%s", header_buf);
    
}

/*
 * Sends response header and file (pointed to by fp) to client
 * Catches:
 *      None
 */
int send_response(FILE *fp, char *header, int sockfd)
{
    // printf("in send_response\n");
    
    char data[KILO] = {0};

    if (send(sockfd, header, strlen(header), 0) == -1)
    {
        perror("[-]Error in sending header.");
        exit(1);
    }

    while (fread(data, sizeof(char), KILO, fp) != 0)
    {
        if (send(sockfd, data, sizeof(data), 0) == -1)
        {
            perror("[-]Error in sending file.");
            exit(1);
        }
        bzero(data, KILO);
    }
}

/*
 * Sends specified error to client
 */
int send_error(int error, int http_v, int sockfd)
{
    return 0;
}