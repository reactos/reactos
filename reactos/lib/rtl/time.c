/* $Id: time.c,v 1.2 2004/12/16 23:46:41 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rtl/time.c
 * PURPOSE:         Conversion between Time and TimeFields
 * PROGRAMMER:      Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *   08/03/98  RJJ  Implemented these functions
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <debug.h>

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

#if defined(__GNUC__)
#define TICKSTO1970         0x019db1ded53e8000LL
#define TICKSTO1980         0x01a8e79fe1d58000LL
#else
#define TICKSTO1970         0x019db1ded53e8000i64
#define TICKSTO1980         0x01a8e79fe1d58000i64
#endif


static const int YearLengths[2] =
   {
      DAYSPERNORMALYEAR, DAYSPERLEAPYEAR
   };
static const int MonthLengths[2][MONSPERYEAR] =
   {
      {
         31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
      },
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
 * @implemented
 */
BOOLEAN
STDCALL
RtlTimeFieldsToTime(
   PTIME_FIELDS TimeFields,
   PLARGE_INTEGER Time)
{
   int CurYear;
   int CurMonth;
   TIME_FIELDS IntTimeFields;

   Time->QuadPart = 0;
   memcpy(&IntTimeFields,
          TimeFields,
          sizeof(TIME_FIELDS));

   /* Normalize the TIME_FIELDS structure here */
   while (IntTimeFields.Second >= SECSPERMIN)
   {
      NormalizeTimeFields(&IntTimeFields.Second,
                          &IntTimeFields.Minute,
                          SECSPERMIN);
   }
   while (IntTimeFields.Minute >= MINSPERHOUR)
   {
      NormalizeTimeFields(&IntTimeFields.Minute,
                          &IntTimeFields.Hour,
                          MINSPERHOUR);
   }
   while (IntTimeFields.Hour >= HOURSPERDAY)
   {
      NormalizeTimeFields(&IntTimeFields.Hour,
                          &IntTimeFields.Day,
                          HOURSPERDAY);
   }
   while (IntTimeFields.Day >
          MonthLengths[IsLeapYear(IntTimeFields.Year)][IntTimeFields.Month - 1])
   {
      NormalizeTimeFields(&IntTimeFields.Day,
                          &IntTimeFields.Month,
                          SECSPERMIN);
   }
   while (IntTimeFields.Month > MONSPERYEAR)
   {
      NormalizeTimeFields(&IntTimeFields.Month,
                          &IntTimeFields.Year,
                          MONSPERYEAR);
   }

   /* Compute the time */
   for (CurYear = EPOCHYEAR; CurYear < IntTimeFields.Year; CurYear++)
   {
      Time->QuadPart += YearLengths[IsLeapYear(CurYear)];
   }
   for (CurMonth = 1; CurMonth < IntTimeFields.Month; CurMonth++)
   {
      Time->QuadPart += MonthLengths[IsLeapYear(CurYear)][CurMonth - 1];
   }
   Time->QuadPart += IntTimeFields.Day - 1;
   Time->QuadPart *= SECSPERDAY;
   Time->QuadPart += IntTimeFields.Hour * SECSPERHOUR + IntTimeFields.Minute * SECSPERMIN +
                     IntTimeFields.Second;
   Time->QuadPart *= TICKSPERSEC;
   Time->QuadPart += IntTimeFields.Milliseconds * TICKSPERMSEC;

   return TRUE;
}


/*
 * @implemented
 */
VOID
STDCALL
RtlTimeToElapsedTimeFields(IN PLARGE_INTEGER Time,
                           OUT PTIME_FIELDS TimeFields)
{
   ULONGLONG ElapsedSeconds;
   ULONG SecondsInDay;
   ULONG SecondsInMinute;

   /* Extract millisecond from time */
   TimeFields->Milliseconds = (CSHORT)((Time->QuadPart % TICKSPERSEC) / TICKSPERMSEC);

   /* Compute elapsed seconds */
   ElapsedSeconds = (ULONGLONG)Time->QuadPart / TICKSPERSEC;

   /* Compute seconds within the day */
   SecondsInDay = ElapsedSeconds % SECSPERDAY;

   /* Compute elapsed minutes within the day */
   SecondsInMinute = SecondsInDay % SECSPERHOUR;

   /* Compute elapsed time of day */
   TimeFields->Hour = (CSHORT)(SecondsInDay / SECSPERHOUR);
   TimeFields->Minute = (CSHORT)(SecondsInMinute / SECSPERMIN);
   TimeFields->Second = (CSHORT)(SecondsInMinute % SECSPERMIN);

   /* Compute elapsed days */
   TimeFields->Day = (CSHORT)(ElapsedSeconds / SECSPERDAY);

   /* The elapsed number of months and days cannot be calculated */
   TimeFields->Month = 0;
   TimeFields->Year = 0;
}


/*
 * @unimplemented
 */
VOID
STDCALL
RtlTimeToTimeFields(
   PLARGE_INTEGER Time,
   PTIME_FIELDS TimeFields)
{
   const int *Months;
   int SecondsInDay, CurYear;
   int LeapYear, CurMonth;
   long int Days;
   LONGLONG IntTime = (LONGLONG)Time->QuadPart;

   /* Extract millisecond from time and convert time into seconds */
   TimeFields->Milliseconds = (CSHORT) ((IntTime % TICKSPERSEC) / TICKSPERMSEC);
   IntTime = IntTime / TICKSPERSEC;

   /* Split the time into days and seconds within the day */
   Days = IntTime / SECSPERDAY;
   SecondsInDay = IntTime % SECSPERDAY;

   /* Adjust the values for days and seconds in day */
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
 * @implemented
 */
BOOLEAN
STDCALL
RtlTimeToSecondsSince1970(
   PLARGE_INTEGER Time,
   PULONG SecondsSince1970)
{
   LARGE_INTEGER IntTime;

   IntTime.QuadPart = Time->QuadPart - TICKSTO1970;
   IntTime.QuadPart = IntTime.QuadPart / TICKSPERSEC;

   if (IntTime.u.HighPart != 0)
      return FALSE;

   *SecondsSince1970 = IntTime.u.LowPart;

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
   LARGE_INTEGER IntTime;

   IntTime.QuadPart = Time->QuadPart - TICKSTO1980;
   IntTime.QuadPart = IntTime.QuadPart / TICKSPERSEC;

   if (IntTime.u.HighPart != 0)
      return FALSE;

   *SecondsSince1980 = IntTime.u.LowPart;

   return TRUE;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlLocalTimeToSystemTime(PLARGE_INTEGER LocalTime,
                         PLARGE_INTEGER SystemTime)
{
   SYSTEM_TIMEOFDAY_INFORMATION TimeInformation;
   NTSTATUS Status;

   Status = NtQuerySystemInformation(SystemTimeOfDayInformation,
                                     &TimeInformation,
                                     sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                                     NULL);
   if (!NT_SUCCESS(Status))
      return(Status);

   SystemTime->QuadPart = LocalTime->QuadPart +
                          TimeInformation.TimeZoneBias.QuadPart;

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
RtlSystemTimeToLocalTime(PLARGE_INTEGER SystemTime,
                         PLARGE_INTEGER LocalTime)
{
   SYSTEM_TIMEOFDAY_INFORMATION TimeInformation;
   NTSTATUS Status;

   Status = NtQuerySystemInformation(SystemTimeOfDayInformation,
                                     &TimeInformation,
                                     sizeof(SYSTEM_TIMEOFDAY_INFORMATION),
                                     NULL);
   if (!NT_SUCCESS(Status))
      return(Status);

   LocalTime->QuadPart = SystemTime->QuadPart -
                         TimeInformation.TimeZoneBias.QuadPart;

   return(STATUS_SUCCESS);
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
   Time->QuadPart = ((LONGLONG)SecondsSince1970 * TICKSPERSEC) + TICKSTO1970;
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
   Time->QuadPart = ((LONGLONG)SecondsSince1980 * TICKSPERSEC) + TICKSTO1980;
}

/* EOF */
