/* $Id: send.c,v 1.15 2004/05/31 11:47:05 gvg Exp $
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
#include <internal/safe.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>


/**********************************************************************
 * NAME
 *	LpcSendTerminationPort/2
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL 
LpcSendTerminationPort (IN PEPORT Port,
			IN TIME	CreationTime)
{
  NTSTATUS Status;
  LPC_TERMINATION_MESSAGE Msg;
  
#ifdef __USE_NT_LPC__
  Msg.Header.MessageType = LPC_NEW_MESSAGE;
#endif
  Msg.CreationTime = CreationTime;
  Status = LpcRequestPort (Port, &Msg.Header);
  return(Status);
}


/**********************************************************************
 * NAME
 *	LpcSendDebugMessagePort/3
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
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
 *	LpcRequestPort/2
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *	2002-03-01 EA
 *	I investigated this function a bit more in depth.
 *	It looks like the legal values for the MessageType field in the 
 *	message to send are in the range LPC_NEW_MESSAGE .. LPC_CLIENT_DIED,
 *	but LPC_DATAGRAM is explicitly forbidden.
 *
 * @implemented
 */
NTSTATUS STDCALL LpcRequestPort (IN	PEPORT		Port,
				 IN	PLPC_MESSAGE	LpcMessage)
{
   NTSTATUS Status;
   
   DPRINT("LpcRequestPort(PortHandle %08x, LpcMessage %08x)\n", Port, LpcMessage);

#ifdef __USE_NT_LPC__
   /* Check the message's type */
   if (LPC_NEW_MESSAGE == LpcMessage->MessageType)
   {
      LpcMessage->MessageType = LPC_DATAGRAM;
   }
   else if (LPC_DATAGRAM == LpcMessage->MessageType)
   {
      return STATUS_INVALID_PARAMETER;
   }
   else if (LpcMessage->MessageType > LPC_CLIENT_DIED)
   {
      return STATUS_INVALID_PARAMETER;
   }
   /* Check the range offset */
   if (0 != LpcMessage->VirtualRangesOffset)
   {
      return STATUS_INVALID_PARAMETER;
   }
#endif

   Status = EiReplyOrRequestPort(Port, 
				 LpcMessage, 
				 LPC_DATAGRAM,
				 Port);
   KeReleaseSemaphore( &Port->Semaphore, IO_NO_INCREMENT, 1, FALSE );

   return(Status);
}


/**********************************************************************
 * NAME
 *	NtRequestPort/2
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
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
 *	NtRequestWaitReplyPort/3
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 * @implemented
 */
NTSTATUS STDCALL 
NtRequestWaitReplyPort (IN HANDLE PortHandle,
			PLPC_MESSAGE UnsafeLpcRequest,    
			PLPC_MESSAGE UnsafeLpcReply)
{
   PETHREAD CurrentThread;
   struct _EPROCESS *AttachedProcess;
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

   /* win32k sometimes needs to KeAttach() the CSRSS process in order to make
      the PortHandle valid. Now that we've got the EPORT structure from the
      handle we can undo this, so everything is normal again. Need to
      re-KeAttach() before returning though */
   CurrentThread = PsGetCurrentThread();
   if (NULL == CurrentThread->OldProcess)
     {
       AttachedProcess = NULL;
     }
   else
     {
       AttachedProcess = CurrentThread->ThreadsProcess;
       KeDetachProcess();
     }

   Status = MmCopyFromCaller(&LpcRequestMessageSize,
			     &UnsafeLpcRequest->MessageSize,
			     sizeof(USHORT));
   if (!NT_SUCCESS(Status))
     {
       if (NULL != AttachedProcess)
         {
           KeAttachProcess(AttachedProcess);
         }
       ObDereferenceObject(Port);
       return(Status);
     }
   if (LpcRequestMessageSize > (sizeof(LPC_MESSAGE) + MAX_MESSAGE_DATA))
     {
       if (NULL != AttachedProcess)
         {
           KeAttachProcess(AttachedProcess);
         }
       ObDereferenceObject(Port);
       return(STATUS_PORT_MESSAGE_TOO_LONG);
     }
   LpcRequest = ExAllocatePool(NonPagedPool, LpcRequestMessageSize);
   if (LpcRequest == NULL)
     {
       if (NULL != AttachedProcess)
         {
           KeAttachProcess(AttachedProcess);
         }
       ObDereferenceObject(Port);
       return(STATUS_NO_MEMORY);
     }
   Status = MmCopyFromCaller(LpcRequest, UnsafeLpcRequest,
			     LpcRequestMessageSize);
   if (!NT_SUCCESS(Status))
     {
       ExFreePool(LpcRequest);
       if (NULL != AttachedProcess)
         {
           KeAttachProcess(AttachedProcess);
         }
       ObDereferenceObject(Port);
       return(Status);
     }
   LpcRequestMessageSize = LpcRequest->MessageSize;
   if (LpcRequestMessageSize > (sizeof(LPC_MESSAGE) + MAX_MESSAGE_DATA))
     {
       ExFreePool(LpcRequest);
       if (NULL != AttachedProcess)
         {
           KeAttachProcess(AttachedProcess);
         }
       ObDereferenceObject(Port);
       return(STATUS_PORT_MESSAGE_TOO_LONG);
     }
   if (LpcRequest->DataSize != (LpcRequest->MessageSize - sizeof(LPC_MESSAGE)))
     {
       ExFreePool(LpcRequest);
       if (NULL != AttachedProcess)
         {
           KeAttachProcess(AttachedProcess);
         }
       ObDereferenceObject(Port);
       return(STATUS_PORT_MESSAGE_TOO_LONG);
     }

   Status = EiReplyOrRequestPort(Port->OtherPort, 
				 LpcRequest, 
				 LPC_REQUEST,
				 Port);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("Enqueue failed\n");
	ExFreePool(LpcRequest);
        if (NULL != AttachedProcess)
          {
            KeAttachProcess(AttachedProcess);
          }
	ObDereferenceObject(Port);
	return(Status);
     }
   ExFreePool(LpcRequest);
   KeReleaseSemaphore (&Port->OtherPort->Semaphore, IO_NO_INCREMENT, 
		       1, FALSE);   
   
   /*
    * Wait for a reply
    */
   Status = KeWaitForSingleObject(&Port->Semaphore,
			          UserRequest,
			          UserMode,
			          FALSE,
			          NULL);
   if (Status == STATUS_SUCCESS)
     {
   
       /*
        * Dequeue the reply
        */
       KeAcquireSpinLock(&Port->Lock, &oldIrql);
       Message = EiDequeueMessagePort(Port);
       KeReleaseSpinLock(&Port->Lock, oldIrql);
       if (Message)
         {
           DPRINT("Message->Message.MessageSize %d\n",
	          Message->Message.MessageSize);
           Status = MmCopyToCaller(UnsafeLpcReply, &Message->Message, 
			           Message->Message.MessageSize);
           ExFreePool(Message);
         }
       else
         Status = STATUS_UNSUCCESSFUL;
     }
   else
     {
       if (NT_SUCCESS(Status))
         {
	   Status = STATUS_UNSUCCESSFUL;
	 }
     }
   if (NULL != AttachedProcess)
     {
       KeAttachProcess(AttachedProcess);
     }
   ObDereferenceObject(Port);
   
   return(Status);
}


/**********************************************************************
 * NAME
 *	NtWriteRequestData/6
 *	
 * DESCRIPTION
 *
 * ARGUMENTS
 *
 * RETURN VALUE
 *
 * REVISIONS
 */
NTSTATUS STDCALL NtWriteRequestData (HANDLE		PortHandle,
				     PLPC_MESSAGE	Message,
				     ULONG		Index,
				     PVOID		Buffer,
				     ULONG		BufferLength,
				     PULONG		ReturnLength)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
