/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/lpc/complete.c
* PURPOSE:         Local Procedure Call: Connection Completion
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
LpcpPrepareToWakeClient(IN PETHREAD Thread)
{
    PAGED_CODE();

    /* Make sure the thread isn't dying and it has a valid chain */
    if (!(Thread->LpcExitThreadCalled) &&
        !(IsListEmpty(&Thread->LpcReplyChain)))
    {
        /* Remove it from the list and reinitialize it */
        RemoveEntryList(&Thread->LpcReplyChain);
        InitializeListHead(&Thread->LpcReplyChain);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAcceptConnectPort(OUT PHANDLE PortHandle,
                    IN PVOID PortContext OPTIONAL,
                    IN PPORT_MESSAGE ReplyMessage,
                    IN BOOLEAN AcceptConnection,
                    IN PPORT_VIEW ServerView,
                    IN PREMOTE_PORT_VIEW ClientView)
{
    PLPCP_PORT_OBJECT ConnectionPort, ServerPort, ClientPort;
    PVOID ClientSectionToMap = NULL;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;
    ULONG ConnectionInfoLength;
    PLPCP_MESSAGE Message;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    PEPROCESS ClientProcess;
    PETHREAD ClientThread;
    LARGE_INTEGER SectionOffset;
    PAGED_CODE();
    LPCTRACE(LPC_COMPLETE_DEBUG,
             "Context: %p. Message: %p. Accept: %lx. Views: %p/%p\n",
             PortContext,
             ReplyMessage,
             AcceptConnection,
             ClientView,
             ServerView);

    /* Validate the size of the server view */
    if ((ServerView) && (ServerView->Length != sizeof(PORT_VIEW)))
    {
        /* Invalid size */
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate the size of the client view */
    if ((ClientView) && (ClientView->Length != sizeof(REMOTE_PORT_VIEW)))
    {
        /* Invalid size */
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the client process and thread */
    Status = PsLookupProcessThreadByCid(&ReplyMessage->ClientId,
                                        &ClientProcess,
                                        &ClientThread);
    if (!NT_SUCCESS(Status)) return Status;

    /* Acquire the LPC Lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Make sure that the client wants a reply, and this is the right one */
    if (!(LpcpGetMessageFromThread(ClientThread)) ||
        !(ReplyMessage->MessageId) ||
        (ClientThread->LpcReplyMessageId != ReplyMessage->MessageId))
    {
        /* Not the reply asked for, or no reply wanted, fail */
        KeReleaseGuardedMutex(&LpcpLock);
        ObDereferenceObject(ClientProcess);
        ObDereferenceObject(ClientThread);
        return STATUS_REPLY_MESSAGE_MISMATCH;
    }

    /* Now get the message and connection message */
    Message = LpcpGetMessageFromThread(ClientThread);
    ConnectMessage = (PLPCP_CONNECTION_MESSAGE)(Message + 1);

    /* Get the client and connection port as well */
    ClientPort = ConnectMessage->ClientPort;
    ConnectionPort = ClientPort->ConnectionPort;

    /* Make sure that the reply is being sent to the proper server process */
    if (ConnectionPort->ServerProcess != PsGetCurrentProcess())
    {
        /* It's not, so fail */
        KeReleaseGuardedMutex(&LpcpLock);
        ObDereferenceObject(ClientProcess);
        ObDereferenceObject(ClientThread);
        return STATUS_REPLY_MESSAGE_MISMATCH;
    }

    /* At this point, don't let other accept attempts happen */
    ClientThread->LpcReplyMessage = NULL;
    ClientThread->LpcReplyMessageId = 0;

    /* Clear the client port for now as well, then release the lock */
    ConnectMessage->ClientPort = NULL;
    KeReleaseGuardedMutex(&LpcpLock);

    /* Get the connection information length */
    ConnectionInfoLength = ReplyMessage->u1.s1.DataLength;
    if (ConnectionInfoLength > ConnectionPort->MaxConnectionInfoLength)
    {
        /* Normalize it since it's too large */
        ConnectionInfoLength = ConnectionPort->MaxConnectionInfoLength;
    }

    /* Set the sizes of our reply message */
    Message->Request.u1.s1.DataLength = sizeof(LPCP_CONNECTION_MESSAGE) +
                                    ConnectionInfoLength;
    Message->Request.u1.s1.TotalLength = sizeof(LPCP_MESSAGE) +
                                     Message->Request.u1.s1.DataLength;

    /* Setup the reply message */
    Message->Request.u2.s2.Type = LPC_REPLY;
    Message->Request.u2.s2.DataInfoOffset = 0;
    Message->Request.ClientId = ReplyMessage->ClientId;
    Message->Request.MessageId = ReplyMessage->MessageId;
    Message->Request.ClientViewSize = 0;
    RtlCopyMemory(ConnectMessage + 1, ReplyMessage + 1, ConnectionInfoLength);

    /* At this point, if the caller refused the connection, go to cleanup */
    if (!AcceptConnection) goto Cleanup;

    /* Otherwise, create the actual port */
    Status = ObCreateObject(PreviousMode,
                            LpcPortObjectType,
                            NULL,
                            PreviousMode,
                            NULL,
                            sizeof(LPCP_PORT_OBJECT),
                            0,
                            0,
                            (PVOID*)&ServerPort);
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Set it up */
    RtlZeroMemory(ServerPort, sizeof(LPCP_PORT_OBJECT));
    ServerPort->PortContext = PortContext;
    ServerPort->Flags = LPCP_COMMUNICATION_PORT;
    ServerPort->MaxMessageLength = ConnectionPort->MaxMessageLength;
    InitializeListHead(&ServerPort->LpcReplyChainHead);
    InitializeListHead(&ServerPort->LpcDataInfoChainHead);

    /* Reference the connection port until we're fully setup */
    ObReferenceObject(ConnectionPort);

    /* Link the ports together */
    ServerPort->ConnectionPort = ConnectionPort;
    ServerPort->ConnectedPort = ClientPort;
    ClientPort->ConnectedPort = ServerPort;

    /* Also set the creator CID */
    ServerPort->Creator = PsGetCurrentThread()->Cid;
    ClientPort->Creator = Message->Request.ClientId;

    /* Get the section associated and then clear it, while inside the lock */
    KeAcquireGuardedMutex(&LpcpLock);
    ClientSectionToMap = ConnectMessage->SectionToMap;
    ConnectMessage->SectionToMap = NULL;
    KeReleaseGuardedMutex(&LpcpLock);

    /* Now check if there's a client section */
    if (ClientSectionToMap)
    {
        /* Setup the offset */
        SectionOffset.QuadPart = ConnectMessage->ClientView.SectionOffset;

        /* Map the section */
        Status = MmMapViewOfSection(ClientSectionToMap,
                                    PsGetCurrentProcess(),
                                    &ServerPort->ClientSectionBase,
                                    0,
                                    0,
                                    &SectionOffset,
                                    &ConnectMessage->ClientView.ViewSize,
                                    ViewUnmap,
                                    0,
                                    PAGE_READWRITE);

        /* Update the offset and check for mapping status */
        ConnectMessage->ClientView.SectionOffset = SectionOffset.LowPart;
        if (NT_SUCCESS(Status))
        {
            /* Set the view base */
            ConnectMessage->ClientView.ViewRemoteBase = ServerPort->
                                                        ClientSectionBase;

            /* Save and reference the mapping process */
            ServerPort->MappingProcess = PsGetCurrentProcess();
            ObReferenceObject(ServerPort->MappingProcess);
        }
        else
        {
            /* Otherwise, quit */
            ObDereferenceObject(ServerPort);
            goto Cleanup;
        }
    }

    /* Check if there's a server section */
    if (ServerView)
    {
        /* FIXME: TODO */
        ASSERT(FALSE);
    }

    /* Reference the server port until it's fully inserted */
    ObReferenceObject(ServerPort);

    /* Insert the server port in the namespace */
    Status = ObInsertObject(ServerPort,
                            NULL,
                            PORT_ALL_ACCESS,
                            0,
                            NULL,
                            &Handle);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, remove the extra reference and cleanup */
        ObDereferenceObject(ServerPort);
        goto Cleanup;
    }

    /* Check if the caller gave a client view */
    if (ClientView)
    {
        /* Fill it out */
        ClientView->ViewBase = ConnectMessage->ClientView.ViewRemoteBase;
        ClientView->ViewSize = ConnectMessage->ClientView.ViewSize;
    }

    /* Return the handle to user mode */
    *PortHandle = Handle;
    LPCTRACE(LPC_COMPLETE_DEBUG,
             "Handle: %lx. Messages: %p/%p. Ports: %p/%p/%p\n",
             Handle,
             Message,
             ConnectMessage,
             ServerPort,
             ClientPort,
             ConnectionPort);

    /* If there was no port context, use the handle by default */
    if (!PortContext) ServerPort->PortContext = Handle;
    ServerPort->ClientThread = ClientThread;

    /* Set this message as the LPC Reply message while holding the lock */
    KeAcquireGuardedMutex(&LpcpLock);
    ClientThread->LpcReplyMessage = Message;
    KeReleaseGuardedMutex(&LpcpLock);

    /* Clear the thread pointer so it doesn't get cleaned later */
    ClientThread = NULL;

    /* Remove the extra reference we had added */
    ObDereferenceObject(ServerPort);

Cleanup:
    /* If there was a section, dereference it */
    if (ClientSectionToMap) ObDereferenceObject(ClientSectionToMap);

    /* Check if we got here while still having a client thread */
    if (ClientThread)
    {
        /* FIXME: Complex cleanup code */
        ASSERT(FALSE);
    }

    /* Dereference the client port if we have one, and the process */
    LPCTRACE(LPC_COMPLETE_DEBUG,
             "Status: %lx. Thread: %p. Process: [%.16s]\n",
             Status,
             ClientThread,
             ClientProcess->ImageFileName);
    if (ClientPort) ObDereferenceObject(ClientPort);
    ObDereferenceObject(ClientProcess);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCompleteConnectPort(IN HANDLE PortHandle)
{
    NTSTATUS Status;
    PLPCP_PORT_OBJECT Port;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD Thread;
    PAGED_CODE();
    LPCTRACE(LPC_COMPLETE_DEBUG, "Handle: %lx\n", PortHandle);

    /* Get the Port Object */
    Status = ObReferenceObjectByHandle(PortHandle,
                                       PORT_ALL_ACCESS,
                                       LpcPortObjectType,
                                       PreviousMode,
                                       (PVOID*)&Port,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Make sure this is a connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_COMMUNICATION_PORT)
    {
        /* It isn't, fail */
        ObDereferenceObject(Port);
        return STATUS_INVALID_PORT_HANDLE;
    }

    /* Acquire the lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Make sure we have a client thread */
    if (!Port->ClientThread)
    {
        /* We don't, fail */
        KeReleaseGuardedMutex(&LpcpLock);
        ObDereferenceObject(Port);
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the thread */
    Thread = Port->ClientThread;

    /* Make sure it has a reply message */
    if (!LpcpGetMessageFromThread(Thread))
    {
        /* It doesn't, quit */
        KeReleaseGuardedMutex(&LpcpLock);
        ObDereferenceObject(Port);
        return STATUS_SUCCESS;
    }

    /* Clear the client thread and wake it up */
    Port->ClientThread = NULL;
    LpcpPrepareToWakeClient(Thread);

    /* Release the lock and wait for an answer */
    KeReleaseGuardedMutex(&LpcpLock);
    LpcpCompleteWait(&Thread->LpcReplySemaphore);

    /* Dereference the Thread and Port  and return */
    ObDereferenceObject(Port);
    ObDereferenceObject(Thread);
    LPCTRACE(LPC_COMPLETE_DEBUG, "Port: %p. Thread: %p\n", Port, Thread);
    return Status;
}

/* EOF */
