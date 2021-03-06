#ifndef STRING_H
#define STRING_H


/* returns the length of 'str' */
int str_len(char str[]);

/* copies the contents of 'source' into 'dest' */
void str_cpy(char source[], char dest[]);

/* returns 1 if 'a' and 'b' are equal; 0 otherwise */
int str_cmp(char a[], char b[]);

/* converts the input string to an integer */
int a_to_i(char c[]);


#endif /* STRING_H */
