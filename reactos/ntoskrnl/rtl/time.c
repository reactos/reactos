/*
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

#include <internal/debug.h>

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

static __inline void NormalizeTimeFields(CSHORT *FieldToNormalize,
                                         CSHORT *CarryField,
                                         int Modulus)
{
  *FieldToNormalize = (CSHORT) (*FieldToNormalize - Modulus);
  *CarryField = (CSHORT) (*CarryField + 1);
}

/* FUNCTIONS *****************************************************************/

VOID RtlTimeToTimeFields(PLARGE_INTEGER liTime,
			 PTIME_FIELDS TimeFields)
{

#if 1

  const int *Months;
  int LeapSecondCorrections, SecondsInDay, CurYear;
  int LeapYear, CurMonth, GMTOffset;
  long int Days;
  long long int Time = *(long long int *)&liTime;

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
  Months = MonthLengths[LeapYear];
  for (CurMonth = 0; Days >= (long) Months[CurMonth]; CurMonth++)
    Days = Days - (long) Months[CurMonth];
  TimeFields->Month = (CSHORT) (CurMonth + 1);
  TimeFields->Day = (CSHORT) (Days + 1);

#else

   UNIMPLEMENTED;

#endif

}

BOOLEAN RtlTimeFieldsToTime(PTIME_FIELDS tfTimeFields,
			    PLARGE_INTEGER Time)
{

#if 1

  int CurYear, CurMonth;
  long long int rcTime;
  TIME_FIELDS TimeFields = *tfTimeFields;

  rcTime = 0;
  
    /* FIXME: normalize the TIME_FIELDS structure here */
  while (TimeFields.Second >= SECSPERMIN)
    {
      NormalizeTimeFields(&TimeFields.Second, 
                          &TimeFields.Minute, 
                          SECSPERMIN);
    }
  while (TimeFields.Minute >= MINSPERHOUR)
    {
      NormalizeTimeFields(&TimeFields.Minute, 
                          &TimeFields.Hour, 
                          MINSPERHOUR);
    }
  while (TimeFields.Hour >= HOURSPERDAY)
    {
      NormalizeTimeFields(&TimeFields.Hour, 
                          &TimeFields.Day, 
                          HOURSPERDAY);
    }
  while (TimeFields.Day >
         MonthLengths[IsLeapYear(TimeFields.Year)][TimeFields.Month - 1])
    {
      NormalizeTimeFields(&TimeFields.Day, 
                          &TimeFields.Month, 
                          SECSPERMIN);
    }
  while (TimeFields.Month > MONSPERYEAR)
    {
      NormalizeTimeFields(&TimeFields.Month, 
                          &TimeFields.Year, 
                          MONSPERYEAR);
    }

    /* FIXME: handle calendar corrections here */
  for (CurYear = EPOCHYEAR; CurYear < TimeFields.Year; CurYear++)
    {
      rcTime += YearLengths[IsLeapYear(CurYear)];
    }
  for (CurMonth = 1; CurMonth < TimeFields.Month; CurMonth++)
    {
      rcTime += MonthLengths[IsLeapYear(CurYear)][CurMonth - 1];
    }
  rcTime += TimeFields.Day - 1;
  rcTime *= SECSPERDAY;
  rcTime += TimeFields.Hour * SECSPERHOUR + TimeFields.Minute * SECSPERMIN +
            TimeFields.Second;
  rcTime *= TICKSPERSEC;
  rcTime += TimeFields.Milliseconds * TICKSPERMSEC;
  *Time = *(LARGE_INTEGER *)&rcTime;

  return TRUE;

#else

   UNIMPLEMENTED;

#endif

}
