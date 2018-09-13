/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    event.c

Abstract:

   This module implements the executive event object. Functions are provided
   to create, open, set, reset, pulse, and query event objects.

Author:

    David N. Cutler (davec) 8-May-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Temporary so boost is patchable
//

ULONG ExpEventBoost = EVENT_INCREMENT;

//
// Address of event object type descriptor.
//

POBJECT_TYPE ExEventObjectType;

//
// Structure that describes the mapping of generic access rights to object
// specific access rights for event objects.
//

GENERIC_MAPPING ExpEventMapping = {
    STANDARD_RIGHTS_READ |
        EVENT_QUERY_STATE,
    STANDARD_RIGHTS_WRITE |
        EVENT_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    EVENT_ALL_ACCESS
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpEventInitialization)
#pragma alloc_text(PAGE, NtClearEvent)
#pragma alloc_text(PAGE, NtCreateEvent)
#pragma alloc_text(PAGE, NtOpenEvent)
#pragma alloc_text(PAGE, NtPulseEvent)
#pragma alloc_text(PAGE, NtQueryEvent)
#pragma alloc_text(PAGE, NtResetEvent)
#pragma alloc_text(PAGE, NtSetEvent)
#endif

BOOLEAN
ExpEventInitialization (
    )

/*++

Routine Description:

    This function creates the event object type descriptor at system
    initialization and stores the address of the object type descriptor
    in global storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the event object type descriptor is
    successfully initialized. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Event");

    //
    // Create event object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = ExpEventMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KEVENT);
    ObjectTypeInitializer.ValidAccessMask = EVENT_ALL_ACCESS;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExEventObjectType);

    //
    // If the event object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}

NTSTATUS
NtClearEvent (
    IN HANDLE EventHandle
    )

/*++

Routine Description:

    This function sets an event object to a Not-Signaled state.

Arguments:

    EventHandle - Supplies a handle to an event object.

Return Value:

    TBS

--*/

{

    PVOID Event;
    NTSTATUS Status;

    //
    // Reference event object by handle.
    //

    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       KeGetPreviousMode(),
                                       &Event,
                                       NULL);

    //
    // If the reference was successful, then set the state of the event
    // object to Not-Signaled and dereference event object.
    //

    if (NT_SUCCESS(Status)) {
        KeClearEvent((PKEVENT)Event);
        ObDereferenceObject(Event);
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtCreateEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN EVENT_TYPE EventType,
    IN BOOLEAN InitialState
    )

/*++

Routine Description:

    This function creates an event object, sets it initial state to the
    specified value, and opens a handle to the object with the specified
    desired access.

Arguments:

    EventHandle - Supplies a pointer to a variable that will receive the
        event object handle.

    DesiredAccess - Supplies the desired types of access for the event object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

    EventType - Supplies the type of the event (autoclearing or notification).

    InitialState - Supplies the initial state of the event object.

Return Value:

    TBS

--*/

{

    PVOID Event;
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
            ProbeForWriteHandle(EventHandle);
        }

        //
        // Check argument validity.
        //

        if ((EventType != NotificationEvent) && (EventType != SynchronizationEvent)) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Allocate event object.
        //

        Status = ObCreateObject(PreviousMode,
                                ExEventObjectType,
                                ObjectAttributes,
                                PreviousMode,
                                NULL,
                                sizeof(KEVENT),
                                0,
                                0,
                                (PVOID *)&Event);

        //
        // If the event object was successfully allocated, then initialize the
        // event object and attempt to insert the event object in the current
        // process' handle table.
        //

        if (NT_SUCCESS(Status)) {
            KeInitializeEvent((PKEVENT)Event, EventType, InitialState);
            Status = ObInsertObject(Event,
                                    NULL,
                                    DesiredAccess,
                                    0,
                                    (PVOID *)NULL,
                                    &Handle);

            //
            // If the event object was successfully inserted in the current
            // process' handle table, then attempt to write the event object
            // handle value. If the write attempt fails, then do not report
            // an error. When the caller attempts to access the handle value,
            // an access violation will occur.
            //

            if (NT_SUCCESS(Status)) {
                try {
                    *EventHandle = Handle;

                } except(ExSystemExceptionFilter()) {
                }
            }
        }

    //
    // If an exception occurs during the probe of the output handle address,
    // then always handle the exception and return the exception code as the
    // status value.
    //

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtOpenEvent (
    OUT PHANDLE EventHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to an event object with the specified
    desired access.

Arguments:

    EventHandle - Supplies a pointer to a variable that will receive the
        event object handle.

    DesiredAccess - Supplies the desired types of access for the event object.

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
            ProbeForWriteHandle(EventHandle);
        }

        //
        // Open handle to the event object with the specified desired access.
        //

        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExEventObjectType,
                                    PreviousMode,
                                    NULL,
                                    DesiredAccess,
                                    NULL,
                                    &Handle);

        //
        // If the open was successful, then attempt to write the event object
        // handle value. If the write attempt fails, then do not report an
        // error. When the caller attempts to access the handle value, an
        // access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                *EventHandle = Handle;

            } except(ExSystemExceptionFilter()) {
            }
        }

    //
    // If an exception occurs during the probe of the output event handle,
    // then always handle the exception and return the exception code as the
    // status value.
    //

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtPulseEvent (
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
    )

/*++

Routine Description:

    This function sets an event object to a Signaled state, attempts to
    satisfy as many waits as possible, and then resets the state of the
    event object to Not-Signaled.

Arguments:

    EventHandle - Supplies a handle to an event object.

    PreviousState - Supplies an optional pointer to a variable that will
        receive the previous state of the event object.

Return Value:

    TBS

--*/

{

    PVOID Event;
    KPROCESSOR_MODE PreviousMode;
    LONG State;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the previous state address if
    // specified, reference the event object, and pulse the event object. If
    // the probe fails, then return the exception code as the service status.
    // Otherwise return the status value returned by the reference object by
    // handle routine.
    //

    try {

        //
        // Get previous processor mode and probe previous state address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if ((PreviousMode != KernelMode) && (ARGUMENT_PRESENT(PreviousState))) {
            ProbeForWriteLong(PreviousState);
        }

        //
        // Reference event object by handle.
        //

        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           &Event,
                                           NULL);

        //
        // If the reference was successful, then pulse the event object,
        // dereference event object, and write the previous state value if
        // specified. If the write of the previous state fails, then do not
        // report an error. When the caller attempts to access the previous
        // state value, an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            State = KePulseEvent((PKEVENT)Event, ExpEventBoost, FALSE);
            ObDereferenceObject(Event);
            if (ARGUMENT_PRESENT(PreviousState)) {
                try {
                    *PreviousState = State;

                } except(ExSystemExceptionFilter()) {
                }
            }
        }

    //
    // If an exception occurs during the probe of the previous state, then
    // always handle the exception and return the exception code as the status
    // value.
    //

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtQueryEvent (
    IN HANDLE EventHandle,
    IN EVENT_INFORMATION_CLASS EventInformationClass,
    OUT PVOID EventInformation,
    IN ULONG EventInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries the state of an event object and returns the
    requested information in the specified record structure.

Arguments:

    EventHandle - Supplies a handle to an event object.

    EventInformationClass - Supplies the class of information being requested.

    EventInformation - Supplies a pointer to a record that is to receive the
        requested information.

    EventInformationLength - Supplies the length of the record that is to
        receive the requested information.

    ReturnLength - Supplies an optional pointer to a variable that is to
        receive the actual length of information that is returned.

Return Value:

    TBS

--*/

{

    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode;
    LONG State;
    NTSTATUS Status;
    EVENT_TYPE EventType;

    //
    // Check argument validity.
    //

    if (EventInformationClass != EventBasicInformation) {
        return STATUS_INVALID_INFO_CLASS;
    }

    if (EventInformationLength != sizeof(EVENT_BASIC_INFORMATION)) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    // Establish an exception handler, probe the output arguments, reference
    // the event object, and return the specified information. If the probe
    // fails, then return the exception code as the service status. Otherwise
    // return the status value returned by the reference object by handle
    // routine.
    //

    try {

        //
        // Get previous processor mode and probe output arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWrite(EventInformation,
                          sizeof(EVENT_BASIC_INFORMATION),
                          sizeof(ULONG));

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }
        }

        //
        // Reference event object by handle.
        //

        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_QUERY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           (PVOID *)&Event,
                                           NULL);

        //
        // If the reference was successful, then read the current state of
        // the event object, deference event object, fill in the information
        // structure, and return the length of the information structure if
        // specified. If the write of the event information or the return
        // length fails, then do not report an error. When the caller accesses
        // the information structure or length an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            State = KeReadStateEvent(Event);
            EventType = Event->Header.Type;
            ObDereferenceObject(Event);
            try {
                ((PEVENT_BASIC_INFORMATION)EventInformation)->EventType = EventType;
                ((PEVENT_BASIC_INFORMATION)EventInformation)->EventState = State;
                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(EVENT_BASIC_INFORMATION);
                }

            } except(ExSystemExceptionFilter()) {
            }
        }

    //
    // If an exception occurs during the probe of the output arguments, then
    // always handle the exception and return the exception code as the status
    // value.
    //

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtResetEvent (
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
    )

/*++

Routine Description:

    This function sets an event object to a Not-Signaled state.

Arguments:

    EventHandle - Supplies a handle to an event object.

    PreviousState - Supplies an optional pointer to a variable that will
        receive the previous state of the event object.

Return Value:

    TBS

--*/

{

    PVOID Event;
    KPROCESSOR_MODE PreviousMode;
    LONG State;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the previous state address if
    // specified, reference the event object, and reset the event object. If
    // the probe fails, then return the exception code as the service status.
    // Otherwise return the status value returned by the reference object by
    // handle routine.
    //

    try {

        //
        // Get previous processor mode and probe previous state address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if ((PreviousMode != KernelMode) && (ARGUMENT_PRESENT(PreviousState))) {
            ProbeForWriteLong(PreviousState);
        }

        //
        // Reference event object by handle.
        //

        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           &Event,
                                           NULL);

        //
        // If the reference was successful, then set the state of the event
        // object to Not-Signaled, dereference event object, and write the
        // previous state value if specified. If the write of the previous
        // state fails, then do not report an error. When the caller attempts
        // to access the previous state value, an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            State = KeResetEvent((PKEVENT)Event);
            ObDereferenceObject(Event);
            if (ARGUMENT_PRESENT(PreviousState)) {
                try {
                    *PreviousState = State;

                } except(ExSystemExceptionFilter()) {
                }
            }
        }

    //
    // If an exception occurs during the probe of the previous state, then
    // always handle the exception and return the exception code as the status
    // value.
    //

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtSetEvent (
    IN HANDLE EventHandle,
    OUT PLONG PreviousState OPTIONAL
    )

/*++

Routine Description:

    This function sets an event object to a Signaled state and attempts to
    satisfy as many waits as possible.

Arguments:

    EventHandle - Supplies a handle to an event object.

    PreviousState - Supplies an optional pointer to a variable that will
        receive the previous state of the event object.

Return Value:

    TBS

--*/

{

    PVOID Event;
    KPROCESSOR_MODE PreviousMode;
    LONG State;
    NTSTATUS Status;
#if DBG

    //
    // Sneaky trick here to catch sleazy apps (csrss) that erroneously call
    // NtSetEvent on an event that happens to be somebody else's 
    // critical section. Only allow setting a protected handle if the low
    // bit of PreviousState is set.
    //
    OBJECT_HANDLE_INFORMATION HandleInfo;

#endif

    //
    // Establish an exception handler, probe the previous state address if
    // specified, reference the event object, and set the event object. If
    // the probe fails, then return the exception code as the service status.
    // Otherwise return the status value returned by the reference object by
    // handle routine.
    //

    try {

        //
        // Get previous processor mode and probe previous state address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
#if DBG
        if ((PreviousMode != KernelMode) && 
            (ARGUMENT_PRESENT(PreviousState)) &&
            (PreviousState != (PLONG)1)) {
            ProbeForWriteLong(PreviousState);
        }
#else
        if ((PreviousMode != KernelMode) && (ARGUMENT_PRESENT(PreviousState))) {
            ProbeForWriteLong(PreviousState);
        }
#endif

        //
        // Reference event object by handle.
        //

#if DBG
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           &Event,
                                           &HandleInfo);
        if (NT_SUCCESS(Status)) {

            if ((HandleInfo.HandleAttributes & 1) &&
                (PreviousState != (PLONG)1)) {
#if 0
                //
                // This is a protected handle. If the low bit of PreviousState is NOT set,
                // break into the debugger
                //

                DbgPrint("NtSetEvent: Illegal call to NtSetEvent on a protected handle\n");
                DbgBreakPoint();
                PreviousState = NULL;
#endif
            }
        } else {
            if ((KeGetPreviousMode() != KernelMode) &&
                (EventHandle != NULL) &&
                ((NtGlobalFlag & FLG_ENABLE_CLOSE_EXCEPTIONS) ||
                 (PsGetCurrentProcess()->DebugPort != NULL))) {

                Status = KeRaiseUserException(STATUS_INVALID_HANDLE);

            }
        }
#else
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           PreviousMode,
                                           &Event,
                                           NULL);
#endif

        //
        // If the reference was successful, then set the event object to the
        // Signaled state, dereference event object, and write the previous
        // state value if specified. If the write of the previous state fails,
        // then do not report an error. When the caller attempts to access the
        // previous state value, an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            State = KeSetEvent((PKEVENT)Event, ExpEventBoost, FALSE);
            ObDereferenceObject(Event);
            if (ARGUMENT_PRESENT(PreviousState)) {
                try {
                    *PreviousState = State;

                } except(ExSystemExceptionFilter()) {
                }
            }
        }

    //
    // If an exception occurs during the probe of the previous state, then
    // always handle the exception and return the exception code as the status
    // value.
    //

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Return service status.
    //

    return Status;
}
