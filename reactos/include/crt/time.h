/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within this package.
 */
#ifndef _TIME_H_
#define _TIME_H_

#include <crtdefs.h>

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
  typedef __int64 __time64_t;
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
  typedef unsigned __int64 size_t;
#endif
#else
  typedef unsigned int size_t;
#endif
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL 0
#else
#define NULL ((void *)0)
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

  _CRTIMP errno_t __cdecl _get_daylight(int *_Daylight);
  _CRTIMP errno_t __cdecl _get_dstbias(long *_Daylight_savings_bias);
  _CRTIMP errno_t __cdecl _get_timezone(long *_Timezone);
  _CRTIMP errno_t __cdecl _get_tzname(size_t *_ReturnValue,char *_Buffer,size_t _SizeInBytes,int _Index);

  _CRTIMP _CRT_INSECURE_DEPRECATE(asctime_s) char *__cdecl asctime(const struct tm *_Tm);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_ctime32_s) char *__cdecl _ctime32(const __time32_t *_Time);
  _CRTIMP clock_t __cdecl clock(void);
  _CRTIMP double __cdecl _difftime32(__time32_t _Time1,__time32_t _Time2);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_gmtime32_s) struct tm *__cdecl _gmtime32(const __time32_t *_Time);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_localtime32_s) struct tm *__cdecl _localtime32(const __time32_t *_Time);
  _CRTIMP size_t __cdecl strftime(char *_Buf,size_t _SizeInBytes,const char *_Format,const struct tm *_Tm);
  _CRTIMP size_t __cdecl _strftime_l(char *_Buf,size_t _Max_size,const char *_Format,const struct tm *_Tm,_locale_t _Locale);
  _CRTIMP char *__cdecl _strdate(char *_Buffer);
  _CRTIMP char *__cdecl _strtime(char *_Buffer);
  _CRTIMP __time32_t __cdecl _time32(__time32_t *_Time);
  _CRTIMP __time32_t __cdecl _mktime32(struct tm *_Tm);
  _CRTIMP __time32_t __cdecl _mkgmtime32(struct tm *_Tm);
  _CRTIMP void __cdecl _tzset(void);
  _CRT_OBSOLETE(GetLocalTime) unsigned __cdecl _getsystime(struct tm *_Tm);
  _CRT_OBSOLETE(GetLocalTime) unsigned __cdecl _setsystime(struct tm *_Tm,unsigned _MilliSec);

  _CRTIMP errno_t __cdecl asctime_s(char *_Buf,size_t _SizeInWords,const struct tm *_Tm);
  _CRTIMP errno_t __cdecl _ctime32_s(char *_Buf,size_t _SizeInBytes,const __time32_t *_Time);
  _CRTIMP errno_t __cdecl _gmtime32_s(struct tm *_Tm,const __time32_t *_Time);
  _CRTIMP errno_t __cdecl localtime_s(struct tm *_Tm,const time_t *_Time);
  _CRTIMP errno_t __cdecl _localtime32_s(struct tm *_Tm,const __time32_t *_Time);
  _CRTIMP errno_t __cdecl _strdate_s(char *_Buf,size_t _SizeInBytes);
  _CRTIMP errno_t __cdecl _strtime_s(char *_Buf ,size_t _SizeInBytes);

#if _INTEGRAL_MAX_BITS >= 64
  _CRTIMP double __cdecl _difftime64(__time64_t _Time1,__time64_t _Time2);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_ctime64_s) char *__cdecl _ctime64(const __time64_t *_Time);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_gmtime64_s) struct tm *__cdecl _gmtime64(const __time64_t *_Time);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_localtime64_s) struct tm *__cdecl _localtime64(const __time64_t *_Time);
  _CRTIMP __time64_t __cdecl _mktime64(struct tm *_Tm);
  _CRTIMP __time64_t __cdecl _mkgmtime64(struct tm *_Tm);
  _CRTIMP __time64_t __cdecl _time64(__time64_t *_Time);

  _CRTIMP errno_t __cdecl _ctime64_s(char *_Buf,size_t _SizeInBytes,const __time64_t *_Time);
  _CRTIMP errno_t __cdecl _gmtime64_s(struct tm *_Tm,const __time64_t *_Time);
  _CRTIMP errno_t __cdecl _localtime64_s(struct tm *_Tm,const __time64_t *_Time);
#endif

#ifndef _WTIME_DEFINED
#define _WTIME_DEFINED
  _CRTIMP _CRT_INSECURE_DEPRECATE(_wasctime_s) wchar_t *__cdecl _wasctime(const struct tm *_Tm);
  _CRTIMP wchar_t *__cdecl _wctime(const time_t *_Time);
  _CRTIMP _CRT_INSECURE_DEPRECATE(_wctime32_s) wchar_t *__cdecl _wctime32(const __time32_t *_Time);
  _CRTIMP size_t __cdecl wcsftime(wchar_t *_Buf,size_t _SizeInWords,const wchar_t *_Format,const struct tm *_Tm);
  _CRTIMP size_t __cdecl _wcsftime_l(wchar_t *_Buf,size_t _SizeInWords,const wchar_t *_Format,const struct tm *_Tm,_locale_t _Locale);
  _CRTIMP wchar_t *__cdecl _wstrdate(wchar_t *_Buffer);
  _CRTIMP wchar_t *__cdecl _wstrtime(wchar_t *_Buffer);

  _CRTIMP errno_t __cdecl _wasctime_s(wchar_t *_Buf,size_t _SizeInWords,const struct tm *_Tm);
  _CRTIMP errno_t __cdecl _wctime32_s(wchar_t *_Buf,size_t _SizeInWords,const __time32_t *_Time);
  _CRTIMP errno_t __cdecl _wstrdate_s(wchar_t *_Buf,size_t _SizeInWords);
  _CRTIMP errno_t __cdecl _wstrtime_s(wchar_t *_Buf,size_t _SizeInWords);
#if _INTEGRAL_MAX_BITS >= 64
  _CRTIMP _CRT_INSECURE_DEPRECATE(_wctime64_s) wchar_t *__cdecl _wctime64(const __time64_t *_Time);
  _CRTIMP errno_t __cdecl _wctime64_s(wchar_t *_Buf,size_t _SizeInWords,const __time64_t *_Time);
#endif

#if !defined (RC_INVOKED) && !defined (_INC_WTIME_INL)
#define _INC_WTIME_INL
#ifdef _USE_32BIT_TIME_T
/* Do it like this to be compatible to msvcrt.dll on 32 bit windows XP and before */
__CRT_INLINE wchar_t *__cdecl _wctime32(const time_t *_Time) { return _wctime(_Time); }
__CRT_INLINE errno_t _wctime32_s(wchar_t *_Buffer, size_t _SizeInWords,const __time32_t *_Time) { return _wctime32_s(_Buffer, _SizeInWords, _Time); }
#else
__CRT_INLINE wchar_t *__cdecl _wctime(const time_t *_Time) { return _wctime64(_Time); }
__CRT_INLINE errno_t _wctime_s(wchar_t *_Buffer, size_t _SizeInWords,const time_t *_Time) { return _wctime64_s(_Buffer, _SizeInWords, _Time); }
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

#ifndef RC_INVOKED
#ifdef _USE_32BIT_TIME_T
#if 0
__CRT_INLINE double __cdecl difftime(time_t _Time1,time_t _Time2) { return _difftime32(_Time1,_Time2); }
__CRT_INLINE char *__cdecl ctime(const time_t *_Time) { return _ctime32(_Time); }
__CRT_INLINE struct tm *__cdecl gmtime(const time_t *_Time) { return _gmtime32(_Time); }
__CRT_INLINE struct tm *__cdecl localtime(const time_t *_Time) { return _localtime32(_Time); }
__CRT_INLINE time_t __cdecl mktime(struct tm *_Tm) { return _mktime32(_Tm); }
__CRT_INLINE time_t __cdecl _mkgmtime(struct tm *_Tm) { return _mkgmtime32(_Tm); }
__CRT_INLINE time_t __cdecl time(time_t *_Time) { return _time32(_Time); }
#endif
#else
__CRT_INLINE double __cdecl difftime(time_t _Time1,time_t _Time2) { return _difftime64(_Time1,_Time2); }
__CRT_INLINE char *__cdecl ctime(const time_t *_Time) { return _ctime64(_Time); }
__CRT_INLINE struct tm *__cdecl gmtime(const time_t *_Time) { return _gmtime64(_Time); }
__CRT_INLINE struct tm *__cdecl localtime(const time_t *_Time) { return _localtime64(_Time); }
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
  void __cdecl tzset(void);
#endif

#ifdef __cplusplus
}
#endif

#pragma pack(pop)

#endif /* End _TIME_H_ */

