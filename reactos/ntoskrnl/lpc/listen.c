/* $Id: listen.c,v 1.1 2000/06/04 17:27:39 ea Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/listen.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>


/**********************************************************************
 * NAME							EXPORTED
 *	NtListenPort@8
 *
 * DESCRIPTION
 *	Listen on a named port and wait for a connection attempt.
 *
 * ARGUMENTS
 *	PortHandle	[IN] LPC port to listen on.
 *
 *	ConnectMsg	[IN] User provided storage for a
 *			possible connection request LPC message.
 *
 * RETURN VALUE
 *	STATUS_SUCCESS if a connection request is received
 *	successfully; otherwise an error code.
 *
 *	The buffer ConnectMessage is filled with the connection
 *	request message queued by NtConnectPort() in PortHandle.
 *
 * NOTE
 * 	
 */
EXPORTED
NTSTATUS
STDCALL
NtListenPort (
	IN	HANDLE		PortHandle,
	IN	PLPC_MESSAGE	ConnectMsg
	)
{
	NTSTATUS	Status;

	/*
	 * Wait forever for a connection request.
	 */
	for (;;)
	{
		Status = NtReplyWaitReceivePort (
				PortHandle,
				NULL,
				NULL,
				ConnectMsg
				);
		/*
		 * Accept only LPC_CONNECTION_REQUEST requests.
		 * Drop any other message.
		 */
		if (	!NT_SUCCESS(Status)
			|| (LPC_CONNECTION_REQUEST == ConnectMsg->MessageType)
			)
		{
			DPRINT("Got message (type %x)\n", LPC_CONNECTION_REQUEST);
			break;
		}
		DPRINT("Got message (type %x)\n", ConnectMsg->MessageType);
	}

	return (Status);
}


/* EOF */
