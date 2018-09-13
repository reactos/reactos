/***
*time.c - get current system time
*
*       Copyright (c) 1989-1996, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       defines time() - gets the current system time and converts it to
*                        internal (time_t) format time.
*
*******************************************************************************/


#include <cruntime.h>
#include <time.h>
#include <internal.h>

#include <windows.h>

/*
 * Cache holding the last time (GMT) for which the Daylight time status was
 * determined by an API call.
 */
static SYSTEMTIME gmt_cache;

/*
 * Three values of dstflag_cache and dstflag (local variable in code
 * below)
 */
#define DAYLIGHT_TIME   1
#define STANDARD_TIME   0
#define UNKNOWN_TIME    -1

// From ctime.h
#define _DAY_SEC           (24L * 60L * 60L)    /* secs in a day */
#define _YEAR_SEC          (365L * _DAY_SEC)    /* secs in a year */
#define _FOUR_YEAR_SEC     (1461L * _DAY_SEC)   /* secs in a 4 year interval */
#define _DEC_SEC           315532800L           /* secs in 1970-1979 */
#define _BASE_YEAR         70L                  /* 1970 is the base year */
#define _BASE_DOW          4                    /* 01-01-70 was a Thursday */
#define _LEAP_YEAR_ADJUST  17L                  /* Leap years 1900 - 1970 */
#define _MAX_YEAR          138L                 /* 2038 is the max year */

static int dstflag_cache;

/*
 * Number of milliseconds in one day
 */
#define DAY_MILLISEC    (24L * 60L * 60L * 1000L)

/*
 * The macro below is valid for years between 1901 and 2099, which easily
 * includes all years representable by the current implementation of time_t.
 */
#define IS_LEAP_YEAR(year)  ( (year & 3) == 0 )

/*
 * Pointer to a saved copy of the TZ value obtained in the previous call
 * to tzset() set (if any).
 */
static char * lastTZ = NULL;

static int tzapiused;

static TIME_ZONE_INFORMATION tzinfo;

/*
 * Structure used to represent DST transition date/times.
 */
typedef struct {
        int  yr;        /* year of interest */
        int  yd;        /* day of year */
        long ms;        /* milli-seconds in the day */
        } transitiondate;

/*
 * DST start and end structs.
 */
static transitiondate dststart = { -1, 0, 0L };
static transitiondate dstend   = { -1, 0, 0L };

// From timeset.c
long _timezone = 8 * 3600L; /* Pacific Time Zone */
int _daylight = 1;          /* Daylight Saving Time (DST) in timezone */
long _dstbias = -3600L;     /* DST offset in seconds */

/* note that NT Posix's TZNAME_MAX is only 10 */

static char tzstd[64] = { "PST" };
static char tzdst[64] = { "PDT" };

char *_tzname[2] = { tzstd, tzdst };


int _lpdays[] = {
        -1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

int _days[] = {
        -1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};

/*  Day names must be Three character abbreviations strung together */

const char __dnames[] = {
        "SunMonTueWedThuFriSat"
};

/*  Month names must be Three character abbreviations strung together */

const char __mnames[] = {
        "JanFebMarAprMayJunJulAugSepOctNovDec"
};

/***
*static void cvtdate( trantype, datetype, year, month, week, dayofweek,
*                     date, hour, min, second, millisec ) - convert
*       transition date format
*
*Purpose:
*       Convert the format of a transition date specification to a value of
*       a transitiondate structure.
*
*Entry:
*       int trantype    - 1, if it is the start of DST
*                         0, if is the end of DST (in which case the date is
*                            is a DST date)
*       int datetype    - 1, if a day-in-month format is specified.
*                         0, if an absolute date is specified.
*       int year        - year for which the date is being converted (70 ==
*                         1970)
*       int month       - month (0 == January)
*       int week        - week of month, if datetype == 1 (note that 5== last
*                         week of month),
*                         0, otherwise.
*       int dayofweek   - day of week (0 == Sunday), if datetype == 1.
*                         0, otherwise.
*       int date        - date of month (1 - 31)
*       int hour        - hours (0 - 23)
*       int min         - minutes (0 - 59)
*       int sec         - seconds (0 - 59)
*       int msec        - milliseconds (0 - 999)
*
*Exit:
*       dststart or dstend is filled in with the converted date.
*
*******************************************************************************/

static void __cdecl cvtdate (
        int trantype,
        int datetype,
        int year,
        int month,
        int week,
        int dayofweek,
        int date,
        int hour,
        int min,
        int sec,
        int msec
        )
{
        int yearday;
        int monthdow;

        if ( datetype == 1 ) {

            /*
             * Transition day specified in day-in-month format.
             */

            /*
             * Figure the year-day of the start of the month.
             */
            yearday = 1 + (IS_LEAP_YEAR(year) ? _lpdays[month - 1] :
                      _days[month - 1]);

            /*
             * Figure the day of the week of the start of the month.
             */
            monthdow = (yearday + ((year - 70) * 365) + ((year - 1) >> 2) -
                        _LEAP_YEAR_ADJUST + _BASE_DOW) % 7;

            /*
             * Figure the year-day of the transition date
             */
            if ( monthdow < dayofweek )
                yearday += (dayofweek - monthdow) + (week - 1) * 7;
            else
                yearday += (dayofweek - monthdow) + week * 7;

            /*
             * May have to adjust the calculation above if week == 5 (meaning
             * the last instance of the day in the month). Check if year falls
             * beyond after month and adjust accordingly.
             */
            if ( (week == 5) &&
                 (yearday > (IS_LEAP_YEAR(year) ? _lpdays[month] :
                             _days[month])) )
            {
                yearday -= 7;
            }
        }
        else {
            /*
             * Transition day specified as an absolute day
             */
            yearday = IS_LEAP_YEAR(year) ? _lpdays[month - 1] :
                      _days[month - 1];

            yearday += date;
        }

        if ( trantype == 1 ) {
            /*
             * Converted date was for the start of DST
             */
            dststart.yd = yearday;
            dststart.ms = (long)msec +
                          (1000L * (sec + 60L * (min + 60L * hour)));
            /*
             * Set year field of dststart so that unnecessary calls to
             * cvtdate() may be avoided.
             */
            dststart.yr = year;
        }
        else {
            /*
             * Converted date was for the end of DST
             */
            dstend.yd = yearday;
            dstend.ms = (long)msec +
                              (1000L * (sec + 60L * (min + 60L * hour)));
            /*
             * The converted date is still a DST date. Must convert to a
             * standard (local) date while being careful the millisecond field
             * does not overflow or underflow.
             */
            if ( (dstend.ms += (_dstbias * 1000L)) < 0 ) {
                dstend.ms += DAY_MILLISEC;
                dstend.ms--;
            }
            else if ( dstend.ms >= DAY_MILLISEC ) {
                dstend.ms -= DAY_MILLISEC;
                dstend.ms++;
            }

            /*
             * Set year field of dstend so that unnecessary calls to cvtdate()
             * may be avoided.
             */
            dstend.yr = year;
        }

        return;
}

/***
*int _isindst(tb) - determine if broken-down time falls in DST
*
*Purpose:
*       Determine if the given broken-down time falls within daylight saving
*       time (DST). The DST rules are either obtained from Win32 (tzapiused !=
*       TRUE) or assumed to be USA rules, post 1986.
*
*       If the DST rules are obtained from Win32's GetTimeZoneInformation API,
*       the transition dates to/from DST can be specified in either of two
*       formats. First, a day-in-month format, similar to the way USA rules
*       are specified, can be used. The transition date is given as the n-th
*       occurence of a specified day of the week in a specified month. Second,
*       an absolute date can be specified. The two cases are distinguished by
*       the value of wYear field in the SYSTEMTIME structure (0 denotes a
*       day-in-month format).
*
*       USA rules for DST are that a time is in DST iff it is on or after
*       02:00 on the first Sunday in April, and before 01:00 on the last
*       Sunday in October.
*
*Entry:
*       struct tm *tb - structure holding broken-down time value
*
*Exit:
*       1, if time represented is in DST
*       0, otherwise
*
*******************************************************************************/

int __cdecl _isindst (
        struct tm *tb
        )
#ifdef _MTTIME
{
        int retval;

        _mlock( _TIME_LOCK );
        retval = _isindst_lk( tb );
        _munlock( _TIME_LOCK );

        return retval;
}

static int __cdecl _isindst_lk (
        struct tm *tb
        )
#endif  /* _MTTIME */
{
        long ms;

        if ( _daylight == 0 )
            return 0;

        /*
         * Compute (recompute) the transition dates for daylight saving time
         * if necessary.The yr (year) fields of dststart and dstend is
         * compared to the year of interest to determine necessity.
         */
        if ( (tb->tm_year != dststart.yr) || (tb->tm_year != dstend.yr) ) {
            if ( tzapiused ) {
                /*
                 * Convert the start of daylight saving time to dststart.
                 */
                if ( tzinfo.DaylightDate.wYear == 0 )
                    cvtdate( 1,
                             1,             /* day-in-month format */
                             tb->tm_year,
                             tzinfo.DaylightDate.wMonth,
                             tzinfo.DaylightDate.wDay,
                             tzinfo.DaylightDate.wDayOfWeek,
                             0,
                             tzinfo.DaylightDate.wHour,
                             tzinfo.DaylightDate.wMinute,
                             tzinfo.DaylightDate.wSecond,
                             tzinfo.DaylightDate.wMilliseconds );
                else
                    cvtdate( 1,
                             0,             /* absolute date */
                             tb->tm_year,
                             tzinfo.DaylightDate.wMonth,
                             0,
                             0,
                             tzinfo.DaylightDate.wDay,
                             tzinfo.DaylightDate.wHour,
                             tzinfo.DaylightDate.wMinute,
                             tzinfo.DaylightDate.wSecond,
                             tzinfo.DaylightDate.wMilliseconds );
                /*
                 * Convert start of standard time to dstend.
                 */
                if ( tzinfo.StandardDate.wYear == 0 )
                    cvtdate( 0,
                             1,             /* day-in-month format */
                             tb->tm_year,
                             tzinfo.StandardDate.wMonth,
                             tzinfo.StandardDate.wDay,
                             tzinfo.StandardDate.wDayOfWeek,
                             0,
                             tzinfo.StandardDate.wHour,
                             tzinfo.StandardDate.wMinute,
                             tzinfo.StandardDate.wSecond,
                             tzinfo.StandardDate.wMilliseconds );
                else
                    cvtdate( 0,
                             0,             /* absolute date */
                             tb->tm_year,
                             tzinfo.StandardDate.wMonth,
                             0,
                             0,
                             tzinfo.StandardDate.wDay,
                             tzinfo.StandardDate.wHour,
                             tzinfo.StandardDate.wMinute,
                             tzinfo.StandardDate.wSecond,
                             tzinfo.StandardDate.wMilliseconds );

            }
            else {
                /*
                 * GetTimeZoneInformation API was NOT used, or failed. USA
                 * daylight saving time rules are assumed.
                 */
                cvtdate( 1,
                         1,
                         tb->tm_year,
                         4,                 /* April */
                         1,                 /* first... */
                         0,                 /* ...Sunday */
                         0,
                         2,                 /* 02:00 (2 AM) */
                         0,
                         0,
                         0 );

                cvtdate( 0,
                         1,
                         tb->tm_year,
                         10,                /* October */
                         5,                 /* last... */
                         0,                 /* ...Sunday */
                         0,
                         2,                 /* 02:00 (2 AM) */
                         0,
                         0,
                         0 );
            }
        }

        /*
         * Handle simple cases first.
         */
        if ( dststart.yd < dstend.yd ) {
            /*
             * Northern hemisphere ordering
             */
            if ( (tb->tm_yday < dststart.yd) || (tb->tm_yday > dstend.yd) )
                return 0;
            if ( (tb->tm_yday > dststart.yd) && (tb->tm_yday < dstend.yd) )
                return 1;
        }
        else {
            /*
             * Southern hemisphere ordering
             */
            if ( (tb->tm_yday < dstend.yd) || (tb->tm_yday > dststart.yd) )
                return 1;
            if ( (tb->tm_yday > dstend.yd) && (tb->tm_yday < dststart.yd) )
                return 0;
        }

        ms = 1000L * (tb->tm_sec + 60L * tb->tm_min + 3600L * tb->tm_hour);

        if ( tb->tm_yday == dststart.yd ) {
            if ( ms >= dststart.ms )
                return 1;
            else
                return 0;
        }
        else {
            /*
             * tb->tm_yday == dstend.yd
             */
            if ( ms < dstend.ms )
                return 1;
            else
                return 0;
        }

}

/***
*void tzset() - sets timezone information and calc if in daylight time
*
*Purpose:
*       Sets the timezone information from the TZ environment variable
*       and then sets _timezone, _daylight, and _tzname. If we're in daylight
*       time is automatically calculated.
*
*Entry:
*       None, reads TZ environment variable.
*
*Exit:
*       sets _daylight, _timezone, and _tzname global vars, no return value
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __tzset (void)
{
    if ( GetTimeZoneInformation( &tzinfo ) != 0xFFFFFFFF ) {
        /*
         * Note that the API was used.
         */
        tzapiused = 1;

        /*
         * Derive _timezone value from Bias and StandardBias fields.
         */
        _timezone = tzinfo.Bias * 60L;

        if ( tzinfo.StandardDate.wMonth != 0 )
            _timezone += (tzinfo.StandardBias * 60L);

        /*
         * Check to see if there is a daylight time bias. Since the
         * StandardBias has been added into _timezone, it must be
         * compensated for in the value computed for _dstbias.
         */
        if ( (tzinfo.DaylightDate.wMonth != 0) &&
             (tzinfo.DaylightBias != 0) )
        {
            _daylight = 1;
            _dstbias = (tzinfo.DaylightBias - tzinfo.StandardBias) *
                       60L;
        }
        else {
                _daylight = 0;

            /*
             * Set daylight bias to 0 because GetTimeZoneInformation
             * may return TIME_ZONE_ID_DAYLIGHT even though there is
             * no DST (in NT 3.51, just turn off the automatic DST
             * adjust in the control panel)!
             */
            _dstbias = 0;
        }
    }
}

/***
*time_t __loctotime_t(yr, mo, dy, hr, mn, sc, dstflag) - converts OS local
*       time to internal time format (i.e., a time_t value)
*
*Purpose:
*       Converts a local time value, obtained in a broken down format from
*       the host OS, to time_t format (i.e., the number elapsed seconds since
*       01-01-70, 00:00:00, UTC).
*
*Entry:
*       int yr, mo, dy -    date
*       int hr, mn, sc -    time
*       int dstflag    -    1 if Daylight Time, 0 if Standard Time, -1 if
*                           not specified.
*
*Exit:
*       Returns calendar time value.
*
*Exceptions:
*
*******************************************************************************/

time_t __cdecl __loctotime_t (
        int yr,         /* 0 based */
        int mo,         /* 1 based */
        int dy,         /* 1 based */
        int hr,
        int mn,
        int sc,
        int dstflag )
{
        int tmpdays;
        long tmptim;
        struct tm tb;

        /*
         * Do a quick range check on the year and convert it to a delta
         * off of 1900.
         */
        if ( ((long)(yr -= 1900) < _BASE_YEAR) || ((long)yr > _MAX_YEAR) )
                return (time_t)(-1);

        /*
         * Compute the number of elapsed days in the current year. Note the
         * test for a leap year would fail in the year 2100, if this was in
         * range (which it isn't).
         */
        tmpdays = dy + _days[mo - 1];
        if ( !(yr & 3) && (mo > 2) )
                tmpdays++;

        /*
         * Compute the number of elapsed seconds since the Epoch. Note the
         * computation of elapsed leap years would break down after 2100
         * if such values were in range (fortunately, they aren't).
         */
        tmptim = /* 365 days for each year */
                 (((long)yr - _BASE_YEAR) * 365L

                 /* one day for each elapsed leap year */
                 + (long)((yr - 1) >> 2) - _LEAP_YEAR_ADJUST

                 /* number of elapsed days in yr */
                 + tmpdays)

                 /* convert to hours and add in hr */
                 * 24L + hr;

        tmptim = /* convert to minutes and add in mn */
                 (tmptim * 60L + mn)

                 /* convert to seconds and add in sec */
                 * 60L + sc;
        /*
         * Account for time zone.
         */
        __tzset();
        tmptim += _timezone;

        /*
         * Fill in enough fields of tb for _isindst(), then call it to
         * determine DST.
         */
        tb.tm_yday = tmpdays;
        tb.tm_year = yr;
        tb.tm_mon  = mo - 1;
        tb.tm_hour = hr;
        if ( (dstflag == 1) || ((dstflag == -1) && _daylight &&
                                _isindst(&tb)) )
                tmptim += _dstbias;
        return(tmptim);
}

/***
*time_t time(timeptr) - Get current system time and convert to time_t value.
*
*Purpose:
*       Gets the current date and time and stores it in internal (time_t)
*       format. The time is returned and stored via the pointer passed in
*       timeptr. If timeptr == NULL, the time is only returned, not stored in
*       *timeptr. The internal (time_t) format is the number of seconds since
*       00:00:00, Jan 1 1970 (UTC).
*
*       Note: We cannot use GetSystemTime since its return is ambiguous. In
*       Windows NT, in return UTC. In Win32S, probably also Win32C, it
*       returns local time.
*
*Entry:
*       time_t *timeptr - pointer to long to store time in.
*
*Exit:
*       returns the current time.
*
*Exceptions:
*
*******************************************************************************/

time_t __cdecl time (
        time_t *timeptr
        )
{
        time_t tim;

#ifdef _WIN32

        SYSTEMTIME loct, gmt;
        TIME_ZONE_INFORMATION tzinfo;
        DWORD tzstate;
        int dstflag;

        /*
         * Get local time from Win32
         */
        GetLocalTime( &loct );

        /*
         * Determine whether or not the local time is a Daylight Saving
         * Time. On Windows NT, the GetTimeZoneInformation API is *VERY*
         * expensive. The scheme below is intended to avoid this API call in
         * many important case by caching the GMT value and dstflag.In a
         * subsequent call to time(), the cached value of dstflag is used
         * unless the new GMT differs from the cached value at least in the
         * minutes place.
         */
        GetSystemTime( &gmt );

        if ( (gmt.wMinute == gmt_cache.wMinute) &&
             (gmt.wHour == gmt_cache.wHour) &&
             (gmt.wDay == gmt_cache.wDay) &&
             (gmt.wMonth == gmt_cache.wMonth) &&
             (gmt.wYear == gmt_cache.wYear) )
        {
            dstflag = dstflag_cache;
        }
        else
        {
            if ( (tzstate = GetTimeZoneInformation( &tzinfo )) != 0xFFFFFFFF )
            {
                /*
                 * Must be very careful in determining whether or not DST is
                 * really in effect.
                 */
                if ( (tzstate == TIME_ZONE_ID_DAYLIGHT) &&
                     (tzinfo.DaylightDate.wMonth != 0) &&
                     (tzinfo.DaylightBias != 0) )
                    dstflag = DAYLIGHT_TIME;
                else
                    /*
                     * When in doubt, assume standard time
                     */
                    dstflag = STANDARD_TIME;
            }
            else
                dstflag = UNKNOWN_TIME;

            dstflag_cache = dstflag;
            gmt_cache = gmt;
        }

        /* convert using our private routine */

        tim = __loctotime_t( (int)loct.wYear,
                             (int)loct.wMonth,
                             (int)loct.wDay,
                             (int)loct.wHour,
                             (int)loct.wMinute,
                             (int)loct.wSecond,
                             dstflag );

#else  /* _WIN32 */
#if defined (_M_MPPC) || defined (_M_M68K)

        DateTimeRec dt;

        GetTime(&dt);
        /* convert using our private routine */
        tim = _gmtotime_t((int)dt.year,
                          (int)dt.month,
                          (int)dt.day,
                          (int)dt.hour,
                          dt.minute,
                          dt.second);

#endif  /* defined (_M_MPPC) || defined (_M_M68K) */
#endif  /* _WIN32 */

        if (timeptr)
                *timeptr = tim;         /* store time if requested */

        return tim;
}

