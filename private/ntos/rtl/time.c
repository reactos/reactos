/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Time.c

Abstract:

    This module implements the absolute time conversion routines for NT.

    Absolute LARGE_INTEGER in NT is represented by a 64-bit large integer accurate
    to 100ns resolution.  The smallest time resolution used by this package
    is One millisecond.  The basis for NT time is the start of 1601 which
    was chosen because it is the start of a new quadricentury.  Some facts
    to note are:

    o At 100ns resolution 32 bits is good for about 429 seconds (or 7 minutes)

    o At 100ns resolution a large integer (i.e., 63 bits) is good for
      about 29,247 years, or around 10,682,247 days.

    o At 1 second resolution 31 bits is good for about 68 years

    o At 1 second resolution 32 bits is good for about 136 years

    o 100ns Time (ignoring time less than a millisecond) can be expressed
      as two values, Days and Milliseconds.  Where Days is the number of
      whole days and Milliseconds is the number of milliseconds for the
      partial day.  Both of these values are ULONG.

    Given these facts most of the conversions are done by first splitting
    LARGE_INTEGER into Days and Milliseconds.

Author:

    Gary Kimura     [GaryKi]    26-Aug-1989

Environment:

    Pure utility routine

Revision History:

--*/

#include "ntrtlp.h"

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE, RtlCutoverTimeToSystemTime)
#pragma alloc_text(PAGE, RtlTimeToElapsedTimeFields)
#endif


//
//  The following two tables map a day offset within a year to the month
//  containing the day.  Both tables are zero based.  For example, day
//  offset of 0 to 30 map to 0 (which is Jan).
//

CONST UCHAR LeapYearDayToMonth[366] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // January
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,        // February
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // March
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // April
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // May
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,     // June
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  // July
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  // August
     8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // September
     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  // October
    10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,     // November
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11}; // December

CONST UCHAR NormalYearDayToMonth[365] = {
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // January
     1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,           // February
     2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // March
     3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,     // April
     4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // May
     5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,     // June
     6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  // July
     7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,  // August
     8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,     // September
     9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,  // October
    10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,     // November
    11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11}; // December

//
//  The following two tables map a month index to the number of days preceding
//  the month in the year.  Both tables are zero based.  For example, 1 (Feb)
//  has 31 days preceding it.  To help calculate the maximum number of days
//  in a month each table has 13 entries, so the number of days in a month
//  of index i is the table entry of i+1 minus the table entry of i.
//

CONST CSHORT LeapYearDaysPrecedingMonth[13] = {
    0,                                 // January
    31,                                // February
    31+29,                             // March
    31+29+31,                          // April
    31+29+31+30,                       // May
    31+29+31+30+31,                    // June
    31+29+31+30+31+30,                 // July
    31+29+31+30+31+30+31,              // August
    31+29+31+30+31+30+31+31,           // September
    31+29+31+30+31+30+31+31+30,        // October
    31+29+31+30+31+30+31+31+30+31,     // November
    31+29+31+30+31+30+31+31+30+31+30,  // December
    31+29+31+30+31+30+31+31+30+31+30+31};

CONST CSHORT NormalYearDaysPrecedingMonth[13] = {
    0,                                 // January
    31,                                // February
    31+28,                             // March
    31+28+31,                          // April
    31+28+31+30,                       // May
    31+28+31+30+31,                    // June
    31+28+31+30+31+30,                 // July
    31+28+31+30+31+30+31,              // August
    31+28+31+30+31+30+31+31,           // September
    31+28+31+30+31+30+31+31+30,        // October
    31+28+31+30+31+30+31+31+30+31,     // November
    31+28+31+30+31+30+31+31+30+31+30,  // December
    31+28+31+30+31+30+31+31+30+31+30+31};


//
//  The following definitions and declarations are some important constants
//  used in the time conversion routines
//

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

const LARGE_INTEGER SecondsToStartOf1970 = {0xb6109100, 0x00000002};

const LARGE_INTEGER SecondsToStartOf1980 = {0xc8df3700, 0x00000002};

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

const LARGE_INTEGER Magic10000    = {0xe219652c, 0xd1b71758};
#define SHIFT10000                       13

const LARGE_INTEGER Magic10000000 = {0xe57a42bd, 0xd6bf94d5};
#define SHIFT10000000                    23

const LARGE_INTEGER Magic86400000 = {0xfa67b90e, 0xc6d750eb};
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
    RtlExtendedIntegerMultiply( (SECONDS), 10000000 )              \
    )

#define ConvertMillisecondsToDays(LARGE_INTEGER) (                          \
    RtlExtendedMagicDivide( (LARGE_INTEGER), Magic86400000, SHIFT86400000 ) \
    )

#define ConvertDaysToMilliseconds(DAYS) (                          \
    Int32x32To64( (DAYS), 86400000 )                               \
    )


//
//  Local support routine
//

ULONG
ElapsedDaysToYears (
    IN ULONG ElapsedDays
    )

/*++

Routine Description:

    This routine computes the number of total years contained in the indicated
    number of elapsed days.  The computation is to first compute the number of
    400 years and subtract that it, then do the 100 years and subtract that out,
    then do the number of 4 years and subtract that out.  Then what we have left
    is the number of days with in a normalized 4 year block.  Normalized being that
    the first three years are not leap years.

Arguments:

    ElapsedDays - Supplies the number of days to use

Return Value:

    ULONG - Returns the number of whole years contained within the input number
        of days.

--*/

{
    ULONG NumberOf400s;
    ULONG NumberOf100s;
    ULONG NumberOf4s;
    ULONG Years;

    //
    //  A 400 year time block is 365*400 + 400/4 - 400/100 + 400/400 = 146097 days
    //  long.  So we simply compute the number of whole 400 year block and the
    //  the number days contained in those whole blocks, and subtract if from the
    //  elapsed day total
    //

    NumberOf400s = ElapsedDays / 146097;
    ElapsedDays -= NumberOf400s * 146097;

    //
    //  A 100 year time block is 365*100 + 100/4 - 100/100 = 36524 days long.
    //  The computation for the number of 100 year blocks is biased by 3/4 days per
    //  100 years to account for the extra leap day thrown in on the last year
    //  of each 400 year block.
    //

    NumberOf100s = (ElapsedDays * 100 + 75) / 3652425;
    ElapsedDays -= NumberOf100s * 36524;

    //
    //  A 4 year time block is 365*4 + 4/4 = 1461 days long.
    //

    NumberOf4s = ElapsedDays / 1461;
    ElapsedDays -= NumberOf4s * 1461;

    //
    //  Now the number of whole years is the number of 400 year blocks times 400,
    //  100 year blocks time 100, 4 year blocks times 4, and the number of elapsed
    //  whole years, taking into account the 3/4 day per year needed to handle the
    //  leap year.
    //

    Years = (NumberOf400s * 400) +
            (NumberOf100s * 100) +
            (NumberOf4s * 4) +
            (ElapsedDays * 100 + 75) / 36525;

    return Years;
}


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
//  Internal Support routine
//

static
VOID
TimeToDaysAndFraction (
    IN PLARGE_INTEGER Time,
    OUT PULONG ElapsedDays,
    OUT PULONG Milliseconds
    )

/*++

Routine Description:

    This routine converts an input 64-bit time value to the number
    of total elapsed days and the number of milliseconds in the
    partial day.

Arguments:

    Time - Supplies the input time to convert from

    ElapsedDays - Receives the number of elapsed days

    Milliseconds - Receives the number of milliseconds in the partial day

Return Value:

    None

--*/

{
    LARGE_INTEGER TotalMilliseconds;
    LARGE_INTEGER Temp;

    //
    //  Convert the input time to total milliseconds
    //

    TotalMilliseconds = Convert100nsToMilliseconds( *(PLARGE_INTEGER)Time );

    //
    //  Convert milliseconds to total days
    //

    Temp = ConvertMillisecondsToDays( TotalMilliseconds );

    //
    //  Set the elapsed days from temp, we've divided it enough so that
    //  the high part must be zero.
    //

    *ElapsedDays = Temp.LowPart;

    //
    //  Calculate the exact number of milliseconds in the elapsed days
    //  and subtract that from the total milliseconds to figure out
    //  the number of milliseconds left in the partial day
    //

    Temp.QuadPart = ConvertDaysToMilliseconds( *ElapsedDays );

    Temp.QuadPart = TotalMilliseconds.QuadPart - Temp.QuadPart;

    //
    //  Set the fraction part from temp, the total number of milliseconds in
    //  a day guarantees that the high part must be zero.
    //

    *Milliseconds = Temp.LowPart;

    //
    //  And return to our caller
    //

    return;
}


//
//  Internal Support routine
//

//static
VOID
DaysAndFractionToTime (
    IN ULONG ElapsedDays,
    IN ULONG Milliseconds,
    OUT PLARGE_INTEGER Time
    )

/*++

Routine Description:

    This routine converts an input elapsed day count and partial time
    in milliseconds to a 64-bit time value.

Arguments:

    ElapsedDays - Supplies the number of elapsed days

    Milliseconds - Supplies the number of milliseconds in the partial day

    Time - Receives the output time to value

Return Value:

    None

--*/

{
    LARGE_INTEGER Temp;
    LARGE_INTEGER Temp2;

    //
    //  Calculate the exact number of milliseconds in the elapsed days.
    //

    Temp.QuadPart = ConvertDaysToMilliseconds( ElapsedDays );

    //
    //  Convert milliseconds to a large integer
    //

    Temp2.LowPart = Milliseconds;
    Temp2.HighPart = 0;

    //
    //  add milliseconds to the whole day milliseconds
    //

    Temp.QuadPart = Temp.QuadPart + Temp2.QuadPart;

    //
    //  Finally convert the milliseconds to 100ns resolution
    //

    *(PLARGE_INTEGER)Time = ConvertMillisecondsTo100ns( Temp );

    //
    //  and return to our caller
    //

    return;
}


VOID
RtlTimeToTimeFields (
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine converts an input 64-bit LARGE_INTEGER variable to its corresponding
    time field record.  It will tell the caller the year, month, day, hour,
    minute, second, millisecond, and weekday corresponding to the input time
    variable.

Arguments:

    Time - Supplies the time value to interpret

    TimeFields - Receives a value corresponding to Time

Return Value:

    None

--*/

{
    ULONG Years;
    ULONG Month;
    ULONG Days;

    ULONG Hours;
    ULONG Minutes;
    ULONG Seconds;
    ULONG Milliseconds;

    //
    //  First divide the input time 64 bit time variable into
    //  the number of whole days and part days (in milliseconds)
    //

    TimeToDaysAndFraction( Time, &Days, &Milliseconds );

    //
    //  Compute which weekday it is and save it away now in the output
    //  variable.  We add the weekday of the base day to bias our computation
    //  which means that if one day has elapsed then we the weekday we want
    //  is the Jan 2nd, 1601.
    //

    TimeFields->Weekday = (CSHORT)((Days + WEEKDAY_OF_1601) % 7);

    //
    //  Calculate the number of whole years contained in the elapsed days
    //  For example if Days = 500 then Years = 1
    //

    Years = ElapsedDaysToYears( Days );

    //
    //  And subtract the number of whole years from our elapsed days
    //  For example if Days = 500, Years = 1, and the new days is equal
    //  to 500 - 365 (normal year).
    //

    Days = Days - ElapsedYearsToDays( Years );

    //
    //  Now test whether the year we are working on (i.e., The year
    //  after the total number of elapsed years) is a leap year
    //  or not.
    //

    if (IsLeapYear( Years + 1 )) {

        //
        //  The current year is a leap year, so figure out what month
        //  it is, and then subtract the number of days preceding the
        //  month from the days to figure out what day of the month it is
        //

        Month = LeapYearDayToMonth[Days];
        Days = Days - LeapYearDaysPrecedingMonth[Month];

    } else {

        //
        //  The current year is a normal year, so figure out the month
        //  and days as described above for the leap year case
        //

        Month = NormalYearDayToMonth[Days];
        Days = Days - NormalYearDaysPrecedingMonth[Month];

    }

    //
    //  Now we need to compute the elapsed hour, minute, second, milliseconds
    //  from the millisecond variable.  This variable currently contains
    //  the number of milliseconds in our input time variable that did not
    //  fit into a whole day.  To compute the hour, minute, second part
    //  we will actually do the arithmetic backwards computing milliseconds
    //  seconds, minutes, and then hours.  We start by computing the
    //  number of whole seconds left in the day, and then computing
    //  the millisecond remainder.
    //

    Seconds = Milliseconds / 1000;
    Milliseconds = Milliseconds % 1000;

    //
    //  Now we compute the number of whole minutes left in the day
    //  and the number of remainder seconds
    //

    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    //
    //  Now compute the number of whole hours left in the day
    //  and the number of remainder minutes
    //

    Hours = Minutes / 60;
    Minutes = Minutes % 60;

    //
    //  As our final step we put everything into the time fields
    //  output variable
    //

    TimeFields->Year         = (CSHORT)(Years + 1601);
    TimeFields->Month        = (CSHORT)(Month + 1);
    TimeFields->Day          = (CSHORT)(Days + 1);
    TimeFields->Hour         = (CSHORT)Hours;
    TimeFields->Minute       = (CSHORT)Minutes;
    TimeFields->Second       = (CSHORT)Seconds;
    TimeFields->Milliseconds = (CSHORT)Milliseconds;

    //
    //  and return to our caller
    //

    return;
}

BOOLEAN
RtlCutoverTimeToSystemTime(
    PTIME_FIELDS CutoverTime,
    PLARGE_INTEGER SystemTime,
    PLARGE_INTEGER CurrentSystemTime,
    BOOLEAN ThisYear
    )
{
    TIME_FIELDS CurrentTimeFields;

    //
    // Get the current system time
    //

    RtlTimeToTimeFields(CurrentSystemTime,&CurrentTimeFields);

    //
    // check for absolute time field. If the year is specified,
    // the the time is an abosulte time
    //

    if ( CutoverTime->Year ) {

        //
        // Convert this to a time value and make sure it
        // is greater than the current system time
        //

        if ( !RtlTimeFieldsToTime(CutoverTime,SystemTime) ) {
            return FALSE;
            }

        if (SystemTime->QuadPart < CurrentSystemTime->QuadPart) {
            return FALSE;
            }
        return TRUE;
        }
    else {

        TIME_FIELDS WorkingTimeField;
        TIME_FIELDS ScratchTimeField;
        LARGE_INTEGER ScratchTime;
        CSHORT BestWeekdayDate;
        CSHORT WorkingWeekdayNumber;
        CSHORT TargetWeekdayNumber;
        CSHORT TargetYear;
        CSHORT TargetMonth;
        CSHORT TargetWeekday;     // range [0..6] == [Sunday..Saturday]
        BOOLEAN MonthMatches;
        //
        // The time is an day in the month style time
        //
        // the convention is the Day is 1-5 specifying 1st, 2nd... Last
        // day within the month. The day is WeekDay.
        //

        //
        // Compute the target month and year
        //

        TargetWeekdayNumber = CutoverTime->Day;
        if ( TargetWeekdayNumber > 5 || TargetWeekdayNumber == 0 ) {
            return FALSE;
            }
        TargetWeekday = CutoverTime->Weekday;
        TargetMonth = CutoverTime->Month;
        MonthMatches = FALSE;
        if ( !ThisYear ) {
            if ( TargetMonth < CurrentTimeFields.Month ) {
                TargetYear = CurrentTimeFields.Year + 1;
                }
            else if ( TargetMonth > CurrentTimeFields.Month ) {
                TargetYear = CurrentTimeFields.Year;
                }
            else {
                TargetYear = CurrentTimeFields.Year;
                MonthMatches = TRUE;
                }
            }
        else {
            TargetYear = CurrentTimeFields.Year;
            }
try_next_year:
        BestWeekdayDate = 0;

        WorkingTimeField.Year = TargetYear;
        WorkingTimeField.Month = TargetMonth;
        WorkingTimeField.Day = 1;
        WorkingTimeField.Hour = CutoverTime->Hour;
        WorkingTimeField.Minute = CutoverTime->Minute;
        WorkingTimeField.Second = CutoverTime->Second;
        WorkingTimeField.Milliseconds = CutoverTime->Milliseconds;
        WorkingTimeField.Weekday = 0;

        //
        // Convert to time and then back to time fields so we can determine
        // the weekday of day 1 on the month
        //

        if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
            return FALSE;
            }
        RtlTimeToTimeFields(&ScratchTime,&ScratchTimeField);

        //
        // Compute bias to target weekday
        //
        if ( ScratchTimeField.Weekday > TargetWeekday ) {
            WorkingTimeField.Day += (7-(ScratchTimeField.Weekday - TargetWeekday));
            }
        else if ( ScratchTimeField.Weekday < TargetWeekday ) {
            WorkingTimeField.Day += (TargetWeekday - ScratchTimeField.Weekday);
            }

        //
        // We are now at the first weekday that matches our target weekday
        //

        BestWeekdayDate = WorkingTimeField.Day;
        WorkingWeekdayNumber = 1;

        //
        // Keep going one week at a time until we either pass the
        // target weekday, or we match exactly
        //

        while ( WorkingWeekdayNumber < TargetWeekdayNumber ) {
            WorkingTimeField.Day += 7;
            if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
                break;
                }
            RtlTimeToTimeFields(&ScratchTime,&ScratchTimeField);
            WorkingWeekdayNumber++;
            BestWeekdayDate = ScratchTimeField.Day;
            }
        WorkingTimeField.Day = BestWeekdayDate;

        //
        // If the months match, and the date is less than the current
        // date, then be have to go to next year.
        //

        if ( !RtlTimeFieldsToTime(&WorkingTimeField,&ScratchTime) ) {
            return FALSE;
            }
        if ( MonthMatches ) {
            if ( WorkingTimeField.Day < CurrentTimeFields.Day ) {
                MonthMatches = FALSE;
                TargetYear++;
                goto try_next_year;
                }
            if ( WorkingTimeField.Day == CurrentTimeFields.Day ) {

                if (ScratchTime.QuadPart < CurrentSystemTime->QuadPart) {
                    MonthMatches = FALSE;
                    TargetYear++;
                    goto try_next_year;
                    }
                }
            }
        *SystemTime = ScratchTime;

        return TRUE;
        }
}


BOOLEAN
RtlTimeFieldsToTime (
    IN PTIME_FIELDS TimeFields,
    OUT PLARGE_INTEGER Time
    )

/*++

Routine Description:

    This routine converts an input Time Field variable to a 64-bit NT time
    value.  It ignores the WeekDay of the time field.

Arguments:

    TimeFields - Supplies the time field record to use

    Time - Receives the NT Time corresponding to TimeFields

Return Value:

    BOOLEAN - TRUE if the Time Fields is well formed and within the
        range of time expressible by LARGE_INTEGER and FALSE otherwise.

--*/

{
    ULONG Year;
    ULONG Month;
    ULONG Day;
    ULONG Hour;
    ULONG Minute;
    ULONG Second;
    ULONG Milliseconds;

    ULONG ElapsedDays;
    ULONG ElapsedMilliseconds;

    //
    //  Load the time field elements into local variables.  This should
    //  ensure that the compiler will only load the input elements
    //  once, even if there are alias problems.  It will also make
    //  everything (except the year) zero based.  We cannot zero base the
    //  year because then we can't recognize cases where we're given a year
    //  before 1601.
    //

    Year         = TimeFields->Year;
    Month        = TimeFields->Month - 1;
    Day          = TimeFields->Day - 1;
    Hour         = TimeFields->Hour;
    Minute       = TimeFields->Minute;
    Second       = TimeFields->Second;
    Milliseconds = TimeFields->Milliseconds;

    //
    //  Check that the time field input variable contains
    //  proper values.
    //

    if ((TimeFields->Month < 1)                      ||
        (TimeFields->Day < 1)                        ||
        (Year < 1601)                                ||
        (Month > 11)                                 ||
        ((CSHORT)Day >= MaxDaysInMonth(Year, Month)) ||
        (Hour > 23)                                  ||
        (Minute > 59)                                ||
        (Second > 59)                                ||
        (Milliseconds > 999)) {

        return FALSE;

    }

    //
    //  Compute the total number of elapsed days represented by the
    //  input time field variable
    //

    ElapsedDays = ElapsedYearsToDays( Year - 1601 );

    if (IsLeapYear( Year - 1600 )) {

        ElapsedDays += LeapYearDaysPrecedingMonth[ Month ];

    } else {

        ElapsedDays += NormalYearDaysPrecedingMonth[ Month ];

    }

    ElapsedDays += Day;

    //
    //  Now compute the total number of milliseconds in the fractional
    //  part of the day
    //

    ElapsedMilliseconds = (((Hour*60) + Minute)*60 + Second)*1000 + Milliseconds;

    //
    //  Given the elapsed days and milliseconds we can now build
    //  the output time variable
    //

    DaysAndFractionToTime( ElapsedDays, ElapsedMilliseconds, Time );

    //
    //  And return to our caller
    //

    return TRUE;
}


VOID
RtlTimeToElapsedTimeFields (
    IN PLARGE_INTEGER Time,
    OUT PTIME_FIELDS TimeFields
    )

/*++

Routine Description:

    This routine converts an input 64-bit LARGE_INTEGER variable to its corresponding
    time field record.  The input time is the elapsed time (difference
    between to times).  It will tell the caller the number of days, hour,
    minute, second, and milliseconds that the elapsed time represents.

Arguments:

    Time - Supplies the time value to interpret

    TimeFields - Receives a value corresponding to Time

Return Value:

    None

--*/

{
    ULONG Days;
    ULONG Hours;
    ULONG Minutes;
    ULONG Seconds;
    ULONG Milliseconds;

    //
    //  First divide the input time 64 bit time variable into
    //  the number of whole days and part days (in milliseconds)
    //

    TimeToDaysAndFraction( Time, &Days, &Milliseconds );

    //
    //  Now we need to compute the elapsed hour, minute, second, milliseconds
    //  from the millisecond variable.  This variable currently contains
    //  the number of milliseconds in our input time variable that did not
    //  fit into a whole day.  To compute the hour, minute, second part
    //  we will actually do the arithmetic backwards computing milliseconds
    //  seconds, minutes, and then hours.  We start by computing the
    //  number of whole seconds left in the day, and then computing
    //  the millisecond remainder.
    //

    Seconds = Milliseconds / 1000;
    Milliseconds = Milliseconds % 1000;

    //
    //  Now we compute the number of whole minutes left in the day
    //  and the number of remainder seconds
    //

    Minutes = Seconds / 60;
    Seconds = Seconds % 60;

    //
    //  Now compute the number of whole hours left in the day
    //  and the number of remainder minutes
    //

    Hours = Minutes / 60;
    Minutes = Minutes % 60;

    //
    //  As our final step we put everything into the time fields
    //  output variable
    //

    TimeFields->Year         = 0;
    TimeFields->Month        = 0;
    TimeFields->Day          = (CSHORT)Days;
    TimeFields->Hour         = (CSHORT)Hours;
    TimeFields->Minute       = (CSHORT)Minutes;
    TimeFields->Second       = (CSHORT)Seconds;
    TimeFields->Milliseconds = (CSHORT)Milliseconds;

    //
    //  and return to our caller
    //

    return;
}


BOOLEAN
RtlTimeToSecondsSince1980 (
    IN PLARGE_INTEGER Time,
    OUT PULONG ElapsedSeconds
    )

/*++

Routine Description:

    This routine converts an input 64-bit NT Time variable to the
    number of seconds since the start of 1980.  The NT time must be
    within the range 1980 to around 2115.

Arguments:

    Time - Supplies the Time to convert from

    ElapsedSeconds - Receives the number of seconds since the start of 1980
        denoted by Time

Return Value:

    BOOLEAN - TRUE if the input Time is within a range expressible by
        ElapsedSeconds and FALSE otherwise

--*/

{
    LARGE_INTEGER Seconds;

    //
    //  First convert time to seconds since 1601
    //

    Seconds = Convert100nsToSeconds( *(PLARGE_INTEGER)Time );

    //
    //  Then subtract the number of seconds from 1601 to 1980.
    //

    Seconds.QuadPart = Seconds.QuadPart - SecondsToStartOf1980.QuadPart;

    //
    //  If the results is negative then the date was before 1980 or if
    //  the results is greater than a ulong then its too far in the
    //  future so we return FALSE
    //

    if (Seconds.HighPart != 0) {

        return FALSE;

    }

    //
    //  Otherwise we have the answer
    //

    *ElapsedSeconds = Seconds.LowPart;

    //
    //  And return to our caller
    //

    return TRUE;
}


VOID
RtlSecondsSince1980ToTime (
    IN ULONG ElapsedSeconds,
    OUT PLARGE_INTEGER Time
    )

/*++

Routine Description:

    This routine converts the seconds since the start of 1980 to an
    NT Time value.

Arguments:

    ElapsedSeconds - Supplies the number of seconds from the start of 1980
        to convert from

    Time - Receives the converted Time value

Return Value:

    None

--*/

{
    LARGE_INTEGER Seconds;

    //
    //  Move elapsed seconds to a large integer
    //

    Seconds.LowPart = ElapsedSeconds;
    Seconds.HighPart = 0;

    //
    //  convert number of seconds from 1980 to number of seconds from 1601
    //

    Seconds.QuadPart = Seconds.QuadPart + SecondsToStartOf1980.QuadPart;

    //
    //  Convert seconds to 100ns resolution
    //

    *(PLARGE_INTEGER)Time = ConvertSecondsTo100ns( Seconds );

    //
    //  and return to our caller
    //

    return;
}


BOOLEAN
RtlTimeToSecondsSince1970 (
    IN PLARGE_INTEGER Time,
    OUT PULONG ElapsedSeconds
    )

/*++

Routine Description:

    This routine converts an input 64-bit NT Time variable to the
    number of seconds since the start of 1970.  The NT time must be
    within the range 1970 to around 2105.

Arguments:

    Time - Supplies the Time to convert from

    ElapsedSeconds - Receives the number of seconds since the start of 1970
        denoted by Time

Return Value:

    BOOLEAN - TRUE if the input time is within the range expressible by
        ElapsedSeconds and FALSE otherwise

--*/

{
    LARGE_INTEGER Seconds;

    //
    //  First convert time to seconds since 1601
    //

    Seconds = Convert100nsToSeconds( *(PLARGE_INTEGER)Time );

    //
    //  Then subtract the number of seconds from 1601 to 1970.
    //

    Seconds.QuadPart = Seconds.QuadPart - SecondsToStartOf1970.QuadPart;

    //
    //  If the results is negative then the date was before 1970 or if
    //  the results is greater than a ulong then its too far in the
    //  future so we return FALSE
    //

    if (Seconds.HighPart != 0) {

        return FALSE;

    }

    //
    //  Otherwise we have the answer
    //

    *ElapsedSeconds = Seconds.LowPart;

    //
    //  And return to our caller
    //

    return TRUE;
}


VOID
RtlSecondsSince1970ToTime (
    IN ULONG ElapsedSeconds,
    OUT PLARGE_INTEGER Time
    )

/*++

Routine Description:

    This routine converts the seconds since the start of 1970 to an
    NT Time value

Arguments:

    ElapsedSeconds - Supplies the number of seconds from the start of 1970
        to convert from

    Time - Receives the converted Time value

Return Value:

    None

--*/

{
    LARGE_INTEGER Seconds;

    //
    //  Move elapsed seconds to a large integer
    //

    Seconds.LowPart = ElapsedSeconds;
    Seconds.HighPart = 0;

    //
    //  Convert number of seconds from 1970 to number of seconds from 1601
    //

    Seconds.QuadPart = Seconds.QuadPart + SecondsToStartOf1970.QuadPart;

    //
    //  Convert seconds to 100ns resolution
    //

    *(PLARGE_INTEGER)Time = ConvertSecondsTo100ns( Seconds );

    //
    //  return to our caller
    //

    return;
}

NTSTATUS
RtlSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    )
{
    NTSTATUS Status;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;

    Status = ZwQuerySystemInformation(
                SystemTimeOfDayInformation,
                &TimeOfDay,
                sizeof(TimeOfDay),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
        }

    //
    // LocalTime = SystemTime - TimeZoneBias
    //

    LocalTime->QuadPart = SystemTime->QuadPart - TimeOfDay.TimeZoneBias.QuadPart;

    return STATUS_SUCCESS;
}

NTSTATUS
RtlLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    )
{

    NTSTATUS Status;
    SYSTEM_TIMEOFDAY_INFORMATION TimeOfDay;

    Status = ZwQuerySystemInformation(
                SystemTimeOfDayInformation,
                &TimeOfDay,
                sizeof(TimeOfDay),
                NULL
                );
    if ( !NT_SUCCESS(Status) ) {
        return Status;
        }

    //
    // SystemTime = LocalTime + TimeZoneBias
    //

    SystemTime->QuadPart = LocalTime->QuadPart + TimeOfDay.TimeZoneBias.QuadPart;

    return STATUS_SUCCESS;
}
