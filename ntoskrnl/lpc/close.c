/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/close.c
 * PURPOSE:         Local Procedure Call: Rundown, Cleanup, Deletion
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
LpcExitThread(IN PETHREAD Thread)
{
    PLPCP_MESSAGE Message;
    ASSERT(Thread == PsGetCurrentThread());

    /* Acquire the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Make sure that the Reply Chain is empty */
    if (!IsListEmpty(&Thread->LpcReplyChain))
    {
        /* It's not, remove the entry */
        RemoveEntryList(&Thread->LpcReplyChain);
    }

    /* Set the thread in exit mode */
    Thread->LpcExitThreadCalled = TRUE;
    Thread->LpcReplyMessageId = 0;

    /* Check if there's a reply message */
    Message = LpcpGetMessageFromThread(Thread);
    if (Message)
    {
        /* FIXME: TODO */
        ASSERT(FALSE);
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);
}

VOID
NTAPI
LpcpFreeToPortZone(IN PLPCP_MESSAGE Message,
                   IN ULONG LockFlags)
{
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    PLPCP_PORT_OBJECT ClientPort = NULL;
    PETHREAD Thread = NULL;
    BOOLEAN LockHeld = (LockFlags & LPCP_LOCK_HELD);
    BOOLEAN ReleaseLock = (LockFlags & LPCP_LOCK_RELEASE);

    PAGED_CODE();

    LPCTRACE(LPC_CLOSE_DEBUG, "Message: %p. LockFlags: %lx\n", Message, LockFlags);

    /* Acquire the lock if not already */
    if (!LockHeld) KeAcquireGuardedMutex(&LpcpLock);

    /* Check if the queue list is empty */
    if (!IsListEmpty(&Message->Entry))
    {
        /* Remove and re-initialize */
        RemoveEntryList(&Message->Entry);
        InitializeListHead(&Message->Entry);
    }

    /* Check if we've already replied */
    if (Message->RepliedToThread)
    {
        /* Set thread to dereference and clean up */
        Thread = Message->RepliedToThread;
        Message->RepliedToThread = NULL;
    }

    /* Check if this is a connection request */
    if (Message->Request.u2.s2.Type == LPC_CONNECTION_REQUEST)
    {
        /* Get the connection message */
        ConnectMessage = (PLPCP_CONNECTION_MESSAGE)(Message + 1);

        /* Clear the client port */
        ClientPort = ConnectMessage->ClientPort;
        if (ClientPort) ConnectMessage->ClientPort = NULL;
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

    /* Check if we had anything to dereference */
    if (Thread) ObDereferenceObject(Thread);
    if (ClientPort) ObDereferenceObject(ClientPort);

    /* Free the entry */
    ExFreeToPagedLookasideList(&LpcpMessagesLookaside, Message);

    /* Reacquire the lock if needed */
    if ((LockHeld) && !(ReleaseLock)) KeAcquireGuardedMutex(&LpcpLock);
}

VOID
NTAPI
LpcpDestroyPortQueue(IN PLPCP_PORT_OBJECT Port,
                     IN BOOLEAN Destroy)
{
    PLIST_ENTRY ListHead, NextEntry;
    PETHREAD Thread;
    PLPCP_MESSAGE Message;
    PLPCP_PORT_OBJECT ConnectionPort = NULL;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    PAGED_CODE();
    LPCTRACE(LPC_CLOSE_DEBUG, "Port: %p. Flags: %lx\n", Port, Port->Flags);

    /* Hold the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check if we have a connected port */
    if (((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_UNCONNECTED_PORT) &&
        (Port->ConnectedPort))
    {
        /* Disconnect it */
        Port->ConnectedPort->ConnectedPort = NULL;
        ConnectionPort = Port->ConnectedPort->ConnectionPort;
        if (ConnectionPort)
        {
            /* Clear connection port */
            Port->ConnectedPort->ConnectionPort = NULL;
        }
    }

    /* Check if this is a connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_CONNECTION_PORT)
    {
        /* Delete the name */
        Port->Flags |= LPCP_NAME_DELETED;
    }

    /* Walk all the threads waiting and signal them */
    ListHead = &Port->LpcReplyChainHead;
    NextEntry = ListHead->Flink;
    while ((NextEntry) && (NextEntry != ListHead))
    {
        /* Get the Thread */
        Thread = CONTAINING_RECORD(NextEntry, ETHREAD, LpcReplyChain);

        /* Make sure we're not in exit */
        if (Thread->LpcExitThreadCalled) break;

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;

        /* Remove and reinitialize the List */
        RemoveEntryList(&Thread->LpcReplyChain);
        InitializeListHead(&Thread->LpcReplyChain);

        /* Check if someone is waiting */
        if (!KeReadStateSemaphore(&Thread->LpcReplySemaphore))
        {
            /* Get the message */
            Message = LpcpGetMessageFromThread(Thread);
            if (Message)
            {
                /* Check if it's a connection request */
                if (Message->Request.u2.s2.Type == LPC_CONNECTION_REQUEST)
                {
                    /* Get the connection message */
                    ConnectMessage = (PLPCP_CONNECTION_MESSAGE)(Message + 1);

                    /* Check if it had a section */
                    if (ConnectMessage->SectionToMap)
                    {
                        /* Dereference it */
                        ObDereferenceObject(ConnectMessage->SectionToMap);
                    }
                }

                /* Clear the reply message */
                Thread->LpcReplyMessage = NULL;

                /* And remove the message from the port zone */
                LpcpFreeToPortZone(Message, LPCP_LOCK_HELD);
                NextEntry = Port->LpcReplyChainHead.Flink;
            }

            /* Release the semaphore and reset message id count */
            Thread->LpcReplyMessageId = 0;
            KeReleaseSemaphore(&Thread->LpcReplySemaphore, 0, 1, FALSE);
        }
    }

    /* Reinitialize the list head */
    InitializeListHead(&Port->LpcReplyChainHead);

    /* Loop queued messages */
    while ((Port->MsgQueue.ReceiveHead.Flink) &&
           !(IsListEmpty(&Port->MsgQueue.ReceiveHead)))
    {
        /* Get the message */
        Message = CONTAINING_RECORD(Port->MsgQueue.ReceiveHead.Flink,
                                    LPCP_MESSAGE,
                                    Entry);

        /* Free and reinitialize it's list head */
        RemoveEntryList(&Message->Entry);
        InitializeListHead(&Message->Entry);

        /* Remove it from the port zone */
        LpcpFreeToPortZone(Message, LPCP_LOCK_HELD);
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

    /* Dereference the connection port */
    if (ConnectionPort) ObDereferenceObject(ConnectionPort);

    /* Check if we have to free the port entirely */
    if (Destroy)
    {
        /* Check if the semaphore exists */
        if (Port->MsgQueue.Semaphore)
        {
            /* Use the semaphore to find the port queue and free it */
            ExFreePool(CONTAINING_RECORD(Port->MsgQueue.Semaphore,
                                         LPCP_NONPAGED_PORT_QUEUE,
                                         Semaphore));
        }
    }
}

VOID
NTAPI
LpcpClosePort(IN PEPROCESS Process OPTIONAL,
              IN PVOID Object,
              IN ACCESS_MASK GrantedAccess,
              IN ULONG ProcessHandleCount,
              IN ULONG SystemHandleCount)
{
    PLPCP_PORT_OBJECT Port = (PLPCP_PORT_OBJECT)Object;
    LPCTRACE(LPC_CLOSE_DEBUG, "Port: %p. Flags: %lx\n", Port, Port->Flags);

    /* Only Server-side Connection Ports need clean up*/
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_CONNECTION_PORT)
    {
        /* Check the handle count */
        switch (SystemHandleCount)
        {
            /* No handles left */
            case 0:

                /* Destroy the port queue */
                LpcpDestroyPortQueue(Port, TRUE);
                break;

            /* Last handle remaining */
            case 1:

                /* Reset the queue only */
                LpcpDestroyPortQueue(Port, FALSE);

            /* More handles remain, do nothing */
            default:
                break;
        }
    }
}

VOID
NTAPI
LpcpFreePortClientSecurity(IN PLPCP_PORT_OBJECT Port)
{
    /* Check if this is a client port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_CLIENT_PORT)
    {
        /* Check if security is static */
        if (!(Port->Flags & LPCP_SECURITY_DYNAMIC))
        {
            /* Check if we have a token */
            if (Port->StaticSecurity.ClientToken)
            {
                /* Free security */
                SeDeleteClientSecurity(&Port->StaticSecurity);
            }
        }
    }
}

VOID
NTAPI
LpcpDeletePort(IN PVOID ObjectBody)
{
    LARGE_INTEGER Timeout;
    PETHREAD Thread;
    PLPCP_PORT_OBJECT Port = (PLPCP_PORT_OBJECT)ObjectBody;
    PLPCP_PORT_OBJECT ConnectionPort;
    PLPCP_MESSAGE Message;
    PLIST_ENTRY ListHead, NextEntry;
    HANDLE Pid;
    CLIENT_DIED_MSG ClientDiedMsg;
    Timeout.QuadPart = -1000000;
    PAGED_CODE();
    LPCTRACE(LPC_CLOSE_DEBUG, "Port: %p. Flags: %lx\n", Port, Port->Flags);

    /* Check if this is a communication port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_COMMUNICATION_PORT)
    {
        /* Acquire the lock */
        KeAcquireGuardedMutex(&LpcpLock);

        /* Get the thread */
        Thread = Port->ClientThread;
        if (Thread)
        {
            /* Clear it */
            Port->ClientThread = NULL;

            /* Release the lock and dereference */
            KeReleaseGuardedMutex(&LpcpLock);
            ObDereferenceObject(Thread);
        }
        else
        {
            /* Release the lock */
            KeReleaseGuardedMutex(&LpcpLock);
        }
    }

    /* Check if this is a client-side port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_CLIENT_PORT)
    {
        /* Setup the client died message */
        ClientDiedMsg.h.u1.s1.TotalLength = sizeof(ClientDiedMsg);
        ClientDiedMsg.h.u1.s1.DataLength = sizeof(ClientDiedMsg.CreateTime);
        ClientDiedMsg.h.u2.ZeroInit = 0;
        ClientDiedMsg.h.u2.s2.Type = LPC_PORT_CLOSED;
        ClientDiedMsg.CreateTime = PsGetCurrentProcess()->CreateTime;

        /* Send it */
        for (;;)
        {
            /* Send the message */
            if (LpcRequestPort(Port,
                               &ClientDiedMsg.h) != STATUS_NO_MEMORY) break;

            /* Wait until trying again */
            KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        }
    }

    /* Destroy the port queue */
    LpcpDestroyPortQueue(Port, TRUE);

    /* Check if we had views */
    if ((Port->ClientSectionBase) || (Port->ServerSectionBase))
    {
        /* Check if we had a client view */
        if (Port->ClientSectionBase)
        {
            /* Unmap it */
            MmUnmapViewOfSection(Port->MappingProcess,
                                 Port->ClientSectionBase);
        }

        /* Check for a server view */
        if (Port->ServerSectionBase)
        {
            /* Unmap it */
            MmUnmapViewOfSection(Port->MappingProcess,
                                 Port->ServerSectionBase);
        }

        /* Dereference the mapping process */
        ObDereferenceObject(Port->MappingProcess);
        Port->MappingProcess = NULL;
    }

    /* Acquire the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Get the connection port */
    ConnectionPort = Port->ConnectionPort;
    if (ConnectionPort)
    {
        /* Get the PID */
        Pid = PsGetCurrentProcessId();

        /* Loop the data lists */
        ListHead = &ConnectionPort->LpcDataInfoChainHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the message */
            Message = CONTAINING_RECORD(NextEntry, LPCP_MESSAGE, Entry);
            NextEntry = NextEntry->Flink;

            /* Check if this is the connection port */
            if (Port == ConnectionPort)
            {
                /* Free queued messages */
                RemoveEntryList(&Message->Entry);
                InitializeListHead(&Message->Entry);
                LpcpFreeToPortZone(Message, LPCP_LOCK_HELD);

                /* Restart at the head */
                NextEntry = ListHead->Flink;
            }
            else if ((Message->Request.ClientId.UniqueProcess == Pid) &&
                     ((Message->SenderPort == Port) ||
                      (Message->SenderPort == Port->ConnectedPort) ||
                      (Message->SenderPort == ConnectionPort)))
            {
                /* Remove it */
                RemoveEntryList(&Message->Entry);
                InitializeListHead(&Message->Entry);
                LpcpFreeToPortZone(Message, LPCP_LOCK_HELD);

                /* Restart at the head */
                NextEntry = ListHead->Flink;
            }
        }

        /* Release the lock */
        KeReleaseGuardedMutex(&LpcpLock);

        /* Dereference the object unless it's the same port */
        if (ConnectionPort != Port) ObDereferenceObject(ConnectionPort);
    }
    else
    {
        /* Release the lock */
        KeReleaseGuardedMutex(&LpcpLock);
    }

    /* Check if this is a connection port with a server process */
    if (((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_CONNECTION_PORT) &&
        (ConnectionPort->ServerProcess))
    {
        /* Dereference the server process */
        ObDereferenceObject(ConnectionPort->ServerProcess);
        ConnectionPort->ServerProcess = NULL;
    }

    /* Free client security */
    LpcpFreePortClientSecurity(Port);
    LPCTRACE(LPC_CLOSE_DEBUG, "Port: %p deleted\n", Port);
}

/* EOF */
