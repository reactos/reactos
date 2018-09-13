/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    stdtimep.h

Abstract:

    This module contains definitions and function prototypes which are local to
    stdime.c and fmttime.c.

Author:

    Rob McKaughan (t-robmc) 17-Jul-1991

Revision History:

--*/

#ifndef _STD_TIME_P_
#define _STD_TIME_P_

//
//  These are the magic numbers needed to do our extended division.  The
//  only numbers we ever need to divide by are
//
//      10,000 = convert 100ns tics to millisecond tics
//
//      10,000,000 = convert 100ns tics to one second tics
//
//      86,400,000 = convert Millisecond tics to one day tics
//

extern LARGE_INTEGER Magic10000;
#define SHIFT10000                       13

extern LARGE_INTEGER Magic10000000;
#define SHIFT10000000                    23

extern LARGE_INTEGER Magic86400000;
#define SHIFT86400000                    26

//
//  To make the code more readable we'll also define some macros to
//  do the actual division for use
//

#define Convert100nsToMilliseconds(LARGE_INTEGER) (                         \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic10000, SHIFT10000 )       \
    )

#define ConvertMillisecondsTo100ns(MILLISECONDS) (                 \
    RtlExtendedIntegerMultiply( (MILLISECONDS), 10000 )            \
    )

#define Convert100nsToSeconds(LARGE_INTEGER) (                              \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic10000000, SHIFT10000000 ) \
    )

#define ConvertSecondsTo100ns(SECONDS) (                           \
    RtlExtendedIntegerMultiply( (SECONDS), 10000000L )             \
    )

#define ConvertMillisecondsToDays(LARGE_INTEGER) (                          \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic86400000, SHIFT86400000 ) \
    )

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Macros for Time Differentials and Time Revisions                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// The following define the minimum and maximum possible values for the Time
// Differential Factor as defined by ISO 4031-1978.
//

#define MAX_STDTIME_TDF (780)
#define MIN_STDTIME_TDF (-720)

//
// The revision of this design (will be inserted in the revision field of any
// STANDARD_TIMEs created by this revision).
//

#define STDTIME_REVISION (4)


//
// The number of bits we need to shift to get to and from a revision in a
// StdTime.TdfAndRevision field.
//

#define STDTIME_REVISION_SHIFT 12


//
// USHORT
// ShiftStandardTimeRevision(
//    IN USHORT Rev
//    )
// Description:
//    This routine shifts the given revision number to its proper place for
//    storing in a STANDARD_TIME.TdfAndRevision field.
//

#define ShiftStandardTimeRevision(Rev)                                        \
   ((USHORT) ((Rev) << STDTIME_REVISION_SHIFT))


//
// The pre-shifted value of the current revision
//

#define SHIFTED_STDTIME_REVISION (ShiftStandardTimeRevision(STDTIME_REVISION))


//
// The bit mask used to mask a STANDARD_TIME.TdfAndRevision field to retrieve
// the Tdf value.
//

#define TDF_MASK ((USHORT) 0x0fff)


//
// USHORT
// MaskStandardTimeTdf(
//    IN USHORT Tdf
//    )
// Description:
//    This routine masks the given tdf field with TDF_MASK and returns the
//    result.
//
// BUG: Byte order dependant
//

#define MaskStandardTimeTdf(Tdf) ((USHORT) ((Tdf) & TDF_MASK))


//
// SHORT
// GetStandardTimeTdf(
//    IN STANDARD_TIME
//    )
// Description:
//    This routine gets the Time Differential Factor from a tdf field and
//    makes any adjustments necessary to preserve the sign of the TDF.
//    The resulting TDF is returned.
//
//    Since the TDF is stored as a signed 12 bit int, it's sign bit is the
//    bit 0x0800.  To make it a 16 bit negative, we subtract 0x1000 from the
//    bottome 12 bits of the TdfAndRevision field.
//
// BUG: Byte order dependant
//

#define GetStandardTimeTdf(StdTime)                                           \
   ((SHORT)                                                                   \
     (((StdTime)->TdfAndRevision) & 0x0800)                                   \
        ? (MaskStandardTimeTdf((StdTime)->TdfAndRevision) - 0x1000)           \
        : MaskStandardTimeTdf((StdTime)->TdfAndRevision)                      \
   )


//
// USHORT
// GetStandardTimeRev(
//    IN USHORT Tdf
//    )
// Description:
//    This routine gets the revision number from a tdf field and returns it
//    shifted back down to its place as a SHORT.
//

#define GetStandardTimeRev(StdTime)                                           \
   ((USHORT) (((StdTime)->TdfAndRevision) >> STDTIME_REVISION_SHIFT))



///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Tests for absolute and delta times                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

//
// BOOLEAN
// IsPositive(
//    IN LARGE_INTEGER Time
//    )
// Returns:
//    TRUE - if the time in Time is positive.
//    FALSE - if Time is negative.
//

#define IsPositive(Time)                                                      \
   ( ((Time).HighPart > 0) || (((Time).HighPart = 0) & ((Time).LowPart > 0)) )

//
// BOOLEAN
// IsAbsoluteTime(
//    IN PSTANDARDTIME Time
//    )
// Returns:
//    TRUE - if the given time is an absolute time
//    FALSE - If the given time is not an absolute time
//

#define IsAbsoluteTime(Time)                                                  \
   ( IsPositive(Time->SimpleTime) )


//
// BOOLEAN
// IsDeltaTime(
//    IN PSTANDARDTIME Time
//    )
// Returns:
//    TRUE - if the given time is a delta time
//    FALSE - If the given time is not a delta time
//

#define IsDeltaTime(Time)                                                     \
   ( !IsAbsoluteTime(Time) )


//
// BOOLEAN
// GreaterThanTime(
//    IN PLARGE_INTEGER Time1,
//    IN PLARGE_INTEGER Time2
//    )
// Returns:
//    TRUE - If Time1 is greater (older) than Time2
//    FALSE - If not
//
// BUG: Byte order dependant
// BUG: Only works on absolute times
//

#define GreaterThanTime(Time1, Time2)                                         \
   (                                                                          \
     ((Time1).HighPart > (Time2).HighPart)                                    \
     ||                                                                       \
     (                                                                        \
      ((Time1).HighPart == (Time2).HighPart)                                  \
      &&                                                                      \
      ((Time1).LowPart > (Time2).LowPart)                                     \
     )                                                                        \
   )


//
// BOOLEAN
// GreaterThanStandardTime(
//    IN PSTANDARD_TIME Time1,
//    IN PSTANDARD_TIME Time2
//    )
// Returns:
//    TRUE - If Time1 is greater (older) than Time2
//    FALSE - If not
//

#define GreaterThanStdTime(Time1, Time2) \
   GreaterThanTime((Time1).SimpleTime, (Time2).SimpleTime)



//////////////////////////////////////////////////////////////////////////////
//                                                                           /
//  The following definitions and declarations are some important constants  /
//  used in the time conversion routines                                     /
//                                                                           /
//////////////////////////////////////////////////////////////////////////////

//
//  This is the week day that January 1st, 1601 fell on (a Monday)
//

#define WEEKDAY_OF_1601                  1

//
//  These are known constants used to convert 1970 and 1980 times to 1601
//  times.  They are the number of seconds from the 1601 base to the start
//  of 1970 and the start of 1980.  The number of seconds from 1601 to
//  1970 is 369 years worth, or (369 * 365) + 89 leap days = 134774 days, or
//  134774 * 864000 seconds, which is equal to the large integer defined
//  below.  The number of seconds from 1601 to 1980 is 379 years worth, or etc.
//
//  These are declared in time.c
//

extern LARGE_INTEGER SecondsToStartOf1970;
extern LARGE_INTEGER SecondsToStartOf1980;


//
//  ULONG
//  ElapsedDaysToYears (
//      IN ULONG ElapsedDays
//      );
//
//  To be completely true to the Gregorian calendar the equation to
//  go from days to years is really
//
//      ElapsedDays / 365.2425
//
//  But because we are doing the computation in ulong integer arithmetic
//  and the LARGE_INTEGER variable limits the number of expressible days to around
//  11,000,000 we use the following computation
//
//      (ElapsedDays * 128 + 127) / (365.2425 * 128)
//
//  which will be off from the Gregorian calendar in about 150,000 years
//  but that doesn't really matter because LARGE_INTEGER can only express around
//  30,000 years
//

#define ElapsedDaysToYears(DAYS) ( \
    ((DAYS) * 128 + 127) / 46751   \
    )

//
//  ULONG
//  NumberOfLeapYears (
//      IN ULONG ElapsedYears
//      );
//
//  The number of leap years is simply the number of years divided by 4
//  minus years divided by 100 plus years divided by 400.  This says
//  that every four years is a leap year except centuries, and the
//  exception to the exception is the quadricenturies
//

#define NumberOfLeapYears(YEARS) (                    \
    ((YEARS) / 4) - ((YEARS) / 100) + ((YEARS) / 400) \
    )

//
//  ULONG
//  ElapsedYearsToDays (
//      IN ULONG ElapsedYears
//      );
//
//  The number of days contained in elapsed years is simply the number
//  of years times 365 (because every year has at least 365 days) plus
//  the number of leap years there are (i.e., the number of 366 days years)
//

#define ElapsedYearsToDays(YEARS) (            \
    ((YEARS) * 365) + NumberOfLeapYears(YEARS) \
    )

//
//  BOOLEAN
//  IsLeapYear (
//      IN ULONG ElapsedYears
//      );
//
//  If it is an even 400 or a non century leapyear then the
//  answer is true otherwise it's false
//

#define IsLeapYear(YEARS) (                        \
    (((YEARS) % 400 == 0) ||                       \
     ((YEARS) % 100 != 0) && ((YEARS) % 4 == 0)) ? \
        TRUE                                       \
    :                                              \
        FALSE                                      \
    )

//
//  ULONG
//  MaxDaysInMonth (
//      IN ULONG Year,
//      IN ULONG Month
//      );
//
//  The maximum number of days in a month depend on the year and month.
//  It is the difference between the days to the month and the days
//  to the following month
//

#define MaxDaysInMonth(YEAR,MONTH) (                                      \
    IsLeapYear(YEAR) ?                                                    \
        LeapYearDaysPrecedingMonth[(MONTH) + 1] -                         \
                                    LeapYearDaysPrecedingMonth[(MONTH)]   \
    :                                                                     \
        NormalYearDaysPrecedingMonth[(MONTH) + 1] -                       \
                                    NormalYearDaysPrecedingMonth[(MONTH)] \
    )


//
// Local utlity function prototypes
//

VOID
RtlpConvert48To64(
   IN PSTDTIME_ERROR num48,
   OUT LARGE_INTEGER *num64
   );

NTSTATUS
RtlpConvert64To48(
   IN LARGE_INTEGER num64,
   OUT PSTDTIME_ERROR num48
   );

LARGE_INTEGER
RtlpTimeToLargeInt(
   IN LARGE_INTEGER Time
   );

LARGE_INTEGER
RtlpLargeIntToTime(
   IN LARGE_INTEGER Int
   );

NTSTATUS
RtlpAdd48Int(
   IN PSTDTIME_ERROR First48,
   IN PSTDTIME_ERROR Second48,
   IN PSTDTIME_ERROR Result48
   );

NTSTATUS
RtlpAddTime(
   IN LARGE_INTEGER Time1,
   IN LARGE_INTEGER Time2,
   OUT PLARGE_INTEGER Result
   );

NTSTATUS
RtlpSubtractTime(
   IN LARGE_INTEGER Time1,
   IN LARGE_INTEGER Time2,
   OUT PLARGE_INTEGER Result
   );

LARGE_INTEGER
RtlpAbsTime(
   IN LARGE_INTEGER Time
   );

#endif //_STD_TIME_P_
