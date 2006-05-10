/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/halx86/up/spinlock.c
 * PURPOSE:         Implements spinlocks
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#undef KeAcquireSpinLock
#undef KeReleaseSpinLock

/* FUNCTIONS ***************************************************************/

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
    KfLowerIrql(OldIrql);
}

/*
 * @implemented
 */
KIRQL
FASTCALL
KeAcquireQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle)
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
KeReleaseQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle,
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
KeTryToAcquireQueuedSpinLockRaiseToSynch(IN PKLOCK_QUEUE_HANDLE LockHandle,
                                         IN PKIRQL OldIrql)
{
    /* Simply raise to dispatch */
    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

    /* Always return true on UP Machines */
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
KeTryToAcquireQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle,
                             IN PKIRQL OldIrql)
{
    /* Simply raise to dispatch */
    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

    /* Always return true on UP Machines */
    return TRUE;
}


/* EOF */
