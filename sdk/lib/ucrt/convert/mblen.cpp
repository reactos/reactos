//
// mblen.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The mblen() and _mblen_l() functions, which return the number of bytes
// contained in a multibyte character.
//
#include <corecrt_internal_mbstring.h>
#include <corecrt_internal_ptd_propagation.h>

using namespace __crt_mbstring;

// Computes the number of bytes contained in a multibyte character.  If the string
// is null, zero is returned to indicate that we support only state-independent
// character encodings.  If the next max_count bytes of the string are not a valid
// multibyte character, -1 is returned.  Otherwise, the number of bytes that
// compose the next multibyte character are returned.
static int __cdecl _mblen_internal(
    char const*        const string,
    size_t             const max_count,
    __crt_cached_ptd_host&   ptd
    )
{
    mbstate_t internal_state{};
    if (!string || *string == '\0' || max_count == 0)
    {
        internal_state = {};
        return 0;
    }

    _locale_t const locale = ptd.get_locale();

    if (locale->locinfo->_public._locale_lc_codepage == CP_UTF8)
    {
        int result = static_cast<int>(__mbrtowc_utf8(nullptr, string, max_count, &internal_state, ptd));
        if (result < 0)
        {
            result = -1;
        }
        return result;
    }

    _ASSERTE(
        locale->locinfo->_public._locale_mb_cur_max == 1 ||
        locale->locinfo->_public._locale_mb_cur_max == 2);

    if (_isleadbyte_fast_internal(static_cast<unsigned char>(*string), locale))
    {
        _ASSERTE(locale->locinfo->_public._locale_lc_codepage != CP_UTF8 && L"UTF-8 isn't supported in this _mblen_l function yet!!!");

        // If this is a lead byte, then the codepage better be a multibyte codepage
        _ASSERTE(locale->locinfo->_public._locale_mb_cur_max > 1);

        // Multi-byte character; verify that it is valid:
        if (locale->locinfo->_public._locale_mb_cur_max <= 1)
        {
            return -1;
        }

        if (max_count > INT_MAX || static_cast<int>(max_count) < locale->locinfo->_public._locale_mb_cur_max)
        {
            return -1;
        }

        int const status = __acrt_MultiByteToWideChar(
            locale->locinfo->_public._locale_lc_codepage,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            string,
            locale->locinfo->_public._locale_mb_cur_max,
            nullptr,
            0);

        if (status == 0)
        {
            return -1;
        }

        return locale->locinfo->_public._locale_mb_cur_max;
    }
    else
    {
        // Single byte character; verify that it is valid:
        // CP_ACP is known to be valid for all values
        if (locale->locinfo->_public._locale_lc_codepage != CP_ACP)
        {
            int const status = __acrt_MultiByteToWideChar(
                locale->locinfo->_public._locale_lc_codepage,
                MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
                string,
                1,
                nullptr,
                0);

            if (status == 0)
            {
                return -1;
            }
        }

        return sizeof(char);
    }
}

extern "C" int __cdecl _mblen_l(
    char const* const string,
    size_t      const max_count,
    _locale_t   const locale
    )
{
    __crt_cached_ptd_host ptd(locale);
    return _mblen_internal(string, max_count, ptd);
}



extern "C" int __cdecl mblen(
    char const* const string,
    size_t      const max_count
    )
{
    __crt_cached_ptd_host ptd;
    return _mblen_internal(string, max_count, ptd);
}
