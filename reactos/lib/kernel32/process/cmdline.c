/* $Id: cmdline.c,v 1.16 2003/01/15 21:24:35 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#include <k32.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* GLOBALS ******************************************************************/

static UNICODE_STRING CommandLineStringW;
static ANSI_STRING CommandLineStringA;

static WINBOOL bCommandLineInitialized = FALSE;


/* FUNCTIONS ****************************************************************/

static VOID
InitCommandLines (VOID)
{
	PRTL_USER_PROCESS_PARAMETERS Params;

	// get command line
	Params = NtCurrentPeb()->ProcessParameters;
	RtlNormalizeProcessParams (Params);

	// initialize command line buffers
	CommandLineStringW.Length = Params->CommandLine.Length;
	CommandLineStringW.MaximumLength = CommandLineStringW.Length + sizeof(WCHAR);
	CommandLineStringW.Buffer = RtlAllocateHeap(GetProcessHeap(),
						    HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY, 
						    CommandLineStringW.MaximumLength);

	RtlInitAnsiString(&CommandLineStringA, NULL);
	
	// copy command line
	RtlCopyUnicodeString (&CommandLineStringW,
	                      &(Params->CommandLine));
	CommandLineStringW.Buffer[CommandLineStringW.Length / sizeof(WCHAR)] = 0;

	/* convert unicode string to ansi (or oem) */
	if (bIsFileApiAnsi)
	    RtlUnicodeStringToAnsiString (&CommandLineStringA,
					  &CommandLineStringW, 
					  TRUE);
	else
	    RtlUnicodeStringToOemString (&CommandLineStringA,
					 &CommandLineStringW,
					 TRUE);

	CommandLineStringA.Buffer[CommandLineStringA.Length] = 0;

	bCommandLineInitialized = TRUE;
}


LPSTR STDCALL GetCommandLineA(VOID)
{
	if (bCommandLineInitialized == FALSE)
	{
		InitCommandLines ();
	}

	DPRINT ("CommandLine \'%s\'\n", CommandLineStringA.Buffer);

	return(CommandLineStringA.Buffer);
}

LPWSTR STDCALL GetCommandLineW (VOID)
{
	if (bCommandLineInitialized == FALSE)
	{
		InitCommandLines ();
	}

	DPRINT ("CommandLine \'%S\'\n", CommandLineStringW.Buffer);

	return(CommandLineStringW.Buffer);
}

/* EOF */
