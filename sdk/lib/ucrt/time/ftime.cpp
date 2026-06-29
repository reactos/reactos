//
// ftime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The ftime() family of functions, which return the system date and time in a
// time structure.
//
#include <corecrt_internal_time.h>
#include <sys/timeb.h>
#include <sys/types.h>



// Cache for the minutes count for with DST status was last assessed
// CRT_REFACTOR TODO Check synchronization of access to this global variable.
static __time64_t elapsed_minutes_cache = 0;



// Three values of dstflag_cache
#define DAYLIGHT_TIME   1
#define STANDARD_TIME   0
#define UNKNOWN_TIME   -1



// Cache for the last determined DST status:
static int dstflag_cache = UNKNOWN_TIME;



// Returns the system time in a structure; returns zero on success; returns an
// error code on failure.
template <typename TimeType, typename TimeBType>
static errno_t __cdecl common_ftime_s(TimeBType* const tp) throw()
{
    _VALIDATE_RETURN_ERRCODE(tp != nullptr, EINVAL)

    __tzset();

    long timezone = 0;
    _ERRCHECK(_get_timezone(&timezone));
    tp->timezone = static_cast<short>(timezone / 60);

    __crt_filetime_union system_time;
    __acrt_GetSystemTimePreciseAsFileTime(&system_time._filetime);

    // Obtain the current Daylight Savings Time status.  Note that the status is
    // cached and only updated once per minute, if necessary.
    TimeType const current_minutes_value = static_cast<TimeType>(system_time._scalar / 600000000ll);
    if (static_cast<__time64_t>(current_minutes_value) != elapsed_minutes_cache)
    {
        TIME_ZONE_INFORMATION tz_info;
        DWORD const tz_state = GetTimeZoneInformation(&tz_info);
        if (tz_state == 0xFFFFFFFF)
        {
            dstflag_cache = UNKNOWN_TIME;
        }
        else
        {
            // Must be very careful when determining whether or not Daylight
            // Savings Time is in effect:
            if (tz_state == TIME_ZONE_ID_DAYLIGHT &&
                tz_info.DaylightDate.wMonth != 0 &&
                tz_info.DaylightBias != 0)
            {
                dstflag_cache = DAYLIGHT_TIME;
            }
            else
            {
                // Assume Standard Time:
                dstflag_cache = STANDARD_TIME;
            }
        }

        elapsed_minutes_cache = current_minutes_value;
    }

    tp->dstflag = static_cast<short>(dstflag_cache);
    tp->millitm = static_cast<unsigned short>((system_time._scalar / 10000ll) % 1000ll);
    tp->time    = static_cast<TimeType>((system_time._scalar - _EPOCH_BIAS) / 10000000ll);

    return 0;
}

extern "C" errno_t __cdecl _ftime32_s(__timeb32* const tp)
{
    return common_ftime_s<__time32_t>(tp);
}

extern "C" void __cdecl _ftime32(__timeb32* const tp)
{
    _ftime32_s(tp);
}

extern "C" errno_t __cdecl _ftime64_s(__timeb64* const tp)
{
    return common_ftime_s<__time64_t>(tp);
}

extern "C" void __cdecl _ftime64(__timeb64* const tp)
{
    _ftime64_s(tp);
}
