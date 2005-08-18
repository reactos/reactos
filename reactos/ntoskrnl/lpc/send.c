/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/lpc/send.c
 * PURPOSE:         Communication mechanism
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

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
			IN LARGE_INTEGER CreationTime)
{
  NTSTATUS Status;
  LPC_TERMINATION_MESSAGE Msg;

#ifdef __USE_NT_LPC__
  Msg.Header.u2.s2.Type = LPC_NEW_MESSAGE;
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
   memcpy(Reply, &ReplyMessage->Message, ReplyMessage->Message.u1.s1.TotalLength);
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
				 IN	PPORT_MESSAGE	LpcMessage)
{
   NTSTATUS Status;

   DPRINT("LpcRequestPort(PortHandle %08x, LpcMessage %08x)\n", Port, LpcMessage);

#ifdef __USE_NT_LPC__
   /* Check the message's type */
   if (LPC_NEW_MESSAGE == LpcMessage->u2.s2.Type)
   {
      LpcMessage->u2.s2.Type = LPC_DATAGRAM;
   }
   else if (LPC_DATAGRAM == LpcMessage->u2.s2.Type)
   {
      return STATUS_INVALID_PARAMETER;
   }
   else if (LpcMessage->u2.s2.Type > LPC_CLIENT_DIED)
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
				IN	PPORT_MESSAGE	LpcMessage)
{
   NTSTATUS Status;
   PEPORT Port;

   DPRINT("NtRequestPort(PortHandle %x LpcMessage %x)\n", PortHandle,
	  LpcMessage);

   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      LpcPortObjectType,
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
			PPORT_MESSAGE UnsafeLpcRequest,
			PPORT_MESSAGE UnsafeLpcReply)
{
   PETHREAD CurrentThread;
   struct _KPROCESS *AttachedProcess;
   PEPORT Port;
   PQUEUEDMESSAGE Message;
   KIRQL oldIrql;
   PPORT_MESSAGE LpcRequest;
   USHORT LpcRequestMessageSize = 0, LpcRequestDataSize = 0;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PreviousMode = ExGetPreviousMode();
   
   if (PreviousMode != KernelMode)
     {
       _SEH_TRY
         {
           ProbeForRead(UnsafeLpcRequest,
                        sizeof(PORT_MESSAGE),
                        1);
           ProbeForWrite(UnsafeLpcReply,
                         sizeof(PORT_MESSAGE),
                         1);
           LpcRequestMessageSize = UnsafeLpcRequest->u1.s1.TotalLength;
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
       
       if (!NT_SUCCESS(Status))
         {
           return Status;
         }
     }
   else
     {
       LpcRequestMessageSize = UnsafeLpcRequest->u1.s1.TotalLength;
     }

   DPRINT("NtRequestWaitReplyPort(PortHandle %x, LpcRequest %x, "
	  "LpcReply %x)\n", PortHandle, UnsafeLpcRequest, UnsafeLpcReply);

   Status = ObReferenceObjectByHandle(PortHandle,
				      PORT_ALL_ACCESS,
				      LpcPortObjectType,
				      UserMode,
				      (PVOID*)&Port,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   if (EPORT_DISCONNECTED == Port->State)
     {
	ObDereferenceObject(Port);
	return STATUS_PORT_DISCONNECTED;
     }

   /* win32k sometimes needs to KeAttach() the CSRSS process in order to make
      the PortHandle valid. Now that we've got the EPORT structure from the
      handle we can undo this, so everything is normal again. Need to
      re-KeAttach() before returning though */
   CurrentThread = PsGetCurrentThread();
   if (&CurrentThread->ThreadsProcess->Pcb == CurrentThread->Tcb.ApcState.Process)
     {
       AttachedProcess = NULL;
     }
   else
     {
       AttachedProcess = CurrentThread->Tcb.ApcState.Process;
       KeDetachProcess();
     }

   if (LpcRequestMessageSize > LPC_MAX_MESSAGE_LENGTH)
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
   if (PreviousMode != KernelMode)
     {
       _SEH_TRY
         {
           RtlCopyMemory(LpcRequest,
                         UnsafeLpcRequest,
                         LpcRequestMessageSize);
           LpcRequestMessageSize = LpcRequest->u1.s1.TotalLength;
           LpcRequestDataSize = LpcRequest->u1.s1.DataLength;
         }
       _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
       _SEH_END;
       
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
     }
   else
     {
       RtlCopyMemory(LpcRequest,
                     UnsafeLpcRequest,
                     LpcRequestMessageSize);
       LpcRequestMessageSize = LpcRequest->u1.s1.TotalLength;
       LpcRequestDataSize = LpcRequest->u1.s1.DataLength;
     }

   if (LpcRequestMessageSize > LPC_MAX_MESSAGE_LENGTH)
     {
       ExFreePool(LpcRequest);
       if (NULL != AttachedProcess)
         {
           KeAttachProcess(AttachedProcess);
         }
       ObDereferenceObject(Port);
       return(STATUS_PORT_MESSAGE_TOO_LONG);
     }
   if (LpcRequestDataSize > LPC_MAX_DATA_LENGTH)
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
           DPRINT("Message->Message.u1.s1.TotalLength %d\n",
	          Message->Message.u1.s1.TotalLength);
           if (PreviousMode != KernelMode)
             {
               _SEH_TRY
                 {
                   RtlCopyMemory(UnsafeLpcReply,
                                 &Message->Message,
                                 Message->Message.u1.s1.TotalLength);
                 }
               _SEH_HANDLE
                 {
                   Status = _SEH_GetExceptionCode();
                 }
               _SEH_END;
             }
           else
             {
               RtlCopyMemory(UnsafeLpcReply,
                             &Message->Message,
                             Message->Message.u1.s1.TotalLength);
             }
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
				     PPORT_MESSAGE	Message,
				     ULONG		Index,
				     PVOID		Buffer,
				     ULONG		BufferLength,
				     PULONG		ReturnLength)
{
   UNIMPLEMENTED;
   return(STATUS_NOT_IMPLEMENTED);
}


/* EOF */
