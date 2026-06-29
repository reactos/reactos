/*
 * Time definitions
 *
 * Copyright 2000 Francois Gouget.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#ifndef __WINE_TIME_H
#define __WINE_TIME_H

#include <corecrt_wtime.h>

#include <pshpack8.h>

#ifndef _CLOCK_T_DEFINED
typedef __msvcrt_long clock_t;
#define _CLOCK_T_DEFINED
#endif

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct _timespec32
{
    __time32_t tv_sec;
    __msvcrt_long tv_nsec;
};

struct _timespec64
{
    __time64_t tv_sec;
    __msvcrt_long tv_nsec;
};

struct timespec
{
    time_t tv_sec;
    __msvcrt_long tv_nsec;
};

#ifdef __i386__
#define _daylight (*__p__daylight())
#define _dstbias (*__p__dstbias())
#define _timezone (*__p__timezone())
#define _tzname (__p__tzname())

_ACRTIMP int *   __cdecl __p__daylight(void);
_ACRTIMP __msvcrt_long *  __cdecl __p__dstbias(void);
_ACRTIMP __msvcrt_long *  __cdecl __p__timezone(void);
_ACRTIMP char ** __cdecl __p__tzname(void);
#else
extern int _daylight;
extern __msvcrt_long _dstbias;
extern __msvcrt_long _timezone;
extern char *_tzname;
#endif

#if !defined(_UCRT) && defined(_USE_32BIT_TIME_T)
#define _ctime32     ctime
#define _difftime32  difftime
#define _gmtime32    gmtime
#define _localtime32 localtime
#define _mktime32    mktime
#define _time32      time
#endif

_ACRTIMP unsigned    __cdecl _getsystime(struct tm*);
_ACRTIMP unsigned    __cdecl _setsystime(struct tm*,unsigned);
_ACRTIMP char*       __cdecl _strdate(char*);
_ACRTIMP errno_t     __cdecl _strdate_s(char*,size_t);
_ACRTIMP char*       __cdecl _strtime(char*);
_ACRTIMP errno_t     __cdecl _strtime_s(char*,size_t);
_ACRTIMP void        __cdecl _tzset(void);

_ACRTIMP char*       __cdecl asctime(const struct tm*);
_ACRTIMP clock_t     __cdecl clock(void);
_ACRTIMP char*       __cdecl _ctime32(const __time32_t*);
_ACRTIMP errno_t     __cdecl _ctime32_s(char*,size_t,const __time32_t*);
_ACRTIMP char*       __cdecl _ctime64(const __time64_t*);
_ACRTIMP errno_t     __cdecl _ctime64_s(char*,size_t,const __time64_t*);
_ACRTIMP double      __cdecl _difftime32(__time32_t,__time32_t);
_ACRTIMP double      __cdecl _difftime64(__time64_t,__time64_t);
_ACRTIMP struct tm*  __cdecl _gmtime32(const __time32_t*);
_ACRTIMP int         __cdecl _gmtime32_s(struct tm *res, const __time32_t *secs);
_ACRTIMP struct tm*  __cdecl _gmtime64(const __time64_t*);
_ACRTIMP int         __cdecl _gmtime64_s(struct tm *res, const __time64_t *secs);

_ACRTIMP struct tm*  __cdecl _localtime32(const __time32_t*);
_ACRTIMP errno_t     __cdecl _localtime32_s(struct tm*, const __time32_t*);
_ACRTIMP struct tm*  __cdecl _localtime64(const __time64_t*);
_ACRTIMP errno_t     __cdecl _localtime64_s(struct tm*, const __time64_t*);
_ACRTIMP __time32_t  __cdecl _mktime32(struct tm*);
_ACRTIMP __time64_t  __cdecl _mktime64(struct tm*);
_ACRTIMP size_t      __cdecl strftime(char*,size_t,const char*,const struct tm*);
_ACRTIMP size_t      __cdecl _strftime_l(char*,size_t,const char*,const struct tm*,_locale_t);
_ACRTIMP __time32_t  __cdecl _time32(__time32_t*);
_ACRTIMP __time64_t  __cdecl _time64(__time64_t*);

#ifndef _USE_32BIT_TIME_T
static inline char* ctime(const time_t *t) { return _ctime64(t); }
static inline errno_t ctime_s(char *res, size_t len, const __time64_t *t) { return _ctime64_s(res, len, t); }
static inline double difftime(time_t t1, time_t t2) { return _difftime64(t1, t2); }
static inline struct tm* gmtime(const time_t *t) { return _gmtime64(t); }
static inline struct tm* localtime(const time_t *t) { return _localtime64(t); }
static inline errno_t localtime_s(struct tm *res, const time_t *t) { return _localtime64_s(res, t); }
static inline time_t mktime(struct tm *tm) { return _mktime64(tm); }
static inline time_t time(time_t *t) { return _time64(t); }
#elif defined(_UCRT)
static inline char* ctime(const time_t *t) { return _ctime32(t); }
static inline errno_t ctime_s(char *res, size_t len, const __time32_t *t) { return _ctime32_s(res, len, t); }
static inline double difftime(time_t t1, time_t t2) { return _difftime32(t1, t2); }
static inline struct tm* gmtime(const time_t *t) { return _gmtime32(t); }
static inline struct tm* localtime(const time_t *t) { return _localtime32(t); }
static inline errno_t localtime_s(struct tm *res, const time_t *t) { return _localtime32_s(res, t); }
static inline time_t mktime(struct tm *tm) { return _mktime32(tm); }
static inline time_t time(time_t *t) { return _time32(t); }
#endif

#ifdef __cplusplus
}
#endif

#include <poppack.h>

#endif /* __WINE_TIME_H */
