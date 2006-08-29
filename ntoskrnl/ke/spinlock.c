/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/spinlock.c
 * PURPOSE:         Implements spinlocks
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#undef KefAcquireSpinLockAtDpcLevel
#undef KeAcquireSpinLockAtDpcLevel
#undef KefReleaseSpinLockFromDpcLevel
#undef KeReleaseSpinLockFromDpcLevel

#define LQ_WAIT     1
#define LQ_OWN      2

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 *
 * FUNCTION: Synchronizes the execution of a given routine with the ISR
 * of a given interrupt object
 * ARGUMENTS:
 *       Interrupt = Interrupt object to synchronize with
 *       SynchronizeRoutine = Routine to call whose execution is
 *                            synchronized with the ISR
 *       SynchronizeContext = Parameter to pass to the synchronized routine
 * RETURNS: TRUE if the operation succeeded
 */
BOOLEAN
STDCALL
KeSynchronizeExecution(PKINTERRUPT Interrupt,
                       PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
                       PVOID SynchronizeContext)
{
    KIRQL OldIrql;
    BOOLEAN Status;

    /* Raise IRQL and acquire lock on MP */
    OldIrql = KeAcquireInterruptSpinLock(Interrupt);

    /* Call the routine */
    Status = SynchronizeRoutine(SynchronizeContext);

    /* Release lock and lower IRQL */
    KeReleaseInterruptSpinLock(Interrupt, OldIrql);

    /* Return routine status */
    return Status;
}

/*
 * @implemented
 */
KIRQL
STDCALL
KeAcquireInterruptSpinLock(IN PKINTERRUPT Interrupt)
{
    KIRQL OldIrql;

    /* Raise IRQL */
    KeRaiseIrql(Interrupt->SynchronizeIrql, &OldIrql);

    /* Acquire spinlock on MP */
    KiAcquireSpinLock(Interrupt->ActualLock);
    return OldIrql;
}

/*
 * @implemented
 *
 * FUNCTION: Initalizes a spinlock
 * ARGUMENTS:
 *           SpinLock = Caller supplied storage for the spinlock
 */
VOID
STDCALL
KeInitializeSpinLock(PKSPIN_LOCK SpinLock)
{
    *SpinLock = 0;
}

/*
 * @implemented
 */
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel(PKSPIN_LOCK SpinLock)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    KiAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 *
 * FUNCTION: Acquires a spinlock when the caller is already running at
 * dispatch level
 * ARGUMENTS:
 *        SpinLock = Spinlock to acquire
 */
VOID
STDCALL
KeAcquireSpinLockAtDpcLevel (PKSPIN_LOCK SpinLock)
{
    KefAcquireSpinLockAtDpcLevel(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel(PKSPIN_LOCK SpinLock)
{
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    KiReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 *
 * FUNCTION: Releases a spinlock when the caller was running at dispatch
 * level before acquiring it
 * ARGUMENTS:
 *         SpinLock = Spinlock to release
 */
VOID
STDCALL
KeReleaseSpinLockFromDpcLevel (PKSPIN_LOCK SpinLock)
{
    KefReleaseSpinLockFromDpcLevel(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KiAcquireSpinLock(PKSPIN_LOCK SpinLock)
{
#ifdef CONFIG_SMP
    for (;;)
    {
        /* Try to acquire it */
        if (InterlockedBitTestAndSet((PLONG)SpinLock, 0))
        {
            /* Value changed... wait until it's locked */
            while (*(volatile KSPIN_LOCK *)SpinLock == 1) YieldProcessor();
        }
        else
        {
            /* All is well, break out */
            break;
        }
    }
#endif /* CONFIG_SMP */
}

/*
 * @implemented
 */
VOID
STDCALL
KeReleaseInterruptSpinLock(IN PKINTERRUPT Interrupt,
                           IN KIRQL OldIrql)
{
    /* Release lock on MP */
    KiReleaseSpinLock(Interrupt->ActualLock);

    /* Lower IRQL */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
FASTCALL
KiReleaseSpinLock(PKSPIN_LOCK SpinLock)
{
#ifdef CONFIG_SMP
    /* Simply clear it */
    *SpinLock = 0;
#endif
}

VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifdef CONFIG_SMP
    PKSPIN_LOCK_QUEUE Prev;

    /* Set the new lock */
    Prev = (PKSPIN_LOCK_QUEUE)
           InterlockedExchange((PLONG)LockHandle->LockQueue.Lock,
                               (LONG)LockHandle);
    if (!Prev)
    {
        /* There was nothing there before. We now own it */
         *(ULONG_PTR*)&LockHandle->LockQueue.Lock |= LQ_OWN;
        return;
    }

    /* Set the wait flag */
     *(ULONG_PTR*)&LockHandle->LockQueue.Lock |= LQ_WAIT;

    /* Link us */
    Prev->Next = (PKSPIN_LOCK_QUEUE)LockHandle;

    /* Loop and wait */
    while ( *(ULONG_PTR*)&LockHandle->LockQueue.Lock & LQ_WAIT) YieldProcessor();
    return;
#endif
}

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifdef CONFIG_SMP
    KSPIN_LOCK LockVal;
    PKSPIN_LOCK_QUEUE Waiter;

    /* Remove own and wait flags */
     *(ULONG_PTR*)&LockHandle->LockQueue.Lock &= ~(LQ_OWN | LQ_WAIT);
    LockVal = *LockHandle->LockQueue.Lock;

    /* Check if we already own it */
    if (LockVal == (KSPIN_LOCK)LockHandle)
    {
        /* Disown it */
        LockVal = (KSPIN_LOCK)
                  InterlockedCompareExchangePointer(LockHandle->LockQueue.Lock,
                                                    NULL,
                                                    LockHandle);
    }
    if (LockVal == (KSPIN_LOCK)LockHandle) return;

    /* Need to wait for it */
    Waiter = LockHandle->LockQueue.Next;
    while (!Waiter)
    {
        YieldProcessor();
        Waiter = LockHandle->LockQueue.Next;
    }

    /* It's gone */
    *(ULONG_PTR*)&Waiter->Lock ^= (LQ_OWN | LQ_WAIT);
    LockHandle->LockQueue.Next = NULL;
#endif
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock,
                                         IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Set it up properly */
    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;
    KeAcquireQueuedSpinLockAtDpcLevel((PKLOCK_QUEUE_HANDLE)
                                      &LockHandle->LockQueue.Next);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Call the internal function */
    KeReleaseQueuedSpinLockFromDpcLevel((PKLOCK_QUEUE_HANDLE)
                                        &LockHandle->LockQueue.Next);
}

/* EOF */
