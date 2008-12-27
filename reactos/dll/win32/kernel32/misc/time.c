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

#define LL2FILETIME( ll, pft )\
    (pft)->dwLowDateTime = (UINT)(ll); \
    (pft)->dwHighDateTime = (UINT)((ll) >> 32);
#define FILETIME2LL( pft, ll) \
    ll = (((LONGLONG)((pft)->dwHighDateTime))<<32) + (pft)-> dwLowDateTime ;

static const int MonthLengths[2][12] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

/* STATIC FUNTIONS **********************************************************/

static inline int IsLeapYear(int Year)
{
	return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}

/***********************************************************************
 *  TIME_DayLightCompareDate
 *
 * Compares two dates without looking at the year.
 *
 * PARAMS
 *   date        [in] The local time to compare.
 *   compareDate [in] The daylight savings begin or end date.
 *
 * RETURNS
 *
 *  -1 if date < compareDate
 *   0 if date == compareDate
 *   1 if date > compareDate
 *  -2 if an error occurs
 */
static int TIME_DayLightCompareDate( const SYSTEMTIME *date,
    const SYSTEMTIME *compareDate )
{
    int limit_day, dayinsecs;

    if (date->wMonth < compareDate->wMonth)
        return -1; /* We are in a month before the date limit. */

    if (date->wMonth > compareDate->wMonth)
        return 1; /* We are in a month after the date limit. */

    /* if year is 0 then date is in day-of-week format, otherwise
     * it's absolute date.
     */
    if (compareDate->wYear == 0)
    {
        WORD First;
        /* compareDate->wDay is interpreted as number of the week in the month
         * 5 means: the last week in the month */
        int weekofmonth = compareDate->wDay;
          /* calculate the day of the first DayOfWeek in the month */
        First = ( 6 + compareDate->wDayOfWeek - date->wDayOfWeek + date->wDay 
               ) % 7 + 1;
        limit_day = First + 7 * (weekofmonth - 1);
        /* check needed for the 5th weekday of the month */
        if(limit_day > MonthLengths[date->wMonth==2 && IsLeapYear(date->wYear)]
                [date->wMonth - 1])
            limit_day -= 7;
    }
    else
    {
       limit_day = compareDate->wDay;
    }

    /* convert to seconds */
    limit_day = ((limit_day * 24  + compareDate->wHour) * 60 +
            compareDate->wMinute ) * 60;
    dayinsecs = ((date->wDay * 24  + date->wHour) * 60 +
            date->wMinute ) * 60 + date->wSecond;
    /* and compare */
    return dayinsecs < limit_day ? -1 :
           dayinsecs > limit_day ? 1 :
           0;   /* date is equal to the date limit. */
}

/***********************************************************************
 *  TIME_CompTimeZoneID
 *
 *  Computes the local time bias for a given time and time zone.
 *
 *  PARAMS
 *      pTZinfo     [in] The time zone data.
 *      lpFileTime  [in] The system or local time.
 *      islocal     [in] it is local time.
 *
 *  RETURNS
 *      TIME_ZONE_ID_INVALID    An error occurred
 *      TIME_ZONE_ID_UNKNOWN    There are no transition time known
 *      TIME_ZONE_ID_STANDARD   Current time is standard time
 *      TIME_ZONE_ID_DAYLIGHT   Current time is daylight savings time
 */
static DWORD TIME_CompTimeZoneID ( const TIME_ZONE_INFORMATION *pTZinfo,
    FILETIME *lpFileTime, BOOL islocal )
{
    int ret;
    BOOL beforeStandardDate, afterDaylightDate;
    DWORD retval = TIME_ZONE_ID_INVALID;
    LONGLONG llTime = 0; /* initialized to prevent gcc complaining */
    SYSTEMTIME SysTime;
    FILETIME ftTemp;

    if (pTZinfo->DaylightDate.wMonth != 0)
    {
        /* if year is 0 then date is in day-of-week format, otherwise
         * it's absolute date.
         */
        if (pTZinfo->StandardDate.wMonth == 0 ||
            (pTZinfo->StandardDate.wYear == 0 &&
            (pTZinfo->StandardDate.wDay<1 ||
            pTZinfo->StandardDate.wDay>5 ||
            pTZinfo->DaylightDate.wDay<1 ||
            pTZinfo->DaylightDate.wDay>5)))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return TIME_ZONE_ID_INVALID;
        }

        if (!islocal) {
            FILETIME2LL( lpFileTime, llTime );
            llTime -= ( pTZinfo->Bias + pTZinfo->DaylightBias )
                * (LONGLONG)TICKSPERMIN;
            LL2FILETIME( llTime, &ftTemp)
            lpFileTime = &ftTemp;
        }

        FileTimeToSystemTime(lpFileTime, &SysTime);
        
         /* check for daylight savings */
        ret = TIME_DayLightCompareDate( &SysTime, &pTZinfo->StandardDate);
        if (ret == -2)
          return TIME_ZONE_ID_INVALID;

        beforeStandardDate = ret < 0;

        if (!islocal) {
            llTime -= ( pTZinfo->StandardBias - pTZinfo->DaylightBias )
                * (LONGLONG)TICKSPERMIN;
            LL2FILETIME( llTime, &ftTemp)
            FileTimeToSystemTime(lpFileTime, &SysTime);
        }

        ret = TIME_DayLightCompareDate( &SysTime, &pTZinfo->DaylightDate);
        if (ret == -2)
          return TIME_ZONE_ID_INVALID;

        afterDaylightDate = ret >= 0;

        retval = TIME_ZONE_ID_STANDARD;
        if( pTZinfo->DaylightDate.wMonth <  pTZinfo->StandardDate.wMonth ) {
            /* Northern hemisphere */
            if( beforeStandardDate && afterDaylightDate )
                retval = TIME_ZONE_ID_DAYLIGHT;
        } else    /* Down south */
            if( beforeStandardDate || afterDaylightDate )
            retval = TIME_ZONE_ID_DAYLIGHT;
    } else 
        /* No transition date */
        retval = TIME_ZONE_ID_UNKNOWN;
        
    return retval;
}

/***********************************************************************
 *  TIME_GetTimezoneBias
 *
 *  Calculates the local time bias for a given time zone.
 *
 * PARAMS
 *  pTZinfo    [in]  The time zone data.
 *  lpFileTime [in]  The system or local time.
 *  islocal    [in]  It is local time.
 *  pBias      [out] The calculated bias in minutes.
 *
 * RETURNS
 *  TRUE when the time zone bias was calculated.
 */
static BOOL TIME_GetTimezoneBias( const TIME_ZONE_INFORMATION *pTZinfo,
    FILETIME *lpFileTime, BOOL islocal, LONG *pBias )
{
    LONG bias = pTZinfo->Bias;
    DWORD tzid = TIME_CompTimeZoneID( pTZinfo, lpFileTime, islocal);

    if( tzid == TIME_ZONE_ID_INVALID)
        return FALSE;
    if (tzid == TIME_ZONE_ID_DAYLIGHT)
        bias += pTZinfo->DaylightBias;
    else if (tzid == TIME_ZONE_ID_STANDARD)
        bias += pTZinfo->StandardBias;
    *pBias = bias;
    return TRUE;
}


/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
BOOL
WINAPI
FileTimeToDosDateTime(
		      CONST FILETIME *lpFileTime,
		      LPWORD lpFatDate,
		      LPWORD lpFatTime
		      )
{
   PDOSTIME  pdtime=(PDOSTIME) lpFatTime;
   PDOSDATE  pddate=(PDOSDATE) lpFatDate;
   SYSTEMTIME SystemTime = { 0, 0, 0, 0, 0, 0, 0, 0 };

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
WINAPI
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
WINAPI
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
VOID WINAPI
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
WINAPI
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

  SetLastError(ERROR_INVALID_PARAMETER);
  return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
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
WINAPI
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
WINAPI
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
VOID WINAPI
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
VOID WINAPI
GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
  FILETIME FileTime;

  GetSystemTimeAsFileTime(&FileTime);
  FileTimeToSystemTime(&FileTime, lpSystemTime);
}


/*
 * @implemented
 */
BOOL WINAPI
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
BOOL WINAPI
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
DWORD WINAPI
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
BOOL WINAPI
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
DWORD WINAPI
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
BOOL WINAPI
SystemTimeToTzSpecificLocalTime(
                                CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation,
                                CONST SYSTEMTIME *lpUniversalTime,
                                LPSYSTEMTIME lpLocalTime
                               )
{
 TIME_ZONE_INFORMATION TzInfo;
  FILETIME FileTime;
  LONGLONG llTime;
  LONG lBias;

  if (lpTimeZoneInformation != NULL)
  {
    TzInfo = *lpTimeZoneInformation;
  }
  else
  {
	if (GetTimeZoneInformation(&TzInfo) == TIME_ZONE_ID_INVALID)
	   return FALSE;
  }

  if (!lpUniversalTime || !lpLocalTime)
    return FALSE;

  if (!SystemTimeToFileTime(lpUniversalTime, &FileTime))
     return FALSE;

  FILETIME2LL(&FileTime, llTime)

    if (!TIME_GetTimezoneBias(&TzInfo, &FileTime, FALSE, &lBias))
        return FALSE;

    /* convert minutes to 100-nanoseconds-ticks */
    llTime -= (LONGLONG)lBias * TICKSPERMIN;

    LL2FILETIME( llTime, &FileTime)

	return FileTimeToSystemTime(&FileTime, lpLocalTime);
}


/*
 * @implemented (Wine 13 sep 2008)
 */
BOOL
WINAPI
TzSpecificLocalTimeToSystemTime(
    CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation,
    CONST SYSTEMTIME *lpLocalTime,
    LPSYSTEMTIME lpUniversalTime
    )
{
    FILETIME ft;
    LONG lBias;
    LONGLONG t;
    TIME_ZONE_INFORMATION tzinfo;

    if (lpTimeZoneInformation != NULL)
    {
        tzinfo = *lpTimeZoneInformation;
    }
    else
    {
        if (GetTimeZoneInformation(&tzinfo) == TIME_ZONE_ID_INVALID)
            return FALSE;
    }

    if (!SystemTimeToFileTime(lpLocalTime, &ft))
        return FALSE;
    FILETIME2LL( &ft, t)
    if (!TIME_GetTimezoneBias(&tzinfo, &ft, TRUE, &lBias))
        return FALSE;
    /* convert minutes to 100-nanoseconds-ticks */
    t += (LONGLONG)lBias * TICKSPERMIN;
    LL2FILETIME( t, &ft)
    return FileTimeToSystemTime(&ft, lpUniversalTime);
}


/*
 * @implemented
 */
BOOL WINAPI
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
BOOL WINAPI
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
WINAPI
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
