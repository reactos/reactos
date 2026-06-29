/***
*ismbspc.c - Test is character is whitespace (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Test is character is whitespace (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

/***
* _ismbcspace - Test is character is whitespace (MBCS)
*
*Purpose:
*       Test if the character is a whitespace character.
*       Handles MBCS chars correctly.
*
*       Note:  Use test against 0x00FF instead of _ISLEADBYTE
*       to ensure that we don't call SBCS routine with a two-byte
*       value.
*
*Entry:
*       unsigned int c = character to test
*
*Exit:
*       Returns TRUE if character is whitespace, else FALSE
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _ismbcspace_l(unsigned int const c, _locale_t const locale)
{
    _LocaleUpdate locale_update(locale);

    if (c <= 0x00FF)
    {
        return _isspace_l(c, locale_update.GetLocaleT());
    }

    return __dcrt_multibyte_check_type(c, locale_update.GetLocaleT(), _SPACE, true);
}

extern "C" int __cdecl _ismbcspace(unsigned int const c)
{
    return _ismbcspace_l(c, nullptr);
}
