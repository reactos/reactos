/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_CONIO
#define _INC_CONIO

#include <corecrt.h>

#define __need___va_list
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

  _CRTIMP
  char*
  __cdecl
  _cgets(
    _Pre_notnull_ _Post_z_ char *_Buffer);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cprintf(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cputs(
    _In_z_ const char *_Str);

  _Check_return_opt_
  _CRT_INSECURE_DEPRECATE(_cscanf_s)
  _CRTIMP
  int
  __cdecl
  _cscanf(
    _In_z_ _Scanf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRT_INSECURE_DEPRECATE(_cscanf_s_l)
  _CRTIMP
  int
  __cdecl
  _cscanf_l(
    _In_z_ _Scanf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _getch(void);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _getche(void);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcprintf(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cprintf_p(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcprintf_p(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cprintf_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcprintf_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cprintf_p_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcprintf_p_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _CRTIMP
  int
  __cdecl
  _kbhit(void);

  _CRTIMP
  int
  __cdecl
  _putch(
    _In_ int _Ch);

  _CRTIMP
  int
  __cdecl
  _ungetch(
    _In_ int _Ch);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _getch_nolock(void);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _getche_nolock(void);

  _CRTIMP
  int
  __cdecl
  _putch_nolock(
    _In_ int _Ch);

  _CRTIMP
  int
  __cdecl
  _ungetch_nolock(
    _In_ int _Ch);

#if defined(_X86_) && !defined(__x86_64)
  int __cdecl _inp(unsigned short);
  unsigned short __cdecl _inpw(unsigned short);
  unsigned long __cdecl _inpd(unsigned short);
  int __cdecl _outp(unsigned short,int);
  unsigned short __cdecl _outpw(unsigned short,unsigned short);
  unsigned long __cdecl _outpd(unsigned short,unsigned long);
#endif


#ifndef _WCONIO_DEFINED
#define _WCONIO_DEFINED

#ifndef WEOF
#define WEOF (wint_t)(0xFFFF)
#endif

  _CRTIMP
  wchar_t*
  _cgetws(
    _Pre_notnull_ _Post_z_ wchar_t *_Buffer);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _getwch(void);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _getwche(void);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _putwch(
    wchar_t _WCh);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _ungetwch(
    wint_t _WCh);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cputws(
    _In_z_ const wchar_t *_String);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRT_INSECURE_DEPRECATE(_cwscanf_s)
  _CRTIMP
  int
  __cdecl
  _cwscanf(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRT_INSECURE_DEPRECATE(_cwscanf_s_l)
  _CRTIMP
  int
  __cdecl
  _cwscanf_l(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcwprintf(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _CRTIMP
  int
  __cdecl
  _cwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _CRTIMP
  int
  __cdecl
  _vcwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _CRTIMP
  int
  __cdecl
  _cwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _CRTIMP
  int
  __cdecl
  _vcwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _putwch_nolock(
    wchar_t _WCh);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _getwch_nolock(void);

  _Check_return_
  _CRTIMP
  wint_t
  __cdecl
  _getwche_nolock(void);

  _Check_return_opt_
  _CRTIMP
  wint_t
  __cdecl
  _ungetwch_nolock(
    wint_t _WCh);

#endif /* _WCONIO_DEFINED */

#ifndef _MT
#define _putwch() _putwch_nolock()
#define _getwch() _getwch_nolock()
#define _getwche() _getwche_nolock()
#define _ungetwch() _ungetwch_nolock()
#endif

#ifndef NO_OLDNAMES

  _Check_return_opt_
  _CRT_NONSTDC_DEPRECATE(_cgets)
  _CRT_INSECURE_DEPRECATE(_cgets_s)
  _CRTIMP
  char*
  __cdecl
  cgets(
    _Out_writes_z_(_Inexpressible_(*_Buffer + 2)) char *_Buffer);

  _Check_return_opt_
  _CRT_NONSTDC_DEPRECATE(_cprintf)
  _CRTIMP
  int
  __cdecl
  cprintf(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRT_NONSTDC_DEPRECATE(_cputs)
  _CRTIMP
  int
  __cdecl
  cputs(
    _In_z_ const char *_Str);

  _Check_return_opt_
  _CRT_NONSTDC_DEPRECATE(_cscanf)
  _CRTIMP
  int
  __cdecl
  cscanf(
    _In_z_ _Scanf_format_string_ const char *_Format,
    ...);

  _Check_return_
  _CRT_NONSTDC_DEPRECATE(_getch)
  _CRTIMP
  int
  __cdecl
  getch(void);

  _Check_return_
  _CRT_NONSTDC_DEPRECATE(_getche)
  _CRTIMP
  int
  __cdecl
  getche(void);

  _Check_return_
  _CRT_NONSTDC_DEPRECATE(_kbhit)
  _CRTIMP
  int
  __cdecl
  kbhit(void);

  _Check_return_opt_
  _CRT_NONSTDC_DEPRECATE(_putch)
  _CRTIMP
  int
  __cdecl
  putch(
    int _Ch);

  _Check_return_opt_
  _CRT_NONSTDC_DEPRECATE(_ungetch)
  _CRTIMP
  int
  __cdecl
  ungetch(
    int _Ch);

#if (defined(_X86_) && !defined(__x86_64))
  _CRT_NONSTDC_DEPRECATE(_inp) int __cdecl inp(unsigned short);
  _CRT_NONSTDC_DEPRECATE(_inpw) unsigned short __cdecl inpw(unsigned short);
  _CRT_NONSTDC_DEPRECATE(_outp) int __cdecl outp(unsigned short,int);
  _CRT_NONSTDC_DEPRECATE(_outpw) unsigned short __cdecl outpw(unsigned short,unsigned short);
#endif

#endif /* !NO_OLDNAMES */

#ifdef __cplusplus
}
#endif

#include <sec_api/conio_s.h>

#endif /* _INC_CONIO */
