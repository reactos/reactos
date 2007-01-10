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
    Message = Thread->LpcReplyMessage;
    if (Message)
    {
        /* FIXME: TODO */
        KEBUGCHECK(0);
    }

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);
}

VOID
NTAPI
LpcpFreeToPortZone(IN PLPCP_MESSAGE Message,
                   IN ULONG Flags)
{
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    PLPCP_PORT_OBJECT ClientPort = NULL;
    PETHREAD Thread = NULL;
    BOOLEAN LockHeld = Flags & 1;
    PAGED_CODE();
    LPCTRACE(LPC_CLOSE_DEBUG, "Message: %p. Flags: %lx\n", Message, Flags);

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
    if ((LockHeld) && !(Flags & 2)) KeAcquireGuardedMutex(&LpcpLock);
}

VOID
NTAPI
LpcpDestroyPortQueue(IN PLPCP_PORT_OBJECT Port,
                     IN BOOLEAN Destroy)
{
    PLIST_ENTRY ListHead, NextEntry;
    PETHREAD Thread;
    PLPCP_MESSAGE Message;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    LPCTRACE(LPC_CLOSE_DEBUG, "Port: %p. Flags: %lx\n", Port, Port->Flags);

    /* Hold the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Disconnect the port to which this port is connected */
    if (Port->ConnectedPort) Port->ConnectedPort->ConnectedPort = NULL;

    /* Check if this is a connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) == LPCP_CONNECTION_PORT)
    {
        /* Delete the name */
        Port->Flags |= LPCP_NAME_DELETED;
    }

    /* Walk all the threads waiting and signal them */
    ListHead = &Port->LpcReplyChainHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
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
            /* Get the message and check if it's a connection request */
            Message = Thread->LpcReplyMessage;
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
            LpcpFreeToPortZone(Message, TRUE);
        }

        /* Release the semaphore and reset message id count */
        Thread->LpcReplyMessageId = 0;
        LpcpCompleteWait(&Thread->LpcReplySemaphore);
    }

    /* Reinitialize the list head */
    InitializeListHead(&Port->LpcReplyChainHead);

    /* Loop queued messages */
    ListHead = &Port->MsgQueue.ReceiveHead;
    NextEntry =  ListHead->Flink;
    while (ListHead != NextEntry)
    {
        /* Get the message */
        Message = CONTAINING_RECORD(NextEntry, LPCP_MESSAGE, Entry);
        NextEntry = NextEntry->Flink;

        /* Free and reinitialize it's list head */
        InitializeListHead(&Message->Entry);

        /* Remove it from the port zone */
        LpcpFreeToPortZone(Message, TRUE);
    }

    /* Reinitialize the message queue list head */
    InitializeListHead(&Port->MsgQueue.ReceiveHead);

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

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
        ClientDiedMsg.h.u2.ZeroInit = LPC_PORT_CLOSED;
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

    /* Check if we had a client view */
    if (Port->ClientSectionBase) MmUnmapViewOfSection(PsGetCurrentProcess(),
                                                      Port->ClientSectionBase);

    /* Check for a server view */
    if (Port->ServerSectionBase) MmUnmapViewOfSection(PsGetCurrentProcess(),
                                                      Port->ServerSectionBase);

    /* Get the connection port */
    ConnectionPort = Port->ConnectionPort;
    if (ConnectionPort)
    {
        /* Get the PID */
        Pid = PsGetCurrentProcessId();

        /* Acquire the lock */
        KeAcquireGuardedMutex(&LpcpLock);

        /* Loop the data lists */
        ListHead = &ConnectionPort->LpcDataInfoChainHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the message */
            Message = CONTAINING_RECORD(NextEntry, LPCP_MESSAGE, Entry);
            NextEntry = NextEntry->Flink;

            /* Check if the PID matches */
            if (Message->Request.ClientId.UniqueProcess == Pid)
            {
                /* Remove it */
                RemoveEntryList(&Message->Entry);
                LpcpFreeToPortZone(Message, TRUE);
            }
        }

        /* Release the lock */
        KeReleaseGuardedMutex(&LpcpLock);

        /* Dereference the object unless it's the same port */
        if (ConnectionPort != Port) ObDereferenceObject(ConnectionPort);
    }

    /* Check if this is a connection port with a server process*/
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
