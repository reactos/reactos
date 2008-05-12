/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#ifndef __dj_include_tzfile_h__
#define __dj_include_tzfile_h__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __dj_ENFORCE_ANSI_FREESTANDING

#ifndef __STRICT_ANSI__

#ifndef _POSIX_SOURCE

/*
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Arthur David Olson of the National Cancer Institute.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *      @(#)tzfile.h    5.9 (Berkeley) 6/11/90
 */

/*
** Information about time zone files.
*/

                        /* Time zone object file directory */
#define TZDIR           "/usr/share/zoneinfo"
#define TZDEFAULT       "/etc/localtime"
#define TZDEFRULES      "posixrules"

/*
** Each file begins with. . .
*/

struct tzhead {
        char    tzh_reserved[24];       /* reserved for future use */
        char    tzh_ttisstdcnt[4];      /* coded number of trans. time flags */
        char    tzh_leapcnt[4];         /* coded number of leap seconds */
        char    tzh_timecnt[4];         /* coded number of transition times */
        char    tzh_typecnt[4];         /* coded number of local time types */
        char    tzh_charcnt[4];         /* coded number of abbr. chars */
};

/*
** . . .followed by. . .
**
**      tzh_timecnt (char [4])s         coded transition times a la time(2)
**      tzh_timecnt (unsigned char)s    types of local time starting at above
**      tzh_typecnt repetitions of
**              one (char [4])          coded GMT offset in seconds
**              one (unsigned char)     used to set tm_isdst
**              one (unsigned char)     that's an abbreviation list index
**      tzh_charcnt (char)s             '\0'-terminated zone abbreviations
**      tzh_leapcnt repetitions of
**              one (char [4])          coded leap second transition times
**              one (char [4])          total correction after above
**      tzh_ttisstdcnt (char)s          indexed by type; if TRUE, transition
**                                      time is standard time, if FALSE,
**                                      transition time is wall clock time
**                                      if absent, transition times are
**                                      assumed to be wall clock time
*/

/*
** In the current implementation, "tzset()" refuses to deal with files that
** exceed any of the limits below.
*/

/*
** The TZ_MAX_TIMES value below is enough to handle a bit more than a
** year's worth of solar time (corrected daily to the nearest second) or
** 138 years of Pacific Presidential Election time
** (where there are three time zone transitions every fourth year).
*/
#define TZ_MAX_TIMES    370

#define NOSOLAR                 /* 4BSD doesn't currently handle solar time */

#ifndef NOSOLAR
#define TZ_MAX_TYPES    256     /* Limited by what (unsigned char)'s can hold */
#else
#define TZ_MAX_TYPES    10      /* Maximum number of local time types */
#endif

#define TZ_MAX_CHARS    50      /* Maximum number of abbreviation characters */

#define TZ_MAX_LEAPS    50      /* Maximum number of leap second corrections */

#define SECSPERMIN      60
#define MINSPERHOUR     60
#define HOURSPERDAY     24
#define DAYSPERWEEK     7
#define DAYSPERNYEAR    365
#define DAYSPERLYEAR    366
#define SECSPERHOUR     (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY      ((long) SECSPERHOUR * HOURSPERDAY)
#define MONSPERYEAR     12

#define TM_SUNDAY       0
#define TM_MONDAY       1
#define TM_TUESDAY      2
#define TM_WEDNESDAY    3
#define TM_THURSDAY     4
#define TM_FRIDAY       5
#define TM_SATURDAY     6

#define TM_JANUARY      0
#define TM_FEBRUARY     1
#define TM_MARCH        2
#define TM_APRIL        3
#define TM_MAY          4
#define TM_JUNE         5
#define TM_JULY         6
#define TM_AUGUST       7
#define TM_SEPTEMBER    8
#define TM_OCTOBER      9
#define TM_NOVEMBER     10
#define TM_DECEMBER     11

#define TM_YEAR_BASE    1900

#define EPOCH_YEAR      1970
#define EPOCH_WDAY      TM_THURSDAY

/*
** Accurate only for the past couple of centuries;
** that will probably do.
*/

#define isleap(y) ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

#endif /* !_POSIX_SOURCE */
#endif /* !__STRICT_ANSI__ */
#endif /* !__dj_ENFORCE_ANSI_FREESTANDING */

#ifndef __dj_ENFORCE_FUNCTION_CALLS
#endif /* !__dj_ENFORCE_FUNCTION_CALLS */

#ifdef __cplusplus
}
#endif

#endif /* __dj_include_tzfile_h__ */
