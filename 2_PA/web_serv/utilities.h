#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // tolower

int str_to_lower(char * str, int len);

int get_file_size(FILE *fileptr);

#endif // UTILITIES_H_