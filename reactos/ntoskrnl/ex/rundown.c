/*
 *  ReactOS kernel
 *  Copyright (C) 1998 - 2004 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/rundown.c
 * PURPOSE:         Rundown Protection Functions
 * PORTABILITY:     Checked
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
    Count <<= EX_RUNDOWN_COUNT_SHIFT;
    
    for (;;)
    {
        ULONG_PTR Current = RunRef->Count;
        
        /* Check if Rundown is active */
        if (Current & EX_RUNDOWN_ACTIVE)
        {
            /* Get Pointer */
            PRUNDOWN_DESCRIPTOR RundownDescriptor = (PRUNDOWN_DESCRIPTOR)((ULONG_PTR)RunRef->Ptr & ~EX_RUNDOWN_ACTIVE);

            /* Decrease Reference Count by RundownDescriptor->References */
            for (;;)
            {
                ULONG_PTR PrevCount, NewCount;

                if ((Current >> EX_RUNDOWN_COUNT_SHIFT) == RundownDescriptor->References)
                {
                    NewCount = 0;
                }
                else
                {
                    NewCount = (((Current >> EX_RUNDOWN_COUNT_SHIFT) - RundownDescriptor->References) << EX_RUNDOWN_COUNT_SHIFT) | EX_RUNDOWN_ACTIVE;
                }
#ifdef _WIN64
                PrevCount = (ULONG_PTR)InterlockedCompareExchange64((LONGLONG*)&RunRef->Count, (LONGLONG)NewCount, (LONGLONG)Current);
#else
                PrevCount = (ULONG_PTR)InterlockedCompareExchange((LONG*)&RunRef->Count, (LONG)NewCount, (LONG)Current);
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
    /* mark the  */
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
    
#ifdef _WIN64
    PrevCount = (ULONG_PTR)InterlockedCompareExchange64((LONGLONG*)&RunRef->Ptr, (LONGLONG)EX_RUNDOWN_ACTIVE, 0LL);
#else
    PrevCount = (ULONG_PTR)InterlockedCompareExchange((LONG*)&RunRef->Ptr, EX_RUNDOWN_ACTIVE, 0);
#endif

    if (PrevCount == 0 ||
        PrevCount & EX_RUNDOWN_ACTIVE)
    {
        return;
    }
    
    /* save the reference counter */
    RundownDescriptor.References = PrevCount >> EX_RUNDOWN_COUNT_SHIFT;
    
    /* Pending references... wait on them to be closed with an event */
    KeInitializeEvent(&RundownDescriptor.RundownEvent, NotificationEvent, FALSE);
    
    NewPtr = (ULONG_PTR)&RundownDescriptor | EX_RUNDOWN_ACTIVE;
    PrevCount = EX_RUNDOWN_ACTIVE;
    
    do
    {
#ifdef _WIN64
        PrevPtr = (ULONG_PTR)InterlockedCompareExchange64((LONGLONG*)&RunRef->Ptr, (LONGLONG)NewPtr, (LONGLONG)PrevCount);
#else
        PrevPtr = (ULONG_PTR)InterlockedCompareExchange((LONG*)&RunRef->Ptr, (LONG)NewPtr, (LONG)PrevCount);
#endif

        PrevCount = PrevPtr;
    } while (PrevPtr != PrevCount);
    
    /* Wait for whoever needs to release to notify us */
    KeWaitForSingleObject(&RundownDescriptor.RundownEvent, Executive, KernelMode, FALSE, NULL);
}

/* EOF */
