/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/proc/proc.c
 * PURPOSE:         Process functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ****************************************************************/

#define UNICODE
#include <windows.h>
#include <kernel32/proc.h>
#include <kernel32/thread.h>
#include <wchar.h>
#include <string.h>
#include <internal/teb.h>

/* GLOBALS ******************************************************************/

static CHAR CommandLineA[MAX_PATH];

/* FUNCTIONS ****************************************************************/

LPSTR STDCALL GetCommandLineA(VOID)
{
   ULONG i;
   PWSTR CommandLineW;
   
   CommandLineW = GetCommandLineW();
   for (i=0; i<MAX_PATH && CommandLineW[i]!=0; i++)
     {
	CommandLineA[i] = (CHAR)CommandLineW[i];
     }
   CommandLineW[i] = 0;
   return(CommandLineA);
}

LPWSTR STDCALL GetCommandLineW(VOID)
{
   return(NtCurrentPeb()->StartupInfo->CommandLine);
}

