/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    eventpr.c

Abstract:

    This module implements the executive event pair object.  Functions
    are provided to create, open, waitlow, waithi, setlow, sethi,
    sethiwaitlo, setlowaithi.

Author:

    Mark Lucovsky (markl) 18-Oct-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Define performance counters.
//

ULONG EvPrSetHigh = 0;
ULONG EvPrSetLow = 0;

//
// Address of event pair object type descriptor.
//

POBJECT_TYPE ExEventPairObjectType;

//
// Structure that describes the mapping of generic access rights to object
// specific access rights for event pair objects.
//

GENERIC_MAPPING ExpEventPairMapping = {
    STANDARD_RIGHTS_READ |
        SYNCHRONIZE,
    STANDARD_RIGHTS_WRITE |
        SYNCHRONIZE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    EVENT_PAIR_ALL_ACCESS
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpEventPairInitialization)
#pragma alloc_text(PAGE, NtCreateEventPair)
#pragma alloc_text(PAGE, NtOpenEventPair)
#pragma alloc_text(PAGE, NtWaitLowEventPair)
#pragma alloc_text(PAGE, NtWaitHighEventPair)
#pragma alloc_text(PAGE, NtSetLowWaitHighEventPair)
#pragma alloc_text(PAGE, NtSetHighWaitLowEventPair)
#pragma alloc_text(PAGE, NtSetHighEventPair)
#pragma alloc_text(PAGE, NtSetLowEventPair)
#endif

BOOLEAN
ExpEventPairInitialization (
    )

/*++

Routine Description:

    This function creates the event pair object type descriptor at system
    initialization and stores the address of the object type descriptor
    in global storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the event pair object type descriptor is
    successfully initialized. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    ULONG Offset = 0;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"EventPair");

    //
    // Create event object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = ExpEventPairMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EEVENT_PAIR);
    ObjectTypeInitializer.ValidAccessMask = EVENT_PAIR_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExEventPairObjectType);

    //
    // If the event pair object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}

NTSTATUS
NtCreateEventPair (
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    )

/*++

Routine Description:

    This function creates an event pair object, sets it initial state,
    and opens a handle to the object with the specified desired access.

Arguments:

    EventPairHandle - Supplies a pointer to a variable that will receive the
        event pair object handle.

    DesiredAccess - Supplies the desired types of access for the event
        pair object.

    ObjectAttributes - Supplies a pointer to an object attributes
        structure.

Return Value:

    TBS

--*/

{

    PEEVENT_PAIR EventPair;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to create an event object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object insertion routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(EventPairHandle);
        }

        //
        // Allocate event object.
        //

        Status = ObCreateObject(PreviousMode,
                                ExEventPairObjectType,
                                ObjectAttributes,
                                PreviousMode,
                                NULL,
                                sizeof(EEVENT_PAIR),
                                0,
                                0,
                                (PVOID *)&EventPair);

        //
        // If the event pair object was successfully allocated, then
        // initialize the event pair object and attempt to insert the
        // event pair object in the current process' handle table.
        //

        if (NT_SUCCESS(Status)) {
            KeInitializeEventPair(&EventPair->KernelEventPair);
            Status = ObInsertObject((PVOID)EventPair,
                                    NULL,
                                    DesiredAccess,
                                    0,
                                    (PVOID *)NULL,
                                    &Handle);

            //
            // If the event pair object was successfully inserted in the
            // current process' handle table, then attempt to write the
            // event pair object handle value.  If the write attempt
            // fails, then do not report an error.  When the caller
            // attempts to access the handle value, an access violation
            // will occur.

            if (NT_SUCCESS(Status)) {
                try {
                    *EventPairHandle = Handle;

                } except(ExSystemExceptionFilter()) {
                }
            }
        }

        //
        // If an exception occurs during the probe of the output handle address,
        // then always handle the exception and return the exception code as the
        // status value.
        //

    } except (ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtOpenEventPair(
    OUT PHANDLE EventPairHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to an event pair object with the specified
    desired access.

Arguments:

    EventPairHandle - Supplies a pointer to a variable that will receive
        the event pair object handle.

    DesiredAccess - Supplies the desired types of access for the event
        pair object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

Return Value:

    TBS

--*/

{

    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to open the event object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object open routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(EventPairHandle);
        }

        //
        // Open handle to the event pair object with the specified
        // desired access.
        //

        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExEventPairObjectType,
                                    PreviousMode,
                                    NULL,
                                    DesiredAccess,
                                    NULL,
                                    &Handle);


        //
        // If the open was successful, then attempt to write the event
        // pair object handle value.  If the write attempt fails, then do
        // not report an error.  When the caller attempts to access the
        // handle value, an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                *EventPairHandle = Handle;

            } except(ExSystemExceptionFilter()) {
            }
        }

        //
        // If an exception occurs during the probe of the output event handle,
        // then always handle the exception and return the exception code as the
        // status value.
        //

    } except (ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtWaitLowEventPair(
    IN HANDLE EventPairHandle
    )

/*++

Routine Description:

    This function waits on the low event of an event pair object.

Arguments:

    EventPairHandle - Supplies a handle to an event pair object.

Return Value:

    TBS

--*/

{

    PEEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Reference event pair object by handle.
    //

    PreviousMode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID *)&EventPair,
                                       NULL);

    //
    // If the reference was successful, then wait on the Low event
    // of the event pair.
    //

    if (NT_SUCCESS(Status)) {
        Status = KeWaitForLowEventPair(&EventPair->KernelEventPair,
                                       PreviousMode,
                                       FALSE,
                                       NULL);

        ObDereferenceObject(EventPair);
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtWaitHighEventPair(
    IN HANDLE EventPairHandle
    )

/*++

Routine Description:

    This function waits on the high event of an event pair object.

Arguments:

    EventPairHandle - Supplies a handle to an event pair object.

Return Value:

    TBS

--*/

{

    PEEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Reference event pair object by handle.
    //

    PreviousMode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID *)&EventPair,
                                       NULL);

    //
    // If the reference was successful, then wait on the Low event
    // of the event pair.
    //

    if (NT_SUCCESS(Status)) {
        Status = KeWaitForHighEventPair(&EventPair->KernelEventPair,
                                        PreviousMode,
                                        FALSE,
                                        NULL);

        ObDereferenceObject(EventPair);
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtSetLowWaitHighEventPair(
    IN HANDLE EventPairHandle
    )

/*++

Routine Description:

    This function sets the low event of an event pair and then
    waits on the high event of an event pair object.

Arguments:

    EventPairHandle - Supplies a handle to an event pair object.

Return Value:

    TBS

--*/

{

    PEEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Reference event pair object by handle.
    //

    PreviousMode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID *)&EventPair,
                                       NULL);

    //
    // If the reference was successful, then wait on the Low event
    // of the event pair.
    //

    if (NT_SUCCESS(Status)) {
        EvPrSetLow++;
        Status = KeSetLowWaitHighEventPair(&EventPair->KernelEventPair,
                                           PreviousMode);

        ObDereferenceObject(EventPair);
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtSetHighWaitLowEventPair(
    IN HANDLE EventPairHandle
    )

/*++

Routine Description:

    This function sets the high event of an event pair and then
    waits on the low event of an event pair object.

Arguments:

    EventPairHandle - Supplies a handle to an event pair object.

Return Value:

    TBS

--*/

{

    PEEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Reference event pair object by handle.
    //

    PreviousMode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID *)&EventPair,
                                       NULL);

    //
    // If the reference was successful, then wait on the Low event
    // of the event pair.
    //

    if (NT_SUCCESS(Status)) {
        EvPrSetHigh++;
        Status = KeSetHighWaitLowEventPair(&EventPair->KernelEventPair,
                                           PreviousMode);

        ObDereferenceObject(EventPair);
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtSetLowEventPair(
    IN HANDLE EventPairHandle
    )

/*++

Routine Description:

    This function sets the low event of an event pair object.

Arguments:

    EventPairHandle - Supplies a handle to an event pair object.

Return Value:

    TBS

--*/

{

    PEEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Reference event pair object by handle.
    //

    PreviousMode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID *)&EventPair,
                                       NULL);

    //
    // If the reference was successful, then wait on the Low event
    // of the event pair.
    //

    if (NT_SUCCESS(Status)) {
        EvPrSetLow++;
        KeSetLowEventPair(&EventPair->KernelEventPair,
                          EVENT_PAIR_INCREMENT,FALSE);

        ObDereferenceObject(EventPair);
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtSetHighEventPair(
    IN HANDLE EventPairHandle
    )

/*++

Routine Description:

    This function sets the high event of an event pair object.

Arguments:

    EventPairHandle - Supplies a handle to an event pair object.

Return Value:

    TBS

--*/

{

    PEEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Reference event pair object by handle.
    //

    PreviousMode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID *)&EventPair,
                                       NULL);

    //
    // If the reference was successful, then wait on the Low event
    // of the event pair.
    //

    if (NT_SUCCESS(Status)) {
        EvPrSetHigh++;
        KeSetHighEventPair(&EventPair->KernelEventPair,
                           EVENT_PAIR_INCREMENT,FALSE);

        ObDereferenceObject(EventPair);
    }

    //
    // Return service status.
    //

    return Status;
}
