/***
*mbsnicmp.c - Compare n characters of strings, ignoring case (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Compare n characters of strings, ignoring case (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <string.h>

/***
* _mbsnicmp - Compare n characters of strings, ignoring case (MBCS)
*
*Purpose:
*       Compares up to n charcters of two strings for ordinal order.
*       Strings are compared on a character basis, not a byte basis.
*       Case of characters is not considered.
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

extern "C" int __cdecl _mbsnicmp_l(
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
            return _strnicmp((const char *)s1, (const char *)s2, n);

        /* validation section */
        _VALIDATE_RETURN(s1 != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(s2 != nullptr, EINVAL, _NLSCMPERROR);

        while (n--) {

            c1 = *s1++;
            if ( _ismbblead_l(c1, _loc_update.GetLocaleT()) ) {
                if (*s1 == '\0')
                    c1 = 0;
                else {
                    c1 = ((c1<<8) | *s1++);

                    if ( ((c1 >= _MBUPPERLOW1_MT(_loc_update.GetLocaleT())) &&
                          (c1 <= _MBUPPERHIGH1_MT(_loc_update.GetLocaleT()))) )
                        c1 += _MBCASEDIFF1_MT(_loc_update.GetLocaleT());
                    else if ( ((c1 >= _MBUPPERLOW2_MT(_loc_update.GetLocaleT())) &&
                               (c1 <= _MBUPPERHIGH2_MT(_loc_update.GetLocaleT()))) )
                        c1 += _MBCASEDIFF2_MT(_loc_update.GetLocaleT());
                }
            }
            else
                c1 = _mbbtolower_l(c1, _loc_update.GetLocaleT());

            c2 = *s2++;
            if ( _ismbblead_l(c2, _loc_update.GetLocaleT()) ) {
                if (*s2 == '\0')
                    c2 = 0;
                else {
                    c2 = ((c2<<8) | *s2++);
                    if ( ((c2 >= _MBUPPERLOW1_MT(_loc_update.GetLocaleT())) &&
                          (c2 <= _MBUPPERHIGH1_MT(_loc_update.GetLocaleT()))) )
                        c2 += _MBCASEDIFF1_MT(_loc_update.GetLocaleT());
                    else if ( ((c2 >= _MBUPPERLOW2_MT(_loc_update.GetLocaleT())) &&
                               (c2 <= _MBUPPERHIGH2_MT(_loc_update.GetLocaleT()))) )
                        c2 += _MBCASEDIFF2_MT(_loc_update.GetLocaleT());
                }
            }
            else
                c2 = _mbbtolower_l(c2, _loc_update.GetLocaleT());

            if (c1 != c2)
                return( (c1 > c2) ? 1 : -1 );

            if (c1 == 0)
                return(0);
        }

        return(0);
}

extern "C" int (__cdecl _mbsnicmp)(
        const unsigned char *s1,
        const unsigned char *s2,
        size_t n
        )
{
    return _mbsnicmp_l(s1, s2, n, nullptr);
}
