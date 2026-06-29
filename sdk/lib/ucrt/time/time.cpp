//
// time.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The timespec_get() and time() families of functions, which get the current
// system time as a timespec and a time_t, respectively.
//
#include <corecrt_internal_time.h>


using get_system_time_function_type = void(WINAPI *)(LPFILETIME);

template <get_system_time_function_type GetSystemTimeFunction, typename TimeSpecType>
_Success_(return != 0)
static int __cdecl common_timespec_get(TimeSpecType* const ts, int const base) throw()
{
    typedef decltype(ts->tv_sec)                time_type;
    typedef __crt_time_time_t_traits<time_type> time_traits;

    _VALIDATE_RETURN(ts != nullptr, EINVAL, 0);

    if (base != TIME_UTC)
    {
        return 0;
    }

    __crt_filetime_union system_time{};
    GetSystemTimeFunction(&system_time._filetime);

    __time64_t const filetime_scale{10 * 1000 * 1000}; // 100ns units

    __time64_t const epoch_time{static_cast<__time64_t>(system_time._scalar) - _EPOCH_BIAS};

    __time64_t const seconds    {epoch_time / filetime_scale};
    __time64_t const nanoseconds{epoch_time % filetime_scale * 100};

    if (seconds > static_cast<__time64_t>(time_traits::max_time_t))
    {
        return 0;
    }

    ts->tv_sec  = static_cast<time_type>(seconds);
    ts->tv_nsec = static_cast<long>(nanoseconds);
    return base;
}

extern "C" int __cdecl _timespec32_get(_timespec32* const ts, int const base)
{
    return common_timespec_get<__acrt_GetSystemTimePreciseAsFileTime>(ts, base);
}

extern "C" int __cdecl _timespec64_get(_timespec64* const ts, int const base)
{
    return common_timespec_get<__acrt_GetSystemTimePreciseAsFileTime>(ts, base);
}



// Gets the current system time and converts it to a time_t value. If 'result'
// is non-null, the time is stored in '*result'.  The time is also returned
// (even if 'result' is null).
template <typename TimeType>
static TimeType __cdecl common_time(TimeType* const result) throw()
{
    typedef __crt_time_time_t_traits<TimeType> time_traits;

    typename time_traits::timespec_type ts{};

    // The resolution of time() is in seconds, so we can afford to use
    // a less precise, but faster method of getting the time.
    // GetSystemTimePreciseAsFileTime uses QueryPerformanceCounter which
    // needs to be virtualized on some systems in order to be accurate, which can
    // be significantly slower than calling GetSystemTimeAsFileTime.
    // For example, hypervisors will virtualize QPC calls in order to take into
    // account virtual machine live migration.
    // Note that calling the less precise API is still observable because the tick
    // count may update +/- 15ms, but this should not affect common use of time().
    // See https://docs.microsoft.com/en-us/windows/desktop/sysinfo/acquiring-high-resolution-time-stamps
    // for more details.
    if (common_timespec_get<GetSystemTimeAsFileTime>(&ts, TIME_UTC) != TIME_UTC)
    {
        ts.tv_sec = static_cast<TimeType>(-1);
    }

    if (result)
    {
        *result = ts.tv_sec;
    }

    return ts.tv_sec;
}

extern "C" __time32_t __cdecl _time32(__time32_t* const result)
{
    return common_time(result);
}

extern "C" __time64_t __cdecl _time64(__time64_t* const result)
{
    return common_time(result);
}
