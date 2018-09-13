/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psopen.c

Abstract:

    This module implements Process and Thread open.
    This module also contains NtRegisterThreadTerminationPort.

Author:

    Mark Lucovsky (markl) 20-Sep-1989

Revision History:

--*/

#include "psp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtOpenProcess)
#pragma alloc_text(PAGE, NtOpenThread)
#endif

NTSTATUS
NtOpenProcess (
    OUT PHANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    )

/*++

Routine Description:

    This function opens a handle to a process object with the specified
    desired access.

    The object is located either by name, or by locating a thread whose
    Client ID matches the specified Client ID and then opening that thread's
    process.

Arguments:

    ProcessHandle - Supplies a pointer to a variable that will receive
        the process object handle.

    DesiredAccess - Supplies the desired types of access for the process
        object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.
        If the ObjectName field is specified, then ClientId must not be
        specified.

    ClientId - Supplies a pointer to a ClientId that if supplied
        specifies the thread whose process is to be opened. If this
        argument is specified, then ObjectName field of the ObjectAttributes
        structure must not be specified.

Return Value:

    TBS

--*/

{

    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PEPROCESS Process;
    PETHREAD Thread;
    CLIENT_ID CapturedCid;
    BOOLEAN ObjectNamePresent;
    BOOLEAN ClientIdPresent;
    ACCESS_STATE AccessState;
    AUX_ACCESS_DATA AuxData;
    ULONG Attributes;

    PAGED_CODE();

    //
    // Make sure that only one of either ClientId or ObjectName is
    // present.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        //
        // Since we need to look at the ObjectName field, probe
        // ObjectAttributes and capture object name present indicator.
        //

        try {

            ProbeForWriteHandle(ProcessHandle);

            ProbeForRead(ObjectAttributes,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));
            ObjectNamePresent = (BOOLEAN)ARGUMENT_PRESENT(ObjectAttributes->ObjectName);
            Attributes = ObjectAttributes->Attributes;

            if (ARGUMENT_PRESENT(ClientId)) {
                ProbeForRead(ClientId, sizeof(CLIENT_ID), sizeof(ULONG));
                CapturedCid = *ClientId;
                ClientIdPresent = TRUE;
                }
            else {
                ClientIdPresent = FALSE;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }
        }
    else {
        ObjectNamePresent = (BOOLEAN)ARGUMENT_PRESENT(ObjectAttributes->ObjectName);
        Attributes = ObjectAttributes->Attributes;
        if (ARGUMENT_PRESENT(ClientId)) {
            CapturedCid = *ClientId;
            ClientIdPresent = TRUE;
            }
        else {
            ClientIdPresent = FALSE;
            }
        }

    if ( ObjectNamePresent && ClientIdPresent ) {
        return STATUS_INVALID_PARAMETER_MIX;
        }

    //
    // Create an AccessState here, because the caller may have
    // DebugPrivilege, which requires us to make special adjustments
    // to his desired access mask.  We do this by modifying the
    // internal fields in the AccessState to achieve the effect
    // we desire.
    //

    Status = SeCreateAccessState(
                 &AccessState,
                 &AuxData,
                 DesiredAccess,
                 &PsProcessType->TypeInfo.GenericMapping
                 );

    if ( !NT_SUCCESS(Status) ) {

        return Status;
    }

    //
    // Check here to see if the caller has SeDebugPrivilege.  If
    // he does, we will allow him any access he wants to the process.
    // We do this by clearing the DesiredAccess in the AccessState
    // and recording what we want him to have in the PreviouslyGrantedAccess
    // field.
    //
    // Note that this routine performs auditing as appropriate.
    //

    if (SeSinglePrivilegeCheck( SeDebugPrivilege, PreviousMode )) {

        if ( AccessState.RemainingDesiredAccess & MAXIMUM_ALLOWED ) {
            AccessState.PreviouslyGrantedAccess |= PROCESS_ALL_ACCESS;

        } else {

            AccessState.PreviouslyGrantedAccess |= ( AccessState.RemainingDesiredAccess );
        }

        AccessState.RemainingDesiredAccess = 0;

    }

    if ( ObjectNamePresent ) {

        //
        // Open handle to the process object with the specified desired access,
        // set process handle value, and return service completion status.
        //

        Status = ObOpenObjectByName(
                    ObjectAttributes,
                    PsProcessType,
                    PreviousMode,
                    &AccessState,
                    0,
                    NULL,
                    &Handle
                    );

        SeDeleteAccessState( &AccessState );

        if ( NT_SUCCESS(Status) ) {
            try {
                *ProcessHandle = Handle;
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                return Status;
                }
            }

        return Status;
        }

    if ( ClientIdPresent ) {

        Thread = NULL;
        if (CapturedCid.UniqueThread) {
            Status = PsLookupProcessThreadByCid(
                        &CapturedCid,
                        &Process,
                        &Thread
                        );

            if ( !NT_SUCCESS(Status) ) {
                SeDeleteAccessState( &AccessState );
                return Status;
                }
            }
        else {
            Status = PsLookupProcessByProcessId(
                        CapturedCid.UniqueProcess,
                        &Process
                        );

            if ( !NT_SUCCESS(Status) ) {
                SeDeleteAccessState( &AccessState );
                return Status;
                }
            }

        //
        // OpenObjectByAddress
        //

        Status = ObOpenObjectByPointer(
                    Process,
                    Attributes,
                    &AccessState,
                    0,
                    PsProcessType,
                    PreviousMode,
                    &Handle
                    );

        SeDeleteAccessState( &AccessState );

        if ( Thread ) {
            ObDereferenceObject(Thread);
            }
        ObDereferenceObject(Process);

        if ( NT_SUCCESS(Status) ) {

            try {
                *ProcessHandle = Handle;
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                return Status;
                }
            }

        return Status;

        }

    return STATUS_INVALID_PARAMETER_MIX;
}

NTSTATUS
NtOpenThread (
    OUT PHANDLE ThreadHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PCLIENT_ID ClientId OPTIONAL
    )

/*++

Routine Description:

    This function opens a handle to a thread object with the specified
    desired access.

    The object is located either by name, or by locating a thread whose
    Client ID matches the specified Client ID.

Arguments:

    ThreadHandle - Supplies a pointer to a variable that will receive
        the thread object handle.

    DesiredAccess - Supplies the desired types of access for the Thread
        object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.
        If the ObjectName field is specified, then ClientId must not be
        specified.

    ClientId - Supplies a pointer to a ClientId that if supplied
        specifies the thread whose thread is to be opened. If this
        argument is specified, then ObjectName field of the ObjectAttributes
        structure must not be specified.

Return Value:

    TBS

--*/

{

    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PETHREAD Thread;
    CLIENT_ID CapturedCid;
    BOOLEAN ObjectNamePresent;
    BOOLEAN ClientIdPresent;
    ACCESS_STATE AccessState;
    AUX_ACCESS_DATA AuxData;
    ULONG HandleAttributes;

    PAGED_CODE();

    //
    // Make sure that only one of either ClientId or ObjectName is
    // present.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        //
        // Since we need to look at the ObjectName field, probe
        // ObjectAttributes and capture object name present indicator.
        //

        try {

            ProbeForWriteHandle(ThreadHandle);

            ProbeForRead(ObjectAttributes,
                         sizeof(OBJECT_ATTRIBUTES),
                         sizeof(ULONG));
            ObjectNamePresent = (BOOLEAN)ARGUMENT_PRESENT(ObjectAttributes->ObjectName);
            HandleAttributes = ObjectAttributes->Attributes;

            if (ARGUMENT_PRESENT(ClientId)) {
                ProbeForRead(ClientId, sizeof(CLIENT_ID), sizeof(ULONG));
                CapturedCid = *ClientId;
                ClientIdPresent = TRUE;
                }
            else {
                ClientIdPresent = FALSE;
                }
            }
        except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
            }
        }
    else {
        ObjectNamePresent = (BOOLEAN) ARGUMENT_PRESENT(ObjectAttributes->ObjectName);
        HandleAttributes = ObjectAttributes->Attributes;
        if (ARGUMENT_PRESENT(ClientId)) {
            CapturedCid = *ClientId;
            ClientIdPresent = TRUE;
            }
        else {
            ClientIdPresent = FALSE;
            }
        }

    if ( ObjectNamePresent && ClientIdPresent ) {
        return STATUS_INVALID_PARAMETER_MIX;
        }

    Status = SeCreateAccessState(
                 &AccessState,
                 &AuxData,
                 DesiredAccess,
                 &PsProcessType->TypeInfo.GenericMapping
                 );

    if ( !NT_SUCCESS(Status) ) {

        return Status;
    }

    //
    // Check here to see if the caller has SeDebugPrivilege.  If
    // he does, we will allow him any access he wants to the process.
    // We do this by clearing the DesiredAccess in the AccessState
    // and recording what we want him to have in the PreviouslyGrantedAccess
    // field.

    if (SeSinglePrivilegeCheck( SeDebugPrivilege, PreviousMode )) {

        if ( AccessState.RemainingDesiredAccess & MAXIMUM_ALLOWED ) {
            AccessState.PreviouslyGrantedAccess |= THREAD_ALL_ACCESS;

        } else {

            AccessState.PreviouslyGrantedAccess |= ( AccessState.RemainingDesiredAccess );
        }

        AccessState.RemainingDesiredAccess = 0;

    }

    if ( ObjectNamePresent ) {

        //
        // Open handle to the Thread object with the specified desired access,
        // set Thread handle value, and return service completion status.
        //

        Status = ObOpenObjectByName(
                    ObjectAttributes,
                    PsThreadType,
                    PreviousMode,
                    &AccessState,
                    0,
                    NULL,
                    &Handle
                    );

        SeDeleteAccessState( &AccessState );

        if ( NT_SUCCESS(Status) ) {
            try {
                *ThreadHandle = Handle;
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                return Status;
                }
            }
        return Status;
        }

    if ( ClientIdPresent ) {

        if ( CapturedCid.UniqueProcess ) {
            Status = PsLookupProcessThreadByCid(
                        &CapturedCid,
                        NULL,
                        &Thread
                        );

            if ( !NT_SUCCESS(Status) ) {
                SeDeleteAccessState( &AccessState );
                return Status;
                }
            }
        else {
            Status = PsLookupThreadByThreadId(
                        CapturedCid.UniqueThread,
                        &Thread
                        );

            if ( !NT_SUCCESS(Status) ) {
                SeDeleteAccessState( &AccessState );
                return Status;
                }

            }

        Status = ObOpenObjectByPointer(
                    Thread,
                    HandleAttributes,
                    &AccessState,
                    0,
                    PsThreadType,
                    PreviousMode,
                    &Handle
                    );

        SeDeleteAccessState( &AccessState );
        ObDereferenceObject(Thread);

        if ( NT_SUCCESS(Status) ) {

            try {
                *ThreadHandle = Handle;
                }
            except(EXCEPTION_EXECUTE_HANDLER) {
                return Status;
                }
            }

        return Status;

        }

    return STATUS_INVALID_PARAMETER_MIX;
}
