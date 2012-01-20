/* taken from wine wcs.c */

#include <precomp.h>

/*********************************************************************
 *		wcstok_s  (MSVCRT.@)
 */
wchar_t * CDECL wcstok_s( wchar_t *str, const wchar_t *delim,
                                 wchar_t **next_token )
{
    wchar_t *ret;

    if (!MSVCRT_CHECK_PMT(delim != NULL) || !MSVCRT_CHECK_PMT(next_token != NULL) ||
        !MSVCRT_CHECK_PMT(str != NULL || *next_token != NULL))
    {
        _set_errno(EINVAL);
        return NULL;
    }
    if (!str) str = *next_token;

    while (*str && strchrW( delim, *str )) str++;
    if (!*str) return NULL;
    ret = str++;
    while (*str && !strchrW( delim, *str )) str++;
    if (*str) *str++ = 0;
    *next_token = str;
    return ret;
}

/*********************************************************************
 *		wcstok  (MSVCRT.@)
 */
wchar_t * CDECL wcstok( wchar_t *str, const wchar_t *delim )
{
    return wcstok_s(str, delim, &msvcrt_get_thread_data()->wcstok_next);
}
