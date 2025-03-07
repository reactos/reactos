/***
*mbsnbcnt.c - Returns byte count of MBCS string
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Returns byte count of MBCS string
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

/***
* _mbsnbcnt - Returns byte count of MBCS string
*
*Purpose:
*       Returns the number of bytes between the start of the supplied
*       string and the char count supplied.  That is, this routine
*       indicates how many bytes are in the first "ccnt" characters
*       of the string.
*
*Entry:
*       unsigned char *string = pointer to string
*       unsigned int ccnt = number of characters to scan
*
*Exit:
*       Returns number of bytes between string and ccnt.
*
*       If the end of the string is encountered before ccnt chars were
*       scanned, then the length of the string in bytes is returned.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" size_t __cdecl _mbsnbcnt_l(
        const unsigned char *string,
        size_t ccnt,
        _locale_t plocinfo
        )
{
        unsigned char *p;
        _LocaleUpdate _loc_update(plocinfo);

        /* validation section */
        _VALIDATE_RETURN(string != nullptr || ccnt == 0, EINVAL, 0);

        for (p = (unsigned char *)string; (ccnt-- && *p); p++) {
            if ( _ismbblead_l(*p, _loc_update.GetLocaleT()) ) {
                if (*++p == '\0') {
                    --p;
                    break;
                }
            }
        }

        return ((size_t) ((char *)p - (char *)string));
}

extern "C" size_t (__cdecl _mbsnbcnt)(
        const unsigned char *string,
        size_t ccnt
        )
{
        return _mbsnbcnt_l(string, ccnt, nullptr);
}
