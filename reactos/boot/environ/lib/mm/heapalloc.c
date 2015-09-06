/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/heapalloc.c
 * PURPOSE:         Boot Library Memory Manager Heap Allocator
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

#define BL_HEAP_POINTER_FLAG_BITS   3

typedef struct _BL_HEAP_POINTER
{
    union
    {
        struct
        {
            ULONG_PTR BufferFree : 1;
            ULONG_PTR BufferOnHeap : 1;
            ULONG_PTR NotUsed : 1;
            ULONG_PTR BufferPointer : (sizeof(ULONG_PTR) - BL_HEAP_POINTER_FLAG_BITS);
        };
        PVOID P;
    };
} BL_HEAP_POINTER, *PBL_HEAP_POINTER;

typedef struct _BL_FREE_HEAP_ENTRY
{
    BL_HEAP_POINTER BufferNext;
    BL_HEAP_POINTER BufferPrevious;
    BL_HEAP_POINTER FreeNext;
    BL_HEAP_POINTER FreePrevious;
} BL_FREE_HEAP_ENTRY, *PBL_FREE_HEAP_ENTRY;

typedef struct _BL_BUSY_HEAP_ENTRY
{
    BL_HEAP_POINTER BufferNext;
    BL_HEAP_POINTER BufferPrevious;
    UCHAR Buffer[ANYSIZE_ARRAY];
} BL_BUSY_HEAP_ENTRY, *PBL_BUSY_HEAP_ENTRY;

typedef struct _BL_HEAP_BOUNDARIES
{
    LIST_ENTRY ListEntry;
    ULONG_PTR HeapHigh;
    ULONG_PTR HeapLimit;
    ULONG_PTR HeapBottom;
    PBL_BUSY_HEAP_ENTRY HeapTop;
} BL_HEAP_BOUNDARIES, *PBL_HEAP_BOUNDARIES;

ULONG HapInitializationStatus;
LIST_ENTRY MmHeapBoundaries;
ULONG HapMinimumHeapSize;
ULONG HapAllocationAttributes;
PBL_FREE_HEAP_ENTRY* MmFreeList;

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmHapHeapAllocatorExtend (
    _In_ ULONG ExtendSize
    )
{
    ULONG HeapSize, AlignedSize, HeapLimit;
    PBL_HEAP_BOUNDARIES Heap, NewHeap;
    NTSTATUS Status;
    PBL_BUSY_HEAP_ENTRY HeapBase = NULL;

    /* Compute a new heap, and add 2 more pages for the free list */
    HeapSize = ExtendSize + (2 * PAGE_SIZE);
    if ((ExtendSize + (2 * PAGE_SIZE)) < ExtendSize)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    /* Make sure the new heap is at least the minimum configured size */
    if (HapMinimumHeapSize > HeapSize)
    {
        HeapSize = HapMinimumHeapSize;
    }

    /* Align it on a page boundary */
    AlignedSize = ALIGN_UP_BY(HeapSize, PAGE_SIZE);
    if (!AlignedSize)
    {
        return STATUS_INTEGER_OVERFLOW;
    }

    /* Check if we already have a heap */
    if (!IsListEmpty(&MmHeapBoundaries))
    {
        /* Find the first heap*/
        Heap = CONTAINING_RECORD(MmHeapBoundaries.Flink,
                                 BL_HEAP_BOUNDARIES,
                                 ListEntry);

        /* Check if we have a page free above the heap */
        HeapLimit = Heap->HeapLimit + PAGE_SIZE;
        if (HeapLimit <= Heap->HeapHigh)
        {
            EarlyPrint(L"TODO\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* We do not -- allocate one */
    Status = MmPapAllocatePagesInRange((PULONG)&HeapBase,
                                       BlLoaderHeap,
                                       AlignedSize >> PAGE_SHIFT,
                                       HapAllocationAttributes,
                                       0,
                                       NULL,
                                       0);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Set the heap bottom, limit, and top */
    NewHeap = (PBL_HEAP_BOUNDARIES)HeapBase->Buffer;
    NewHeap->HeapBottom = (ULONG_PTR)HeapBase;
    NewHeap->HeapLimit = (ULONG_PTR)HeapBase + AlignedSize;
    NewHeap->HeapTop = (PBL_BUSY_HEAP_ENTRY)(NewHeap + 1);

    /* Set the buffer links */
    HeapBase->BufferPrevious.P = NULL;
    HeapBase->BufferNext.P = NewHeap->HeapTop;

    /* Set the buffer at the top of the heap and mark it as being free */
    NewHeap->HeapTop->BufferPrevious.P = HeapBase;
    NewHeap->HeapTop->BufferNext.P =  NewHeap->HeapTop;
    NewHeap->HeapTop->BufferNext.BufferFree = 1;
    NewHeap->HeapTop->BufferNext.BufferOnHeap = 1;

    /* Is this the first heap ever? */
    if (IsListEmpty(&MmHeapBoundaries))
    {
        /* We will host the free list at the top of the heap */
        MmFreeList = (PBL_FREE_HEAP_ENTRY*)((ULONG_PTR)NewHeap->HeapLimit - sizeof(BL_HEAP_BOUNDARIES));
        NewHeap->HeapLimit = (ULONG_PTR)MmFreeList;
        RtlZeroMemory(MmFreeList, 8 * sizeof(PBL_FREE_HEAP_ENTRY));
    }

    /* Remove a page on top */
    HeapLimit = NewHeap->HeapLimit;
    NewHeap->HeapHigh = NewHeap->HeapLimit;
    NewHeap->HeapLimit -= PAGE_SIZE;

    /* Add us into the heap list */
    InsertTailList(&MmHeapBoundaries, &NewHeap->ListEntry);
    return STATUS_SUCCESS;
}

NTSTATUS
MmHaInitialize (
    _In_ ULONG HeapSize,
    _In_ ULONG HeapAttributes
    )
{
    NTSTATUS Status;

    /* No free list to begin with */
    MmFreeList = NULL;

    /* Configure the minimum heap size and allocation attributes */
    HapMinimumHeapSize = ALIGN_UP_BY(HeapSize, PAGE_SIZE);
    HapAllocationAttributes = HeapAttributes & 0x20000;

    /* Initialize the heap boundary list */
    InitializeListHead(&MmHeapBoundaries);

    /* Initialize a heap big enough to handle a one pointer long allocation */
    Status = MmHapHeapAllocatorExtend(sizeof(PVOID));
    if (NT_SUCCESS(Status))
    {
        /* The heap is ready! */
        HapInitializationStatus = 1;
        Status = STATUS_SUCCESS;
    }

    /* Return initialization status */
    return Status;
}
