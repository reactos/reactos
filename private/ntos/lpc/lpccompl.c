/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpccompl.c

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

VOID
LpcpPrepareToWakeClient (
    IN PETHREAD ClientThread
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtAcceptConnectPort)
#pragma alloc_text(PAGE,NtCompleteConnectPort)
#pragma alloc_text(PAGE,LpcpPrepareToWakeClient)
#endif


NTSTATUS
NtAcceptConnectPort (
    OUT PHANDLE PortHandle,
    IN PVOID PortContext OPTIONAL,
    IN PPORT_MESSAGE ConnectionRequest,
    IN BOOLEAN AcceptConnection,
    IN OUT PPORT_VIEW ServerView OPTIONAL,
    OUT PREMOTE_PORT_VIEW ClientView OPTIONAL
    )

/*++

Routine Description:

    A server process can accept or reject a client connection request
    using the NtAcceptConnectPort service.

    The ConnectionRequest parameter must specify a connection request
    returned by a previous call to the NtListenPort service.  This
    service will either complete the connection if the AcceptConnection
    parameter is TRUE, or reject the connection request if the
    AcceptConnection parameter is FALSE.

    In either case, the contents of the data portion of the connection
    request is the data to return to the caller of NtConnectPort.

    If the connection request is accepted, then two communication port
    objects will be created and connected together.  One will be
    inserted in the client process' handle table and returned to the
    client via the PortHandle parameter it specified on the
    NtConnectPort service.  The other will be inserted in the server
    process' handle table and returned via the PortHandle parameter
    specified on the NtCompleteConnectPort service.  In addition the
    two communication ports (client and server) will be linked together.

    If the connection request is accepted, and the ServerView parameter
    was specified, then the section handle is examined.  If it is valid,
    then the portion of the section described by the SectionOffset and
    ViewSize fields will be mapped into both the client and server
    process address spaces.  The address in server's address space will
    be returned in the ViewBase field.  The address in the client's
    address space will be returned in the ViewRemoteBase field.  The
    actual offset and size used to map the section will be returned in
    the SectionOffset and ViewSize fields.

    Communication port objects are temporary objects that have no names
    and cannot be inherited.  When either the client or server process
    calls the !f NtClose service for a communication port, the port will
    be deleted since there can never be more than one outstanding handle
    for each communication port.  The port object type specific delete
    procedure will then be invoked.  This delete procedure will examine
    the communication port, and if it is connected to another
    communication port, it will queue an LPC_PORT_CLOSED datagram to
    that port's message queue.  This will allow both the client and
    server processes to notice when a port becomes disconnected, either
    because of an explicit call to NtClose or an implicit call due to
    process termination.  In addition, the delete procedure will scan
    the message queue of the port being closed and for each message
    still in the queue, it will return an ERROR_PORT_CLOSED status to
    any thread that is waiting for a reply to the message.

Arguments:

    PortHandle - A pointer to a variable that will receive the server
        communication port object handle value.

    PortContext - An uninterpreted pointer that is stored in the
        server communication port.  This pointer is returned whenever
        a message is received for this port.

    ConnectionRequest - A pointer to a structure that describes the
        connection request being accepted or rejected:

        The ConnectionRequest structure

        ULONG Length - Specifies the size of this data structure in
            bytes.

        CLIENT_ID ClientId - Specifies a structure that contains the
            client identifier (CLIENT_ID) of the thread that sent the
            request.

        The ClientId Structure

            ULONG UniqueProcessId - A unique value for each process
                in the system.

            ULONG UniqueThreadId - A unique value for each thread in the
                system.

        ULONG MessageId - A unique value that identifies the connection
            request being completed.

        ULONG PortAttributes - This field has no meaning for this service.

        ULONG ClientViewSize - This field has no meaning for this service.

    AcceptConnection - Specifies a boolean value which indicates where
        the connection request is being accepted or rejected.  A value
        of TRUE means that the connection request is accepted and a
        server communication port handle will be created and connected
        to the client's communication port handle.  A value of FALSE
        means that the connection request is not accepted.

    ServerView - A pointer to a structure that specifies the section that
        the server process will use to send messages back to the client
        process connected to this port.

        The ServerView Structure

        ULONG Length - Specifies the size of this data structure in
            bytes.

        HANDLE SectionHandle - Specifies an open handle to a section
            object.

        ULONG SectionOffset - Specifies a field that will receive the
            actual offset, in bytes, from the start of the section.  The
            initial value of this parameter specifies the byte offset
            within the section that the client's view is based.  The
            value is rounded down to the next host page size boundary.

        ULONG ViewSize - Specifies the size of the view, in bytes.

        PVOID ViewBase - Specifies a field that will receive the base
            address of the port memory in the server's address space.

        PVOID ViewRemoteBase - Specifies a field that will receive
            the base address of the server port's memory in the client's
            address space.  Used to generate pointers that are
            meaningful to the client.

    ClientView - An optional pointer to a structure that will receive
        information about the client process' view in the server's
        address space.  The server process can use this information
        to validate pointers it receives from the client process.

        The ClientView Structure

        ULONG Length - Specifies the size of this data structure in
            bytes.

        PVOID ViewBase - Specifies a field that will receive the base
            address of the client port's memory in the server's address
            space.

        ULONG ViewSize - Specifies a field that will receive the
            size, in bytes, of the client's view in the server's address
            space.  If this field is zero, then client has no view in
            the server's address space.

Return Value:

    NTSTATUS - An appropriate status value.

--*/

{
    PLPCP_PORT_OBJECT ConnectionPort;
    PLPCP_PORT_OBJECT ServerPort;
    PLPCP_PORT_OBJECT ClientPort;
    PVOID ClientSectionToMap;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    ULONG ConnectionInfoLength;
    PLPCP_MESSAGE Msg;
    PLPCP_CONNECTION_MESSAGE ConnectMsg;
    PORT_MESSAGE CapturedReplyMessage;
    PVOID SectionToMap;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize;
    PEPROCESS ClientProcess;
    PETHREAD ClientThread;
    PORT_VIEW CapturedServerView;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle( PortHandle );

            ProbeForRead( ConnectionRequest,
                          sizeof( *ConnectionRequest ),
                          sizeof( ULONG ));

            CapturedReplyMessage = *ConnectionRequest;

            if (ARGUMENT_PRESENT( ServerView )) {

                CapturedServerView = ProbeAndReadStructure( ServerView, PORT_VIEW );

                if (CapturedServerView.Length != sizeof( *ServerView )) {

                    return STATUS_INVALID_PARAMETER;
                }

                ProbeForWrite( ServerView,
                               sizeof( *ServerView ),
                               sizeof( ULONG ));
            }

            if (ARGUMENT_PRESENT( ClientView )) {

                if (ProbeAndReadUlong( &ClientView->Length ) != sizeof( *ClientView )) {

                    return STATUS_INVALID_PARAMETER;
                }

                ProbeForWrite( ClientView,
                               sizeof( *ClientView ),
                               sizeof( ULONG ));
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return GetExceptionCode();
        }

    } else {

        //
        //  Otherwise the previous mode is kernel mode
        //

        CapturedReplyMessage = *ConnectionRequest;

        if (ARGUMENT_PRESENT( ServerView )) {

            if (ServerView->Length != sizeof( *ServerView )) {

                return STATUS_INVALID_PARAMETER;
            }

            CapturedServerView = *ServerView;
        }

        if (ARGUMENT_PRESENT( ClientView )) {

            if (ClientView->Length != sizeof( *ClientView )) {

                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    //  Translate the ClientId from the connection request into a
    //  thread pointer.  This is a referenced pointer to keep the thread
    //  from evaporating out from under us.
    //

    Status = PsLookupProcessThreadByCid( &CapturedReplyMessage.ClientId,
                                         &ClientProcess,
                                         &ClientThread );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Acquire the mutex that guards the LpcReplyMessage field of
    //  the thread and get the pointer to the message that the thread
    //  is waiting for a reply to.
    //

    LpcpAcquireLpcpLock();

    //
    //  See if the thread is waiting for a reply to the connection request
    //  specified on this call.  If not then a bogus connection request
    //  has been specified, so release the mutex, dereference the thread
    //  and return failure.
    //
    //  The check is that the client is waiting for a reply to a connection
    //  request and that the message id is both valid and lines up correctly
    //

    if ((ClientThread->LpcReplyMessage == NULL) ||
        (CapturedReplyMessage.MessageId == 0) ||
        (ClientThread->LpcReplyMessageId != CapturedReplyMessage.MessageId) ||
        ((((PLPCP_MESSAGE)ClientThread->LpcReplyMessage)->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) != LPC_CONNECTION_REQUEST)) {

        Msg = NULL;

    } else {

        //
        //  Remember the LPCP message from the thread
        //

        Msg = ClientThread->LpcReplyMessage;

        //
        //  Get connection message immediately following the LPCP message
        //

        ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

        //
        //  Remember the client port from the connection message
        //

        ClientPort = ConnectMsg->ClientPort;
        
        //
        //  Get a pointer to the connection port from the client port.
        //

        ConnectionPort = ClientPort->ConnectionPort;
        
        //
        //  Check if the server process accept the connection
        //
        
        if ( ConnectionPort->ServerProcess != PsGetCurrentProcess() ) {
            
            //
            //  Release the LPC mutex
            //
            
            LpcpReleaseLpcpLock();
            
            ObDereferenceObject( ClientProcess );
            ObDereferenceObject( ClientThread );

            return (STATUS_REPLY_MESSAGE_MISMATCH);
        }
        
        //
        //  Remove the LPC message from the thread
        //
                
        ClientThread->LpcReplyMessage = NULL;
        
        //
        //  Remove the client port from the connection message
        //

        ConnectMsg->ClientPort = NULL;

        //
        //  Clean up the rest of the client thread.  This cleanup use to be
        //  done unconditionally right before releasing the mutex however
        //  this causes trouble if our caller supplied a bad reply message
        //  and we clobber an arbitrary threads state
        //

        ClientThread->LpcReplyMessageId = 0;
    }

    //
    //  Release the mutex that guards the field.
    //

    LpcpReleaseLpcpLock();

    //
    //  Now if we did not get an LPCP message from a client thread then this
    //  isn't a good call and we'll dereference what we thought was the
    //  client thread/process and tell our caller their mistake
    //

    if ( !Msg ) {

        LpcpPrint(( "%s Attempted AcceptConnectPort to Thread %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    ClientThread,
                    THREAD_TO_PROCESS( ClientThread )->ImageFileName ));
        LpcpPrint(( "failed.  MessageId == %u\n", CapturedReplyMessage.MessageId ));
        LpcpPrint(( "         Thread MessageId == %u\n", ClientThread->LpcReplyMessageId ));
        LpcpPrint(( "         Thread Msg == %x\n", ClientThread->LpcReplyMessage ));

        ObDereferenceObject( ClientProcess );
        ObDereferenceObject( ClientThread );

        return (STATUS_REPLY_MESSAGE_MISMATCH);
    }

    //
    //  At this point we have a good matching client for this accept connect
    //  call.
    //
    
    LpcpTrace(("Replying to Connect Msg %lx to Port %lx\n",
               Msg, ClientPort->ConnectionPort ));

    //
    //  Regardless of whether we are accepting or rejecting the connection,
    //  return the connection information to the waiting thread.
    //

    ConnectionInfoLength = CapturedReplyMessage.u1.s1.DataLength;

    if (ConnectionInfoLength > ConnectionPort->MaxConnectionInfoLength) {

        ConnectionInfoLength = ConnectionPort->MaxConnectionInfoLength;
    }

    Msg->Request.u1.s1.DataLength = (CSHORT)(sizeof( *ConnectMsg ) +
                                             ConnectionInfoLength);

    Msg->Request.u1.s1.TotalLength = (CSHORT)(sizeof( *Msg ) +
                                              Msg->Request.u1.s1.DataLength);

    Msg->Request.u2.s2.Type = LPC_REPLY;
    Msg->Request.u2.s2.DataInfoOffset = 0;
    Msg->Request.ClientId = CapturedReplyMessage.ClientId;
    Msg->Request.MessageId = CapturedReplyMessage.MessageId;
    Msg->Request.ClientViewSize = 0;

    try {

        RtlMoveMemory( ConnectMsg + 1,
                       (PCHAR)(ConnectionRequest + 1),
                       ConnectionInfoLength );

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();
    }

    //
    //  Now it is time to process a positive accept request
    //

    ClientSectionToMap = NULL;

    if (AcceptConnection) {

        //
        //  Allocate and initialize a server communication port object.
        //  Communication ports have no names, can not be inherited and
        //  are process private handles.
        //

        Status = ObCreateObject( PreviousMode,
                                 LpcPortObjectType,
                                 NULL,
                                 PreviousMode,
                                 NULL,
                                 sizeof( LPCP_PORT_OBJECT ),
                                 0,
                                 0,
                                 (PVOID *)&ServerPort );

        if (!NT_SUCCESS( Status )) {

            goto bailout;
        }

        RtlZeroMemory( ServerPort, sizeof( LPCP_PORT_OBJECT ));

        ServerPort->Length = sizeof( LPCP_PORT_OBJECT );
        ServerPort->PortContext = PortContext;
        ServerPort->Flags = SERVER_COMMUNICATION_PORT;

        InitializeListHead( &ServerPort->LpcReplyChainHead );
        InitializeListHead( &ServerPort->LpcDataInfoChainHead );

        //
        //  Connect the newly created server communication port to the
        //  connection port with a referenced pointer.  Prevents the
        //  connection port from going away until all of the communication
        //  ports have been closed.
        //

        ObReferenceObject( ConnectionPort );

        ServerPort->ConnectionPort = ConnectionPort;
        ServerPort->MaxMessageLength = ConnectionPort->MaxMessageLength;

        //
        //  Connect the client and server communication ports together
        //  with unreferenced pointers.  They are unreferenced so that
        //  the PortObjectType delete procedure will get called when a
        //  communication port is closed.  If this were not the case then
        //  we would need a special NtClosePort system service in order
        //  to tear down a pair of connected communication ports.
        //

        ServerPort->ConnectedPort = ClientPort;
        ClientPort->ConnectedPort = ServerPort;

        ServerPort->Creator = PsGetCurrentThread()->Cid;
        ClientPort->Creator = Msg->Request.ClientId;

        //
        //  If the client has allocated a port memory section that is mapped
        //  into the client's address space, then map a view of the same
        //  section for the server process to see.
        //

        LpcpAcquireLpcpLock();

        ClientSectionToMap = ConnectMsg->SectionToMap;
        ConnectMsg->SectionToMap = NULL;

        LpcpReleaseLpcpLock();

        if (ClientSectionToMap) {

            LARGE_INTEGER LargeSectionOffset;

            LargeSectionOffset.LowPart = ConnectMsg->ClientView.SectionOffset;
            LargeSectionOffset.HighPart = 0;

            Status = MmMapViewOfSection( ClientSectionToMap,
                                         PsGetCurrentProcess(),
                                         &ServerPort->ClientSectionBase,
                                         0,
                                         0,
                                         &LargeSectionOffset,
                                         &ConnectMsg->ClientView.ViewSize,
                                         ViewUnmap,
                                         0,
                                         PAGE_READWRITE );

            ConnectMsg->ClientView.SectionOffset = LargeSectionOffset.LowPart;

            if (NT_SUCCESS( Status )) {

                ConnectMsg->ClientView.ViewRemoteBase = ServerPort->ClientSectionBase;

            } else {

                //
                //  At this point we're really going to drop all the way
                //  out to the label bailout: because everything else is
                //  protected with a test against Status.  But first we have
                //  to release the server port that we've just created
                //

                ObDereferenceObject( ServerPort );
            }
        }

        //
        //  If the server process has allocated a port memory section for
        //  send data to the client on call back requests, map two views
        //  of that section, the first for the server process and the
        //  second view for the client process.  Return the location of the
        //  server's view to the caller of this function.  Return the
        //  client's view to the client process via the reply to the
        //  connection request.
        //

        if (NT_SUCCESS( Status ) && ARGUMENT_PRESENT( ServerView )) {

            LARGE_INTEGER LargeSectionOffset;

            LargeSectionOffset.LowPart = CapturedServerView.SectionOffset;
            LargeSectionOffset.HighPart = 0;

            //
            //  Map in the section into the servers address space
            //

            //
            //  **** Does this call need to verify that the section handle
            //       is still valid.
            //

            Status = ObReferenceObjectByHandle( CapturedServerView.SectionHandle,
                                                SECTION_MAP_READ |
                                                SECTION_MAP_WRITE,
                                                MmSectionObjectType,
                                                PreviousMode,
                                                (PVOID *)&SectionToMap,
                                                NULL );

            if (NT_SUCCESS( Status )) {

                Status = MmMapViewOfSection( SectionToMap,
                                             PsGetCurrentProcess(),
                                             &ServerPort->ServerSectionBase,
                                             0,
                                             0,
                                             &LargeSectionOffset,
                                             &CapturedServerView.ViewSize,
                                             ViewUnmap,
                                             0,
                                             PAGE_READWRITE );

                if (NT_SUCCESS( Status )) {

                    CapturedServerView.SectionOffset = LargeSectionOffset.LowPart;

                    CapturedServerView.ViewBase = ServerPort->ServerSectionBase;


                    SectionOffset.LowPart = CapturedServerView.SectionOffset;
                    SectionOffset.HighPart = 0;

                    ViewSize = CapturedServerView.ViewSize;

                    Status = MmMapViewOfSection( SectionToMap,
                                                 ClientProcess,
                                                 &ClientPort->ServerSectionBase,
                                                 0,
                                                 0,
                                                 &SectionOffset,
                                                 &ViewSize,
                                                 ViewUnmap,
                                                 0,
                                                 PAGE_READWRITE );

                    if (NT_SUCCESS( Status )) {

                        //
                        //  Let the server know where the client's view of the
                        //  section got mapped
                        //

                        CapturedServerView.ViewRemoteBase = ClientPort->ServerSectionBase;

                        //
                        //  Let the client know where the server's view of the
                        //  section got mapped
                        //

                        ConnectMsg->ServerView.ViewBase = ClientPort->ServerSectionBase;
                        ConnectMsg->ServerView.ViewSize = ViewSize;

                    } else {

                        ObDereferenceObject( ServerPort );
                    }

                } else {

                    ObDereferenceObject( ServerPort );
                }

                ObDereferenceObject( SectionToMap );

            } else {

                ObDereferenceObject( ServerPort );
            }
        }

        //
        //  Insert the server communication port object in specified object
        //  table.  Set port handle value if successful.  If not
        //  successful, then the port will have been dereferenced, which
        //  will cause it to be freed, after our delete procedure is
        //  called.  The delete procedure will undo the work done to
        //  initialize the port.
        //

        if (NT_SUCCESS( Status )) {

            //
            //  Add an extra reference to the object otherwise right when we
            //  create the handle a rouge caller might close and destroy the
            //  port.
            //

            ObReferenceObject( ServerPort );

            //
            //  Now add the handle
            //

            Status = ObInsertObject( ServerPort,
                                     NULL,
                                     PORT_ALL_ACCESS,
                                     0,
                                     (PVOID *)NULL,
                                     &Handle );

            if (NT_SUCCESS( Status )) {

                try {

                    if (ARGUMENT_PRESENT( ServerView )) {

                        *ServerView = CapturedServerView;
                    }

                    if (ARGUMENT_PRESENT( ClientView )) {

                        ClientView->ViewBase = ConnectMsg->ClientView.ViewRemoteBase;
                        ClientView->ViewSize = ConnectMsg->ClientView.ViewSize;
                    }

                    *PortHandle = Handle;

                    if (!ARGUMENT_PRESENT( PortContext )) {

                        ServerPort->PortContext = Handle;
                    }

                    ServerPort->ClientThread = ClientThread;

                    LpcpAcquireLpcpLock();
                    ClientThread->LpcReplyMessage = Msg;
                    LpcpReleaseLpcpLock();

                    ClientThread = NULL;

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    NtClose( Handle );
                    Status = GetExceptionCode();
                }
            }

            //
            //  Now we can remove the extra object reference
            //

            ObDereferenceObject( ServerPort );
        }

    } else {

        //
        //  Otherwise the server has not accepted the connection request
        //

        LpcpPrint(( "Refusing connection from %x.%x\n",
                    Msg->Request.ClientId.UniqueProcess,
                    Msg->Request.ClientId.UniqueThread ));
    }

bailout:

    if ( ClientSectionToMap ) {

        ObDereferenceObject( ClientSectionToMap );
    }

    //
    //  If the client is not null then this is an error condition and we need
    //  to cleanup and wake the client thread.  In success cases the client
    //  thread is woken up with a call to Complete Connect Request
    //

    if (ClientThread != NULL) {

        LpcpAcquireLpcpLock();

        ClientThread->LpcReplyMessage = Msg;

        if (AcceptConnection) {

            LpcpPrint(( "LPC: Failing AcceptConnection with Status == %x\n", Status ));
        }

        LpcpPrepareToWakeClient( ClientThread );

        LpcpReleaseLpcpLock();

        //
        //  Wake up the thread that is waiting for an answer to its connection
        //  request inside of NtConnectPort.
        //

        KeReleaseSemaphore( &ClientThread->LpcReplySemaphore,
                            0,
                            1L,
                            FALSE );

        //
        //  Dereference client thread and return the system service status.
        //

        ObDereferenceObject( ClientThread );
    }

    if (ClientPort) {

        ObDereferenceObject( ClientPort );
    }

    ObDereferenceObject( ClientProcess );

    //
    //  And return to our caller
    //

    return Status;
}


NTSTATUS
NtCompleteConnectPort (
    IN HANDLE PortHandle
    )

/*++

Routine Description:

    This routine is called by the server after it calls NtAcceptConnectPort to
    wake up the client thread.  Between calling NtAcceptConnectPort and
    NtCompleteConnectPort the server can do whatever work is necessary before
    waking up the client

Arguments:

    PortHandle - Supplies a handle to the server communcation port

Return Value:

    NTSTATUS - An appropriate status value.

--*/

{
    PLPCP_PORT_OBJECT PortObject;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PETHREAD ClientThread;

    PAGED_CODE();

    //
    //  Get previous processor mode
    //

    PreviousMode = KeGetPreviousMode();

    //
    //  Reference the port object by handle
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Error if a port type is invalid.
    //

    if ((PortObject->Flags & PORT_TYPE) != SERVER_COMMUNICATION_PORT) {

        ObDereferenceObject( PortObject );

        return STATUS_INVALID_PORT_HANDLE;
    }

    //
    //  Under the LPC lock we need to check for a client thread and if there
    //  is one we'll remember and remove the client thread, and then prepare
    //  to wake the client
    //

    LpcpAcquireLpcpLock();

    if (PortObject->ClientThread == NULL) {

        LpcpReleaseLpcpLock();

        ObDereferenceObject( PortObject );

        return STATUS_INVALID_PARAMETER;
    }

    ClientThread = PortObject->ClientThread;

    //
    //  Double check that the thread is still waiting for a reply message
    //

    if (ClientThread->LpcReplyMessage == NULL) {

        LpcpReleaseLpcpLock();

        ObDereferenceObject( PortObject );

        return STATUS_PORT_DISCONNECTED;
    }

    //
    //  The check needs to ensure that the client thread is really on the
    //  reply chain for the sever's connection port.  This is a quick and
    //  dirty fix for NT 5.0.  We zoom down the connection port lpc reply
    //  chain looking for an entry that contains the client threads.  If
    //  we find a match it's okay if we don't it's bad.
    //

    {
        PLIST_ENTRY Entry;

        for (Entry = PortObject->ConnectionPort->LpcReplyChainHead.Flink;
             Entry != (PLIST_ENTRY)(&PortObject->ConnectionPort->LpcReplyChainHead.Flink);
             Entry = Entry->Flink) {

            if (Entry == ((PLIST_ENTRY)(&ClientThread->LpcReplyChain.Flink))) {

                break;
            }
        }

        if (Entry != ((PLIST_ENTRY)(&ClientThread->LpcReplyChain.Flink))) {

            LpcpReleaseLpcpLock();

            ObDereferenceObject( PortObject );

            return STATUS_PORT_DISCONNECTED;
        }
    }

    //
    //  Now do the wakeup
    //

    PortObject->ClientThread = NULL;

    LpcpPrepareToWakeClient( ClientThread );

    LpcpReleaseLpcpLock();

    //
    //  Wake up the thread that is waiting for an answer to its connection
    //  request inside of NtConnectPort.
    //

    KeReleaseSemaphore( &ClientThread->LpcReplySemaphore,
                        0,
                        1L,
                        FALSE );

    //
    //  Dereference client thread
    //

    ObDereferenceObject( ClientThread );
    ObDereferenceObject( PortObject );

    //
    //  And return to our caller
    //

    return Status;
}


//
//  Local support routine
//

VOID
LpcpPrepareToWakeClient (
    IN PETHREAD ClientThread
    )

/*++

Routine Description:

    This routine is used to prepare the client thread to receive a reply to
    its connection request

Arguments:

    ClientThread - Specifies the thread we are preparing to wake up

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Remove the thread from the rundown list the connection port as we are
    //  sending a reply.  The operation only needs to take place if the
    //  thread isn't exiting and it's in the lpc reply chain for a connection
    //  port
    //

    if ((!ClientThread->LpcExitThreadCalled) &&
        (!IsListEmpty( &ClientThread->LpcReplyChain ))) {

        RemoveEntryList( &ClientThread->LpcReplyChain );
        InitializeListHead( &ClientThread->LpcReplyChain );
    }

    //
    //  And return to our caller
    //

    return;
}
