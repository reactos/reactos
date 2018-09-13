/*++

     Copyright (c) 1996 Intel Corporation
     Copyright (c) 1996 Microsoft Corporation
     All Rights Reserved
     
     Permission is granted to use, copy and distribute this software and 
     its documentation for any purpose and without fee, provided, that 
     the above copyright notice and this statement appear in all copies. 
     Intel makes no representations about the suitability of this 
     software for any purpose.  This software is provided "AS IS."  
     
     Intel specifically disclaims all warranties, express or implied, 
     and all liability, including consequential and other indirect 
     damages, for the use of this software, including liability for 
     infringement of any proprietary rights, and including the 
     warranties of merchantability and fitness for a particular purpose. 
     Intel does not assume any responsibility for any errors which may 
     appear in this software nor any responsibility to update it.

Module Name:

    TRACE.CPP : 

Abstract:

    This module implements the traceing functions used in the winsock2 layered
    service provider example.
    
Author:

    bugs@brandy.jf.intel.com
    
Revision History:

   
--*/

#include <windows.h>
#include <memory.h>
#include <stdio.h>
#include <stdarg.h>
#include <io.h>
#include <malloc.h>

/* because windows.h is brain damaged and turns this one back on */
#pragma warning(disable: 4001)

#include "trace.h"

#ifdef TRACING

//
// Internal functions
//
BOOL  InitMemoryBuffers(VOID);

//
// Module local variables
//

// The buffers to use to format debug messages.
LPSTR g_CurrentMessage=NULL;
LPSTR g_PreviousMessage=NULL;

// Critcal section to keep multiple thread from walking on each others output
CRITICAL_SECTION OutputRoutine;

// Where should the output go? If it is a file what is the file name
int iTraceDestination=TRACE_TO_AUX;
char TraceFile[] = "trace.log";

// The default output level
DWORD debugLevel=0;



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

    g_CurrentMessage = (LPSTR)malloc(TRACE_OUTPUT_BUFFER_SIZE);
    if (g_CurrentMessage)
    {
        g_PreviousMessage = (LPSTR)malloc(TRACE_OUTPUT_BUFFER_SIZE);
        if (g_PreviousMessage)
        {
            ReturnCode=TRUE;
        } //if
        else
        {
            free( g_CurrentMessage );
        } //else
    } //if
    return(ReturnCode);
}

#endif  // TRACING
