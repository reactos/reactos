/***
*mbsbtype.c - Return type of byte within a string (MBCS)
*
*       Copyright (c) Microsoft Corporation.  All rights reserved.
*
*Purpose:
*       Return type of byte within a string (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
* _mbsbtype - Return type of byte within a string
*
*Purpose:
*       Test byte within a string for MBCS char type.
*       This function requires the start of the string because
*       context must be taken into account.
*
*Entry:
*       const unsigned char *string = pointer to string
*       size_t len = position of the char in string
*
*Exit:
*       returns one of the following values:
*
*       _MBC_LEAD      = if 1st byte of MBCS char
*       _MBC_TRAIL     = if 2nd byte of MBCS char
*       _MBC_SINGLE    = valid single byte char
*
*       _MBC_ILLEGAL   = if illegal char
*
*WARNING:
*       This function was intended for SBCS/DBCS code pages.  UTF-8 will always return _MBC_SINGLE
*       It is recommended that apps do not try to reverse engineer how encodings work.
*
*Exceptions:
*       Returns _MBC_ILLEGAL if char is invalid.
*       Calls invalid parameter if len is bigger than string length (and errno is set to EINVAL).
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _mbsbtype_l(unsigned char const* string, size_t length, _locale_t const locale)
{
    _VALIDATE_RETURN(string != nullptr, EINVAL, _MBC_ILLEGAL);

    _LocaleUpdate locale_update(locale);

    if (locale_update.GetLocaleT()->mbcinfo->ismbcodepage == 0)
        return _MBC_SINGLE;

    int chartype = _MBC_ILLEGAL;

    do
    {
        /* If the char at the position asked for is a '\0' we return
        _MBC_ILLEGAL. But, If any char before the position asked for is
        a '\0', then we call invalid_param */

        if (length == 0 && *string == '\0')
            return _MBC_ILLEGAL;

        _VALIDATE_RETURN(*string != '\0', EINVAL, _MBC_ILLEGAL);

        chartype = _mbbtype_l(*string++, chartype, locale_update.GetLocaleT());
    }  while (length--);

    return chartype;
}

int __cdecl _mbsbtype(unsigned char const* const string, size_t const length)
{
    return _mbsbtype_l(string, length, nullptr);
}
