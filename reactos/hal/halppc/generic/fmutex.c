/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS HAL
 * FILE:            hal/halppc/generic/fmutex.c
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

/* FUNCTIONS *****************************************************************/

VOID
FASTCALL
ExAcquireFastMutex(PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;

    /* Raise IRQL to APC */
    KeRaiseIrql(APC_LEVEL, &OldIrql);

    /* Decrease the count */
    if (InterlockedDecrement(&FastMutex->Count))
    {
        /* Someone is still holding it, use slow path */
        FastMutex->Contention++;
        KeWaitForSingleObject(&FastMutex->Event,
                              WrExecutive,
                              KernelMode,
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
    KIRQL OldIrql;

    /* Erase the owner */
    FastMutex->Owner = (PVOID)1;
    OldIrql = FastMutex->OldIrql;

    /* Increase the count */
    if (InterlockedIncrement(&FastMutex->Count) <= 0)
    {
        /* Someone was waiting for it, signal the waiter */
        KeSetEventBoostPriority(&FastMutex->Event, IO_NO_INCREMENT);
    }

    /* Lower IRQL back */
    KeLowerIrql(OldIrql);
}

BOOLEAN
FASTCALL
ExiTryToAcquireFastMutex(PFAST_MUTEX FastMutex)
{
    KIRQL OldIrql;

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
