/***
*time.h - definitions/declarations for time routines
*
*   Copyright (c) 1985-1992, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file contains the various declarations and definitions
*   for the time routines.
*   [ANSI/System V]
*
****/

#ifndef _INC_TIME

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#define __near      _near
#define __pascal    _pascal
#endif 

/* implementation defined time types */

#ifndef _TIME_T_DEFINED
typedef long    time_t;
#define _TIME_T_DEFINED
#endif 

#ifndef _CLOCK_T_DEFINED
typedef long clock_t;
#define _CLOCK_T_DEFINED
#endif 

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;
#define _SIZE_T_DEFINED
#endif 

/* structure for use with localtime(), gmtime(), etc. */

#ifndef _TM_DEFINED
struct tm {
    int tm_sec; /* seconds after the minute - [0,59] */
    int tm_min; /* minutes after the hour - [0,59] */
    int tm_hour;    /* hours since midnight - [0,23] */
    int tm_mday;    /* day of the month - [1,31] */
    int tm_mon; /* months since January - [0,11] */
    int tm_year;    /* years since 1900 */
    int tm_wday;    /* days since Sunday - [0,6] */
    int tm_yday;    /* days since January 1 - [0,365] */
    int tm_isdst;   /* daylight savings time flag */
    };
#define _TM_DEFINED
#endif 


/* define NULL pointer value */

#ifndef NULL
#ifdef __cplusplus
#define NULL    0
#else 
#define NULL    ((void *)0)
#endif 
#endif 


/* clock ticks macro - ANSI version */

#define CLOCKS_PER_SEC  1000


/* extern declarations for the global variables used by the ctime family of
 * routines.
 */

extern int __near __cdecl _daylight;    /* non-zero if daylight savings time is used */
extern long __near __cdecl _timezone;   /* difference in seconds between GMT and local time */
extern char * __near __cdecl _tzname[2];/* standard/daylight savings time zone names */


/* function prototypes */

#ifdef _MT
double __pascal difftime(time_t, time_t);
#else 
double __cdecl difftime(time_t, time_t);
#endif 

char * __cdecl asctime(const struct tm *);
char * __cdecl ctime(const time_t *);
#ifndef _WINDLL
clock_t __cdecl clock(void);
#endif 
struct tm * __cdecl gmtime(const time_t *);
struct tm * __cdecl localtime(const time_t *);
time_t __cdecl mktime(struct tm *);
#ifndef _WINDLL
size_t __cdecl strftime(char *, size_t, const char *,
    const struct tm *);
#endif 
char * __cdecl _strdate(char *);
char * __cdecl _strtime(char *);
time_t __cdecl time(time_t *);
void __cdecl _tzset(void);

#ifndef __STDC__
/* Non-ANSI names for compatibility */

#define CLK_TCK  CLOCKS_PER_SEC

extern int __near __cdecl daylight;
extern long __near __cdecl timezone;
extern char * __near __cdecl tzname[2];

void __cdecl tzset(void);

#endif 

#ifdef __cplusplus
}
#endif 

#define _INC_TIME
#endif 
