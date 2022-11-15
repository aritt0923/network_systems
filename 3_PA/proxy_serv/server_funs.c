#include <proxy_serv_funs.h>
#include <utilities.h>
#include <status_codes.h>
#include <wrappers.h>
#include <cache.h>

//#define FUN_TRACE
//#define PEDANTIC

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
int handle_get(hash_table *cache, const char *client_req, int sockfd, sem_t *socket_sem)
{
    req_params *params = calloc_wrap(1, sizeof(req_params));
    init_params(params);

    int res = 0;
    if ((res = parse_req(client_req, params)) != 0)
    { // exit point for any errors returned by parse_req
        fprintf(stderr, "error in parse_req\n");
        send_error(res, params, sockfd, socket_sem);
        return res;
    }
    char *md5_hash = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    char *str_to_hash = calloc_wrap(MAX_URL_LEN + MAX_HOSTNAME_LEN, sizeof(char));
    memcpy(str_to_hash, params->hostname, strlen(params->hostname));
    memcpy(&str_to_hash[strlen(params->hostname)], params->filepath, strlen(params->filepath));
    md5_str(str_to_hash, md5_hash);
    
    free(str_to_hash);
    
    
    struct cache_node *file = get_cache_entry(cache, md5_hash);
    
    if ((res = check_ttl(cache, file, cache->ttl_seconds)) < 0)
    {
        return -1;
    }
    
    if (res == 0)
    { // ttl is not expired
    
        return (send_file_from_cache(cache, file, params, sockfd, socket_sem));
    }
    // need to fetch remote
    if ((res = fetch_remote(cache, client_req, params, file, sockfd, socket_sem)) != 0)
    { // exit point for any errors returned by parse_req
        fprintf(stderr, "error in parse_req\n");
        send_error(res, params, sockfd, socket_sem);
        return res;
    }

    free_params(params);
    return 0;
}

/*
 * Parses the clients request, returns 0 for success.
 *
 * populates:
 *  params->req_type;
 *  params->url;
 *  params->hostname;
 *  params->port_num;
 *
 * Example request:
 *  GET http://www.yahoo.com/ HTTP/1.1
 *  Host: www.yahoo.com
 */
int parse_req(const char *client_req, req_params *params)
{
//#define PRINT_PARAMS
#ifdef FUN_TRACE
    printf("Entered parse_req\n\n");
#endif // FUN_TRACE
    // check request type
    if (strncmp(client_req, "GET ", 4) != 0)
    {
        fprintf(stderr, "METHOD NOT ALLOWED in parse_req\n");
        return BAD_REQ;
    }
    memcpy(params->req_type, "GET", strlen("GET"));
#ifdef PRINT_PARAMS
    printf("params->req_type: %s\n\n", params->req_type);
#endif // PRINT_PARAMS

    // get URL
    int url_start_idx = 4;
    int url_end_idx = -1; // idx of whitespace after URL
    for (int i = url_start_idx; i < MAX_MSG_SZ; i++)
    { // url starts at idx 4
        if (client_req[i] == ' ')
        {
            url_end_idx = i;
            break;
        }
    }
    if (url_end_idx == -1)
    { // no whitespace found after URL
        fprintf(stderr, "BAD REQUEST in parse_req\n");
        return BAD_REQ;
    }
    if((url_end_idx - url_start_idx) > MAX_URL_LEN)
    {
        fprintf(stderr, "URL too long\n");
        return BAD_REQ;
    }
    strncpy(params->url, &client_req[4], url_end_idx - url_start_idx);
    str_to_lower(params->url, strlen(params->url));
#ifdef PRINT_PARAMS
    printf("params->url: %s\n\n", params->url);
#endif // PRINT_PARAMS

    // pull filepath from url
    int filepath_start_idx = -1;
    for (int i = url_start_idx; i < url_end_idx; i++)
    {
        if (client_req[i] == '/')
        {
            if (client_req[i + 1] == '/' || client_req[i - 1] == '/')
            { // needs to be an isolated forward slash
                continue;
            }
            filepath_start_idx = i;
        }
    }
    if (filepath_start_idx == -1)
    {
        memcpy(params->filepath, "/", 1);
    }
    else
    {
        strncpy(params->filepath, &client_req[url_start_idx], url_end_idx-filepath_start_idx);
    }
#ifdef PRINT_PARAMS
    printf("params->filepath: %s\n\n", params->filepath);
#endif // PRINT_PARAMS

    // get port_num from URL
    char *port_num_tmp = strrchr(params->url, ':');
    char port_num_url[5];
    memset(port_num_url, '\0', 5);

    if (port_num_tmp != NULL)
    {
        port_num_tmp = &port_num_tmp[1];
        int i;
        for (i = 0; i < 5; i++)
        {
            if (!isdigit((unsigned char)port_num_tmp[i]))
            {
                break;
            }
        }
        if (i > 0)
        {
            memcpy(port_num_url, port_num_tmp, strlen(port_num_tmp));
        }
    }
    
    params->dynamic = check_dynamic(params->url);

    // get http ver
    char http_str[9];
    memset(http_str, '\0', 9);
    strncpy(http_str, &client_req[url_end_idx + 1], 8);

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

    // cycle through request line by line for hostname param
    char *client_req_cp = calloc_wrap(MAX_MSG_SZ, sizeof(char));
    memcpy(client_req_cp, client_req, strlen(client_req));

    char *token;
    char *rest = client_req_cp;
    char *tmp_str;
    char *hostname_tmp = calloc_wrap(MAX_HOSTNAME_LEN, sizeof(char));

    while ((token = strtok_r(rest, "\r\n", &rest)))
    {
        if (strncmp(token, "Host: ", strlen("Host: ")) == 0)
        {
            tmp_str = &token[strlen("Host: ")];
            if (strlen(tmp_str) > MAX_HOSTNAME_LEN)
            {
                fprintf(stderr, "Hostname too big\n");
                return -1;
            }
            memcpy(hostname_tmp, tmp_str, strlen(tmp_str));
            break;
        }
    }
    if (strlen(hostname_tmp) == 0)
    {
        fprintf(stderr, "No Host: field\n");
        return BAD_REQ;
    }

    port_num_tmp = strrchr(hostname_tmp, ':');
    char port_num_hn[5];
    memset(port_num_hn, '\0', 5);
    if (port_num_tmp != NULL)
    {
        port_num_tmp = &port_num_tmp[1];
        int i;
        for (i = 0; i < 5; i++)
        {
            if (!isdigit((unsigned char)port_num_tmp[i]))
            {
                break;
            }
        }
        if (i > 0)
        {
            memcpy(port_num_hn, port_num_tmp, strlen(port_num_tmp));
        }
    }
    memcpy(params->hostname, hostname_tmp, strlen(hostname_tmp));
#ifdef PRINT_PARAMS
    printf("params->hostname: %s\n\n", params->hostname);
#endif // PRINT_PARAMS

    if (strlen(port_num_url) == 0 && strlen(port_num_hn) == 0)
    { // no port found in url or hn
        memcpy(params->port_num, DEFAULT_PORT, 2);
    }
    // one of them is not empty
    else if (strlen(port_num_hn) != 0)
    { // got port from hostname
        if (strlen(port_num_url) == 0)
        { // no port from url, go ahead and copy
            memcpy(params->port_num, port_num_hn, strlen(port_num_hn));
        }
        else
        { // got port_num from both, need to compare
            if (strncmp(port_num_hn, port_num_url, 4) != 0)
            { // dont match
                return BAD_REQ;
            }
            memcpy(params->port_num, port_num_hn, strlen(port_num_hn));
        }
    }
    else
    { // got port from url and not hostname
        memcpy(params->port_num, port_num_url, strlen(port_num_url));
    }

#ifdef PRINT_PARAMS
    printf("params->port_num: %s\n\n", params->port_num);
#endif // PRINT_PARAMS
    return 0;
}

/*
 * Constructs response header for response to valid request
 * e.g.
 *      HTTP/1.1 200 OK
 *      Content-Type: text/html
 *      Content-Length: 3348
 */

int build_header(req_params *params, struct cache_node *file, char *header_buf)
{
    if (params->http_v == 1)
    {
        memcpy(header_buf, "HTTP/1.1 200 OK\r\n", strlen("HTTP/1.1 200 OK\r\n"));
    }
    else if (params->http_v == 0)
    {
        memcpy(header_buf, "HTTP/1.0 200 OK\r\n", strlen("HTTP/1.0 200 OK\r\n"));
    }
    memcpy(&header_buf[strlen(header_buf)], "Content-Type: ", strlen("Content-Type: "));
    memcpy(&header_buf[strlen(header_buf)], file->filetype, strlen(file->filetype));
    memcpy(&header_buf[strlen(header_buf)], "\r\n", strlen("\r\n"));
    
    int filesize_str_len = snprintf(NULL, 0, "%d", file->filesize);
    // alloc the string
    char *filesize_buf = calloc(filesize_str_len, sizeof(char));
    // copy it over
    sprintf(filesize_buf, "%d", file->filesize);
    
    memcpy(&header_buf[strlen(header_buf)], "Content-Length: ", strlen("Content-Length: "));
    memcpy(&header_buf[strlen(header_buf)], filesize_buf, strlen(filesize_buf));
    memcpy(&header_buf[strlen(header_buf)], "\r\n\r\n", strlen("\r\n\r\n"));
    free(filesize_buf);
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
    char *header_buf = calloc_wrap(KILO, sizeof(char));

    char http_str[10];
    if (params->http_v == 1)
    {
        memcpy(http_str, "HTTP/1.1 ", strlen("HTTP/1.1 "));
    }
    else if (params->http_v == 0)
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

int check_ttl(hash_table *cache, struct cache_node *file, int ttl_seconds)
{
    if (file == NULL)
    {
        return 1;
    }
    char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
    get_full_filepath(filepath, file);

    get_ll_rdlock(cache, file);
    int fd = open(filepath, __O_PATH);
    free(filepath);
    struct stat info;

    if (fstat(fd, &info) != 0)
    {
        perror("fstat() error");
        release_ll_rwlock(cache, file);
        return -1;
    }
    time_t now = time(0);
    int age = now - info.st_ctim.tv_sec;
    release_ll_rwlock(cache, file);
    close(fd);
    if (age >= ttl_seconds)
    {
        return 1;
    }
    return 0;
}

int get_full_filepath(char *filepath, struct cache_node *file)
{
    memcpy(filepath, "./cache/", strlen("./cache/"));
    memcpy(&filepath[strlen(filepath)], file->md5_hash, EVP_MAX_MD_SIZE);
    return 0;
}

double diff_timespec(const struct timespec *time1, const struct timespec *time0)
{
    return (time1->tv_sec - time0->tv_sec) + (time1->tv_nsec - time0->tv_nsec) / 1000000000.0;
}


int send_file_from_cache(hash_table *cache, struct cache_node *file, req_params *params, int sockfd, sem_t *socket_sem)
{
    get_ll_rdlock(cache, file);
    char *header_buf = calloc_wrap(KILO, sizeof(char));
    build_header(params, file, header_buf);
    
    char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
    get_full_filepath(filepath, file);
    FILE * file_ptr = fopen(filepath, "rb");
    free(filepath);
    if (file_ptr ==NULL) 
    {
        return NOTFOUND;
    }
    send_file(file_ptr, header_buf, sockfd, socket_sem);
    
    free(header_buf);
    release_ll_rwlock(cache, file);
    return 0;
}


int check_dynamic(char * url)
{
    char * query = strstr(url, "?");
    if(query == NULL)
    {
        return 0;
    }
    return 1;
}

int check_blocklist(char *hostname)
{
    
}