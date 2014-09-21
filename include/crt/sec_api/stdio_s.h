/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_STDIO_S
#define _INC_STDIO_S

#include <stdio.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STDIO_S_DEFINED
#define _STDIO_S_DEFINED

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  clearerr_s(
    _Inout_ FILE *_File);

  _Check_return_opt_
  int
  __cdecl
  fprintf_s(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  size_t
  __cdecl
  fread_s(
    _Out_writes_bytes_(_ElementSize * _Count) void *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _ElementSize,
    _In_ size_t _Count,
    _Inout_ FILE *_File);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fscanf_s_l(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_s_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  int
  __cdecl
  printf_s(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _scanf_l(
    _In_z_ _Scanf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _scanf_s_l(
    _In_z_ _Scanf_s_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snprintf_s(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snprintf_c(
    _Out_writes_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnprintf_c(
    _Out_writes_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  int
  __cdecl
  sprintf_s(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fscanf_l(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _sscanf_l(
    _In_z_ const char *_Src,
    _In_z_ _Scanf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _sscanf_s_l(
    _In_z_ const char *_Src,
    _In_z_ _Scanf_s_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snscanf_s(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const char *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_s_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snscanf_l(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const char *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snscanf_s_l(
    _In_reads_bytes_(_MaxCount) _Pre_z_ const char *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_s_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  int
  __cdecl
  vfprintf_s(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  int
  __cdecl
  vprintf_s(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  int
  __cdecl
  vsnprintf_s(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnprintf_s(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  int
  __cdecl
  vsprintf_s(
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fprintf_p(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _printf_p(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _sprintf_p(
    _Out_writes_z_(_MaxCount) char *_Dst,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfprintf_p(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vprintf_p(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsprintf_p(
    _Out_writes_z_(_MaxCount) char *_Dst,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scprintf_p(
    _In_z_ _Printf_format_string_ const char *_Format,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscprintf_p(
    _In_z_ _Printf_format_string_ const char *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _printf_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _printf_p_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vprintf_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vprintf_p_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fprintf_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fprintf_p_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfprintf_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfprintf_p_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _sprintf_l(
    _Pre_notnull_ _Post_z_ char *_DstBuf,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _sprintf_p_l(
    _Out_writes_z_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsprintf_l(
    _Pre_notnull_ _Post_z_ char *_DstBuf,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsprintf_p_l(
    _Out_writes_z_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _scprintf_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _scprintf_p_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vscprintf_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vscprintf_p_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _printf_s_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vprintf_s_l(
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fprintf_s_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfprintf_s_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _sprintf_s_l(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsprintf_s_l(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snprintf_s_l(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnprintf_s_l(
    _Out_writes_z_(_DstSize) char *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snprintf_l(
    _Out_writes_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snprintf_c_l(
    _Out_writes_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnprintf_l(
    _Out_writes_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnprintf_c_l(
    _Out_writes_(_MaxCount) char *_DstBuf,
    _In_ size_t _MaxCount,
    const char *,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  fopen_s(
    _Outptr_result_maybenull_ FILE **_File,
    _In_z_ const char *_Filename,
    _In_z_ const char *_Mode);

#ifndef _WSTDIO_S_DEFINED
#define _WSTDIO_S_DEFINED

  _Check_return_opt_
  _CRTIMP
  wchar_t *
  __cdecl
  _getws_s(
    _Out_writes_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords);

  _Check_return_opt_
  int
  __cdecl
  fwprintf_s(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  int
  __cdecl
  wprintf_s(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  int
  __cdecl
  vfwprintf_s(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  int
  __cdecl
  vwprintf_s(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  int
  __cdecl
  swprintf_s(
    _Out_writes_z_(_SizeInWords) wchar_t *_Dst,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  int
  __cdecl
  vswprintf_s(
    _Out_writes_z_(_SizeInWords) wchar_t *_Dst,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwprintf_s(
    _Out_writes_z_(_DstSizeInWords) wchar_t *_DstBuf,
    _In_ size_t _DstSizeInWords,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnwprintf_s(
    _Out_writes_z_(_DstSizeInWords) wchar_t *_DstBuf,
    _In_ size_t _DstSizeInWords,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wprintf_s_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vwprintf_s_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwprintf_s_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfwprintf_s_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_s_l(
    _Out_writes_z_(_DstSize) wchar_t *_DstBuf,
    _In_ size_t _DstSize,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_s_l(
    _Out_writes_z_(_DstSize) wchar_t *_DstBuf,
    _In_ size_t _DstSize,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwprintf_s_l(
    _Out_writes_z_(_DstSize) wchar_t *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnwprintf_s_l(
    _Out_writes_z_(_DstSize) wchar_t *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwscanf_s_l(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_s_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swscanf_s_l(
    _In_z_ const wchar_t *_Src,
    _In_z_ _Scanf_s_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwscanf_s(
    _In_reads_(_MaxCount) _Pre_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_s_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwscanf_s_l(
    _In_reads_(_MaxCount) _Pre_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_s_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wscanf_s_l(
    _In_z_ _Scanf_s_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wfopen_s(
    _Outptr_result_maybenull_ FILE **_File,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wfreopen_s(
    _Outptr_result_maybenull_ FILE **_File,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode,
    _Inout_ FILE *_OldFile);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wtmpnam_s(
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwprintf_p(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfwprintf_p(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_p(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_p(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf_p(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwprintf_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwprintf_p_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfwprintf_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vfwprintf_p_l(
    _Inout_ FILE *_File,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_c_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swprintf_p_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_c_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vswprintf_p_l(
    _Out_writes_z_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _scwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf_p_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwprintf_l(
    _Out_writes_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _vsnwprintf_l(
    _Out_writes_(_MaxCount) wchar_t *_DstBuf,
    _In_ size_t _MaxCount,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _CRTIMP int __cdecl __swprintf_l(wchar_t *_Dest,const wchar_t *_Format,_locale_t _Plocinfo,...);
  _CRTIMP int __cdecl __vswprintf_l(wchar_t *_Dest,const wchar_t *_Format,_locale_t _Plocinfo,va_list _Args);

  _Check_return_
  _CRTIMP
  int
  __cdecl
  _vscwprintf_l(
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    va_list _ArgList);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _fwscanf_l(
    _Inout_ FILE *_File,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _swscanf_l(
    _In_z_ const wchar_t *_Src,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _snwscanf_l(
    _In_reads_(_MaxCount) _Pre_z_ const wchar_t *_Src,
    _In_ size_t _MaxCount,
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_opt_
  _CRTIMP
  int
  __cdecl
  _wscanf_l(
    _In_z_ _Scanf_format_string_ const wchar_t *_Format,
    _In_opt_ _locale_t _Locale,
    ...);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wfopen_s(
    _Outptr_result_maybenull_ FILE ** _File,
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_Mode);

#endif /* _WSTDIO_S_DEFINED */

#endif /* _STDIO_S_DEFINED */

  _Check_return_opt_
  _CRTIMP
  size_t
  __cdecl
  _fread_nolock_s(
    _Out_writes_bytes_(_ElementSize * _Count) void *_DstBuf,
    _In_ size_t _DstSize,
    _In_ size_t _ElementSize,
    _In_ size_t _Count,
    _Inout_ FILE *_File);

#ifdef __cplusplus
}
#endif

#endif /* MINGW_HAS_SECURE_API */

#endif /* _INC_STDIO_S */
