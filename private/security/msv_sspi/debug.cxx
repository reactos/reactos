/*++

Copyright (c) 1987-1993  Microsoft Corporation

Module Name:

    debug.cxx

Abstract:

    Support routines allowing the NtLmSsp DLL side use the common routines
    shared between the DLL and the SERVICE.

    These routines exist in the DLL side.  They are different implementations
    of the same routines that exist on the SERVICE side.  These implementations
    are significantly simpler because they run in the address space of the
    caller.

Author:

    Cliff Van Dyke (CliffV) 22-Sep-1993

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:
    ChandanS  03-Aug-1996 Stolen from net\svcdlls\ntlmssp\client\support.c

--*/

//
// Common include files.
//

#include <global.h>


#if DBG
#include <stdio.h>
#define MAX_PRINTF_LEN 1024        // Arbitrary.


VOID
SspPrintRoutine(
    IN DWORD DebugFlag,
    IN LPSTR Format,
    ...
    )

{
    va_list arglist;
    char OutputBuffer[MAX_PRINTF_LEN];
    ULONG length;
    static BeginningOfLine = TRUE;
    static LineCount = 0;

    //
    // If we aren't debugging this functionality, just return.
    //
    if ( DebugFlag != 0 && (SspGlobalDbflag & DebugFlag) == 0 ) {
        return;
    }

    //
    // vsprintf isn't multithreaded + we don't want to intermingle output
    // from different threads.
    //

    EnterCriticalSection( &SspGlobalLogFileCritSect );
    length = 0;

    //
    // Handle the beginning of a new line.
    //
    //

    if ( BeginningOfLine ) {

        //
        // If we're writing to the debug terminal,
        //  indicate this is an NtLmSsp message.
        //

        length += (ULONG) sprintf( &OutputBuffer[length], "[MSV1_0.dll] " );

        //
        // Put the timestamp at the begining of the line.
        //
        IF_DEBUG( TIMESTAMP ) {
            SYSTEMTIME SystemTime;
            GetLocalTime( &SystemTime );
            length += (ULONG) sprintf( &OutputBuffer[length],
                                  "%02u/%02u %02u:%02u:%02u ",
                                  SystemTime.wMonth,
                                  SystemTime.wDay,
                                  SystemTime.wHour,
                                  SystemTime.wMinute,
                                  SystemTime.wSecond );
        }

        //
        // Indicate the type of message on the line
        //
        {
            char *Text;

            switch (DebugFlag) {
            case SSP_INIT:
                Text = "INIT"; break;
            case SSP_MISC:
                Text = "MISC"; break;
            case SSP_CRITICAL:
                Text = "CRITICAL"; break;
            case SSP_LEAK_TRACK:
                Text = "LEAK_TRACK"; break;
            case SSP_LPC:
            case SSP_LPC_MORE:
                Text = "LPC"; break;
            case SSP_API:
                Text = "API"; break;
            case SSP_API_MORE:
                Text = "APIMORE"; break;
            case SSP_SESSION_KEYS:
                Text = "SESSION_KEYS"; break;
            case SSP_NEGOTIATE_FLAGS:
                Text = "NEGOTIATE_FLAGS"; break;
            default:
                Text = "UNKNOWN"; break;

            case 0:
                Text = NULL;
            }
            if ( Text != NULL ) {
                length += (ULONG) sprintf( &OutputBuffer[length], "[%s] ", Text );
            }
        }
    }
    //
    // Put a the information requested by the caller onto the line
    //

    va_start(arglist, Format);

    length += (ULONG) vsprintf(&OutputBuffer[length], Format, arglist);
    BeginningOfLine = (length > 0 && OutputBuffer[length-1] == '\n' );

    va_end(arglist);

    ASSERT(length <= MAX_PRINTF_LEN);


    //
    //  just output to the debug terminal
    //

    (void) DbgPrint( (PCH) OutputBuffer);

    LeaveCriticalSection( &SspGlobalLogFileCritSect );

} // SspPrintRoutine

#endif // DBG
