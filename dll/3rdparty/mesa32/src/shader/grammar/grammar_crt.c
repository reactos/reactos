#include "grammar_crt.h"

#define GRAMMAR_PORT_BUILD 1
#include "grammar.c"
#undef GRAMMAR_PORT_BUILD


void grammar_alloc_free (void *ptr)
{
    free (ptr);
}

void *grammar_alloc_malloc (size_t size)
{
    return malloc (size);
}

void *grammar_alloc_realloc (void *ptr, size_t old_size, size_t size)
{
    return realloc (ptr, size);
}

void *grammar_memory_copy (void *dst, const void * src, size_t size)
{
    return memcpy (dst, src, size);
}

int grammar_string_compare (const byte *str1, const byte *str2)
{
    return strcmp ((const char *) str1, (const char *) str2);
}

int grammar_string_compare_n (const byte *str1, const byte *str2, size_t n)
{
    return strncmp ((const char *) str1, (const char *) str2, n);
}

byte *grammar_string_copy (byte *dst, const byte *src)
{
    return (byte *) strcpy ((char *) dst, (const char *) src);
}

byte *grammar_string_copy_n (byte *dst, const byte *src, size_t n)
{
    return (byte *) strncpy ((char *) dst, (const char *) src, n);
}

unsigned int grammar_string_length (const byte *str)
{
    return strlen ((const char *) str);
}

byte *grammar_string_duplicate (const byte *src)
{
    const unsigned int size = grammar_string_length (src);
    byte *str = grammar_alloc_malloc (size + 1);
    if (str != NULL)
    {
        grammar_memory_copy (str, src, size);
        str[size] = '\0';
    }
    return str;
}

