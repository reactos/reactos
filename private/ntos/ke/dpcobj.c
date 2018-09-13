/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dpcobj.c

Abstract:

    This module implements the kernel DPC object. Functions are provided
    to initialize, insert, and remove DPC objects.

Author:

    David N. Cutler (davec) 6-Mar-1989

Environment:

    Kernel mode only.

Revision History:


--*/

#include "ki.h"

//
// The following assert macro is used to check that an input dpc is
// really a kdpc and not something else, like deallocated pool.
//

#define ASSERT_DPC(E) {             \
    ASSERT((E)->Type == DpcObject); \
}

VOID
KeInitializeDpc (
    IN PRKDPC Dpc,
    IN PKDEFERRED_ROUTINE DeferredRoutine,
    IN PVOID DeferredContext
    )

/*++

Routine Description:

    This function initializes a kernel DPC object. The deferred routine
    and context parameter are stored in the DPC object.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredRoutine - Supplies a pointer to a function that is called when
        the DPC object is removed from the current processor's DPC queue.

    DeferredContext - Supplies a pointer to an arbitrary data structure which is
        to be passed to the function specified by the DeferredRoutine parameter.

Return Value:

    None.

--*/

{

    //
    // Initialize standard control object header.
    //

    Dpc->Type = DpcObject;
    Dpc->Number = 0;
    Dpc->Importance = MediumImportance;

    //
    // Initialize deferred routine address and deferred context parameter.
    //

    Dpc->DeferredRoutine = DeferredRoutine;
    Dpc->DeferredContext = DeferredContext;
    Dpc->Lock = NULL;
    return;
}

BOOLEAN
KeInsertQueueDpc (
    IN PRKDPC Dpc,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This function inserts a DPC object into the DPC queue. If the DPC object
    is already in the DPC queue, then no operation is performed. Otherwise,
    the DPC object is inserted in the DPC queue and a dispatch interrupt is
    requested.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    SystemArgument1, SystemArgument2  - Supply a set of two arguments that
        contain untyped data provided by the executive.

Return Value:

    If the DPC object is already in a DPC queue, then a value of FALSE is
    returned. Otherwise a value of TRUE is returned.

--*/

{

    ULONG Index;
    PKSPIN_LOCK Lock;
    KIRQL OldIrql;
    PKPRCB Prcb;
    ULONG Processor;

    ASSERT_DPC(Dpc);

    //
    // Disable interrupts.
    //

    KeRaiseIrql(HIGH_LEVEL, &OldIrql);

    //
    // Acquire the DPC queue lock for the specified target processor.
    //

#if !defined(NT_UP)

    if (Dpc->Number >= MAXIMUM_PROCESSORS) {
        Processor = Dpc->Number - MAXIMUM_PROCESSORS;
        Prcb = KiProcessorBlock[Processor];

    } else {
        Prcb = KeGetCurrentPrcb();
    }

    KiAcquireSpinLock(&Prcb->DpcLock);

#else

    Prcb = KeGetCurrentPrcb();

#endif

    //
    // If the DPC object is not in a DPC queue, then store the system
    // arguments, insert the DPC object in the DPC queue, increment the
    // number of DPCs queued to the target processor, increment the DPC
    // queue depth, set the address of the DPC target DPC spinlock, and
    // request a dispatch interrupt if appropriate.
    //

    if ((Lock = InterlockedCompareExchangePointer(&Dpc->Lock, &Prcb->DpcLock, NULL)) == NULL) {
        Prcb->DpcCount += 1;
        Prcb->DpcQueueDepth += 1;
        Dpc->SystemArgument1 = SystemArgument1;
        Dpc->SystemArgument2 = SystemArgument2;

        //
        // If the DPC is of high importance, then insert the DPC at the
        // head of the DPC queue. Otherwise, insert the DPC at the end
        // of the DPC queue.
        //

        if (Dpc->Importance == HighImportance) {
            InsertHeadList(&Prcb->DpcListHead, &Dpc->DpcListEntry);

        } else {
            InsertTailList(&Prcb->DpcListHead, &Dpc->DpcListEntry);
        }
        //
        // A memory barrier is required here to synchronize with retire DPC
        // list, which clears DpcRoutineActive and DpcInterruptRequested
        // without owning the dispatcher lock.
        //

#if defined(_ALPHA_) && !defined(NT_UP)

        __MB();

#endif

        //
        // If a DPC routine is not active on the target processor, then
        // request a dispatch interrupt if appropriate.
        //

        if ((Prcb->DpcRoutineActive == FALSE) &&
            (Prcb->DpcInterruptRequested == FALSE)) {

            //
            // Request a dispatch interrupt on the current processor if
            // the DPC is not of low importance, the length of the DPC
            // queue has exceeded the maximum threshold, or if the DPC
            // request rate is below the minimum threshold.
            //

#if defined(NT_UP)

            if ((Dpc->Importance != LowImportance) ||
                (Prcb->DpcQueueDepth >= Prcb->MaximumDpcQueueDepth) ||
                (Prcb->DpcRequestRate < Prcb->MinimumDpcRate)) {
                Prcb->DpcInterruptRequested = TRUE;
                KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
            }

            //
            // If the DPC is being queued to another processor and the
            // DPC is of high importance, or the length of the other
            // processor's DPC queue has exceeded the maximum threshold,
            // then request a dispatch interrupt.
            //

#else

            if (Prcb != KeGetCurrentPrcb()) {
                if (((Dpc->Importance == HighImportance) ||
                     (Prcb->DpcQueueDepth >= Prcb->MaximumDpcQueueDepth))) {
                    Prcb->DpcInterruptRequested = TRUE;
                    KiIpiSend((KAFFINITY)(1 << Processor), IPI_DPC);
                }

            } else {

                //
                // Request a dispatch interrupt on the current processor if
                // the DPC is not of low importance, the length of the DPC
                // queue has exceeded the maximum threshold, or if the DPC
                // request rate is below the minimum threshold.
                //

                if ((Dpc->Importance != LowImportance) ||
                    (Prcb->DpcQueueDepth >= Prcb->MaximumDpcQueueDepth) ||
                    (Prcb->DpcRequestRate < Prcb->MinimumDpcRate)) {
                    Prcb->DpcInterruptRequested = TRUE;
                    KiRequestSoftwareInterrupt(DISPATCH_LEVEL);
                }
            }

#endif

        }
     }

     //
     // Release the DPC lock, enable interrupts, and return whether the
     // DPC was queued or not.
     //

#if !defined(NT_UP)

     KiReleaseSpinLock(&Prcb->DpcLock);

#endif

     KeLowerIrql(OldIrql);
     return (Lock == NULL);
}

BOOLEAN
KeRemoveQueueDpc (
    IN PRKDPC Dpc
    )

/*++

Routine Description:

    This function removes a DPC object from the DPC queue. If the DPC object
    is not in the DPC queue, then no operation is performed. Otherwise, the
    DPC object is removed from the DPC queue and its inserted state is set
    FALSE.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

Return Value:

    If the DPC object is not in the DPC queue, then a value of FALSE is
    returned. Otherwise a value of TRUE is returned.

--*/

{

    PKSPIN_LOCK Lock;
    PKPRCB Prcb;

    ASSERT_DPC(Dpc);

    //
    // If the DPC object is in the DPC queue, then remove it from the queue
    // and set its inserted state to FALSE.
    //

    _disable();
    Lock = Dpc->Lock;
    if (Lock != NULL) {

        //
        // Acquire the DPC lock of the target processor.
        //

#if !defined(NT_UP)

        KiAcquireSpinLock(Lock);

#endif

        //
        // If the specified DPC is still in the DPC queue, then remove
        // it.
        //
        // N.B. It is possible for specified DPC to be removed from the
        //      specified DPC queue before the DPC lock is obtained.
        //
        //

        if (Lock == Dpc->Lock) {
            Prcb = CONTAINING_RECORD(Lock, KPRCB, DpcLock);
            Prcb->DpcQueueDepth -= 1;
            RemoveEntryList(&Dpc->DpcListEntry);

#if defined(_ALPHA_) && !defined(NT_UP)

            __MB();

#endif

            Dpc->Lock = NULL;
        }

        //
        // Release the DPC lock of the target processor.
        //

#if !defined(NT_UP)

        KiReleaseSpinLock(Lock);

#endif

    }

    //
    // Enable interrupts and return whether the DPC was removed from a DPC
    // queue.
    //

    _enable();
    return (Lock != NULL);
}

VOID
KeSetImportanceDpc (
    IN PRKDPC Dpc,
    IN KDPC_IMPORTANCE Importance
    )

/*++

Routine Description:

    This function sets the importance of a DPC.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    Number - Supplies the importance of the DPC.

Return Value:

    None.

--*/

{

    //
    // Set the importance of the DPC.
    //

    Dpc->Importance = (UCHAR)Importance;
    return;
}

VOID
KeSetTargetProcessorDpc (
    IN PRKDPC Dpc,
    IN CCHAR Number
    )

/*++

Routine Description:

    This function sets the processor number to which the DPC is targeted.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    Number - Supplies the target processor number.

Return Value:

    None.

--*/

{

    //
    // Set target processor number.
    //
    // The target processor number if biased by the maximum number of
    // processors that are supported.
    //

    Dpc->Number = MAXIMUM_PROCESSORS + Number;
    return;
}
