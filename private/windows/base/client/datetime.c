/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    datetime.c

Abstract:

    This module implements Win32 time of day functions

Author:

    Mark Lucovsky (markl) 08-Oct-1990

Revision History:

--*/

#include "basedll.h"

VOID
WINAPI
GetLocalTime(
    LPSYSTEMTIME lpLocalTime
    )

/*++

Routine Description:

    The current local system date and time can be returned using
    GetLocalTime.

Arguments:

    lpLocalTime - Returns the current system date and time:

        SYSTEMTIME Structure:

        WORD wYear - Returns the current year.

        WORD wMonth - Returns the current month with January equal to 1.

        WORD wDayOfWeek - Returns the current day of the week where
            0=Sunday, 1=Monday...

        WORD wDay - Returns the current day of the month.

        WORD wHour - Returns the current hour.

        WORD wMinute - Returns the current minute within the hour.

        WORD wSecond - Returns the current second within the minute.

        WORD wMilliseconds - Returns the current millisecond within the
            second.

Return Value:

    None.

--*/

{
    LARGE_INTEGER LocalTime;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER Bias;
    TIME_FIELDS TimeFields;

#if defined(_WIN64)
    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemHigh1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemLowTime;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemHigh2Time);
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#elif defined(_ALPHA_)
    SystemTime.QuadPart = USER_SHARED_DATA->SystemTime;
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#elif defined(_MIPS_)
    QUERY_SYSTEM_TIME(&SystemTime);
    do {
        Bias.HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        Bias.LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
    } while (Bias.HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);
#else
    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);
    do {
        Bias.HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        Bias.LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
    } while (Bias.HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);
#endif

    LocalTime.QuadPart = SystemTime.QuadPart - Bias.QuadPart;

    RtlTimeToTimeFields(&LocalTime,&TimeFields);

    lpLocalTime->wYear         = TimeFields.Year        ;
    lpLocalTime->wMonth        = TimeFields.Month       ;
    lpLocalTime->wDayOfWeek    = TimeFields.Weekday     ;
    lpLocalTime->wDay          = TimeFields.Day         ;
    lpLocalTime->wHour         = TimeFields.Hour        ;
    lpLocalTime->wMinute       = TimeFields.Minute      ;
    lpLocalTime->wSecond       = TimeFields.Second      ;
    lpLocalTime->wMilliseconds = TimeFields.Milliseconds;
}

VOID
WINAPI
GetSystemTime(
    LPSYSTEMTIME lpSystemTime
    )

/*++

Routine Description:

    The current system date and time (UTC based) can be returned using
    GetSystemTime.

Arguments:

    lpSystemTime - Returns the current system date and time:

        SYSTEMTIME Structure:

        WORD wYear - Returns the current year.

        WORD wMonth - Returns the current month with January equal to 1.

        WORD wDayOfWeek - Returns the current day of the week where
            0=Sunday, 1=Monday...

        WORD wDay - Returns the current day of the month.

        WORD wHour - Returns the current hour.

        WORD wMinute - Returns the current minute within the hour.

        WORD wSecond - Returns the current second within the minute.

        WORD wMilliseconds - Returns the current millisecond within the
            second.

Return Value:

    None.

--*/

{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;

#if defined(_WIN64)
    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemHigh1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemLowTime;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemHigh2Time);
#elif defined(_ALPHA_)
    SystemTime.QuadPart = USER_SHARED_DATA->SystemTime;
#elif defined(_MIPS_)
    QUERY_SYSTEM_TIME(&SystemTime);
#else
    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);
#endif

    RtlTimeToTimeFields(&SystemTime,&TimeFields);

    lpSystemTime->wYear         = TimeFields.Year        ;
    lpSystemTime->wMonth        = TimeFields.Month       ;
    lpSystemTime->wDayOfWeek    = TimeFields.Weekday     ;
    lpSystemTime->wDay          = TimeFields.Day         ;
    lpSystemTime->wHour         = TimeFields.Hour        ;
    lpSystemTime->wMinute       = TimeFields.Minute      ;
    lpSystemTime->wSecond       = TimeFields.Second      ;
    lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
}

VOID
WINAPI
GetSystemTimeAsFileTime(
    LPFILETIME lpSystemTimeAsFileTime
    )

/*++

Routine Description:

    The current system date and time (UTC based) can be returned using
    GetSystemTimeAsFileTime.

Arguments:

    lpSystemTimeAsFileTime - Returns the current system date and time formatted as
        a FILETIME structure

Return Value:

    None.

--*/

{
    LARGE_INTEGER SystemTime;

#if defined(_WIN64)
    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemHigh1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemLowTime;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemHigh2Time);
#elif defined(_ALPHA_)
    SystemTime.QuadPart = USER_SHARED_DATA->SystemTime;
#elif defined(_MIPS_)
    QUERY_SYSTEM_TIME(&SystemTime);
#else
    do {
        SystemTime.HighPart = USER_SHARED_DATA->SystemTime.High1Time;
        SystemTime.LowPart = USER_SHARED_DATA->SystemTime.LowPart;
    } while (SystemTime.HighPart != USER_SHARED_DATA->SystemTime.High2Time);
#endif

    lpSystemTimeAsFileTime->dwLowDateTime = SystemTime.LowPart;
    lpSystemTimeAsFileTime->dwHighDateTime = SystemTime.HighPart;
}

BOOL
WINAPI
SetSystemTime(
    CONST SYSTEMTIME *lpSystemTime
    )

/*++

Routine Description:

    The current UTC based system date and time can be set using
    SetSystemTime.

Arguments:

    lpSystemTime - Supplies the date and time to set. The wDayOfWeek field
        is ignored.

Return Value:

    TRUE - The current system date and time was set.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;
    BOOLEAN ReturnValue;
    PVOID State;
    NTSTATUS Status;

    ReturnValue = TRUE;

    TimeFields.Year         = lpSystemTime->wYear        ;
    TimeFields.Month        = lpSystemTime->wMonth       ;
    TimeFields.Day          = lpSystemTime->wDay         ;
    TimeFields.Hour         = lpSystemTime->wHour        ;
    TimeFields.Minute       = lpSystemTime->wMinute      ;
    TimeFields.Second       = lpSystemTime->wSecond      ;
    TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

    if ( !RtlTimeFieldsToTime(&TimeFields,&SystemTime) ) {
        Status = STATUS_INVALID_PARAMETER;
        ReturnValue = FALSE;
        }
    else {
        Status = BasepAcquirePrivilegeEx( SE_SYSTEMTIME_PRIVILEGE, &State );
        if ( NT_SUCCESS(Status) ) {
            Status = NtSetSystemTime(&SystemTime,NULL);
            BasepReleasePrivilege( State );
            }
        if ( !NT_SUCCESS(Status) ) {
            ReturnValue = FALSE;
            }
        }

    if ( !ReturnValue ) {
        BaseSetLastNTError(Status);
        }

    return ReturnValue;
}

BOOL
WINAPI
SetLocalTime(
    CONST SYSTEMTIME *lpLocalTime
    )

/*++

Routine Description:

    The current local system date and time can be set using
    SetLocalTime.

Arguments:

    lpSystemTime - Supplies the date and time to set. The wDayOfWeek field
        is ignored.

Return Value:

    TRUE - The current system date and time was set.

    FALSE/NULL - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER LocalTime;
    TIME_FIELDS TimeFields;
    BOOLEAN ReturnValue;
    PVOID State;
    NTSTATUS Status;
    LARGE_INTEGER Bias;

#ifdef _ALPHA_
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#elif defined(_IA64_)
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#else
    do {
        Bias.HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        Bias.LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
    } while (Bias.HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);
#endif

    ReturnValue = TRUE;

    TimeFields.Year         = lpLocalTime->wYear        ;
    TimeFields.Month        = lpLocalTime->wMonth       ;
    TimeFields.Day          = lpLocalTime->wDay         ;
    TimeFields.Hour         = lpLocalTime->wHour        ;
    TimeFields.Minute       = lpLocalTime->wMinute      ;
    TimeFields.Second       = lpLocalTime->wSecond      ;
    TimeFields.Milliseconds = lpLocalTime->wMilliseconds;

    if ( !RtlTimeFieldsToTime(&TimeFields,&LocalTime) ) {
        Status = STATUS_INVALID_PARAMETER;
        ReturnValue = FALSE;
        }
    else {

        SystemTime.QuadPart = LocalTime.QuadPart + Bias.QuadPart;
        Status = BasepAcquirePrivilegeEx( SE_SYSTEMTIME_PRIVILEGE, &State );
        if ( NT_SUCCESS(Status) ) {
            Status = NtSetSystemTime(&SystemTime,NULL);
            BasepReleasePrivilege( State );
            if ( !NT_SUCCESS(Status) ) {
                ReturnValue = FALSE;
                }
            }
        else {
            ReturnValue = FALSE;
            }
        }

    if ( !ReturnValue ) {
        BaseSetLastNTError(Status);
        }

    return ReturnValue;
}


DWORD
GetTickCount(
    VOID
    )

/*++

Routine Description:

    Win32 systems implement a free-running millisecond counter.  The
    value of this counter can be read using GetTickCount.

Arguments:

    None.

Return Value:

    This function returns the number of milliseconds that have elapsed
    since the system was started. If the system has been running for
    a long time, it is possible that the count will repeat. The value of
    the counter is accurate within 55 milliseconds.

--*/

{
    return (DWORD)NtGetTickCount();
}


BOOL
APIENTRY
FileTimeToSystemTime(
    CONST FILETIME *lpFileTime,
    LPSYSTEMTIME lpSystemTime
    )

/*++

Routine Description:

    This functions converts a 64-bit file time value to a time in system
    time format.

Arguments:

    lpFileTime - Supplies the 64-bit file time to convert to the system
        date and time format.

    lpSystemTime - Returns the converted value of the 64-bit file time.

Return Value:

    TRUE - The 64-bit file time was successfully converted.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    LARGE_INTEGER FileTime;
    TIME_FIELDS TimeFields;

    FileTime.LowPart = lpFileTime->dwLowDateTime;
    FileTime.HighPart = lpFileTime->dwHighDateTime;

    if ( FileTime.QuadPart < 0 ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }

    RtlTimeToTimeFields(&FileTime, &TimeFields);

    lpSystemTime->wYear         = TimeFields.Year        ;
    lpSystemTime->wMonth        = TimeFields.Month       ;
    lpSystemTime->wDay          = TimeFields.Day         ;
    lpSystemTime->wDayOfWeek    = TimeFields.Weekday     ;
    lpSystemTime->wHour         = TimeFields.Hour        ;
    lpSystemTime->wMinute       = TimeFields.Minute      ;
    lpSystemTime->wSecond       = TimeFields.Second      ;
    lpSystemTime->wMilliseconds = TimeFields.Milliseconds;

    return TRUE;
}


BOOL
APIENTRY
SystemTimeToFileTime(
    CONST SYSTEMTIME *lpSystemTime,
    LPFILETIME lpFileTime
    )

/*++

Routine Description:

    This functions converts a system time value into a 64-bit file time.

Arguments:

    lpSystemTime - Supplies the time that is to be converted into
        the 64-bit file time format.  The wDayOfWeek field is ignored.

    lpFileTime - Returns the 64-bit file time representation of
        lpSystemTime.

Return Value:

    TRUE - The time was successfully converted.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER FileTime;

    TimeFields.Year         = lpSystemTime->wYear        ;
    TimeFields.Month        = lpSystemTime->wMonth       ;
    TimeFields.Day          = lpSystemTime->wDay         ;
    TimeFields.Hour         = lpSystemTime->wHour        ;
    TimeFields.Minute       = lpSystemTime->wMinute      ;
    TimeFields.Second       = lpSystemTime->wSecond      ;
    TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

    if ( !RtlTimeFieldsToTime(&TimeFields,&FileTime)) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
        }
    else {
        lpFileTime->dwLowDateTime = FileTime.LowPart;
        lpFileTime->dwHighDateTime = FileTime.HighPart;
        return TRUE;
        }
}

BOOL
WINAPI
FileTimeToLocalFileTime(
    CONST FILETIME *lpFileTime,
    LPFILETIME lpLocalFileTime
    )

/*++

Routine Description:

    This functions converts a UTC based file time to a local file time.

Arguments:

    lpFileTime - Supplies the UTC based file time that is to be
        converted into a local file time

    lpLocalFileTime - Returns the 64-bit local file time representation of
        lpFileTime.

Return Value:

    TRUE - The time was successfully converted.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    LARGE_INTEGER FileTime;
    LARGE_INTEGER LocalFileTime;
    LARGE_INTEGER Bias;

#ifdef _ALPHA_
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#elif defined(_IA64_)
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#else
    do {
        Bias.HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        Bias.LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
    } while (Bias.HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);
#endif

    FileTime.LowPart = lpFileTime->dwLowDateTime;
    FileTime.HighPart = lpFileTime->dwHighDateTime;

    LocalFileTime.QuadPart = FileTime.QuadPart - Bias.QuadPart;

    lpLocalFileTime->dwLowDateTime = LocalFileTime.LowPart;
    lpLocalFileTime->dwHighDateTime = LocalFileTime.HighPart;

    return TRUE;
}

BOOL
WINAPI
LocalFileTimeToFileTime(
    CONST FILETIME *lpLocalFileTime,
    LPFILETIME lpFileTime
    )

/*++

Routine Description:

    This functions converts a local file time to a UTC based file time.

Arguments:

    lpLocalFileTime - Supplies the local file time that is to be
        converted into a UTC based file time

    lpFileTime - Returns the 64-bit UTC based file time representation of
        lpLocalFileTime.

Return Value:

    TRUE - The time was successfully converted.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    LARGE_INTEGER FileTime;
    LARGE_INTEGER LocalFileTime;
    LARGE_INTEGER Bias;

#ifdef _ALPHA_
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#elif defined(_IA64_)
    Bias.QuadPart = USER_SHARED_DATA->TimeZoneBias;
#else
    do {
        Bias.HighPart = USER_SHARED_DATA->TimeZoneBias.High1Time;
        Bias.LowPart = USER_SHARED_DATA->TimeZoneBias.LowPart;
    } while (Bias.HighPart != USER_SHARED_DATA->TimeZoneBias.High2Time);
#endif

    LocalFileTime.LowPart = lpLocalFileTime->dwLowDateTime;
    LocalFileTime.HighPart = lpLocalFileTime->dwHighDateTime;

    FileTime.QuadPart = LocalFileTime.QuadPart + Bias.QuadPart;

    lpFileTime->dwLowDateTime = FileTime.LowPart;
    lpFileTime->dwHighDateTime = FileTime.HighPart;

    return TRUE;
}


#define AlmostTwoSeconds (2*1000*1000*10 - 1)

BOOL
APIENTRY
FileTimeToDosDateTime(
    CONST FILETIME *lpFileTime,
    LPWORD lpFatDate,
    LPWORD lpFatTime
    )

/*++

Routine Description:

    This function converts a 64-bit file time into DOS date and time value
    which is represented as two 16-bit unsigned integers.

    Since the DOS date format can only represent dates between 1/1/80 and
    12/31/2107, this conversion can fail if the input file time is outside
    of this range.

Arguments:

    lpFileTime - Supplies the 64-bit file time to convert to DOS date and
        time format.

    lpFatDate - Returns the 16-bit DOS representation of date.

    lpFatTime - Returns the 16-bit DOS representation of time.

Return Value:

    TRUE - The file time was successfully converted.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER FileTime;

    FileTime.LowPart = lpFileTime->dwLowDateTime;
    FileTime.HighPart = lpFileTime->dwHighDateTime;

    FileTime.QuadPart = FileTime.QuadPart + (LONGLONG)AlmostTwoSeconds;

    if ( FileTime.QuadPart < 0 ) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
        }
    RtlTimeToTimeFields(&FileTime, &TimeFields);

    if (TimeFields.Year < 1980 || TimeFields.Year > 2107) {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
        }

    *lpFatDate = (WORD)( ((USHORT)(TimeFields.Year-(CSHORT)1980) << 9) |
                         ((USHORT)TimeFields.Month << 5) |
                         (USHORT)TimeFields.Day
                       );

    *lpFatTime = (WORD)( ((USHORT)TimeFields.Hour << 11) |
                         ((USHORT)TimeFields.Minute << 5) |
                         ((USHORT)TimeFields.Second >> 1)
                       );

    return TRUE;
}


BOOL
APIENTRY
DosDateTimeToFileTime(
    WORD wFatDate,
    WORD wFatTime,
    LPFILETIME lpFileTime
    )

/*++

Routine Description:

    This function converts a DOS date and time value, which is
    represented as two 16-bit unsigned integers, into a 64-bit file
    time.

Arguments:

    lpFatDate - Supplies the 16-bit DOS representation of date.

    lpFatTime - Supplies the 16-bit DOS representation of time.

    lpFileTime - Returns the 64-bit file time converted from the DOS
        date and time format.

Return Value:

    TRUE - The Dos date and time were successfully converted.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER FileTime;

    TimeFields.Year         = (CSHORT)((wFatDate & 0xFE00) >> 9)+(CSHORT)1980;
    TimeFields.Month        = (CSHORT)((wFatDate & 0x01E0) >> 5);
    TimeFields.Day          = (CSHORT)((wFatDate & 0x001F) >> 0);
    TimeFields.Hour         = (CSHORT)((wFatTime & 0xF800) >> 11);
    TimeFields.Minute       = (CSHORT)((wFatTime & 0x07E0) >>  5);
    TimeFields.Second       = (CSHORT)((wFatTime & 0x001F) << 1);
    TimeFields.Milliseconds = 0;

    if (RtlTimeFieldsToTime(&TimeFields,&FileTime)) {
        lpFileTime->dwLowDateTime = FileTime.LowPart;
        lpFileTime->dwHighDateTime = FileTime.HighPart;
        return TRUE;
        }
    else {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
        }
}

LONG
APIENTRY
CompareFileTime(
    CONST FILETIME *lpFileTime1,
    CONST FILETIME *lpFileTime2
    )

/*++

Routine Description:

    This function compares two 64-bit file times.

Arguments:

    lpFileTime1 - pointer to a 64-bit file time.

    lpFileTime2 - pointer to a 64-bit file time.

Return Value:

    -1 - *lpFileTime1 <  *lpFileTime2

     0 - *lpFileTime1 == *lpFileTime2

    +1 - *lpFileTime1 >  *lpFileTime2

--*/

{
    ULARGE_INTEGER FileTime1;
    ULARGE_INTEGER FileTime2;

    FileTime1.LowPart = lpFileTime1->dwLowDateTime;
    FileTime1.HighPart = lpFileTime1->dwHighDateTime;
    FileTime2.LowPart = lpFileTime2->dwLowDateTime;
    FileTime2.HighPart = lpFileTime2->dwHighDateTime;
    if (FileTime1.QuadPart < FileTime2.QuadPart) {
        return( -1 );
        }
    else
    if (FileTime1.QuadPart > FileTime2.QuadPart) {
        return( 1 );
        }
    else {
        return( 0 );
        }
}

DWORD
WINAPI
GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    )

/*++

Routine Description:

    This function allows an application to get the current timezone
    parameters These parameters control the Universal time to Local time
    translations.

    All UTC time to Local time translations are based on the following
    formula:

        UTC = LocalTime + Bias

    The return value of this function is the systems best guess of
    the current time zone parameters. This is one of:

        - Unknown

        - Standard Time

        - Daylight Savings Time

    If SetTimeZoneInformation was called without the transition date
    information, Unknown is returned, but the currect bias is used for
    local time translation.  Otherwise, the system will correctly pick
    either daylight savings time or standard time.

    The information returned by this API is identical to the information
    stored in the last successful call to SetTimeZoneInformation.  The
    exception is the Bias field returns the current Bias value in

Arguments:

    lpTimeZoneInformation - Supplies the address of the time zone
        information structure.

Return Value:

    TIME_ZONE_ID_UNKNOWN - The system can not determine the current
        timezone.  This is usually due to a previous call to
        SetTimeZoneInformation where only the Bias was supplied and no
        transition dates were supplied.

    TIME_ZONE_ID_STANDARD - The system is operating in the range covered
        by StandardDate.

    TIME_ZONE_ID_DAYLIGHT - The system is operating in the range covered
        by DaylightDate.

    0xffffffff - The operation failed.  Extended error status is
        available using GetLastError.

--*/
{
    RTL_TIME_ZONE_INFORMATION tzi;
    NTSTATUS Status;

    //
    // get the timezone data from the system
    //

    Status = NtQuerySystemInformation(
                SystemCurrentTimeZoneInformation,
                (PVOID)&tzi,
                sizeof(tzi),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        return 0xffffffff;
        }


    lpTimeZoneInformation->Bias         = tzi.Bias;
    lpTimeZoneInformation->StandardBias = tzi.StandardBias;
    lpTimeZoneInformation->DaylightBias = tzi.DaylightBias;

    RtlMoveMemory(&lpTimeZoneInformation->StandardName,&tzi.StandardName,sizeof(tzi.StandardName));
    RtlMoveMemory(&lpTimeZoneInformation->DaylightName,&tzi.DaylightName,sizeof(tzi.DaylightName));

    lpTimeZoneInformation->StandardDate.wYear         = tzi.StandardStart.Year        ;
    lpTimeZoneInformation->StandardDate.wMonth        = tzi.StandardStart.Month       ;
    lpTimeZoneInformation->StandardDate.wDayOfWeek    = tzi.StandardStart.Weekday     ;
    lpTimeZoneInformation->StandardDate.wDay          = tzi.StandardStart.Day         ;
    lpTimeZoneInformation->StandardDate.wHour         = tzi.StandardStart.Hour        ;
    lpTimeZoneInformation->StandardDate.wMinute       = tzi.StandardStart.Minute      ;
    lpTimeZoneInformation->StandardDate.wSecond       = tzi.StandardStart.Second      ;
    lpTimeZoneInformation->StandardDate.wMilliseconds = tzi.StandardStart.Milliseconds;

    lpTimeZoneInformation->DaylightDate.wYear         = tzi.DaylightStart.Year        ;
    lpTimeZoneInformation->DaylightDate.wMonth        = tzi.DaylightStart.Month       ;
    lpTimeZoneInformation->DaylightDate.wDayOfWeek    = tzi.DaylightStart.Weekday     ;
    lpTimeZoneInformation->DaylightDate.wDay          = tzi.DaylightStart.Day         ;
    lpTimeZoneInformation->DaylightDate.wHour         = tzi.DaylightStart.Hour        ;
    lpTimeZoneInformation->DaylightDate.wMinute       = tzi.DaylightStart.Minute      ;
    lpTimeZoneInformation->DaylightDate.wSecond       = tzi.DaylightStart.Second      ;
    lpTimeZoneInformation->DaylightDate.wMilliseconds = tzi.DaylightStart.Milliseconds;

    return USER_SHARED_DATA->TimeZoneId;
}

BOOL
WINAPI
SetTimeZoneInformation(
    CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
    )

/*++

Routine Description:

    This function allows an application to set timezone parameters into
    their system.  These parameters control the Universal time to Local
    time translations.

    All UTC time to Local time translations are based on the following
    formula:

        UTC = LocalTime + Bias

    This API allows the caller to program the current time zone bias,
    and optionally set up the system to automatically sense daylight
    savings time and standard time transitions.

    The timezone bias information is controlled by the
    TIME_ZONE_INFORMATION structure.

    Bias - Supplies the current bias in minutes for local time
        translation on this machine where LocalTime + Bias = UTC.  This
        is a required filed of this structure.

    StandardName - Supplies an optional abbreviation string associated
        with standard time on this system.  This string is uniterpreted
        and is supplied and used only by callers of this API and of
        GetTimeZoneInformation.

    StandardDate - Supplies an optional date and time (UTC) that
        describes the transition into standard time.  A value of 0 in
        the wMonth field tells the system that StandardDate is not
        specified.  If this field is specified, then DaylightDate must
        also be specified.  Additionally, local time translations done
        during the StandardTime range will be done relative to the
        supplied StandardBias value (added to Bias).

        This field supports two date formats. Absolute form specifies and
        exact date and time when standard time begins. In this form, the
        wYear, wMonth, wDay, wHour, wMinute, wSecond, and wMilliseconds
        of the SYSTEMTIME structure are used to specify an exact date.

        Day-in-month time is specified by setting wYear to 0, setting
        wDayOfWeek to an appropriate weekday, and using wDay in the
        range of 1-5 to select the correct day in the month.  Using this
        notation, the first sunday in april may be specified as can be
        the last thursday in october (5 is equal to "the last").

    StandardBias - Supplies an optional bias value to be used during
        local time translations that occur during Standard Time. This
        field is ignored if StandardDate is not supplied.
         This bias value
        is added to the Bias field to form the Bias used during standard
        time. In most time zones, the value of this field is zero.

    DaylightName - Supplies an optional abbreviation string associated
        with daylight savings time on this system.  This string is
        uniterpreted and is supplied and used only by callers of this
        API and of GetTimeZoneInformation.

    DaylightDate - Supplies an optional date and time (UTC) that
        describes the transition into daylight savings time.  A value of
        0 in the wMonth field tells the system that DaylightDate is not
        specified.  If this field is specified, then StandardDate must
        also be specified.  Additionally, local time translations done
        during the DaylightTime range will be done relative to the
        supplied DaylightBias value (added to Bias). The same dat formats
        supported by StandardDate are supported ib DaylightDate.

    DaylightBias - Supplies an optional bias value to be used during
        local time translations that occur during Daylight Savings Time.
        This field is ignored if DaylightDate is not supplied.  This
        bias value is added to the Bias field to form the Bias used
        during daylight time.  In most time zones, the value of this
        field is -60.

Arguments:

    lpTimeZoneInformation - Supplies the address of the time zone
        information structure.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed. Extended error status is available
        using GetLastError.

--*/
{
    RTL_TIME_ZONE_INFORMATION tzi;
    NTSTATUS Status;

    tzi.Bias            = lpTimeZoneInformation->Bias;
    tzi.StandardBias    = lpTimeZoneInformation->StandardBias;
    tzi.DaylightBias    = lpTimeZoneInformation->DaylightBias;

    RtlMoveMemory(&tzi.StandardName,&lpTimeZoneInformation->StandardName,sizeof(tzi.StandardName));
    RtlMoveMemory(&tzi.DaylightName,&lpTimeZoneInformation->DaylightName,sizeof(tzi.DaylightName));

    tzi.StandardStart.Year         = lpTimeZoneInformation->StandardDate.wYear        ;
    tzi.StandardStart.Month        = lpTimeZoneInformation->StandardDate.wMonth       ;
    tzi.StandardStart.Weekday      = lpTimeZoneInformation->StandardDate.wDayOfWeek   ;
    tzi.StandardStart.Day          = lpTimeZoneInformation->StandardDate.wDay         ;
    tzi.StandardStart.Hour         = lpTimeZoneInformation->StandardDate.wHour        ;
    tzi.StandardStart.Minute       = lpTimeZoneInformation->StandardDate.wMinute      ;
    tzi.StandardStart.Second       = lpTimeZoneInformation->StandardDate.wSecond      ;
    tzi.StandardStart.Milliseconds = lpTimeZoneInformation->StandardDate.wMilliseconds;

    tzi.DaylightStart.Year         = lpTimeZoneInformation->DaylightDate.wYear        ;
    tzi.DaylightStart.Month        = lpTimeZoneInformation->DaylightDate.wMonth       ;
    tzi.DaylightStart.Weekday      = lpTimeZoneInformation->DaylightDate.wDayOfWeek   ;
    tzi.DaylightStart.Day          = lpTimeZoneInformation->DaylightDate.wDay         ;
    tzi.DaylightStart.Hour         = lpTimeZoneInformation->DaylightDate.wHour        ;
    tzi.DaylightStart.Minute       = lpTimeZoneInformation->DaylightDate.wMinute      ;
    tzi.DaylightStart.Second       = lpTimeZoneInformation->DaylightDate.wSecond      ;
    tzi.DaylightStart.Milliseconds = lpTimeZoneInformation->DaylightDate.wMilliseconds;

    Status = RtlSetTimeZoneInformation( &tzi );
    if (!NT_SUCCESS( Status )) {
        BaseSetLastNTError(Status);
        return FALSE;
        }
    //
    // Refresh the system's concept of time
    //

    NtSetSystemTime(NULL,NULL);

    return TRUE;
}

BOOL
WINAPI
GetSystemTimeAdjustment(
    PDWORD lpTimeAdjustment,
    PDWORD lpTimeIncrement,
    PBOOL  lpTimeAdjustmentDisabled
    )

/*++

Routine Description:

    This function is used to support algorithms that want to synchronize
    the time of day (reported via GetSystemTime and GetLocalTime) with
    another time source using a programmed clock adjustment over a
    period of time.

    To facilitate this, the system computes the time of day by adding a
    value to a time of day counter at a periodic interval.  This API
    allows the caller to obtain the periodic interval (clock interrupt
    rate), and the amount added to the time of day with each interrupt.

    A boolean value is also returned which indicates whether or not this
    time adjustment algorithm is even being used.  A value of TRUE
    indicates that adjustment is not being used.  If this is the case,
    the system may attempt to keep the time of day clock in sync using
    its own internal mechanisms.  This may cause time of day to
    periodicly "jump" to the "correct time".


Arguments:

    lpTimeAdjustment - Returns the number of 100ns units added to the
        time of day counter at each clock interrupt.

    lpTimeIncrement - Returns the clock interrupt rate in 100ns units.

    lpTimeAdjustmentDisabled - Returns an indicator which specifies
        whether or not time adjustment is inabled.  A value of TRUE
        indicates that periodic adjustment is disabled
        (*lpTimeAdjustment == *lpTimeIncrement), AND that the system is
        free to serialize time of day using any mechanism it wants.
        This may cause periodic time jumps as the system serializes time
        of day to the "correct time".  A value of false indicates that
        programmed time adjustment is being used to serialize the time
        of day, and that the system will not interfere with this scheme
        and will not attempt to synchronize time of day on its own.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.  Use GetLastError to obtain detailed
        error information.

--*/
{
    NTSTATUS Status;
    SYSTEM_QUERY_TIME_ADJUST_INFORMATION TimeAdjust;
    BOOL b;
    Status = NtQuerySystemInformation(
                SystemTimeAdjustmentInformation,
                &TimeAdjust,
                sizeof(TimeAdjust),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        b = FALSE;
        }
    else {
        *lpTimeAdjustment = TimeAdjust.TimeAdjustment;
        *lpTimeIncrement = TimeAdjust.TimeIncrement;
        *lpTimeAdjustmentDisabled = TimeAdjust.Enable;
        b = TRUE;
        }

    return b;
}

BOOL
WINAPI
SetSystemTimeAdjustment(
    DWORD dwTimeAdjustment,
    BOOL  bTimeAdjustmentDisabled
    )

/*++

Routine Description:

    This function is used to tell the system the parameters it should
    use to periodicaly synchronize time of day with some other source.

    This API supports two modes of operation.

    In the first mode, bTimeAdjustmentDisabled is set to FALSE.  At each
    clock interrupt, the value of dwTimeAdjustment is added to the time
    of day.  The clock interrupt rate may be obtained using
    GetSystemTimeAdjustment, and looking at the returned value of
    lpTimeIncrement.

    In the second mode, bTimeAdjustmentDisabled is set to TRUE.  At each
    clock interrupt, the clock interrupt rate is added to the time of
    day.  The system may also periodically refresh the time of day using
    other internal algorithms.  These may produce "jumps" in time.

    The application must have system-time privilege (the
    SE_SYSTEMTIME_NAME privilege) for this function to succeed.  This
    privilege is disabled by default.  Use the AdjustTokenPrivileges
    function to enable the privilege and again to disable it after the
    time adjustment has been set.

Arguments:

    dwTimeAdjustment - Supplies the value (in 100ns units) that is to be
        added to the time of day at each clock interrupt.

    bTimeAdjustmentDisabled - Supplies a flag which specifies the time
        adjustment mode that the system is to use.  A value of TRUE
        indicates the the system should synchronize time of day using
        its own internal mechanisms.  When this is the case, the value
        of dwTimeAdjustment is ignored.  A value of FALSE indicates that
        the application is in control, and that the value specified by
        dwTimeAdjustment is to be added to the time of day at each clock
        interrupt.

Return Value:

    TRUE - The operation was successful.

    FALSE - The operation failed.  Use GetLastError to obtain detailed
        error information.

--*/

{
    NTSTATUS Status;
    SYSTEM_SET_TIME_ADJUST_INFORMATION TimeAdjust;
    BOOL b;

    b = TRUE;
    TimeAdjust.TimeAdjustment = dwTimeAdjustment;
    TimeAdjust.Enable = (BOOLEAN)bTimeAdjustmentDisabled;
    Status = NtSetSystemInformation(
                SystemTimeAdjustmentInformation,
                &TimeAdjust,
                sizeof(TimeAdjust)
                );
    if ( !NT_SUCCESS(Status) ) {
        BaseSetLastNTError(Status);
        b = FALSE;
        }

    return b;
}

BOOL
WINAPI
SystemTimeToTzSpecificLocalTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpUniversalTime,
    LPSYSTEMTIME lpLocalTime
    )
{

    TIME_ZONE_INFORMATION TziData;
    LPTIME_ZONE_INFORMATION Tzi;
    RTL_TIME_ZONE_INFORMATION tzi;
    LARGE_INTEGER TimeZoneBias;
    LARGE_INTEGER NewTimeZoneBias;
    LARGE_INTEGER LocalCustomBias;
    LARGE_INTEGER StandardTime;
    LARGE_INTEGER DaylightTime;
    LARGE_INTEGER UtcStandardTime;
    LARGE_INTEGER UtcDaylightTime;
    LARGE_INTEGER CurrentUniversalTime;
    LARGE_INTEGER ComputedLocalTime;
    ULONG CurrentTimeZoneId = 0xffffffff;

    //
    // Get the timezone information into a useful format
    //
    if ( !ARGUMENT_PRESENT(lpTimeZoneInformation) ) {

        //
        // Convert universal time to local time using current timezone info
        //
        if (GetTimeZoneInformation(&TziData) == TIME_ZONE_ID_INVALID) {
            return FALSE;
            }
        Tzi = &TziData;
        }
    else {
        Tzi = lpTimeZoneInformation;
        }

    tzi.Bias            = Tzi->Bias;
    tzi.StandardBias    = Tzi->StandardBias;
    tzi.DaylightBias    = Tzi->DaylightBias;

    RtlMoveMemory(&tzi.StandardName,&Tzi->StandardName,sizeof(tzi.StandardName));
    RtlMoveMemory(&tzi.DaylightName,&Tzi->DaylightName,sizeof(tzi.DaylightName));

    tzi.StandardStart.Year         = Tzi->StandardDate.wYear        ;
    tzi.StandardStart.Month        = Tzi->StandardDate.wMonth       ;
    tzi.StandardStart.Weekday      = Tzi->StandardDate.wDayOfWeek   ;
    tzi.StandardStart.Day          = Tzi->StandardDate.wDay         ;
    tzi.StandardStart.Hour         = Tzi->StandardDate.wHour        ;
    tzi.StandardStart.Minute       = Tzi->StandardDate.wMinute      ;
    tzi.StandardStart.Second       = Tzi->StandardDate.wSecond      ;
    tzi.StandardStart.Milliseconds = Tzi->StandardDate.wMilliseconds;

    tzi.DaylightStart.Year         = Tzi->DaylightDate.wYear        ;
    tzi.DaylightStart.Month        = Tzi->DaylightDate.wMonth       ;
    tzi.DaylightStart.Weekday      = Tzi->DaylightDate.wDayOfWeek   ;
    tzi.DaylightStart.Day          = Tzi->DaylightDate.wDay         ;
    tzi.DaylightStart.Hour         = Tzi->DaylightDate.wHour        ;
    tzi.DaylightStart.Minute       = Tzi->DaylightDate.wMinute      ;
    tzi.DaylightStart.Second       = Tzi->DaylightDate.wSecond      ;
    tzi.DaylightStart.Milliseconds = Tzi->DaylightDate.wMilliseconds;

    //
    // convert the input universal time to NT style time
    //
    if ( !SystemTimeToFileTime(lpUniversalTime,(LPFILETIME)&CurrentUniversalTime) ) {
        return FALSE;
        }

    //
    // Get the new timezone bias
    //

    NewTimeZoneBias.QuadPart = Int32x32To64(tzi.Bias*60, 10000000);

    //
    // Now see if we have stored cutover times
    //

    if ( tzi.StandardStart.Month && tzi.DaylightStart.Month ) {

        //
        // We have timezone cutover information. Compute the
        // cutover dates and compute what our current bias
        // is
        //

        if ( !RtlCutoverTimeToSystemTime(
                &tzi.StandardStart,
                &StandardTime,
                &CurrentUniversalTime,
                TRUE
                ) ) {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;
            }

        if ( !RtlCutoverTimeToSystemTime(
                &tzi.DaylightStart,
                &DaylightTime,
                &CurrentUniversalTime,
                TRUE
                ) ) {
            BaseSetLastNTError(STATUS_INVALID_PARAMETER);
            return FALSE;
            }

        //
        // Convert standard time and daylight time to utc
        //

        LocalCustomBias.QuadPart = Int32x32To64(tzi.StandardBias*60, 10000000);
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcDaylightTime.QuadPart = DaylightTime.QuadPart + TimeZoneBias.QuadPart;

        LocalCustomBias.QuadPart = Int32x32To64(tzi.DaylightBias*60, 10000000);
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcStandardTime.QuadPart = StandardTime.QuadPart + TimeZoneBias.QuadPart;

        //
        // If daylight < standard, then time >= daylight and
        // less than standard is daylight
        //

        if ( UtcDaylightTime.QuadPart < UtcStandardTime.QuadPart ) {

            //
            // If today is >= DaylightTime and < StandardTime, then
            // We are in daylight savings time
            //

            if ( (CurrentUniversalTime.QuadPart >= UtcDaylightTime.QuadPart) &&
                 (CurrentUniversalTime.QuadPart < UtcStandardTime.QuadPart) ) {

                CurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
                }
            else {
                CurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
                }
            }
        else {

            //
            // If today is >= StandardTime and < DaylightTime, then
            // We are in standard time
            //

            if ( (CurrentUniversalTime.QuadPart >= UtcStandardTime.QuadPart ) &&
                 (CurrentUniversalTime.QuadPart < UtcDaylightTime.QuadPart ) ) {

                CurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
                }
            else {
                CurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
                }
            }

        //
        // At this point, we know our current timezone and the
        // Universal time of the next cutover.
        //

        LocalCustomBias.QuadPart = Int32x32To64(
                            CurrentTimeZoneId == TIME_ZONE_ID_DAYLIGHT ?
                                tzi.DaylightBias*60 :
                                tzi.StandardBias*60,                // Bias in seconds
                            10000000
                            );

        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;

        }
    else {
        TimeZoneBias = NewTimeZoneBias;
        }

    ComputedLocalTime.QuadPart = CurrentUniversalTime.QuadPart - TimeZoneBias.QuadPart;

    if ( !FileTimeToSystemTime((LPFILETIME)&ComputedLocalTime,lpLocalTime) ) {
        return FALSE;
        }

    return TRUE;
}
