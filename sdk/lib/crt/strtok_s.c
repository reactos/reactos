#define _CRTIMP
#include <precomp.h>

char * CDECL strtok_s(char *str, const char *delim, char **ctx)
{
    if (!MSVCRT_CHECK_PMT(delim != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(ctx != NULL)) return NULL;
    if (!MSVCRT_CHECK_PMT(str != NULL || *ctx != NULL)) return NULL;

    if(!str)
        str = *ctx;

    while(*str && strchr(delim, *str))
        str++;
    if(!*str)
    {
        *ctx = str;
        return NULL;
    }

    *ctx = str+1;
    while(**ctx && !strchr(delim, **ctx))
        (*ctx)++;
    if(**ctx)
        *(*ctx)++ = 0;

    return str;
}

#ifdef _WIN64
void* __imp_strtok_s = strtok_s;
#else
void* _imp__strtok_s = strtok_s;
#endif
