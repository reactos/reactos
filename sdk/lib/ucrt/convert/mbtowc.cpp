/***
*mbtowc.c - Convert multibyte char to wide char.
*
*       Copyright (c) Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Convert a multibyte character into the equivalent wide character.
*
*******************************************************************************/
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>
#include <locale.h>
#include <stdlib.h>
#include <wchar.h>

using namespace __crt_mbstring;

/***
*int mbtowc() - Convert multibyte char to wide character.
*
*Purpose:
*       Convert a multi-byte character into the equivalent wide character,
*       according to the LC_CTYPE category of the current locale.
*       [ANSI].
*
*       NOTE:  Currently, the C libraries support the "C" locale only.
*              Non-C locale support now available under _INTL switch.
*Entry:
*       wchar_t  *pwc = pointer to destination wide character
*       const char *s = pointer to multibyte character
*       size_t      n = maximum length of multibyte character to consider
*
*Exit:
*       If s = nullptr, returns 0, indicating we only use state-independent
*       character encodings.
*       If s != nullptr, returns:  0 (if *s = null char)
*                               -1 (if the next n or fewer bytes not valid mbc)
*                               number of bytes comprising converted mbc
*
*Exceptions:
*       If errors are encountered, -1 is returned and errno is set to EILSEQ.
*
*******************************************************************************/

extern "C" int __cdecl _mbtowc_internal(
    wchar_t *              pwc,
    const char *           s,
    size_t                 n,
    __crt_cached_ptd_host& ptd
    )
{
    static mbstate_t internal_state{};
    if (!s || n == 0)
    {
        /* indicate do not have state-dependent encodings,
        handle zero length string */
        internal_state = {};
        return 0;
    }

    if (!*s)
    {
        /* handle NULL char */
        if (pwc)
        {
            *pwc = 0;
        }
        return 0;
    }

    const _locale_t locale = ptd.get_locale();

    if (locale->locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        int result = static_cast<int>(__mbrtowc_utf8(pwc, s, n, &internal_state, ptd));
        if (result < 0)
            result = -1;
        return result;
    }

    _ASSERTE(locale->locinfo->_public._locale_mb_cur_max == 1 ||
             locale->locinfo->_public._locale_mb_cur_max == 2);

    if (locale->locinfo->locale_name[LC_CTYPE] == nullptr)
    {
        if (pwc)
        {
            *pwc = (wchar_t) (unsigned char) *s;
        }
        return sizeof(char);
    }

    if (_isleadbyte_fast_internal((unsigned char) *s, locale))
    {
        _ASSERTE(locale->locinfo->_public._locale_lc_codepage != CP_UTF8 && L"UTF-8 isn't supported in this _mbtowc_l function yet!!!");

        /* multi-byte char */
        // If this is a lead byte, then the codepage better be a multibyte codepage
        _ASSERTE(locale->locinfo->_public._locale_mb_cur_max > 1);

        if ((locale->locinfo->_public._locale_mb_cur_max <= 1) || ((int) n < locale->locinfo->_public._locale_mb_cur_max) ||
            (__acrt_MultiByteToWideChar(locale->locinfo->_public._locale_lc_codepage,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            s,
            locale->locinfo->_public._locale_mb_cur_max,
            pwc,
            (pwc) ? 1 : 0) == 0))
        {
            /* validate high byte of mbcs char */
            if ((n < (size_t) locale->locinfo->_public._locale_mb_cur_max) || (!*(s + 1)))
            {
                ptd.get_errno().set(EILSEQ);
                return -1;
            }
        }
        return locale->locinfo->_public._locale_mb_cur_max;
    }
    else {
        /* single byte char */
        if (__acrt_MultiByteToWideChar(locale->locinfo->_public._locale_lc_codepage,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            s,
            1,
            pwc,
            (pwc) ? 1 : 0) == 0)
        {
            ptd.get_errno().set(EILSEQ);
            return -1;
        }
        return sizeof(char);
    }
}

extern "C" int __cdecl _mbtowc_l(
    wchar_t  *pwc,
    const char *s,
    size_t n,
    _locale_t plocinfo
    )
{
    __crt_cached_ptd_host ptd(plocinfo);
    return _mbtowc_internal(pwc, s, n, ptd);
}

extern "C" int __cdecl mbtowc(
    wchar_t  *pwc,
    const char *s,
    size_t n
    )
{
    __crt_cached_ptd_host ptd;
    return _mbtowc_internal(pwc, s, n, ptd);
}
