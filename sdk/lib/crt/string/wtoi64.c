#include <precomp.h>
#include <internal/wine/msvcrt.h>

/*********************************************************************
 *              _wtoi64_l (MSVCRT.@)
 */
__int64 CDECL _wtoi64_l(const wchar_t *str, _locale_t locale)
{
    ULONGLONG RunningTotal = 0;
    BOOL bMinus = FALSE;

    while (iswctype((int)*str, _SPACE)) {
        str++;
    } /* while */

    if (*str == '+') {
        str++;
    } else if (*str == '-') {
        bMinus = TRUE;
        str++;
    } /* if */

    while (*str >= '0' && *str <= '9') {
        RunningTotal = RunningTotal * 10 + *str - '0';
        str++;
    } /* while */

    return bMinus ? -(__int64)RunningTotal : RunningTotal;
}

/*********************************************************************
 *              _wtoi64 (MSVCRT.@)
 */
__int64 CDECL _wtoi64(const wchar_t *str)
{
    return _wtoi64_l(str, NULL);
}


/*********************************************************************
 *  _wcstoi64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL _wcstoi64_l(const wchar_t *nptr,
        wchar_t **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

#ifndef _LIBCNT_
    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);
#endif

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while (iswctype((int)*nptr, _SPACE)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && towlower(*(nptr+1))=='x') {
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
        wchar_t cur = towlower(*nptr);
        int v;

        if(cur>='0' && cur<='9') {
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
        *endptr = (wchar_t*)nptr;

    return ret;
}

/*********************************************************************
 *  _wcstoi64 (MSVCRT.@)
 */
__int64 CDECL _wcstoi64(const wchar_t *nptr,
        wchar_t **endptr, int base)
{
    return _wcstoi64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _wcstoui64_l (MSVCRT.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL _wcstoui64_l(const wchar_t *nptr,
        wchar_t **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE;
    unsigned __int64 ret = 0;

#ifndef _LIBCNT_
    TRACE("(%s %p %d %p)\n", debugstr_w(nptr), endptr, base, locale);
#endif

    if (!MSVCRT_CHECK_PMT(nptr != NULL)) return 0;
    if (!MSVCRT_CHECK_PMT(base == 0 || base >= 2)) return 0;
    if (!MSVCRT_CHECK_PMT(base <= 36)) return 0;

    while (iswctype((int)*nptr, _SPACE)) nptr++;

    if(*nptr == '-') {
        negative = TRUE;
        nptr++;
    } else if(*nptr == '+')
        nptr++;

    if((base==0 || base==16) && *nptr=='0' && towlower(*(nptr+1))=='x') {
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
        wchar_t cur = towlower(*nptr);
        int v;

        if(cur>='0' && cur<='9') {
            if(cur >= '0'+base)
                break;
            v = *nptr-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }

        nptr++;

        if(ret>_UI64_MAX/base || ret*base>_UI64_MAX-v) {
            ret = _UI64_MAX;
#ifndef _LIBCNT_
            *_errno() = ERANGE;
#endif
            } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (wchar_t*)nptr;

    return negative ? -(__int64)ret : ret;
}

/*********************************************************************
 *  _wcstoui64 (MSVCRT.@)
 */
unsigned __int64 CDECL _wcstoui64(const wchar_t *nptr,
        wchar_t **endptr, int base)
{
    return _wcstoui64_l(nptr, endptr, base, NULL);
}


/* EOF */
