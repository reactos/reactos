/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/rundown.c
 * PURPOSE:         Rundown and Cache-Aware Rundown Protection
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller
 *                  Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*++
 * @name ExfAcquireRundownProtection
 * @implemented NT5.1
 *
 *     The ExfAcquireRundownProtection routine acquires rundown protection for
 *     the specified descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return TRUE if access to the protected structure was granted, FALSE otherwise.
 *
 * @remarks Callers of ExfAcquireRundownProtection can be running at any IRQL.
 *
 *--*/
BOOLEAN
FASTCALL
ExfAcquireRundownProtection(IN PEX_RUNDOWN_REF RunRef)
{
    ULONG_PTR Value = RunRef->Count, NewValue;

    /* Loop until successfully incremented the counter */
    for (;;)
    {
        /* Make sure a rundown is not active */
        if (Value & EX_RUNDOWN_ACTIVE) return FALSE;

        /* Add a reference */
        NewValue = Value + EX_RUNDOWN_COUNT_INC;

        /* Change the value */
        NewValue = ExpChangeRundown(RunRef, NewValue, Value);
        if (NewValue == Value) return TRUE;

        /* Update it */
        Value = NewValue;
    }
}

/*++
 * @name ExfAcquireRundownProtectionEx
 * @implemented NT5.2
 *
 *     The ExfAcquireRundownProtectionEx routine acquires multiple rundown
 *     protection references for the specified descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @param Count
 *         Number of times to reference the descriptor.
 *
 * @return TRUE if access to the protected structure was granted, FALSE otherwise.
 *
 * @remarks Callers of ExfAcquireRundownProtectionEx can be running at any IRQL.
 *
 *--*/
BOOLEAN
FASTCALL
ExfAcquireRundownProtectionEx(IN PEX_RUNDOWN_REF RunRef,
                              IN ULONG Count)
{
    ULONG_PTR Value = RunRef->Count, NewValue;

    /* Loop until successfully incremented the counter */
    for (;;)
    {
        /* Make sure a rundown is not active */
        if (Value & EX_RUNDOWN_ACTIVE) return FALSE;

        /* Add references */
        NewValue = Value + EX_RUNDOWN_COUNT_INC * Count;

        /* Change the value */
        NewValue = ExpChangeRundown(RunRef, NewValue, Value);
        if (NewValue == Value) return TRUE;

        /* Update the value */
        Value = NewValue;
    }
}

/*++
 * @name ExfInitializeRundownProtection
 * @implemented NT5.1
 *
 *     The ExfInitializeRundownProtection routine initializes a rundown
 *     protection descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks Callers of ExfInitializeRundownProtection can be running at any IRQL.
 *
 *--*/
VOID
FASTCALL
ExfInitializeRundownProtection(IN PEX_RUNDOWN_REF RunRef)
{
    /* Set the count to zero */
    RunRef->Count = 0;
}

/*++
 * @name ExfReInitializeRundownProtection
 * @implemented NT5.1
 *
 *     The ExfReInitializeRundownProtection routine re-initializes a rundown
 *     protection descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks Callers of ExfReInitializeRundownProtection can be running at any IRQL.
 *
 *--*/
VOID
FASTCALL
ExfReInitializeRundownProtection(IN PEX_RUNDOWN_REF RunRef)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT((RunRef->Count & EX_RUNDOWN_ACTIVE) != 0);

    /* Reset the count */
    ExpSetRundown(RunRef, 0);
}

/*++
 * @name ExfRundownCompleted
 * @implemented NT5.1
 *
 *     The ExfRundownCompleted routine completes the rundown of the specified
 *     descriptor by setting the active bit.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks Callers of ExfRundownCompleted must be running at IRQL <= APC_LEVEL.
 *
 *--*/
VOID
FASTCALL
ExfRundownCompleted(IN PEX_RUNDOWN_REF RunRef)
{
    PAGED_CODE();

    /* Sanity check */
    ASSERT((RunRef->Count & EX_RUNDOWN_ACTIVE) != 0);

    /* Mark the counter as active */
    ExpSetRundown(RunRef, EX_RUNDOWN_ACTIVE);
}

/*++
 * @name ExfReleaseRundownProtection
 * @implemented NT5.1
 *
 *     The ExfReleaseRundownProtection routine releases the rundown protection
 *     reference for the specified descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks Callers of ExfReleaseRundownProtection can be running at any IRQL.
 *
 *--*/
VOID
FASTCALL
ExfReleaseRundownProtection(IN PEX_RUNDOWN_REF RunRef)
{
    ULONG_PTR Value = RunRef->Count, NewValue;
    PEX_RUNDOWN_WAIT_BLOCK WaitBlock;

    /* Loop until successfully incremented the counter */
    for (;;)
    {
        /* Check if rundown is not active */
        if (!(Value & EX_RUNDOWN_ACTIVE))
        {
            /* Sanity check */
            ASSERT((Value >= EX_RUNDOWN_COUNT_INC) || (KeNumberProcessors > 1));

            /* Get the new value */
            NewValue = Value - EX_RUNDOWN_COUNT_INC;

            /* Change the value */
            NewValue = ExpChangeRundown(RunRef, NewValue, Value);
            if (NewValue == Value) break;

            /* Update value */
            Value = NewValue;
        }
        else
        {
            /* Get the wait block */
            WaitBlock = (PEX_RUNDOWN_WAIT_BLOCK)(Value & ~EX_RUNDOWN_ACTIVE);
            ASSERT((WaitBlock->Count > 0) || (KeNumberProcessors > 1));

            /* Remove the one count */
            if (!InterlockedDecrementSizeT(&WaitBlock->Count))
            {
                /* We're down to 0 now, so signal the event */
                KeSetEvent(&WaitBlock->WakeEvent, IO_NO_INCREMENT, FALSE);
            }

            /* We're all done */
            break;
        }
    }
}

/*++
 * @name ExfReleaseRundownProtectionEx
 * @implemented NT5.2
 *
 *     The ExfReleaseRundownProtectionEx routine releases multiple rundown
 *     protection references for the specified descriptor.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @param Count
 *         Number of times to dereference the descriptor.
 *
 * @return None.
 *
 * @remarks Callers of ExfAcquireRundownProtectionEx can be running at any IRQL.
 *
 *--*/
VOID
FASTCALL
ExfReleaseRundownProtectionEx(IN PEX_RUNDOWN_REF RunRef,
                              IN ULONG Count)
{
    ULONG_PTR Value = RunRef->Count, NewValue;
    PEX_RUNDOWN_WAIT_BLOCK WaitBlock;

    /* Loop until successfully incremented the counter */
    for (;;)
    {
        /* Check if rundown is not active */
        if (!(Value & EX_RUNDOWN_ACTIVE))
        {
            /* Sanity check */
            ASSERT((Value >= EX_RUNDOWN_COUNT_INC * Count) ||
                   (KeNumberProcessors > 1));

            /* Get the new value */
            NewValue = Value - EX_RUNDOWN_COUNT_INC * Count;

            /* Change the value */
            NewValue = ExpChangeRundown(RunRef, NewValue, Value);
            if (NewValue == Value) break;

            /* Update value */
            Value = NewValue;
        }
        else
        {
            /* Get the wait block */
            WaitBlock = (PEX_RUNDOWN_WAIT_BLOCK)(Value & ~EX_RUNDOWN_ACTIVE);
            ASSERT((WaitBlock->Count >= Count) || (KeNumberProcessors > 1));

            /* Remove the counts */
            if (InterlockedExchangeAddSizeT(&WaitBlock->Count,
                                            -(LONG)Count) == (LONG)Count)
            {
                /* We're down to 0 now, so signal the event */
                KeSetEvent(&WaitBlock->WakeEvent, IO_NO_INCREMENT, FALSE);
            }

            /* We're all done */
            break;
        }
    }
}

/*++
 * @name ExfWaitForRundownProtectionRelease
 * @implemented NT5.1
 *
 *     The ExfWaitForRundownProtectionRelease routine waits until the specified
 *     rundown descriptor has been released.
 *
 * @param RunRef
 *        Pointer to a rundown reference descriptor.
 *
 * @return None.
 *
 * @remarks Callers of ExfWaitForRundownProtectionRelease must be running
 *          at IRQL <= APC_LEVEL.
 *
 *--*/
VOID
FASTCALL
ExfWaitForRundownProtectionRelease(IN PEX_RUNDOWN_REF RunRef)
{
    ULONG_PTR Value, Count, NewValue;
    EX_RUNDOWN_WAIT_BLOCK WaitBlock;
    PEX_RUNDOWN_WAIT_BLOCK WaitBlockPointer;
    PKEVENT Event;
    PAGED_CODE();

    /* Set the active bit */
    Value = ExpChangeRundown(RunRef, EX_RUNDOWN_ACTIVE, 0);
    if ((Value == 0) || (Value == EX_RUNDOWN_ACTIVE)) return;

    /* No event for now */
    Event = NULL;
    WaitBlockPointer = (PEX_RUNDOWN_WAIT_BLOCK)((ULONG_PTR)&WaitBlock |
                                                EX_RUNDOWN_ACTIVE);

    /* Start waitblock set loop */
    for (;;)
    {
        /* Save the count */
        Count = Value >> EX_RUNDOWN_COUNT_SHIFT;

        /* If the count is over one and we don't have en event yet, create it */
        if ((Count) && !(Event))
        {
            /* Initialize the event */
            KeInitializeEvent(&WaitBlock.WakeEvent,
                              SynchronizationEvent,
                              FALSE);

            /* Set the pointer */
            Event = &WaitBlock.WakeEvent;
        }

        /* Set the count */
        WaitBlock.Count = Count;

        /* Now set the pointer */
        NewValue = ExpChangeRundown(RunRef, (ULONG_PTR)WaitBlockPointer, Value);
        if (NewValue == Value) break;

        /* Loop again */
        Value = NewValue;
        ASSERT((Value & EX_RUNDOWN_ACTIVE) == 0);
    }

    /* If the count was 0, we're done */
    if (!Count) return;

    /* Wait for whoever needs to release to notify us */
    KeWaitForSingleObject(Event, Executive, KernelMode, FALSE, NULL);
    ASSERT(WaitBlock.Count == 0);
}

/*
 * @implemented NT5.2
 */
BOOLEAN
FASTCALL
ExfAcquireRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    PEX_RUNDOWN_REF RunRef;

    RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, KeGetCurrentProcessorNumber());
    return _ExAcquireRundownProtection(RunRef);
}

/*
 * @implemented NT5.2
 */
BOOLEAN
FASTCALL
ExfAcquireRundownProtectionCacheAwareEx(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
                                        IN ULONG Count)
{
    PEX_RUNDOWN_REF RunRef;

    RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, KeGetCurrentProcessorNumber());
    return ExfAcquireRundownProtectionEx(RunRef, Count);
}

/*
 * @implemented NT5.2
 */
VOID
FASTCALL
ExfReleaseRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    PEX_RUNDOWN_REF RunRef;

    RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, KeGetCurrentProcessorNumber());
    _ExReleaseRundownProtection(RunRef);
}

/*
 * @implemented NT5.2
 */
VOID
FASTCALL
ExfReleaseRundownProtectionCacheAwareEx(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
                                        IN ULONG Count)
{
    PEX_RUNDOWN_REF RunRef;

    RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, KeGetCurrentProcessorNumber());
    ExfReleaseRundownProtectionEx(RunRef, Count);
}

/*
 * @implemented NT5.2
 */
VOID
FASTCALL
ExfWaitForRundownProtectionReleaseCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    PEX_RUNDOWN_REF RunRef;
    EX_RUNDOWN_WAIT_BLOCK WaitBlock;
    PEX_RUNDOWN_WAIT_BLOCK WaitBlockPointer;
    ULONG ProcCount, Current, Value, OldValue, TotalCount;

    ProcCount = RunRefCacheAware->Number;
    /* No proc, nothing to do */
    if (ProcCount == 0)
    {
        return;
    }

    TotalCount = 0;
    WaitBlock.Count = 0;
    WaitBlockPointer = (PEX_RUNDOWN_WAIT_BLOCK)((ULONG_PTR)&WaitBlock |
                                                EX_RUNDOWN_ACTIVE);
    /* We will check all our runrefs */
    for (Current = 0; Current < ProcCount; ++Current)
    {
        /* Get the runref for the proc */
        RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, Current);
        /* Loop for setting the wait block */
        do
        {
            Value = RunRef->Count;
            ASSERT((Value & EX_RUNDOWN_ACTIVE) == 0);

            /* Remove old value and set our waitblock instead */
            OldValue = ExpChangeRundown(RunRef, WaitBlockPointer, Value);
            if (OldValue == Value)
            {
                break;
            }

            Value = OldValue;
        }
        while (TRUE);

        /* Count the deleted values */
        TotalCount += Value;
    }

    /* Sanity check: we didn't overflow */
    ASSERT((LONG)TotalCount >= 0);
    if (TotalCount != 0)
    {
        /* Init the waitblock event */
        KeInitializeEvent(&WaitBlock.WakeEvent,
                          SynchronizationEvent,
                          FALSE);

        /* Do we have to wait? If so, go ahead! */
        if (InterlockedExchangeAddSizeT(&WaitBlock.Count,
                                        (LONG)TotalCount >> EX_RUNDOWN_COUNT_SHIFT) ==
                                       -(LONG)(TotalCount >> EX_RUNDOWN_COUNT_SHIFT))
        {
            KeWaitForSingleObject(&WaitBlock.WakeEvent, Executive, KernelMode, FALSE, NULL);
        }
    }
}

/*
 * @implemented NT5.2
 */
VOID
FASTCALL
ExfRundownCompletedCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    PEX_RUNDOWN_REF RunRef;
    ULONG ProcCount, Current;

    ProcCount = RunRefCacheAware->Number;
    /* No proc, nothing to do */
    if (ProcCount == 0)
    {
        return;
    }

    /* We will mark all our runrefs active */
    for (Current = 0; Current < ProcCount; ++Current)
    {
        /* Get the runref for the proc */
        RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, Current);
        ASSERT((RunRef->Count & EX_RUNDOWN_ACTIVE) != 0);

        ExpSetRundown(RunRef, EX_RUNDOWN_ACTIVE);
    }
}

/*
 * @implemented NT5.2
 */
VOID
FASTCALL
ExfReInitializeRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    PEX_RUNDOWN_REF RunRef;
    ULONG ProcCount, Current;

    ProcCount = RunRefCacheAware->Number;
    /* No proc, nothing to do */
    if (ProcCount == 0)
    {
        return;
    }

    /* We will mark all our runrefs inactive */
    for (Current = 0; Current < ProcCount; ++Current)
    {
        /* Get the runref for the proc */
        RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, Current);
        ASSERT((RunRef->Count & EX_RUNDOWN_ACTIVE) != 0);

        ExpSetRundown(RunRef, 0);
    }
}

/*
 * @implemented NT5.2
 */
PEX_RUNDOWN_REF_CACHE_AWARE
NTAPI
ExAllocateCacheAwareRundownProtection(IN POOL_TYPE PoolType,
                                      IN ULONG Tag)
{
    PEX_RUNDOWN_REF RunRef;
    PVOID PoolToFree, RunRefs;
    ULONG RunRefSize, Count, Align;
    PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware;

    PAGED_CODE();

    /* Allocate the master structure */
    RunRefCacheAware = ExAllocatePoolWithTag(PoolType, sizeof(EX_RUNDOWN_REF_CACHE_AWARE), Tag);
    if (RunRefCacheAware == NULL)
    {
        return NULL;
    }

    /* Compute the size of each runref */
    RunRefCacheAware->Number = KeNumberProcessors;
    if (KeNumberProcessors <= 1)
    {
        RunRefSize = sizeof(EX_RUNDOWN_REF);
    }
    else
    {
        Align = KeGetRecommendedSharedDataAlignment();
        RunRefSize = Align;
        ASSERT((RunRefSize & (RunRefSize - 1)) == 0);
    }

    /* It must at least hold a EX_RUNDOWN_REF structure */
    ASSERT(sizeof(EX_RUNDOWN_REF) <= RunRefSize);
    RunRefCacheAware->RunRefSize = RunRefSize;

    /* Allocate our runref pool */
    PoolToFree = ExAllocatePoolWithTag(PoolType, RunRefSize * RunRefCacheAware->Number, Tag);
    if (PoolToFree == NULL)
    {
        ExFreePoolWithTag(RunRefCacheAware, Tag);
        return NULL;
    }

    /* On SMP, check for alignment */
    if (RunRefCacheAware->Number > 1 && (ULONG_PTR)PoolToFree & (Align - 1))
    {
        /* Not properly aligned, do it again! */
        ExFreePoolWithTag(PoolToFree, Tag);

        /* Allocate a bigger buffer to be able to align properly */
        PoolToFree = ExAllocatePoolWithTag(PoolType, RunRefSize * (RunRefCacheAware->Number + 1), Tag);
        if (PoolToFree == NULL)
        {
            ExFreePoolWithTag(RunRefCacheAware, Tag);
            return NULL;
        }

        RunRefs = (PVOID)ALIGN_UP_BY(PoolToFree, Align);
    }
    else
    {
        RunRefs = PoolToFree;
    }

    RunRefCacheAware->RunRefs = RunRefs;
    RunRefCacheAware->PoolToFree = PoolToFree;

    /* And initialize runref */
    if (RunRefCacheAware->Number != 0)
    {
        for (Count = 0; Count < RunRefCacheAware->Number; ++Count)
        {
            RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, Count);
            _ExInitializeRundownProtection(RunRef);
        }
    }

    return RunRefCacheAware;
}

/*
 * @implemented NT5.2
 */
VOID
NTAPI
ExFreeCacheAwareRundownProtection(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    PAGED_CODE();

    /*
     * This is to be called for RunRefCacheAware that were allocated with
     * ExAllocateCacheAwareRundownProtection and not for user-allocated
     * ones
     */
    ASSERT(RunRefCacheAware->PoolToFree != (PVOID)0xBADCA11);

    /* We don't know the tag that as used for allocation */
    ExFreePoolWithTag(RunRefCacheAware->PoolToFree, 0);
    ExFreePoolWithTag(RunRefCacheAware, 0);
}

/*
 * @implemented NT5.2
 */
VOID
NTAPI
ExInitializeRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
                                        IN SIZE_T Size)
{
    PVOID Pool;
    PEX_RUNDOWN_REF RunRef;
    ULONG Count, RunRefSize, Align;

    PAGED_CODE();

    /* Get the user allocate pool for runrefs */
    Pool = (PVOID)((ULONG_PTR)RunRefCacheAware + sizeof(EX_RUNDOWN_REF_CACHE_AWARE));

    /* By default a runref is structure-sized */
    RunRefSize = sizeof(EX_RUNDOWN_REF);

    /*
     * If we just have enough room for a single runref, deduce were on a single
     * processor machine
     */
    if (Size == sizeof(EX_RUNDOWN_REF_CACHE_AWARE) + sizeof(EX_RUNDOWN_REF))
    {
        Count = 1;
    }
    else
    {
        /* Get alignment constraint */
        Align = KeGetRecommendedSharedDataAlignment();

        /* How many runrefs given the alignment? */
        RunRefSize = Align;
        Count = ((Size - sizeof(EX_RUNDOWN_REF_CACHE_AWARE)) / Align) - 1;
        Pool = (PVOID)ALIGN_UP_BY(Pool, Align);
    }

    /* Initialize the structure */
    RunRefCacheAware->RunRefs = Pool;
    RunRefCacheAware->RunRefSize = RunRefSize;
    RunRefCacheAware->Number = Count;

    /* There is no allocated pool! */
    RunRefCacheAware->PoolToFree = (PVOID)0xBADCA11u;

    /* Initialize runref */
    if (RunRefCacheAware->Number != 0)
    {
        for (Count = 0; Count < RunRefCacheAware->Number; ++Count)
        {
            RunRef = ExGetRunRefForGivenProcessor(RunRefCacheAware, Count);
            _ExInitializeRundownProtection(RunRef);
        }
    }
}

/*
 * @implemented NT5.2
 */
SIZE_T
NTAPI
ExSizeOfRundownProtectionCacheAware(VOID)
{
    SIZE_T Size;

    PAGED_CODE();

    /* Compute the needed size for runrefs */
    if (KeNumberProcessors <= 1)
    {
        Size = sizeof(EX_RUNDOWN_REF);
    }
    else
    {
        /* We +1, to have enough room for alignment */
        Size = (KeNumberProcessors + 1) * KeGetRecommendedSharedDataAlignment();
    }

    /* Return total size (master structure and runrefs) */
    return Size + sizeof(EX_RUNDOWN_REF_CACHE_AWARE);
}

