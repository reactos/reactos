/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Fmtft.c

Abstract:

    This module contains the FormatFileTime function which is part of the
    Configuration Registry Tools (CRTools) library.

Author:

    David J. Gilman (davegi) 02-Jan-1992

Environment:

    Windows, Crt - User Mode

--*/

#include <stdio.h>

#include "crtools.h"

PSTR
FormatFileTime(
    IN PFILETIME FileTime OPTIONAL,
    IN PSTR Buffer OPTIONAL
    )

/*++

Routine Description:

    Format the supplied FILETIME argument into a string. If the FILETIME
    is not supplied, format the current time.

Arguments:

    FileTime - Supplies an optional pointer to the FILETIME to be
        formatted.

    Buffer - Supplies an optional buffer to put the formatted time. This
        buffer nust be at least FILE_TIME_STRING_LENGTH bytes in length.

Return Value:

    PSTR - Returns a pointer to a string containg the formatted time.

Notes:

    If the Buffer is not supplied an static buffer used to store the formatted
    time. Therefore each call to FormatFileTime will overwrite the previous
    results.

--*/
{
    SYSTEMTIME  SystemTime;
    PSTR        BufferPtr;

    static PSTR Months[ ] = {
                            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
                            };

    static PSTR Days[ ]   = {
                            "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
                            };

    static TSTR StaticBuffer[ FILE_TIME_STRING_LENGTH ];

    //
    // If the FileTime is supplied format that, otherwise format the
    // current time.
    //

    if( ARGUMENT_PRESENT( FileTime )) {

        //
        // Check that the supplied time was a valid FILETIME.
        //

        if( ! FileTimeToSystemTime( FileTime, &SystemTime )) {

            ASSERT_MESSAGE( FALSE, "Invalid FILETIME" );
            return NULL;
        }
    } else {

        GetSystemTime( &SystemTime );
    }

    //
    // If Buffer is supplied use it, otherwise use the static buffer.
    //

    BufferPtr = ( ARGUMENT_PRESENT( Buffer )) ? Buffer : StaticBuffer;

    //
    // DDD dd-MMM-yyyy hh:mm:ss
    //

    //
    // Check that there is room for the formatted string.
    //

    ASSERT( strlen( "DDD dd-MMM-yyyy hh:mm:ss" ) < FILE_TIME_STRING_LENGTH );

    sprintf( BufferPtr,
        "%s %02d-%s-%4d %02d:%02d:%02d",
        Days[ SystemTime.wDayOfWeek ],
        SystemTime.wDay,
        Months[ SystemTime.wMonth - 1 ],
        SystemTime.wYear,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond
        );

    return BufferPtr;
}
