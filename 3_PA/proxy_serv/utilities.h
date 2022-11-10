#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // tolower
#include <errno.h>
#include <limits.h>

#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>


// https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

int str_to_lower(char * str, int len);

int get_file_size(FILE *fileptr);

// https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
str2int_errno str2int(int *out, char *s);

int md5_str(char * url, char * hash_res_buf);


#endif // UTILITIES_H_