/* $Id: close.c,v 1.2 2000/10/22 16:36:51 ekohl Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/close.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>


/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
VOID
NiClosePort (
	PVOID	ObjectBody,
	ULONG	HandleCount
	)
{
	PEPORT		Port = (PEPORT) ObjectBody;
	LPC_MESSAGE	Message;
   
//	DPRINT1("NiClosePort(ObjectBody %x, HandleCount %d) RefCount %d\n",
//		ObjectBody, HandleCount, ObGetReferenceCount(Port));
   
	if (	(HandleCount == 0)
		&& (Port->State == EPORT_CONNECTED_CLIENT)
		&& (ObGetReferenceCount(Port) == 2)
		)
	{
//		DPRINT1("All handles closed to client port\n");
	
		Message.MessageSize = sizeof(LPC_MESSAGE);
		Message.DataSize = 0;
	
		EiReplyOrRequestPort (
			Port->OtherPort,
			& Message,
			LPC_PORT_CLOSED,
			Port
			);
		KeSetEvent (
			& Port->OtherPort->Event,
			IO_NO_INCREMENT,
			FALSE
			);

		Port->OtherPort->OtherPort = NULL;
		Port->OtherPort->State = EPORT_DISCONNECTED;
		ObDereferenceObject (Port);
	}
	if (	(HandleCount == 0)
		&& (Port->State == EPORT_CONNECTED_SERVER)
		&& (ObGetReferenceCount(Port) == 2)
		)
	{
//		DPRINT("All handles closed to server\n");
	
		Port->OtherPort->OtherPort = NULL;
		Port->OtherPort->State = EPORT_DISCONNECTED;
		ObDereferenceObject(Port->OtherPort);
	}
}


/**********************************************************************
 * NAME
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
VOID
NiDeletePort (
	PVOID	ObjectBody
	)
{
//   PEPORT Port = (PEPORT)ObjectBody;
   
//   DPRINT1("Deleting port %x\n", Port);
}


/* EOF */
