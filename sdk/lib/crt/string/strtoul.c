#include <precomp.h>

/* Based on Wine Staging 1.9.9 - dlls/msvcrt/string.c */
__int64 CDECL strtoi64_l(const char *nptr, char **endptr, int base, _locale_t locale);

/******************************************************************
 *		_strtoul_l (MSVCRT.@)
 */
unsigned long CDECL strtoul_l(const char* nptr, char** end, int base, _locale_t locale)
{
    __int64 ret = strtoi64_l(nptr, end, base, locale);

    if(ret > ULONG_MAX) {
        ret = ULONG_MAX;
#ifndef _LIBCNT_
        *_errno() = ERANGE;
#endif
    }else if(ret < -(__int64)ULONG_MAX) {
        ret = 1;
#ifndef _LIBCNT_
        *_errno() = ERANGE;
#endif
    }

    return ret;
}

/******************************************************************
 *		strtoul (MSVCRT.@)
 */
unsigned long CDECL strtoul(const char* nptr, char** end, int base)
{
    return strtoul_l(nptr, end, base, NULL);
}
