/* $Id$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/rundown.c
 * PURPOSE:         Rundown Protection Functions
 * 
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOLEAN
FASTCALL
ExAcquireRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    /* Call the general function with only one Reference add */
    return ExAcquireRundownProtectionEx(RunRef, 1);
}

/*
 * @implemented
 */
BOOLEAN
FASTCALL
ExAcquireRundownProtectionEx (
    IN PEX_RUNDOWN_REF RunRef,
    IN ULONG Count
    )
{
    ULONG_PTR PrevCount, Current;
    
    PAGED_CODE();
    
    Count <<= EX_RUNDOWN_COUNT_SHIFT;
    
    /* Loop until successfully incremented the counter */
    do
    {
        Current = RunRef->Count;
        
        /* Make sure a rundown is not active */
        if (Current & EX_RUNDOWN_ACTIVE)
        {
            return FALSE;
        }

#ifdef _WIN64
        PrevCount = (ULONG_PTR)InterlockedExchangeAdd64((LONGLONG*)&RunRef->Count, (LONGLONG)Count);
#else
        PrevCount = (ULONG_PTR)InterlockedExchangeAdd((LONG*)&RunRef->Count, (LONG)Count);
#endif
    } while (PrevCount != Current);

    /* Return Success */
    return TRUE;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExInitializeRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    PAGED_CODE();
    
    /* Set the count to zero */
    RunRef->Count = 0;
}

/*
 * @implemented
 */
VOID
FASTCALL
ExReInitializeRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    PAGED_CODE();
    
    /* Reset the count */
#ifdef _WIN64
    InterlockedExchangeAdd64((LONGLONG*)&RunRef->Count, 0LL);
#else
    InterlockedExchangeAdd((LONG*)&RunRef->Count, 0);
#endif
}


/*
 * @implemented
 */
VOID
FASTCALL
ExReleaseRundownProtectionEx (
    IN PEX_RUNDOWN_REF RunRef,
    IN ULONG Count
    )
{
    PAGED_CODE();

    Count <<= EX_RUNDOWN_COUNT_SHIFT;
    
    for (;;)
    {
        ULONG_PTR Current = RunRef->Count;
        
        /* Check if Rundown is active */
        if (Current & EX_RUNDOWN_ACTIVE)
        {
            /* Get Pointer */
            PRUNDOWN_DESCRIPTOR RundownDescriptor = (PRUNDOWN_DESCRIPTOR)(Current & ~EX_RUNDOWN_ACTIVE);
            
            if (RundownDescriptor == NULL)
            {
                /* the rundown was completed and there's no one to notify */
                break;
            }
            
            Current = RundownDescriptor->References;

            /* Decrease RundownDescriptor->References by Count references */
            for (;;)
            {
                ULONG_PTR PrevCount, NewCount;

                if ((Count >> EX_RUNDOWN_COUNT_SHIFT) == Current)
                {
                    NewCount = 0;
                }
                else
                {
                    NewCount = ((RundownDescriptor->References - (Count >> EX_RUNDOWN_COUNT_SHIFT)) << EX_RUNDOWN_COUNT_SHIFT) | EX_RUNDOWN_ACTIVE;
                }
#ifdef _WIN64
                PrevCount = (ULONG_PTR)InterlockedCompareExchange64((LONGLONG*)&RundownDescriptor->References, (LONGLONG)NewCount, (LONGLONG)Current);
#else
                PrevCount = (ULONG_PTR)InterlockedCompareExchange((LONG*)&RundownDescriptor->References, (LONG)NewCount, (LONG)Current);
#endif
                if (PrevCount == Current)
                {
                    if (NewCount == 0)
                    {
                        /* Signal the event so anyone waiting on it can now kill it */
                        KeSetEvent(&RundownDescriptor->RundownEvent, 0, FALSE);
                    }

                    /* Successfully decremented the counter, so bail! */
                    break;
                }
                
                Current = PrevCount;
            }
            
            break;
        }
        else
        {
            ULONG_PTR PrevCount, NewCount = Current - (ULONG_PTR)Count;
#ifdef _WIN64
            PrevCount = (ULONG_PTR)InterlockedCompareExchange64((LONGLONG*)&RunRef->Count, (LONGLONG)NewCount, (LONGLONG)Current);
#else
            PrevCount = (ULONG_PTR)InterlockedCompareExchange((LONG*)&RunRef->Count, (LONG)NewCount, (LONG)Current);
#endif
            if (PrevCount == Current)
            {
                /* Successfully decremented the counter, so bail! */
                break;
            }
        }
    }
}

/*
* @implemented
*/
VOID
FASTCALL
ExReleaseRundownProtection (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    /* Call the general function with only 1 reference removal */
    ExReleaseRundownProtectionEx(RunRef, 1);
}

/*
 * @implemented
 */
VOID
FASTCALL
ExRundownCompleted (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    PAGED_CODE();
    
    /* mark the counter as active */
#ifdef _WIN64
    InterlockedExchange64((LONGLONG*)&RunRef->Count, (LONGLONG)EX_RUNDOWN_ACTIVE);
#else
    InterlockedExchange((LONG*)&RunRef->Count, EX_RUNDOWN_ACTIVE);
#endif
}

/*
 * @implemented
 */
VOID
FASTCALL
ExWaitForRundownProtectionRelease (
    IN PEX_RUNDOWN_REF RunRef
    )
{
    ULONG_PTR PrevCount, NewPtr, PrevPtr;
    RUNDOWN_DESCRIPTOR RundownDescriptor;
    
    PAGED_CODE();
    
    PrevCount = RunRef->Count;
    
    if (PrevCount != 0 && !(PrevCount & EX_RUNDOWN_ACTIVE))
    {
        /* save the reference counter */
        RundownDescriptor.References = PrevCount >> EX_RUNDOWN_COUNT_SHIFT;

        /* Pending references... wait on them to be closed with an event */
        KeInitializeEvent(&RundownDescriptor.RundownEvent, NotificationEvent, FALSE);
        
        ASSERT(!((ULONG_PTR)&RundownDescriptor & EX_RUNDOWN_ACTIVE));
        
        NewPtr = (ULONG_PTR)&RundownDescriptor | EX_RUNDOWN_ACTIVE;
        
        for (;;)
        {
#ifdef _WIN64
            PrevPtr = (ULONG_PTR)InterlockedCompareExchange64((LONGLONG*)&RunRef->Ptr, (LONGLONG)NewPtr, (LONGLONG)PrevCount);
#else
            PrevPtr = (ULONG_PTR)InterlockedCompareExchange((LONG*)&RunRef->Ptr, (LONG)NewPtr, (LONG)PrevCount);
#endif
            if (PrevPtr == PrevCount)
            {
                /* Wait for whoever needs to release to notify us */
                KeWaitForSingleObject(&RundownDescriptor.RundownEvent, Executive, KernelMode, FALSE, NULL);
                break;
            }
            else if (PrevPtr == 0 || (PrevPtr & EX_RUNDOWN_ACTIVE))
            {
                /* some one else was faster, let's just bail */
                break;
            }
            
            PrevCount = PrevPtr;
            
            /* save the changed reference counter and try again */
            RundownDescriptor.References = PrevCount >> EX_RUNDOWN_COUNT_SHIFT;
        }
    }
}

/* EOF */
