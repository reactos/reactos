//
// ctype.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the narrow character (char) classification functionality.
//
#pragma once
#ifndef _INC_CTYPE // include guard for 3rd party interop
#define _INC_CTYPE

#include <corecrt.h>
#include <corecrt_wctype.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER
#if !defined __midl && !defined RC_INVOKED



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Character Classification Function Declarations
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
_Check_return_ _ACRTIMP int __cdecl _isctype(_In_ int _C, _In_ int _Type);
_Check_return_ _ACRTIMP int __cdecl _isctype_l(_In_ int _C, _In_ int _Type, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl isalpha(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _isalpha_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl isupper(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _isupper_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl islower(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _islower_l(_In_ int _C, _In_opt_ _locale_t _Locale);

_When_(_Param_(1) == 0, _Post_equal_to_(0))
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl isdigit(_In_ int _C);

_Check_return_ _ACRTIMP int __cdecl _isdigit_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl isxdigit(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _isxdigit_l(_In_ int _C, _In_opt_ _locale_t _Locale);

_When_(_Param_(1) == 0, _Post_equal_to_(0))
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl isspace(_In_ int _C);

_Check_return_ _ACRTIMP int __cdecl _isspace_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl ispunct(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _ispunct_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl isblank(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _isblank_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl isalnum(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _isalnum_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl isprint(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _isprint_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl isgraph(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _isgraph_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl iscntrl(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _iscntrl_l(_In_ int _C, _In_opt_ _locale_t _Locale);

_When_(_Param_(1) == 0, _Post_equal_to_(0))
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl toupper(_In_ int _C);

_When_(_Param_(1) == 0, _Post_equal_to_(0))
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl tolower(_In_ int _C);

_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl _tolower(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _tolower_l(_In_ int _C, _In_opt_ _locale_t _Locale);
_Check_return_ _CRT_JIT_INTRINSIC _ACRTIMP int __cdecl _toupper(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl _toupper_l(_In_ int _C, _In_opt_ _locale_t _Locale);

_Check_return_ _ACRTIMP int __cdecl __isascii(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl __toascii(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl __iscsymf(_In_ int _C);
_Check_return_ _ACRTIMP int __cdecl __iscsym(_In_ int _C);



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Character Classification Macro Definitions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
__inline int __CRTDECL __acrt_locale_get_ctype_array_value(
    _In_reads_(_Char_value + 1) unsigned short const * const _Locale_pctype_array,
    _In_range_(-1, 255)         int                    const _Char_value,
    _In_                        int                    const _Mask
    )
{
    // The C Standard specifies valid input to a ctype function ranges from -1 to 255.
    // To avoid undefined behavior, we should check this range for all accesses.
    // Note _locale_pctype array does extend to -127 to support accessing
    // _pctype directly with signed chars.

    if (_Char_value >= -1 && _Char_value <= 255)
    {
        return _Locale_pctype_array[_Char_value] & _Mask;
    }

    return 0;
}

#ifndef _CTYPE_DISABLE_MACROS

    // Maximum number of bytes in multi-byte character in the current locale
    // (also defined in stdlib.h).
    #ifndef MB_CUR_MAX
          #if defined _CRT_DISABLE_PERFCRIT_LOCKS && !defined _DLL
            #define MB_CUR_MAX __mb_cur_max
        #else
            #define MB_CUR_MAX ___mb_cur_max_func()
        #endif

        #ifdef _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
            extern int __mb_cur_max;
        #else
            #define __mb_cur_max (___mb_cur_max_func())
        #endif

        // MB_LEN_MAX = 5 in limits.h but we do not include that header here so use 5
        // directly.
        _Post_satisfies_(return > 0 && return < 5)
        _ACRTIMP int __cdecl ___mb_cur_max_func(void);
        _Post_satisfies_(return > 0 && return < 5)
        _ACRTIMP int __cdecl ___mb_cur_max_l_func(_locale_t _Locale);
    #endif

    // In the debug CRT, we make all calls through the validation function to catch
    // invalid integer inputs that yield undefined behavior.
    #ifdef _DEBUG
        _ACRTIMP int __cdecl _chvalidator(_In_ int _Ch, _In_ int _Mask);
        #define __chvalidchk(a, b) _chvalidator(a, b)
    #else

        #define __chvalidchk(a, b) (__acrt_locale_get_ctype_array_value(__PCTYPE_FUNC, (a), (b)))
    #endif



    #define __ascii_isalpha(c)   ( __chvalidchk(c, _ALPHA))
    #define __ascii_isdigit(c)   ( __chvalidchk(c, _DIGIT))

    #ifdef _CRT_DEFINE_ASCII_CTYPE_MACROS
        #define __ascii_tolower(c)   ( (((c) >= 'A') && ((c) <= 'Z')) ? ((c) - 'A' + 'a') : (c) )
        #define __ascii_toupper(c)   ( (((c) >= 'a') && ((c) <= 'z')) ? ((c) - 'a' + 'A') : (c) )
        #define __ascii_iswalpha(c)  ( ('A' <= (c) && (c) <= 'Z') || ( 'a' <= (c) && (c) <= 'z'))
        #define __ascii_iswdigit(c)  ( '0' <= (c) && (c) <= '9')
        #define __ascii_towlower(c)  ( (((c) >= L'A') && ((c) <= L'Z')) ? ((c) - L'A' + L'a') : (c) )
        #define __ascii_towupper(c)  ( (((c) >= L'a') && ((c) <= L'z')) ? ((c) - L'a' + L'A') : (c) )
    #else
        __forceinline int __CRTDECL __ascii_tolower(int const _C)
        {
            if (_C >= 'A' && _C <= 'Z')
            {
                return _C - ('A' - 'a');
            }
            return _C;
        }

        __forceinline int __CRTDECL __ascii_toupper(int const _C)
        {
            if (_C >= 'a' && _C <= 'z')
            {
                return _C - ('a' - 'A');
            }
            return _C;
        }

        __forceinline int __CRTDECL __ascii_iswalpha(int const _C)
        {
            return (_C >= 'A' && _C <= 'Z') || (_C >= 'a' && _C <= 'z');
        }

        __forceinline int __CRTDECL __ascii_iswdigit(int const _C)
        {
            return _C >= '0' && _C <= '9';
        }

        __forceinline int __CRTDECL __ascii_towlower(int const _C)
        {
            return __ascii_tolower(_C);
        }

        __forceinline int __CRTDECL __ascii_towupper(int const _C)
        {
            return __ascii_toupper(_C);
        }
    #endif



    #if defined _CRT_DISABLE_PERFCRIT_LOCKS && !defined _DLL && !defined __cplusplus
        #define isalpha(c)  (MB_CUR_MAX > 1 ? _isctype(c, _ALPHA) : __chvalidchk(c, _ALPHA))
        #define isupper(c)  (MB_CUR_MAX > 1 ? _isctype(c, _UPPER) : __chvalidchk(c, _UPPER))
        #define islower(c)  (MB_CUR_MAX > 1 ? _isctype(c, _LOWER) : __chvalidchk(c, _LOWER))
        #define isdigit(c)  (MB_CUR_MAX > 1 ? _isctype(c, _DIGIT) : __chvalidchk(c, _DIGIT))
        #define isxdigit(c) (MB_CUR_MAX > 1 ? _isctype(c, _HEX)   : __chvalidchk(c, _HEX))
        #define isspace(c)  (MB_CUR_MAX > 1 ? _isctype(c, _SPACE) : __chvalidchk(c, _SPACE))
        #define ispunct(c)  (MB_CUR_MAX > 1 ? _isctype(c, _PUNCT) : __chvalidchk(c, _PUNCT))
        #define isblank(c)  (MB_CUR_MAX > 1 ? (((c) == '\t') ? _BLANK : _isctype(c, _BLANK)) : (((c) == '\t') ? _BLANK : __chvalidchk(c, _BLANK)))
        #define isalnum(c)  (MB_CUR_MAX > 1 ? _isctype(c, _ALPHA | _DIGIT) : __chvalidchk(c, (_ALPHA | _DIGIT)))
        #define isprint(c)  (MB_CUR_MAX > 1 ? _isctype(c, _BLANK | _PUNCT | _ALPHA | _DIGIT) : __chvalidchk(c, (_BLANK | _PUNCT | _ALPHA | _DIGIT)))
        #define isgraph(c)  (MB_CUR_MAX > 1 ? _isctype(c, _PUNCT | _ALPHA | _DIGIT) : __chvalidchk(c, (_PUNCT | _ALPHA | _DIGIT)))
        #define iscntrl(c)  (MB_CUR_MAX > 1 ? _isctype(c, _CONTROL) : __chvalidchk(c, _CONTROL))
    #endif

    __inline __crt_locale_data_public* __CRTDECL __acrt_get_locale_data_prefix(void const volatile* const _LocalePointers)
    {
        _locale_t const _TypedLocalePointers = (_locale_t)_LocalePointers;
        return (__crt_locale_data_public*)_TypedLocalePointers->locinfo;
    }

    #ifdef _DEBUG
    _ACRTIMP int __cdecl _chvalidator_l(_In_opt_ _locale_t, _In_ int _Ch, _In_ int _Mask);
    #endif

    __inline int __CRTDECL _chvalidchk_l(
        _In_     int       const _C,
        _In_     int       const _Mask,
        _In_opt_ _locale_t const _Locale
        )
    {
        #ifdef _DEBUG
        return _chvalidator_l(_Locale, _C, _Mask);
        #else
        if (!_Locale)
        {
            return __chvalidchk(_C, _Mask);
        }

        return __acrt_locale_get_ctype_array_value(__acrt_get_locale_data_prefix(_Locale)->_locale_pctype, _C, _Mask);
        #endif
    }

    #define __ascii_isalpha_l(c, locale) (_chvalidchk_l(c, _ALPHA, locale))
    #define __ascii_isdigit_l(c, locale) (_chvalidchk_l(c, _DIGIT, locale))

    __inline int __CRTDECL _ischartype_l(
        _In_     int       const _C,
        _In_     int       const _Mask,
        _In_opt_ _locale_t const _Locale
        )
    {
        if (!_Locale)
        {
            return _chvalidchk_l(_C, _Mask, 0);
        }

        if (_C >= -1 && _C <= 255)
        {
            return __acrt_get_locale_data_prefix(_Locale)->_locale_pctype[_C] & _Mask;
        }

        if (__acrt_get_locale_data_prefix(_Locale)->_locale_mb_cur_max > 1)
        {
            return _isctype_l(_C, _Mask, _Locale);
        }

        return 0; // >0xFF and SBCS locale
    }

    #define _isalpha_l(c, locale)  _ischartype_l(c, _ALPHA, locale)
    #define _isupper_l(c, locale)  _ischartype_l(c, _UPPER, locale)
    #define _islower_l(c, locale)  _ischartype_l(c, _LOWER, locale)
    #define _isdigit_l(c, locale)  _ischartype_l(c, _DIGIT, locale)
    #define _isxdigit_l(c, locale) _ischartype_l(c, _HEX, locale)
    #define _isspace_l(c, locale)  _ischartype_l(c, _SPACE, locale)
    #define _ispunct_l(c, locale)  _ischartype_l(c, _PUNCT, locale)
    #define _isblank_l(c, locale)  (((c) == '\t') ? _BLANK : _ischartype_l(c, _BLANK, locale))
    #define _isalnum_l(c, locale)  _ischartype_l(c, _ALPHA | _DIGIT, locale)
    #define _isprint_l(c, locale)  _ischartype_l(c, _BLANK | _PUNCT | _ALPHA | _DIGIT, locale)
    #define _isgraph_l(c, locale)  _ischartype_l(c, _PUNCT | _ALPHA | _DIGIT, locale)
    #define _iscntrl_l(c, locale)  _ischartype_l(c, _CONTROL, locale)

    #define _tolower(c) ((c) - 'A' + 'a')
    #define _toupper(c) ((c) - 'a' + 'A')

    #define __isascii(c)   ((unsigned)(c) < 0x80)
    #define __toascii(c)   ((c) & 0x7f)


    // Microsoft C version 2.0 extended ctype macros
    #define __iscsymf(c)  (isalpha(c) || ((c) == '_'))
    #define __iscsym(c)   (isalnum(c) || ((c) == '_'))
    #define __iswcsymf(c) (iswalpha(c) || ((c) == '_'))
    #define __iswcsym(c)  (iswalnum(c) || ((c) == '_'))

    #define _iscsymf_l(c, p)  (_isalpha_l(c, p) || ((c) == '_'))
    #define _iscsym_l(c, p)   (_isalnum_l(c, p) || ((c) == '_'))
    #define _iswcsymf_l(c, p) (iswalpha(c) || ((c) == '_'))
    #define _iswcsym_l(c, p)  (iswalnum(c) || ((c) == '_'))

#endif // _CTYPE_DISABLE_MACROS


#if defined(_CRT_INTERNAL_NONSTDC_NAMES) && _CRT_INTERNAL_NONSTDC_NAMES
    #define isascii __isascii
    #define toascii __toascii
    #define iscsymf __iscsymf
    #define iscsym  __iscsym
#endif



#endif // !defined __midl && !defined RC_INVOKED
_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
#endif // _INC_CTYPE
