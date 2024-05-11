//
// localtime.cpp
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Defines the localtime family of functions, which convert a time_t to a tm
// structure containing the local time.
//
#include <corecrt_internal_time.h>



// Converts a time_t value to a tm value containing the corresponding local time.
// Returns zero and updates the tm structure on success; returns nonzero and
// leaves the tm structure in an indeterminate state on failure.
//
// Assumptions:
//      (1) gmtime must be called before _isindst to ensure that the tb time
//          structure is initialized.
//      (2) gmtime, _gtime64, localtime and _localtime64() all use a single
//          statically allocated buffer. Each call to one of these routines
//          destroys the contents of the previous call.
//      (3) It is assumed that __time64_t is a 64-bit integer representing
//          the number of seconds since 00:00:00, 01-01-70 (UTC) (i.e., the
//          Posix/Unix Epoch. Only non-negative values are supported.
//      (4) It is assumed that the maximum adjustment for local time is
//          less than three days (include Daylight Savings Time adjustment).
//          This only a concern in Posix where the specification of the TZ
//          environment restricts the combined offset for time zone and
//          Daylight Savings Time to 2 * (24:59:59), just under 50 hours.
// If any of these assumptions are violated, the behavior is undefined.
template <typename TimeType>
static errno_t __cdecl common_localtime_s(
    tm*             const ptm,
    TimeType const* const ptime
    ) throw()
{
    typedef __crt_time_time_t_traits<TimeType> time_traits;

    _VALIDATE_RETURN_ERRCODE(ptm != nullptr, EINVAL);
    memset(ptm, 0xff, sizeof(tm));

    _VALIDATE_RETURN_ERRCODE(ptime != nullptr, EINVAL);

    // Check for illegal time_t value:
    _VALIDATE_RETURN_ERRCODE_NOEXC(*ptime >= 0,                       EINVAL);
    _VALIDATE_RETURN_ERRCODE_NOEXC(*ptime <= time_traits::max_time_t, EINVAL);

    __tzset();

    int  daylight = 0;
    long dstbias  = 0;
    long timezone = 0;
    _ERRCHECK(_get_daylight(&daylight));
    _ERRCHECK(_get_dstbias (&dstbias ));
    _ERRCHECK(_get_timezone(&timezone));

    if (*ptime > 3 * _DAY_SEC && *ptime < time_traits::max_time_t - 3 * _DAY_SEC)
    {
        // The date does not fall within the first three or last three representable
        // days; therefore, there is no possibility of overflowing or underflowing
        // the time_t representation as we compensate for time zone and daylight
        // savings time.
        TimeType ltime = *ptime - timezone;

        errno_t status0 = time_traits::gmtime_s(ptm, &ltime);
        if (status0 != 0)
            return status0;

        // Check and adjust for daylight savings time:
        if (daylight && _isindst(ptm))
        {
            ltime -= dstbias;
            
            errno_t const status1 = time_traits::gmtime_s(ptm, &ltime);
            if (status1 != 0)
                return status1;

            ptm->tm_isdst = 1;
        }
    }
    else
    {
        // The date falls within the first three or last three representable days;
        // therefore, it is possible that the time_t representation would overflow
        // or underflow while compensating for time zone and daylight savings time.
        // Therefore, we make the time zone and daylight savings time adjustments
        // directly in the tm structure.
        errno_t const status0 = time_traits::gmtime_s(ptm, ptime);
        if (status0 != 0)
            return status0;

        TimeType ltime = static_cast<TimeType>(ptm->tm_sec);

        // First, adjust for the time zone:
        if (daylight && _isindst(ptm))
        {
            ltime -= (timezone + dstbias);
            ptm->tm_isdst = 1;
        }
        else
        {
            ltime -= timezone;
        }

        ptm->tm_sec = static_cast<int>(ltime % 60);
        if (ptm->tm_sec < 0)
        {
            ptm->tm_sec += 60;
            ltime -= 60;
        }

        ltime = static_cast<TimeType>(ptm->tm_min) + ltime / 60;
        ptm->tm_min = static_cast<int>(ltime % 60);
        if (ptm->tm_min < 0)
        {
            ptm->tm_min += 60;
            ltime -= 60;
        }

        ltime = static_cast<TimeType>(ptm->tm_hour) + ltime / 60;
        ptm->tm_hour = static_cast<int>(ltime % 24);
        if (ptm->tm_hour < 0)
        {
            ptm->tm_hour += 24;
            ltime -=24;
        }

        ltime /= 24;

        if (ltime > 0)
        {
            // There is no possibility of overflowing the tm_day and tm_yday
            // members because the date can be no later than January 19.
            ptm->tm_wday = (ptm->tm_wday + static_cast<int>(ltime)) % 7;
            ptm->tm_mday += static_cast<int>(ltime);
            ptm->tm_yday += static_cast<int>(ltime);
        }
        else if (ltime < 0)
        {
            // It is possible to underflow the tm_mday and tm_yday fields.  If
            // this happens, then the adjusted date must lie in December 1969:
            ptm->tm_wday = (ptm->tm_wday + 7 + static_cast<int>(ltime)) % 7;
            ptm->tm_mday += static_cast<int>(ltime);
            if (ptm->tm_mday <= 0)
            {
                ptm->tm_mday += 31;

                // Per assumption #4 above, the time zone can cause the date to
                // underflow the epoch by more than a day.
                ptm->tm_yday = ptm->tm_yday + static_cast<int>(ltime) + 365;
                ptm->tm_mon = 11;
                ptm->tm_year--;
            }
            else
            {
                ptm->tm_yday += static_cast<int>(ltime);
            }
        }
    }

    return 0;
}

extern "C" errno_t __cdecl _localtime32_s(
    tm*               const ptm,
    __time32_t const* const ptime
    )
{
    return common_localtime_s(ptm, ptime);
}

extern "C" errno_t __cdecl _localtime64_s(
    tm*               const ptm,
    __time64_t const* const ptime
    )
{
    return common_localtime_s(ptm, ptime);
}



// Converts a time_t value to a tm value containing the corresponding local time.
// Returns a pointer to the thread-local tm buffer containing the result on
// success; returns nullptr on failure.
template <typename TimeType>
_Success_(return != 0)
static tm* __cdecl common_localtime(TimeType const* const ptime) throw()
{
    typedef __crt_time_time_t_traits<TimeType> time_traits;

    tm* const ptm = __getgmtimebuf();
    if (ptm == nullptr)
        return nullptr;

    errno_t const status = time_traits::localtime_s(ptm, ptime);
    if (status != 0)
        return nullptr;

    return ptm;
}

extern "C" tm* __cdecl _localtime32(__time32_t const* const ptime)
{
    return common_localtime(ptime);
}

extern "C" tm* __cdecl _localtime64(__time64_t const* const ptime)
{
    return common_localtime(ptime);
}
