#include <tcp_serv.h>

/*
 * Handles GET request from client, builds response, sends file
 * Catches:
 *      404: Not Found
 */
int handle_get(const char *client_req, int sockfd, sem_t *socket_sem)
{
    // printf("in handle get\n");
    char *filepath_buf = calloc_wrap(KILO, sizeof(char));
    char *filetype_buf = calloc_wrap(25, sizeof(char));
    char *header_buf = calloc_wrap(KILO, sizeof(char));
    char *true_filepath = calloc_wrap(KILO, sizeof(char));

    int http_v = -1;

    int res = 0;
    if ((res = parse_req(client_req, filepath_buf,
                         filetype_buf, &http_v)) != 0)
    { // exit point for any errors returned by parse_req
        send_error(header_buf, res, http_v, sockfd, socket_sem);
        return res;
    }
    memcpy(true_filepath, "../www/", strlen("../www/"));
    memcpy(&true_filepath[strlen(true_filepath)], filepath_buf, strlen(filepath_buf));

    // printf("Searching for file\n");
    FILE *fp = fopen(true_filepath, "rb");
    if (fp == NULL)
    {
        // perror("[-]Error in reading file.");
        send_error(header_buf, NOTFOUND, http_v, sockfd, socket_sem);
        fprintf(stderr, "NOTFOUND in handle_get\n");
        return NOTFOUND;
    }

    int filesize = get_file_size(fp);

    build_header(filetype_buf, header_buf, http_v, filesize);

    send_response(fp, header_buf, sockfd, socket_sem);

    free(filepath_buf);
    free(filetype_buf);
    free(header_buf);
    free(true_filepath);

    return 0;
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

    if (filepath_buf[strlen(filepath_buf) - 1] == '/')
    { // client passed a directory
      // give them the index.html there if it exists
        memcpy(filepath_buf, "index.html", strlen("index.html"));
    }
    get_filetype(filepath_buf, filetype_buf);

    // get http ver
    char http_str[9];
    strncpy(http_str, &client_req[fn_end_idx + 1], 8);
    http_str[8] = '\0';

    if (strncmp(http_str, "HTTP/1.1", 8) == 0)
    {
        *http_v = 1;
    }
    else if (strncmp(http_str, "HTTP/1.0", 8) == 0)
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
 *      405: Bad Request
 *      404: Not Found
 */
int get_filetype(char *filepath, char *filetype_buf)
{
    char *file_ext_buf = calloc_wrap(10, sizeof(char));
    int ext_start_idx = -1;
    int filepath_len = strlen(filepath);
    for (int i = 0; i < filepath_len; i++)
    {
        if (filepath[i] == '.')
        {
            ext_start_idx = i + 1;
        }
    }

    if (ext_start_idx == -1)
    { // no file extension found
        fprintf(stderr, "BAD REQ in get_filetype\n");
        return BAD_REQ;
    }

    strncpy(file_ext_buf, &filepath[ext_start_idx], 4);
    file_ext_buf[4] = '\0';
    if (strncmp(file_ext_buf, "html", 4) == 0)
    {
        memcpy(filetype_buf, "text/html", strlen("text/html"));
        return 0;
    }
    if (strncmp(file_ext_buf, "txt", 3) == 0)
    {
        memcpy(filetype_buf, "text/plain", strlen("text/plain"));
        return 0;
    }
    if (strncmp(file_ext_buf, "png", 3) == 0)
    {
        memcpy(filetype_buf, "image/png", strlen("image/png"));
        return 0;
    }
    if (strncmp(file_ext_buf, "gif", 3) == 0)
    {
        memcpy(filetype_buf, "image/gif", strlen("image/gif"));
        return 0;
    }
    if (strncmp(file_ext_buf, "jpg", 3) == 0)
    {
        memcpy(filetype_buf, "image/jpg", strlen("image/jpg"));
        return 0;
    }
    if (strncmp(file_ext_buf, "css", 3) == 0)
    {
        memcpy(filetype_buf, "text/css", strlen("text/css"));
        return 0;
    }
    if (strncmp(file_ext_buf, "js", 2) == 0)
    {
        memcpy(filetype_buf, "application/javascript", strlen("application/javascript"));
        return 0;
    }
    fprintf(stderr, "NOTFOUND in get_filetype\n");
    return NOTFOUND;
}

/*
 * Constructs response header for response to valid request
 * e.g. 
 *      HTTP/1.1 200 OK
 *      Content-Type: text/html
 *      Content-Length: 3348
 */
int build_header(char *filetype, char *header_buf, int http_v, int filesize)
{

    char http_str[10];
    if (http_v == 1)
    {
        memcpy(http_str, "HTTP/1.1 ", strlen("HTTP/1.1 "));
    }
    else
    {
        memcpy(http_str, "HTTP/1.0 ", strlen("HTTP/1.0 "));
    }

    memcpy(header_buf, http_str, strlen("HTTP/1._ "));

    int curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "200 OK\r\n", strlen("200 OK\r\n"));

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "Content-Type: ", strlen("Content-Type: "));

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], filetype, strlen(filetype));

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "\r\n", strlen("\r\n"));

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "Content-Length: ", strlen("Content-Length: "));

    int fs_str_len = snprintf(NULL, 0, "%d", filesize);
    char *fs_str = malloc(fs_str_len + 1);
    snprintf(fs_str, fs_str_len + 1, "%d", filesize);
    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], fs_str, fs_str_len);
    free(fs_str);

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "\r\n\r\n", strlen("\r\n\r\n"));

    return 0;
}

/*
 * Sends response header and file (pointed to by fp) to client
 * Catches:
 *      None
 */
int send_response(FILE *fp, char *header, int sockfd, sem_t *socket_sem)
{

    char data[KILO] = {0};

    sem_wait(socket_sem);
    send_wrap(sockfd, header, strlen(header), 0);
    sem_post(socket_sem);
    

    while (fread(data, sizeof(char), KILO, fp) != 0)
    {
        sem_wait(socket_sem); 
        send_wrap(sockfd, data, sizeof(data), 0);
        sem_post(socket_sem);   
        bzero(data, KILO);
    }
    printf("Thread %ld sent %s\n", pthread_self(), header);
    return 0;
}

/*
 * Sends specified error to client
 */
int send_error(char *header_buf, int error, int http_v, int sockfd, sem_t *socket_sem)
{
    char http_str[10];
    if (http_v == 1)
    {
        memcpy(http_str, "HTTP/1.1 ", strlen("HTTP/1.1 "));
    }
    else if (http_v == 0)
    {
        memcpy(http_str, "HTTP/1.0 ", strlen("HTTP/1.0 "));
    }
    else
    {
        memcpy(http_str, "HTTP/0.9 ", strlen("HTTP/0.9 "));
    }
    strncpy(header_buf, http_str, strlen("HTTP/1._ "));

    int curr_idx = strlen(header_buf);

    switch (error)
    {

    case BAD_REQ:
        memcpy(&header_buf[curr_idx], "400 Bad Request\r\n", strlen("400 Bad Request\r\n"));
        break;

    case FBDN:
        memcpy(&header_buf[curr_idx], "403 Forbidden\r\n", strlen("403 Forbidden\r\n"));
        break;

    case NOTFOUND:
        memcpy(&header_buf[curr_idx], "404 Not Found\r\n", strlen("404 Not Found\r\n"));
        break;

    case MNA:
        memcpy(&header_buf[curr_idx], "405 Method Not Allowed\r\n", strlen("405 Method Not Allowed\r\n"));
        break;

    case HTTP:
        memcpy(&header_buf[curr_idx], "505 HTTP Version Not Supported\r\n", strlen("505 HTTP Version Not Supported\r\n"));
        break;
    }

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "Content-Type: text/plain\r\n", strlen("Content-Type: text/plain\r\n"));

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "Content-Length: 0", strlen("Content-Length: 0"));

    curr_idx = strlen(header_buf);
    memcpy(&header_buf[curr_idx], "\r\n\r\n", strlen("\r\n\r\n"));
    // printf("%s", header_buf);

    sem_wait(socket_sem);
    send_wrap(sockfd, header_buf, strlen(header_buf), 0);
    sem_post(socket_sem);
    printf("Thread %ld sent %s\n", pthread_self(), header_buf);
    
    return 0;
}

void join_threads(int num_threads, pthread_t *thr_arr)
{
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(thr_arr[i], NULL);
    }
}
