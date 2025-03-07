//
// corecrt_internal_time.h
//
//      Copyright (c) Microsoft Corporation.  All rights reserved.
//
// This internal header defines internal utilities for working with the time
// library.
//
#pragma once

#include <corecrt.h>
#include <corecrt_internal.h>
#include <corecrt_internal_traits.h>
#include <io.h>
#include <sys/utime.h>
#include <time.h>

#pragma pack(push, _CRT_PACKING)



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Constants
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Number of 100 nanosecond units from 1/1/1601 to 1/1/1970
#define _EPOCH_BIAS 116444736000000000ll

#define _DAY_SEC       (24 * 60 * 60)    // Seconds in a day
#define _YEAR_SEC      (365 * _DAY_SEC)  // Seconds in a year
#define _FOUR_YEAR_SEC (1461 * _DAY_SEC) // Seconds in a four-year interval
#define _BASE_YEAR     70                // The epoch year (1970)
#define _BASE_DOW      4                 // The day of week of 01-Jan-70 (Thursday)

// Maximum local time adjustment (GMT + 14 Hours, DST -0 Hours)
#define _MAX_LOCAL_TIME (14 * 60 * 60)

// Minimum local time adjustment (GMT - 11 Hours, DST - 1 Hours)
#define _MIN_LOCAL_TIME (-12 * 60 * 60)

#define _TZ_STRINGS_SIZE 64


//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Global Data
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
extern "C"
{
    extern char const __dnames[];
    extern char const __mnames[];

    extern int const _days[];
    extern int const _lpdays[];
}




//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Integer Traits
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
template <typename TimeType>
struct __crt_time_time_t_traits;

template <>
struct __crt_time_time_t_traits<__time32_t> : __crt_integer_traits<__time32_t>
{
    typedef _timespec32 timespec_type;

    enum : long
    {
        // Number of seconds from 00:00:00, 01/01/1970 UTC to 23:59:59, 01/18/2038 UTC
        max_time_t = 0x7fffd27f,
    };

    enum : unsigned long
    {
        // The maximum representable year
        max_year = 138,  // 2038 is the maximum year
    };
};

template <>
struct __crt_time_time_t_traits<__time64_t> : __crt_integer_traits<__time64_t>
{
    typedef _timespec64 timespec_type;

    enum : long long
    {
        // Number of seconds from 00:00:00, 01/01/1970 UTC to 07:59:59, 01/19/3001 UTC
        // Note that the end of the epoch was intended to be 23:59:59, 01/18/3001 UTC,
        // but this was mistakenly computed from a PST value (thus the 8 hour delta).
        max_time_t = 0x793582affLL,
    };

    enum : unsigned long long
    {
        // The maximum representable year
        max_year = 1101, // 3001 is the maximum year
    };
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Combined Traits
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
template <typename TimeType, typename Character>
struct __crt_time_traits
    : __crt_char_traits<Character>,
      __crt_time_time_t_traits<TimeType>
{
};



//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
// Utilities
//
//-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// Union to facilitate converting from FILETIME to unsigned __int64
union __crt_filetime_union
{
    unsigned __int64 _scalar;
    FILETIME         _filetime;
};



extern "C"
{
    int  __cdecl _isindst(_In_ tm* _Time);
    void __cdecl __tzset();
    tm*  __cdecl __getgmtimebuf();

    __time32_t __cdecl __loctotime32_t(int, int, int, int, int, int, int);
    __time64_t __cdecl __loctotime64_t(int, int, int, int, int, int, int);
}



// Tests if the given year is a leap year.  The year is not the absolute year;
// it is the number of years since 1900.
template <typename TimeType>
bool __cdecl __crt_time_is_leap_year(TimeType const yr) throw()
{
    if (yr % 4 == 0 && yr % 100 != 0)
        return true;

    if ((yr + 1900) % 400 == 0)
        return true;

    return false;
}

// Computes the number of leap years that have elapsed betwwen 1970 up to, but
// not including, the specified year.  The year is not the absolute year; it is
// the number of years since 1900.
template <typename TimeType>
TimeType __cdecl __crt_time_elapsed_leap_years(TimeType const yr) throw()
{
    static TimeType const leap_years_between_1900_and_1970 = 17;

    TimeType const elapsed_leap_years = ((yr - 1) / 4) - ((yr - 1) / 100) + ((yr + 299) / 400);

    return elapsed_leap_years - leap_years_between_1900_and_1970;
}

// Tests if the given date is valid (i.e., if such a day actually existed).
inline bool __cdecl __crt_time_is_day_valid(int const yr, int const mo, int const dy) throw()
{
    if (dy <= 0)
        return false;

    int const days_in_month = _days[mo + 1] - _days[mo];
    if (dy <= days_in_month)
        return true;

    // Special case for February:
    if (__crt_time_is_leap_year(yr) && mo == 1 && dy <= 29)
        return true;

    return false;
}

inline int __crt_get_2digit_year(int const year) throw()
{
    return (1900 + year) % 100;
}

inline int __crt_get_century(int const year) throw()
{
    return (1900 + year) / 100;
}

extern "C" _Success_(return > 0) size_t __cdecl _Wcsftime_l(
    _Out_writes_z_(max_size) wchar_t*       string,
    _In_                     size_t         max_size,
    _In_z_                   wchar_t const* format,
    _In_                     tm const*      timeptr,
    _In_opt_                 void*          lc_time_arg,
    _In_opt_                 _locale_t      locale
    );

_Check_return_ _Deref_ret_z_
extern "C" wchar_t** __cdecl __wide_tzname();

#pragma pack(pop)
