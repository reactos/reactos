/* $Id: time.c,v 1.17 2003/07/11 01:23:16 royce Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/rtl/time.c
 * PURPOSE:         Conversion between Time and TimeFields
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   08/03/98  RJJ  Implemented these functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>

#define TICKSPERMIN        600000000
#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       1
#define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12

#define TICKSTO1970         0x019db1ded53e8000LL
#define TICKSTO1980         0x01a8e79fe1d58000LL


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

static __inline void NormalizeTimeFields(CSHORT *FieldToNormalize,
                                         CSHORT *CarryField,
                                         int Modulus)
{
  *FieldToNormalize = (CSHORT) (*FieldToNormalize - Modulus);
  *CarryField = (CSHORT) (*CarryField + 1);
}

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
VOID
STDCALL
RtlTimeToTimeFields(
	PLARGE_INTEGER liTime,
	PTIME_FIELDS TimeFields)
{
  const int *Months;
  int LeapSecondCorrections, SecondsInDay, CurYear;
  int LeapYear, CurMonth, GMTOffset;
  long int Days;
  long long int Time = (long long int)liTime->QuadPart;

    /* Extract millisecond from time and convert time into seconds */
  TimeFields->Milliseconds = (CSHORT) ((Time % TICKSPERSEC) / TICKSPERMSEC);
  Time = Time / TICKSPERSEC;

    /* FIXME: Compute the number of leap second corrections here */
  LeapSecondCorrections = 0;

    /* FIXME: get the GMT offset here */
  GMTOffset = 0;

    /* Split the time into days and seconds within the day */
  Days = Time / SECSPERDAY;
  SecondsInDay = Time % SECSPERDAY;

    /* Adjust the values for GMT and leap seconds */
  SecondsInDay += (GMTOffset - LeapSecondCorrections);
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
  TimeFields->Hour = (CSHORT) (SecondsInDay / SECSPERHOUR);
  SecondsInDay = SecondsInDay % SECSPERHOUR;
  TimeFields->Minute = (CSHORT) (SecondsInDay / SECSPERMIN);
  TimeFields->Second = (CSHORT) (SecondsInDay % SECSPERMIN);

    /* FIXME: handle the possibility that we are on a leap second (i.e. Second = 60) */

    /* compute day of week */
  TimeFields->Weekday = (CSHORT) ((EPOCHWEEKDAY + Days) % DAYSPERWEEK);

    /* compute year */
  CurYear = EPOCHYEAR;
  CurYear += Days / DAYSPERLEAPYEAR;
  Days -= (CurYear - EPOCHYEAR) * DAYSPERLEAPYEAR;
  CurYear--; /* The next calculation needs CurYear - 1 */
  Days += CurYear - CurYear / 4 + CurYear / 100 - CurYear / 400;
  CurYear++;
  Days -= EPOCHYEAR - 1 - (EPOCHYEAR -1) / 4 + (EPOCHYEAR -1) / 100 - (EPOCHYEAR - 1) / 400;
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
  TimeFields->Year = (CSHORT) CurYear;

    /* Compute month of year */
  LeapYear = IsLeapYear(CurYear);
  Months = MonthLengths[LeapYear];
  for (CurMonth = 0; Days >= (long) Months[CurMonth]; CurMonth++)
    Days = Days - (long) Months[CurMonth];
  TimeFields->Month = (CSHORT) (CurMonth + 1);
  TimeFields->Day = (CSHORT) (Days + 1);
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
RtlTimeFieldsToTime(
	PTIME_FIELDS tfTimeFields,
	PLARGE_INTEGER Time)
{
  int CurMonth;
  long long int rcTime;
  TIME_FIELDS TimeFields = *tfTimeFields;
  const int *Months;

  rcTime = 0;
  
  /* Normalize the month value, because we don't know the length for month -1, 0, 13, 14, ... */  
  if (TimeFields.Month < 1 || TimeFields.Month > 12)
  {
     TimeFields.Year += (TimeFields.Month - 1) / MONSPERYEAR;
     TimeFields.Month = ((TimeFields.Month - 1) % MONSPERYEAR) + 1;
     if (TimeFields.Month < 1)
     {
	TimeFields.Year--;
	TimeFields.Month += MONSPERYEAR;
     }
  }
    /* FIXME: handle calendar corrections here */

  rcTime += (TimeFields.Year - EPOCHYEAR) * DAYSPERNORMALYEAR;
  /* Adjust leap years */
  rcTime += (TimeFields.Year - 1)/ 4 - (TimeFields.Year - 1) / 100 + (TimeFields.Year - 1) / 400;
  rcTime -= EPOCHYEAR / 4 - EPOCHYEAR / 100 + EPOCHYEAR / 400;

    /* FIXME: handle calendar corrections here */
  Months = MonthLengths[IsLeapYear(TimeFields.Year)];
  for (CurMonth = 1; CurMonth < TimeFields.Month; CurMonth++)
    {
      rcTime += Months[CurMonth - 1];
    }
  rcTime += TimeFields.Day - 1;
  rcTime *= SECSPERDAY;
  rcTime += TimeFields.Hour * SECSPERHOUR + TimeFields.Minute * SECSPERMIN +
            TimeFields.Second;
  rcTime *= TICKSPERSEC;
  rcTime += TimeFields.Milliseconds * TICKSPERMSEC;

    /* FIXME: handle UTC bias here */
//  rcTime += UTCBias * TICKSPERMIN;

  *Time = *(LARGE_INTEGER *)&rcTime;

  return TRUE;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlSecondsSince1970ToTime(
	ULONG SecondsSince1970,
	PLARGE_INTEGER Time)
{
   LONGLONG llTime;

   llTime = (SecondsSince1970 * TICKSPERSEC) + TICKSTO1970;

   *Time = *(LARGE_INTEGER *)&llTime;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlSecondsSince1980ToTime(
	ULONG SecondsSince1980,
	PLARGE_INTEGER Time)
{
   LONGLONG llTime;

   llTime = (SecondsSince1980 * TICKSPERSEC) + TICKSTO1980;

   *Time = *(LARGE_INTEGER *)&llTime;
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlTimeToSecondsSince1970(
	PLARGE_INTEGER Time,
	PULONG SecondsSince1970)
{
   LARGE_INTEGER liTime;

   liTime.QuadPart = Time->QuadPart - TICKSTO1970;
   liTime.QuadPart = liTime.QuadPart / TICKSPERSEC;

   if (liTime.u.HighPart != 0)
      return FALSE;

   *SecondsSince1970 = liTime.u.LowPart;

   return TRUE;
}


/*
 * @implemented
 */
BOOLEAN
STDCALL
RtlTimeToSecondsSince1980(
	PLARGE_INTEGER Time,
	PULONG SecondsSince1980)
{
   LARGE_INTEGER liTime;

   liTime.QuadPart = Time->QuadPart - TICKSTO1980;
   liTime.QuadPart = liTime.QuadPart / TICKSPERSEC;

   if (liTime.u.HighPart != 0)
      return FALSE;

   *SecondsSince1980 = liTime.u.LowPart;

   return TRUE;
}

/* EOF */
