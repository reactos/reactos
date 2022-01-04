/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/ke_x.h
* PURPOSE:         Internal Inlined Functions for the Kernel
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef _M_ARM
FORCEINLINE
KPROCESSOR_MODE
KeGetPreviousMode(VOID)
{
    /* Return the current mode */
    return KeGetCurrentThread()->PreviousMode;
}
#endif

//
// Enters a Guarded Region
//
#define KeEnterGuardedRegionThread(_Thread)                                 \
{                                                                           \
    /* Sanity checks */                                                     \
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);                                \
    ASSERT((_Thread) == KeGetCurrentThread());                              \
    ASSERT(((_Thread)->SpecialApcDisable <= 0) &&                           \
           ((_Thread)->SpecialApcDisable != -32768));                       \
                                                                            \
    /* Disable Special APCs */                                              \
    (_Thread)->SpecialApcDisable--;                                         \
}

#define KeEnterGuardedRegion()                                              \
{                                                                           \
    PKTHREAD _Thread = KeGetCurrentThread();                                \
    KeEnterGuardedRegionThread(_Thread);                                    \
}

//
// Leaves a Guarded Region
//
#define KeLeaveGuardedRegionThread(_Thread)                                 \
{                                                                           \
    /* Sanity checks */                                                     \
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);                                \
    ASSERT((_Thread) == KeGetCurrentThread());                              \
    ASSERT((_Thread)->SpecialApcDisable < 0);                               \
                                                                            \
    /* Leave region and check if APCs are OK now */                         \
    if (!(++(_Thread)->SpecialApcDisable))                                  \
    {                                                                       \
        /* Check for Kernel APCs on the list */                             \
        if (!IsListEmpty(&(_Thread)->ApcState.                              \
                         ApcListHead[KernelMode]))                          \
        {                                                                   \
            /* Check for APC Delivery */                                    \
            KiCheckForKernelApcDelivery();                                  \
        }                                                                   \
    }                                                                       \
}

#define KeLeaveGuardedRegion()                                              \
{                                                                           \
    PKTHREAD _Thread = KeGetCurrentThread();                                \
    KeLeaveGuardedRegionThread(_Thread);                                    \
}

//
// Enters a Critical Region
//
#define KeEnterCriticalRegionThread(_Thread)                                \
{                                                                           \
    /* Sanity checks */                                                     \
    ASSERT((_Thread) == KeGetCurrentThread());                              \
    ASSERT(((_Thread)->KernelApcDisable <= 0) &&                            \
           ((_Thread)->KernelApcDisable != -32768));                        \
                                                                            \
    /* Disable Kernel APCs */                                               \
    (_Thread)->KernelApcDisable--;                                          \
}

#define KeEnterCriticalRegion()                                             \
{                                                                           \
    PKTHREAD _Thread = KeGetCurrentThread();                                \
    KeEnterCriticalRegionThread(_Thread);                                   \
}

//
// Leaves a Critical Region
//
#define KeLeaveCriticalRegionThread(_Thread)                                \
{                                                                           \
    /* Sanity checks */                                                     \
    ASSERT((_Thread) == KeGetCurrentThread());                              \
    ASSERT((_Thread)->KernelApcDisable < 0);                                \
                                                                            \
    /* Enable Kernel APCs */                                                \
    (_Thread)->KernelApcDisable++;                                          \
                                                                            \
    /* Check if Kernel APCs are now enabled */                              \
    if (!((_Thread)->KernelApcDisable))                                     \
    {                                                                       \
        /* Check if we need to request an APC Delivery */                   \
        if (!(IsListEmpty(&(_Thread)->ApcState.ApcListHead[KernelMode])) && \
            !((_Thread)->SpecialApcDisable))                                \
        {                                                                   \
            /* Check for the right environment */                           \
            KiCheckForKernelApcDelivery();                                  \
        }                                                                   \
    }                                                                       \
}

#define KeLeaveCriticalRegion()                                             \
{                                                                           \
    PKTHREAD _Thread = KeGetCurrentThread();                                \
    KeLeaveCriticalRegionThread(_Thread);                                   \
}

#ifndef CONFIG_SMP

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
VOID
KiAcquireDispatcherObject(IN DISPATCHER_HEADER* Object)
{
    UNREFERENCED_PARAMETER(Object);
}

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
VOID
KiReleaseDispatcherObject(IN DISPATCHER_HEADER* Object)
{
    UNREFERENCED_PARAMETER(Object);
}

FORCEINLINE
KIRQL
KiAcquireDispatcherLock(VOID)
{
    /* Raise to synch level */
    return KfRaiseIrql(SYNCH_LEVEL);
}

FORCEINLINE
VOID
KiReleaseDispatcherLock(IN KIRQL OldIrql)
{
    /* Just exit the dispatcher */
    KiExitDispatcher(OldIrql);
}

FORCEINLINE
VOID
KiAcquireDispatcherLockAtSynchLevel(VOID)
{
    /* This is a no-op at SYNCH_LEVEL for UP systems */
    ASSERT(KeGetCurrentIrql() >= SYNCH_LEVEL);
    return;
}

FORCEINLINE
VOID
KiReleaseDispatcherLockFromSynchLevel(VOID)
{
    /* This is a no-op at SYNCH_LEVEL for UP systems */
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

//
// This routine protects against multiple CPU acquires, it's meaningless on UP.
//
FORCEINLINE
BOOLEAN
KiTryThreadLock(IN PKTHREAD Thread)
{
    UNREFERENCED_PARAMETER(Thread);
    return FALSE;
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
KiRequestApcInterrupt(IN BOOLEAN NeedApc,
                      IN UCHAR Processor)
{
    /* We deliver instantly on UP */
    UNREFERENCED_PARAMETER(NeedApc);
    UNREFERENCED_PARAMETER(Processor);
}

FORCEINLINE
PKSPIN_LOCK_QUEUE
KiAcquireTimerLock(IN ULONG Hand)
{
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Nothing to do on UP */
    UNREFERENCED_PARAMETER(Hand);
    return NULL;
}

FORCEINLINE
VOID
KiReleaseTimerLock(IN PKSPIN_LOCK_QUEUE LockQueue)
{
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Nothing to do on UP */
    UNREFERENCED_PARAMETER(LockQueue);
}

#else

FORCEINLINE
VOID
KiAcquireDispatcherObject(IN DISPATCHER_HEADER* Object)
{
    LONG OldValue;

    /* Make sure we're at a safe level to touch the lock */
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Start acquire loop */
    do
    {
        /* Loop until the other CPU releases it */
        while (TRUE)
        {
            /* Check if it got released */
            OldValue = Object->Lock;
            if ((OldValue & KOBJECT_LOCK_BIT) == 0) break;

            /* Let the CPU know that this is a loop */
            YieldProcessor();
        }

        /* Try acquiring the lock now */
    } while (InterlockedCompareExchange(&Object->Lock,
                                        OldValue | KOBJECT_LOCK_BIT,
                                        OldValue) != OldValue);
}

FORCEINLINE
VOID
KiReleaseDispatcherObject(IN DISPATCHER_HEADER* Object)
{
    /* Make sure we're at a safe level to touch the lock */
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Release it */
    InterlockedAnd(&Object->Lock, ~KOBJECT_LOCK_BIT);
}

FORCEINLINE
KIRQL
KiAcquireDispatcherLock(VOID)
{
    /* Raise to synchronization level and acquire the dispatcher lock */
    return KeAcquireQueuedSpinLockRaiseToSynch(LockQueueDispatcherLock);
}

FORCEINLINE
VOID
KiReleaseDispatcherLock(IN KIRQL OldIrql)
{
    /* First release the lock */
    KeReleaseQueuedSpinLockFromDpcLevel(&KeGetCurrentPrcb()->
                                        LockQueue[LockQueueDispatcherLock]);

    /* Then exit the dispatcher */
    KiExitDispatcher(OldIrql);
}

FORCEINLINE
VOID
KiAcquireDispatcherLockAtSynchLevel(VOID)
{
    /* Acquire the dispatcher lock */
    ASSERT(KeGetCurrentIrql() >= SYNCH_LEVEL);
    KeAcquireQueuedSpinLockAtDpcLevel(&KeGetCurrentPrcb()->
                                      LockQueue[LockQueueDispatcherLock]);
}

FORCEINLINE
VOID
KiReleaseDispatcherLockFromSynchLevel(VOID)
{
    /* Release the dispatcher lock */
    KeReleaseQueuedSpinLockFromDpcLevel(&KeGetCurrentPrcb()->
                                        LockQueue[LockQueueDispatcherLock]);
}

//
// This routine inserts a thread into the deferred ready list of the current CPU
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
    if ((NewThread) && !(KeGetCurrentPrcb()->Number == Cpu))
    {
        /* Send an IPI to request delivery */
        KiIpiSend(AFFINITY_MASK(Cpu), IPI_DPC);
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
// Since this is a simple optimized spin-lock, it must only be acquired
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
        if (!InterlockedExchange((PLONG)&Prcb->PrcbLock, 1)) break;

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
    /* Make sure we are above dispatch and the lock is acquired! */
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    ASSERT(Prcb->PrcbLock != 0);

    /* Release it */
    InterlockedAnd((PLONG)&Prcb->PrcbLock, 0);
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
        if (!InterlockedExchange((PLONG)&Thread->ThreadLock, 1)) break;

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
    /* Make sure we are still above dispatch */
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Release it */
    InterlockedAnd((PLONG)&Thread->ThreadLock, 0);
}

FORCEINLINE
BOOLEAN
KiTryThreadLock(IN PKTHREAD Thread)
{
    LONG Value;

    /* If the lock isn't acquired, return false */
    if (!Thread->ThreadLock) return FALSE;

    /* Otherwise, try to acquire it and check the result */
    Value = 1;
    Value = InterlockedExchange((PLONG)&Thread->ThreadLock, Value);

    /* Return the lock state */
    return (Value == 1);
}

FORCEINLINE
VOID
KiCheckDeferredReadyList(IN PKPRCB Prcb)
{
    /* Scan the deferred ready lists if required */
    if (Prcb->DeferredReadyListHead.Next) KiProcessDeferredReadyList(Prcb);
}

FORCEINLINE
VOID
KiRequestApcInterrupt(IN BOOLEAN NeedApc,
                      IN UCHAR Processor)
{
    /* Check if we need to request APC delivery */
    if (NeedApc)
    {
        /* Check if it's on another CPU */
        if (KeGetCurrentPrcb()->Number != Processor)
        {
            /* Send an IPI to request delivery */
            KiIpiSend(AFFINITY_MASK(Processor), IPI_APC);
        }
        else
        {
            /* Request a software interrupt */
            HalRequestSoftwareInterrupt(APC_LEVEL);
        }
    }
}

FORCEINLINE
PKSPIN_LOCK_QUEUE
KiAcquireTimerLock(IN ULONG Hand)
{
    PKSPIN_LOCK_QUEUE LockQueue;
    ULONG LockIndex;
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Get the lock index */
    LockIndex = Hand >> LOCK_QUEUE_TIMER_LOCK_SHIFT;
    LockIndex &= (LOCK_QUEUE_TIMER_TABLE_LOCKS - 1);

    /* Now get the lock */
    LockQueue = &KeGetCurrentPrcb()->LockQueue[LockQueueTimerTableLock + LockIndex];

    /* Acquire it and return */
    KeAcquireQueuedSpinLockAtDpcLevel(LockQueue);
    return LockQueue;
}

FORCEINLINE
VOID
KiReleaseTimerLock(IN PKSPIN_LOCK_QUEUE LockQueue)
{
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);

    /* Release the lock */
    KeReleaseQueuedSpinLockFromDpcLevel(LockQueue);
}

#endif

FORCEINLINE
VOID
KiAcquireApcLockRaiseToSynch(IN PKTHREAD Thread,
                 IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Acquire the lock and raise to synchronization level */
    KeAcquireInStackQueuedSpinLockRaiseToSynch(&Thread->ApcQueueLock, Handle);
}

FORCEINLINE
VOID
KiAcquireApcLockAtSynchLevel(IN PKTHREAD Thread,
                           IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Acquire the lock */
    ASSERT(KeGetCurrentIrql() >= SYNCH_LEVEL);
    KeAcquireInStackQueuedSpinLockAtDpcLevel(&Thread->ApcQueueLock, Handle);
}

FORCEINLINE
VOID
KiAcquireApcLockRaiseToDpc(IN PKTHREAD Thread,
                           IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Acquire the lock */
    KeAcquireInStackQueuedSpinLock(&Thread->ApcQueueLock, Handle);
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
KiReleaseApcLockFromSynchLevel(IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Release the lock */
    KeReleaseInStackQueuedSpinLockFromDpcLevel(Handle);
}

FORCEINLINE
VOID
KiAcquireProcessLockRaiseToSynch(IN PKPROCESS Process,
                     IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Acquire the lock and raise to synchronization level */
    KeAcquireInStackQueuedSpinLockRaiseToSynch(&Process->ProcessLock, Handle);
}

FORCEINLINE
VOID
KiReleaseProcessLock(IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Release the lock and restore previous IRQL */
    KeReleaseInStackQueuedSpinLock(Handle);
}

FORCEINLINE
VOID
KiReleaseProcessLockFromSynchLevel(IN PKLOCK_QUEUE_HANDLE Handle)
{
    /* Release the lock without lowering IRQL */
    KeReleaseInStackQueuedSpinLockFromDpcLevel(Handle);
}

FORCEINLINE
VOID
KiAcquireDeviceQueueLock(IN PKDEVICE_QUEUE DeviceQueue,
                         IN PKLOCK_QUEUE_HANDLE DeviceLock)
{
    /* Check if we were called from a threaded DPC */
    if (KeGetCurrentPrcb()->DpcThreadActive)
    {
        /* Lock the Queue, we're not at DPC level */
        KeAcquireInStackQueuedSpinLock(&DeviceQueue->Lock, DeviceLock);
    }
    else
    {
        /* We must be at DPC level, acquire the lock safely */
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
        KeAcquireInStackQueuedSpinLockAtDpcLevel(&DeviceQueue->Lock,
                                                 DeviceLock);
    }
}

FORCEINLINE
VOID
KiReleaseDeviceQueueLock(IN PKLOCK_QUEUE_HANDLE DeviceLock)
{
    /* Check if we were called from a threaded DPC */
    if (KeGetCurrentPrcb()->DpcThreadActive)
    {
        /* Unlock the Queue, we're not at DPC level */
        KeReleaseInStackQueuedSpinLock(DeviceLock);
    }
    else
    {
        /* We must be at DPC level, release the lock safely */
        ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
        KeReleaseInStackQueuedSpinLockFromDpcLevel(DeviceLock);
    }
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
        Thread->KernelApcDisable = Thread->KernelApcDisable -               \
                                   (Object)->ApcDisable;                    \
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
#define KiSatisfyNonMutantWait(Object)                                      \
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
// Satisfies the wait of any dispatcher object
//
#define KiSatisfyObjectWait(Object, Thread)                                 \
{                                                                           \
    /* Special case for Mutants */                                          \
    if ((Object)->Header.Type == MutantObject)                              \
    {                                                                       \
        KiSatisfyMutantWait((Object), (Thread));                            \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        KiSatisfyNonMutantWait(Object);                                     \
    }                                                                       \
}

//
// Recalculates the due time
//
FORCEINLINE
PLARGE_INTEGER
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
// Determines whether a thread should be added to the wait list
//
FORCEINLINE
BOOLEAN
KiCheckThreadStackSwap(IN PKTHREAD Thread,
                       IN KPROCESSOR_MODE WaitMode)
{
    /* Check the required conditions */
    if ((WaitMode != KernelMode) &&
        (Thread->EnableStackSwap) &&
        (Thread->Priority >= (LOW_REALTIME_PRIORITY + 9)))
    {
        /* We are go for swap */
        return TRUE;
    }
    else
    {
        /* Don't swap the thread */
        return FALSE;
    }
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
// Checks if a wait in progress should be interrupted by APCs or an alertable
// state.
//
FORCEINLINE
NTSTATUS
KiCheckAlertability(IN PKTHREAD Thread,
                    IN BOOLEAN Alertable,
                    IN KPROCESSOR_MODE WaitMode)
{
    /* Check if the wait is alertable */
    if (Alertable)
    {
        /* It is, first check if the thread is alerted in this mode */
        if (Thread->Alerted[WaitMode])
        {
            /* It is, so bail out of the wait */
            Thread->Alerted[WaitMode] = FALSE;
            return STATUS_ALERTED;
        }
        else if ((WaitMode != KernelMode) &&
                (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
        {
            /* It's isn't, but this is a user wait with queued user APCs */
            Thread->ApcState.UserApcPending = TRUE;
            return STATUS_USER_APC;
        }
        else if (Thread->Alerted[KernelMode])
        {
            /* It isn't that either, but we're alered in kernel mode */
            Thread->Alerted[KernelMode] = FALSE;
            return STATUS_ALERTED;
        }
    }
    else if ((WaitMode != KernelMode) && (Thread->ApcState.UserApcPending))
    {
        /* Not alertable, but this is a user wait with pending user APCs */
        return STATUS_USER_APC;
    }

    /* Otherwise, we're fine */
    return STATUS_WAIT_0;
}

FORCEINLINE
ULONG
KiComputeTimerTableIndex(IN ULONGLONG DueTime)
{
    return (DueTime / KeMaximumIncrement) & (TIMER_TABLE_SIZE - 1);
}

//
// Called from KiCompleteTimer, KiInsertTreeTimer, KeSetSystemTime
// to remove timer entries
// See Windows HPI blog for more information.
FORCEINLINE
VOID
KiRemoveEntryTimer(IN PKTIMER Timer)
{
    ULONG Hand;
    PKTIMER_TABLE_ENTRY TableEntry;

    /* Remove the timer from the timer list and check if it's empty */
    Hand = Timer->Header.Hand;
    if (RemoveEntryList(&Timer->TimerListEntry))
    {
        /* Get the respective timer table entry */
        TableEntry = &KiTimerTableListHead[Hand];
        if (&TableEntry->Entry == TableEntry->Entry.Flink)
        {
            /* Set the entry to an infinite absolute time */
            TableEntry->Time.HighPart = 0xFFFFFFFF;
        }
    }

    /* Clear the list entries on dbg builds so we can tell the timer is gone */
#if DBG
    Timer->TimerListEntry.Flink = NULL;
    Timer->TimerListEntry.Blink = NULL;
#endif
}

//
// Called by Wait and Queue code to insert a timer for dispatching.
// Also called by KeSetTimerEx to insert a timer from the caller.
//
FORCEINLINE
VOID
KxInsertTimer(IN PKTIMER Timer,
              IN ULONG Hand)
{
    PKSPIN_LOCK_QUEUE LockQueue;
    ASSERT(KeGetCurrentIrql() >= SYNCH_LEVEL);

    /* Acquire the lock and release the dispatcher lock */
    LockQueue = KiAcquireTimerLock(Hand);
    KiReleaseDispatcherLockFromSynchLevel();

    /* Try to insert the timer */
    if (KiInsertTimerTable(Timer, Hand))
    {
        /* Complete it */
        KiCompleteTimer(Timer, LockQueue);
    }
    else
    {
        /* Do nothing, just release the lock */
        KiReleaseTimerLock(LockQueue);
    }
}

//
// Called by KeSetTimerEx and KiInsertTreeTimer to calculate Due Time
// See the Windows HPI Blog for more information
//
FORCEINLINE
BOOLEAN
KiComputeDueTime(IN PKTIMER Timer,
                 IN LARGE_INTEGER DueTime,
                 OUT PULONG Hand)
{
    LARGE_INTEGER InterruptTime, SystemTime, DifferenceTime;

    /* Convert to relative time if needed */
    Timer->Header.Absolute = FALSE;
    if (DueTime.HighPart >= 0)
    {
        /* Get System Time */
        KeQuerySystemTime(&SystemTime);

        /* Do the conversion */
        DifferenceTime.QuadPart = SystemTime.QuadPart - DueTime.QuadPart;

        /* Make sure it hasn't already expired */
        Timer->Header.Absolute = TRUE;
        if (DifferenceTime.HighPart >= 0)
        {
            /* Cancel everything */
            Timer->Header.SignalState = TRUE;
            Timer->Header.Hand = 0;
            Timer->DueTime.QuadPart = 0;
            *Hand = 0;
            return FALSE;
        }

        /* Set the time as Absolute */
        DueTime = DifferenceTime;
    }

    /* Get the Interrupt Time */
    InterruptTime.QuadPart = KeQueryInterruptTime();

    /* Recalculate due time */
    Timer->DueTime.QuadPart = InterruptTime.QuadPart - DueTime.QuadPart;

    /* Get the handle */
    *Hand = KiComputeTimerTableIndex(Timer->DueTime.QuadPart);
    Timer->Header.Hand = (UCHAR)*Hand;
    Timer->Header.Inserted = TRUE;
    return TRUE;
}

//
// Called from Unlink and Queue Insert Code.
// Also called by timer code when canceling an inserted timer.
// Removes a timer from it's tree.
//
FORCEINLINE
VOID
KxRemoveTreeTimer(IN PKTIMER Timer)
{
    ULONG Hand = Timer->Header.Hand;
    PKSPIN_LOCK_QUEUE LockQueue;
    PKTIMER_TABLE_ENTRY TimerEntry;

    /* Acquire timer lock */
    LockQueue = KiAcquireTimerLock(Hand);

    /* Set the timer as non-inserted */
    Timer->Header.Inserted = FALSE;

    /* Remove it from the timer list */
    if (RemoveEntryList(&Timer->TimerListEntry))
    {
        /* Get the entry and check if it's empty */
        TimerEntry = &KiTimerTableListHead[Hand];
        if (IsListEmpty(&TimerEntry->Entry))
        {
            /* Clear the time then */
            TimerEntry->Time.HighPart = 0xFFFFFFFF;
        }
    }

    /* Release the timer lock */
    KiReleaseTimerLock(LockQueue);
}

FORCEINLINE
VOID
KxSetTimerForThreadWait(IN PKTIMER Timer,
                        IN LARGE_INTEGER Interval,
                        OUT PULONG Hand)
{
    ULONGLONG DueTime;
    LARGE_INTEGER InterruptTime, SystemTime, TimeDifference;

    /* Check the timer's interval to see if it's absolute */
    Timer->Header.Absolute = FALSE;
    if (Interval.HighPart >= 0)
    {
        /* Get the system time and calculate the relative time */
        KeQuerySystemTime(&SystemTime);
        TimeDifference.QuadPart = SystemTime.QuadPart - Interval.QuadPart;
        Timer->Header.Absolute = TRUE;

        /* Check if we've already expired */
        if (TimeDifference.HighPart >= 0)
        {
            /* Reset everything */
            Timer->DueTime.QuadPart = 0;
            *Hand = 0;
            Timer->Header.Hand = 0;
            return;
        }
        else
        {
            /* Update the interval */
            Interval = TimeDifference;
        }
    }

    /* Calculate the due time */
    InterruptTime.QuadPart = KeQueryInterruptTime();
    DueTime = InterruptTime.QuadPart - Interval.QuadPart;
    Timer->DueTime.QuadPart = DueTime;

    /* Calculate the timer handle */
    *Hand = KiComputeTimerTableIndex(DueTime);
    Timer->Header.Hand = (UCHAR)*Hand;
}

#define KxDelayThreadWait()                                                 \
                                                                            \
    /* Setup the Wait Block */                                              \
    Thread->WaitBlockList = TimerBlock;                                     \
                                                                            \
    /* Setup the timer */                                                   \
    KxSetTimerForThreadWait(Timer, *Interval, &Hand);                       \
                                                                            \
    /* Save the due time for the caller */                                  \
    DueTime.QuadPart = Timer->DueTime.QuadPart;                             \
                                                                            \
    /* Link the timer to this Wait Block */                                 \
    TimerBlock->NextWaitBlock = TimerBlock;                                 \
    Timer->Header.WaitListHead.Flink = &TimerBlock->WaitListEntry;          \
    Timer->Header.WaitListHead.Blink = &TimerBlock->WaitListEntry;          \
                                                                            \
    /* Clear wait status */                                                 \
    Thread->WaitStatus = STATUS_SUCCESS;                                    \
                                                                            \
    /* Setup wait fields */                                                 \
    Thread->Alertable = Alertable;                                          \
    Thread->WaitReason = DelayExecution;                                    \
    Thread->WaitMode = WaitMode;                                            \
                                                                            \
    /* Check if we can swap the thread's stack */                           \
    Thread->WaitListEntry.Flink = NULL;                                     \
    Swappable = KiCheckThreadStackSwap(Thread, WaitMode);                   \
                                                                            \
    /* Set the wait time */                                                 \
    Thread->WaitTime = KeTickCount.LowPart;

#define KxMultiThreadWait()                                                 \
    /* Link wait block array to the thread */                               \
    Thread->WaitBlockList = WaitBlockArray;                                 \
                                                                            \
    /* Reset the index */                                                   \
    Index = 0;                                                              \
                                                                            \
    /* Loop wait blocks */                                                  \
    do                                                                      \
    {                                                                       \
        /* Fill out the wait block */                                       \
        WaitBlock = &WaitBlockArray[Index];                                 \
        WaitBlock->Object = Object[Index];                                  \
        WaitBlock->WaitKey = (USHORT)Index;                                 \
        WaitBlock->WaitType = WaitType;                                     \
        WaitBlock->Thread = Thread;                                         \
                                                                            \
        /* Link to next block */                                            \
        WaitBlock->NextWaitBlock = &WaitBlockArray[Index + 1];              \
        Index++;                                                            \
    } while (Index < Count);                                                \
                                                                            \
    /* Link the last block */                                               \
    WaitBlock->NextWaitBlock = WaitBlockArray;                              \
                                                                            \
    /* Set default wait status */                                           \
    Thread->WaitStatus = STATUS_WAIT_0;                                     \
                                                                            \
    /* Check if we have a timer */                                          \
    if (Timeout)                                                            \
    {                                                                       \
        /* Link to the block */                                             \
        TimerBlock->NextWaitBlock = WaitBlockArray;                         \
                                                                            \
        /* Setup the timer */                                               \
        KxSetTimerForThreadWait(Timer, *Timeout, &Hand);                    \
                                                                            \
        /* Save the due time for the caller */                              \
        DueTime.QuadPart = Timer->DueTime.QuadPart;                         \
                                                                            \
        /* Initialize the list */                                           \
        InitializeListHead(&Timer->Header.WaitListHead);                    \
    }                                                                       \
                                                                            \
    /* Set wait settings */                                                 \
    Thread->Alertable = Alertable;                                          \
    Thread->WaitMode = WaitMode;                                            \
    Thread->WaitReason = WaitReason;                                        \
                                                                            \
    /* Check if we can swap the thread's stack */                           \
    Thread->WaitListEntry.Flink = NULL;                                     \
    Swappable = KiCheckThreadStackSwap(Thread, WaitMode);                   \
                                                                            \
    /* Set the wait time */                                                 \
    Thread->WaitTime = KeTickCount.LowPart;

#define KxSingleThreadWait()                                                \
    /* Setup the Wait Block */                                              \
    Thread->WaitBlockList = WaitBlock;                                      \
    WaitBlock->WaitKey = STATUS_SUCCESS;                                    \
    WaitBlock->Object = Object;                                             \
    WaitBlock->WaitType = WaitAny;                                          \
                                                                            \
    /* Clear wait status */                                                 \
    Thread->WaitStatus = STATUS_SUCCESS;                                    \
                                                                            \
    /* Check if we have a timer */                                          \
    if (Timeout)                                                            \
    {                                                                       \
        /* Setup the timer */                                               \
        KxSetTimerForThreadWait(Timer, *Timeout, &Hand);                    \
                                                                            \
        /* Save the due time for the caller */                              \
        DueTime.QuadPart = Timer->DueTime.QuadPart;                         \
                                                                            \
        /* Pointer to timer block */                                        \
        WaitBlock->NextWaitBlock = TimerBlock;                              \
        TimerBlock->NextWaitBlock = WaitBlock;                              \
                                                                            \
        /* Link the timer to this Wait Block */                             \
        Timer->Header.WaitListHead.Flink = &TimerBlock->WaitListEntry;      \
        Timer->Header.WaitListHead.Blink = &TimerBlock->WaitListEntry;      \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /* No timer block, just ourselves */                                \
        WaitBlock->NextWaitBlock = WaitBlock;                               \
    }                                                                       \
                                                                            \
    /* Set wait settings */                                                 \
    Thread->Alertable = Alertable;                                          \
    Thread->WaitMode = WaitMode;                                            \
    Thread->WaitReason = WaitReason;                                        \
                                                                            \
    /* Check if we can swap the thread's stack */                           \
    Thread->WaitListEntry.Flink = NULL;                                     \
    Swappable = KiCheckThreadStackSwap(Thread, WaitMode);                   \
                                                                            \
    /* Set the wait time */                                                 \
    Thread->WaitTime = KeTickCount.LowPart;

#define KxQueueThreadWait()                                                 \
    /* Setup the Wait Block */                                              \
    Thread->WaitBlockList = WaitBlock;                                      \
    WaitBlock->WaitKey = STATUS_SUCCESS;                                    \
    WaitBlock->Object = Queue;                                              \
    WaitBlock->WaitType = WaitAny;                                          \
    WaitBlock->Thread = Thread;                                             \
                                                                            \
    /* Clear wait status */                                                 \
    Thread->WaitStatus = STATUS_SUCCESS;                                    \
                                                                            \
    /* Check if we have a timer */                                          \
    if (Timeout)                                                            \
    {                                                                       \
        /* Setup the timer */                                               \
        KxSetTimerForThreadWait(Timer, *Timeout, &Hand);                    \
                                                                            \
        /* Save the due time for the caller */                              \
        DueTime.QuadPart = Timer->DueTime.QuadPart;                         \
                                                                            \
        /* Pointer to timer block */                                        \
        WaitBlock->NextWaitBlock = TimerBlock;                              \
        TimerBlock->NextWaitBlock = WaitBlock;                              \
                                                                            \
        /* Link the timer to this Wait Block */                             \
        Timer->Header.WaitListHead.Flink = &TimerBlock->WaitListEntry;      \
        Timer->Header.WaitListHead.Blink = &TimerBlock->WaitListEntry;      \
    }                                                                       \
    else                                                                    \
    {                                                                       \
        /* No timer block, just ourselves */                                \
        WaitBlock->NextWaitBlock = WaitBlock;                               \
    }                                                                       \
                                                                            \
    /* Set wait settings */                                                 \
    Thread->Alertable = FALSE;                                              \
    Thread->WaitMode = WaitMode;                                            \
    Thread->WaitReason = WrQueue;                                           \
                                                                            \
    /* Check if we can swap the thread's stack */                           \
    Thread->WaitListEntry.Flink = NULL;                                     \
    Swappable = KiCheckThreadStackSwap(Thread, WaitMode);                   \
                                                                            \
    /* Set the wait time */                                                 \
    Thread->WaitTime = KeTickCount.LowPart;

//
// Unwaits a Thread
//
FORCEINLINE
VOID
KxUnwaitThread(IN DISPATCHER_HEADER *Object,
               IN KPRIORITY Increment)
{
    PLIST_ENTRY WaitEntry, WaitList;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD WaitThread;
    ULONG WaitKey;

    /* Loop the Wait Entries */
    WaitList = &Object->WaitListHead;
    ASSERT(IsListEmpty(&Object->WaitListHead) == FALSE);
    WaitEntry = WaitList->Flink;
    do
    {
        /* Get the current wait block */
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);

        /* Get the waiting thread */
        WaitThread = WaitBlock->Thread;

        /* Check the current Wait Mode */
        if (WaitBlock->WaitType == WaitAny)
        {
            /* Use the actual wait key */
            WaitKey = WaitBlock->WaitKey;
        }
        else
        {
            /* Otherwise, use STATUS_KERNEL_APC */
            WaitKey = STATUS_KERNEL_APC;
        }

        /* Unwait the thread */
        KiUnwaitThread(WaitThread, WaitKey, Increment);

        /* Next entry */
        WaitEntry = WaitList->Flink;
    } while (WaitEntry != WaitList);
}

//
// Unwaits a Thread waiting on an event
//
FORCEINLINE
VOID
KxUnwaitThreadForEvent(IN PKEVENT Event,
                       IN KPRIORITY Increment)
{
    PLIST_ENTRY WaitEntry, WaitList;
    PKWAIT_BLOCK WaitBlock;
    PKTHREAD WaitThread;

    /* Loop the Wait Entries */
    WaitList = &Event->Header.WaitListHead;
    ASSERT(IsListEmpty(&Event->Header.WaitListHead) == FALSE);
    WaitEntry = WaitList->Flink;
    do
    {
        /* Get the current wait block */
        WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);

        /* Get the waiting thread */
        WaitThread = WaitBlock->Thread;

        /* Check the current Wait Mode */
        if (WaitBlock->WaitType == WaitAny)
        {
            /* Un-signal it */
            Event->Header.SignalState = 0;

            /* Un-signal the event and unwait the thread */
            KiUnwaitThread(WaitThread, WaitBlock->WaitKey, Increment);
            break;
        }

        /* Unwait the thread with STATUS_KERNEL_APC */
        KiUnwaitThread(WaitThread, STATUS_KERNEL_APC, Increment);

        /* Next entry */
        WaitEntry = WaitList->Flink;
    } while (WaitEntry != WaitList);
}

//
// This routine queues a thread that is ready on the PRCB's ready lists.
// If this thread cannot currently run on this CPU, then the thread is
// added to the deferred ready list instead.
//
// This routine must be entered with the PRCB lock held and it will exit
// with the PRCB lock released!
//
_Requires_lock_held_(Prcb->PrcbLock)
_Releases_lock_(Prcb->PrcbLock)
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
#ifdef CONFIG_SMP
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
    ULONG PrioritySet;
    LONG HighPriority;
    PLIST_ENTRY ListEntry;
    PKTHREAD Thread = NULL;

    /* Save the current mask and get the priority set for the CPU */
    PrioritySet = Prcb->ReadySummary >> Priority;
    if (!PrioritySet) goto Quickie;

    /* Get the highest priority possible */
    BitScanReverse((PULONG)&HighPriority, PrioritySet);
    ASSERT((PrioritySet & PRIORITY_MASK(HighPriority)) != 0);
    HighPriority += Priority;

    /* Make sure the list isn't empty at the highest priority */
    ASSERT(IsListEmpty(&Prcb->DispatcherReadyListHead[HighPriority]) == FALSE);

    /* Get the first thread on the list */
    ListEntry = Prcb->DispatcherReadyListHead[HighPriority].Flink;
    Thread = CONTAINING_RECORD(ListEntry, KTHREAD, WaitListEntry);

    /* Make sure this thread is here for a reason */
    ASSERT(HighPriority == Thread->Priority);
    ASSERT(Thread->Affinity & AFFINITY_MASK(Prcb->Number));
    ASSERT(Thread->NextProcessor == Prcb->Number);

    /* Remove it from the list */
    if (RemoveEntryList(&Thread->WaitListEntry))
    {
        /* The list is empty now, reset the ready summary */
        Prcb->ReadySummary ^= PRIORITY_MASK(HighPriority);
    }

    /* Sanity check and return the thread */
Quickie:
    ASSERT((Thread == NULL) ||
           (Thread->BasePriority == 0) ||
           (Thread->Priority != 0));
    return Thread;
}

//
// This routine computes the new priority for a thread. It is only valid for
// threads with priorities in the dynamic priority range.
//
FORCEINLINE
SCHAR
KiComputeNewPriority(IN PKTHREAD Thread,
                     IN SCHAR Adjustment)
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
        /* Decrease priority by the priority decrement */
        Priority -= (Thread->PriorityDecrement + Adjustment);

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

//
// Guarded Mutex Routines
//
FORCEINLINE
VOID
_KeInitializeGuardedMutex(OUT PKGUARDED_MUTEX GuardedMutex)
{
    /* Setup the Initial Data */
    GuardedMutex->Count = GM_LOCK_BIT;
    GuardedMutex->Owner = NULL;
    GuardedMutex->Contention = 0;

    /* Initialize the Wait Gate */
    KeInitializeGate(&GuardedMutex->Gate);
}

FORCEINLINE
VOID
_KeAcquireGuardedMutexUnsafe(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Sanity checks */
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread->SpecialApcDisable < 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(GuardedMutex->Owner != Thread);

    /* Remove the lock */
    if (!InterlockedBitTestAndReset(&GuardedMutex->Count, GM_LOCK_BIT_V))
    {
        /* The Guarded Mutex was already locked, enter contented case */
        KiAcquireGuardedMutex(GuardedMutex);
    }

    /* Set the Owner */
    GuardedMutex->Owner = Thread;
}

FORCEINLINE
VOID
_KeReleaseGuardedMutexUnsafe(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    LONG OldValue, NewValue;

    /* Sanity checks */
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (KeGetCurrentThread()->SpecialApcDisable < 0) ||
           (KeGetCurrentThread()->Teb == NULL) ||
           (KeGetCurrentThread()->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(GuardedMutex->Owner == KeGetCurrentThread());

    /* Destroy the Owner */
    GuardedMutex->Owner = NULL;

    /* Add the Lock Bit */
    OldValue = InterlockedExchangeAdd(&GuardedMutex->Count, GM_LOCK_BIT);
    ASSERT((OldValue & GM_LOCK_BIT) == 0);

    /* Check if it was already locked, but not woken */
    if ((OldValue) && !(OldValue & GM_LOCK_WAITER_WOKEN))
    {
        /* Update the Oldvalue to what it should be now */
        OldValue += GM_LOCK_BIT;

        /* The mutex will be woken, minus one waiter */
        NewValue = OldValue + GM_LOCK_WAITER_WOKEN -
            GM_LOCK_WAITER_INC;

        /* Remove the Woken bit */
        if (InterlockedCompareExchange(&GuardedMutex->Count,
                                       NewValue,
                                       OldValue) == OldValue)
        {
            /* Signal the Gate */
            KeSignalGateBoostPriority(&GuardedMutex->Gate);
        }
    }
}

FORCEINLINE
VOID
_KeAcquireGuardedMutex(IN PKGUARDED_MUTEX GuardedMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Sanity checks */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(GuardedMutex->Owner != Thread);

    /* Disable Special APCs */
    KeEnterGuardedRegionThread(Thread);

    /* Remove the lock */
    if (!InterlockedBitTestAndReset(&GuardedMutex->Count, GM_LOCK_BIT_V))
    {
        /* The Guarded Mutex was already locked, enter contented case */
        KiAcquireGuardedMutex(GuardedMutex);
    }

    /* Set the Owner and Special APC Disable state */
    GuardedMutex->Owner = Thread;
    GuardedMutex->SpecialApcDisable = Thread->SpecialApcDisable;
}

FORCEINLINE
VOID
_KeReleaseGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();
    LONG OldValue, NewValue;

    /* Sanity checks */
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);
    ASSERT(GuardedMutex->Owner == Thread);
    ASSERT(Thread->SpecialApcDisable == GuardedMutex->SpecialApcDisable);

    /* Destroy the Owner */
    GuardedMutex->Owner = NULL;

    /* Add the Lock Bit */
    OldValue = InterlockedExchangeAdd(&GuardedMutex->Count, GM_LOCK_BIT);
    ASSERT((OldValue & GM_LOCK_BIT) == 0);

    /* Check if it was already locked, but not woken */
    if ((OldValue) && !(OldValue & GM_LOCK_WAITER_WOKEN))
    {
        /* Update the Oldvalue to what it should be now */
        OldValue += GM_LOCK_BIT;

        /* The mutex will be woken, minus one waiter */
        NewValue = OldValue + GM_LOCK_WAITER_WOKEN -
            GM_LOCK_WAITER_INC;

        /* Remove the Woken bit */
        if (InterlockedCompareExchange(&GuardedMutex->Count,
                                       NewValue,
                                       OldValue) == OldValue)
        {
            /* Signal the Gate */
            KeSignalGateBoostPriority(&GuardedMutex->Gate);
        }
    }

    /* Re-enable APCs */
    KeLeaveGuardedRegionThread(Thread);
}

FORCEINLINE
BOOLEAN
_KeTryToAcquireGuardedMutex(IN OUT PKGUARDED_MUTEX GuardedMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Block APCs */
    KeEnterGuardedRegionThread(Thread);

    /* Remove the lock */
    if (!InterlockedBitTestAndReset(&GuardedMutex->Count, GM_LOCK_BIT_V))
    {
        /* Re-enable APCs */
        KeLeaveGuardedRegionThread(Thread);
        YieldProcessor();

        /* Return failure */
        return FALSE;
    }

    /* Set the Owner and APC State */
    GuardedMutex->Owner = Thread;
    GuardedMutex->SpecialApcDisable = Thread->SpecialApcDisable;
    return TRUE;
}


FORCEINLINE
VOID
KiAcquireNmiListLock(OUT PKIRQL OldIrql)
{
    KeAcquireSpinLock(&KiNmiCallbackListLock, OldIrql);
}

FORCEINLINE
VOID
KiReleaseNmiListLock(IN KIRQL OldIrql)
{
    KeReleaseSpinLock(&KiNmiCallbackListLock, OldIrql);
}

#if defined(_M_IX86) || defined(_M_AMD64)
FORCEINLINE
VOID
KiCpuId(
    PCPU_INFO CpuInfo,
    ULONG Function)
{
    __cpuid((INT*)CpuInfo->AsUINT32, Function);
}

FORCEINLINE
VOID
KiCpuIdEx(
    PCPU_INFO CpuInfo,
    ULONG Function,
    ULONG SubFunction)
{
    __cpuidex((INT*)CpuInfo->AsUINT32, Function, SubFunction);
}
#endif /* _M_IX86 || _M_AMD64 */

#ifdef __cplusplus
} // extern "C"
#endif
