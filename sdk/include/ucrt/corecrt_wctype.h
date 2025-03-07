//
// corecrt_wctype.h
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// This file declares the wide character (wchar_t) classification functionality,
// shared by <ctype.h>, <wchar.h>, and <wctype.h>.
//
#pragma once

#include <corecrt.h>

#pragma warning(push)
#pragma warning(disable: _UCRT_DISABLED_WARNINGS)
_UCRT_DISABLE_CLANG_WARNINGS

_CRT_BEGIN_C_HEADER



#define WEOF ((wint_t)(0xFFFF))



// This declaration allows the user access to the ctype look-up
// array _ctype defined in ctype.obj by simply including ctype.h
#ifndef _CTYPE_DISABLE_MACROS

    #if defined _CRT_DISABLE_PERFCRIT_LOCKS && !defined _DLL
        #define __PCTYPE_FUNC  _pctype
    #else
        #define __PCTYPE_FUNC __pctype_func()
    #endif

    _ACRTIMP const unsigned short* __cdecl __pctype_func(void);
    _ACRTIMP const wctype_t*       __cdecl __pwctype_func(void);

    #ifdef _CRT_DECLARE_GLOBAL_VARIABLES_DIRECTLY
        extern const unsigned short* _pctype;
        extern const wctype_t*       _pwctype;
    #else
        #define _pctype  (__pctype_func())
        #define _pwctype (__pwctype_func())
    #endif
#endif

// Bit masks for the possible character types
#define _UPPER   0x01     // uppercase letter
#define _LOWER   0x02     // lowercase letter
#define _DIGIT   0x04     // digit[0-9]
#define _SPACE   0x08     // tab, carriage return, newline, vertical tab, or form feed
#define _PUNCT   0x10     // punctuation character
#define _CONTROL 0x20     // control character
#define _BLANK   0x40     // space char (tab is handled separately)
#define _HEX     0x80     // hexadecimal digit

#define _LEADBYTE 0x8000                    // multibyte leadbyte
#define _ALPHA   (0x0100 | _UPPER | _LOWER) // alphabetic character



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Wide Character Classification and Conversion Functions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
_Check_return_ _ACRTIMP int __cdecl iswalnum  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswalpha  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswascii  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswblank  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswcntrl  (_In_ wint_t _C);

_When_(_Param_(1) == 0, _Post_equal_to_(0))
_Check_return_ _ACRTIMP int __cdecl iswdigit  (_In_ wint_t _C);

_Check_return_ _ACRTIMP int __cdecl iswgraph  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswlower  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswprint  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswpunct  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswspace  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswupper  (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl iswxdigit (_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl __iswcsymf(_In_ wint_t _C);
_Check_return_ _ACRTIMP int __cdecl __iswcsym (_In_ wint_t _C);

_Check_return_ _ACRTIMP int __cdecl _iswalnum_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswalpha_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswblank_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswcntrl_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswdigit_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswgraph_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswlower_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswprint_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswpunct_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswspace_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswupper_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswxdigit_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswcsymf_l (_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int __cdecl _iswcsym_l  (_In_ wint_t _C, _In_opt_ _locale_t _Locale);


_Check_return_ _ACRTIMP wint_t __cdecl towupper(_In_ wint_t _C);
_Check_return_ _ACRTIMP wint_t __cdecl towlower(_In_ wint_t _C);
_Check_return_ _ACRTIMP int    __cdecl iswctype(_In_ wint_t _C, _In_ wctype_t _Type);

_Check_return_ _ACRTIMP wint_t __cdecl _towupper_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP wint_t __cdecl _towlower_l(_In_ wint_t _C, _In_opt_ _locale_t _Locale);
_Check_return_ _ACRTIMP int    __cdecl _iswctype_l(_In_ wint_t _C, _In_ wctype_t _Type, _In_opt_ _locale_t _Locale);


#ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
    _Check_return_ _ACRTIMP int __cdecl isleadbyte(_In_ int _C);
    _Check_return_ _ACRTIMP int __cdecl _isleadbyte_l(_In_ int _C, _In_opt_ _locale_t _Locale);

    _CRT_OBSOLETE(iswctype) _DCRTIMP int __cdecl is_wctype(_In_ wint_t _C, _In_ wctype_t _Type);
#endif



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Macro and Inline Definitions
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
#if !defined __cplusplus || defined _M_CEE_PURE || defined MRTDLL || defined _CORECRT_BUILD
    #ifndef _CTYPE_DISABLE_MACROS

        #define iswalpha(_c)  (iswctype(_c, _ALPHA))
        #define iswupper(_c)  (iswctype(_c, _UPPER))
        #define iswlower(_c)  (iswctype(_c, _LOWER))
        #define iswdigit(_c)  (iswctype(_c, _DIGIT))
        #define iswxdigit(_c) (iswctype(_c, _HEX))
        #define iswspace(_c)  (iswctype(_c, _SPACE))
        #define iswpunct(_c)  (iswctype(_c, _PUNCT))
        #define iswblank(_c)  (((_c) == '\t') ? _BLANK : iswctype(_c,_BLANK) )
        #define iswalnum(_c)  (iswctype(_c, _ALPHA | _DIGIT))
        #define iswprint(_c)  (iswctype(_c, _BLANK | _PUNCT | _ALPHA | _DIGIT))
        #define iswgraph(_c)  (iswctype(_c, _PUNCT | _ALPHA | _DIGIT))
        #define iswcntrl(_c)  (iswctype(_c, _CONTROL))
        #define iswascii(_c)  ((unsigned)(_c) < 0x80)

        #define _iswalpha_l(_c,_p)  (iswctype(_c, _ALPHA))
        #define _iswupper_l(_c,_p)  (iswctype(_c, _UPPER))
        #define _iswlower_l(_c,_p)  (iswctype(_c, _LOWER))
        #define _iswdigit_l(_c,_p)  (iswctype(_c, _DIGIT))
        #define _iswxdigit_l(_c,_p) (iswctype(_c, _HEX))
        #define _iswspace_l(_c,_p)  (iswctype(_c, _SPACE))
        #define _iswpunct_l(_c,_p)  (iswctype(_c, _PUNCT))
        #define _iswblank_l(_c,_p)  (iswctype(_c, _BLANK))
        #define _iswalnum_l(_c,_p)  (iswctype(_c, _ALPHA | _DIGIT))
        #define _iswprint_l(_c,_p)  (iswctype(_c, _BLANK | _PUNCT | _ALPHA | _DIGIT))
        #define _iswgraph_l(_c,_p)  (iswctype(_c, _PUNCT | _ALPHA | _DIGIT))
        #define _iswcntrl_l(_c,_p)  (iswctype(_c, _CONTROL))

        #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
            #define isleadbyte(_c)  (__PCTYPE_FUNC[(unsigned char)(_c)] & _LEADBYTE)
        #endif

    #endif // _CTYPE_DISABLE_MACROS
// CRT_REFACTOR TODO I've had to remove the inline function definitions because
// they break the debugger build.  These were moved here from <wctype.h> in
// C968560.  We need to figure out what is wrong.
//#else
//    #ifndef _CTYPE_DISABLE_MACROS
//        inline int __cdecl iswalpha (_In_ wint_t _C) { return iswctype(_C, _ALPHA); }
//        inline int __cdecl iswupper (_In_ wint_t _C) { return iswctype(_C, _UPPER); }
//        inline int __cdecl iswlower (_In_ wint_t _C) { return iswctype(_C, _LOWER); }
//        inline int __cdecl iswdigit (_In_ wint_t _C) { return iswctype(_C, _DIGIT); }
//        inline int __cdecl iswxdigit(_In_ wint_t _C) { return iswctype(_C, _HEX); }
//        inline int __cdecl iswspace (_In_ wint_t _C) { return iswctype(_C, _SPACE); }
//        inline int __cdecl iswpunct (_In_ wint_t _C) { return iswctype(_C, _PUNCT); }
//        inline int __cdecl iswblank (_In_ wint_t _C) { return (((_C) == '\t') ? _BLANK : iswctype(_C,_BLANK)); }
//        inline int __cdecl iswalnum (_In_ wint_t _C) { return iswctype(_C, _ALPHA | _DIGIT); }
//        inline int __cdecl iswprint (_In_ wint_t _C) { return iswctype(_C, _BLANK | _PUNCT | _ALPHA | _DIGIT); }
//        inline int __cdecl iswgraph (_In_ wint_t _C) { return iswctype(_C, _PUNCT | _ALPHA | _DIGIT); }
//        inline int __cdecl iswcntrl (_In_ wint_t _C) { return iswctype(_C, _CONTROL); }
//        inline int __cdecl iswascii (_In_ wint_t _C) { return (unsigned)(_C) < 0x80; }
//
//        inline int __cdecl _iswalpha_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _ALPHA); }
//        inline int __cdecl _iswupper_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _UPPER); }
//        inline int __cdecl _iswlower_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _LOWER); }
//        inline int __cdecl _iswdigit_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _DIGIT); }
//        inline int __cdecl _iswxdigit_l(_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _HEX); }
//        inline int __cdecl _iswspace_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _SPACE); }
//        inline int __cdecl _iswpunct_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _PUNCT); }
//        inline int __cdecl _iswblank_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _BLANK); }
//        inline int __cdecl _iswalnum_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _ALPHA | _DIGIT); }
//        inline int __cdecl _iswprint_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _BLANK | _PUNCT | _ALPHA | _DIGIT); }
//        inline int __cdecl _iswgraph_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _PUNCT | _ALPHA | _DIGIT); }
//        inline int __cdecl _iswcntrl_l (_In_ wint_t _C, _In_opt_ _locale_t) { return iswctype(_C, _CONTROL); }
//
//        #ifdef _CRT_USE_WINAPI_FAMILY_DESKTOP_APP
//        inline int __cdecl isleadbyte(_In_ int _C)
//        {
//            return __pctype_func()[(unsigned char)(_C)] & _LEADBYTE;
//        }
//        #endif
//    #endif
#endif



_CRT_END_C_HEADER
_UCRT_RESTORE_CLANG_WARNINGS
#pragma warning(pop) // _UCRT_DISABLED_WARNINGS
