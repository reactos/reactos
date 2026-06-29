/***
*mbsrchr.c - Search for last occurence of character (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Search for last occurence of character (MBCS)
*
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <stddef.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
* _mbsrchr - Search for last occurence of character (MBCS)
*
*Purpose:
*       Find the last occurrence of the specified character in
*       the supplied string.  Handles MBCS chars/strings correctly.
*
*Entry:
*       unsigned char *str = string to search in
*       unsigned int c = character to search for
*
*Exit:
*       returns pointer to last occurrence of c in str
*       returns nullptr if c not found
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" _CONST_RETURN unsigned char * __cdecl _mbsrchr_l(
        const unsigned char *str,
        unsigned int c,
        _locale_t plocinfo
        )
{
        char *r = nullptr;
        unsigned int cc;
        _LocaleUpdate _loc_update(plocinfo);

        /* validation section */
        _VALIDATE_RETURN(str != nullptr, EINVAL, 0);

        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return (_CONST_RETURN unsigned char *)strrchr((const char *)str, c);

        do {
            cc = *str;
            if ( _ismbblead_l(cc, _loc_update.GetLocaleT()) ) {
                if(*++str) {
                    if (c == ((cc<<8)|*str))
                        r = (char *)str - 1;
                }
                else if(!r)
                    /* return pointer to '\0' */
                    r = (char *)str;
            }
            else if (c == cc)
                r = (char *)str;
        }
        while (*str++);

        return((_CONST_RETURN unsigned char *)r);
}

extern "C" _CONST_RETURN unsigned char * (__cdecl _mbsrchr)(
        const unsigned char *str,
        unsigned int c
        )
{
    return _mbsrchr_l(str, c, nullptr);
}
