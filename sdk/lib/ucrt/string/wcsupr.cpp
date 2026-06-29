/***
*wcsupr.c - routine to map lower-case characters in a wchar_t string
*       to upper-case
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Converts all the lower case characters in a wchar_t string
*       to upper case, in place.
*
*******************************************************************************/
#include <corecrt_internal.h>
#include <ctype.h>
#include <corecrt_internal_securecrt.h>
#include <locale.h>
#include <string.h>

#pragma warning(disable:__WARNING_POTENTIAL_BUFFER_OVERFLOW_NULLTERMINATED) // 26018

/***
*wchar_t *_wcsupr(string) - map lower-case characters in a string to upper-case
*
*Purpose:
*       wcsupr converts lower-case characters in a null-terminated wchar_t
*       string to their upper-case equivalents.  The result may be longer or
*       shorter than the original string.  Assumes enough space in string
*       to hold the result.
*
*Entry:
*       wchar_t *wsrc - wchar_t string to change to upper case
*
*Exit:
*       input string address
*
*Exceptions:
*       on an error, the original string is unaltered, and errno is set
*
*******************************************************************************/

extern "C" wchar_t * __cdecl _wcsupr_l (
        wchar_t * wsrc,
        _locale_t plocinfo
        )
{
    _wcsupr_s_l(wsrc, (size_t)(-1), plocinfo);
    return wsrc;
}

extern "C" wchar_t * __cdecl _wcsupr (
        wchar_t * wsrc
        )
{
    if (!__acrt_locale_changed())
    {
        wchar_t * p;

        /* validation section */
        _VALIDATE_RETURN(wsrc != nullptr, EINVAL, nullptr);

        for (p=wsrc; *p; ++p)
        {
                if (L'a' <= *p && *p <= L'z')
                        *p += (wchar_t)(L'A' - L'a');
        }

        return wsrc;
    }
    else
    {
        _wcsupr_s_l(wsrc, (size_t)(-1), nullptr);
        return wsrc;
    }
}

/***
*errno_t _wcsupr_s(string, size_t) - map lower-case characters in a string to upper-case
*
*Purpose:
*       wcsupr converts lower-case characters in a null-terminated wchar_t
*       string to their upper-case equivalents.  The result may be longer or
*       shorter than the original string.
*
*Entry:
*       wchar_t *wsrc - wchar_t string to change to upper case
*       size_t sizeInWords - size of the destination buffer
*
*Exit:
*       the error code
*
*Exceptions:
*       on an error, the original string is unaltered, and errno is set
*
*******************************************************************************/

static errno_t __cdecl _wcsupr_s_l_stat (
        _Inout_updates_z_(sizeInWords)  wchar_t *   const  wsrc,
                                        size_t      const  sizeInWords,
                                        _locale_t   const  plocinfo
        )
{

    wchar_t *p;             /* traverses string for C locale conversion */
    int dstsize;            /* size in wide chars of wdst string buffer (include null) */

    /* validation section */
    _VALIDATE_RETURN_ERRCODE(wsrc != nullptr, EINVAL);
    size_t const stringlen = wcsnlen(wsrc, sizeInWords);
    if (stringlen >= sizeInWords)
    {
        _RESET_STRING(wsrc, sizeInWords);
        _RETURN_DEST_NOT_NULL_TERMINATED(wsrc, sizeInWords);
    }
    _FILL_STRING(wsrc, sizeInWords, stringlen + 1);

    if ( plocinfo->locinfo->locale_name[LC_CTYPE] == nullptr )
    {
        for ( p = wsrc ; *p ; p++ )
        {
            if ( (*p >= (wchar_t)L'a') && (*p <= (wchar_t)L'z') )
                *p = *p - (L'a' - L'A');
        }
        return 0;
    }   /* C locale */


    /* Inquire size of wdst string */
    if ( (dstsize = __acrt_LCMapStringW(
                    plocinfo->locinfo->locale_name[LC_CTYPE],
                    LCMAP_UPPERCASE,
                    wsrc,
                    -1,
                    nullptr,
                    0 )) == 0 )
    {
        errno = EILSEQ;
        return errno;
    }

    if (sizeInWords < (size_t)dstsize)
    {
        _RESET_STRING(wsrc, sizeInWords);
        _RETURN_BUFFER_TOO_SMALL(wsrc, sizeInWords);
    }

    /* Allocate space for wdst */
    __crt_scoped_stack_ptr<wchar_t> const wdst(_malloca_crt_t(wchar_t, dstsize));
    if (wdst.get() == nullptr)
    {
        errno = ENOMEM;
        return errno;
    }

    /* Map wrc string to wide-character wdst string in alternate case */
    if (__acrt_LCMapStringW(
                plocinfo->locinfo->locale_name[LC_CTYPE],
                LCMAP_UPPERCASE,
                wsrc,
                -1,
                wdst.get(),
                dstsize ) != 0)
    {
        /* Copy wdst string to user string */
        return wcscpy_s(wsrc, sizeInWords, wdst.get());
    }
    else
    {
        return errno = EILSEQ;
    }
}

extern "C" errno_t __cdecl _wcsupr_s_l (
        wchar_t * wsrc,
        size_t sizeInWords,
        _locale_t plocinfo
        )
{
    _LocaleUpdate _loc_update(plocinfo);

    return _wcsupr_s_l_stat(wsrc, sizeInWords, _loc_update.GetLocaleT());
}


extern "C" errno_t __cdecl _wcsupr_s (
        wchar_t * wsrc,
        size_t sizeInWords
        )
{
    return _wcsupr_s_l(wsrc, sizeInWords, nullptr);
}
