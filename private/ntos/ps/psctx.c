/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    psctx.c

Abstract:

    This procedure implements Get/Set Context Thread

Author:

    Mark Lucovsky (markl) 25-May-1989

Revision History:

--*/

#include "psp.h"

VOID
PspQueueApcSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtGetContextThread)
#pragma alloc_text(PAGE, NtSetContextThread)
#pragma alloc_text(PAGE, NtQueueApcThread)
#pragma alloc_text(PAGE, PspQueueApcSpecialApc )
#endif

VOID
PspQueueApcSpecialApc(
    IN PKAPC Apc,
    IN PKNORMAL_ROUTINE *NormalRoutine,
    IN PVOID *NormalContext,
    IN PVOID *SystemArgument1,
    IN PVOID *SystemArgument2
    )
{
    PAGED_CODE();

    ExFreePool(Apc);
}

NTSYSAPI
NTSTATUS
NTAPI
NtQueueApcThread(
    IN HANDLE ThreadHandle,
    IN PPS_APC_ROUTINE ApcRoutine,
    IN PVOID ApcArgument1,
    IN PVOID ApcArgument2,
    IN PVOID ApcArgument3
    )

/*++

Routine Description:

    This function is used to queue a user-mode APC to the specified thread. The APC
    will fire when the specified thread does an alertable wait

Arguments:

    ThreadHandle - Supplies a handle to a thread object.  The caller
        must have THREAD_SET_CONTEXT access to the thread.

    ApcRoutine - Supplies the address of the APC routine to execute when the
        APC fires.

    ApcArgument1 - Supplies the first PVOID passed to the APC

    ApcArgument2 - Supplies the second PVOID passed to the APC

    ApcArgument3 - Supplies the third PVOID passed to the APC

Return Value:

    Returns an NT Status code indicating success or failure of the API

--*/

{
    PETHREAD Thread;
    NTSTATUS st;
    KPROCESSOR_MODE Mode;
    KIRQL Irql;
    PKAPC Apc;

    PAGED_CODE();

    Mode = KeGetPreviousMode();

    st = ObReferenceObjectByHandle(
            ThreadHandle,
            THREAD_SET_CONTEXT,
            PsThreadType,
            Mode,
            (PVOID *)&Thread,
            NULL
            );

    if ( NT_SUCCESS(st) ) {
        st = STATUS_SUCCESS;
        if ( IS_SYSTEM_THREAD(Thread) ) {
            st = STATUS_INVALID_HANDLE;
            }
        else {
            Apc = ExAllocatePoolWithQuotaTag(
                    (NonPagedPool | POOL_QUOTA_FAIL_INSTEAD_OF_RAISE),
                    sizeof(*Apc),
                    'pasP'
                    );

            if ( !Apc ) {
                st = STATUS_NO_MEMORY;
                }
            else {
                KeInitializeApc(
                    Apc,
                    &Thread->Tcb,
                    OriginalApcEnvironment,
                    PspQueueApcSpecialApc,
                    NULL,
                    (PKNORMAL_ROUTINE)ApcRoutine,
                    UserMode,
                    ApcArgument1
                    );

                if ( !KeInsertQueueApc(Apc,ApcArgument2,ApcArgument3,0) ) {
                    ExFreePool(Apc);
                    st = STATUS_UNSUCCESSFUL;
                    }
                }
            }
        ObDereferenceObject(Thread);
        }

    return st;
}

NTSTATUS
NtGetContextThread(
    IN HANDLE ThreadHandle,
    IN OUT PCONTEXT ThreadContext
    )

/*++

Routine Description:

    This function returns the usermode context of the specified thread. This
    function will fail if the specified thread is a system thread. It will
    return the wrong answer if the thread is a non-system thread that does
    not execute in user-mode.

Arguments:

    ThreadHandle - Supplies an open handle to the thread object from
                   which to retrieve context information.  The handle
                   must allow THREAD_GET_CONTEXT access to the thread.

    ThreadContext - Supplies the address of a buffer that will receive
                    the context of the specified thread.

Return Value:

    None.

--*/

{

    ULONG Alignment;
    ULONG ContextFlags;
    GETSETCONTEXT ContextFrame;
    ULONG ContextLength;
    KIRQL Irql;
    KPROCESSOR_MODE Mode;
    NTSTATUS Status;
    PETHREAD Thread;

    PAGED_CODE();

    //
    // Get previous mode and reference specified thread.
    //

    Mode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                   THREAD_GET_CONTEXT,
                                   PsThreadType,
                                   Mode,
                                   (PVOID *)&Thread,
                                   NULL);

    //
    // If the reference was successful, the check if the specified thread
    // is a system thread.
    //

    if (NT_SUCCESS(Status)) {

        //
        // If the thread is not a system thread, then attempt to get the
        // context of the thread.
        //

        if (IS_SYSTEM_THREAD(Thread) == FALSE) {

            //
            // Attempt to get the context of the specified thread.
            //

            try {

                //
                // Set the default alignment, capture the context flags,
                // and set the default size of the context record.
                //

                Alignment = CONTEXT_ALIGN;
                ContextFlags = ProbeAndReadUlong(&ThreadContext->ContextFlags);
                ContextLength = sizeof(CONTEXT);

#if defined(_X86_)
                //
                // CONTEXT_EXTENDED_REGISTERS is SET, then we want sizeof(CONTEXT) set above
                // otherwise (not set) we only want the old part of the context record.
                //
                if ((ContextFlags & CONTEXT_EXTENDED_REGISTERS) != CONTEXT_EXTENDED_REGISTERS) {
                    ContextLength = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
                }
#endif

#if defined(_MIPS_)

                //
                // The following code is included for backward compatibility
                // with old code that does not understand extended context
                // records on MIPS systems.
                //

                if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
                    Alignment = sizeof(ULONG);
                    ContextLength = FIELD_OFFSET(CONTEXT, ContextFlags) + 4;
                }

#endif

                if (Mode != KernelMode) {
                    ProbeForWrite(ThreadContext, ContextLength, Alignment);
                }

            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
            }

            //
            // If an exception did not occur during the probe of the thread
            // context, then get the context of the target thread.
            //

            if (NT_SUCCESS(Status)) {
                KeInitializeEvent(&ContextFrame.OperationComplete,
                                  NotificationEvent,
                                  FALSE);

                ContextFrame.Context.ContextFlags = ContextFlags;

                ContextFrame.Mode = Mode;
                if (Thread == PsGetCurrentThread()) {
                    ContextFrame.Apc.SystemArgument1 = NULL;
                    ContextFrame.Apc.SystemArgument2 = Thread;
                    KeRaiseIrql(APC_LEVEL, &Irql);
                    PspGetSetContextSpecialApc(&ContextFrame.Apc,
                                               NULL,
                                               NULL,
                                               &ContextFrame.Apc.SystemArgument1,
                                               &ContextFrame.Apc.SystemArgument2);

                    KeLowerIrql(Irql);

                    //
                    // Move context to specfied context record. If an exception
                    // occurs, then silently handle it and return success.
                    //

                    try {
                        RtlMoveMemory(ThreadContext,
                                      &ContextFrame.Context,
                                      ContextLength);

                    } except(EXCEPTION_EXECUTE_HANDLER) {
                    }

                } else {
                    KeInitializeApc(&ContextFrame.Apc,
                                    &Thread->Tcb,
                                    OriginalApcEnvironment,
                                    PspGetSetContextSpecialApc,
                                    NULL,
                                    NULL,
                                    KernelMode,
                                    NULL);

                    if (!KeInsertQueueApc(&ContextFrame.Apc, NULL, Thread, 2)) {
                        Status = STATUS_UNSUCCESSFUL;

                    } else {
                        KeWaitForSingleObject(&ContextFrame.OperationComplete,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);
                        //
                        // Move context to specfied context record. If an
                        // exception occurs, then silently handle it and
                        // return success.
                        //

                        try {
                            RtlMoveMemory(ThreadContext,
                                          &ContextFrame.Context,
                                          ContextLength);

                        } except(EXCEPTION_EXECUTE_HANDLER) {
                        }
                    }
                }
            }

        } else {
            Status = STATUS_INVALID_HANDLE;
        }

        ObDereferenceObject(Thread);
    }

    return Status;
}

NTSTATUS
NtSetContextThread(
    IN HANDLE ThreadHandle,
    IN PCONTEXT ThreadContext
    )

/*++

Routine Description:

    This function sets the usermode context of the specified thread. This
    function will fail if the specified thread is a system thread. It will
    return the wrong answer if the thread is a non-system thread that does
    not execute in user-mode.

Arguments:

    ThreadHandle - Supplies an open handle to the thread object from
                   which to retrieve context information.  The handle
                   must allow THREAD_SET_CONTEXT access to the thread.

    ThreadContext - Supplies the address of a buffer that contains new
                    context for the specified thread.

Return Value:

    None.

--*/

{

    ULONG Alignment;
    ULONG ContextFlags;
    GETSETCONTEXT ContextFrame;
    ULONG ContextLength;
    KIRQL Irql;
    KPROCESSOR_MODE Mode;
    NTSTATUS Status;
    PETHREAD Thread;

    PAGED_CODE();

    //
    // Get previous mode and reference specified thread.
    //

    Mode = KeGetPreviousMode();
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_CONTEXT,
                                       PsThreadType,
                                       Mode,
                                       (PVOID *)&Thread,
                                       NULL);

    //
    // If the reference was successful, the check if the specified thread
    // is a system thread.
    //

    if (NT_SUCCESS(Status)) {

        //
        // If the thread is not a system thread, then attempt to get the
        // context of the thread.
        //

        if (IS_SYSTEM_THREAD(Thread) == FALSE) {

            //
            // Attempt to get the context of the specified thread.
            //

            try {

                //
                // Set the default alignment, capture the context flags,
                // and set the default size of the context record.
                //

                Alignment = CONTEXT_ALIGN;
                ContextFlags = ProbeAndReadUlong(&ThreadContext->ContextFlags);
                ContextLength = sizeof(CONTEXT);

#if defined(_X86_)
                //
                // CONTEXT_EXTENDED_REGISTERS is SET, then we want sizeof(CONTEXT) set above
                // otherwise (not set) we only want the old part of the context record.
                //
                if ((ContextFlags & CONTEXT_EXTENDED_REGISTERS) != CONTEXT_EXTENDED_REGISTERS) {
                    ContextLength = FIELD_OFFSET(CONTEXT, ExtendedRegisters);
                }
#endif

#if defined(_MIPS_)

                //
                // The following code is included for backward compatibility
                // with old code that does not understand extended context
                // records on MIPS systems.
                //

                if ((ContextFlags & CONTEXT_EXTENDED_INTEGER) != CONTEXT_EXTENDED_INTEGER) {
                    Alignment = sizeof(ULONG);
                    ContextLength = FIELD_OFFSET(CONTEXT, ContextFlags) + 4;
                }

#endif

                if (Mode != KernelMode) {
                    ProbeForRead(ThreadContext, ContextLength, Alignment);
                }

                RtlMoveMemory(&ContextFrame.Context, ThreadContext, ContextLength);

            } except(EXCEPTION_EXECUTE_HANDLER) {
                Status = GetExceptionCode();
            }

            //
            // If an exception did not occur during the probe of the thread
            // context, then set the context of the target thread.
            //

            if (NT_SUCCESS(Status)) {
                KeInitializeEvent(&ContextFrame.OperationComplete,
                                  NotificationEvent,
                                  FALSE);

                ContextFrame.Context.ContextFlags = ContextFlags;

                ContextFrame.Mode = Mode;
                if (Thread == PsGetCurrentThread()) {
                    ContextFrame.Apc.SystemArgument1 = (PVOID)1;
                    ContextFrame.Apc.SystemArgument2 = Thread;
                    KeRaiseIrql(APC_LEVEL, &Irql);
                    PspGetSetContextSpecialApc(&ContextFrame.Apc,
                                               NULL,
                                               NULL,
                                               &ContextFrame.Apc.SystemArgument1,
                                               &ContextFrame.Apc.SystemArgument2);

                    KeLowerIrql(Irql);

                } else {
                    KeInitializeApc(&ContextFrame.Apc,
                                    &Thread->Tcb,
                                    OriginalApcEnvironment,
                                    PspGetSetContextSpecialApc,
                                    NULL,
                                    NULL,
                                    KernelMode,
                                    NULL);

                    if (!KeInsertQueueApc(&ContextFrame.Apc, (PVOID)1, Thread, 2)) {
                        Status = STATUS_UNSUCCESSFUL;

                    } else {
                        KeWaitForSingleObject(&ContextFrame.OperationComplete,
                                              Executive,
                                              KernelMode,
                                              FALSE,
                                              NULL);
                    }
                }
            }

        } else {
            Status = STATUS_INVALID_HANDLE;
        }

        ObDereferenceObject(Thread);
    }

    return Status;
}
