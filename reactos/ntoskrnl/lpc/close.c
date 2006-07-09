/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/close.c
 * PURPOSE:         Communication mechanism
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
LpcExitThread(IN PETHREAD Thread)
{
    /* Make sure that the Reply Chain is empty */
    if (!IsListEmpty(&Thread->LpcReplyChain))
    {
        /* It's not, remove the entry */
        RemoveEntryList(&Thread->LpcReplyChain);
    }

    /* Set the thread in exit mode */
    Thread->LpcExitThreadCalled = TRUE;
    Thread->LpcReplyMessageId = 0;

    /* FIXME: Reply to the LpcReplyMessage */
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
 */
VOID STDCALL
LpcpClosePort (IN PEPROCESS Process OPTIONAL,
               IN PVOID ObjectBody,
               IN ACCESS_MASK GrantedAccess,
               IN ULONG HandleCount,
               IN ULONG SystemHandleCount)
{
  PEPORT Port = (PEPORT)ObjectBody;
  PORT_MESSAGE Message;

  /* FIXME Race conditions here! */

  DPRINT("NiClosePort 0x%p OtherPort 0x%p State %d\n", Port, Port->OtherPort, Port->State);

  /*
   * If the client has just closed its handle then tell the server what
   * happened and disconnect this port.
   */
  if (HandleCount == 1 && Port->State == EPORT_CONNECTED_CLIENT)
    {
      DPRINT("Informing server\n");
      Message.u1.s1.TotalLength = sizeof(PORT_MESSAGE);
      Message.u1.s1.DataLength = 0;
      EiReplyOrRequestPort (Port->OtherPort,
			    &Message,
			    LPC_PORT_CLOSED,
			    Port);
      Port->OtherPort->OtherPort = NULL;
      Port->OtherPort->State = EPORT_DISCONNECTED;
      KeReleaseSemaphore( &Port->OtherPort->Semaphore,
			  IO_NO_INCREMENT,
			  1,
			  FALSE );
      ObDereferenceObject (Port);
    }

  /*
   * If the server has closed all of its handles then disconnect the port,
   * don't actually notify the client until it attempts an operation.
   */
  if (HandleCount == 1 && Port->State == EPORT_CONNECTED_SERVER)
    {
        DPRINT("Cleaning up server\n");
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
 */
VOID STDCALL
LpcpDeletePort (PVOID	ObjectBody)
{
   PLIST_ENTRY Entry;
   PQUEUEDMESSAGE	Message;

   PEPORT Port = (PEPORT)ObjectBody;

   DPRINT("Deleting port %x\n", Port);

   /* Free all waiting messages */
   while (!IsListEmpty(&Port->QueueListHead))
     {
       Entry = RemoveHeadList(&Port->QueueListHead);
       Message = CONTAINING_RECORD (Entry, QUEUEDMESSAGE, QueueListEntry);
       ExFreePool(Message);
     }

   while (!IsListEmpty(&Port->ConnectQueueListHead))
     {
       Entry = RemoveHeadList(&Port->ConnectQueueListHead);
       Message = CONTAINING_RECORD (Entry, QUEUEDMESSAGE, QueueListEntry);
       ExFreePool(Message);
     }
}


/* EOF */
