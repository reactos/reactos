/***
*strlwr.c - routine to map upper-case characters in a string to lower-case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts all the upper case characters in a string to lower case,
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
*char *_strlwr(string) - map upper-case characters in a string to lower-case
*
*Purpose:
*       _strlwr() converts upper-case characters in a null-terminated string
*       to their lower-case equivalents.  Conversion is done in place and
*       characters other than upper-case letters are not modified.
*
*       In the C locale, this function modifies only 7-bit ASCII characters
*       in the range 0x41 through 0x5A ('A' through 'Z').
*
*       If the locale is not the 'C' locale, LCMapString() is used to do
*       the work.  Assumes enough space in the string to hold result.
*
*Entry:
*       char *string - string to change to lower case
*
*Exit:
*       input string address
*
*Exceptions:
*       The original string is returned unchanged on any error, and errno is set.
*
*******************************************************************************/

extern "C" char * __cdecl _strlwr_l (
        char * string,
        _locale_t plocinfo
        )
{
    _strlwr_s_l(string, (size_t)(-1), plocinfo);
    return string;
}

extern "C" char * __cdecl _strlwr (
        char * string
        )
{
    if (!__acrt_locale_changed())
    {
        char * cp;

        /* validation section */
        _VALIDATE_RETURN(string != nullptr, EINVAL, nullptr);

        for (cp=string; *cp; ++cp)
        {
            if ('A' <= *cp && *cp <= 'Z')
                *cp += 'a' - 'A';
        }

        return(string);
    }
    else
    {
        _strlwr_s_l(string, (size_t)(-1), nullptr);
        return string;
    }
}

/***
*errno_t _strlwr_s(string, size_t) - map upper-case characters in a string to lower-case
*
*Purpose:
*       _strlwr_s() converts upper-case characters in a null-terminated string
*       to their lower-case equivalents.  Conversion is done in place and
*       characters other than upper-case letters are not modified.
*
*       In the C locale, this function modifies only 7-bit ASCII characters
*       in the range 0x41 through 0x5A ('A' through 'Z').
*
*       If the locale is not the 'C' locale, LCMapString() is used to do
*       the work.  Assumes enough space in the string to hold result.
*
*Entry:
*       char *string - string to change to lower case
*       size_t sizeInBytes - size of the destination buffer
*
*Exit:
*       the error code
*
*Exceptions:
*       The original string is returned unchanged on any error, and errno is set.
*
*******************************************************************************/

static
errno_t __cdecl _strlwr_s_l_stat (
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

    if ( plocinfo->locinfo->locale_name[LC_CTYPE] == nullptr ) {
        char *cp;       /* traverses string for C locale conversion */

        for ( cp = string ; *cp ; ++cp )
            if ( ('A' <= *cp) && (*cp <= 'Z') )
                *cp -= 'A' - 'a';

        return 0;
    }   /* C locale */

    /* Inquire size of dst string */
    if ( 0 == (dstsize = __acrt_LCMapStringA(
                    plocinfo,
                    plocinfo->locinfo->locale_name[LC_CTYPE],
                    LCMAP_LOWERCASE,
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
    __crt_scoped_stack_ptr<char> dst(_malloca_crt_t(char, dstsize));
    if (!dst)
    {
        errno = ENOMEM;
        return errno;
    }

    /* Map src string to dst string in alternate case */
    if (__acrt_LCMapStringA(
                plocinfo,
                plocinfo->locinfo->locale_name[LC_CTYPE],
                LCMAP_LOWERCASE,
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

extern "C" errno_t __cdecl _strlwr_s_l (
        char * string,
        size_t sizeInBytes,
        _locale_t plocinfo
        )
{

    _LocaleUpdate _loc_update(plocinfo);

    return _strlwr_s_l_stat(string, sizeInBytes, _loc_update.GetLocaleT());
}

extern "C" errno_t __cdecl _strlwr_s(
        char * string,
        size_t sizeInBytes
        )
{
    return _strlwr_s_l(string, sizeInBytes, nullptr);
}
