#include <proxy_serv_funs.h>
#include <utilities.h>
#include <status_codes.h>
#include <wrappers.h>
#include <cache.h>

//#define PEDANTIC
//#define FUN_TRACE

/*
 * allocs:
 *      char * message_buf
 */
int fetch_remote(hash_table *cache, const char *client_req,
                 req_params *params, struct cache_node *file, int client_sockfd, sem_t *socket_sem)
{

#ifdef FUN_TRACE
    printf("entered fetch_remote\n");
#endif // FUN_TRACE
    int res;
    int serv_sockfd;
    if ((res = connect_remote(&serv_sockfd, params)) != 0)
    {
        return (res);
    }

    if (file == NULL)
    { // no existing entry, need to prepare a cache node
        file = get_cache_node();
        get_md5_str(file->md5_hash, params);
    }

    // send client message
    sem_wait(socket_sem);
    printf("CLIENT REQ:\n%s", client_req);
    send_wrap(serv_sockfd, client_req, strnlen(client_req, MAX_MSG_SZ), 0);
    sem_post(socket_sem);

    FILE *cache_file = NULL;

    // recv server response
    int bytes_recvd = 1;
    char *message_buf = calloc_wrap(MAX_MSG_SZ, sizeof(char));
    int first_pass = 1;
    int header_size = 0;
    int bytes_rem;
    int cache_flag = 1;
    if (params->dynamic)
    {
        cache_flag = 0;
    }
    while (bytes_recvd > 0)
    {
        sem_wait(socket_sem);
        bytes_recvd = recv(serv_sockfd, message_buf, MAX_MSG_SZ, 0);
        sem_post(socket_sem);

        if (first_pass)
        { // this packet has the header
            if ((header_size = parse_header(message_buf, file)) == -1)
            { // malformed response
                free(message_buf);
                close(serv_sockfd);

                return BAD_RESP;
            }
            if (header_size == -2)
            { // error from server, just forward it to the client
                cache_flag = 0;
            }

            bytes_rem = file->filesize;
            bytes_rem -= (bytes_recvd - header_size);
            first_pass = 0;

            if (cache_flag)
            { //
                get_ll_wlock(cache, file);

                // get a file pointer to the file we're looking for
                cache_file = open_file_from_cache_node_wr(file);
                if (cache_file == NULL)
                { // couldn't open a file to cache this
                    printf("cache flag was null\n");
                    release_ll_rwlock(cache, file);
                    struct cache_node *tmp_cache_node;
                    if ((tmp_cache_node = get_cache_entry(cache, file->md5_hash)) == NULL)
                    { // this node has not been added to the cache, delete it
                        free_cache_node(file);
                    }
                    cache_flag = 0;
                    continue;
                }

                add_cache_entry_non_block(cache, file);
                fprintf(cache_file, "%.*s", MAX_MSG_SZ - header_size, &message_buf[header_size]);
            }

            // printf("%s", &message_buf[header_size]);
        }
        else
        {
            // else fetch_remote\n");

            if (cache_flag)
            {
                fprintf(cache_file, "%.*s", MAX_MSG_SZ, message_buf);
            }
            bytes_rem -= bytes_recvd;
        }
        sem_wait(socket_sem);
        send_wrap(client_sockfd, message_buf, bytes_recvd, 0);
        sem_post(socket_sem);
        if (bytes_rem <= 0)
        {
            break;
        }
    }
    if (bytes_recvd < 0)
    {
        perror("recv");
        // exit(1);
    }
    if (cache_file != NULL)
        fclose(cache_file);

    if (cache_flag)
    {
        release_ll_rwlock(cache, file);
    }
    close(serv_sockfd);
#ifdef FUN_TRACE
    printf("exited fetch_remote\n");
#endif // FUN_TRACE
free(message_buf);  
    return 0;
}

int parse_header(char *header_packet, struct cache_node *file)
{
#ifdef FUN_TRACE
    printf("entered parse_header\n");
#endif // FUN_TRACE
    //#define PARSE_HEADER_DB_
    //#define PARSE_HEADER_PED_
    if ((strncmp(&header_packet[9], "200 OK", strlen("200 OK")) != 0))
    {
        // fprintf(stderr, "Error from server\n");
        return -2;
    }
    ptrdiff_t header_size;
    char *header_end = strstr(header_packet, "\r\n\r\n");
    header_size = &header_end[4] - header_packet;
#ifdef PARSE_HEADER_PED_
    printf("header size is %ld\n", header_size);
#endif // PARSE_HEADER_DB_
    char *header_buf = calloc_wrap(MAX_MSG_SZ, sizeof(char));
    memcpy(header_buf, header_packet, header_size);

#ifdef PARSE_HEADER_PED_
    printf("HEADER???\n%s\n", header_buf);
#endif // PARSE_HEADER_DB_

    char *token;
    char *rest = header_buf;
    char *tmp_str;
    char *filesize_str = calloc_wrap(16, sizeof(char));
    while ((token = strtok_r(rest, "\r\n", &rest)))
    {
        if (strncmp(token, "Content-Length: ", strlen("Content-Length: ")) == 0)
        {
            tmp_str = &token[strlen("Content-Length: ")];
            if (strlen(tmp_str) > 15)
            {
                fprintf(stderr, "Filesize too big\n");
                return -1;
            }
            memcpy(filesize_str, tmp_str, strlen(tmp_str));
            continue;
        }
        if (strncmp(token, "Content-Type: ", strlen("Content-Type: ")) == 0)
        {
            tmp_str = &token[strlen("Content-Type: ")];
            if (strlen(tmp_str) > MAX_FILETYPE_SIZE)
            {
                fprintf(stderr, "Content-Type field too large\n");
                return -1;
            }
            memcpy(file->filetype, tmp_str, strlen(tmp_str));
        }
    }
    if (strlen(filesize_str) == 0)
    { // no Content-Length in header
        fprintf(stderr, "No Content-Length found in response header. Chunking encoding not supported.\n");
        return -1;
    }
    if (str2int((int *)&file->filesize, filesize_str) != 0)
    {
        fprintf(stderr, "Could not convert filesize_str to int\n");
        return -1;
    }
#ifdef PARSE_HEADER_DB_
    printf("file->filetype: %s\n\nstrlen(file->filetype): %ld\n", file->filetype, strlen(file->filetype));
#endif // PARSE_HEADER_DB_
    free(filesize_str);
    free(header_buf);
    return header_size;
#ifdef FUN_TRACE
    printf("exited fetch_remote\n");
#endif // FUN_TRACE
}

int connect_remote(int *serv_sockfd, req_params *params)
{
#ifdef FUN_TRACE
    printf("entered connect_remote\n");
#endif // FUN_TRACE
    struct addrinfo hints;
    struct addrinfo *servinfo; // will point to the results

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    int status;
    if ((status = getaddrinfo(params->hostname, params->port_num, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return NOTFOUND;
    }

    // https://beej.us/guide/bgnet/examples/client.c
    char s[INET6_ADDRSTRLEN];

    struct addrinfo *p;
    // loop through all the results and connect to the first we can

    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((*serv_sockfd = socket(p->ai_family, p->ai_socktype,
                                   p->ai_protocol)) == -1)
        {
            perror("client: socket");
            continue;
        }

        struct timeval timeout;
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
        if (setsockopt(*serv_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout,
                       sizeof(timeout)) < 0)
            perror("setsockopt failed\n");

        if (setsockopt(*serv_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                       sizeof(timeout)) < 0)
            perror("setsockopt failed\n");

        if (connect(*serv_sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(*serv_sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }
    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return NOTFOUND;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);

    printf("client: connecting to %s\n", s);
    int res;
    if ((res = check_blocklist(params->hostname, s)) != 0)
    {
        return res;
    }

    freeaddrinfo(servinfo);
#ifdef FUN_TRACE
    printf("exited connect_remote\n");
#endif // FUN_TRACE
    return 0;
}

int check_blocklist(char *hostname, char *ip_addr)
{
#ifdef FUN_TRACE
    printf("entered check_blocklist\n");
#endif // FUN_TRACE
    FILE *blocklist;
    blocklist = fopen(".blocklist", "r");
    if (blocklist == NULL)
    {
        return 0;
    }
    char *blocked_host = calloc_wrap(MAX_HOSTNAME_LEN, sizeof(char));
    size_t len = MAX_HOSTNAME_LEN;
    ssize_t read;

    char *prepend_www_str = calloc_wrap(MAX_HOSTNAME_LEN, sizeof(char));

    memcpy(prepend_www_str, "www.", strlen("www."));

    memcpy(&prepend_www_str[strlen("www.")], hostname, strlen(hostname));

    while ((read = getline(&blocked_host, &len, blocklist)) != -1)
    {
        if (strncmp(blocked_host, hostname, read - 1) == 0)
        {
            fclose(blocklist);

            free(prepend_www_str);
            free(blocked_host);
            return FBDN;
        }
        if (strncmp(blocked_host, prepend_www_str, read - 1) == 0)
        {
            fclose(blocklist);

            free(prepend_www_str);
            free(blocked_host);
            return FBDN;
        }
        if (strncmp(blocked_host, ip_addr, read - 1) == 0)
        {
            fclose(blocklist);
            free(prepend_www_str);
            free(blocked_host);
            return FBDN;
        }
    }
    fclose(blocklist);
    free(prepend_www_str);
    free(blocked_host);
#ifdef FUN_TRACE
    printf("exited check_blocklist\n");
#endif // FUN_TRACE
    return 0;
}