/* $Id: $
 *
 * WIN32MU.DLL - init.c - Initialize the server DLL
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
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.LIB. If not, write
 * to the Free Software Foundation, Inc., 675 Mass Ave, Cambridge,
 * MA 02139, USA.  
 *
 * --------------------------------------------------------------------
 */
#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <debug.h>

#include "w32mu.h"

static NTSTATUS STDCALL
W32muLoadRemoteTerminalProxy (VOID)
{
	SYSTEM_LOAD_AND_CALL_IMAGE ImageInfo;
	NTSTATUS                   Status = STATUS_SUCCESS;

	DPRINT("W32MU: loading remote terminal device\n");
 
	/* Load kernel mode module */
	RtlInitUnicodeString (& ImageInfo.ModuleName,
			      L"\\SystemRoot\\system32\\w32mut.sys");

	Status = NtSetSystemInformation (SystemLoadAndCallImage,
					 & ImageInfo,
					 sizeof (SYSTEM_LOAD_AND_CALL_IMAGE));

	DPRINT("W32MU: w32mut.sys loaded\n", Status);
	if (!NT_SUCCESS(Status))
	{
		DPRINT("W32MU: loading w32mut.sys failed (Status=0x%08lx)\n", Status);
		return Status;
	}
	return Status;
}

/* Public entry point for CSRSS.EXE to load us */

NTSTATUS STDCALL
ServerDllInitialization (int a0, int a1, int a2, int a3, int a4)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	/* TODO:
	 * 1) load a kernel mode module to make Kmode happy
	 *    (it will provide keyoard, display and pointer
	 *    devices for window stations not attached to 
	 *    the console);
	 */
	Status = W32muLoadRemoteTerminalProxy ();		
	/*
	 * 2) pick up from the registry the list of session
	 *    access providers (SAP: Local, RFB, RDP, ICA, ...);
	 * 3) initialize each SAP;
	 * 4) on SAP events, provide:
	 *    4.1) create session (SESSION->new);
	 *    4.2) suspend session (SESSION->state_change);
	 *    4.3) destroy session (SESSION->delete).
	 */
	return Status;
}

/* EOF */
