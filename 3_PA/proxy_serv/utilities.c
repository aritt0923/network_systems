#include <utilities.h>

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
}