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

/* GLOBALS *******************************************************************/

#define RTL_ASSERT_PROBE_MAGIC 0x50415452
#define RTL_ASSERT_PROBE_TEXT_LENGTH 128
#define RTL_ASSERT_PROBE_FILE_LENGTH 192

typedef struct _RTL_ASSERT_PROBE_SNAPSHOT
{
    ULONG Magic;
    ULONG Version;
    ULONG Count;
    ULONG LineNumber;
    ULONG_PTR FailedAssertion;
    ULONG_PTR FileName;
    ULONG_PTR Message;
    ULONG_PTR ContextRecord;
    ULONG_PTR ProgramCounter;
    CHAR FailedAssertionText[RTL_ASSERT_PROBE_TEXT_LENGTH];
    CHAR FileNameText[RTL_ASSERT_PROBE_FILE_LENGTH];
    CHAR MessageText[RTL_ASSERT_PROBE_TEXT_LENGTH];
} RTL_ASSERT_PROBE_SNAPSHOT;

volatile RTL_ASSERT_PROBE_SNAPSHOT RtlpAssertProbeSnapshot;

/* PRIVATE FUNCTIONS *********************************************************/

static
VOID
RtlpCopyAssertProbeString(
    _Out_writes_(DestLength) volatile CHAR *Dest,
    _In_ ULONG DestLength,
    _In_opt_ PCSTR Source)
{
    ULONG Index;

    if (DestLength == 0)
        return;

    if (Source == NULL)
    {
        Dest[0] = ANSI_NULL;
        return;
    }

    for (Index = 0; Index < DestLength - 1; ++Index)
    {
        Dest[Index] = Source[Index];
        if (Dest[Index] == ANSI_NULL)
            return;
    }

    Dest[Index] = ANSI_NULL;
}

static
ULONG_PTR
RtlpGetAssertProbeProgramCounter(
    _In_ PCONTEXT Context)
{
#if defined(_M_IX86)
    return Context->Eip;
#elif defined(_M_AMD64)
    return Context->Rip;
#elif defined(_M_ARM)
    return Context->Pc;
#else
    UNREFERENCED_PARAMETER(Context);
    return 0;
#endif
}

static
VOID
RtlpRecordAssertProbe(
    _In_ PVOID FailedAssertion,
    _In_ PVOID FileName,
    _In_ ULONG LineNumber,
    _In_opt_ PCHAR Message,
    _In_ PCONTEXT Context)
{
    RtlpAssertProbeSnapshot.Magic = RTL_ASSERT_PROBE_MAGIC;
    RtlpAssertProbeSnapshot.Version = 1;
    RtlpAssertProbeSnapshot.Count++;
    RtlpAssertProbeSnapshot.LineNumber = LineNumber;
    RtlpAssertProbeSnapshot.FailedAssertion = (ULONG_PTR)FailedAssertion;
    RtlpAssertProbeSnapshot.FileName = (ULONG_PTR)FileName;
    RtlpAssertProbeSnapshot.Message = (ULONG_PTR)Message;
    RtlpAssertProbeSnapshot.ContextRecord = (ULONG_PTR)Context;
    RtlpAssertProbeSnapshot.ProgramCounter = RtlpGetAssertProbeProgramCounter(Context);
    RtlpCopyAssertProbeString(RtlpAssertProbeSnapshot.FailedAssertionText,
                              RTL_ASSERT_PROBE_TEXT_LENGTH,
                              (PCSTR)FailedAssertion);
    RtlpCopyAssertProbeString(RtlpAssertProbeSnapshot.FileNameText,
                              RTL_ASSERT_PROBE_FILE_LENGTH,
                              (PCSTR)FileName);
    RtlpCopyAssertProbeString(RtlpAssertProbeSnapshot.MessageText,
                              RTL_ASSERT_PROBE_TEXT_LENGTH,
                              Message);
}

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
    RtlpRecordAssertProbe(FailedAssertion,
                          FileName,
                          LineNumber,
                          Message,
                          &Context);

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
