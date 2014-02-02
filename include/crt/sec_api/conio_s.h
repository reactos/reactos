/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_CONIO_S
#define _INC_CONIO_S

#include <conio.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _cgets_s(
    _Out_writes_z_(_Size) char *_Buffer,
    _In_ size_t _Size,
    _Out_ size_t *_SizeRead);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cprintf_s(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cscanf_s(
    _In_z_ _Scanf_s_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cscanf_s_l(
    _In_z_ _Scanf_s_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcprintf_s(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cprintf_s_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcprintf_s_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

#ifndef _WCONIO_DEFINED_S
#define _WCONIO_DEFINED_S

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _cgetws_s(
    _Out_writes_to_(_SizeInWords, *_SizeRead) wchar_t *_Buffer,
    _In_ size_t _SizeInWords,
    _Out_ size_t *_SizeRead);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwprintf_s(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwscanf_s(
    _In_z_ _Scanf_s_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _cwscanf_s_l(
    _In_z_ _Scanf_s_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vcwprintf_s(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _CRTIMP
  int
  __cdecl
  _cwprintf_s_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _CRTIMP
  int
  __cdecl
  _vcwprintf_s_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

#endif /* _WCONIO_DEFINED_S */

#ifdef __cplusplus
}
#endif

#endif /* MINGW_HAS_SECURE_API */

#endif /* _INC_CONIO_S */
