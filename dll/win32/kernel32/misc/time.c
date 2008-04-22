/* $Id$
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

#define TICKSPERMIN        600000000

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
FileTimeToDosDateTime(
		      CONST FILETIME *lpFileTime,
		      LPWORD lpFatDate,
		      LPWORD lpFatTime
		      )
{
   PDOSTIME  pdtime=(PDOSTIME) lpFatTime;
   PDOSDATE  pddate=(PDOSDATE) lpFatDate;
   SYSTEMTIME SystemTime = { 0 };

   if ( lpFileTime == NULL )
		return FALSE;

   if ( lpFatDate == NULL )
	    return FALSE;

   if ( lpFatTime == NULL )
	    return FALSE;

   FileTimeToSystemTime(
		     lpFileTime,
		     &SystemTime
		     );

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
STDCALL
DosDateTimeToFileTime(
		      WORD wFatDate,
		      WORD wFatTime,
		      LPFILETIME lpFileTime
		      )
{
   PDOSTIME  pdtime = (PDOSTIME) &wFatTime;
   PDOSDATE  pddate = (PDOSDATE) &wFatDate;
   SYSTEMTIME SystemTime;

   if ( lpFileTime == NULL )
		return FALSE;

   SystemTime.wMilliseconds = 0;
   SystemTime.wSecond = pdtime->Second * 2;
   SystemTime.wMinute = pdtime->Minute;
   SystemTime.wHour = pdtime->Hour;

   SystemTime.wDay = pddate->Day;
   SystemTime.wMonth = pddate->Month;
   SystemTime.wYear = 1980 + pddate->Year;

   SystemTimeToFileTime(&SystemTime,lpFileTime);

   return TRUE;
}


/*
 * @implemented
 */
LONG
STDCALL
CompareFileTime(
		CONST FILETIME *lpFileTime1,
		CONST FILETIME *lpFileTime2
		)
{
  if ( lpFileTime1 == NULL )
	return 0;
  if ( lpFileTime2 == NULL )
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
VOID STDCALL
GetSystemTimeAsFileTime (PFILETIME lpFileTime)
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
STDCALL
SystemTimeToFileTime(
    CONST SYSTEMTIME *  lpSystemTime,
    LPFILETIME  lpFileTime
   )

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
  return FALSE;
}


/*
 * @implemented
 */
BOOL
STDCALL
FileTimeToSystemTime(
		     CONST FILETIME *lpFileTime,
		     LPSYSTEMTIME lpSystemTime
		     )
{
  TIME_FIELDS TimeFields;
  LARGE_INTEGER liTime;

  if(lpFileTime->dwHighDateTime & 0x80000000)
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
STDCALL
FileTimeToLocalFileTime(
			CONST FILETIME *lpFileTime,
			LPFILETIME lpLocalFileTime
			)
{
  LARGE_INTEGER TimeZoneBias;

  do
    {
      TimeZoneBias.HighPart = SharedUserData->TimeZoneBias.High1Time;
      TimeZoneBias.LowPart = SharedUserData->TimeZoneBias.LowPart;
    }
  while (TimeZoneBias.HighPart != SharedUserData->TimeZoneBias.High2Time);

  *((PLONGLONG)lpLocalFileTime) =
    *((PLONGLONG)lpFileTime) - TimeZoneBias.QuadPart;

  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
LocalFileTimeToFileTime(
			CONST FILETIME *lpLocalFileTime,
			LPFILETIME lpFileTime
			)
{
  LARGE_INTEGER TimeZoneBias;

  do
    {
      TimeZoneBias.HighPart = SharedUserData->TimeZoneBias.High1Time;
      TimeZoneBias.LowPart = SharedUserData->TimeZoneBias.LowPart;
    }
  while (TimeZoneBias.HighPart != SharedUserData->TimeZoneBias.High2Time);

  *((PLONGLONG)lpFileTime) =
    *((PLONGLONG)lpLocalFileTime) + TimeZoneBias.QuadPart;

  return TRUE;
}


/*
 * @implemented
 */
VOID STDCALL
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
VOID STDCALL
GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
  FILETIME FileTime;

  GetSystemTimeAsFileTime(&FileTime);
  FileTimeToSystemTime(&FileTime, lpSystemTime);
}


/*
 * @implemented
 */
BOOL STDCALL
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
BOOL STDCALL
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
DWORD STDCALL
GetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
   NTSTATUS Status;

   DPRINT("GetTimeZoneInformation()\n");

   Status = NtQuerySystemInformation(SystemCurrentTimeZoneInformation,
				     lpTimeZoneInformation,
				     sizeof(TIME_ZONE_INFORMATION),
				     NULL);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return TIME_ZONE_ID_INVALID;
     }

   return(SharedUserData->TimeZoneId);
}


/*
 * @implemented
 */
BOOL STDCALL
SetTimeZoneInformation(CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation)
{
   NTSTATUS Status;

   DPRINT("SetTimeZoneInformation()\n");

   Status = RtlSetTimeZoneInformation((LPTIME_ZONE_INFORMATION)lpTimeZoneInformation);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("RtlSetTimeZoneInformation() failed (Status %lx)\n", Status);
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   Status = NtSetSystemInformation(SystemCurrentTimeZoneInformation,
				   (PVOID)lpTimeZoneInformation,
				   sizeof(TIME_ZONE_INFORMATION));
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("NtSetSystemInformation() failed (Status %lx)\n", Status);
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}


/*
 * @implemented
 */
DWORD STDCALL
GetTickCount(VOID)
{
  return (DWORD)((ULONGLONG)SharedUserData->TickCountLowDeprecated * SharedUserData->TickCountMultiplier / 16777216);
}


/*
 * @implemented
 */
ULONGLONG WINAPI
GetTickCount64(VOID)
{
    return (ULONGLONG)SharedUserData->TickCountLowDeprecated * (ULONGLONG)SharedUserData->TickCountMultiplier / 16777216;
}


/*
 * @implemented
 */
BOOL STDCALL
SystemTimeToTzSpecificLocalTime(
                                CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation,
                                CONST SYSTEMTIME *lpUniversalTime,
                                LPSYSTEMTIME lpLocalTime
                               )
{
  TIME_ZONE_INFORMATION TimeZoneInformation;
  LPTIME_ZONE_INFORMATION lpTzInfo;
  LARGE_INTEGER FileTime;

  if (!lpTimeZoneInformation)
  {
    GetTimeZoneInformation(&TimeZoneInformation);
    lpTzInfo = &TimeZoneInformation;
  }
  else
    lpTzInfo = (LPTIME_ZONE_INFORMATION)lpTimeZoneInformation;

  if (!lpUniversalTime)
    return FALSE;

  if (!lpLocalTime)
    return FALSE;

  SystemTimeToFileTime(lpUniversalTime, (PFILETIME)&FileTime);
  FileTime.QuadPart -= (lpTzInfo->Bias * TICKSPERMIN);
  FileTimeToSystemTime((PFILETIME)&FileTime, lpLocalTime);

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
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
	SetLastErrorByStatus(Status);
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
BOOL STDCALL
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
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetSystemTimes(
    LPFILETIME lpIdleTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
    )
{
   SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SysProcPerfInfo;
   NTSTATUS Status;

   Status = ZwQuerySystemInformation(SystemProcessorPerformanceInformation,
                                     &SysProcPerfInfo,
                                     sizeof(SysProcPerfInfo),
                                     NULL);

   if (!NT_SUCCESS(Status))
     {
        SetLastErrorByStatus(Status);
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
