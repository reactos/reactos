/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _INC_STDLIB_S
#define _INC_STDLIB_S

#include <stdlib.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  _dupenv_s(
    _Outptr_result_buffer_maybenull_(*_PBufferSizeInBytes) _Outptr_result_maybenull_z_ char **_PBuffer,
    _Out_opt_ size_t *_PBufferSizeInBytes,
    _In_z_ const char *_VarName);

  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  _itoa_s(
    _In_ int _Value,
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ int _Radix);

  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  _i64toa_s(
    _In_ __int64 _Val,
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ int _Radix);

  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  _ui64toa_s(
    _In_ unsigned __int64 _Val,
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ int _Radix);

  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  _ltoa_s(
    _In_ long _Val,
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ int _Radix);

  _Success_(return!=EINVAL)
  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  mbstowcs_s(
    _Out_opt_ size_t *pcchConverted,
    _Out_writes_to_opt_(sizeInWords, *pcchConverted) wchar_t *pwcstr,
    _In_ size_t sizeInWords,
    _In_reads_or_z_(count) const char *pmbstr,
    _In_ size_t count);

  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  _mbstowcs_s_l(
    _Out_opt_ size_t *_PtNumOfCharConverted,
    _Out_writes_to_opt_(_SizeInWords, *_PtNumOfCharConverted) wchar_t *_DstBuf,
    _In_ size_t _SizeInWords,
    _In_reads_or_z_(_MaxCount) const char *_SrcBuf,
    _In_ size_t _MaxCount,
    _In_opt_ _locale_t _Locale);

  _Check_return_opt_
  _CRTIMP
  errno_t
  __cdecl
  _ultoa_s(
    _In_ unsigned long _Val,
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ int _Radix);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wctomb_s_l(
    _Out_opt_ int *_SizeConverted,
    _Out_writes_opt_z_(_SizeInBytes) char *_MbCh,
    _In_ size_t _SizeInBytes,
    _In_ wchar_t _WCh,
    _In_opt_ _locale_t _Locale);

  _Success_(return!=EINVAL)
  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  wcstombs_s(
    _Out_opt_ size_t *pcchConverted,
    _Out_writes_bytes_to_opt_(cjDstSize, *pcchConverted) char *pmbstrDst,
    _In_ size_t cjDstSize,
    _In_z_ const wchar_t *pwszSrc,
    _In_ size_t cjMaxCount);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wcstombs_s_l(
    _Out_opt_ size_t *_PtNumOfCharConverted,
    _Out_writes_bytes_to_opt_(_DstSizeInBytes, *_PtNumOfCharConverted) char *_Dst,
    _In_ size_t _DstSizeInBytes,
    _In_z_ const wchar_t *_Src,
    _In_ size_t _MaxCountInBytes,
    _In_opt_ _locale_t _Locale);

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
    _Out_writes_opt_z_(_DstSizeInWords) wchar_t *_DstBuf,
    _In_ size_t _DstSizeInWords,
    _In_z_ const wchar_t *_VarName);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wdupenv_s(
    _Outptr_result_buffer_maybenull_(*_BufferSizeInWords) _Outptr_result_maybenull_z_ wchar_t **_Buffer,
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

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _ecvt_s(
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ double _Val,
    _In_ int _NumOfDights,
    _Out_ int *_PtDec,
    _Out_ int *_PtSign);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _fcvt_s(
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ double _Val,
    _In_ int _NumOfDec,
    _Out_ int *_PtDec,
    _Out_ int *_PtSign);

  _CRTIMP
  errno_t
  __cdecl
  _gcvt_s(
    _Out_writes_z_(_Size) char *_DstBuf,
    _In_ size_t _Size,
    _In_ double _Val,
    _In_ int _NumOfDigits);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _makepath_s(
    _Out_writes_z_(_Size) char *_PathResult,
    _In_ size_t _Size,
    _In_opt_z_ const char *_Drive,
    _In_opt_z_ const char *_Dir,
    _In_opt_z_ const char *_Filename,
    _In_opt_z_ const char *_Ext);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _putenv_s(
    _In_z_ const char *_Name,
    _In_z_ const char *_Value);

  _CRTIMP
  errno_t
  __cdecl
  _searchenv_s(
    _In_z_ const char *_Filename,
    _In_z_ const char *_EnvVar,
    _Out_writes_z_(_SizeInBytes) char *_ResultPath,
    _In_ size_t _SizeInBytes);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _splitpath_s(
    _In_z_ const char *path,
    _Out_writes_opt_z_(drive_size) char *drive,
    _In_ size_t drive_size,
    _Out_writes_opt_z_(dir_size) char *dir,
    _In_ size_t dir_size,
    _Out_writes_opt_z_(fname_size) char *fname,
    _In_ size_t fname_size,
    _Out_writes_opt_z_(ext_size) char *ext,
    _In_ size_t ext_size);

#ifndef _WSTDLIBP_S_DEFINED
#define _WSTDLIBP_S_DEFINED

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _wmakepath_s(
    _Out_writes_z_(_SizeInWords) wchar_t *_PathResult,
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
    _In_z_ const wchar_t *path,
    _Out_writes_opt_z_(drive_size) wchar_t *drive,
    _In_ size_t drive_size,
    _Out_writes_opt_z_(dir_size) wchar_t *dir,
    _In_ size_t dir_size,
    _Out_writes_opt_z_(fname_size) wchar_t *fname,
    _In_ size_t fname_size,
    _Out_writes_opt_z_(ext_size) wchar_t *ext,
    _In_ size_t ext_size);

#endif /* _WSTDLIBP_S_DEFINED */

#endif /* _POSIX_ */

#ifdef __cplusplus
}
#endif

#endif /* defined(MINGW_HAS_SECURE_API) */

#endif /* _INC_STDLIB_S */
