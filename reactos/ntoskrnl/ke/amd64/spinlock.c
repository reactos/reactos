/*
 * PROJECT:         ReactOS HAL
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            hal/halx86/up/spinlock.c
 * PURPOSE:         Spinlock and Queued Spinlock Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#undef KeAcquireSpinLock
#undef KeReleaseSpinLock

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
KIRQL
KeAcquireSpinLockRaiseToSynch(PKSPIN_LOCK SpinLock)
{
#ifndef CONFIG_SMP
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
KIRQL
NTAPI
KeAcquireSpinLockRaiseToDpc(PKSPIN_LOCK SpinLock)
{
#ifndef CONFIG_SMP
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
VOID
NTAPI
KeReleaseSpinLock(PKSPIN_LOCK SpinLock,
                  KIRQL OldIrql)
{
#ifndef CONFIG_SMP
    /* Simply lower IRQL back */
    KfLowerIrql(OldIrql);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
KIRQL
KeAcquireQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
#ifndef CONFIG_SMP
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
KIRQL
KeAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber)
{
#ifndef CONFIG_SMP
    /* Simply raise to dispatch */
    return KfRaiseIrql(DISPATCH_LEVEL);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
VOID
KeAcquireInStackQueuedSpinLock(IN PKSPIN_LOCK SpinLock,
                               IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifndef CONFIG_SMP
    /* Simply raise to dispatch */
    LockHandle->OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
VOID
KeAcquireInStackQueuedSpinLockRaiseToSynch(IN PKSPIN_LOCK SpinLock,
                                           IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifndef CONFIG_SMP
    /* Simply raise to synch */
    LockHandle->OldIrql = KfRaiseIrql(SYNCH_LEVEL);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
VOID
KeReleaseQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                        IN KIRQL OldIrql)
{
#ifndef CONFIG_SMP
    /* Simply lower IRQL back */
    KfLowerIrql(OldIrql);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
VOID
KeReleaseInStackQueuedSpinLock(IN PKLOCK_QUEUE_HANDLE LockHandle)
{
#ifndef CONFIG_SMP
    /* Simply lower IRQL back */
    KfLowerIrql(LockHandle->OldIrql);
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
BOOLEAN
KeTryToAcquireQueuedSpinLockRaiseToSynch(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                                         IN PKIRQL OldIrql)
{
#ifndef CONFIG_SMP
    /* Simply raise to dispatch */
    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

    /* Always return true on UP Machines */
    return TRUE;
#else
    UNIMPLEMENTED;
#endif
}

/*
 * @implemented
 */
LOGICAL
KeTryToAcquireQueuedSpinLock(IN KSPIN_LOCK_QUEUE_NUMBER LockNumber,
                             OUT PKIRQL OldIrql)
{
#ifndef CONFIG_SMP
    /* Simply raise to dispatch */
    *OldIrql = KfRaiseIrql(DISPATCH_LEVEL);

    /* Always return true on UP Machines */
    return TRUE;
#else
    UNIMPLEMENTED;
#endif
}

/* EOF */
