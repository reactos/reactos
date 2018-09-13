/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psspnd.c

Abstract:

    This module implements NtSuspendThread and NtResumeThread

Author:

    Mark Lucovsky (markl) 25-May-1989

Revision History:

--*/

#include "psp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtSuspendThread)
#pragma alloc_text(PAGE, NtResumeThread)
#pragma alloc_text(PAGE, NtAlertThread)
#pragma alloc_text(PAGE, NtAlertResumeThread)
#pragma alloc_text(PAGE, NtTestAlert)
#endif


NTSTATUS
NtSuspendThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    )

/*++

Routine Description:

    This function suspends the target thread, and optionally
    returns the previous suspend count.

Arguments:

    ThreadHandle - Supplies a handle to the thread object to suspend.

    PreviousSuspendCount - An optional parameter, that if specified
        points to a variable that receives the thread's previous suspend
        count.

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    PETHREAD Thread;
    NTSTATUS st;
    ULONG LocalPreviousSuspendCount;
    KPROCESSOR_MODE Mode;

    PAGED_CODE();

    try {

        Mode = KeGetPreviousMode();

        if ( Mode != KernelMode ) {
            if (ARGUMENT_PRESENT(PreviousSuspendCount)) {
                ProbeForWriteUlong(PreviousSuspendCount);
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_SUSPEND_RESUME,
            PsThreadType,
            Mode,
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    try {

        if ( Thread != PsGetCurrentThread() ) {
            if ( Thread->HasTerminated ) {
                ObDereferenceObject(Thread);
                return STATUS_THREAD_IS_TERMINATING;
            }

            LocalPreviousSuspendCount = (ULONG) KeSuspendThread(&Thread->Tcb);

        } else {
            LocalPreviousSuspendCount = (ULONG) KeSuspendThread(&Thread->Tcb);
        }

        ObDereferenceObject(Thread);

        if (ARGUMENT_PRESENT(PreviousSuspendCount))
            *PreviousSuspendCount = LocalPreviousSuspendCount;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        st = GetExceptionCode();

        //
        // Either the suspend, or the store could cause an
        // exception. The store is a partial success, while the
        // suspend exception is an error
        //

        if ( st == STATUS_SUSPEND_COUNT_EXCEEDED ) {
            ObDereferenceObject(Thread);
        } else {
            st = STATUS_SUCCESS;
        }

        return st;
    }

    return STATUS_SUCCESS;

}

NTSTATUS
NtResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    PETHREAD Thread;
    NTSTATUS st;
    ULONG LocalPreviousSuspendCount;
    KPROCESSOR_MODE Mode;

    PAGED_CODE();

    try {

        Mode = KeGetPreviousMode();

        if ( Mode != KernelMode ) {
            if (ARGUMENT_PRESENT(PreviousSuspendCount))
                ProbeForWriteUlong(PreviousSuspendCount);
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_SUSPEND_RESUME,
            PsThreadType,
            Mode,
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    LocalPreviousSuspendCount = (ULONG) KeResumeThread(&Thread->Tcb);

    ObDereferenceObject(Thread);

    try {
        if (ARGUMENT_PRESENT(PreviousSuspendCount))
            *PreviousSuspendCount = LocalPreviousSuspendCount;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;

}

NTSTATUS
NtAlertThread(
    IN HANDLE ThreadHandle
    )

/*++

Routine Description:

    This function alerts the target thread using the previous mode
    as the mode of the alert.

Arguments:

    ThreadHandle - Supplies an open handle to the thread to be alerted

Return Value:

    TBD

--*/

{
    PETHREAD Thread;
    NTSTATUS st;
    KPROCESSOR_MODE Mode;

    PAGED_CODE();

    Mode = KeGetPreviousMode();

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_ALERT,
            PsThreadType,
            Mode,
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    (VOID) KeAlertThread(&Thread->Tcb,Mode);

    ObDereferenceObject(Thread);

    return STATUS_SUCCESS;

}


NTSTATUS
NtAlertResumeThread(
    IN HANDLE ThreadHandle,
    OUT PULONG PreviousSuspendCount OPTIONAL
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    return-value - Description of conditions needed to return value. - or -
    None.

--*/

{
    PETHREAD Thread;
    NTSTATUS st;
    ULONG LocalPreviousSuspendCount;
    KPROCESSOR_MODE Mode;

    PAGED_CODE();

    try {

        Mode = KeGetPreviousMode();

        if ( Mode != KernelMode ) {
            if (ARGUMENT_PRESENT(PreviousSuspendCount)) {
                ProbeForWriteUlong(PreviousSuspendCount);
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {

        return GetExceptionCode();
    }

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_SUSPEND_RESUME,
            PsThreadType,
            Mode,
            (PVOID *)&Thread,
            NULL
            );

    if ( !NT_SUCCESS(st) ) {
        return st;
    }

    LocalPreviousSuspendCount = (ULONG) KeAlertResumeThread(&Thread->Tcb);

    ObDereferenceObject(Thread);

    try {

        if (ARGUMENT_PRESENT(PreviousSuspendCount))
            *PreviousSuspendCount = LocalPreviousSuspendCount;

    } except (EXCEPTION_EXECUTE_HANDLER) {

        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NtTestAlert(
    VOID
    )

/*++

Routine Description:

    This function tests the alert flag inside the current thread. If
    an alert is pending for the previous mode, then the alerted status
    is returned, pending APC's may also be delivered at this time.

Arguments:

    None

Return Value:

    STATUS_ALERTED - An alert was pending for the current thread at the
        time this function was called.

    STATUS_SUCCESS - No alert was pending for this thread.

--*/

{

    PAGED_CODE();

    if ( KeTestAlertThread(KeGetPreviousMode()) ) {
        return STATUS_ALERTED;
    } else {
        return STATUS_SUCCESS;
    }
}
