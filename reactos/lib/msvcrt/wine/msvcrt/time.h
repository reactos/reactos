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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_TIME_H
#define __WINE_TIME_H
#ifndef __WINE_USE_MSVCRT
#define __WINE_USE_MSVCRT
#endif

#ifndef MSVCRT
# ifdef USE_MSVCRT_PREFIX
#  define MSVCRT(x)    MSVCRT_##x
# else
#  define MSVCRT(x)    x
# endif
#endif

#ifndef MSVCRT_WCHAR_T_DEFINED
#define MSVCRT_WCHAR_T_DEFINED
#ifndef __cplusplus
typedef unsigned short MSVCRT(wchar_t);
#endif
#endif

#ifndef MSVCRT_SIZE_T_DEFINED
typedef unsigned int MSVCRT(size_t);
#define MSVCRT_SIZE_T_DEFINED
#endif

#ifndef MSVCRT_TIME_T_DEFINED
typedef long MSVCRT(time_t);
#define MSVCRT_TIME_T_DEFINED
#endif

#ifndef MSVCRT_CLOCK_T_DEFINED
typedef long MSVCRT(clock_t);
#define MSVCRT_CLOCK_T_DEFINED
#endif

#ifndef NULL
#ifdef __cplusplus
#define NULL  0
#else
#define NULL  ((void *)0)
#endif
#endif

#ifndef CLOCKS_PER_SEC
#define CLOCKS_PER_SEC 1000
#endif

#ifndef MSVCRT_TM_DEFINED
#define MSVCRT_TM_DEFINED
struct MSVCRT(tm) {
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
#endif /* MSVCRT_TM_DEFINED */

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME: Must do something for _daylight, _dstbias, _timezone, _tzname */


unsigned    _getsystime(struct MSVCRT(tm)*);
unsigned    _setsystime(struct MSVCRT(tm)*,unsigned);
char*       _strdate(char*);
char*       _strtime(char*);
void        _tzset(void);

char*       MSVCRT(asctime)(const struct MSVCRT(tm)*);
MSVCRT(clock_t) MSVCRT(clock)(void);
char*       MSVCRT(ctime)(const MSVCRT(time_t)*);
double      MSVCRT(difftime)(MSVCRT(time_t),MSVCRT(time_t));
struct MSVCRT(tm)* MSVCRT(gmtime)(const MSVCRT(time_t)*);
struct MSVCRT(tm)* MSVCRT(localtime)(const MSVCRT(time_t)*);
MSVCRT(time_t) MSVCRT(mktime)(struct MSVCRT(tm)*);
size_t      MSVCRT(strftime)(char*,size_t,const char*,const struct MSVCRT(tm)*);
MSVCRT(time_t) MSVCRT(time)(MSVCRT(time_t)*);

#ifndef MSVCRT_WTIME_DEFINED
#define MSVCRT_WTIME_DEFINED
MSVCRT(wchar_t)*_wasctime(const struct MSVCRT(tm)*);
MSVCRT(size_t)  wcsftime(MSVCRT(wchar_t)*,MSVCRT(size_t),const MSVCRT(wchar_t)*,const struct MSVCRT(tm)*);
MSVCRT(wchar_t)*_wctime(const MSVCRT(time_t)*);
MSVCRT(wchar_t)*_wstrdate(MSVCRT(wchar_t)*);
MSVCRT(wchar_t)*_wstrtime(MSVCRT(wchar_t)*);
#endif /* MSVCRT_WTIME_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* __WINE_TIME_H */
