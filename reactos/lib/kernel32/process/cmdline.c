/* $Id: cmdline.c,v 1.11 2000/01/26 10:07:23 dwelch Exp $
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

#include <ddk/ntddk.h>
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <internal/teb.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <kernel32/kernel32.h>


/* GLOBALS ******************************************************************/

static UNICODE_STRING CommandLineStringW;
static ANSI_STRING CommandLineStringA;

static WCHAR CommandLineW[MAX_PATH];
static CHAR CommandLineA[MAX_PATH];

static WINBOOL bCommandLineInitialized = FALSE;


/* FUNCTIONS ****************************************************************/

static VOID
InitCommandLines (VOID)
{
	PRTL_USER_PROCESS_PARAMETERS Params;
	// initialize command line buffers
	CommandLineW[0] = 0;
	CommandLineStringW.Buffer = CommandLineW;
	CommandLineStringW.Length = 0;
	CommandLineStringW.MaximumLength = MAX_PATH * sizeof(WCHAR);

	CommandLineA[0] = 0;
	CommandLineStringA.Buffer = CommandLineA;
	CommandLineStringA.Length = 0;
	CommandLineStringA.MaximumLength = MAX_PATH;

	// get command line
	Params = NtCurrentPeb()->ProcessParameters;
	RtlNormalizeProcessParams (Params);

	RtlCopyUnicodeString (&CommandLineStringW,
	                      &(Params->CommandLine));
	RtlUnicodeStringToAnsiString (&CommandLineStringA,
	                              &CommandLineStringW,
	                              FALSE);

	bCommandLineInitialized = TRUE;
}


LPSTR STDCALL GetCommandLineA(VOID)
{
	if (bCommandLineInitialized == FALSE)
	{
		InitCommandLines ();
	}

	DPRINT ("CommandLine \'%s\'\n", CommandLineA);

	return(CommandLineA);
}

LPWSTR STDCALL GetCommandLineW (VOID)
{
	if (bCommandLineInitialized == FALSE)
	{
		InitCommandLines ();
	}

	DPRINT ("CommandLine \'%S\'\n", CommandLineW);

	return(CommandLineW);
}

/* EOF */
