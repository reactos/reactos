/* $Id$
 *
 * server.c - VMS Enviroment Subsystem Server - Initialization
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
#include "vmssrv.h"

//#define NDEBUG
#include <debug.h>

HANDLE VmsApiPort = NULL;

/**********************************************************************
 * NAME							PRIVATE
 * 	VmsStaticServerThread/1
 */
VOID STDCALL VmsStaticServerThread (PVOID x)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PPORT_MESSAGE Request = (PPORT_MESSAGE) x;
	PPORT_MESSAGE Reply = NULL;
	ULONG MessageType = 0;

	DPRINT("VMSSRV: %s called\n", __FUNCTION__);

	MessageType = Request->u2.s2.Type;
	DPRINT("VMSSRV: %s received a message (Type=%d)\n",
		__FUNCTION__, MessageType);
	switch (MessageType)
	{
		default:
			Reply = Request;
			Status = NtReplyPort (VmsApiPort, Reply);
			break;
	}
}

/*=====================================================================
 * 	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL ServerDllInitialization (ULONG ArgumentCount,
					  LPWSTR *Argument)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("VMSSRV: %s called\n", __FUNCTION__);

	// Get the listening port from csrsrv.dll
	VmsApiPort = CsrQueryApiPort ();
	if (NULL == VmsApiPort)
	{
		return STATUS_UNSUCCESSFUL;
	}
	// Register our message dispatcher
	Status = CsrAddStaticServerThread (VmsStaticServerThread);
	if (NT_SUCCESS(Status))
	{
		//TODO: perform the real VMS server internal initialization here
	}
	return Status;
}

/* EOF */
