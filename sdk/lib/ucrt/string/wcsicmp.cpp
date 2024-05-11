/***
*wcsicmp.cpp - contains case-insensitive wide string comp routine _wcsicmp
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       contains _wcsicmp()
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018 Prefast can't see that we are checking for terminal nul.

/***
*int _wcsicmp(lhs, rhs) - compare wide-character strings, ignore case
*
*Purpose:
*       _wcsicmp performs a case-insensitive wchar_t string comparision.
*       _wcsicmp is independent of locale.
*
*Entry:
*       wchar_t *lhs, *rhs - strings to compare
*
*Return:
*       Returns <0 if lhs < rhs
*       Returns 0 if lhs = rhs
*       Returns >0 if lhs > rhs
*       Returns _NLSCMPERROR if something went wrong
*       This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _wcsicmp_l (
        wchar_t const * const lhs,
        wchar_t const * const rhs,
        _locale_t       const plocinfo
        )
{
    /* validation section */
    _VALIDATE_RETURN(lhs != nullptr, EINVAL, _NLSCMPERROR);
    _VALIDATE_RETURN(rhs != nullptr, EINVAL, _NLSCMPERROR);

    _LocaleUpdate _loc_update(plocinfo);

    // This check is still worth doing for wide but not narrow because
    // we need to consult the UTF-16 ctype map for towlower operations.
    if (_loc_update.GetLocaleT()->locinfo->locale_name[LC_CTYPE] == nullptr)
    {
        return __ascii_wcsicmp(lhs, rhs);
    }

    unsigned short const * lhs_ptr = reinterpret_cast<unsigned short const *>(lhs);
    unsigned short const * rhs_ptr = reinterpret_cast<unsigned short const *>(rhs);

    int result;
    int lhs_value;
    int rhs_value;
    do
    {
        lhs_value = _towlower_internal(*lhs_ptr++, _loc_update.GetLocaleT());
        rhs_value = _towlower_internal(*rhs_ptr++, _loc_update.GetLocaleT());
        result = lhs_value - rhs_value;
    }
    while (result == 0 && lhs_value != 0);

    return result;
}

extern "C" int __cdecl __ascii_wcsicmp(
        wchar_t const * const lhs,
        wchar_t const * const rhs
        )
{
    unsigned short const * lhs_ptr = reinterpret_cast<unsigned short const *>(lhs);
    unsigned short const * rhs_ptr = reinterpret_cast<unsigned short const *>(rhs);

    int result;
    int lhs_value;
    int rhs_value;
    do
    {
        lhs_value = __ascii_towlower(*lhs_ptr++);
        rhs_value = __ascii_towlower(*rhs_ptr++);
        result = lhs_value - rhs_value;
    }
    while (result == 0 && lhs_value != 0);

    return result;
}

extern "C" int __cdecl _wcsicmp(
        wchar_t const * const lhs,
        wchar_t const * const rhs
        )
{
    if (!__acrt_locale_changed())
    {
        /* validation section */
        _VALIDATE_RETURN(lhs != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(rhs != nullptr, EINVAL, _NLSCMPERROR);

        return __ascii_wcsicmp(lhs, rhs);
    }

    return _wcsicmp_l(lhs, rhs, nullptr);
}
