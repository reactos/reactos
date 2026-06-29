/***
*mbscmp.c - Compare MBCS strings
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Compare MBCS strings
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
* _mbscmp - Compare MBCS strings
*
*Purpose:
*       Compares two strings in ordinal order. 
*       Single byte strings are compared byte-by-byte
*       Double byte strings are compared by character
*           (basically ensuring that double byte sequences are higher than single byte)
*       UTF-8 is compared byte-by-byte, which work since its the same as character order.
*
*Entry:
*       char *s1, *s2 = strings to compare
*
*WARNING:
*       No validation for improper UTF-8 strings, mixed up lead/trail byte orders are not defined.
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

extern "C" int __cdecl _mbscmp_l(
        const unsigned char *s1,
        const unsigned char *s2,
        _locale_t plocinfo
        )
{
        unsigned short c1, c2;
        _LocaleUpdate _loc_update(plocinfo);

        /* validation section */
        _VALIDATE_RETURN(s1 != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(s2 != nullptr, EINVAL, _NLSCMPERROR);
        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return strcmp((const char *)s1, (const char *)s2);

        for (;;) {

            c1 = *s1++;
            if ( _ismbblead_l(c1, _loc_update.GetLocaleT()) )
                c1 = ( (*s1 == '\0') ? 0 : ((c1<<8) | *s1++) );

            c2 = *s2++;
            if ( _ismbblead_l(c2, _loc_update.GetLocaleT()) )
                c2 = ( (*s2 == '\0') ? 0 : ((c2<<8) | *s2++) );

            if (c1 != c2)
                return (c1 > c2) ? 1 : -1;

            if (c1 == 0)
                return 0;

        }
}

extern "C" int (__cdecl _mbscmp)(
        const unsigned char *s1,
        const unsigned char *s2
        )
{
    return _mbscmp_l(s1, s2, nullptr);
}
