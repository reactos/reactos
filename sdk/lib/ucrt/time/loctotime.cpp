//
// loctotime.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the loctotime_t() family of functions, which convert a local time to
// the internal time format (one of the time_t types).
//
#include <corecrt_internal_time.h>



// Converts a local time to a time_t value.  Returns the time_t value on success;
// returns a time_t with the value -1 and sets errno on failure.  The 'dstflag'
// is 1 for Daylight Time, 0 for Standard Time, and -1 if not specified.
template <typename TimeType>
static TimeType __cdecl common_loctotime_t(
    int       yr, // Zero-based
    int const mo, // One-based
    int const dy, // One-based
    int const hr,
    int const mn,
    int const sc,
    int const dstflag
    ) throw()
{
    typedef __crt_time_time_t_traits<TimeType> time_traits;

    static TimeType const invalid_time = static_cast<TimeType>(-1);

    // Adjust the absolute year to be an offset from the year 1900:
    yr -= 1900;

    _VALIDATE_RETURN_NOEXC(yr >= _BASE_YEAR && yr <= time_traits::max_year, EINVAL, invalid_time)
    _VALIDATE_RETURN_NOEXC(mo >= 1 && mo <= 12,                             EINVAL, invalid_time)
    _VALIDATE_RETURN_NOEXC(__crt_time_is_day_valid(yr, mo - 1, dy),         EINVAL, invalid_time)
    _VALIDATE_RETURN_NOEXC(hr >= 0 && hr <= 23,                             EINVAL, invalid_time)
    _VALIDATE_RETURN_NOEXC(mn >= 0 && mn <= 59,                             EINVAL, invalid_time)
    _VALIDATE_RETURN_NOEXC(sc >= 0 && sc <= 59,                             EINVAL, invalid_time)

    // Compute the number of elapsed days in the current year:
    int elapsed_days_this_year = dy + _days[mo - 1];
    if (__crt_time_is_leap_year(yr) && mo > 2)
        ++elapsed_days_this_year;

    TimeType const elapsed_years = static_cast<TimeType>(yr) - _BASE_YEAR;
    TimeType const elapsed_leap_years = static_cast<TimeType>(__crt_time_elapsed_leap_years(yr));

    // The number of elapsed days is the number of days in each completed year
    // since the epoch, plus one day per leap year, plus the number of days
    // elapsed so far this year:
    TimeType const elapsed_days = 
        elapsed_years * 365 +
        elapsed_leap_years +
        elapsed_days_this_year;

    TimeType const elapsed_hours   = elapsed_days    * 24 + hr;
    TimeType const elapsed_minutes = elapsed_hours   * 60 + mn;
    TimeType const elapsed_seconds = elapsed_minutes * 60 + sc;

    // Account for the time zone:
    __tzset();

    int  daylight = 0;
    long dstbias  = 0;
    long timezone = 0;
    _ERRCHECK(_get_daylight(&daylight));
    _ERRCHECK(_get_dstbias (&dstbias ));
    _ERRCHECK(_get_timezone(&timezone));

    TimeType const timezone_adjusted_seconds = elapsed_seconds + timezone;

    // Determine whether we are in Daylight Savings Time and adjust:
    TimeType const dst_adjusted_seconds = timezone_adjusted_seconds + dstbias;

    if (dstflag == 1)
        return dst_adjusted_seconds;

    tm tm_value;
    tm_value.tm_yday = elapsed_days_this_year;
    tm_value.tm_year = yr;
    tm_value.tm_mon  = mo - 1;
    tm_value.tm_hour = hr;
    tm_value.tm_min  = mn;
    tm_value.tm_sec  = sc;
    if (dstflag == -1 && daylight != 0 && _isindst(&tm_value))
        return dst_adjusted_seconds;

    // Otherwise, we are not in Daylight Savings Time:
    return timezone_adjusted_seconds;
}

extern "C" __time32_t __cdecl __loctotime32_t(
    int const yr,
    int const mo,
    int const dy,
    int const hr,
    int const mn,
    int const sc,
    int const dstflag
    )
{
    return common_loctotime_t<__time32_t>(yr, mo, dy, hr, mn, sc, dstflag);
}

extern "C" __time64_t __cdecl __loctotime64_t(
    int const yr,
    int const mo,
    int const dy,
    int const hr,
    int const mn,
    int const sc,
    int const dstflag
    )
{
    return common_loctotime_t<__time64_t>(yr, mo, dy, hr, mn, sc, dstflag);
}
