/* $Id: time.c,v 1.20 2003/02/12 00:39:31 hyperion Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/time.c
 * PURPOSE:         Time conversion functions
 * PROGRAMMER:      Boudewijn ( ariadne@xs4all.nl)
 *                  DOSDATE and DOSTIME structures from Onno Hovers
 * UPDATE HISTORY:
 *                  Created 19/01/99
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>

/* TYPES *********************************************************************/

typedef struct __DOSTIME
{
   WORD	Second:5;
   WORD	Minute:6;
   WORD Hour:5;
} DOSTIME, *PDOSTIME;

typedef struct __DOSDATE
{
   WORD	Day:5;
   WORD	Month:4;
   WORD Year:5;
} DOSDATE, *PDOSDATE;

#define TICKSPERMIN        600000000

/* FUNCTIONS ****************************************************************/

WINBOOL
STDCALL
FileTimeToDosDateTime(
		      CONST FILETIME *lpFileTime,
		      LPWORD lpFatDate,
		      LPWORD lpFatTime
		      )
{
   PDOSTIME  pdtime=(PDOSTIME) lpFatTime;
   PDOSDATE  pddate=(PDOSDATE) lpFatDate;
   SYSTEMTIME SystemTime;

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

   pdtime->Second = SystemTime.wSecond;
   pdtime->Minute = SystemTime.wMinute;
   pdtime->Hour = SystemTime.wHour;

   pddate->Day = SystemTime.wDay;
   pddate->Month = SystemTime.wMonth;
   pddate->Year = SystemTime.wYear - 1980;

   return TRUE; 
}

WINBOOL
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
   SystemTime.wSecond = pdtime->Second;
   SystemTime.wMinute = pdtime->Minute;
   SystemTime.wHour = pdtime->Hour;

   SystemTime.wDay = pddate->Day;
   SystemTime.wMonth = pddate->Month;
   SystemTime.wYear = 1980 + pddate->Year;

   SystemTimeToFileTime(&SystemTime,lpFileTime);
   
   return TRUE;
}

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

VOID
STDCALL 
GetSystemTimeAsFileTime(PFILETIME lpFileTime)
{
   NtQuerySystemTime ((TIME *)lpFileTime);
}

WINBOOL 
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

//   dwDayOfWeek = RtlLargeIntegerDivide(FileTime,LIDAY,&dwRemDay);
//   lpSystemTime->wDayOfWeek = 1 + GET_LARGE_INTEGER_LOW_PART(dwDayOfWeek) % 7;

  

WINBOOL
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


WINBOOL
STDCALL
FileTimeToLocalFileTime(
			CONST FILETIME *lpFileTime,
			LPFILETIME lpLocalFileTime
			)
{
  // FIXME: include time bias
  *((PLONGLONG)lpLocalFileTime) = *((PLONGLONG)lpFileTime);

  return TRUE;
}

WINBOOL
STDCALL
LocalFileTimeToFileTime(
			CONST FILETIME *lpLocalFileTime,
			LPFILETIME lpFileTime
			)
{
  // FIXME: include time bias
  *((PLONGLONG)lpFileTime) = *((PLONGLONG)lpLocalFileTime);

  return TRUE;
}


VOID STDCALL
GetLocalTime(LPSYSTEMTIME lpSystemTime)
{
  FILETIME FileTime;
  FILETIME LocalFileTime;

  NtQuerySystemTime ((TIME*)&FileTime);
  FileTimeToLocalFileTime (&FileTime, &LocalFileTime);
  FileTimeToSystemTime (&LocalFileTime, lpSystemTime);
}


VOID STDCALL
GetSystemTime(LPSYSTEMTIME lpSystemTime)
{
  FILETIME FileTime;

  NtQuerySystemTime ((TIME*)&FileTime);
  FileTimeToSystemTime (&FileTime, lpSystemTime);
}


WINBOOL STDCALL
SetLocalTime(CONST SYSTEMTIME *lpSystemTime)
{
  FILETIME LocalFileTime;
  LARGE_INTEGER FileTime;
  NTSTATUS errCode;

  SystemTimeToFileTime (lpSystemTime, &LocalFileTime);
  LocalFileTimeToFileTime (&LocalFileTime, (FILETIME *)&FileTime);
  errCode = NtSetSystemTime (&FileTime, &FileTime);
  if (!NT_SUCCESS(errCode))
    return FALSE;
  return TRUE;
}


WINBOOL STDCALL
SetSystemTime(CONST SYSTEMTIME *lpSystemTime)
{
  LARGE_INTEGER NewSystemTime;
  NTSTATUS errCode;

  SystemTimeToFileTime (lpSystemTime, (PFILETIME)&NewSystemTime);
  errCode = NtSetSystemTime (&NewSystemTime, &NewSystemTime);
  if (!NT_SUCCESS(errCode))
    return FALSE;
  return TRUE;
}


DWORD STDCALL
GetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation)
{
   TIME_ZONE_INFORMATION TimeZoneInformation;
   NTSTATUS Status;

   DPRINT("GetTimeZoneInformation()\n");

   Status = NtQuerySystemInformation(SystemCurrentTimeZoneInformation,
				     &TimeZoneInformation,
				     sizeof(TIME_ZONE_INFORMATION),
				     NULL);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return TIME_ZONE_ID_INVALID;
     }

   memcpy(lpTimeZoneInformation,
	  &TimeZoneInformation,
	  sizeof(TIME_ZONE_INFORMATION));

   return(SharedUserData->TimeZoneId);
}


BOOL STDCALL
SetTimeZoneInformation(CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation)
{
   TIME_ZONE_INFORMATION TimeZoneInformation;
   NTSTATUS Status;

   DPRINT("SetTimeZoneInformation()\n");

   memcpy(&TimeZoneInformation,
	  lpTimeZoneInformation,
	  sizeof(TIME_ZONE_INFORMATION));

   Status = RtlSetTimeZoneInformation(&TimeZoneInformation);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }

   NtSetSystemTime(0,0);

   return TRUE;
}


DWORD STDCALL
GetCurrentTime(VOID)
{
  return GetTickCount();
}


DWORD STDCALL
GetTickCount(VOID)
{
  ULONG UpTime;
  NtGetTickCount(&UpTime);
  return UpTime;
}


WINBOOL STDCALL
SystemTimeToTzSpecificLocalTime(
                                LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
                                LPSYSTEMTIME lpUniversalTime,
                                LPSYSTEMTIME lpLocalTime
                               )
{
  TIME_ZONE_INFORMATION TimeZoneInformation;
  LPTIME_ZONE_INFORMATION lpTzInfo;
  LARGE_INTEGER FileTime;

  if (!lpTimeZoneInformation)
  {
    GetTimeZoneInformation (&TimeZoneInformation);
    lpTzInfo = &TimeZoneInformation;
  }
  else
    lpTzInfo = lpTimeZoneInformation;

  if (!lpUniversalTime)
    return FALSE;

  if (!lpLocalTime)
    return FALSE;

  SystemTimeToFileTime (lpUniversalTime, (PFILETIME)&FileTime);
  FileTime.QuadPart -= (lpTzInfo->Bias * TICKSPERMIN);
  FileTimeToSystemTime ((PFILETIME)&FileTime, lpLocalTime);

  return TRUE;
}


WINBOOL STDCALL
GetSystemTimeAdjustment(PDWORD lpTimeAdjustment,
			PDWORD lpTimeIncrement,
			PWINBOOL lpTimeAdjustmentDisabled)
{
   SYSTEM_QUERY_TIME_ADJUSTMENT Buffer;
   NTSTATUS Status;
   
   Status = NtQuerySystemInformation(SystemTimeAdjustmentInformation,
				     &Buffer,
				     sizeof(SYSTEM_QUERY_TIME_ADJUSTMENT),
				     NULL);
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   
   *lpTimeAdjustment = (DWORD)Buffer.TimeAdjustment;
   *lpTimeIncrement = (DWORD)Buffer.MaximumIncrement;
   *lpTimeAdjustmentDisabled = (WINBOOL)Buffer.TimeSynchronization;
   
   return TRUE;
}


WINBOOL STDCALL
SetSystemTimeAdjustment(DWORD dwTimeAdjustment,
			WINBOOL bTimeAdjustmentDisabled)
{
   NTSTATUS Status;
   SYSTEM_TIME_ADJUSTMENT_INFO Buffer;
   
   Buffer.TimeAdjustment = (ULONG)dwTimeAdjustment;
   Buffer.TimeSynchronization = (BOOLEAN)bTimeAdjustmentDisabled;
   
   Status = NtSetSystemInformation(SystemTimeAdjustmentInformation,
				   &Buffer,
				   sizeof(SYSTEM_TIME_ADJUSTMENT_INFO));
   if (!NT_SUCCESS(Status))
     {
	SetLastErrorByStatus(Status);
	return FALSE;
     }
   
   return TRUE;
}

/* EOF */
