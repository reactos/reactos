/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    timer.c

Abstract:

   This module implements the executive timer object. Functions are provided
   to create, open, cancel, set, and query timer objects.

Author:

    David N. Cutler (davec) 12-May-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Executive timer object structure definition.
//

typedef struct _ETIMER {
    KTIMER KeTimer;
    KAPC TimerApc;
    KDPC TimerDpc;
    LIST_ENTRY ActiveTimerListEntry;
    KSPIN_LOCK Lock;
    LONG Period;
    BOOLEAN ApcAssociated;
    BOOLEAN WakeTimer;
    LIST_ENTRY WakeTimerListEntry;
} ETIMER, *PETIMER;

//
// List of all timers which are set to wake
//

KSPIN_LOCK ExpWakeTimerListLock;
LIST_ENTRY ExpWakeTimerList;

//
// Address of timer object type descriptor.
//

POBJECT_TYPE ExTimerObjectType;

//
// Structure that describes the mapping of generic access rights to object
// specific access rights for timer objects.
//

GENERIC_MAPPING ExpTimerMapping = {
    STANDARD_RIGHTS_READ |
        TIMER_QUERY_STATE,
    STANDARD_RIGHTS_WRITE |
        TIMER_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    TIMER_ALL_ACCESS
};

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpTimerInitialization)
#pragma alloc_text(PAGE, NtCreateTimer)
#pragma alloc_text(PAGE, NtOpenTimer)
#pragma alloc_text(PAGE, NtQueryTimer)
#pragma alloc_text(PAGELK, ExGetNextWakeTime)
#endif

VOID
ExpTimerApcRoutine (
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )

/*++

Routine Description:

    This function is the special APC routine that is called to remove
    a timer from the current thread's active timer list.

Arguments:

    Apc - Supplies a pointer to the APC object used to invoke this routine.

    NormalRoutine - Supplies a pointer to a pointer to the normal routine
        function that was specified when the APC was initialized.

    NormalContext - Supplies a pointer to a pointer to an arbitrary data
        structure that was specified when the APC was initialized.

    SystemArgument1, SystemArgument2 - Supplies a set of two pointers to
        two arguments that contain untyped data.

Return Value:

    None.

--*/

{

    BOOLEAN Dereference;
    PETHREAD ExThread;
    PETIMER ExTimer;
    KIRQL OldIrql1;

    //
    // Get address of executive timer object and the current thread object.
    //

    ExThread = PsGetCurrentThread();
    ExTimer = CONTAINING_RECORD(Apc, ETIMER, TimerApc);

    //
    // If the timer is still in the current thread's active timer list, then
    // remove it if it is not a periodic timer and set APC associated FALSE.
    // It is possible for the timer not to be in the current thread's active
    // timer list since the APC could have been delivered, and then another
    // thread could have set the timer again with another APC. This would
    // have caused the timer to be removed from the current thread's active
    // timer list.
    //
    // N. B. The spin locks for the timer and the active timer list must be
    //  acquired in the order: 1) timer lock, 2) thread list lock.
    //

    Dereference = FALSE;
    ExAcquireSpinLock(&ExTimer->Lock, &OldIrql1);
    ExAcquireSpinLockAtDpcLevel(&ExThread->ActiveTimerListLock);
    if ((ExTimer->ApcAssociated) && (&ExThread->Tcb == ExTimer->TimerApc.Thread)) {
        if (ExTimer->Period == 0) {
            RemoveEntryList(&ExTimer->ActiveTimerListEntry);
            ExTimer->ApcAssociated = FALSE;
            Dereference = TRUE;
        }

    } else {
        *NormalRoutine = (PKNORMAL_ROUTINE)NULL;
    }

    ExReleaseSpinLockFromDpcLevel(&ExThread->ActiveTimerListLock);
    ExReleaseSpinLock(&ExTimer->Lock, OldIrql1);
    if (Dereference) {
        ObDereferenceObject((PVOID)ExTimer);
    }

    return;
}

VOID
ExpTimerDpcRoutine (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function is the DPC routine that is called when a timer expires that
    has an associated APC routine. Its function is to insert the associated
    APC into the target thread's APC queue.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Supplies a pointer to the executive timer that contains
        the DPC that caused this routine to be executed.

    SystemArgument1, SystemArgument2 - Supplies values that are not used by
        this routine.

Return Value:

    None.

--*/

{

    PETIMER ExTimer;
    PKTIMER KeTimer;
    KIRQL OldIrql;

    //
    // Get address of executive and kernel timer objects.
    //

    ExTimer = (PETIMER)DeferredContext;
    KeTimer = &ExTimer->KeTimer;

    //
    // If there is still an APC associated with the timer, then insert the APC
    // in target thread's APC queue. It is possible that the timer does not
    // have an associated APC. This can happen when the timer is set to expire
    // by a thread running on another processor just after the DPC has been
    // removed from the DPC queue, but before it has acquired the timer related
    // spin lock.
    //

    ExAcquireSpinLock(&ExTimer->Lock, &OldIrql);
    if (ExTimer->ApcAssociated) {
        KeInsertQueueApc(&ExTimer->TimerApc,
                         SystemArgument1,
                         SystemArgument2,
                         TIMER_APC_INCREMENT);
    }

    ExReleaseSpinLock(&ExTimer->Lock, OldIrql);
    return;
}

static VOID
ExpDeleteTimer (
    IN PVOID Object
    )

/*++

Routine Description:

    This function is the delete routine for timer objects. Its function is
    to cancel the timer and free the spin lock associated with a timer.

Arguments:

    Object - Supplies a pointer to an executive timer object.

Return Value:

    None.

--*/

{
    PETIMER     ExTimer;
    KIRQL       OldIrql;

    ExTimer = (PETIMER) Object;

    //
    // Remove from wake list
    //

    if (ExTimer->WakeTimerListEntry.Flink) {
        ExAcquireSpinLock(&ExpWakeTimerListLock, &OldIrql);
        if (ExTimer->WakeTimerListEntry.Flink) {
            RemoveEntryList(&ExTimer->WakeTimerListEntry);
            ExTimer->WakeTimerListEntry.Flink = NULL;
        }
        ExReleaseSpinLock(&ExpWakeTimerListLock, OldIrql);
    }

    //
    // Cancel the timer and free the spin lock associated with the timer.
    //

    KeCancelTimer(&ExTimer->KeTimer);
    return;
}

BOOLEAN
ExpTimerInitialization (
    )

/*++

Routine Description:

    This function creates the timer object type descriptor at system
    initialization and stores the address of the object type descriptor
    in local static storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the timer object type descriptor is
    successfully initialized. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    KeInitializeSpinLock (&ExpWakeTimerListLock);
    InitializeListHead (&ExpWakeTimerList);

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Timer");

    //
    // Create timer object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = ExpTimerMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ETIMER);
    ObjectTypeInitializer.ValidAccessMask = TIMER_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = ExpDeleteTimer;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExTimerObjectType);



    //
    // If the time object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}

VOID
ExTimerRundown (
    )

/*++

Routine Description:

    This function is called when a thread is about to be terminated to
    process the active timer list. It is assumed that APC's have been
    disabled for the subject thread, thus this code cannot be interrupted
    to execute an APC for the current thread.

Arguments:

    None.

Return Value:

    None.

--*/

{

    BOOLEAN Dereference;
    PETHREAD ExThread;
    PETIMER ExTimer;
    PLIST_ENTRY NextEntry;
    KIRQL OldIrql1;

    //
    // Process each entry in the active timer list.
    //

    ExThread = PsGetCurrentThread();
    ExAcquireSpinLock(&ExThread->ActiveTimerListLock, &OldIrql1);
    NextEntry = ExThread->ActiveTimerListHead.Flink;
    while (NextEntry != &ExThread->ActiveTimerListHead) {
        ExTimer = CONTAINING_RECORD(NextEntry, ETIMER, ActiveTimerListEntry);

        //
        // Increment the reference count on the object so that it cannot be
        // deleted, and then drop the active timer list lock.
        //
        // N. B. The object reference cannot fail and will acquire no mutexes.
        //

        ObReferenceObject(ExTimer);
        ExReleaseSpinLock(&ExThread->ActiveTimerListLock, OldIrql1);

        //
        // Acquire the timer spin lock and reacquire the active time list spin
        // lock. If the timer is still in the current thread's active timer
        // list, then cancel the timer, remove the timer's DPC from the DPC
        // queue, remove the timer's APC from the APC queue, remove the timer
        // from the thread's active timer list, and set the associate APC
        // flag FALSE.
        //
        // N. B. The spin locks for the timer and the active timer list must be
        //  acquired in the order: 1) timer lock, 2) thread list lock.
        //

        ExAcquireSpinLock(&ExTimer->Lock, &OldIrql1);
        ExAcquireSpinLockAtDpcLevel(&ExThread->ActiveTimerListLock);
        if ((ExTimer->ApcAssociated) && (&ExThread->Tcb == ExTimer->TimerApc.Thread)) {
            RemoveEntryList(&ExTimer->ActiveTimerListEntry);
            ExTimer->ApcAssociated = FALSE;
            KeCancelTimer(&ExTimer->KeTimer);
            KeRemoveQueueDpc(&ExTimer->TimerDpc);
            KeRemoveQueueApc(&ExTimer->TimerApc);
            Dereference = TRUE;

        } else {
            Dereference = FALSE;
        }

        ExReleaseSpinLockFromDpcLevel(&ExThread->ActiveTimerListLock);
        ExReleaseSpinLock(&ExTimer->Lock, OldIrql1);
        if (Dereference) {
            ObDereferenceObject((PVOID)ExTimer);
        }

        ObDereferenceObject((PVOID)ExTimer);

        //
        // Raise IRQL to DISPATCH_LEVEL and reacquire active timer list
        // spin lock.
        //

        ExAcquireSpinLock(&ExThread->ActiveTimerListLock, &OldIrql1);
        NextEntry = ExThread->ActiveTimerListHead.Flink;
    }

    ExReleaseSpinLock(&ExThread->ActiveTimerListLock, OldIrql1);
    return;
}

NTSTATUS
NtCreateTimer (
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
    IN TIMER_TYPE TimerType
    )

/*++

Routine Description:

    This function creates an timer object and opens a handle to the object with
    the specified desired access.

Arguments:

    TimerHandle - Supplies a pointer to a variable that will receive the
        timer object handle.

    DesiredAccess - Supplies the desired types of access for the timer object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

    TimerType - Supplies the type of the timer (autoclearing or notification).

Return Value:

    TBS

--*/

{

    PETIMER ExTimer;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to create a timer object. If the probe fails, then return the
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
            ProbeForWriteHandle(TimerHandle);
        }

        //
        // Check argument validity.
        //

        if ((TimerType != NotificationTimer) &&
            (TimerType != SynchronizationTimer)) {
            return STATUS_INVALID_PARAMETER_4;
        }

        //
        // Allocate timer object.
        //

        Status = ObCreateObject(PreviousMode,
                                ExTimerObjectType,
                                ObjectAttributes,
                                PreviousMode,
                                NULL,
                                sizeof(ETIMER),
                                0,
                                0,
                                (PVOID *)&ExTimer);

        //
        // If the timer object was successfully allocated, then initialize the
        // timer object and attempt to insert the time object in the current
        // process' handle table.
        //

        if (NT_SUCCESS(Status)) {
            KeInitializeDpc(&ExTimer->TimerDpc,
                            ExpTimerDpcRoutine,
                            (PVOID)ExTimer);

            KeInitializeTimerEx(&ExTimer->KeTimer, TimerType);
            KeInitializeSpinLock(&ExTimer->Lock);
            ExTimer->ApcAssociated = FALSE;
            ExTimer->WakeTimer = FALSE;
            ExTimer->WakeTimerListEntry.Flink = NULL;
            Status = ObInsertObject((PVOID)ExTimer,
                                    NULL,
                                    DesiredAccess,
                                    0,
                                    (PVOID *)NULL,
                                    &Handle);

            //
            // If the timer object was successfully inserted in the current
            // process' handle table, then attempt to write the timer object
            // handle value. If the write attempt fails, then do not report
            // an error. When the caller attempts to access the handle value,
            // an access violation will occur.
            //

            if (NT_SUCCESS(Status)) {
                try {
                    *TimerHandle = Handle;
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
NtOpenTimer (
    OUT PHANDLE TimerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to an timer object with the specified
    desired access.

Arguments:

    TimerHandle - Supplies a pointer to a variable that will receive the
        timer object handle.

    DesiredAccess - Supplies the desired types of access for the timer object.

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
    // attempt to open a timer object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the open object routine.
    //

    try {

        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(TimerHandle);
        }

        //
        // Open handle to the timer object with the specified desired access.
        //

        Status = ObOpenObjectByName(ObjectAttributes,
                                    ExTimerObjectType,
                                    PreviousMode,
                                    NULL,
                                    DesiredAccess,
                                    NULL,
                                    &Handle);

        //
        // If the open was successful, then attempt to write the timer object
        // handle value. If the write attempt fails, then do not report an
        // error. When the caller attempts to access the handle value, an
        // access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                *TimerHandle = Handle;

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
NtCancelTimer (
    IN HANDLE TimerHandle,
    OUT PBOOLEAN CurrentState OPTIONAL
    )

/*++

Routine Description:

    This function cancels a timer object.

Arguments:

    TimerHandle - Supplies a handle to an timer object.

    CurrentState - Supplies an optional pointer to a variable that will
        receive the current state of the timer object.

Return Value:

    TBS

--*/

{

    BOOLEAN Dereference;
    PETHREAD ExThread;
    PETIMER ExTimer;
    KIRQL OldIrql1;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN State;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the current state address if
    // specified, reference the timer object, and cancel the timer object.
    // If the probe fails, then return the exception code as the service
    // status. Otherwise return the status value returned by the reference
    // object by handle routine.
    //

    try {

        //
        // Get previous processor mode and probe current state address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if ((PreviousMode != KernelMode) && (ARGUMENT_PRESENT(CurrentState))) {
            ProbeForWriteBoolean(CurrentState);
        }

        //
        // Reference timer object by handle.
        //

        Status = ObReferenceObjectByHandle(TimerHandle,
                                           TIMER_MODIFY_STATE,
                                           ExTimerObjectType,
                                           PreviousMode,
                                           (PVOID *)&ExTimer,
                                           NULL);

        //
        // If the reference was successful, then cancel the timer object,
        // dereference the timer object, and write the current state value
        // if specified. If the write attempt fails, then do not report an
        // error. When the caller attempts to access the current state value,
        // an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            ExAcquireSpinLock(&ExTimer->Lock, &OldIrql1);
            if (ExTimer->ApcAssociated) {
                ExThread = CONTAINING_RECORD(ExTimer->TimerApc.Thread, ETHREAD, Tcb);
                ExAcquireSpinLockAtDpcLevel(&ExThread->ActiveTimerListLock);
                RemoveEntryList(&ExTimer->ActiveTimerListEntry);
                ExTimer->ApcAssociated = FALSE;
                ExReleaseSpinLockFromDpcLevel(&ExThread->ActiveTimerListLock);
                KeCancelTimer(&ExTimer->KeTimer);
                KeRemoveQueueDpc(&ExTimer->TimerDpc);
                KeRemoveQueueApc(&ExTimer->TimerApc);
                Dereference = TRUE;

            } else {
                KeCancelTimer(&ExTimer->KeTimer);
                Dereference = FALSE;
            }

            if (ExTimer->WakeTimerListEntry.Flink) {
                ExAcquireSpinLockAtDpcLevel(&ExpWakeTimerListLock);

                //
                // Check again as ExGetNextWakeTime might have removed it.
                //
                if (ExTimer->WakeTimerListEntry.Flink) {
                    RemoveEntryList(&ExTimer->WakeTimerListEntry);
                    ExTimer->WakeTimerListEntry.Flink = NULL;
                }
                ExReleaseSpinLockFromDpcLevel(&ExpWakeTimerListLock);
            }

            ExReleaseSpinLock(&ExTimer->Lock, OldIrql1);
            if (Dereference) {
                ObDereferenceObject((PVOID)ExTimer);
            }

            //
            // Read current state of timer, dereference timer object, and set
            // current state.
            //

            State = KeReadStateTimer(&ExTimer->KeTimer);
            ObDereferenceObject(ExTimer);
            if (ARGUMENT_PRESENT(CurrentState)) {
                try {
                    *CurrentState = State;

                } except(ExSystemExceptionFilter()) {
                }
            }
        }

    //
    // If an exception occurs during the probe of the current state address,
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
NtQueryTimer (
    IN HANDLE TimerHandle,
    IN TIMER_INFORMATION_CLASS TimerInformationClass,
    OUT PVOID TimerInformation,
    IN ULONG TimerInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function queries the state of an timer object and returns the
    requested information in the specified record structure.

Arguments:

    TimerHandle - Supplies a handle to an timer object.

    TimerInformationClass - Supplies the class of information being requested.

    TimerInformation - Supplies a pointer to a record that is to receive the
        requested information.

    TimerInformationLength - Supplies the length of the record that is to
        receive the requested information.

    ReturnLength - Supplies an optional pointer to a variable that is to
        receive the actual length of information that is returned.

Return Value:

    TBS

--*/

{

    PETIMER ExTimer;
    PKTIMER KeTimer;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN State;
    NTSTATUS Status;
    LARGE_INTEGER TimeToGo;

    //
    // Establish an exception handler, probe the output arguments, reference
    // the timer object, and return the specified information. If the probe
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
            ProbeForWrite(TimerInformation,
                          sizeof(TIMER_BASIC_INFORMATION),
                          sizeof(ULONG));

            if (ARGUMENT_PRESENT(ReturnLength)) {
                ProbeForWriteUlong(ReturnLength);
            }
        }

        //
        // Check argument validity.
        //

        if (TimerInformationClass != TimerBasicInformation) {
            return STATUS_INVALID_INFO_CLASS;
        }

        if (TimerInformationLength != sizeof(TIMER_BASIC_INFORMATION)) {
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Reference timer object by handle.
        //

        Status = ObReferenceObjectByHandle(TimerHandle,
                                           TIMER_QUERY_STATE,
                                           ExTimerObjectType,
                                           PreviousMode,
                                           (PVOID *)&ExTimer,
                                           NULL);

        //
        // If the reference was successful, then read the current state,
        // compute the time remaining, dereference the timer object, fill in
        // the information structure, and return the length of the information
        // structure if specified. If the write of the time information or the
        // return length fails, then do not report an error. When the caller
        // accesses the information structure or the length, an violation will
        // occur.
        //

        if (NT_SUCCESS(Status)) {
            KeTimer = &ExTimer->KeTimer;
            State = KeReadStateTimer(KeTimer);
            KiQueryInterruptTime(&TimeToGo);
            TimeToGo.QuadPart = KeTimer->DueTime.QuadPart - TimeToGo.QuadPart;
            ObDereferenceObject(ExTimer);
            try {
                ((PTIMER_BASIC_INFORMATION)TimerInformation)->TimerState = State;
                ((PTIMER_BASIC_INFORMATION)TimerInformation)->RemainingTime = TimeToGo;
                if (ARGUMENT_PRESENT(ReturnLength)) {
                    *ReturnLength = sizeof(TIMER_BASIC_INFORMATION);
                }

            } except(ExSystemExceptionFilter()) {
            }
        }

    //
    // If an exception occurs during the probe of the current state address,
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
NtSetTimer (
    IN HANDLE TimerHandle,
    IN PLARGE_INTEGER DueTime,
    IN PTIMER_APC_ROUTINE TimerApcRoutine OPTIONAL,
    IN PVOID TimerContext OPTIONAL,
    IN BOOLEAN WakeTimer,
    IN LONG Period OPTIONAL,
    OUT PBOOLEAN PreviousState OPTIONAL
    )

/*++

Routine Description:

    This function sets an timer object to a Not-Signaled state and sets the timer
    to expire at the specified time.

Arguments:

    TimerHandle - Supplies a handle to an timer object.

    DueTime - Supplies a pointer to absolute of relative time at which the
        timer is to expire.

    TimerApcRoutine - Supplies an optional pointer to a function which is to
        be executed when the timer expires. If this parameter is not specified,
        then the TimerContext parameter is ignored.

    TimerContext - Supplies an optional pointer to an arbitrary data structure
        that will be passed to the function specified by the TimerApcRoutine
        parameter. This parameter is ignored if the TimerApcRoutine parameter
        is not specified.

    WakeTimer - Supplies a boolean value that specifies whether the timer
        wakes computer operation if sleeping

    Period - Supplies an optional repetitive period for the timer.

    PreviousState - Supplies an optional pointer to a variable that will
        receive the previous state of the timer object.

Return Value:

    TBS

--*/

{

    BOOLEAN AssociatedApc;
    BOOLEAN Dereference;
    PETHREAD ExThread;
    PETIMER ExTimer;
    LARGE_INTEGER ExpirationTime;
    KIRQL OldIrql1;
    KPROCESSOR_MODE PreviousMode;
    BOOLEAN State;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the due time and previous state
    // address if specified, reference the timer object, and set the timer
    // object. If the probe fails, then return the exception code as the
    // service status. Otherwise return the status value returned by the
    // reference object by handle routine.
    //

    try {

        //
        // Get previous processor mode and probe previous state address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            if (ARGUMENT_PRESENT(PreviousState)) {
                ProbeForWriteBoolean(PreviousState);
            }

            ProbeForRead(DueTime, sizeof(LARGE_INTEGER), sizeof(ULONG));
        }

        //
        // Check argument validity.
        //

        if (Period < 0) {
            return STATUS_INVALID_PARAMETER_6;
        }

        //
        // Capture the expiration time.
        //

        ExpirationTime = *DueTime;

        //
        // Reference timer object by handle.
        //

        Status = ObReferenceObjectByHandle(TimerHandle,
                                           TIMER_MODIFY_STATE,
                                           ExTimerObjectType,
                                           PreviousMode,
                                           (PVOID *)&ExTimer,
                                           NULL);

        //
        // If this WakeTimer flag is set, return the appropiate informational
        // success status code.
        //

        if (NT_SUCCESS(Status) && WakeTimer && !PoWakeTimerSupported()) {
            Status = STATUS_TIMER_RESUME_IGNORED;
        }

        //
        // If the reference was successful, then cancel the timer object, set
        // the timer object, dereference time object, and write the previous
        // state value if specified. If the write of the previous state value
        // fails, then do not report an error. When the caller attempts to
        // access the previous state value, an access violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            ExAcquireSpinLock(&ExTimer->Lock, &OldIrql1);

            if (ExTimer->ApcAssociated) {
                ExThread = CONTAINING_RECORD(ExTimer->TimerApc.Thread, ETHREAD, Tcb);
                ExAcquireSpinLockAtDpcLevel(&ExThread->ActiveTimerListLock);
                RemoveEntryList(&ExTimer->ActiveTimerListEntry);
                ExTimer->ApcAssociated = FALSE;
                ExReleaseSpinLockFromDpcLevel(&ExThread->ActiveTimerListLock);
                KeCancelTimer(&ExTimer->KeTimer);
                KeRemoveQueueDpc(&ExTimer->TimerDpc);
                KeRemoveQueueApc(&ExTimer->TimerApc);
                Dereference = TRUE;

            } else {
                KeCancelTimer(&ExTimer->KeTimer);
                Dereference = FALSE;
            }

            //
            // Read the current state of the timer.
            //

            State = KeReadStateTimer(&ExTimer->KeTimer);

            //
            // If this is a wake timer ensure it's on the wake timer list
            //

            ExTimer->WakeTimer = WakeTimer;
            ExAcquireSpinLockAtDpcLevel(&ExpWakeTimerListLock);
            if (WakeTimer) {
                if (!ExTimer->WakeTimerListEntry.Flink) {
                    InsertTailList(&ExpWakeTimerList, &ExTimer->WakeTimerListEntry);
                }
            } else {
                if (ExTimer->WakeTimerListEntry.Flink) {
                    RemoveEntryList(&ExTimer->WakeTimerListEntry);
                    ExTimer->WakeTimerListEntry.Flink = NULL;
                }
            }
            ExReleaseSpinLockFromDpcLevel(&ExpWakeTimerListLock);

            //
            // If an APC routine is specified, then initialize the APC, acquire the
            // thread's active time list lock, insert the timer in the thread's
            // active timer list, set the timer with an associated DPC, and set the
            // associated APC flag TRUE. Otherwise set the timer without an associated
            // DPC, and set the associated APC flag FALSE.
            //

            ExTimer->Period = Period;
            if (ARGUMENT_PRESENT(TimerApcRoutine)) {
                ExThread = PsGetCurrentThread();
                KeInitializeApc(&ExTimer->TimerApc,
                                &ExThread->Tcb,
                                CurrentApcEnvironment,
                                ExpTimerApcRoutine,
                                (PKRUNDOWN_ROUTINE)NULL,
                                (PKNORMAL_ROUTINE)TimerApcRoutine,
                                PreviousMode,
                                TimerContext);

                ExAcquireSpinLockAtDpcLevel(&ExThread->ActiveTimerListLock);
                InsertTailList(&ExThread->ActiveTimerListHead,
                               &ExTimer->ActiveTimerListEntry);

                ExTimer->ApcAssociated = TRUE;
                ExReleaseSpinLockFromDpcLevel(&ExThread->ActiveTimerListLock);
                KeSetTimerEx(&ExTimer->KeTimer,
                             ExpirationTime,
                             Period,
                             &ExTimer->TimerDpc);

                AssociatedApc = TRUE;

            } else {
                KeSetTimerEx(&ExTimer->KeTimer,
                             ExpirationTime,
                             Period,
                             NULL);

                AssociatedApc = FALSE;
            }

            ExReleaseSpinLock(&ExTimer->Lock, OldIrql1);

            //
            // Dereference the object as appropriate.
            //

            if (Dereference) {
                ObDereferenceObject((PVOID)ExTimer);
            }

            if (AssociatedApc == FALSE) {
                ObDereferenceObject((PVOID)ExTimer);
            }

            if (ARGUMENT_PRESENT(PreviousState)) {
                try {
                    *PreviousState = State;

                } except(ExSystemExceptionFilter()) {
                }
            }
        }

    //
    // If an exception occurs during the probe of the current state address,
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


VOID
ExGetNextWakeTime (
    OUT PULONGLONG      DueTime,
    OUT PTIME_FIELDS    TimeFields,
    OUT PVOID           *TimerObject
    )
{
    PLIST_ENTRY     Link;
    PETIMER         ExTimer;
    PETIMER         BestTimer;
    KIRQL           OldIrql;
    ULONGLONG       TimerDueTime;
    ULONGLONG       BestDueTime;
    ULONGLONG       InterruptTime;
    LARGE_INTEGER   SystemTime;
    LARGE_INTEGER   CmosTime;

    ExAcquireSpinLock(&ExpWakeTimerListLock, &OldIrql);
    BestDueTime = 0;
    BestTimer = NULL;
    Link = ExpWakeTimerList.Flink;
    while (Link != &ExpWakeTimerList) {
        ExTimer = CONTAINING_RECORD(Link, ETIMER, WakeTimerListEntry);
        Link = Link->Flink;

        if (ExTimer->WakeTimer) {

            TimerDueTime = KeQueryTimerDueTime(&ExTimer->KeTimer);
            TimerDueTime = 0 - TimerDueTime;

            //
            // Is this timers due time closer?
            //

            if (TimerDueTime > BestDueTime) {
                BestDueTime = TimerDueTime;
                BestTimer = ExTimer;
            }

        } else {

            //
            // Timer is not an active wake timer, remove it
            //

            RemoveEntryList(&ExTimer->WakeTimerListEntry);
            ExTimer->WakeTimerListEntry.Flink = NULL;
        }
    }

    ExReleaseSpinLock(&ExpWakeTimerListLock, OldIrql);

    if (BestDueTime) {
        //
        // Convert time to timefields
        //

        KeQuerySystemTime (&SystemTime);
        InterruptTime = KeQueryInterruptTime ();
        BestDueTime = 0 - BestDueTime;

        SystemTime.QuadPart += BestDueTime - InterruptTime;

        //
        // Many system alarms are only good to 1 second resolution.
        // Add one sceond to the target time so that the timer is really
        // elasped if this is the wake event.
        //

        SystemTime.QuadPart += 10000000;

        ExSystemTimeToLocalTime(&SystemTime,&CmosTime);
        RtlTimeToTimeFields(&CmosTime, TimeFields);
    }

    *DueTime = BestDueTime;
    *TimerObject = BestTimer;
}
