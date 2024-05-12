/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */

#ifndef _TIME_H_
#define _TIME_H_

#include <corecrt.h>

#ifndef _WIN32
#error Only Win32 target is supported!
#endif

#pragma pack(push,_CRT_PACKING)

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _TIME32_T_DEFINED
#define _TIME32_T_DEFINED
  typedef long __time32_t;
#endif

#ifndef _TIME64_T_DEFINED
#define _TIME64_T_DEFINED
#if _INTEGRAL_MAX_BITS >= 64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef int _time64_t __attribute__ ((mode (DI)));
#else
  __MINGW_EXTENSION typedef __int64 __time64_t;
#endif
#endif
#endif

#ifndef _TIME_T_DEFINED
#define _TIME_T_DEFINED
#ifdef _USE_32BIT_TIME_T
  typedef __time32_t time_t;
#else
  typedef __time64_t time_t;
#endif
#endif

#ifndef _CLOCK_T_DEFINED
#define _CLOCK_T_DEFINED
  typedef long clock_t;
#endif

#ifndef _SIZE_T_DEFINED
#define _SIZE_T_DEFINED
#undef size_t
#ifdef _WIN64
#if defined(__GNUC__) && defined(__STRICT_ANSI__)
  typedef unsigned int size_t __attribute__ ((mode (DI)));
#else
  __MINGW_EXTENSION typedef unsigned __int64 size_t;
#endif
#else
  typedef unsigned int size_t;
#endif
#endif

#ifndef _TM_DEFINED
#define _TM_DEFINED
  struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
  };
#endif

#define CLOCKS_PER_SEC 1000

  _CRTDATA(extern int _daylight);
  _CRTDATA(extern long _dstbias);
  _CRTDATA(extern long _timezone);
  _CRTDATA(extern char * _tzname[2]);

  _CRTIMP errno_t __cdecl _get_daylight(_Out_ int *_Daylight);
  _CRTIMP errno_t __cdecl _get_dstbias(_Out_ long *_Daylight_savings_bias);
  _CRTIMP errno_t __cdecl _get_timezone(_Out_ long *_Timezone);

  _CRTIMP
  errno_t
  __cdecl
  _get_tzname(
    _Out_ size_t *_ReturnValue,
    _Out_writes_z_(_SizeInBytes) char *_Buffer,
    _In_ size_t _SizeInBytes,
    _In_ int _Index);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(asctime_s)
  char *
  __cdecl
  asctime(
    _In_ const struct tm *_Tm);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_ctime32_s)
  char *
  __cdecl
  _ctime32(
    _In_ const __time32_t *_Time);

  _Check_return_ _CRTIMP clock_t __cdecl clock(void);

  _CRTIMP
  double
  __cdecl
  _difftime32(
    _In_ __time32_t _Time1,
    _In_ __time32_t _Time2);

  _Check_return_
  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_gmtime32_s)
  struct tm *
  __cdecl
  _gmtime32(
    _In_ const __time32_t *_Time);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_localtime32_s)
  struct tm *
  __cdecl
  _localtime32(
    _In_ const __time32_t *_Time);

  _Success_(return > 0)
  _CRTIMP
  size_t
  __cdecl
  strftime(
    _Out_writes_z_(_SizeInBytes) char *_Buf,
    _In_ size_t _SizeInBytes,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_ const struct tm *_Tm);

  _Success_(return > 0)
  _CRTIMP
  size_t
  __cdecl
  _strftime_l(
    _Out_writes_z_(_Max_size) char *_Buf,
    _In_ size_t _Max_size,
    _In_z_ _Printf_format_string_ const char *_Format,
    _In_ const struct tm *_Tm,
    _In_opt_ _locale_t _Locale);

  _CRTIMP char *__cdecl _strdate(_Out_writes_z_(9) char *_Buffer);
  _CRTIMP char *__cdecl _strtime(_Out_writes_z_(9) char *_Buffer);
  _CRTIMP __time32_t __cdecl _time32(_Out_opt_ __time32_t *_Time);
  _CRTIMP __time32_t __cdecl _mktime32(_Inout_ struct tm *_Tm);
  _CRTIMP __time32_t __cdecl _mkgmtime32(_Inout_ struct tm *_Tm);
  _CRTIMP void __cdecl _tzset(void);
  _CRT_OBSOLETE(GetLocalTime) unsigned __cdecl _getsystime(_Out_ struct tm *_Tm);

  _CRT_OBSOLETE(GetLocalTime)
  unsigned
  __cdecl
  _setsystime(
    _In_ struct tm *_Tm,
    unsigned _MilliSec);

  _Check_return_wat_
  _CRTIMP
  errno_t
  __cdecl
  asctime_s(
    _Out_writes_(_SizeInBytes) _Post_readable_size_(26) char *_Buf,
    _In_range_(>= , 26) size_t _SizeInBytes,
    _In_ const struct tm *_Tm);

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

#if _INTEGRAL_MAX_BITS >= 64

  _Check_return_
  _CRTIMP
  double
  __cdecl
  _difftime64(
    _In_ __time64_t _Time1,
    _In_ __time64_t _Time2);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_ctime64_s)
  char *
  __cdecl
  _ctime64(
    _In_ const __time64_t *_Time);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_gmtime64_s)
  struct tm *
  __cdecl
  _gmtime64(
    _In_ const __time64_t *_Time);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_localtime64_s)
  struct tm *
  __cdecl
  _localtime64(
    _In_ const __time64_t *_Time);

  _CRTIMP __time64_t __cdecl _mktime64(_Inout_ struct tm *_Tm);
  _CRTIMP __time64_t __cdecl _mkgmtime64(_Inout_ struct tm *_Tm);
  _CRTIMP __time64_t __cdecl _time64(_Out_opt_ __time64_t *_Time);

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

#endif /* _INTEGRAL_MAX_BITS >= 64 */

#ifndef _WTIME_DEFINED
#define _WTIME_DEFINED

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_wasctime_s)
  wchar_t *
  __cdecl
  _wasctime(
    _In_ const struct tm *_Tm);

  _CRTIMP wchar_t *__cdecl _wctime(const time_t *_Time);

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_wctime32_s)
  wchar_t *
  __cdecl
  _wctime32(
    _In_ const __time32_t *_Time);

  _Success_(return > 0)
  _CRTIMP
  size_t
  __cdecl
  wcsftime(
    _Out_writes_z_(_SizeInWords) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_ const struct tm *_Tm);

  _Success_(return > 0)
  _CRTIMP
  size_t
  __cdecl
  _wcsftime_l(
    _Out_writes_z_(_SizeInWords) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_z_ _Printf_format_string_ const wchar_t *_Format,
    _In_ const struct tm *_Tm,
    _In_opt_ _locale_t _Locale);

  _CRTIMP wchar_t *__cdecl _wstrdate(_Out_writes_z_(9) wchar_t *_Buffer);
  _CRTIMP wchar_t *__cdecl _wstrtime(_Out_writes_z_(9) wchar_t *_Buffer);

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

#if _INTEGRAL_MAX_BITS >= 64

  _CRTIMP
  _CRT_INSECURE_DEPRECATE(_wctime64_s)
  wchar_t *
  __cdecl
  _wctime64(
    _In_ const __time64_t *_Time);

  _Success_(return == 0)
  _CRTIMP
  errno_t
  __cdecl
  _wctime64_s(
    _Out_writes_(_SizeInWords) _Post_readable_size_(26) wchar_t *_Buf,
    _In_ size_t _SizeInWords,
    _In_ const __time64_t *_Time);

#endif /* _INTEGRAL_MAX_BITS >= 64 */

#if !defined (RC_INVOKED) && !defined (_INC_WTIME_INL)
#define _INC_WTIME_INL
#ifdef _USE_32BIT_TIME_T
/* Do it like this to be compatible to msvcrt.dll on 32 bit windows XP and before */
__CRT_INLINE wchar_t *__cdecl _wctime(const time_t *_Time) { return _wctime32(_Time); }
#else
__CRT_INLINE wchar_t *__cdecl _wctime(const time_t *_Time) { return _wctime64(_Time); }
#endif
#endif

#endif /* !_WTIME_DEFINED */

 _CRTIMP double __cdecl difftime(time_t _Time1,time_t _Time2);
 _CRTIMP char *__cdecl ctime(const time_t *_Time);
 _CRTIMP struct tm *__cdecl gmtime(const time_t *_Time);
 _CRTIMP struct tm *__cdecl localtime(const time_t *_Time);
 _CRTIMP struct tm *__cdecl localtime_r(const time_t *_Time,struct tm *);

 _CRTIMP time_t __cdecl mktime(struct tm *_Tm);
 _CRTIMP time_t __cdecl _mkgmtime(struct tm *_Tm);
 _CRTIMP time_t __cdecl time(time_t *_Time);

#if !defined(RC_INVOKED)  && !defined(_NO_INLINING)  && !defined(_CRTBLD)
#ifdef _USE_32BIT_TIME_T
#if 0
__CRT_INLINE double __cdecl difftime(time_t _Time1,time_t _Time2) { return _difftime32(_Time1,_Time2); }
__CRT_INLINE char *__cdecl ctime(const time_t *_Time) { return _ctime32(_Time); }
__CRT_INLINE struct tm *__cdecl gmtime(const time_t *_Time) { return _gmtime32(_Time); }
__CRT_INLINE struct tm *__cdecl localtime(const time_t *_Time) { return _localtime32(_Time); }
__CRT_INLINE errno_t __cdecl localtime_s(struct tm *_Tm,const time_t *_Time) { return _localtime32_s(_Tm,_Time); }
__CRT_INLINE time_t __cdecl mktime(struct tm *_Tm) { return _mktime32(_Tm); }
__CRT_INLINE time_t __cdecl _mkgmtime(struct tm *_Tm) { return _mkgmtime32(_Tm); }
__CRT_INLINE time_t __cdecl time(time_t *_Time) { return _time32(_Time); }
#endif
#else
__CRT_INLINE double __cdecl difftime(time_t _Time1,time_t _Time2) { return _difftime64(_Time1,_Time2); }
__CRT_INLINE char *__cdecl ctime(const time_t *_Time) { return _ctime64(_Time); }
__CRT_INLINE struct tm *__cdecl gmtime(const time_t *_Time) { return _gmtime64(_Time); }
__CRT_INLINE struct tm *__cdecl localtime(const time_t *_Time) { return _localtime64(_Time); }
__CRT_INLINE errno_t __cdecl localtime_s(struct tm *_Tm,const time_t *_Time) { return _localtime64_s(_Tm,_Time); }
__CRT_INLINE time_t __cdecl mktime(struct tm *_Tm) { return _mktime64(_Tm); }
__CRT_INLINE time_t __cdecl _mkgmtime(struct tm *_Tm) { return _mkgmtime64(_Tm); }
__CRT_INLINE time_t __cdecl time(time_t *_Time) { return _time64(_Time); }
#endif
#endif

#if !defined(NO_OLDNAMES) || defined(_POSIX)
#define CLK_TCK CLOCKS_PER_SEC

  _CRTIMP extern int daylight;
  _CRTIMP extern long timezone;
  _CRTIMP extern char *tzname[2];
  _CRTIMP void __cdecl tzset(void);
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif /* End _TIME_H_ */
