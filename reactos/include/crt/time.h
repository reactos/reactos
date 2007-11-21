/*
 * time.h
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is a part of the mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER within the package.
 *
 * Date and time functions and types.
 *
 */

#ifndef	_TIME_H_
#define	_TIME_H_

/* All the headers include this file. */
#include <_mingw.h>

#define __need_wchar_t
#define __need_size_t
#define __need_NULL
#ifndef RC_INVOKED
#include <stddef.h>
#endif	/* Not RC_INVOKED */

/*
 * Number of clock ticks per second. A clock tick is the unit by which
 * processor time is measured and is returned by 'clock'.
 */
#define	CLOCKS_PER_SEC	((clock_t)1000)
#define	CLK_TCK		CLOCKS_PER_SEC


#ifndef RC_INVOKED

/*
 * A type for storing the current time and date. This is the number of
 * seconds since midnight Jan 1, 1970.
 * NOTE: This is also defined in non-ISO sys/types.h.
 */
#ifndef _TIME_T_DEFINED
typedef	long	time_t;
#define _TIME_T_DEFINED
#endif

#ifndef __STRICT_ANSI__
/* A 64-bit time_t to get to Y3K */
#ifndef _TIME64_T_DEFINED
typedef __int64 __time64_t;
#define _TIME64_T_DEFINED
#endif
#endif
/*
 * A type for measuring processor time (in clock ticks).
 */
#ifndef _CLOCK_T_DEFINED
typedef	long	clock_t;
#define _CLOCK_T_DEFINED
#endif

#ifndef _TM_DEFINED
/*
 * A structure for storing all kinds of useful information about the
 * current (or another) time.
 */
struct tm
{
	int	tm_sec;		/* Seconds: 0-59 (K&R says 0-61?) */
	int	tm_min;		/* Minutes: 0-59 */
	int	tm_hour;	/* Hours since midnight: 0-23 */
	int	tm_mday;	/* Day of the month: 1-31 */
	int	tm_mon;		/* Months *since* january: 0-11 */
	int	tm_year;	/* Years since 1900 */
	int	tm_wday;	/* Days since Sunday (0-6) */
	int	tm_yday;	/* Days since Jan. 1: 0-365 */
	int	tm_isdst;	/* +1 Daylight Savings Time, 0 No DST,
				 * -1 don't know */
};
#define _TM_DEFINED
#endif

#ifdef	__cplusplus
extern "C" {
#endif

_CRTIMP clock_t __cdecl __MINGW_NOTHROW	clock (void);
_CRTIMP time_t __cdecl __MINGW_NOTHROW	time (time_t*);
_CRTIMP double __cdecl __MINGW_NOTHROW	difftime (time_t, time_t);
_CRTIMP time_t __cdecl __MINGW_NOTHROW	mktime (struct tm*);

/*
 * These functions write to and return pointers to static buffers that may
 * be overwritten by other function calls. Yikes!
 *
 * NOTE: localtime, and perhaps the others of the four functions grouped
 * below may return NULL if their argument is not 'acceptable'. Also note
 * that calling asctime with a NULL pointer will produce an Invalid Page
 * Fault and crap out your program. Guess how I know. Hint: stat called on
 * a directory gives 'invalid' times in st_atime etc...
 */
_CRTIMP char* __cdecl __MINGW_NOTHROW		asctime (const struct tm*);
_CRTIMP char* __cdecl __MINGW_NOTHROW		ctime (const time_t*);
_CRTIMP struct tm*  __cdecl __MINGW_NOTHROW	gmtime (const time_t*);
_CRTIMP struct tm*  __cdecl __MINGW_NOTHROW	localtime (const time_t*);

_CRTIMP size_t __cdecl __MINGW_NOTHROW		strftime (char*, size_t, const char*, const struct tm*);

#ifndef __STRICT_ANSI__

extern _CRTIMP void __cdecl __MINGW_NOTHROW	_tzset (void);

#ifndef _NO_OLDNAMES
extern _CRTIMP void __cdecl __MINGW_NOTHROW	tzset (void);
#endif

_CRTIMP char* __cdecl __MINGW_NOTHROW	_strdate(char*);
_CRTIMP char* __cdecl __MINGW_NOTHROW	_strtime(char*);

/* These require newer versions of msvcrt.dll (6.10 or higher). */
#if __MSVCRT_VERSION__ >= 0x0601
_CRTIMP __time64_t __cdecl __MINGW_NOTHROW  _time64( __time64_t*);
_CRTIMP __time64_t __cdecl __MINGW_NOTHROW  _mktime64 (struct tm*);
_CRTIMP char* __cdecl __MINGW_NOTHROW _ctime64 (const __time64_t*);
_CRTIMP struct tm*  __cdecl __MINGW_NOTHROW _gmtime64 (const __time64_t*);
_CRTIMP struct tm*  __cdecl __MINGW_NOTHROW _localtime64 (const __time64_t*);
#endif /* __MSVCRT_VERSION__ >= 0x0601 */

/*
 * _daylight: non zero if daylight savings time is used.
 * _timezone: difference in seconds between GMT and local time.
 * _tzname: standard/daylight savings time zone names (an array with two
 *          elements).
 */
#ifdef __MSVCRT__

/* These are for compatibility with pre-VC 5.0 suppied MSVCRT. */
extern _CRTIMP int* __cdecl __MINGW_NOTHROW	__p__daylight (void);
extern _CRTIMP long* __cdecl __MINGW_NOTHROW	__p__timezone (void);
extern _CRTIMP char** __cdecl __MINGW_NOTHROW	__p__tzname (void);

__MINGW_IMPORT int	_daylight;
__MINGW_IMPORT long	_timezone;
__MINGW_IMPORT char 	*_tzname[2];

#else /* not __MSVCRT (ie. crtdll) */

#ifndef __DECLSPEC_SUPPORTED

extern int*	_imp___daylight_dll;
extern long*	_imp___timezone_dll;
extern char**	_imp___tzname;

#define _daylight	(*_imp___daylight_dll)
#define _timezone	(*_imp___timezone_dll)
#define _tzname		(*_imp___tzname)

#else /* __DECLSPEC_SUPPORTED */

__MINGW_IMPORT int	_daylight_dll;
__MINGW_IMPORT long	_timezone_dll;
__MINGW_IMPORT char*	_tzname[2];

#define _daylight	_daylight_dll
#define _timezone	_timezone_dll

#endif /* __DECLSPEC_SUPPORTED */

#endif /* not __MSVCRT__ */

#ifndef _NO_OLDNAMES

#ifdef __MSVCRT__

/* These go in the oldnames import library for MSVCRT. */
__MINGW_IMPORT int	daylight;
__MINGW_IMPORT long	timezone;
__MINGW_IMPORT char 	*tzname[2];

#else /* not __MSVCRT__ */

/* CRTDLL is royally messed up when it comes to these macros.
   TODO: import and alias these via oldnames import library instead
   of macros.  */

#define daylight        _daylight
/* NOTE: timezone not defined as macro because it would conflict with
   struct timezone in sys/time.h.
   Also, tzname used to a be macro, but now it's in moldname. */
__MINGW_IMPORT char 	*tzname[2];

#endif /* not __MSVCRT__ */

#endif	/* Not _NO_OLDNAMES */
#endif	/* Not __STRICT_ANSI__ */

#ifndef _WTIME_DEFINED
/* wide function prototypes, also declared in wchar.h */
#ifndef __STRICT_ANSI__
#ifdef __MSVCRT__
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW	_wasctime(const struct tm*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW	_wctime(const time_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW	_wstrdate(wchar_t*);
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW	_wstrtime(wchar_t*);
#if __MSVCRT_VERSION__ >= 0x0601
_CRTIMP wchar_t* __cdecl __MINGW_NOTHROW	_wctime64 (const __time64_t*);
#endif
#endif /*  __MSVCRT__ */
#endif /* __STRICT_ANSI__ */
_CRTIMP size_t __cdecl __MINGW_NOTHROW		wcsftime (wchar_t*, size_t, const wchar_t*, const struct tm*);
#define _WTIME_DEFINED
#endif /* _WTIME_DEFINED */

#ifdef	__cplusplus
}
#endif

#endif	/* Not RC_INVOKED */

#endif	/* Not _TIME_H_ */


