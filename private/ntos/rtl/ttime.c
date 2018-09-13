/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ttime.c

Abstract:

    Test program for the time conversion package

Author:

    Gary Kimura     [GaryKi]    27-Aug-1989

Revision History:

--*/

#include <stdio.h>

#include "nt.h"
#include "ntrtl.h"

VOID
PrintTimeFields(
    IN PTIME_FIELDS TimeFields
    );

LARGE_INTEGER Zero;
LARGE_INTEGER OneSecond;
LARGE_INTEGER OneMinute;
LARGE_INTEGER OneHour;
LARGE_INTEGER OneDay;
LARGE_INTEGER OneWeek;
LARGE_INTEGER OneNormalYear;
LARGE_INTEGER OneLeapYear;
LARGE_INTEGER OneCentury;
LARGE_INTEGER TwoCenturies;
LARGE_INTEGER ThreeCenturies;
LARGE_INTEGER FourCenturies;

LARGE_INTEGER Sum;

TIME_FIELDS TimeFields;
LARGE_INTEGER Time;

LARGE_INTEGER StartOf1970;
LARGE_INTEGER StartOf1980;

int
main(
    int argc,
    char *argv[]
    )
{
    ULONG i;

    //
    //  We're starting the test
    //

    DbgPrint("Start Time Test\n");

    //
    //  Start by initializing some constants and making sure they
    //  are correct
    //

    Zero.QuadPart = 0;
    OneSecond.QuadPart = 10000000;
    OneMinute.QuadPart = OneSecond.QuadPart * 60;
    OneHour.QuadPart = OneMinute.QuadPart * 60;
    OneDay.QuadPart = OneHour.QuadPart * 24;
    OneWeek.QuadPart = OneDay.QuadPart * 7;
    OneNormalYear.QuadPart = OneDay.QuadPart * 365;
    OneLeapYear.QuadPart = OneDay.QuadPart * 366;
    OneCentury.QuadPart =  (OneNormalYear.QuadPart * 76) + (OneLeapYear.QuadPart * 24);
    TwoCenturies.QuadPart = OneCentury.QuadPart * 2;
    ThreeCenturies.QuadPart = OneCentury.QuadPart * 3;
    FourCenturies.QuadPart = (OneCentury.QuadPart * 4) + OneDay.QuadPart;

    Sum.QuadPart   = Zero.QuadPart +
                     OneSecond.QuadPart +
                     OneMinute.QuadPart +
                     OneHour.QuadPart +
                     OneDay.QuadPart +
                     OneWeek.QuadPart +
                     OneNormalYear.QuadPart +
                     ThreeCenturies.QuadPart;


    RtlTimeToTimeFields( (PLARGE_INTEGER)&Zero, &TimeFields );
    DbgPrint("StartOf1601     = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to Zero\n");
    }
    if ((Time.LowPart != Zero.LowPart) || (Time.HighPart != Zero.HighPart)) {
        DbgPrint("****ERROR Time != Zero\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneSecond, &TimeFields );
    DbgPrint(" + 1 Second     = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneSecond\n");
    }
    if ((Time.LowPart != OneSecond.LowPart) || (Time.HighPart != OneSecond.HighPart)) {
        DbgPrint("****ERROR Time != OneSecond\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneMinute, &TimeFields );
    DbgPrint(" + 1 Minute     = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneMinute\n");
    }
    if ((Time.LowPart != OneMinute.LowPart) || (Time.HighPart != OneMinute.HighPart)) {
        DbgPrint("****ERROR Time != OneMinute\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneHour, &TimeFields );
    DbgPrint(" + 1 Hour       = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneHour\n");
    }
    if ((Time.LowPart != OneHour.LowPart) || (Time.HighPart != OneHour.HighPart)) {
        DbgPrint("****ERROR Time != OneHour\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneDay, &TimeFields );
    DbgPrint(" + 1 Day        = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneDay\n");
    }
    if ((Time.LowPart != OneDay.LowPart) || (Time.HighPart != OneDay.HighPart)) {
        DbgPrint("****ERROR Time != OneDay\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneWeek, &TimeFields );
    DbgPrint(" + 1 Week       = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneWeek\n");
    }
    if ((Time.LowPart != OneWeek.LowPart) || (Time.HighPart != OneWeek.HighPart)) {
        DbgPrint("****ERROR Time != OneWeek\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneNormalYear, &TimeFields );
    DbgPrint(" + 1 NormalYear = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneNormalYear\n");
    }
    if ((Time.LowPart != OneNormalYear.LowPart) || (Time.HighPart != OneNormalYear.HighPart)) {
        DbgPrint("****ERROR Time != OneNormalYear\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneLeapYear, &TimeFields );
    DbgPrint(" + 1 LeapYear   = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneLeapYear\n");
    }
    if ((Time.LowPart != OneLeapYear.LowPart) || (Time.HighPart != OneLeapYear.HighPart)) {
        DbgPrint("****ERROR Time != OneLeapYear\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&OneCentury, &TimeFields );
    DbgPrint(" + 1 Century    = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to OneCentury\n");
    }
    if ((Time.LowPart != OneCentury.LowPart) || (Time.HighPart != OneCentury.HighPart)) {
        DbgPrint("****ERROR Time != OneCentury\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&TwoCenturies, &TimeFields );
    DbgPrint(" + 2 Centuries  = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to TwoCenturies\n");
    }
    if ((Time.LowPart != TwoCenturies.LowPart) || (Time.HighPart != TwoCenturies.HighPart)) {
        DbgPrint("****ERROR Time != TwoCenturies\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&ThreeCenturies, &TimeFields );
    DbgPrint(" + 3 Centuries  = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to ThreeCenturies\n");
    }
    if ((Time.LowPart != ThreeCenturies.LowPart) || (Time.HighPart != ThreeCenturies.HighPart)) {
        DbgPrint("****ERROR Time != ThreeCenturies\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&FourCenturies, &TimeFields );
    DbgPrint(" + 4 Centuries  = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to FourCenturies\n");
    }
    if ((Time.LowPart != FourCenturies.LowPart) || (Time.HighPart != FourCenturies.HighPart)) {
        DbgPrint("****ERROR Time != FourCenturies\n");
    }

    RtlTimeToTimeFields( (PLARGE_INTEGER)&Sum, &TimeFields );
    DbgPrint(" + sum          = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to Sum\n");
    }
    if ((Time.LowPart != Sum.LowPart) || (Time.HighPart != Sum.HighPart)) {
        DbgPrint("****ERROR Time != Sum\n");
    }

    DbgPrint("\n");

    //
    //  Setup and test the start 1970 time
    //

    RtlSecondsSince1970ToTime( 0, &StartOf1970 );
    RtlTimeToTimeFields( &StartOf1970, &TimeFields );
    DbgPrint(" Start of 1970  = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to start of 1970\n");
    }
    if ((Time.LowPart != StartOf1970.LowPart) || (Time.HighPart != StartOf1970.HighPart)) {
        DbgPrint("****ERROR Time != StartOf1970\n");
    }
    if (!RtlTimeToSecondsSince1970( &StartOf1970, &i )) {
        DbgPrint("****ERROR converting time to seconds since 1970\n");
    }
    if (i != 0) {
        DbgPrint("****ERROR seconds since 1970 != 0\n");
    }

    //
    //  Setup and test the start 1980 time
    //

    RtlSecondsSince1980ToTime( 0, &StartOf1980 );
    RtlTimeToTimeFields( &StartOf1980, &TimeFields );
    DbgPrint(" Start of 1980  = "); PrintTimeFields( &TimeFields ); DbgPrint("\n");
    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
        DbgPrint("****ERROR converting TimeFields back to start of 1980\n");
    }
    if ((Time.LowPart != StartOf1980.LowPart) || (Time.HighPart != StartOf1980.HighPart)) {
        DbgPrint("****ERROR Time != StartOf1980\n");
    }
    if (!RtlTimeToSecondsSince1980( &StartOf1980, &i )) {
        DbgPrint("****ERROR converting time to seconds since 1980\n");
    }
    if (i != 0) {
        DbgPrint("****ERROR seconds since 1980 != 0\n");
    }

    //
    //  Lets try to print the Christmas when Santa arrives for 1901 to 2001
    //  every 10 years
    //

    TimeFields.Month = 12;
    TimeFields.Day = 25;
    TimeFields.Hour = 3;
    TimeFields.Minute = 30;
    TimeFields.Second = 15;
    TimeFields.Milliseconds = 250;

    for (i = 1901; i < 2002; i += 10) {

        TimeFields.Year = i;

        if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
            DbgPrint("****ERROR converting TimeFields to Christmas %4d\n", TimeFields.Year);
        }
        RtlTimeToTimeFields( &Time, &TimeFields );
        DbgPrint(" Christmas %4d = ", i); PrintTimeFields( &TimeFields ); DbgPrint("\n");

    }

    //
    //  Let's see how old I really am, when I turn 10, 20, 30, ...
    //

    TimeFields.Month = 12;
    TimeFields.Day = 5;
    TimeFields.Hour = 3;
    TimeFields.Minute = 14;
    TimeFields.Second = 0;
    TimeFields.Milliseconds = 0;

    for (i = 1956; i <= 1956+60; i += 10) {

        TimeFields.Year = i;

        if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {
            DbgPrint("****ERROR converting TimeFields to DOB %4d\n", TimeFields.Year);
        }
        RtlTimeToTimeFields( &Time, &TimeFields );
        DbgPrint(" DOB + %4d = ", i-1956); PrintTimeFields( &TimeFields ); DbgPrint("\n");

    }

    DbgPrint("End Time Test\n");

    return TRUE;
}

VOID
PrintTimeFields (
    IN PTIME_FIELDS TimeFields
    )
{
    static PCHAR Months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    static PCHAR Days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

    DbgPrint(" %d", TimeFields->Year);
    DbgPrint("-%s", Months[TimeFields->Month-1]);
    DbgPrint("-%d", TimeFields->Day);

    DbgPrint(" %2d", TimeFields->Hour);
    DbgPrint(":%2d", TimeFields->Minute);
    DbgPrint(":%2d", TimeFields->Second);
    DbgPrint(".%3d", TimeFields->Milliseconds);

    DbgPrint(" (%s)", Days[TimeFields->Weekday]);
}
