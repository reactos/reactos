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
                 "***   Source File: %s, line %lu\n\n",
                 Message != NULL ? Message : "",
                 (PSTR)FailedAssertion,
                 (PSTR)FileName,
                 LineNumber);

        /* Check for reactos specific flag (set by rosautotest) */
        if (RtlGetNtGlobalFlags() & FLG_DISABLE_DEBUG_PROMPTS)
        {
            RtlRaiseStatus(STATUS_ASSERTION_FAILURE);
        }

        /* Prompt for action */
        DbgPrompt("Break repeatedly, break Once, Ignore, "
                  "terminate Process or terminate Thread (boipt)? ",
                  Action,
                  sizeof(Action));
        switch (Action[0])
        {
            /* Break repeatedly / Break once */
            case 'B': case 'b':
            case 'O': case 'o':
                DbgPrint("Execute '.cxr %p' to dump context\n", &Context);
                /* Do a breakpoint, then prompt again or return */
                DbgBreakPoint();
                if ((Action[0] == 'B') || (Action[0] == 'b'))
                    break;
                /* else ('O','o'): fall through */

            /* Ignore: Return to caller */
            case 'I': case 'i':
                return;

            /* Terminate current process */
            case 'P': case 'p':
                ZwTerminateProcess(ZwCurrentProcess(), STATUS_UNSUCCESSFUL);
                break;

            /* Terminate current thread */
            case 'T': case 't':
                ZwTerminateThread(ZwCurrentThread(), STATUS_UNSUCCESSFUL);
                break;

            /* Unrecognized: Prompt again */
            default:
                break;
        }
    }

    /* Shouldn't get here */
    DbgBreakPoint();
    ZwTerminateProcess(ZwCurrentProcess(), STATUS_UNSUCCESSFUL);
}
