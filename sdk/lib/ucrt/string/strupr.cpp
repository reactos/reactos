/***
*strupr.c - routine to map lower-case characters in a string to upper-case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts all the lower case characters in a string to upper case,
*       in place.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <corecrt_internal_securecrt.h>
#include <limits.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
*char *_strupr(string) - map lower-case characters in a string to upper-case
*
*Purpose:
*       _strupr() converts lower-case characters in a null-terminated string
*       to their upper-case equivalents.  Conversion is done in place and
*       characters other than lower-case letters are not modified.
*
*       In the C locale, this function modifies only 7-bit ASCII characters
*       in the range 0x61 through 0x7A ('a' through 'z').
*
*       If the locale is not the 'C' locale, LCMapString() is used to do
*       the work.  Assumes enough space in the string to hold result.
*
*Entry:
*       char *string - string to change to upper case
*
*Exit:
*       input string address
*
*Exceptions:
*       The original string is returned unchanged on any error, and errno is set.
*
*******************************************************************************/

extern "C" char * __cdecl _strupr_l (
        char * string,
        _locale_t plocinfo
        )
{
    _strupr_s_l(string, (size_t)(-1), plocinfo);
    return (string);
}

extern "C" char * __cdecl _strupr (
        char * string
        )
{
    if (!__acrt_locale_changed())
    {
        /* validation section */
        _VALIDATE_RETURN(string != nullptr, EINVAL, nullptr);

        char *cp;       /* traverses string for C locale conversion */

        for ( cp = string ; *cp ; ++cp )
            if ( ('a' <= *cp) && (*cp <= 'z') )
                *cp -= 'a' - 'A';

        return(string);
    }
    else
    {
        _strupr_s_l(string, (size_t)(-1), nullptr);
        return (string);
    }
}

/***
*errno_t _strupr_s(string, size_t) - map lower-case characters in a string to upper-case
*
*Purpose:
*       _strupr() converts lower-case characters in a null-terminated string
*       to their upper-case equivalents.  Conversion is done in place and
*       characters other than lower-case letters are not modified.
*
*       In the C locale, this function modifies only 7-bit ASCII characters
*       in the range 0x61 through 0x7A ('a' through 'z').
*
*       If the locale is not the 'C' locale, LCMapString() is used to do
*       the work.  Assumes enough space in the string to hold result.
*
*Entry:
*       char *string - string to change to upper case
*       size_t sizeInBytes - size of the destination buffer
*
*Exit:
*       the error code
*
*Exceptions:
*       The original string is returned unchanged on any error, and errno is set.
*
*******************************************************************************/

static errno_t __cdecl _strupr_s_l_stat (
        _Inout_updates_z_(sizeInBytes)  char *      const  string,
                                        size_t      const  sizeInBytes,
                                        _locale_t   const  plocinfo
        ) throw()
{
    int dstsize;                /* size of dst string buffer (include null)  */
    size_t stringlen;

    /* validation section */
    _VALIDATE_RETURN_ERRCODE(string != nullptr, EINVAL);
    stringlen = strnlen(string, sizeInBytes);
    if (stringlen >= sizeInBytes)
    {
        _RESET_STRING(string, sizeInBytes);
        _RETURN_DEST_NOT_NULL_TERMINATED(string, sizeInBytes);
    }
    _FILL_STRING(string, sizeInBytes, stringlen + 1);

    if ( plocinfo->locinfo->locale_name[LC_CTYPE] == nullptr )
    {
        char *cp=string;       /* traverses string for C locale conversion */

        for ( ; *cp ; ++cp )
        {
            if ( ('a' <= *cp) && (*cp <= 'z') )
            {
                *cp -= 'a' - 'A';
            }
        }

        return 0;
    }   /* C locale */

    /* Inquire size of dst string */
    if ( 0 == (dstsize = __acrt_LCMapStringA(
                    plocinfo,
                    plocinfo->locinfo->locale_name[LC_CTYPE],
                    LCMAP_UPPERCASE,
                    string,
                    -1,
                    nullptr,
                    0,
                    plocinfo->locinfo->_public._locale_lc_codepage,
                    TRUE )) )
    {
        errno = EILSEQ;
        return errno;
    }

    if (sizeInBytes < (size_t)dstsize)
    {
        _RESET_STRING(string, sizeInBytes);
        _RETURN_BUFFER_TOO_SMALL(string, sizeInBytes);
    }

    /* Allocate space for dst */
    __crt_scoped_stack_ptr<char> const dst(_malloca_crt_t(char, dstsize));
    if (!dst)
    {
        errno = ENOMEM;
        return errno;
    }

    /* Map src string to dst string in alternate case */
    if (__acrt_LCMapStringA(
                plocinfo,
                plocinfo->locinfo->locale_name[LC_CTYPE],
                LCMAP_UPPERCASE,
                string,
                -1,
                dst.get(),
                dstsize,
                plocinfo->locinfo->_public._locale_lc_codepage,
                TRUE ) != 0)
    {
        /* copy dst string to return string */
        return strcpy_s(string, sizeInBytes, dst.get());
    }
    else
    {
        return errno = EILSEQ;
    }
}

extern "C" errno_t __cdecl _strupr_s_l (
        char * string,
        size_t sizeInBytes,
        _locale_t plocinfo
        )
{
    _LocaleUpdate _loc_update(plocinfo);

    return _strupr_s_l_stat(string, sizeInBytes, _loc_update.GetLocaleT());
}

extern "C" errno_t __cdecl _strupr_s (
        char * string,
        size_t sizeInBytes
        )
{
    return _strupr_s_l(string, sizeInBytes, nullptr);
}
