/***
*memicmp.cpp - compare memory, ignore case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines _memicmp() - compare two blocks of memory for ordinal
*       order.  Case is ignored in the comparison.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>

/***
*int _memicmp(lhs, rhs, count) - compare two blocks of memory, ignore case
*
*Purpose:
*       Compares count bytes of the two blocks of memory stored at lhs
*       and rhs.  The characters are converted to lowercase before
*       comparing (not permanently), so case is ignored in the search.
*
*Entry:
*       char *lhs, *rhs - memory buffers to compare
*       size_t count - maximum length to compare
*
*Exit:
*       Returns < 0 if lhs < rhs
*       Returns 0 if lhs == rhs
*       Returns > 0 if lhs > rhs
*       Returns _NLSCMPERROR if something went wrong
*
*Uses:
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" int __cdecl _memicmp_l (
        void const * const lhs,
        void const * const rhs,
        size_t       const count,
        _locale_t    const plocinfo
        )
{
    /* validation section */
    _VALIDATE_RETURN(lhs != nullptr || count == 0, EINVAL, _NLSCMPERROR);
    _VALIDATE_RETURN(rhs != nullptr || count == 0, EINVAL, _NLSCMPERROR);

    if (count == 0)
    {
        return 0;
    }

    unsigned char const * lhs_ptr = static_cast<unsigned char const *>(lhs);
    unsigned char const * rhs_ptr = static_cast<unsigned char const *>(rhs);

    _LocaleUpdate _loc_update(plocinfo);

    int result;
    size_t remaining = count;
    do
    {
        result = _tolower_fast_internal(*lhs_ptr++, _loc_update.GetLocaleT())
            - _tolower_fast_internal(*rhs_ptr++, _loc_update.GetLocaleT());
    }
    while (result == 0 && --remaining != 0);

    return result;
}


#if !defined(_M_IX86) || defined(_M_HYBRID_X86_ARM64)

extern "C" int __cdecl __ascii_memicmp (
        void const * const lhs,
        void const * const rhs,
        size_t       const count
        )
{
    if (count == 0)
    {
        return 0;
    }

    unsigned char const * lhs_ptr = static_cast<unsigned char const *>(lhs);
    unsigned char const * rhs_ptr = static_cast<unsigned char const *>(rhs);

    int result;
    size_t remaining = count;
    do
    {
        result = __ascii_tolower(*lhs_ptr++) - __ascii_tolower(*rhs_ptr++);
    }
    while (result == 0 && --remaining != 0);

    return result;
}

#endif  /* !_M_IX86 || _M_HYBRID_X86_ARM64 */

extern "C" int __cdecl _memicmp (
        void const * const lhs,
        void const * const rhs,
        size_t       const count
        )
{
    if (!__acrt_locale_changed())
    {
        /* validation section */
        _VALIDATE_RETURN(lhs != nullptr || count == 0, EINVAL, _NLSCMPERROR);
        _VALIDATE_RETURN(rhs != nullptr || count == 0, EINVAL, _NLSCMPERROR);

        return __ascii_memicmp(lhs, rhs, count);
    }

    return _memicmp_l(lhs, rhs, count, nullptr);
}
