/***
*ismbalnm - Test if character is alpha numeric (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Test if character is alpha numeric (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>


/***
* _ismbcalnum - Test if character is alpha numeric (MBCS)
*
*Purpose:
*       Test if the supplied character is alpha numeric or not.
*       Handles MBCS characters correctly.
*
*       Note:  Use test against 0x00FF instead of _ISLEADBYTE
*       to ensure that we don't call SBCS routine with a two-byte
*       value.
*
*Entry:
*       unsigned int c = character to test
*
*Exit:
*       Returns TRUE if c is an alpha numeric character; else FALSE
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _ismbcalnum_l(unsigned int const c, _locale_t const locale)
{
    _LocaleUpdate locale_update(locale);

    if (c <= 0x00FF)
    {
        return _ismbbalnum_l(c, locale_update.GetLocaleT());
    }

    return __dcrt_multibyte_check_type(c, locale_update.GetLocaleT(), _ALPHA | _DIGIT, true);
}

extern "C" int __cdecl _ismbcalnum(unsigned int const c)
{
    return _ismbcalnum_l(c, nullptr);
}
