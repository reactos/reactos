/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    apcsup.c

Abstract:

    This module contains the support routines for the APC object. Functions
    are provided to insert in an APC queue and to deliver user and kernel
    mode APC's.

Author:

    David N. Cutler (davec) 14-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

VOID
KiDeliverApc (
    IN KPROCESSOR_MODE PreviousMode,
    IN PKEXCEPTION_FRAME ExceptionFrame,
    IN PKTRAP_FRAME TrapFrame
    )

/*++

Routine Description:

    This function is called from the APC interrupt code and when one or
    more of the APC pending flags are set at system exit and the previous
    IRQL is zero. All special kernel APC's are delivered first, followed
    by normal kernel APC's if one is not already in progress, and finally
    if the user APC queue is not empty, the user APC pending flag is set,
    and the previous mode is user, then a user APC is delivered. On entry
    to this routine IRQL is set to APC_LEVEL.

    N.B. The exception frame and trap frame addresses are only guaranteed
         to be valid if, and only if, the previous mode is user.

Arguments:

    PreviousMode - Supplies the previous processor mode.

    ExceptionFrame - Supplies a pointer to an exception frame.

    TrapFrame - Supplies a pointer to a trap frame.

Return Value:

    None.

--*/

{

    PKAPC Apc;
    PKKERNEL_ROUTINE KernelRoutine;
    PLIST_ENTRY NextEntry;
    PVOID NormalContext;
    PKNORMAL_ROUTINE NormalRoutine;
    KIRQL OldIrql;
    PVOID SystemArgument1;
    PVOID SystemArgument2;
    PKTHREAD Thread;

    //
    // Raise IRQL to dispatcher level and lock the APC queue.
    //

    Thread = KeGetCurrentThread();
    KiLockApcQueue(Thread, &OldIrql);

    //
    // Get address of current thread object, clear kernel APC pending, and
    // check if any kernel mode APC's can be delivered.
    //

    Thread->ApcState.KernelApcPending = FALSE;
    while (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) {
        NextEntry = Thread->ApcState.ApcListHead[KernelMode].Flink;
        Apc = CONTAINING_RECORD(NextEntry, KAPC, ApcListEntry);
        KernelRoutine = Apc->KernelRoutine;
        NormalRoutine = Apc->NormalRoutine;
        NormalContext = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;
        if (NormalRoutine == (PKNORMAL_ROUTINE)NULL) {

            //
            // First entry in the kernel APC queue is a special kernel APC.
            // Remove the entry from the APC queue, set its inserted state
            // to FALSE, release dispatcher database lock, and call the kernel
            // routine. On return raise IRQL to dispatcher level and lock
            // dispatcher database lock.
            //

            RemoveEntryList(NextEntry);
            Apc->Inserted = FALSE;
            KiUnlockApcQueue(Thread, OldIrql);
            (KernelRoutine)(Apc, &NormalRoutine, &NormalContext,
                            &SystemArgument1, &SystemArgument2);

#if DBG

                if (KeGetCurrentIrql() != OldIrql) {
                    KeBugCheckEx(IRQL_UNEXPECTED_VALUE,
                                 KeGetCurrentIrql() << 16 | OldIrql << 8,
                                 (ULONG_PTR)KernelRoutine,
                                 (ULONG_PTR)Apc,
                                 (ULONG_PTR)NormalRoutine);
                }

#endif

            KiLockApcQueue(Thread, &OldIrql);

        } else {

            //
            // First entry in the kernel APC queue is a normal kernel APC.
            // If there is not a normal kernel APC in progress and kernel
            // APC's are not disabled, then remove the entry from the APC
            // queue, set its inserted state to FALSE, release the APC queue
            // lock, call the specified kernel routine, set kernel APC in
            // progress, lower the IRQL to zero, and call the normal kernel
            // APC routine. On return raise IRQL to dispatcher level, lock
            // the APC queue, and clear kernel APC in progress.
            //

            if ((Thread->ApcState.KernelApcInProgress == FALSE) &&
               (Thread->KernelApcDisable == 0)) {
                RemoveEntryList(NextEntry);
                Apc->Inserted = FALSE;
                KiUnlockApcQueue(Thread, OldIrql);
                (KernelRoutine)(Apc, &NormalRoutine, &NormalContext,
                                &SystemArgument1, &SystemArgument2);

#if DBG

                if (KeGetCurrentIrql() != OldIrql) {
                    KeBugCheckEx(IRQL_UNEXPECTED_VALUE,
                                 KeGetCurrentIrql() << 16 | OldIrql << 8 | 1,
                                 (ULONG_PTR)KernelRoutine,
                                 (ULONG_PTR)Apc,
                                 (ULONG_PTR)NormalRoutine);
                }

#endif

                if (NormalRoutine != (PKNORMAL_ROUTINE)NULL) {
                    Thread->ApcState.KernelApcInProgress = TRUE;
                    KeLowerIrql(0);
                    (NormalRoutine)(NormalContext, SystemArgument1,
                                    SystemArgument2);
                    KeRaiseIrql(APC_LEVEL, &OldIrql);
                }

                KiLockApcQueue(Thread, &OldIrql);
                Thread->ApcState.KernelApcInProgress = FALSE;

            } else {
                KiUnlockApcQueue(Thread, OldIrql);
                return;
            }
        }
    }

    //
    // Kernel APC queue is empty. If the previous mode is user, user APC
    // pending is set, and the user APC queue is not empty, then remove
    // the first entry from the user APC queue, set its inserted state to
    // FALSE, clear user APC pending, release the dispatcher database lock,
    // and call the specified kernel routine. If the normal routine address
    // is not NULL on return from the kernel routine, then initialize the
    // user mode APC context and return. Otherwise, check to determine if
    // another user mode APC can be processed.
    //

    if ((IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]) == FALSE) &&
       (PreviousMode == UserMode) && (Thread->ApcState.UserApcPending == TRUE)) {
        Thread->ApcState.UserApcPending = FALSE;
        NextEntry = Thread->ApcState.ApcListHead[UserMode].Flink;
        Apc = CONTAINING_RECORD(NextEntry, KAPC, ApcListEntry);
        KernelRoutine = Apc->KernelRoutine;
        NormalRoutine = Apc->NormalRoutine;
        NormalContext = Apc->NormalContext;
        SystemArgument1 = Apc->SystemArgument1;
        SystemArgument2 = Apc->SystemArgument2;
        RemoveEntryList(NextEntry);
        Apc->Inserted = FALSE;
        KiUnlockApcQueue(Thread, OldIrql);
        (KernelRoutine)(Apc, &NormalRoutine, &NormalContext,
                        &SystemArgument1, &SystemArgument2);

        if (NormalRoutine == (PKNORMAL_ROUTINE)NULL) {
            KeTestAlertThread(UserMode);

        } else {
            KiInitializeUserApc(ExceptionFrame, TrapFrame, NormalRoutine,
                                NormalContext, SystemArgument1, SystemArgument2);
        }

    } else {
        KiUnlockApcQueue(Thread, OldIrql);
    }

    return;
}

BOOLEAN
FASTCALL
KiInsertQueueApc (
    IN PKAPC Apc,
    IN KPRIORITY Increment
    )

/*++

Routine Description:

    This function inserts an APC object into a thread's APC queue. The address
    of the thread object, the APC queue, and the type of APC are all derived
    from the APC object. If the APC object is already in an APC queue, then
    no opertion is performed and a function value of FALSE is returned. Else
    the APC is inserted in the specified APC queue, its inserted state is set
    to TRUE, and a function value of TRUE is returned. The APC will actually
    be delivered when proper enabling conditions exist.

Arguments:

    Apc - Supplies a pointer to a control object of type APC.

    Increment - Supplies the priority increment that is to be applied if
        queuing the APC causes a thread wait to be satisfied.

Return Value:

    If the APC object is already in an APC queue, then a value of FALSE is
    returned. Else a value of TRUE is returned.

--*/

{

    KPROCESSOR_MODE ApcMode;
    PKAPC ApcEntry;
    PKAPC_STATE ApcState;
    BOOLEAN Inserted;
    PLIST_ENTRY ListEntry;
    PKTHREAD Thread;

    //
    // If the APC object is already in an APC queue, then set inserted to
    // FALSE. Else insert the APC object in the proper queue, set the APC
    // inserted state to TRUE, check to determine if the APC should be delivered
    // immediately, and set inserted to TRUE.
    //
    // For multiprocessor performance, the following code utilizes the fact
    // that kernel APC disable count is incremented before checking whether
    // the kernel APC queue is nonempty.
    //
    // See KeLeaveCriticalRegion().
    //

    Thread = Apc->Thread;
    KiAcquireSpinLock(&Thread->ApcQueueLock);
    if (Apc->Inserted) {
        Inserted = FALSE;

    } else {
        ApcState = Thread->ApcStatePointer[Apc->ApcStateIndex];

        //
        // Insert the APC after all other special APC entries selected by
        // the processor mode if the normal routine value is null. Else
        // insert the APC object at the tail of the APC queue selected by
        // the processor mode unless the APC mode is user and the address
        // of the special APC routine is exit thread, in which case insert
        // the APC at the front of the list and set user APC pending.
        //

        ApcMode = Apc->ApcMode;
        if (Apc->NormalRoutine != NULL) {
            if ((ApcMode != KernelMode) && (Apc->KernelRoutine == PsExitSpecialApc)) {
                Thread->ApcState.UserApcPending = TRUE;
                InsertHeadList(&ApcState->ApcListHead[ApcMode],
                               &Apc->ApcListEntry);

            } else {
                InsertTailList(&ApcState->ApcListHead[ApcMode],
                               &Apc->ApcListEntry);
            }

        } else {
            ListEntry = ApcState->ApcListHead[ApcMode].Flink;
            while (ListEntry != &ApcState->ApcListHead[ApcMode]) {
                ApcEntry = CONTAINING_RECORD(ListEntry, KAPC, ApcListEntry);
                if (ApcEntry->NormalRoutine != NULL) {
                    break;
                }

                ListEntry = ListEntry->Flink;
            }

            ListEntry = ListEntry->Blink;
            InsertHeadList(ListEntry, &Apc->ApcListEntry);
        }

        Apc->Inserted = TRUE;

        //
        // If the APC index from the APC object matches the APC Index of
        // the thread, then check to determine if the APC should interrupt
        // thread execution or sequence the thread out of a wait state.
        //

        if (Apc->ApcStateIndex == Thread->ApcStateIndex) {

            //
            // If the processor mode of the APC is kernel, then check if
            // the APC should either interrupt the thread or sequence the
            // thread out of a Waiting state. Else check if the APC should
            // sequence the thread out of an alertable Waiting state.
            //

            if (ApcMode == KernelMode) {
                Thread->ApcState.KernelApcPending = TRUE;
                if (Thread->State == Running) {
                    KiRequestApcInterrupt(Thread->NextProcessor);

                } else if ((Thread->State == Waiting) &&
                          (Thread->WaitIrql == 0) &&
                          ((Apc->NormalRoutine == NULL) ||
                          ((Thread->KernelApcDisable == 0) &&
                          (Thread->ApcState.KernelApcInProgress == FALSE)))) {
                    KiUnwaitThread(Thread, STATUS_KERNEL_APC, Increment);
                }

            } else if ((Thread->State == Waiting) &&
                      (Thread->WaitMode == UserMode) &&
                      (Thread->Alertable)) {
                Thread->ApcState.UserApcPending = TRUE;
                KiUnwaitThread(Thread, STATUS_USER_APC, Increment);
            }
        }

        Inserted = TRUE;
    }

    //
    // Unlock the APC queue lock, and return whether the APC object was
    // inserted in an APC queue.
    //

    KiReleaseSpinLock(&Thread->ApcQueueLock);
    return Inserted;
}
