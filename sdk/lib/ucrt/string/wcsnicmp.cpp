/***
*wcsnicmp.cpp - compare n chars of wide-character strings, ignoring case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _wcsnicmp() - Compares at most n characters of two wchar_t
*       strings, without regard to case.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <corecrt_internal_securecrt.h>
#include <locale.h>
#include <string.h>

/***
*int _wcsnicmp(lhs, rhs, count) - compares count wchar_t of strings,
*       ignore case
*
*Purpose:
*       Compare the two strings for ordinal order.  Stops the comparison
*       when the following occurs: (1) strings differ, (2) the end of the
*       strings is reached, or (3) count characters have been compared.
*       For the purposes of the comparison, upper case characters are
*       converted to lower case (wide-characters).
*
*Entry:
*       wchar_t *lhs, *rhs - strings to compare
*       size_t count - maximum number of characters to compare
*
*Exit:
*       Returns -1 if lhs < rhs
*       Returns 0 if lhs == rhs
*       Returns 1 if lhs > rhs
*       Returns _NLSCMPERROR if something went wrong
*       This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _wcsnicmp_l (
        wchar_t const * const lhs,
        wchar_t const * const rhs,
        size_t          const count,
        _locale_t       const plocinfo
        )
{
    /* validation section */
    _VALIDATE_RETURN(lhs != nullptr, EINVAL, _NLSCMPERROR);
    _VALIDATE_RETURN(rhs != nullptr, EINVAL, _NLSCMPERROR);

    if (count == 0)
    {
        return 0;
    }

    _LocaleUpdate _loc_update(plocinfo);

    // This check is still worth doing for wide but not narrow because
    // we need to consult the UTF-16 ctype map for towlower operations.
    if (_loc_update.GetLocaleT()->locinfo->locale_name[LC_CTYPE] == nullptr)
    {
        return __ascii_wcsnicmp(lhs, rhs, count);
    }

    unsigned short const * lhs_ptr = reinterpret_cast<unsigned short const *>(lhs);
    unsigned short const * rhs_ptr = reinterpret_cast<unsigned short const *>(rhs);

    int result;
    int lhs_value;
    int rhs_value;
    size_t remaining = count;
    do
    {
        lhs_value = _towlower_internal(*lhs_ptr++, _loc_update.GetLocaleT());
        rhs_value = _towlower_internal(*rhs_ptr++, _loc_update.GetLocaleT());
        result = lhs_value - rhs_value;
    }
    while (result == 0 && lhs_value != 0 && --remaining != 0);

    return result;
}

extern "C" int __cdecl __ascii_wcsnicmp(
        wchar_t const * const lhs,
        wchar_t const * const rhs,
        size_t          const count
        )
{
    if (count == 0)
    {
        return 0;
    }

    unsigned short const * lhs_ptr = reinterpret_cast<unsigned short const *>(lhs);
    unsigned short const * rhs_ptr = reinterpret_cast<unsigned short const *>(rhs);

    int result;
    int lhs_value;
    int rhs_value;
    size_t remaining = count;
    do
    {
        lhs_value = __ascii_towlower(*lhs_ptr++);
        rhs_value = __ascii_towlower(*rhs_ptr++);
        result = lhs_value - rhs_value;
    }
    while (result == 0 && lhs_value != 0 && --remaining != 0);

    return result;
}

extern "C" int __cdecl _wcsnicmp (
        wchar_t const * const lhs,
        wchar_t const * const rhs,
        size_t          const count
        )
{
    if (!__acrt_locale_changed())
    {
        /* validation section */
        _VALIDATE_RETURN(lhs != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(rhs != nullptr, EINVAL, _NLSCMPERROR);

        return __ascii_wcsnicmp(lhs, rhs, count);
    }

    return _wcsnicmp_l(lhs, rhs, count, nullptr);
}
