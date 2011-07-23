/* $Id: time.c 52770 2011-07-22 02:13:57Z ion $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/time.c
 * PURPOSE:         Time conversion functions
 * PROGRAMMER:      Ariadne
 *                  DOSDATE and DOSTIME structures from Onno Hovers
 * UPDATE HISTORY:
 *                  Created 19/01/99
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

typedef struct __DOSTIME
{
    WORD Second:5;
    WORD Minute:6;
    WORD Hour:5;
} DOSTIME, *PDOSTIME;

typedef struct __DOSDATE
{
    WORD Day:5;
    WORD Month:4;
    WORD Year:5;
} DOSDATE, *PDOSDATE;

/* FUNCTIONS ******************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
FileTimeToDosDateTime(CONST FILETIME *lpFileTime,
                      LPWORD lpFatDate,
                      LPWORD lpFatTime)
{
    PDOSTIME  pdtime=(PDOSTIME) lpFatTime;
    PDOSDATE  pddate=(PDOSDATE) lpFatDate;
    SYSTEMTIME SystemTime = { 0, 0, 0, 0, 0, 0, 0, 0 };

    if (lpFileTime == NULL)
        return FALSE;

    if (lpFatDate == NULL)
        return FALSE;

    if (lpFatTime == NULL)
        return FALSE;

    FileTimeToSystemTime(lpFileTime, &SystemTime);

    pdtime->Second = SystemTime.wSecond / 2;
    pdtime->Minute = SystemTime.wMinute;
    pdtime->Hour = SystemTime.wHour;

    pddate->Day = SystemTime.wDay;
    pddate->Month = SystemTime.wMonth;
    pddate->Year = SystemTime.wYear - 1980;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DosDateTimeToFileTime(WORD wFatDate,
                      WORD wFatTime,
                      LPFILETIME lpFileTime)
{
    PDOSTIME  pdtime = (PDOSTIME) &wFatTime;
    PDOSDATE  pddate = (PDOSDATE) &wFatDate;
    SYSTEMTIME SystemTime;

    if (lpFileTime == NULL)
        return FALSE;

    SystemTime.wMilliseconds = 0;
    SystemTime.wSecond = pdtime->Second * 2;
    SystemTime.wMinute = pdtime->Minute;
    SystemTime.wHour = pdtime->Hour;

    SystemTime.wDay = pddate->Day;
    SystemTime.wMonth = pddate->Month;
    SystemTime.wYear = 1980 + pddate->Year;

    if (SystemTimeToFileTime(&SystemTime, lpFileTime) == FALSE)
    {
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
LONG
WINAPI
CompareFileTime(CONST FILETIME *lpFileTime1, CONST FILETIME *lpFileTime2)
{
    if (lpFileTime1 == NULL)
        return 0;
    if (lpFileTime2 == NULL)
        return 0;

    if (*((PLONGLONG)lpFileTime1) > *((PLONGLONG)lpFileTime2))
        return 1;
    else if (*((PLONGLONG)lpFileTime1) < *((PLONGLONG)lpFileTime2))
        return -1;

    return 0;
}


/*
 * @implemented
 */
VOID
WINAPI
GetSystemTimeAsFileTime(PFILETIME lpFileTime)
{
    do
    {
        lpFileTime->dwHighDateTime = SharedUserData->SystemTime.High1Time;
        lpFileTime->dwLowDateTime = SharedUserData->SystemTime.LowPart;
    }
    while (lpFileTime->dwHighDateTime != (DWORD)SharedUserData->SystemTime.High2Time);
}


/*
 * @implemented
 */
BOOL
WINAPI
SystemTimeToFileTime(CONST SYSTEMTIME *lpSystemTime, LPFILETIME lpFileTime)
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

    if (RtlTimeFieldsToTime (&TimeFields, &liTime))
    {
        lpFileTime->dwLowDateTime = liTime.u.LowPart;
        lpFileTime->dwHighDateTime = liTime.u.HighPart;
        return TRUE;
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
FileTimeToSystemTime(CONST FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime)
{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER liTime;

    if (lpFileTime->dwHighDateTime & 0x80000000)
        return FALSE;

    liTime.u.LowPart = lpFileTime->dwLowDateTime;
    liTime.u.HighPart = lpFileTime->dwHighDateTime;

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
FileTimeToLocalFileTime(CONST FILETIME *lpFileTime, LPFILETIME lpLocalFileTime)
{
    LARGE_INTEGER TimeZoneBias;

    do
    {
        TimeZoneBias.HighPart = SharedUserData->TimeZoneBias.High1Time;
        TimeZoneBias.LowPart = SharedUserData->TimeZoneBias.LowPart;
    }
    while (TimeZoneBias.HighPart != SharedUserData->TimeZoneBias.High2Time);

    *((PLONGLONG)lpLocalFileTime) = *((PLONGLONG)lpFileTime) - TimeZoneBias.QuadPart;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
LocalFileTimeToFileTime(CONST FILETIME *lpLocalFileTime, LPFILETIME lpFileTime)
{
    LARGE_INTEGER TimeZoneBias;

    do
    {
        TimeZoneBias.HighPart = SharedUserData->TimeZoneBias.High1Time;
        TimeZoneBias.LowPart = SharedUserData->TimeZoneBias.LowPart;
    }
    while (TimeZoneBias.HighPart != SharedUserData->TimeZoneBias.High2Time);

    *((PLONGLONG)lpFileTime) = *((PLONGLONG)lpLocalFileTime) + TimeZoneBias.QuadPart;

    return TRUE;
}


/*
 * @implemented
 */
VOID
WINAPI
GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
    FILETIME FileTime;
    FILETIME LocalFileTime;

    GetSystemTimeAsFileTime(&FileTime);
    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, lpSystemTime);
}


/*
 * @implemented
 */
VOID
WINAPI
GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
    FILETIME FileTime;

    GetSystemTimeAsFileTime(&FileTime);
    FileTimeToSystemTime(&FileTime, lpSystemTime);
}


/*
 * @implemented
 */
BOOL
WINAPI
SetLocalTime(CONST SYSTEMTIME *lpSystemTime)
{
    FILETIME LocalFileTime;
    LARGE_INTEGER FileTime;
    NTSTATUS Status;

    SystemTimeToFileTime(lpSystemTime, &LocalFileTime);
    LocalFileTimeToFileTime(&LocalFileTime, (FILETIME *)&FileTime);
    Status = NtSetSystemTime(&FileTime, &FileTime);
    if (!NT_SUCCESS(Status))
        return FALSE;
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSystemTime(CONST SYSTEMTIME *lpSystemTime)
{
    LARGE_INTEGER NewSystemTime;
    NTSTATUS Status;

    SystemTimeToFileTime(lpSystemTime, (PFILETIME)&NewSystemTime);
    Status = NtSetSystemTime(&NewSystemTime, &NewSystemTime);
    if (!NT_SUCCESS(Status))
        return FALSE;
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

    while (TRUE)
    {
        TickCount.HighPart = (ULONG)SharedUserData->TickCount.High1Time;
        TickCount.LowPart = SharedUserData->TickCount.LowPart;

        if (TickCount.HighPart == (ULONG)SharedUserData->TickCount.High2Time)
            break;

        YieldProcessor();
    }

    return (ULONG)((UInt32x32To64(TickCount.LowPart, SharedUserData->TickCountMultiplier) >> 24) +
                    UInt32x32To64((TickCount.HighPart << 8) & 0xFFFFFFFF, SharedUserData->TickCountMultiplier));

}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetClientTimeZoneInformation(
    CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation)
{
    STUB;
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetSystemTimeAdjustment(PDWORD lpTimeAdjustment,
                        PDWORD lpTimeIncrement,
                        PBOOL lpTimeAdjustmentDisabled)
{
    SYSTEM_QUERY_TIME_ADJUST_INFORMATION Buffer;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemTimeAdjustmentInformation,
                                      &Buffer,
                                      sizeof(SYSTEM_QUERY_TIME_ADJUST_INFORMATION),
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpTimeAdjustment = (DWORD)Buffer.TimeAdjustment;
    *lpTimeIncrement = (DWORD)Buffer.TimeIncrement;
    *lpTimeAdjustmentDisabled = (BOOL)Buffer.Enable;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetSystemTimeAdjustment(DWORD dwTimeAdjustment,
                        BOOL bTimeAdjustmentDisabled)
{
    NTSTATUS Status;
    SYSTEM_SET_TIME_ADJUST_INFORMATION Buffer;

    Buffer.TimeAdjustment = (ULONG)dwTimeAdjustment;
    Buffer.Enable = (BOOLEAN)bTimeAdjustmentDisabled;

    Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
                                    &Buffer,
                                    sizeof(SYSTEM_SET_TIME_ADJUST_INFORMATION));
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
GetSystemTimes(LPFILETIME lpIdleTime,
               LPFILETIME lpKernelTime,
               LPFILETIME lpUserTime)
{
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SysProcPerfInfo;
    NTSTATUS Status;

    Status = ZwQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      &SysProcPerfInfo,
                                      sizeof(SysProcPerfInfo),
                                      NULL);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }
/*
    Good only for one processor system.
 */

    lpIdleTime->dwLowDateTime = SysProcPerfInfo.IdleTime.LowPart;
    lpIdleTime->dwHighDateTime = SysProcPerfInfo.IdleTime.HighPart;

    lpKernelTime->dwLowDateTime = SysProcPerfInfo.KernelTime.LowPart;
    lpKernelTime->dwHighDateTime = SysProcPerfInfo.KernelTime.HighPart;

    lpUserTime->dwLowDateTime = SysProcPerfInfo.UserTime.LowPart;
    lpUserTime->dwHighDateTime = SysProcPerfInfo.UserTime.HighPart;

    return TRUE;
}



/* EOF */
