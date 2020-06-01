/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/reply.c
 * PURPOSE:         Local Procedure Call: Receive (Replies)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
LpcpFreeDataInfoMessage(IN PLPCP_PORT_OBJECT Port,
                        IN ULONG MessageId,
                        IN ULONG CallbackId,
                        IN CLIENT_ID ClientId)
{
    PLPCP_MESSAGE Message;
    PLIST_ENTRY ListHead, NextEntry;

    /* Check if the port we want is the connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) > LPCP_UNCONNECTED_PORT)
    {
        /* Use it */
        Port = Port->ConnectionPort;
        if (!Port) return;
    }

    /* Loop the list */
    ListHead = &Port->LpcDataInfoChainHead;
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the message */
        Message = CONTAINING_RECORD(NextEntry, LPCP_MESSAGE, Entry);

        /* Make sure it matches */
        if ((Message->Request.MessageId == MessageId) &&
            (Message->Request.ClientId.UniqueThread == ClientId.UniqueThread) &&
            (Message->Request.ClientId.UniqueProcess == ClientId.UniqueProcess))
        {
            LPCTRACE(LPC_REPLY_DEBUG,
                     "%s removing DataInfo Message %p (%u.%u) Port: %lx\n",
                     PsGetCurrentProcess()->ImageFileName,
                     Message,
                     Message->Request.MessageId,
                     Message->Request.CallbackId,
                     Port);

            /* Unlink and free it */
            RemoveEntryList(&Message->Entry);
            InitializeListHead(&Message->Entry);
            LpcpFreeToPortZone(Message, LPCP_LOCK_HELD);
            return;
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }
    LPCTRACE(LPC_REPLY_DEBUG,
             "%s Unable to find DataInfo Message (%u.%u) Port: %lx\n",
             PsGetCurrentProcess()->ImageFileName,
             MessageId,
             CallbackId,
             Port);
}

VOID
NTAPI
LpcpSaveDataInfoMessage(IN PLPCP_PORT_OBJECT Port,
                        IN PLPCP_MESSAGE Message,
                        IN ULONG LockFlags)
{
    BOOLEAN LockHeld = (LockFlags & LPCP_LOCK_HELD);

    PAGED_CODE();

    /* Acquire the lock */
    if (!LockHeld) KeAcquireGuardedMutex(&LpcpLock);

    /* Check if the port we want is the connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) > LPCP_UNCONNECTED_PORT)
    {
        /* Use it */
        Port = Port->ConnectionPort;
        if (!Port)
        {
            /* Release the lock and return */
            if (!LockHeld) KeReleaseGuardedMutex(&LpcpLock);
            return;
        }
    }

    LPCTRACE(LPC_REPLY_DEBUG,
             "%s saving DataInfo Message %p (%u.%u) Port: %lx\n",
             PsGetCurrentProcess()->ImageFileName,
             Message,
             Message->Request.MessageId,
             Message->Request.CallbackId,
             Port);

    /* Link the message */
    InsertTailList(&Port->LpcDataInfoChainHead, &Message->Entry);

    /* Release the lock */
    if (!LockHeld) KeReleaseGuardedMutex(&LpcpLock);
}

PLPCP_MESSAGE
NTAPI
LpcpFindDataInfoMessage(
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId,
    IN LPC_CLIENT_ID ClientId)
{
    PLPCP_MESSAGE Message;
    PLIST_ENTRY ListEntry;

    PAGED_CODE();

    /* Check if the port we want is the connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) > LPCP_UNCONNECTED_PORT)
    {
        /* Use it */
        Port = Port->ConnectionPort;
        if (!Port)
        {
            /* Return NULL */
            return NULL;
        }
    }

    /* Loop all entries in the list */
    for (ListEntry = Port->LpcDataInfoChainHead.Flink;
         ListEntry != &Port->LpcDataInfoChainHead;
         ListEntry = ListEntry->Flink)
    {
        Message = CONTAINING_RECORD(ListEntry, LPCP_MESSAGE, Entry);

        /* Check if this is the desired message */
        if ((Message->Request.MessageId == MessageId) &&
            (Message->Request.ClientId.UniqueProcess == ClientId.UniqueProcess) &&
            (Message->Request.ClientId.UniqueThread == ClientId.UniqueThread))
        {
            LPCTRACE(LPC_REPLY_DEBUG,
                    "%s Found DataInfo Message %lx (%u.%u) Port: %lx\n",
                     PsGetCurrentProcess()->ImageFileName,
                     Message,
                     Message->Request.MessageId,
                     Message->Request.CallbackId,
                     Port);

            /* It is, return it */
            return Message;
        }
    }

    LPCTRACE(LPC_REPLY_DEBUG,
             "%s Unable to find DataInfo Message (%u.%u) Port: %lx\n",
             PsGetCurrentProcess()->ImageFileName,
             MessageId,
             CallbackId,
             Port);

    return NULL;
}

VOID
NTAPI
LpcpMoveMessage(IN PPORT_MESSAGE Destination,
                IN PPORT_MESSAGE Origin,
                IN PVOID Data,
                IN ULONG MessageType,
                IN PCLIENT_ID ClientId)
{
    LPCTRACE((LPC_REPLY_DEBUG | LPC_SEND_DEBUG),
             "Destination/Origin: %p/%p. Data: %p. Length: %lx\n",
             Destination,
             Origin,
             Data,
             Origin->u1.Length);

    /* Set the Message size */
    Destination->u1.Length = Origin->u1.Length;

    /* Set the Message Type */
    Destination->u2.s2.Type = !MessageType ?
                              Origin->u2.s2.Type : MessageType & 0xFFFF;

    /* Check if we have a Client ID */
    if (ClientId)
    {
        /* Set the Client ID */
        Destination->ClientId.UniqueProcess = ClientId->UniqueProcess;
        Destination->ClientId.UniqueThread = ClientId->UniqueThread;
    }
    else
    {
        /* Otherwise, copy it */
        Destination->ClientId.UniqueProcess = Origin->ClientId.UniqueProcess;
        Destination->ClientId.UniqueThread = Origin->ClientId.UniqueThread;
    }

    /* Copy the MessageId and ClientViewSize */
    Destination->MessageId = Origin->MessageId;
    Destination->ClientViewSize = Origin->ClientViewSize;

    /* Copy the Message Data */
    RtlCopyMemory(Destination + 1,
                  Data,
                  ALIGN_UP_BY(Destination->u1.s1.DataLength, sizeof(ULONG)));
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReplyPort(IN HANDLE PortHandle,
            IN PPORT_MESSAGE ReplyMessage)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PORT_MESSAGE CapturedReplyMessage;
    PLPCP_PORT_OBJECT Port;
    PLPCP_MESSAGE Message;
    PETHREAD Thread = PsGetCurrentThread(), WakeupThread;

    PAGED_CODE();
    LPCTRACE(LPC_REPLY_DEBUG,
             "Handle: %p. Message: %p.\n",
             PortHandle,
             ReplyMessage);

    /* Check if the call comes from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(ReplyMessage, sizeof(*ReplyMessage), sizeof(ULONG));
            CapturedReplyMessage = *(volatile PORT_MESSAGE*)ReplyMessage;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        CapturedReplyMessage = *ReplyMessage;
    }

    /* Validate its length */
    if (((ULONG)CapturedReplyMessage.u1.s1.DataLength + sizeof(PORT_MESSAGE)) >
         (ULONG)CapturedReplyMessage.u1.s1.TotalLength)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure it has a valid ID */
    if (!CapturedReplyMessage.MessageId) return STATUS_INVALID_PARAMETER;

    /* Get the Port object */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        Status = ObReferenceObjectByHandle(PortHandle,
                                           0,
                                           LpcWaitablePortObjectType,
                                           PreviousMode,
                                           (PVOID*)&Port,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Validate its length in respect to the port object */
    if (((ULONG)CapturedReplyMessage.u1.s1.TotalLength > Port->MaxMessageLength) ||
        ((ULONG)CapturedReplyMessage.u1.s1.TotalLength <=
         (ULONG)CapturedReplyMessage.u1.s1.DataLength))
    {
        /* Too large, fail */
        ObDereferenceObject(Port);
        return STATUS_PORT_MESSAGE_TOO_LONG;
    }

    /* Get the ETHREAD corresponding to it */
    Status = PsLookupProcessThreadByCid(&CapturedReplyMessage.ClientId,
                                        NULL,
                                        &WakeupThread);
    if (!NT_SUCCESS(Status))
    {
        /* No thread found, fail */
        ObDereferenceObject(Port);
        return Status;
    }

    /* Allocate a message from the port zone */
    Message = LpcpAllocateFromPortZone();
    if (!Message)
    {
        /* Fail if we couldn't allocate a message */
        ObDereferenceObject(WakeupThread);
        ObDereferenceObject(Port);
        return STATUS_NO_MEMORY;
    }

    /* Keep the lock acquired */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Make sure this is the reply the thread is waiting for */
    if ((WakeupThread->LpcReplyMessageId != CapturedReplyMessage.MessageId) ||
        ((LpcpGetMessageFromThread(WakeupThread)) &&
        (LpcpGetMessageType(&LpcpGetMessageFromThread(WakeupThread)->Request)
            != LPC_REQUEST)) ||
        (!LpcpValidateClientPort(WakeupThread, Port)))
    {
        /* It isn't, fail */
        LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
        ObDereferenceObject(WakeupThread);
        ObDereferenceObject(Port);
        return STATUS_REPLY_MESSAGE_MISMATCH;
    }

    /* Copy the message */
    _SEH2_TRY
    {
        LpcpMoveMessage(&Message->Request,
                        &CapturedReplyMessage,
                        ReplyMessage + 1,
                        LPC_REPLY,
                        NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Cleanup and return the exception code */
        LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
        ObDereferenceObject(WakeupThread);
        ObDereferenceObject(Port);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    LPCTRACE(LPC_REPLY_DEBUG,
              "%s Sending Reply Message %lx (%u, %x) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
              PsGetCurrentProcess()->ImageFileName,
              Message,
              CapturedReplyMessage.MessageId,
              CapturedReplyMessage.u2.s2.DataInfoOffset,
              *((PULONG)(Message+1)+0),
              *((PULONG)(Message+1)+1),
              *((PULONG)(Message+1)+2),
              *((PULONG)(Message+1)+3),
              WakeupThread,
              WakeupThread->ThreadsProcess->ImageFileName);

    /* Reference the thread while we use it */
    ObReferenceObject(WakeupThread);
    Message->RepliedToThread = WakeupThread;

    /* Set this as the reply message */
    WakeupThread->LpcReplyMessageId = 0;
    WakeupThread->LpcReplyMessage = (PVOID)Message;

    /* Check if we have messages on the reply chain */
    if (!(WakeupThread->LpcExitThreadCalled) &&
        !(IsListEmpty(&WakeupThread->LpcReplyChain)))
    {
        /* Remove us from it and reinitialize it */
        RemoveEntryList(&WakeupThread->LpcReplyChain);
        InitializeListHead(&WakeupThread->LpcReplyChain);
    }

    /* Check if this is the message the thread had received */
    if ((Thread->LpcReceivedMsgIdValid) &&
        (Thread->LpcReceivedMessageId == CapturedReplyMessage.MessageId))
    {
        /* Clear this data */
        Thread->LpcReceivedMessageId = 0;
        Thread->LpcReceivedMsgIdValid = FALSE;
    }

    /* Free any data information */
    LpcpFreeDataInfoMessage(Port,
                            CapturedReplyMessage.MessageId,
                            CapturedReplyMessage.CallbackId,
                            CapturedReplyMessage.ClientId);

    /* Release the lock and release the LPC semaphore to wake up waiters */
    KeReleaseGuardedMutex(&LpcpLock);
    LpcpCompleteWait(&WakeupThread->LpcReplySemaphore);

    /* Now we can let go of the thread */
    ObDereferenceObject(WakeupThread);

    /* Dereference port object */
    ObDereferenceObject(Port);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReplyWaitReceivePortEx(IN HANDLE PortHandle,
                         OUT PVOID *PortContext OPTIONAL,
                         IN PPORT_MESSAGE ReplyMessage OPTIONAL,
                         OUT PPORT_MESSAGE ReceiveMessage,
                         IN PLARGE_INTEGER Timeout OPTIONAL)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode(), WaitMode = PreviousMode;
    PORT_MESSAGE CapturedReplyMessage;
    LARGE_INTEGER CapturedTimeout;
    PLPCP_PORT_OBJECT Port, ReceivePort, ConnectionPort = NULL;
    PLPCP_MESSAGE Message;
    PETHREAD Thread = PsGetCurrentThread(), WakeupThread;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    ULONG ConnectionInfoLength;

    PAGED_CODE();
    LPCTRACE(LPC_REPLY_DEBUG,
             "Handle: %p. Messages: %p/%p. Context: %p\n",
             PortHandle,
             ReplyMessage,
             ReceiveMessage,
             PortContext);

    /* Check if the call comes from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            if (PortContext != NULL)
                ProbeForWritePointer(PortContext);

            if (ReplyMessage != NULL)
            {
                ProbeForRead(ReplyMessage, sizeof(*ReplyMessage), sizeof(ULONG));
                CapturedReplyMessage = *(volatile PORT_MESSAGE*)ReplyMessage;
            }

            if (Timeout != NULL)
            {
                ProbeForReadLargeInteger(Timeout);
                CapturedTimeout = *(volatile LARGE_INTEGER*)Timeout;
                Timeout = &CapturedTimeout;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        /* If this is a system thread, then let it page out its stack */
        if (Thread->SystemThread) WaitMode = UserMode;

        if (ReplyMessage != NULL)
            CapturedReplyMessage = *ReplyMessage;
    }

    /* Check if caller has a reply message */
    if (ReplyMessage)
    {
        /* Validate its length */
        if (((ULONG)CapturedReplyMessage.u1.s1.DataLength + sizeof(PORT_MESSAGE)) >
             (ULONG)CapturedReplyMessage.u1.s1.TotalLength)
        {
            /* Fail */
            return STATUS_INVALID_PARAMETER;
        }

        /* Make sure it has a valid ID */
        if (!CapturedReplyMessage.MessageId) return STATUS_INVALID_PARAMETER;
    }

    /* Get the Port object */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        Status = ObReferenceObjectByHandle(PortHandle,
                                           0,
                                           LpcWaitablePortObjectType,
                                           PreviousMode,
                                           (PVOID*)&Port,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Check if the caller has a reply message */
    if (ReplyMessage)
    {
        /* Validate its length in respect to the port object */
        if (((ULONG)CapturedReplyMessage.u1.s1.TotalLength > Port->MaxMessageLength) ||
            ((ULONG)CapturedReplyMessage.u1.s1.TotalLength <=
             (ULONG)CapturedReplyMessage.u1.s1.DataLength))
        {
            /* Too large, fail */
            ObDereferenceObject(Port);
            return STATUS_PORT_MESSAGE_TOO_LONG;
        }
    }

    /* Check if this is anything but a client port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_CLIENT_PORT)
    {
        /* Check if this is the connection port */
        if (Port->ConnectionPort == Port)
        {
            /* Use this port */
            ConnectionPort = ReceivePort = Port;
            ObReferenceObject(ConnectionPort);
        }
        else
        {
            /* Acquire the lock */
            KeAcquireGuardedMutex(&LpcpLock);

            /* Get the port */
            ConnectionPort = ReceivePort = Port->ConnectionPort;
            if (!ConnectionPort)
            {
                /* Fail */
                KeReleaseGuardedMutex(&LpcpLock);
                ObDereferenceObject(Port);
                return STATUS_PORT_DISCONNECTED;
            }

            /* Release lock and reference */
            ObReferenceObject(ConnectionPort);
            KeReleaseGuardedMutex(&LpcpLock);
        }
    }
    else
    {
        /* Otherwise, use the port itself */
        ReceivePort = Port;
    }

    /* Check if the caller gave a reply message */
    if (ReplyMessage)
    {
        /* Get the ETHREAD corresponding to it */
        Status = PsLookupProcessThreadByCid(&CapturedReplyMessage.ClientId,
                                            NULL,
                                            &WakeupThread);
        if (!NT_SUCCESS(Status))
        {
            /* No thread found, fail */
            ObDereferenceObject(Port);
            if (ConnectionPort) ObDereferenceObject(ConnectionPort);
            return Status;
        }

        /* Allocate a message from the port zone */
        Message = LpcpAllocateFromPortZone();
        if (!Message)
        {
            /* Fail if we couldn't allocate a message */
            if (ConnectionPort) ObDereferenceObject(ConnectionPort);
            ObDereferenceObject(WakeupThread);
            ObDereferenceObject(Port);
            return STATUS_NO_MEMORY;
        }

        /* Keep the lock acquired */
        KeAcquireGuardedMutex(&LpcpLock);

        /* Make sure this is the reply the thread is waiting for */
        if ((WakeupThread->LpcReplyMessageId != CapturedReplyMessage.MessageId) ||
            ((LpcpGetMessageFromThread(WakeupThread)) &&
             (LpcpGetMessageType(&LpcpGetMessageFromThread(WakeupThread)->Request) != LPC_REQUEST)) ||
             (!LpcpValidateClientPort(WakeupThread, Port)))
        {
            /* It isn't, fail */
            LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
            if (ConnectionPort) ObDereferenceObject(ConnectionPort);
            ObDereferenceObject(WakeupThread);
            ObDereferenceObject(Port);
            return STATUS_REPLY_MESSAGE_MISMATCH;
        }

        /* Copy the message */
        _SEH2_TRY
        {
            LpcpMoveMessage(&Message->Request,
                            &CapturedReplyMessage,
                            ReplyMessage + 1,
                            LPC_REPLY,
                            NULL);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Cleanup and return the exception code */
            LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
            if (ConnectionPort) ObDereferenceObject(ConnectionPort);
            ObDereferenceObject(WakeupThread);
            ObDereferenceObject(Port);
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        LPCTRACE(LPC_REPLY_DEBUG,
                  "%s Sending Reply Message %lx (%u.%u, %x) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                  PsGetCurrentProcess()->ImageFileName,
                  Message,
                  CapturedReplyMessage.MessageId,
                  CapturedReplyMessage.CallbackId,
                  CapturedReplyMessage.u2.s2.DataInfoOffset,
                  *((PULONG)(Message+1)+0),
                  *((PULONG)(Message+1)+1),
                  *((PULONG)(Message+1)+2),
                  *((PULONG)(Message+1)+3),
                  WakeupThread,
                  WakeupThread->ThreadsProcess->ImageFileName);

        /* Reference the thread while we use it */
        ObReferenceObject(WakeupThread);
        Message->RepliedToThread = WakeupThread;

        /* Set this as the reply message */
        WakeupThread->LpcReplyMessageId = 0;
        WakeupThread->LpcReplyMessage = (PVOID)Message;

        /* Check if we have messages on the reply chain */
        if (!(WakeupThread->LpcExitThreadCalled) &&
            !(IsListEmpty(&WakeupThread->LpcReplyChain)))
        {
            /* Remove us from it and reinitialize it */
            RemoveEntryList(&WakeupThread->LpcReplyChain);
            InitializeListHead(&WakeupThread->LpcReplyChain);
        }

        /* Check if this is the message the thread had received */
        if ((Thread->LpcReceivedMsgIdValid) &&
            (Thread->LpcReceivedMessageId == CapturedReplyMessage.MessageId))
        {
            /* Clear this data */
            Thread->LpcReceivedMessageId = 0;
            Thread->LpcReceivedMsgIdValid = FALSE;
        }

        /* Free any data information */
        LpcpFreeDataInfoMessage(Port,
                                CapturedReplyMessage.MessageId,
                                CapturedReplyMessage.CallbackId,
                                CapturedReplyMessage.ClientId);

        /* Release the lock and release the LPC semaphore to wake up waiters */
        KeReleaseGuardedMutex(&LpcpLock);
        LpcpCompleteWait(&WakeupThread->LpcReplySemaphore);

        /* Now we can let go of the thread */
        ObDereferenceObject(WakeupThread);
    }

    /* Now wait for someone to reply to us */
    LpcpReceiveWait(ReceivePort->MsgQueue.Semaphore, WaitMode);
    if (Status != STATUS_SUCCESS) goto Cleanup;

    /* Wait done, get the LPC lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check if we've received nothing */
    if (IsListEmpty(&ReceivePort->MsgQueue.ReceiveHead))
    {
        /* Check if this was a waitable port and wake it */
        if (ReceivePort->Flags & LPCP_WAITABLE_PORT)
        {
            /* Reset its event */
            KeClearEvent(&ReceivePort->WaitEvent);
        }

        /* Release the lock and fail */
        KeReleaseGuardedMutex(&LpcpLock);
        if (ConnectionPort) ObDereferenceObject(ConnectionPort);
        ObDereferenceObject(Port);
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the message on the queue */
    Message = CONTAINING_RECORD(RemoveHeadList(&ReceivePort->MsgQueue.ReceiveHead),
                                LPCP_MESSAGE,
                                Entry);

    /* Check if the queue is empty now */
    if (IsListEmpty(&ReceivePort->MsgQueue.ReceiveHead))
    {
        /* Check if this was a waitable port */
        if (ReceivePort->Flags & LPCP_WAITABLE_PORT)
        {
            /* Reset its event */
            KeClearEvent(&ReceivePort->WaitEvent);
        }
    }

    /* Re-initialize the message's list entry */
    InitializeListHead(&Message->Entry);

    /* Set this as the received message */
    Thread->LpcReceivedMessageId = Message->Request.MessageId;
    Thread->LpcReceivedMsgIdValid = TRUE;

    _SEH2_TRY
    {
        /* Check if this was a connection request */
        if (LpcpGetMessageType(&Message->Request) == LPC_CONNECTION_REQUEST)
        {
            /* Get the connection message */
            ConnectMessage = (PLPCP_CONNECTION_MESSAGE)(Message + 1);
            LPCTRACE(LPC_REPLY_DEBUG,
                     "Request Messages: %p/%p\n",
                     Message,
                     ConnectMessage);

            /* Get its length */
            ConnectionInfoLength = Message->Request.u1.s1.DataLength -
                                   sizeof(LPCP_CONNECTION_MESSAGE);

            /* Return it as the receive message */
            *ReceiveMessage = Message->Request;

            /* Clear our stack variable so the message doesn't get freed */
            Message = NULL;

            /* Setup the receive message */
            ReceiveMessage->u1.s1.TotalLength = (CSHORT)(sizeof(LPCP_MESSAGE) +
                                                         ConnectionInfoLength);
            ReceiveMessage->u1.s1.DataLength = (CSHORT)ConnectionInfoLength;
            RtlCopyMemory(ReceiveMessage + 1,
                          ConnectMessage + 1,
                          ConnectionInfoLength);

            /* Clear the port context if the caller requested one */
            if (PortContext) *PortContext = NULL;
        }
        else if (LpcpGetMessageType(&Message->Request) != LPC_REPLY)
        {
            /* Otherwise, this is a new message or event */
            LPCTRACE(LPC_REPLY_DEBUG,
                     "Non-Reply Messages: %p/%p\n",
                     &Message->Request,
                     (&Message->Request) + 1);

            /* Copy it */
            LpcpMoveMessage(ReceiveMessage,
                            &Message->Request,
                            (&Message->Request) + 1,
                            0,
                            NULL);

            /* Return its context */
            if (PortContext) *PortContext = Message->PortContext;

            /* And check if it has data information */
            if (Message->Request.u2.s2.DataInfoOffset)
            {
                /* It does, save it, and don't free the message below */
                LpcpSaveDataInfoMessage(Port, Message, LPCP_LOCK_HELD);
                Message = NULL;
            }
        }
        else
        {
            /* This is a reply message, should never happen! */
            LPCTRACE(LPC_REPLY_DEBUG,
                     "Bogus reply message (%08x) in receive queue for port %08x\n",
                     Message,
                     ReceivePort);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Check if we have a message pointer here */
    if (Message)
    {
        /* Free it and release the lock */
        LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
    }
    else
    {
        /* Just release the lock */
        KeReleaseGuardedMutex(&LpcpLock);
    }

Cleanup:
    /* All done, dereference the port and return the status */
    LPCTRACE(LPC_REPLY_DEBUG,
             "Port: %p. Status: %d\n",
             Port,
             Status);
    if (ConnectionPort) ObDereferenceObject(ConnectionPort);
    ObDereferenceObject(Port);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReplyWaitReceivePort(IN HANDLE PortHandle,
                       OUT PVOID *PortContext OPTIONAL,
                       IN PPORT_MESSAGE ReplyMessage OPTIONAL,
                       OUT PPORT_MESSAGE ReceiveMessage)
{
    /* Call the newer API */
    return NtReplyWaitReceivePortEx(PortHandle,
                                    PortContext,
                                    ReplyMessage,
                                    ReceiveMessage,
                                    NULL);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReplyWaitReplyPort(IN HANDLE PortHandle,
                     IN PPORT_MESSAGE ReplyMessage)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;
    PLPCP_PORT_OBJECT Port, RundownPort;
    PORT_MESSAGE CapturedReplyMessage;
    PLPCP_MESSAGE Message;
    PETHREAD Thread = PsGetCurrentThread(), WakeupThread;

    PAGED_CODE();
    LPCTRACE(LPC_REPLY_DEBUG,
             "%s Attempted reply wait reply (Handle: %p, Message: %p)\n",
             PsGetCurrentProcess()->ImageFileName,
             PortHandle,
             ReplyMessage);

    /* Check if the call comes from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForRead(ReplyMessage, sizeof(*ReplyMessage), sizeof(ULONG));
            CapturedReplyMessage = *(volatile PORT_MESSAGE*)ReplyMessage;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        CapturedReplyMessage = *ReplyMessage;
    }

    /* Validate its length */
    if (((ULONG)CapturedReplyMessage.u1.s1.DataLength + sizeof(PORT_MESSAGE)) >
        (ULONG)CapturedReplyMessage.u1.s1.TotalLength)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure it has a valid ID */
    if (!CapturedReplyMessage.MessageId) return STATUS_INVALID_PARAMETER;

    /* Get the port object */
    Status = ObReferenceObjectByHandle(PortHandle,
                                      0,
                                      LpcWaitablePortObjectType,
                                      PreviousMode,
                                      (PVOID*)&Port,
                                      NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Validate its length in respect to the port object */
    if (((ULONG)CapturedReplyMessage.u1.s1.TotalLength > Port->MaxMessageLength) ||
        ((ULONG)CapturedReplyMessage.u1.s1.TotalLength <=
         (ULONG)CapturedReplyMessage.u1.s1.DataLength))
    {
        /* Too large, fail */
        ObDereferenceObject(Port);
        return STATUS_PORT_MESSAGE_TOO_LONG;
    }

    /* Get the ETHREAD corresponding to it */
    Status = PsLookupProcessThreadByCid(&CapturedReplyMessage.ClientId,
                                        NULL,
                                        &WakeupThread);
    if (!NT_SUCCESS(Status))
    {
        /* No thread found, fail */
        ObDereferenceObject(Port);
        return Status;
    }

    /* Allocate a message from port zone */
    Message = LpcpAllocateFromPortZone();
    if (!Message)
    {
        /* Fail if we couldn't allocate a message */
        ObDereferenceObject(WakeupThread);
        ObDereferenceObject(Port);
        return STATUS_NO_MEMORY;
    }

    /* Keep the lock acquired */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Make sure this is the reply the thread is waiting for */
    if ((WakeupThread->LpcReplyMessageId != CapturedReplyMessage.MessageId) ||
        ((LpcpGetMessageFromThread(WakeupThread)) &&
         (LpcpGetMessageType(&LpcpGetMessageFromThread(WakeupThread)->Request)
             != LPC_REQUEST)) ||
         (!LpcpValidateClientPort(WakeupThread, Port)))
    {
        /* It isn't, fail */
        LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
        ObDereferenceObject(WakeupThread);
        ObDereferenceObject(Port);
        return STATUS_REPLY_MESSAGE_MISMATCH;
    }

    /* Copy the message */
    _SEH2_TRY
    {
        LpcpMoveMessage(&Message->Request,
                        &CapturedReplyMessage,
                        ReplyMessage + 1,
                        LPC_REPLY,
                        NULL);

    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Cleanup and return the exception code */
        LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
        ObDereferenceObject(WakeupThread);
        ObDereferenceObject(Port);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    LPCTRACE(LPC_REPLY_DEBUG,
                "%s Sending Reply Wait Reply Msg %lx (%u, %x) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                PsGetCurrentProcess()->ImageFileName,
                Message,
                CapturedReplyMessage.MessageId,
                CapturedReplyMessage.u2.s2.DataInfoOffset,
                *((PULONG)(Message+1)+0),
                *((PULONG)(Message+1)+1),
                *((PULONG)(Message+1)+2),
                *((PULONG)(Message+1)+3),
                WakeupThread,
                WakeupThread->ThreadsProcess->ImageFileName);

    /* Reference the thread while we use it */
    ObReferenceObject(WakeupThread);
    Message->RepliedToThread = WakeupThread;

    /* Set this as the reply message */
    WakeupThread->LpcReplyMessageId = 0;
    WakeupThread->LpcReplyMessage = (PVOID)Message;

    /* Check if we have messages on the replay chain */
    if (!(WakeupThread->LpcExitThreadCalled) &&
        !(IsListEmpty(&WakeupThread->LpcReplyChain)))
    {
        /* Remove us from it and re-initialize it */
        RemoveEntryList(&WakeupThread->LpcReplyChain);
        InitializeListHead(&WakeupThread->LpcReplyChain);
    }

    /* Set us up to get the following reply */
    Thread->LpcReplyMessageId = CapturedReplyMessage.MessageId;
    Thread->LpcReplyMessage = NULL;

    /* Check if this is the message the thread has received */
    if ((Thread->LpcReceivedMsgIdValid) &&
        (Thread->LpcReceivedMessageId == CapturedReplyMessage.MessageId))
    {
        /* Clear this data */
        Thread->LpcReceivedMessageId = 0;
        Thread->LpcReceivedMsgIdValid = FALSE;
    }

    /* Insert the thread into the rundown queue */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_CONNECTION_PORT)
    {
        RundownPort = Port->ConnectedPort;
    }
    else
    {
        RundownPort = Port;
    }

    /* Insert the message in our chain */
    InsertTailList(&RundownPort->LpcReplyChainHead, &Thread->LpcReplyChain);
    LpcpSetPortToThread(Thread, Port);

    /* Free any data information */
    LpcpFreeDataInfoMessage(Port,
                            CapturedReplyMessage.MessageId,
                            CapturedReplyMessage.CallbackId,
                            CapturedReplyMessage.ClientId);

    /* Release the lock and release the LPC semaphore to wake up waiters */
    KeReleaseGuardedMutex(&LpcpLock);
    LpcpCompleteWait(&WakeupThread->LpcReplySemaphore);

    /* Now we can let go of the thread */
    ObDereferenceObject(WakeupThread);

    /* Now wait for a reply */
    LpcpConnectWait(&Thread->LpcReplySemaphore, PreviousMode);

    /* Acquire the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check if the reply chain is not empty */
    if (!IsListEmpty(&Thread->LpcReplyChain))
    {
        /* Remove this entry and re-initialize it */
        RemoveEntryList(&Thread->LpcReplyChain);
        InitializeListHead(&Thread->LpcReplyChain);
    }

    /* Check if wait succeeded */
    if (Status == STATUS_SUCCESS)
    {
		/* Get the LPC message and clear our thread's reply data */
        Message = LpcpGetMessageFromThread(Thread);
        Thread->LpcReplyMessage = NULL;

        /* Release the lock */
        KeReleaseGuardedMutex(&LpcpLock);

        /* Check if we have a valid message */
        if (Message != NULL)
        {
            /* Copy the message */
            _SEH2_TRY
            {
                LpcpMoveMessage(ReplyMessage,
                                &Message->Request,
                                (&Message->Request) + 1,
                                0,
                                NULL);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Acquire the lock */
            KeAcquireGuardedMutex(&LpcpLock);

            /* Check if we replied to a thread */
            if (Message->RepliedToThread)
            {
                /* Dereference */
                ObDereferenceObject(Message->RepliedToThread);
                Message->RepliedToThread = NULL;
            }

            /* Free the message */
            LpcpFreeToPortZone(Message, LPCP_LOCK_HELD | LPCP_LOCK_RELEASE);
        }
        else
        {
            /* We don't have a reply */
            Status = STATUS_LPC_REPLY_LOST;
        }
    }
    else
    {
        /* Release the lock */
        KeReleaseGuardedMutex(&LpcpLock);
    }

    /* All done */
    ObDereferenceObject(Port);
    return Status;
}

NTSTATUS
NTAPI
LpcpCopyRequestData(
    IN BOOLEAN Write,
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message,
    IN ULONG Index,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PULONG ReturnLength)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PORT_MESSAGE CapturedMessage;
    PLPCP_PORT_OBJECT Port = NULL;
    PETHREAD ClientThread = NULL;
    SIZE_T LocalReturnLength;
    PLPCP_MESSAGE InfoMessage;
    PLPCP_DATA_INFO DataInfo;
    PVOID DataInfoBaseAddress;

    PAGED_CODE();

    /* Check if the call comes from user mode */
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            if (Write)
            {
                ProbeForRead(Buffer, BufferLength, 1);
            }
            else
            {
                ProbeForWrite(Buffer, BufferLength, 1);
            }
            ProbeForRead(Message, sizeof(*Message), sizeof(PVOID));
            CapturedMessage = *(volatile PORT_MESSAGE*)Message;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        CapturedMessage = *Message;
    }

    /* Make sure there is any data to copy */
    if (CapturedMessage.u2.s2.DataInfoOffset == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Reference the port handle */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       PORT_ALL_ACCESS,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference port handle: 0x%ls\n", Status);
        return Status;
    }

    /* Look up the client thread */
    Status = PsLookupProcessThreadByCid(&CapturedMessage.ClientId,
                                        NULL,
                                        &ClientThread);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to lookup client thread for [0x%lx:0x%lx]: 0x%ls\n",
                CapturedMessage.ClientId.UniqueProcess,
                CapturedMessage.ClientId.UniqueThread, Status);
        goto Cleanup;
    }

    /* Acquire the global LPC lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check for message id mismatch */
    if ((ClientThread->LpcReplyMessageId != CapturedMessage.MessageId) ||
        (CapturedMessage.MessageId == 0))
    {
        DPRINT1("LpcReplyMessageId mismatch: 0x%lx/0x%lx.\n",
                ClientThread->LpcReplyMessageId, CapturedMessage.MessageId);
        Status = STATUS_REPLY_MESSAGE_MISMATCH;
        goto CleanupWithLock;
    }

    /* Validate the port */
    if (!LpcpValidateClientPort(ClientThread, Port))
    {
        DPRINT1("LpcpValidateClientPort failed\n");
        Status = STATUS_REPLY_MESSAGE_MISMATCH;
        goto CleanupWithLock;
    }

    /* Find the message with the data */
    InfoMessage = LpcpFindDataInfoMessage(Port,
                                          CapturedMessage.MessageId,
                                          CapturedMessage.CallbackId,
                                          CapturedMessage.ClientId);
    if (InfoMessage == NULL)
    {
        DPRINT1("LpcpFindDataInfoMessage failed\n");
        Status = STATUS_INVALID_PARAMETER;
        goto CleanupWithLock;
    }

    /* Get the data info */
    DataInfo = LpcpGetDataInfoFromMessage(&InfoMessage->Request);

    /* Check if the index is within bounds */
    if (Index >= DataInfo->NumberOfEntries)
    {
        DPRINT1("Message data index %lu out of bounds (%lu in msg)\n",
                Index, DataInfo->NumberOfEntries);
        Status = STATUS_INVALID_PARAMETER;
        goto CleanupWithLock;
    }

    /* Check if the caller wants to read/write more data than expected */
    if (BufferLength > DataInfo->Entries[Index].DataLength)
    {
        DPRINT1("Trying to read more data (%lu) than available (%lu)\n",
                BufferLength, DataInfo->Entries[Index].DataLength);
        Status = STATUS_INVALID_PARAMETER;
        goto CleanupWithLock;
    }

    /* Get the data pointer */
    DataInfoBaseAddress = DataInfo->Entries[Index].BaseAddress;

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

    if (Write)
    {
        /* Copy data from the caller to the message sender */
        Status = MmCopyVirtualMemory(PsGetCurrentProcess(),
                                     Buffer,
                                     ClientThread->ThreadsProcess,
                                     DataInfoBaseAddress,
                                     BufferLength,
                                     PreviousMode,
                                     &LocalReturnLength);
    }
    else
    {
        /* Copy data from the message sender to the caller */
        Status = MmCopyVirtualMemory(ClientThread->ThreadsProcess,
                                     DataInfoBaseAddress,
                                     PsGetCurrentProcess(),
                                     Buffer,
                                     BufferLength,
                                     PreviousMode,
                                     &LocalReturnLength);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("MmCopyVirtualMemory failed: 0x%ls\n", Status);
        goto Cleanup;
    }

    /* Check if the caller asked to return the copied length */
    if (ReturnLength != NULL)
    {
        _SEH2_TRY
        {
            *ReturnLength = LocalReturnLength;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Ignore */
            DPRINT1("Exception writing ReturnLength, ignoring\n");
        }
        _SEH2_END;
    }

Cleanup:

    if (ClientThread != NULL)
        ObDereferenceObject(ClientThread);

    ObDereferenceObject(Port);

    return Status;

CleanupWithLock:

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);
    goto Cleanup;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReadRequestData(IN HANDLE PortHandle,
                  IN PPORT_MESSAGE Message,
                  IN ULONG Index,
                  IN PVOID Buffer,
                  IN ULONG BufferLength,
                  OUT PULONG ReturnLength)
{
    /* Call the internal function */
    return LpcpCopyRequestData(FALSE,
                               PortHandle,
                               Message,
                               Index,
                               Buffer,
                               BufferLength,
                               ReturnLength);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtWriteRequestData(IN HANDLE PortHandle,
                   IN PPORT_MESSAGE Message,
                   IN ULONG Index,
                   IN PVOID Buffer,
                   IN ULONG BufferLength,
                   OUT PULONG ReturnLength)
{
    /* Call the internal function */
    return LpcpCopyRequestData(TRUE,
                               PortHandle,
                               Message,
                               Index,
                               Buffer,
                               BufferLength,
                               ReturnLength);
}

/* EOF */
