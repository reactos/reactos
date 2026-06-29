/***
*mbrtowc.c - Convert multibyte char to wide char.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert a multibyte character into the equivalent wide character.
*
*******************************************************************************/
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <corecrt_internal_securecrt.h>
#include <limits.h>
#include <locale.h>
#include <stdio.h>
#include <uchar.h>
#include <wchar.h>

using namespace __crt_mbstring;

/***
*errno_t _mbrtowc_internal() - Helper function to convert multibyte char to wide character.
*
*Purpose:
*       Convert a multi-byte character into the equivalent wide character,
*       according to the specified LC_CTYPE category, or the current locale.
*       [ANSI].
*
*       NOTE:  Currently, the C libraries support the "C" locale only.
*              Non-C locale support now available under _INTL switch.
*Entry:
*       wchar_t *dst       = pointer to (single) destination wide character
*       const char *s      = pointer to multibyte character
*       size_t n           = maximum length of multibyte character to consider
*       mbstate_t *pmbst   = pointer to state (must be not nullptr)
*       _locale_t plocinfo = locale info
*
*Exit:
*       returns, in *pRetValue:
*       If s = nullptr, 0, indicating we only use state-independent
*       character encodings.
*       If s != nullptr:  0 (if *s = null char)
*                      -1 (if the next n or fewer bytes not valid mbc)
*                      number of bytes comprising converted mbc
*
*Exceptions:
*
*******************************************************************************/

_Success_(return != 0)
_Post_satisfies_(*pRetValue <= _String_length_(s))
static errno_t __cdecl _mbrtowc_internal(
    _Inout_ _Out_range_(<=, 1)              int *                  pRetValue,
    _Pre_maybenull_ _Out_writes_opt_z_(1)   wchar_t *              dst,
    _In_opt_z_                              const char *           s,
    _In_                                    size_t                 n,
    _Inout_                                 mbstate_t *            pmbst,
    _Inout_                                 __crt_cached_ptd_host& ptd
    ) throw()
{
    _ASSERTE(pmbst != nullptr);
    _ASSIGN_IF_NOT_NULL(dst, 0);

    if (!s || n == 0)
    {
        /* indicate do not have state-dependent encodings,
        handle zero length string */
        _ASSIGN_IF_NOT_NULL(pRetValue, 0);
        return 0;
    }

    if (!*s)
    {
        /* handle nullptr char */
        _ASSIGN_IF_NOT_NULL(pRetValue, 0);
        return 0;
    }

    const _locale_t locale = ptd.get_locale();

    if (locale->locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        const size_t retval = __mbrtowc_utf8(dst, s, n, pmbst, ptd);
        _ASSIGN_IF_NOT_NULL(pRetValue, static_cast<int>(retval));
        return ptd.get_errno().value_or(0);
    }

    const int locale_mb_cur_max = locale->locinfo->_public._locale_mb_cur_max;
    _ASSERTE(locale_mb_cur_max == 1 || locale_mb_cur_max == 2);

    if (locale->locinfo->locale_name[LC_CTYPE] == nullptr)
    {
        _ASSIGN_IF_NOT_NULL(dst, (wchar_t) (unsigned char) *s);
        _ASSIGN_IF_NOT_NULL(pRetValue, 1);
        return 0;
    }

    if (pmbst->_Wchar != 0)
    {
        /* complete two-byte multibyte character */
        ((char *) pmbst)[1] = *s;
        if (locale_mb_cur_max <= 1 ||
            (__acrt_MultiByteToWideChar(
            locale->locinfo->_public._locale_lc_codepage,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            (char *) pmbst,
            2,
            dst,
            (dst != nullptr ? 1 : 0)) == 0))
        {
            /* translation failed */
            pmbst->_Wchar = 0;
            _ASSIGN_IF_NOT_NULL(dst, 0);
            _ASSIGN_IF_NOT_NULL(pRetValue, -1);
            return ptd.get_errno().set(EILSEQ);
        }
        pmbst->_Wchar = 0;
        _ASSIGN_IF_NOT_NULL(pRetValue, locale_mb_cur_max);
        return 0;
    }
    else if (_isleadbyte_fast_internal((unsigned char) *s, locale))
    {
        /* multi-byte char */
        if (n < (size_t) locale_mb_cur_max)
        {
            /* save partial multibyte character */
            ((char *) pmbst)[0] = *s;
            _ASSIGN_IF_NOT_NULL(pRetValue, -2);
            return 0;
        }
        else if (locale_mb_cur_max <= 1 ||
            (__acrt_MultiByteToWideChar(locale->locinfo->_public._locale_lc_codepage,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            s,
            static_cast<int>(__min(strlen(s), INT_MAX)),
            dst,
            (dst != nullptr ? 1 : 0)) == 0))
        {
            /* validate high byte of mbcs char */
            if (!*(s + 1))
            {
                pmbst->_Wchar = 0;
                _ASSIGN_IF_NOT_NULL(dst, 0);
                _ASSIGN_IF_NOT_NULL(pRetValue, -1);
                return ptd.get_errno().set(EILSEQ);
            }
        }
        _ASSIGN_IF_NOT_NULL(pRetValue, locale_mb_cur_max);
        return 0;
    }
    else {
        /* single byte char */
        if (__acrt_MultiByteToWideChar(
            locale->locinfo->_public._locale_lc_codepage,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            s,
            1,
            dst,
            (dst != nullptr ? 1 : 0)) == 0)
        {
            _ASSIGN_IF_NOT_NULL(dst, 0);
            _ASSIGN_IF_NOT_NULL(pRetValue, -1);
            return ptd.get_errno().set(EILSEQ);
        }

        _ASSIGN_IF_NOT_NULL(pRetValue, sizeof(char) );
        return 0;
    }
}


/***
*wint_t btowc(c) - translate single byte to wide char
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

extern "C" wint_t __cdecl btowc(
    int c
    )
{
    if (c == EOF)
    {
        return WEOF;
    }
    else
    {
        /* convert as one-byte string */
        char ch = (char) c;
        mbstate_t mbst = {};
        wchar_t wc = 0;
        int retValue = -1;

        __crt_cached_ptd_host ptd;
        _mbrtowc_internal(&retValue, &wc, &ch, 1, &mbst, ptd);
        return (retValue < 0 ? WEOF : wc);
    }
}


/***
*size_t mbrlen(s, n, pst) - determine next multibyte code, restartably
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

extern "C" size_t __cdecl mbrlen(
    const char *s,
    size_t n,
    mbstate_t *pst
    )
{
    static mbstate_t mbst = {};
    int retValue = -1;

    __crt_cached_ptd_host ptd;
    _mbrtowc_internal(&retValue, nullptr, s, n, (pst != nullptr ? pst : &mbst), ptd);
    return retValue;
}


/***
*size_t mbrtowc(pwc, s, n, pst) - translate multibyte to wchar_t, restartably
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

extern "C" size_t __cdecl mbrtowc(
    wchar_t *dst,
    const char *s,
    size_t n,
    mbstate_t *pst
    )
{
    static mbstate_t mbst = {};
    int retValue = -1;

    __crt_cached_ptd_host ptd;

    if (s != nullptr)
    {
        _mbrtowc_internal(&retValue, dst, s, n, (pst != nullptr ? pst : &mbst), ptd);
    }
    else
    {
        _mbrtowc_internal(&retValue, nullptr, "", 1, (pst != nullptr ? pst : &mbst), ptd);
    }
    return retValue;
}


/***
*size_t mbsrtowcs(wcs, ps, n, pst) - translate multibyte string to wide,
*       restartably
*
*Purpose:
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

/* Helper function shared by the secure and non-secure versions. */

_Success_(return == 0)
static size_t __cdecl _mbsrtowcs_helper(
    _Out_writes_opt_z_(n)               wchar_t *              wcs,
    _Deref_pre_opt_z_                   const char **          ps,
    _In_                                size_t                 n,
    _Inout_                             mbstate_t *            pst,
    _Inout_                             __crt_cached_ptd_host& ptd
    ) throw()
{
    /* validation section */
    _UCRT_VALIDATE_RETURN(ptd, ps != nullptr, EINVAL, (size_t) - 1);

    static mbstate_t mbst = {};
    const char *s = *ps;
    int i = 0;
    size_t nwc = 0;

    // Use the static cached state if necessary
    if (pst == nullptr)
    {
        pst = &mbst;
    }

    const _locale_t locale = ptd.get_locale();

    if (locale->locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        return __mbsrtowcs_utf8(wcs, ps, n, pst, ptd);
    }

    if (wcs == nullptr)
    {
        for (;; ++nwc, s += i)
        {
            /* translate but don't store */
            wchar_t wc;
            _mbrtowc_internal(&i, &wc, s, INT_MAX, pst, ptd);
            if (i < 0)
            {
                return (size_t) - 1;
            }
            else if (i == 0)
            {
                return nwc;
            }
        }
    }

    for (; 0 < n; ++nwc, s += i, ++wcs, --n)
    {
        /* translate and store */
        _mbrtowc_internal(&i, wcs, s, INT_MAX, pst, ptd);
        if (i < 0)
        {
            /* encountered invalid sequence */
            nwc = (size_t) - 1;
            break;
        }
        else if (i == 0)
        {
            /* encountered terminating null */
            s = 0;
            break;
        }
    }

    *ps = s;
    return nwc;
}

/***
*size_t mbsrtowcs() - Convert multibyte char string to wide char string.
*
*Purpose:
*       Convert a multi-byte char string into the equivalent wide char string,
*       according to the LC_CTYPE category of the current locale.
*       Same as mbsrtowcs_s(), but the destination may not be null terminated.
*       If there's not enough space, we return EINVAL.
*
*Entry:
*       wchar_t *pwcs = pointer to destination wide character string buffer
*       const char **s = pointer to source multibyte character string
*       size_t n = maximum number of wide characters to store (not including the terminating null character)
*       mbstate_t *pst = pointer to the conversion state
*
*Exit:
*       The nunber if wide characters written to *wcs, not including any terminating null character)
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/
extern "C" size_t __cdecl mbsrtowcs(
    wchar_t *     wcs,
    const char ** ps,
    size_t        n,
    mbstate_t *   pst
    )
{
    /* Call a non-deprecated helper to do the work. */
    __crt_cached_ptd_host ptd;
    return _mbsrtowcs_helper(wcs, ps, n, pst, ptd);
}


/***
*errno_t mbsrtowcs_s() - Convert multibyte char string to wide char string.
*
*Purpose:
*       Convert a multi-byte char string into the equivalent wide char string,
*       according to the LC_CTYPE category of the current locale.
*       Same as mbsrtowcs(), but the destination is ensured to be null terminated.
*       If there's not enough space, we return EINVAL.
*
*Entry:
*       size_t *pRetValue = Number of bytes modified including the terminating nullptr
*                           This pointer can be nullptr.
*       wchar_t *pwcs = pointer to destination wide character string buffer
*       size_t sizeInWords = size of the destination buffer
*       const char **s = pointer to source multibyte character string
*       size_t n = maximum number of wide characters to store (not including the terminating null character)
*       mbstate_t *pst = pointer to the conversion state
*
*Exit:
*       The error code.
*
*Exceptions:
*       Input parameters are validated. Refer to the validation section of the function.
*
*******************************************************************************/

static errno_t __cdecl mbsrtowcs_s_internal(
    size_t *               pRetValue,
    wchar_t *              dst,
    size_t                 sizeInWords,
    const char **          ps,
    size_t                 n,
    mbstate_t *            pmbst,
    __crt_cached_ptd_host& ptd
    )
{
    size_t retsize;

    /* validation section */
    _ASSIGN_IF_NOT_NULL(pRetValue, (size_t) - 1);
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, (dst == nullptr && sizeInWords == 0) || (dst != nullptr && sizeInWords > 0), EINVAL);
    if (dst != nullptr)
    {
        _RESET_STRING(dst, sizeInWords);
    }
    _UCRT_VALIDATE_RETURN_ERRCODE(ptd, ps != nullptr, EINVAL);

    /* Call a non-deprecated helper to do the work. */

    retsize = _mbsrtowcs_helper(dst, ps, (n > sizeInWords ? sizeInWords : n), pmbst, ptd);

    if (retsize == (size_t) - 1)
    {
        if (dst != nullptr)
        {
            _RESET_STRING(dst, sizeInWords);
        }
        return ptd.get_errno().value_or(0);
    }

    /* count the null terminator */
    retsize++;

    if (dst != nullptr)
    {
        /* return error if the string does not fit */
        if (retsize > sizeInWords)
        {
            _RESET_STRING(dst, sizeInWords);
            _UCRT_VALIDATE_RETURN_ERRCODE(ptd, sizeInWords <= retsize, ERANGE);
        }
        else
        {
            /* ensure the string is null terminated */
            dst[retsize - 1] = '\0';
        }
    }

    _ASSIGN_IF_NOT_NULL(pRetValue, retsize);

    return 0;
}

extern "C" errno_t __cdecl mbsrtowcs_s(
    size_t *      pRetValue,
    wchar_t *     dst,
    size_t        sizeInWords,
    const char ** ps,
    size_t        n,
    mbstate_t *   pmbst
    )
{
    __crt_cached_ptd_host ptd;
    return mbsrtowcs_s_internal(pRetValue, dst, sizeInWords, ps, n, pmbst, ptd);
}

size_t __cdecl __crt_mbstring::__mbrtowc_utf8(wchar_t* pwc, const char* s, size_t n, mbstate_t* ps, __crt_cached_ptd_host& ptd)
{
    static_assert(sizeof(wchar_t) == 2, "wchar_t is assumed to be 16 bits");
    char32_t c32;
    const size_t retval = __mbrtoc32_utf8(&c32, s, n, ps, ptd);
    // If we succesfully consumed a character, write the result after a quick range check
    if (retval <= 4)
    {
        if (c32 > 0xffff)
        {
            // A 4-byte UTF-8 character won't fit into a single UTF-16 wchar
            // So return the "replacement char"
            c32 = 0xfffd;
        }
        _ASSIGN_IF_NOT_NULL(pwc, static_cast<wchar_t>(c32));
    }
    return retval;
}

size_t __cdecl __crt_mbstring::__mbsrtowcs_utf8(wchar_t* dst, const char** src, size_t len, mbstate_t* ps, __crt_cached_ptd_host& ptd)
{
    const char* current_src = *src;

    auto compute_available = [](const char* s) -> size_t
    {
        // We shouldn't just blindly request to read 4 bytes, because there might not be 4 bytes left to read.
        if (s[0] == '\0')
        {
            return 1;
        }
        else if (s[1] == '\0')
        {
            return 2;
        }
        else if (s[2] == '\0')
        {
            return 3;
        }
        return 4;
    };

    if (dst != nullptr)
    {
        wchar_t* current_dest = dst;
        for (; len > 0; --len)
        {
            const size_t avail = compute_available(current_src);
            char32_t c32;
            const size_t retval = __mbrtoc32_utf8(&c32, current_src, avail, ps, ptd);
            if (retval == __crt_mbstring::INVALID)
            {
                // Set src to the beginning of the invalid char
                *src = current_src;
                ptd.get_errno().set(EILSEQ);
                return retval;
            }
            else if (retval == 0)
            {
                current_src = nullptr;
                *current_dest = L'\0';
                break;
            }
            else if (c32 > 0xffff)
            {
                // This is going to take two output wchars. Make sure we have enough room for this output.
                if (len > 1)
                {
                    --len;
                    c32 -= 0x10000;
                    const char16_t high_surrogate = static_cast<char16_t>((c32 >> 10) | 0xd800);
                    const char16_t low_surrogate = static_cast<char16_t>((c32 & 0x03ff) | 0xdc00);
                    *current_dest++ = high_surrogate;
                    *current_dest++ = low_surrogate;
                }
                else
                {
                    break;
                }
            }
            else
            {
                *current_dest++ = static_cast<wchar_t>(c32);
            }
            current_src += retval;
        }
        *src = current_src;
        return current_dest - dst;
    }
    else
    {
        size_t total_count = 0;
        for (;; ++total_count)
        {
            const size_t avail = compute_available(current_src);

            const size_t retval = __mbrtoc32_utf8(nullptr, current_src, avail, ps, ptd);
            if (retval == __crt_mbstring::INVALID)
            {
                ptd.get_errno().set(EILSEQ);
                return retval;
            }
            else if (retval == 0)
            {
                break;
            }
            else if (retval == 4)
            {
                // SMP characters take two UTF-16 wide chars
                ++total_count;
            }
            else
            {
                // This should be impossible. Means we encountered a multibyte char
                // that extended past the null terminator, or is more than 4 bytes long
                _ASSERTE(retval != __crt_mbstring::INCOMPLETE);
            }
            current_src += retval;
        }
        return total_count;
    }
}
