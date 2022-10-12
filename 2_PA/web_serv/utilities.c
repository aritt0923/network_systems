#include <utilities.h>

int str_to_lower(char * str, int len)
{
    char * tmp_str = calloc(len+1, sizeof(char));
    
    for (int i = 0; i < len; i++)
    {
        tmp_str[i] = tolower(str[i]);
    }
    
    memset(str, len, '\n');
    
    strncpy(str, tmp_str, len);
    
    free(tmp_str);
    
}
