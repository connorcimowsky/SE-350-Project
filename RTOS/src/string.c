#include "string.h"


int str_len(char str[])
{
    int i = 0;
    
    while (1) {
        
        if (str[i] == '\0') {
            break;
        }
        
        i++;
        
    }
    
    return i;
}

void str_cpy(char source[], char dest[])
{
    int i = 0;
    
    while (1) {
        dest[i] = source[i];
        
        if (dest[i] == '\0') {
            break;
        }
        
        i++;
    }
}

int str_cmp(char a[], char b[])
{
    int i = 0;
    
    while (a[i] == b[i]) {
        
        if (a[i] == '\0' || b[i] == '\0') {
            break;
        }
        
        i++;
        
    }
    
    return (a[i] == '\0' && b[i] == '\0');
}

int a_to_i(char c[])
{
    int i = 0;
    int k = 0;
    
    while (c[i] != '\0') {
        k = (k << 3) + (k << 1) + (c[i] - '0');
        i++;
    }
    
    return k;
}
