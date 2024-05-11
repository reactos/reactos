/***
*mbsicmp.c - Case-insensitive string comparision routine (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Case-insensitive string comparision routine (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
* _mbsicmp - Case-insensitive string comparision routine (MBCS)
*
*Purpose:
*       Compares two strings for ordinal order without regard to case.
*       Strings are compared on a character basis, not a byte basis.
*
*Entry:
*       char *s1, *s2 = strings to compare
*
*Exit:
*       Returns <0 if s1 < s2
*       Returns  0 if s1 == s2
*       Returns >0 if s1 > s2
*       Returns _NLSCMPERROR if something went wrong
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _mbsicmp_l(
        const unsigned char *s1,
        const unsigned char *s2,
        _locale_t plocinfo
        )
{
        unsigned short c1, c2;
        _LocaleUpdate _loc_update(plocinfo);
        int    retval;
        unsigned char szResult[4];

        /* validation section */
        _VALIDATE_RETURN(s1 != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(s2 != nullptr, EINVAL, _NLSCMPERROR);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return _stricmp_l((const char *)s1, (const char *)s2, _loc_update.GetLocaleT());

        for (;;)
        {
            c1 = *s1++;
            if ( _ismbblead_l(c1, _loc_update.GetLocaleT()) )
            {
                if (*s1 == '\0')
                    c1 = 0;
                else
                {
                    retval = __acrt_LCMapStringA(
                            _loc_update.GetLocaleT(),
                            _loc_update.GetLocaleT()->mbcinfo->mblocalename,
                            LCMAP_UPPERCASE,
                            (LPCSTR)s1 - 1,
                            2,
                            (LPSTR)szResult,
                            2,
                            _loc_update.GetLocaleT()->mbcinfo->mbcodepage,
                            TRUE );

                    if (retval == 1)
                        c1 = szResult[0];
                    else if (retval == 2)
                        c1 = (szResult[0] << 8) + szResult[1];
                    else
                    {
                        errno = EINVAL;
                        return _NLSCMPERROR;
                    }
                    s1++;
                }
            }
            else
                c1 = _mbbtolower_l(c1, _loc_update.GetLocaleT());

            c2 = *s2++;
            if ( _ismbblead_l(c2, _loc_update.GetLocaleT()) )
            {
                if (*s2 == '\0')
                    c2 = 0;
                else
                {
                    retval = __acrt_LCMapStringA(
                            _loc_update.GetLocaleT(),
                            _loc_update.GetLocaleT()->mbcinfo->mblocalename,
                            LCMAP_UPPERCASE,
                            (LPCSTR)s2 - 1,
                            2,
                            (LPSTR)szResult,
                            2,
                            _loc_update.GetLocaleT()->mbcinfo->mbcodepage,
                            TRUE );

                    if (retval == 1)
                        c2 = szResult[0];
                    else if (retval == 2)
                        c2 = (szResult[0] << 8) + szResult[1];
                    else
                    {
                        errno = EINVAL;
                        return _NLSCMPERROR;
                    }
                    s2++;
                }
            }
            else
                c2 = _mbbtolower_l(c2, _loc_update.GetLocaleT());

            if (c1 != c2)
                return( (c1 > c2) ? 1 : -1 );

            if (c1 == 0)
                return(0);
        }
}

extern "C" int (__cdecl _mbsicmp)(
        const unsigned char *s1,
        const unsigned char *s2
        )
{
    return _mbsicmp_l(s1, s2, nullptr);
}
