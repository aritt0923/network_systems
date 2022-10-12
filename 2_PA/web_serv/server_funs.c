#include <tcp_serv.h>

/*
 * Handles GET request from client, builds response, sends file
 * Catches:
 *      404: Not Found
 */
int handle_get(const char *client_req, int sockfd)
{
    char * filepath_buf = calloc(KILO, sizeof(char));
    char * filetype_buf = calloc(25, sizeof(char));
    char * header_buf = calloc(KILO*2, sizeof(char));
    
    int http_v = 0;

    int res = 0;
    if ((res = parse_req(client_req, filepath_buf, 
        filetype_buf, http_v)) != 0)
    {
        send_error(res, &http_v, sockfd);
        return res;
    }
    
    int hdr_size = build_header(filepath_buf, filetype_buf, header_buf, http_v);
    
    FILE * fp = fopen(filepath_buf, "wb");
    if (fp == NULL)
    {
        // perror("[-]Error in reading file.");
        send_error(NOTFOUND, http_v, sockfd);
        return NOTFOUND;
    }
    
    send_response(fp, header_buf, sockfd);
    
    free(filepath_buf);
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
            char * filetype_buf, int * http_v)
{
    if (strncmp(client_req, "GET ", 4) != 0)
    {
        fprintf(stderr, "Method Not Allowed");
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
    
    if(fn_end_idx == -1)
    { // no whitespace found after filepath
        return BAD_REQ;
    }
    
    strncpy(filepath_buf, &client_req[4], fn_end_idx -4);
    str_to_lower(filepath_buf, strlen(filepath_buf));
    if (strstr(filepath_buf, "..") != NULL)
    { // don't allow the client to navigate up the filetree
        return FBDN;
    }
    get_filetype(filepath_buf, filetype_buf);
    
    // get http ver
    char http_str[9];
    strncpy(http_str, &client_req[fn_end_idx+1], 8);
    http_str[8] = '\0';
    
    if(strncmp(http_str, "HTTP/1.1", 8))
    {
        http_v = 1;
    }
    else if(strncmp(http_str, "HTTP/1.0", 8))
    {
        http_v = 0;
    }
    else
    {
        return HTTP;
    }
}

/*
 * Grabs the filetype from the filepath
 * Catches:
 *      400: Bad Request
 */
int get_filetype(char * filepath, char* filetype_buf)
{
    char * file_ext_buf = calloc(10, sizeof(char));
    int ext_start_idx = -1;
    for (int i = 0; i < strlen(filepath); i++)
    {
        if(filepath[i] == '.')
        {
            ext_start_idx = i+1;
        }
    }
    
    if(ext_start_idx == -1)
    { // no file extension found
        return BAD_REQ;
    }
    
    strncpy(file_ext_buf, &filepath[ext_start_idx], 4);
    file_ext_buf[4] = '\0';
    if(strncmp(file_ext_buf, "html", 4))
    {
        strncpy(filetype_buf, "text/html", strlen("text/html")+1);
        return 0;
    }
    if(strncmp(file_ext_buf, "txt", 3))
    {
        strncpy(filetype_buf, "text/plain", strlen("text/plain")+1);
        return 0;
    }
    if(strncmp(file_ext_buf, "png", 3))
    {
        strncpy(filetype_buf, "image/png", strlen("image/png")+1);
        return 0;
    }
    if(strncmp(file_ext_buf, "gif", 3))
    {
        strncpy(filetype_buf, "image/gif", strlen("image/gif")+1);
        return 0;
    }
    if(strncmp(file_ext_buf, "jpg", 3))
    {
        strncpy(filetype_buf, "image/jpg", strlen("image/jpg")+1);
        return 0;
    }
    if(strncmp(file_ext_buf, "css", 3))
    {
        strncpy(filetype_buf, "text/css", strlen("text/css")+1);
        return 0;
    }
    if(strncmp(file_ext_buf, "js", 2))
    {
        strncpy(filetype_buf, "application/javascript", strlen("application/javascript")+1);
        return 0;
    }
    return BAD_REQ;
}


int build_header(char * filepath, char * filetype, char* header, int http_v)
{
    
}

/*
 * Sends response header and file (pointed to by fp) to client
 * Catches:
 *      None
 */
int send_response(FILE *fp, char * header, int sockfd)
{
    int n;
    char data[KILO] = {0};

    while (fgets(data, KILO, fp) != NULL)
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