/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/time.c
 * PURPOSE:         Time conversion functions
 * PROGRAMMER:      Ariadne
 *                  DOSDATE and DOSTIME structures from Onno Hovers
 * UPDATE HISTORY:
 *                  Created 19/01/99
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
IsTimeZoneRedirectionEnabled(VOID)
{
    /* Return if a TS Timezone ID is active */
    return (BaseStaticServerData->TermsrvClientTimeZoneId != TIME_ZONE_ID_INVALID);
}

/*
 * @implemented
 */
BOOL
WINAPI
FileTimeToDosDateTime(IN CONST FILETIME *lpFileTime,
                      OUT LPWORD lpFatDate,
                      OUT LPWORD lpFatTime)
{
    LARGE_INTEGER FileTime;
    TIME_FIELDS TimeFields;

    FileTime.HighPart = lpFileTime->dwHighDateTime;
    FileTime.LowPart = lpFileTime->dwLowDateTime;

    if (FileTime.QuadPart < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlTimeToTimeFields(&FileTime, &TimeFields);
    if ((TimeFields.Year < 1980) || (TimeFields.Year > 2107))
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    *lpFatDate = (TimeFields.Day) |
                 (TimeFields.Month << 5) |
                 ((TimeFields.Year - 1980) << 9);
    *lpFatTime = (TimeFields.Second >> 1) |
                 (TimeFields.Minute << 5) |
                 (TimeFields.Hour << 16);

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DosDateTimeToFileTime(IN WORD wFatDate,
                      IN WORD wFatTime,
                      OUT LPFILETIME lpFileTime)
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER SystemTime;

    TimeFields.Year = (wFatDate >> 9) + 1980;
    TimeFields.Month = (wFatDate >> 5) & 0xF;
    TimeFields.Day = (wFatDate & 0x1F);
    TimeFields.Hour = (wFatTime >> 11);
    TimeFields.Minute = (wFatTime >> 5) & 0x3F;
    TimeFields.Second = (wFatTime & 0x1F) << 1;
    TimeFields.Milliseconds = 0;

    if (RtlTimeFieldsToTime(&TimeFields, &SystemTime))
    {
        lpFileTime->dwLowDateTime = SystemTime.LowPart;
        lpFileTime->dwHighDateTime = SystemTime.HighPart;
        return TRUE;
    }

    BaseSetLastNTError(STATUS_INVALID_PARAMETER);
    return FALSE;
}

/*
 * @implemented
 */
LONG
WINAPI
CompareFileTime(IN CONST FILETIME *lpFileTime1,
                IN CONST FILETIME *lpFileTime2)
{
    LARGE_INTEGER Time1, Time2, Diff;

    Time1.LowPart = lpFileTime1->dwLowDateTime;
    Time2.LowPart = lpFileTime2->dwLowDateTime;
    Time1.HighPart = lpFileTime1->dwHighDateTime;
    Time2.HighPart = lpFileTime2->dwHighDateTime;

    Diff.QuadPart = Time1.QuadPart - Time2.QuadPart;

    if (Diff.HighPart < 0) return -1;
    if (Diff.QuadPart == 0) return 0;
    return 1;
}

/*
 * @implemented
 */
VOID
WINAPI
GetSystemTimeAsFileTime(OUT PFILETIME lpFileTime)
{
    LARGE_INTEGER SystemTime;

    do
    {
        SystemTime.HighPart = SharedUserData->SystemTime.High1Time;
        SystemTime.LowPart = SharedUserData->SystemTime.LowPart;
    }
    while (SystemTime.HighPart != SharedUserData->SystemTime.High2Time);

    lpFileTime->dwLowDateTime = SystemTime.LowPart;
    lpFileTime->dwHighDateTime = SystemTime.HighPart;
}

/*
 * @implemented
 */
BOOL
WINAPI
SystemTimeToFileTime(IN CONST SYSTEMTIME *lpSystemTime,
                     OUT LPFILETIME lpFileTime)
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER liTime;

    TimeFields.Year = lpSystemTime->wYear;
    TimeFields.Month = lpSystemTime->wMonth;
    TimeFields.Day = lpSystemTime->wDay;
    TimeFields.Hour = lpSystemTime->wHour;
    TimeFields.Minute = lpSystemTime->wMinute;
    TimeFields.Second = lpSystemTime->wSecond;
    TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

    if (RtlTimeFieldsToTime(&TimeFields, &liTime))
    {
        lpFileTime->dwLowDateTime = liTime.u.LowPart;
        lpFileTime->dwHighDateTime = liTime.u.HighPart;
        return TRUE;
    }

    BaseSetLastNTError(STATUS_INVALID_PARAMETER);
    return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
FileTimeToSystemTime(IN CONST FILETIME *lpFileTime,
                     OUT LPSYSTEMTIME lpSystemTime)
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER liTime;

    liTime.u.LowPart = lpFileTime->dwLowDateTime;
    liTime.u.HighPart = lpFileTime->dwHighDateTime;
    if (liTime.QuadPart < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlTimeToTimeFields(&liTime, &TimeFields);

    lpSystemTime->wYear = TimeFields.Year;
    lpSystemTime->wMonth = TimeFields.Month;
    lpSystemTime->wDay = TimeFields.Day;
    lpSystemTime->wHour = TimeFields.Hour;
    lpSystemTime->wMinute = TimeFields.Minute;
    lpSystemTime->wSecond = TimeFields.Second;
    lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
    lpSystemTime->wDayOfWeek = TimeFields.Weekday;

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
FileTimeToLocalFileTime(IN CONST FILETIME *lpFileTime,
                        OUT LPFILETIME lpLocalFileTime)
{
    LARGE_INTEGER TimeZoneBias, FileTime;
    volatile KSYSTEM_TIME *TimePtr;

    TimePtr = IsTimeZoneRedirectionEnabled() ?
              &BaseStaticServerData->ktTermsrvClientBias :
              &SharedUserData->TimeZoneBias;
    do
    {
        TimeZoneBias.HighPart = TimePtr->High1Time;
        TimeZoneBias.LowPart = TimePtr->LowPart;
    }
    while (TimeZoneBias.HighPart != TimePtr->High2Time);

    FileTime.LowPart = lpFileTime->dwLowDateTime;
    FileTime.HighPart = lpFileTime->dwHighDateTime;

    FileTime.QuadPart -= TimeZoneBias.QuadPart;

    lpLocalFileTime->dwLowDateTime = FileTime.LowPart;
    lpLocalFileTime->dwHighDateTime = FileTime.HighPart;

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
LocalFileTimeToFileTime(IN CONST FILETIME *lpLocalFileTime,
                        OUT LPFILETIME lpFileTime)
{
    LARGE_INTEGER TimeZoneBias, FileTime;
    volatile KSYSTEM_TIME *TimePtr;

    TimePtr = IsTimeZoneRedirectionEnabled() ?
              &BaseStaticServerData->ktTermsrvClientBias :
              &SharedUserData->TimeZoneBias;

    do
    {
        TimeZoneBias.HighPart = TimePtr->High1Time;
        TimeZoneBias.LowPart = TimePtr->LowPart;
    }
    while (TimeZoneBias.HighPart != TimePtr->High2Time);

    FileTime.LowPart = lpLocalFileTime->dwLowDateTime;
    FileTime.HighPart = lpLocalFileTime->dwHighDateTime;

    FileTime.QuadPart += TimeZoneBias.QuadPart;

    lpFileTime->dwLowDateTime = FileTime.LowPart;
    lpFileTime->dwHighDateTime = FileTime.HighPart;

    return TRUE;
}

/*
 * @implemented
 */
VOID
WINAPI
GetLocalTime(OUT LPSYSTEMTIME lpSystemTime)
{
    LARGE_INTEGER SystemTime, TimeZoneBias;
    TIME_FIELDS TimeFields;
    volatile KSYSTEM_TIME *TimePtr;

    do
    {
        SystemTime.HighPart = SharedUserData->SystemTime.High1Time;
        SystemTime.LowPart = SharedUserData->SystemTime.LowPart;
    }
    while (SystemTime.HighPart != SharedUserData->SystemTime.High2Time);

    TimePtr = IsTimeZoneRedirectionEnabled() ?
              &BaseStaticServerData->ktTermsrvClientBias :
              &SharedUserData->TimeZoneBias;
    do
    {
        TimeZoneBias.HighPart = TimePtr->High1Time;
        TimeZoneBias.LowPart = TimePtr->LowPart;
    }
    while (TimeZoneBias.HighPart != TimePtr->High2Time);

    SystemTime.QuadPart -= TimeZoneBias.QuadPart;
    RtlTimeToTimeFields(&SystemTime, &TimeFields);

    lpSystemTime->wYear = TimeFields.Year;
    lpSystemTime->wMonth = TimeFields.Month;
    lpSystemTime->wDay = TimeFields.Day;
    lpSystemTime->wHour = TimeFields.Hour;
    lpSystemTime->wMinute = TimeFields.Minute;
    lpSystemTime->wSecond = TimeFields.Second;
    lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
    lpSystemTime->wDayOfWeek = TimeFields.Weekday;
}

/*
 * @implemented
 */
VOID
WINAPI
GetSystemTime(OUT LPSYSTEMTIME lpSystemTime)
{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;

    do
    {
        SystemTime.HighPart = SharedUserData->SystemTime.High1Time;
        SystemTime.LowPart = SharedUserData->SystemTime.LowPart;
    }
    while (SystemTime.HighPart != SharedUserData->SystemTime.High2Time);

    RtlTimeToTimeFields(&SystemTime, &TimeFields);

    lpSystemTime->wYear = TimeFields.Year;
    lpSystemTime->wMonth = TimeFields.Month;
    lpSystemTime->wDay = TimeFields.Day;
    lpSystemTime->wHour = TimeFields.Hour;
    lpSystemTime->wMinute = TimeFields.Minute;
    lpSystemTime->wSecond = TimeFields.Second;
    lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
    lpSystemTime->wDayOfWeek = TimeFields.Weekday;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetLocalTime(IN CONST SYSTEMTIME *lpSystemTime)
{
    LARGE_INTEGER NewSystemTime, TimeZoneBias;
    NTSTATUS Status;
    ULONG Privilege = SE_SYSTEMTIME_PRIVILEGE;
    TIME_FIELDS TimeFields;
    PVOID State;
    volatile KSYSTEM_TIME *TimePtr;

    TimePtr = IsTimeZoneRedirectionEnabled() ?
              &BaseStaticServerData->ktTermsrvClientBias :
              &SharedUserData->TimeZoneBias;
    do
    {
        TimeZoneBias.HighPart = TimePtr->High1Time;
        TimeZoneBias.LowPart = TimePtr->LowPart;
    }
    while (TimeZoneBias.HighPart != TimePtr->High2Time);

    TimeFields.Year = lpSystemTime->wYear;
    TimeFields.Month = lpSystemTime->wMonth;
    TimeFields.Day = lpSystemTime->wDay;
    TimeFields.Hour = lpSystemTime->wHour;
    TimeFields.Minute = lpSystemTime->wMinute;
    TimeFields.Second = lpSystemTime->wSecond;
    TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

    if (!RtlTimeFieldsToTime(&TimeFields, &NewSystemTime))
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    NewSystemTime.QuadPart += TimeZoneBias.QuadPart;

    Status = RtlAcquirePrivilege(&Privilege, 1, 0, &State);
    if (NT_SUCCESS(Status))
    {
        Status = NtSetSystemTime(&NewSystemTime, NULL);
        RtlReleasePrivilege(State);
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetSystemTime(IN CONST SYSTEMTIME *lpSystemTime)
{
    LARGE_INTEGER NewSystemTime;
    NTSTATUS Status;
    ULONG Privilege = SE_SYSTEMTIME_PRIVILEGE;
    TIME_FIELDS TimeFields;
    PVOID State;

    TimeFields.Year = lpSystemTime->wYear;
    TimeFields.Month = lpSystemTime->wMonth;
    TimeFields.Day = lpSystemTime->wDay;
    TimeFields.Hour = lpSystemTime->wHour;
    TimeFields.Minute = lpSystemTime->wMinute;
    TimeFields.Second = lpSystemTime->wSecond;
    TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

    if (!RtlTimeFieldsToTime(&TimeFields, &NewSystemTime))
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    Status = RtlAcquirePrivilege(&Privilege, 1, 0, &State);
    if (NT_SUCCESS(Status))
    {
        Status = NtSetSystemTime(&NewSystemTime, NULL);
        RtlReleasePrivilege(State);
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetTickCount(VOID)
{
    ULARGE_INTEGER TickCount;

#ifdef _WIN64
    TickCount.QuadPart = *((volatile ULONG64*)&SharedUserData->TickCount);
#else
    while (TRUE)
    {
        TickCount.HighPart = (ULONG)SharedUserData->TickCount.High1Time;
        TickCount.LowPart = SharedUserData->TickCount.LowPart;

        if (TickCount.HighPart == (ULONG)SharedUserData->TickCount.High2Time)
            break;

        YieldProcessor();
    }
#endif

    return (ULONG)((UInt32x32To64(TickCount.LowPart,
                                  SharedUserData->TickCountMultiplier) >> 24) +
                    UInt32x32To64((TickCount.HighPart << 8) & 0xFFFFFFFF,
                                  SharedUserData->TickCountMultiplier));

}

/*
 * @implemented
 */
BOOL
WINAPI
GetSystemTimeAdjustment(OUT PDWORD lpTimeAdjustment,
                        OUT PDWORD lpTimeIncrement,
                        OUT PBOOL lpTimeAdjustmentDisabled)
{
    SYSTEM_QUERY_TIME_ADJUST_INFORMATION TimeInfo;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemTimeAdjustmentInformation,
                                      &TimeInfo,
                                      sizeof(TimeInfo),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpTimeAdjustment = (DWORD)TimeInfo.TimeAdjustment;
    *lpTimeIncrement = (DWORD)TimeInfo.TimeIncrement;
    *lpTimeAdjustmentDisabled = (BOOL)TimeInfo.Enable;

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetSystemTimeAdjustment(IN DWORD dwTimeAdjustment,
                        IN BOOL bTimeAdjustmentDisabled)
{
    NTSTATUS Status;
    SYSTEM_SET_TIME_ADJUST_INFORMATION TimeInfo;

    TimeInfo.TimeAdjustment = (ULONG)dwTimeAdjustment;
    TimeInfo.Enable = (BOOLEAN)bTimeAdjustmentDisabled;

    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &TimeInfo,
                                    sizeof(TimeInfo));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetSystemTimes(OUT LPFILETIME lpIdleTime OPTIONAL,
               OUT LPFILETIME lpKernelTime OPTIONAL,
               OUT LPFILETIME lpUserTime OPTIONAL)
{
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION ProcPerfInfo;
    LARGE_INTEGER TotalUserTime, TotalKernTime, TotalIdleTime;
    SIZE_T BufferSize, ReturnLength;
    CCHAR i;
    NTSTATUS Status;

    TotalUserTime.QuadPart = TotalKernTime.QuadPart = TotalIdleTime.QuadPart = 0;

    BufferSize = sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) *
                 BaseStaticServerData->SysInfo.NumberOfProcessors;

    ProcPerfInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferSize);
    if (!ProcPerfInfo)
    {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return FALSE;
    }

    Status = NtQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      ProcPerfInfo,
                                      BufferSize,
                                      &ReturnLength);
    if ((NT_SUCCESS(Status)) && (ReturnLength == BufferSize))
    {
        if (lpIdleTime)
        {
            for (i = 0; i < BaseStaticServerData->SysInfo.NumberOfProcessors; i++)
            {
                TotalIdleTime.QuadPart += ProcPerfInfo[i].IdleTime.QuadPart;
            }

            lpIdleTime->dwLowDateTime = TotalIdleTime.LowPart;
            lpIdleTime->dwHighDateTime = TotalIdleTime.HighPart;
        }

        if (lpKernelTime)
        {
            for (i = 0; i < BaseStaticServerData->SysInfo.NumberOfProcessors; i++)
            {
                TotalKernTime.QuadPart += ProcPerfInfo[i].KernelTime.QuadPart;
            }

            lpKernelTime->dwLowDateTime = TotalKernTime.LowPart;
            lpKernelTime->dwHighDateTime = TotalKernTime.HighPart;
        }

        if (lpUserTime)
        {
            for (i = 0; i < BaseStaticServerData->SysInfo.NumberOfProcessors; i++)
            {
                TotalUserTime.QuadPart += ProcPerfInfo[i].UserTime.QuadPart;
            }

            lpUserTime->dwLowDateTime = TotalUserTime.LowPart;
            lpUserTime->dwHighDateTime = TotalUserTime.HighPart;
        }
    }
    else if (NT_SUCCESS(Status))
    {
         Status = STATUS_INTERNAL_ERROR;
    }

    RtlFreeHeap(RtlGetProcessHeap(), 0, ProcPerfInfo);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetClientTimeZoneInformation(IN CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation)
{
    STUB;
    return 0;
}

/* EOF */
