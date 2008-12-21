/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_MBCTYPE
#define _INC_MBCTYPE

#include <crtdefs.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

  /* CRT stuff */
#if 1
#if defined (_DLL) && defined (_M_IX86)
  /* Retained for compatibility with VC++ 5.0 and earlier versions */
  _CRTIMP unsigned char * __cdecl __p__mbctype(void);
  _CRTIMP unsigned char * __cdecl __p__mbcasemap(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#endif
#ifndef _mbctype
#ifdef _MSVCRT_
  extern unsigned char _mbctype[257];
#else
#define _mbctype	(*_imp___mbctype)
  extern unsigned char **_imp___mbctype;
#endif
#endif
#ifndef _mbcasemap
#ifdef _MSVCRT_
  extern unsigned char *_mbcasemap;
#else
#define _mbcasemap	(*_imp___mbcasemap)
  extern unsigned char **_imp___mbcasemap;
#endif
#endif

  /* CRT stuff */
#if 1
  extern pthreadmbcinfo __ptmbcinfo;
  extern int __globallocalestatus;
  extern int __locale_changed;
  extern struct threadmbcinfostruct __initialmbcinfo;
  pthreadmbcinfo __cdecl __updatetmbcinfo(void);
#endif

#define _MS 0x01
#define _MP 0x02
#define _M1 0x04
#define _M2 0x08

#define _SBUP 0x10
#define _SBLOW 0x20

#define _MBC_SINGLE 0
#define _MBC_LEAD 1
#define _MBC_TRAIL 2
#define _MBC_ILLEGAL (-1)

#define _KANJI_CP 932

#define _MB_CP_SBCS 0
#define _MB_CP_OEM -2
#define _MB_CP_ANSI -3
#define _MB_CP_LOCALE -4

#ifndef _MBCTYPE_DEFINED
#define _MBCTYPE_DEFINED

  _CRTIMP int __cdecl _setmbcp(int _CodePage);
  _CRTIMP int __cdecl _getmbcp(void);
  _CRTIMP int __cdecl _ismbbkalnum(unsigned int _C);
  _CRTIMP int __cdecl _ismbbkalnum_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbkana(unsigned int _C);
  _CRTIMP int __cdecl _ismbbkana_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbkpunct(unsigned int _C);
  _CRTIMP int __cdecl _ismbbkpunct_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbkprint(unsigned int _C);
  _CRTIMP int __cdecl _ismbbkprint_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbalpha(unsigned int _C);
  _CRTIMP int __cdecl _ismbbalpha_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbpunct(unsigned int _C);
  _CRTIMP int __cdecl _ismbbpunct_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbalnum(unsigned int _C);
  _CRTIMP int __cdecl _ismbbalnum_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbprint(unsigned int _C);
  _CRTIMP int __cdecl _ismbbprint_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbgraph(unsigned int _C);
  _CRTIMP int __cdecl _ismbbgraph_l(unsigned int _C,_locale_t _Locale);
#ifndef _MBLEADTRAIL_DEFINED
#define _MBLEADTRAIL_DEFINED
  _CRTIMP int __cdecl _ismbblead(unsigned int _C);
  _CRTIMP int __cdecl _ismbblead_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbbtrail(unsigned int _C);
  _CRTIMP int __cdecl _ismbbtrail_l(unsigned int _C,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbslead(const unsigned char *_Str,const unsigned char *_Pos);
  _CRTIMP int __cdecl _ismbslead_l(const unsigned char *_Str,const unsigned char *_Pos,_locale_t _Locale);
  _CRTIMP int __cdecl _ismbstrail(const unsigned char *_Str,const unsigned char *_Pos);
  _CRTIMP int __cdecl _ismbstrail_l(const unsigned char *_Str,const unsigned char *_Pos,_locale_t _Locale);
#endif
#endif

#ifdef __cplusplus
}
#endif
#endif
