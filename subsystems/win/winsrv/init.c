/* $Id$
 *
 * init.c - ReactOS/Win32 Console+User Enviroment Subsystem Server - Initialization
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
#include "winsrv.h"

//#define NDEBUG
#include <debug.h>

HANDLE WinSrvApiPort = NULL;

/**********************************************************************
 * NAME							PRIVATE
 * 	ConStaticServerThread/1
 */
VOID STDCALL ConStaticServerThread (PVOID x)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PPORT_MESSAGE Request = (PPORT_MESSAGE) x;
	PPORT_MESSAGE Reply = NULL;
	ULONG MessageType = 0;

	DPRINT("WINSRV: %s(%08lx) called\n", __FUNCTION__, x);

	MessageType = Request->u2.s2.Type;
	DPRINT("WINSRV: %s(%08lx) received a message (Type=%d)\n",
		__FUNCTION__, x, MessageType);
	switch (MessageType)
	{
		default:
			Reply = Request;
			Status = NtReplyPort (WinSrvApiPort, Reply);
			break;
	}
}

/**********************************************************************
 * NAME							PRIVATE
 * 	UserStaticServerThread/1
 */
VOID STDCALL UserStaticServerThread (PVOID x)
{
	NTSTATUS Status = STATUS_SUCCESS;
	PPORT_MESSAGE Request = (PPORT_MESSAGE) x;
	PPORT_MESSAGE Reply = NULL;
	ULONG MessageType = 0;

	DPRINT("WINSRV: %s(%08lx) called\n", __FUNCTION__, x);

	MessageType = Request->u2.s2.Type;
	DPRINT("WINSRV: %s(%08lx) received a message (Type=%d)\n",
		__FUNCTION__, x, MessageType);
	switch (MessageType)
	{
		default:
			Reply = Request;
			Status = NtReplyPort (WinSrvApiPort, Reply);
			break;
	}
}

/*=====================================================================
 * 	PUBLIC API
 *===================================================================*/

NTSTATUS STDCALL ConServerDllInitialization (ULONG ArgumentCount,
					     LPWSTR *Argument)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("WINSRV: %s called\n", __FUNCTION__);

	// Get the listening port from csrsrv.dll
	WinSrvApiPort = CsrQueryApiPort ();
	if (NULL == WinSrvApiPort)
	{
		return STATUS_UNSUCCESSFUL;
	}
	// Register our message dispatcher
	Status = CsrAddStaticServerThread (ConStaticServerThread);
	if (NT_SUCCESS(Status))
	{
		//TODO: perform the real console server internal initialization here
	}
	return Status;
}

NTSTATUS STDCALL UserServerDllInitialization (ULONG ArgumentCount,
					      LPWSTR *Argument)
{
	NTSTATUS Status = STATUS_SUCCESS;
	
	DPRINT("WINSRV: %s called\n", __FUNCTION__);

	// Get the listening port from csrsrv.dll
	WinSrvApiPort = CsrQueryApiPort ();
	if (NULL == WinSrvApiPort)
	{
		return STATUS_UNSUCCESSFUL;
	}
	// Register our message dispatcher
	Status = CsrAddStaticServerThread (UserStaticServerThread);
	if (NT_SUCCESS(Status))
	{
		//TODO: perform the real user server internal initialization here
	}
	return Status;
}

/* EOF */
