/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcinit.c

Abstract:

    Initialization module for the LPC subcomponent of NTOS

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

//
//  The following two object types are defined system wide to handle lpc ports
//

POBJECT_TYPE LpcPortObjectType;
POBJECT_TYPE LpcWaitablePortObjectType;

//
//  This is the default access mask mapping for lpc port objects
//

GENERIC_MAPPING LpcpPortMapping = {
    READ_CONTROL | PORT_CONNECT,
    DELETE | PORT_CONNECT,
    0,
    PORT_ALL_ACCESS
};

//
//  This lock is used to protect practically everything in lpc
//

LPC_MUTEX LpcpLock;

//
//  The following array of strings is used to debugger purposes and the
//  values correspond to the Port message types defined in ntlpcapi.h
//

#if ENABLE_LPC_TRACING

char *LpcpMessageTypeName[] = {
    "UNUSED_MSG_TYPE",
    "LPC_REQUEST",
    "LPC_REPLY",
    "LPC_DATAGRAM",
    "LPC_LOST_REPLY",
    "LPC_PORT_CLOSED",
    "LPC_CLIENT_DIED",
    "LPC_EXCEPTION",
    "LPC_DEBUG_EVENT",
    "LPC_ERROR_EVENT",
    "LPC_CONNECTION_REQUEST"
};

#endif // ENABLE_LPC_TRACING

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,LpcInitSystem)
#endif


BOOLEAN
LpcInitSystem (
    VOID
)

/*++

Routine Description:

    This function performs the system initialization for the LPC package.
    LPC stands for Local Inter-Process Communication.

Arguments:

    None.

Return Value:

    TRUE if successful and FALSE if an error occurred.

    The following errors can occur:

    - insufficient memory

--*/

{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING PortTypeName;
    ULONG ZoneElementSize;
    NTSTATUS Status;

    //
    //  Initialize our global lpc lock
    //

    LpcpInitializeLpcpLock();

    //
    //  Create the object type for the port object
    //

    RtlInitUnicodeString( &PortTypeName, L"Port" );

    RtlZeroMemory( &ObjectTypeInitializer, sizeof( ObjectTypeInitializer ));

    ObjectTypeInitializer.Length = sizeof( ObjectTypeInitializer );
    ObjectTypeInitializer.GenericMapping = LpcpPortMapping;
    ObjectTypeInitializer.MaintainTypeList = TRUE;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof( LPCP_PORT_OBJECT );
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof( LPCP_NONPAGED_PORT_QUEUE );
    ObjectTypeInitializer.InvalidAttributes = OBJ_VALID_ATTRIBUTES ^ PORT_VALID_OBJECT_ATTRIBUTES;
    ObjectTypeInitializer.ValidAccessMask = PORT_ALL_ACCESS;
    ObjectTypeInitializer.CloseProcedure = LpcpClosePort;
    ObjectTypeInitializer.DeleteProcedure = LpcpDeletePort;
    ObjectTypeInitializer.UseDefaultObject = TRUE ;

    ObCreateObjectType( &PortTypeName,
                        &ObjectTypeInitializer,
                        (PSECURITY_DESCRIPTOR)NULL,
                        &LpcPortObjectType );

    //
    //  Create the object type for the waitable port object
    //

    RtlInitUnicodeString( &PortTypeName, L"WaitablePort" );
    ObjectTypeInitializer.PoolType = NonPagedPool ;
    ObjectTypeInitializer.UseDefaultObject = FALSE ;

    ObCreateObjectType( &PortTypeName,
                        &ObjectTypeInitializer,
                        (PSECURITY_DESCRIPTOR)NULL,
                        &LpcWaitablePortObjectType );

    //
    //  Initialize the lpc message and callback id counters
    //

    LpcpNextMessageId = 1;
    LpcpNextCallbackId = 1;

    //
    //  Initialize the lpc port zone.  Each element can contain a max
    //  message, plus an LPCP message structure, plus an LPCP connection
    //  message
    //

    ZoneElementSize = PORT_MAXIMUM_MESSAGE_LENGTH +
                      sizeof( LPCP_MESSAGE ) +
                      sizeof( LPCP_CONNECTION_MESSAGE );

    //
    //  Round up the size to the next 16 byte alignment
    //

    ZoneElementSize = (ZoneElementSize + LPCP_ZONE_ALIGNMENT - 1) &
                      LPCP_ZONE_ALIGNMENT_MASK;

    //
    //  Initialize the zone
    //

    Status = LpcpInitializePortZone( ZoneElementSize,
                                     PAGE_SIZE,
                                     LPCP_ZONE_MAX_POOL_USAGE );

    if (!NT_SUCCESS( Status )) {

        return( FALSE );
    }

    return( TRUE );
}


char *
LpcpGetCreatorName (
    PLPCP_PORT_OBJECT PortObject
    )

/*++

Routine Description:

    This routine returns the name of the process that created the specified
    port object

Arguments:

    PortObject - Supplies the port object being queried

Return Value:

    char * - The image name of the process that created the port process

--*/

{
    NTSTATUS Status;
    PEPROCESS Process;

    //
    //  First find the process that created the port object
    //

    Status = PsLookupProcessByProcessId( PortObject->Creator.UniqueProcess, &Process );

    //
    //  If we were able to get the process then return the name of the process
    //  to our caller
    //

    if (NT_SUCCESS( Status )) {

        return Process->ImageFileName;

    } else {

        //
        //  Otherwise tell our caller we don't know the name
        //

        return "Unknown";
    }
}

#ifdef LPCP_TRACE_SERVER_THREADS

VOID LpcpPortExtraDataCreate ( PLPCP_PORT_OBJECT PortObject )
/*++

Routine Description:

    Allocate a LPCP_PORT_EXTRA_DATA structure and initializate it.

Arguments:

    PortObject - The port object that will contains this structure

Return Value:

    None


--*/
{
    PLPCP_PORT_EXTRA_DATA PortData;

    PortData = ExAllocatePoolWithTag(NonPagedPool, sizeof(LPCP_PORT_EXTRA_DATA), 'DEPL');

    PortData->ThreadCount = 0;

    PortData->PortObject = PortObject;

    PortObject->Reserved = (ULONG)PortData;

}

VOID LpcpPortExtraDataDestroy (PLPCP_PORT_OBJECT PortObject)
/*++

Routine Description:

    Free the memory alocated for the extra data structure

Arguments:

    PortObject - The port object that will contains this structure

Return Value:

    None

--*/
{
    PLPCP_PORT_EXTRA_DATA PortData;

    PortData = (PLPCP_PORT_EXTRA_DATA) PortObject->Reserved;

    if ((PortData != NULL) &&
        (PortData->PortObject == PortObject)) {

        ExFreePoolWithTag( PortData, 'DEPL');

        PortObject->Reserved = 0;
    }
}

VOID LpcpPortExtraDataPush (PLPCP_PORT_EXTRA_DATA PortData, PVOID Thread)
/*++

Routine Description:

    Push a new thread on the array. The opperation occurs only if the thread is first time
    using this port.

Arguments:

    PortData - the pointer to the Extra data port structure

    Thread - The thread that should be added to the thread array


Return Value:

    None


--*/
{
    ULONG Position;
    ULONG EndSearch;

    EndSearch = PortData->ThreadCount;

    if ( EndSearch >=  LPCP_SERVER_THREAD_ARRAY_SIZE) {

        EndSearch = LPCP_SERVER_THREAD_ARRAY_SIZE - 1;
    }

    for (Position = 0; Position < EndSearch ; Position++) {

        if ( PortData->Threads[Position] == Thread ) {

            return;
        }
    }

    PortData->Threads[PortData->ThreadCount % LPCP_SERVER_THREAD_ARRAY_SIZE] = Thread;

    PortData->ThreadCount += 1;

//    DbgPrint( "Port %lx : Threads: %ld  (%lx)\n", PortData->PortObject, PortData->ThreadCount, Thread);

}


VOID LpcpSaveThread (PLPCP_PORT_OBJECT PortObject)
/*++

Routine Description:

    This function is used to store the current thread to the PortObject. If the port object does not
    have the extra-data structure allocated, this function will call LpcpPortExtraDataCreate to do this.

Arguments:

    PortObject - The port object that will contains this structure

Return Value:

    None

--*/
{

    if (PortObject->Reserved == 0) {

        LpcpPortExtraDataCreate( PortObject );
    }

    if ( ((PortObject->Flags & PORT_TYPE) == SERVER_COMMUNICATION_PORT) &&
         (PortObject->Reserved != 0) ) {

             LpcpPortExtraDataPush( (PLPCP_PORT_EXTRA_DATA)PortObject->Reserved, PsGetCurrentThread());
    }
}

#endif // LPCP_TRACE_SERVER_THREADS

