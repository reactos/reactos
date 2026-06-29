/***
*ismblwr - Test if character is lower case (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Test if character is lower case (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>


/***
* _ismbclower - Test if character is lower case (MBCS)
*
*Purpose:
*       Test if the supplied character is lower case or not.
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
*       returns TRUE if character is lower case, else FALSE
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _ismbclower_l(unsigned int const c, _locale_t const locale)
{
    _LocaleUpdate locale_update(locale);

    if (c <= 0x00FF)
    {
        return _mbbislower_l(c, locale_update.GetLocaleT());
    }

    return __dcrt_multibyte_check_type(c, locale_update.GetLocaleT(), _LOWER, true);
}

extern "C" int __cdecl _ismbclower(unsigned int const c)
{
    return _ismbclower_l(c, nullptr);
}
