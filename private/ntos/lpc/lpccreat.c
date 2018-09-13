/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpccreat.c

Abstract:

    Local Inter-Process Communication (LPC) connection system services.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

//
//  Local procedure prototype
//

NTSTATUS
LpcpCreatePort (
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage,
    IN BOOLEAN Waitable
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtCreatePort)
#pragma alloc_text(PAGE,NtCreateWaitablePort)
#pragma alloc_text(PAGE,LpcpCreatePort)
#endif


NTSTATUS
NtCreatePort (
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage
    )

/*++

Routine Description:

    See LpcpCreatePort.

Arguments:

    See LpcpCreatePort.

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    Status = LpcpCreatePort( PortHandle,
                             ObjectAttributes,
                             MaxConnectionInfoLength,
                             MaxMessageLength,
                             MaxPoolUsage,
                             FALSE );

    return Status ;

}


NTSTATUS
NtCreateWaitablePort (
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage
    )

/*++

Routine Description:

    Same as NtCreatePort.

    The only difference between this call and NtCreatePort is that the
    working KEVENT that can be used to wait for LPC messages to arrive
    asynchronously.

Arguments:

    See LpcpCreatePort.

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    NTSTATUS Status ;

    PAGED_CODE();

    Status = LpcpCreatePort( PortHandle,
                             ObjectAttributes,
                             MaxConnectionInfoLength,
                             MaxMessageLength,
                             MaxPoolUsage,
                             TRUE );

    return Status ;
}


//
//  Local support routine
//

NTSTATUS
LpcpCreatePort (
    OUT PHANDLE PortHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN ULONG MaxConnectionInfoLength,
    IN ULONG MaxMessageLength,
    IN ULONG MaxPoolUsage,
    IN BOOLEAN Waitable
    )

/*++

Routine Description:

    A server process can create a named connection port with the NtCreatePort
    service.

    A connection port is created with the name and SECURITY_DESCRIPTOR
    specified in the ObjectAttributes structure.  A handle to the connection
    port object is returned in the location pointed to by the PortHandle
    parameter.  The returned handle can then be used to listen for connection
    requests to that port name, using the NtListenPort service.

    The standard object architecture defined desired access parameter is not
    necessary since this service can only create a new port, not access an
    existing port.

    Connection ports cannot be used to send and receive messages.  They are
    only valid as a parameter to the NtListenPort service.

Arguments:

    PortHandle - A pointer to a variable that will receive the connection port
        object handle value.

    ObjectAttributes - A pointer to a structure that specifies the name of the
        object, an access control list (SECURITY_DESCRIPTOR) to be applied to
        the object, and a set of object attribute flags.

        PUNICODE_STRING ObjectName - An optional pointer to a null terminated
            port name string.  The form of the name is
            [\name...\name]\port_name.  If no name is specified then an
            unconnected communication port is created rather than a connection
            port.  This is useful for sending and receiving messages between
            threads of a single process.

        ULONG Attributes - A set of flags that control the port object
            attributes.

            None of the standard values are relevant for this call.
            Connection ports cannot be inherited, are always placed in the
            system handle table and are exclusive to the creating process.
            This field must be zero.  Future implementations might support
            specifying the OBJ_PERMANENT attribute.

    MaxMessageLength - Specifies the maximum length of messages sent or
        received on communication ports created from this connection
        port.  The value of this parameter cannot exceed
        MAX_PORTMSG_LENGTH bytes.

    MaxPoolUsage - Specifies the maximum amount of NonPaged pool used for
        message storage.

    Waitable - Specifies if the event used by the port can be use to wait
        for LPC messages to arrive asynchronously.

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    PLPCP_PORT_OBJECT ConnectionPort;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PUNICODE_STRING NamePtr;
    UNICODE_STRING CapturedObjectName;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    RtlInitUnicodeString( &CapturedObjectName, NULL );

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle( PortHandle );

            ProbeForRead( ObjectAttributes,
                          sizeof( OBJECT_ATTRIBUTES ),
                          sizeof( ULONG ));

            NamePtr = ObjectAttributes->ObjectName;

            if (NamePtr != NULL) {

                CapturedObjectName = ProbeAndReadStructure( NamePtr,
                                                            UNICODE_STRING );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }

    } else {

        if (ObjectAttributes->ObjectName != NULL) {

            CapturedObjectName = *(ObjectAttributes->ObjectName);
        }
    }

    //
    //  Make the null buffer indicate an unspecified port name
    //

    if (CapturedObjectName.Length == 0) {

        CapturedObjectName.Buffer = NULL;
    }

    //
    //  Allocate and initialize a port object.  If an object name was
    //  specified, then this is a connection port.  Otherwise this is an
    //  unconnected communication port that a process can use to communicate
    //  between threads.
    //

    Status = ObCreateObject( PreviousMode,
                             (Waitable ? LpcWaitablePortObjectType
                                       : LpcPortObjectType),
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             (ULONG)sizeof( LPCP_PORT_OBJECT ),
                             0,
                             0,
                             (PVOID *)&ConnectionPort );

    if (!NT_SUCCESS( Status )) {

        return( Status );
    }

    //
    //  Zero out the connection port object and then initialize its fields
    //

    RtlZeroMemory( ConnectionPort, (ULONG)sizeof( LPCP_PORT_OBJECT ));

    ConnectionPort->Length = sizeof( LPCP_PORT_OBJECT );
    ConnectionPort->ConnectionPort = ConnectionPort;
    ConnectionPort->Creator = PsGetCurrentThread()->Cid;

    InitializeListHead( &ConnectionPort->LpcReplyChainHead );

    InitializeListHead( &ConnectionPort->LpcDataInfoChainHead );

    //
    //  Named ports get a connection message queue.
    //

    if (CapturedObjectName.Buffer == NULL) {

        ConnectionPort->Flags = UNCONNECTED_COMMUNICATION_PORT;
        ConnectionPort->ConnectedPort = ConnectionPort;
        ConnectionPort->ServerProcess = NULL;

    } else {

        ConnectionPort->Flags = SERVER_CONNECTION_PORT;

        ObReferenceObject( PsGetCurrentProcess() );
        ConnectionPort->ServerProcess = PsGetCurrentProcess();
    }
    
    if ( Waitable ) {

        ConnectionPort->Flags |= PORT_WAITABLE;
    }
    
    //
    //  All ports get a request message queue.
    //

    Status = LpcpInitializePortQueue( ConnectionPort );

    if (!NT_SUCCESS(Status)) {

        ObDereferenceObject( ConnectionPort );

        return(Status);
    }

    //
    //  For a waitable port, create the KEVENT that will
    //  be used to signal clients
    //

    if (ConnectionPort->Flags & PORT_WAITABLE) {

        KeInitializeEvent( &ConnectionPort->WaitEvent,
                           NotificationEvent,
                           FALSE );
    }

    //
    //  Set the maximum message length and connection info length based on the
    //  zone block size less the structs overhead.
    //

    ConnectionPort->MaxMessageLength = LpcpZone.Zone.BlockSize -
                                        FIELD_OFFSET( LPCP_MESSAGE, Request );

    ConnectionPort->MaxConnectionInfoLength = ConnectionPort->MaxMessageLength -
                                                sizeof( PORT_MESSAGE ) -
                                                sizeof( LPCP_CONNECTION_MESSAGE );

#if DBG
    LpcpTrace(( "Created port %ws (%x) - MaxMsgLen == %x  MaxConnectInfoLen == %x\n",
                CapturedObjectName.Buffer == NULL ? L"** UnNamed **" : ObjectAttributes->ObjectName->Buffer,
                ConnectionPort,
                ConnectionPort->MaxMessageLength,
                ConnectionPort->MaxConnectionInfoLength ));
#endif

    //
    //  Sanity check that the max message length being asked for is not
    //  greater than the max message length possible in the system
    //

    if (ConnectionPort->MaxMessageLength < MaxMessageLength) {

#if DBG
        LpcpPrint(( "MaxMessageLength granted is %x but requested %x\n",
                    ConnectionPort->MaxMessageLength,
                    MaxMessageLength ));
        DbgBreakPoint();
#endif

        ObDereferenceObject( ConnectionPort );

        return STATUS_INVALID_PARAMETER_4;
    }
    
    //
    //  Save the MaxMessageLength to the connection port
    //

    ConnectionPort->MaxMessageLength = MaxMessageLength;

    //
    //  Sanity check that the max connection info length being asked for is
    //  not greater than the maximum possible in the system
    //

    if (ConnectionPort->MaxConnectionInfoLength < MaxConnectionInfoLength) {

#if DBG
        LpcpPrint(( "MaxConnectionInfoLength granted is %x but requested %x\n",
                    ConnectionPort->MaxConnectionInfoLength,
                    MaxConnectionInfoLength ));
        DbgBreakPoint();
#endif

        ObDereferenceObject( ConnectionPort );

        return STATUS_INVALID_PARAMETER_3;
    }

    //
    //  Insert connection port object in specified object table.  Set port
    //  handle value if successful.  If not successful, then the port will
    //  have been dereferenced, which will cause it to be freed, after our
    //  delete procedure is called.  The delete procedure will undo the work
    //  done to initialize the port.  Finally, return the system server status.
    //

    Status = ObInsertObject( ConnectionPort,
                             NULL,
                             PORT_ALL_ACCESS,
                             0,
                             (PVOID *)NULL,
                             &Handle );

    if (NT_SUCCESS( Status )) {

        //
        //  Set the output variable protected against access faults
        //

        try {

            *PortHandle = Handle;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            NtClose( Handle );

            Status = GetExceptionCode();
        }
    }

    //
    //  And return to our caller
    //

    return Status;
}


