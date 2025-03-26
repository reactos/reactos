/***
*strnicmp.cpp - compare n chars of strings, ignoring case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _strnicmp() - Compares at most n characters of two strings,
*       without regard to case.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>

/***
*int _strnicmp(lhs, rhs, count) - compares count char of strings, ignore case
*
*Purpose:
*       Compare the two strings for ordinal order.  Stops the comparison
*       when the following occurs: (1) strings differ, (2) the end of the
*       strings is reached, or (3) count characters have been compared.
*       For the purposes of the comparison, upper case characters are
*       converted to lower case.
*
*Entry:
*       char *lhs, *rhs - strings to compare
*       size_t count - maximum number of characters to compare
*
*Exit:
*       returns <0 if lhs < rhs
*       returns 0 if lhs == rhs
*       returns >0 if lhs > rhs
*       Returns _NLSCMPERROR if something went wrong
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _strnicmp_l (
        char const * const lhs,
        char const * const rhs,
        size_t       const count,
        _locale_t    const plocinfo
        )
{
    /* validation section */
    _VALIDATE_RETURN(lhs != nullptr, EINVAL, _NLSCMPERROR);
    _VALIDATE_RETURN(rhs != nullptr, EINVAL, _NLSCMPERROR);
    _VALIDATE_RETURN(count <= INT_MAX, EINVAL, _NLSCMPERROR);

    if (count == 0)
    {
        return 0;
    }

    unsigned char const * lhs_ptr = reinterpret_cast<unsigned char const *>(lhs);
    unsigned char const * rhs_ptr = reinterpret_cast<unsigned char const *>(rhs);

    _LocaleUpdate _loc_update(plocinfo);

    int result;
    int lhs_value;
    int rhs_value;
    size_t remaining = count;
    do
    {
        lhs_value = _tolower_fast_internal(*lhs_ptr++, _loc_update.GetLocaleT());
        rhs_value = _tolower_fast_internal(*rhs_ptr++, _loc_update.GetLocaleT());
        result = lhs_value - rhs_value;
    }
    while (result == 0 && lhs_value != 0 && --remaining != 0);

    return result;
}


#if !defined(_M_IX86) || defined(_M_HYBRID_X86_ARM64)

extern "C" int __cdecl __ascii_strnicmp (
        char const * const lhs,
        char const * const rhs,
        size_t       const count
        )
{
    if (count == 0)
    {
        return 0;
    }

    unsigned char const * lhs_ptr = reinterpret_cast<unsigned char const *>(lhs);
    unsigned char const * rhs_ptr = reinterpret_cast<unsigned char const *>(rhs);

    int result;
    int lhs_value;
    int rhs_value;
    size_t remaining = count;
    do
    {
        lhs_value = __ascii_tolower(*lhs_ptr++);
        rhs_value = __ascii_tolower(*rhs_ptr++);
        result = lhs_value - rhs_value;
    }
    while (result == 0 && lhs_value != 0 && --remaining != 0);

    return result;
}

#endif  /* !_M_IX86 || _M_HYBRID_X86_ARM64 */

extern "C" int __cdecl _strnicmp (
        char const * const lhs,
        char const * const rhs,
        size_t       const count
        )
{
    if (!__acrt_locale_changed())
    {
        /* validation section */
        _VALIDATE_RETURN(lhs != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(rhs != nullptr, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(count <= INT_MAX, EINVAL, _NLSCMPERROR);

        return __ascii_strnicmp(lhs, rhs, count);
    }

    return _strnicmp_l(lhs, rhs, count, nullptr);
}
