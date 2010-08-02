/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/spinlock.h
* PURPOSE:         Internal Inlined Functions for spinlocks, shared with HAL
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

VOID
NTAPI
Kii386SpinOnSpinLock(PKSPIN_LOCK SpinLock, ULONG Flags);

#ifndef CONFIG_SMP

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

#else

//
// Spinlock Acquisition at IRQL >= DISPATCH_LEVEL
//
FORCEINLINE
VOID
KxAcquireSpinLock(IN PKSPIN_LOCK SpinLock)
{
#ifdef DBG
    /* Make sure that we don't own the lock already */
    if (((KSPIN_LOCK)KeGetCurrentThread() | 1) == *SpinLock)
    {
        /* We do, bugcheck! */
        KeBugCheckEx(SPIN_LOCK_ALREADY_OWNED, (ULONG_PTR)SpinLock, 0, 0, 0);
    }
#endif

    /* Try to acquire the lock */
    while (InterlockedBitTestAndSet((PLONG)SpinLock, 0))
    {
#if defined(_M_IX86) && defined(DBG)
        /* On x86 debug builds, we use a much slower but useful routine */
        Kii386SpinOnSpinLock(SpinLock, 5);
#else
        /* It's locked... spin until it's unlocked */
        while (*(volatile KSPIN_LOCK *)SpinLock & 1)
        {
                /* Yield and keep looping */
                YieldProcessor();
        }
#endif
    }
#ifdef DBG
    /* On debug builds, we OR in the KTHREAD */
    *SpinLock = (KSPIN_LOCK)KeGetCurrentThread() | 1;
#endif
}

//
// Spinlock Release at IRQL >= DISPATCH_LEVEL
//
FORCEINLINE
VOID
KxReleaseSpinLock(IN PKSPIN_LOCK SpinLock)
{
#if DBG
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

#endif
