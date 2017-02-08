#include <precomp.h>


__int64
_strtoi64(const char *nptr, char **endptr, int base)
{
    BOOL negative = FALSE;
    __int64 ret = 0;

   while(isspace((unsigned char)*nptr)) nptr++;

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

        if(isdigit((unsigned char)cur)) {
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
            *_errno() = ERANGE;
        } else if(negative && (ret<_I64_MIN/base || ret*base<_I64_MIN-v)) {
            ret = _I64_MIN;
            *_errno() = ERANGE;
        } else
            ret = ret*base + v;
    }

    if(endptr)
        *endptr = (char*)nptr;

    return ret;
}


/* EOF */
