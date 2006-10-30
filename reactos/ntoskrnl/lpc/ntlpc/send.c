/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/send.c
 * PURPOSE:         Local Procedure Call: Sending (Requests)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include "lpc.h"
#define NDEBUG
#include <internal/debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
LpcRequestPort(IN PVOID Port,
               IN PPORT_MESSAGE LpcMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
* @unimplemented
*/
NTSTATUS
NTAPI
LpcRequestWaitReplyPort(IN PVOID Port,
                        IN PPORT_MESSAGE LpcMessageRequest,
                        OUT PPORT_MESSAGE LpcMessageReply)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtRequestPort(IN HANDLE PortHandle,
              IN PPORT_MESSAGE LpcMessage)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtRequestWaitReplyPort(IN HANDLE PortHandle,
                       IN PPORT_MESSAGE LpcRequest,
                       IN OUT PPORT_MESSAGE LpcReply)
{
    PLPCP_PORT_OBJECT Port, QueuePort, ReplyPort;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;
    PLPCP_MESSAGE Message;
    PETHREAD Thread = PsGetCurrentThread();
    BOOLEAN Callback;
    PKSEMAPHORE Semaphore;
    ULONG MessageType;
    PAGED_CODE();
    LPCTRACE(LPC_SEND_DEBUG,
             "Handle: %lx. Messages: %p/%p. Type: %lx\n",
             PortHandle,
             LpcRequest,
             LpcReply,
             LpcpGetMessageType(LpcRequest));

    /* Check if the thread is dying */
    if (Thread->LpcExitThreadCalled) return STATUS_THREAD_IS_TERMINATING;

    /* Check if this is an LPC Request */
    if (LpcpGetMessageType(LpcRequest) == LPC_REQUEST)
    {
        /* Then it's a callback */
        Callback = TRUE;
    }
    else if (LpcpGetMessageType(LpcRequest))
    {
        /* This is a not kernel-mode message */
        return STATUS_INVALID_PARAMETER;
    }
    else
    {
        /* This is a kernel-mode message without a callback */
        LpcRequest->u2.s2.Type |= LPC_REQUEST;
        Callback = FALSE;
    }

    /* Get the message type */
    MessageType = LpcRequest->u2.s2.Type;

    /* Validate the length */
    if ((LpcRequest->u1.s1.DataLength + sizeof(PORT_MESSAGE)) >
         LpcRequest->u1.s1.TotalLength)
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Reference the object */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       0,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Validate the message length */
    if ((LpcRequest->u1.s1.TotalLength > Port->MaxMessageLength) ||
        (LpcRequest->u1.s1.TotalLength <= LpcRequest->u1.s1.DataLength))
    {
        /* Fail */
        ObDereferenceObject(Port);
        return STATUS_PORT_MESSAGE_TOO_LONG;
    }

    /* Acquire the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Allocate a message */
    Message = ExAllocateFromPagedLookasideList(&LpcpMessagesLookaside);
    KeReleaseGuardedMutex(&LpcpLock);

    /* Check if allocation worked */
    if (!Message)
    {
        /* Fail */
        ObDereferenceObject(Port);
        return STATUS_NO_MEMORY;
    }

    /* Check if this is a callback */
    if (Callback)
    {
        /* FIXME: TODO */
        Semaphore = NULL; // we'd use the Thread Semaphore here
        ASSERT(FALSE);
    }
    else
    {
        /* No callback, just copy the message */
        LpcpMoveMessage(&Message->Request,
                        LpcRequest,
                        LpcRequest + 1,
                        MessageType,
                        &Thread->Cid);

        /* Acquire the LPC lock */
        KeAcquireGuardedMutex(&LpcpLock);

        /* Right now clear the port context */
        Message->PortContext = NULL;

        /* Check if this is a not connection port */
        if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_CONNECTION_PORT)
        {
            /* We want the connected port */
            QueuePort = Port->ConnectedPort;
            if (!QueuePort)
            {
                /* We have no connected port, fail */
                LpcpFreeToPortZone(Message, TRUE);
                KeReleaseGuardedMutex(&LpcpLock);
                ObDereferenceObject(Port);
                return STATUS_PORT_DISCONNECTED;
            }

            /* This will be the rundown port */
            ReplyPort = QueuePort;

            /* Check if this is a communication port */
            if ((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_CLIENT_PORT)
            {
                /* Copy the port context and use the connection port */
                Message->PortContext = ReplyPort->PortContext;
                QueuePort = Port->ConnectionPort;
            }
            else if ((Port->Flags & LPCP_PORT_TYPE_MASK) !=
                      LPCP_COMMUNICATION_PORT)
            {
                /* Use the connection port for anything but communication ports */
                QueuePort = Port->ConnectionPort;
            }
        }
        else
        {
            /* Otherwise, for a connection port, use the same port object */
            QueuePort = ReplyPort = Port;
        }

        /* No reply thread */
        Message->RepliedToThread = NULL;

        /* Generate the Message ID and set it */
        Message->Request.MessageId =  LpcpNextMessageId++;
        if (!LpcpNextMessageId) LpcpNextMessageId = 1;
        Message->Request.CallbackId = 0;

        /* Set the message ID for our thread now */
        Thread->LpcReplyMessageId = Message->Request.MessageId;
        Thread->LpcReplyMessage = NULL;

        /* Insert the message in our chain */
        InsertTailList(&QueuePort->MsgQueue.ReceiveHead, &Message->Entry);
        InsertTailList(&ReplyPort->LpcReplyChainHead, &Thread->LpcReplyChain);

        /* Release the lock and get the semaphore we'll use later */
        KeReleaseGuardedMutex(&LpcpLock);
        Semaphore = QueuePort->MsgQueue.Semaphore;

        /* If this is a waitable port, wake it up */
        if (QueuePort->Flags & LPCP_WAITABLE_PORT)
        {
            /* Wake it */
            KeSetEvent(&QueuePort->WaitEvent, IO_NO_INCREMENT, FALSE);
        }
    }

    /* Now release the semaphore */
    LpcpCompleteWait(Semaphore);

    /* And let's wait for the reply */
    LpcpReplyWait(&Thread->LpcReplySemaphore, PreviousMode);

    /* Acquire the LPC lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Get the LPC Message and clear our thread's reply data */
    Message = Thread->LpcReplyMessage;
    Thread->LpcReplyMessage = NULL;
    Thread->LpcReplyMessageId = 0;

    /* Check if we have anything on the reply chain*/
    if (!IsListEmpty(&Thread->LpcReplyChain))
    {
        /* Remove this thread and reinitialize the list */
        RemoveEntryList(&Thread->LpcReplyChain);
        InitializeListHead(&Thread->LpcReplyChain);
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

    /* Check if we got a reply */
    if (Status == STATUS_SUCCESS)
    {
        /* Check if we have a valid message */
        if (Message)
        {
            LPCTRACE(LPC_SEND_DEBUG,
                     "Reply Messages: %p/%p\n",
                     &Message->Request,
                     (&Message->Request) + 1);

            /* Move the message */
            LpcpMoveMessage(LpcReply,
                            &Message->Request,
                            (&Message->Request) + 1,
                            0,
                            NULL);

            /* Check if this is an LPC request with data information */
            if ((LpcpGetMessageType(&Message->Request) == LPC_REQUEST) &&
                (Message->Request.u2.s2.DataInfoOffset))
            {
                /* Save the data information */
                LpcpSaveDataInfoMessage(Port, Message);
            }
            else
            {
                /* Otherwise, just free it */
                LpcpFreeToPortZone(Message, FALSE);
            }
        }
        else
        {
            /* We don't have a reply */
            Status = STATUS_LPC_REPLY_LOST;
        }
    }
    else
    {
        /* The wait failed, free the message while holding the lock */
        KeAcquireGuardedMutex(&LpcpLock);
        LpcpFreeToPortZone(Message, TRUE);
        KeReleaseGuardedMutex(&LpcpLock);
    }

    /* All done */
    LPCTRACE(LPC_SEND_DEBUG,
             "Port: %p. Status: %p\n",
             Port,
             Status);
    ObDereferenceObject(Port);
    return Status;
}

/* EOF */
