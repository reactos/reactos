/* Taken from Wine Staging msvcrt/string.c */

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
