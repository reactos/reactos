#ifndef ETHERBOOT_STRING_H
#define ETHERBOOT_STRING_H


#include "bits/string.h"

void *memcpy(void *dest, const void *src, size_t n);
void *memmove(void *dest, const void *src, size_t n);
void *memset(void *s, int c, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
size_t strlen(const char *s);


#endif /* ETHERBOOT_STRING */
