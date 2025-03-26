//
// mktime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The mktime and mkgmtime families of functions, which convert a time value in
// a (possibly incomplete) tm structure into a time_t value, then update all of
// the tm structure fields with the "normalized" values.
//
#include <corecrt_internal_time.h>


// ChkAdd evaluates to TRUE if dest = src1 + src2 has overflowed
#define ChkAdd(dest, src1, src2)  \
    (((src1 >= 0L) && (src2 >= 0L) && (dest <  0L)) || \
     ((src1 <  0L) && (src2 <  0L) && (dest >= 0L)))

// ChkMul evaluates to TRUE if dest = src1 * src2 has overflowed
#define ChkMul(dest, src1, src2) ( src1 ? (dest / src1 != src2) : 0 )



// The implementation of the _mktime and _mkgmtime functions.  If 'use_local_time'
// is true, the time is assumed to be in local time; otherwise, the time is
// assumed to be in UTC.
template <typename TimeType>
static TimeType __cdecl common_mktime(
    tm*  const tb,
    bool const use_local_time
    ) throw()
{
    typedef __crt_time_time_t_traits<TimeType> time_traits;

    TimeType const invalid_time = static_cast<TimeType>(-1);

    _VALIDATE_RETURN(tb != nullptr, EINVAL, invalid_time)

    TimeType tmptm1, tmptm2, tmptm3;

    // First, make sure tm_year is reasonably close to being in range.
    if ((tmptm1 = tb->tm_year) < _BASE_YEAR - 1 || tmptm1 > time_traits::max_year + 1)
        return (errno = EINVAL), invalid_time;

    // Adjust month value so it is in the range 0 - 11.  This is because
    // we don't know how many days are in months 12, 13, 14, etc.
    if (tb->tm_mon < 0 || tb->tm_mon > 11)
    {
        // No danger of overflow because the range check above.
        tmptm1 += (tb->tm_mon / 12);

        if ((tb->tm_mon %= 12) < 0)
        {
            tb->tm_mon += 12;
            --tmptm1;
        }

        // Make sure year count is still in range.
        if (tmptm1 < _BASE_YEAR - 1 || tmptm1 > time_traits::max_year + 1)
            return (errno = EINVAL), invalid_time;
    }

    // HERE: tmptm1 holds number of elapsed years

    // Calculate days elapsed minus one, in the given year, to the given
    // month. Check for leap year and adjust if necessary.
    tmptm2 = _days[tb->tm_mon];
    if (__crt_time_is_leap_year(tmptm1) && tb->tm_mon > 1)
        ++tmptm2;

    // Calculate elapsed days since base date (midnight, 1/1/70, UTC)
    //
    // 365 days for each elapsed year since 1970, plus one more day for
    // each elapsed leap year. no danger of overflow because of the range
    // check (above) on tmptm1.
    tmptm3 = (tmptm1 - _BASE_YEAR) * 365 + __crt_time_elapsed_leap_years(tmptm1);

    // Elapsed days to current month (still no possible overflow)
    tmptm3 += tmptm2;

    // Elapsed days to current date. overflow is now possible.
    tmptm1 = tmptm3 + (tmptm2 = static_cast<TimeType>(tb->tm_mday));
    _VALIDATE_RETURN_NOEXC(!ChkAdd(tmptm1, tmptm3, tmptm2), EINVAL, invalid_time)

    // HERE: tmptm1 holds number of elapsed days

    // Calculate elapsed hours since base date
    tmptm2 = tmptm1 * 24;
    _VALIDATE_RETURN_NOEXC(!ChkMul(tmptm2, tmptm1, 24), EINVAL, invalid_time)


    tmptm1 = tmptm2 + (tmptm3 = static_cast<TimeType>(tb->tm_hour));
    _VALIDATE_RETURN_NOEXC(!ChkAdd(tmptm1, tmptm2, tmptm3), EINVAL, invalid_time)


    // HERE: tmptm1 holds number of elapsed hours

    // Calculate elapsed minutes since base date
    tmptm2 = tmptm1 * 60;
    _VALIDATE_RETURN_NOEXC(!ChkMul(tmptm2, tmptm1, 60), EINVAL, invalid_time)


    tmptm1 = tmptm2 + (tmptm3 = static_cast<TimeType>(tb->tm_min));
    _VALIDATE_RETURN_NOEXC(!ChkAdd(tmptm1, tmptm2, tmptm3), EINVAL, invalid_time)


    // HERE: tmptm1 holds number of elapsed minutes

    // Calculate elapsed seconds since base date
    tmptm2 = tmptm1 * 60L;
    _VALIDATE_RETURN_NOEXC(!ChkMul(tmptm2, tmptm1, 60L), EINVAL, invalid_time)


    tmptm1 = tmptm2 + (tmptm3 = static_cast<TimeType>(tb->tm_sec));
    _VALIDATE_RETURN_NOEXC(!ChkAdd(tmptm1, tmptm2, tmptm3), EINVAL, invalid_time)


    // HERE: tmptm1 holds number of elapsed seconds

    tm tbtemp;
    if (use_local_time)
    {
        // Adjust for timezone. No need to check for overflow since
        // localtime() will check its arg value
        __tzset();

        long dstbias = 0;
        long timezone = 0;
        _ERRCHECK(_get_dstbias(&dstbias));
        _ERRCHECK(_get_timezone(&timezone));

        tmptm1 += timezone;

        // Convert this second count back into a time block structure.
        // If localtime returns nullptr, return an error.
        if (time_traits::localtime_s(&tbtemp, &tmptm1) != 0)
            return (errno = EINVAL), invalid_time;

        // Now must compensate for DST. The ANSI rules are to use the passed-in
        // tm_isdst flag if it is non-negative. Otherwise, compute if DST
        // applies. Recall that tbtemp has the time without DST compensation,
        // but has set tm_isdst correctly.
        if (tb->tm_isdst > 0 || (tb->tm_isdst < 0 && tbtemp.tm_isdst > 0))
        {
            tmptm1 += dstbias;
            if (time_traits::localtime_s(&tbtemp, &tmptm1) != 0)
                return (errno = EINVAL), invalid_time;
        }

    }
    else
    {
        if (time_traits::gmtime_s(&tbtemp, &tmptm1) != 0)
            return (errno = EINVAL), invalid_time;
    }

    // HERE: tmptm1 holds number of elapsed seconds, adjusted for local time if
    // requested

    *tb = tbtemp;
    return tmptm1;
}



// Converts a tm structure value into a time_t value.  These functions also
// update the tm structure to normalize it and populate any missing fields.
// There are three practical uses for these functions:
//
// (1) To convert a broken-down time to the internal time format (time_t)
// (2) To complete the tm value with the correct tm_wday, tm_yday, or tm_isdst
//     values given the rest of the contents of the tm.
// (3) To pass in a time structure with "out of range" values for some fields
//     and get back a "normalized" tm structure (e.g., to pass in 1/35/1987
//     and get back 2/4/1987).
//
// Returns the resulting time_t value on success; returns -1 on failure.
extern "C" __time32_t __cdecl _mktime32(tm* const tb)
{
    return common_mktime<__time32_t>(tb, true);
}

extern "C" __time64_t __cdecl _mktime64(tm* const tb)
{
    return common_mktime<__time64_t>(tb, true);
}



// Converts a UTC time stored in a tm structure into a time_t value.  These
// functions also update the tm structure to normalize it and populate any
// missing fields.  Returns the resulting time_t value on success; returns
// -1 on failure.
extern "C"  __time32_t __cdecl _mkgmtime32(tm* const tb)
{
    return common_mktime<__time32_t>(tb, false);
}

extern "C"  __time64_t __cdecl _mkgmtime64(tm* const tb)
{
    return common_mktime<__time64_t>(tb, false);
}
