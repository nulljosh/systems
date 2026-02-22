/* NullOS String Library Header */

#ifndef _LIB_STRING_H
#define _LIB_STRING_H

#include <lib/stdint.h>

/* Memory operations */
void    *memcpy(void *dest, const void *src, uint32_t n);
void    *memset(void *dest, int val, uint32_t n);
void    *memmove(void *dest, const void *src, uint32_t n);
int      memcmp(const void *a, const void *b, uint32_t n);

/* String operations */
uint32_t strlen(const char *s);
int      strcmp(const char *a, const char *b);
int      strncmp(const char *a, const char *b, uint32_t n);
char    *strcpy(char *dest, const char *src);
char    *strncpy(char *dest, const char *src, uint32_t n);
char    *strcat(char *dest, const char *src);
char    *strchr(const char *s, int c);

/* Conversion */
int      atoi(const char *s);
void     itoa(int value, char *buf, int base);

#endif /* _LIB_STRING_H */
