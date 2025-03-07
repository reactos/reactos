/***
*mbsncpy.c - Copy one string to another, n chars only (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Copy one string to another, n chars only (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_INCORRECT_VALIDATION __WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26014 26018

/***
* _mbsncpy - Copy one string to another, n chars only (MBCS)
*
*Purpose:
*       Copies exactly cnt character from src to dst.  If strlen(src) < cnt, the
*       remaining character are padded with null bytes.  If strlen >= cnt, no
*       terminating null byte is added.  2-byte MBCS characters are handled
*       correctly.
*
*Entry:
*       unsigned char *dst = destination for copy
*       unsigned char *src = source for copy
*       int cnt = number of characters to copy
*
*Exit:
*       returns dst = destination of copy
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

#pragma warning(suppress:6101) // Returning uninitialized memory '*dst'.  A successful path through the function does not set the named _Out_ parameter.
extern "C" unsigned char * __cdecl _mbsncpy_l(
        unsigned char *dst,
        const unsigned char *src,
        size_t cnt,
        _locale_t plocinfo
        )
{
        unsigned char *start = dst;
        _LocaleUpdate _loc_update(plocinfo);

        /* validation section */
        _VALIDATE_RETURN(dst != nullptr || cnt == 0, EINVAL, nullptr);
        _VALIDATE_RETURN(src != nullptr || cnt == 0, EINVAL, nullptr);

        _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
        if (_loc_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
#pragma warning(suppress:__WARNING_BANNED_API_USAGE)
            return (unsigned char *)strncpy((char *)dst, (const char *)src, cnt);
        _END_SECURE_CRT_DEPRECATION_DISABLE

        while (cnt) {

            cnt--;
            if ( _ismbblead_l(*src, _loc_update.GetLocaleT()) ) {
                *dst++ = *src++;
                if ((*dst++ = *src++) == '\0') {
                    dst[-2] = '\0';
                    break;
                }
            }
            else
                if ((*dst++ = *src++) == '\0')
                    break;

        }

        /* pad with nulls as needed */

        while (cnt--)
            *dst++ = '\0';

#pragma warning(suppress:__WARNING_POSTCONDITION_NULLTERMINATION_VIOLATION) // 26036 REVIEW annotation mistake?
        return start;
}
extern "C" unsigned char * (__cdecl _mbsncpy)(
        unsigned char *dst,
        const unsigned char *src,
        size_t cnt
        )
{
    _BEGIN_SECURE_CRT_DEPRECATION_DISABLE
    return _mbsncpy_l(dst, src, cnt, nullptr);
    _END_SECURE_CRT_DEPRECATION_DISABLE
}
