/* $Id$
 *
 * server.c - OS/2 Enviroment Subsystem Server - Initialization
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
#include "os2srv.h"

//#define NDEBUG
#include <debug.h>

HANDLE Os2ApiPort = NULL;

/**********************************************************************
 * NAME							PRIVATE
 * 	Os2StaticServerThread/1
 */
VOID STDCALL Os2StaticServerThread (PVOID x)
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
			Status = NtReplyPort (Os2ApiPort, Reply);
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
	Os2ApiPort = CsrQueryApiPort ();
	if (NULL == Os2ApiPort)
	{
		return STATUS_UNSUCCESSFUL;
	}
	// Register our message dispatcher
	Status = CsrAddStaticServerThread (Os2StaticServerThread);
	if (NT_SUCCESS(Status))
	{
		//TODO: perform the real OS/2 server internal initialization here
	}
	return Status;
}

/* EOF */
