/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/time.c
 * PURPOSE:         Time conversion functions
 * PROGRAMMER:      Boudewijn ( ariadne@xs4all.nl)
		    DOSDATE and DOSTIME structures from Onno Hovers
 * UPDATE HISTORY:
 *                  Created 19/01/99
 */
#include <windows.h>
#include <ddk/ntddk.h>
#include <string.h>

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

#define NSPERSEC	10000000

#define SECOND		1
#define MINUTE		60*SECOND 
#define HOUR		60*MINUTE
#define DAY		24*HOUR
#define YEAR		(365*DAY)
#define FOURYEAR	(4*YEAR+DAY)
#define CENTURY		(25*FOURYEAR-DAY)
#define MILLENIUM	(100*CENTURY)


#define LISECOND RtlEnlargedUnsignedMultiply(SECOND,NSPERSEC)
#define LIMINUTE RtlEnlargedUnsignedMultiply(MINUTE,NSPERSEC)
#define LIHOUR RtlEnlargedUnsignedMultiply(HOUR,NSPERSEC)
#define LIDAY RtlEnlargedUnsignedMultiply(DAY,NSPERSEC)
#define LIYEAR RtlEnlargedUnsignedMultiply(YEAR,NSPERSEC)
#define LIFOURYEAR RtlEnlargedUnsignedMultiply(FOURYEAR,NSPERSEC)
#define LICENTURY RtlEnlargedUnsignedMultiply(CENTURY,NSPERSEC) 
#define LIMILLENIUM RtlEnlargedUnsignedMultiply(CENTURY,10*NSPERSEC)






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
 /*
  if (lpFileTime1.HighPart > lpFileTime2.HighPart)
	return 1;
  else if (lpFileTime1.HighPart < lpFileTime2.HighPart)
   	return -1;
  else if (lpFileTime1.LowPart > lpFileTime2.LowPart)
	return 1;
  else if (lpFileTime1.LowPart < lpFileTime2.LowPart)
   	return -1;
  else
	return 0;
 
 */
}

VOID
STDCALL 
GetSystemTimeAsFileTime(PFILETIME lpFileTime)
{
	NTSTATUS errCode;

	errCode = NtQuerySystemTime (
		(TIME *)lpFileTime
	);
	return;
}

WINBOOL 
STDCALL
SystemTimeToFileTime(
    CONST SYSTEMTIME *  lpSystemTime,	
    LPFILETIME  lpFileTime 	
   )

{
	LARGE_INTEGER FileTime;
	LARGE_INTEGER Year;
	LARGE_INTEGER Month;
	LARGE_INTEGER Day;
	LARGE_INTEGER Hour;
	LARGE_INTEGER Minute;
	LARGE_INTEGER Second;
	LARGE_INTEGER Milliseconds;
	DWORD LeapDay = 0;
	DWORD dwMonthDays = 0;

	if ( (lpSystemTime->wYear % 4 == 0 && lpSystemTime->wYear % 100 != 0) || lpSystemTime->wYear % 400 == 0)
		LeapDay = 1;
  	else
		LeapDay = 0;

	
	Year = RtlEnlargedIntegerMultiply(lpSystemTime->wYear - 1601,YEAR);
	if ( lpSystemTime->wMonth > 1)
		dwMonthDays = 31;
	if ( lpSystemTime->wMonth > 2)
		dwMonthDays += ( 28 + LeapDay );
	if ( lpSystemTime->wMonth > 3)
		dwMonthDays += 31;
	if ( lpSystemTime->wMonth > 4)
		dwMonthDays += 30;
	if ( lpSystemTime->wMonth > 5)
		dwMonthDays += 31;
	if ( lpSystemTime->wMonth > 6)
		dwMonthDays += 30;
	if ( lpSystemTime->wMonth > 7)
		dwMonthDays += 31;
	if ( lpSystemTime->wMonth > 8)
		dwMonthDays += 31;
	if ( lpSystemTime->wMonth > 9)
		dwMonthDays += 30;
	if ( lpSystemTime->wMonth > 10)
		dwMonthDays += 31;
	if ( lpSystemTime->wMonth > 11)
		dwMonthDays += 30;

	Month = RtlEnlargedIntegerMultiply(dwMonthDays,DAY);

	Day = RtlEnlargedIntegerMultiply(lpSystemTime->wDay,DAY);

	Hour = RtlEnlargedIntegerMultiply(lpSystemTime->wHour,HOUR);
	Minute = RtlEnlargedIntegerMultiply(lpSystemTime->wMinute,MINUTE);
	Second = RtlEnlargedIntegerMultiply(lpSystemTime->wSecond,DAY);

	Milliseconds =  RtlEnlargedIntegerMultiply(lpSystemTime->wMilliseconds,10000);

        FileTime = RtlLargeIntegerAdd(FileTime,Year);
        FileTime = RtlLargeIntegerAdd(FileTime,Month);
        FileTime = RtlLargeIntegerAdd(FileTime,Day);
        FileTime = RtlLargeIntegerAdd(FileTime,Hour);
        FileTime = RtlLargeIntegerAdd(FileTime,Minute);
        FileTime = RtlLargeIntegerAdd(FileTime,Second);

        FileTime = RtlExtendedIntegerMultiply(FileTime,NSPERSEC);

        FileTime = RtlLargeIntegerAdd(FileTime,Milliseconds);

	memcpy(lpFileTime,&FileTime,sizeof(FILETIME));

	return TRUE;
}



WINBOOL
STDCALL
FileTimeToSystemTime(
		     CONST FILETIME *lpFileTime,
		     LPSYSTEMTIME lpSystemTime
		     )
{


   LARGE_INTEGER FileTime;

   LARGE_INTEGER dwMillenium;
   LARGE_INTEGER dwRemMillenium; 
              
   LARGE_INTEGER dwCentury;
   LARGE_INTEGER dwRemCentury;
              
   LARGE_INTEGER dwFourYear;
   LARGE_INTEGER dwRemFourYear;
              
   LARGE_INTEGER dwYear;
   LARGE_INTEGER dwRemYear;
              
   LARGE_INTEGER dwDay;
   LARGE_INTEGER dwRemDay;
              
   LARGE_INTEGER dwHour;
   LARGE_INTEGER dwRemHour; 
              
   LARGE_INTEGER dwMinute;
   LARGE_INTEGER dwRemMinute;
              
   LARGE_INTEGER dwSecond;
   LARGE_INTEGER dwRemSecond;

   LARGE_INTEGER dwDayOfWeek;


   DWORD LeapDay = 0;

   
   memcpy(&FileTime,lpFileTime,sizeof(FILETIME));
   
   
   dwMillenium = RtlLargeIntegerDivide(FileTime,LIMILLENIUM,&dwRemMillenium);
   dwCentury = RtlLargeIntegerDivide(dwRemMillenium,LICENTURY,&dwRemCentury);
   dwFourYear = RtlLargeIntegerDivide(dwRemCentury,LIFOURYEAR,&dwRemFourYear);
   dwYear = RtlLargeIntegerDivide(dwRemFourYear,LIYEAR,&dwRemYear);
   dwDay = RtlLargeIntegerDivide(dwRemYear,LIDAY,&dwRemDay);
   dwHour = RtlLargeIntegerDivide(dwRemDay,LIHOUR,&dwRemHour);
   dwMinute = RtlLargeIntegerDivide(dwRemHour,LIMINUTE,&dwRemMinute);
   dwSecond = RtlLargeIntegerDivide(dwRemMinute,LISECOND,&dwRemSecond);
   
   lpSystemTime->wHour=   (WORD)(dwHour.u.LowPart);
   lpSystemTime->wMinute= (WORD)(dwMinute.u.LowPart); 
   lpSystemTime->wSecond= (WORD)(dwSecond.u.LowPart); 
   lpSystemTime->wMilliseconds = (WORD)(dwRemSecond.u.LowPart/10000);
  

   if ( lpSystemTime->wSecond > 60 ) {
	lpSystemTime->wSecond -= 60;
	lpSystemTime->wMinute ++; 
   }

   if ( lpSystemTime->wMinute > 60 ) {
	lpSystemTime->wMinute -= 60;
	lpSystemTime->wHour ++; 
   }

   if (lpSystemTime->wHour > 24 ) {
	lpSystemTime->wHour-= 24;
        dwDay.u.LowPart = dwDay.u.LowPart + 1; 
   }
 
  //FIXME since 1972 some years have a leap second [ aprox 15 out of 20 ]

  // if leap year  
  lpSystemTime->wYear = 1601 + 1000* (LONG)dwMillenium.u.LowPart + 100 * (LONG)dwCentury.u.LowPart + 4 * (LONG)dwFourYear.u.LowPart + (LONG)dwYear.u.LowPart; 

  if ( (lpSystemTime->wYear % 4 == 0 && lpSystemTime->wYear % 100 != 0) || lpSystemTime->wYear % 400 == 0)
	LeapDay = 1;
  else
	LeapDay = 0;

  

  if ( dwDay.u.LowPart >= 0 && dwDay.u.LowPart < 31 ) {
	lpSystemTime->wMonth = 1;
        lpSystemTime->wDay = dwDay.u.LowPart + 1;
  }
  else if ( dwDay.u.LowPart >= 31 && dwDay.u.LowPart < ( 59 + LeapDay )) {
	lpSystemTime->wMonth = 2;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - 31;
  }  
  else if ( dwDay.u.LowPart >= ( 59 + LeapDay ) && dwDay.u.LowPart < ( 90 + LeapDay ) ) {
	lpSystemTime->wMonth = 3;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - ( 59 + LeapDay);
  }  
  else if ( dwDay.u.LowPart >= 90+ LeapDay && dwDay.u.LowPart < 120 + LeapDay) {
	lpSystemTime->wMonth = 4;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - (31 + LeapDay );
  }  
  else if ( dwDay.u.LowPart >= 120 + LeapDay && dwDay.u.LowPart < 151 + LeapDay ) {
	lpSystemTime->wMonth = 5;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - (120 + LeapDay);
  }  
  else if ( dwDay.u.LowPart >= ( 151 + LeapDay) && dwDay.u.LowPart < ( 181 + LeapDay ) ) {
	lpSystemTime->wMonth = 6;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - ( 151 + LeapDay );
  }  
  else if ( dwDay.u.LowPart >= ( 181 + LeapDay ) && dwDay.u.LowPart <  ( 212 + LeapDay ) ) {
	lpSystemTime->wMonth = 7;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - ( 181 + LeapDay );
  }  
   else if ( dwDay.u.LowPart >= ( 212 + LeapDay ) && dwDay.u.LowPart < ( 243 + LeapDay ) ) {
	lpSystemTime->wMonth = 8;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 -  (212 + LeapDay );
  }  
  else if ( dwDay.u.LowPart >= ( 243+ LeapDay ) && dwDay.u.LowPart < ( 273 + LeapDay ) ) {
	lpSystemTime->wMonth = 9;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - ( 243 + LeapDay );
  }  
  else if ( dwDay.u.LowPart >= ( 273 + LeapDay ) && dwDay.u.LowPart < ( 304 + LeapDay ) ) {
	lpSystemTime->wMonth = 10;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - ( 273 + LeapDay);
  }  
  else if ( dwDay.u.LowPart >= ( 304 + LeapDay ) && dwDay.u.LowPart < ( 334 + LeapDay ) ) {
	lpSystemTime->wMonth = 11;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - ( 304 + LeapDay );
  }  
  else if ( dwDay.u.LowPart >= ( 334 + LeapDay ) && dwDay.u.LowPart < ( 365 + LeapDay )) {
	lpSystemTime->wMonth = 12;
        lpSystemTime->wDay = dwDay.u.LowPart + 1 - ( 334 + LeapDay );
  }  
 

   dwDayOfWeek = RtlLargeIntegerDivide(FileTime,LIDAY,&dwRemDay);
   lpSystemTime->wDayOfWeek = 1 + dwDayOfWeek.u.LowPart % 7;

  


   return TRUE;	
}


WINBOOL
STDCALL
FileTimeToLocalFileTime(
			CONST FILETIME *lpFileTime,
			LPFILETIME lpLocalFileTime
			)
{
   // memcpy(lpLocalFileTime,lpFileTime,sizeof(FILETIME));
    
    return TRUE;
}

WINBOOL
STDCALL
LocalFileTimeToFileTime(
			CONST FILETIME *lpLocalFileTime,
			LPFILETIME lpFileTime
			)
{
 
    return TRUE;
}

VOID
STDCALL
GetLocalTime(
	     LPSYSTEMTIME lpSystemTime
	     )
{
  GetSystemTime(lpSystemTime);
  return;
}

VOID
STDCALL
GetSystemTime(
	      LPSYSTEMTIME lpSystemTime
	      )
{
  FILETIME FileTime;
  GetSystemTimeAsFileTime(&FileTime);
  FileTimeToSystemTime(&FileTime,lpSystemTime);
  return;
}

WINBOOL
STDCALL
SetLocalTime(
	     CONST SYSTEMTIME *lpSystemTime
	     )
{
   return SetSystemTime(lpSystemTime);
}
WINBOOL
STDCALL
SetSystemTime(
	      CONST SYSTEMTIME *lpSystemTime
	      )
{
  	NTSTATUS errCode;
	LARGE_INTEGER NewSystemTime;
        
	SystemTimeToFileTime(lpSystemTime, (FILETIME *)&NewSystemTime);
	errCode = NtSetSystemTime (&NewSystemTime,&NewSystemTime);
	if ( !NT_SUCCESS(errCode) )
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

DWORD STDCALL GetCurrentTime(VOID)
{
	return GetTickCount();
}

DWORD
STDCALL
GetTickCount(VOID)
{
	ULONG UpTime;
	NtGetTickCount(&UpTime);
	return UpTime;
}
