/* COPYRIGHT:       See COPYING in the top level directory
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
*/

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

// Various defines, to be moved to a separate header file
#define HEAP_FREELISTS 128
#define HEAP_SEGMENTS 64
#define HEAP_MAX_PROCESS_HEAPS 16

#define HEAP_ENTRY_SIZE ((ULONG)sizeof(HEAP_ENTRY))
#define HEAP_ENTRY_SHIFT 3
#define HEAP_MAX_BLOCK_SIZE ((0x80000 - PAGE_SIZE) >> HEAP_ENTRY_SHIFT)

#define ARENA_INUSE_FILLER     0x55555555
#define ARENA_FREE_FILLER      0xaaaaaaaa
#define HEAP_TAIL_FILL         0xab

// from ntifs.h, should go to another header!
#define HEAP_GLOBAL_TAG                 0x0800
#define HEAP_PSEUDO_TAG_FLAG            0x8000
#define HEAP_TAG_MASK                  (HEAP_MAXIMUM_TAG << HEAP_TAG_SHIFT)

#define HEAP_EXTRA_FLAGS_MASK (HEAP_CAPTURE_STACK_BACKTRACES | \
                               HEAP_SETTABLE_USER_VALUE | \
                               (HEAP_TAG_MASK ^ (0xFF << HEAP_TAG_SHIFT)))

struct _HEAP_COMMON_ENTRY
{
    union
    {
        struct
        {
            USHORT Size; // 0x0
            UCHAR Flags; // 0x2
            UCHAR SmallTagIndex; //0x3
        };
        struct
        {
            PVOID SubSegmentCode; // 0x0
            USHORT PreviousSize; // 0x4
            union
            {
                UCHAR SegmentOffset; // 0x6
                UCHAR LFHFlags; // 0x6
            };
            UCHAR UnusedBytes; // 0x7
        };
        struct
        {
            USHORT FunctionIndex; // 0x0
            USHORT ContextValue; // 0x2
        };
        struct
        {
            ULONG InterceptorValue; // 0x0
            USHORT UnusedBytesLength; // 0x4
            UCHAR EntryOffset; // 0x6
            UCHAR ExtendedBlockSignature; // 0x7
        };
        struct
        {
            ULONG Code1; // 0x0
            USHORT Code2; // 0x4
            UCHAR Code3; // 0x6
            UCHAR Code4; // 0x7
        };
        ULONGLONG AgregateCode; // 0x0
    };
};

typedef struct _HEAP_FREE_ENTRY
{
    struct _HEAP_COMMON_ENTRY;
    LIST_ENTRY FreeList; // 0x8
}  HEAP_FREE_ENTRY, *PHEAP_FREE_ENTRY;

typedef struct _HEAP_ENTRY
{
    struct _HEAP_COMMON_ENTRY;
}  HEAP_ENTRY, *PHEAP_ENTRY;

C_ASSERT(sizeof(HEAP_ENTRY) == 8);
C_ASSERT((1 << HEAP_ENTRY_SHIFT) == sizeof(HEAP_ENTRY));

typedef struct _HEAP_TAG_ENTRY
{
    ULONG Allocs;
    ULONG Frees;
    ULONG Size;
    USHORT TagIndex;
    USHORT CreatorBackTraceIndex;
    WCHAR TagName[24];
} HEAP_TAG_ENTRY, *PHEAP_TAG_ENTRY;

typedef struct _HEAP_PSEUDO_TAG_ENTRY
{
    ULONG Allocs;
    ULONG Frees;
    ULONG Size;
} HEAP_PSEUDO_TAG_ENTRY, *PHEAP_PSEUDO_TAG_ENTRY;

typedef struct _HEAP_COUNTERS
{
    ULONG TotalMemoryReserved;
    ULONG TotalMemoryCommitted;
    ULONG TotalMemoryLargeUCR;
    ULONG TotalSizeInVirtualBlocks;
    ULONG TotalSegments;
    ULONG TotalUCRs;
    ULONG CommittOps;
    ULONG DeCommitOps;
    ULONG LockAcquires;
    ULONG LockCollisions;
    ULONG CommitRate;
    ULONG DecommittRate;
    ULONG CommitFailures;
    ULONG InBlockCommitFailures;
    ULONG CompactHeapCalls;
    ULONG CompactedUCRs;
    ULONG InBlockDeccommits;
    ULONG InBlockDeccomitSize;
} HEAP_COUNTERS, *PHEAP_COUNTERS;

typedef struct _HEAP_TUNING_PARAMETERS
{
    ULONG CommittThresholdShift;
    ULONG MaxPreCommittThreshold;
} HEAP_TUNING_PARAMETERS, *PHEAP_TUNING_PARAMETERS;

typedef struct _HEAP
{
    HEAP_ENTRY Entry;
    ULONG SegmentSignature;
    ULONG SegmentFlags;
    LIST_ENTRY SegmentListEntry;
    struct _HEAP *Heap;
    PVOID BaseAddress;
    ULONG NumberOfPages;
    PHEAP_ENTRY FirstEntry;
    PHEAP_ENTRY LastValidEntry;
    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;
    USHORT SegmentAllocatorBackTraceIndex;
    USHORT Reserved;
    LIST_ENTRY UCRSegmentList;
    ULONG Flags;
    ULONG ForceFlags;
    ULONG CompatibilityFlags;
    ULONG EncodeFlagMask;
    HEAP_ENTRY Encoding;
    ULONG PointerKey;
    ULONG Interceptor;
    ULONG VirtualMemoryThreshold;
    ULONG Signature;
    ULONG SegmentReserve;
    ULONG SegmentCommit;
    ULONG DeCommitFreeBlockThreshold;
    ULONG DeCommitTotalFreeThreshold;
    ULONG TotalFreeSize;
    ULONG MaximumAllocationSize;
    USHORT ProcessHeapsListIndex;
    USHORT HeaderValidateLength;
    PVOID HeaderValidateCopy;
    USHORT NextAvailableTagIndex;
    USHORT MaximumTagIndex;
    PHEAP_TAG_ENTRY TagEntries;
    LIST_ENTRY UCRList;
    ULONG AlignRound;
    ULONG AlignMask;
    LIST_ENTRY VirtualAllocdBlocks;
    LIST_ENTRY SegmentList;
    struct _HEAP_SEGMENT *Segments[HEAP_SEGMENTS]; //FIXME: non-Vista
    USHORT AllocatorBackTraceIndex;
    ULONG NonDedicatedListLength;
    PVOID BlocksIndex;
    PVOID UCRIndex;
    PHEAP_PSEUDO_TAG_ENTRY PseudoTagEntries;
    LIST_ENTRY FreeLists[HEAP_FREELISTS]; //FIXME: non-Vista
    union
    {
        ULONG FreeListsInUseUlong[HEAP_FREELISTS / (sizeof(ULONG) * 8)]; //FIXME: non-Vista
        UCHAR FreeListsInUseBytes[HEAP_FREELISTS / (sizeof(UCHAR) * 8)]; //FIXME: non-Vista
    } u;
    PHEAP_LOCK LockVariable;
    PRTL_HEAP_COMMIT_ROUTINE CommitRoutine;
    PVOID FrontEndHeap;
    USHORT FrontHeapLockCount;
    UCHAR FrontEndHeapType;
    HEAP_COUNTERS Counters;
    HEAP_TUNING_PARAMETERS TuningParameters;
} HEAP, *PHEAP;

typedef struct _HEAP_SEGMENT
{
    HEAP_ENTRY Entry;
    ULONG SegmentSignature;
    ULONG SegmentFlags;
    LIST_ENTRY SegmentListEntry;
    PHEAP Heap;
    PVOID BaseAddress;
    ULONG NumberOfPages;
    PHEAP_ENTRY FirstEntry;
    PHEAP_ENTRY LastValidEntry;
    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;
    USHORT SegmentAllocatorBackTraceIndex;
    USHORT Reserved;
    LIST_ENTRY UCRSegmentList;
    PHEAP_ENTRY LastEntryInSegment; //FIXME: non-Vista
} HEAP_SEGMENT, *PHEAP_SEGMENT;

typedef struct _HEAP_UCR_DESCRIPTOR
{
    LIST_ENTRY ListEntry;
    LIST_ENTRY SegmentEntry;
    PVOID Address;
    ULONG Size;
} HEAP_UCR_DESCRIPTOR, *PHEAP_UCR_DESCRIPTOR;

typedef struct _HEAP_ENTRY_EXTRA
{
     union
     {
          struct
          {
               USHORT AllocatorBackTraceIndex;
               USHORT TagIndex;
               ULONG Settable;
          };
          UINT64 ZeroInit;
     };
} HEAP_ENTRY_EXTRA, *PHEAP_ENTRY_EXTRA;

typedef HEAP_ENTRY_EXTRA HEAP_FREE_ENTRY_EXTRA, *PHEAP_FREE_ENTRY_EXTRA;

typedef struct _HEAP_VIRTUAL_ALLOC_ENTRY
{
    LIST_ENTRY Entry;
    HEAP_ENTRY_EXTRA ExtraStuff;
    ULONG CommitSize;
    ULONG ReserveSize;
    HEAP_ENTRY BusyBlock;
} HEAP_VIRTUAL_ALLOC_ENTRY, *PHEAP_VIRTUAL_ALLOC_ENTRY;

HANDLE NTAPI
RtlpSpecialHeapCreate(ULONG Flags,
                      PVOID Addr,
                      SIZE_T TotalSize,
                      SIZE_T CommitSize,
                      PVOID Lock,
                      PRTL_HEAP_PARAMETERS Parameters) { return NULL; };

BOOLEAN RtlpSpecialHeapEnabled = FALSE;
HEAP_LOCK RtlpProcessHeapsListLock;
PHEAP RtlpProcessHeaps[HEAP_MAX_PROCESS_HEAPS]; /* Usermode only */

/* Heap entry flags */
#define HEAP_ENTRY_BUSY           0x01
#define HEAP_ENTRY_EXTRA_PRESENT  0x02
#define HEAP_ENTRY_FILL_PATTERN   0x04
#define HEAP_ENTRY_VIRTUAL_ALLOC  0x08
#define HEAP_ENTRY_LAST_ENTRY     0x10
#define HEAP_ENTRY_SETTABLE_FLAG1 0x20
#define HEAP_ENTRY_SETTABLE_FLAG2 0x40
#define HEAP_ENTRY_SETTABLE_FLAG3 0x80
#define HEAP_ENTRY_SETTABLE_FLAGS (HEAP_ENTRY_SETTABLE_FLAG1 | HEAP_ENTRY_SETTABLE_FLAG2 | HEAP_ENTRY_SETTABLE_FLAG3)

/* Signatures */
#define HEAP_SIGNATURE         0xeefeeff
#define HEAP_SEGMENT_SIGNATURE 0xffeeffee

/* Segment flags */
#define HEAP_USER_ALLOCATED    0x1

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

UCHAR FORCEINLINE
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

ULONG NTAPI
RtlCompareMemoryUlong(PVOID Source, ULONG Length, ULONG Value);

PHEAP_FREE_ENTRY NTAPI
RtlpCoalesceFreeBlocks (PHEAP Heap,
                        PHEAP_FREE_ENTRY FreeEntry,
                        PSIZE_T FreeSize,
                        BOOLEAN Remove);

/* FUNCTIONS *****************************************************************/

VOID NTAPI
RtlpInitializeHeap(PHEAP Heap,
                   PULONG HeaderSize,
                   ULONG Flags,
                   BOOLEAN AllocateLock,
                   PVOID Lock)
{
    PVOID NextHeapBase = Heap + 1;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;
    ULONG NumUCRs = 8;
    ULONG i;
    NTSTATUS Status;

    /* Add UCRs size */
    *HeaderSize += NumUCRs * sizeof(*UcrDescriptor);

    /* Prepare a list of UCRs */
    InitializeListHead(&Heap->UCRList);
    InitializeListHead(&Heap->UCRSegmentList);
    UcrDescriptor = NextHeapBase;

    for (i=0; i<NumUCRs; i++, UcrDescriptor++)
    {
        InsertTailList(&Heap->UCRList, &UcrDescriptor->ListEntry);
    }

    NextHeapBase = UcrDescriptor;
    // TODO: Add tagging

    /* Round up header size again */
    *HeaderSize = ROUND_UP(*HeaderSize, HEAP_ENTRY_SIZE);

    ASSERT(*HeaderSize <= PAGE_SIZE);

    /* Initialize heap's header */
    Heap->Entry.Size = (*HeaderSize) >> HEAP_ENTRY_SHIFT;
    Heap->Entry.Flags = HEAP_ENTRY_BUSY;

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
    Heap->HeaderValidateCopy = NULL;
    Heap->HeaderValidateLength = ((PCHAR)NextHeapBase - (PCHAR)Heap);

    /* Initialize free lists */
    for (i=0; i<HEAP_FREELISTS; i++)
    {
        InitializeListHead(&Heap->FreeLists[i]);
    }

    /* Initialize "big" allocations list */
    InitializeListHead(&Heap->VirtualAllocdBlocks);

    /* Initialize lock */
    if (AllocateLock)
    {
        Lock = NextHeapBase;
        Status = RtlInitializeHeapLock((PHEAP_LOCK)Lock);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Initializing the lock failed!\n");
            return /*NULL*/; // FIXME!
        }
    }

    /* Set the lock variable */
    Heap->LockVariable = Lock;
}

VOID FORCEINLINE
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

VOID FORCEINLINE
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

    /* Check if PreviousSize of the next entry matches ours */
    if (!(FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
    {
        ASSERT(((PHEAP_ENTRY)FreeEntry + BlockSize)->PreviousSize = BlockSize);
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
            Size = BlockSize;
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
    PLIST_ENTRY OldBlink, OldFlink;

    // FIXME: Maybe use RemoveEntryList?

    /* Remove the free block */
    OldFlink = FreeEntry->FreeList.Flink;
    OldBlink = FreeEntry->FreeList.Blink;
    OldBlink->Flink = OldFlink;
    OldFlink->Blink = OldBlink;

    /* Update the freelists bitmap */
    if ((OldFlink == OldBlink) &&
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

    /* Restore the real size */
    return VirtualEntry->CommitSize - HeapEntry->Size;
}

PHEAP_UCR_DESCRIPTOR NTAPI
RtlpCreateUnCommittedRange(PHEAP_SEGMENT Segment)
{
    PLIST_ENTRY Entry;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;
    PHEAP Heap = Segment->Heap;

    DPRINT("RtlpCreateUnCommittedRange(%p)\n", Segment);

    /* Check if we have unused UCRs */
    if (IsListEmpty(&Heap->UCRList))
    {
        ASSERT(FALSE);
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

    DPRINT("RtlpInsertUnCommittedPages(%p %p %x)\n", Segment, Address, Size);

    /* Go through the list of UCR descriptors, they are sorted from lowest address
       to the highest */
    Current = Segment->UCRSegmentList.Flink;
    while(Current != &Segment->UCRSegmentList)
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

            /* We found the block after which the new one should go */
            break;
        }
        else if (((ULONG_PTR)UcrDescriptor->Address + UcrDescriptor->Size) == Address)
        {
            /* Modify this entry */
            Address = (ULONG_PTR)UcrDescriptor->Address;
            Size += UcrDescriptor->Size;

            /* Remove it from the list and destroy it */
            RemoveEntryList(Current);
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

    /* "Current" is the descriptor after which our one should go */
    InsertTailList(Current, &UcrDescriptor->SegmentEntry);

    DPRINT("Added segment UCR with base %p, size 0x%x\n", Address, Size);

    /* Increase counters */
    Segment->NumberOfUnCommittedRanges++;
}

PHEAP_FREE_ENTRY NTAPI
RtlpFindAndCommitPages(PHEAP Heap,
                       PHEAP_SEGMENT Segment,
                       PSIZE_T Size,
                       PVOID Address)
{
    PLIST_ENTRY Current;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor, PreviousUcr = NULL;
    PHEAP_ENTRY FirstEntry, LastEntry, PreviousLastEntry;
    NTSTATUS Status;

    DPRINT("RtlpFindAndCommitPages(%p %p %x %p)\n", Heap, Segment, *Size, Address);

    /* Go through UCRs in a segment */
    Current = Segment->UCRSegmentList.Flink;
    while(Current != &Segment->UCRSegmentList)
    {
        UcrDescriptor = CONTAINING_RECORD(Current, HEAP_UCR_DESCRIPTOR, SegmentEntry);

        /* Check if we can use that one right away */
        if (UcrDescriptor->Size >= *Size &&
            (UcrDescriptor->Address == Address || !Address))
        {
            /* Get the address */
            Address = UcrDescriptor->Address;

            /* Commit it */
            if (Heap->CommitRoutine)
            {
                Status = Heap->CommitRoutine(Heap, &Address, Size);
            }
            else
            {
                Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                                 &Address,
                                                 0,
                                                 Size,
                                                 MEM_COMMIT,
                                                 PAGE_READWRITE);
            }

            DPRINT("Committed %d bytes at base %p, UCR size is %d\n", *Size, Address, UcrDescriptor->Size);

            /* Fail in unsuccessful case */
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("Committing page failed with status 0x%08X\n", Status);
                return NULL;
            }

            /* Update tracking numbers */
            Segment->NumberOfUnCommittedPages -= *Size / PAGE_SIZE;

            /* Calculate first and last entries */
            FirstEntry = (PHEAP_ENTRY)Address;

            if ((Segment->LastEntryInSegment->Flags & HEAP_ENTRY_LAST_ENTRY) &&
                (ULONG_PTR)(Segment->LastEntryInSegment + Segment->LastEntryInSegment->Size) == (ULONG_PTR)UcrDescriptor->Address)
            {
                LastEntry = Segment->LastEntryInSegment;
            }
            else
            {
                /* Go through the entries to find the last one */

                if (PreviousUcr)
                    LastEntry = (PHEAP_ENTRY)((ULONG_PTR)PreviousUcr->Address + PreviousUcr->Size);
                else
                    LastEntry = Segment->FirstEntry;

                while (!(LastEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
                {
                    PreviousLastEntry = LastEntry;
                    LastEntry += LastEntry->Size;

                    if ((ULONG_PTR)LastEntry >= (ULONG_PTR)Segment->LastValidEntry ||
                        LastEntry->Size == 0)
                    {
                        if (LastEntry == (PHEAP_ENTRY)Address)
                        {
                            /* Found it */
                            LastEntry = PreviousLastEntry;
                            break;
                        }

                        DPRINT1("Last entry not found in a committed range near to %p\n", PreviousLastEntry);
                        return NULL;
                    }
                }
            }

            /* Unmark it as a last entry */
            LastEntry->Flags &= ~HEAP_ENTRY_LAST_ENTRY;

            /* Update UCR descriptor */
            UcrDescriptor->Address = (PUCHAR)UcrDescriptor->Address + *Size;
            UcrDescriptor->Size -= *Size;

            DPRINT("Updating UcrDescriptor %p, new Address %p, size %d\n",
                UcrDescriptor, UcrDescriptor->Address, UcrDescriptor->Size);

            /* Check if anything left in this UCR */
            if (UcrDescriptor->Size == 0)
            {
                /* It's fully exhausted */
                if (UcrDescriptor->Address == Segment->LastValidEntry)
                {
                    FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
                    Segment->LastEntryInSegment = FirstEntry;
                }
                else
                {
                    FirstEntry->Flags = 0;
                    Segment->LastEntryInSegment = Segment->FirstEntry;
                }

                /* This UCR needs to be removed because it became useless */
                RemoveEntryList(&UcrDescriptor->SegmentEntry);

                RtlpDestroyUnCommittedRange(Segment, UcrDescriptor);
                Segment->NumberOfUnCommittedRanges--;
            }
            else
            {
                FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
                Segment->LastEntryInSegment = FirstEntry;
            }

            /* Set various first entry fields*/
            FirstEntry->SegmentOffset = LastEntry->SegmentOffset;
            FirstEntry->Size = *Size >> HEAP_ENTRY_SHIFT;
            FirstEntry->PreviousSize = LastEntry->Size;

            /* Update previous size */
            if (!(FirstEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
                (FirstEntry + FirstEntry->Size)->PreviousSize = FirstEntry->Size;

            /* We're done */
            return (PHEAP_FREE_ENTRY)FirstEntry;
        }

        /* Advance to the next descriptor */
        Current = Current->Flink;
    }

    return NULL;
}

VOID NTAPI
RtlpDeCommitFreeBlock(PHEAP Heap,
                      PHEAP_FREE_ENTRY FreeEntry,
                      SIZE_T Size)
{
#if 0
    PHEAP_SEGMENT Segment;
    PHEAP_ENTRY PrecedingInUseEntry = NULL, NextInUseEntry = NULL;
    PHEAP_FREE_ENTRY NextFreeEntry;
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;
    ULONG PrecedingSize, NextSize, DecommitSize;
    ULONG DecommitBase;
    NTSTATUS Status;

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
    NextFreeEntry = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeEntry + (Size >> HEAP_ENTRY_SHIFT));
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

    NextFreeEntry  = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)NextFreeEntry - NextSize);

    if (DecommitSize > DecommitBase)
        DecommitSize -= DecommitBase;
    else
    {
        /* Nothing to decommit */
        RtlpInsertFreeBlock(Heap, FreeEntry, PrecedingSize);
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
        RtlpInsertFreeBlock(Heap, FreeEntry, PrecedingSize);
        return;
    }

    /* Insert uncommitted pages */
    RtlpInsertUnCommittedPages(Segment, DecommitBase, DecommitSize);
    Segment->NumberOfUnCommittedPages += (DecommitSize / PAGE_SIZE);

    if (PrecedingSize)
    {
        /* Adjust size of this free entry and insert it */
        FreeEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
        FreeEntry->Size = PrecedingSize;
        Heap->TotalFreeSize += PrecedingSize;
        Segment->LastEntryInSegment = (PHEAP_ENTRY)FreeEntry;
        RtlpInsertFreeBlockHelper(Heap, FreeEntry, PrecedingSize);
    }
    else if (NextInUseEntry)
    {
        /* Adjust preceding in use entry */
        PrecedingInUseEntry->Flags |= HEAP_ENTRY_LAST_ENTRY;
        Segment->LastEntryInSegment = PrecedingInUseEntry;
    }
    else if ((Segment->LastEntryInSegment >= (PHEAP_ENTRY)DecommitBase))
    {
        /* Adjust last entry in the segment */
        Segment->LastEntryInSegment = Segment->FirstEntry;
    }

    /* Now the next one */
    if (NextSize)
    {
        /* Adjust size of this free entry and insert it */
        NextFreeEntry->Flags = 0;
        NextFreeEntry->PreviousSize = 0;
        NextFreeEntry->SegmentOffset = Segment->Entry.SegmentOffset;
        NextFreeEntry->Size = NextSize;

        ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)NextFreeEntry + NextSize))->PreviousSize = NextSize;

        Heap->TotalFreeSize += PrecedingSize;
        RtlpInsertFreeBlockHelper(Heap, NextFreeEntry, NextSize);
    }
    else if (NextInUseEntry)
    {
        NextInUseEntry->PreviousSize = 0;
    }
#else
    RtlpInsertFreeBlock(Heap, FreeEntry, Size);
#endif
}

BOOLEAN NTAPI
RtlpInitializeHeapSegment(PHEAP Heap,
                          PHEAP_SEGMENT Segment,
                          UCHAR SegmentIndex,
                          ULONG Flags,
                          PVOID BaseAddress,
                          PVOID UncommittedBase,
                          PVOID LimitAddress)
{
    ULONG Pages, CommitSize;
    PHEAP_ENTRY HeapEntry;
    USHORT PreviousSize = 0, NewSize;
    NTSTATUS Status;

    Pages = ((PCHAR)LimitAddress - (PCHAR)BaseAddress) / PAGE_SIZE;

    HeapEntry = (PHEAP_ENTRY)ROUND_UP(Segment + 1, HEAP_ENTRY_SIZE);

    DPRINT("RtlpInitializeHeapSegment(%p %p %x %x %p %p %p)\n", Heap, Segment, SegmentIndex, Flags, BaseAddress, UncommittedBase, LimitAddress);
    DPRINT("Pages %x, HeapEntry %p, sizeof(HEAP_SEGMENT) %x\n", Pages, HeapEntry, sizeof(HEAP_SEGMENT));

    /* Check if it's the first segment and remember its size */
    if (Heap == BaseAddress)
        PreviousSize = Heap->Entry.Size;

    NewSize = ((PCHAR)HeapEntry - (PCHAR)Segment) >> HEAP_ENTRY_SHIFT;

    if ((PVOID)(HeapEntry + 1) >= UncommittedBase)
    {
        /* Check if it goes beyond the limit */
        if ((PVOID)(HeapEntry + 1) >= LimitAddress)
            return FALSE;

        /* Need to commit memory */
        CommitSize = (PCHAR)(HeapEntry + 1) - (PCHAR)UncommittedBase;
        Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                        (PVOID)&UncommittedBase,
                                        0,
                                        &CommitSize,
                                        MEM_COMMIT,
                                        PAGE_READWRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Committing page failed with status 0x%08X\n", Status);
            return FALSE;
        }

        DPRINT("Committed %d bytes at base %p\n", CommitSize, UncommittedBase);

        /* Calcule the new uncommitted base */
        UncommittedBase = (PVOID)((PCHAR)UncommittedBase + CommitSize);
    }

    /* Initialize the segment entry */
    Segment->Entry.PreviousSize = PreviousSize;
    Segment->Entry.Size = NewSize;
    Segment->Entry.Flags = HEAP_ENTRY_BUSY;
    Segment->Entry.SegmentOffset = SegmentIndex;

    /* Initialize the segment itself */
    Segment->SegmentSignature = HEAP_SEGMENT_SIGNATURE;
    Segment->Heap = Heap;
    Segment->BaseAddress = BaseAddress;
    Segment->FirstEntry = HeapEntry;
    Segment->LastValidEntry = (PHEAP_ENTRY)((PCHAR)BaseAddress + Pages * PAGE_SIZE);
    Segment->NumberOfPages = Pages;
    Segment->NumberOfUnCommittedPages = ((PCHAR)LimitAddress - (PCHAR)UncommittedBase) / PAGE_SIZE;
    InitializeListHead(&Segment->UCRSegmentList);

    /* Insert uncommitted pages into UCR (uncommitted ranges) list */
    if (Segment->NumberOfUnCommittedPages)
    {
        RtlpInsertUnCommittedPages(Segment, (ULONG_PTR)UncommittedBase, Segment->NumberOfUnCommittedPages * PAGE_SIZE);
    }

    /* Set the segment index pointer */
    Heap->Segments[SegmentIndex] = Segment;

    /* Prepare a free heap entry */
    HeapEntry->Flags = HEAP_ENTRY_LAST_ENTRY;
    HeapEntry->PreviousSize = Segment->Entry.Size;
    HeapEntry->SegmentOffset = SegmentIndex;

    /* Set last entry in segment */
    Segment->LastEntryInSegment = HeapEntry;

    /* Insert it */
    RtlpInsertFreeBlock(Heap, (PHEAP_FREE_ENTRY)HeapEntry, (PHEAP_ENTRY)UncommittedBase - HeapEntry);

    return TRUE;
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
    DPRINT1("Destroying segment %p, BA %p\n", Segment, BaseAddress);

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

/* Usermode only! */
VOID NTAPI
RtlpAddHeapToProcessList(PHEAP Heap)
{
    PPEB Peb;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Acquire the lock */
    RtlEnterHeapLock(&RtlpProcessHeapsListLock);

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
    Heap->ProcessHeapsListIndex = Peb->NumberOfHeaps;
    // } _SEH2_FINALLY {

    /* Release the lock */
    RtlLeaveHeapLock(&RtlpProcessHeapsListLock);

    // } _SEH2_END
}

/* Usermode only! */
VOID NTAPI
RtlpRemoveHeapFromProcessList(PHEAP Heap)
{
    PPEB Peb;
    PHEAP *Current, *Next;
    ULONG Count;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Acquire the lock */
    RtlEnterHeapLock(&RtlpProcessHeapsListLock);

    /* Check if we don't need anything to do */
    if ((Heap->ProcessHeapsListIndex == 0) ||
        (Heap->ProcessHeapsListIndex > Peb->NumberOfHeaps) ||
        (Peb->NumberOfHeaps == 0))
    {
        /* Release the lock */
        RtlLeaveHeapLock(&RtlpProcessHeapsListLock);

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
    RtlLeaveHeapLock(&RtlpProcessHeapsListLock);
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

        /* Update last entry in the segment */
        if (CurrentEntry->Flags & HEAP_ENTRY_LAST_ENTRY)
            Heap->Segments[CurrentEntry->SegmentOffset]->LastEntryInSegment = (PHEAP_ENTRY)CurrentEntry;

        /* Advance FreeEntry and update sizes */
        FreeEntry = CurrentEntry;
        *FreeSize += CurrentEntry->Size;
        Heap->TotalFreeSize -= CurrentEntry->Size;
        FreeEntry->Size = *FreeSize;

        /* Also update previous size if needed */
        if (!(FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
        {
            ((PHEAP_ENTRY)FreeEntry + *FreeSize)->PreviousSize = *FreeSize;
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

            /* Update last entry in the segment */
            if (FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY)
                Heap->Segments[FreeEntry->SegmentOffset]->LastEntryInSegment = (PHEAP_ENTRY)FreeEntry;

            /* Remove next entry now */
            RtlpRemoveFreeBlock(Heap, NextEntry, FALSE, FALSE);

            /* Update sizes */
            *FreeSize += NextEntry->Size;
            Heap->TotalFreeSize -= NextEntry->Size;
            FreeEntry->Size = *FreeSize;

            /* Also update previous size if needed */
            if (!(FreeEntry->Flags & HEAP_ENTRY_LAST_ENTRY))
            {
                ((PHEAP_ENTRY)FreeEntry + *FreeSize)->PreviousSize = *FreeSize;
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
    Pages = (Size + PAGE_SIZE - 1) / PAGE_SIZE;
    FreeSize = Pages * PAGE_SIZE;
    DPRINT("Pages %x, FreeSize %x. Going through segments...\n", Pages, FreeSize);

    /* Find an empty segment */
    EmptyIndex = HEAP_SEGMENTS;
    for (Index = 0; Index < HEAP_SEGMENTS; Index++)
    {
        Segment = Heap->Segments[Index];

        if (Segment) DPRINT("Segment[%d] %p with NOUCP %x\n", Index, Segment, Segment->NumberOfUnCommittedPages);

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

            DPRINT("Committed %d bytes at base %p\n", CommitSize, Segment);

            /* Initialize heap segment if commit was successful */
            if (NT_SUCCESS(Status))
            {
                if (!RtlpInitializeHeapSegment(Heap, Segment, EmptyIndex, 0, Segment,
                    (PCHAR)Segment + CommitSize, (PCHAR)Segment + ReserveSize))
                {
                    Status = STATUS_NO_MEMORY;
                }
            }

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
    PPEB Peb;
    ULONG_PTR MaximumUserModeAddress;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    MEMORY_BASIC_INFORMATION MemoryInfo;
    ULONG NtGlobalFlags = RtlGetNtGlobalFlags();
    ULONG HeapSegmentFlags = 0;
    NTSTATUS Status;
    ULONG MaxBlockSize, HeaderSize;
    BOOLEAN AllocateLock = FALSE;

    /* Check for a special heap */
    if (RtlpSpecialHeapEnabled && !Addr && !Lock)
    {
        Heap = RtlpSpecialHeapCreate(Flags, Addr, TotalSize, CommitSize, Lock, Parameters);
        if (Heap) return Heap;

        ASSERT(FALSE);
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

        /* Get PEB */
        Peb = RtlGetCurrentPeb();

        /* Apply defaults for non-set parameters */
        if (!Parameters->SegmentCommit) Parameters->SegmentCommit = Peb->HeapSegmentCommit;
        if (!Parameters->SegmentReserve) Parameters->SegmentReserve = Peb->HeapSegmentReserve;
        if (!Parameters->DeCommitFreeBlockThreshold) Parameters->DeCommitFreeBlockThreshold = Peb->HeapDeCommitFreeBlockThreshold;
        if (!Parameters->DeCommitTotalFreeThreshold) Parameters->DeCommitTotalFreeThreshold = Peb->HeapDeCommitTotalFreeThreshold;
    }
    else
    {
        /* Apply defaults for non-set parameters */
#if 0
        if (!Parameters->SegmentCommit) Parameters->SegmentCommit = MmHeapSegmentCommit;
        if (!Parameters->SegmentReserve) Parameters->SegmentReserve = MmHeapSegmentReserve;
        if (!Parameters->DeCommitFreeBlockThreshold) Parameters->DeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;
        if (!Parameters->DeCommitTotalFreeThreshold) Parameters->DeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
#endif
    }

    // FIXME: Move to memory manager
        if (!Parameters->SegmentCommit) Parameters->SegmentCommit = PAGE_SIZE * 2;
        if (!Parameters->SegmentReserve) Parameters->SegmentReserve = 1048576;
        if (!Parameters->DeCommitFreeBlockThreshold) Parameters->DeCommitFreeBlockThreshold = PAGE_SIZE;
        if (!Parameters->DeCommitTotalFreeThreshold) Parameters->DeCommitTotalFreeThreshold = 65536;

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

    /* Calculate header size */
    HeaderSize = sizeof(HEAP);
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        if (Lock)
        {
            Flags |= HEAP_LOCK_USER_ALLOCATED;
        }
        else
        {
            HeaderSize += sizeof(HEAP_LOCK);
            AllocateLock = TRUE;
        }
    }
    else if (Lock)
    {
        /* Invalid parameters */
        return NULL;
    }

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

        DPRINT("Committed %d bytes at base %p\n", CommitSize, CommittedAddress);

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

    DPRINT("Created heap %p, CommitSize %x, ReserveSize %x\n", Heap, CommitSize, TotalSize);

    /* Initialize the heap */
    RtlpInitializeHeap(Heap, &HeaderSize, Flags, AllocateLock, Lock);

    /* Initialize heap's first segment */
    if (!RtlpInitializeHeapSegment(Heap,
                                   (PHEAP_SEGMENT)((PCHAR)Heap + HeaderSize),
                                   0,
                                   HeapSegmentFlags,
                                   CommittedAddress,
                                   UncommittedAddress,
                                   (PCHAR)CommittedAddress + TotalSize))
    {
        DPRINT1("Failed to initialize heap segment\n");
        return NULL;
    }

    /* Set other data */
    Heap->ProcessHeapsListIndex = 0;
    Heap->SegmentCommit = Parameters->SegmentCommit;
    Heap->SegmentReserve = Parameters->SegmentReserve;
    Heap->DeCommitFreeBlockThreshold = Parameters->DeCommitFreeBlockThreshold >> HEAP_ENTRY_SHIFT;
    Heap->DeCommitTotalFreeThreshold = Parameters->DeCommitTotalFreeThreshold >> HEAP_ENTRY_SHIFT;
    Heap->MaximumAllocationSize = Parameters->MaximumAllocationSize;
    Heap->VirtualMemoryThreshold = ROUND_UP(Parameters->VirtualMemoryThreshold, HEAP_ENTRY_SIZE) >> HEAP_ENTRY_SHIFT;
    Heap->CommitRoutine = Parameters->CommitRoutine;

    /* Set alignment */
    if (Flags & HEAP_CREATE_ALIGN_16)
    {
        Heap->AlignMask = (ULONG)~15;
        Heap->AlignRound = 15 + sizeof(HEAP_ENTRY);
    }
    else
    {
        Heap->AlignMask = (ULONG)~(HEAP_ENTRY_SIZE - 1);
        Heap->AlignRound = HEAP_ENTRY_SIZE - 1 + sizeof(HEAP_ENTRY);
    }

    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED)
        Heap->AlignRound += HEAP_ENTRY_SIZE;

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
    PHEAP_UCR_DESCRIPTOR UcrDescriptor;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualEntry;
    PVOID BaseAddress;
    SIZE_T Size;
    LONG i;
    PHEAP_SEGMENT Segment;

    if (!HeapPtr) return NULL;

    // TODO: Check for special heap

    /* Check for a process heap */
    if (RtlpGetMode() == UserMode &&
        HeapPtr == NtCurrentPeb()->ProcessHeap) return HeapPtr;

    /* Free up all big allocations */
    Current = Heap->VirtualAllocdBlocks.Flink;
    while (Current != &Heap->VirtualAllocdBlocks)
    {
        VirtualEntry = CONTAINING_RECORD(Current, HEAP_VIRTUAL_ALLOC_ENTRY, Entry);
        BaseAddress = (PVOID)VirtualEntry;
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

    /* Go through heap's global uncommitted ranges list and free them */
    DPRINT1("HEAP: Freeing segment's UCRs is not yet implemented!\n");
    Current = Heap->UCRSegmentList.Flink;
    while(Current != &Heap->UCRSegmentList)
    {
        UcrDescriptor = CONTAINING_RECORD(Current, HEAP_UCR_DESCRIPTOR, ListEntry);

        if (UcrDescriptor)
        {
            BaseAddress = UcrDescriptor->Address;
            Size = 0;

            /* Release that memory */
            ZwFreeVirtualMemory(NtCurrentProcess(),
                                &BaseAddress,
                                &Size,
                                MEM_RELEASE);
        }

        /* Advance to the next descriptor */
        Current = Current->Flink;
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
               PHEAP_FREE_ENTRY FreeBlock,
               SIZE_T AllocationSize,
               SIZE_T Index,
               SIZE_T Size)
{
    PHEAP_FREE_ENTRY SplitBlock, SplitBlock2;
    UCHAR FreeFlags;
    PHEAP_ENTRY InUseEntry;
    SIZE_T FreeSize;

    /* Save flags, update total free size */
    FreeFlags = FreeBlock->Flags;
    Heap->TotalFreeSize -= FreeBlock->Size;

    /* Make this block an in-use one */
    InUseEntry = (PHEAP_ENTRY)FreeBlock;
    InUseEntry->Flags = HEAP_ENTRY_BUSY;
    InUseEntry->SmallTagIndex = 0;

    /* Calculate the extra amount */
    FreeSize = InUseEntry->Size - Index;

    /* Update it's size fields (we don't need their data anymore) */
    InUseEntry->Size = Index;
    InUseEntry->UnusedBytes = AllocationSize - Size;

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
            SplitBlock->Size = FreeSize;
            SplitBlock->PreviousSize = Index;

            /* Check if it's the last entry */
            if (FreeFlags & HEAP_ENTRY_LAST_ENTRY)
            {
                /* Insert it to the free list if it's the last entry */
                RtlpInsertFreeBlockHelper(Heap, SplitBlock, FreeSize, TRUE);
                Heap->TotalFreeSize += FreeSize;
            }
            else
            {
                /* Not so easy - need to update next's previous size too */
                SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);

                if (SplitBlock2->Flags & HEAP_ENTRY_BUSY)
                {
                    SplitBlock2->PreviousSize = (USHORT)FreeSize;
                    RtlpInsertFreeBlockHelper(Heap, SplitBlock, FreeSize, TRUE);
                    Heap->TotalFreeSize += FreeSize;
                }
                else
                {
                    /* Even more complex - the next entry is free, so we can merge them into one! */
                    SplitBlock->Flags = SplitBlock2->Flags;

                    /* Remove that next entry */
                    RtlpRemoveFreeBlock(Heap, SplitBlock2, FALSE, TRUE);

                    /* Update sizes */
                    FreeSize += SplitBlock2->Size;
                    Heap->TotalFreeSize -= SplitBlock2->Size;

                    if (FreeSize <= HEAP_MAX_BLOCK_SIZE)
                    {
                        /* Insert it back */
                        SplitBlock->Size = FreeSize;

                        /* Don't forget to update previous size of the next entry! */
                        if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY))
                        {
                            ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = FreeSize;
                        }

                        /* Actually insert it */
                        RtlpInsertFreeBlockHelper( Heap, SplitBlock, (USHORT)FreeSize, TRUE );

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

            /* Update last entry in segment */
            if (SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY)
            {
                Heap->Segments[SplitBlock->SegmentOffset]->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;
            }
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
                    InUseEntry = RtlpSplitEntry(Heap, FreeBlock, AllocationSize, Index, Size);

                    /* Release the lock */
                    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

                    /* Zero memory if that was requested */
                    if (Flags & HEAP_ZERO_MEMORY)
                        RtlZeroMemory(InUseEntry + 1, Size);

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
        InUseEntry = RtlpSplitEntry(Heap, FreeBlock, AllocationSize, Index, Size);

        /* Release the lock */
        if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

        /* Zero memory if that was requested */
        if (Flags & HEAP_ZERO_MEMORY)
            RtlZeroMemory(InUseEntry + 1, Size);

        /* Return pointer to the */
        return InUseEntry + 1;
    }

    /* Really unfortunate, out of memory condition */
    //STATUS_NO_MEMORY;

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
    SIZE_T Index;
    PLIST_ENTRY FreeListHead;
    PHEAP_ENTRY InUseEntry;
    PHEAP_FREE_ENTRY FreeBlock;
    ULONG InUseIndex, i;
    UCHAR FreeFlags;
    EXCEPTION_RECORD ExceptionRecord;
    BOOLEAN HeapLocked = FALSE;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualBlock = NULL;
    NTSTATUS Status;

    /* Force flags */
    Flags |= Heap->ForceFlags;

    /* Check for the maximum size */
    if (Size >= 0x80000000)
    {
        // STATUS_NO_MEMORY
        return FALSE;
    }

    if (Flags & (
        HEAP_VALIDATE_ALL_ENABLED |
        HEAP_VALIDATE_PARAMETERS_ENABLED |
        HEAP_FLAG_PAGE_ALLOCS |
        HEAP_EXTRA_FLAGS_MASK |
        HEAP_CREATE_ENABLE_TRACING |
        HEAP_FREE_CHECKING_ENABLED |
        HEAP_TAIL_CHECKING_ENABLED |
        HEAP_CREATE_ALIGN_16))
    {
        DPRINT1("HEAP: RtlAllocateHeap is called with unsupported flags %x, ignoring\n", Flags);
    }

    /* Calculate allocation size and index */
    if (!Size) Size = 1;
    AllocationSize = (Size + Heap->AlignRound) & Heap->AlignMask;
    Index = AllocationSize >>  HEAP_ENTRY_SHIFT;

    /* Acquire the lock if necessary */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock( Heap->LockVariable );
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
            RtlpRemoveFreeBlock(Heap, FreeBlock, TRUE, TRUE);

            /* Update the total free size of the heap */
            Heap->TotalFreeSize -= Index;

            /* Initialize this block */
            InUseEntry = (PHEAP_ENTRY)FreeBlock;
            InUseEntry->Flags = HEAP_ENTRY_BUSY | (FreeFlags & HEAP_ENTRY_LAST_ENTRY);
            InUseEntry->UnusedBytes = AllocationSize - Size;
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
            RtlpRemoveFreeBlock(Heap, FreeBlock, TRUE, TRUE);

            /* Split it */
            InUseEntry = RtlpSplitEntry(Heap, FreeBlock, AllocationSize, Index, Size);
        }

        /* Release the lock */
        if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);

        /* Zero memory if that was requested */
        if (Flags & HEAP_ZERO_MEMORY)
            RtlZeroMemory(InUseEntry + 1, Size);

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
            return NULL;
        }

        /* Initialize the newly allocated block */
        VirtualBlock->BusyBlock.Size = (AllocationSize - Size);
        VirtualBlock->BusyBlock.Flags = HEAP_ENTRY_VIRTUAL_ALLOC | HEAP_ENTRY_EXTRA_PRESENT | HEAP_ENTRY_BUSY;
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

    //STATUS_BUFFER_TOO_SMALL;

    /* Release the lock */
    if (HeapLocked) RtlLeaveHeapLock(Heap->LockVariable);
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
    BOOLEAN Locked = FALSE;

    /* Freeing NULL pointer is a legal operation */
    if (!Ptr) return TRUE;

    /* Get pointer to the heap and force flags */
    Heap = (PHEAP)HeapPtr;
    Flags |= Heap->ForceFlags;

    /* Lock if necessary */
    if (!(Flags & HEAP_NO_SERIALIZE))
    {
        RtlEnterHeapLock(Heap->LockVariable);
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
        // FIXME: Set STATUS_INVALID_PARAMETER

        /* Release the heap lock */
        if (Locked) RtlLeaveHeapLock(Heap->LockVariable);
        return FALSE;
    }

    if (HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
    {
        /* Big allocation */
        ASSERT(FALSE);
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
    /* We always fail growing in place now */
    return FALSE;
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
        // STATUS_SUCCESS
        return NULL;
    }

    /* Force heap flags */
    Flags |= Heap->ForceFlags;

    // Check for special heap

    /* Make sure size is valid */
    if (Size >= 0x80000000)
    {
        // STATUS_NO_MEMORY
        return NULL;
    }

    /* Calculate allocation size and index */
    if (!Size) Size = 1;
    AllocationSize = (Size + Heap->AlignRound) & Heap->AlignMask;

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
        RtlEnterHeapLock(Heap->LockVariable);
        HeapLocked = TRUE;
        Flags ^= HEAP_NO_SERIALIZE;
    }

    /* Get the pointer to the in-use entry */
    InUseEntry = (PHEAP_ENTRY)Ptr - 1;

    /* If that entry is not really in-use, we have a problem */
    if (!(InUseEntry->Flags & HEAP_ENTRY_BUSY))
    {
        // STATUS_INVALID_PARAMETER

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
            InUseEntry->Size = AllocationSize - Size;
        }
        else if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
        {
            /* There is extra stuff, take it into account */
            OldExtra = (PHEAP_ENTRY_EXTRA)(InUseEntry + InUseEntry->Size - 1);
            NewExtra = (PHEAP_ENTRY_EXTRA)(InUseEntry + Index - 1);
            *NewExtra = *OldExtra;

            // FIXME Tagging, TagIndex

            /* Update unused bytes count */
            InUseEntry->UnusedBytes = AllocationSize - Size;
        }
        else
        {
            // FIXME Tagging, SmallTagIndex
            InUseEntry->UnusedBytes = AllocationSize - Size;
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
                SplitBlock->PreviousSize = Index;
                SplitBlock->SegmentOffset = InUseEntry->SegmentOffset;

                /* Remember free size */
                FreeSize = InUseEntry->Size - Index;

                /* Set new size */
                InUseEntry->Size = Index;
                InUseEntry->Flags &= ~HEAP_ENTRY_LAST_ENTRY;

                /* Is that the last entry */
                if (FreeFlags & HEAP_ENTRY_LAST_ENTRY)
                {
                    /* Update segment's last entry */
                    Heap->Segments[SplitBlock->SegmentOffset]->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;

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
                            SplitBlock->Size = FreeSize;

                            if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY))
                            {
                                /* Update previous size of the next entry */
                                ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = FreeSize;
                            }
                            else
                            {
                                Heap->Segments[SplitBlock->SegmentOffset]->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;
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

                    UNIMPLEMENTED;
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
                        UNIMPLEMENTED;
                        NewExtra = NULL;//RtlpGetExtraStuffPointer(NewInUseEntry);

                        if (InUseEntry->Flags & HEAP_ENTRY_EXTRA_PRESENT)
                        {
                            OldExtra = NULL;//RtlpGetExtraStuffPointer(InUseEntry);
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
                        RtlZeroMemory((PCHAR)NewBaseAddress + OldSize, Size - OldSize );
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
        RtlEnterHeapLock(Heap->LockVariable);
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

    /* Lock if it's lockable */
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

    /* Force flags */
    Flags |= Heap->Flags;

    // FIXME Special heap

    /* Get the heap entry pointer */
    HeapEntry = (PHEAP_ENTRY)Ptr - 1;

    /* Return -1 if that entry is free */
    if (!(HeapEntry->Flags & HEAP_ENTRY_BUSY))
    {
        // STATUS_INVALID_PARAMETER
        return (SIZE_T)-1;
    }

    /* Get size of this block depending if it's a usual or a big one */
    if (HeapEntry->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)
    {
        // FIXME implement
        UNIMPLEMENTED;
        ASSERT(FALSE);
        EntrySize = 0;
    }
    else
    {
        /* Calculate it */
        EntrySize = (HeapEntry->Size << HEAP_ENTRY_SHIFT) - HeapEntry->UnusedBytes;
    }

    /* Return calculated size */
    return EntrySize;
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
   HANDLE Heap,
   ULONG Flags,
   PVOID Block
)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
RtlInitializeHeapManager(VOID)
{
    PPEB Peb;

    /* Get PEB */
    Peb = RtlGetCurrentPeb();

    /* Initialize heap-related fields of PEB */
    Peb->NumberOfHeaps = 0;
    Peb->MaximumNumberOfHeaps = HEAP_MAX_PROCESS_HEAPS;
    Peb->ProcessHeaps = (PVOID)RtlpProcessHeaps;

    /* Initialize the process heaps list protecting lock */
    RtlInitializeHeapLock(&RtlpProcessHeapsListLock);
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
                   HANDLE *heaps )
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
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RtlSetUserFlagsHeap(IN PVOID HeapHandle,
                    IN ULONG Flags,
                    IN PVOID BaseAddress,
                    IN ULONG UserFlags)
{
    return FALSE;
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
    UNIMPLEMENTED;
    return FALSE;
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

DWORD
NTAPI
RtlSetHeapInformation(IN HANDLE HeapHandle OPTIONAL,
                      IN HEAP_INFORMATION_CLASS HeapInformationClass,
                      IN PVOID HeapInformation,
                      IN SIZE_T HeapInformationLength)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
RtlQueryHeapInformation(HANDLE HeapHandle,
                        HEAP_INFORMATION_CLASS HeapInformationClass,
                        PVOID HeapInformation OPTIONAL,
                        SIZE_T HeapInformationLength OPTIONAL,
                        PSIZE_T ReturnLength OPTIONAL)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
RtlMultipleAllocateHeap(IN PVOID HeapHandle,
                        IN DWORD Flags,
                        IN SIZE_T Size,
                        IN DWORD Count,
                        OUT PVOID *Array)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
NTAPI
RtlMultipleFreeHeap(IN PVOID HeapHandle,
                    IN DWORD Flags,
                    IN DWORD Count,
                    OUT PVOID *Array)
{
    UNIMPLEMENTED;
    return 0;
}

/* EOF */
