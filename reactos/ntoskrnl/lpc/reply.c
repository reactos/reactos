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
            /* Unlink and free it */
            RemoveEntryList(&Message->Entry);
            InitializeListHead(&Message->Entry);
            LpcpFreeToPortZone(Message, 1);
            break;
        }

        /* Go to the next entry */
        NextEntry = NextEntry->Flink;
    }
}

VOID
NTAPI
LpcpSaveDataInfoMessage(IN PLPCP_PORT_OBJECT Port,
                        IN PLPCP_MESSAGE Message,
                        IN ULONG LockHeld)
{
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

    /* Link the message */
    InsertTailList(&Port->LpcDataInfoChainHead, &Message->Entry);

    /* Release the lock */
    if (!LockHeld) KeReleaseGuardedMutex(&LpcpLock);
}

VOID
NTAPI
LpcpMoveMessage(IN PPORT_MESSAGE Destination,
                IN PPORT_MESSAGE Origin,
                IN PVOID Data,
                IN ULONG MessageType,
                IN PCLIENT_ID ClientId)
{
    /* Set the Message size */
    LPCTRACE((LPC_REPLY_DEBUG | LPC_SEND_DEBUG),
             "Destination/Origin: %p/%p. Data: %p. Length: %lx\n",
             Destination,
             Origin,
             Data,
             Origin->u1.Length);
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
                  ((Destination->u1.Length & 0xFFFF) + 3) &~3);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReplyPort(IN HANDLE PortHandle,
            IN PPORT_MESSAGE LpcReply)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
    PLPCP_PORT_OBJECT Port, ReceivePort, ConnectionPort = NULL;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode(), WaitMode = PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    PLPCP_MESSAGE Message;
    PETHREAD Thread = PsGetCurrentThread(), WakeupThread;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    ULONG ConnectionInfoLength;
    //PORT_MESSAGE CapturedReplyMessage;
    LARGE_INTEGER CapturedTimeout;

    PAGED_CODE();
    LPCTRACE(LPC_REPLY_DEBUG,
             "Handle: %lx. Messages: %p/%p. Context: %p\n",
             PortHandle,
             ReplyMessage,
             ReceiveMessage,
             PortContext);

    if (KeGetPreviousMode() == UserMode)
    {
        _SEH_TRY
        {
            if (ReplyMessage != NULL)
            {
                ProbeForRead(ReplyMessage, sizeof(PORT_MESSAGE), sizeof(ULONG));
                /*RtlCopyMemory(&CapturedReplyMessage, ReplyMessage, sizeof(PORT_MESSAGE));
                ReplyMessage = &CapturedReplyMessage;*/
            }

            if (Timeout != NULL)
            {
                ProbeForReadLargeInteger(Timeout);
                RtlCopyMemory(&CapturedTimeout, Timeout, sizeof(LARGE_INTEGER));
                Timeout = &CapturedTimeout;
            }

            if (PortContext != NULL)
                ProbeForWritePointer(PortContext);
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            DPRINT1("SEH crash [1]\n");
	    DbgBreakPoint();
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        /* Bail out if pointer was invalid */
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        /* If this is a system thread, then let it page out its stack */
        if (Thread->SystemThread) WaitMode = UserMode;
    }

    /* Check if caller has a reply message */
    if (ReplyMessage)
    {
        /* Validate its length */
        if (((ULONG)ReplyMessage->u1.s1.DataLength + sizeof(PORT_MESSAGE)) >
            (ULONG)ReplyMessage->u1.s1.TotalLength)
        {
            /* Fail */
            return STATUS_INVALID_PARAMETER;
        }

        /* Make sure it has a valid ID */
        if (!ReplyMessage->MessageId) return STATUS_INVALID_PARAMETER;
    }

    /* Get the Port object */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if the caller has a reply message */
    if (ReplyMessage)
    {
        /* Validate its length in respect to the port object */
        if (((ULONG)ReplyMessage->u1.s1.TotalLength > Port->MaxMessageLength) ||
            ((ULONG)ReplyMessage->u1.s1.TotalLength <=
             (ULONG)ReplyMessage->u1.s1.DataLength))
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
        Status = PsLookupProcessThreadByCid(&ReplyMessage->ClientId,
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
        if ((WakeupThread->LpcReplyMessageId != ReplyMessage->MessageId) ||
            ((LpcpGetMessageFromThread(WakeupThread)) &&
             (LpcpGetMessageType(&LpcpGetMessageFromThread(WakeupThread)->
                                 Request) != LPC_REQUEST)))
        {
            /* It isn't, fail */
            LpcpFreeToPortZone(Message, 3);
            if (ConnectionPort) ObDereferenceObject(ConnectionPort);
            ObDereferenceObject(WakeupThread);
            ObDereferenceObject(Port);
            return STATUS_REPLY_MESSAGE_MISMATCH;
        }

        /* Copy the message */
        LpcpMoveMessage(&Message->Request,
                        ReplyMessage,
                        ReplyMessage + 1,
                        LPC_REPLY,
                        NULL);

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
            (Thread->LpcReceivedMessageId == ReplyMessage->MessageId))
        {
            /* Clear this data */
            Thread->LpcReceivedMessageId = 0;
            Thread->LpcReceivedMsgIdValid = FALSE;
        }

        /* Free any data information */
        LpcpFreeDataInfoMessage(Port,
                                ReplyMessage->MessageId,
                                ReplyMessage->CallbackId,
                                ReplyMessage->ClientId);

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
            KeResetEvent(&ReceivePort->WaitEvent);
        }

        /* Release the lock and fail */
        KeReleaseGuardedMutex(&LpcpLock);
        if (ConnectionPort) ObDereferenceObject(ConnectionPort);
        ObDereferenceObject(Port);
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the message on the queue */
    Message = CONTAINING_RECORD(RemoveHeadList(&ReceivePort->
                                               MsgQueue.ReceiveHead),
                                LPCP_MESSAGE,
                                Entry);

    /* Check if the queue is empty now */
    if (IsListEmpty(&ReceivePort->MsgQueue.ReceiveHead))
    {
        /* Check if this was a waitable port */
        if (ReceivePort->Flags & LPCP_WAITABLE_PORT)
        {
            /* Reset its event */
            KeResetEvent(&ReceivePort->WaitEvent);
        }
    }

    /* Re-initialize the message's list entry */
    InitializeListHead(&Message->Entry);

    /* Set this as the received message */
    Thread->LpcReceivedMessageId = Message->Request.MessageId;
    Thread->LpcReceivedMsgIdValid = TRUE;

    _SEH_TRY
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
                LpcpSaveDataInfoMessage(Port, Message, 1);
                Message = NULL;
            }
        }
        else
        {
            /* This is a reply message, should never happen! */
            ASSERT(FALSE);
        }
    }
    _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
    {
        DPRINT1("SEH crash [2]\n");
        DbgBreakPoint();
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Check if we have a message pointer here */
    if (Message)
    {
        /* Free it and release the lock */
        LpcpFreeToPortZone(Message, 3);
    }
    else
    {
        /* Just release the lock */
        KeReleaseGuardedMutex(&LpcpLock);
    }

Cleanup:
    /* All done, dereference the port and return the status */
    LPCTRACE(LPC_REPLY_DEBUG,
             "Port: %p. Status: %p\n",
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
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReplyWaitReplyPort(IN HANDLE PortHandle,
                     IN PPORT_MESSAGE ReplyMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtReadRequestData(IN HANDLE PortHandle,
                  IN PPORT_MESSAGE Message,
                  IN ULONG Index,
                  IN PVOID Buffer,
                  IN ULONG BufferLength,
                  OUT PULONG Returnlength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
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
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
