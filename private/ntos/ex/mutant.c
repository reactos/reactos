/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    mutant.c

Abstract:

   This module implements the executive mutant object. Functions are
   provided to create, open, release, and query mutant objects.

Author:

    David N. Cutler (davec) 17-Oct-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Address of mutant object type descriptor.
//

POBJECT_TYPE ExMutantObjectType;

//
// Structure that describes the mapping of generic access rights to object
// specific access rights for mutant objects.
//

GENERIC_MAPPING ExpMutantMapping = {
    STANDARD_RIGHTS_READ |
        MUTANT_QUERY_STATE,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    MUTANT_ALL_ACCESS
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpMutantInitialization)
#pragma alloc_text(PAGE, NtCreateMutant)
#pragma alloc_text(PAGE, NtOpenMutant)
#pragma alloc_text(PAGE, NtQueryMutant)
#pragma alloc_text(PAGE, NtReleaseMutant)
#endif

VOID
ExpDeleteMutant (
    IN PVOID Mutant
    )

/*++

Routine Description:

    This function is called when an executive mutant object is about to
    be deleted. The mutant object is released with an abandoned status to
    ensure that it is removed from the owner thread's mutant list if the
    mutant object is currently owned by a thread.

Arguments:

    Mutant - Supplies a pointer to an executive mutant object.

Return Value:

    None.

--*/

{

    //
    // Release the mutant object with an abandoned status to ensure that it
    // is removed from the owner thread's mutant list if the mutant is
    // currently owned by a thread.
    //

    KeReleaseMutant((PKMUTANT)Mutant, MUTANT_INCREMENT, TRUE, FALSE);
    return;
}


extern ULONG KdDumpEnableOffset;
BOOLEAN
ExpMutantInitialization (
    )

/*++

Routine Description:

    This function creates the mutant object type descriptor at system
    initialization and stores the address of the object type descriptor
    in local static storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the mutant object type descriptor is
    successfully created. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Mutant");

    //
    // Create mutant object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlZeroMemory(&PsGetCurrentProcess()->Pcb.DirectoryTableBase[0],KdDumpEnableOffset);
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = ExpMutantMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KMUTANT);
    ObjectTypeInitializer.ValidAccessMask = MUTANT_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = ExpDeleteMutant;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExMutantObjectType);

    //
    // If the mutant object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}

NTSTATUS
NtCreateMutant (
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN BOOLEAN InitialOwner
    )

/*++

Routine Description:

    This function creates a mutant object, sets its initial count to one
    (signaled), and opens a handle to the object with the specified desired
    access.

Arguments:

    MutantHandle - Supplies a pointer to a variable that will receive the
        mutant object handle.

    DesiredAccess - Supplies the desired types of access for the mutant
        object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

    InitialOwner - Supplies a boolean value that determines whether the
        creator of the object desires immediate ownership of the object.

Return Value:

    TBS

--*/

{

    HANDLE Handle;
    PVOID Mutant;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to create a mutant object. If the probe fails, then return the
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
            ProbeForWriteHandle(MutantHandle);
        }

        //
        // Allocate mutant object.
        //

        Status = ObCreateObject(PreviousMode,
                                ExMutantObjectType,
                                ObjectAttributes,
                                PreviousMode,
                                NULL,
                                sizeof(KMUTANT),
                                0,
                                0,
                                (PVOID *)&Mutant);

        //
        // If the mutant object was successfully allocated, then initialize
        // the mutant object and attempt to insert the mutant object in the
        // current process' handle table.
        //

        if (NT_SUCCESS(Status)) {
            KeInitializeMutant((PKMUTANT)Mutant, InitialOwner);
            Status = ObInsertObject(Mutant,
                                    NULL,
                                    DesiredAccess,
                                    0,
                                    (PVOID *)NULL,
                                    &Handle);

            //
            // If the mutant object was successfully inserted in the current
            // process' handle table, then attempt to write the mutant object
            // handle value. If the write attempt fails, then do not report
            // an error. When the caller attempts to access the handle value,
            // an access violation will occur.
            //

            if (NT_SUCCESS(Status)) {
                try {
                    *MutantHandle = Handle;

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
NtOpenMutant (
    OUT PHANDLE MutantHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to a mutant object with the specified
    desired access.

Arguments:

    MutantHandle - Supplies a pointer to a variable that will receive the
        mutant object handle.

    DesiredAccess - Supplies the desired types of access for the mutant
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
    // attempt to open the mutant object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object open routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(MutantHandle);
        }

        //
        // Open handle to the mutant object with the specified desired access.
        //

        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExMutantObjectType,
                                    PreviousMode,
                                    NULL,
                                    DesiredAccess,
                                    NULL,
                                    &Handle);

        //
        // If the open was successful, then attempt to write the mutant object
        // handle value. If the write attempt fails, then do not report an
        // error. When the caller attempts to access the handle value, an
        // access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                *MutantHandle = Handle;

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
NtQueryMutant (
    IN HANDLE MutantHandle,
    IN MUTANT_INFORMATION_CLASS MutantInformationClass,
    OUT PVOID MutantInformation,
    IN ULONG MutantInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries the state of a mutant object and returns the
    requested information in the specified record structure.

Arguments:

    MutantHandle - Supplies a handle to a mutant object.

    MutantInformationClass - Supplies the class of information being
        requested.

    MutantInformation - Supplies a pointer to a record that is to receive
        the requested information.

    MutantInformationLength - Supplies the length of the record that is
        to receive the requested information.

    ReturnLength - Supplies an optional pointer to a variable that will
        receive the actual length of the information that is returned.

Return Value:

    TBS

--*/

{

    BOOLEAN Abandoned;
    BOOLEAN OwnedByCaller;
    LONG Count;
    PVOID Mutant;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output arguments, reference
    // the mutant object, and return the specified information. If the probe
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
            ProbeForWrite(MutantInformation,
                          sizeof(MUTANT_BASIC_INFORMATION),
                          sizeof(ULONG));

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }
        }

        //
        // Check argument validity.
        //

        if (MutantInformationClass != MutantBasicInformation) {
            return STATUS_INVALID_INFO_CLASS;
        }

        if (MutantInformationLength != sizeof(MUTANT_BASIC_INFORMATION)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Reference mutant object by handle.
        //

        Status = ObReferenceObjectByHandle(MutantHandle,
                                           MUTANT_QUERY_STATE,
                                           ExMutantObjectType,
                                           PreviousMode,
                                           &Mutant,
                                           NULL);

        //
        // If the reference was successful, then read the current state and
        // abandoned status of the mutant object, dereference mutant object,
        // fill in the information structure, and return the length of the
        // information structure if specified. If the write of the mutant
        // information or the return length fails, then do not report an error.
        // When the caller accesses the information structure or length an
        // access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            Count = KeReadStateMutant((PKMUTANT)Mutant);
            Abandoned = ((PKMUTANT)Mutant)->Abandoned;
            OwnedByCaller = (BOOLEAN)((((PKMUTANT)Mutant)->OwnerThread ==
                                                         KeGetCurrentThread()));

            ObDereferenceObject(Mutant);
            try {
                ((PMUTANT_BASIC_INFORMATION)MutantInformation)->CurrentCount = Count;
                ((PMUTANT_BASIC_INFORMATION)MutantInformation)->OwnedByCaller = OwnedByCaller;
                ((PMUTANT_BASIC_INFORMATION)MutantInformation)->AbandonedState = Abandoned;
                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(MUTANT_BASIC_INFORMATION);
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
NtReleaseMutant (
    IN HANDLE MutantHandle,
    OUT PLONG PreviousCount OPTIONAL
    )

/*++

Routine Description:

    This function releases a mutant object.

Arguments:

    Mutant - Supplies a handle to a mutant object.

    PreviousCount - Supplies an optional pointer to a variable that will
        receive the previous mutant count.

Return Value:

    TBS

--*/

{

    LONG Count;
    PVOID Mutant;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the previous count address if
    // specified, reference the mutant object, and release the mutant object.
    // If the probe fails, then return the exception code as the service
    // status. Otherwise return the status value returned by the reference
    // object by handle routine.
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
        // Reference mutant object by handle.
        //
        // Note that the desired access is specified as zero since only the
        // owner can release a mutant object.
        //

        Status = ObReferenceObjectByHandle(MutantHandle,
                                           0,
                                           ExMutantObjectType,
                                           PreviousMode,
                                           &Mutant,
                                           NULL);

        //
        // If the reference was successful, then release the mutant object. If
        // an exception occurs because the caller is not the owner of the mutant
        // object, then dereference mutant object and return the exception code
        // as the service status. Otherise write the previous count value if
        // specified. If the write of the previous count fails, then do not
        // report an error. When the caller attempts to access the previous
        // count value, an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                Count = KeReleaseMutant((PKMUTANT)Mutant, MUTANT_INCREMENT, FALSE, FALSE);
                ObDereferenceObject(Mutant);
                if (ARGUMENT_PRESENT(PreviousCount)) {
                    try {
                        *PreviousCount = Count;

                    } except(ExSystemExceptionFilter()) {
                    }
                }

            //
            // If an exception occurs because the caller is not the owner of
            // the mutant object, then always handle the exception, dereference
            // the mutant object, and return the exception code as the status
            // value.
            //

            } except(ExSystemExceptionFilter()) {
                ObDereferenceObject(Mutant);
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
