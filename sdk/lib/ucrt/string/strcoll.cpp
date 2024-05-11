/***
*strcoll.c - Collate locale strings
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>

/***
*int strcoll() - Collate locale strings
*
*Purpose:
*       Compare two strings using the locale LC_COLLATE information.
*       [ANSI].
*
*       Non-C locale support available under _INTL switch.
*       In the C locale, strcoll() simply resolves to strcmp().
*Entry:
*       const char *s1 = pointer to the first string
*       const char *s2 = pointer to the second string
*
*Exit:
*       Less than 0    = first string less than second string
*       0              = strings are equal
*       Greater than 0 = first string greater than second string
*       Returns _NLSCMPERROR is something went wrong
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _strcoll_l (
        const char *_string1,
        const char *_string2,
        _locale_t plocinfo
        )
{
    int ret;
    _LocaleUpdate _loc_update(plocinfo);

    /* validation section */
    _VALIDATE_RETURN(_string1 != nullptr, EINVAL, _NLSCMPERROR);
    _VALIDATE_RETURN(_string2 != nullptr, EINVAL, _NLSCMPERROR);

    if ( _loc_update.GetLocaleT()->locinfo->locale_name[LC_COLLATE] == nullptr )
            return strcmp(_string1, _string2);

    if ( 0 == (ret = __acrt_CompareStringA(
                    _loc_update.GetLocaleT(), _loc_update.GetLocaleT()->locinfo->locale_name[LC_COLLATE],
                               SORT_STRINGSORT,
                               _string1,
                               -1,
                               _string2,
                               -1,
                    _loc_update.GetLocaleT()->locinfo->lc_collate_cp )) )
    {
        errno = EINVAL;
        return _NLSCMPERROR;
    }

    return (ret - 2);

}

extern "C" int __cdecl strcoll (
        const char *_string1,
        const char *_string2
        )
{
    return _strcoll_l(_string1, _string2, nullptr);
}
