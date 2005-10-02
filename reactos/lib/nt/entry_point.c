/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS
 * FILE:            lib/nt/entry_point.c
 * PURPOSE:         Native NT Runtime Library
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

/* PSDK/NDK Headers */
#include <stdio.h>
#include <windows.h>
#define NTOS_MODE_USER
#include <ndk/ntndk.h>

NTSTATUS
__cdecl
_main(
    int argc,
    char *argv[],
    char *envp[],
    ULONG DebugFlag
);

#define NDEBUG
#include <debug.h>

/* FUNCTIONSS ****************************************************************/

VOID
STDCALL
NtProcessStartup(PPEB Peb)
{
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PUNICODE_STRING CmdLineString;
    ANSI_STRING AnsiCmdLine;
    PCHAR NullPointer = NULL;
    INT argc = 0;
    PCHAR *argv;
    PCHAR *envp;
    PCHAR *ArgumentList;
    PCHAR Source, Destination;
    ULONG Length;
    ASSERT(Peb);

    /* Normalize and get the Process Parameters */
    ProcessParameters = RtlNormalizeProcessParams(Peb->ProcessParameters);
    ASSERT(ProcessParameters);

    /* Allocate memory for the argument list, enough for 512 tokens */
    ArgumentList = RtlAllocateHeap(Peb->ProcessHeap, 0, 512 * sizeof(PCHAR));

    /* Use a null pointer as default */
    argv = &NullPointer;
    envp = &NullPointer;

    /* Set the first pointer to NULL, and set the argument array to the buffer */
    *ArgumentList = NULL;
    argv = ArgumentList;

    /* Get the pointer to the Command Line */
    CmdLineString = &ProcessParameters->CommandLine;

    /* If we don't have a command line, use the image path instead */
    if (!CmdLineString->Buffer || !CmdLineString->Length)
    {
        CmdLineString = &ProcessParameters->ImagePathName;
    }

    /* Convert it to an ANSI string */
    RtlUnicodeStringToAnsiString(&AnsiCmdLine, CmdLineString, TRUE);

    /* Save parameters for parsing */
    Source = AnsiCmdLine.Buffer;
    Length = AnsiCmdLine.Length;

    /* Ensure it's valid */
    if (Source)
    {
        /* Allocate a buffer for the destination */
        Destination = RtlAllocateHeap(Peb->ProcessHeap, 0, Length + sizeof(WCHAR));

        /* Start parsing */
        while (*Source)
        {
            /* Skip the white space. */
            while (*Source && *Source <= ' ') Source++;

            /* Copy until the next white space is reached */
            if (*Source)
            {
                /* Save one token pointer */
                *ArgumentList++ = Destination;

                /* Increase one token count */
                argc++;

                /* Copy token until white space */
                while (*Source > ' ') *Destination++ = *Source++;

                /* Null terminate it */
                *Destination++ = '\0';
            }
        }
    }

    /* Null terminate the token pointer list */
    *ArgumentList++ = NULL;

    /* Now handle the enviornment, point the envp at our current list location. */
    envp = ArgumentList;

    /* Change our source to the enviroment pointer */
    Source = (PCHAR)ProcessParameters->Environment;

    /* Simply do a direct copy */
    if (Source)
    {
        while (*Source)
        {
            /* Save a pointer to this token */
            *ArgumentList++ = Source;

            /* Keep looking for another variable */
            while (*Source++);
        }
    }

    /* Null terminate the list again */
    *ArgumentList++ = NULL;

    /* Breakpoint if we were requested to do so */
    if (ProcessParameters->DebugFlags) DbgBreakPoint();

    /* Call the Main Function */
    Status = _main(argc, argv, envp, ProcessParameters->DebugFlags);

    /* We're done here */
    NtTerminateProcess(NtCurrentProcess(), Status);
}

/* EOF */
