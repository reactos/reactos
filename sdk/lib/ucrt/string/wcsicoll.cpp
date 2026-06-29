/***
*wcsicoll.c - Collate wide-character locale strings without regard to case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two wchar_t strings using the locale LC_COLLATE information
*       without regard to case.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018 Prefast can't see that we are checking for terminal nul.

/***
*int _wcsicoll() - Collate wide-character locale strings without regard to case
*
*Purpose:
*       Compare two wchar_t strings using the locale LC_COLLATE information
*       without regard to case.
*       In the C locale, _wcsicmp() is used to make the comparison.
*
*Entry:
*       const wchar_t *s1 = pointer to the first string
*       const wchar_t *s2 = pointer to the second string
*
*Exit:
*       -1 = first string less than second string
*        0 = strings are equal
*        1 = first string greater than second string
*       Returns _NLSCMPERROR is something went wrong
*       This range of return values may differ from other *cmp/*coll functions.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _wcsicoll_l (
        const wchar_t *_string1,
        const wchar_t *_string2,
        _locale_t plocinfo
        )
{
    int ret;
    _LocaleUpdate _loc_update(plocinfo);

    /* validation section */
    _VALIDATE_RETURN(_string1 != nullptr, EINVAL, _NLSCMPERROR );
    _VALIDATE_RETURN(_string2 != nullptr, EINVAL, _NLSCMPERROR );

    if ( _loc_update.GetLocaleT()->locinfo->locale_name[LC_COLLATE] == nullptr )
    {
        return __ascii_wcsicmp(_string1, _string2);
    }

    if ( 0 == (ret = __acrt_CompareStringW(
                    _loc_update.GetLocaleT()->locinfo->locale_name[LC_COLLATE],
                    SORT_STRINGSORT | NORM_IGNORECASE,
                    _string1,
                    -1,
                    _string2,
                    -1)) )
    {
        errno = EINVAL;
        return _NLSCMPERROR;
    }

    return (ret - 2);
}

extern "C" int __cdecl _wcsicoll (
        const wchar_t *_string1,
        const wchar_t *_string2
        )
{
    if (!__acrt_locale_changed())
    {
        /* validation section */
        _VALIDATE_RETURN(_string1 != nullptr, EINVAL, _NLSCMPERROR );
        _VALIDATE_RETURN(_string2 != nullptr, EINVAL, _NLSCMPERROR );

        return __ascii_wcsicmp(_string1, _string2);
    }
    else
    {
        return _wcsicoll_l(_string1, _string2, nullptr);
    }

}
