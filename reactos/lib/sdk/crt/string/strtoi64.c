#include <precomp.h>

/*********************************************************************
 *  _strtoi64_l (MSVCR90.@)
 *
 * FIXME: locale parameter is ignored
 */
__int64 CDECL _strtoi64_l(const char *nptr, char **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", nptr, endptr, base, locale);

    if(!nptr || base<0 || base>36 || base==1) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return 0;
    }

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

        if(!negative && (ret>INT64_MAX/base || ret*base>INT64_MAX-v)) {
            ret = INT64_MAX;
            *_errno() = ERANGE;
        } else if(negative && (ret<INT64_MIN/base || ret*base<INT64_MIN-v)) {
            ret = INT64_MIN;
            *_errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return ret;
}

__int64
_strtoi64(const char *nptr, char **endptr, int base)
{
   return _strtoi64_l(nptr, endptr, base, NULL);
}

/*********************************************************************
 *  _strtoui64_l (MSVCR90.@)
 *
 * FIXME: locale parameter is ignored
 */
unsigned __int64 CDECL _strtoui64_l(const char *nptr, char **endptr, int base, _locale_t locale)
{
    BOOL negative = FALSE;
    unsigned __int64 ret = 0;

    TRACE("(%s %p %d %p)\n", nptr, endptr, base, locale);

    if(!nptr || base<0 || base>36 || base==1) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        return 0;
    }

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
            v = *nptr-'0';
        } else {
            if(cur<'a' || cur>='a'+base-10)
                break;
            v = cur-'a'+10;
        }

        nptr++;

        if(ret>UINT64_MAX/base || ret*base>UINT64_MAX-v) {
            ret = UINT64_MAX;
            *_errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return negative ? -ret : ret;
}

/*********************************************************************
 *  _strtoui64 (MSVCR90.@)
 */
unsigned __int64 CDECL _strtoui64(const char *nptr, char **endptr, int base)
{
    return _strtoui64_l(nptr, endptr, base, NULL);
}


/* EOF */
