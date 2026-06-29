/***
*mbtokata.c - Converts character to katakana.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts a character from hiragana to katakana.
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>


/***
*unsigned short _mbctokata(c) - Converts character to katakana.
*
*Purpose:
*       If the character c is hiragana, convert to katakana.
*
*Entry:
*       unsigned int c - Character to convert.
*
*Exit:
*       Returns converted character.
*
*Exceptions:
*
*******************************************************************************/

extern "C" unsigned int __cdecl _mbctokata_l(
        unsigned int c,
        _locale_t plocinfo
        )
{
        if (_ismbchira_l(c, plocinfo)) {
                c += 0xa1;
                if (c >= 0x837f)
                        c++;
        }
        return(c);
}

extern "C" unsigned int (__cdecl _mbctokata)(
        unsigned int c
        )
{
    return _mbctokata_l(c, nullptr);
}
