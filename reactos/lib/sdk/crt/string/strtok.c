/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <precomp.h>

#include <string.h>
#include <internal/wine/msvcrt.h>
#include <internal/tls.h>

/*********************************************************************
 *		strtok  (MSVCRT.@)
 */
char* strtok(char* str, const char* delim)
{
    MSVCRT_thread_data *data = msvcrt_get_thread_data();
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
    if(!delim || !ctx || (!str && !*ctx)) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
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
