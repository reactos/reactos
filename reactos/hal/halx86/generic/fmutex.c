/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS HAL
 * FILE:            ntoskrnl/hal/x86/fmutex.c
 * PURPOSE:         Deprecated HAL Fast Mutex
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/*
 * NOTE: Even HAL itself has #defines to use the Exi* APIs inside NTOSKRNL.
 *       These are only exported here for compatibility with really old
 *       drivers. Also note that in theory, these can be made much faster
 *       by using assembly and inlining all the operations, including
 *       raising and lowering irql.
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#define NDEBUG
#include <debug.h>

#undef ExAcquireFastMutex
#undef ExReleaseFastMutex
#undef ExTryToAcquireFastMutex

/* FUNCTIONS *****************************************************************/

VOID
FASTCALL
ExAcquireFastMutex(PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;

    /* Raise IRQL to APC */
    OldIrql = KfRaiseIrql(APC_LEVEL);

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
    {
        /* Someone is still holding it, use slow path */
        FastMutex->Contention++;
        KeWaitForSingleObject(&FastMutex->Gate,
                              WrExecutive,
                              WaitAny,
                              FALSE,
                              NULL);
    }

    /* Set the owner and IRQL */
    FastMutex->Owner = KeGetCurrentThread();
    FastMutex->OldIrql = OldIrql;
}

VOID
FASTCALL
ExReleaseFastMutex(PFAST_MUTEX FastMutex)
{
    /* Erase the owner */
    FastMutex->Owner = (PVOID)1;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Gate, IO_NO_INCREMENT);
    }

    /* Lower IRQL back */
    KfLowerIrql(FastMutex->OldIrql);
}

BOOLEAN
FASTCALL
ExTryToAcquireFastMutex(PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;

    /* Raise to APC_LEVEL */
    OldIrql = KfRaiseIrql(APC_LEVEL);

    /* Check if we can quickly acquire it */
    if (InterlockedCompareExchange(&FastMutex->Count, 0, 1) == 1)
    {
        /* We have, set us as owners */
        FastMutex->Owner = KeGetCurrentThread();
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
