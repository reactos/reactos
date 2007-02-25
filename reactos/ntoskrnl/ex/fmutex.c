/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/fmutex.c
 * PURPOSE:         Implements fast mutexes
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

VOID
FASTCALL
KiAcquireFastMutex(IN PFAST_MUTEX FastMutex);

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
FASTCALL
ExEnterCriticalRegionAndAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();

    /* Enter the Critical Region */
    KeEnterCriticalRegion();
    /*
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread == NULL) ||
           (Thread->CombinedApcDisable != 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    */
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread == NULL) ||
           (Thread->CombinedApcDisable != 0));
           
    ASSERT((Thread == NULL) || (FastMutex->Owner != Thread));

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
    {
        /* Someone is still holding it, use slow path */
        KiAcquireFastMutex(FastMutex);
    }

    /* Set the owner */
    FastMutex->Owner = Thread;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutexUnsafeAndLeaveCriticalRegion(PFAST_MUTEX FastMutex)
{
    /*
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (KeGetCurrentThread() == NULL) ||
           (KeGetCurrentThread()->CombinedApcDisable != 0) ||
           (KeGetCurrentThread()->Teb == NULL) ||
           (KeGetCurrentThread()->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
   
    */
     ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (KeGetCurrentThread() == NULL) ||
           (KeGetCurrentThread()->CombinedApcDisable != 0));
     ASSERT(FastMutex->Owner == KeGetCurrentThread());        
  
    /* Erase the owner */
    FastMutex->Owner = NULL;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Gate, IO_NO_INCREMENT);
    }

    /* Leave the critical region */
    KeLeaveCriticalRegion();
}

/*
 * @implemented
 */
VOID
FASTCALL
ExAcquireFastMutex(PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);

    /* Raise IRQL to APC */
    OldIrql = KfRaiseIrql(APC_LEVEL);
   
    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
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
ExReleaseFastMutex (PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT_IRQL(APC_LEVEL);

    /* Erase the owner */
    FastMutex->Owner = NULL;
    OldIrql = FastMutex->OldIrql;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Gate, IO_NO_INCREMENT);
    }

    /* Lower IRQL back */
    KfLowerIrql(OldIrql);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExAcquireFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
    PKTHREAD Thread = KeGetCurrentThread();
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (Thread == NULL) ||
           (Thread->CombinedApcDisable != 0) ||
           (Thread->Teb == NULL) ||
           (Thread->Teb >= (PTEB)MM_SYSTEM_RANGE_START));
    ASSERT((Thread == NULL) || (FastMutex->Owner != Thread));

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
    {
        /* Someone is still holding it, use slow path */
        KiAcquireFastMutex(FastMutex);
    }

    /* Set the owner */
    FastMutex->Owner = Thread;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseFastMutexUnsafe(PFAST_MUTEX FastMutex)
{
    ASSERT((KeGetCurrentIrql() == APC_LEVEL) ||
           (KeGetCurrentThread() == NULL) ||
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
        KeSetEventBoostPriority(&FastMutex->Gate, IO_NO_INCREMENT);
    }
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;
    ASSERT_IRQL_LESS_OR_EQUAL(APC_LEVEL);

    /* Raise to APC_LEVEL */
    OldIrql = KfRaiseIrql(APC_LEVEL);

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
        KfLowerIrql(OldIrql);
        return FALSE;
    }
}

/* EOF */
