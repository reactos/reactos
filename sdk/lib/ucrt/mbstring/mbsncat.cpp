/***
*mbsncat.c - concatenate string2 onto string1, max length n
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       defines mbsncat() - concatenate maximum of n characters
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <string.h>


/***
* _mbsncat - concatenate max cnt characters onto dst
*
*Purpose:
*       Concatenates src onto dst, with a maximum of cnt characters copied.
*       Handles 2-byte MBCS characters correctly.
*
*Entry:
*       unsigned char *dst - string to concatenate onto
*       unsigned char *src - string to concatenate from
*       int cnt - number of characters to copy
*
*Exit:
*       returns dst, with src (at least part) concatenated on
*
*Exceptions:
*
*******************************************************************************/

extern "C" unsigned char * __cdecl _mbsncat_l(
        unsigned char *dst,
        const unsigned char *src,
        size_t cnt,
        _locale_t plocinfo
        )
{
        unsigned char *start;

        if (!cnt)
            return(dst);

        /* validation section */
        _VALIDATE_RETURN(dst != nullptr, EINVAL, nullptr);
        _VALIDATE_RETURN(src != nullptr, EINVAL, nullptr);

        _LocaleUpdate _loc_update(plocinfo);

        _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
            return (unsigned char *)strncat((char *)dst, (const char *)src, cnt);
        _END_SECURE_CRT_DEPRECATION_DISABLE

        start = dst;
        while (*dst++)
                ;
        --dst;          // dst now points to end of dst string


        /* if last char in string is a lead byte, back up pointer */

        if ( _ismbslead_l(start, dst, _loc_update.GetLocaleT()) )
            --dst;

        /* copy over the characters */

        while (cnt--) {
            if ( _ismbblead_l(*src, _loc_update.GetLocaleT()) ) {
                *dst++ = *src++;
                if ((*dst++ = *src++) == '\0') {
                    dst[-2] = '\0';
                    break;
                }
            }

            else if ((*dst++ = *src++) == '\0')
                break;

        }

        /* enter final nul, if necessary */
        if ( dst!=start && _mbsbtype_l(start, (int) ((dst - start) - 1), _loc_update.GetLocaleT()) == _MBC_LEAD )
        {
            dst[-1] = '\0';
        }
        else
        {
            *dst = '\0';
        }

        return(start);
}

extern "C" unsigned char * (__cdecl _mbsncat)(
        unsigned char *dst,
        const unsigned char *src,
        size_t cnt
        )
{
    _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
    return _mbsncat_l(dst, src, cnt, nullptr);
    _END_SECURE_CRT_DEPRECATION_DISABLE
}
