/* $Id: send.c,v 1.4 2001/03/18 19:35:13 dwelch Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/send.c
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
NTSTATUS STDCALL 
LpcSendTerminationPort (IN PEPORT Port,
			IN TIME	CreationTime)
{
  NTSTATUS Status;
  LPC_TERMINATION_MESSAGE Msg;
   
  Msg.CreationTime = CreationTime;
  Status = LpcRequestPort (Port, &Msg.Header);
  return(Status);
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
LpcSendDebugMessagePort (IN PEPORT Port,
			 IN PLPC_DBG_MESSAGE Message,
			 OUT PLPC_DBG_MESSAGE Reply)
{
   NTSTATUS Status;
   KIRQL oldIrql;
   PQUEUEDMESSAGE ReplyMessage;
   
   Status = EiReplyOrRequestPort(Port, 
				 &Message->Header, 
				 LPC_REQUEST,
				 Port);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(Port);
	return(Status);
     }
   KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);   
   
   /*
    * Wait for a reply
    */
   KeWaitForSingleObject(&Port->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);
   
   /*
    * Dequeue the reply
    */
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   ReplyMessage = EiDequeueMessagePort(Port);
   KeReleaseSpinLock(&Port->Lock, oldIrql);
   memcpy(Reply, &ReplyMessage->Message, ReplyMessage->Message.MessageSize);
   ExFreePool(ReplyMessage);

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
NTSTATUS STDCALL LpcRequestPort (IN	PEPORT		Port,
				 IN	PLPC_MESSAGE	LpcMessage)
{
   NTSTATUS Status;
   
   DPRINT("LpcRequestPort(PortHandle %x LpcMessage %x)\n", Port, LpcMessage);
   
   Status = EiReplyOrRequestPort(Port, 
				 LpcMessage, 
				 LPC_DATAGRAM,
				 Port);
   KeSetEvent(&Port->Event, IO_NO_INCREMENT, FALSE);

   return(Status);
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
NTSTATUS STDCALL NtRequestPort (IN	HANDLE		PortHandle,
				IN	PLPC_MESSAGE	LpcMessage)
{
   NTSTATUS Status;
   PEPORT Port;
   
   DPRINT("NtRequestPort(PortHandle %x LpcMessage %x)\n", PortHandle, 
	  LpcMessage);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtRequestPort() = %x\n", Status);
	return(Status);
     }

   Status = LpcRequestPort(Port->OtherPort, 
			   LpcMessage);
   
   ObDereferenceObject(Port);
   return(Status);
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
NtRequestWaitReplyPort (IN HANDLE PortHandle,
			PLPC_MESSAGE LpcRequest,    
			PLPC_MESSAGE LpcReply)
{
   NTSTATUS Status;
   PEPORT Port;
   PQUEUEDMESSAGE Message;
   KIRQL oldIrql;
   
   DPRINT("NtRequestWaitReplyPort(PortHandle %x, LpcRequest %x, "
	  "LpcReply %x)\n", PortHandle, LpcRequest, LpcReply);
   
   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS, 
				      ExPortType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   
   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcRequest, 
				 LPC_REQUEST,
				 Port);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Enqueue failed\n");
	ObDereferenceObject(Port);
	return(Status);
     }
   KeSetEvent(&Port->OtherPort->Event, IO_NO_INCREMENT, FALSE);   
   
   /*
    * Wait for a reply
    */
   KeWaitForSingleObject(&Port->Event,
			 UserRequest,
			 UserMode,
			 FALSE,
			 NULL);
   
   /*
    * Dequeue the reply
    */
   KeAcquireSpinLock(&Port->Lock, &oldIrql);
   Message = EiDequeueMessagePort(Port);
   KeReleaseSpinLock(&Port->Lock, oldIrql);
   DPRINT("Message->Message.MessageSize %d\n",
	   Message->Message.MessageSize);
   memcpy(LpcReply, &Message->Message, Message->Message.MessageSize);
   ExFreePool(Message);
   
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
NTSTATUS STDCALL NtWriteRequestData (HANDLE		PortHandle,
				     PLPC_MESSAGE	Message,
				     ULONG		Index,
				     PVOID		Buffer,
				     ULONG		BufferLength,
				     PULONG		ReturnLength)
{
   UNIMPLEMENTED;
}


/* EOF */
