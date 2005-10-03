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

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

ULONG FASTCALL WideCharStringToUnicodeString (PWCHAR wsIn, PUNICODE_STRING usOut)
{
	ULONG   Length = 0;
	PWCHAR  CurrentChar = wsIn;

	DPRINT("%s(%08lx,%08lx) called\n", __FUNCTION__, wsIn, usOut);

	if (NULL != CurrentChar)
	{
		usOut->Buffer = CurrentChar;
		while (*CurrentChar ++)
		{
			++ Length;
			while (*CurrentChar ++)
			{
				++ Length;
			}
		}
		++ Length;
	}
	usOut->Length = Length;
	usOut->MaximumLength = Length;
	return Length;
}

VOID
STDCALL
NtProcessStartup(PPEB Peb)
{
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PUNICODE_STRING CmdLineString;
    ANSI_STRING AnsiCmdLine;
    UNICODE_STRING UnicodeEnvironment;
    ANSI_STRING AnsiEnvironment;
    PCHAR NullPointer = NULL;
    INT argc = 0;
    PCHAR *argv;
    PCHAR *envp;
    PCHAR *ArgumentList;
    PCHAR Source, Destination;
    ULONG Length;
    ASSERT(Peb);

    DPRINT("%s(%08lx) called\n", __FUNCTION__, Peb);

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
		DPRINT("NT: argv[%d]=[%d]\n", argc, Destination);

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

    if (0 < WideCharStringToUnicodeString (ProcessParameters->Environment, & UnicodeEnvironment))
    {
    	RtlUnicodeStringToAnsiString (& AnsiEnvironment, & UnicodeEnvironment, TRUE);

    	/* Change our source to the enviroment pointer */
    	Source = AnsiEnvironment.Buffer;

    	/* Simply do a direct copy */
    	if (Source)
    	{
        	while (*Source)
        	{
            		/* Save a pointer to this token */
			*ArgumentList++ = Source;
			DPRINT("NT: envp[%08x]=[%s]\n",Source,Source);

			/* Keep looking for another variable */
			while (*Source++);
		}
	}

    	/* Null terminate the list again */
    	*ArgumentList++ = NULL;
    }
    /* Breakpoint if we were requested to do so */
    if (ProcessParameters->DebugFlags) DbgBreakPoint();

    /* Call the Main Function */
    Status = _main(argc, argv, envp, ProcessParameters->DebugFlags);

    /* We're done here */
    NtTerminateProcess(NtCurrentProcess(), Status);
}

/* EOF */
