/* $Id: queue.c,v 1.6 2002/09/07 15:12:58 chorns Exp $
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

#include <ntoskrnl.h>

#define NDEBUG
#include <internal/debug.h>


/* FUNCTIONS *****************************************************************/

VOID STDCALL
EiEnqueueMessagePort (IN OUT	PEPORT		Port,
		      IN	PQUEUEDMESSAGE	Message)
{
  InsertTailList (&Port->QueueListHead,
		  &Message->QueueListEntry);
  Port->QueueLength++;
}

VOID STDCALL
EiEnqueueMessageAtHeadPort (IN OUT	PEPORT		Port,
			    IN	PQUEUEDMESSAGE	Message)
{
  InsertTailList (&Port->QueueListHead,
		  &Message->QueueListEntry);
  Port->QueueLength++;
}

PQUEUEDMESSAGE STDCALL
EiDequeueMessagePort (IN OUT	PEPORT	Port)
{
  PQUEUEDMESSAGE	Message;
  PLIST_ENTRY	entry;
  
  if (IsListEmpty(&Port->QueueListHead))
    {
      return(NULL);
    }
  entry = RemoveHeadList (&Port->QueueListHead);
  Message = CONTAINING_RECORD (entry, QUEUEDMESSAGE, QueueListEntry);
  Port->QueueLength--;
   
  return (Message);
}


VOID STDCALL
EiEnqueueConnectMessagePort (IN OUT	PEPORT		Port,
			     IN	PQUEUEDMESSAGE	Message)
{
  InsertTailList (&Port->ConnectQueueListHead,
		  &Message->QueueListEntry);
  Port->ConnectQueueLength++;
}


PQUEUEDMESSAGE STDCALL
EiDequeueConnectMessagePort (IN OUT	PEPORT	Port)
{
  PQUEUEDMESSAGE	Message;
  PLIST_ENTRY	entry;
  
  if (IsListEmpty(&Port->ConnectQueueListHead))
    {
      return(NULL);
    }
  entry = RemoveHeadList (&Port->ConnectQueueListHead);
  Message = CONTAINING_RECORD (entry, QUEUEDMESSAGE, QueueListEntry);
  Port->ConnectQueueLength--;
  
  return (Message);
}


/* EOF */
