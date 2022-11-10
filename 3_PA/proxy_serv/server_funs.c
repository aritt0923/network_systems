#include <proxy_serv_funs.h>

/*
 * Handles GET request from client
 * Checks for cached page
 * If found, forward file to client
 * If no cached page is found:
 *      relays request to remote server
 *      caches page
 *      foward to client
 * Catches:
 *      404: Not Found
 */
int handle_get(const char *client_req, int sockfd, sem_t *socket_sem)
{
    req_params params;
    init_params(&params);
    
    int res = 0;
    if ((res = parse_req(client_req, &params)) != 0)
    { // exit point for any errors returned by parse_req
        send_error(res, &params, sockfd, socket_sem);
        return res;
    }    
    
    char *md5_hash = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    md5_str(params.url, md5_hash);
    
    if(send_from_cache(&params, md5_hash, sockfd, socket_sem))
    { // file was found in cache and sent to client
        return 0;
    }

    free_params(&params);
    return 0;
}

/*
 * Parses the clients request, returns 0 for success.
 *
 * populates: 
 *  params->req_type;
 *  params->url;
 *  params->hostname;
 *  
 *  params->http_v;
 *  params->req_type_len;
 *  params->url_len;
 *  params->hostname_len;
 *  params->http_v_len;
 * 
 * Example request:
 *  GET http://www.yahoo.com/ HTTP/1.1
 *  Host: www.yahoo.com
 */
int parse_req(const char *client_req, req_params *params)
{
    if (strncmp(client_req, "GET ", 4) != 0)
    {
        fprintf(stderr, "METHOD NOT ALLOWED in parse_req\n");
        return BAD_REQ;
    }
    
    // get the filepath
    int url_end_idx = -1; // idx of whitespace after filepath
    for (int i = 4; i < MAX_CLNT_MSG_SZ; i++)
    { // filepath starts at idx 4
        if (client_req[i] == ' ')
        {
            url_end_idx = i;
            break;
        }
    }
    if (url_end_idx == -1)
    { // no whitespace found after filepath
        fprintf(stderr, "BAD REQUEST in parse_req\n");
        return BAD_REQ;
    }
    strncpy(params->url, &client_req[4], url_end_idx - 4);
    params->url_len = strlen(params->url);
    str_to_lower(params->url, params->url_len);
    
    // get http ver
    char http_str[9];
    strncpy(http_str, &client_req[url_end_idx + 1], 8);
    http_str[8] = '\0';

    if (strncmp(http_str, "HTTP/1.1", 8) == 0)
    {
        params->http_v = 1;
    }
    else if (strncmp(http_str, "HTTP/1.0", 8) == 0)
    {
        params->http_v = 0;
    }
    else
    {
        fprintf(stderr, "HTTP in parse_req\n");
        return HTTP;
    }
    
    
}


/*
 * Constructs response header for response to valid request
 * e.g. 
 *      HTTP/1.1 200 OK
 *      Content-Type: text/html
 *      Content-Length: 3348
 */
int build_header(req_params *params, char *header_buf)
{
    return 0;
}

/*
 * Sends response header and file (pointed to by fp) to client
 * Catches:
 *      None
 */
int send_file(FILE *fp, char *header, int sockfd, sem_t *socket_sem)
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
int send_error(int error, req_params *params, int sockfd, sem_t *socket_sem)
{
    char * header_buf = calloc_wrap(KILO, sizeof(char));

    strncpy(header_buf, params->http_v, params->http_v_len);

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
    free(header_buf);
    
    return 0;
}

void join_threads(int num_threads, pthread_t *thr_arr)
{
    for (int i = 0; i < num_threads; i++)
    {
        pthread_join(thr_arr[i], NULL);
    }
}


int init_params(req_params *params)
{
    params->req_type = calloc_wrap(MAX_REQ_TYPE_LEN, sizeof(char));
    params->url = calloc_wrap(MAX_URL_LEN, sizeof(char));
    params->hostname = calloc_wrap(MAX_HOSTNAME_LEN, sizeof(char));
    params->http_v = calloc_wrap(MAX_HTTP_V_LEN, sizeof(char));
    
    params->req_type_len = MAX_REQ_TYPE_LEN;
    params->url_len = MAX_URL_LEN;
    params->hostname_len = MAX_HOSTNAME_LEN;
    params->http_v_len = MAX_HTTP_V_LEN;
    
    return 0;
}

int free_params(req_params *params)
{
    free(params->req_type);
    free(params->url);
    free(params->hostname);
    free(params->http_v);
}
 
 
int send_from_cache(req_params *params, char * md5_hash, int sockfd, sem_t *socket_sem)
{
    // READER LOCK
    if (!check_cache(md5_hash))
    {
        return 0;
    }
    char * header_buf = calloc_wrap(KILO, sizeof(char));
    // BUILD HEADER
    // SEND FILE
    // READER UNLOCK
    free(header_buf);
}

bool check_cache(char * md5_hash)
{
    return false;
}