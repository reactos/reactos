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

TIME_ZONE_INFORMATION TimeZoneInformation;

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
#define FOURCENTURY	(4*CENTURY+DAY)


#define MINUTE60	1 
#define HOUR60		60*MINUTE60
#define DAY60		24*HOUR60
#define YEAR60		(365*DAY60)
#define FOURYEAR60	(4*YEAR60+DAY60)
#define CENTURY60	(25*FOURYEAR60-DAY60)
#define FOURCENTURY60	(4*CENTURY60+DAY60)

#ifdef COMPILER_LARGE_INTEGERS
	#define RtlEnlargedUnsignedMultiply(m,n) 	(LARGE_INTEGER_QUAD_PART(((ULONGLONG)m)*((ULONGLONG)n)))
	#define RtlEnlargedIntegerMultiply(m,n) 	(LARGE_INTEGER_QUAD_PART(((LONGLONG)m)*((LONGLONG)n)))
	#define RtlLargeIntegerAdd(a1,a2) 		(LARGE_INTEGER_QUAD_PART(a1) + LARGE_INTEGER_QUAD_PART(a2))
	#define RtlExtendedIntegerMultiply(m1,m2)	(LARGE_INTEGER_QUAD_PART(m1) * m2)
	#define RtlLargeIntegerDivide(d1,d2,r1)		LARGE_INTEGER_QUAD_PART(d1) / LARGE_INTEGER_QUAD_PART(d2);LARGE_INTEGER_QUAD_PART(*r1) = LARGE_INTEGER_QUAD_PART(d1) % LARGE_INTEGER_QUAD_PART(d2)
	
#endif

#define LISECOND RtlEnlargedUnsignedMultiply(SECOND,NSPERSEC)
#define LIMINUTE RtlEnlargedUnsignedMultiply(MINUTE60,60*NSPERSEC)
#define LIHOUR RtlEnlargedUnsignedMultiply(HOUR60,60*NSPERSEC)
#define LIDAY RtlEnlargedUnsignedMultiply(DAY60,60*NSPERSEC)
#define LIYEAR RtlEnlargedUnsignedMultiply(YEAR60,60*NSPERSEC)
#define LIFOURYEAR RtlEnlargedUnsignedMultiply(FOURYEAR60,60*NSPERSEC)
#define LICENTURY RtlEnlargedUnsignedMultiply(CENTURY60,60*NSPERSEC) 
#define LIFOURCENTURY RtlEnlargedUnsignedMultiply(FOURCENTURY60,60*NSPERSEC)




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
  LARGE_INTEGER FileTime1, FileTime2;   



  if ( lpFileTime1 == NULL )
	return 0;
  if ( lpFileTime2 == NULL )
	return 0;

  memcpy(&FileTime1,lpFileTime1,sizeof(FILETIME));
  memcpy(&FileTime2,lpFileTime2,sizeof(FILETIME));
  if ((GET_LARGE_INTEGER_HIGH_PART((FileTime1))) > (GET_LARGE_INTEGER_HIGH_PART((FileTime2))) )
	return 1;
  else if ((GET_LARGE_INTEGER_HIGH_PART(FileTime1)) < (GET_LARGE_INTEGER_HIGH_PART(FileTime2)))
   	return -1;
  else if ((GET_LARGE_INTEGER_LOW_PART(FileTime1)) > (GET_LARGE_INTEGER_LOW_PART(FileTime2)))
	return 1;
  else if ((GET_LARGE_INTEGER_LOW_PART(FileTime1)) < (GET_LARGE_INTEGER_LOW_PART(FileTime2)))
   	return -1;
  else
	return 0;
 
 
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

	FileTime += RtlLargeIntegerAdd(FileTime,Year);
	FileTime += RtlLargeIntegerAdd(FileTime,Month);
	FileTime += RtlLargeIntegerAdd(FileTime,Day);
	FileTime += RtlLargeIntegerAdd(FileTime,Hour);
	FileTime += RtlLargeIntegerAdd(FileTime,Minute);
	FileTime += RtlLargeIntegerAdd(FileTime,Second);

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

   LARGE_INTEGER liFourCentury;
   LARGE_INTEGER liRemFourCentury; 
              
   LARGE_INTEGER liCentury;
   LARGE_INTEGER liRemCentury;
              
   LARGE_INTEGER liFourYear;
   LARGE_INTEGER liRemFourYear;
              
   LARGE_INTEGER liYear;
   LARGE_INTEGER liRemYear;
              
   LARGE_INTEGER liDay;
   LARGE_INTEGER liRemDay;
              
   LARGE_INTEGER liHour;
   LARGE_INTEGER liRemHour; 
              
   LARGE_INTEGER liMinute;
   LARGE_INTEGER liRemMinute;
              
   LARGE_INTEGER liSecond;
   LARGE_INTEGER liRemSecond;

   LARGE_INTEGER liDayOfWeek;


   DWORD LeapDay = 0;

   
   memcpy(&FileTime,lpFileTime,sizeof(FILETIME));
   
   
   liFourCentury = RtlLargeIntegerDivide(FileTime,LIFOURCENTURY,&liRemFourCentury);
   liCentury = RtlLargeIntegerDivide(liRemFourCentury,LICENTURY,&liRemCentury);
   liFourYear = RtlLargeIntegerDivide(liRemCentury,LIFOURYEAR,&liRemFourYear);
   liYear = RtlLargeIntegerDivide(liRemFourYear,LIYEAR,&liRemYear);
   liDay = RtlLargeIntegerDivide(liRemYear,LIDAY,&liRemDay);
   liHour = RtlLargeIntegerDivide(liRemDay,LIHOUR,&liRemHour);
   liMinute = RtlLargeIntegerDivide(liRemHour,LIMINUTE,&liRemMinute);
   liSecond = RtlLargeIntegerDivide(liRemMinute,LISECOND,&liRemSecond);
   
   lpSystemTime->wHour=  (WORD) GET_LARGE_INTEGER_LOW_PART(liHour);  
   lpSystemTime->wMinute= (WORD)GET_LARGE_INTEGER_LOW_PART(liMinute); 
   lpSystemTime->wSecond= (WORD)GET_LARGE_INTEGER_LOW_PART(liSecond); 
   lpSystemTime->wMilliseconds = (WORD)(GET_LARGE_INTEGER_LOW_PART(liRemSecond)/10000);
  

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
	SET_LARGE_INTEGER_LOW_PART(liDay,GET_LARGE_INTEGER_LOW_PART(liDay)+1); 
   }
 
  //FIXME since 1972 some years have a leap second [ aprox 15 out of 20 ]

 // printf("400 %d 100 %d 4 %d 1 %d\n",(LONG)GET_LARGE_INTEGER_LOW_PART(liFourCentury) , (LONG)GET_LARGE_INTEGER_LOW_PART(liCentury), (LONG)GET_LARGE_INTEGER_LOW_PART(liFourYear), (LONG)GET_LARGE_INTEGER_LOW_PART(liYear));
  // if leap year  
  lpSystemTime->wYear= 1601 + 400* (WORD)GET_LARGE_INTEGER_LOW_PART(liFourCentury) + 100 * (WORD)GET_LARGE_INTEGER_LOW_PART(liCentury) + 4*(LONG)GET_LARGE_INTEGER_LOW_PART(liFourYear) + (WORD)GET_LARGE_INTEGER_LOW_PART(liYear); 

  if ( (lpSystemTime->wYear % 4 == 0 && lpSystemTime->wYear % 100 != 0) || lpSystemTime->wYear % 400 == 0)
	LeapDay = 1;
  else
	LeapDay = 0;

  

  if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= 0 && GET_LARGE_INTEGER_LOW_PART(liDay) < 31 ) {
	lpSystemTime->wMonth = 1;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1;
  }
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= 31 && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 59 + LeapDay )) {
	lpSystemTime->wMonth = 2;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - 31;
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 59 + LeapDay ) && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 90 + LeapDay ) ) {
	lpSystemTime->wMonth = 3;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - ( 59 + LeapDay);
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= 90+ LeapDay && GET_LARGE_INTEGER_LOW_PART(liDay) < 120 + LeapDay) {
	lpSystemTime->wMonth = 4;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - (31 + LeapDay );
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= 120 + LeapDay && GET_LARGE_INTEGER_LOW_PART(liDay) < 151 + LeapDay ) {
	lpSystemTime->wMonth = 5;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - (120 + LeapDay);
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 151 + LeapDay) && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 181 + LeapDay ) ) {
	lpSystemTime->wMonth = 6;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - ( 151 + LeapDay );
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 181 + LeapDay ) && GET_LARGE_INTEGER_LOW_PART(liDay) <  ( 212 + LeapDay ) ) {
	lpSystemTime->wMonth = 7;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - ( 181 + LeapDay );
  }  
   else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 212 + LeapDay ) && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 243 + LeapDay ) ) {
	lpSystemTime->wMonth = 8;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 -  (212 + LeapDay );
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 243+ LeapDay ) && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 273 + LeapDay ) ) {
	lpSystemTime->wMonth = 9;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - ( 243 + LeapDay );
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 273 + LeapDay ) && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 304 + LeapDay ) ) {
	lpSystemTime->wMonth = 10;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - ( 273 + LeapDay);
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 304 + LeapDay ) && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 334 + LeapDay ) ) {
	lpSystemTime->wMonth = 11;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - ( 304 + LeapDay );
  }  
  else if ( GET_LARGE_INTEGER_LOW_PART(liDay) >= ( 334 + LeapDay ) && GET_LARGE_INTEGER_LOW_PART(liDay) < ( 365 + LeapDay )) {
	lpSystemTime->wMonth = 12;
   	lpSystemTime->wDay = GET_LARGE_INTEGER_LOW_PART(liDay) + 1 - ( 334 + LeapDay );
  }  
 

   liDayOfWeek = RtlLargeIntegerDivide(FileTime,LIDAY,&liRemDay);
   lpSystemTime->wDayOfWeek = 1 + GET_LARGE_INTEGER_LOW_PART(liDayOfWeek) % 7;

  


   return TRUE;	
}


WINBOOL
STDCALL
FileTimeToLocalFileTime(
			CONST FILETIME *lpFileTime,
			LPFILETIME lpLocalFileTime
			)
{
	TIME_ZONE_INFORMATION TimeZoneInformation;
	LARGE_INTEGER *FileTime, *LocalFileTime;

	if ( lpFileTime == NULL || lpLocalFileTime == NULL )
		return FALSE;
	FileTime = (LARGE_INTEGER *)lpFileTime;
	LocalFileTime = (LARGE_INTEGER *)lpLocalFileTime;

	GetTimeZoneInformation(&TimeZoneInformation);
	*LocalFileTime = RtlLargeIntegerAdd(*FileTime,RtlExtendedIntegerMultiply((LIMINUTE),(LONG)-1*TimeZoneInformation.Bias));

    	return TRUE;
}

WINBOOL
STDCALL
LocalFileTimeToFileTime(
			CONST FILETIME *lpLocalFileTime,
			LPFILETIME lpFileTime
			)
{
	TIME_ZONE_INFORMATION TimeZoneInformation;
	LARGE_INTEGER *FileTime, *LocalFileTime;

	if ( lpFileTime == NULL || lpLocalFileTime == NULL )
		return FALSE;
	FileTime = (LARGE_INTEGER *)lpFileTime;
	LocalFileTime = (LARGE_INTEGER *)lpLocalFileTime;

	GetTimeZoneInformation(&TimeZoneInformation);
	*FileTime = RtlLargeIntegerAdd(*LocalFileTime,RtlExtendedIntegerMultiply((LIMINUTE),(LONG)TimeZoneInformation.Bias));

    	return TRUE;
}

VOID
STDCALL
GetLocalTime(
	     LPSYSTEMTIME lpSystemTime
	     )
{

  FILETIME FileTime;
  FILETIME LocalTime;
  GetSystemTimeAsFileTime(&FileTime);
  FileTimeToLocalFileTime(&FileTime,&LocalTime);
  FileTimeToSystemTime(&LocalTime,lpSystemTime);
  
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
	NTSTATUS errCode;
	FILETIME FileTime, LocalTime;
	SystemTimeToFileTime(lpSystemTime,&LocalTime);
	LocalFileTimeToFileTime(&LocalTime,&FileTime);
	errCode = NtSetSystemTime ((LARGE_INTEGER *)&FileTime,(LARGE_INTEGER *)&FileTime);
	if ( !NT_SUCCESS(errCode) )
		return FALSE;
	return TRUE;

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
	errCode = NtSetSystemTime ((LARGE_INTEGER *)&NewSystemTime,(LARGE_INTEGER *)&NewSystemTime);
	if ( !NT_SUCCESS(errCode) )
		return FALSE;
	return TRUE;

} 

DWORD
STDCALL
GetCurrentTime(VOID)
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



VOID __InitTimeZoneInformation(VOID)
{
	TimeZoneInformation.Bias = 60;
	TimeZoneInformation.StandardName[0] = 0;
	TimeZoneInformation.StandardDate.wMonth = 9;
	TimeZoneInformation.StandardDate.wDay = 21;
	TimeZoneInformation.StandardBias = 0;
	TimeZoneInformation.DaylightName[0] = 0;
	TimeZoneInformation.DaylightDate.wMonth = 3;
	TimeZoneInformation.DaylightDate.wDay = 21;
	TimeZoneInformation.DaylightBias = -60;
	
}

WINBOOL
STDCALL
SetTimeZoneInformation(
		       CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
		       )
{
	if ( lpTimeZoneInformation == NULL )
		return FALSE;
	TimeZoneInformation = *lpTimeZoneInformation;
	return TRUE;
}
	


DWORD
STDCALL
GetTimeZoneInformation(
		       LPTIME_ZONE_INFORMATION lpTimeZoneInformation
		       )
{
 // aprintf("GetTimeZoneInformation()\n");
  SYSTEMTIME SystemTime;   
  if ( lpTimeZoneInformation == NULL )
	return -1;
  

  *lpTimeZoneInformation = TimeZoneInformation;
/*
  FIXME I should not recurse
  GetLocalTime(&SystemTime);
  if ( TimeZoneInformation.DaylightDate.wMonth == 0 || TimeZoneInformation.StandardDate.wMonth == 0 )
	return TIME_ZONE_ID_UNKNOWN;	
  else if ( ( SystemTime.wMonth > TimeZoneInformation.DaylightDate.wMonth &&
	 SystemTime.wDay   > TimeZoneInformation.DaylightDate.wDay  ) && 
       ( SystemTime.wMonth < TimeZoneInformation.StandardDate.wMonth &&
	 SystemTime.wDay   < TimeZoneInformation.StandardDate.wDay  ) ) 
	return TIME_ZONE_ID_DAYLIGHT;
  else
	return TIME_ZONE_ID_STANDARD;
  */
  return TIME_ZONE_ID_UNKNOWN;
}


WINBOOL
STDCALL
SetSystemTimeAdjustment(
			DWORD dwTimeAdjustment,
			WINBOOL  bTimeAdjustmentDisabled
			)
{
	return FALSE;
}


WINBOOL
STDCALL
GetSystemTimeAdjustment(
			PDWORD lpTimeAdjustment,
			PDWORD lpTimeIncrement,
			PWINBOOL  lpTimeAdjustmentDisabled
			)
{
	return FALSE;
}
