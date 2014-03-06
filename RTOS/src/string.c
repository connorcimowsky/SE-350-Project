#include "string.h"


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
