//
// corecrt_internal_mbstring.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This internal header defines internal utilities for working with the multibyte
// character and string library.
//
#pragma once

#include <corecrt_internal.h>
#include <mbctype.h>
#include <mbstring.h>
#include <uchar.h>

_CRT_BEGIN_C_HEADER



 // Multibyte full-width-latin upper/lower info
#define NUM_ULINFO 6

/* internal use macros since tolower/toupper are locale-dependent */
#define _mbbisupper(_c) ((_mbctype.value()[(_c) + 1] & _SBUP) == _SBUP)
#define _mbbislower(_c) ((_mbctype.value()[(_c) + 1] & _SBLOW) == _SBLOW)

#define _mbbtolower(_c) (_mbbisupper(_c) ? _mbcasemap.value()[_c] : _c)
#define _mbbtoupper(_c) (_mbbislower(_c) ? _mbcasemap.value()[_c] : _c)

#define _ismbbtruelead_l(_lb,_ch,p)   (!(_lb) && _ismbblead_l((_ch), p))
#define _mbbisupper_l(_c, p)      ((p->mbcinfo->mbctype[(_c) + 1] & _SBUP) == _SBUP)
#define _mbbislower_l(_c, p)      ((p->mbcinfo->mbctype[(_c) + 1] & _SBLOW) == _SBLOW)
#define _mbbtolower_l(_c, p)      (_mbbisupper_l(_c, p) ? p->mbcinfo->mbcasemap[_c] : _c)
#define _mbbtoupper_l(_c, p)      (_mbbislower_l(_c, p) ? p->mbcinfo->mbcasemap[_c] : _c)

/* define full-width-latin upper/lower ranges */

#define _MBUPPERLOW1_MT(p)  p->mbcinfo->mbulinfo[0]
#define _MBUPPERHIGH1_MT(p) p->mbcinfo->mbulinfo[1]
#define _MBCASEDIFF1_MT(p)  p->mbcinfo->mbulinfo[2]

#define _MBUPPERLOW2_MT(p)  p->mbcinfo->mbulinfo[3]
#define _MBUPPERHIGH2_MT(p) p->mbcinfo->mbulinfo[4]
#define _MBCASEDIFF2_MT(p)  p->mbcinfo->mbulinfo[5]

// Kanji-specific ranges
#define _MBHIRALOW      0x829f  // Hiragana
#define _MBHIRAHIGH     0x82f1

#define _MBKATALOW      0x8340  // Katakana
#define _MBKATAHIGH     0x8396
#define _MBKATAEXCEPT   0x837f  // Exception

#define _MBKIGOULOW     0x8141  // Kanji punctuation
#define _MBKIGOUHIGH    0x81ac
#define _MBKIGOUEXCEPT  0x817f  // Exception

// Macros used in the implementation of the classification functions.
// These accesses of _locale_pctype are internal and guarded by bounds checks when used.
#define _ismbbalnum_l(_c, pt)  ((((pt)->locinfo->_public._locale_pctype)[_c] & \
                                (_ALPHA|_DIGIT)) || \
                                (((pt)->mbcinfo->mbctype+1)[_c] & _MS))
#define _ismbbalpha_l(_c, pt)  ((((pt)->locinfo->_public._locale_pctype)[_c] & \
                            (_ALPHA)) || \
                            (((pt)->mbcinfo->mbctype+1)[_c] & _MS))
#define _ismbbgraph_l(_c, pt)  ((((pt)->locinfo->_public._locale_pctype)[_c] & \
                            (_PUNCT|_ALPHA|_DIGIT)) || \
                            (((pt)->mbcinfo->mbctype+1)[_c] & (_MS|_MP)))
#define _ismbbprint_l(_c, pt)  ((((pt)->locinfo->_public._locale_pctype)[_c] & \
                            (_BLANK|_PUNCT|_ALPHA|_DIGIT)) || \
                            (((pt)->mbcinfo->mbctype + 1)[_c] & (_MS|_MP)))
#define _ismbbpunct_l(_c, pt)  ((((pt)->locinfo->_public._locale_pctype)[_c] & _PUNCT) || \
                                (((pt)->mbcinfo->mbctype+1)[_c] & _MP))
#define _ismbbblank_l(_c, pt)  (((_c) == '\t') ? _BLANK : (((pt)->locinfo->_public._locale_pctype)[_c] & _BLANK) || \
                               (((pt)->mbcinfo->mbctype+1)[_c] & _MP))
// Note that these are intended for double byte character sets (DBCS) and so UTF-8 doesn't consider either to be true for any bytes
// (for UTF-8 we never set _M1 or _M2 in this array)
#define _ismbblead_l(_c, p)   ((p->mbcinfo->mbctype + 1)[_c] & _M1)
#define _ismbbtrail_l(_c, p)  ((p->mbcinfo->mbctype + 1)[_c] & _M2)



#ifdef __cplusplus
extern "C" inline int __cdecl __dcrt_multibyte_check_type(
    unsigned int   const c,
    _locale_t      const locale,
    unsigned short const category_bits,
    bool           const expected
    )
{
    // Return false if we are not in a supported multibyte codepage:
    if (!locale->mbcinfo->ismbcodepage)
        return FALSE;

    int const code_page = locale->mbcinfo->mbcodepage;

    char const bytes[] = { static_cast<char>((c >> 8) & 0xff), static_cast<char>(c & 0xff) };

    // The 'c' "character" could be two one-byte multibyte characters, so we
    // need room in the type array to handle this.  If 'c' is two one-byte
    // multibyte characters, the second element in the type array will be
    // nonzero.
    unsigned short ctypes[2] = { };

    if (__acrt_GetStringTypeA(locale, CT_CTYPE1, bytes, _countof(bytes), ctypes, code_page, TRUE) == 0)
        return FALSE;

    // Ensure 'c' is a single multibyte character:
    if (ctypes[1] != 0)
        return FALSE;

    // Test the category:
    return static_cast<bool>((ctypes[0] & category_bits) != 0) == expected ? TRUE : FALSE;
}
#endif

_Check_return_wat_
extern "C" errno_t __cdecl _wctomb_internal(
    _Out_opt_                        int*                  _SizeConverted,
    _Out_writes_opt_z_(_SizeInBytes) char*                 _MbCh,
    _In_                             size_t                _SizeInBytes,
    _In_                             wchar_t               _WCh,
    _Inout_                         __crt_cached_ptd_host& _Ptd
    );

_Success_(return != -1)
extern "C" int __cdecl _mbtowc_internal(
    _Pre_notnull_ _Post_z_               wchar_t*               _DstCh,
    _In_reads_or_z_opt_(_SrcSizeInBytes) char const*            _SrcCh,
    _In_                                 size_t                 _SrcSizeInBytes,
    _Inout_                              __crt_cached_ptd_host& _Ptd
    );

_CRT_END_C_HEADER

namespace __crt_mbstring
{
    size_t __cdecl __c16rtomb_utf8(char* s, char16_t c16, mbstate_t* ps, __crt_cached_ptd_host& ptd);
    size_t __cdecl __c32rtomb_utf8(char* s, char32_t c32, mbstate_t* ps, __crt_cached_ptd_host& ptd);
    size_t __cdecl __mbrtoc16_utf8(char16_t* pc32, const char* s, size_t n, mbstate_t* ps, __crt_cached_ptd_host& ptd);
    size_t __cdecl __mbrtoc32_utf8(char32_t* pc32, const char* s, size_t n, mbstate_t* ps, __crt_cached_ptd_host& ptd);

    size_t __cdecl __mbrtowc_utf8(wchar_t* pwc, const char* s, size_t n, mbstate_t* ps, __crt_cached_ptd_host& ptd);
    size_t __cdecl __mbsrtowcs_utf8(wchar_t* dst, const char** src, size_t len, mbstate_t* ps, __crt_cached_ptd_host& ptd);
    size_t __cdecl __wcsrtombs_utf8(char* dst, const wchar_t** src, size_t len, mbstate_t* ps, __crt_cached_ptd_host& ptd);

    constexpr size_t INVALID = static_cast<size_t>(-1);
    constexpr size_t INCOMPLETE = static_cast<size_t>(-2);

    size_t return_illegal_sequence(mbstate_t* ps, __crt_cached_ptd_host& ptd);
    size_t reset_and_return(size_t retval, mbstate_t* ps);
}
