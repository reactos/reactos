/* $Id: reply.c,v 1.4 2001/01/18 15:00:09 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/reply.c
 * PURPOSE:         Communication mechanism
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/port.h>
#include <internal/dbg.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

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
NTSTATUS STDCALL
EiReplyOrRequestPort (IN	PEPORT		Port, 
		      IN	PLPC_MESSAGE	LpcReply, 
		      IN	ULONG		MessageType,
		      IN	PEPORT		Sender)
{
   KIRQL oldIrql;
   PQUEUEDMESSAGE MessageReply;
   
   if (Port == NULL)
     {
       KeBugCheck(0);
     }

   MessageReply = ExAllocatePool(NonPagedPool, sizeof(QUEUEDMESSAGE));
   MessageReply->Sender = Sender;
   
   if (LpcReply != NULL)
     {
	memcpy(&MessageReply->Message, LpcReply, LpcReply->MessageSize);
     }
   
   MessageReply->Message.Cid.UniqueProcess = PsGetCurrentProcessId();
   MessageReply->Message.Cid.UniqueThread = PsGetCurrentThreadId();
   MessageReply->Message.MessageType = MessageType;
   MessageReply->Message.MessageId = InterlockedIncrement(&EiNextLpcMessageId);
   
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   EiEnqueueMessagePort(Port, MessageReply);
   KeReleaseSpinLock(&Port->Lock, oldIrql);
   
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
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
NTSTATUS STDCALL
NtReplyPort (IN	HANDLE		PortHandle,
	     IN	PLPC_MESSAGE	LpcReply)
{
   NTSTATUS Status;
   PEPORT Port;
   
   DPRINT("NtReplyPort(PortHandle %x, LpcReply %x)\n", PortHandle, LpcReply);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,   /* AccessRequired */
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtReplyPort() = %x\n", Status);
	return(Status);
     }
   
   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcReply, 
				 LPC_REPLY,
				 Port);
   KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);
   
   ObDereferenceObject(Port);
   
   return(Status);
}


/**********************************************************************
 * NAME							EXPORTED
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
NTSTATUS STDCALL
NtReplyWaitReceivePort (HANDLE		PortHandle,
			PULONG		PortId,
			PLPC_MESSAGE	LpcReply,     
			PLPC_MESSAGE	LpcMessage)
{
   NTSTATUS Status;
   PEPORT Port;
   KIRQL oldIrql;
   PQUEUEDMESSAGE Request;
   
   DPRINT("NtReplyWaitReceivePort(PortHandle %x, LpcReply %x, "
	  "LpcMessage %x)\n", PortHandle, LpcReply, LpcMessage);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("NtReplyWaitReceivePort() = %x\n", Status);
	return(Status);
     }
   
   if (Port->State != EPORT_CONNECTED_CLIENT &&
       Port->State != EPORT_CONNECTED_SERVER &&
       LpcReply != NULL)
     {
       DPRINT1("NtReplyWaitReceivePort() = %x (State was %x)\n", 
	       STATUS_PORT_DISCONNECTED, Port->State);
       return(STATUS_PORT_DISCONNECTED);
     }

   /*
    * Send the reply
    */
   if (LpcReply != NULL)
     {
	Status = EiReplyOrRequestPort(Port->OtherPort, 
				      LpcReply,
				      LPC_REPLY,
				      Port);
	KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);
	
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Port);
	     DPRINT1("NtReplyWaitReceivePort() = %x\n", Status);
	     return(Status);
	  }
     }
   
   /*
    * Want for a message to be received
    */
   do
     {
       Status = KeWaitForSingleObject(&Port->Event,
				      UserRequest,
				      UserMode,
				      FALSE,
				      NULL);
       if (!NT_SUCCESS(Status))
	 {
	   DPRINT1("NtReplyWaitReceivePort() = %x\n", Status);
	   return(Status);
	 }

       /*
	* Dequeue the message
	*/
       KeAcquireSpinLock(&Port->Lock, &oldIrql);
       Request = EiDequeueMessagePort(Port);
       
       /*
	* There is a race between the event being set and the port lock being
	* taken in which another thread may dequeue the same request so
	* we may need to loop.
	*/
       if (Request == NULL)
	 {
	   KeReleaseSpinLock(&Port->Lock, oldIrql);
	 }
     } while(Request == NULL);
   
   memcpy(LpcMessage, &Request->Message, Request->Message.MessageSize);
   if (Request->Message.MessageType == LPC_CONNECTION_REQUEST)
     {
       EiEnqueueConnectMessagePort(Port, Request);
       KeReleaseSpinLock(&Port->Lock, oldIrql);
     }
   else
     {
       KeReleaseSpinLock(&Port->Lock, oldIrql);
       ExFreePool(Request);
     }
   
   /*
    * Dereference the port
    */
   ObDereferenceObject(Port);
   return(STATUS_SUCCESS);
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
NTSTATUS STDCALL
NtReplyWaitReplyPort (HANDLE		PortHandle,
		      PLPC_MESSAGE	ReplyMessage)
{
   UNIMPLEMENTED;
}


/* EOF */
