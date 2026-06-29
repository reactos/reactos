/***
*mbstowcs.c - Convert multibyte char string to wide char string.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert a multibyte char string into the equivalent wide char string.
*
*******************************************************************************/
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <corecrt_internal_securecrt.h>
#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdlib.h>

using namespace __crt_mbstring;

/***
*size_t mbstowcs() - Convert multibyte char string to wide char string.
*
*Purpose:
*       Convert a multi-byte char string into the equivalent wide char string,
*       according to the LC_CTYPE category of the current locale.
*       [ANSI].
*
*Entry:
*       wchar_t *pwcs = pointer to destination wide character string buffer
*       const char *s = pointer to source multibyte character string
*       size_t      n = maximum number of wide characters to store
*
*Exit:
*       If pwcs != nullptr returns the number of words modified (<=n): note that
*       if the return value == n, then no destination string is not 0 terminated.
*       If pwcs == nullptr returns the length (not size) needed for the destination buffer.
*
*Exceptions:
*       Returns (size_t)-1 if s is nullptr or invalid mbcs character encountered
*       and errno is set to EILSEQ.
*
*******************************************************************************/

/* Helper shared by secure and non-secure functions */

static size_t __cdecl _mbstowcs_l_helper(
    _Out_writes_opt_z_(n)               wchar_t *              pwcs,
    _In_reads_or_z_(n) _Pre_z_          const char *           s,
    _In_                                size_t                 n,
    _In_opt_                            __crt_cached_ptd_host& ptd
    ) throw()
{
    size_t count = 0;

    if (pwcs && n == 0)
    {
        /* dest string exists, but 0 bytes converted */
        return (size_t) 0;
    }

    if (pwcs && n > 0)
    {
        *pwcs = '\0';
    }

    /* validation section */
    _UCRT_VALIDATE_RETURN(ptd, s != nullptr, EINVAL, (size_t) - 1);

    _locale_t const locale = ptd.get_locale();

    if (locale->locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        mbstate_t state{};
        return __mbsrtowcs_utf8(pwcs, &s, n, &state, ptd);
    }

    /* if destination string exists, fill it in */
    if (pwcs)
    {
        if (locale->locinfo->locale_name[LC_CTYPE] == nullptr)
        {
            /* C locale: easy and fast */
            while (count < n)
            {
                *pwcs = (wchar_t) ((unsigned char) s[count]);
                if (!s[count])
                {
                    return count;
                }
                count++;
                pwcs++;
            }
            return count;

        }
        else {
            int bytecnt, charcnt;
            unsigned char *p;

            /* Assume that the buffer is large enough */
            if ((count = __acrt_MultiByteToWideChar(locale->locinfo->_public._locale_lc_codepage,
                MB_PRECOMPOSED |
                MB_ERR_INVALID_CHARS,
                s,
                -1,
                pwcs,
                (int) n)) != 0)
            {
                return count - 1; /* don't count NUL */
            }

            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                ptd.get_errno().set(EILSEQ);
                *pwcs = '\0';
                return (size_t) - 1;
            }

            /* User-supplied buffer not large enough. */

            /* How many bytes are in n characters of the string? */
            charcnt = (int) n;
            for (p = (unsigned char *) s; (charcnt-- && *p); p++)
            {
                if (_isleadbyte_fast_internal(*p, locale))
                {
                    if (p[1] == '\0')
                    {
                        /*  this is a leadbyte followed by EOS -- a dud MBCS string
                        We choose not to assert here because this
                        function is defined to deal with dud strings on
                        input and return a known value
                        */
                        ptd.get_errno().set(EILSEQ);
                        *pwcs = '\0';
                        return (size_t) - 1;
                    }
                    else
                    {
                        p++;
                    }
                }
            }
            bytecnt = ((int) ((char *) p - (char *) s));

            if ((count = __acrt_MultiByteToWideChar(locale->locinfo->_public._locale_lc_codepage,
                MB_PRECOMPOSED,
                s,
                bytecnt,
                pwcs,
                (int) n)) == 0)
            {
                ptd.get_errno().set(EILSEQ);
                *pwcs = '\0';
                return (size_t) - 1;
            }

            return count; /* no NUL in string */
        }
    }
    else {
        /* pwcs == nullptr, get size only, s must be NUL-terminated */
        if (locale->locinfo->locale_name[LC_CTYPE] == nullptr)
        {
            return strlen(s);
        }
        else if ((count = __acrt_MultiByteToWideChar(locale->locinfo->_public._locale_lc_codepage,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            s,
            -1,
            nullptr,
            0)) == 0)
        {
                ptd.get_errno().set(EILSEQ);
                return (size_t) - 1;
        }
        else
        {
            return count - 1;
        }
    }

}

extern "C" size_t __cdecl _mbstowcs_l(
    wchar_t  *pwcs,
    const char *s,
    size_t n,
    _locale_t plocinfo
    )
{
    /* Call a non-deprecated helper to do the work. */
    __crt_cached_ptd_host ptd(plocinfo);
    return _mbstowcs_l_helper(pwcs, s, n, ptd);
}

extern "C" size_t __cdecl mbstowcs(
    wchar_t  *pwcs,
    const char *s,
    size_t n
    )
{
    __crt_cached_ptd_host ptd;
    return _mbstowcs_l_helper(pwcs, s, n, ptd);
}

/***
*errno_t mbstowcs_s() - Convert multibyte char string to wide char string.
*
*Purpose:
*       Convert a multi-byte char string into the equivalent wide char string,
*       according to the LC_CTYPE category of the current locale.
*       Same as mbstowcs(), but the destination is ensured to be null terminated.
*       If there's not enough space, we return EINVAL.
*
*Entry:
*       size_t *pConvertedChars = Number of bytes modified including the terminating nullptr
*                                 This pointer can be nullptr.
*       wchar_t *pwcs = pointer to destination wide character string buffer
*       size_t sizeInWords = size of the destination buffer
*       const char *s = pointer to source multibyte character string
*       size_t n = maximum number of wide characters to store (not including the terminating nullptr)
*
*Exit:
*       The error code.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

static errno_t __cdecl _mbstowcs_internal(
    size_t *               pConvertedChars,
    wchar_t *              pwcs,
    size_t                 sizeInWords,
    const char *           s,
    size_t                 n,
    __crt_cached_ptd_host& ptd
    )
{
    size_t retsize;
    errno_t retvalue = 0;

    /* validation section */
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, (pwcs == nullptr && sizeInWords == 0) || (pwcs != nullptr && sizeInWords > 0), EINVAL);

    if (pwcs != nullptr)
    {
        _RESET_STRING(pwcs, sizeInWords);
    }

    if (pConvertedChars != nullptr)
    {
        *pConvertedChars = 0;
    }

    size_t bufferSize = n > sizeInWords ? sizeInWords : n;
    /* n must fit into an int for MultiByteToWideChar */
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, bufferSize <= INT_MAX, EINVAL);

    /* Call a non-deprecated helper to do the work. */

    retsize = _mbstowcs_l_helper(pwcs, s, bufferSize, ptd);

    if (retsize == (size_t) - 1)
    {
        if (pwcs != nullptr)
        {
            _RESET_STRING(pwcs, sizeInWords);
        }
        return ptd.get_errno().value_or(0);
    }

    /* count the null terminator */
    retsize++;

    if (pwcs != nullptr)
    {
        /* return error if the string does not fit, unless n == _TRUNCATE */
        if (retsize > sizeInWords)
        {
            if (n != _TRUNCATE)
            {
                _RESET_STRING(pwcs, sizeInWords);
                _UCRT_VALIDATE_RETURN_ERRCODE(ptd, retsize <= sizeInWords, ERANGE);
            }
            retsize = sizeInWords;
            retvalue = STRUNCATE;
        }

        /* ensure the string is null terminated */
        pwcs[retsize - 1] = '\0';
    }

    if (pConvertedChars != nullptr)
    {
        *pConvertedChars = retsize;
    }

    return retvalue;
}

extern "C" errno_t __cdecl _mbstowcs_s_l(
    size_t *     pConvertedChars,
    wchar_t *    pwcs,
    size_t       sizeInWords,
    const char * s,
    size_t       n,
    _locale_t    plocinfo
    )
{
    __crt_cached_ptd_host ptd(plocinfo);
    return _mbstowcs_internal(pConvertedChars, pwcs, sizeInWords, s, n, ptd);
}

extern "C" errno_t __cdecl mbstowcs_s(
    size_t *pConvertedChars,
    wchar_t  *pwcs,
    size_t sizeInWords,
    const char *s,
    size_t n
    )
{
    __crt_cached_ptd_host ptd;
    return _mbstowcs_internal(pConvertedChars, pwcs, sizeInWords, s, n, ptd);
}
