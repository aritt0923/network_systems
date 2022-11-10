
#include <stdio.h>
#include <string.h> //strlen
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h> //for threading , link with lpthread
#include <semaphore.h>
#include <signal.h>

//#include <wrappers.h>
//#include <utilities.h>
//#include <status_codes.h>

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

int md5_str(char * url, char * hash_res_buf);

int main()
{
    char * hash_res = calloc(17, sizeof(char));
    md5_str("hello world", hash_res);
    printf("%s\n", hash_res);
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