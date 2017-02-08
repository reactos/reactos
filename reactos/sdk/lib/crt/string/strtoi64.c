#include <precomp.h>

/* Based on Wine Staging 1.9.9 - dlls/msvcrt/string.c */

/*********************************************************************
*  _strtoi64_l (MSVCRT.@)
*
* FIXME: locale parameter is ignored
*/
__int64 CDECL _strtoi64_l(const char *nptr, char **endptr, int base, _locale_t locale)
{
    const char *p = nptr;
    BOOL negative = FALSE;
    BOOL got_digit = FALSE;
    __int64 ret = 0;

#ifndef _LIBCNT_
    TRACE("(%s %p %d %p)\n", debugstr_a(nptr), endptr, base, locale);
#endif

#ifndef __REACTOS__
    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
#endif
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while (isspace(*nptr)) nptr++;

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
        got_digit = TRUE;

        if(negative)
            v = -v;

        nptr++;

        if(!negative && (ret>_I64_MAX/base || ret*base>_I64_MAX-v)) {
            ret = _I64_MAX;
#ifndef _LIBCNT_
            *_errno() = ERANGE;
#endif
        }
        else if (negative && (ret<_I64_MIN / base || ret*base<_I64_MIN - v)) {
            ret = _I64_MIN;
#ifndef _LIBCNT_
            *_errno() = ERANGE;
#endif
        }
        else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)(got_digit ? nptr : p);

    return ret;
}

__int64
_strtoi64(const char *nptr, char **endptr, int base)
{
    return _strtoi64_l(nptr, endptr, base, NULL);
}


/* EOF */
