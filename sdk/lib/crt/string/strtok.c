/* taken from wine string.c */

#include <precomp.h>
#include <internal/wine/msvcrt.h>

/*********************************************************************
 *		strtok  (MSVCRT.@)
 */
char * CDECL strtok( char *str, const char *delim )
{
    thread_data_t *data = msvcrt_get_thread_data();
    char *ret;

    if (!str)
        if (!(str = data->strtok_next)) return NULL;

    while (*str && strchr( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !strchr( delim, *str )) str++;
    if (*str) *str++ = 0;
    data->strtok_next = str;
    return ret;
}

/*********************************************************************
 *		strtok_s  (MSVCRT.@)
 */
char * CDECL strtok_s(char *str, const char *delim, char **ctx)
{
    if (!MSVCRT_CHECK_PMT(delim != NULL) || !MSVCRT_CHECK_PMT(ctx != NULL) ||
        !MSVCRT_CHECK_PMT(str != NULL || *ctx != NULL)) {
        *_errno() = EINVAL;
        return NULL;
    }

    if(!str)
        str = *ctx;

    while(*str && strchr(delim, *str))
        str++;
    if(!*str)
        return NULL;

    *ctx = str+1;
    while(**ctx && !strchr(delim, **ctx))
        (*ctx)++;
    if(**ctx)
        *(*ctx)++ = 0;

    return str;
}
