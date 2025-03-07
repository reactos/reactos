/***
*mbsnccnt.c - Return char count of MBCS string
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Return char count of MBCS string
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

/***
* _mbsnccnt - Return char count of MBCS string
*
*Purpose:
*       Returns the number of chars between the start of the supplied
*       string and the byte count supplied.  That is, this routine
*       indicates how many chars are in the first "bcnt" bytes
*       of the string.
*
*Entry:
*       const unsigned char *string = pointer to string
*       unsigned int bcnt = number of bytes to scan
*
*Exit:
*       Returns number of chars between string and bcnt.
*
*       If the end of the string is encountered before bcnt chars were
*       scanned, then the length of the string in chars is returned.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" size_t __cdecl _mbsnccnt_l(
        const unsigned char *string,
        size_t bcnt,
        _locale_t plocinfo
        )
{
        size_t n;
        _LocaleUpdate _loc_update(plocinfo);

        _VALIDATE_RETURN(string != nullptr || bcnt == 0, EINVAL, 0);

        for (n = 0; (bcnt-- && *string); n++, string++) {
            if ( _ismbblead_l(*string, _loc_update.GetLocaleT()) ) {
                if ( (!bcnt--) || (*++string == '\0'))
                    break;
            }
        }

        return(n);
}
extern "C" size_t (__cdecl _mbsnccnt)(
        const unsigned char *string,
        size_t bcnt
        )
{
    return _mbsnccnt_l(string, bcnt, nullptr);
}
