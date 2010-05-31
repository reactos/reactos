
#include <precomp.h>
#include <locale.h>
#include <internal/wine/msvcrt.h>

/*********************************************************************
 *		_strtod_l  (MSVCRT.@)
 */
double CDECL _strtod_l( const char *str, char **end, _locale_t locale)
{
    unsigned __int64 d=0, hlp;
    int exp=0, sign=1;
    const char *p;
    double ret;

    if(!str) {
        _invalid_parameter(NULL, NULL, NULL, 0, 0);
        *_errno() = EINVAL;
        return 0;
    }

    if(!locale)
        locale = get_locale();

    /* FIXME: use *_l functions */
    p = str;
    while(isspace(*p))
        p++;

    if(*p == '-') {
        sign = -1;
        p++;
    } else  if(*p == '+')
        p++;

    while(isdigit(*p)) {
        hlp = d*10+*(p++)-'0';
        if(d>UINT64_MAX/10 || hlp<d) {
            exp++;
            break;
        } else
            d = hlp;
    }
    while(isdigit(*p)) {
        exp++;
        p++;
    }

    if(*p == *locale->locinfo->lconv->decimal_point)
        p++;

    while(isdigit(*p)) {
        hlp = d*10+*(p++)-'0';
        if(d>UINT64_MAX/10 || hlp<d)
            break;

        d = hlp;
        exp--;
    }
    while(isdigit(*p))
        p++;

    if(p == str) {
        if(end)
            *end = (char*)str;
        return 0.0;
    }

    if(*p=='e' || *p=='E' || *p=='d' || *p=='D') {
        int e=0, s=1;

        p++;
        if(*p == '-') {
            s = -1;
            p++;
        } else if(*p == '+')
            p++;

        if(isdigit(*p)) {
            while(isdigit(*p)) {
                if(e>INT_MAX/10 || (e=e*10+*p-'0')<0)
                    e = INT_MAX;
                p++;
            }
            e *= s;

            if(exp<0 && e<0 && exp+e>=0) exp = INT_MIN;
            else if(exp>0 && e>0 && exp+e<0) exp = INT_MAX;
            else exp += e;
        } else {
            if(*p=='-' || *p=='+')
                p--;
            p--;
        }
    }

    if(exp>0)
        ret = (double)sign*d*pow(10, exp);
    else
        ret = (double)sign*d/pow(10, -exp);

    if((d && ret==0.0) || isinf(ret))
        *_errno() = ERANGE;

    if(end)
        *end = (char*)p;

    return ret;
}

/*
 * @implemented
 */
double
strtod(const char *str, char **end)
{
    return _strtod_l( str, end, NULL );
}
