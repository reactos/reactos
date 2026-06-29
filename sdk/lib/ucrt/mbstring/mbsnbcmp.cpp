/***
*mbsnbcmp.c - Compare n bytes of two MBCS strings
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Compare n bytes of two MBCS strings
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
*int mbsnbcmp(s1, s2, n) - Compare n bytes of two MBCS strings
*
*Purpose:
*       Compares up to n bytes of two strings for ordinal order.
*
*       UTF-8 and SBCS are merely compared in byte order.
*       DBCS are compared by codepoint to ensure double byte chars sort last
*
*Entry:
*       unsigned char *s1, *s2 = strings to compare
*       size_t n = maximum number of bytes to compare
*
*Exit:
*       Returns <0 if s1 < s2
*       Returns  0 if s1 == s2
*       Returns >0 if s1 > s2
*       Returns _NLSCMPERROR is something went wrong
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _mbsnbcmp_l(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n,
        _locale_t plocinfo
        )
{
        unsigned short c1, c2;

        if (n==0)
                return(0);

        _LocaleUpdate _loc_update(plocinfo);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return strncmp((const char *)s1, (const char *)s2, n);

        /* validation section */
        _VALIDATE_RETURN(s1 != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(s2 != nullptr, EINVAL, _NLSCMPERROR);

        while (n--) {

            c1 = *s1++;
            if ( _ismbblead_l(c1, _loc_update.GetLocaleT()) )
            {
                if (n==0)
                {
                    c1 = 0; /* 'naked' lead - end of string */
                    c2 = _ismbblead_l(*s2, _loc_update.GetLocaleT()) ? 0 : *s2;
                    goto test;
                }
                c1 = ( (*s1 == '\0') ? 0 : ((c1<<8) | *s1++) );
            }

            c2 = *s2++;
            if ( _ismbblead_l(c2, _loc_update.GetLocaleT()) )
            {
                if (n==0)
                {
                    c2 = 0; /* 'naked' lead - end of string */
                    goto test;
                }
                --n;
                c2 = ( (*s2 == '\0') ? 0 : ((c2<<8) | *s2++) );
            }
test:
            if (c1 != c2)
                return( (c1 > c2) ? 1 : -1);

            if (c1 == 0)
                return(0);
        }

        return(0);
}

extern "C" int (__cdecl _mbsnbcmp)(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n
        )
{
    return _mbsnbcmp_l(s1, s2, n, nullptr);
}
