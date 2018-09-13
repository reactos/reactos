/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1992  Microsoft Corporation

Module Name:

    obwait.c

Abstract:

    This module implements the generic wait system services.

Author:

    Steve Wood (stevewo) 12-May-1989

Revision History:

--*/

#include "obp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtWaitForSingleObject)
#endif

//
//  We special case these three object types in the wait routine
//

extern POBJECT_TYPE ExEventObjectType;
extern POBJECT_TYPE ExMutantObjectType;
extern POBJECT_TYPE ExSemaphoreObjectType;


NTSTATUS
NtSignalAndWaitForSingleObject (
    IN HANDLE SignalHandle,
    IN HANDLE WaitHandle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function atomically signals the specified signal object and then
    waits until the specified wait object attains a state of Signaled.  An
    optional timeout can also be specified.  If a timeout is not specified,
    then the wait will not be satisfied until the wait object attains a
    state of Signaled.  If a timeout is specified, and the wait object has
    not attained a state of Signaled when the timeout expires, then the
    wait is automatically satisfied.  If an explicit timeout value of zero
    is specified, then no wait will occur if the wait cannot be satisfied
    immediately.  The wait can also be specified as alertable.

Arguments:

    SignalHandle - Supplies the handle of the signal object.

    WaitHandle  - Supplies the handle for the wait object.

    Alertable - Supplies a boolean value that specifies whether the wait
        is alertable.

    Timeout - Supplies an pointer to an absolute or relative time over
        which the wait is to occur.

Return Value:

    The wait completion status.  A value of STATUS_TIMEOUT is returned if a
    timeout occurred.  A value of STATUS_SUCCESS is returned if the specified
    object satisfied the wait.  A value of STATUS_ALERTED is returned if the
    wait was aborted to deliver an alert to the current thread.  A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{
    OBJECT_HANDLE_INFORMATION HandleInformation;
    KPROCESSOR_MODE PreviousMode;
    PVOID RealObject;
    PVOID SignalObject;
    POBJECT_HEADER SignalObjectHeader;
    NTSTATUS Status;
    LARGE_INTEGER TimeoutValue;
    PVOID WaitObject;
    POBJECT_HEADER WaitObjectHeader;

    //
    //  Establish an exception handler and probe the specified timeout value
    //  if necessary.  If the probe fails, then return the exception code as
    //  the service status.
    //

    PreviousMode = KeGetPreviousMode();

    if ((ARGUMENT_PRESENT(Timeout)) && (PreviousMode != KernelMode)) {

        try {

            TimeoutValue = ProbeAndReadLargeInteger(Timeout);
            Timeout = &TimeoutValue;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();
        }
    }

    //
    //  Reference the signal object by handle.
    //

    Status = ObReferenceObjectByHandle( SignalHandle,
                                        0,
                                        NULL,
                                        PreviousMode,
                                        &SignalObject,
                                        &HandleInformation );

    //
    //  If the reference was successful, then reference the wait object by
    //  handle.
    //

    if (NT_SUCCESS(Status)) {

        Status = ObReferenceObjectByHandle( WaitHandle,
                                            SYNCHRONIZE,
                                            NULL,
                                            PreviousMode,
                                            &WaitObject,
                                            NULL );

        //
        //  If the reference was successful, then determine the real wait
        //  object, check the signal object access, signal the signal object,
        //  and wait for the real wait object.
        //

        if (NT_SUCCESS(Status)) {

            WaitObjectHeader = OBJECT_TO_OBJECT_HEADER(WaitObject);
            RealObject = WaitObjectHeader->Type->DefaultObject;

            if ((LONG_PTR)RealObject >= 0) {

                RealObject = (PVOID)((PCHAR)WaitObject + (ULONG_PTR)RealObject);
            }

            //
            //  If the signal object is an event, then check for modify access
            //  and set the event.  Otherwise, if the signal object is a
            //  mutant, then attempt to release ownership of the mutant.
            //  Otherwise, if the object is a semaphore, then check for modify
            //  access and release the semaphore.  Otherwise, the signal objet
            //  is invalid.
            //

            SignalObjectHeader = OBJECT_TO_OBJECT_HEADER(SignalObject);
            Status = STATUS_ACCESS_DENIED;

            if (SignalObjectHeader->Type == ExEventObjectType) {

                //
                //  Check for access to the specified event object,
                //

                if ((PreviousMode != KernelMode) &&
                    (SeComputeDeniedAccesses( HandleInformation.GrantedAccess,
                                              EVENT_MODIFY_STATE) != 0 )) {

                    goto WaitExit;
                }

                //
                //  Set the specified event and wait atomically.
                //

                KeSetEvent((PKEVENT)SignalObject, EVENT_INCREMENT, TRUE);

            } else if (SignalObjectHeader->Type == ExMutantObjectType) {

                //
                //  Release the specified mutant and wait atomically.
                //
                //  N.B. The release will only be successful if the current
                //       thread is the owner of the mutant.
                //

                try {

                    KeReleaseMutant( (PKMUTANT)SignalObject,
                                     MUTANT_INCREMENT,
                                     FALSE,
                                     TRUE );

                } except(EXCEPTION_EXECUTE_HANDLER) {

                    Status = GetExceptionCode();

                    goto WaitExit;
                }

            } else if (SignalObjectHeader->Type == ExSemaphoreObjectType) {

                //
                //  Check for access to the specified semaphore object,
                //

                if ((PreviousMode != KernelMode) &&
                    (SeComputeDeniedAccesses( HandleInformation.GrantedAccess,
                                              SEMAPHORE_MODIFY_STATE) != 0 )) {

                    goto WaitExit;
                }

                //
                //  Release the specified semaphore and wait atomically.
                //

                try {

                    //
                    //  Release the specified semaphore and wait atomically.
                    //

                    KeReleaseSemaphore( (PKSEMAPHORE)SignalObject,
                                        SEMAPHORE_INCREMENT,
                                        1,
                                        TRUE );

                } except(EXCEPTION_EXECUTE_HANDLER) {

                    Status = GetExceptionCode();

                    goto WaitExit;
                }

            } else {

                Status = STATUS_OBJECT_TYPE_MISMATCH;

                goto WaitExit;
            }

            //
            //  Protect the wait call just in case KeWait decides to raise
            //  For example, a mutant level is exceeded
            //

            try {

                Status = KeWaitForSingleObject( RealObject,
                                                UserRequest,
                                                PreviousMode,
                                                Alertable,
                                                Timeout );

            } except(EXCEPTION_EXECUTE_HANDLER) {

                Status = GetExceptionCode();
            }

WaitExit:

            ObDereferenceObject(WaitObject);
        }

        ObDereferenceObject(SignalObject);
    }

    return Status;
}


NTSTATUS
NtWaitForSingleObject (
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified object attains a state of
    Signaled.  An optional timeout can also be specified.  If a timeout
    is not specified, then the wait will not be satisfied until the
    object attains a state of Signaled.  If a timeout is specified, and
    the object has not attained a state of Signaled when the timeout
    expires, then the wait is automatically satisfied.  If an explicit
    timeout value of zero is specified, then no wait will occur if the
    wait cannot be satisfied immediately.  The wait can also be specified
    as alertable.

Arguments:

    Handle  - Supplies the handle for the wait object.

    Alertable - Supplies a boolean value that specifies whether the wait
        is alertable.

    Timeout - Supplies an pointer to an absolute or relative time over
        which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred.  A value of STATUS_SUCCESS is returned if the specified
    object satisfied the wait.  A value of STATUS_ALERTED is returned if the
    wait was aborted to deliver an alert to the current thread. A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{
    PVOID Object;
    POBJECT_HEADER ObjectHeader;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    LARGE_INTEGER TimeoutValue;
    PVOID WaitObject;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe and capture timeout argument
    //  if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if ((ARGUMENT_PRESENT(Timeout)) && (PreviousMode != KernelMode)) {

        try {

            TimeoutValue = ProbeAndReadLargeInteger(Timeout);
            Timeout = &TimeoutValue;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();
        }
    }

    //
    //  Get a referenced pointer to the specified object with synchronize
    //  access.
    //

    Status = ObReferenceObjectByHandle( Handle,
                                        SYNCHRONIZE,
                                        NULL,
                                        PreviousMode,
                                        &Object,
                                        NULL );

    //
    //  If access is granted, then check to determine if the specified object
    //  can be waited on.
    //

    if (NT_SUCCESS(Status)) {

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
        WaitObject = ObjectHeader->Type->DefaultObject;

        if ((LONG_PTR)WaitObject >= 0) {

            WaitObject = (PVOID)((PCHAR)Object + (ULONG_PTR)WaitObject);
        }

        //
        //  Protect the wait call just in case KeWait decides to raise
        //  For example, a mutant level is exceeded
        //

        try {

            Status = KeWaitForSingleObject( WaitObject,
                                            UserRequest,
                                            PreviousMode,
                                            Alertable,
                                            Timeout );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

        ObDereferenceObject(Object);
    }

    return Status;
}


NTSTATUS
NtWaitForMultipleObjects (
    IN ULONG Count,
    IN HANDLE Handles[],
    IN WAIT_TYPE WaitType,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function waits until the specified objects attain a state of
    Signaled.  The wait can be specified to wait until all of the objects
    attain a state of Signaled or until one of the objects attains a state
    of Signaled.  An optional timeout can also be specified.  If a timeout
    is not specified, then the wait will not be satisfied until the objects
    attain a state of Signaled.  If a timeout is specified, and the objects
    have not attained a state of Signaled when the timeout expires, then
    the wait is automatically satisfied.  If an explicit timeout value of
    zero is specified, then no wait will occur if the wait cannot be satisfied
    immediately.  The wait can also be specified as alertable.

Arguments:

    Count - Supplies a count of the number of objects that are to be waited
        on.

    Handles[] - Supplies an array of handles to wait objects.

    WaitType - Supplies the type of wait to perform (WaitAll, WaitAny).

    Alertable - Supplies a boolean value that specifies whether the wait is
        alertable.

    Timeout - Supplies a pointer to an optional absolute of relative time over
        which the wait is to occur.

Return Value:

    The wait completion status.  A value of STATUS_TIMEOUT is returned if a
    timeout occurred.  The index of the object (zero based) in the object
    pointer array is returned if an object satisfied the wait.  A value of
    STATUS_ALERTED is returned if the wait was aborted to deliver an alert
    to the current thread.  A value of STATUS_USER_APC is returned if the
    wait was aborted to deliver a user APC to the current thread.

--*/

{
    HANDLE CapturedHandles[MAXIMUM_WAIT_OBJECTS];
    ULONG i;
    ULONG j;
    POBJECT_HEADER ObjectHeader;
    PVOID Objects[MAXIMUM_WAIT_OBJECTS];
    KPROCESSOR_MODE PreviousMode;
    ULONG RefCount;
    ULONG Size;
    NTSTATUS Status;
    LARGE_INTEGER TimeoutValue;
    PKWAIT_BLOCK WaitBlockArray;
    ACCESS_MASK GrantedAccess;
    PVOID WaitObjects[MAXIMUM_WAIT_OBJECTS];
    PHANDLE_TABLE HandleTable;
    PHANDLE_TABLE_ENTRY HandleEntry;
    BOOLEAN AttachedToProcess = FALSE;
    BOOLEAN InCriticalRegion = FALSE;
    KAPC_STATE ApcState;

    PAGED_CODE();

    //
    //  Protect ourselves from being interrupted while we hold a handle table
    //  entry lock
    //

    KeEnterCriticalRegion();
    InCriticalRegion = TRUE;

    try {

        //
        //  If the number of objects is zero or greater than the largest number
        //  that can be waited on, then return and invalid parameter status.
        //

        if ((Count == 0) || (Count > MAXIMUM_WAIT_OBJECTS)) {

            Status = STATUS_INVALID_PARAMETER_1;
            leave;
        }

        //
        //  If the wait type is not wait any or wait all, then return an invalid
        //  parameter status.
        //

        if ((WaitType != WaitAny) && (WaitType != WaitAll)) {

            Status = STATUS_INVALID_PARAMETER_3;
            leave;
        }

        //
        //  Get previous processor mode and probe and capture input arguments if
        //  necessary.
        //

        PreviousMode = KeGetPreviousMode();

        try {

            if (PreviousMode != KernelMode) {

                if (ARGUMENT_PRESENT(Timeout)) {

                    TimeoutValue = ProbeAndReadLargeInteger(Timeout);
                    Timeout = &TimeoutValue;
                }

                ProbeForRead( Handles, Count * sizeof(HANDLE), sizeof(HANDLE) );
            }

            i= 0;

            do {

                CapturedHandles[i] = Handles[i];
                i += 1;

            } while (i < Count);

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
            leave;
        }

        //
        //  If the number of objects to be waited on is greater than the number
        //  of builtin wait blocks, then allocate an array of wait blocks from
        //  nonpaged pool. If the wait block array cannot be allocated, then
        //  return insufficient resources.
        //

        WaitBlockArray = NULL;

        if (Count > THREAD_WAIT_OBJECTS) {

            Size = Count * sizeof( KWAIT_BLOCK );
            WaitBlockArray = ExAllocatePoolWithTag(NonPagedPool, Size, 'tiaW');

            if (WaitBlockArray == NULL) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }
        }

        //
        //  Loop through the array of handles and get a referenced pointer to
        //  each object.
        //

        i = 0;
        RefCount = 0;

        Status = STATUS_SUCCESS;

        do {

    #if DBG
            //
            //  On checked builds, check that if the Kernel handle bit is set,
            //  then we're coming from Kernel mode.  We should probably fail the
            //  call if bit set && !Kmode
            //

            if ((CapturedHandles[i] != NtCurrentThread()) &&
                (CapturedHandles[i] != NtCurrentProcess())) {

                ASSERT((CapturedHandles[i] < 0) ? (PreviousMode == KernelMode) : TRUE);
            }
    #endif

            //
            //  Get a pointer to the object table entry.  Check if this is a kernel
            //  handle and if so then use the kernel handle table otherwise use the
            //  processes handle table.  If we are going for a kernel handle we'll
            //  need to attach to the kernel process otherwise we need to ensure
            //  that we aren't attached.
            //

            if (IsKernelHandle( CapturedHandles[i], PreviousMode )) {

                HANDLE KernelHandle;

                //
                //  If we aren't already attached to the system process do so now
                //  and remember that we are
                //

                if (PsGetCurrentProcess() != PsInitialSystemProcess) {
                    KeStackAttachProcess (&PsInitialSystemProcess->Pcb, &ApcState);
                    AttachedToProcess = TRUE;
                }

                //
                //  Decode the user supplied handle into a regular handle value
                //  and get its handle table entry
                //

                KernelHandle = DecodeKernelHandle( CapturedHandles[i] );

                HandleTable = ObpKernelHandleTable;
                HandleEntry = ExMapHandleToPointer( HandleTable, KernelHandle );

            } else {

                //
                //  If we are attached to the system process then go back to the
                //  original callers process
                //

                if (AttachedToProcess) {
                    KeUnstackDetachProcess(&ApcState);
                    AttachedToProcess = FALSE;
                }

                //
                //  Get the handle table entry
                //

                HandleTable = ObpGetObjectTable();
                HandleEntry = ExMapHandleToPointer( HandleTable, CapturedHandles[ i ] );
            }

            //
            //  Make sure the handle really did translate to a valid
            //  entry
            //

            if (HandleEntry != NULL) {

                //
                //  Get the granted access for the handle
                //

    #if i386 && !FPO

                if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

                    if (PreviousMode != KernelMode) {

                        GrantedAccess = ObpTranslateGrantedAccessIndex( HandleEntry->GrantedAccessIndex );
                    }

                } else {

                    GrantedAccess = HandleEntry->GrantedAccess;
                }

    #else
                GrantedAccess = HandleEntry->GrantedAccess;

    #endif // i386 && !FPO

                //
                //  Make sure the handle as synchronize access to the
                //  object
                //

                if ((PreviousMode != KernelMode) &&
                    (SeComputeDeniedAccesses( GrantedAccess, SYNCHRONIZE ) != 0)) {

                    Status = STATUS_ACCESS_DENIED;

                    ExUnlockHandleTableEntry( HandleTable, HandleEntry );

                    goto ServiceFailed;

                } else {

                    //
                    //  We have a object with proper access so get the header
                    //  and if the default objects points to a real object
                    //  then that is the one we're going to wait on.
                    //  Otherwise we'll find the kernel wait object at an
                    //  offset into the object body
                    //

                    ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(HandleEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

                    if ((LONG_PTR)ObjectHeader->Type->DefaultObject < 0) {

                        RefCount += 1;
                        Objects[i] = NULL;
                        WaitObjects[i] = ObjectHeader->Type->DefaultObject;

                    } else {

                        ObpIncrPointerCount( ObjectHeader );
                        RefCount += 1;
                        Objects[i] = &ObjectHeader->Body;

                        //
                        //  Compute the address of the kernel wait object.
                        //

                        WaitObjects[i] = (PVOID)((PCHAR)&ObjectHeader->Body +
                                                 (ULONG_PTR)ObjectHeader->Type->DefaultObject);
                    }
                }

                ExUnlockHandleTableEntry( HandleTable, HandleEntry );

            } else {

                //
                //  The entry in the handle table isn't in use
                //

                Status = STATUS_INVALID_HANDLE;

                goto ServiceFailed;
            }

            i += 1;

        } while (i < Count);

        //
        //  At this point the WaitObjects[] is set to the kernel wait objects
        //
        //  Now Check to determine if any of the objects are specified more than
        //  once, but we only need to check this for wait all, with a wait any
        //  the user can specify the same object multiple times.
        //

        if (WaitType == WaitAll) {

            i = 0;

            do {

                for (j = i + 1; j < Count; j += 1) {

                    if (WaitObjects[i] == WaitObjects[j]) {

                        Status = STATUS_INVALID_PARAMETER_MIX;

                        goto ServiceFailed;
                    }
                }

                i += 1;

            } while (i < Count);
        }

        //
        //  Wait for the specified objects to attain a state of Signaled or a
        //  time out to occur.  Protect the wait call just in case KeWait decides
        //  to raise For example, a mutant level is exceeded
        //

        try {

            InCriticalRegion = FALSE;
            KeLeaveCriticalRegion();
            Status = KeWaitForMultipleObjects( Count,
                                               WaitObjects,
                                               WaitType,
                                               UserRequest,
                                               PreviousMode,
                                               Alertable,
                                               Timeout,
                                               WaitBlockArray );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

        //
        //  If any objects were referenced, then deference them.
        //

    ServiceFailed:

        while (RefCount > 0) {

            RefCount -= 1;

            if (Objects[RefCount] != NULL) {

                ObDereferenceObject(Objects[RefCount]);
            }
        }

        //
        //  If are attached to the system process then return back to our callers
        //  process

        if (AttachedToProcess) {
            KeUnstackDetachProcess(&ApcState);
            AttachedToProcess = FALSE;
        }

        //
        //  If a wait block array was allocated, then deallocate it.
        //

        if (WaitBlockArray != NULL) {

            ExFreePool(WaitBlockArray);
        }

    } finally {

        if ( InCriticalRegion ) {

            KeLeaveCriticalRegion();
        }
    }

    return Status;
}


NTSTATUS
ObWaitForSingleObject (
    IN HANDLE Handle,
    IN BOOLEAN Alertable,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    Please refer to NtWaitForSingleObject

Arguments:

    Handle  - Supplies the handle for the wait object.

    Alertable - Supplies a boolean value that specifies whether the wait
        is alertable.

    Timeout - Supplies an pointer to an absolute or relative time over
        which the wait is to occur.

Return Value:

    The wait completion status. A value of STATUS_TIMEOUT is returned if a
    timeout occurred.  A value of STATUS_SUCCESS is returned if the specified
    object satisfied the wait.  A value of STATUS_ALERTED is returned if the
    wait was aborted to deliver an alert to the current thread. A value of
    STATUS_USER_APC is returned if the wait was aborted to deliver a user
    APC to the current thread.

--*/

{
    POBJECT_HEADER ObjectHeader;
    PVOID Object;
    NTSTATUS Status;
    PVOID WaitObject;

    PAGED_CODE();

    //
    //  Get a referenced pointer to the specified object with synchronize
    //  access.
    //

    Status = ObReferenceObjectByHandle( Handle,
                                        SYNCHRONIZE,
                                        (POBJECT_TYPE)NULL,
                                        KernelMode,
                                        &Object,
                                        NULL );

    //
    //  If access is granted, then check to determine if the specified object
    //  can be waited on.
    //

    if (NT_SUCCESS( Status ) != FALSE) {

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

        if ((LONG_PTR)ObjectHeader->Type->DefaultObject < 0) {

            WaitObject = (PVOID)ObjectHeader->Type->DefaultObject;

        } else {

            WaitObject = (PVOID)((PCHAR)Object + (ULONG_PTR)ObjectHeader->Type->DefaultObject);
        }

        //
        //  Protect the wait call just in case KeWait decides
        //  to raise For example, a mutant level is exceeded
        //

        try {

            Status = KeWaitForSingleObject( WaitObject,
                                            UserRequest,
                                            KernelMode,
                                            Alertable,
                                            Timeout );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

        ObDereferenceObject(Object);
    }

    return Status;
}
