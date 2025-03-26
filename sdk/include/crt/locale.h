/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_LOCALE
#define _INC_LOCALE

#include <corecrt.h>

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#define LC_ALL 0
#define LC_COLLATE 1
#define LC_CTYPE 2
#define LC_MONETARY 3
#define LC_NUMERIC 4
#define LC_TIME 5

#define LC_MIN LC_ALL
#define LC_MAX LC_TIME

#ifndef _LCONV_DEFINED
#define _LCONV_DEFINED
  struct lconv {
    char *decimal_point;
    char *thousands_sep;
    char *grouping;
    char *int_curr_symbol;
    char *currency_symbol;
    char *mon_decimal_point;
    char *mon_thousands_sep;
    char *mon_grouping;
    char *positive_sign;
    char *negative_sign;
    char int_frac_digits;
    char frac_digits;
    char p_cs_precedes;
    char p_sep_by_space;
    char n_cs_precedes;
    char n_sep_by_space;
    char p_sign_posn;
    char n_sign_posn;
  };
#endif

#ifndef _CONFIG_LOCALE_SWT
#define _CONFIG_LOCALE_SWT

#define _ENABLE_PER_THREAD_LOCALE 0x1
#define _DISABLE_PER_THREAD_LOCALE 0x2
#define _ENABLE_PER_THREAD_LOCALE_GLOBAL 0x10
#define _DISABLE_PER_THREAD_LOCALE_GLOBAL 0x20
#define _ENABLE_PER_THREAD_LOCALE_NEW 0x100
#define _DISABLE_PER_THREAD_LOCALE_NEW 0x200

#endif

  _Check_return_opt_
  int
  __cdecl
  _configthreadlocale(
    _In_ int _Flag);

  _Check_return_opt_
  char*
  __cdecl
  setlocale(
    _In_ int _Category,
    _In_opt_z_ const char *_Locale);

  _Check_return_opt_
  _CRTIMP
  struct lconv*
  __cdecl
  localeconv(void);

  _Check_return_opt_
  _locale_t
  __cdecl
  _get_current_locale(void);

  _Check_return_opt_
  _locale_t
  __cdecl
  _create_locale(
    _In_ int _Category,
    _In_z_ const char *_Locale);

  void
  __cdecl
  _free_locale(
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _locale_t
  __cdecl
  __get_current_locale(void);

  _Check_return_
  _locale_t
  __cdecl
  __create_locale(
    _In_ int _Category,
    _In_z_ const char *_Locale);

  void
  __cdecl
  __free_locale(
    _In_opt_ _locale_t _Locale);

_CRTIMP
unsigned int
__cdecl
___lc_collate_cp_func(void);

_CRTIMP
unsigned int
__cdecl
___lc_codepage_func(void);

#ifndef _WLOCALE_DEFINED
#define _WLOCALE_DEFINED
  _Check_return_opt_
  _CRTIMP
  wchar_t*
  __cdecl
  _wsetlocale(
    _In_ int _Category,
    _In_opt_z_ const wchar_t *_Locale);
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)
#endif
