/* $Id: send.c,v 1.8 2002/09/07 15:12:59 chorns Exp $
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

#include <ntoskrnl.h>

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
   KeReleaseSemaphore(&Port->OtherPort->Semaphore, IO_NO_INCREMENT, 1, FALSE);

   /*
    * Wait for a reply
    */
   KeWaitForSingleObject(&Port->Semaphore,
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
   KeReleaseSemaphore( &Port->Semaphore, IO_NO_INCREMENT, 1, FALSE );

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
			PLPC_MESSAGE UnsafeLpcRequest,    
			PLPC_MESSAGE UnsafeLpcReply)
{
   NTSTATUS Status;
   PEPORT Port;
   PQUEUEDMESSAGE Message;
   KIRQL oldIrql;
   PLPC_MESSAGE LpcRequest;
   USHORT LpcRequestMessageSize;

   DPRINT("NtRequestWaitReplyPort(PortHandle %x, LpcRequest %x, "
	  "LpcReply %x)\n", PortHandle, UnsafeLpcRequest, UnsafeLpcReply);

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

   Status = MmCopyFromCaller(&LpcRequestMessageSize,
			     &UnsafeLpcRequest->MessageSize,
			     sizeof(USHORT));
   if (!NT_SUCCESS(Status))
     {
       ObDereferenceObject(Port);
       return(Status);
     }
   if (LpcRequestMessageSize > (sizeof(LPC_MESSAGE) + MAX_MESSAGE_DATA))
     {
       ObDereferenceObject(Port);
       return(STATUS_PORT_MESSAGE_TOO_LONG);
     }
   LpcRequest = ExAllocatePool(NonPagedPool, LpcRequestMessageSize);
   if (LpcRequest == NULL)
     {
       ObDereferenceObject(Port);
       return(STATUS_NO_MEMORY);
     }
   Status = MmCopyFromCaller(LpcRequest, UnsafeLpcRequest,
			     LpcRequestMessageSize);
   if (!NT_SUCCESS(Status))
     {
       ExFreePool(LpcRequest);
       ObDereferenceObject(Port);
       return(Status);
     }
   LpcRequestMessageSize = LpcRequest->MessageSize;
   if (LpcRequestMessageSize > (sizeof(LPC_MESSAGE) + MAX_MESSAGE_DATA))
     {
       ExFreePool(LpcRequest);
       ObDereferenceObject(Port);
       return(STATUS_PORT_MESSAGE_TOO_LONG);
     }
   if (LpcRequest->DataSize != (LpcRequest->MessageSize - sizeof(LPC_MESSAGE)))
     {
       ExFreePool(LpcRequest);
       ObDereferenceObject(Port);
       return(STATUS_PORT_MESSAGE_TOO_LONG);
     }

   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcRequest, 
				 LPC_REQUEST,
				 Port);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Enqueue failed\n");
	ExFreePool(LpcRequest);
	ObDereferenceObject(Port);
	return(Status);
     }
   ExFreePool(LpcRequest);
   KeReleaseSemaphore (&Port->OtherPort->Semaphore, IO_NO_INCREMENT, 
		       1, FALSE);   
   
   /*
    * Wait for a reply
    */
   KeWaitForSingleObject(&Port->Semaphore,
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
   Status = MmCopyToCaller(UnsafeLpcReply, &Message->Message, 
			   Message->Message.MessageSize);
   ExFreePool(Message);
   
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
