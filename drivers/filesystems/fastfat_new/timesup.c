/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    TimeSup.c

Abstract:

    This module implements the Fat Time conversion support routines


--*/

#include "fatprocs.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatNtTimeToFatTime)
#pragma alloc_text(PAGE, FatFatDateToNtTime)
#pragma alloc_text(PAGE, FatFatTimeToNtTime)
#pragma alloc_text(PAGE, FatGetCurrentFatTime)
#endif

_Success_(return != FALSE)
BOOLEAN
FatNtTimeToFatTime (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PLARGE_INTEGER NtTime,
    _In_ BOOLEAN Rounding,
    _Out_ PFAT_TIME_STAMP FatTime,
    _Out_opt_ PUCHAR TenMsecs
    )

/*++

Routine Description:

    This routine converts an NtTime value to its corresponding Fat time value.

Arguments:

    NtTime - Supplies the Nt GMT Time value to convert from

    Rounding - Indicates whether the NT time should be rounded up to a FAT boundary.
        This should only be done *once* in the lifetime of a timestamp (important
        for tunneling, which will cause a timestamp to pass through at least twice).
        If true, rounded up. If false, rounded down to 10ms boundary. This obeys
        the rules for non-creation time and creation times (respectively).

    FatTime - Receives the equivalent Fat time value

    TenMsecs - Optionally receive the number of tens of milliseconds the NtTime, after
        any rounding, is greater than the FatTime

Return Value:

    BOOLEAN - TRUE if the Nt time value is within the range of Fat's
        time range, and FALSE otherwise

--*/

{
    TIME_FIELDS TimeFields;

    PAGED_CODE();

    //
    //  Convert the input to the a time field record.
    //

    if (Rounding) {

        //
        //   Add almost two seconds to round up to the nearest double second.
        //

        NtTime->QuadPart = NtTime->QuadPart + AlmostTwoSeconds;
    }

    ExSystemTimeToLocalTime( NtTime, NtTime );

    RtlTimeToTimeFields( NtTime, &TimeFields );

    //
    //  Check the range of the date found in the time field record
    //

    if ((TimeFields.Year < 1980) || (TimeFields.Year > (1980 + 127))) {

        ExLocalTimeToSystemTime( NtTime, NtTime );

        return FALSE;
    }

    //
    //  The year will fit in Fat so simply copy over the information
    //

    FatTime->Time.DoubleSeconds = (USHORT)(TimeFields.Second / 2);
    FatTime->Time.Minute        = (USHORT)(TimeFields.Minute);
    FatTime->Time.Hour          = (USHORT)(TimeFields.Hour);

    FatTime->Date.Year          = (USHORT)(TimeFields.Year - 1980);
    FatTime->Date.Month         = (USHORT)(TimeFields.Month);
    FatTime->Date.Day           = (USHORT)(TimeFields.Day);

    if (TenMsecs) {

        if (!Rounding) {

            //
            //  If the number of seconds was not divisible by two, then there
            //  is another second of time (1 sec, 3 sec, etc.) Note we round down
            //  the number of milleconds onto tens of milleseconds boundaries.
            //

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4244 )
#endif
            *TenMsecs = (TimeFields.Milliseconds / 10) +
                ((TimeFields.Second % 2) * 100);
#ifdef _MSC_VER
#pragma warning( pop )
#endif
        } else {

            //
            //  If we rounded up, we have in effect changed the NT time. Therefore,
            //  it does not differ from the FAT time.
            //

            *TenMsecs = 0;
        }
    }

    if (Rounding) {

        //
        //  Slice off non-FAT boundary time and convert back to 64bit form
        //

        TimeFields.Milliseconds = 0;
        TimeFields.Second -= TimeFields.Second % 2;

    } else {

        //
        //  Round down to 10ms boundary
        //

        TimeFields.Milliseconds -= TimeFields.Milliseconds % 10;
    }

    //
    //  Convert back to NT time
    //

    (VOID) RtlTimeFieldsToTime(&TimeFields, NtTime);

    ExLocalTimeToSystemTime( NtTime, NtTime );

    UNREFERENCED_PARAMETER( IrpContext );

    return TRUE;
}


LARGE_INTEGER
FatFatDateToNtTime (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ FAT_DATE FatDate
    )

/*++

Routine Description:

    This routine converts a Fat datev value to its corresponding Nt GMT
    Time value.

Arguments:

    FatDate - Supplies the Fat Date to convert from

Return Value:

    LARGE_INTEGER - Receives the corresponding Nt Time value

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER Time;

    PAGED_CODE();

    //
    //  Pack the input time/date into a time field record
    //

    TimeFields.Year         = (USHORT)(FatDate.Year + 1980);
    TimeFields.Month        = (USHORT)(FatDate.Month);
    TimeFields.Day          = (USHORT)(FatDate.Day);
    TimeFields.Hour         = (USHORT)0;
    TimeFields.Minute       = (USHORT)0;
    TimeFields.Second       = (USHORT)0;
    TimeFields.Milliseconds = (USHORT)0;

    //
    //  Convert the time field record to Nt LARGE_INTEGER, and set it to zero
    //  if we were given a bogus time.
    //

    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {

        Time.LowPart = 0;
        Time.HighPart = 0;

    } else {

        ExLocalTimeToSystemTime( &Time, &Time );
    }

    return Time;

    UNREFERENCED_PARAMETER( IrpContext );
}


LARGE_INTEGER
FatFatTimeToNtTime (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ FAT_TIME_STAMP FatTime,
    _In_ UCHAR TenMilliSeconds
    )

/*++

Routine Description:

    This routine converts a Fat time value pair to its corresponding Nt GMT
    Time value.

Arguments:

    FatTime - Supplies the Fat Time to convert from

    TenMilliSeconds - A 10 Milisecond resolution

Return Value:

    LARGE_INTEGER - Receives the corresponding Nt GMT Time value

--*/

{
    TIME_FIELDS TimeFields;
    LARGE_INTEGER Time;

    PAGED_CODE();

    //
    //  Pack the input time/date into a time field record
    //

    TimeFields.Year         = (USHORT)(FatTime.Date.Year + 1980);
    TimeFields.Month        = (USHORT)(FatTime.Date.Month);
    TimeFields.Day          = (USHORT)(FatTime.Date.Day);
    TimeFields.Hour         = (USHORT)(FatTime.Time.Hour);
    TimeFields.Minute       = (USHORT)(FatTime.Time.Minute);
    TimeFields.Second       = (USHORT)(FatTime.Time.DoubleSeconds * 2);

    if (TenMilliSeconds != 0) {

        TimeFields.Second      += (USHORT)(TenMilliSeconds / 100);
        TimeFields.Milliseconds = (USHORT)((TenMilliSeconds % 100) * 10);

    } else {

        TimeFields.Milliseconds = (USHORT)0;
    }

    //
    //  If the second value is greater than 59 then we truncate it to 0.
    //  Note that this can't happen with a proper FAT timestamp.
    //

    if (TimeFields.Second > 59) {

        TimeFields.Second = 0;
    }

    //
    //  Convert the time field record to Nt LARGE_INTEGER, and set it to zero
    //  if we were given a bogus time.
    //

    if (!RtlTimeFieldsToTime( &TimeFields, &Time )) {

        Time.LowPart = 0;
        Time.HighPart = 0;

    } else {

        ExLocalTimeToSystemTime( &Time, &Time );
    }

    return Time;

    UNREFERENCED_PARAMETER( IrpContext );
}


FAT_TIME_STAMP
FatGetCurrentFatTime (
    _In_ PIRP_CONTEXT IrpContext
    )

/*++

Routine Description:

    This routine returns the current system time in Fat time

Arguments:

Return Value:

    FAT_TIME_STAMP - Receives the current system time

--*/

{
    LARGE_INTEGER Time;
    TIME_FIELDS TimeFields;
    FAT_TIME_STAMP FatTime;

    PAGED_CODE();

    //
    //  Get the current system time, and map it into a time field record.
    //

    KeQuerySystemTime( &Time );

    ExSystemTimeToLocalTime( &Time, &Time );

    //
    //  Always add almost two seconds to round up to the nearest double second.
    //

    Time.QuadPart = Time.QuadPart + AlmostTwoSeconds;

    (VOID)RtlTimeToTimeFields( &Time, &TimeFields );

    //
    //  Now simply copy over the information
    //

    FatTime.Time.DoubleSeconds = (USHORT)(TimeFields.Second / 2);
    FatTime.Time.Minute        = (USHORT)(TimeFields.Minute);
    FatTime.Time.Hour          = (USHORT)(TimeFields.Hour);

    FatTime.Date.Year          = (USHORT)(TimeFields.Year - 1980);
    FatTime.Date.Month         = (USHORT)(TimeFields.Month);
    FatTime.Date.Day           = (USHORT)(TimeFields.Day);

    UNREFERENCED_PARAMETER( IrpContext );

    return FatTime;
}


