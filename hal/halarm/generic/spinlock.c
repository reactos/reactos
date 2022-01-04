/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            hal/halarm/generic/spinlock.c
 * PURPOSE:         SpinLock Routines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#undef KeAcquireSpinLock
#undef KeReleaseSpinLock
#undef KeRaiseIrql
#undef KeLowerIrql

/* FUNCTIONS *****************************************************************/

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
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
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
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KfReleaseSpinLock(PKSPIN_LOCK SpinLock,
                  KIRQL OldIrql)
{
    /* Simply lower IRQL back */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLock(IN PKSPIN_LOCK SpinLock,
                               IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Simply raise to dispatch */
    LockHandle->OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeAcquireInStackQueuedSpinLockRaiseToSynch(IN PKSPIN_LOCK SpinLock,
                                           IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Simply raise to dispatch */
    LockHandle->OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                        IN KIRQL OldIrql)
{
    /* Simply lower IRQL back */
    KfLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
FASTCALL
KeReleaseInStackQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
    /* Simply lower IRQL back */
    KfLowerIrql(LockHandle->OldIrql);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                                         IN PKIRQL OldIrql)
{
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
    /* Simply raise to dispatch */
    KeRaiseIrql(DISPATCH_LEVEL, OldIrql);

    /* Always return true on UP Machines */
    return TRUE;
}

/* EOF */
