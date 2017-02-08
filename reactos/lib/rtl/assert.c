/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Implements RtlAssert used by the ASSERT
 *                    and ASSERTMSG debugging macros
 * FILE:              lib/rtl/assert.c
 * PROGRAMERS:        Stefan Ginsberg (stefan.ginsberg@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
RtlAssert(IN PVOID FailedAssertion,
          IN PVOID FileName,
          IN ULONG LineNumber,
          IN PCHAR Message OPTIONAL)
{
    CHAR Action[2];
    CONTEXT Context;

    /* Capture caller's context for the debugger */
    RtlCaptureContext(&Context);

    /* Enter prompt loop */
    for (;;)
    {
        /* Print the assertion */
        DbgPrint("\n*** Assertion failed: %s%s\n"
                 "***   Source File: %s, line %ld\n\n",
                 Message != NULL ? Message : "",
                 (PSTR)FailedAssertion,
                 (PSTR)FileName,
                 LineNumber);

        /* Prompt for action */
        DbgPrompt("Break repeatedly, break Once, Ignore,"
                  " terminate Process or terminate Thread (boipt)? ",
                  Action,
                  sizeof(Action));
        switch (Action[0])
        {
            /* Break repeatedly */
            case 'B': case 'b':

                /* Do a breakpoint, then prompt again */
                DbgPrint("Execute '.cxr %p' to dump context\n", &Context);
                DbgBreakPoint();
                break;

            /* Ignore */
            case 'I': case 'i':

                /* Return to caller */
                return;

            /* Break once */
            case 'O': case 'o':

                /* Do a breakpoint and return */
                DbgPrint("Execute '.cxr %p' to dump context\n", &Context);
                DbgBreakPoint();
                return;

            /* Terminate process*/
            case 'P': case 'p':

                /* Terminate us */
                ZwTerminateProcess(ZwCurrentProcess(), STATUS_UNSUCCESSFUL);
                break;

            /* Terminate thread */
            case 'T': case 't':

                /* Terminate us */
                ZwTerminateThread(ZwCurrentThread(), STATUS_UNSUCCESSFUL);
                break;

            /* Unrecognized */
            default:

                /* Prompt again */
                break;
        }
    }

    /* Shouldn't get here */
    DbgBreakPoint();
    ZwTerminateProcess(ZwCurrentProcess(), STATUS_UNSUCCESSFUL);
}
