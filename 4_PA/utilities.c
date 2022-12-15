
#include "utilities.h"
#include "wrappers.h"

#define check(expr) if (!(expr)) { perror(#expr); kill(0, SIGTERM); }


int str_to_lower(char * str, int len)
{
    char * tmp_str = calloc(len+1, sizeof(char));
    
    for (int i = 0; i < len; i++)
    {
        tmp_str[i] = tolower(str[i]);
    }
    
    memset(str, '\0', len);
    
    strncpy(str, tmp_str, len);
    
    free(tmp_str);
    return 0;
    
}


int get_file_size(FILE *fileptr)
{
    fseek(fileptr, 0, SEEK_END);  // Jump to the end of the file
    int filelen = ftell(fileptr); // Get the current byte offset in the file
    rewind(fileptr);              // Jump back to the beginning of the file
    return filelen;
}

// https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
str2int_errno str2int(int *out, char *s)
{
    int base = 10;
    char *end;
    if (s[0] == '\0' || isspace(s[0]))
        return STR2INT_INCONVERTIBLE;
    errno = 0;
    long l = strtol(s, &end, base);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (*end != '\0')
        return STR2INT_INCONVERTIBLE;
    *out = l;
    return STR2INT_SUCCESS;
}

int md5_str(char * url, char * hash_res_buf)
{
    EVP_MD_CTX *mdctx;
    const EVP_MD *md;
    unsigned char md_value[EVP_MAX_MD_SIZE];
    unsigned int md_len, i;

    md = EVP_md5();

    mdctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(mdctx, md, NULL);
    EVP_DigestUpdate(mdctx, url, strlen(url));
    EVP_DigestFinal_ex(mdctx, md_value, &md_len);
    EVP_MD_CTX_free(mdctx);

    //memcpy(hash_res_buf, md_value, md_len);
    int str_idx = 0;
    for (i = 0; i < md_len; i++)
    {
        sprintf(&hash_res_buf[str_idx], "%02x", md_value[i]);
        str_idx += 2;
    }
    return 0;
}

// https://beej.us/guide/bgnet/examples/client.c
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) 
{ 
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
/*
FILE * open_file_from_cache_node_wr(struct cache_node *file)
{
    FILE * cache_file = NULL;
    char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
    get_full_filepath(filepath, file);
    cache_file = fopen(filepath, "wb+");
    free(filepath);
    return cache_file;
}
*/

/*
FILE * open_file_from_cache_node_rd(struct cache_node *file)
{
    FILE * cache_file = NULL;
    char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
    get_full_filepath(filepath, file);
    cache_file = fopen(filepath, "rb");
    free(filepath);
    return cache_file;
}
*/

/*
int get_md5_str(char *md5_str_buf, req_params *params)
{
    char *str_to_hash = calloc_wrap(MAX_URL_LEN + MAX_HOSTNAME_LEN, sizeof(char));
    
    memcpy(str_to_hash, params->hostname, strlen(params->hostname));
    memcpy(&str_to_hash[strlen(params->hostname)], params->filepath, strlen(params->filepath));
    md5_str(str_to_hash, md5_str_buf);
    
    free(str_to_hash);
    return 0;
}
*/

df_serv_cmd * get_serv_cmd_ptr(int n)
{
    df_serv_cmd * cmd = calloc_wrap(n, sizeof(df_serv_cmd));
    for(int i = 0; i < n; i++)
    {
        init_serv_cmd(&cmd[i]);
    }
    return cmd;
}

int init_serv_cmd(df_serv_cmd *serv_cmd)
{
    serv_cmd->cmd = calloc_wrap(MAX_CMD_LEN, sizeof(char));
    serv_cmd->filename_hr = calloc_wrap(MAX_FILENAME_LEN, sizeof(char));
    serv_cmd->filename_md5 = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    serv_cmd->filename_tmstmp_md5 = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    serv_cmd->chunk_name = calloc_wrap(EVP_MAX_MD_SIZE, sizeof(char));
    serv_cmd->offset = calloc_wrap(MAX_FILESIZE_STR, sizeof(char));
    serv_cmd->chunk_size = calloc_wrap(MAX_FILESIZE_STR, sizeof(char));
    serv_cmd->filesize = calloc_wrap(MAX_FILESIZE_STR, sizeof(char));
}


int free_serv_cmd(df_serv_cmd *serv_cmd)
{
    free(serv_cmd->cmd);
    free(serv_cmd->filename_hr);
    free(serv_cmd->filename_md5);
    free(serv_cmd->filename_tmstmp_md5);
    free(serv_cmd->chunk_name);
    free(serv_cmd->offset);
    free(serv_cmd->chunk_size);
    free(serv_cmd->filesize);
    
    free(serv_cmd);
}



int serv_cmd_to_str(df_serv_cmd *serv_cmd, char *serv_cmd_str)
{
    memcpy(serv_cmd_str, "Command: ", strlen("Command: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->cmd, strlen(serv_cmd->cmd));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Require_Timestamp: ", strlen("Require_Timestamp: "));
    memset(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->req_tmstmp, 1);
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Filename: ", strlen("Filename: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->filename_hr, strlen(serv_cmd->filename_hr));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Filename_Hash: ", strlen("Filename_Hash: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->filename_md5, strlen(serv_cmd->filename_md5));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Filename_Timestamp_Hash: ", strlen("Filename_Timestamp_Hash: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->filename_tmstmp_md5, strlen(serv_cmd->filename_tmstmp_md5));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Chunk: ", strlen("Chunk: "));
    memset(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->chunk, 1);
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Chunk_Name: ", strlen("Chunk_Name: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->chunk_name, strlen(serv_cmd->chunk_name));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Offset: ", strlen("Offset: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->offset, strlen(serv_cmd->offset));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Chunk_Size: ", strlen("Chunk_Size: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->chunk_size, strlen(serv_cmd->chunk_size));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n", strlen("\r\n"));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "Filesize: ", strlen("Filesize: "));
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], serv_cmd->filesize, strlen(serv_cmd->filesize));
    
    memcpy(&serv_cmd_str[strlen(serv_cmd_str)], "\r\n\r\n", strlen("\r\n\r\n"));
    
    int space_remaining = MAX_CMD_STR_SIZE-strlen(serv_cmd_str);
    memset(&serv_cmd_str[strlen(serv_cmd_str)], '\n', space_remaining);    //printf("%s", serv_cmd_str);
}



int str_to_serv_cmd(df_serv_cmd *serv_cmd, char *serv_cmd_str)
{
    
    char *token;
    char *rest = serv_cmd_str;
    char *tmp_str;
    
    
    while ((token = strtok_r(rest, "\r\n", &rest)))
    {
        if (strncmp(token, "Command: ", strlen("Command: ")) == 0)
        {
            tmp_str = &token[strlen("Command: ")];
            
            if (strlen(tmp_str) > MAX_CMD_LEN)
            {
                fprintf(stderr, "Command too long\n");
                return -1;
            }
            memcpy(serv_cmd->cmd, tmp_str, strlen(tmp_str));
            continue;
        }
        
        if (strncmp(token, "Require_Timestamp: ", strlen("Require_Timestamp: ")) == 0)
        {
            tmp_str = &token[strlen("Require_Timestamp: ")];
            if (strlen(tmp_str) > 1)
            {
                fprintf(stderr, "req_tmstmp too long\n");
                return -1;
            }
            serv_cmd->req_tmstmp = tmp_str[0];
            continue;
        }
        
        if (strncmp(token, "Filename: ", strlen("Filename: ")) == 0)
        {
            tmp_str = &token[strlen("Filename: ")];
            if (strlen(tmp_str) > MAX_FILENAME_LEN)
            {
                fprintf(stderr, "filename too long\n");
                return -1;
            }
            memcpy(serv_cmd->filename_hr, tmp_str, strlen(tmp_str));
            continue;
        }
        
        if (strncmp(token, "Filename_Hash: ", strlen("Filename_Hash: ")) == 0)
        {
            tmp_str = &token[strlen("Filename_Hash: ")];
            if (strlen(tmp_str) > EVP_MAX_MD_SIZE)
            {
                fprintf(stderr, "filename_md5 too long\n");
                return -1;
            }
            memcpy(serv_cmd->filename_md5, tmp_str, strlen(tmp_str));
            continue;
        }
        
        if (strncmp(token, "Filename_Timestamp_Hash: ", strlen("Filename_Timestamp_Hash: ")) == 0)
        {
            tmp_str = &token[strlen("Filename_Timestamp_Hash: ")];
            if (strlen(tmp_str) > EVP_MAX_MD_SIZE)
            {
                fprintf(stderr, "filename_tmstmp_md5 too long\n");
                return -1;
            }
            memcpy(serv_cmd->filename_tmstmp_md5, tmp_str, strlen(tmp_str));
            continue;
        }
        
        if (strncmp(token, "Chunk: ", strlen("Chunk: ")) == 0)
        {
            tmp_str = &token[strlen("Chunk: ")];
            if (strlen(tmp_str) > 1)
            {
                fprintf(stderr, "chunk too long\n");
                return -1;
            }
            serv_cmd->chunk = tmp_str[0];
            continue;
        }
        
        if (strncmp(token, "Chunk_Name: ", strlen("Chunk_Name: ")) == 0)
        {
            tmp_str = &token[strlen("Chunk_Name: ")];
            if (strlen(tmp_str) > EVP_MAX_MD_SIZE)
            {
                fprintf(stderr, "chunk_name too long\n");
                return -1;
            }
            memcpy(serv_cmd->chunk_name, tmp_str, strlen(tmp_str));
            continue;
        }
        
        if (strncmp(token, "Offset: ", strlen("Offset: ")) == 0)
        {
            tmp_str = &token[strlen("Offset: ")];
            if (strlen(tmp_str) > MAX_FILESIZE_STR)
            {
                fprintf(stderr, "offset too long\n");
                return -1;
            }
            memcpy(serv_cmd->offset, tmp_str, strlen(tmp_str));
            continue;
        }
        
        if (strncmp(token, "Chunk_Size: ", strlen("Chunk_Size: ")) == 0)
        {
            tmp_str = &token[strlen("Chunk_Size: ")];
            if (strlen(tmp_str) > MAX_FILESIZE_STR)
            {
                fprintf(stderr, "chunk_size too long\n");
                return -1;
            }
            memcpy(serv_cmd->chunk_size, tmp_str, strlen(tmp_str));
            continue;
        }
        
        if (strncmp(token, "Filesize: ", strlen("Filesize: ")) == 0)
        {
            tmp_str = &token[strlen("Filesize: ")];
            if (strlen(tmp_str) > MAX_FILESIZE_STR)
            {
                fprintf(stderr, "filesize too long\n");
                return -1;
            }
            memcpy(serv_cmd->filesize, tmp_str, strlen(tmp_str));
            continue;
        }
        
    }
}

int get_timestamp(char *buf)
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    unsigned long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    
    int filesize_str_len = snprintf(NULL, 0, "%ld", time_in_micros) + 1;
    // alloc the string
    // copy it over
    sprintf(buf, "%ld", time_in_micros);
}


void enable_keepalive(int sock) 
{
    int yes = 1;
    check(setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(int)) != -1);

    int idle = 1;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(int)) != -1);

    int interval = 1;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(int)) != -1);

    int maxpkt = 10;
    check(setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(int)) != -1);
}