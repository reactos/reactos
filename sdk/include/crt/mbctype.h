/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_MBCTYPE
#define _INC_MBCTYPE

#include <corecrt.h>
#include <ctype.h>

#ifdef __cplusplus
extern "C" {
#endif

  /* CRT stuff */
#if 1
#if defined (_DLL) && defined (_M_IX86)
  /* Retained for compatibility with VC++ 5.0 and earlier versions */
  _Check_return_ _CRTIMP unsigned char * __cdecl __p__mbctype(void);
  _Check_return_ _CRTIMP unsigned char * __cdecl __p__mbcasemap(void);
#endif  /* defined (_DLL) && defined (_M_IX86) */
#endif
#ifndef _mbctype
  _CRTIMP extern unsigned char _mbctype[257];
#endif
#ifndef _mbcasemap
  _CRTIMP extern unsigned char _mbcasemap[257];
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

  _CRTIMP
  int
  __cdecl
  _setmbcp(
    _In_ int _CodePage);

  _CRTIMP
  int
  __cdecl
  _getmbcp(void);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkalnum(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkalnum_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkana(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkana_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkpunct(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkpunct_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkprint(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbkprint_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbalpha(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbalpha_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbpunct(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbpunct_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbalnum(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbalnum_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbprint(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbprint_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbgraph(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbgraph_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

#ifndef _MBLEADTRAIL_DEFINED
#define _MBLEADTRAIL_DEFINED

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbblead(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbblead_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbtrail(
    _In_ unsigned int _C);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbbtrail_l(
    _In_ unsigned int _C,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbslead(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbslead_l(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbstrail(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _ismbstrail_l(
    _In_reads_z_(_Pos - _Str + 1) const unsigned char *_Str,
    _In_z_ const unsigned char *_Pos,
    _In_opt_ _locale_t _Locale);

#endif /* _MBLEADTRAIL_DEFINED */

#endif /* _MBCTYPE_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* _INC_MBCTYPE */
