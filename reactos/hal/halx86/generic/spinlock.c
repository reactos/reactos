/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/up/spinlock.c
 * PURPOSE:         Spinlock and Queued Spinlock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

/* Enable this (and the define in irq.S) to make UP HAL work for MP Kernel */
/* #define CONFIG_SMP */

#include <hal.h>
#define NDEBUG
#include <debug.h>

#undef KeAcquireSpinLock
#undef KeReleaseSpinLock

//
// This is duplicated from ke_x.h
//
#ifdef CONFIG_SMP
//
// Spinlock Acquisition at IRQL >= DISPATCH_LEVEL
//
FORCEINLINE
VOID
KxAcquireSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* Make sure that we don't own the lock already */
    if (((KSPIN_LOCK)KeGetCurrentThread() | 1) == *SpinLock)
    {
        /* We do, bugcheck! */
        KeBugCheckEx(SPIN_LOCK_ALREADY_OWNED, (ULONG_PTR)SpinLock, 0, 0, 0);
    }

    for (;;)
    {
        /* Try to acquire it */
        if (InterlockedBitTestAndSet((PLONG)SpinLock, 0))
        {
            /* Value changed... wait until it's locked */
            while (*(volatile KSPIN_LOCK *)SpinLock == 1)
            {
#ifdef DBG
                /* On debug builds, we use a much slower but useful routine */
                //Kii386SpinOnSpinLock(SpinLock, 5);

                /* FIXME: Do normal yield for now */
                YieldProcessor();
#else
                /* Otherwise, just yield and keep looping */
                YieldProcessor();
#endif
            }
        }
        else
        {
#ifdef DBG
            /* On debug builds, we OR in the KTHREAD */
            *SpinLock = (KSPIN_LOCK)KeGetCurrentThread() | 1;
#endif
            /* All is well, break out */
            break;
        }
    }
}

//
// Spinlock Release at IRQL >= DISPATCH_LEVEL
//
FORCEINLINE
VOID
KxReleaseSpinLock(IN PKSPIN_LOCK SpinLock)
{
#ifdef DBG
    /* Make sure that the threads match */
    if (((KSPIN_LOCK)KeGetCurrentThread() | 1) != *SpinLock)
    {
        /* They don't, bugcheck */
        KeBugCheckEx(SPIN_LOCK_NOT_OWNED, (ULONG_PTR)SpinLock, 0, 0, 0);
    }
#endif
    /* Clear the lock */
    InterlockedAnd((PLONG)SpinLock, 0);
}

#else

//
// Spinlock Acquire at IRQL >= DISPATCH_LEVEL
//
FORCEINLINE
VOID
KxAcquireSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* On UP builds, spinlocks don't exist at IRQL >= DISPATCH */
    UNREFERENCED_PARAMETER(SpinLock);
}

//
// Spinlock Release at IRQL >= DISPATCH_LEVEL
//
FORCEINLINE
VOID
KxReleaseSpinLock(IN PKSPIN_LOCK SpinLock)
{
    /* On UP builds, spinlocks don't exist at IRQL >= DISPATCH */
    UNREFERENCED_PARAMETER(SpinLock);
}

#endif

/* GLOBALS ********************************************************************/

ULONG HalpSystemHardwareFlags;
KSPIN_LOCK HalpSystemHardwareLock;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
HalpAcquireSystemHardwareSpinLock(VOID)
{
    ULONG Flags;

    /* Get flags and disable interrupts */
    Flags = __readeflags();
    _disable();

    /* Acquire the lock */
    KxAcquireSpinLock(&HalpSystemHardwareLock);

    /* We have the lock, save the flags now */
    HalpSystemHardwareFlags = Flags;
}

VOID
NTAPI
HalpReleaseCmosSpinLock(VOID)
{
    ULONG Flags;

    /* Get the flags */
    Flags = HalpSystemHardwareFlags;

    /* Release the lock */
    KxReleaseSpinLock(&HalpSystemHardwareLock);

    /* Restore the flags */
    __writeeflags(Flags);
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
KeAcquireSpinLock(PKSPIN_LOCK SpinLock,
                  PKIRQL OldIrql)
{
    /* Call the fastcall function */
    *OldIrql = KfAcquireSpinLock(SpinLock);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireSpinLockRaiseToSynch(PKSPIN_LOCK SpinLock)
{
    KIRQL OldIrql;

    /* Raise to sync */
    KeRaiseIrql(SYNCH_LEVEL, &OldIrql);

    /* Acquire the lock and return */
    KxAcquireSpinLock(SpinLock);
    return OldIrql;
}

/*
 * @implemented
 */
VOID
NTAPI
KeReleaseSpinLock(PKSPIN_LOCK SpinLock,
                  KIRQL NewIrql)
{
    /* Call the fastcall function */
    KfReleaseSpinLock(SpinLock, NewIrql);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KfAcquireSpinLock(PKSPIN_LOCK SpinLock)
{
    KIRQL OldIrql;

    /* Raise to dispatch and acquire the lock */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    KxAcquireSpinLock(SpinLock);
    return OldIrql;
}

/*
 * @implemented
 */
VOID
FASTCALL
KfReleaseSpinLock(PKSPIN_LOCK SpinLock,
                  KIRQL OldIrql)
{
    /* Release the lock and lower IRQL back */
    KxReleaseSpinLock(SpinLock);
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
    KIRQL OldIrql;

    /* Raise to dispatch */
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    /* Acquire the lock */
    KxAcquireSpinLock(KeGetCurrentPrcb()->LockQueue[LockNumber].Lock); // HACK
    return OldIrql;
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
    KIRQL OldIrql;

    /* Raise to synch */
    KeRaiseIrql(SYNCH_LEVEL, &OldIrql);

    /* Acquire the lock */
    KxAcquireSpinLock(KeGetCurrentPrcb()->LockQueue[LockNumber].Lock); // HACK
    return OldIrql;
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(IN PKSPIN_LOCK SpinLock,
                               IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Set up the lock */
    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;

    /* Raise to dispatch */
    KeRaiseIrql(DISPATCH_LEVEL, &LockHandle->OldIrql);

    /* Acquire the lock */
    KxAcquireSpinLock(LockHandle->LockQueue.Lock); // HACK
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockRaiseToSynch(IN PKSPIN_LOCK SpinLock,
                                           IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Set up the lock */
    LockHandle->LockQueue.Next = NULL;
    LockHandle->LockQueue.Lock = SpinLock;

    /* Raise to synch */
    KeRaiseIrql(SYNCH_LEVEL, &LockHandle->OldIrql);

    /* Acquire the lock */
    KxAcquireSpinLock(LockHandle->LockQueue.Lock); // HACK
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                        IN KIRQL OldIrql)
{
    /* Release the lock */
    KxReleaseSpinLock(KeGetCurrentPrcb()->LockQueue[LockNumber].Lock); // HACK

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Simply lower IRQL back */
    KxReleaseSpinLock(LockHandle->LockQueue.Lock); // HACK
    KeLowerIrql(LockHandle->OldIrql);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                                         IN PKIRQL OldIrql)
{
#ifdef CONFIG_SMP
    ASSERT(FALSE); // FIXME: Unused
    while (TRUE);
#endif

    /* Simply raise to synch */
    KeRaiseIrql(SYNCH_LEVEL, OldIrql);

    /* Always return true on UP Machines */
    return TRUE;
}

/*
 * @implemented
 */
LOGICAL
FASTCALL
KeTryToAcquireQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                             OUT PKIRQL OldIrql)
{
#ifdef CONFIG_SMP
    ASSERT(FALSE); // FIXME: Unused
    while (TRUE);
#endif

    /* Simply raise to dispatch */
    KeRaiseIrql(DISPATCH_LEVEL, OldIrql);

    /* Always return true on UP Machines */
    return TRUE;
}

#undef KeRaiseIrql
/*
 * @implemented
 */
VOID
NTAPI
KeRaiseIrql(KIRQL NewIrql,
            PKIRQL OldIrql)
{
    /* Call the fastcall function */
    *OldIrql = KfRaiseIrql(NewIrql);
}

#undef KeLowerIrql
/*
 * @implemented
 */
VOID
NTAPI
KeLowerIrql(KIRQL NewIrql)
{
    /* Call the fastcall function */
    KfLowerIrql(NewIrql);
}
