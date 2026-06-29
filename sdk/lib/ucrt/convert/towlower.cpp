/***
*towlower.cpp - convert wide character to lower case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Defines towlower().
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>

/***
*wint_t _towlower_l(c, ptloci) - convert wide character to lower case
*
*Purpose:
*       Multi-thread function only! Non-locking version of towlower.
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

extern "C" wint_t __cdecl _towlower_l(
        wint_t const c,
        _locale_t const plocinfo
        )
{
    if (c == WEOF)
    {
        return c;
    }

    _LocaleUpdate _loc_update(plocinfo);
    auto const locinfo = _loc_update.GetLocaleT()->locinfo;

    if (locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        // For 128 <= c < 256, the tolower map would consider the wide character as it is in UTF-8, not UTF-16.
        // Below 128, UTF-8 and UTF-16 have the same encodings, so we can use the table.
        if (c < 128)
        {
            return _towlower_fast_internal(static_cast<unsigned char>(c), _loc_update.GetLocaleT());
        }
    }
    else
    {
        // For 128 <= c < 256, the tolower map will consider the wide character as it would be in the current narrow code page,
        // which can lead to unexpected behavior. This behavior is maintained for backwards compatibility.
        if (c < 256)
        {
            return _towlower_fast_internal(static_cast<unsigned char>(c), _loc_update.GetLocaleT());
        }

        if (locinfo->locale_name[LC_CTYPE] == nullptr)
        {
            // If the locale is C, then the only characters that would be transformed are <256
            // and have been processed already.
            return c;
        }
    }

    wint_t widechar;
    if (0 == __acrt_LCMapStringW(
                locinfo->locale_name[LC_CTYPE],
                LCMAP_LOWERCASE,
                (LPCWSTR)&c,
                1,
                (LPWSTR)&widechar,
                1))
    {
        return c;
    }

    return widechar;
}

/***
*wint_t towlower(c) - convert wide character to lower case
*
*Purpose:
*       towlower() returns the lowercase equivalent of its argument
*
*Entry:
*       c - wint_t value of character to be converted
*
*Exit:
*       if c is an upper case letter, returns wint_t value of lower case
*       representation of c. otherwise, it returns c.
*
*Exceptions:
*
*******************************************************************************/

extern "C" wint_t __cdecl towlower (
        wint_t c
        )
{

    return _towlower_l(c, nullptr);
}
