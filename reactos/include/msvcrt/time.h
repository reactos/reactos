/* 
 * time.h
 *
 * Date and time functions and types.
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Colin Peters <colin@bird.fu.is.saga-u.ac.jp>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.2 $
 * $Author: ekohl $
 * $Date: 2001/07/06 12:50:47 $
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
#ifndef	_TIME_H_
#define	_TIME_H_

#define __need_wchar_t
#define __need_size_t
#include <msvcrt/stddef.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Number of clock ticks per second. A clock tick is the unit by which
 * processor time is measured and is returned by 'clock'.
 */
#define	CLOCKS_PER_SEC	1000.0
#define	CLK_TICK	CLOCKS_PER_SEC

/*
 * A type for measuring processor time (in clock ticks).
 */
#ifndef _CLOCK_T_
#define _CLOCK_T_
typedef	long	clock_t;
#endif

/*
 * Need a definition of time_t.
 */
#include <msvcrt/sys/types.h>

/*
 * A type for storing the current time and date. This is the number of
 * seconds since midnight Jan 1, 1970.
 * NOTE: Normally this is defined by the above include of sys/types.h
 */
#ifndef _TIME_T_
#define _TIME_T_
typedef	long	time_t;
#endif

/*
 * A structure for storing all kinds of useful information about the
 * current (or another) time.
 */
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
  char *tm_zone;
  int tm_gmtoff;
};



clock_t	clock (void);
time_t	time (time_t* tp);
double	difftime (time_t t2, time_t t1);
time_t	mktime (struct tm* tmsp);

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
char*		asctime (const struct tm* tmsp);
char*		ctime (const time_t* tp);
struct tm*	gmtime (const time_t* tm);
struct tm*	localtime (const time_t* tm);

char*		_strdate(const char *datestr);
wchar_t*	_wstrdate(const wchar_t *datestr);

size_t	strftime (char* caBuffer, size_t sizeMax, const char* szFormat,
		  const struct tm* tpPrint);

size_t	wcsftime (wchar_t* wcaBuffer, size_t sizeMax,
		  const wchar_t* wsFormat, const struct tm* tpPrint);

char*		_strtime(char* buf);
wchar_t*	_wstrtime(wchar_t* buf);

#ifdef	__cplusplus
}
#endif

#endif
