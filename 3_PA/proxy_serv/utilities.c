
#include <utilities.h>
#include <proxy_serv_funs.h>
#include <wrappers.h>
#include <status_codes.h>

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

FILE * open_file_from_cache_node_wr(struct cache_node *file)
{
    FILE * cache_file = NULL;
    char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
    get_full_filepath(filepath, file);
    cache_file = fopen(filepath, "wb+");
    free(filepath);
    return cache_file;
}


FILE * open_file_from_cache_node_rd(struct cache_node *file)
{
    FILE * cache_file = NULL;
    char *filepath = calloc_wrap(EVP_MAX_MD_SIZE + strlen("./cache/"), sizeof(char));
    get_full_filepath(filepath, file);
    cache_file = fopen(filepath, "rb");
    free(filepath);
    return cache_file;
}


int get_md5_str(char *md5_str_buf, req_params *params)
{
    char *str_to_hash = calloc_wrap(MAX_URL_LEN + MAX_HOSTNAME_LEN, sizeof(char));
    
    memcpy(str_to_hash, params->hostname, strlen(params->hostname));
    memcpy(&str_to_hash[strlen(params->hostname)], params->filepath, strlen(params->filepath));
    md5_str(str_to_hash, md5_str_buf);
    
    free(str_to_hash);
    return 0;
}

int init_params(req_params *params)
{
    params->req_type = calloc_wrap(MAX_REQ_TYPE_LEN, sizeof(char));
    params->url = calloc_wrap(MAX_URL_LEN, sizeof(char));
    params->hostname = calloc_wrap(MAX_HOSTNAME_LEN, sizeof(char));
    params->port_num = calloc_wrap(MAX_PORT_NUM_LEN, sizeof(char));
    params->filepath = calloc_wrap(MAX_URL_LEN, sizeof(char));
    params->query = calloc_wrap(MAX_URL_LEN, sizeof(char));

    params->http_v = -1; // for error checking
    params->dynamic = 0; // don't cache dynamic files
    return 0;
}

int free_params(req_params *params)
{
    free(params->port_num);
    free(params->req_type);
    free(params->url);
    free(params->hostname);
    free(params->filepath);
    free(params->query);
    free(params);
    return 0;
}
