/*
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

#include <windows.h>
#include <ddk/ntddk.h>
#include <string.h>
#include <internal/string.h>
#include <string.h>

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
#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       0
#define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12

static const int YearLengths[2] = {DAYSPERNORMALYEAR, DAYSPERLEAPYEAR};
static const int MonthLengths[2][MONSPERYEAR] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static __inline int IsLeapYear(int Year)
{
  return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}

static __inline void NormalizeTimeFields(WORD *FieldToNormalize,
                                         WORD *CarryField,
                                         int Modulus)
{
  *FieldToNormalize = (WORD) (*FieldToNormalize - Modulus);
  *CarryField = (WORD) (*CarryField + 1);
}

#define LISECOND RtlEnlargedUnsignedMultiply(SECOND,NSPERSEC)
#define LIMINUTE RtlEnlargedUnsignedMultiply(MINUTE,NSPERSEC)
#define LIHOUR RtlEnlargedUnsignedMultiply(HOUR,NSPERSEC)
#define LIDAY RtlEnlargedUnsignedMultiply(DAY,NSPERSEC)
#define LIYEAR RtlEnlargedUnsignedMultiply(YEAR,NSPERSEC)
#define LIFOURYEAR RtlEnlargedUnsignedMultiply(FOURYEAR,NSPERSEC)
#define LICENTURY RtlEnlargedUnsignedMultiply(CENTURY,NSPERSEC) 
#define LIMILLENIUM RtlEnlargedUnsignedMultiply(CENTURY,10*NSPERSEC)




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
  int CurYear, CurMonth, MonthLength;
  long long int rcTime;
  SYSTEMTIME SystemTime;

  memcpy (&SystemTime, lpSystemTime, sizeof(SYSTEMTIME));

  rcTime = 0;
  
    /* FIXME: normalize the TIME_FIELDS structure here */
  while (SystemTime.wSecond >= SECSPERMIN)
    {
      NormalizeTimeFields(&SystemTime.wSecond, 
                          &SystemTime.wMinute, 
                          SECSPERMIN);
    }
  while (SystemTime.wMinute >= MINSPERHOUR)
    {
      NormalizeTimeFields(&SystemTime.wMinute, 
                          &SystemTime.wHour, 
                          MINSPERHOUR);
    }
  while (SystemTime.wHour >= HOURSPERDAY)
    {
      NormalizeTimeFields(&SystemTime.wHour, 
                          &SystemTime.wDay, 
                          HOURSPERDAY);
    }
  MonthLength =
    MonthLengths[IsLeapYear(SystemTime.wYear)][SystemTime.wMonth - 1];
  while (SystemTime.wDay > MonthLength)
    {
      NormalizeTimeFields(&SystemTime.wDay, 
                          &SystemTime.wMonth, 
                          MonthLength);
    }
  while (SystemTime.wMonth > MONSPERYEAR)
    {
      NormalizeTimeFields(&SystemTime.wMonth, 
                          &SystemTime.wYear, 
                          MONSPERYEAR);
    }

    /* FIXME: handle calendar corrections here */
  for (CurYear = EPOCHYEAR; CurYear < SystemTime.wYear; CurYear++)
    {
      rcTime += YearLengths[IsLeapYear(CurYear)];
    }
  for (CurMonth = 1; CurMonth < SystemTime.wMonth; CurMonth++)
    {
      rcTime += MonthLengths[IsLeapYear(CurYear)][CurMonth - 1];
    }
  rcTime += SystemTime.wDay - 1;
  rcTime *= SECSPERDAY;
  rcTime += SystemTime.wHour * SECSPERHOUR +
            SystemTime.wMinute * SECSPERMIN + SystemTime.wSecond;
  rcTime *= TICKSPERSEC;
  rcTime += SystemTime.wMilliseconds * TICKSPERMSEC;

  *lpFileTime = *(FILETIME *)&rcTime;

  return TRUE;
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
  const int *Months;
  int LeapSecondCorrections, SecondsInDay, CurYear;
  int LeapYear, CurMonth;
  long int Days;
  long long int Time = *((long long int*)lpFileTime);

    /* Extract millisecond from time and convert time into seconds */
  lpSystemTime->wMilliseconds = (WORD)((Time % TICKSPERSEC) / TICKSPERMSEC);
  Time = Time / TICKSPERSEC;

    /* FIXME: Compute the number of leap second corrections here */
  LeapSecondCorrections = 0;

    /* Split the time into days and seconds within the day */
  Days = Time / SECSPERDAY;
  SecondsInDay = Time % SECSPERDAY;

    /* Adjust the values for GMT and leap seconds */
  SecondsInDay += LeapSecondCorrections;
  while (SecondsInDay < 0) 
    {
      SecondsInDay += SECSPERDAY;
      Days--;
    }
  while (SecondsInDay >= SECSPERDAY) 
    {
      SecondsInDay -= SECSPERDAY;
      Days++;
    }

    /* compute time of day */
  lpSystemTime->wHour = (WORD) (SecondsInDay / SECSPERHOUR);
  SecondsInDay = SecondsInDay % SECSPERHOUR;
  lpSystemTime->wMinute = (WORD) (SecondsInDay / SECSPERMIN);
  lpSystemTime->wSecond = (WORD) (SecondsInDay % SECSPERMIN);

    /* FIXME: handle the possibility that we are on a leap second (i.e. Second = 60) */

    /* compute day of week */
  lpSystemTime->wDayOfWeek = (WORD) ((EPOCHWEEKDAY + Days) % DAYSPERWEEK);

    /* compute year */
  CurYear = EPOCHYEAR;
    /* FIXME: handle calendar modifications */
  while (1)
    {
      LeapYear = IsLeapYear(CurYear);
      if (Days < (long) YearLengths[LeapYear])
        {
          break;
        }
      CurYear++;
      Days = Days - (long) YearLengths[LeapYear];
    }
  lpSystemTime->wYear = (WORD) CurYear;

    /* Compute month of year */
  Months = MonthLengths[LeapYear];
  for (CurMonth = 0; Days >= (long) Months[CurMonth]; CurMonth++)
    Days = Days - (long) Months[CurMonth];
  lpSystemTime->wMonth = (WORD) (CurMonth + 1);
  lpSystemTime->wDay = (WORD) (Days + 1);

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

VOID
STDCALL
GetLocalTime(
	     LPSYSTEMTIME lpSystemTime
	     )
{
  FILETIME FileTime;
  FILETIME LocalFileTime;

  NtQuerySystemTime ((TIME*)&FileTime);
  FileTimeToLocalFileTime (&FileTime, &LocalFileTime);
  FileTimeToSystemTime (&LocalFileTime, lpSystemTime);
}

VOID
STDCALL
GetSystemTime(
	      LPSYSTEMTIME lpSystemTime
	      )
{
  FILETIME FileTime;

  NtQuerySystemTime ((TIME*)&FileTime);
  FileTimeToSystemTime (&FileTime, lpSystemTime);
}

WINBOOL
STDCALL
SetLocalTime(
	     CONST SYSTEMTIME *lpSystemTime
	     )
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

WINBOOL
STDCALL
SetSystemTime(
	      CONST SYSTEMTIME *lpSystemTime
	      )
{
  LARGE_INTEGER NewSystemTime;
  NTSTATUS errCode;

  SystemTimeToFileTime (lpSystemTime, (PFILETIME)&NewSystemTime);
  errCode = NtSetSystemTime (&NewSystemTime, &NewSystemTime);
  if (!NT_SUCCESS(errCode))
    return FALSE;
  return TRUE;
}

/*
typedef struct _TIME_ZONE_INFORMATION { // tzi 
    LONG       Bias; 
    WCHAR      StandardName[ 32 ]; 
    SYSTEMTIME StandardDate; 
    LONG       StandardBias; 
    WCHAR      DaylightName[ 32 ]; 
    SYSTEMTIME DaylightDate; 
    LONG       DaylightBias; 
} TIME_ZONE_INFORMATION; 
TIME_ZONE_INFORMATION TimeZoneInformation = {60,"CET",;

*/
DWORD
STDCALL
GetTimeZoneInformation(
		       LPTIME_ZONE_INFORMATION lpTimeZoneInformation
		       )
{
 // aprintf("GetTimeZoneInformation()\n");
   
 // memset(lpTimeZoneInformation, 0, sizeof(TIME_ZONE_INFORMATION));
  
  return TIME_ZONE_ID_UNKNOWN;
}

BOOL
STDCALL
SetTimeZoneInformation(
                       CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
		       )
{
  
  return FALSE;
}

DWORD STDCALL GetCurrentTime(VOID)
{
  return GetTickCount ();
}

DWORD
STDCALL
GetTickCount(VOID)
{
  ULONG UpTime;
  NtGetTickCount (&UpTime);
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

WINBOOL
STDCALL
GetSystemTimeAdjustment(
                        PDWORD lpTimeAdjustment,
                        PDWORD lpTimeIncrement,
                        PWINBOOL lpTimeAdjustmentDisabled
                       )
{
  // FIXME: Preliminary default settings.
  *lpTimeAdjustment = 0;
  *lpTimeIncrement = 0;
  *lpTimeAdjustmentDisabled = TRUE;

  return TRUE;
}

WINBOOL
STDCALL
SetSystemTimeAdjustment(
                        DWORD   dwTimeAdjustment,
                        WINBOOL bTimeAdjustmentDisabled
                       )
{

  return TRUE;
}
