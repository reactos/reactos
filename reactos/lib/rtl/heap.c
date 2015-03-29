/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/heap.c
 * PURPOSE:         RTL Heap backend allocator
 * PROGRAMMERS:     Copyright 2010 Aleksey Bragin
 */

/* Useful references:
   http://msdn.microsoft.com/en-us/library/ms810466.aspx
   http://msdn.microsoft.com/en-us/library/ms810603.aspx
   http://www.securitylab.ru/analytics/216376.php
   http://binglongx.spaces.live.com/blog/cns!142CBF6D49079DE8!596.entry
   http://www.phreedom.org/research/exploits/asn1-bitstring/
   http://illmatics.com/Understanding_the_LFH.pdf
   http://www.alex-ionescu.com/?p=18
*/

/* INCLUDES *****************************************************************/

#include <rtl.h>
#include <heap.h>

#define NDEBUG
#include <debug.h>

/* Bitmaps stuff */

/* How many least significant bits are clear */
UCHAR RtlpBitsClearLow[] =
{
    8,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    7,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    6,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    5,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0,
    4,0,1,0,2,0,1,0,3,0,1,0,2,0,1,0
};

FORCEINLINE
UCHAR
RtlpFindLeastSetBit(ULONG Bits)
{
    if (Bits & 0xFFFF)
    {
        if (Bits & 0xFF)
            return RtlpBitsClearLow[Bits & 0xFF]; /* Lowest byte */
        else
            return RtlpBitsClearLow[(Bits >> 8) & 0xFF] + 8; /* 2nd byte */
    }
    else
    {
        if ((Bits >> 16) & 0xFF)
            return RtlpBitsClearLow[(Bits >> 16) & 0xFF] + 16; /* 3rd byte */
        else
            return RtlpBitsClearLow[(Bits >> 24) & 0xFF] + 24; /* Highest byte */
    }
}

/* Maximum size of a tail-filling pattern used for compare operation */
UCHAR FillPattern[HEAP_ENTRY_SIZE] =
{
    HEAP_TAIL_FILL,
    HEAP_TAIL_FILL,
    HEAP_TAIL_FILL,
    HEAP_TAIL_FILL,
    HEAP_TAIL_FILL,
    HEAP_TAIL_FILL,
    HEAP_TAIL_FILL,
    HEAP_TAIL_FILL
};

/* FUNCTIONS *****************************************************************/

NTSTATUS NTAPI
RtlpInitializeHeap(OUT PHEAP Heap,
                   IN ULONG Flags,
                   IN PHEAP_LOCK Lock OPTIONAL,
                   IN PRTL_HEAP_PARAMETERS Parameters)
{
    ULONG NumUCRs = 8;
    ULONG Index;
    SIZE_T HeaderSize;
    NTSTATUS Status;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;

    /* Preconditions */
    ASSERT(Heap != NULL);
    ASSERT(Parameters != NULL);
    ASSERT(!(Flags & HEAP_LOCK_USER_ALLOCATED));
    ASSERT(!(Flags & HEAP_NO_SERIALIZE) || (Lock == NULL));  /* HEAP_NO_SERIALIZE => no lock */

    /* Start out with the size of a plain Heap header */
    HeaderSize = ROUND_UP(sizeof(HEAP), sizeof(HEAP_ENTRY));

    /* Check if space needs to be added for the Heap Lock */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        if (Lock != NULL)
            /* The user manages the Heap Lock */
            Flags |= HEAP_LOCK_USER_ALLOCATED;
        else
        if (RtlpGetMode() == UserMode)
        {
            /* In user mode, the Heap Lock trails the Heap header */
            Lock = (PHEAP_LOCK) ((ULONG_PTR) (Heap) + HeaderSize);
            HeaderSize += ROUND_UP(sizeof(HEAP_LOCK), sizeof(HEAP_ENTRY));
        }
    }

    /* Add space for the initial Heap UnCommitted Range Descriptor list */
    UcrDescriptor = (PHEAP_UCR_DESCRIPTOR) ((ULONG_PTR) (Heap) + HeaderSize);
    HeaderSize += ROUND_UP(NumUCRs * sizeof(HEAP_UCR_DESCRIPTOR), sizeof(HEAP_ENTRY));

    /* Sanity check */
    ASSERT(HeaderSize <= PAGE_SIZE);

    /* Initialise the Heap Entry header containing the Heap header */
    Heap->Entry.Size = (USHORT)(HeaderSize >> HEAP_ENTRY_SHIFT);
    Heap->Entry.Flags = HEAP_ENTRY_BUSY;
    Heap->Entry.SmallTagIndex = LOBYTE(Heap->Entry.Size) ^ HIBYTE(Heap->Entry.Size) ^ Heap->Entry.Flags;
    Heap->Entry.PreviousSize = 0;
    Heap->Entry.SegmentOffset = 0;
    Heap->Entry.UnusedBytes = 0;

    /* Initialise the Heap header */
    Heap->Signature = HEAP_SIGNATURE;
    Heap->Flags = Flags;
    Heap->ForceFlags = (Flags & (HEAP_NO_SERIALIZE |
                                 HEAP_GENERATE_EXCEPTIONS |
                                 HEAP_ZERO_MEMORY |
                                 HEAP_REALLOC_IN_PLACE_ONLY |
                                 HEAP_VALIDATE_PARAMETERS_ENABLED |
                                 HEAP_VALIDATE_ALL_ENABLED |
                                 HEAP_TAIL_CHECKING_ENABLED |
                                 HEAP_CREATE_ALIGN_16 |
                                 HEAP_FREE_CHECKING_ENABLED));

    /* Initialise the Heap parameters */
    Heap->VirtualMemoryThreshold = ROUND_UP(Parameters->VirtualMemoryThreshold, sizeof(HEAP_ENTRY)) >> HEAP_ENTRY_SHIFT;
    Heap->SegmentReserve = Parameters->SegmentReserve;
    Heap->SegmentCommit = Parameters->SegmentCommit;
    Heap->DeCommitFreeBlockThreshold = Parameters->DeCommitFreeBlockThreshold >> HEAP_ENTRY_SHIFT;
    Heap->DeCommitTotalFreeThreshold = Parameters->DeCommitTotalFreeThreshold >> HEAP_ENTRY_SHIFT;
    Heap->MaximumAllocationSize = Parameters->MaximumAllocationSize;
    Heap->CommitRoutine = Parameters->CommitRoutine;

    /* Initialise the Heap validation info */
    Heap->HeaderValidateCopy = NULL;
    Heap->HeaderValidateLength = (USHORT)HeaderSize;

    /* Initialise the Heap Lock */
    if (!(Flags & HEAP_NO_SERIALIZE) && !(Flags & HEAP_LOCK_USER_ALLOCATED))
    {
        Status = RtlInitializeHeapLock(&Lock);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    Heap->LockVariable = Lock;

    /* Initialise the Heap alignment info */
    if (Flags & HEAP_CREATE_ALIGN_16)
    {
        Heap->AlignMask = (ULONG) ~15;
        Heap->AlignRound = 15 + sizeof(HEAP_ENTRY);
    }
    else
    {
        Heap->AlignMask = (ULONG) ~(sizeof(HEAP_ENTRY) - 1);
        Heap->AlignRound = 2 * sizeof(HEAP_ENTRY) - 1;
    }

    if (Flags & HEAP_TAIL_CHECKING_ENABLED)
        Heap->AlignRound += sizeof(HEAP_ENTRY);

    /* Initialise the Heap Segment list */
    for (Index = 0; Index < HEAP_SEGMENTS; ++Index)
        Heap->Segments[Index] = NULL;

    /* Initialise the Heap Free Heap Entry lists */
    for (Index = 0; Index < HEAP_FREELISTS; ++Index)
        InitializeListHead(&Heap->FreeLists[Index]);

    /* Initialise the Heap Virtual Allocated Blocks list */
    InitializeListHead(&Heap->VirtualAllocdBlocks);

    /* Initialise the Heap UnCommitted Region lists */
    InitializeListHead(&Heap->UCRSegments);
    InitializeListHead(&Heap->UCRList);

    /* Register the initial Heap UnCommitted Region Descriptors */
    for (Index = 0; Index < NumUCRs; ++Index)
        InsertTailList(&Heap->UCRList, &UcrDescriptor[Index].ListEntry);

    return STATUS_SUCCESS;
}

FORCEINLINE
VOID
RtlpSetFreeListsBit(PHEAP Heap,
                    PHEAP_FREE_ENTRY FreeEntry)
{
    ULONG Index, Bit;

    ASSERT(FreeEntry->Size < HEAP_FREELISTS);

    /* Calculate offset in the free list bitmap */
    Index = FreeEntry->Size >> 3; /* = FreeEntry->Size / (sizeof(UCHAR) * 8)*/
    Bit = 1 << (FreeEntry->Size & 7);

    /* Assure it's not already set */
    ASSERT((Heap->u.FreeListsInUseBytes[Index] & Bit) == 0);

    /* Set it */
    Heap->u.FreeListsInUseBytes[Index] |= Bit;
}

FORCEINLINE
VOID
RtlpClearFreeListsBit(PHEAP Heap,
                      PHEAP_FREE_ENTRY FreeEntry)
{
    ULONG Index, Bit;

    ASSERT(FreeEntry->Size < HEAP_FREELISTS);

    /* Calculate offset in the free list bitmap */
    Index = FreeEntry->Size >> 3; /* = FreeEntry->Size / (sizeof(UCHAR) * 8)*/
    Bit = 1 << (FreeEntry->Size & 7);

    /* Assure it was set and the corresponding free list is empty */
    ASSERT(Heap->u.FreeListsInUseBytes[Index] & Bit);
    ASSERT(IsListEmpty(&Heap->FreeLists[FreeEntry->Size]));

    /* Clear it */
    Heap->u.FreeListsInUseBytes[Index] ^= Bit;
}

VOID NTAPI
RtlpInsertFreeBlockHelper(PHEAP Heap,
                          PHEAP_FREE_ENTRY FreeEntry,
                          SIZE_T BlockSize,
                          BOOLEAN NoFill)
{
    PLIST_ENTRY FreeListHead, Current;
    PHEAP_FREE_ENTRY CurrentEntry;

    ASSERT(FreeEntry->Size == BlockSize);

    /* Fill if it's not denied */
    if (!NoFill)
    {
        FreeEntry->Flags &= ~(HEAP_ENTRY_FILL_PATTERN |
                              HEAP_ENTRY_EXTRA_PRESENT |
                              HEAP_ENTRY_BUSY);

        if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
        {
            RtlFillMemoryUlong((PCHAR)(FreeEntry + 1),
                               (BlockSize << HEAP_ENTRY_SHIFT) - sizeof(*FreeEntry),
                               ARENA_FREE_FILLER);

            FreeEntry->Flags |= HEAP_ENTRY_FILL_PATTERN;
        }
    }
    else
    {
        /* Clear out all flags except the last entry one */
        FreeEntry->Flags &= HEAP_ENTRY_LAST_ENTRY;
    }

    /* Insert it either into dedicated or non-dedicated list */
    if (BlockSize < HEAP_FREELISTS)
    {
        /* Dedicated list */
        FreeListHead = &Heap->FreeLists[BlockSize];

        if (IsListEmpty(FreeListHead))
        {
            RtlpSetFreeListsBit(Heap, FreeEntry);
        }
    }
    else
    {
        /* Non-dedicated one */
        FreeListHead = &Heap->FreeLists[0];
        Current = FreeListHead->Flink;

        /* Find a position where to insert it to (the list must be sorted) */
        while (FreeListHead != Current)
        {
            CurrentEntry = CONTAINING_RECORD(Current, HEAP_FREE_ENTRY, FreeList);

            if (BlockSize <= CurrentEntry->Size)
                break;

            Current = Current->Flink;
        }

        FreeListHead = Current;
    }

    /* Actually insert it into the list */
    InsertTailList(FreeListHead, &FreeEntry->FreeList);
}

VOID NTAPI
RtlpInsertFreeBlock(PHEAP Heap,
                    PHEAP_FREE_ENTRY FreeEntry,
                    SIZE_T BlockSize)
{
    USHORT Size, PreviousSize;
    UCHAR SegmentOffset, Flags;
    PHEAP_SEGMENT Segment;

    DPRINT("RtlpInsertFreeBlock(%p %p %x)\n", Heap, FreeEntry, BlockSize);

    /* Increase the free size counter */
    Heap->TotalFreeSize += BlockSize;

    /* Remember certain values */
    Flags = FreeEntry->Flags;
    PreviousSize = FreeEntry->PreviousSize;
    SegmentOffset = FreeEntry->SegmentOffset;
    Segment = Heap->Segments[SegmentOffset];

    /* Process it */
    while (BlockSize)
    {
        /* Check for the max size */
        if (BlockSize > HEAP_MAX_BLOCK_SIZE)
        {
            Size = HEAP_MAX_BLOCK_SIZE;

            /* Special compensation if it goes above limit just by 1 */
            if (BlockSize == (HEAP_MAX_BLOCK_SIZE + 1))
                Size -= 16;

            FreeEntry->Flags = 0;
        }
        else
        {
            Size = (USHORT)BlockSize;
            FreeEntry->Flags = Flags;
        }

        /* Change its size and insert it into a free list */
        FreeEntry->Size = Size;
        FreeEntry->PreviousSize = PreviousSize;
        FreeEntry->SegmentOffset = SegmentOffset;

        /* Call a helper to actually insert the block */
        RtlpInsertFreeBlockHelper(Heap, FreeEntry, Size, FALSE);

        /* Update sizes */
        PreviousSize = Size;
        BlockSize -= Size;

        /* Go to the next entry */
        FreeEntry = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeEntry + Size);

        /* Check if that's all */
        if ((PHEAP_ENTRY)FreeEntry >= Segment->LastValidEntry) return;
    }

    /* Update previous size if needed */
    if (!(Flags & HEAP_ENTRY_LAST_ENTRY))
        FreeEntry->PreviousSize = PreviousSize;
}

VOID NTAPI
RtlpRemoveFreeBlock(PHEAP Heap,
                    PHEAP_FREE_ENTRY FreeEntry,
                    BOOLEAN Dedicated,
                    BOOLEAN NoFill)
{
    SIZE_T Result, RealSize;

    /* Remove the free block and update the freelists bitmap */
    if (RemoveEntryList(&FreeEntry->FreeList) &&
        (Dedicated || (!Dedicated && FreeEntry->Size < HEAP_FREELISTS)))
    {
        RtlpClearFreeListsBit(Heap, FreeEntry);
    }

    /* Fill with pattern if necessary */
    if (!NoFill &&
        (FreeEntry->Flags & HEAP_ENTRY_FILL_PATTERN))
    {
        RealSize = (FreeEntry->Size << HEAP_ENTRY_SHIFT) - sizeof(*FreeEntry);

        /* Deduct extra stuff from block's real size */
        if (FreeEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT &&
            RealSize > sizeof(HEAP_FREE_ENTRY_EXTRA))
        {
            RealSize -= sizeof(HEAP_FREE_ENTRY_EXTRA);
        }

        /* Check if the free filler is intact */
        Result = RtlCompareMemoryUlong((PCHAR)(FreeEntry + 1),
                                        RealSize,
                                        ARENA_FREE_FILLER);

        if (Result != RealSize)
        {
            DPRINT1("Free heap block %p modified at %p after it was freed\n",
                FreeEntry,
                (PCHAR)(FreeEntry + 1) + Result);
        }
    }
}

SIZE_T NTAPI
RtlpGetSizeOfBigBlock(PHEAP_ENTRY HeapEntry)
{
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualEntry;

    /* Get pointer to the containing record */
    VirtualEntry = CONTAINING_RECORD(HeapEntry, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock);
    ASSERT(VirtualEntry->BusyBlock.Size >= sizeof(HEAP_VIRTUAL_ALLOC_ENTRY));

    /* Restore the real size */
    return VirtualEntry->CommitSize - HeapEntry->Size;
}

PHEAP_UCR_DESCRIPTOR NTAPI
RtlpCreateUnCommittedRange(PHEAP_SEGMENT Segment)
{
    PLIST_ENTRY Entry;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;
    PHEAP_UCR_SEGMENT UcrSegment;
    PHEAP Heap = Segment->Heap;
    SIZE_T ReserveSize = 16 * PAGE_SIZE;
    SIZE_T CommitSize = 1 * PAGE_SIZE;
    NTSTATUS Status;

    DPRINT("RtlpCreateUnCommittedRange(%p)\n", Segment);

    /* Check if we have unused UCRs */
    if (IsListEmpty(&Heap->UCRList))
    {
        /* Get a pointer to the first UCR segment */
        UcrSegment = CONTAINING_RECORD(Heap->UCRSegments.Flink, HEAP_UCR_SEGMENT, ListEntry);

        /* Check the list of UCR segments */
        if (IsListEmpty(&Heap->UCRSegments) ||
            UcrSegment->ReservedSize == UcrSegment->CommittedSize)
        {
            /* We need to create a new one. Reserve 16 pages for it */
            UcrSegment = NULL;
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             (PVOID *)&UcrSegment,
                                             0,
                                             &ReserveSize,
                                             MEM_RESERVE,
                                             PAGE_READWRITE);

            if (!NT_SUCCESS(Status)) return NULL;

            /* Commit one page */
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             (PVOID *)&UcrSegment,
                                             0,
                                             &CommitSize,
                                             MEM_COMMIT,
                                             PAGE_READWRITE);

            if (!NT_SUCCESS(Status))
            {
                /* Release reserved memory */
                ZwFreeVirtualMemory(NtCurrentProcess(),
                                    (PVOID *)&UcrSegment,
                                    &ReserveSize,
                                    MEM_RELEASE);
                return NULL;
            }

            /* Set it's data */
            UcrSegment->ReservedSize = ReserveSize;
            UcrSegment->CommittedSize = CommitSize;

            /* Add it to the head of the list */
            InsertHeadList(&Heap->UCRSegments, &UcrSegment->ListEntry);

            /* Get a pointer to the first available UCR descriptor */
            UcrDescriptor = (PHEAP_UCR_DESCRIPTOR)(UcrSegment + 1);
        }
        else
        {
            /* It's possible to use existing UCR segment. Commit one more page */
            UcrDescriptor = (PHEAP_UCR_DESCRIPTOR)((PCHAR)UcrSegment + UcrSegment->CommittedSize);
            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             (PVOID *)&UcrDescriptor,
                                             0,
                                             &CommitSize,
                                             MEM_COMMIT,
                                             PAGE_READWRITE);

            if (!NT_SUCCESS(Status)) return NULL;

            /* Update sizes */
            UcrSegment->CommittedSize += CommitSize;
        }

        /* There is a whole bunch of new UCR descriptors. Put them into the unused list */
        while ((PCHAR)(UcrDescriptor + 1) <= (PCHAR)UcrSegment + UcrSegment->CommittedSize)
        {
            InsertTailList(&Heap->UCRList, &UcrDescriptor->ListEntry);
            UcrDescriptor++;
        }
    }

    /* There are unused UCRs, just get the first one */
    Entry = RemoveHeadList(&Heap->UCRList);
    UcrDescriptor = CONTAINING_RECORD(Entry, HEAP_UCR_DESCRIPTOR, ListEntry);
    return UcrDescriptor;
}

VOID NTAPI
RtlpDestroyUnCommittedRange(PHEAP_SEGMENT Segment,
                            PHEAP_UCR_DESCRIPTOR UcrDescriptor)
{
    /* Zero it out */
    UcrDescriptor->Address = NULL;
    UcrDescriptor->Size = 0;

    /* Put it into the heap's list of unused UCRs */
    InsertHeadList(&Segment->Heap->UCRList, &UcrDescriptor->ListEntry);
}

VOID NTAPI
RtlpInsertUnCommittedPages(PHEAP_SEGMENT Segment,
                           ULONG_PTR Address,
                           SIZE_T Size)
{
    PLIST_ENTRY Current;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;

    DPRINT("RtlpInsertUnCommittedPages(%p %08Ix %Ix)\n", Segment, Address, Size);

    /* Go through the list of UCR descriptors, they are sorted from lowest address
       to the highest */
    Current = Segment->UCRSegmentList.Flink;
    while (Current != &Segment->UCRSegmentList)
    {
        UcrDescriptor = CONTAINING_RECORD(Current, HEAP_UCR_DESCRIPTOR, SegmentEntry);

        if ((ULONG_PTR)UcrDescriptor->Address > Address)
        {
            /* Check for a really lucky case */
            if ((Address + Size) == (ULONG_PTR)UcrDescriptor->Address)
            {
                /* Exact match */
                UcrDescriptor->Address = (PVOID)Address;
                UcrDescriptor->Size += Size;
                return;
            }

            /* We found the block before which the new one should go */
            break;
        }
        else if (((ULONG_PTR)UcrDescriptor->Address + UcrDescriptor->Size) == Address)
        {
            /* Modify this entry */
            Address = (ULONG_PTR)UcrDescriptor->Address;
            Size += UcrDescriptor->Size;

            /* Advance to the next descriptor */
            Current = Current->Flink;

            /* Remove the current descriptor from the list and destroy it */
            RemoveEntryList(&UcrDescriptor->SegmentEntry);
            RtlpDestroyUnCommittedRange(Segment, UcrDescriptor);

            Segment->NumberOfUnCommittedRanges--;
        }
        else
        {
            /* Advance to the next descriptor */
            Current = Current->Flink;
        }
    }

    /* Create a new UCR descriptor */
    UcrDescriptor = RtlpCreateUnCommittedRange(Segment);
    if (!UcrDescriptor) return;

    UcrDescriptor->Address = (PVOID)Address;
    UcrDescriptor->Size = Size;

    /* "Current" is the descriptor before which our one should go */
    InsertTailList(Current, &UcrDescriptor->SegmentEntry);

    DPRINT("Added segment UCR with base %08Ix, size 0x%x\n", Address, Size);

    /* Increase counters */
    Segment->NumberOfUnCommittedRanges++;
}

PHEAP_FREE_ENTRY NTAPI
RtlpFindAndCommitPages(PHEAP Heap,
                       PHEAP_SEGMENT Segment,
                       PSIZE_T Size,
                       PVOID AddressRequested)
{
    PLIST_ENTRY Current;
    ULONG_PTR Address = 0;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor, PreviousUcr = NULL;
    PHEAP_ENTRY FirstEntry, LastEntry;
    NTSTATUS Status;

    DPRINT("RtlpFindAndCommitPages(%p %p %Ix %08Ix)\n", Heap, Segment, *Size, Address);

    /* Go through UCRs in a segment */
    Current = Segment->UCRSegmentList.Flink;
    while (Current != &Segment->UCRSegmentList)
    {
        UcrDescriptor = CONTAINING_RECORD(Current, HEAP_UCR_DESCRIPTOR, SegmentEntry);

        /* Check if we can use that one right away */
        if (UcrDescriptor->Size >= *Size &&
            (UcrDescriptor->Address == AddressRequested || !AddressRequested))
        {
            /* Get the address */
            Address = (ULONG_PTR)UcrDescriptor->Address;

            /* Commit it */
            if (Heap->CommitRoutine)
            {
                Status = Heap->CommitRoutine(Heap, (PVOID *)&Address, Size);
            }
            else
            {
                Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                                 (PVOID *)&Address,
                                                 0,
                                                 Size,
                                                 MEM_COMMIT,
                                                 PAGE_READWRITE);
            }

            DPRINT("Committed %Iu bytes at base %08Ix, UCR size is %lu\n", *Size, Address, UcrDescriptor->Size);

            /* Fail in unsuccessful case */
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Committing page failed with status 0x%08X\n", Status);
                return NULL;
            }

            /* Update tracking numbers */
            Segment->NumberOfUnCommittedPages -= (ULONG)(*Size / PAGE_SIZE);

            /* Calculate first and last entries */
            FirstEntry = (PHEAP_ENTRY)Address;

            /* Go through the entries to find the last one */
            if (PreviousUcr)
                LastEntry = (PHEAP_ENTRY)((ULONG_PTR)PreviousUcr->Address + PreviousUcr->Size);
            else
                LastEntry = &Segment->Entry;

            while (!(LastEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
            {
                ASSERT(LastEntry->Size != 0);
                LastEntry += LastEntry->Size;
            }
            ASSERT((LastEntry + LastEntry->Size) == FirstEntry);

            /* Unmark it as a last entry */
            LastEntry->Flags &= ~HEAP_ENTRY_LAST_ENTRY;

            /* Update UCR descriptor */
            UcrDescriptor->Address = (PVOID)((ULONG_PTR)UcrDescriptor->Address + *Size);
            UcrDescriptor->Size -= *Size;

            DPRINT("Updating UcrDescriptor %p, new Address %p, size %lu\n",
                UcrDescriptor, UcrDescriptor->Address, UcrDescriptor->Size);

            /* Set various first entry fields */
            FirstEntry->SegmentOffset = LastEntry->SegmentOffset;
            FirstEntry->Size = (USHORT)(*Size >> HEAP_ENTRY_SHIFT);
            FirstEntry->PreviousSize = LastEntry->Size;

            /* Check if anything left in this UCR */
            if (UcrDescriptor->Size == 0)
            {
                /* It's fully exhausted */

                /* Check if this is the end of the segment */
                if(UcrDescriptor->Address == Segment->LastValidEntry)
                {
                    FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
                }
                else
                {
                    FirstEntry->Flags = 0;
                    /* Update field of next entry */
                    ASSERT((FirstEntry + FirstEntry->Size)->PreviousSize == 0);
                    (FirstEntry + FirstEntry->Size)->PreviousSize = FirstEntry->Size;
                }

                /* This UCR needs to be removed because it became useless */
                RemoveEntryList(&UcrDescriptor->SegmentEntry);

                RtlpDestroyUnCommittedRange(Segment, UcrDescriptor);
                Segment->NumberOfUnCommittedRanges--;
            }
            else
            {
                FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
            }

            /* We're done */
            return (PHEAP_FREE_ENTRY)FirstEntry;
        }

        /* Advance to the next descriptor */
        PreviousUcr = UcrDescriptor;
        Current = Current->Flink;
    }

    return NULL;
}

VOID NTAPI
RtlpDeCommitFreeBlock(PHEAP Heap,
                      PHEAP_FREE_ENTRY FreeEntry,
                      SIZE_T Size)
{
    PHEAP_SEGMENT Segment;
    PHEAP_ENTRY PrecedingInUseEntry = NULL, NextInUseEntry = NULL;
    PHEAP_FREE_ENTRY NextFreeEntry;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;
    SIZE_T PrecedingSize, NextSize, DecommitSize;
    ULONG_PTR DecommitBase;
    NTSTATUS Status;

    DPRINT("Decommitting %p %p %x\n", Heap, FreeEntry, Size);

    /* We can't decommit if there is a commit routine! */
    if (Heap->CommitRoutine)
    {
        /* Just add it back the usual way */
        RtlpInsertFreeBlock(Heap, FreeEntry, Size);
        return;
    }

    /* Get the segment */
    Segment = Heap->Segments[FreeEntry->SegmentOffset];

    /* Get the preceding entry */
    DecommitBase = ROUND_UP(FreeEntry, PAGE_SIZE);
    PrecedingSize = (PHEAP_ENTRY)DecommitBase - (PHEAP_ENTRY)FreeEntry;

    if (PrecedingSize == 1)
    {
        /* Just 1 heap entry, increase the base/size */
        DecommitBase += PAGE_SIZE;
        PrecedingSize += PAGE_SIZE >> HEAP_ENTRY_SHIFT;
    }
    else if (FreeEntry->PreviousSize &&
             (DecommitBase == (ULONG_PTR)FreeEntry))
    {
        PrecedingInUseEntry = (PHEAP_ENTRY)FreeEntry - FreeEntry->PreviousSize;
    }

    /* Get the next entry */
    NextFreeEntry = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeEntry + Size);
    DecommitSize = ROUND_DOWN(NextFreeEntry, PAGE_SIZE);
    NextSize = (PHEAP_ENTRY)NextFreeEntry - (PHEAP_ENTRY)DecommitSize;

    if (NextSize == 1)
    {
        /* Just 1 heap entry, increase the size */
        DecommitSize -= PAGE_SIZE;
        NextSize += PAGE_SIZE >> HEAP_ENTRY_SHIFT;
    }
    else if (NextSize == 0 &&
             !(FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
    {
        NextInUseEntry = (PHEAP_ENTRY)NextFreeEntry;
    }

    NextFreeEntry = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)NextFreeEntry - NextSize);

    /* Calculate real decommit size */
    if (DecommitSize > DecommitBase)
    {
        DecommitSize -= DecommitBase;
    }
    else
    {
        /* Nothing to decommit */
        RtlpInsertFreeBlock(Heap, FreeEntry, Size);
        return;
    }

    /* A decommit is necessary. Create a UCR descriptor */
    UcrDescriptor = RtlpCreateUnCommittedRange(Segment);
    if (!UcrDescriptor)
    {
        DPRINT1("HEAP: Failed to create UCR descriptor\n");
        RtlpInsertFreeBlock(Heap, FreeEntry, PrecedingSize);
        return;
    }

    /* Decommit the memory */
    Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                 (PVOID *)&DecommitBase,
                                 &DecommitSize,
                                 MEM_DECOMMIT);

    /* Delete that UCR. This is needed to assure there is an unused UCR entry in the list */
    RtlpDestroyUnCommittedRange(Segment, UcrDescriptor);

    if (!NT_SUCCESS(Status))
    {
        RtlpInsertFreeBlock(Heap, FreeEntry, Size);
        return;
    }

    /* Insert uncommitted pages */
    RtlpInsertUnCommittedPages(Segment, DecommitBase, DecommitSize);
    Segment->NumberOfUnCommittedPages += (ULONG)(DecommitSize / PAGE_SIZE);

    if (PrecedingSize)
    {
        /* Adjust size of this free entry and insert it */
        FreeEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
        FreeEntry->Size = (USHORT)PrecedingSize;
        Heap->TotalFreeSize += PrecedingSize;

        /* Insert it into the free list */
        RtlpInsertFreeBlockHelper(Heap, FreeEntry, PrecedingSize, FALSE);
    }
    else if (PrecedingInUseEntry)
    {
        /* Adjust preceding in use entry */
        PrecedingInUseEntry->Flags |= HEAP_ENTRY_LAST_ENTRY;
    }

    /* Now the next one */
    if (NextSize)
    {
        /* Adjust size of this free entry and insert it */
        NextFreeEntry->Flags = 0;
        NextFreeEntry->PreviousSize = 0;
        NextFreeEntry->SegmentOffset = Segment->Entry.SegmentOffset;
        NextFreeEntry->Size = (USHORT)NextSize;

        ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)NextFreeEntry + NextSize))->PreviousSize = (USHORT)NextSize;

        Heap->TotalFreeSize += NextSize;
        RtlpInsertFreeBlockHelper(Heap, NextFreeEntry, NextSize, FALSE);
    }
    else if (NextInUseEntry)
    {
        NextInUseEntry->PreviousSize = 0;
    }
}

NTSTATUS
NTAPI
RtlpInitializeHeapSegment(IN OUT PHEAP Heap,
                          OUT PHEAP_SEGMENT Segment,
                          IN UCHAR SegmentIndex,
                          IN ULONG SegmentFlags,
                          IN SIZE_T SegmentReserve,
                          IN SIZE_T SegmentCommit)
{
    PHEAP_ENTRY HeapEntry;

    /* Preconditions */
    ASSERT(Heap != NULL);
    ASSERT(Segment != NULL);
    ASSERT(SegmentCommit >= PAGE_SIZE);
    ASSERT(ROUND_DOWN(SegmentCommit, PAGE_SIZE) == SegmentCommit);
    ASSERT(SegmentReserve >= SegmentCommit);
    ASSERT(ROUND_DOWN(SegmentReserve, PAGE_SIZE) == SegmentReserve);

    DPRINT("RtlpInitializeHeapSegment(%p %p %x %x %lx %lx)\n", Heap, Segment, SegmentIndex, SegmentFlags, SegmentReserve, SegmentCommit);

    /* Initialise the Heap Entry header if this is not the first Heap Segment */
    if ((PHEAP_SEGMENT) (Heap) != Segment)
    {
        Segment->Entry.Size = ROUND_UP(sizeof(HEAP_SEGMENT), sizeof(HEAP_ENTRY)) >> HEAP_ENTRY_SHIFT;
        Segment->Entry.Flags = HEAP_ENTRY_BUSY;
        Segment->Entry.SmallTagIndex = LOBYTE(Segment->Entry.Size) ^ HIBYTE(Segment->Entry.Size) ^ Segment->Entry.Flags;
        Segment->Entry.PreviousSize = 0;
        Segment->Entry.SegmentOffset = SegmentIndex;
        Segment->Entry.UnusedBytes = 0;
    }

    /* Sanity check */
    ASSERT((Segment->Entry.Size << HEAP_ENTRY_SHIFT) <= PAGE_SIZE);

    /* Initialise the Heap Segment header */
    Segment->SegmentSignature = HEAP_SEGMENT_SIGNATURE;
    Segment->SegmentFlags = SegmentFlags;
    Segment->Heap = Heap;
    Heap->Segments[SegmentIndex] = Segment;

    /* Initialise the Heap Segment location information */
    Segment->BaseAddress = Segment;
    Segment->NumberOfPages = (ULONG)(SegmentReserve >> PAGE_SHIFT);

    /* Initialise the Heap Entries contained within the Heap Segment */
    Segment->FirstEntry = &Segment->Entry + Segment->Entry.Size;
    Segment->LastValidEntry = (PHEAP_ENTRY)((ULONG_PTR)Segment + SegmentReserve);

    if (((SIZE_T)Segment->Entry.Size << HEAP_ENTRY_SHIFT) < SegmentCommit)
    {
        HeapEntry = Segment->FirstEntry;

        /* Prepare a Free Heap Entry header */
        HeapEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
        HeapEntry->PreviousSize = Segment->Entry.Size;
        HeapEntry->SegmentOffset = SegmentIndex;

        /* Register the Free Heap Entry */
        RtlpInsertFreeBlock(Heap, (PHEAP_FREE_ENTRY) HeapEntry, (SegmentCommit >> HEAP_ENTRY_SHIFT) - Segment->Entry.Size);
    }

    /* Initialise the Heap Segment UnCommitted Range information */
    Segment->NumberOfUnCommittedPages = (ULONG)((SegmentReserve - SegmentCommit) >> PAGE_SHIFT);
    Segment->NumberOfUnCommittedRanges = 0;
    InitializeListHead(&Segment->UCRSegmentList);

    /* Register the UnCommitted Range of the Heap Segment */
    if (Segment->NumberOfUnCommittedPages != 0)
        RtlpInsertUnCommittedPages(Segment, (ULONG_PTR) (Segment) + SegmentCommit, SegmentReserve - SegmentCommit);

    return STATUS_SUCCESS;
}

VOID NTAPI
RtlpDestroyHeapSegment(PHEAP_SEGMENT Segment)
{
    NTSTATUS Status;
    PVOID BaseAddress;
    SIZE_T Size = 0;

    /* Make sure it's not user allocated */
    if (Segment->SegmentFlags & HEAP_USER_ALLOCATED) return;

    BaseAddress = Segment->BaseAddress;
    DPRINT("Destroying segment %p, BA %p\n", Segment, BaseAddress);

    /* Release virtual memory */
    Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                 &BaseAddress,
                                 &Size,
                                 MEM_RELEASE);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("HEAP: Failed to release segment's memory with status 0x%08X\n", Status);
    }
}

PHEAP_FREE_ENTRY NTAPI
RtlpCoalesceHeap(PHEAP Heap)
{
    UNIMPLEMENTED;
    return NULL;
}

PHEAP_FREE_ENTRY NTAPI
RtlpCoalesceFreeBlocks (PHEAP Heap,
                        PHEAP_FREE_ENTRY FreeEntry,
                        PSIZE_T FreeSize,
                        BOOLEAN Remove)
{
    PHEAP_FREE_ENTRY CurrentEntry, NextEntry;

    /* Get the previous entry */
    CurrentEntry = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeEntry - FreeEntry->PreviousSize);

    /* Check it */
    if (CurrentEntry != FreeEntry &&
        !(CurrentEntry->Flags & HEAP_ENTRY_BUSY) &&
        (*FreeSize + CurrentEntry->Size) <= HEAP_MAX_BLOCK_SIZE)
    {
        ASSERT(FreeEntry->PreviousSize == CurrentEntry->Size);

        /* Remove it if asked for */
        if (Remove)
        {
            RtlpRemoveFreeBlock(Heap, FreeEntry, FALSE, FALSE);
            Heap->TotalFreeSize -= FreeEntry->Size;

            /* Remove it only once! */
            Remove = FALSE;
        }

        /* Remove previous entry too */
        RtlpRemoveFreeBlock(Heap, CurrentEntry, FALSE, FALSE);

        /* Copy flags */
        CurrentEntry->Flags = FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY;

        /* Advance FreeEntry and update sizes */
        FreeEntry = CurrentEntry;
        *FreeSize = *FreeSize + CurrentEntry->Size;
        Heap->TotalFreeSize -= CurrentEntry->Size;
        FreeEntry->Size = (USHORT)(*FreeSize);

        /* Also update previous size if needed */
        if (!(FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
        {
            ((PHEAP_ENTRY)FreeEntry + *FreeSize)->PreviousSize = (USHORT)(*FreeSize);
        }
    }

    /* Check the next block if it exists */
    if (!(FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
    {
        NextEntry = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeEntry + *FreeSize);

        if (!(NextEntry->Flags & HEAP_ENTRY_BUSY) &&
            NextEntry->Size + *FreeSize <= HEAP_MAX_BLOCK_SIZE)
        {
            ASSERT(*FreeSize == NextEntry->PreviousSize);

            /* Remove it if asked for */
            if (Remove)
            {
                RtlpRemoveFreeBlock(Heap, FreeEntry, FALSE, FALSE);
                Heap->TotalFreeSize -= FreeEntry->Size;
            }

            /* Copy flags */
            FreeEntry->Flags = NextEntry->Flags & HEAP_ENTRY_LAST_ENTRY;

            /* Remove next entry now */
            RtlpRemoveFreeBlock(Heap, NextEntry, FALSE, FALSE);

            /* Update sizes */
            *FreeSize = *FreeSize + NextEntry->Size;
            Heap->TotalFreeSize -= NextEntry->Size;
            FreeEntry->Size = (USHORT)(*FreeSize);

            /* Also update previous size if needed */
            if (!(FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
            {
                ((PHEAP_ENTRY)FreeEntry + *FreeSize)->PreviousSize = (USHORT)(*FreeSize);
            }
        }
    }
    return FreeEntry;
}

PHEAP_FREE_ENTRY NTAPI
RtlpExtendHeap(PHEAP Heap,
               SIZE_T Size)
{
    ULONG Pages;
    UCHAR Index, EmptyIndex;
    SIZE_T FreeSize, CommitSize, ReserveSize;
    PHEAP_SEGMENT Segment;
    PHEAP_FREE_ENTRY FreeEntry;
    NTSTATUS Status;

    DPRINT("RtlpExtendHeap(%p %x)\n", Heap, Size);

    /* Calculate amount in pages */
    Pages = (ULONG)((Size + PAGE_SIZE - 1) / PAGE_SIZE);
    FreeSize = Pages * PAGE_SIZE;
    DPRINT("Pages %x, FreeSize %x. Going through segments...\n", Pages, FreeSize);

    /* Find an empty segment */
    EmptyIndex = HEAP_SEGMENTS;
    for (Index = 0; Index < HEAP_SEGMENTS; Index++)
    {
        Segment = Heap->Segments[Index];

        if (Segment) DPRINT("Segment[%u] %p with NOUCP %x\n", Index, Segment, Segment->NumberOfUnCommittedPages);

        /* Check if its size suits us */
        if (Segment &&
            Pages <= Segment->NumberOfUnCommittedPages)
        {
            DPRINT("This segment is suitable\n");

            /* Commit needed amount */
            FreeEntry = RtlpFindAndCommitPages(Heap, Segment, &FreeSize, NULL);

            /* Coalesce it with adjacent entries */
            if (FreeEntry)
            {
                FreeSize = FreeSize >> HEAP_ENTRY_SHIFT;
                FreeEntry = RtlpCoalesceFreeBlocks(Heap, FreeEntry, &FreeSize, FALSE);
                RtlpInsertFreeBlock(Heap, FreeEntry, FreeSize);
                return FreeEntry;
            }
        }
        else if (!Segment &&
                 EmptyIndex == HEAP_SEGMENTS)
        {
            /* Remember the first unused segment index */
            EmptyIndex = Index;
        }
    }

    /* No luck, need to grow the heap */
    if ((Heap->Flags & HEAP_GROWABLE) &&
        (EmptyIndex != HEAP_SEGMENTS))
    {
        Segment = NULL;

        /* Reserve the memory */
        if ((Size + PAGE_SIZE) <= Heap->SegmentReserve)
            ReserveSize = Heap->SegmentReserve;
        else
            ReserveSize = Size + PAGE_SIZE;

        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         (PVOID)&Segment,
                                         0,
                                         &ReserveSize,
                                         MEM_RESERVE,
                                         PAGE_READWRITE);

        /* If it failed, retry again with a half division algorithm */
        while (!NT_SUCCESS(Status) &&
            ReserveSize != Size + PAGE_SIZE)
        {
            ReserveSize /= 2;

            if (ReserveSize < (Size + PAGE_SIZE))
                ReserveSize = Size + PAGE_SIZE;

            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             (PVOID)&Segment,
                                             0,
                                             &ReserveSize,
                                             MEM_RESERVE,
                                             PAGE_READWRITE);
        }

        /* Proceed only if it's success */
        if (NT_SUCCESS(Status))
        {
            Heap->SegmentReserve += ReserveSize;

            /* Now commit the memory */
            if ((Size + PAGE_SIZE) <= Heap->SegmentCommit)
                CommitSize = Heap->SegmentCommit;
            else
                CommitSize = Size + PAGE_SIZE;

            Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                             (PVOID)&Segment,
                                             0,
                                             &CommitSize,
                                             MEM_COMMIT,
                                             PAGE_READWRITE);

            DPRINT("Committed %lu bytes at base %p\n", CommitSize, Segment);

            /* Initialize heap segment if commit was successful */
            if (NT_SUCCESS(Status))
                Status = RtlpInitializeHeapSegment(Heap, Segment, EmptyIndex, 0, ReserveSize, CommitSize);

            /* If everything worked - cool */
            if (NT_SUCCESS(Status)) return (PHEAP_FREE_ENTRY)Segment->FirstEntry;

            DPRINT1("Committing failed with status 0x%08X\n", Status);

            /* Nope, we failed. Free memory */
            ZwFreeVirtualMemory(NtCurrentProcess(),
                                (PVOID)&Segment,
                                &ReserveSize,
                                MEM_RELEASE);
        }
        else
        {
            DPRINT1("Reserving failed with status 0x%08X\n", Status);
        }
    }

    if (RtlpGetMode() == UserMode)
    {
        /* If coalescing on free is disabled in usermode, then do it here */
        if (Heap->Flags & HEAP_DISABLE_COALESCE_ON_FREE)
        {
            FreeEntry = RtlpCoalesceHeap(Heap);

            /* If it's a suitable one - return it */
            if (FreeEntry &&
                FreeEntry->Size >= Size)
            {
                return FreeEntry;
            }
        }
    }

    return NULL;
}

/***********************************************************************
 *           RtlCreateHeap
 * RETURNS
 * Handle of heap: Success
 * NULL: Failure
 *
 * @implemented
 */
HANDLE NTAPI
RtlCreateHeap(ULONG Flags,
              PVOID Addr,
              SIZE_T TotalSize,
              SIZE_T CommitSize,
              PVOID Lock,
              PRTL_HEAP_PARAMETERS Parameters)
{
    PVOID CommittedAddress = NULL, UncommittedAddress = NULL;
    PHEAP Heap = NULL;
    RTL_HEAP_PARAMETERS SafeParams = {0};
    ULONG_PTR MaximumUserModeAddress;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    MEMORY_BASIC_INFORMATION MemoryInfo;
    ULONG NtGlobalFlags = RtlGetNtGlobalFlags();
    ULONG HeapSegmentFlags = 0;
    NTSTATUS Status;
    ULONG MaxBlockSize;

    /* Check for a special heap */
    if (RtlpPageHeapEnabled && !Addr && !Lock)
    {
        Heap = RtlpPageHeapCreate(Flags, Addr, TotalSize, CommitSize, Lock, Parameters);
        if (Heap) return Heap;

        /* Reset a special Parameters == -1 hack */
        if ((ULONG_PTR)Parameters == (ULONG_PTR)-1)
            Parameters = NULL;
        else
            DPRINT1("Enabling page heap failed\n");
    }

    /* Check validation flags */
    if (!(Flags & HEAP_SKIP_VALIDATION_CHECKS) && (Flags & ~HEAP_CREATE_VALID_MASK))
    {
        DPRINT1("Invalid flags 0x%08x, fixing...\n", Flags);
        Flags &= HEAP_CREATE_VALID_MASK;
    }

    /* TODO: Capture parameters, once we decide to use SEH */
    if (!Parameters) Parameters = &SafeParams;

    /* Check global flags */
    if (NtGlobalFlags & FLG_HEAP_DISABLE_COALESCING)
        Flags |= HEAP_DISABLE_COALESCE_ON_FREE;

    if (NtGlobalFlags & FLG_HEAP_ENABLE_FREE_CHECK)
        Flags |= HEAP_FREE_CHECKING_ENABLED;

    if (NtGlobalFlags & FLG_HEAP_ENABLE_TAIL_CHECK)
        Flags |= HEAP_TAIL_CHECKING_ENABLED;

    if (RtlpGetMode() == UserMode)
    {
        /* Also check these flags if in usermode */
        if (NtGlobalFlags & FLG_HEAP_VALIDATE_ALL)
            Flags |= HEAP_VALIDATE_ALL_ENABLED;

        if (NtGlobalFlags & FLG_HEAP_VALIDATE_PARAMETERS)
            Flags |= HEAP_VALIDATE_PARAMETERS_ENABLED;

        if (NtGlobalFlags & FLG_USER_STACK_TRACE_DB)
            Flags |= HEAP_CAPTURE_STACK_BACKTRACES;
    }

    /* Set tunable parameters */
    RtlpSetHeapParameters(Parameters);

    /* Get the max um address */
    Status = ZwQuerySystemInformation(SystemBasicInformation,
                                      &SystemInformation,
                                      sizeof(SystemInformation),
                                      NULL);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Getting max usermode address failed with status 0x%08x\n", Status);
        return NULL;
    }

    MaximumUserModeAddress = SystemInformation.MaximumUserModeAddress;

    /* Calculate max alloc size */
    if (!Parameters->MaximumAllocationSize)
        Parameters->MaximumAllocationSize = MaximumUserModeAddress - (ULONG_PTR)0x10000 - PAGE_SIZE;

    MaxBlockSize = 0x80000 - PAGE_SIZE;

    if (!Parameters->VirtualMemoryThreshold ||
        Parameters->VirtualMemoryThreshold > MaxBlockSize)
    {
        Parameters->VirtualMemoryThreshold = MaxBlockSize;
    }

    /* Check reserve/commit sizes and set default values */
    if (!CommitSize)
    {
        CommitSize = PAGE_SIZE;
        if (TotalSize)
            TotalSize = ROUND_UP(TotalSize, PAGE_SIZE);
        else
            TotalSize = 64 * PAGE_SIZE;
    }
    else
    {
        /* Round up the commit size to be at least the page size */
        CommitSize = ROUND_UP(CommitSize, PAGE_SIZE);

        if (TotalSize)
            TotalSize = ROUND_UP(TotalSize, PAGE_SIZE);
        else
            TotalSize = ROUND_UP(CommitSize, 16 * PAGE_SIZE);
    }

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugCreateHeap(Flags, Addr, TotalSize, CommitSize, Lock, Parameters);

    /* Without serialization, a lock makes no sense */
    if ((Flags & HEAP_NO_SERIALIZE) && (Lock != NULL))
        return NULL;

    /* See if we are already provided with an address for the heap */
    if (Addr)
    {
        if (Parameters->CommitRoutine)
        {
            /* There is a commit routine, so no problem here, check params */
            if ((Flags & HEAP_GROWABLE) ||
                !Parameters->InitialCommit ||
                !Parameters->InitialReserve ||
                (Parameters->InitialCommit > Parameters->InitialReserve))
            {
                /* Fail */
                return NULL;
            }

            /* Calculate committed and uncommitted addresses */
            CommittedAddress = Addr;
            UncommittedAddress = (PCHAR)Addr + Parameters->InitialCommit;
            TotalSize = Parameters->InitialReserve;

            /* Zero the initial page ourselves */
            RtlZeroMemory(CommittedAddress, PAGE_SIZE);
        }
        else
        {
            /* Commit routine is absent, so query how much memory caller reserved */
            Status = ZwQueryVirtualMemory(NtCurrentProcess(),
                                          Addr,
                                          MemoryBasicInformation,
                                          &MemoryInfo,
                                          sizeof(MemoryInfo),
                                          NULL);

            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Querying amount of user supplied memory failed with status 0x%08X\n", Status);
                return NULL;
            }

            /* Validate it */
            if (MemoryInfo.BaseAddress != Addr ||
                MemoryInfo.State == MEM_FREE)
            {
                return NULL;
            }

            /* Validation checks passed, set committed/uncommitted addresses */
            CommittedAddress = Addr;

            /* Check if it's committed or not */
            if (MemoryInfo.State == MEM_COMMIT)
            {
                /* Zero it out because it's already committed */
                RtlZeroMemory(CommittedAddress, PAGE_SIZE);

                /* Calculate uncommitted address value */
                CommitSize = MemoryInfo.RegionSize;
                TotalSize = CommitSize;
                UncommittedAddress = (PCHAR)Addr + CommitSize;

                /* Check if uncommitted address is reserved */
                Status = ZwQueryVirtualMemory(NtCurrentProcess(),
                                              UncommittedAddress,
                                              MemoryBasicInformation,
                                              &MemoryInfo,
                                              sizeof(MemoryInfo),
                                              NULL);

                if (NT_SUCCESS(Status) &&
                    MemoryInfo.State == MEM_RESERVE)
                {
                    /* It is, so add it up to the reserve size */
                    TotalSize += MemoryInfo.RegionSize;
                }
            }
            else
            {
                /* It's not committed, inform following code that a commit is necessary */
                CommitSize = PAGE_SIZE;
                UncommittedAddress = Addr;
            }
        }

        /* Mark this as a user-committed mem */
        HeapSegmentFlags = HEAP_USER_ALLOCATED;
        Heap = (PHEAP)Addr;
    }
    else
    {
        /* Check commit routine */
        if (Parameters->CommitRoutine) return NULL;

        /* Reserve memory */
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         (PVOID *)&Heap,
                                         0,
                                         &TotalSize,
                                         MEM_RESERVE,
                                         PAGE_READWRITE);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to reserve memory with status 0x%08x\n", Status);
            return NULL;
        }

        /* Set base addresses */
        CommittedAddress = Heap;
        UncommittedAddress = Heap;
    }

    /* Check if we need to commit something */
    if (CommittedAddress == UncommittedAddress)
    {
        /* Commit the required size */
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         &CommittedAddress,
                                         0,
                                         &CommitSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);

        DPRINT("Committed %Iu bytes at base %p\n", CommitSize, CommittedAddress);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failure, Status 0x%08X\n", Status);

            /* Release memory if it was reserved */
            if (!Addr) ZwFreeVirtualMemory(NtCurrentProcess(),
                                           (PVOID *)&Heap,
                                           &TotalSize,
                                           MEM_RELEASE);

            return NULL;
        }

        /* Calculate new uncommitted address */
        UncommittedAddress = (PCHAR)UncommittedAddress + CommitSize;
    }

    /* Initialize the heap */
    Status = RtlpInitializeHeap(Heap, Flags, Lock, Parameters);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize heap (%x)\n", Status);
        return NULL;
    }

    /* Initialize heap's first segment */
    Status = RtlpInitializeHeapSegment(Heap, (PHEAP_SEGMENT) (Heap), 0, HeapSegmentFlags, TotalSize, CommitSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize heap segment (%x)\n", Status);
        return NULL;
    }

    DPRINT("Created heap %p, CommitSize %x, ReserveSize %x\n", Heap, CommitSize, TotalSize);

    /* Add heap to process list in case of usermode heap */
    if (RtlpGetMode() == UserMode)
    {
        RtlpAddHeapToProcessList(Heap);

        // FIXME: What about lookasides?
    }

    return Heap;
}

/***********************************************************************
 *           RtlDestroyHeap
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 *
 * RETURNS
 *  Success: A NULL HANDLE, if heap is NULL or it was destroyed
 *  Failure: The Heap handle, if heap is the process heap.
 */
HANDLE NTAPI
RtlDestroyHeap(HANDLE HeapPtr) /* [in] Handle of heap */
{
    PHEAP Heap = (PHEAP)HeapPtr;
    PLIST_ENTRY Current;
    PHEAP_UCR_SEGMENT UcrSegment;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualEntry;
    PVOID BaseAddress;
    SIZE_T Size;
    LONG i;
    PHEAP_SEGMENT Segment;

    if (!HeapPtr) return NULL;

    /* Call page heap routine if required */
    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS) return RtlpPageHeapDestroy(HeapPtr);

    /* Call special heap */
    if (RtlpHeapIsSpecial(Heap->Flags))
    {
        if (!RtlDebugDestroyHeap(Heap)) return HeapPtr;
    }

    /* Check for a process heap */
    if (RtlpGetMode() == UserMode &&
        HeapPtr == NtCurrentPeb()->ProcessHeap) return HeapPtr;

    /* Free up all big allocations */
    Current = Heap->VirtualAllocdBlocks.Flink;
    while (Current != &Heap->VirtualAllocdBlocks)
    {
        VirtualEntry = CONTAINING_RECORD(Current, HEAP_VIRTUAL_ALLOC_ENTRY, Entry);
        BaseAddress = (PVOID)VirtualEntry;
        Current = Current->Flink;
        Size = 0;
        ZwFreeVirtualMemory(NtCurrentProcess(),
                            &BaseAddress,
                            &Size,
                            MEM_RELEASE);
    }

    /* Delete tags and remove heap from the process heaps list in user mode */
    if (RtlpGetMode() == UserMode)
    {
        // FIXME DestroyTags
        RtlpRemoveHeapFromProcessList(Heap);
    }

    /* Delete the heap lock */
    if (!(Heap->Flags & HEAP_NO_SERIALIZE))
    {
        /* Delete it if it wasn't user allocated */
        if (!(Heap->Flags & HEAP_LOCK_USER_ALLOCATED))
            RtlDeleteHeapLock(Heap->LockVariable);

        /* Clear out the lock variable */
        Heap->LockVariable = NULL;
    }

    /* Free UCR segments if any were created */
    Current = Heap->UCRSegments.Flink;
    while (Current != &Heap->UCRSegments)
    {
        UcrSegment = CONTAINING_RECORD(Current, HEAP_UCR_SEGMENT, ListEntry);

        /* Advance to the next descriptor */
        Current = Current->Flink;

        BaseAddress = (PVOID)UcrSegment;
        Size = 0;

        /* Release that memory */
        ZwFreeVirtualMemory(NtCurrentProcess(),
                            &BaseAddress,
                            &Size,
                            MEM_RELEASE);
    }

    /* Go through segments and destroy them */
    for (i = HEAP_SEGMENTS - 1; i >= 0; i--)
    {
        Segment = Heap->Segments[i];
        if (Segment) RtlpDestroyHeapSegment(Segment);
    }

    return NULL;
}

PHEAP_ENTRY NTAPI
RtlpSplitEntry(PHEAP Heap,
               ULONG Flags,
               PHEAP_FREE_ENTRY FreeBlock,
               SIZE_T AllocationSize,
               SIZE_T Index,
               SIZE_T Size)
{
    PHEAP_FREE_ENTRY SplitBlock, SplitBlock2;
    UCHAR FreeFlags, EntryFlags = HEAP_ENTRY_BUSY;
    PHEAP_ENTRY InUseEntry;
    SIZE_T FreeSize;

    /* Add extra flags in case of settable user value feature is requested,
       or there is a tag (small or normal) or there is a request to
       capture stack backtraces */
    if ((Flags & HEAP_EXTRA_FLAGS_MASK) ||
        Heap->PseudoTagEntries)
    {
        /* Add flag which means that the entry will have extra stuff attached */
        EntryFlags |= HEAP_ENTRY_EXTRA_PRESENT;

        /* NB! AllocationSize is already adjusted by RtlAllocateHeap */
    }

    /* Add settable user flags, if any */
    EntryFlags |= (Flags & HEAP_SETTABLE_USER_FLAGS) >> 4;

    /* Save flags, update total free size */
    FreeFlags = FreeBlock->Flags;
    Heap->TotalFreeSize -= FreeBlock->Size;

    /* Make this block an in-use one */
    InUseEntry = (PHEAP_ENTRY)FreeBlock;
    InUseEntry->Flags = EntryFlags;
    InUseEntry->SmallTagIndex = 0;

    /* Calculate the extra amount */
    FreeSize = InUseEntry->Size - Index;

    /* Update it's size fields (we don't need their data anymore) */
    InUseEntry->Size = (USHORT)Index;
    InUseEntry->UnusedBytes = (UCHAR)(AllocationSize - Size);

    /* If there is something to split - do the split */
    if (FreeSize != 0)
    {
        /* Don't split if resulting entry can't contain any payload data
        (i.e. being just HEAP_ENTRY_SIZE) */
        if (FreeSize == 1)
        {
            /* Increase sizes of the in-use entry */
            InUseEntry->Size++;
            InUseEntry->UnusedBytes += sizeof(HEAP_ENTRY);
        }
        else
        {
            /* Calculate a pointer to the new entry */
            SplitBlock = (PHEAP_FREE_ENTRY)(InUseEntry + Index);

            /* Initialize it */
            SplitBlock->Flags = FreeFlags;
            SplitBlock->SegmentOffset = InUseEntry->SegmentOffset;
            SplitBlock->Size = (USHORT)FreeSize;
            SplitBlock->PreviousSize = (USHORT)Index;

            /* Check if it's the last entry */
            if (FreeFlags & HEAP_ENTRY_LAST_ENTRY)
            {
                /* Insert it to the free list if it's the last entry */
                RtlpInsertFreeBlockHelper(Heap, SplitBlock, FreeSize, FALSE);
                Heap->TotalFreeSize += FreeSize;
            }
            else
            {
                /* Not so easy - need to update next's previous size too */
                SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);

                if (SplitBlock2->Flags & HEAP_ENTRY_BUSY)
                {
                    SplitBlock2->PreviousSize = (USHORT)FreeSize;
                    RtlpInsertFreeBlockHelper(Heap, SplitBlock, FreeSize, FALSE);
                    Heap->TotalFreeSize += FreeSize;
                }
                else
                {
                    /* Even more complex - the next entry is free, so we can merge them into one! */
                    SplitBlock->Flags = SplitBlock2->Flags;

                    /* Remove that next entry */
                    RtlpRemoveFreeBlock(Heap, SplitBlock2, FALSE, FALSE);

                    /* Update sizes */
                    FreeSize += SplitBlock2->Size;
                    Heap->TotalFreeSize -= SplitBlock2->Size;

                    if (FreeSize <= HEAP_MAX_BLOCK_SIZE)
                    {
                        /* Insert it back */
                        SplitBlock->Size = (USHORT)FreeSize;

                        /* Don't forget to update previous size of the next entry! */
                        if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY))
                        {
                            ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                        }

                        /* Actually insert it */
                        RtlpInsertFreeBlockHelper(Heap, SplitBlock, (USHORT)FreeSize, FALSE);

                        /* Update total size */
                        Heap->TotalFreeSize += FreeSize;
                    }
                    else
                    {
                        /* Resulting block is quite big */
                        RtlpInsertFreeBlock(Heap, SplitBlock, FreeSize);
                    }
                }
            }

            /* Reset flags of the free entry */
            FreeFlags = 0;
        }
    }

    /* Set last entry flag */
    if (FreeFlags & HEAP_ENTRY_LAST_ENTRY)
        InUseEntry->Flags |= HEAP_ENTRY_LAST_ENTRY;

    return InUseEntry;
}

PVOID NTAPI
RtlpAllocateNonDedicated(PHEAP Heap,
                         ULONG Flags,
                         SIZE_T Size,
                         SIZE_T AllocationSize,
                         SIZE_T Index,
                         BOOLEAN HeapLocked)
{
    PLIST_ENTRY FreeListHead, Next;
    PHEAP_FREE_ENTRY FreeBlock;
    PHEAP_ENTRY InUseEntry;
    PHEAP_ENTRY_EXTRA Extra;
    EXCEPTION_RECORD ExceptionRecord;

    /* Go through the zero list to find a place where to insert the new entry */
    FreeListHead = &Heap->FreeLists[0];

    /* Start from the largest block to reduce time */
    Next = FreeListHead->Blink;
    if (FreeListHead != Next)
    {
        FreeBlock = CONTAINING_RECORD(Next, HEAP_FREE_ENTRY, FreeList);

        if (FreeBlock->Size >= Index)
        {
            /* Our request is smaller than the largest entry in the zero list */

            /* Go through the list to find insertion place */
            Next = FreeListHead->Flink;
            while (FreeListHead != Next)
            {
                FreeBlock = CONTAINING_RECORD(Next, HEAP_FREE_ENTRY, FreeList);

                if (FreeBlock->Size >= Index)
                {
                    /* Found minimally fitting entry. Proceed to either using it as it is
                    or splitting it to two entries */
                    RemoveEntryList(&FreeBlock->FreeList);

                    /* Split it */
                    InUseEntry = RtlpSplitEntry(Heap, Flags, FreeBlock, AllocationSize, Index, Size);

                    /* Release the lock */
                    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

                    /* Zero memory if that was requested */
                    if (Flags & HEAP_ZERO_MEMORY)
                        RtlZeroMemory(InUseEntry + 1, Size);
                    else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
                    {
                        /* Fill this block with a special pattern */
                        RtlFillMemoryUlong(InUseEntry + 1, Size & ~0x3, ARENA_INUSE_FILLER);
                    }

                    /* Fill tail of the block with a special pattern too if requested */
                    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED)
                    {
                        RtlFillMemory((PCHAR)(InUseEntry + 1) + Size, sizeof(HEAP_ENTRY), HEAP_TAIL_FILL);
                        InUseEntry->Flags |= HEAP_ENTRY_FILL_PATTERN;
                    }

                    /* Prepare extra if it's present */
                    if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
                    {
                        Extra = RtlpGetExtraStuffPointer(InUseEntry);
                        RtlZeroMemory(Extra, sizeof(HEAP_ENTRY_EXTRA));

                        // TODO: Tagging
                    }

                    /* Return pointer to the */
                    return InUseEntry + 1;
                }

                /* Advance to the next entry */
                Next = Next->Flink;
            }
        }
    }

    /* Extend the heap, 0 list didn't have anything suitable */
    FreeBlock = RtlpExtendHeap(Heap, AllocationSize);

    /* Use the new biggest entry we've got */
    if (FreeBlock)
    {
        RemoveEntryList(&FreeBlock->FreeList);

        /* Split it */
        InUseEntry = RtlpSplitEntry(Heap, Flags, FreeBlock, AllocationSize, Index, Size);

        /* Release the lock */
        if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

        /* Zero memory if that was requested */
        if (Flags & HEAP_ZERO_MEMORY)
            RtlZeroMemory(InUseEntry + 1, Size);
        else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
        {
            /* Fill this block with a special pattern */
            RtlFillMemoryUlong(InUseEntry + 1, Size & ~0x3, ARENA_INUSE_FILLER);
        }

        /* Fill tail of the block with a special pattern too if requested */
        if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED)
        {
            RtlFillMemory((PCHAR)(InUseEntry + 1) + Size, sizeof(HEAP_ENTRY), HEAP_TAIL_FILL);
            InUseEntry->Flags |= HEAP_ENTRY_FILL_PATTERN;
        }

        /* Prepare extra if it's present */
        if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
        {
            Extra = RtlpGetExtraStuffPointer(InUseEntry);
            RtlZeroMemory(Extra, sizeof(HEAP_ENTRY_EXTRA));

            // TODO: Tagging
        }

        /* Return pointer to the */
        return InUseEntry + 1;
    }

    /* Really unfortunate, out of memory condition */
    RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_NO_MEMORY);

    /* Generate an exception */
    if (Flags & HEAP_GENERATE_EXCEPTIONS)
    {
        ExceptionRecord.ExceptionCode = STATUS_NO_MEMORY;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.NumberParameters = 1;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[0] = AllocationSize;

        RtlRaiseException(&ExceptionRecord);
    }

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);
    DPRINT1("HEAP: Allocation failed!\n");
    DPRINT1("Flags %x\n", Heap->Flags);
    return NULL;
}

/***********************************************************************
 *           HeapAlloc   (KERNEL32.334)
 * RETURNS
 * Pointer to allocated memory block
 * NULL: Failure
 * 0x7d030f60--invalid flags in RtlHeapAllocate
 * @implemented
 */
PVOID NTAPI
RtlAllocateHeap(IN PVOID HeapPtr,
                IN ULONG Flags,
                IN SIZE_T Size)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    PULONG FreeListsInUse;
    ULONG FreeListsInUseUlong;
    SIZE_T AllocationSize;
    SIZE_T Index, InUseIndex, i;
    PLIST_ENTRY FreeListHead;
    PHEAP_ENTRY InUseEntry;
    PHEAP_FREE_ENTRY FreeBlock;
    UCHAR FreeFlags, EntryFlags = HEAP_ENTRY_BUSY;
    EXCEPTION_RECORD ExceptionRecord;
    BOOLEAN HeapLocked = FALSE;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualBlock = NULL;
    PHEAP_ENTRY_EXTRA Extra;
    NTSTATUS Status;

    /* Force flags */
    Flags |= Heap->ForceFlags;

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugAllocateHeap(Heap, Flags, Size);

    /* Check for the maximum size */
    if (Size >= 0x80000000)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_NO_MEMORY);
        DPRINT1("HEAP: Allocation failed!\n");
        return NULL;
    }

    if (Flags & (HEAP_CREATE_ENABLE_TRACING |
                 HEAP_CREATE_ALIGN_16))
    {
        DPRINT1("HEAP: RtlAllocateHeap is called with unsupported flags %x, ignoring\n", Flags);
    }

    //DPRINT("RtlAllocateHeap(%p %x %x)\n", Heap, Flags, Size);

    /* Calculate allocation size and index */
    if (Size)
        AllocationSize = Size;
    else
        AllocationSize = 1;
    AllocationSize = (AllocationSize + Heap->AlignRound) & Heap->AlignMask;

    /* Add extra flags in case of settable user value feature is requested,
       or there is a tag (small or normal) or there is a request to
       capture stack backtraces */
    if ((Flags & HEAP_EXTRA_FLAGS_MASK) ||
        Heap->PseudoTagEntries)
    {
        /* Add flag which means that the entry will have extra stuff attached */
        EntryFlags |= HEAP_ENTRY_EXTRA_PRESENT;

        /* Account for extra stuff size */
        AllocationSize += sizeof(HEAP_ENTRY_EXTRA);
    }

    /* Add settable user flags, if any */
    EntryFlags |= (Flags & HEAP_SETTABLE_USER_FLAGS) >> 4;

    Index = AllocationSize >>  HEAP_ENTRY_SHIFT;

    /* Acquire the lock if necessary */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
    }

    /* Depending on the size, the allocation is going to be done from dedicated,
       non-dedicated lists or a virtual block of memory */
    if (Index < HEAP_FREELISTS)
    {
        FreeListHead = &Heap->FreeLists[Index];

        if (!IsListEmpty(FreeListHead))
        {
            /* There is a free entry in this list */
            FreeBlock = CONTAINING_RECORD(FreeListHead->Blink,
                                          HEAP_FREE_ENTRY,
                                          FreeList);

            /* Save flags and remove the free entry */
            FreeFlags = FreeBlock->Flags;
            RtlpRemoveFreeBlock(Heap, FreeBlock, TRUE, FALSE);

            /* Update the total free size of the heap */
            Heap->TotalFreeSize -= Index;

            /* Initialize this block */
            InUseEntry = (PHEAP_ENTRY)FreeBlock;
            InUseEntry->Flags = EntryFlags | (FreeFlags & HEAP_ENTRY_LAST_ENTRY);
            InUseEntry->UnusedBytes = (UCHAR)(AllocationSize - Size);
            InUseEntry->SmallTagIndex = 0;
        }
        else
        {
            /* Find smallest free block which this request could fit in */
            InUseIndex = Index >> 5;
            FreeListsInUse = &Heap->u.FreeListsInUseUlong[InUseIndex];

            /* This bit magic disables all sizes which are less than the requested allocation size */
            FreeListsInUseUlong = *FreeListsInUse++ & ~((1 << ((ULONG)Index & 0x1f)) - 1);

            /* If size is definitily more than our lists - go directly to the non-dedicated one */
            if (InUseIndex > 3)
                return RtlpAllocateNonDedicated(Heap, Flags, Size, AllocationSize, Index, HeapLocked);

            /* Go through the list */
            for (i = InUseIndex; i < 4; i++)
            {
                if (FreeListsInUseUlong)
                {
                    FreeListHead = &Heap->FreeLists[i * 32];
                    break;
                }

                if (i < 3) FreeListsInUseUlong = *FreeListsInUse++;
            }

            /* Nothing found, search in the non-dedicated list */
            if (i == 4)
                return RtlpAllocateNonDedicated(Heap, Flags, Size, AllocationSize, Index, HeapLocked);

            /* That list is found, now calculate exact block  */
            FreeListHead += RtlpFindLeastSetBit(FreeListsInUseUlong);

            /* Take this entry and remove it from the list of free blocks */
            FreeBlock = CONTAINING_RECORD(FreeListHead->Blink,
                                          HEAP_FREE_ENTRY,
                                          FreeList);
            RtlpRemoveFreeBlock(Heap, FreeBlock, TRUE, FALSE);

            /* Split it */
            InUseEntry = RtlpSplitEntry(Heap, Flags, FreeBlock, AllocationSize, Index, Size);
        }

        /* Release the lock */
        if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

        /* Zero memory if that was requested */
        if (Flags & HEAP_ZERO_MEMORY)
            RtlZeroMemory(InUseEntry + 1, Size);
        else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
        {
            /* Fill this block with a special pattern */
            RtlFillMemoryUlong(InUseEntry + 1, Size & ~0x3, ARENA_INUSE_FILLER);
        }

        /* Fill tail of the block with a special pattern too if requested */
        if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED)
        {
            RtlFillMemory((PCHAR)(InUseEntry + 1) + Size, sizeof(HEAP_ENTRY), HEAP_TAIL_FILL);
            InUseEntry->Flags |= HEAP_ENTRY_FILL_PATTERN;
        }

        /* Prepare extra if it's present */
        if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
        {
            Extra = RtlpGetExtraStuffPointer(InUseEntry);
            RtlZeroMemory(Extra, sizeof(HEAP_ENTRY_EXTRA));

            // TODO: Tagging
        }

        /* User data starts right after the entry's header */
        return InUseEntry + 1;
    }
    else if (Index <= Heap->VirtualMemoryThreshold)
    {
        /* The block is too large for dedicated lists, but fine for a non-dedicated one */
        return RtlpAllocateNonDedicated(Heap, Flags, Size, AllocationSize, Index, HeapLocked);
    }
    else if (Heap->Flags & HEAP_GROWABLE)
    {
        /* We've got a very big allocation request, satisfy it by directly allocating virtual memory */
        AllocationSize += sizeof(HEAP_VIRTUAL_ALLOC_ENTRY) - sizeof(HEAP_ENTRY);

        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                         (PVOID *)&VirtualBlock,
                                         0,
                                         &AllocationSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);

        if (!NT_SUCCESS(Status))
        {
            // Set STATUS!
            /* Release the lock */
            if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);
            DPRINT1("HEAP: Allocation failed!\n");
            return NULL;
        }

        /* Initialize the newly allocated block */
        VirtualBlock->BusyBlock.Size = (USHORT)(AllocationSize - Size);
        ASSERT(VirtualBlock->BusyBlock.Size >= sizeof(HEAP_VIRTUAL_ALLOC_ENTRY));
        VirtualBlock->BusyBlock.Flags = EntryFlags | HEAP_ENTRY_VIRTUAL_ALLOC | HEAP_ENTRY_EXTRA_PRESENT;
        VirtualBlock->CommitSize = AllocationSize;
        VirtualBlock->ReserveSize = AllocationSize;

        /* Insert it into the list of virtual allocations */
        InsertTailList(&Heap->VirtualAllocdBlocks, &VirtualBlock->Entry);

        /* Release the lock */
        if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

        /* Return pointer to user data */
        return VirtualBlock + 1;
    }

    /* Generate an exception */
    if (Flags & HEAP_GENERATE_EXCEPTIONS)
    {
        ExceptionRecord.ExceptionCode = STATUS_NO_MEMORY;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.NumberParameters = 1;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[0] = AllocationSize;

        RtlRaiseException(&ExceptionRecord);
    }

    RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_BUFFER_TOO_SMALL);

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);
    DPRINT1("HEAP: Allocation failed!\n");
    return NULL;
}


/***********************************************************************
 *           HeapFree   (KERNEL32.338)
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI RtlFreeHeap(
   HANDLE HeapPtr, /* [in] Handle of heap */
   ULONG Flags,   /* [in] Heap freeing flags */
   PVOID Ptr     /* [in] Address of memory to free */
)
{
    PHEAP Heap;
    PHEAP_ENTRY HeapEntry;
    USHORT TagIndex = 0;
    SIZE_T BlockSize;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualEntry;
    BOOLEAN Locked = FALSE;
    NTSTATUS Status;

    /* Freeing NULL pointer is a legal operation */
    if (!Ptr) return TRUE;

    /* Get pointer to the heap and force flags */
    Heap = (PHEAP)HeapPtr;
    Flags |= Heap->ForceFlags;

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugFreeHeap(Heap, Flags, Ptr);

    /* Lock if necessary */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        Locked = TRUE;
    }

    /* Get pointer to the heap entry */
    HeapEntry = (PHEAP_ENTRY)Ptr - 1;

    /* Check this entry, fail if it's invalid */
    if (!(HeapEntry->Flags & HEAP_ENTRY_BUSY) ||
        (((ULONG_PTR)Ptr & 0x7) != 0) ||
        (HeapEntry->SegmentOffset >= HEAP_SEGMENTS))
    {
        /* This is an invalid block */
        DPRINT1("HEAP: Trying to free an invalid address %p!\n", Ptr);
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);

        /* Release the heap lock */
        if (Locked) RtlLeaveHeapLock(Heap->LockVariable);
        return FALSE;
    }

    if (HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
    {
        /* Big allocation */
        VirtualEntry = CONTAINING_RECORD(HeapEntry, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock);

        /* Remove it from the list */
        RemoveEntryList(&VirtualEntry->Entry);

        // TODO: Tagging

        BlockSize = 0;
        Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                     (PVOID *)&VirtualEntry,
                                     &BlockSize,
                                     MEM_RELEASE);

        if (!NT_SUCCESS(Status))
        {
            DPRINT1("HEAP: Failed releasing memory with Status 0x%08X. Heap %p, ptr %p, base address %p\n",
                Status, Heap, Ptr, VirtualEntry);
            RtlSetLastWin32ErrorAndNtStatusFromNtStatus(Status);
        }
    }
    else
    {
        /* Normal allocation */
        BlockSize = HeapEntry->Size;

        // TODO: Tagging

        /* Coalesce in kernel mode, and in usermode if it's not disabled */
        if (RtlpGetMode() == KernelMode ||
            (RtlpGetMode() == UserMode && !(Heap->Flags & HEAP_DISABLE_COALESCE_ON_FREE)))
        {
            HeapEntry = (PHEAP_ENTRY)RtlpCoalesceFreeBlocks(Heap,
                                                           (PHEAP_FREE_ENTRY)HeapEntry,
                                                           &BlockSize,
                                                           FALSE);
        }

        /* If there is no need to decommit the block - put it into a free list */
        if (BlockSize < Heap->DeCommitFreeBlockThreshold ||
            (Heap->TotalFreeSize + BlockSize < Heap->DeCommitTotalFreeThreshold))
        {
            /* Check if it needs to go to a 0 list */
            if (BlockSize > HEAP_MAX_BLOCK_SIZE)
            {
                /* General-purpose 0 list */
                RtlpInsertFreeBlock(Heap, (PHEAP_FREE_ENTRY)HeapEntry, BlockSize);
            }
            else
            {
                /* Usual free list */
                RtlpInsertFreeBlockHelper(Heap, (PHEAP_FREE_ENTRY)HeapEntry, BlockSize, FALSE);

                /* Assert sizes are consistent */
                if (!(HeapEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
                {
                    ASSERT((HeapEntry + BlockSize)->PreviousSize == BlockSize);
                }

                /* Increase the free size */
                Heap->TotalFreeSize += BlockSize;
            }


            if (RtlpGetMode() == UserMode &&
                TagIndex != 0)
            {
                // FIXME: Tagging
                UNIMPLEMENTED;
            }
        }
        else
        {
            /* Decommit this block */
            RtlpDeCommitFreeBlock(Heap, (PHEAP_FREE_ENTRY)HeapEntry, BlockSize);
        }
    }

    /* Release the heap lock */
    if (Locked) RtlLeaveHeapLock(Heap->LockVariable);

    return TRUE;
}

BOOLEAN NTAPI
RtlpGrowBlockInPlace (IN PHEAP Heap,
                      IN ULONG Flags,
                      IN PHEAP_ENTRY InUseEntry,
                      IN SIZE_T Size,
                      IN SIZE_T Index)
{
    UCHAR EntryFlags, RememberFlags;
    PHEAP_FREE_ENTRY FreeEntry, UnusedEntry, FollowingEntry;
    SIZE_T FreeSize, PrevSize, TailPart, AddedSize = 0;
    PHEAP_ENTRY_EXTRA OldExtra, NewExtra;

    /* We can't grow beyond specified threshold */
    if (Index > Heap->VirtualMemoryThreshold)
        return FALSE;

    /* Get entry flags */
    EntryFlags = InUseEntry->Flags;

    /* Get the next free entry */
    FreeEntry = (PHEAP_FREE_ENTRY)(InUseEntry + InUseEntry->Size);

    if (EntryFlags & HEAP_ENTRY_LAST_ENTRY)
    {
        /* There is no next block, just uncommitted space. Calculate how much is needed */
        FreeSize = (Index - InUseEntry->Size) << HEAP_ENTRY_SHIFT;
        FreeSize = ROUND_UP(FreeSize, PAGE_SIZE);

        /* Find and commit those pages */
        FreeEntry = RtlpFindAndCommitPages(Heap,
                                           Heap->Segments[InUseEntry->SegmentOffset],
                                           &FreeSize,
                                           FreeEntry);

        /* Fail if it failed... */
        if (!FreeEntry) return FALSE;

        /* It was successful, perform coalescing */
        FreeSize = FreeSize >> HEAP_ENTRY_SHIFT;
        FreeEntry = RtlpCoalesceFreeBlocks(Heap, FreeEntry, &FreeSize, FALSE);

        /* Check if it's enough */
        if (FreeSize + InUseEntry->Size < Index)
        {
            /* Still not enough */
            RtlpInsertFreeBlock(Heap, FreeEntry, FreeSize);
            Heap->TotalFreeSize += FreeSize;
            return FALSE;
        }

        /* Remember flags of this free entry */
        RememberFlags = FreeEntry->Flags;

        /* Sum up sizes */
        FreeSize += InUseEntry->Size;
    }
    else
    {
        /* The next block indeed exists. Check if it's free or in use */
        if (FreeEntry->Flags & HEAP_ENTRY_BUSY) return FALSE;

        /* Next entry is free, check if it can fit the block we need */
        FreeSize = InUseEntry->Size + FreeEntry->Size;
        if (FreeSize < Index) return FALSE;

        /* Remember flags of this free entry */
        RememberFlags = FreeEntry->Flags;

        /* Remove this block from the free list */
        RtlpRemoveFreeBlock(Heap, FreeEntry, FALSE, FALSE);
        Heap->TotalFreeSize -= FreeEntry->Size;
    }

    PrevSize = (InUseEntry->Size << HEAP_ENTRY_SHIFT) - InUseEntry->UnusedBytes;
    FreeSize -= Index;

    /* Don't produce too small blocks */
    if (FreeSize <= 2)
    {
        Index += FreeSize;
        FreeSize = 0;
    }

    /* Process extra stuff */
    if (RememberFlags & HEAP_ENTRY_EXTRA_PRESENT)
    {
        /* Calculate pointers */
        OldExtra = (PHEAP_ENTRY_EXTRA)(InUseEntry + InUseEntry->Size - 1);
        NewExtra = (PHEAP_ENTRY_EXTRA)(InUseEntry + Index - 1);

        /* Copy contents */
        *NewExtra = *OldExtra;

        // FIXME Tagging
    }

    /* Update sizes */
    InUseEntry->Size = (USHORT)Index;
    InUseEntry->UnusedBytes = (UCHAR)((Index << HEAP_ENTRY_SHIFT) - Size);

    /* Check if there is a free space remaining after merging those blocks */
    if (!FreeSize)
    {
        /* Update flags and sizes */
        InUseEntry->Flags |= RememberFlags & HEAP_ENTRY_LAST_ENTRY;

        /* Either update previous size of the next entry or mark it as a last
           entry in the segment*/
        if (!(RememberFlags & HEAP_ENTRY_LAST_ENTRY))
            (InUseEntry + InUseEntry->Size)->PreviousSize = InUseEntry->Size;
    }
    else
    {
        /* Complex case, we need to split the block to give unused free space
           back to the heap */
        UnusedEntry = (PHEAP_FREE_ENTRY)(InUseEntry + Index);
        UnusedEntry->PreviousSize = (USHORT)Index;
        UnusedEntry->SegmentOffset = InUseEntry->SegmentOffset;

        /* Update the following block or set the last entry in the segment */
        if (RememberFlags & HEAP_ENTRY_LAST_ENTRY)
        {
            /* Set flags and size */
            UnusedEntry->Flags = RememberFlags;
            UnusedEntry->Size = (USHORT)FreeSize;

            /* Insert it to the heap and update total size  */
            RtlpInsertFreeBlockHelper(Heap, UnusedEntry, FreeSize, FALSE);
            Heap->TotalFreeSize += FreeSize;
        }
        else
        {
            /* There is a block after this one  */
            FollowingEntry = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)UnusedEntry + FreeSize);

            if (FollowingEntry->Flags & HEAP_ENTRY_BUSY)
            {
                /* Update flags and set size of the unused space entry */
                UnusedEntry->Flags = RememberFlags & (~HEAP_ENTRY_LAST_ENTRY);
                UnusedEntry->Size = (USHORT)FreeSize;

                /* Update previous size of the following entry */
                FollowingEntry->PreviousSize = (USHORT)FreeSize;

                /* Insert it to the heap and update total free size */
                RtlpInsertFreeBlockHelper(Heap, UnusedEntry, FreeSize, FALSE);
                Heap->TotalFreeSize += FreeSize;
            }
            else
            {
                /* That following entry is also free, what a fortune! */
                RememberFlags = FollowingEntry->Flags;

                /* Remove it */
                RtlpRemoveFreeBlock(Heap, FollowingEntry, FALSE, FALSE);
                Heap->TotalFreeSize -= FollowingEntry->Size;

                /* And make up a new combined block */
                FreeSize += FollowingEntry->Size;
                UnusedEntry->Flags = RememberFlags;

                /* Check where to put it */
                if (FreeSize <= HEAP_MAX_BLOCK_SIZE)
                {
                    /* Fine for a dedicated list */
                    UnusedEntry->Size = (USHORT)FreeSize;

                    if (!(RememberFlags & HEAP_ENTRY_LAST_ENTRY))
                        ((PHEAP_ENTRY)UnusedEntry + FreeSize)->PreviousSize = (USHORT)FreeSize;

                    /* Insert it back and update total size */
                    RtlpInsertFreeBlockHelper(Heap, UnusedEntry, FreeSize, FALSE);
                    Heap->TotalFreeSize += FreeSize;
                }
                else
                {
                    /* The block is very large, leave all the hassle to the insertion routine */
                    RtlpInsertFreeBlock(Heap, UnusedEntry, FreeSize);
                }
            }
        }
    }

    /* Properly "zero out" (and fill!) the space */
    if (Flags & HEAP_ZERO_MEMORY)
    {
        RtlZeroMemory((PCHAR)(InUseEntry + 1) + PrevSize, Size - PrevSize);
    }
    else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
    {
        /* Calculate tail part which we need to fill */
        TailPart = PrevSize & (sizeof(ULONG) - 1);

        /* "Invert" it as usual */
        if (TailPart) TailPart = 4 - TailPart;

        if (Size > (PrevSize + TailPart))
            AddedSize = (Size - (PrevSize + TailPart)) & ~(sizeof(ULONG) - 1);

        if (AddedSize)
        {
            RtlFillMemoryUlong((PCHAR)(InUseEntry + 1) + PrevSize + TailPart,
                               AddedSize,
                               ARENA_INUSE_FILLER);
        }
    }

    /* Fill the new tail */
    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED)
    {
        RtlFillMemory((PCHAR)(InUseEntry + 1) + Size,
                      HEAP_ENTRY_SIZE,
                      HEAP_TAIL_FILL);
    }

    /* Copy user settable flags */
    InUseEntry->Flags &= ~HEAP_ENTRY_SETTABLE_FLAGS;
    InUseEntry->Flags |= ((Flags & HEAP_SETTABLE_USER_FLAGS) >> 4);

    /* Return success */
    return TRUE;
}

PHEAP_ENTRY_EXTRA NTAPI
RtlpGetExtraStuffPointer(PHEAP_ENTRY HeapEntry)
{
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualEntry;

    /* Check if it's a big block */
    if (HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
    {
        VirtualEntry = CONTAINING_RECORD(HeapEntry, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock);

        /* Return a pointer to the extra stuff*/
        return &VirtualEntry->ExtraStuff;
    }
    else
    {
        /* This is a usual entry, which means extra stuff follows this block */
        return (PHEAP_ENTRY_EXTRA)(HeapEntry + HeapEntry->Size - 1);
    }
}


/***********************************************************************
 *           RtlReAllocateHeap
 * PARAMS
 *   Heap   [in] Handle of heap block
 *   Flags    [in] Heap reallocation flags
 *   Ptr,    [in] Address of memory to reallocate
 *   Size     [in] Number of bytes to reallocate
 *
 * RETURNS
 * Pointer to reallocated memory block
 * NULL: Failure
 * 0x7d030f60--invalid flags in RtlHeapAllocate
 * @implemented
 */
PVOID NTAPI
RtlReAllocateHeap(HANDLE HeapPtr,
                  ULONG Flags,
                  PVOID Ptr,
                  SIZE_T Size)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    PHEAP_ENTRY InUseEntry, NewInUseEntry;
    PHEAP_ENTRY_EXTRA OldExtra, NewExtra;
    SIZE_T AllocationSize, FreeSize, DecommitSize;
    BOOLEAN HeapLocked = FALSE;
    PVOID NewBaseAddress;
    PHEAP_FREE_ENTRY SplitBlock, SplitBlock2;
    SIZE_T OldSize, Index, OldIndex;
    UCHAR FreeFlags;
    NTSTATUS Status;
    PVOID DecommitBase;
    SIZE_T RemainderBytes, ExtraSize;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;
    EXCEPTION_RECORD ExceptionRecord;

    /* Return success in case of a null pointer */
    if (!Ptr)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_SUCCESS);
        return NULL;
    }

    /* Force heap flags */
    Flags |= Heap->ForceFlags;

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugReAllocateHeap(Heap, Flags, Ptr, Size);

    /* Make sure size is valid */
    if (Size >= 0x80000000)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_NO_MEMORY);
        return NULL;
    }

    /* Calculate allocation size and index */
    if (Size)
        AllocationSize = Size;
    else
        AllocationSize = 1;
    AllocationSize = (AllocationSize + Heap->AlignRound) & Heap->AlignMask;

    /* Add up extra stuff, if it is present anywhere */
    if (((((PHEAP_ENTRY)Ptr)-1)->Flags & HEAP_ENTRY_EXTRA_PRESENT) ||
        (Flags & HEAP_EXTRA_FLAGS_MASK) ||
        Heap->PseudoTagEntries)
    {
        AllocationSize += sizeof(HEAP_ENTRY_EXTRA);
    }

    /* Acquire the lock if necessary */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
        Flags &= ~HEAP_NO_SERIALIZE;
    }

    /* Get the pointer to the in-use entry */
    InUseEntry = (PHEAP_ENTRY)Ptr - 1;

    /* If that entry is not really in-use, we have a problem */
    if (!(InUseEntry->Flags & HEAP_ENTRY_BUSY))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);

        /* Release the lock and return */
        if (HeapLocked)
            RtlLeaveHeapLock(Heap->LockVariable);
        return Ptr;
    }

    if (InUseEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
    {
        /* This is a virtually allocated block. Get its size */
        OldSize = RtlpGetSizeOfBigBlock(InUseEntry);

        /* Convert it to an index */
        OldIndex = (OldSize + InUseEntry->Size) >> HEAP_ENTRY_SHIFT;

        /* Calculate new allocation size and round it to the page size */
        AllocationSize += FIELD_OFFSET(HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock);
        AllocationSize = ROUND_UP(AllocationSize, PAGE_SIZE);
    }
    else
    {
        /* Usual entry */
        OldIndex = InUseEntry->Size;

        OldSize = (OldIndex << HEAP_ENTRY_SHIFT) - InUseEntry->UnusedBytes;
    }

    /* Calculate new index */
    Index = AllocationSize >> HEAP_ENTRY_SHIFT;

    /* Check for 4 different scenarios (old size, new size, old index, new index) */
    if (Index <= OldIndex)
    {
        /* Difference must be greater than 1, adjust if it's not so */
        if (Index + 1 == OldIndex)
        {
            Index++;
            AllocationSize += sizeof(HEAP_ENTRY);
        }

        /* Calculate new size */
        if (InUseEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
        {
            /* Simple in case of a virtual alloc - just an unused size */
            InUseEntry->Size = (USHORT)(AllocationSize - Size);
            ASSERT(InUseEntry->Size >= sizeof(HEAP_VIRTUAL_ALLOC_ENTRY));
        }
        else if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
        {
            /* There is extra stuff, take it into account */
            OldExtra = (PHEAP_ENTRY_EXTRA)(InUseEntry + InUseEntry->Size - 1);
            NewExtra = (PHEAP_ENTRY_EXTRA)(InUseEntry + Index - 1);
            *NewExtra = *OldExtra;

            // FIXME Tagging, TagIndex

            /* Update unused bytes count */
            InUseEntry->UnusedBytes = (UCHAR)(AllocationSize - Size);
        }
        else
        {
            // FIXME Tagging, SmallTagIndex
            InUseEntry->UnusedBytes = (UCHAR)(AllocationSize - Size);
        }

        /* If new size is bigger than the old size */
        if (Size > OldSize)
        {
            /* Zero out that additional space if required */
            if (Flags & HEAP_ZERO_MEMORY)
            {
                RtlZeroMemory((PCHAR)Ptr + OldSize, Size - OldSize);
            }
            else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED)
            {
                /* Fill it on free if required */
                RemainderBytes = OldSize & (sizeof(ULONG) - 1);

                if (RemainderBytes)
                    RemainderBytes = 4 - RemainderBytes;

                if (Size > (OldSize + RemainderBytes))
                {
                    /* Calculate actual amount of extra bytes to fill */
                    ExtraSize = (Size - (OldSize + RemainderBytes)) & ~(sizeof(ULONG) - 1);

                    /* Fill them if there are any */
                    if (ExtraSize != 0)
                    {
                        RtlFillMemoryUlong((PCHAR)(InUseEntry + 1) + OldSize + RemainderBytes,
                                           ExtraSize,
                                           ARENA_INUSE_FILLER);
                    }
                }
            }
        }

        /* Fill tail of the heap entry if required */
        if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED)
        {
            RtlFillMemory((PCHAR)(InUseEntry + 1) + Size,
                          HEAP_ENTRY_SIZE,
                          HEAP_TAIL_FILL);
        }

        /* Check if the difference is significant or not */
        if (Index != OldIndex)
        {
            /* Save flags */
            FreeFlags = InUseEntry->Flags & ~HEAP_ENTRY_BUSY;

            if (FreeFlags & HEAP_ENTRY_VIRTUAL_ALLOC)
            {
                /* This is a virtual block allocation */
                VirtualAllocBlock = CONTAINING_RECORD(InUseEntry, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock);

                // FIXME Tagging!

                DecommitBase = (PCHAR)VirtualAllocBlock + AllocationSize;
                DecommitSize = (OldIndex << HEAP_ENTRY_SHIFT) - AllocationSize;

                /* Release the memory */
                Status = ZwFreeVirtualMemory(NtCurrentProcess(),
                                             (PVOID *)&DecommitBase,
                                             &DecommitSize,
                                             MEM_RELEASE);

                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("HEAP: Unable to release memory (pointer %p, size 0x%x), Status %08x\n", DecommitBase, DecommitSize, Status);
                }
                else
                {
                    /* Otherwise reduce the commit size */
                    VirtualAllocBlock->CommitSize -= DecommitSize;
                }
            }
            else
            {
                /* Reduce size of the block and possibly split it */
                SplitBlock = (PHEAP_FREE_ENTRY)(InUseEntry + Index);

                /* Initialize this entry */
                SplitBlock->Flags = FreeFlags;
                SplitBlock->PreviousSize = (USHORT)Index;
                SplitBlock->SegmentOffset = InUseEntry->SegmentOffset;

                /* Remember free size */
                FreeSize = InUseEntry->Size - Index;

                /* Set new size */
                InUseEntry->Size = (USHORT)Index;
                InUseEntry->Flags &= ~HEAP_ENTRY_LAST_ENTRY;

                /* Is that the last entry */
                if (FreeFlags & HEAP_ENTRY_LAST_ENTRY)
                {
                    /* Set its size and insert it to the list */
                    SplitBlock->Size = (USHORT)FreeSize;
                    RtlpInsertFreeBlockHelper(Heap, SplitBlock, FreeSize, FALSE);

                    /* Update total free size */
                    Heap->TotalFreeSize += FreeSize;
                }
                else
                {
                    /* Get the block after that one */
                    SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);

                    if (SplitBlock2->Flags & HEAP_ENTRY_BUSY)
                    {
                        /* It's in use, add it here*/
                        SplitBlock->Size = (USHORT)FreeSize;

                        /* Update previous size of the next entry */
                        ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;

                        /* Insert it to the list */
                        RtlpInsertFreeBlockHelper(Heap, SplitBlock, FreeSize, FALSE);

                        /* Update total size */
                        Heap->TotalFreeSize += FreeSize;
                    }
                    else
                    {
                        /* Next entry is free, so merge with it */
                        SplitBlock->Flags = SplitBlock2->Flags;

                        /* Remove it, update total size */
                        RtlpRemoveFreeBlock(Heap, SplitBlock2, FALSE, FALSE);
                        Heap->TotalFreeSize -= SplitBlock2->Size;

                        /* Calculate total free size */
                        FreeSize += SplitBlock2->Size;

                        if (FreeSize <= HEAP_MAX_BLOCK_SIZE)
                        {
                            SplitBlock->Size = (USHORT)FreeSize;

                            if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY))
                            {
                                /* Update previous size of the next entry */
                                ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                            }

                            /* Insert the new one back and update total size */
                            RtlpInsertFreeBlockHelper(Heap, SplitBlock, FreeSize, FALSE);
                            Heap->TotalFreeSize += FreeSize;
                        }
                        else
                        {
                            /* Just add it */
                            RtlpInsertFreeBlock(Heap, SplitBlock, FreeSize);
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* We're growing the block */
        if ((InUseEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) ||
            !RtlpGrowBlockInPlace(Heap, Flags, InUseEntry, Size, Index))
        {
            /* Growing in place failed, so growing out of place */
            if (Flags & HEAP_REALLOC_IN_PLACE_ONLY)
            {
                DPRINT1("Realloc in place failed, but it was the only option\n");
                Ptr = NULL;
            }
            else
            {
                /* Clear tag bits */
                Flags &= ~HEAP_TAG_MASK;

                /* Process extra stuff */
                if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
                {
                    /* Preserve user settable flags */
                    Flags &= ~HEAP_SETTABLE_USER_FLAGS;

                    Flags |= HEAP_SETTABLE_USER_VALUE | ((InUseEntry->Flags & HEAP_ENTRY_SETTABLE_FLAGS) << 4);

                    /* Get pointer to the old extra data */
                    OldExtra = RtlpGetExtraStuffPointer(InUseEntry);

                    /* Save tag index if it was set */
                    if (OldExtra->TagIndex &&
                        !(OldExtra->TagIndex & HEAP_PSEUDO_TAG_FLAG))
                    {
                        Flags |= OldExtra->TagIndex << HEAP_TAG_SHIFT;
                    }
                }
                else if (InUseEntry->SmallTagIndex)
                {
                    /* Take small tag index into account */
                    Flags |= InUseEntry->SmallTagIndex << HEAP_TAG_SHIFT;
                }

                /* Allocate new block from the heap */
                NewBaseAddress = RtlAllocateHeap(HeapPtr,
                                                 Flags & ~HEAP_ZERO_MEMORY,
                                                 Size);

                /* Proceed if it didn't fail */
                if (NewBaseAddress)
                {
                    /* Get new entry pointer */
                    NewInUseEntry = (PHEAP_ENTRY)NewBaseAddress - 1;

                    /* Process extra stuff if it exists */
                    if (NewInUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
                    {
                        NewExtra = RtlpGetExtraStuffPointer(NewInUseEntry);

                        if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
                        {
                            OldExtra = RtlpGetExtraStuffPointer(InUseEntry);
                            NewExtra->Settable = OldExtra->Settable;
                        }
                        else
                        {
                            RtlZeroMemory(NewExtra, sizeof(*NewExtra));
                        }
                    }

                    /* Copy actual user bits */
                    if (Size < OldSize)
                        RtlMoveMemory(NewBaseAddress, Ptr, Size);
                    else
                        RtlMoveMemory(NewBaseAddress, Ptr, OldSize);

                    /* Zero remaining part if required */
                    if (Size > OldSize &&
                        (Flags & HEAP_ZERO_MEMORY))
                    {
                        RtlZeroMemory((PCHAR)NewBaseAddress + OldSize, Size - OldSize);
                    }

                    /* Free the old block */
                    RtlFreeHeap(HeapPtr, Flags, Ptr);
                }

                Ptr = NewBaseAddress;
            }
        }
    }

    /* Did resizing fail? */
    if (!Ptr && (Flags & HEAP_GENERATE_EXCEPTIONS))
    {
        /* Generate an exception if required */
        ExceptionRecord.ExceptionCode = STATUS_NO_MEMORY;
        ExceptionRecord.ExceptionRecord = NULL;
        ExceptionRecord.NumberParameters = 1;
        ExceptionRecord.ExceptionFlags = 0;
        ExceptionRecord.ExceptionInformation[0] = AllocationSize;

        RtlRaiseException(&ExceptionRecord);
    }

    /* Release the heap lock if it was acquired */
    if (HeapLocked)
        RtlLeaveHeapLock(Heap->LockVariable);

    return Ptr;
}


/***********************************************************************
 *           RtlCompactHeap
 *
 * @unimplemented
 */
ULONG NTAPI
RtlCompactHeap(HANDLE Heap,
		ULONG Flags)
{
   UNIMPLEMENTED;
   return 0;
}


/***********************************************************************
 *           RtlLockHeap
 * Attempts to acquire the critical section object for a specified heap.
 *
 * PARAMS
 *   Heap  [in] Handle of heap to lock for exclusive access
 *
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI
RtlLockHeap(IN HANDLE HeapPtr)
{
    PHEAP Heap = (PHEAP)HeapPtr;

    // FIXME Check for special heap

    /* Check if it's really a heap */
    if (Heap->Signature != HEAP_SIGNATURE) return FALSE;

    /* Lock if it's lockable */
    if (!(Heap->Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
    }

    return TRUE;
}


/***********************************************************************
 *           RtlUnlockHeap
 * Releases ownership of the critical section object.
 *
 * PARAMS
 *   Heap  [in] Handle to the heap to unlock
 *
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI
RtlUnlockHeap(HANDLE HeapPtr)
{
    PHEAP Heap = (PHEAP)HeapPtr;

    // FIXME Check for special heap

    /* Check if it's really a heap */
    if (Heap->Signature != HEAP_SIGNATURE) return FALSE;

    /* Unlock if it's lockable */
    if (!(Heap->Flags & HEAP_NO_SERIALIZE))
    {
        RtlLeaveHeapLock(Heap->LockVariable);
    }

    return TRUE;
}


/***********************************************************************
 *           RtlSizeHeap
 * PARAMS
 *   Heap  [in] Handle of heap
 *   Flags   [in] Heap size control flags
 *   Ptr     [in] Address of memory to return size for
 *
 * RETURNS
 * Size in bytes of allocated memory
 * 0xffffffff: Failure
 *
 * @implemented
 */
SIZE_T NTAPI
RtlSizeHeap(
   HANDLE HeapPtr,
   ULONG Flags,
   PVOID Ptr
)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    PHEAP_ENTRY HeapEntry;
    SIZE_T EntrySize;

    // FIXME This is a hack around missing SEH support!
    if (!Heap)
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_HANDLE);
        return (SIZE_T)-1;
    }

    /* Force flags */
    Flags |= Heap->ForceFlags;

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugSizeHeap(Heap, Flags, Ptr);

    /* Get the heap entry pointer */
    HeapEntry = (PHEAP_ENTRY)Ptr - 1;

    /* Return -1 if that entry is free */
    if (!(HeapEntry->Flags & HEAP_ENTRY_BUSY))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);
        return (SIZE_T)-1;
    }

    /* Get size of this block depending if it's a usual or a big one */
    if (HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
    {
        EntrySize = RtlpGetSizeOfBigBlock(HeapEntry);
    }
    else
    {
        /* Calculate it */
        EntrySize = (HeapEntry->Size << HEAP_ENTRY_SHIFT) - HeapEntry->UnusedBytes;
    }

    /* Return calculated size */
    return EntrySize;
}

BOOLEAN NTAPI
RtlpCheckInUsePattern(PHEAP_ENTRY HeapEntry)
{
    SIZE_T Size, Result;
    PCHAR TailPart;

    /* Calculate size */
    if (HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
        Size = RtlpGetSizeOfBigBlock(HeapEntry);
    else
        Size = (HeapEntry->Size << HEAP_ENTRY_SHIFT) - HeapEntry->UnusedBytes;

    /* Calculate pointer to the tail part of the block */
    TailPart = (PCHAR)(HeapEntry + 1) + Size;

    /* Compare tail pattern */
    Result = RtlCompareMemory(TailPart,
                              FillPattern,
                              HEAP_ENTRY_SIZE);

    if (Result != HEAP_ENTRY_SIZE)
    {
        DPRINT1("HEAP: Heap entry (size %x) %p tail is modified at %p\n", Size, HeapEntry, TailPart + Result);
        return FALSE;
    }

    /* All is fine */
    return TRUE;
}

BOOLEAN NTAPI
RtlpValidateHeapHeaders(
    PHEAP Heap,
    BOOLEAN Recalculate)
{
    // We skip header validation for now
    return TRUE;
}

BOOLEAN NTAPI
RtlpValidateHeapEntry(
    PHEAP Heap,
    PHEAP_ENTRY HeapEntry)
{
    BOOLEAN BigAllocation, EntryFound = FALSE;
    PHEAP_SEGMENT Segment;
    ULONG SegmentOffset;

    /* Perform various consistency checks of this entry */
    if (!HeapEntry) goto invalid_entry;
    if ((ULONG_PTR)HeapEntry & (HEAP_ENTRY_SIZE - 1)) goto invalid_entry;
    if (!(HeapEntry->Flags & HEAP_ENTRY_BUSY)) goto invalid_entry;

    BigAllocation = HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC;
    Segment = Heap->Segments[HeapEntry->SegmentOffset];

    if (BigAllocation &&
        (((ULONG_PTR)HeapEntry & (PAGE_SIZE - 1)) != FIELD_OFFSET(HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock)))
         goto invalid_entry;

    if (!BigAllocation && (HeapEntry->SegmentOffset >= HEAP_SEGMENTS ||
        !Segment ||
        HeapEntry < Segment->FirstEntry ||
        HeapEntry >= Segment->LastValidEntry))
        goto invalid_entry;

    if ((HeapEntry->Flags & HEAP_ENTRY_FILL_PATTERN) &&
        !RtlpCheckInUsePattern(HeapEntry))
        goto invalid_entry;

    /* Checks are done, if this is a virtual entry, that's all */
    if (HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) return TRUE;

    /* Go through segments and check if this entry fits into any of them */
    for (SegmentOffset = 0; SegmentOffset < HEAP_SEGMENTS; SegmentOffset++)
    {
        Segment = Heap->Segments[SegmentOffset];
        if (!Segment) continue;

        if ((HeapEntry >= Segment->FirstEntry) &&
            (HeapEntry < Segment->LastValidEntry))
        {
            /* Got it */
            EntryFound = TRUE;
            break;
        }
    }

    /* Return our result of finding entry in the segments */
    return EntryFound;

invalid_entry:
    DPRINT1("HEAP: Invalid heap entry %p in heap %p\n", HeapEntry, Heap);
    return FALSE;
}

BOOLEAN NTAPI
RtlpValidateHeapSegment(
    PHEAP Heap,
    PHEAP_SEGMENT Segment,
    UCHAR SegmentOffset,
    PULONG FreeEntriesCount,
    PSIZE_T TotalFreeSize,
    PSIZE_T TagEntries,
    PSIZE_T PseudoTagEntries)
{
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;
    PLIST_ENTRY UcrEntry;
    SIZE_T ByteSize, Size, Result;
    PHEAP_ENTRY CurrentEntry;
    ULONG UnCommittedPages;
    ULONG UnCommittedRanges;
    ULONG PreviousSize;

    UnCommittedPages = 0;
    UnCommittedRanges = 0;

    if (IsListEmpty(&Segment->UCRSegmentList))
    {
        UcrEntry = NULL;
        UcrDescriptor = NULL;
    }
    else
    {
        UcrEntry = Segment->UCRSegmentList.Flink;
        UcrDescriptor = CONTAINING_RECORD(UcrEntry, HEAP_UCR_DESCRIPTOR, SegmentEntry);
    }

    if (Segment->BaseAddress == Heap)
        CurrentEntry = &Heap->Entry;
    else
        CurrentEntry = &Segment->Entry;

    while (CurrentEntry < Segment->LastValidEntry)
    {
        if (UcrDescriptor &&
            ((PVOID)CurrentEntry >= UcrDescriptor->Address))
        {
            DPRINT1("HEAP: Entry %p is not inside uncommited range [%p .. %p)\n",
                    CurrentEntry, UcrDescriptor->Address,
                    (PCHAR)UcrDescriptor->Address + UcrDescriptor->Size);

            return FALSE;
        }

        PreviousSize = 0;

        while (CurrentEntry < Segment->LastValidEntry)
        {
            if (PreviousSize != CurrentEntry->PreviousSize)
            {
                DPRINT1("HEAP: Entry %p has incorrect PreviousSize %x instead of %x\n",
                    CurrentEntry, CurrentEntry->PreviousSize, PreviousSize);

                return FALSE;
            }

            PreviousSize = CurrentEntry->Size;
            Size = CurrentEntry->Size << HEAP_ENTRY_SHIFT;

            if (CurrentEntry->Flags & HEAP_ENTRY_BUSY)
            {
                if (TagEntries)
                {
                    UNIMPLEMENTED;
                }

                /* Check fill pattern */
                if (CurrentEntry->Flags & HEAP_ENTRY_FILL_PATTERN)
                {
                    if (!RtlpCheckInUsePattern(CurrentEntry))
                        return FALSE;
                }
            }
            else
            {
                /* The entry is free, increase free entries count and total free size */
                *FreeEntriesCount = *FreeEntriesCount + 1;
                *TotalFreeSize += CurrentEntry->Size;

                if ((Heap->Flags & HEAP_FREE_CHECKING_ENABLED) &&
                    (CurrentEntry->Flags & HEAP_ENTRY_FILL_PATTERN))
                {
                    ByteSize = Size - sizeof(HEAP_FREE_ENTRY);

                    if ((CurrentEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT) &&
                        (ByteSize > sizeof(HEAP_FREE_ENTRY_EXTRA)))
                    {
                        ByteSize -= sizeof(HEAP_FREE_ENTRY_EXTRA);
                    }

                    Result = RtlCompareMemoryUlong((PCHAR)((PHEAP_FREE_ENTRY)CurrentEntry + 1),
                                                    ByteSize,
                                                    ARENA_FREE_FILLER);

                    if (Result != ByteSize)
                    {
                        DPRINT1("HEAP: Free heap block %p modified at %p after it was freed\n",
                            CurrentEntry,
                            (PCHAR)(CurrentEntry + 1) + Result);

                        return FALSE;
                    }
                }
            }

            if (CurrentEntry->SegmentOffset != SegmentOffset)
            {
                DPRINT1("HEAP: Heap entry %p SegmentOffset is incorrect %x (should be %x)\n",
                        CurrentEntry, SegmentOffset, CurrentEntry->SegmentOffset);
                return FALSE;
            }

            /* Check if it's the last entry */
            if (CurrentEntry->Flags & HEAP_ENTRY_LAST_ENTRY)
            {
                CurrentEntry = (PHEAP_ENTRY)((PCHAR)CurrentEntry + Size);

                if (!UcrDescriptor)
                {
                    /* Check if it's not really the last one */
                    if (CurrentEntry != Segment->LastValidEntry)
                    {
                        DPRINT1("HEAP: Heap entry %p is not last block in segment (%p)\n",
                                CurrentEntry, Segment->LastValidEntry);
                        return FALSE;
                    }
                }
                else if (CurrentEntry != UcrDescriptor->Address)
                {
                    DPRINT1("HEAP: Heap entry %p does not match next uncommitted address (%p)\n",
                        CurrentEntry, UcrDescriptor->Address);

                    return FALSE;
                }
                else
                {
                    UnCommittedPages += (ULONG)(UcrDescriptor->Size / PAGE_SIZE);
                    UnCommittedRanges++;

                    CurrentEntry = (PHEAP_ENTRY)((PCHAR)UcrDescriptor->Address + UcrDescriptor->Size);

                    /* Go to the next UCR descriptor */
                    UcrEntry = UcrEntry->Flink;
                    if (UcrEntry == &Segment->UCRSegmentList)
                    {
                        UcrEntry = NULL;
                        UcrDescriptor = NULL;
                    }
                    else
                    {
                        UcrDescriptor = CONTAINING_RECORD(UcrEntry, HEAP_UCR_DESCRIPTOR, SegmentEntry);
                    }
                }

                break;
            }

            /* Advance to the next entry */
            CurrentEntry = (PHEAP_ENTRY)((PCHAR)CurrentEntry + Size);
        }
    }

    /* Check total numbers of UCP and UCR */
    if (Segment->NumberOfUnCommittedPages != UnCommittedPages)
    {
        DPRINT1("HEAP: Segment %p NumberOfUnCommittedPages is invalid (%x != %x)\n",
            Segment, Segment->NumberOfUnCommittedPages, UnCommittedPages);

        return FALSE;
    }

    if (Segment->NumberOfUnCommittedRanges != UnCommittedRanges)
    {
        DPRINT1("HEAP: Segment %p NumberOfUnCommittedRanges is invalid (%x != %x)\n",
            Segment, Segment->NumberOfUnCommittedRanges, UnCommittedRanges);

        return FALSE;
    }

    return TRUE;
}

BOOLEAN NTAPI
RtlpValidateHeap(PHEAP Heap,
                 BOOLEAN ForceValidation)
{
    PHEAP_SEGMENT Segment;
    BOOLEAN EmptyList;
    UCHAR SegmentOffset;
    SIZE_T Size, TotalFreeSize;
    ULONG PreviousSize;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;
    PLIST_ENTRY ListHead, NextEntry;
    PHEAP_FREE_ENTRY FreeEntry;
    ULONG FreeBlocksCount, FreeListEntriesCount;

    /* Check headers */
    if (!RtlpValidateHeapHeaders(Heap, FALSE))
        return FALSE;

    /* Skip validation if it's not needed */
    if (!ForceValidation && !(Heap->Flags & HEAP_VALIDATE_ALL_ENABLED))
        return TRUE;

    /* Check free lists bitmaps */
    FreeListEntriesCount = 0;
    ListHead = &Heap->FreeLists[0];

    for (Size = 0; Size < HEAP_FREELISTS; Size++)
    {
        if (Size)
        {
            /* This is a dedicated list. Check if it's empty */
            EmptyList = IsListEmpty(ListHead);

            if (Heap->u.FreeListsInUseBytes[Size >> 3] & (1 << (Size & 7)))
            {
                if (EmptyList)
                {
                    DPRINT1("HEAP: Empty %x-free list marked as non-empty\n", Size);
                    return FALSE;
                }
            }
            else
            {
                if (!EmptyList)
                {
                    DPRINT1("HEAP: Non-empty %x-free list marked as empty\n", Size);
                    return FALSE;
                }
            }
        }

        /* Now check this list entries */
        NextEntry = ListHead->Flink;
        PreviousSize = 0;

        while (ListHead != NextEntry)
        {
            FreeEntry = CONTAINING_RECORD(NextEntry, HEAP_FREE_ENTRY, FreeList);
            NextEntry = NextEntry->Flink;

            /* If there is an in-use entry in a free list - that's quite a big problem */
            if (FreeEntry->Flags & HEAP_ENTRY_BUSY)
            {
                DPRINT1("HEAP: %Ix-dedicated list free element %p is marked in-use\n", Size, FreeEntry);
                return FALSE;
            }

            /* Check sizes according to that specific list's size */
            if ((Size == 0) && (FreeEntry->Size < HEAP_FREELISTS))
            {
                DPRINT1("HEAP: Non dedicated list free element %p has size %x which would fit a dedicated list\n", FreeEntry, FreeEntry->Size);
                return FALSE;
            }
            else if (Size && (FreeEntry->Size != Size))
            {
                DPRINT1("HEAP: %Ix-dedicated list free element %p has incorrect size %x\n", Size, FreeEntry, FreeEntry->Size);
                return FALSE;
            }
            else if ((Size == 0) && (FreeEntry->Size < PreviousSize))
            {
                DPRINT1("HEAP: Non dedicated list free element %p is not put in order\n", FreeEntry);
                return FALSE;
            }

            /* Remember previous size*/
            PreviousSize = FreeEntry->Size;

            /* Add up to the total amount of free entries */
            FreeListEntriesCount++;
        }

        /* Go to the head of the next free list */
        ListHead++;
    }

    /* Check big allocations */
    ListHead = &Heap->VirtualAllocdBlocks;
    NextEntry = ListHead->Flink;

    while (ListHead != NextEntry)
    {
        VirtualAllocBlock = CONTAINING_RECORD(NextEntry, HEAP_VIRTUAL_ALLOC_ENTRY, Entry);

        /* We can only check the fill pattern */
        if (VirtualAllocBlock->BusyBlock.Flags & HEAP_ENTRY_FILL_PATTERN)
        {
            if (!RtlpCheckInUsePattern(&VirtualAllocBlock->BusyBlock))
                return FALSE;
        }

        NextEntry = NextEntry->Flink;
    }

    /* Check all segments */
    FreeBlocksCount = 0;
    TotalFreeSize = 0;

    for (SegmentOffset = 0; SegmentOffset < HEAP_SEGMENTS; SegmentOffset++)
    {
        Segment = Heap->Segments[SegmentOffset];

        /* Go to the next one if there is no segment */
        if (!Segment) continue;

        if (!RtlpValidateHeapSegment(Heap,
                                     Segment,
                                     SegmentOffset,
                                     &FreeBlocksCount,
                                     &TotalFreeSize,
                                     NULL,
                                     NULL))
        {
            return FALSE;
        }
    }

    if (FreeListEntriesCount != FreeBlocksCount)
    {
        DPRINT1("HEAP: Free blocks count in arena (%lu) does not match free blocks number in the free lists (%lu)\n", FreeBlocksCount, FreeListEntriesCount);
        return FALSE;
    }

    if (Heap->TotalFreeSize != TotalFreeSize)
    {
        DPRINT1("HEAP: Total size of free blocks in arena (%Iu) does not equal to the one in heap header (%Iu)\n", TotalFreeSize, Heap->TotalFreeSize);
        return FALSE;
    }

    return TRUE;
}

/***********************************************************************
 *           RtlValidateHeap
 * Validates a specified heap.
 *
 * PARAMS
 *   Heap  [in] Handle to the heap
 *   Flags   [in] Bit flags that control access during operation
 *   Block  [in] Optional pointer to memory block to validate
 *
 * NOTES
 * Flags is ignored.
 *
 * RETURNS
 * TRUE: Success
 * FALSE: Failure
 *
 * @implemented
 */
BOOLEAN NTAPI RtlValidateHeap(
   HANDLE HeapPtr,
   ULONG Flags,
   PVOID Block
)
{
    PHEAP Heap = (PHEAP)HeapPtr;
    BOOLEAN HeapLocked = FALSE;
    BOOLEAN HeapValid;

    /* Check for page heap */
    if (Heap->ForceFlags & HEAP_FLAG_PAGE_ALLOCS)
        return RtlpDebugPageHeapValidate(HeapPtr, Flags, Block);

    /* Check signature */
    if (Heap->Signature != HEAP_SIGNATURE)
    {
        DPRINT1("HEAP: Signature %lx is invalid for heap %p\n", Heap->Signature, Heap);
        return FALSE;
    }

    /* Force flags */
    Flags = Heap->ForceFlags;

    /* Acquire the lock if necessary */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
    }

    /* Either validate whole heap or just one entry */
    if (!Block)
        HeapValid = RtlpValidateHeap(Heap, TRUE);
    else
        HeapValid = RtlpValidateHeapEntry(Heap, (PHEAP_ENTRY)Block - 1);

    /* Unlock if it's lockable */
    if (HeapLocked)
    {
        RtlLeaveHeapLock(Heap->LockVariable);
    }

    return HeapValid;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
RtlEnumProcessHeaps(PHEAP_ENUMERATION_ROUTINE HeapEnumerationRoutine,
                    PVOID lParam)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlGetProcessHeaps(ULONG count,
                   HANDLE *heaps)
{
    UNIMPLEMENTED;
    return 0;
}


/*
 * @implemented
 */
BOOLEAN NTAPI
RtlValidateProcessHeaps(VOID)
{
    UNIMPLEMENTED;
    return TRUE;
}


/*
 * @unimplemented
 */
BOOLEAN NTAPI
RtlZeroHeap(
    IN PVOID HeapHandle,
    IN ULONG Flags
    )
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlSetUserValueHeap(IN PVOID HeapHandle,
                    IN ULONG Flags,
                    IN PVOID BaseAddress,
                    IN PVOID UserValue)
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY HeapEntry;
    PHEAP_ENTRY_EXTRA Extra;
    BOOLEAN HeapLocked = FALSE, ValueSet = FALSE;

    /* Force flags */
    Flags |= Heap->Flags;

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugSetUserValueHeap(Heap, Flags, BaseAddress, UserValue);

    /* Lock if it's lockable */
    if (!(Heap->Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
    }

    /* Get a pointer to the entry */
    HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;

    /* If it's a free entry - return error */
    if (!(HeapEntry->Flags & HEAP_ENTRY_BUSY))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);

        /* Release the heap lock if it was acquired */
        if (HeapLocked)
            RtlLeaveHeapLock(Heap->LockVariable);

        return FALSE;
    }

    /* Check if this entry has an extra stuff associated with it */
    if (HeapEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
    {
        /* Use extra to store the value */
        Extra = RtlpGetExtraStuffPointer(HeapEntry);
        Extra->Settable = (ULONG_PTR)UserValue;

        /* Indicate that value was set */
        ValueSet = TRUE;
    }

    /* Release the heap lock if it was acquired */
    if (HeapLocked)
        RtlLeaveHeapLock(Heap->LockVariable);

    return ValueSet;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlSetUserFlagsHeap(IN PVOID HeapHandle,
                    IN ULONG Flags,
                    IN PVOID BaseAddress,
                    IN ULONG UserFlagsReset,
                    IN ULONG UserFlagsSet)
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY HeapEntry;
    BOOLEAN HeapLocked = FALSE;

    /* Force flags */
    Flags |= Heap->Flags;

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugSetUserFlagsHeap(Heap, Flags, BaseAddress, UserFlagsReset, UserFlagsSet);

    /* Lock if it's lockable */
    if (!(Heap->Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
    }

    /* Get a pointer to the entry */
    HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;

    /* If it's a free entry - return error */
    if (!(HeapEntry->Flags & HEAP_ENTRY_BUSY))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);

        /* Release the heap lock if it was acquired */
        if (HeapLocked)
            RtlLeaveHeapLock(Heap->LockVariable);

        return FALSE;
    }

    /* Set / reset flags */
    HeapEntry->Flags &= ~(UserFlagsReset >> 4);
    HeapEntry->Flags |= (UserFlagsSet >> 4);

    /* Release the heap lock if it was acquired */
    if (HeapLocked)
        RtlLeaveHeapLock(Heap->LockVariable);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlGetUserInfoHeap(IN PVOID HeapHandle,
                   IN ULONG Flags,
                   IN PVOID BaseAddress,
                   OUT PVOID *UserValue,
                   OUT PULONG UserFlags)
{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY HeapEntry;
    PHEAP_ENTRY_EXTRA Extra;
    BOOLEAN HeapLocked = FALSE;

    /* Force flags */
    Flags |= Heap->Flags;

    /* Call special heap */
    if (RtlpHeapIsSpecial(Flags))
        return RtlDebugGetUserInfoHeap(Heap, Flags, BaseAddress, UserValue, UserFlags);

    /* Lock if it's lockable */
    if (!(Heap->Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable, TRUE);
        HeapLocked = TRUE;
    }

    /* Get a pointer to the entry */
    HeapEntry = (PHEAP_ENTRY)BaseAddress - 1;

    /* If it's a free entry - return error */
    if (!(HeapEntry->Flags & HEAP_ENTRY_BUSY))
    {
        RtlSetLastWin32ErrorAndNtStatusFromNtStatus(STATUS_INVALID_PARAMETER);

        /* Release the heap lock if it was acquired */
        if (HeapLocked)
            RtlLeaveHeapLock(Heap->LockVariable);

        return FALSE;
    }

    /* Check if this entry has an extra stuff associated with it */
    if (HeapEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
    {
        /* Get pointer to extra data */
        Extra = RtlpGetExtraStuffPointer(HeapEntry);

        /* Pass user value */
        if (UserValue)
            *UserValue = (PVOID)Extra->Settable;
    }

    /* Decode and return user flags */
    if (UserFlags)
        *UserFlags = (HeapEntry->Flags & HEAP_ENTRY_SETTABLE_FLAGS) << 4;

    /* Release the heap lock if it was acquired */
    if (HeapLocked)
        RtlLeaveHeapLock(Heap->LockVariable);

    return TRUE;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlUsageHeap(IN HANDLE Heap,
             IN ULONG Flags,
             OUT PRTL_HEAP_USAGE Usage)
{
    /* TODO */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

PWSTR
NTAPI
RtlQueryTagHeap(IN PVOID HeapHandle,
                IN ULONG Flags,
                IN USHORT TagIndex,
                IN BOOLEAN ResetCounters,
                OUT PRTL_HEAP_TAG_INFO HeapTagInfo)
{
    /* TODO */
    UNIMPLEMENTED;
    return NULL;
}

ULONG
NTAPI
RtlExtendHeap(IN HANDLE Heap,
              IN ULONG Flags,
              IN PVOID P,
              IN SIZE_T Size)
{
    /* TODO */
    UNIMPLEMENTED;
    return 0;
}

ULONG
NTAPI
RtlCreateTagHeap(IN HANDLE HeapHandle,
                 IN ULONG Flags,
                 IN PWSTR TagName,
                 IN PWSTR TagSubName)
{
    /* TODO */
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
RtlWalkHeap(IN HANDLE HeapHandle,
            IN PVOID HeapEntry)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

PVOID
NTAPI
RtlProtectHeap(IN PVOID HeapHandle,
               IN BOOLEAN ReadOnly)
{
    UNIMPLEMENTED;
    return NULL;
}

NTSTATUS
NTAPI
RtlSetHeapInformation(IN HANDLE HeapHandle OPTIONAL,
                      IN HEAP_INFORMATION_CLASS HeapInformationClass,
                      IN PVOID HeapInformation,
                      IN SIZE_T HeapInformationLength)
{
    /* Setting heap information is not really supported except for enabling LFH */
    if (HeapInformationClass == HeapCompatibilityInformation)
    {
        /* Check buffer length */
        if (HeapInformationLength < sizeof(ULONG))
        {
            /* The provided buffer is too small */
            return STATUS_BUFFER_TOO_SMALL;
        }

        /* Check for a special magic value for enabling LFH */
        if (*(PULONG)HeapInformation != 2)
        {
            return STATUS_UNSUCCESSFUL;
        }

        DPRINT1("RtlSetHeapInformation() needs to enable LFH\n");
        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlQueryHeapInformation(HANDLE HeapHandle,
                        HEAP_INFORMATION_CLASS HeapInformationClass,
                        PVOID HeapInformation,
                        SIZE_T HeapInformationLength,
                        PSIZE_T ReturnLength OPTIONAL)
{
    PHEAP Heap = (PHEAP)HeapHandle;

    /* Only HeapCompatibilityInformation is supported */
    if (HeapInformationClass == HeapCompatibilityInformation)
    {
        /* Set result length */
        if (ReturnLength)
            *ReturnLength = sizeof(ULONG);

        /* Check buffer length */
        if (HeapInformationLength < sizeof(ULONG))
        {
            /* It's too small, return needed length */
            return STATUS_BUFFER_TOO_SMALL;
        }

        /* Return front end heap type */
        *(PULONG)HeapInformation = Heap->FrontEndHeapType;

        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
RtlMultipleAllocateHeap(IN PVOID HeapHandle,
                        IN ULONG Flags,
                        IN SIZE_T Size,
                        IN ULONG Count,
                        OUT PVOID *Array)
{
    UNIMPLEMENTED;
    return 0;
}

NTSTATUS
NTAPI
RtlMultipleFreeHeap(IN PVOID HeapHandle,
                    IN ULONG Flags,
                    IN ULONG Count,
                    OUT PVOID *Array)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
