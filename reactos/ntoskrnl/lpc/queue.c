/* $Id: queue.c,v 1.8 2003/07/10 20:42:53 royce Exp $
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

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID STDCALL
EiEnqueueMessagePort (IN OUT	PEPORT		Port,
		      IN	PQUEUEDMESSAGE	Message)
{
  InsertTailList (&Port->QueueListHead,
		  &Message->QueueListEntry);
  Port->QueueLength++;
}

/*
 * @implemented
 */
VOID STDCALL
EiEnqueueMessageAtHeadPort (IN OUT	PEPORT		Port,
			    IN	PQUEUEDMESSAGE	Message)
{
  InsertTailList (&Port->QueueListHead,
		  &Message->QueueListEntry);
  Port->QueueLength++;
}

/*
 * @implemented
 */
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


/*
 * @implemented
 */
VOID STDCALL
EiEnqueueConnectMessagePort (IN OUT	PEPORT		Port,
			     IN	PQUEUEDMESSAGE	Message)
{
  InsertTailList (&Port->ConnectQueueListHead,
		  &Message->QueueListEntry);
  Port->ConnectQueueLength++;
}


/*
 * @implemented
 */
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
