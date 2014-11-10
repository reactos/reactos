/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _INC_WCHAR_S
#define _INC_WCHAR_S

#include <wchar.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _WIO_S_DEFINED
#define _WIO_S_DEFINED

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _waccess_s(
    _In_z_ const wchar_t *_Filename,
    _In_ int _AccessMode);

  _CRTIMP
  errno_t
  __cdecl
  _wmktemp_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *_TemplateName,
    _In_ size_t _SizeInWords);

#endif /* _WIO_S_DEFINED */

#ifndef _WCONIO_S_DEFINED
#define _WCONIO_S_DEFINED

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

#endif /* _WCONIO_S_DEFINED */

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

#endif /* _WSTDIO_S_DEFINED */

#ifndef _WSTDLIB_S_DEFINED
#define _WSTDLIB_S_DEFINED

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _itow_s(
    _In_ int _Val,
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_ int _Radix);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _ltow_s(
    _In_ long _Val,
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_ int _Radix);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _ultow_s(
    _In_ unsigned long _Val,
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_ int _Radix);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wgetenv_s(
    _Out_ size_t *_ReturnSize,
    _Out_writes_z_(_DstSizeInWords) wchar_t *_DstBuf,
    _In_ size_t _DstSizeInWords,
    _In_z_ const wchar_t *_VarName);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wdupenv_s(
    _Outptr_result_buffer_maybenull_(*_BufferSizeInWords) _Deref_post_z_ wchar_t **_Buffer,
    _Out_opt_ size_t *_BufferSizeInWords,
    _In_z_ const wchar_t *_VarName);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _i64tow_s(
    _In_ __int64 _Val,
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_ int _Radix);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _ui64tow_s(
    _In_ unsigned __int64 _Val,
    _Out_writes_z_(_SizeInWords) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_ int _Radix);

#endif /* _WSTDLIB_S_DEFINED */

#ifndef _POSIX_
#ifndef _WSTDLIBP_S_DEFINED
#define _WSTDLIBP_S_DEFINED

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wmakepath_s(
    _Out_writes_z_(_SizeInBytes) wchar_t *_PathResult,
    _In_ size_t _SizeInWords,
    _In_opt_z_ const wchar_t *_Drive,
    _In_opt_z_ const wchar_t *_Dir,
    _In_opt_z_ const wchar_t *_Filename,
    _In_opt_z_ const wchar_t *_Ext);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wputenv_s(
    _In_z_ const wchar_t *_Name,
    _In_z_ const wchar_t *_Value);

  _CRTIMP
  errno_t
  __cdecl
  _wsearchenv_s(
    _In_z_ const wchar_t *_Filename,
    _In_z_ const wchar_t *_EnvVar,
    _Out_writes_z_(_SizeInWords) wchar_t *_ResultPath,
    _In_ size_t _SizeInWords);

  _CRTIMP
  errno_t
  __cdecl
  _wsplitpath_s(
    _In_z_ const wchar_t *_FullPath,
    _Out_writes_opt_z_(_DriveSizeInWords) wchar_t *_Drive,
    _In_ size_t _DriveSizeInWords,
    _Out_writes_opt_z_(_DirSizeInWords) wchar_t *_Dir,
    _In_ size_t _DirSizeInWords,
    _Out_writes_opt_z_(_FilenameSizeInWords) wchar_t *_Filename,
    _In_ size_t _FilenameSizeInWords,
    _Out_writes_opt_z_(_ExtSizeInWords) wchar_t *_Ext,
    _In_ size_t _ExtSizeInWords);

#endif /* _WSTDLIBP_S_DEFINED */
#endif /* _POSIX_ */

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
    wchar_t _Val,
    _In_ size_t _MaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcsset_s(
    _Inout_updates_z_(_SizeInWords) wchar_t *_Str,
    _In_ size_t _SizeInWords,
    wchar_t _Val);

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
    wchar_t *_Str,
    const wchar_t *_Delim,
    wchar_t **_Context,
    _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _wcsset_s_l(
    wchar_t *_Str,
    size_t _SizeInChars,
    unsigned int _Val,
    _locale_t _Locale);

  _CRTIMP
  errno_t
  __cdecl
  _wcsnset_s_l(
    wchar_t *_Str,
    size_t _SizeInChars,
    unsigned int _Val,
    size_t _Count,
    _locale_t _Locale);

#endif /* _WSTRING_S_DEFINED */

#ifndef _WTIME_S_DEFINED
#define _WTIME_S_DEFINED

  _CRTIMP
  errno_t
  __cdecl
  _wasctime_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ const struct tm *_Tm);

  _CRTIMP
  errno_t
  __cdecl
  _wctime32_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ const __time32_t *_Time);

  _CRTIMP
  errno_t
  __cdecl
  _wstrdate_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(9) wchar_t *_Buf,
    _In_range_(>= , 9) size_t _SizeInWords);

  _CRTIMP
  errno_t
  __cdecl
  _wstrtime_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(9) wchar_t *_Buf,
    _In_ size_t _SizeInWords);

  _CRTIMP
  errno_t
  __cdecl
  _wctime64_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ const __time64_t *_Time);

#if !defined (RC_INVOKED) && !defined (_INC_WTIME_S_INL)
#define _INC_WTIME_S_INL
  errno_t __cdecl _wctime_s(wchar_t *, size_t, const time_t *);
#ifndef _USE_32BIT_TIME_T
__CRT_INLINE errno_t __cdecl _wctime_s(wchar_t *_Buffer,size_t _SizeInWords,const time_t *_Time) { return _wctime64_s(_Buffer,_SizeInWords,_Time); }
#endif
#endif

#endif /* _WTIME_S_DEFINED */

  _CRTIMP
  errno_t
  __cdecl
  mbsrtowcs_s(
    _Out_opt_ size_t *_Retval,
    _Out_writes_opt_z_(_SizeInWords) wchar_t *_Dst,
    _In_ size_t _SizeInWords,
    _Inout_ _Deref_prepost_opt_valid_ const char **_PSrc,
    _In_ size_t _N,
    _Out_opt_ mbstate_t *_State);

  _CRTIMP
  errno_t
  __cdecl
  wcrtomb_s(
    _Out_opt_ size_t *_Retval,
    _Out_writes_opt_z_(_SizeInBytes) char *_Dst,
    _In_ size_t _SizeInBytes,
    _In_ wchar_t _Ch,
    _Out_opt_ mbstate_t *_State);

  _CRTIMP
  errno_t
  __cdecl
  wcsrtombs_s(
    _Out_opt_ size_t *_Retval,
    _Out_writes_bytes_to_opt_(_SizeInBytes, *_Retval) char *_Dst,
    _In_ size_t _SizeInBytes,
    _Inout_ _Deref_prepost_z_ const wchar_t **_Src,
    _In_ size_t _Size,
    _Out_opt_ mbstate_t *_State);

#ifdef __cplusplus
}
#endif

#endif /* MINGW_HAS_SECURE_API */

#endif /* _INC_WCHAR_S */
