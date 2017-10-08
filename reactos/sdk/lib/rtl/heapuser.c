/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/heapuser.c
 * PURPOSE:         RTL Heap backend allocator (user mode only functions)
 * PROGRAMMERS:     Copyright 2010 Aleksey Bragin
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include <heap.h>

#define NDEBUG
#include <debug.h>

RTL_CRITICAL_SECTION RtlpProcessHeapsListLock;


/* Usermode only! */
VOID
NTAPI
RtlpAddHeapToProcessList(PHEAP Heap)
{
    PPEB Peb;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Acquire the lock */
    RtlEnterCriticalSection(&RtlpProcessHeapsListLock);

    //_SEH2_TRY {
    /* Check if max number of heaps reached */
    if (Peb->NumberOfHeaps == Peb->MaximumNumberOfHeaps)
    {
        // TODO: Handle this case
        ASSERT(FALSE);
    }

    /* Add the heap to the process heaps */
    Peb->ProcessHeaps[Peb->NumberOfHeaps] = Heap;
    Peb->NumberOfHeaps++;
    Heap->ProcessHeapsListIndex = (USHORT)Peb->NumberOfHeaps;
    // } _SEH2_FINALLY {

    /* Release the lock */
    RtlLeaveCriticalSection(&RtlpProcessHeapsListLock);

    // } _SEH2_END
}

/* Usermode only! */
VOID
NTAPI
RtlpRemoveHeapFromProcessList(PHEAP Heap)
{
    PPEB Peb;
    PHEAP *Current, *Next;
    ULONG Count;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Acquire the lock */
    RtlEnterCriticalSection(&RtlpProcessHeapsListLock);

    /* Check if we don't need anything to do */
    if ((Heap->ProcessHeapsListIndex == 0) ||
        (Heap->ProcessHeapsListIndex > Peb->NumberOfHeaps) ||
        (Peb->NumberOfHeaps == 0))
    {
        /* Release the lock */
        RtlLeaveCriticalSection(&RtlpProcessHeapsListLock);

        return;
    }

    /* The process actually has more than one heap.
       Use classic, lernt from university times algorithm for removing an entry
       from a static array */

    Current = (PHEAP *)&Peb->ProcessHeaps[Heap->ProcessHeapsListIndex - 1];
    Next = Current + 1;

    /* How many items we need to shift to the left */
    Count = Peb->NumberOfHeaps - (Heap->ProcessHeapsListIndex - 1);

    /* Move them all in a loop */
    while (--Count)
    {
        /* Copy it and advance next pointer */
        *Current = *Next;

        /* Update its index */
        (*Current)->ProcessHeapsListIndex -= 1;

        /* Advance pointers */
        Current++;
        Next++;
    }

    /* Decrease total number of heaps */
    Peb->NumberOfHeaps--;

    /* Zero last unused item */
    Peb->ProcessHeaps[Peb->NumberOfHeaps] = NULL;
    Heap->ProcessHeapsListIndex = 0;

    /* Release the lock */
    RtlLeaveCriticalSection(&RtlpProcessHeapsListLock);
}

VOID
NTAPI
RtlInitializeHeapManager(VOID)
{
    PPEB Peb;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Initialize heap-related fields of PEB */
    Peb->NumberOfHeaps = 0;

    /* Initialize the process heaps list protecting lock */
    RtlInitializeCriticalSection(&RtlpProcessHeapsListLock);
}

