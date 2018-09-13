/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcconn.c

Abstract:

    Local Inter-Process Communication (LPC) connection system services.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

//
//  Local procedure prototypes
//

PVOID
LpcpFreeConMsg(
    IN PLPCP_MESSAGE *Msg,
    PLPCP_CONNECTION_MESSAGE *ConnectMsg,
    IN PETHREAD CurrentThread
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtConnectPort)
#pragma alloc_text(PAGE,LpcpFreeConMsg)
#endif


NTSYSAPI
NTSTATUS
NTAPI
NtConnectPort (
    OUT PHANDLE PortHandle,
    IN PUNICODE_STRING PortName,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN OUT PPORT_VIEW ClientView OPTIONAL,
    IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
    OUT PULONG MaxMessageLength OPTIONAL,
    IN OUT PVOID ConnectionInformation OPTIONAL,
    IN OUT PULONG ConnectionInformationLength OPTIONAL
    )

/*++

Routine Description:

    See NtSecureConnectPort

Arguments:

    See NtSecureConnectPort

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    return NtSecureConnectPort( PortHandle,
                                PortName,
                                SecurityQos,
                                ClientView,
                                NULL,
                                ServerView,
                                MaxMessageLength,
                                ConnectionInformation,
                                ConnectionInformationLength );
}


NTSTATUS
NtSecureConnectPort (
    OUT PHANDLE PortHandle,
    IN PUNICODE_STRING PortName,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos,
    IN OUT PPORT_VIEW ClientView OPTIONAL,
    IN PSID RequiredServerSid,
    IN OUT PREMOTE_PORT_VIEW ServerView OPTIONAL,
    OUT PULONG MaxMessageLength OPTIONAL,
    IN OUT PVOID ConnectionInformation OPTIONAL,
    IN OUT PULONG ConnectionInformationLength OPTIONAL
    )

/*++

Routine Description:

    A client process can connect to a server process by name using the
    NtConnectPort service.

    The PortName parameter specifies the name of the server port to
    connect to.  It must correspond to an object name specified on a
    call to NtCreatePort.  The service sends a connection request to the
    server thread that is listening for them with the NtListenPort
    service.  The client thread then blocks until a server thread
    receives the connection request and responds with a call to the
    NtCompleteConnectPort service.  The server thread receives the ID of
    the client thread, along with any information passed via the
    ConnectionInformation parameter.  The server thread then decides to
    either accept or reject the connection request.

    The server communicates the acceptance or rejection with the
    NtCompleteConnectPort service.  The server can pass back data to the
    client about the acceptance or rejection via the
    ConnectionInformation data block.

    If the server accepts the connection request, then the client
    receives a communication port object in the location pointed to by
    the PortHandle parameter.  This object handle has no name associated
    with it and is private to the client process (i.e.  it cannot be
    inherited by a child process).  The client uses the handle to send
    and receive messages to/from the server process using the
    NtRequestWaitReplyPort service.

    If the ClientView parameter was specified, then the section handle
    is examined.  If it is a valid section handle, then the portion of
    the section described by the SectionOffset and ViewSize fields will
    be mapped into both the client and server process' address spaces.
    The address in client address space will be returned in the ViewBase
    field.  The address in the server address space will be returned in
    the ViewRemoteBase field.  The actual offset and size used to map
    the section will be returned in the SectionOffset and ViewSize
    fields.

    If the server rejects the connection request, then no communication
    port object handle is returned, and the return status indicates an
    error occurred.  The server may optionally return information in the
    ConnectionInformation data block giving the reason the connection
    requests was rejected.

    If the PortName does not exist, or the client process does not have
    sufficient access rights then the returned status will indicate that
    the port was not found.

Arguments:

    PortHandle - A pointer to a variable that will receive the client
        communication port object handle value.

    PortName - A pointer to a port name string.  The form of the name
        is [\name...\name]\port_name.

    SecurityQos - A pointer to security quality of service information
        to be applied to the server on the client's behalf.

    ClientView - An optional pointer to a structure that specifies the
        section that all client threads will use to send messages to the
        server.

    ClientView Structure

        ULONG Length - Specifies the size of this data structure in
            bytes.

        HANDLE SectionHandle - Specifies an open handle to a section
            object.

        ULONG SectionOffset - Specifies a field that will receive the
            actual offset, in bytes, from the start of the section.  The
            initial value of this parameter specifies the byte offset
            within the section that the client's view is based.  The
            value is rounded down to the next host page size boundary.

        ULONG ViewSize - Specifies a field that will receive the
            actual size, in bytes, of the view.  If the value of this
            parameter is zero, then the client's view of the section
            will be mapped starting at the specified section offset and
            continuing to the end of the section.  Otherwise, the
            initial value of this parameter specifies the size, in
            bytes, of the client's view and is rounded up to the next
            host page size boundary.

        PVOID ViewBase - Specifies a field that will receive the base
            address of the section in the client's address space.

        PVOID ViewRemoteBase - Specifies a field that will receive
            the base address of the client's section in the server's
            address space.  Used to generate pointers that are
            meaningful to the server.

    RequiredServerSid - Optionally specifies the SID that we expect the
        server side of the port to possess.  If not specified then we'll
        connect to any server SID.

    ServerView - An optional pointer to a structure that will receive
        information about the server process' view in the client's
        address space.  The client process can use this information
        to validate pointers it receives from the server process.

        ServerView Structure

        ULONG Length - Specifies the size of this data structure in
            bytes.

        PVOID ViewBase - Specifies a field that will receive the base
            address of the server's section in the client's address
            space.

        ULONG ViewSize - Specifies a field that will receive the
            size, in bytes, of the server's view in the client's address
            space.  If this field is zero, then server has no view in
            the client's address space.

    MaxMessageLength - An optional pointer to a variable that will
        receive maximum length of messages that can be sent to the
        server.  The value of this parameter will not exceed
        MAX_PORTMSG_LENGTH bytes.

    ConnectionInformation - An optional pointer to uninterpreted data.
        This data is intended for clients to pass package, version and
        protocol identification information to the server to allow the
        server to determine if it can satisify the client before
        accepting the connection.  Upon return to the client, the
        ConnectionInformation data block contains any information passed
        back from the server by its call to the NtCompleteConnectPort
        service.  The output data overwrites the input data.

    ConnectionInformationLength - Pointer to the length of the
        ConnectionInformation data block.  The output value is the
        length of the data stored in the ConnectionInformation data
        block by the server's call to the NtCompleteConnectPort
        service.  This parameter is OPTIONAL only if the
        ConnectionInformation parameter is null, otherwise it is
        required.

Return Value:

    NTSTATUS - An appropriate status value.

--*/

{
    PLPCP_PORT_OBJECT ConnectionPort;
    PLPCP_PORT_OBJECT ClientPort;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    ULONG ConnectionInfoLength;
    PVOID SectionToMap;
    PLPCP_MESSAGE Msg;
    PLPCP_CONNECTION_MESSAGE ConnectMsg;
    PETHREAD CurrentThread = PsGetCurrentThread();
    LARGE_INTEGER SectionOffset;
    PORT_VIEW CapturedClientView;
    SECURITY_QUALITY_OF_SERVICE CapturedQos;
    PSID CapturedRequiredServerSid;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe input and output arguments if
    //  necessary.
    //

    PreviousMode = KeGetPreviousMode();
    ConnectionInfoLength = 0;

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle( PortHandle );

            if (ARGUMENT_PRESENT( ClientView )) {

                CapturedClientView = ProbeAndReadStructure( ClientView, PORT_VIEW );

                if (CapturedClientView.Length != sizeof( *ClientView )) {

                    return( STATUS_INVALID_PARAMETER );
                }

                ProbeForWrite( ClientView,
                               sizeof( *ClientView ),
                               sizeof( ULONG ));
            }

            if (ARGUMENT_PRESENT( ServerView )) {

                if (ProbeAndReadUlong( &ServerView->Length ) != sizeof( *ServerView )) {

                    return( STATUS_INVALID_PARAMETER );
                }

                ProbeForWrite( ServerView,
                               sizeof( *ServerView ),
                               sizeof( ULONG ));
            }

            if (ARGUMENT_PRESENT( MaxMessageLength )) {

                ProbeForWriteUlong( MaxMessageLength );
            }

            if (ARGUMENT_PRESENT( ConnectionInformationLength )) {

                ConnectionInfoLength = ProbeAndReadUlong( ConnectionInformationLength );
                ProbeForWriteUlong( ConnectionInformationLength );
            }

            if (ARGUMENT_PRESENT( ConnectionInformation )) {

                ProbeForWrite( ConnectionInformation,
                               ConnectionInfoLength,
                               sizeof( UCHAR ));
            }

            CapturedQos = ProbeAndReadStructure( SecurityQos, SECURITY_QUALITY_OF_SERVICE );

            CapturedRequiredServerSid = RequiredServerSid;

            if (ARGUMENT_PRESENT( RequiredServerSid )) {

                Status = SeCaptureSid( RequiredServerSid,
                                       PreviousMode,
                                       NULL,
                                       0,
                                       PagedPool,
                                       TRUE,
                                       &CapturedRequiredServerSid );

                if (!NT_SUCCESS(Status)) {

                    return Status;
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }

    //
    //  Otherwise this is a kernel mode operation
    //

    } else {

        if (ARGUMENT_PRESENT( ClientView )) {

            if (ClientView->Length != sizeof( *ClientView )) {

                return( STATUS_INVALID_PARAMETER );
            }

            CapturedClientView = *ClientView;
        }

        if (ARGUMENT_PRESENT( ServerView )) {

            if (ServerView->Length != sizeof( *ServerView )) {

                return( STATUS_INVALID_PARAMETER );
            }
        }

        if (ARGUMENT_PRESENT( ConnectionInformationLength )) {

            ConnectionInfoLength = *ConnectionInformationLength;
        }

        CapturedQos = *SecurityQos;
        CapturedRequiredServerSid = RequiredServerSid;
    }

    //
    //  Reference the connection port object by name.  Return status if
    //  unsuccessful.
    //

    Status = ObReferenceObjectByName( PortName,
                                      0,
                                      NULL,
                                      PORT_CONNECT,
                                      LpcPortObjectType,
                                      PreviousMode,
                                      NULL,
                                      (PVOID *)&ConnectionPort );

    //
    //  If the port type object didn't work then try for a waitable port type
    //  object
    //

    if ( Status == STATUS_OBJECT_TYPE_MISMATCH ) {

        Status = ObReferenceObjectByName( PortName,
                                          0,
                                          NULL,
                                          PORT_CONNECT,
                                          LpcWaitablePortObjectType,
                                          PreviousMode,
                                          NULL,
                                          (PVOID *)&ConnectionPort );
    }

    //
    //  We can't locate the name so release the sid if we captured one and
    //  return error status back to our caller
    //

    if (!NT_SUCCESS( Status )) {

        if (CapturedRequiredServerSid != RequiredServerSid) {

            SeReleaseSid( CapturedRequiredServerSid, PreviousMode, TRUE);
        }

        return Status;
    }

    LpcpTrace(("Connecting to port %wZ\n", PortName ));

    //
    //  Error if user didn't give us a server communication port
    //

    if ((ConnectionPort->Flags & PORT_TYPE) != SERVER_CONNECTION_PORT) {

        ObDereferenceObject( ConnectionPort );

        if (CapturedRequiredServerSid != RequiredServerSid) {

            SeReleaseSid( CapturedRequiredServerSid, PreviousMode, TRUE);
        }

        return STATUS_INVALID_PORT_HANDLE;
    }

    //
    //  If this is NtSecureConnectPort, validated the required SID against
    //  the SID of the server process.  Fail if not equal.
    //

    if (ARGUMENT_PRESENT( RequiredServerSid )) {

        PTOKEN_USER TokenInfo;

        if (ConnectionPort->ServerProcess != NULL) {

            PACCESS_TOKEN Token ;

            Token = PsReferencePrimaryToken( ConnectionPort->ServerProcess );

    
            Status = SeQueryInformationToken( Token,
                                              TokenUser,
                                              &TokenInfo );
            
            PsDereferencePrimaryToken( Token );

            if (NT_SUCCESS( Status )) {

                if (!RtlEqualSid( CapturedRequiredServerSid, TokenInfo->User.Sid )) {

                    Status = STATUS_SERVER_SID_MISMATCH;
                }

                ExFreePool( TokenInfo );
            }

        } else {

            Status = STATUS_SERVER_SID_MISMATCH;
        }

        //
        //  We are all done with the required server sid if specified so
        //  now release one if we had to capture it
        //

        if (CapturedRequiredServerSid != RequiredServerSid) {

            SeReleaseSid( CapturedRequiredServerSid, PreviousMode, TRUE);
        }

        //
        //  If the se information token query didn't work then return the
        //  error to our caller
        //

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( ConnectionPort );

            return Status;
        }
    }

    //
    //  Allocate and initialize a client communication port object.  Give
    //  the port a request message queue for lost reply datagrams.  If
    //  unable to initialize the port, then deference the port object which
    //  will cause it to be deleted and return the system service status.
    //

    Status = ObCreateObject( PreviousMode,
                             LpcPortObjectType,
                             NULL,
                             PreviousMode,
                             NULL,
                             sizeof( LPCP_PORT_OBJECT ),
                             0,
                             0,
                             (PVOID *)&ClientPort );

    if (!NT_SUCCESS( Status )) {

        ObDereferenceObject( ConnectionPort );

        return Status;
    }

    //
    //  Note, that from here on, none of the error paths dereference the
    //  connection port pointer, just the newly created client port pointer.
    //  The port delete routine will get called when the client port is
    //  deleted and it will dereference the connection port pointer stored
    //  in the client port object.
    //

    //
    //  Initialize the client port object to zeros and then fill in the
    //  fields.
    //

    RtlZeroMemory( ClientPort, sizeof( LPCP_PORT_OBJECT ));

    ClientPort->Length = sizeof( LPCP_PORT_OBJECT );
    ClientPort->Flags = CLIENT_COMMUNICATION_PORT;
    ClientPort->ConnectionPort = ConnectionPort;
    ClientPort->MaxMessageLength = ConnectionPort->MaxMessageLength;
    ClientPort->SecurityQos = CapturedQos;

    InitializeListHead( &ClientPort->LpcReplyChainHead );
    InitializeListHead( &ClientPort->LpcDataInfoChainHead );

    //
    //  Set the security tracking mode, and initialize the client security
    //  context if it is static tracking.
    //

    if (CapturedQos.ContextTrackingMode == SECURITY_DYNAMIC_TRACKING) {

        ClientPort->Flags |= PORT_DYNAMIC_SECURITY;

    } else {

        Status = SeCreateClientSecurity( CurrentThread,
                                         &CapturedQos,
                                         FALSE,
                                         &ClientPort->StaticSecurity );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( ClientPort );

            return Status;
        }
    }

    //
    //  Client communication ports get a request message queue for lost
    //  replies.
    //

    Status = LpcpInitializePortQueue( ClientPort );

    if (!NT_SUCCESS( Status )) {

        ObDereferenceObject( ClientPort );

        return Status;
    }

    //
    //  If client has allocated a port memory section, then map a view of
    //  that section into the client's address space.  Also reference the
    //  section object so we can pass a pointer to the section object in
    //  connection request message.  If the server accepts the connection,
    //  then it will map a corresponding view of the section in the server's
    //  address space, using the referenced pointer passed in the connection
    //  request message.
    //

    if (ARGUMENT_PRESENT( ClientView )) {

        Status = ObReferenceObjectByHandle( CapturedClientView.SectionHandle,
                                            SECTION_MAP_READ |
                                            SECTION_MAP_WRITE,
                                            MmSectionObjectType,
                                            PreviousMode,
                                            (PVOID *)&SectionToMap,
                                            NULL );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( ClientPort );

            return Status;
        }

        SectionOffset.LowPart = CapturedClientView.SectionOffset,
        SectionOffset.HighPart = 0;

        //
        //  Now map a view of the section using the reference we just captured
        //  and not the section handle itself, because the handle may have changed
        //

        Status = MmMapViewOfSection( SectionToMap,
                                     PsGetCurrentProcess(),
                                     &ClientPort->ClientSectionBase,
                                     0,
                                     0,
                                     &SectionOffset,
                                     &CapturedClientView.ViewSize,
                                     ViewUnmap,
                                     0,
                                     PAGE_READWRITE );

        CapturedClientView.SectionOffset = SectionOffset.LowPart;

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( SectionToMap );
            ObDereferenceObject( ClientPort );

            return Status;
        }

        CapturedClientView.ViewBase = ClientPort->ClientSectionBase;

    } else {

        SectionToMap = NULL;
    }

    //
    //  Adjust the size of the connection info length that the client supplied
    //  to be the no longer than one the connection port will accept
    //

    if (ConnectionInfoLength > ConnectionPort->MaxConnectionInfoLength) {

        ConnectionInfoLength = ConnectionPort->MaxConnectionInfoLength;
    }

    //
    //  At this point the client port is all setup and now we have to
    //  allocate a request connection message for the server and send it off
    //
    //  Lock, allocate and then unlock the zone.  We are allocation a
    //  connection request message.  It holds the LPCP message, the LPCP
    //  connection message, and the user supplied connection information
    //

    LpcpAcquireLpcpLock();

    Msg = LpcpAllocateFromPortZone( sizeof( *Msg ) +
                                    sizeof( *ConnectMsg ) +
                                    ConnectionInfoLength );

    LpcpReleaseLpcpLock();

    //
    //  If we didn't get memory for the message then tell our caller we failed
    //

    if (Msg == NULL) {

        if (SectionToMap != NULL) {

            ObDereferenceObject( SectionToMap );
        }

        ObDereferenceObject( ClientPort );

        return STATUS_NO_MEMORY;
    }

    //
    //  Msg points to the LPCP message, followed by ConnectMsg which points to
    //  the LPCP connection message, followed by client specified information.
    //  We'll now fill it all in.
    //

    ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

    //
    //  This thread originated the message
    //

    Msg->Request.ClientId = CurrentThread->Cid;

    //
    //  If we have a client view then copy over the client view information
    //  otherwise we'll zero out all of the view information
    //

    if (ARGUMENT_PRESENT( ClientView )) {

        Msg->Request.ClientViewSize = CapturedClientView.ViewSize;

        RtlMoveMemory( &ConnectMsg->ClientView,
                       &CapturedClientView,
                       sizeof( CapturedClientView ));

        RtlZeroMemory( &ConnectMsg->ServerView, sizeof( ConnectMsg->ServerView ));

    } else {

        Msg->Request.ClientViewSize = 0;
        RtlZeroMemory( ConnectMsg, sizeof( *ConnectMsg ));
    }

    ConnectMsg->ClientPort = NULL;              // Set below
    ConnectMsg->SectionToMap = SectionToMap;

    //
    //  The data length is everything after the port message within the lpcp
    //  message.  In other words the connection message and the user supplied
    //  information
    //

    Msg->Request.u1.s1.DataLength = (CSHORT)(sizeof( *ConnectMsg ) +
                                             ConnectionInfoLength);

    //
    //  The total length add on the LPCP message
    //
    //  **** should this just add on the size of a port message instead?
    //

    Msg->Request.u1.s1.TotalLength = (CSHORT)(sizeof( *Msg ) +
                                              Msg->Request.u1.s1.DataLength);

    //
    //  This will be a connection request message
    //

    Msg->Request.u2.s2.Type = LPC_CONNECTION_REQUEST;

    //
    //  If the caller supplied some connection information then copy
    //  that into place right now
    //

    if (ARGUMENT_PRESENT( ConnectionInformation )) {

        try {

            RtlMoveMemory( ConnectMsg + 1,
                           ConnectionInformation,
                           ConnectionInfoLength );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            //  If we fail then cleanup after ourselves and return the
            //  error to our caller
            //

            LpcpFreeToPortZone( Msg, FALSE );

            if (SectionToMap != NULL) {

                ObDereferenceObject( SectionToMap );
            }

            ObDereferenceObject( ClientPort );

            return GetExceptionCode();
        }
    }

    //
    //  The message is mostly ready to go now put it on the servers queue.
    //
    //  Acquire the mutex that guards the LpcReplyMessage field of the
    //  thread.  Also acquire the semaphore that guards the connection
    //  request message queue.  Stamp the connection request message with
    //  a serial number, insert the message at the tail of the connection
    //  request message queue and remember the address of the message in
    //  the LpcReplyMessage field for the current thread.
    //

    LpcpAcquireLpcpLock();

    //
    //  See if the port name has been deleted from under us.  If so, then
    //  don't queue the message and don't wait for a reply
    //

    if (ConnectionPort->Flags & PORT_NAME_DELETED) {

        Status = STATUS_OBJECT_NAME_NOT_FOUND;

    } else {

        Status = STATUS_SUCCESS;

        LpcpTrace(( "Send Connect Msg %lx to Port %wZ (%lx)\n", Msg, PortName, ConnectionPort ));

        //
        //  Stamp the request message with a serial number, insert the message
        //  at the tail of the request message queue
        //

        Msg->RepliedToThread = NULL;
        Msg->Request.MessageId = LpcpGenerateMessageId();

        CurrentThread->LpcReplyMessageId = Msg->Request.MessageId;

        InsertTailList( &ConnectionPort->MsgQueue.ReceiveHead, &Msg->Entry );
        InsertTailList( &ConnectionPort->LpcReplyChainHead, &CurrentThread->LpcReplyChain );

        CurrentThread->LpcReplyMessage = Msg;

        //
        //  Reference the port we are passing in the connect msg so if we die
        //  it will still be valid for the server in NtAcceptConnectPort.  The
        //  reference will be release when the message is freed.
        //

        ObReferenceObject( ClientPort );

        ConnectMsg->ClientPort = ClientPort;
    }

    LpcpReleaseLpcpLock();

    //
    //  At this point the client's communcation port is all set up and the
    //  connection request message is in the server's queue.  So now we have
    //  to single the server and wait for a reply
    //

    if (NT_SUCCESS( Status )) {

        //
        //  If this is a waitable port then set the event that they might be
        //  waiting on
        //

        if ( ConnectionPort->Flags & PORT_WAITABLE ) {

            KeSetEvent( &ConnectionPort->WaitEvent, 1, FALSE );
        }

        //
        //  Increment the connection request message queue semaphore by one for
        //  the newly inserted connection request message.  Release the spin
        //  locks, while remaining at the dispatcher IRQL.  Then wait for the
        //  reply to this connection request by waiting on the LpcReplySemaphore
        //  for the current thread.
        //

        KeReleaseSemaphore( ConnectionPort->MsgQueue.Semaphore,
                            1,
                            1,
                            FALSE );

        Status = KeWaitForSingleObject( &CurrentThread->LpcReplySemaphore,
                                        Executive,
                                        PreviousMode,
                                        FALSE,
                                        NULL );
    }

    if (Status == STATUS_USER_APC) {

        //
        //  if the semaphore is signaled, then clear it
        //

        if (KeReadStateSemaphore( &CurrentThread->LpcReplySemaphore )) {

            KeWaitForSingleObject( &CurrentThread->LpcReplySemaphore,
                                   WrExecutive,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            Status = STATUS_SUCCESS;
        }
    }

    //
    //  A connection request is accepted if the ConnectedPort of the client's
    //  communication port has been filled in.
    //

    if (Status == STATUS_SUCCESS) {

        SectionToMap = LpcpFreeConMsg( &Msg, &ConnectMsg, CurrentThread );

        //
        //  Check that we got a reply message
        //

        if (Msg != NULL) {

            //
            //  Copy any connection information back to the caller, but first
            //  calculate the new connection data length for the reply and
            //  don't let it grow beyond what we probed originally
            //

            if ((Msg->Request.u1.s1.DataLength - sizeof( *ConnectMsg )) < ConnectionInfoLength) {

                ConnectionInfoLength = Msg->Request.u1.s1.DataLength - sizeof( *ConnectMsg );
            }

            if (ARGUMENT_PRESENT( ConnectionInformation )) {

                try {

                    if (ARGUMENT_PRESENT( ConnectionInformationLength )) {

                        *ConnectionInformationLength = ConnectionInfoLength;
                    }

                    RtlMoveMemory( ConnectionInformation,
                                   ConnectMsg + 1,
                                   ConnectionInfoLength );

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    Status = GetExceptionCode();
                }
            }

            //
            //  Insert client communication port object in specified object
            //  table.  Set port handle value if successful.  If not
            //  successful, then the port will have been dereferenced, which
            //  will cause it to be freed, after our delete procedure is
            //  called.  The delete procedure will undo the work done to
            //  initialize the port.
            //

            if (ClientPort->ConnectedPort != NULL) {

                ULONG CapturedMaxMessageLength;

                //
                //  Before we do the object insert we need to get the max
                //  message length because right after the call the object
                //  could be dereferenced and gone away
                //

                CapturedMaxMessageLength = ConnectionPort->MaxMessageLength;

                //
                //  Now create a handle for the new client port object.
                //

                Status = ObInsertObject( ClientPort,
                                         NULL,
                                         PORT_ALL_ACCESS,
                                         0,
                                         (PVOID *)NULL,
                                         &Handle );

                if (NT_SUCCESS( Status )) {

                    //
                    //  This is the only successful path through this routine.
                    //  Set the output variables, later we'll free the msg
                    //  back to the port zone and return to our caller
                    //

                    try {

                        *PortHandle = Handle;

                        if (ARGUMENT_PRESENT( MaxMessageLength )) {

                            *MaxMessageLength = CapturedMaxMessageLength;
                        }

                        if (ARGUMENT_PRESENT( ClientView )) {

                            RtlMoveMemory( ClientView,
                                           &ConnectMsg->ClientView,
                                           sizeof( *ClientView ));
                        }

                        if (ARGUMENT_PRESENT( ServerView )) {

                            RtlMoveMemory( ServerView,
                                           &ConnectMsg->ServerView,
                                           sizeof( *ServerView ));
                        }

                    } except( EXCEPTION_EXECUTE_HANDLER ) {

                        Status = GetExceptionCode();
                        NtClose( Handle );
                    }
                }

            } else {

                //
                //  Otherwise we did not get a connect port from the server so
                //  the connection was refused
                //

                LpcpTrace(( "Connection request refused.\n" ));

                if ( SectionToMap != NULL ) {

                    ObDereferenceObject( SectionToMap );
                }

                if (ConnectionPort->Flags & PORT_NAME_DELETED) {

                    Status = STATUS_OBJECT_NAME_NOT_FOUND;

                } else {

                    Status = STATUS_PORT_CONNECTION_REFUSED;
                }

                ObDereferenceObject( ClientPort );
            }

            //
            //  Free the reply message back to the port zone
            //

            LpcpFreeToPortZone( Msg, FALSE );

        } else {

            //
            //  We did not get a reply message so the connection must have
            //  been refused
            //

            if (SectionToMap != NULL) {

                ObDereferenceObject( SectionToMap );
            }

            ObDereferenceObject( ClientPort );

            Status = STATUS_PORT_CONNECTION_REFUSED;
        }

    } else {

        //
        //  Our wait was not successful
        //

        //
        //  Remove the connection request message from the received
        //  queue and free the message back to the connection
        //  port's zone.
        //

        SectionToMap = LpcpFreeConMsg( &Msg, &ConnectMsg, CurrentThread );
        
        //
        //  The wait was not successful, but in the meantime the server could
        //  replied, so it signaled the lpc semaphore. We have to clear the
        //  semaphore state right now.
        //

        if (KeReadStateSemaphore( &CurrentThread->LpcReplySemaphore )) {

            KeWaitForSingleObject( &CurrentThread->LpcReplySemaphore,
                                   WrExecutive,
                                   KernelMode,
                                   FALSE,
                                   NULL );
        }

        if (Msg != NULL) {

            LpcpFreeToPortZone( Msg, FALSE );
        }

        //
        //  If a client section was specified, then dereference the section
        //  object.
        //

        if ( SectionToMap != NULL ) {

            ObDereferenceObject( SectionToMap );
        }

        //
        //  If the connection was rejected or the wait failed, then
        //  dereference the client port object, which will cause it to
        //  be deleted.
        //

        ObDereferenceObject( ClientPort );
    }

    //
    //  And return to our caller
    //

    return Status;
}


//
//  Local support routine
//

PVOID
LpcpFreeConMsg (
    IN PLPCP_MESSAGE *Msg,
    PLPCP_CONNECTION_MESSAGE *ConnectMsg,
    IN PETHREAD CurrentThread
    )

/*++

Routine Description:

    This routine returns a connection reply message for the specified thread

Arguments:

    Msg - Receives a pointer to the LPCP message if there is a reply

    ConnectMsg - Receives a pointer to the LPCP connection message if there
        is a reply

    CurrentThread - Specifies the thread we're to be examining

Return Value:

    PVOID - Returns a pointer to the section to map in the connection message

--*/

{
    PVOID SectionToMap;

    //
    //  Acquire the LPC mutex, remove the connection request message
    //  from the received queue and free the message back to the connection
    //  port's zone.
    //

    LpcpAcquireLpcpLock();

    //
    //  Remove the thread from the reply rundown list in case we did not wakeup due to
    //  a reply
    //

    if (!IsListEmpty( &CurrentThread->LpcReplyChain )) {

        RemoveEntryList( &CurrentThread->LpcReplyChain );

        InitializeListHead( &CurrentThread->LpcReplyChain );
    }

    //
    //  Check if the thread has an LPC reply message waiting to be handled
    //

    if (CurrentThread->LpcReplyMessage != NULL) {

        //
        //  Take the message off the threads list
        //

        *Msg = CurrentThread->LpcReplyMessage;

        CurrentThread->LpcReplyMessageId = 0;
        CurrentThread->LpcReplyMessage = NULL;

        //
        //  Set the connection message pointer, and copy over the section
        //  to map location before zeroing it out
        //

        *ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(*Msg + 1);

        SectionToMap = (*ConnectMsg)->SectionToMap;
        (*ConnectMsg)->SectionToMap = NULL;

    } else {

        //
        //  Otherwise there is no LPC message to be handle so we'll return
        //  null's to our caller
        //

        *Msg = NULL;
        SectionToMap = NULL;
    }

    //
    //  Release the global lock and return to our caller
    //

    LpcpReleaseLpcpLock();

    return SectionToMap;
}
