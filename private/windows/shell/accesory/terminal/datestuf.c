/*===========================================================================*/
/*          Copyright (c) 1987 - 1988, Future Soft Engineering, Inc.         */
/*                              Houston, Texas                               */
/*===========================================================================*/

#include "winrev.h"
#include <windows.h>
#include "port1632.h"
#include "dcrc.h"
#include "dynacomm.h"

#include  <io.h>     /* added for get_osfhandle crt -sdj*/

static INT dayTable[2][13] = 
{
   {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
   {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
};


/*-------------- >>> Routines Specific to Windows Start Here <<< ------------*/

/*---------------------------------------------------------------------------*/
/* getDateTime() - Set a long int to number of elapsed seconds since  [scf]  */
/*                 January 1, 1904.  (Mimic the MAC funtion)                 */
/*---------------------------------------------------------------------------*/

VOID getDateTime (LONG *elapsedSecs)
{
   DOSTIME present;

   readDateTime (&present);
   date2secs (&present, elapsedSecs);
}


/*---------------------------------------------------------------------------*/
/* date2secs() - Convert a date found in DOSTIME struct to elapsed seconds   */
/*               since January 1, 1904.  (Mimic the MAC)             [scf]   */
/*---------------------------------------------------------------------------*/

VOID date2secs (DOSTIME *date, LONG *elapsedSecs)
{
#ifdef ORGCODE
   INT  year;
   INT  month;
   INT  leapYear;
#else
   WORD  year;
   WORD  month;
   WORD  leapYear;
#endif



   *elapsedSecs = 0l;
   for (year = 1904; year < date->yy; year++)
   {
      leapYear = year % 4 == 0 && year % 100 != 0 || year % 400 == 0;
      for (month = 1; month <= 12; month++)
         *elapsedSecs += (LONG) dayTable[leapYear][month] * SECS_IN_A_DAY;
   }
   leapYear = date->yy % 4 == 0 && date->yy % 100 != 0 || date->yy % 400 == 0;
   for (month = 1; month < date->mm; month++)
      *elapsedSecs += (LONG) dayTable[leapYear][month] * SECS_IN_A_DAY;
   *elapsedSecs += (LONG) (date->dd-1) *SECS_IN_A_DAY;
   *elapsedSecs += (LONG) date->hour *60*60;
   *elapsedSecs += (LONG) date->minute * 60;
   *elapsedSecs += (LONG) date->second;
}



/*---------------------------------------------------------------------------*/
/* secs2date() - Sets DOSTIME struct (except dayOfWeek) according to the     */
/*               number of elapsed  seconds since January 1, 1904.     [scf] */
/*---------------------------------------------------------------------------*/

VOID secs2date (LONG secs, DOSTIME *date)
{
   register  INT  year;
             INT  month;
             INT  day;
             INT  hour;
             INT  minute;
             INT  leapYear;
             INT  daysPerYear[2];

           DWORD  usecs;

   daysPerYear[0] = DAYS_IN_A_YEAR;
   daysPerYear[1] = DAYS_IN_A_YEAR + (leapYear = 1);

   usecs = (DWORD) secs;
   year = 1904;
   while (usecs >= (DWORD) daysPerYear[leapYear] * SECS_IN_A_DAY)
   {
      usecs -= (DWORD) daysPerYear[leapYear] * SECS_IN_A_DAY;
      year++;
      leapYear = (year % 4 == 0 && year % 100 != 0 || year % 400 == 0) ? 1 : 0;
   }
   secs = (LONG) usecs;
   for (month = 1; secs >= (LONG) dayTable[leapYear][month] *SECS_IN_A_DAY; 
         month++)
      secs -= (LONG) dayTable[leapYear][month] *SECS_IN_A_DAY;
   for (day = 1; secs >= SECS_IN_A_DAY; day++)
      secs -= SECS_IN_A_DAY;
   for (hour = 0; secs >= 60*60; hour++)
      secs -= 60 * 60;
   for (minute = 0; secs >= 60; minute++)
      secs -= 60;

   date->yy        = year;
   date->mm        = month;
   date->dd        = day;
   date->dayOfWeek = DONTCARE;
   date->hour      = hour;
   date->minute    = minute;
   date->second    = (INT) secs;
}


VOID readDateTime(DOSTIME *pDosTime)
{

SYSTEMTIME NtSystemTime;

/************

typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

***************/

/********************

typedef
   struct {
             WORD  hour;
             WORD  minute;
             WORD  second;
             WORD  dayOfWeek;
             WORD  mm;
             WORD  dd;
             WORD  yy;
          }  DOSTIME;

*************************/

DEBOUT("readDateTime:%s\n","Calling GetSystemTime()");

// GetSystemTime(&NtSystemTime);
// -sdj this time will not make sense, have to use
// -sdj GetLocalTime instead, this is due to Zone,Bias,UTC and all
// -sdj the other complicated things I have to learn sometime!


GetLocalTime(&NtSystemTime);

pDosTime->hour =        NtSystemTime.wHour;
pDosTime->minute =      NtSystemTime.wMinute;
pDosTime->second =      NtSystemTime.wSecond;
pDosTime->dayOfWeek =   NtSystemTime.wDayOfWeek;
pDosTime->mm =          NtSystemTime.wMonth;
pDosTime->dd =          NtSystemTime.wDay;
pDosTime->yy =          NtSystemTime.wYear;

}


VOID getFileDate(DOSTIME *pDosTime, int fh)
{
//BOOL	    bRc;
//HANDLE     hFile;
//FILETIME   CrTime,AccTime,WrTime;
SYSTEMTIME SysTime;

DEBOUT("getFileDate: UNDEF!! %s\n","converting the crt fh to os fh using get_osfhandle");

/* hFile = get_osfhandle(fh); */

DEBOUT("getFileDate: UNDEF!! got os fh as %lx\n", hFile);

/* DEBOUT("getFileDate: %s\n","calling GetFileTime , for LastWrTime"); */

/* bRc = GetFileTime(hFile,&CrTime,&AccTime,&WrTime); */

/* DEBOUT("getFileDate: GetFileTime Rc= %lx\n conv to SystemTime",bRc); */

/* bRc = FileTimeToSystemTime(&WrTime,&SysTime); */

/* DEBOUT("getFileDate: FileTimeToSystemTime Rc= %lx\n store in DosTime",bRc); */

DEBOUT("getFileDate: HACK %s\n","calling sys time instead of filetime for now");
GetSystemTime(&SysTime);

pDosTime->hour =        SysTime.wHour;
pDosTime->minute =      SysTime.wMinute;
pDosTime->second =      SysTime.wSecond;
pDosTime->dayOfWeek =   SysTime.wDayOfWeek;
pDosTime->mm =          SysTime.wMonth;
pDosTime->dd =          SysTime.wDay;
pDosTime->yy =          SysTime.wYear;

}


#ifdef OLDCODE
VOID lmovmem(LPSTR lpsrc,LPSTR lpdst, WORD wCount)
{
   LPBYTE TmpSrc,TmpDst;
   WORD i;

   TmpSrc = (LPBYTE)lpsrc;
   TmpDst = (LPBYTE)lpdst;


   for (i=0; i< wCount; i++)
   {
      *TmpDst = *TmpSrc;
      TmpDst++;TmpSrc++;
   }
}

VOID lsetmem(LPSTR str,BYTE ch,WORD wCount)
{
   WORD i;
   LPSTR tmp;

   tmp = str;

   for (i=0; i < wCount; i++)
   {
      *tmp = ch;
      tmp++;
   }

}
#endif
