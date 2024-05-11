/***
*ismbdgt.cpp - Test if character is a digit (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Test if character is a digit (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>


/***
* _ismbcdigit - Test if character is a digit (MBCS)
*
*Purpose:
*       Tests the character to see if it is a digit.
*       Handles MBCS chars correctly.
*
*       Note:  Use test against 0x00FF instead of _ISLEADBYTE
*       to ensure that we don't call SBCS routine with a two-byte
*       value.
*
*Entry:
*       unsigned int *c = character to test
*
*Exit:
*       Returns TRUE if character is a digit, else FALSE
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _ismbcdigit_l(unsigned int const c, _locale_t const locale)
{
    _LocaleUpdate locale_update(locale);

    if (c <= 0x00FF)
    {
        return _isdigit_fast_internal(static_cast<unsigned char>(c), locale_update.GetLocaleT());
    }

    return __dcrt_multibyte_check_type(c, locale_update.GetLocaleT(), _DIGIT, true);
}

extern "C" int __cdecl _ismbcdigit(unsigned int const c)
{
    return _ismbcdigit_l(c, nullptr);
}
