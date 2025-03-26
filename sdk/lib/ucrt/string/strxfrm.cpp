/***
*strxfrm.c - Transform a string using locale information
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Transform a string using the locale information as set by
*       LC_COLLATE.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <locale.h>
#include <string.h>

/***
*size_t strxfrm() - Transform a string using locale information
*
*Purpose:
*       Transform the string pointed to by _string2 and place the
*       resulting string into the array pointed to by _string1.
*       No more than _count characters are place into the
*       resulting string (including the null).
*
*       The transformation is such that if strcmp() is applied to
*       the two transformed strings, the return value is equal to
*       the result of strcoll() applied to the two original strings.
*       Thus, the conversion must take the locale LC_COLLATE info
*       into account.
*       [ANSI]
*
*       The value of the following expression is the size of the array
*       needed to hold the transformation of the source string:
*
*           1 + strxfrm(nullptr,string,0)
*
*Entry:
*       char *_string1       = result string
*       const char *_string2 = source string
*       size_t _count        = max chars to move
*
*       [If _count is 0, _string1 is permitted to be nullptr.]
*
*Exit:
*       Length of the transformed string (not including the terminating
*       null).  If the value returned is >= _count, the contents of the
*       _string1 array are indeterminate.
*
*Exceptions:
*       Non-standard: if OM/API error, return INT_MAX.
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

extern "C" size_t __cdecl _strxfrm_l (
        char *_string1,
        const char *_string2,
        size_t _count,
        _locale_t plocinfo
        )
{
    int dstlen;
    size_t retval = INT_MAX;   /* NON-ANSI: default if OM or API error */
    _LocaleUpdate _loc_update(plocinfo);

    /* validation section */
    _VALIDATE_RETURN(_count <= INT_MAX, EINVAL, INT_MAX);
    _VALIDATE_RETURN(_string1 != nullptr || _count == 0, EINVAL, INT_MAX);
    _VALIDATE_RETURN(_string2 != nullptr, EINVAL, INT_MAX);

    /* pre-init output in case of error */
    if(_string1!=nullptr && _count>0)
    {
        *_string1='\0';
    }

    if ( (_loc_update.GetLocaleT()->locinfo->locale_name[LC_COLLATE] == nullptr) &&
            (_loc_update.GetLocaleT()->locinfo->lc_collate_cp == CP_ACP) )
    {
_BEGIN_SECURE_CRT_DEPRECATION_DISABLE
        strncpy(_string1, _string2, _count);
_END_SECURE_CRT_DEPRECATION_DISABLE
        return strlen(_string2);
    }

    /* Inquire size of dst string in BYTES */
    if ( 0 == (dstlen = __acrt_LCMapStringA(
                    _loc_update.GetLocaleT(),
                    _loc_update.GetLocaleT()->locinfo->locale_name[LC_COLLATE],
                    LCMAP_SORTKEY,
                    _string2,
                    -1,
                    nullptr,
                    0,
                    _loc_update.GetLocaleT()->locinfo->lc_collate_cp,
                    TRUE )) )
    {
        errno = EILSEQ;
        return INT_MAX;
    }

    retval = (size_t)dstlen;

    /* if not enough room, return amount needed */
    if ( retval > _count )
    {
        if (_string1 != nullptr && _count > 0)
        {
            *_string1 = '\0';
            errno = ERANGE;
        }
        /* the return value is the string length (without the terminating 0) */
        retval--;
        return retval;
    }

    /* Map src string to dst string */
    if ( 0 == __acrt_LCMapStringA(
                _loc_update.GetLocaleT(),
                _loc_update.GetLocaleT()->locinfo->locale_name[LC_COLLATE],
                LCMAP_SORTKEY,
                _string2,
                -1,
                _string1,
                (int)_count,
                _loc_update.GetLocaleT()->locinfo->lc_collate_cp,
                TRUE ) )
    {
        errno = EILSEQ;
        return INT_MAX;
    }
    /* the return value is the string length (without the terminating 0) */
    retval--;

    return retval;
}

extern "C" size_t __cdecl strxfrm (
        char *_string1,
        const char *_string2,
        size_t _count
        )
{

    return _strxfrm_l(_string1, _string2, _count, nullptr);

}
