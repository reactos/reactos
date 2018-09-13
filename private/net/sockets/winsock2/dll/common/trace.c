/*/////////////////////////////////////////////////////////////////////////
//
// INTEL Corporation Proprietary Information
// Copyright (c) Intel Corporation
//
// This listing is supplied under the terms of a license aggreement
// with INTEL Corporation and may not be used, copied nor disclosed
// except in accordance with that agreement.
//
//////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
// $Workfile:   TRACE.C  $
// $Revision:   1.3  $
// $Modtime:   27 Nov 1995 08:38:08  $
//
//  DESCRIPTION:
// This file contains the output function for the tracing facility
// used in PII DLL
//////////////////////////////////////////////////////////////////
*/

/* Single Line Comments */
#pragma warning(disable: 4001)
// Disable some more benign warnings for compiling at warning level 4

// nonstandard extension used : nameless struct/union
#pragma warning(disable: 4201)

// nonstandard extension used : bit field types other than int
#pragma warning(disable: 4214)

// Note: Creating precompiled header
#pragma warning(disable: 4699)

// unreferenced inline function has been removed
#pragma warning(disable: 4514)

// unreferenced formal parameter
//#pragma warning(disable: 4100)

// 'type' differs in indirection to slightly different base
// types from 'other type'
#pragma warning(disable: 4057)

// named type definition in parentheses
#pragma warning(disable: 4115)

// nonstandard extension used : benign typedef redefinition
#pragma warning(disable: 4209)

#include <windows.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <io.h>

/* because windows.h is brain damaged and turns this one back on */
#pragma warning(disable: 4001)

#include "trace.h"
#include "osdef.h"

#ifdef TRACING

BOOL  InitMemoryBuffers(VOID);

LPSTR g_CurrentMessage=NULL;
LPSTR g_PreviousMessage=NULL;
CRITICAL_SECTION OutputRoutine;
int iTraceDestination=TRACE_TO_AUX;
char TraceFile[] = "trace.log";
DWORD debugLevel=DBG_ERR;



VOID
PrintDebugString(
                 char *Format,
                 ...
                 )
/*++
  Routine Description:

  This routine outputs a debug messages.  Debug messages are routed
  to a file or a debug window depnding on the value of a global
  variable defined in this module

  Arguments:

  Format - A "printf()" compatable format specification.

  ... - Additional arguments to "printf()" format specification.

  Returns:

  NONE

  --*/
{
    va_list ArgumentList; // argument list for varargs processing
    static HANDLE OutputFile =NULL; // file descriptor for file output
    static OFSTRUCT of; // open file struct for file output
    static int RepeatCount=0; // Count of repeated output statements
    static BOOL LogOpened = FALSE; // have we opened the file for
                                     // output
    static BOOL TraceInited = FALSE;
    DWORD  BytesWritten;

    if (!TraceInited)
    {
        HANDLE InitMutex;
        // Create the mutex to protect the rest of the init code
        InitMutex = CreateMutex(
                                NULL,  // Use default security attributes
                                FALSE, // We don't want automatic ownership
                                "TraceMutextName");
        if (!InitMutex)
        {
            // We failed to create the mutex there is nothign else we
            // can do so return.  This will cause the debug output to
            // be silently lost.
            return;
        } //if

        // Wait on mutex
        WaitForSingleObject( InitMutex,
                             INFINITE);

        // Check to see if init is still needed
        if (!TraceInited)
        {
            // Init the critical section to be used to protect the
            // output portion of this routine.
            InitializeCriticalSection( &OutputRoutine );
// Note:
// DeleteCriticalSection will not be called on this critical section
// we are reling on the OS to clean this one up.  We are doing this so
// the user of this module does not have to Init/DeInit the module explicitly.

            // allocate buffers to hold debug messages
            if (InitMemoryBuffers())
            {
                TraceInited = TRUE;
            } //if
        } //if

        // Signal the mutex and delete this threads handle to the mutex
        ReleaseMutex(InitMutex);
        CloseHandle(InitMutex);

        // Bail out if we couldn't init memory buffers
        if (!TraceInited)
        {
            return;
        }
    }


    // Here is where all the heavy lifting starts
    EnterCriticalSection( &OutputRoutine );

    // print the user message to our buffer
    va_start(ArgumentList, Format);
    vsprintf(g_CurrentMessage, Format, ArgumentList);
    va_end(ArgumentList);

    // Is the current debug message the same as the last debug
    // message?  If the two messages are the same just increment the
    // count of message repeats and return.  This keeps the system
    // from being flooded with debug messages that may be being
    // generated from inside a loop.

    if(lstrcmp(g_CurrentMessage, g_PreviousMessage))
    {
        if (iTraceDestination == TRACE_TO_FILE)
        {
            if (!LogOpened)
            {
                OutputFile =
                CreateFile( TraceFile,
                            GENERIC_WRITE,     // open for writing
                            FILE_SHARE_WRITE,  // Share the file with others
                            NULL,              // default security
                            OPEN_ALWAYS,       // Use file if it exsits
                            FILE_ATTRIBUTE_NORMAL, // Use a normal file
                            NULL);             // No template

                if (OutputFile != INVALID_HANDLE_VALUE)
                {
                    LogOpened = TRUE;
                } //if
            } //if

            if (LogOpened)
            {
                if (RepeatCount > 0)
                {
                    wsprintf(g_PreviousMessage,
                             "Last Message Repeated < %d > times\n",
                             RepeatCount);

                    WriteFile(OutputFile,
                              g_PreviousMessage,
                              lstrlen(g_PreviousMessage),
                              &BytesWritten,
                              NULL);
                } //if

                // Write the current message to the trace file
                WriteFile(OutputFile,
                          g_CurrentMessage,
                          lstrlen(g_CurrentMessage),
                          &BytesWritten,
                          NULL);

                // Flush debug output to file
                FlushFileBuffers( TraceFile );

                //reset the repeat count
                RepeatCount =0;
            } //if
        }

        if( iTraceDestination == TRACE_TO_AUX)
        {
            if(RepeatCount > 0)
            {
                wsprintf(g_PreviousMessage,
                         "Last Message Repeated < %d > times\n",
                         RepeatCount);
                OutputDebugString(g_PreviousMessage);
                RepeatCount = 0;
            }
            // Send message to AUX device
            OutputDebugString(g_CurrentMessage);
        }
        // Store off this message
        lstrcpy(g_PreviousMessage, g_CurrentMessage);
    }
    else
    {
        RepeatCount++;
    }
    LeaveCriticalSection( &OutputRoutine );
}




BOOL
InitMemoryBuffers(
                  VOID
                  )
/*++
  Routine Description:

  Initailizes the memory buffers used by this module.

  Arguments:

  NONE

  Returns:

  TRUE if all memory buffers are successfully created, Otherwise FALSE.

  --*/
{
    BOOL ReturnCode=FALSE;

    g_CurrentMessage = GlobalAlloc (GPTR, TRACE_OUTPUT_BUFFER_SIZE);
    if (g_CurrentMessage)
    {
        ZeroMemory( g_CurrentMessage, TRACE_OUTPUT_BUFFER_SIZE );
        g_PreviousMessage = GlobalAlloc(GPTR, TRACE_OUTPUT_BUFFER_SIZE);
        if (g_PreviousMessage)
        {
            ZeroMemory( g_PreviousMessage, TRACE_OUTPUT_BUFFER_SIZE );
            ReturnCode=TRUE;
        } //if
        else
        {
            GlobalFree( g_CurrentMessage );
            g_CurrentMessage = NULL;
        } //else
    } //if
    return(ReturnCode);
}

LONG
Ws2ExceptionFilter(
    LPEXCEPTION_POINTERS ExceptionPointers,
    LPSTR SourceFile,
    LONG LineNumber
    )
{

    LPSTR fileName;

    //
    // Protect ourselves in case the process is totally screwed.
    //

    __try {

        //
        // Exceptions should never be thrown in a properly functioning
        // system, so this is bad. To ensure that someone will see this,
        // print to the debugger directly
        //


        fileName = strrchr( SourceFile, '\\' );

        if( fileName == NULL ) {
            fileName = SourceFile;
        } else {
            fileName++;
        }

        //
        // Whine about the exception.
        //

        PrintDebugString("-| WS2_32 EXCEPTION   :: ");
        PrintDebugString(" %08lx @ %08lx, caught in %s : %d |-\n",
                            ExceptionPointers->ExceptionRecord->ExceptionCode,
                            ExceptionPointers->ExceptionRecord->ExceptionAddress,
                            fileName, LineNumber );
    }
    __except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Not much we can do here...
        //

        ;
    }

    return EXCEPTION_EXECUTE_HANDLER;

}   // Ws2ExceptionFilter

#endif  // TRACING


#if DBG

VOID
WsAssert(
    LPVOID FailedAssertion,
    LPVOID FileName,
    ULONG LineNumber
    )
{

    PrintDebugString(
        "\n"
        "*** Assertion failed: %s\n"
        "*** Source file %s, line %lu\n\n",
        FailedAssertion,
        FileName,
        LineNumber
        );

    DebugBreak();

}   // WsAssert

#endif
