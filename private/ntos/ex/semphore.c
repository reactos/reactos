/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    semphore.c

Abstract:

   This module implements the executive semaphore object. Functions are
   provided to create, open, release, and query semaphore objects.

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

ULONG ExpSemaphoreBoost = SEMAPHORE_INCREMENT;

//
// Address of semaphore object type descriptor.
//

POBJECT_TYPE ExSemaphoreObjectType;

//
// Structure that describes the mapping of generic access rights to object
// specific access rights for semaphore objects.
//

GENERIC_MAPPING ExpSemaphoreMapping = {
    STANDARD_RIGHTS_READ |
        SEMAPHORE_QUERY_STATE,
    STANDARD_RIGHTS_WRITE |
        SEMAPHORE_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    SEMAPHORE_ALL_ACCESS
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpSemaphoreInitialization)
#pragma alloc_text(PAGE, NtCreateSemaphore)
#pragma alloc_text(PAGE, NtOpenSemaphore)
#pragma alloc_text(PAGE, NtQuerySemaphore)
#pragma alloc_text(PAGE, NtReleaseSemaphore)
#endif

BOOLEAN
ExpSemaphoreInitialization (
    )

/*++

Routine Description:

    This function creates the semaphore object type descriptor at system
    initialization and stores the address of the object type descriptor
    in local static storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the semaphore object type descriptor is
    successfully created. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Semaphore");

    //
    // Create semaphore object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = ExpSemaphoreMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KSEMAPHORE);
    ObjectTypeInitializer.ValidAccessMask = SEMAPHORE_ALL_ACCESS;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExSemaphoreObjectType);

    //
    // If the semaphore object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}

NTSTATUS
NtCreateSemaphore (
    IN PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN LONG InitialCount,
    IN LONG MaximumCount
    )

/*++

Routine Description:

    This function creates a semaphore object, sets its initial count to the
    specified value, sets its maximum count to the specified value, and opens
    a handle to the object with the specified desired access.

Arguments:

    SemaphoreHandle - Supplies a pointer to a variable that will receive the
        semaphore object handle.

    DesiredAccess - Supplies the desired types of access for the semaphore
        object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

    InitialCount - Supplies the initial count of the semaphore object.

    MaximumCount - Supplies the maximum count of the semaphore object.

Return Value:

    TBS

--*/

{

    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    PVOID Semaphore;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to create a semaphore object. If the probe fails, then return
    // the exception code as the service status. Otherwise return the status
    // value returned by the object insertion routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(SemaphoreHandle);
        }

        //
        // Check argument validity.
        //

        if ((MaximumCount <= 0) || (InitialCount < 0) ||
           (InitialCount > MaximumCount)) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Allocate semaphore object.
        //

        Status = ObCreateObject(PreviousMode,
                                ExSemaphoreObjectType,
                                ObjectAttributes,
                                PreviousMode,
                                NULL,
                                sizeof(KSEMAPHORE),
                                0,
                                0,
                                (PVOID *)&Semaphore);

        //
        // If the semaphore object was successfully allocated, then initialize
        // the semaphore object and attempt to insert the semaphore object in
        // the current process' handle table.
        //

        if (NT_SUCCESS(Status)) {
            KeInitializeSemaphore((PKSEMAPHORE)Semaphore,
                                  InitialCount,
                                  MaximumCount);

            Status = ObInsertObject(Semaphore,
                                    NULL,
                                    DesiredAccess,
                                    0,
                                    (PVOID *)NULL,
                                    &Handle);

            //
            // If the semaphore object was successfully inserted in the current
            // process' handle table, then attempt to write the semaphore handle
            // value. If the write attempt fails, then do not report an error.
            // When the caller attempts to access the handle value, an access
            // violation will occur.
            //

            if (NT_SUCCESS(Status)) {
                try {
                    *SemaphoreHandle = Handle;
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
NtOpenSemaphore (
    OUT PHANDLE SemaphoreHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to a semaphore object with the specified
    desired access.

Arguments:

    SemaphoreHandle - Supplies a pointer to a variable that will receive the
        semaphore object handle.

    DesiredAccess - Supplies the desired types of access for the semaphore
        object.

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
    // attempt to open a semaphore object. If the probe fails, then return
    // the exception code as the service status. Otherwise return the status
    // value returned by the object open routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(SemaphoreHandle);
        }

        //
        // Open handle to the semaphore object with the specified desired access.
        //

        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExSemaphoreObjectType,
                                    PreviousMode,
                                    NULL,
                                    DesiredAccess,
                                    NULL,
                                    &Handle);

        //
        // If the open was successful, then attempt to write the semaphore
        // object handle value. If the write attempt fails, then do not report
        // an error. When the caller attempts to access the handle value, an
        // access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                *SemaphoreHandle = Handle;
            } except(ExSystemExceptionFilter()) {
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
NtQuerySemaphore (
    IN HANDLE SemaphoreHandle,
    IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
    OUT PVOID SemaphoreInformation,
    IN ULONG SemaphoreInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries the state of a semaphore object and returns the
    requested information in the specified record structure.

Arguments:

    SemaphoreHandle - Supplies a handle to a semaphore object.

    SemaphoreInformationClass - Supplies the class of information being
        requested.

    SemaphoreInformation - Supplies a pointer to a record that is to receive
        the requested information.

    SemaphoreInformationLength - Supplies the length of the record that is
        to receive the requested information.

    ReturnLength - Supplies an optional pointer to a variable that will
        receive the actual length of the information that is returned.

Return Value:

    TBS

--*/

{

    PVOID Semaphore;
    LONG Count;
    LONG Maximum;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output arguments, reference
    // the semaphore object, and return the specified information. If the probe
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
            ProbeForWrite(SemaphoreInformation,
                          sizeof(SEMAPHORE_BASIC_INFORMATION),
                          sizeof(ULONG));

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }
        }

        //
        // Check argument validity.
        //

        if (SemaphoreInformationClass != SemaphoreBasicInformation) {
            return STATUS_INVALID_INFO_CLASS;
        }
        if (SemaphoreInformationLength != sizeof(SEMAPHORE_BASIC_INFORMATION)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Reference semaphore object by handle.
        //

        Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                           SEMAPHORE_QUERY_STATE,
                                           ExSemaphoreObjectType,
                                           PreviousMode,
                                           &Semaphore,
                                           NULL);

        //
        // If the reference was successful, then read the current state and
        // maximum count of the semaphore object, dereference semaphore object,
        // fill in the information structure, and return the length of the
        // information structure if specified. If the write of the semaphore
        // information or the return length fails, then do not report an error.
        // When the caller accesses the information structure or length an
        // access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            Count = KeReadStateSemaphore((PKSEMAPHORE)Semaphore);
            Maximum = ((PKSEMAPHORE)Semaphore)->Limit;
            ObDereferenceObject(Semaphore);
            try {
                ((PSEMAPHORE_BASIC_INFORMATION)SemaphoreInformation)->CurrentCount = Count;
                ((PSEMAPHORE_BASIC_INFORMATION)SemaphoreInformation)->MaximumCount = Maximum;
                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(SEMAPHORE_BASIC_INFORMATION);
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
NtReleaseSemaphore (
    IN HANDLE SemaphoreHandle,
    IN LONG ReleaseCount,
    OUT PLONG PreviousCount OPTIONAL
    )

/*++

Routine Description:

    This function releases a semaphore object by adding the specified release
    count to the current value.

Arguments:

    Semaphore - Supplies a handle to a semaphore object.

    ReleaseCount - Supplies the release count that is to be added to the
        current semaphore count.

    PreviousCount - Supplies an optional pointer to a variable that will
        receive the previous semaphore count.

Return Value:

    TBS

--*/

{

    LONG Count;
    PVOID Semaphore;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the previous count address if
    // specified, reference the semaphore object, and release the semaphore
    // object. If the probe fails, then return the exception code as the
    // service status. Otherwise return the status value returned by the
    // reference object by handle routine.
    //

    try {

        //
        // Get previous processor mode and probe previous count address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if ((PreviousMode != KernelMode) && (ARGUMENT_PRESENT(PreviousCount))) {
            ProbeForWriteLong(PreviousCount);
        }

        //
        // Check argument validity.
        //

        if (ReleaseCount <= 0) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Reference semaphore object by handle.
        //

        Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                           SEMAPHORE_MODIFY_STATE,
                                           ExSemaphoreObjectType,
                                           PreviousMode,
                                           &Semaphore,
                                           NULL);

        //
        // If the reference was successful, then release the semaphore object.
        // If an exception occurs because the maximum count of the semaphore
        // has been exceeded, then dereference the semaphore object and return
        // the exception code as the service status. Otherwise write the previous
        // count value if specified. If the write of the previous count fails,
        // then do not report an error. When the caller attempts to access the
        // previous count value, an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                Count = KeReleaseSemaphore((PKSEMAPHORE)Semaphore,
                                           ExpSemaphoreBoost,
                                           ReleaseCount,
                                           FALSE);

                ObDereferenceObject(Semaphore);
                if (ARGUMENT_PRESENT(PreviousCount)) {
                    try {
                        *PreviousCount = Count;
                    } except(ExSystemExceptionFilter()) {
                    }
                }
            } except(ExSystemExceptionFilter()) {
                ObDereferenceObject(Semaphore);
                return GetExceptionCode();
            }
        }

    //
    // If an exception occurs during the probe of the previous count, then
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
