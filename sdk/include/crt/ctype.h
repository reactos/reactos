/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_CTYPE
#define _INC_CTYPE

#include <corecrt.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif

#ifndef _CRT_CTYPEDATA_DEFINED
# define _CRT_CTYPEDATA_DEFINED
# ifndef _CTYPE_DISABLE_MACROS
#  ifndef __PCTYPE_FUNC
#   ifdef _DLL
#    define __PCTYPE_FUNC __pctype_func()
#   else
#    define __PCTYPE_FUNC _pctype
#   endif
#  endif /* !__PCTYPE_FUNC */
  _CRTIMP const unsigned short * __cdecl __pctype_func(void);
#  ifndef _M_CEE_PURE
  _CRTDATA(extern const unsigned short *_pctype);
#  else
#   define _pctype (__pctype_func())
#  endif /* !_M_CEE_PURE */
# endif /* !_CTYPE_DISABLE_MACROS */
#endif /* !_CRT_CTYPEDATA_DEFINED */

#ifndef _CRT_WCTYPEDATA_DEFINED
#define _CRT_WCTYPEDATA_DEFINED
# ifndef _CTYPE_DISABLE_MACROS
  _CRTDATA(extern const unsigned short _wctype[]);
  _CRTIMP const wctype_t * __cdecl __pwctype_func(void);
#  ifndef _M_CEE_PURE
  _CRTDATA(extern const wctype_t *_pwctype);
#  else
#   define _pwctype (__pwctype_func())
#  endif /* !_M_CEE_PURE */
# endif /* !_CTYPE_DISABLE_MACROS */
#endif /* !_CRT_WCTYPEDATA_DEFINED */

  /* CRT stuff */
#if 1
  extern const unsigned char __newclmap[];
  extern const unsigned char __newcumap[];
  extern pthreadlocinfo __ptlocinfo;
  extern pthreadmbcinfo __ptmbcinfo;
  extern int __globallocalestatus;
  extern int __locale_changed;
  extern struct threadlocaleinfostruct __initiallocinfo;
  extern _locale_tstruct __initiallocalestructinfo;
  pthreadlocinfo __cdecl __updatetlocinfo(void);
  pthreadmbcinfo __cdecl __updatetmbcinfo(void);
#endif

#define _UPPER 0x1
#define _LOWER 0x2
#define _DIGIT 0x4
#define _SPACE 0x8

#define _PUNCT 0x10
#define _CONTROL 0x20
#define _BLANK 0x40
#define _HEX 0x80

#define _LEADBYTE 0x8000
#define _ALPHA (0x0100|_UPPER|_LOWER)

#ifndef _CTYPE_DEFINED
#define _CTYPE_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isctype(
    _In_ int _C,
    _In_ int _Type);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isctype_l(
    _In_ int _C,
    _In_ int _Type,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isalpha(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isalpha_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isupper(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isupper_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  islower(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _islower_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isdigit(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isdigit_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isxdigit(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isxdigit_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isspace(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isspace_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  ispunct(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ispunct_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isalnum(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isalnum_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isprint(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isprint_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isgraph(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isgraph_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iscntrl(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iscntrl_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  toupper(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  tolower(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _tolower(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _tolower_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _toupper(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _toupper_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __isascii(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __toascii(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __iscsymf(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __iscsym(
    _In_ int _C);

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || !defined (NO_OLDNAMES)
  int __cdecl isblank(int _C);
#endif

#endif /* !_CTYPE_DEFINED */

#ifndef _WCTYPE_DEFINED
#define _WCTYPE_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswalpha(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswalpha_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswupper(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswupper_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswlower(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswlower_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswdigit(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswdigit_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswxdigit(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswxdigit_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswspace(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswspace_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswpunct(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswpunct_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswalnum(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswalnum_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswprint(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswprint_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswgraph(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswgraph_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswcntrl(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswcntrl_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswascii(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  isleadbyte(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _isleadbyte_l(
    _In_ int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  towupper(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _towupper_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  towlower(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _towlower_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  iswctype(
    _In_ wint_t _C,
    _In_ wctype_t _Type);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswctype_l(
    _In_ wint_t _C,
    _In_ wctype_t _Type,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __iswcsymf(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswcsymf_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  __iswcsym(
    _In_ wint_t _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _iswcsym_l(
    _In_ wint_t _C,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  int
  __cdecl
  is_wctype(
    _In_ wint_t _C,
    _In_ wctype_t _Type);

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) || !defined (NO_OLDNAMES)
  int __cdecl iswblank(wint_t _C);
#endif

#endif /* _WCTYPE_DEFINED */

#ifndef _CTYPE_DISABLE_MACROS

#ifndef MB_CUR_MAX
#define MB_CUR_MAX ___mb_cur_max_func()
#ifndef __mb_cur_max
#ifdef _MSVCRT_
  extern int __mb_cur_max;
#else
#define __mb_cur_max	(*_imp____mb_cur_max)
  extern int *_imp____mb_cur_max;
#endif
#endif
#ifdef _MSVCRT_
#define ___mb_cur_max_func() (__mb_cur_max)
#else
#define ___mb_cur_max_func() (*_imp____mb_cur_max)
#endif
#endif

#define __chvalidchk(a,b) (__PCTYPE_FUNC[(a)] & (b))
#define _chvalidchk_l(_Char,_Flag,_Locale) (!_Locale ? __chvalidchk(_Char,_Flag) : ((_locale_t)_Locale)->locinfo->pctype[_Char] & (_Flag))
#define _ischartype_l(_Char,_Flag,_Locale) (((_Locale)!=NULL && (((_locale_t)(_Locale))->locinfo->mb_cur_max) > 1) ? _isctype_l(_Char,(_Flag),_Locale) : _chvalidchk_l(_Char,_Flag,_Locale))
#define _isalpha_l(_Char,_Locale) _ischartype_l(_Char,_ALPHA,_Locale)
#define _isupper_l(_Char,_Locale) _ischartype_l(_Char,_UPPER,_Locale)
#define _islower_l(_Char,_Locale) _ischartype_l(_Char,_LOWER,_Locale)
#define _isdigit_l(_Char,_Locale) _ischartype_l(_Char,_DIGIT,_Locale)
#define _isxdigit_l(_Char,_Locale) _ischartype_l(_Char,_HEX,_Locale)
#define _isspace_l(_Char,_Locale) _ischartype_l(_Char,_SPACE,_Locale)
#define _ispunct_l(_Char,_Locale) _ischartype_l(_Char,_PUNCT,_Locale)
#define _isalnum_l(_Char,_Locale) _ischartype_l(_Char,_ALPHA|_DIGIT,_Locale)
#define _isprint_l(_Char,_Locale) _ischartype_l(_Char,_BLANK|_PUNCT|_ALPHA|_DIGIT,_Locale)
#define _isgraph_l(_Char,_Locale) _ischartype_l(_Char,_PUNCT|_ALPHA|_DIGIT,_Locale)
#define _iscntrl_l(_Char,_Locale) _ischartype_l(_Char,_CONTROL,_Locale)
#define _tolower(_Char) ((_Char)-'A'+'a')
#define _toupper(_Char) ((_Char)-'a'+'A')
#define __isascii(_Char) ((unsigned)(_Char) < 0x80)
#define __toascii(_Char) ((_Char) & 0x7f)

#ifndef _WCTYPE_INLINE_DEFINED
#define _WCTYPE_INLINE_DEFINED

#undef _CRT_WCTYPE_NOINLINE
#ifndef __cplusplus
#define iswalpha(_c) (iswctype(_c,_ALPHA))
#define iswupper(_c) (iswctype(_c,_UPPER))
#define iswlower(_c) (iswctype(_c,_LOWER))
#define iswdigit(_c) (iswctype(_c,_DIGIT))
#define iswxdigit(_c) (iswctype(_c,_HEX))
#define iswspace(_c) (iswctype(_c,_SPACE))
#define iswpunct(_c) (iswctype(_c,_PUNCT))
#define iswalnum(_c) (iswctype(_c,_ALPHA|_DIGIT))
#define iswprint(_c) (iswctype(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT))
#define iswgraph(_c) (iswctype(_c,_PUNCT|_ALPHA|_DIGIT))
#define iswcntrl(_c) (iswctype(_c,_CONTROL))
#define iswascii(_c) ((unsigned)(_c) < 0x80)
#define _iswalpha_l(_c,_p) (_iswctype_l(_c,_ALPHA,_p))
#define _iswupper_l(_c,_p) (_iswctype_l(_c,_UPPER,_p))
#define _iswlower_l(_c,_p) (_iswctype_l(_c,_LOWER,_p))
#define _iswdigit_l(_c,_p) (_iswctype_l(_c,_DIGIT,_p))
#define _iswxdigit_l(_c,_p) (_iswctype_l(_c,_HEX,_p))
#define _iswspace_l(_c,_p) (_iswctype_l(_c,_SPACE,_p))
#define _iswpunct_l(_c,_p) (_iswctype_l(_c,_PUNCT,_p))
#define _iswalnum_l(_c,_p) (_iswctype_l(_c,_ALPHA|_DIGIT,_p))
#define _iswprint_l(_c,_p) (_iswctype_l(_c,_BLANK|_PUNCT|_ALPHA|_DIGIT,_p))
#define _iswgraph_l(_c,_p) (_iswctype_l(_c,_PUNCT|_ALPHA|_DIGIT,_p))
#define _iswcntrl_l(_c,_p) (_iswctype_l(_c,_CONTROL,_p))
#endif
#endif

#define __iscsymf(_c) (isalpha(_c) || ((_c)=='_'))
#define __iscsym(_c) (isalnum(_c) || ((_c)=='_'))
#define __iswcsymf(_c) (iswalpha(_c) || ((_c)=='_'))
#define __iswcsym(_c) (iswalnum(_c) || ((_c)=='_'))
#define _iscsymf_l(_c,_p) (_isalpha_l(_c,_p) || ((_c)=='_'))
#define _iscsym_l(_c,_p) (_isalnum_l(_c,_p) || ((_c)=='_'))
#define _iswcsymf_l(_c,_p) (_iswalpha_l(_c,_p) || ((_c)=='_'))
#define _iswcsym_l(_c,_p) (_iswalnum_l(_c,_p) || ((_c)=='_'))
#endif

#ifndef NO_OLDNAMES

#ifndef _CTYPE_DEFINED

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(__isascii)
  _CRTIMP
  int
  __cdecl
  isascii(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(__toascii)
  _CRTIMP
  int
  __cdecl
  toascii(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(__iscsymf)
  _CRTIMP
  int
  __cdecl
  iscsymf(
    _In_ int _C);

  _Check_return_
  _CRTIMP
  _CRT_NONSTDC_DEPRECATE(__iscsym)
  _CRTIMP
  int
  __cdecl
  iscsym(
    _In_ int _C);

#else /* _CTYPE_DEFINED */

#define isascii __isascii
#define toascii __toascii
#define iscsymf __iscsymf
#define iscsym __iscsym

#endif /* _CTYPE_DEFINED */

#endif /* NO_OLDNAMES */

#ifdef __cplusplus
}
#endif

#endif /* !_INC_CTYPE */
