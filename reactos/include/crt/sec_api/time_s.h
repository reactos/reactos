/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _TIME_H__S
#define _TIME_H__S

#include <time.h>

#if defined(MINGW_HAS_SECURE_API)

#ifdef __cplusplus
extern "C" {
#endif

  _Success_(return == 0)
  _CRTIMP
  errno_t
  __cdecl
  _ctime32_s(
    _Out_writes_(_SizeInBytes) _Post_readable_size_(26) char *_Buf,
    _In_ size_t _SizeInBytes,
    _In_ const __time32_t *_Time);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _gmtime32_s(
    _In_ struct tm *_Tm,
    _In_ const __time32_t *_Time);

  _CRTIMP
  errno_t
  __cdecl
  _localtime32_s(
    _Out_ struct tm *_Tm,
    _In_ const __time32_t *_Time);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strdate_s(
    _Out_writes_(_SizeInBytes) _Post_readable_size_(9) char *_Buf,
    _In_range_(>= , 9) size_t _SizeInBytes);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  _strtime_s(
    _Out_writes_(_SizeInBytes) _Post_readable_size_(9) char *_Buf,
    _In_range_(>= , 9) size_t _SizeInBytes);

  _CRTIMP
  errno_t
  __cdecl
  _ctime64_s(
    _Out_writes_z_(_SizeInBytes) char *_Buf,
    _In_ size_t _SizeInBytes,
    _In_ const __time64_t *_Time);

  _CRTIMP
  errno_t
  __cdecl
  _gmtime64_s(
    _Out_ struct tm *_Tm,
    _In_ const __time64_t *_Time);

  _CRTIMP
  errno_t
  __cdecl
  _localtime64_s(
    _Out_ struct tm *_Tm,
    _In_ const __time64_t *_Time);

#ifndef _WTIME_S_DEFINED
#define _WTIME_S_DEFINED

  _CRTIMP
  errno_t
  __cdecl
  _wasctime_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_range_(>= , 26) size_t _SizeInWords,
    _In_ const struct tm *_Tm);

  _Success_(return == 0)
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
    _In_ size_t _SizeInWords);

  _CRTIMP
  errno_t
  __cdecl
  _wstrtime_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(9) wchar_t *_Buf,
    _In_range_(>= , 9) size_t _SizeInWords);

  _Success_(return == 0)
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

#ifndef RC_INVOKED
#ifdef _USE_32BIT_TIME_T
__CRT_INLINE errno_t __cdecl localtime_s(struct tm *_Tm,const time_t *_Time) { return _localtime32_s(_Tm,_Time); }
#else
__CRT_INLINE errno_t __cdecl localtime_s(struct tm *_Tm,const time_t *_Time) { return _localtime64_s(_Tm,_Time); }
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* MINGW_HAS_SECURE_API */

#endif /* _TIME_H__S */
