/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/lpc/connect.c
 * PURPOSE:         Local Procedure Call: Connection Management
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS *********************************************************/

PVOID
NTAPI
LpcpFreeConMsg(IN OUT PLPCP_MESSAGE *Message,
               IN OUT PLPCP_CONNECTION_MESSAGE *ConnectMessage,
               IN PETHREAD CurrentThread)
{
    PVOID SectionToMap;
    PLPCP_MESSAGE ReplyMessage;

    /* Acquire the LPC lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check if the reply chain is not empty */
    if (!IsListEmpty(&CurrentThread->LpcReplyChain))
    {
        /* Remove this entry and re-initialize it */
        RemoveEntryList(&CurrentThread->LpcReplyChain);
        InitializeListHead(&CurrentThread->LpcReplyChain);
    }

    /* Check if there's a reply message */
    ReplyMessage = LpcpGetMessageFromThread(CurrentThread);
    if (ReplyMessage)
    {
        /* Get the message */
        *Message = ReplyMessage;

        /* Check if it's got messages */
        if (!IsListEmpty(&ReplyMessage->Entry))
        {
            /* Clear the list */
            RemoveEntryList(&ReplyMessage->Entry);
            InitializeListHead(&ReplyMessage->Entry);
        }

        /* Clear message data */
        CurrentThread->LpcReceivedMessageId = 0;
        CurrentThread->LpcReplyMessage = NULL;

        /* Get the connection message and clear the section */
        *ConnectMessage = (PLPCP_CONNECTION_MESSAGE)(ReplyMessage + 1);
        SectionToMap = (*ConnectMessage)->SectionToMap;
        (*ConnectMessage)->SectionToMap = NULL;
    }
    else
    {
        /* No message to return */
        *Message = NULL;
        SectionToMap = NULL;
    }

    /* Release the lock and return the section */
    KeReleaseGuardedMutex(&LpcpLock);
    return SectionToMap;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSecureConnectPort(OUT PHANDLE PortHandle,
                    IN PUNICODE_STRING PortName,
                    IN PSECURITY_QUALITY_OF_SERVICE Qos,
                    IN OUT PPORT_VIEW ClientView OPTIONAL,
                    IN PSID ServerSid OPTIONAL,
                    IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
                    OUT PULONG MaxMessageLength OPTIONAL,
                    IN OUT PVOID ConnectionInformation OPTIONAL,
                    IN OUT PULONG ConnectionInformationLength OPTIONAL)
{
    ULONG ConnectionInfoLength = 0;
    PLPCP_PORT_OBJECT Port, ClientPort;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE Handle;
    PVOID SectionToMap;
    PLPCP_MESSAGE Message;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    PETHREAD Thread = PsGetCurrentThread();
    ULONG PortMessageLength;
    LARGE_INTEGER SectionOffset;
    PTOKEN Token;
    PTOKEN_USER TokenUserInfo;
    PAGED_CODE();
    LPCTRACE(LPC_CONNECT_DEBUG,
             "Name: %wZ. Qos: %p. Views: %p/%p. Sid: %p\n",
             PortName,
             Qos,
             ClientView,
             ServerView,
             ServerSid);

    /* Validate client view */
    if ((ClientView) && (ClientView->Length != sizeof(PORT_VIEW)))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate server view */
    if ((ServerView) && (ServerView->Length != sizeof(REMOTE_PORT_VIEW)))
    {
        /* Fail */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if caller sent connection information length */
    if (ConnectionInformationLength)
    {
        /* Retrieve the input length */
        ConnectionInfoLength = *ConnectionInformationLength;
    }

    /* Get the port */
    Status = ObReferenceObjectByName(PortName,
                                     0,
                                     NULL,
                                     PORT_CONNECT,
                                     LpcPortObjectType,
                                     PreviousMode,
                                     NULL,
                                     (PVOID *)&Port);
    if (!NT_SUCCESS(Status)) return Status;

    /* This has to be a connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_CONNECTION_PORT)
    {
        /* It isn't, so fail */
        ObDereferenceObject(Port);
        return STATUS_INVALID_PORT_HANDLE;
    }

    /* Check if we have a SID */
    if (ServerSid)
    {
        /* Make sure that we have a server */
        if (Port->ServerProcess)
        {
            /* Get its token and query user information */
            Token = PsReferencePrimaryToken(Port->ServerProcess);
            //Status = SeQueryInformationToken(Token, TokenUser, (PVOID*)&TokenUserInfo);
            // FIXME: Need SeQueryInformationToken
            Status = STATUS_SUCCESS;
            TokenUserInfo = ExAllocatePool(PagedPool, sizeof(TOKEN_USER));
            TokenUserInfo->User.Sid = ServerSid;
            PsDereferencePrimaryToken(Token);

            /* Check for success */
            if (NT_SUCCESS(Status))
            {
                /* Compare the SIDs */
                if (!RtlEqualSid(ServerSid, TokenUserInfo->User.Sid))
                {
                    /* Fail */
                    Status = STATUS_SERVER_SID_MISMATCH;
                }

                /* Free token information */
                ExFreePool(TokenUserInfo);
            }
        }
        else
        {
            /* Invalid SID */
            Status = STATUS_SERVER_SID_MISMATCH;
        }

        /* Check if SID failed */
        if (!NT_SUCCESS(Status))
        {
            /* Quit */
            ObDereferenceObject(Port);
            return Status;
        }
    }

    /* Create the client port */
    Status = ObCreateObject(PreviousMode,
                            LpcPortObjectType,
                            NULL,
                            PreviousMode,
                            NULL,
                            sizeof(LPCP_PORT_OBJECT),
                            0,
                            0,
                            (PVOID *)&ClientPort);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, dereference the server port and return */
        ObDereferenceObject(Port);
        return Status;
    }

    /* Setup the client port */
    RtlZeroMemory(ClientPort, sizeof(LPCP_PORT_OBJECT));
    ClientPort->Flags = LPCP_CLIENT_PORT;
    ClientPort->ConnectionPort = Port;
    ClientPort->MaxMessageLength = Port->MaxMessageLength;
    ClientPort->SecurityQos = *Qos;
    InitializeListHead(&ClientPort->LpcReplyChainHead);
    InitializeListHead(&ClientPort->LpcDataInfoChainHead);

    /* Check if we have dynamic security */
    if (Qos->ContextTrackingMode == SECURITY_DYNAMIC_TRACKING)
    {
        /* Remember that */
        ClientPort->Flags |= LPCP_SECURITY_DYNAMIC;
    }
    else
    {
        /* Create our own client security */
        Status = SeCreateClientSecurity(Thread,
                                        Qos,
                                        FALSE,
                                        &ClientPort->StaticSecurity);
        if (!NT_SUCCESS(Status))
        {
            /* Security failed, dereference and return */
            ObDereferenceObject(ClientPort);
            return Status;
        }
    }

    /* Initialize the port queue */
    Status = LpcpInitializePortQueue(ClientPort);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        ObDereferenceObject(ClientPort);
        return Status;
    }

    /* Check if we have a client view */
    if (ClientView)
    {
        /* Get the section handle */
        Status = ObReferenceObjectByHandle(ClientView->SectionHandle,
                                           SECTION_MAP_READ |
                                           SECTION_MAP_WRITE,
                                           MmSectionObjectType,
                                           PreviousMode,
                                           (PVOID*)&SectionToMap,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(Port);
            return Status;
        }

        /* Set the section offset */
        SectionOffset.QuadPart = ClientView->SectionOffset;

        /* Map it */
        Status = MmMapViewOfSection(SectionToMap,
                                    PsGetCurrentProcess(),
                                    &ClientPort->ClientSectionBase,
                                    0,
                                    0,
                                    &SectionOffset,
                                    &ClientView->ViewSize,
                                    ViewUnmap,
                                    0,
                                    PAGE_READWRITE);

        /* Update the offset */
        ClientView->SectionOffset = SectionOffset.LowPart;

        /* Check for failure */
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(SectionToMap);
            ObDereferenceObject(Port);
            return Status;
        }

        /* Update the base */
        ClientView->ViewBase = ClientPort->ClientSectionBase;

        /* Reference and remember the process */
        ClientPort->MappingProcess = PsGetCurrentProcess();
        ObReferenceObject(ClientPort->MappingProcess);
    }
    else
    {
        /* No section */
        SectionToMap = NULL;
    }

    /* Normalize connection information */
    if (ConnectionInfoLength > Port->MaxConnectionInfoLength)
    {
        /* Use the port's maximum allowed value */
        ConnectionInfoLength = Port->MaxConnectionInfoLength;
    }

    /* Allocate a message from the port zone */
    Message = LpcpAllocateFromPortZone();
    if (!Message)
    {
        /* Fail if we couldn't allocate a message */
        if (SectionToMap) ObDereferenceObject(SectionToMap);
        ObDereferenceObject(ClientPort);
        return STATUS_NO_MEMORY;
    }

    /* Set pointer to the connection message and fill in the CID */
    ConnectMessage = (PLPCP_CONNECTION_MESSAGE)(Message + 1);
    Message->Request.ClientId = Thread->Cid;

    /* Check if we have a client view */
    if (ClientView)
    {
        /* Set the view size */
        Message->Request.ClientViewSize = ClientView->ViewSize;

        /* Copy the client view and clear the server view */
        RtlCopyMemory(&ConnectMessage->ClientView,
                      ClientView,
                      sizeof(PORT_VIEW));
        RtlZeroMemory(&ConnectMessage->ServerView, sizeof(REMOTE_PORT_VIEW));
    }
    else
    {
        /* Set the size to 0 and clear the connect message */
        Message->Request.ClientViewSize = 0;
        RtlZeroMemory(ConnectMessage, sizeof(LPCP_CONNECTION_MESSAGE));
    }

    /* Set the section and client port. Port is NULL for now */
    ConnectMessage->ClientPort = NULL;
    ConnectMessage->SectionToMap = SectionToMap;

    /* Set the data for the connection request message */
    Message->Request.u1.s1.DataLength = (CSHORT)ConnectionInfoLength + 
                                         sizeof(LPCP_CONNECTION_MESSAGE);
    Message->Request.u1.s1.TotalLength = sizeof(LPCP_MESSAGE) +
                                         Message->Request.u1.s1.DataLength;
    Message->Request.u2.s2.Type = LPC_CONNECTION_REQUEST;

    /* Check if we have connection information */
    if (ConnectionInformation)
    {
        /* Copy it in */
        RtlCopyMemory(ConnectMessage + 1,
                      ConnectionInformation,
                      ConnectionInfoLength);
    }

    /* Acquire the port lock */
    KeAcquireGuardedMutex(&LpcpLock);

    /* Check if someone already deleted the port name */
    if (Port->Flags & LPCP_NAME_DELETED)
    {
        /* Fail the request */
        Status = STATUS_OBJECT_NAME_NOT_FOUND;
    }
    else
    {
        /* Associate no thread yet */
        Message->RepliedToThread = NULL;

        /* Generate the Message ID and set it */
        Message->Request.MessageId =  LpcpNextMessageId++;
        if (!LpcpNextMessageId) LpcpNextMessageId = 1;
        Thread->LpcReplyMessageId = Message->Request.MessageId;

        /* Insert the message into the queue and thread chain */
        InsertTailList(&Port->MsgQueue.ReceiveHead, &Message->Entry);
        InsertTailList(&Port->LpcReplyChainHead, &Thread->LpcReplyChain);
        Thread->LpcReplyMessage = Message;

        /* Now we can finally reference the client port and link it*/
        ObReferenceObject(ClientPort);
        ConnectMessage->ClientPort = ClientPort;

        /* Enter a critical region */
        KeEnterCriticalRegion();
    }

    /* Add another reference to the port */
    ObReferenceObject(Port);

    /* Release the lock */
    KeReleaseGuardedMutex(&LpcpLock);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        LPCTRACE(LPC_CONNECT_DEBUG,
                 "Messages: %p/%p. Ports: %p/%p. Status: %lx\n",
                 Message,
                 ConnectMessage,
                 Port,
                 ClientPort,
                 Status);

        /* If this is a waitable port, set the event */
        if (Port->Flags & LPCP_WAITABLE_PORT) KeSetEvent(&Port->WaitEvent,
                                                         1,
                                                         FALSE);

        /* Release the queue semaphore and leave the critical region */
        LpcpCompleteWait(Port->MsgQueue.Semaphore);
        KeLeaveCriticalRegion();

        /* Now wait for a reply */
        LpcpConnectWait(&Thread->LpcReplySemaphore, PreviousMode);
    }

    /* Check for failure */
    if (!NT_SUCCESS(Status)) goto Cleanup;

    /* Free the connection message */
    SectionToMap = LpcpFreeConMsg(&Message, &ConnectMessage, Thread);

    /* Check if we got a message back */
    if (Message)
    {
        /* Check for new return length */
        if ((Message->Request.u1.s1.DataLength -
             sizeof(LPCP_CONNECTION_MESSAGE)) < ConnectionInfoLength)
        {
            /* Set new normalized connection length */
            ConnectionInfoLength = Message->Request.u1.s1.DataLength -
                                   sizeof(LPCP_CONNECTION_MESSAGE);
        }

        /* Check if we had connection information */
        if (ConnectionInformation)
        {
            /* Check if we had a length pointer */
            if (ConnectionInformationLength)
            {
                /* Return the length */
                *ConnectionInformationLength = ConnectionInfoLength;
            }

            /* Return the connection information */
            RtlCopyMemory(ConnectionInformation,
                          ConnectMessage + 1,
                          ConnectionInfoLength );
        }

        /* Make sure we had a connected port */
        if (ClientPort->ConnectedPort)
        {
            /* Get the message length before the port might get killed */
            PortMessageLength = Port->MaxMessageLength;

            /* Insert the client port */
            Status = ObInsertObject(ClientPort,
                                    NULL,
                                    PORT_ALL_ACCESS,
                                    0,
                                    (PVOID *)NULL,
                                    &Handle);
            if (NT_SUCCESS(Status))
            {
                /* Return the handle */
                *PortHandle = Handle;
                LPCTRACE(LPC_CONNECT_DEBUG,
                         "Handle: %lx. Length: %lx\n",
                         Handle,
                         PortMessageLength);

                /* Check if maximum length was requested */
                if (MaxMessageLength) *MaxMessageLength = PortMessageLength;

                /* Check if we had a client view */
                if (ClientView)
                {
                    /* Copy it back */
                    RtlCopyMemory(ClientView,
                                  &ConnectMessage->ClientView,
                                  sizeof(PORT_VIEW));
                }

                /* Check if we had a server view */
                if (ServerView)
                {
                    /* Copy it back */
                    RtlCopyMemory(ServerView,
                                  &ConnectMessage->ServerView,
                                  sizeof(REMOTE_PORT_VIEW));
                }
            }
        }
        else
        {
            /* No connection port, we failed */
            if (SectionToMap) ObDereferenceObject(SectionToMap);

            /* Acquire the lock */
            KeAcquireGuardedMutex(&LpcpLock);

            /* Check if it's because the name got deleted */
            if (!(ClientPort->ConnectionPort) ||
                (Port->Flags & LPCP_NAME_DELETED))
            {
                /* Set the correct status */
                Status = STATUS_OBJECT_NAME_NOT_FOUND;
            }
            else
            {
                /* Otherwise, the caller refused us */
                Status = STATUS_PORT_CONNECTION_REFUSED;
            }

            /* Release the lock */
            KeReleaseGuardedMutex(&LpcpLock);

            /* Kill the port */
            ObDereferenceObject(ClientPort);
        }

        /* Free the message */
        LpcpFreeToPortZone(Message, 0);
    }
    else
    {
        /* No reply message, fail */
        if (SectionToMap) ObDereferenceObject(SectionToMap);
        ObDereferenceObject(ClientPort);
        Status = STATUS_PORT_CONNECTION_REFUSED;
    }

    /* Return status */
    ObDereferenceObject(Port);
    return Status;

Cleanup:
    /* We failed, free the message */
    SectionToMap = LpcpFreeConMsg(&Message, &ConnectMessage, Thread);

    /* Check if the semaphore got signaled */
    if (KeReadStateSemaphore(&Thread->LpcReplySemaphore))
    {
        /* Wait on it */
        KeWaitForSingleObject(&Thread->LpcReplySemaphore,
                              WrExecutive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    /* Check if we had a message and free it */
    if (Message) LpcpFreeToPortZone(Message, 0);

    /* Dereference other objects */
    if (SectionToMap) ObDereferenceObject(SectionToMap);
    ObDereferenceObject(ClientPort);

    /* Return status */
    ObDereferenceObject(Port);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtConnectPort(OUT PHANDLE PortHandle,
              IN PUNICODE_STRING PortName,
              IN PSECURITY_QUALITY_OF_SERVICE Qos,
              IN PPORT_VIEW ClientView,
              IN PREMOTE_PORT_VIEW ServerView,
              OUT PULONG MaxMessageLength,
              IN PVOID ConnectionInformation,
              OUT PULONG ConnectionInformationLength)
{
    /* Call the newer API */
    return NtSecureConnectPort(PortHandle,
                               PortName,
                               Qos,
                               ClientView,
                               NULL,
                               ServerView,
                               MaxMessageLength,
                               ConnectionInformation,
                               ConnectionInformationLength);
}

/* EOF */
