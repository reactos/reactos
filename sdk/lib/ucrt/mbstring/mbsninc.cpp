/***
*mbsninc.c - Increment MBCS string pointer by specified char count.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Increment MBCS string pointer by specified char count.
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <mbstring.h>
#include <stddef.h>

/***
*_mbsninc - Increment MBCS string pointer by specified char count.
*
*Purpose:
*       Increment the supplied string pointer by the specified number
*       of characters.  MBCS characters are handled correctly.
*
*Entry:
*       const unsigned char *string = pointer to string
*       unsigned int ccnt = number of char to advance the pointer
*
*Exit:
*       Returns pointer after advancing it.
*       Returns pointer to end of string if string is not ccnt chars long.
*       Returns nullptr is supplied pointer is nullptr.
*
*Exceptions:
*
*******************************************************************************/

extern "C" unsigned char * __cdecl _mbsninc_l(
        const unsigned char *string,
        size_t ccnt,
        _locale_t plocinfo
        )
{
    if (string == nullptr)
        return nullptr;

    return const_cast<unsigned char*>(string) + (unsigned int)_mbsnbcnt_l(string, ccnt, plocinfo);
}

extern "C" unsigned char * (__cdecl _mbsninc)(
        const unsigned char *string,
        size_t ccnt
        )
{
    return _mbsninc_l(string, ccnt, nullptr);
}
