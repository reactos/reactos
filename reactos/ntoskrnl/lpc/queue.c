/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/queue.c
 * PURPOSE:         Communication mechanism
 * 
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
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
