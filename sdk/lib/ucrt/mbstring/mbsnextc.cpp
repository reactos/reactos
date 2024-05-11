/***
*mbsnextc.c - Get the next character in an MBCS string.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       To return the value of the next character in an MBCS string.
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>
#include <stdlib.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
*_mbsnextc:  Returns the next character in a string.
*
*Purpose:
*       To return the value of the next character in an MBCS string.
*       Does not advance pointer to the next character.
*
*Entry:
*       unsigned char *s = string
*
*Exit:
*       unsigned int next = next character.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" unsigned int __cdecl _mbsnextc_l(
        const unsigned char *s,
        _locale_t plocinfo
        )
{
        unsigned int  next = 0;
        _LocaleUpdate _loc_update(plocinfo);

        /* validation section */
        _VALIDATE_RETURN(s != nullptr, EINVAL, 0);

        /* don't skip forward 2 if the leadbyte is followed by EOS (dud string)
           also don't assert as we are too low-level
        */
        if ( _ismbblead_l(*s, _loc_update.GetLocaleT()) && s[1]!='\0')
            next = ((unsigned int) *s++) << 8;

        next += (unsigned int) *s;

        return(next);
}
extern "C" unsigned int (__cdecl _mbsnextc)(
        const unsigned char *s
        )
{
    return _mbsnextc_l(s, nullptr);
}
