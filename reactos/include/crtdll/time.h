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
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * $Revision: 1.4 $
 * $Author: robd $
 * $Date: 2002/11/24 18:09:56 $
 *
 */
/* Appropriated for Reactos Crtdll by Ariadne */
#ifndef	_TIME_H_
#define	_TIME_H_

#define __need_wchar_t
#define __need_size_t
#include <crtdll/stddef.h>


/*
 * Number of clock ticks per second. A clock tick is the unit by which
 * processor time is measured and is returned by 'clock'.
 */
#define	CLOCKS_PER_SEC	1000.0
#define	CLK_TICK	CLOCKS_PER_SEC


/*
 * Need a definition of time_t.
 */
#include <crtdll/sys/types.h>

/*
 * A type for storing the current time and date. This is the number of
 * seconds since midnight Jan 1, 1970.
 * NOTE: Normally this is defined by the above include of sys/types.h
 */
#ifndef _TIME_T_
typedef	long	time_t;
#define _TIME_T_
#endif

/*
 * A type for measuring processor time (in clock ticks).
 */
#ifndef _CLOCK_T_
typedef	long	clock_t;
#define _CLOCK_T_
#endif

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
    char *tm_zone;
    int tm_gmtoff;
};

#ifdef	__cplusplus
extern "C" {
#endif

clock_t	clock (void);
time_t	time (time_t*);
double	difftime (time_t, time_t);
time_t	mktime (struct tm*);

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
char*		asctime (const struct tm*);
char*		ctime (const time_t*);
struct tm*	gmtime (const time_t*);
struct tm*	localtime (const time_t*);


size_t	strftime (char*, size_t, const char*, const struct tm*);

size_t	wcsftime (wchar_t*, size_t, const wchar_t*, const struct tm*);

#ifdef	__cplusplus
}
#endif

#endif	/* Not _TIME_H_ */

