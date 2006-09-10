/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ke_x.h
* PURPOSE:         Internal Inlined Functions for the Kernel
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

//
// Enters a Critical Region
//
#define KeEnterGuardedRegion()                      \
{                                                   \
    PKTHREAD Thread = KeGetCurrentThread();         \
                                                    \
    /* Sanity checks */                             \
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);           \
    ASSERT(Thread == KeGetCurrentThread());         \
    ASSERT((Thread->SpecialApcDisable <= 0) &&      \
           (Thread->SpecialApcDisable != -32768));  \
                                                    \
    /* Disable Special APCs */                      \
    Thread->SpecialApcDisable--;                    \
}

//
// Leaves a Critical Region
//
#define KeLeaveGuardedRegion()                      \
{                                                   \
    PKTHREAD Thread = KeGetCurrentThread();         \
                                                    \
    /* Sanity checks */                             \
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);           \
    ASSERT(Thread == KeGetCurrentThread());         \
    ASSERT(Thread->SpecialApcDisable < 0);          \
                                                    \
    /* Leave region and check if APCs are OK now */ \
    if (!(++Thread->SpecialApcDisable))             \
    {                                               \
        /* Check for Kernel APCs on the list */     \
        if (!IsListEmpty(&Thread->ApcState.         \
                         ApcListHead[KernelMode]))  \
        {                                           \
            /* Check for APC Delivery */            \
            KiCheckForKernelApcDelivery();          \
        }                                           \
    }                                               \
}

//
// TODO: Guarded Mutex Routines
//

//
// TODO: Critical Region Routines
//

//
// Satisfies the wait of any dispatcher object
//
#define KiSatisfyObjectWait(Object, Thread)                                 \
{                                                                           \
    /* Special case for Mutants */                                          \
    if ((Object)->Header.Type == MutantObject)                              \
    {                                                                       \
        /* Decrease the Signal State */                                     \
        (Object)->Header.SignalState--;                                     \
                                                                            \
        /* Check if it's now non-signaled */                                \
        if (!(Object)->Header.SignalState)                                  \
        {                                                                   \
            /* Set the Owner Thread */                                      \
            (Object)->OwnerThread = Thread;                                 \
                                                                            \
            /* Disable APCs if needed */                                    \
            Thread->KernelApcDisable -= (Object)->ApcDisable;               \
                                                                            \
            /* Check if it's abandoned */                                   \
            if ((Object)->Abandoned)                                        \
            {                                                               \
                /* Unabandon it */                                          \
                (Object)->Abandoned = FALSE;                                \
                                                                            \
                /* Return Status */                                         \
                Thread->WaitStatus = STATUS_ABANDONED;                      \
            }                                                               \
                                                                            \
            /* Insert it into the Mutant List */                            \
            InsertHeadList(Thread->MutantListHead.Blink,                    \
                           &(Object)->MutantListEntry);                     \
        }                                                                   \
    }                                                                       \
    else if (((Object)->Header.Type & TIMER_OR_EVENT_TYPE) ==               \
             EventSynchronizationObject)                                    \
    {                                                                       \
        /* Synchronization Timers and Events just get un-signaled */        \
        (Object)->Header.SignalState = 0;                                   \
    }                                                                       \
    else if ((Object)->Header.Type == SemaphoreObject)                      \
    {                                                                       \
        /* These ones can have multiple states, so we only decrease it */   \
        (Object)->Header.SignalState--;                                     \
    }                                                                       \
}

//
// Satisfies the wait of a mutant dispatcher object
//
#define KiSatisfyMutantWait(Object, Thread)                                 \
{                                                                           \
    /* Decrease the Signal State */                                         \
    (Object)->Header.SignalState--;                                         \
                                                                            \
    /* Check if it's now non-signaled */                                    \
    if (!(Object)->Header.SignalState)                                      \
    {                                                                       \
        /* Set the Owner Thread */                                          \
        (Object)->OwnerThread = Thread;                                     \
                                                                            \
        /* Disable APCs if needed */                                        \
        Thread->KernelApcDisable -= (Object)->ApcDisable;                   \
                                                                            \
        /* Check if it's abandoned */                                       \
        if ((Object)->Abandoned)                                            \
        {                                                                   \
            /* Unabandon it */                                              \
            (Object)->Abandoned = FALSE;                                    \
                                                                            \
            /* Return Status */                                             \
            Thread->WaitStatus = STATUS_ABANDONED;                          \
        }                                                                   \
                                                                            \
        /* Insert it into the Mutant List */                                \
        InsertHeadList(Thread->MutantListHead.Blink,                        \
                       &(Object)->MutantListEntry);                         \
    }                                                                       \
}

//
// Satisfies the wait of any nonmutant dispatcher object
//
#define KiSatisfyNonMutantWait(Object, Thread)                              \
{                                                                           \
    if (((Object)->Header.Type & TIMER_OR_EVENT_TYPE) ==                    \
             EventSynchronizationObject)                                    \
    {                                                                       \
        /* Synchronization Timers and Events just get un-signaled */        \
        (Object)->Header.SignalState = 0;                                   \
    }                                                                       \
    else if ((Object)->Header.Type == SemaphoreObject)                      \
    {                                                                       \
        /* These ones can have multiple states, so we only decrease it */   \
        (Object)->Header.SignalState--;                                     \
    }                                                                       \
}

//
// Recalculates the due time
//
PLARGE_INTEGER
FORCEINLINE
KiRecalculateDueTime(IN PLARGE_INTEGER OriginalDueTime,
                     IN PLARGE_INTEGER DueTime,
                     IN OUT PLARGE_INTEGER NewDueTime)
{
    /* Don't do anything for absolute waits */
    if (OriginalDueTime->QuadPart >= 0) return OriginalDueTime;

    /* Otherwise, query the interrupt time and recalculate */
    NewDueTime->QuadPart = KeQueryInterruptTime();
    NewDueTime->QuadPart -= DueTime->QuadPart;
    return NewDueTime;
}

//
// Determines wether a thread should be added to the wait list
//
#define KiCheckThreadStackSwap(WaitMode, Thread, Swappable)                 \
{                                                                           \
    /* Check the required conditions */                                     \
    if ((WaitMode != KernelMode) &&                                         \
        (Thread->EnableStackSwap) &&                                        \
        (Thread->Priority >= (LOW_REALTIME_PRIORITY + 9)))                  \
    {                                                                       \
        /* We are go for swap */                                            \
        Swappable = TRUE;                                                   \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /* Don't swap the thread */                                         \
        Swappable = FALSE;                                                  \
    }                                                                       \
}

//
// Adds a thread to the wait list
//
#define KiAddThreadToWaitList(Thread, Swappable)                            \
{                                                                           \
    /* Make sure it's swappable */                                          \
    if (Swappable)                                                          \
    {                                                                       \
        /* Insert it into the PRCB's List */                                \
        InsertTailList(&KeGetCurrentPrcb()->WaitListHead,                   \
                       &Thread->WaitListEntry);                             \
    }                                                                       \
}

//
// Rules for checking alertability:
//  - For Alertable waits ONLY:
//      * We don't wait and return STATUS_ALERTED if the thread is alerted
//        in EITHER the specified wait mode OR in Kernel Mode.
//  - For BOTH Alertable AND Non-Alertable waits:
//      * We don't want and return STATUS_USER_APC if the User Mode APC list
//        is not empty AND the wait mode is User Mode.
//
#define KiCheckAlertability()                                               \
{                                                                           \
    if (Alertable)                                                          \
    {                                                                       \
        if (CurrentThread->Alerted[(int)WaitMode])                          \
        {                                                                   \
            CurrentThread->Alerted[(int)WaitMode] = FALSE;                  \
            WaitStatus = STATUS_ALERTED;                                    \
            break;                                                          \
        }                                                                   \
        else if ((WaitMode != KernelMode) &&                                \
                (!IsListEmpty(&CurrentThread->                              \
                              ApcState.ApcListHead[UserMode])))             \
        {                                                                   \
            CurrentThread->ApcState.UserApcPending = TRUE;                  \
            WaitStatus = STATUS_USER_APC;                                   \
            break;                                                          \
        }                                                                   \
        else if (CurrentThread->Alerted[KernelMode])                        \
        {                                                                   \
            CurrentThread->Alerted[KernelMode] = FALSE;                     \
            WaitStatus = STATUS_ALERTED;                                    \
            break;                                                          \
        }                                                                   \
    }                                                                       \
    else if ((WaitMode != KernelMode) &&                                    \
             (CurrentThread->ApcState.UserApcPending))                      \
    {                                                                       \
        WaitStatus = STATUS_USER_APC;                                       \
        break;                                                              \
    }                                                                       \
}

//
// Thread Scheduling Routines
//
#ifndef _CONFIG_SMP
KIRQL
FORCEINLINE
KiAcquireDispatcherLock(VOID)
{
    /* Raise to DPC level */
    return KeRaiseIrqlToDpcLevel();
}

VOID
FORCEINLINE
KiReleaseDispatcherLock(IN KIRQL OldIrql)
{
    /* Just exit the dispatcher */
    KiExitDispatcher(OldIrql);
}

VOID
FORCEINLINE
KiAcquireDispatcherLockAtDpcLevel(VOID)
{
    /* This is a no-op at DPC Level for UP systems */
    return;
}

VOID
FORCEINLINE
KiReleaseDispatcherLockFromDpcLevel(VOID)
{
    /* This is a no-op at DPC Level for UP systems */
    return;
}

//
// This routine makes the thread deferred ready on the boot CPU.
//
FORCEINLINE
VOID
KiInsertDeferredReadyList(IN PKTHREAD Thread)
{
    /* Set the thread to deferred state and boot CPU */
    Thread->State = DeferredReady;
    Thread->DeferredProcessor = 0;

    /* Make the thread ready immediately */
    KiDeferredReadyThread(Thread);
}

FORCEINLINE
VOID
KiRescheduleThread(IN BOOLEAN NewThread,
                   IN ULONG Cpu)
{
    /* This is meaningless on UP systems */
    UNREFERENCED_PARAMETER(NewThread);
    UNREFERENCED_PARAMETER(Cpu);
}

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
VOID
KiSetThreadSwapBusy(IN PKTHREAD Thread)
{
    UNREFERENCED_PARAMETER(Thread);
}

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
VOID
KiAcquirePrcbLock(IN PKPRCB Prcb)
{
    UNREFERENCED_PARAMETER(Prcb);
}

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
VOID
KiReleasePrcbLock(IN PKPRCB Prcb)
{
    UNREFERENCED_PARAMETER(Prcb);
}

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
VOID
KiAcquireThreadLock(IN PKTHREAD Thread)
{
    UNREFERENCED_PARAMETER(Thread);
}

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
VOID
KiReleaseThreadLock(IN PKTHREAD Thread)
{
    UNREFERENCED_PARAMETER(Thread);
}

FORCEINLINE
VOID
KiCheckDeferredReadyList(IN PKPRCB Prcb)
{
    /* There are no deferred ready lists on UP systems */
    UNREFERENCED_PARAMETER(Prcb);
}

FORCEINLINE
VOID
KiRundownThread(IN PKTHREAD Thread)
{
    /* Check if this is the NPX Thread */
    if (KeGetCurrentPrcb()->NpxThread == Thread)
    {
        /* Clear it */
        KeGetCurrentPrcb()->NpxThread = NULL;
#ifdef __GNUC__
        __asm__("fninit\n\t");
#else
        __asm fninit;
#endif
    }
}

#else

KIRQL
FORCEINLINE
KiAcquireDispatcherLock(VOID)
{
    /* Raise to synchronization level and acquire the dispatcher lock */
    return KeAcquireQueuedSpinLockRaiseToSynch(LockQueueDispatcherLock);
}

VOID
FORCEINLINE
KiReleaseDispatcherLock(IN KIRQL OldIrql)
{
    /* First release the lock */
    KeReleaseQueuedSpinLockFromDpcLevel(&KeGetCurrentPrcb()->
                                        LockQueue[LockQueueDispatcherLock]);

    /* Then exit the dispatcher */
    KiExitDispatcher(OldIrql);
}

//
// This routine inserts a thread into the deferred ready list of the given CPU
//
FORCEINLINE
VOID
KiInsertDeferredReadyList(IN PKTHREAD Thread)
{
    PKPRCB Prcb = KeGetCurrentPrcb();

    /* Set the thread to deferred state and CPU */
    Thread->State = DeferredReady;
    Thread->DeferredProcessor = Prcb->Number;

    /* Add it on the list */
    PushEntryList(&Prcb->DeferredReadyListHead, &Thread->SwapListEntry);
}

FORCEINLINE
VOID
KiRescheduleThread(IN BOOLEAN NewThread,
                   IN ULONG Cpu)
{
    /* Check if a new thread needs to be scheduled on a different CPU */
    if ((NewThread) && !(KeGetPcr()->Number == Cpu))
    {
        /* Send an IPI to request delivery */
        KiIpiSendRequest(AFFINITY_MASK(Cpu), IPI_DPC);
    }
}

//
// This routine sets the current thread in a swap busy state, which ensure that
// nobody else tries to swap it concurrently.
//
FORCEINLINE
VOID
KiSetThreadSwapBusy(IN PKTHREAD Thread)
{
    /* Make sure nobody already set it */
    ASSERT(Thread->SwapBusy == FALSE);

    /* Set it ourselves */
    Thread->SwapBusy = TRUE;
}

//
// This routine acquires the PRCB lock so that only one caller can touch
// volatile PRCB data.
//
// Since this is a simple optimized spin-lock, it must be be only acquired
// at dispatcher level or higher!
//
FORCEINLINE
VOID
KiAcquirePrcbLock(IN PKPRCB Prcb)
{
    /* Make sure we're at a safe level to touch the PRCB lock */
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Start acquire loop */
    for (;;)
    {
        /* Acquire the lock and break out if we acquired it first */
        if (!InterlockedExchange(&Prcb->PrcbLock, 1)) break;

        /* Loop until the other CPU releases it */
        do
        {
            /* Let the CPU know that this is a loop */
            YieldProcessor();
        } while (Prcb->PrcbLock);
    }
}

//
// This routine releases the PRCB lock so that other callers can touch
// volatile PRCB data.
//
// Since this is a simple optimized spin-lock, it must be be only acquired
// at dispatcher level or higher!
//
FORCEINLINE
VOID
KiReleasePrcbLock(IN PKPRCB Prcb)
{
    /* Make sure it's acquired! */
    ASSERT(Prcb->PrcbLock != 0);

    /* Release it */
    InterlockedAnd(&Prcb->PrcbLock, 0);
}

//
// This routine acquires the thread lock so that only one caller can touch
// volatile thread data.
//
// Since this is a simple optimized spin-lock, it must be be only acquired
// at dispatcher level or higher!
//
FORCEINLINE
VOID
KiAcquireThreadLock(IN PKTHREAD Thread)
{
    /* Make sure we're at a safe level to touch the thread lock */
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Start acquire loop */
    for (;;)
    {
        /* Acquire the lock and break out if we acquired it first */
        if (!InterlockedExchange(&Thread->ThreadLock, 1)) break;

        /* Loop until the other CPU releases it */
        do
        {
            /* Let the CPU know that this is a loop */
            YieldProcessor();
        } while (Thread->ThreadLock);
    }
}

//
// This routine releases the thread lock so that other callers can touch
// volatile thread data.
//
// Since this is a simple optimized spin-lock, it must be be only acquired
// at dispatcher level or higher!
//
FORCEINLINE
VOID
KiReleaseThreadLock(IN PKTHREAD Thread)
{
    /* Release it */
    InterlockedAnd(&Thread->ThreadLock, 0);
}

FORCEINLINE
VOID
KiCheckDeferredReadyList(IN PKPRCB Prcb)
{
    /* Scan the deferred ready lists if required */
    if (Prcb->DeferredReadyListHead.Next) KiProcessDeferredReadyList(Prcb);
}

#endif

FORCEINLINE
VOID
KiAcquireApcLock(IN PKTHREAD Thread,
                 IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Acquire the lock and raise to synchronization level */
    KeAcquireInStackQueuedSpinLockRaiseToSynch(&Thread->ApcQueueLock, Handle);
}

FORCEINLINE
VOID
KiAcquireApcLockAtDpcLevel(IN PKTHREAD Thread,
                           IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Acquire the lock */
    KeAcquireInStackQueuedSpinLockAtDpcLevel(&Thread->ApcQueueLock, Handle);
}

FORCEINLINE
VOID
KiReleaseApcLock(IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Release the lock */
    KeReleaseInStackQueuedSpinLock(Handle);
}

FORCEINLINE
VOID
KiReleaseApcLockFromDpcLevel(IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Release the lock */
    KeReleaseInStackQueuedSpinLockFromDpcLevel(Handle);
}

FORCEINLINE
VOID
KiAcquireProcessLock(IN PKPROCESS Process,
                     IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Acquire the lock and raise to synchronization level */
    KeAcquireInStackQueuedSpinLockRaiseToSynch(&Process->ProcessLock, Handle);
}

FORCEINLINE
VOID
KiReleaseProcessLock(IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Release the lock */
    KeReleaseInStackQueuedSpinLock(Handle);
}

//
// This routine queues a thread that is ready on the PRCB's ready lists.
// If this thread cannot currently run on this CPU, then the thread is
// added to the deferred ready list instead.
//
// This routine must be entered with the PRCB lock held and it will exit
// with the PRCB lock released!
//
FORCEINLINE
VOID
KxQueueReadyThread(IN PKTHREAD Thread,
                   IN PKPRCB Prcb)
{
    BOOLEAN Preempted;
    KPRIORITY Priority;

    /* Sanity checks */
    ASSERT(Prcb == KeGetCurrentPrcb());
    ASSERT(Thread->State == Running);
    ASSERT(Thread->NextProcessor == Prcb->Number);

    /* Check if this thread is allowed to run in this CPU */
#ifdef _CONFIG_SMP
    if ((Thread->Affinity) & (Prcb->SetMember))
#else
    if (TRUE)
#endif
    {
        /* Set thread ready for execution */
        Thread->State = Ready;

        /* Save current priority and if someone had pre-empted it */
        Priority = Thread->Priority;
        Preempted = Thread->Preempted;

        /* We're not pre-empting now, and set the wait time */
        Thread->Preempted = FALSE;
        Thread->WaitTime = KeTickCount.LowPart;

        /* Sanity check */
        ASSERT((Priority >= 0) && (Priority <= HIGH_PRIORITY));

        /* Insert this thread in the appropriate order */
        Preempted ? InsertHeadList(&Prcb->DispatcherReadyListHead[Priority],
                                   &Thread->WaitListEntry) :
                    InsertTailList(&Prcb->DispatcherReadyListHead[Priority],
                                   &Thread->WaitListEntry);

        /* Update the ready summary */
        Prcb->ReadySummary |= PRIORITY_MASK(Priority);

        /* Sanity check */
        ASSERT(Priority == Thread->Priority);

        /* Release the PRCB lock */
        KiReleasePrcbLock(Prcb);
    }
    else
    {
        /* Otherwise, prepare this thread to be deferred */
        Thread->State = DeferredReady;
        Thread->DeferredProcessor = Prcb->Number;

        /* Release the lock and defer scheduling */
        KiReleasePrcbLock(Prcb);
        KiDeferredReadyThread(Thread);
    }
}

//
// This routine scans for an appropriate ready thread to select at the
// given priority and for the given CPU.
//
FORCEINLINE
PKTHREAD
KiSelectReadyThread(IN KPRIORITY Priority,
                    IN PKPRCB Prcb)
{
    LONG PriorityMask, PrioritySet, HighPriority;
    PLIST_ENTRY ListEntry;
    PKTHREAD Thread;

    /* Save the current mask and get the priority set for the CPU */
    PriorityMask = Priority;
    PrioritySet = Prcb->ReadySummary >> (UCHAR)Priority;
    if (!PrioritySet) return NULL;

    /*  Get the highest priority possible */
    BitScanReverse((PULONG)&HighPriority, PrioritySet);
    ASSERT((PrioritySet & PRIORITY_MASK(HighPriority)) != 0);
    HighPriority += PriorityMask;

    /* Make sure the list isn't at highest priority */
    ASSERT(IsListEmpty(&Prcb->DispatcherReadyListHead[HighPriority]) == FALSE);

    /* Get the first thread on the list */
    ListEntry = &Prcb->DispatcherReadyListHead[HighPriority];
    Thread = CONTAINING_RECORD(ListEntry, KTHREAD, WaitListEntry);

    /* Make sure this thread is here for a reason */
    ASSERT(HighPriority == Thread->Priority);
    ASSERT(Thread->Affinity & AFFINITY_MASK(Prcb->Number));
    ASSERT(Thread->NextProcessor == Prcb->Number);

    /* Remove it from the list */
    RemoveEntryList(&Thread->WaitListEntry);
    if (IsListEmpty(&Thread->WaitListEntry))
    {
        /* The list is empty now, reset the ready summary */
        Prcb->ReadySummary ^= PRIORITY_MASK(HighPriority);
    }

    /* Sanity check and return the thread */
    ASSERT((Thread == NULL) ||
           (Thread->BasePriority == 0) ||
           (Thread->Priority != 0));
    return Thread;
}

//
// This routine computes the new priority for a thread. It is only valid for
// threads with priorities in the dynamic priority range.
//
SCHAR
FORCEINLINE
KiComputeNewPriority(IN PKTHREAD Thread)
{
    SCHAR Priority;

    /* Priority sanity checks */
    ASSERT((Thread->PriorityDecrement >= 0) &&
           (Thread->PriorityDecrement <= Thread->Priority));
    ASSERT((Thread->Priority < LOW_REALTIME_PRIORITY) ?
            TRUE : (Thread->PriorityDecrement == 0));

    /* Get the current priority */
    Priority = Thread->Priority;
    if (Priority < LOW_REALTIME_PRIORITY)
    {
        /* Set the New Priority and add the Priority Decrement */
        Priority += (Priority - Thread->PriorityDecrement - 1);

        /* Don't go out of bounds */
        if (Priority < Thread->BasePriority) Priority = Thread->BasePriority;

        /* Reset the priority decrement */
        Thread->PriorityDecrement = 0;
    }

    /* Sanity check */
    ASSERT((Thread->BasePriority == 0) || (Priority != 0));

    /* Return the new priority */
    return Priority;
}

