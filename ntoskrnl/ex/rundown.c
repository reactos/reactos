/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/rundown.c
 * PURPOSE:         Rundown and Cache-Aware Rundown Protection
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller
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
    ExpSetRundown(&RunRef->Count, 0);
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
    ExpSetRundown(&RunRef->Count, EX_RUNDOWN_ACTIVE);
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
        NewValue = ExpChangeRundown(RunRef, PtrToUlong(WaitBlockPointer), Value);
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

/* FIXME: STUBS **************************************************************/

/*
 * @unimplemented NT5.2
 */
BOOLEAN
FASTCALL
ExfAcquireRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented NT5.2
 */
BOOLEAN
FASTCALL
ExfAcquireRundownProtectionCacheAwareEx(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
                                        IN ULONG Count)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    DBG_UNREFERENCED_PARAMETER(Count);
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @unimplemented NT5.2
 */
VOID
FASTCALL
ExfReleaseRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    UNIMPLEMENTED;
}

/*
 * @unimplemented NT5.2
 */
VOID
FASTCALL
ExfReleaseRundownProtectionCacheAwareEx(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
                                        IN ULONG Count)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    DBG_UNREFERENCED_PARAMETER(Count);
    UNIMPLEMENTED;
}

/*
 * @unimplemented NT5.2
 */
VOID
FASTCALL
ExfWaitForRundownProtectionReleaseCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    UNIMPLEMENTED;
}

/*
 * @unimplemented NT5.2
 */
VOID
FASTCALL
ExfRundownCompletedCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    UNIMPLEMENTED;
}

/*
 * @unimplemented NT5.2
 */
VOID
FASTCALL
ExfReInitializeRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    UNIMPLEMENTED;
}

/*
 * @unimplemented NT5.2
 */
PEX_RUNDOWN_REF_CACHE_AWARE
NTAPI
ExAllocateCacheAwareRundownProtection(IN POOL_TYPE PoolType,
                                      IN ULONG Tag)
{
    DBG_UNREFERENCED_PARAMETER(PoolType);
    DBG_UNREFERENCED_PARAMETER(Tag);
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @unimplemented NT5.2
 */
VOID
NTAPI
ExFreeCacheAwareRundownProtection(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    UNIMPLEMENTED;
}

/*
 * @unimplemented NT5.2
 */
VOID
NTAPI
ExInitializeRundownProtectionCacheAware(IN PEX_RUNDOWN_REF_CACHE_AWARE RunRefCacheAware,
                                        IN ULONG Count)
{
    DBG_UNREFERENCED_PARAMETER(RunRefCacheAware);
    DBG_UNREFERENCED_PARAMETER(Count);
    UNIMPLEMENTED;
}

/*
 * @unimplemented NT5.2
 */
SIZE_T
NTAPI
ExSizeOfRundownProtectionCacheAware(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

