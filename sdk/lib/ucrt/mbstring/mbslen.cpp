/***
*mbslen.c - Find length of MBCS string
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Find length of MBCS string
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal.h>
#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <string.h>

/***
* _mbslen - Find length of MBCS string
*
*Purpose:
*       Find the length of the MBCS string (in characters).
*
*Entry:
*       unsigned char *s = string
*
*Exit:
*       Returns the number of MBCS chars in the string.
*
*Exceptions:
*
*******************************************************************************/

extern "C" size_t __cdecl _mbslen_l(
        const unsigned char *s,
        _locale_t plocinfo
        )
{
        int n;
        _LocaleUpdate _loc_update(plocinfo);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return strlen((const char *)s);

        for (n = 0; *s; n++, s++) {
            if ( _ismbblead_l(*s, _loc_update.GetLocaleT()) ) {
                if (*++s == '\0')
                    break;
            }
        }

        return(n);
}

extern "C" size_t (__cdecl _mbslen)(
        const unsigned char *s
        )
{
    return _mbslen_l(s, nullptr);
}
