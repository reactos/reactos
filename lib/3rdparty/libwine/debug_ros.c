/* The use of these four functions was creating unwanted imports
 * from msvcrt.dll in kernel32.dll. */

#define malloc libwine_malloc
#define free libwine_free
#define realloc libwine_realloc
#define _strdup libwine__strdup

#include "debug.c"

__MINGW_ATTRIB_MALLOC
void *malloc(size_t size)
{
    return LocalAlloc(0, size);
}

void free(void *ptr)
{
    LocalFree(ptr);
}

void *realloc(void *ptr, size_t size)
{
    if (ptr == NULL) return malloc(size);
    return LocalReAlloc(ptr, size, LMEM_MOVEABLE);
}

__MINGW_ATTRIB_MALLOC
char *_strdup(const char *str)
{
    char *newstr = malloc(strlen(str) + 1);
    if (newstr) strcpy(newstr, str);
    return newstr;
}
