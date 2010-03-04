/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/spinlock.c
 * PURPOSE:         Spinlock and Queued Spinlock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define LQ_WAIT     1
#define LQ_OWN      2

/* PRIVATE FUNCTIONS *********************************************************/

#if 0
//
// FIXME: The queued spinlock routines are broken.
//

VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel(IN PKSPIN_LOCK_QUEUE LockHandle)
{
#ifdef CONFIG_SMP
    PKSPIN_LOCK_QUEUE Prev;

    /* Set the new lock */
    Prev = (PKSPIN_LOCK_QUEUE)
           InterlockedExchange((PLONG)LockHandle->Next,
                               (LONG)LockHandle);
    if (!Prev)
    {
        /* There was nothing there before. We now own it */
         *LockHandle->Lock |= LQ_OWN;
        return;
    }

    /* Set the wait flag */
     *LockHandle->Lock |= LQ_WAIT;

    /* Link us */
    Prev->Next = (PKSPIN_LOCK_QUEUE)LockHandle;

    /* Loop and wait */
    while (*LockHandle->Lock & LQ_WAIT)
        YieldProcessor();
#endif
}

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel(IN PKSPIN_LOCK_QUEUE LockHandle)
{
#ifdef CONFIG_SMP
    KSPIN_LOCK LockVal;
    PKSPIN_LOCK_QUEUE Waiter;

    /* Remove own and wait flags */
    *LockHandle->Lock &= ~(LQ_OWN | LQ_WAIT);
    LockVal = *LockHandle->Lock;

    /* Check if we already own it */
    if (LockVal == (KSPIN_LOCK)LockHandle)
    {
        /* Disown it */
        LockVal = (KSPIN_LOCK)
                  InterlockedCompareExchangePointer(LockHandle->Lock,
                                                    NULL,
                                                    LockHandle);
    }
    if (LockVal == (KSPIN_LOCK)LockHandle) return;

    /* Need to wait for it */
    Waiter = LockHandle->Next;
    while (!Waiter)
    {
        YieldProcessor();
        Waiter = LockHandle->Next;
    }

    /* It's gone */
    *(ULONG_PTR*)&Waiter->Lock ^= (LQ_OWN | LQ_WAIT);
    LockHandle->Next = NULL;
#endif
}

#else
//
// HACK: Hacked to work like normal spinlocks
//

VOID
FASTCALL
KeAcquireQueuedSpinLockAtDpcLevel(IN PKSPIN_LOCK_QUEUE LockHandle)
{
#ifdef CONFIG_SMP
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)LockHandle->Lock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxAcquireSpinLock(LockHandle->Lock);
#endif
}

VOID
FASTCALL
KeReleaseQueuedSpinLockFromDpcLevel(IN PKSPIN_LOCK_QUEUE LockHandle)
{
#ifdef CONFIG_SMP
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)LockHandle->Lock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxReleaseSpinLock(LockHandle->Lock);
#endif
}

#endif

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
KIRQL
NTAPI
KeAcquireInterruptSpinLock(IN PKINTERRUPT Interrupt)
{
    KIRQL OldIrql;

    /* Raise IRQL */
    KeRaiseIrql(Interrupt->SynchronizeIrql, &OldIrql);

    /* Acquire spinlock on MP */
    KeAcquireSpinLockAtDpcLevel(Interrupt->ActualLock);
    return OldIrql;
}

/*
 * @implemented
 */
VOID
NTAPI
KeReleaseInterruptSpinLock(IN PKINTERRUPT Interrupt,
                           IN KIRQL OldIrql)
{
    /* Release lock on MP */
    KeReleaseSpinLockFromDpcLevel(Interrupt->ActualLock);

    /* Lower IRQL */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
NTAPI
_KeInitializeSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Clear it */
    *SpinLock = 0;
}

/*
 * @implemented
 */
#undef KeAcquireSpinLockAtDpcLevel
VOID
NTAPI
KeAcquireSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
#undef KeReleaseSpinLockFromDpcLevel
VOID
NTAPI
KeReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)SpinLock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Do the inlined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KiAcquireSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Do the inlined function */
    KxAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
VOID
FASTCALL
KiReleaseSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Do the lined function */
    KxReleaseSpinLock(SpinLock);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireSpinLockAtDpcLevel(IN OUT PKSPIN_LOCK SpinLock)
{
#ifdef CONFIG_SMP
    /* Check if it's already acquired */
    if (!(*SpinLock))
    {
        /* Try to acquire it */
        if (InterlockedBitTestAndSet((PLONG)SpinLock, 0))
        {
            /* Someone else acquired it */
            return FALSE;
        }
    }
    else
    {
        /* It was already acquired */
        return FALSE;
    }

#if DBG
    /* On debug builds, we OR in the KTHREAD */
    *SpinLock = (ULONG_PTR)KeGetCurrentThread() | 1;
#endif
#endif

    /* All is well, return TRUE */
    return TRUE;
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockAtDpcLevel(IN PKSPIN_LOCK SpinLock,
                                         IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifdef CONFIG_SMP
    /* Set it up properly */
    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;
#if 0
    KeAcquireQueuedSpinLockAtDpcLevel(LockHandle->LockQueue.Next);
#else
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)LockHandle->LockQueue.Lock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Acquire the lock */
    KxAcquireSpinLock(LockHandle->LockQueue.Lock); // HACK
#endif
#endif
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockFromDpcLevel(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifdef CONFIG_SMP
#if 0
    /* Call the internal function */
    KeReleaseQueuedSpinLockFromDpcLevel(LockHandle->LockQueue.Next);
#else
    /* Make sure we are at DPC or above! */
    if (KeGetCurrentIrql() < DISPATCH_LEVEL)
    {
        /* We aren't -- bugcheck */
        KeBugCheckEx(IRQL_NOT_GREATER_OR_EQUAL,
                     (ULONG_PTR)LockHandle->LockQueue.Lock,
                     KeGetCurrentIrql(),
                     0,
                     0);
    }

    /* Release the lock */
    KxReleaseSpinLock(LockHandle->LockQueue.Lock); // HACK
#endif
#endif
}

/*
 * @unimplemented
 */
KIRQL
FASTCALL
KeAcquireSpinLockForDpc(IN PKSPIN_LOCK SpinLock)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
VOID
FASTCALL
KeReleaseSpinLockForDpc(IN PKSPIN_LOCK SpinLock,
                        IN KIRQL OldIrql)
{
    UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
KIRQL
FASTCALL
KeAcquireInStackQueuedSpinLockForDpc(IN PKSPIN_LOCK SpinLock,
                                     IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLockForDpc(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTestSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Test this spinlock */
    if (*SpinLock)
    {
        /* Spinlock is busy, yield execution */
        YieldProcessor();

        /* Return busy flag */
        return FALSE;
    }

    /* Spinlock appears to be free */
    return TRUE;
}

#ifdef _M_IX86
VOID
NTAPI
Kii386SpinOnSpinLock(PKSPIN_LOCK SpinLock, ULONG Flags)
{
    // FIXME: Handle flags
    UNREFERENCED_PARAMETER(Flags);

    /* Spin until it's unlocked */
    while (*(volatile KSPIN_LOCK *)SpinLock & 1)
    {
        // FIXME: Check for timeout

        /* Yield and keep looping */
        YieldProcessor();
    }
}
#endif
