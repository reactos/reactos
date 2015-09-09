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
            ULONG_PTR BufferPointer : ((8 * sizeof(ULONG_PTR)) - BL_HEAP_POINTER_FLAG_BITS);
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
    ULONG_PTR HeapEnd;
    ULONG_PTR HeapLimit;
    ULONG_PTR HeapBase;
    PBL_BUSY_HEAP_ENTRY HeapStart;
} BL_HEAP_BOUNDARIES, *PBL_HEAP_BOUNDARIES;

ULONG HapInitializationStatus;
LIST_ENTRY MmHeapBoundaries;
ULONG HapMinimumHeapSize;
ULONG HapAllocationAttributes;
PBL_FREE_HEAP_ENTRY* MmFreeList;

/* INLINES *******************************************************************/

FORCEINLINE
PBL_FREE_HEAP_ENTRY
MmHapDecodeLink (
    _In_ BL_HEAP_POINTER Link
    )
{
    /* Decode the buffer pointer by ignoring the flags */
    return (PBL_FREE_HEAP_ENTRY)(Link.BufferPointer << BL_HEAP_POINTER_FLAG_BITS);
}

FORCEINLINE
ULONG
MmHapBufferSize (
    _In_ PVOID FreeEntry
    )
{
    PBL_FREE_HEAP_ENTRY Entry = FreeEntry;

    /* The space between the next buffer header and this one is the size */
    return (ULONG_PTR)MmHapDecodeLink(Entry->BufferNext) - (ULONG_PTR)Entry;
}

FORCEINLINE
ULONG
MmHapUserBufferSize (
    _In_ PVOID FreeEntry
    )
{
    PBL_FREE_HEAP_ENTRY Entry = FreeEntry;

    /* Get the size of the buffer as the user sees it */
    return MmHapBufferSize(Entry) - FIELD_OFFSET(BL_BUSY_HEAP_ENTRY, Buffer);
}


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
    if (HeapSize < ExtendSize)
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
        if (HeapLimit <= Heap->HeapEnd)
        {
            EfiPrintf(L"Heap extension TODO\r\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* We do not -- allocate one */
    Status = MmPapAllocatePagesInRange((PVOID*)&HeapBase,
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
    NewHeap->HeapBase = (ULONG_PTR)HeapBase;
    NewHeap->HeapLimit = (ULONG_PTR)HeapBase + AlignedSize;
    NewHeap->HeapStart = (PBL_BUSY_HEAP_ENTRY)(NewHeap + 1);

    /* Set the buffer links */
    HeapBase->BufferPrevious.P = NULL;
    HeapBase->BufferNext.P = NewHeap->HeapStart;

    /* Set the buffer at the top of the heap and mark it as being free */
    NewHeap->HeapStart->BufferPrevious.P = HeapBase;
    NewHeap->HeapStart->BufferNext.P =  NewHeap->HeapStart;
    NewHeap->HeapStart->BufferNext.BufferFree = 1;
    NewHeap->HeapStart->BufferNext.BufferOnHeap = 1;

    /* Is this the first heap ever? */
    if (IsListEmpty(&MmHeapBoundaries))
    {
        /* We will host the free list at the top of the heap */
        MmFreeList = (PBL_FREE_HEAP_ENTRY*)((ULONG_PTR)NewHeap->HeapLimit - 8 * sizeof(PBL_FREE_HEAP_ENTRY));
        NewHeap->HeapLimit = (ULONG_PTR)MmFreeList;
        RtlZeroMemory(MmFreeList, 8 * sizeof(PBL_FREE_HEAP_ENTRY));
    }

    /* Remove a page on top */
    HeapLimit = NewHeap->HeapLimit;
    NewHeap->HeapEnd = NewHeap->HeapLimit;
    NewHeap->HeapLimit -= PAGE_SIZE;

    /* Add us into the heap list */
    InsertTailList(&MmHeapBoundaries, &NewHeap->ListEntry);
    return STATUS_SUCCESS;
}

ULONG
MmHapGetBucketId (
    _In_ ULONG Size
    )
{
    ULONG BucketIndex = 0;

    /* Use the last bucket if this is a large allocation */
    if (Size >= PAGE_SIZE)
    {
        return 7;
    }

    /* Otherwise, use a higher index for each new power of two */
    while (Size >> BucketIndex)
    {
        BucketIndex++;
    }

    /* Allocations are at least 16 bytes (2^4 = 5th index) */
    return BucketIndex - 5;
}

VOID
MmHapReportHeapCorruption (
    _In_ PBL_FREE_HEAP_ENTRY BufferEntry
    )
{
#if 0
    BOOLEAN DebuggerEnabled;

    BlStatusPrint(L"Heap corruption in the links surrounding %p!\r\n", BufferEntry);

    DebuggerEnabled = BlBdDebuggerEnabled();
    if (DebuggerEnabled)
    {
        BlStatusPrint(L"\n*** Fatal Error 0x%08x :\n                (0x%p, 0x%p, 0x%p, 0x%p)\n\r\n", 2, BufferEntry, NULL, NULL, NULL);
        __debugbreak();
    }
#else
    EfiPrintf(L"Heap corruption in the links surrounding %p!\r\n", BufferEntry);
#endif
}

PVOID
MmHapCheckFreeLinks (
    _In_ PVOID BufferEntry
    )
{
    PBL_FREE_HEAP_ENTRY Prev, Next;
    PBL_FREE_HEAP_ENTRY Entry = BufferEntry;

    /* Get the previous and next free pointers */
    Prev = MmHapDecodeLink(Entry->FreePrevious);
    Next = MmHapDecodeLink(Entry->FreeNext);

    /* Make sure that both the previous and next entries point to this one */
    if (((Next) && (MmHapDecodeLink(Next->FreePrevious)) != Entry) ||
        ((Prev) && (MmHapDecodeLink(Prev->FreeNext)) != Entry))
    {
        /* They don't, so the free headers are corrupted */
        MmHapReportHeapCorruption(Entry);
        return NULL;
    }

    /* They do, return the free entry as valid */
    return Entry;
}

PVOID
MmHapCheckBufferLinks (
    _In_ PVOID BufferEntry
    )
{
    PBL_FREE_HEAP_ENTRY Prev, Next;
    PBL_FREE_HEAP_ENTRY Entry = BufferEntry;

    /* Get the previous and next buffer pointers */
    Prev = MmHapDecodeLink(Entry->BufferPrevious);
    Next = MmHapDecodeLink(Entry->BufferNext);

    /* Make sure that both the previous and next entries point to this one */
    if (((Next) && (MmHapDecodeLink(Next->BufferPrevious)) != Entry) ||
        ((Prev) && (MmHapDecodeLink(Prev->BufferNext)) != Entry))
    {
        /* They don't, so the heap headers are corrupted */
        MmHapReportHeapCorruption(Entry);
        return NULL;
    }

    /* They, do the entry is valid */
    return Entry;
}

PBL_FREE_HEAP_ENTRY
MmHapRemoveBufferFromFreeList (
    _In_ PBL_FREE_HEAP_ENTRY FreeEntry
    )
{
    PBL_FREE_HEAP_ENTRY Prev, Next;

    /* Firest, make sure the free entry is valid */
    FreeEntry = MmHapCheckFreeLinks(FreeEntry);
    if (!FreeEntry)
    {
        return FreeEntry;
    }

    /* Get the previous and next entry */
    Prev = MmHapDecodeLink(FreeEntry->FreePrevious);
    Next = MmHapDecodeLink(FreeEntry->FreeNext);

    /* Update the next entry to point to our previous entry */
    if (Next)
    {
        Next->FreePrevious.P = Prev;
    }

    /* Are we at the head? */
    if (Prev)
    {
        /* Nope, so update our previous entry to point to our next entry */
        Prev->FreeNext.P = Next;
    }
    else
    {
        /* Yep, so update the appropriate bucket listhead */
        MmFreeList[MmHapGetBucketId(MmHapBufferSize(FreeEntry))] = Prev;
    }

    /* Return the (now removed) entry */
    return FreeEntry;
}

PBL_FREE_HEAP_ENTRY
MmHapCoalesceFreeBuffer (
    _In_ PBL_FREE_HEAP_ENTRY FreeEntry
    )
{
    PBL_FREE_HEAP_ENTRY Prev, Next;

    /* First make sure that this is a valid buffer entry */
    if (!MmHapCheckBufferLinks(FreeEntry))
    {
        return NULL;
    }

    /* Get the next entry and check if it's free */
    Next = MmHapDecodeLink(FreeEntry->BufferNext);
    if (!(Next->BufferNext.BufferOnHeap) && (Next->BufferNext.BufferFree))
    {
        /* Remove the next buffer from the free list since we're coalescing */
        Next = MmHapRemoveBufferFromFreeList(Next);
        if (!Next)
        {
            return NULL;
        }

        /* The forward link of the *new* free buffer should now point to us */
        MmHapDecodeLink(Next->BufferNext)->BufferPrevious.P = FreeEntry;

        /* Our forward link should point to the *new* free buffer as well */
        FreeEntry->BufferNext.P = MmHapDecodeLink(Next->BufferNext);

        /* Mark our buffer as free */
        FreeEntry->BufferNext.BufferFree = 1;
    }

    /* Get the previous entry and check if it's free */
    Prev = MmHapDecodeLink(FreeEntry->BufferPrevious);
    if (!(Prev) || !(Prev->BufferNext.BufferFree))
    {
        return FreeEntry;
    }

    /* It's free, so remove it */
    Prev = MmHapRemoveBufferFromFreeList(Prev);
    if (!Prev)
    {
        return NULL;
    }

    /* The previous link of our next buffer should now point to our *previous* */
    MmHapDecodeLink(FreeEntry->BufferNext)->BufferPrevious.P = Prev;

    /* Our previous link should point the next free buffer now */
    Prev->BufferNext.P = MmHapDecodeLink(FreeEntry->BufferNext);

    /* Set the new freed buffer as the previous buffer, and mark it free */
    FreeEntry = Prev;
    FreeEntry->BufferNext.BufferFree = 1;
    return FreeEntry;
}

PBL_FREE_HEAP_ENTRY
MmHapAddToFreeList (
    _In_ PBL_BUSY_HEAP_ENTRY Entry,
    _In_ ULONG Flags
    )
{
    PBL_FREE_HEAP_ENTRY FreeEntry, Head;
    ULONG BucketId;
    BL_LIBRARY_PARAMETERS LocalParameters;

    /* First, check if the entry is valid */
    Entry = MmHapCheckBufferLinks(Entry);
    if (!Entry)
    {
        return NULL;
    }

    /* Check if we should zero the entry */
    LocalParameters = BlpLibraryParameters;
    if ((LocalParameters.LibraryFlags & BL_LIBRARY_FLAG_ZERO_HEAP_ALLOCATIONS_ON_FREE) &&
        !(Flags))
    {
        /* Yep, zero it out */
        RtlZeroMemory(Entry->Buffer, MmHapUserBufferSize(Entry));
    }

    /* Now mark the entry as free */
    Entry->BufferNext.BufferFree = 1;

    /* Now that this buffer is free, try to coalesce it */
    FreeEntry = MmHapCoalesceFreeBuffer((PBL_FREE_HEAP_ENTRY)Entry);
    if (!FreeEntry)
    {
        return FreeEntry;
    }

    /* Compute the bucket ID for the free list */
    BucketId = MmHapGetBucketId(MmHapBufferSize(Entry));

    /* Get the current head for this bucket, if one exists */
    Head = MmFreeList ? MmFreeList[BucketId] : NULL;

    /* Update the head's backlink to point to this newly freed entry */
    if (Head)
    {
        Head->FreePrevious.P = FreeEntry;
    }

    /* Nobody behind us, the old head in front of us */
    FreeEntry->FreePrevious.P = NULL;
    FreeEntry->FreeNext.P = Head;

    /* Put us at the head of list now, and return the entry */
    MmFreeList[BucketId] = FreeEntry;
    return FreeEntry;
}

PBL_BUSY_HEAP_ENTRY
MmHapFindBufferInFreeList (
    _In_ ULONG Size
    )
{
    PBL_FREE_HEAP_ENTRY FreeEntry = NULL;
    PBL_BUSY_HEAP_ENTRY NextEntry;
    ULONG BucketId;

    /* Get the appropriate bucket for our size */
    BucketId = MmHapGetBucketId(Size);
    if (BucketId >= 8)
    {
        return NULL;
    }

    /* Keep going as long as we don't have a free entry */
    while (!FreeEntry)
    {
        /* Fet the first free entry in this list */
        FreeEntry = MmFreeList ? MmFreeList[BucketId] : NULL;

        /* Loop as long as there's entries in the list */
        while (FreeEntry)
        {
            /* Can this free entry satisfy our needs? */
            if (MmHapBufferSize(FreeEntry) >= Size)
            {
                /* All good */
                break;
            }

            /* It cannot, keep going to the next one */
            FreeEntry = MmHapDecodeLink(FreeEntry->FreeNext);
        }

        /* Try the next list -- have we exhausted all the lists? */
        if (++BucketId >= 8)
        {
            /* Have we not found an entry yet? Fail if so... */
            if (!FreeEntry)
            {
                return NULL;
            }
        }
    }

    /* We should have an entry if we're here. Remove it from the free list */
    NT_ASSERT(FreeEntry != NULL);
    FreeEntry = MmHapRemoveBufferFromFreeList(FreeEntry);
    if (!FreeEntry)
    {
        return NULL;
    }

    /* Make sure it's not corrupted */
    FreeEntry = MmHapCheckBufferLinks(FreeEntry);
    if (!FreeEntry)
    {
        return NULL;
    }

    /* Do we have space for at least another buffer? */
    if ((MmHapBufferSize(FreeEntry) - Size) >= sizeof(BL_FREE_HEAP_ENTRY))
    {
        /* Go to where the new next buffer will  start */
        NextEntry = (PBL_BUSY_HEAP_ENTRY)((ULONG_PTR)FreeEntry + Size);

        /* Make the new next buffer point to the next buffer */
        NextEntry->BufferNext.P = MmHapDecodeLink(FreeEntry->BufferNext);

        /* Make the old next buffer point back to the new one */
        MmHapDecodeLink(FreeEntry->BufferNext)->BufferPrevious.P = NextEntry;

        /* Point the new next buffer point back to us */
        NextEntry->BufferPrevious.P = FreeEntry;

        /* Point us to the new next buffer */
        FreeEntry->BufferNext.P = NextEntry;

        /* And insert the new next buffer into the free list */
        MmHapAddToFreeList(NextEntry, 1);
    }

    /* Return the entry, which is now allocated */
    return (PBL_BUSY_HEAP_ENTRY)FreeEntry;
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

PVOID
BlMmAllocateHeap (
    _In_ ULONG Size
    )
{
    ULONG BufferSize;
    PBL_HEAP_BOUNDARIES Heap;
    PBL_BUSY_HEAP_ENTRY BusyEntry, FreeEntry, NextEntry;

    /* Ignore heap allocation if the heap allocator isn't ready yet */
    if (HapInitializationStatus != 1)
    {
        return NULL;
    }

    /* Align the buffer size to the minimum size required */
    BufferSize = ALIGN_UP(Size + FIELD_OFFSET(BL_BUSY_HEAP_ENTRY, Buffer),
                          FIELD_OFFSET(BL_BUSY_HEAP_ENTRY, Buffer));

    /* Watch out for overflow */
    if (BufferSize <= Size)
    {
        return NULL;
    }

    /* Make sure it's at least big enough to hold a free entry later on */
    if (BufferSize < sizeof(BL_FREE_HEAP_ENTRY))
    {
        BufferSize = sizeof(BL_FREE_HEAP_ENTRY);
    }

    /* Loop while we try to allocate memory */
    while (1)
    {
        /* Find a free buffer for this allocation */
        BusyEntry = MmHapFindBufferInFreeList(BufferSize);
        if (BusyEntry)
        {
            break;
        }

        /* We couldn't find a free buffer. Do we have any heaps? */
        if (!IsListEmpty(&MmHeapBoundaries))
        {
            /* Get the current heap */
            Heap = CONTAINING_RECORD(MmHeapBoundaries.Flink,
                                     BL_HEAP_BOUNDARIES,
                                     ListEntry);

            /* Check if we have space in the heap page for this allocation? */
            FreeEntry = Heap->HeapStart;
            NextEntry = (PBL_BUSY_HEAP_ENTRY)((ULONG_PTR)FreeEntry + BufferSize);

            if ((NextEntry >= FreeEntry) &&
                ((ULONG_PTR)NextEntry <=
                 Heap->HeapLimit - FIELD_OFFSET(BL_BUSY_HEAP_ENTRY, Buffer)))
            {
                /* Update the heap top pointer past this allocation */
                Heap->HeapStart = NextEntry;

                /* Make this allocation point to the slot */
                FreeEntry->BufferNext.P = Heap->HeapStart;

                /* And make the free heap entry point back to us */
                Heap->HeapStart->BufferPrevious.P = FreeEntry;

                /* Mark the heap entry as being free and on the heap */
                Heap->HeapStart->BufferNext.BufferFree = 1;
                Heap->HeapStart->BufferNext.BufferOnHeap = 1;

                /* The previously freed entry on the heap page is now ours */
                BusyEntry = FreeEntry;
                break;
            }
        }

        /* We have no heaps or space on any heap -- extend the heap and retry */
        if (!NT_SUCCESS(MmHapHeapAllocatorExtend(BufferSize)))
        {
            EfiPrintf(L"Heap extension failed!\r\n");
            return NULL;
        }

        EfiPrintf(L"Heap extended -- trying again\r\n");
    }

    /* Clear all the bits, marking this entry as allocated */
    BusyEntry->BufferNext.P = MmHapDecodeLink(BusyEntry->BufferNext);

    /* Return the entry's data buffer */
    EfiPrintf(L"Returning buffer at 0x%p\r\n", &BusyEntry->Buffer);
    return &BusyEntry->Buffer;
}

NTSTATUS
BlMmFreeHeap (
    _In_ PVOID Buffer
    )
{
    PBL_BUSY_HEAP_ENTRY BusyEntry;
    PBL_HEAP_BOUNDARIES Heap;
    PLIST_ENTRY NextEntry;

    /* If the heap is not initialized, fail */
    if (HapInitializationStatus != 1)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the heap header */
    EfiPrintf(L"Freeing entry at: %p\r\n", Buffer);
    if (Buffer)
    {
        /* Don't free heap until we discover the corruption */
        return STATUS_SUCCESS;
    }

    BusyEntry = CONTAINING_RECORD(Buffer, BL_BUSY_HEAP_ENTRY, Buffer);

    /* Loop all the heaps */
    NextEntry = MmHeapBoundaries.Flink;
    while (NextEntry != &MmHeapBoundaries)
    {
        /* Get the current heap in the list */
        Heap = CONTAINING_RECORD(NextEntry, BL_HEAP_BOUNDARIES, ListEntry);

        /* Is this entry part of this heap? */
        if (((ULONG_PTR)Heap->HeapBase <= (ULONG_PTR)BusyEntry) &&
            ((ULONG_PTR)BusyEntry < (ULONG_PTR)Heap->HeapStart))
        {
            /* Ignore double-free */
            if (BusyEntry->BufferNext.BufferFree)
            {
                return STATUS_INVALID_PARAMETER;
            }

            /* It is -- add it to the free list */
            MmHapAddToFreeList(BusyEntry, 0);
            return STATUS_SUCCESS;
        }

        /* It isn't, move to the next heap */
        NextEntry = NextEntry->Flink;
    }

    /* The entry is not on any valid heap */
    return STATUS_INVALID_PARAMETER;
}

