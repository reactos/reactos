/***
*mbbtype.c - Return type of byte based on previous byte (MBCS)
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Return type of byte based on previous byte (MBCS)
*
*******************************************************************************/
#ifndef _MBCS
    #error This file should only be compiled with _MBCS defined
#endif

#include <corecrt_internal_mbstring.h>
#include <locale.h>

/***
*int _mbbtype(c, ctype) - Return type of byte based on previous byte (MBCS)
*
*Purpose:
*       Returns type of supplied byte.  This decision is context
*       sensitive so a control test condition is supplied.  Normally,
*       this is the type of the previous byte in the string.
*
*Entry:
*       unsigned char c = character to be checked
*       int ctype = control test condition (i.e., type of previous char)
*
*Exit:
*       _MBC_LEAD      = if 1st byte of MBCS char
*       _MBC_TRAIL     = if 2nd byte of MBCS char
*       _MBC_SINGLE    = valid single byte char
*
*       _MBC_ILLEGAL   = if illegal char
*
*WARNING:
*       These fail for UTF-8, which doesn't have lead bytes matched with single
*       trail bytes as this function expects.
*
*       Applications should not be trying to reverse engineer how any encoding works.
*
*Exceptions:
*
*******************************************************************************/

extern "C" int __cdecl _mbbtype_l(
    unsigned char const c,
    int           const ctype,
    _locale_t     const locale
    )
{
    _LocaleUpdate locale_update(locale);

    switch(ctype)
    {
    case _MBC_LEAD:
        if (_ismbbtrail_l(c, locale_update.GetLocaleT()))
            return _MBC_TRAIL;

        else
            return _MBC_ILLEGAL;

    case _MBC_TRAIL:
    case _MBC_SINGLE:
    case _MBC_ILLEGAL:
    default:
        if (_ismbblead_l(c, locale_update.GetLocaleT()))
            return _MBC_LEAD;

        else if (_ismbbprint_l(c, locale_update.GetLocaleT()))
            return _MBC_SINGLE;

        else
            return _MBC_ILLEGAL;
    }
}

extern "C" int __cdecl _mbbtype(unsigned char const c, int const ctype)
{
    return _mbbtype_l(c, ctype, nullptr);
}
