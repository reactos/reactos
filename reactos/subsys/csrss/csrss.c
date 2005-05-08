/* $Id$
 *
 * csrss.c - Client/Server Runtime subsystem
 *
 * ReactOS Operating System
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.
 *
 * --------------------------------------------------------------------
 *
 *	19990417 (Emanuele Aliberti)
 *		Do nothing native application skeleton
 * 	19990528 (Emanuele Aliberti)
 * 		Compiled successfully with egcs 1.1.2
 * 	19990605 (Emanuele Aliberti)
 * 		First standalone run under ReactOS (it
 * 		actually does nothing but running).
 */
#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <csrss/csrss.h>
#include <rosrtl/string.h>
#include <reactos/buildno.h>

#include "api.h"

#define NDEBUG
#include <debug.h>

#define CSRP_MAX_ARGUMENT_COUNT 512

typedef struct _COMMAND_LINE_ARGUMENT
{
	ULONG		Count;
	UNICODE_STRING	Buffer;
	PWSTR		* Vector;

} COMMAND_LINE_ARGUMENT, *PCOMMAND_LINE_ARGUMENT;

/**********************************************************************
 * NAME							PRIVATE
 * 	CsrpParseCommandLine/3
 */
static NTSTATUS STDCALL
CsrpParseCommandLine (HANDLE                       ProcessHeap,
		      PRTL_USER_PROCESS_PARAMETERS RtlProcessParameters,
		      PCOMMAND_LINE_ARGUMENT       Argument)
{
   INT i = 0;
   INT afterlastspace = 0;


   DPRINT("CSR: %s called\n", __FUNCTION__);

   RtlZeroMemory (Argument, sizeof (COMMAND_LINE_ARGUMENT));

   Argument->Vector = (PWSTR *) RtlAllocateHeap (ProcessHeap,
						 0,
						 (CSRP_MAX_ARGUMENT_COUNT * sizeof Argument->Vector[0]));
   if(NULL == Argument->Vector)
   {
	   DPRINT("CSR: %s: no memory for Argument->Vector\n", __FUNCTION__);
	   return STATUS_NO_MEMORY;
   }

   Argument->Buffer.Length =
   Argument->Buffer.MaximumLength =
   	RtlProcessParameters->CommandLine.Length
	+ sizeof Argument->Buffer.Buffer [0]; /* zero terminated */
   Argument->Buffer.Buffer =
	(PWSTR) RtlAllocateHeap (ProcessHeap,
				 0,
                                 Argument->Buffer.MaximumLength);
   if(NULL == Argument->Buffer.Buffer)
   {
	   DPRINT("CSR: %s: no memory for Argument->Buffer.Buffer\n", __FUNCTION__);
	   return STATUS_NO_MEMORY;
   }

   RtlCopyMemory (Argument->Buffer.Buffer,
		  RtlProcessParameters->CommandLine.Buffer,
		  RtlProcessParameters->CommandLine.Length);

   while (Argument->Buffer.Buffer [i])
     {
	if (Argument->Buffer.Buffer[i] == L' ')
	  {
	     Argument->Count ++;
	     Argument->Buffer.Buffer [i] = L'\0';
	     Argument->Vector [Argument->Count - 1] = & (Argument->Buffer.Buffer [afterlastspace]);
	     i++;
	     while (Argument->Buffer.Buffer [i] == L' ')
	     {
		i++;
	     }
	     afterlastspace = i;
	  }
	else
	  {
	     i++;
	  }
     }

   if (Argument->Buffer.Buffer [afterlastspace] != L'\0')
     {
	Argument->Count ++;
	Argument->Buffer.Buffer [i] = L'\0';
	Argument->Vector [Argument->Count - 1] = & (Argument->Buffer.Buffer [afterlastspace]);
     }

  return STATUS_SUCCESS;
}

/**********************************************************************
 * NAME							PRIVATE
 * 	CsrpFreeCommandLine/2
 */

static VOID STDCALL
CsrpFreeCommandLine (HANDLE                 ProcessHeap,
		     PCOMMAND_LINE_ARGUMENT Argument)
{
	DPRINT("CSR: %s called\n", __FUNCTION__);

	RtlFreeHeap (ProcessHeap,
	             0,
		     Argument->Vector);
	RtlFreeHeap (ProcessHeap,
	             0,
	             Argument->Buffer.Buffer);
}


/* Native process' entry point */

VOID STDCALL NtProcessStartup(PPEB Peb)
{
   PRTL_USER_PROCESS_PARAMETERS RtlProcessParameters = NULL;
   COMMAND_LINE_ARGUMENT        CmdLineArg = {0};
   NTSTATUS                     Status = STATUS_SUCCESS;

   PrintString("ReactOS Client/Server Run-Time %s (Build %s)\n",
	     KERNEL_RELEASE_STR,
	     KERNEL_VERSION_BUILD_STR);

   RtlProcessParameters = RtlNormalizeProcessParams (Peb->ProcessParameters);

   /*==================================================================
    * Parse the command line: TODO actually parse the cl, because
    * it is required to load hosted server DLLs.
    *================================================================*/
   Status = CsrpParseCommandLine (Peb->ProcessHeap,
				  RtlProcessParameters,
				  & CmdLineArg);
   if(STATUS_SUCCESS != Status)
   {
	   DPRINT1("CSR: %s: CsrpParseCommandLine failed (Status=0x%08lx)\n",
		__FUNCTION__, Status);
   }
   /*==================================================================
    *	Initialize the Win32 environment subsystem server.
    *================================================================*/
   if (CsrServerInitialization (CmdLineArg.Count, CmdLineArg.Vector) == TRUE)
     {
	CsrpFreeCommandLine (Peb->ProcessHeap, & CmdLineArg);
	/*
	 * Terminate the current thread only.
	 */
	NtTerminateThread (NtCurrentThread(), 0);
     }
   else
     {
	DisplayString (L"CSR: CsrServerInitialization failed.\n");

	CsrpFreeCommandLine (Peb->ProcessHeap, & CmdLineArg);
	/*
	 * Tell the SM we failed.
	 */
	NtTerminateProcess (NtCurrentProcess(), 0);
     }
}

/* EOF */
