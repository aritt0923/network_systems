#include <proxy_serv_funs.h>
#include <utilities.h>
#include <status_codes.h>
#include <wrappers.h>
#include <cache.h>

#define PEDANTIC

int fetch_remote(hash_table *cache, const char *client_req,
                 req_params *params, struct cache_node *file, int client_sockfd, sem_t *socket_sem)
{
    int res;
    int serv_sockfd;
    if ((res = connect_remote(&serv_sockfd, params) != 0))
    {
        return (res);
    }

    if (file == NULL)
    {
        file = get_cache_node();
        char *str_to_hash = calloc_wrap(MAX_URL_LEN + MAX_HOSTNAME_LEN, sizeof(char));
        memcpy(str_to_hash, params->hostname, strlen(params->hostname));
        memcpy(&str_to_hash[strlen(params->hostname)], params->filepath, strlen(params->filepath));
        md5_str(str_to_hash, file->md5_hash);
        free(str_to_hash);
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
    if(params->dynamic)
    {
        cache_flag = 0;
    }
    while (bytes_recvd > 0)
    {
        sem_wait(socket_sem);
        bytes_recvd = recv(serv_sockfd, message_buf, MAX_MSG_SZ, 0);
        sem_post(socket_sem);

        if (first_pass)
        {
            if ((header_size = parse_header(message_buf, file)) == -1)
            {
                return BAD_RESP;
            }
            if (header_size == -2)
            {
                cache_flag = 0;
            }
            bytes_rem = file->filesize;
            bytes_rem -= (bytes_recvd - header_size);
            first_pass = 0;

            if (cache_flag)
            {
                add_cache_entry(cache, file);
                
                get_ll_wlock(cache, file);
                // get a file pointer to the file we're looking for
                char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
                get_full_filepath(filepath, file);
                cache_file = fopen(filepath, "w+");
                free(filepath);
                if (cache_file == NULL)
                {
                    release_ll_rwlock(cache, file);  
                    cache_flag = 0;
                    continue;
                }
                fprintf(cache_file, "%s", &message_buf[header_size]);
            }

            // printf("%s", &message_buf[header_size]);
        }
        else
        {
            // else fetch_remote\n");

            if (cache_flag)
            {
                fprintf(cache_file, "%s", message_buf);
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
    else
    {
        free_cache_node(file);
    }
    return 0;
}

int parse_header(char *header_packet, struct cache_node *file)
{
    //#define PARSE_HEADER_DB_
    //#define PARSE_HEADER_PED_
    if ((strncmp(&header_packet[9], "200 OK", strlen("200 OK")) != 0))
    {
        //fprintf(stderr, "Error from server\n");
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
}

/*
FILE *create_struct cache_node(struct cache_node *file)
{
    char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
    memcpy(filepath, "./cache/", strlen("./cache/"));
    memcpy(&filepath[strlen(filepath)], file->md5_hash, EVP_MAX_MD_SIZE);
    FILE *fPtr;

    fPtr = fopen("cache/%s", "w");

    if (fPtr == NULL)
    {
        printf("Unable to create file.\n");
        exit(EXIT_FAILURE);
    }
}
*/

int connect_remote(int *serv_sockfd, req_params *params)
{
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

    freeaddrinfo(servinfo);
    return 0;
}

