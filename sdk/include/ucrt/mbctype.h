//
// mbctype.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// Functions and macros for MBCS character classification and conversion.
//
#pragma once
#ifndef _INC_MBCTYPE // include guard for 3rd party interop
#define _INC_MBCTYPE

#include <corecrt.h>
#include <ctype.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



// This declaration allows the user access the _mbctype[] look-up array.
_Check_return_ _ACRTIMP unsigned char* __cdecl __p__mbctype(void);
_Check_return_ _ACRTIMP unsigned char* __cdecl __p__mbcasemap(void);

#ifdef _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
    #ifndef _CORECRT_BUILD
        extern unsigned char _mbctype[];
        extern unsigned char _mbcasemap[];
    #endif
#else
    #define _mbctype   (__p__mbctype())
    #define _mbcasemap (__p__mbcasemap())
#endif



// Bit masks for MBCS character types:
// Different encodings may have different behaviors, applications are discouraged
// from attempting to reverse engineer the mechanics of the various encodings.
#define _MS     0x01 // MBCS single-byte symbol
#define _MP     0x02 // MBCS punctuation
#define _M1     0x04 // MBCS (not UTF-8!) 1st (lead) byte
#define _M2     0x08 // MBCS (not UTF-8!) 2nd byte

// The CRT does not do proper linguistic casing, it is preferred that applications
// use appropriate NLS or Globalization functions.
#define _SBUP   0x10 // SBCS uppercase char
#define _SBLOW  0x20 // SBCS lowercase char

// Byte types
// Different encodings may have different behaviors, use of these is discouraged
#define _MBC_SINGLE    0  // Valid single byte char
#define _MBC_LEAD      1  // Lead byte
#define _MBC_TRAIL     2  // Trailing byte
#define _MBC_ILLEGAL (-1) // Illegal byte

#define _KANJI_CP   932

// _setmbcp parameter defines:
// Use of UTF-8 is encouraged
#define _MB_CP_SBCS    0
#define _MB_CP_OEM    -2
#define _MB_CP_ANSI   -3
#define _MB_CP_LOCALE -4
// CP_UTF8 - UTF-8 was not permitted in earlier CRT versions
#define _MB_CP_UTF8   65001

// Multibyte control routines:
_ACRTIMP int __cdecl _setmbcp(_In_ int _CodePage);
_ACRTIMP int __cdecl _getmbcp(void);



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Multibyte Character Classification and Conversion Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#if _CRT_FUNCTIONS_REQUIRED
    _Check_return_ _DCRTIMP int __cdecl _ismbbkalnum(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbkana  (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbkpunct(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbkprint(_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbalpha (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbpunct (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbblank (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbalnum (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbprint (_In_ unsigned int _C);
    _Check_return_ _DCRTIMP int __cdecl _ismbbgraph (_In_ unsigned int _C);

    _Check_return_ _DCRTIMP int __cdecl _ismbbkalnum_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbkana_l  (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbkpunct_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbkprint_l(_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbalpha_l (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbpunct_l (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbblank_l (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbalnum_l (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbprint_l (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbgraph_l (_In_ unsigned int _C, _In_opt_ _locale_t _Locale);

    // BEGIN _MBLEADTRAIL_DEFINED
    // Lead and trail bytes do not apply correctly to all encodings, including UTF-8.  Applications
    // are recommended to use the system codepage conversion APIs and not attempt to reverse
    // engineer the behavior of any particular encoding.  Lead and trail are always FALSE for UTF-8.
    _When_(_Ch == 0, _Post_equal_to_(0))
    _Check_return_ _DCRTIMP int __cdecl _ismbblead (_In_ unsigned int _Ch);
    _Check_return_ _DCRTIMP int __cdecl _ismbbtrail(_In_ unsigned int _Ch);

    _When_(_Ch == 0, _Post_equal_to_(0))
    _Check_return_ _DCRTIMP int __cdecl _ismbblead_l (_In_ unsigned int _Ch, _In_opt_ _locale_t _Locale);
    _Check_return_ _DCRTIMP int __cdecl _ismbbtrail_l(_In_ unsigned int _Ch, _In_opt_ _locale_t _Locale);

    _Check_return_
    _DCRTIMP int __cdecl _ismbslead(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos
        );

    _Check_return_
    _DCRTIMP int __cdecl _ismbslead_l(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos,
        _In_opt_                         _locale_t            _Locale
        );

    _Check_return_
    _ACRTIMP int __cdecl _ismbstrail(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos
        );

    _Check_return_
    _ACRTIMP int __cdecl _ismbstrail_l(
        _In_reads_z_(_Pos - _String + 1) unsigned char const* _String,
        _In_z_                           unsigned char const* _Pos,
        _In_opt_                         _locale_t            _Locale
        );
#endif // _CRT_FUNCTIONS_REQUIRED



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Unsynchronized Macro Forms of Some Classification Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#if defined _CRT_DISABLE_PERFCRIT_LOCKS && !defined _DLL
    #define _ismbbkalnum(_c) ((_mbctype+1)[(unsigned char)(_c)] & (_MS      ))
    #define _ismbbkprint(_c) ((_mbctype+1)[(unsigned char)(_c)] & (_MS | _MP))
    #define _ismbbkpunct(_c) ((_mbctype+1)[(unsigned char)(_c)] & (_MP      ))

    #define _ismbbalnum(_c)  (((_pctype)[(unsigned char)(_c)] & (_ALPHA | _DIGIT                  )) || _ismbbkalnum(_c))
    #define _ismbbalpha(_c)  (((_pctype)[(unsigned char)(_c)] & (_ALPHA                           )) || _ismbbkalnum(_c))
    #define _ismbbgraph(_c)  (((_pctype)[(unsigned char)(_c)] & (_PUNCT | _ALPHA | _DIGIT         )) || _ismbbkprint(_c))
    #define _ismbbprint(_c)  (((_pctype)[(unsigned char)(_c)] & (_BLANK | _PUNCT | _ALPHA | _DIGIT)) || _ismbbkprint(_c))
    #define _ismbbpunct(_c)  (((_pctype)[(unsigned char)(_c)] & (_PUNCT                           )) || _ismbbkpunct(_c))
    #define _ismbbblank(_c)  (((_c) == '\t') ? _BLANK : (_pctype)[(unsigned char)(_c)] & _BLANK)

    // Note that these are intended for double byte character sets (DBCS) and so UTF-8 doesn't consider either to be true for any bytes
    // (for UTF-8 we never set _M1 or _M2 in this array)
    #define _ismbblead(_c)   ((_mbctype+1)[(unsigned char)(_c)] & _M1)
    #define _ismbbtrail(_c)  ((_mbctype+1)[(unsigned char)(_c)] & _M2)

    #define _ismbbkana(_c)   ((_mbctype+1)[(unsigned char)(_c)] & (_MS | _MP))
#endif

_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_MBCTYPE
