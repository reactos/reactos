/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/fmutex.c
 * PURPOSE:         Implements fast mutexes
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

VOID
FASTCALL
KiAcquireFastMutex(
    IN PFAST_MUTEX FastMutex
);

/* PRIVATE FUNCTIONS *********************************************************/

VOID
FORCEINLINE
ExiAcquireFastMutexUnsafe(IN PFAST_MUTEX FastMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    DPRINT("Sanity print: %d %d %p\n",
            KeGetCurrentIrql(), Thread->CombinedApcDisable, Thread->Teb);

    /* Sanity check */
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread->CombinedApcDisable != 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(FastMutex->Owner != Thread);

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
    {
        /* Someone is still holding it, use slow path */
        KiAcquireFastMutex(FastMutex);
    }

    /* Set the owner */
    FastMutex->Owner = Thread;
}

VOID
FORCEINLINE
ExiReleaseFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (KeGetCurrentThread()->CombinedApcDisable != 0) ||
           (KeGetCurrentThread()->Teb == NULL) ||
           (KeGetCurrentThread()->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT(FastMutex->Owner == KeGetCurrentThread());

    /* Erase the owner */
    FastMutex->Owner = NULL;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Gate, NULL);
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
VOID
FASTCALL
ExEnterCriticalRegionAndAcquireFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    /* Enter the Critical Region */
    KeEnterCriticalRegion();

    /* Acquire the mutex unsafely */
    ExiAcquireFastMutexUnsafe(FastMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(IN OUT PFAST_MUTEX FastMutex)
{
    /* Release the mutex unsafely */
    ExiReleaseFastMutexUnsafe(FastMutex);

    /* Leave the critical region */
    KeLeaveCriticalRegion();
}

/*
 * @implemented
 */
VOID
FASTCALL
ExAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Raise IRQL to APC */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count) != 0)
    {
        /* Someone is still holding it, use slow path */
        KiAcquireFastMutex(FastMutex);
    }

    /* Set the owner and IRQL */
    FastMutex->Owner = KeGetCurrentThread();
    FastMutex->OldIrql = OldIrql;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);

    /* Erase the owner */
    FastMutex->Owner = NULL;
    OldIrql = (KIRQL)FastMutex->OldIrql;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Gate, IO_NO_INCREMENT);
    }

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExAcquireFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    /* Acquire the mutex unsafely */
    ExiAcquireFastMutexUnsafe(FastMutex);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutexUnsafe(IN OUT PFAST_MUTEX FastMutex)
{
    /* Release the mutex unsafely */
    ExiReleaseFastMutexUnsafe(FastMutex);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(IN OUT PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT(KeGetCurrentIrql() <= APC_LEVEL);

    /* Raise to APC_LEVEL */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Check if we can quickly acquire it */
    if (InterlockedCompareExchange(&FastMutex->Count, 0, 1) == 1)
    {
        /* We have, set us as owners */
        FastMutex->Owner = KeGetCurrentThread();
        FastMutex->OldIrql = OldIrql;
        return TRUE;
    }
    else
    {
        /* Acquire attempt failed */
        KeLowerIrql(OldIrql);
        return FALSE;
    }
}

/* EOF */
