/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_STRING_S
#define _INC_STRING_S

#include <string.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strset_s(
    _Inout_updates_z_(_DstSize) char *_Dst,
    _In_ size_t _DstSize,
    _In_ int _Value);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strerror_s(
    _Out_writes_z_(_SizeInBytes) char *_Buf,
    _In_ size_t _SizeInBytes,
    _In_opt_z_ const char *_ErrMsg);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strlwr_s(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strlwr_s_l(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strnset_s(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size,
    _In_ int _Val,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strupr_s(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strupr_s_l(
    _Inout_updates_z_(_Size) char *_Str,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  strncat_s(
    _Inout_updates_z_(_DstSizeInChars) char *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const char *_Src,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strncat_s_l(
    _Inout_updates_z_(_DstSizeInChars) char *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const char *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  strcpy_s(
    _Out_writes_z_(_SizeInBytes) char *_Dst,
    _In_ size_t _SizeInBytes,
    _In_z_ const char *_Src);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  strncpy_s(
    _Out_writes_z_(_DstSizeInChars) char *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const char *_Src,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strncpy_s_l(
    _Out_writes_z_(_DstSizeInChars) char *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const char *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_
  _CRTIMP
  char *
  __cdecl
  strtok_s(
    _Inout_opt_z_ char *_Str,
    _In_z_ const char *_Delim,
    _Inout_ _Deref_prepost_opt_z_ char **_Context);

  _Check_return_
  _CRTIMP
  char *
  __cdecl
  _strtok_s_l(
    _Inout_opt_z_ char *_Str,
    _In_z_ const char *_Delim,
    _Inout_ _Deref_prepost_opt_z_ char **_Context,
    _In_opt_ _locale_t _Locale);

#ifndef _WSTRING_S_DEFINED
#define _WSTRING_S_DEFINED

  _Check_return_
  _CRTIMP
  wchar_t *
  __cdecl
  wcstok_s(
    _Inout_opt_z_ wchar_t *_Str,
    _In_z_ const wchar_t *_Delim,
    _Inout_ _Deref_prepost_opt_z_ wchar_t **_Context);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcserror_s(
    _Out_writes_opt_z_(_SizeInWords) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ int _ErrNum);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  __wcserror_s(
    _Out_writes_opt_z_(_SizeInWords) wchar_t *_Buffer,
    _In_ size_t _SizeInWords,
    _In_z_ const wchar_t *_ErrMsg);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsnset_s(
    _Inout_updates_z_(_DstSizeInWords) wchar_t *_Dst,
    _In_ size_t _DstSizeInWords,
    _In_ wchar_t _Val,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsset_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords,
    _In_ wchar_t _Val);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcslwr_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcslwr_s_l(
    _Inout_updates_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsupr_s(
    _Inout_updates_z_(_Size) wchar_t *_Str,
    _In_ size_t _Size);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsupr_s_l(
    _Inout_updates_z_(_Size) wchar_t *_Str,
    _In_ size_t _Size,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  wcsncat_s(
    _Inout_updates_z_(_DstSizeInChars) wchar_t *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsncat_s_l(
    _Inout_updates_z_(_DstSizeInChars) wchar_t *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  wcsncpy_s(
    _Out_writes_z_(_DstSizeInChars) wchar_t *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsncpy_s_l(
    _Out_writes_z_(_DstSizeInChars) wchar_t *_Dst,
    _In_ size_t _DstSizeInChars,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _CRTIMP
  wchar_t *
  __cdecl
  _wcstok_s_l(
    _Inout_opt_z_ wchar_t *_Str,
    _In_z_ const wchar_t *_Delim,
    _Inout_ _Deref_prepost_opt_z_ wchar_t **_Context,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsset_s_l(
    _Inout_updates_z_(_SizeInChars) wchar_t *_Str,
    _In_ size_t _SizeInChars,
    _In_ unsigned int _Val,
    _In_opt_ _locale_t _Locale);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsnset_s_l(
    _Inout_updates_z_(_SizeInChars) wchar_t *_Str,
    _In_ size_t _SizeInChars,
    _In_ unsigned int _Val,
    _In_ size_t _Count,
    _In_opt_ _locale_t _Locale);

#endif /* _WSTRING_S_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* MINGW_HAS_SECURE_API */

#endif /* _INC_STRING_S */
