/* $Id: queue.c,v 1.2 2000/10/22 16:36:51 ekohl Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/queue.c
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


VOID
STDCALL
EiEnqueueMessagePort (
	IN OUT	PEPORT		Port,
	IN	PQUEUEDMESSAGE	Message
	)
{
	InsertTailList (
		& Port->QueueListHead,
		& Message->QueueListEntry
		);
	Port->QueueLength++;
}


PQUEUEDMESSAGE
STDCALL
EiDequeueMessagePort (
	IN OUT	PEPORT	Port
	)
{
	PQUEUEDMESSAGE	Message;
	PLIST_ENTRY	entry;
   
	entry = RemoveHeadList (& Port->QueueListHead);
	Message = CONTAINING_RECORD (entry, QUEUEDMESSAGE, QueueListEntry);
	Port->QueueLength--;
   
	return (Message);
}


VOID
STDCALL
EiEnqueueConnectMessagePort (
	IN OUT	PEPORT		Port,
	IN	PQUEUEDMESSAGE	Message
	)
{
	InsertTailList (
		& Port->ConnectQueueListHead,
		& Message->QueueListEntry
		);
	Port->ConnectQueueLength++;
}


PQUEUEDMESSAGE
STDCALL
EiDequeueConnectMessagePort (
	IN OUT	PEPORT	Port
	)
{
	PQUEUEDMESSAGE	Message;
	PLIST_ENTRY	entry;
   
	entry = RemoveHeadList (& Port->ConnectQueueListHead);
	Message = CONTAINING_RECORD (entry, QUEUEDMESSAGE, QueueListEntry);
	Port->ConnectQueueLength--;

	return (Message);
}


/* EOF */
