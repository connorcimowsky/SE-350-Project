#ifndef STRING_H
#define STRING_H


/* copies the contents of 'source' into 'dest'; returns the length of the output string */
int str_cpy(char source[], char dest[]);

/* returns 1 if 'a' and 'b' are equal; 0 otherwise */
int str_cmp(char a[], char b[]);

/* converts the input string to an integer */
int a_to_i(char c[]);


#endif /* STRING_H */
