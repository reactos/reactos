/* $Id$
 *
 * main.c - Client/Server Runtime - entry point
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
 * 	20050329 (Emanuele Aliberti)
 * 		C/S run-time moved to CSRSRV.DLL
 * 		Win32 emulation moved to server DLLs basesrv+winsrv
 * 		(previously code was already in win32csr.dll)
 */
#include "csr.h"

#define NDEBUG
#include <debug.h>

COMMAND_LINE_ARGUMENT Argument;

/* never fail or so */

VOID STDCALL CsrpSetDefaultProcessHardErrorMode (VOID)
{
    DWORD DefaultHardErrorMode = 0; 
    NtSetInformationProcess (NtCurrentProcess(),
		    	     ProcessDefaultHardErrorMode,
			     & DefaultHardErrorMode,
			     sizeof DefaultHardErrorMode);
}

/* Native process' entry point */

VOID STDCALL NtProcessStartup (PPEB Peb)
{
  NTSTATUS  Status = STATUS_SUCCESS;

  /*
   *	Parse the command line.
   */
  Status = CsrParseCommandLine (Peb, & Argument);
  if (STATUS_SUCCESS != Status)
  {
    DPRINT1("CSR: %s: CsrParseCommandLine failed (Status=0x%08lx)\n",
	__FUNCTION__, Status);
  }
  /*
   *	Initialize the environment subsystem server.
   */
  Status = CsrServerInitialization (Argument.Count, Argument.Vector);
  if (!NT_SUCCESS(Status))
  {
    /* FATAL! */
    DPRINT1("CSR: %s: CSRSRV!CsrServerInitialization failed (Status=0x%08lx)\n",
	__FUNCTION__, Status);

    CsrFreeCommandLine (Peb, & Argument);
    /*
     *	Tell the SM we failed. If we are a required
     *	subsystem, SM will halt the system.
     */
    NtTerminateProcess (NtCurrentProcess(), Status);
  }
  /*
   *	The server booted OK: never stop on error!
   */
  CsrpSetDefaultProcessHardErrorMode ();
  /*
   *	Cleanup command line
   */
  CsrFreeCommandLine (Peb, & Argument);	
  /*
   *	Terminate the current thread only (server's
   *	threads that serve the LPC port continue
   *	running and keep the process alive).
   */
  NtTerminateThread (NtCurrentThread(), Status);
}

/* EOF */
