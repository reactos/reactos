/***
*mbsncmp.c - Compare n characters of two MBCS strings
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Compare n characters of two MBCS strings
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
*int mbsncmp(s1, s2, n) - Compare n characters of two MBCS strings
*
*Purpose:
*       Compares up to n characters of two strings for ordinal order.
*       Strings are compared on a character basis, not a byte basis.
*
*       UTF-8 and SBCS are merely compared in byte order.
*       DBCS are compared by codepoint to ensure double byte chars sort last
*
*Entry:
*       unsigned char *s1, *s2 = strings to compare
*       size_t n = maximum number of characters to compare
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

extern "C" int __cdecl _mbsncmp_l(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n,
        _locale_t plocinfo
        )
{
        unsigned short c1, c2;
        _LocaleUpdate _loc_update(plocinfo);

        if (n==0)
            return(0);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return strncmp((const char *)s1, (const char *)s2, n);

        /* validation section */
        _VALIDATE_RETURN(s1 != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(s2 != nullptr, EINVAL, _NLSCMPERROR);

        while (n--) {

            c1 = *s1++;
            if ( _ismbblead_l(c1, _loc_update.GetLocaleT()) )
                c1 = ( (*s1 == '\0') ? 0 : ((c1<<8) | *s1++) );

            c2 = *s2++;
            if ( _ismbblead_l(c2, _loc_update.GetLocaleT()) )
                c2 = ( (*s2 == '\0') ? 0 : ((c2<<8) | *s2++) );

            if (c1 != c2)
                return( (c1 > c2) ? 1 : -1);

            if (c1 == 0)
                return(0);
        }

        return(0);
}

extern "C" int (__cdecl _mbsncmp)(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n
        )
{
    return _mbsncmp_l(s1, s2, n, nullptr);
}
