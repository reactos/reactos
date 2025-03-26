//
// gmtime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The gmtime() family of functions, which converts a time_t value into a tm
// structure in UTC.
//
#include <corecrt_internal_time.h>



// This is a utility that allows us to compute the year value differently for
// 32-bit and 64-bit time_t objects.
static int __cdecl compute_year(__time32_t& caltim, bool& is_leap_year) throw()
{
    // Determine years since 1970. First, identify the four-year interval
    // since this makes handling leap-years easy (note that 2000 IS a
    // leap year and 2100 is out-of-range).
    int tmptim = static_cast<int>(caltim / _FOUR_YEAR_SEC);
    caltim -= static_cast<__time32_t>(tmptim) * _FOUR_YEAR_SEC;

    // Determine which year of the interval
    tmptim = (tmptim * 4) + 70;         // 1970, 1974, 1978,...,etc.

    if (caltim >= _YEAR_SEC)
    {
        tmptim++;                       // 1971, 1975, 1979,...,etc.
        caltim -= _YEAR_SEC;

        if (caltim >= _YEAR_SEC)
        {
            tmptim++;                   // 1972, 1976, 1980,...,etc.
            caltim -= _YEAR_SEC;

            // Note, it takes 366 days-worth of seconds to get past a leap
            // year.
            if (caltim >= (_YEAR_SEC + _DAY_SEC))
            {
                tmptim++;               // 1973, 1977, 1981,...,etc.
                caltim -= (_YEAR_SEC + _DAY_SEC);
            }
            else
            {
                // In a leap year after all, set the flag.
                is_leap_year = true;
            }
        }
    }

    return tmptim;
}

static int __cdecl compute_year(__time64_t& caltim, bool& is_leap_year) throw()
{
    // Determine the years since 1900. Start by ignoring leap years:
    int tmptim = static_cast<int>(caltim / _YEAR_SEC) + 70;
    caltim -= static_cast<__time64_t>(tmptim - 70) * _YEAR_SEC;

    // Correct for elapsed leap years:
    caltim -= static_cast<__time64_t>(__crt_time_elapsed_leap_years(tmptim)) * _DAY_SEC;

    // If we have underflowed the __time64_t range (i.e., if caltim < 0),
    // back up one year, adjusting the correction if necessary.
    if (caltim < 0)
    {
        caltim += static_cast<__time64_t>(_YEAR_SEC);
        tmptim--;
        if (__crt_time_is_leap_year(tmptim))
        {
            caltim += _DAY_SEC;
            is_leap_year = true;
        }
    }
    else if (__crt_time_is_leap_year(tmptim))
    {
        is_leap_year = true;
    }

    return tmptim;
}

// Converts a time_t value into a tm structure in UTC.  Stores the tm structure
// into the '*ptm' buffer. Returns zero on success; returns an error code on
// failure
template <typename TimeType>
static errno_t __cdecl common_gmtime_s(tm* const ptm, TimeType const* const timp) throw()
{
    typedef __crt_time_time_t_traits<__time64_t> time_traits;

    _VALIDATE_RETURN_ERRCODE(ptm != nullptr, EINVAL)
    memset(ptm, 0xff, sizeof(tm));

    _VALIDATE_RETURN_ERRCODE(timp != nullptr, EINVAL);
    TimeType caltim = *timp;

    _VALIDATE_RETURN_ERRCODE_NOEXC(caltim >= _MIN_LOCAL_TIME, EINVAL)

    // Upper bound check only necessary for _gmtime64_s (it's > LONG_MAX).
    // For _gmtime32_s, any positive number is within range (<= LONG_MAX).
    _VALIDATE_RETURN_ERRCODE_NOEXC(caltim <= time_traits::max_time_t + _MAX_LOCAL_TIME, EINVAL)

    // tmptim now holds the value for tm_year. caltim now holds the
    // number of elapsed seconds since the beginning of that year.
    bool is_leap_year = false;
    ptm->tm_year = compute_year(caltim, is_leap_year);

    // Determine days since January 1 (0 - 365). This is the tm_yday value.
    // Leave caltim with number of elapsed seconds in that day.
    ptm->tm_yday = static_cast<int>(caltim / _DAY_SEC);
    caltim -= static_cast<TimeType>(ptm->tm_yday) * _DAY_SEC;

    // Determine months since January (0 - 11) and day of month (1 - 31):
    int const* const mdays = is_leap_year ? _lpdays : _days;

    int tmptim = 0;
    for (tmptim = 1 ; mdays[tmptim] < ptm->tm_yday ; tmptim++)
    {
    }

    ptm->tm_mon = --tmptim;

    ptm->tm_mday = ptm->tm_yday - mdays[tmptim];

    // Determine days since Sunday (0 - 6)
    ptm->tm_wday = (static_cast<int>(*timp / _DAY_SEC) + _BASE_DOW) % 7;

    // Determine hours since midnight (0 - 23), minutes after the hour
    // (0 - 59), and seconds after the minute (0 - 59).
    ptm->tm_hour = static_cast<int>(caltim / 3600);
    caltim -= static_cast<TimeType>(ptm->tm_hour) * 3600L;

    ptm->tm_min = static_cast<int>(caltim / 60);
    ptm->tm_sec = static_cast<int>(caltim - (ptm->tm_min) * 60);

    ptm->tm_isdst = 0;
    return 0;
}

extern "C" errno_t __cdecl _gmtime32_s(tm* const result, __time32_t const* const time_value)
{
    return common_gmtime_s(result, time_value);
}

extern "C" errno_t __cdecl _gmtime64_s(tm* const result, __time64_t const* const time_value)
{
    return common_gmtime_s(result, time_value);
}



// Gets the thread-local buffer to be used by gmtime.  Returns a pointer to the
// buffer on success; returns null and sets errno on failure.
extern "C" tm* __cdecl __getgmtimebuf()
{
    __acrt_ptd* const ptd = __acrt_getptd_noexit();
    if (ptd == nullptr)
    {
        errno = ENOMEM;
        return nullptr;
    }

    if (ptd->_gmtime_buffer != nullptr)
    {
        return ptd->_gmtime_buffer;
    }

    ptd->_gmtime_buffer = _malloc_crt_t(tm, 1).detach();
    if (ptd->_gmtime_buffer == nullptr)
    {
        errno = ENOMEM;
        return nullptr;
    }

    return ptd->_gmtime_buffer;
}


// Converts a time_t value into a tm structure in UTC.  Returns a pointer to a
// thread-local buffer containing the tm structure on success; returns null on
// failure.
template <typename TimeType>
_Success_(return != 0)
static tm* __cdecl common_gmtime(TimeType const* const time_value) throw()
{
    tm* const ptm = __getgmtimebuf();
    if (ptm == nullptr)
        return nullptr;

    if (common_gmtime_s(ptm, time_value) != 0)
        return nullptr;

    return ptm;
}

extern "C" tm* __cdecl _gmtime32(__time32_t const* const time_value)
{
    return common_gmtime(time_value);
}

extern "C" tm* __cdecl _gmtime64(__time64_t const* const time_value)
{
    return common_gmtime(time_value);
}
