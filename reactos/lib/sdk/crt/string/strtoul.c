#include <precomp.h>

/* Based on Wine Staging 1.7.37 - dlls/msvcrt/string.c */

/*********************************************************************
 *  _strtoi64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL strtoi64_l(const char *nptr, char **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

#ifndef _LIBCNT_
    TRACE("(%s %p %d %p)\n", debugstr_a(nptr), endptr, base, locale);
#endif

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while(isspace(*nptr)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && tolower(*(nptr+1))=='x') {
        base = 16;
        nptr += 2;
    }

    if(base == 0) {
        if(*nptr=='0')
            base = 8;
        else
            base = 10;
    }

    while(*nptr) {
        char cur = tolower(*nptr);
        int v;

        if(isdigit(cur)) {
            if(cur >= '0'+base)
                break;
            v = cur-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }

        if(negative)
            v = -v;

        nptr++;

        if(!negative && (ret>_I64_MAX/base || ret*base>_I64_MAX-v)) {
            ret = _I64_MAX;
#ifndef _LIBCNT_
            *_errno() = ERANGE;
#endif
        } else if(negative && (ret<_I64_MIN/base || ret*base<_I64_MIN-v)) {
            ret = _I64_MIN;
#ifndef _LIBCNT_
            *_errno() = ERANGE;
#endif
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return ret;
}

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
