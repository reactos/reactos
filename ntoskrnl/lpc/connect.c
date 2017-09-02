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
                    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
                    IN OUT PPORT_VIEW ClientView OPTIONAL,
                    IN PSID ServerSid OPTIONAL,
                    IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
                    OUT PULONG MaxMessageLength OPTIONAL,
                    IN OUT PVOID ConnectionInformation OPTIONAL,
                    IN OUT PULONG ConnectionInformationLength OPTIONAL)
{
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PETHREAD Thread = PsGetCurrentThread();
    SECURITY_QUALITY_OF_SERVICE CapturedQos;
    PORT_VIEW CapturedClientView;
    PSID CapturedServerSid;
    ULONG ConnectionInfoLength = 0;
    PLPCP_PORT_OBJECT Port, ClientPort;
    PLPCP_MESSAGE Message;
    PLPCP_CONNECTION_MESSAGE ConnectMessage;
    ULONG PortMessageLength;
    HANDLE Handle;
    PVOID SectionToMap;
    LARGE_INTEGER SectionOffset;
    PTOKEN Token;
    PTOKEN_USER TokenUserInfo;

    PAGED_CODE();
    LPCTRACE(LPC_CONNECT_DEBUG,
             "Name: %wZ. SecurityQos: %p. Views: %p/%p. Sid: %p\n",
             PortName,
             SecurityQos,
             ClientView,
             ServerView,
             ServerSid);

    /* Check if the call comes from user mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH for probing the parameters */
        _SEH2_TRY
        {
            /* Probe the PortHandle */
            ProbeForWriteHandle(PortHandle);

            /* Probe and capture the QoS */
            ProbeForRead(SecurityQos, sizeof(*SecurityQos), sizeof(ULONG));
            CapturedQos = *(volatile SECURITY_QUALITY_OF_SERVICE*)SecurityQos;
            /* NOTE: Do not care about CapturedQos.Length */

            /* The following parameters are optional */

            /* Capture the client view */
            if (ClientView)
            {
                ProbeForWrite(ClientView, sizeof(*ClientView), sizeof(ULONG));
                CapturedClientView = *(volatile PORT_VIEW*)ClientView;

                /* Validate the size of the client view */
                if (CapturedClientView.Length != sizeof(CapturedClientView))
                {
                    /* Invalid size */
                    _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
                }

            }

            /* Capture the server view */
            if (ServerView)
            {
                ProbeForWrite(ServerView, sizeof(*ServerView), sizeof(ULONG));

                /* Validate the size of the server view */
                if (((volatile REMOTE_PORT_VIEW*)ServerView)->Length != sizeof(*ServerView))
                {
                    /* Invalid size */
                    _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
                }
            }

            if (MaxMessageLength)
                ProbeForWriteUlong(MaxMessageLength);

            /* Capture connection information length */
            if (ConnectionInformationLength)
            {
                ProbeForWriteUlong(ConnectionInformationLength);
                ConnectionInfoLength = *(volatile ULONG*)ConnectionInformationLength;
            }

            /* Probe the ConnectionInformation */
            if (ConnectionInformation)
                ProbeForWrite(ConnectionInformation, ConnectionInfoLength, sizeof(ULONG));

            CapturedServerSid = ServerSid;
            if (ServerSid != NULL)
            {
                /* Capture it */
                Status = SepCaptureSid(ServerSid,
                                       PreviousMode,
                                       PagedPool,
                                       TRUE,
                                       &CapturedServerSid);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("Failed to capture ServerSid!\n");
                    _SEH2_YIELD(return Status);
                }
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* There was an exception, return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        CapturedQos = *SecurityQos;
        /* NOTE: Do not care about CapturedQos.Length */

        /* The following parameters are optional */

        /* Capture the client view */
        if (ClientView)
        {
            /* Validate the size of the client view */
            if (ClientView->Length != sizeof(*ClientView))
            {
                /* Invalid size */
                return STATUS_INVALID_PARAMETER;
            }
            CapturedClientView = *ClientView;
        }

        /* Capture the server view */
        if (ServerView)
        {
            /* Validate the size of the server view */
            if (ServerView->Length != sizeof(*ServerView))
            {
                /* Invalid size */
                return STATUS_INVALID_PARAMETER;
            }
        }

        /* Capture connection information length */
        if (ConnectionInformationLength)
            ConnectionInfoLength = *ConnectionInformationLength;

        CapturedServerSid = ServerSid;
    }

    /* Get the port */
    Status = ObReferenceObjectByName(PortName,
                                     0,
                                     NULL,
                                     PORT_CONNECT,
                                     LpcPortObjectType,
                                     PreviousMode,
                                     NULL,
                                     (PVOID*)&Port);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference port '%wZ': 0x%lx\n", PortName, Status);

        if (CapturedServerSid != ServerSid)
            SepReleaseSid(CapturedServerSid, PreviousMode, TRUE);

        return Status;
    }

    /* This has to be a connection port */
    if ((Port->Flags & LPCP_PORT_TYPE_MASK) != LPCP_CONNECTION_PORT)
    {
        /* It isn't, so fail */
        ObDereferenceObject(Port);

        if (CapturedServerSid != ServerSid)
            SepReleaseSid(CapturedServerSid, PreviousMode, TRUE);

        return STATUS_INVALID_PORT_HANDLE;
    }

    /* Check if we have a (captured) SID */
    if (ServerSid)
    {
        /* Make sure that we have a server */
        if (Port->ServerProcess)
        {
            /* Get its token and query user information */
            Token = PsReferencePrimaryToken(Port->ServerProcess);
            Status = SeQueryInformationToken(Token, TokenUser, (PVOID*)&TokenUserInfo);
            PsDereferencePrimaryToken(Token);

            /* Check for success */
            if (NT_SUCCESS(Status))
            {
                /* Compare the SIDs */
                if (!RtlEqualSid(CapturedServerSid, TokenUserInfo->User.Sid))
                {
                    /* Fail */
                    Status = STATUS_SERVER_SID_MISMATCH;
                }

                /* Free token information */
                ExFreePoolWithTag(TokenUserInfo, TAG_SE);
            }
        }
        else
        {
            /* Invalid SID */
            Status = STATUS_SERVER_SID_MISMATCH;
        }

        /* Finally release the captured SID, we don't need it anymore */
        if (CapturedServerSid != ServerSid)
            SepReleaseSid(CapturedServerSid, PreviousMode, TRUE);

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
                            (PVOID*)&ClientPort);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, dereference the server port and return */
        ObDereferenceObject(Port);
        return Status;
    }

    /*
     * Setup the client port -- From now on, dereferencing the client port
     * will automatically dereference the connection port too.
     */
    RtlZeroMemory(ClientPort, sizeof(LPCP_PORT_OBJECT));
    ClientPort->Flags = LPCP_CLIENT_PORT;
    ClientPort->ConnectionPort = Port;
    ClientPort->MaxMessageLength = Port->MaxMessageLength;
    ClientPort->SecurityQos = CapturedQos;
    InitializeListHead(&ClientPort->LpcReplyChainHead);
    InitializeListHead(&ClientPort->LpcDataInfoChainHead);

    /* Check if we have dynamic security */
    if (CapturedQos.ContextTrackingMode == SECURITY_DYNAMIC_TRACKING)
    {
        /* Remember that */
        ClientPort->Flags |= LPCP_SECURITY_DYNAMIC;
    }
    else
    {
        /* Create our own client security */
        Status = SeCreateClientSecurity(Thread,
                                        &CapturedQos,
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
        Status = ObReferenceObjectByHandle(CapturedClientView.SectionHandle,
                                           SECTION_MAP_READ |
                                           SECTION_MAP_WRITE,
                                           MmSectionObjectType,
                                           PreviousMode,
                                           (PVOID*)&SectionToMap,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(ClientPort);
            return Status;
        }

        /* Set the section offset */
        SectionOffset.QuadPart = CapturedClientView.SectionOffset;

        /* Map it */
        Status = MmMapViewOfSection(SectionToMap,
                                    PsGetCurrentProcess(),
                                    &ClientPort->ClientSectionBase,
                                    0,
                                    0,
                                    &SectionOffset,
                                    &CapturedClientView.ViewSize,
                                    ViewUnmap,
                                    0,
                                    PAGE_READWRITE);

        /* Update the offset */
        CapturedClientView.SectionOffset = SectionOffset.LowPart;

        /* Check for failure */
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ObDereferenceObject(SectionToMap);
            ObDereferenceObject(ClientPort);
            return Status;
        }

        /* Update the base */
        CapturedClientView.ViewBase = ClientPort->ClientSectionBase;

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
        Message->Request.ClientViewSize = CapturedClientView.ViewSize;

        /* Copy the client view and clear the server view */
        RtlCopyMemory(&ConnectMessage->ClientView,
                      &CapturedClientView,
                      sizeof(CapturedClientView));
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
        _SEH2_TRY
        {
            /* Copy it in */
            RtlCopyMemory(ConnectMessage + 1,
                          ConnectionInformation,
                          ConnectionInfoLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Cleanup and return the exception code */

            /* Free the message we have */
            LpcpFreeToPortZone(Message, 0);

            /* Dereference other objects */
            if (SectionToMap) ObDereferenceObject(SectionToMap);
            ObDereferenceObject(ClientPort);

            /* Return status */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Reset the status code */
    Status = STATUS_SUCCESS;

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

        /* Now we can finally reference the client port and link it */
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
        if (Port->Flags & LPCP_WAITABLE_PORT)
            KeSetEvent(&Port->WaitEvent, 1, FALSE);

        /* Release the queue semaphore and leave the critical region */
        LpcpCompleteWait(Port->MsgQueue.Semaphore);
        KeLeaveCriticalRegion();

        /* Now wait for a reply and set 'Status' */
        LpcpConnectWait(&Thread->LpcReplySemaphore, PreviousMode);
    }

    /* Now, always free the connection message */
    SectionToMap = LpcpFreeConMsg(&Message, &ConnectMessage, Thread);

    /* Check for failure */
    if (!NT_SUCCESS(Status))
    {
        /* Check if the semaphore got signaled in the meantime */
        if (KeReadStateSemaphore(&Thread->LpcReplySemaphore))
        {
            /* Wait on it */
            KeWaitForSingleObject(&Thread->LpcReplySemaphore,
                                  WrExecutive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        goto Failure;
    }

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

        /* Check if the caller had connection information */
        if (ConnectionInformation)
        {
            _SEH2_TRY
            {
                /* Return the connection information length if needed */
                if (ConnectionInformationLength)
                    *ConnectionInformationLength = ConnectionInfoLength;

                /* Return the connection information */
                RtlCopyMemory(ConnectionInformation,
                              ConnectMessage + 1,
                              ConnectionInfoLength);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Cleanup and return the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(goto Failure);
            }
            _SEH2_END;
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
                                    NULL,
                                    &Handle);
            if (NT_SUCCESS(Status))
            {
                LPCTRACE(LPC_CONNECT_DEBUG,
                         "Handle: %p. Length: %lx\n",
                         Handle,
                         PortMessageLength);

                _SEH2_TRY
                {
                    /* Return the handle */
                    *PortHandle = Handle;

                    /* Check if maximum length was requested */
                    if (MaxMessageLength)
                        *MaxMessageLength = PortMessageLength;

                    /* Check if we had a client view */
                    if (ClientView)
                    {
                        /* Copy it back */
                        RtlCopyMemory(ClientView,
                                      &ConnectMessage->ClientView,
                                      sizeof(*ClientView));
                    }

                    /* Check if we had a server view */
                    if (ServerView)
                    {
                        /* Copy it back */
                        RtlCopyMemory(ServerView,
                                      &ConnectMessage->ServerView,
                                      sizeof(*ServerView));
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* An exception happened, close the opened handle */
                    ObCloseHandle(Handle, PreviousMode);
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
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
        Status = STATUS_PORT_CONNECTION_REFUSED;
        goto Failure;
    }

    ObDereferenceObject(Port);

    /* Return status */
    return Status;

Failure:
    /* Check if we had a message and free it */
    if (Message) LpcpFreeToPortZone(Message, 0);

    /* Dereference other objects */
    if (SectionToMap) ObDereferenceObject(SectionToMap);
    ObDereferenceObject(ClientPort);
    ObDereferenceObject(Port);

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtConnectPort(OUT PHANDLE PortHandle,
              IN PUNICODE_STRING PortName,
              IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
              IN OUT PPORT_VIEW ClientView OPTIONAL,
              IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
              OUT PULONG MaxMessageLength OPTIONAL,
              IN OUT PVOID ConnectionInformation OPTIONAL,
              IN OUT PULONG ConnectionInformationLength OPTIONAL)
{
    /* Call the newer API */
    return NtSecureConnectPort(PortHandle,
                               PortName,
                               SecurityQos,
                               ClientView,
                               NULL,
                               ServerView,
                               MaxMessageLength,
                               ConnectionInformation,
                               ConnectionInformationLength);
}

/* EOF */
