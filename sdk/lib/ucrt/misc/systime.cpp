//
// systime.cpp
//
//      Copyright (c) Microsoft Corporation. All rights reserved.
//
// The _getsystime() and _setsystime() functions.
//
#include <corecrt_internal_time.h>



// Gets the current system time and stores it in 'result'.  Returns the number
// of milliseconds (the 'result' cannot store milliseconds).
extern "C" unsigned __cdecl _getsystime(struct tm* const result)
{
    _VALIDATE_RETURN(result != nullptr, EINVAL, 0)

    SYSTEMTIME local_time;
    GetLocalTime(&local_time);

    result->tm_isdst       = -1; // mktime() computes whether this is
                              // during Standard or Daylight time.
    result->tm_sec         = static_cast<int>(local_time.wSecond);
    result->tm_min         = static_cast<int>(local_time.wMinute);
    result->tm_hour        = static_cast<int>(local_time.wHour);
    result->tm_mday        = static_cast<int>(local_time.wDay);
    result->tm_mon         = static_cast<int>(local_time.wMonth - 1);
    result->tm_year        = static_cast<int>(local_time.wYear - 1900);
    result->tm_wday        = static_cast<int>(local_time.wDayOfWeek);

    // Normalize uninitialized fields:
    _mktime32(result);

    return (local_time.wMilliseconds);
}



// Sets the system time to the time specified by 'source' and 'milliseconds.
// Returns zero on success; returns a system error code on failure.
extern "C" unsigned __cdecl _setsystime(struct tm* const source, unsigned const milliseconds)
{
    _ASSERTE(source != nullptr);
    if (source == nullptr)
        return ERROR_INVALID_PARAMETER;

    // Normalize uninitialized fields:
    _mktime32(source);

    SYSTEMTIME local_time;
    local_time.wYear         = static_cast<WORD>(source->tm_year + 1900);
    local_time.wMonth        = static_cast<WORD>(source->tm_mon + 1);
    local_time.wDay          = static_cast<WORD>(source->tm_mday);
    local_time.wHour         = static_cast<WORD>(source->tm_hour);
    local_time.wMinute       = static_cast<WORD>(source->tm_min);
    local_time.wSecond       = static_cast<WORD>(source->tm_sec);
    local_time.wMilliseconds = static_cast<WORD>(milliseconds);

    if (!SetLocalTime(&local_time))
        return static_cast<int>(GetLastError());

    return 0;
}
