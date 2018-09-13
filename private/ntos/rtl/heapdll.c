/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    heapdll.c

Abstract:

    This module implements the user mode only portions of the heap allocator.

Author:

    Steve Wood (stevewo) 20-Sep-1994

Revision History:

--*/

#include "ntrtlp.h"
#include "heap.h"
#include "heappriv.h"

#ifdef NTHEAP_ENABLED
#include "heapp.h"
#endif // NTHEAP_ENABLED


//
//  This structure is used by RtlUsageHeap to keep track of heap usage
//  between calls.  This package typecasts an extra reserved buffer passed
//  in by the user to hold this information
//

typedef struct _RTL_HEAP_USAGE_INTERNAL {
    PVOID Base;
    SIZE_T ReservedSize;
    SIZE_T CommittedSize;
    PRTL_HEAP_USAGE_ENTRY FreeList;
    PRTL_HEAP_USAGE_ENTRY LargeEntriesSentinal;
    ULONG Reserved;
} RTL_HEAP_USAGE_INTERNAL, *PRTL_HEAP_USAGE_INTERNAL;


//
//  Note that the following variables are specific to each process
//
//
//  This is a lock used to protect access the this processes heap list
//

HEAP_LOCK RtlpProcessHeapsListLock;

//
//  This is a specific list of heaps initialized and used by the process
//

#define RTLP_STATIC_HEAP_LIST_SIZE 16

PHEAP RtlpProcessHeapsListBuffer[ RTLP_STATIC_HEAP_LIST_SIZE ];

//
//  This variable stores a pointer to the heap used to storage global heap
//  tags
//

PHEAP RtlpGlobalTagHeap = NULL;

//
//  This varible is used by the process as work space to build up names for
//  pseudo tags
//

static WCHAR RtlpPseudoTagNameBuffer[ 24 ];

BOOLEAN
RtlpGrowBlockInPlace (
    IN PHEAP Heap,
    IN ULONG Flags,
    IN PHEAP_ENTRY BusyBlock,
    IN SIZE_T Size,
    IN SIZE_T AllocationIndex
    );

PVOID
RtlDebugReAllocateHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    );

BOOLEAN
RtlDebugGetUserInfoHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    OUT PVOID *UserValue OPTIONAL,
    OUT PULONG UserFlags OPTIONAL
    );

BOOLEAN
RtlDebugSetUserValueHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UserValue
    );

BOOLEAN
RtlDebugSetUserFlagsHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    );

SIZE_T
RtlDebugCompactHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

NTSTATUS
RtlDebugCreateTagHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PWSTR TagPrefix OPTIONAL,
    IN PWSTR TagNames
    );

PWSTR
RtlDebugQueryTagHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN USHORT TagIndex,
    IN BOOLEAN ResetCounters,
    OUT PRTL_HEAP_TAG_INFO TagInfo OPTIONAL
    );

NTSTATUS
RtlDebugUsageHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN OUT PRTL_HEAP_USAGE Usage
    );

BOOLEAN
RtlDebugWalkHeap (
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    );

PHEAP_TAG_ENTRY
RtlpAllocateTags (
    PHEAP Heap,
    ULONG NumberOfTags
    );

PRTL_HEAP_USAGE_ENTRY
RtlpFreeHeapUsageEntry (
    PRTL_HEAP_USAGE_INTERNAL Buffer,
    PRTL_HEAP_USAGE_ENTRY p
    );

NTSTATUS
RtlpAllocateHeapUsageEntry (
    PRTL_HEAP_USAGE_INTERNAL Buffer,
    PRTL_HEAP_USAGE_ENTRY *pp
    );

//
//  Declared in ntrtl.h
//

NTSTATUS
RtlInitializeHeapManager(
    VOID
    )

/*++

Routine Description:

    This routine is used to initialize the heap manager for the current process

Arguments:

    None.

Return Value:

    None.

--*/

{
    PPEB Peb = NtCurrentPeb();

#if DBG

    //
    //  Sanity check the sizes of the header entry structures
    //

    if (sizeof( HEAP_ENTRY ) != sizeof( HEAP_ENTRY_EXTRA )) {

        HeapDebugPrint(( "Heap header and extra header sizes disagree\n" ));

        HeapDebugBreak( NULL );
    }

    if (sizeof( HEAP_ENTRY ) != CHECK_HEAP_TAIL_SIZE) {

        HeapDebugPrint(( "Heap header and tail fill sizes disagree\n" ));

        HeapDebugBreak( NULL );
    }

    if (sizeof( HEAP_FREE_ENTRY ) != (2 * sizeof( HEAP_ENTRY ))) {

        HeapDebugPrint(( "Heap header and free header sizes disagree\n" ));

        HeapDebugBreak( NULL );
    }

#endif // DBG

    //
    //  Initialize the heap specific structures in the current peb
    //

    Peb->NumberOfHeaps = 0;
    Peb->MaximumNumberOfHeaps = RTLP_STATIC_HEAP_LIST_SIZE;
    Peb->ProcessHeaps = RtlpProcessHeapsListBuffer;

#ifdef NTHEAP_ENABLED
    {
        (VOID) RtlInitializeNtHeapManager();
    }
#endif // NTHEAP_ENABLED

    //
    //  Initialize the lock and return to our caller
    //

    return RtlInitializeLockRoutine( &RtlpProcessHeapsListLock.Lock );
}


//
//  Declared in ntrtl.h
//

VOID
RtlProtectHeap (
    IN PVOID HeapHandle,
    IN BOOLEAN MakeReadOnly
    )

/*++

Routine Description:

    This routine will change the protection on all the pages in a heap
    to be either readonly or readwrite

Arguments:

    HeapHandle - Supplies a pointer to the heap being altered

    MakeReadOnly - Specifies if the heap is to be made readonly or
        readwrite

Return Value:

    None.

--*/

{
    PHEAP Heap;
    UCHAR SegmentIndex;
    PHEAP_SEGMENT Segment;
    MEMORY_BASIC_INFORMATION VaInfo;
    NTSTATUS Status;
    PVOID Address;
    PVOID ProtectAddress;
    SIZE_T Size;
    ULONG OldProtect;
    ULONG NewProtect;

    Heap = (PHEAP)HeapHandle;

    //
    //  For every valid segment in the heap we will zoom through all its
    //  regions and for those that are committed we'll change it protection
    //

    for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

        Segment = Heap->Segments[ SegmentIndex ];

        if ( Segment ) {

            //
            //  Starting from the first address for the segment and going to
            //  the last address in the segment we'll step through by regions
            //

            Address = Segment->BaseAddress;

            while ((ULONG_PTR)Address < (ULONG_PTR)(Segment->LastValidEntry)) {

                //
                //  Query the current region to get its state and size
                //

                Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                               Address,
                                               MemoryBasicInformation,
                                               &VaInfo,
                                               sizeof(VaInfo),
                                               NULL );

                if (!NT_SUCCESS( Status )) {

                    HeapDebugPrint(( "VirtualQuery Failed 0x%08x %x\n", Address, Status ));

                    return;
                }

                //
                //  If we found a commited block then set its protection
                //

                if (VaInfo.State == MEM_COMMIT) {

                    Size = VaInfo.RegionSize;

                    ProtectAddress = Address;

                    if (MakeReadOnly) {

                        NewProtect = PAGE_READONLY;

                    } else {

                        NewProtect = PAGE_READWRITE;
                    }

                    Status = ZwProtectVirtualMemory( NtCurrentProcess(),
                                                     &ProtectAddress,
                                                     &Size,
                                                     NewProtect,
                                                     &OldProtect );

                    if (!NT_SUCCESS( Status )) {

                        HeapDebugPrint(( "VirtualProtect Failed 0x%08x %x\n", Address, Status ));

                        return;
                    }
                }

                //
                //  Now calculate the address of the next region in the segment
                //

                Address = (PVOID)((PCHAR)Address + VaInfo.RegionSize);
            }
        }
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Declared in nturtl.h
//

BOOLEAN
RtlLockHeap (
    IN PVOID HeapHandle
    )

/*++

Routine Description:

    This routine is used by lock access to a specific heap structure

Arguments:

    HeapHandle - Supplies a pointer to the heap being locked

Return Value:

    BOOLEAN - TRUE if the heap is now locked and FALSE otherwise (i.e.,
        the heap is ill-formed).  TRUE is returned even if the heap is
        not lockable.

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;

    RTL_PAGED_CODE();

    //
    //  Check for the heap protected by guard pages
    //

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapLock( HeapHandle ));

    //
    //  Validate that HeapAddress points to a HEAP structure.
    //

    if (!RtlpCheckHeapSignature( Heap, "RtlLockHeap" )) {

        return FALSE;
    }

    //
    //  Lock the heap.  And disable the lookaside list by incrementing
    //  its lock count.
    //

    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {

        RtlAcquireLockRoutine( Heap->LockVariable );

        Heap->LookasideLockCount += 1;
    }

    return TRUE;
}


//
//  Declared in nturtl.h
//

BOOLEAN
RtlUnlockHeap (
    IN PVOID HeapHandle
    )

/*++

Routine Description:

    This routine is used to unlock access to a specific heap structure

Arguments:

    HeapHandle - Supplies a pointer to the heep being unlocked

Return Value:

    BOOLEAN - TRUE if the heap is now unlocked and FALSE otherwise (i.e.,
        the heap is ill-formed).  TRUE is also returned if the heap was
        never locked to begin with because it is not seralizable.

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;

    RTL_PAGED_CODE();

    //
    //  Check for the heap protected by guard pages
    //

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapUnlock( HeapHandle ));

    //
    //  Validate that HeapAddress points to a HEAP structure.
    //

    if (!RtlpCheckHeapSignature( Heap, "RtlUnlockHeap" )) {

        return FALSE;
    }

    //
    //  Unlock the heap.  And enable the lookaside logic by decrementing
    //  its lock count
    //

    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {

        Heap->LookasideLockCount -= 1;

        RtlReleaseLockRoutine( Heap->LockVariable );
    }

    return TRUE;
}


//
//  Declared in nturtl.h
//

PVOID
RtlReAllocateHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    )

/*++

Routine Description:

    This routine will resize a user specified heap block.  The new size
    can either be smaller or larger than the current block size.

Arguments:

    HeapHandle - Supplies a pointer to the heap being modified

    Flags - Supplies a set of heap flags to augment those already
        enforced by the heap

    BaseAddress - Supplies the current address of a block allocated
        from heap.  We will try and resize this block at its current
        address, but it could possibly move if this heap structure
        allows for relocation

    Size - Supplies the size, in bytes, for the newly resized heap
        block

Return Value:

    PVOID - A pointer to the resized block.  If the block had to move
        then this address will not be equal to the input base address

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    SIZE_T AllocationSize;
    PHEAP_ENTRY BusyBlock, NewBusyBlock;
    PHEAP_ENTRY_EXTRA OldExtraStuff, NewExtraStuff;
    SIZE_T FreeSize;
    BOOLEAN LockAcquired = FALSE;
    PVOID NewBaseAddress;
    PHEAP_FREE_ENTRY SplitBlock, SplitBlock2;
    SIZE_T OldSize;
    SIZE_T AllocationIndex;
    SIZE_T OldAllocationIndex;
    UCHAR FreeFlags;
    NTSTATUS Status;
    PVOID DeCommitAddress;
    SIZE_T DeCommitSize;
    EXCEPTION_RECORD ExceptionRecord;

    //
    //  If there isn't an address to relocate the heap at then our work is done
    //

    if (BaseAddress == NULL) {

        SET_LAST_STATUS( STATUS_SUCCESS );

        return NULL;
    }

#ifdef NTHEAP_ENABLED
    {
        if (Heap->Flags & NTHEAP_ENABLED_FLAG) {

            return RtlReAllocateNtHeap( HeapHandle, Flags, BaseAddress, Size );
        }
    }
#endif // NTHEAP_ENABLED

    //
    //  Augment the heap flags
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check if we should simply call the debug version of heap to do the work
    //

    if (DEBUG_HEAP( Flags)) {

        return RtlDebugReAllocateHeap( HeapHandle, Flags, BaseAddress, Size );
    }

    //
    //  Make sure we didn't get a negative heap size
    //

    if (Size > 0x7fffffff) {

        SET_LAST_STATUS( STATUS_NO_MEMORY );

        return NULL;
    }

    //
    //  Round the requested size up to the allocation granularity.  Note
    //  that if the request is for 0 bytes, we still allocate memory, because
    //  we add in an extra byte to protect ourselves from idiots.
    //

    AllocationSize = ((Size ? Size : 1) + Heap->AlignRound) & Heap->AlignMask;

    if ((Flags & HEAP_NEED_EXTRA_FLAGS) ||
        (Heap->PseudoTagEntries != NULL) ||
        ((((PHEAP_ENTRY)BaseAddress)-1)->Flags & HEAP_ENTRY_EXTRA_PRESENT)) {

        AllocationSize += sizeof( HEAP_ENTRY_EXTRA );
    }

    try {

        //
        //  Lock the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;

            //
            //  Because it is now zero the following statement will set the no
            //  serialize bit
            //

            Flags ^= HEAP_NO_SERIALIZE;
        }

        try {

            //
            //  Compute the heap block address for user specified block
            //

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            //
            //  Check if the block is not in use then it is an error
            //

            if (!(BusyBlock->Flags & HEAP_ENTRY_BUSY)) {

                SET_LAST_STATUS( STATUS_INVALID_PARAMETER );

                //
                //  Bail if not a busy block.
                //

                leave;

            //
            //  We need the current (i.e., old) size and allocation of the
            //  block.  Check if the block is a big allocation.  The size
            //  field of a big block is really the unused by count
            //

            } else if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

                OldSize = RtlpGetSizeOfBigBlock( BusyBlock );

                OldAllocationIndex = (OldSize + BusyBlock->Size) >> HEAP_GRANULARITY_SHIFT;

                //
                //  We'll need to adjust the new allocation size to account
                //  for the big block header and then round it up to a page
                //

                AllocationSize += FIELD_OFFSET( HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );
                AllocationSize = ROUND_UP_TO_POWER2( AllocationSize, PAGE_SIZE );

            //
            //  Otherwise the block is in use and is a small allocation
            //

            } else {

                OldAllocationIndex = BusyBlock->Size;

                OldSize = (OldAllocationIndex << HEAP_GRANULARITY_SHIFT) -
                          BusyBlock->UnusedBytes;
            }

            //
            //  Compute the new allocation index
            //

            AllocationIndex = AllocationSize >> HEAP_GRANULARITY_SHIFT;

            //
            //  At this point we have the old size and index, and the new size
            //  and index
            //
            //  See if new size less than or equal to the current size.
            //

            if (AllocationIndex <= OldAllocationIndex) {

                //
                //  If the new allocation index is only one less then the current
                //  index then make the sizes equal
                //

                if (AllocationIndex + 1 == OldAllocationIndex) {

                    AllocationIndex += 1;
                    AllocationSize += sizeof( HEAP_ENTRY );
                }

                //
                //  Calculate new residual (unused) amount
                //

                if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

                    //
                    //  In a big block the size is really the unused byte count
                    //

                    BusyBlock->Size = (USHORT)(AllocationSize - Size);

                } else if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                    //
                    //  The extra stuff struct goes after the data.  So compute
                    //  the old and new extra stuff location and copy the data
                    //

                    OldExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + BusyBlock->Size - 1);

                    NewExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + AllocationIndex - 1);

                    *NewExtraStuff = *OldExtraStuff;

                    //
                    //  If we're doing heap tagging then update the tag entry
                    //

                    if (IS_HEAP_TAGGING_ENABLED()) {

                        NewExtraStuff->TagIndex =
                            RtlpUpdateTagEntry( Heap,
                                                NewExtraStuff->TagIndex,
                                                OldAllocationIndex,
                                                AllocationIndex,
                                                ReAllocationAction );
                    }

                    BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);

                } else {

                    //
                    //  If we're doing heap tagging then update the tag entry
                    //

                    if (IS_HEAP_TAGGING_ENABLED()) {

                        BusyBlock->SmallTagIndex = (UCHAR)
                            RtlpUpdateTagEntry( Heap,
                                                BusyBlock->SmallTagIndex,
                                                BusyBlock->Size,
                                                AllocationIndex,
                                                ReAllocationAction );
                    }

                    BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);
                }

                //
                //  Check if the block is getting bigger, then fill in the extra
                //  space.
                //
                //  **** how can this happen if the allocation index is less than or
                //  **** equal to the old allocation index
                //

                if (Size > OldSize) {

                    //
                    //  See if we should zero the extra space
                    //

                    if (Flags & HEAP_ZERO_MEMORY) {

                        RtlZeroMemory( (PCHAR)BaseAddress + OldSize,
                                       Size - OldSize );

                    //
                    //  Otherwise see if we should fill the extra space
                    //

                    } else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED) {

                        SIZE_T PartialBytes, ExtraSize;

                        PartialBytes = OldSize & (sizeof( ULONG ) - 1);

                        if (PartialBytes) {

                            PartialBytes = 4 - PartialBytes;
                        }

                        if (Size > (OldSize + PartialBytes)) {

                            ExtraSize = (Size - (OldSize + PartialBytes)) & ~(sizeof( ULONG ) - 1);

                            if (ExtraSize != 0) {

                                RtlFillMemoryUlong( (PCHAR)(BusyBlock + 1) + OldSize + PartialBytes,
                                                    ExtraSize,
                                                    ALLOC_HEAP_FILL );
                            }
                        }
                    }
                }

                if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {

                    RtlFillMemory( (PCHAR)(BusyBlock + 1) + Size,
                                   CHECK_HEAP_TAIL_SIZE,
                                   CHECK_HEAP_TAIL_FILL );
                }

                //
                //  If amount of change is greater than the size of a free block,
                //  then need to free the extra space.  Otherwise, nothing else to
                //  do.
                //

                if (AllocationIndex != OldAllocationIndex) {

                    FreeFlags = BusyBlock->Flags & ~HEAP_ENTRY_BUSY;

                    if (FreeFlags & HEAP_ENTRY_VIRTUAL_ALLOC) {

                        PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

                        VirtualAllocBlock = CONTAINING_RECORD( BusyBlock, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

                        if (IS_HEAP_TAGGING_ENABLED()) {

                            VirtualAllocBlock->ExtraStuff.TagIndex =
                                RtlpUpdateTagEntry( Heap,
                                                    VirtualAllocBlock->ExtraStuff.TagIndex,
                                                    OldAllocationIndex,
                                                    AllocationIndex,
                                                    VirtualReAllocationAction );
                        }

                        DeCommitAddress = (PCHAR)VirtualAllocBlock + AllocationSize;

                        DeCommitSize = (OldAllocationIndex << HEAP_GRANULARITY_SHIFT) -
                                       AllocationSize;

                        Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                                      (PVOID *)&DeCommitAddress,
                                                      &DeCommitSize,
                                                      MEM_RELEASE );

                        if (!NT_SUCCESS( Status )) {

                            HeapDebugPrint(( "Unable to release memory at %p for %p bytes - Status == %x\n",
                                             DeCommitAddress, DeCommitSize, Status ));

                            HeapDebugBreak( NULL );

                        } else {

                            VirtualAllocBlock->CommitSize -= DeCommitSize;
                        }

                    } else {

                        //
                        //  Otherwise, shrink size of this block to new size, and make extra
                        //  space at end free.
                        //

                        SplitBlock = (PHEAP_FREE_ENTRY)(BusyBlock + AllocationIndex);

                        SplitBlock->Flags = FreeFlags;

                        SplitBlock->PreviousSize = (USHORT)AllocationIndex;

                        SplitBlock->SegmentIndex = BusyBlock->SegmentIndex;

                        FreeSize = BusyBlock->Size - AllocationIndex;

                        BusyBlock->Size = (USHORT)AllocationIndex;

                        BusyBlock->Flags &= ~HEAP_ENTRY_LAST_ENTRY;

                        //
                        //  If the following block is uncommitted then we only need to
                        //  add this new entry to its free list
                        //

                        if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {

                            PHEAP_SEGMENT Segment;

                            Segment = Heap->Segments[SplitBlock->SegmentIndex];
                            Segment->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;

                            SplitBlock->Size = (USHORT)FreeSize;

                            RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                            Heap->TotalFreeSize += FreeSize;

                        } else {

                            //
                            //  Otherwise get the next block and check if it is busy.  If it
                            //  is in use then add this new entry to its free list
                            //

                            SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);

                            if (SplitBlock2->Flags & HEAP_ENTRY_BUSY) {

                                SplitBlock->Size = (USHORT)FreeSize;

                                ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;

                                RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                                Heap->TotalFreeSize += FreeSize;

                            } else {

                                //
                                //  Otherwise the next block is not in use so we
                                //  should be able to merge with it.  Remove the
                                //  second free block and if the combined size is
                                //  still okay then merge the two blocks and add
                                //  the single block back in.  Otherwise call a
                                //  routine that will actually break it apart
                                //  before insertion.
                                //

                                SplitBlock->Flags = SplitBlock2->Flags;

                                RtlpRemoveFreeBlock( Heap, SplitBlock2 );

                                Heap->TotalFreeSize -= SplitBlock2->Size;

                                FreeSize += SplitBlock2->Size;

                                if (FreeSize <= HEAP_MAXIMUM_BLOCK_SIZE) {

                                    SplitBlock->Size = (USHORT)FreeSize;

                                    if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                                        ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;

                                    } else {

                                        PHEAP_SEGMENT Segment;

                                        Segment = Heap->Segments[SplitBlock->SegmentIndex];
                                        Segment->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;
                                    }

                                    RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                                    Heap->TotalFreeSize += FreeSize;

                                } else {

                                    RtlpInsertFreeBlock( Heap, SplitBlock, FreeSize );
                                }
                            }
                        }
                    }
                }

            } else {

                //
                //  At this point the new size is greater than the current size
                //
                //  If the block is a big allocation or we're not able to grow
                //  the block in place then we have a lot of work to do
                //

                if ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) ||
                    !RtlpGrowBlockInPlace( Heap, Flags, BusyBlock, Size, AllocationIndex )) {

                    //
                    //  We're growing the block.  Allocate a new block with the bigger
                    //  size, copy the contents of the old block to the new block and then
                    //  free the old block.  Return the address of the new block.
                    //

                    if (Flags & HEAP_REALLOC_IN_PLACE_ONLY) {

#if DBG
                        // HeapDebugPrint(( "Failing ReAlloc because cant do it inplace.\n" ));
#endif

                        BaseAddress = NULL;

                    } else {

                        //
                        //  Clear the tag bits from the flags
                        //

                        Flags &= ~HEAP_TAG_MASK;

                        //
                        //  If there is an extra struct present then get the tag
                        //  index from the extra stuff and augment the flags with
                        //  the tag index.
                        //

                        if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                            Flags &= ~HEAP_SETTABLE_USER_FLAGS;

                            Flags |= HEAP_SETTABLE_USER_VALUE |
                                     ((BusyBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS) << 4);

                            OldExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

                            try {

                                if ((OldExtraStuff->TagIndex != 0) &&
                                    !(OldExtraStuff->TagIndex & HEAP_PSEUDO_TAG_FLAG)) {

                                    Flags |= OldExtraStuff->TagIndex << HEAP_TAG_SHIFT;
                                }

                            } except (EXCEPTION_EXECUTE_HANDLER) {

                                BusyBlock->Flags &= ~HEAP_ENTRY_EXTRA_PRESENT;
                            }

                        } else if (BusyBlock->SmallTagIndex != 0) {

                            //
                            //  There is not an extra stuff struct, but block
                            //  does have a small tag index so now add this small
                            //  tag to the flags
                            //

                            Flags |= BusyBlock->SmallTagIndex << HEAP_TAG_SHIFT;
                        }

                        //
                        //  Allocate from the heap space for the reallocation
                        //

                        NewBaseAddress = RtlAllocateHeap( HeapHandle,
                                                          Flags & ~HEAP_ZERO_MEMORY,
                                                          Size );

                        if (NewBaseAddress != NULL) {

                            //
                            //  We were able to get the allocation so now back up
                            //  to the heap block and if the block has an extra
                            //  stuff struct then copy over the extra stuff
                            //

                            NewBusyBlock = (PHEAP_ENTRY)NewBaseAddress - 1;

                            if (NewBusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                                NewExtraStuff = RtlpGetExtraStuffPointer( NewBusyBlock );

                                if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                                    OldExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

                                    NewExtraStuff->Settable = OldExtraStuff->Settable;

                                } else {

                                    RtlZeroMemory( NewExtraStuff, sizeof( *NewExtraStuff ));
                                }
                            }

                            //
                            //  Copy over the user's data area to the new block
                            //

                            RtlMoveMemory( NewBaseAddress, BaseAddress, Size < OldSize ? Size : OldSize );

                            //
                            //  Check if we grew the block and we should zero
                            //  the remaining part.
                            //
                            //  **** is this first test always true because we're
                            //  **** in the part that grows blocks
                            //

                            if (Size > OldSize && (Flags & HEAP_ZERO_MEMORY)) {

                                RtlZeroMemory( (PCHAR)NewBaseAddress + OldSize,
                                               Size - OldSize );
                            }

                            //
                            //  Release the old block
                            //

                            RtlFreeHeap( HeapHandle,
                                         Flags,
                                         BaseAddress );
                        }

                        BaseAddress = NewBaseAddress;
                    }
                }
            }

            if ((BaseAddress == NULL) && (Flags & HEAP_GENERATE_EXCEPTIONS)) {

                //
                //  Construct an exception record.
                //

                ExceptionRecord.ExceptionCode = STATUS_NO_MEMORY;
                ExceptionRecord.ExceptionRecord = (PEXCEPTION_RECORD)NULL;
                ExceptionRecord.NumberParameters = 1;
                ExceptionRecord.ExceptionFlags = 0;
                ExceptionRecord.ExceptionInformation[ 0 ] = AllocationSize;

                RtlRaiseException( &ExceptionRecord );
            }

        } except( GetExceptionCode() == STATUS_NO_MEMORY ? EXCEPTION_CONTINUE_SEARCH :
                                                           EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
            BaseAddress = NULL;

        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller
    //

    return BaseAddress;
}


//
//  Declared in nturtl.h
//

BOOLEAN
RtlGetUserInfoHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    OUT PVOID *UserValue OPTIONAL,
    OUT PULONG UserFlags OPTIONAL
    )

/*++

Routine Description:

    This routine returns to the user the set of user flags
    and user values for the specified heap entry.  The user value
    is set via a set call and the user flags is part of the
    user settable flags used when communicating with the heap package
    and can also be set via a set call

Arguments:

    HeapHandle - Supplies a pointer to the heap being queried

    Flags - Supplies a set of flags to agument those already in the heap

    BaseAddress - Supplies a pointer to the users heap entry being
        queried

    UserValue - Optionally supplies a pointer to recieve the heap entry
        value

    UserFlasg - Optionally supplies a pointer to recieve the heap flags

Return Value:

    BOOLEAN - TRUE if the query is successful and FALSE otherwise

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    BOOLEAN LockAcquired = FALSE;
    BOOLEAN Result;

    //
    //  Build up a set of real flags to use in this operation
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check if we should be going the debug route
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugGetUserInfoHeap( HeapHandle, Flags, BaseAddress, UserValue, UserFlags );
    }

    Result = FALSE;

    try {

        try {

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;
            }

            //
            //  Backup the pointer to the heap entry
            //

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            //
            //  If the entry is not in use then it is an error
            //

            if (!(BusyBlock->Flags & HEAP_ENTRY_BUSY)) {

                SET_LAST_STATUS( STATUS_INVALID_PARAMETER );

            } else {

                //
                //  The heap entry is in use so now check if there is
                //  any extra information present
                //

                if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                    //
                    //  Get a pointer to the extra information and if the
                    //  user asked for user values then that field from the
                    //  extra stuff
                    //

                    ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

                    if (ARGUMENT_PRESENT( UserValue )) {

                        *UserValue = (PVOID)ExtraStuff->Settable;
                    }
                }

                //
                //  If the user asked for user flags then return the flags
                //  from the heap entry that are user setable
                //

                if (ARGUMENT_PRESENT( UserFlags )) {

                    *UserFlags = (BusyBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS) << 4;
                }

                //
                //  Now that the assignments are done we can say that
                //  we were successful
                //

                Result = TRUE;
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );

            Result = FALSE;
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Declared in nturtl.h
//

BOOLEAN
RtlSetUserValueHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UserValue
    )

/*++

Routine Description:

    This routine is used to set the user settable value for a heap entry

Arguments:

    HeapHandle - Supplies a pointer to the heap being modified

    Flags - Supplies a set of flags needed to augment those already enforced
        by the heap

    BaseAddress - Supplies a pointer to the heap entry allocation being
        modified

    UserValue - Supplies the value to store in the extra stuff space of
        the heap entry

Return Value:

    BOOLEAN - TRUE if the setting worked, and FALSE otherwise.  It could be
        FALSE if the base address is invalid, or if there is not room for
        the extra stuff

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    BOOLEAN LockAcquired = FALSE;
    BOOLEAN Result;

    //
    //  Augment the set of flags
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check to see if we should be going the debug route
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugSetUserValueHeap( HeapHandle, Flags, BaseAddress, UserValue );
    }

    Result = FALSE;

    try {

        //
        //  Lock the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        //
        //  Get a pointer to the owning heap entry
        //

        BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

        //
        //  If the entry is not in use then its is an error
        //

        if (!(BusyBlock->Flags & HEAP_ENTRY_BUSY)) {

            SET_LAST_STATUS( STATUS_INVALID_PARAMETER );

        //
        //  Otherwise we only can set the value if the entry has space
        //  for the extra stuff
        //

        } else if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

            ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

            ExtraStuff->Settable = (ULONG_PTR)UserValue;

            Result = TRUE;
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Declared in nturtl.h
//

BOOLEAN
RtlSetUserFlagsHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    )

/*++

Routine Description:

    HeapHandle - Supplies a pointer to the heap being modified

    Flags - Supplies a set of flags needed to augment those already enforced
        by the heap

    BaseAddress - Supplies a pointer to the heap entry allocation being
        modified

    UserFlagsReset - Supplies a mask of flags that the user wants cleared

    UserFlagsSet- Supplies a mask of flags that the user wants set

Return Value:

    BOOLEAN - TRUE if the operation is a success and FALSE otherwise

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN LockAcquired = FALSE;
    BOOLEAN Result = FALSE;

    //
    //  Augment the set of flags
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check to see if we should be going the debug route
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugSetUserFlagsHeap( HeapHandle, Flags, BaseAddress, UserFlagsReset, UserFlagsSet );
    }

    try {

        //
        //  Lock the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        try {

            //
            //  Get a pointer to the owning heap entry
            //

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            //
            //  If the entry is not in use then it is an error
            //

            if (!(BusyBlock->Flags & HEAP_ENTRY_BUSY)) {

                SET_LAST_STATUS( STATUS_INVALID_PARAMETER );

            } else {

                //
                //  Otherwise modify the flags in the block
                //
                //  **** this is terrible error prone if the user passes in
                //  **** flags that aren't 0x200 0x400 or 0x800 only.
                //

                BusyBlock->Flags &= ~(UserFlagsReset >> 4);
                BusyBlock->Flags |= (UserFlagsSet >> 4);

                Result = TRUE;
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );

            Result = FALSE;
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Result;
}


//
//  Declared in nturtl.h
//

ULONG
RtlCreateTagHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PWSTR TagPrefix OPTIONAL,
    IN PWSTR TagNames
    )

/*++

Routine Description:

    This routine create a tag heap for either the specified heap or
    for the global tag heap.

Arguments:

    HeapHandle - Optionally supplies a pointer to the heap that we
        want modified.  If null then the global tag heap is used

    Flags - Supplies a list of flags to augment the flags already
        enforced by the heap

    TagPrefix - Optionally supplies a null terminated wchar string
        of a prefix to add to each tag

    TagNames - Supplies a list of tag names separated by null and terminated
        by a double null.  If the first name in the list start with
        a "!" then it is interpreted as the heap name.  The syntax
        for the tag name is

            [!<heapname> nul ] {<tagname> nul}* nul

Return Value:

    ULONG - returns the index of the last tag create shifted to the high
        order word.

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    ULONG TagIndex;
    ULONG NumberOfTags, MaxTagNameLength, TagPrefixLength;
    PWSTR s, s1, HeapName;
    PHEAP_TAG_ENTRY TagEntry;
    ULONG Result;

    //
    //  Check if tagging is disable and so this call is a noop
    //

    if (!IS_HEAP_TAGGING_ENABLED()) {

        return 0;
    }

    //
    //  If the processes global tag heap has not been created yet then
    //  allocate a global tag heap
    //

    if (RtlpGlobalTagHeap == NULL) {

        RtlpGlobalTagHeap = RtlAllocateHeap( RtlProcessHeap( ), HEAP_ZERO_MEMORY, sizeof( HEAP ));

        if (RtlpGlobalTagHeap == NULL) {

            return 0;
        }
    }

    try {

        //
        //  If the user passed in a heap then we'll use the lock from that
        //  heap to synchronize our work.  Otherwise we're unsynchronized
        //

        if (Heap != NULL) {

            //
            //  Tagging is not part of the guard page heap package
            //

            IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle, 0 );

            //
            //  Check if we should be calling the debug version of the heap package
            //

            if (DEBUG_HEAP( Flags )) {

                Result = RtlDebugCreateTagHeap( HeapHandle, Flags, TagPrefix, TagNames );
                leave;
            }

            //
            //  Augment the flags and lock the specified heap
            //

            Flags |= Heap->ForceFlags;

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;
            }
        }

        //
        //  We start off with zero tags
        //

        TagIndex = 0;
        NumberOfTags = 0;

        //
        //  With tag names that start with "!" we assume what follows
        //  is a heap name.
        //

        if (*TagNames == L'!') {

            HeapName = TagNames + 1;

            //
            //  Move up to the following tag name after the heap name
            //  separated by a null
            //

            while (*TagNames++) { NOTHING; }

        } else {

            HeapName = NULL;
        }

        //
        //  Gobble up each tag name keeping count of how many we find
        //

        s = TagNames;

        while (*s) {

            while (*s++) { NOTHING; }

            NumberOfTags += 1;
        }

        //
        //  Now we will only continue on if we were supplied tag names
        //

        if (NumberOfTags > 0) {

            //
            //  Allocate heap entries for the number of tags we need and
            //  only proceed if this allocation succeeded.   The following
            //  call also makes room for the heap name as tag index 0.  Note
            //  that is heap is null then we assume we're using the global
            //  tag heap
            //

            TagEntry = RtlpAllocateTags( Heap, NumberOfTags );

            if (TagEntry != NULL) {

                MaxTagNameLength = (sizeof( TagEntry->TagName ) / sizeof( WCHAR )) - 1;

                TagIndex = TagEntry->TagIndex;

                //
                //  If the first tag index is zero then we'll make this tag entry
                //  the heap name.
                //

                if (TagIndex == 0) {

                    if (HeapName != NULL ) {

                        //
                        //  Copy over the heap name and pad it out with nulls up
                        //  to the end of the name buffer
                        //

                        wcsncpy( TagEntry->TagName, HeapName, MaxTagNameLength );
                    }

                    //
                    //  Whether we add a heap name or not we'll move on to the
                    //  next tag entry and index
                    //

                    TagEntry += 1;

                    TagIndex = TagEntry->TagIndex;

                //
                //  This isn't the first index for a specified heap, but see if
                //  it is the first index for the global heap.  If so then put
                //  name of the global tags into the 0 index
                //

                } else if (TagIndex == HEAP_GLOBAL_TAG) {

                    wcsncpy( TagEntry->TagName, L"GlobalTags", MaxTagNameLength );

                    TagEntry += 1;

                    TagIndex = TagEntry->TagIndex;
                }

                //
                //  Now we've taken case of the 0 index we'll go on to the rest of
                //  the tags.  If there is tag prefix and it is not zero length
                //  then we'll use this tag prefix provided that is leaves us at
                //  least 4 characters for the tag name itself.  Otherwise we'll
                //  ignore the tag prefix (by setting the variable to null).
                //

                if ((ARGUMENT_PRESENT( TagPrefix )) &&
                    (TagPrefixLength = wcslen( TagPrefix ))) {

                    if (TagPrefixLength >= MaxTagNameLength-4) {

                        TagPrefix = NULL;

                    } else {

                        MaxTagNameLength -= TagPrefixLength;
                    }

                } else {

                    TagPrefix = NULL;
                }

                //
                //  For every tag name (note that this varable has already been
                //  advanced beyond the heap name) we'll put it in a tag entry
                //  by copying in the prefix and then appending on the tag itself
                //
                //   s points to the current users supplied tag name
                //  s1 points to the tag name buffer in the current tag entry
                //

                s = TagNames;

                while (*s) {

                    s1 = TagEntry->TagName;

                    //
                    //  Copy in the optional tag prefix and update s1
                    //

                    if (ARGUMENT_PRESENT( TagPrefix )) {

                        wcscpy( s1, TagPrefix );

                        s1 += TagPrefixLength;
                    }

                    //
                    //  Copy over the remaining tag name padding it with nulls
                    //  up to the end of the name buffer
                    //

                    wcsncpy( s1, s, MaxTagNameLength );

                    //
                    //  Skip to the next tag name
                    //

                    while (*s++) { NOTHING; }

                    //
                    //  Skip to the next tag entry
                    //

                    TagEntry += 1;
                }
            }
        }

        Result = TagIndex << HEAP_TAG_SHIFT;

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller.  The answer we return is the last tag index
    //  stored in the high word of a ulong result
    //

    return Result;
}


//
//  Declared in nturtl.h
//

PWSTR
RtlQueryTagHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN USHORT TagIndex,
    IN BOOLEAN ResetCounters,
    OUT PRTL_HEAP_TAG_INFO TagInfo OPTIONAL
    )

/*++

Routine Description:

    This routine returns the name and optional statistics for a given
    tag index.

Arguments:

        **** note that some of the code looks like it can handle the
        **** global tag heap but other places look rather wrong

    HeapHandle - Specifies the heap being queried.  If null then the
        global tag heap is used.

    Flags - Supplies a set flags to augment those enforced by the
        heap

    TagIndex - Specifies the tag index that we want to query

    ResetCounter - Specifies if this routine should reset the counter
        for the tag after the query

    TagInfo - Optionally supplies storage where the output tag information
        should be stored

Return Value:

    PWSTR - Returns a pointer to the tag name or NULL if the index
        doesn't exist

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    PHEAP_TAG_ENTRY TagEntry;
    PWSTR Result;

    //
    //  Tagging is not part of the guard page heap package
    //

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle, NULL );

    //
    //  Check if tagging is disabled
    //

    if (!IS_HEAP_TAGGING_ENABLED()) {

        return NULL;
    }

    try {

        //
        //  Check if the caller has given us a heap to query
        //

        if (Heap != NULL) {

            //
            //  Check if we should be using the debug version of the
            //  heap package
            //

            if (DEBUG_HEAP( Flags )) {

                Result = RtlDebugQueryTagHeap( HeapHandle, Flags, TagIndex, ResetCounters, TagInfo );
                leave;
            }

            //
            //  Lock the heap
            //

            Flags |= Heap->ForceFlags;

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;
            }
        }

        Result = NULL;

        //
        //  **** note that the next test assumes that heap is not null
        //
        //  Check that the specified tag index is valid and that the
        //  this heap does actually have some tag entries
        //

        if ((TagIndex < Heap->NextAvailableTagIndex) &&
            (Heap->TagEntries != NULL)) {

            //
            //  Stride over to the specific tag entry and if the caller gave us
            //  an output buffer then fill in the details
            //

            TagEntry = Heap->TagEntries + TagIndex;

            if (ARGUMENT_PRESENT( TagInfo )) {

                TagInfo->NumberOfAllocations = TagEntry->Allocs;
                TagInfo->NumberOfFrees = TagEntry->Frees;
                TagInfo->BytesAllocated = TagEntry->Size << HEAP_GRANULARITY_SHIFT;
            }

            //
            //  Check if we should reset the counters
            //

            if (ResetCounters) {

                TagEntry->Allocs = 0;
                TagEntry->Frees = 0;
                TagEntry->Size = 0;
            }

            //
            //  Point to the tag name
            //

            Result = &TagEntry->TagName[ 0 ];

        //
        //  If the tag index has the psuedo tag bit set then recalulate the
        //  tag index and if this heap has pseudo tags than that is what
        //  we'll return
        //

        } else if (TagIndex & HEAP_PSEUDO_TAG_FLAG) {

            //
            //  Clear the bit
            //

            TagIndex ^= HEAP_PSEUDO_TAG_FLAG;

            if ((TagIndex < HEAP_NUMBER_OF_PSEUDO_TAG) &&
                (Heap->PseudoTagEntries != NULL)) {

                //
                //  Stride over to the specific pseudo tag entry and if the
                //  caller gave us an output buffer then fill in the details
                //

                TagEntry = (PHEAP_TAG_ENTRY)(Heap->PseudoTagEntries + TagIndex);

                if (ARGUMENT_PRESENT( TagInfo )) {

                    TagInfo->NumberOfAllocations = TagEntry->Allocs;
                    TagInfo->NumberOfFrees = TagEntry->Frees;
                    TagInfo->BytesAllocated = TagEntry->Size << HEAP_GRANULARITY_SHIFT;
                }

                //
                //  Check if we should reset the counters
                //

                if (ResetCounters) {

                    TagEntry->Allocs = 0;
                    TagEntry->Frees = 0;
                    TagEntry->Size = 0;
                }

                //
                //  Pseudo tags do not have names
                //

                Result = L"";
            }
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return the tag name to our caller
    //

    return Result;
}


//
//  Declared in nturtl.h
//

NTSTATUS
RtlExtendHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID Base,
    IN SIZE_T Size
    )

/*++

Routine Description:

    This routine grows the specified heap by adding a new segment to its
    storage.  The memory for the segment is supplied by the caller.

Arguments:

    HeapHandle - Supplies a pointer to the heap being modified

    Flags - Supplies a set of flags used to augment those already
        enforced by the heap

    Base - Supplies the starting address for the new segment being added
        to the input heap

    Size - Supplies the size, in bytes, of the new segment. Note that this
        routine will actually use more memory than specified by this
        variable.  It will use whatever is committed and reserved provided
        the amount is greater than or equal to "Size"

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    NTSTATUS Status;
    PHEAP_SEGMENT Segment;
    BOOLEAN LockAcquired = FALSE;
    UCHAR SegmentIndex, EmptySegmentIndex;
    SIZE_T CommitSize;
    SIZE_T ReserveSize;
    ULONG SegmentFlags;
    PVOID CommittedBase;
    PVOID UnCommittedBase;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    //
    //  Check if the guard page version of heap can do the work
    //

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapExtend( HeapHandle, Flags, Base, Size ));

    //
    //  See what Mm thinks about the base address we were passed in.
    //  The address must not be free.
    //

    Status = NtQueryVirtualMemory( NtCurrentProcess(),
                                   Base,
                                   MemoryBasicInformation,
                                   &MemoryInformation,
                                   sizeof( MemoryInformation ),
                                   NULL );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    if (MemoryInformation.State == MEM_FREE) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  If what we were passed in as a base address is not on a page boundary then
    //  adjust the information supplied by MM to the page boundary right after
    //  the input base address
    //

    if (MemoryInformation.BaseAddress != Base) {

        MemoryInformation.BaseAddress = (PCHAR)MemoryInformation.BaseAddress + PAGE_SIZE;
        MemoryInformation.RegionSize -= PAGE_SIZE;
    }

    try {

        //
        //  Lock the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        //
        //  Scan the heap's segment list for a free segment.  And make sure the address
        //  of all the segment does not contain the input base address
        //

        Status = STATUS_INSUFFICIENT_RESOURCES;

        EmptySegmentIndex = HEAP_MAXIMUM_SEGMENTS;

        for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

            Segment = Heap->Segments[ SegmentIndex ];

            if (Segment) {

                if (((ULONG_PTR)Base >= (ULONG_PTR)Segment) &&
                    ((ULONG_PTR)Base < (ULONG_PTR)(Segment->LastValidEntry))) {

                    Status = STATUS_INVALID_PARAMETER;

                    break;
                }

            } else if ((Segment == NULL) &&
                       (EmptySegmentIndex == HEAP_MAXIMUM_SEGMENTS)) {

                EmptySegmentIndex = SegmentIndex;

                Status = STATUS_SUCCESS;
            }
        }

        //
        //  At this point if status is success then the empty segment index
        //  is available for us to use and base address doesn't overlap an
        //  existing segment.
        //

        if (NT_SUCCESS( Status )) {

            //
            //  Indicate that this segment is user supplied
            //

            SegmentFlags = HEAP_SEGMENT_USER_ALLOCATED;

            CommittedBase = MemoryInformation.BaseAddress;

            //
            //  If the start of the memory supplied by the use is already
            //  committed then check the state of the following
            //  uncommitted piece of memory to see if it is reserved
            //

            if (MemoryInformation.State == MEM_COMMIT) {

                CommitSize = MemoryInformation.RegionSize;

                UnCommittedBase = (PCHAR)CommittedBase + CommitSize;

                Status = NtQueryVirtualMemory( NtCurrentProcess(),
                                               UnCommittedBase,
                                               MemoryBasicInformation,
                                               &MemoryInformation,
                                               sizeof( MemoryInformation ),
                                               NULL );

                ReserveSize = CommitSize;

                if ((NT_SUCCESS( Status )) &&
                    (MemoryInformation.State == MEM_RESERVE)) {

                    ReserveSize += MemoryInformation.RegionSize;
                }

            } else {

                //
                //  Otherwise the user hasn't committed anything in the
                //  the address they gave us and we know it is not free
                //  so it must be reserved.
                //

                UnCommittedBase = CommittedBase;

                ReserveSize = MemoryInformation.RegionSize;
            }

            //
            //  Now if the reserved size is smaller than a page size or
            //  the user specified size is greater than the reserved size
            //  then the buffer we're given is too small to be a segment
            //  of heap
            //

            if ((ReserveSize < PAGE_SIZE) ||
                (Size > ReserveSize)) {

                Status = STATUS_BUFFER_TOO_SMALL;

            } else {

                //
                //  Otherwise the size is okay, now check if we need
                //  to do the commit of the base.  If so we'll commit
                //  one page

                if (UnCommittedBase == CommittedBase) {

                    CommitSize = PAGE_SIZE;

                    Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                                      (PVOID *)&Segment,
                                                      0,
                                                      &CommitSize,
                                                      MEM_COMMIT,
                                                      PAGE_READWRITE );
                }
            }

            //
            //  At this point the if status is good then memory is all set up
            //  with at least one page of committed memory to start with.  So
            //  initialize the heap segment and we're done.
            //

            if (NT_SUCCESS( Status )) {

                if (RtlpInitializeHeapSegment( Heap,
                                               Segment,
                                               EmptySegmentIndex,
                                               0,
                                               Segment,
                                               (PCHAR)Segment + CommitSize,
                                               (PCHAR)Segment + ReserveSize )) {

                    Status = STATUS_NO_MEMORY;
                }
            }
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller
    //

    return Status;
}


//
//  Declared in nturtl.h
//

SIZE_T
NTAPI
RtlCompactHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine compacts the specified heap by coalescing all the free block.
    It also determines the size of the largest available free block and
    returns its, in bytes, back to the caller.

Arguments:

    HeapHandle - Supplies a pointer to the heap being modified

    Flags - Supplies a set of flags used to augment those already
        enforced by the heap

Return Value:

    SIZE_T - Returns the size, in bytes, of the largest free block
        available in the heap

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_FREE_ENTRY FreeBlock;
    PHEAP_SEGMENT Segment;
    UCHAR SegmentIndex;
    SIZE_T LargestFreeSize;
    BOOLEAN LockAcquired = FALSE;

    //
    //  Augment the heap flags
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check if this is a debug version of heap
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugCompactHeap( HeapHandle, Flags );
    }

    try {

        //
        //  Lock the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        LargestFreeSize = 0;

        try {

            //
            //  Coalesce the heap into its largest free blocks possible
            //  and get the largest free block in the heap
            //

            FreeBlock = RtlpCoalesceHeap( (PHEAP)HeapHandle );

            //
            //  If there is a free block then compute its byte size
            //

            if (FreeBlock != NULL) {

                LargestFreeSize = FreeBlock->Size << HEAP_GRANULARITY_SHIFT;
            }

            //
            //  Scan every segment in the heap looking at its largest uncommitted
            //  range.  Remember the largest range if its bigger than anything
            //  we've found so far
            //

            for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

                Segment = Heap->Segments[ SegmentIndex ];

                if (Segment && Segment->LargestUnCommittedRange > LargestFreeSize) {

                    LargestFreeSize = Segment->LargestUnCommittedRange;
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return the largest free size to our caller
    //

    return LargestFreeSize;
}


//
//  Declared in nturtl.h
//

BOOLEAN
RtlValidateHeap (
    PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This routine verifies the structure of a heap and/or heap block

Arguments:

    HeapHandle - Supplies a pointer to the heap being queried

    Flags - Supplies a set of flags used to augment those already
        enforced by the heap

    BaseAddress - Optionally supplies a pointer to the heap block
        that should be individually validated

Return Value:

    BOOLEAN - TRUE if the heap/block is okay and FALSE otherwise

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    BOOLEAN Result;

    try {

        try {

            //
            //  Check for the guard page version of heap
            //

            if ( IS_DEBUG_PAGE_HEAP_HANDLE( HeapHandle )) {

                Result = RtlpDebugPageHeapValidate( HeapHandle, Flags, BaseAddress );

            } else {

                //
                //  If there is an active lookaside list then drain and remove it.
                //  By setting the lookaside field in the heap to null we guarantee
                //  that the call the free heap will not try and use the lookaside
                //  list logic.
                //
                //  We'll actually capture the lookaside pointer from the heap and
                //  only use the captured pointer.  This will take care of the
                //  condition where another walk or lock heap can cause us to check
                //  for a non null pointer and then have it become null when we read
                //  it again.  If it is non null to start with then even if the
                //  user walks or locks the heap via another thread the pointer to
                //  still valid here so we can still try and do a lookaside list pop.
                //

                PHEAP_LOOKASIDE Lookaside = (PHEAP_LOOKASIDE)Heap->Lookaside;

                if (Lookaside != NULL) {

                    ULONG i;
                    PVOID Block;

                    Heap->Lookaside = NULL;

                    for (i = 0; i < HEAP_MAXIMUM_FREELISTS; i += 1) {

                        while ((Block = RtlpAllocateFromHeapLookaside(&(Lookaside[i]))) != NULL) {

                            RtlFreeHeap( HeapHandle, 0, Block );
                        }
                    }
                }

                Result = FALSE;

                //
                //  Validate that HeapAddress points to a HEAP structure.
                //

                if (RtlpCheckHeapSignature( Heap, "RtlValidateHeap" )) {

                    Flags |= Heap->ForceFlags;

                    //
                    //  Lock the heap
                    //

                    if (!(Flags & HEAP_NO_SERIALIZE)) {

                        RtlAcquireLockRoutine( Heap->LockVariable );

                        LockAcquired = TRUE;
                    }

                    //
                    //  If the user did not supply a base address then verify
                    //  the complete heap otherwise just do a single heap
                    //  entry
                    //

                    if (BaseAddress == NULL) {

                        Result = RtlpValidateHeap( Heap, TRUE );

                    } else {

                        Result = RtlpValidateHeapEntry( Heap, (PHEAP_ENTRY)BaseAddress - 1, "RtlValidateHeap" );
                    }
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );

            Result = FALSE;
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Declared in nturtl.h
//

BOOLEAN
RtlValidateProcessHeaps (
    VOID
    )

/*++

Routine Description:

    This routine cycles through all and validates each heap in the current
    process.

Arguments:

    None.

Return Value:

    BOOLEAN - TRUE if all the heap verify okay and FALSE for any other
        reason.

--*/

{
    NTSTATUS Status;
    ULONG i, NumberOfHeaps;
    PVOID HeapsArray[ 512 ];
    PVOID *Heaps;
    SIZE_T Size;
    BOOLEAN Result;

    Result = TRUE;

    Heaps = &HeapsArray[ 0 ];

    //
    //  By default we can handle 512 heaps per process any more than
    //  that and we'll need to allocate storage to do the processing
    //
    //  So now determine how many heaps are in the current process
    //

    NumberOfHeaps = RtlGetProcessHeaps( 512, Heaps );

    //
    //  **** this is bogus because the preceeding routine will
    //  **** never return more than 512.  Either this routine
    //  **** needs to get the heap count from the peb itself
    //  **** or the called routine needs to return the actual
    //  **** number of heaps in the process, Then we have to know
    //  **** not to to beyond the heap array size
    //

    if (NumberOfHeaps > 512) {

        //
        //  The number of heaps is greater than 512 so
        //  allocate extra memory to store the array of
        //  heap pointers
        //

        Heaps = NULL;
        Size = NumberOfHeaps * sizeof( PVOID );

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&Heaps,
                                          0,
                                          &Size,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );

        if (!NT_SUCCESS( Status )) {

            return FALSE;
        }

        //
        //  And retry getting the heaps
        //
        //  **** this won't work again because it still uses 512
        //

        NumberOfHeaps = RtlGetProcessHeaps( 512, Heaps );
    }

    //
    //  Now for each heap in our heap array we'll validate
    //  that heap
    //

    for (i=0; i<NumberOfHeaps; i++) {

        if (!RtlValidateHeap( Heaps[i], 0, NULL )) {

            Result = FALSE;
        }
    }

    //
    //  Check if we need to return the memory that we use for
    //  an enlarged heap array
    //

    if (Heaps != &HeapsArray[ 0 ]) {

        ZwFreeVirtualMemory( NtCurrentProcess(),
                             (PVOID *)&Heaps,
                             &Size,
                             MEM_RELEASE );
    }

    //
    //  And return to our caller
    //

    return Result;
}


//
//  Declared in nturtl.h
//

ULONG
RtlGetProcessHeaps (
    ULONG NumberOfHeapsToReturn,
    PVOID *ProcessHeaps
    )

/*++

Routine Description:

    This routine determines how many individual heaps there are in the
    current process and fills an array with pointers to each heap.

Arguments:

    NumberOfHeapsToReturn - Indicates how many heaps the caller
        is willing to accept in the second parameter

    ProcessHeaps - Supplies a pointer to an array of heap pointer
        to be filled in by this routine.  The maximum size of this
        array is specified by the first parameter

Return Value:

    ULONG - Returns the smaller of the actual number of heaps in the
        the process or the size of the output buffer

--*/

{
    PPEB Peb = NtCurrentPeb();
    ULONG NumberOfHeapsToCopy;
    ULONG TotalHeaps;

    RtlAcquireLockRoutine( &RtlpProcessHeapsListLock.Lock );

    try {

        //
        //  Return no more than the number of heaps currently in use
        //

        TotalHeaps = Peb->NumberOfHeaps;

        if (TotalHeaps > NumberOfHeapsToReturn) {

            NumberOfHeapsToCopy = NumberOfHeapsToReturn;

        } else {

            NumberOfHeapsToCopy = TotalHeaps;

        }

        //
        //  Return the heap pointers to the caller
        //

        RtlMoveMemory( ProcessHeaps,
                       Peb->ProcessHeaps,
                       NumberOfHeapsToCopy * sizeof( *ProcessHeaps ));

        ProcessHeaps += NumberOfHeapsToCopy;
        NumberOfHeapsToReturn -= NumberOfHeapsToCopy;

    } finally {

        RtlReleaseLockRoutine( &RtlpProcessHeapsListLock.Lock );
    }

#ifdef DEBUG_PAGE_HEAP

    //
    //  If we have debugging page heaps, go return what we can from them
    //

    if ( RtlpDebugPageHeap ) {

        TotalHeaps +=
            RtlpDebugPageHeapGetProcessHeaps( NumberOfHeapsToReturn, ProcessHeaps );

    }

#endif

    return TotalHeaps;
}


//
//  Declared in nturtl.h
//

NTSTATUS
RtlEnumProcessHeaps (
    PRTL_ENUM_HEAPS_ROUTINE EnumRoutine,
    PVOID Parameter
    )

/*++

Routine Description:

    This routine cycles through all the heaps in a process and
    invokes the specified call back routine for that heap

Arguments:

    EnumRoutine - Supplies the callback to invoke for each heap
        in the process

    Parameter - Provides an additional parameter to pass to the
        callback routine

Return Value:

    NTSTATUS - returns success or the first error status returned
        by the callback routine

--*/

{
    PPEB Peb = NtCurrentPeb();
    NTSTATUS Status;
    ULONG i;

    Status = STATUS_SUCCESS;

    //
    //  Lock the heap
    //

    RtlAcquireLockRoutine( &RtlpProcessHeapsListLock.Lock );

    try {

        //
        //  For each heap in the process invoke the callback routine
        //  and if the callback returns anything other than success
        //  then break out and return immediately to our caller
        //

        for (i=0; i<Peb->NumberOfHeaps; i++) {

            Status = (*EnumRoutine)( (PHEAP)(Peb->ProcessHeaps[ i ]), Parameter );

            if (!NT_SUCCESS( Status )) {

                break;
            }
        }

    } finally {

        //
        //  Unlock the heap
        //

        RtlReleaseLockRoutine( &RtlpProcessHeapsListLock.Lock );
    }

    //
    //  And return to our caller
    //

    return Status;
}


//
//  Declared in nturtl.h
//

NTSTATUS
RtlUsageHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN OUT PRTL_HEAP_USAGE Usage
    )

/*++

Routine Description:

    This is a rather bizzare routine.  It models heap usage in that it returns
    to the caller the various heap sizes, but it also return three lists.  One
    is a list of entries for each active allocation in the heap.  The next two
    are used for tracking difference between usage calls.  There is a list of
    what was added and a list of what was removed.

Arguments:

    HeapHandle - Supplies a pointer to the heap being queried

    Flags - Supplies a set of flags needed to augment those enforced
        by the heap.

        HEAP_USAGE_ALLOCATED_BLOCKS - Denotes that the calls wants the list
            of allocated entries.

        HEAP_USAGE_FREE_BUFFER - Denotes the last call to this procedure and
            that any temporary storage can now be freed

    Usage - Receives the current usage statistics for the heap.  This variable
        is also used to store state information between calls to this routine.

Return Value:

    NTSTATUS - An appropriate status value.  STATUS_SUCCESS if the heap has
        not changed at all between calls and STATUS_MORE_ENTRIES if thep changed
        between two calls.

--*/

{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    PRTL_HEAP_USAGE_INTERNAL Buffer;
    PHEAP_SEGMENT Segment;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    PLIST_ENTRY Head, Next;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;
    SIZE_T BytesFree;
    UCHAR SegmentIndex;
    BOOLEAN LockAcquired = FALSE;
    BOOLEAN VirtualAllocBlockSeen;
    PRTL_HEAP_USAGE_ENTRY pOldEntries, pNewEntries, pNewEntry;
    PRTL_HEAP_USAGE_ENTRY *ppEntries, *ppAddedEntries, *ppRemovedEntries, *pp;
    PVOID DataAddress;
    SIZE_T DataSize;

    //
    //  Augment the heap flags
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check if we should be using the debug version of heap
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugUsageHeap( HeapHandle, Flags, Usage );
    }

    //
    //  Make sure that the size of the input buffer is correct
    //

    if (Usage->Length != sizeof( RTL_HEAP_USAGE )) {

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    //  Zero out the output fields
    //

    Usage->BytesAllocated = 0;
    Usage->BytesCommitted = 0;
    Usage->BytesReserved = 0;
    Usage->BytesReservedMaximum = 0;

    //
    //  Use the reserved area of the output buffer as an internal
    //  heap usage storage space between calls
    //

    Buffer = (PRTL_HEAP_USAGE_INTERNAL)&Usage->Reserved[ 0 ];

    //
    //  Check if there is not a base buffer and we should allocate
    //  one then do so now
    //

    if ((Buffer->Base == NULL) &&
        (Flags & HEAP_USAGE_ALLOCATED_BLOCKS)) {

        Buffer->ReservedSize = 4 * 1024 * 1024;

        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &Buffer->Base,
                                          0,
                                          &Buffer->ReservedSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE );

        if (!NT_SUCCESS( Status )) {

            return Status;
        }

        Buffer->CommittedSize = 0;
        Buffer->FreeList = NULL;
        Buffer->LargeEntriesSentinal = NULL;

    //
    //  Otherwise check if there already is a base buffer
    //  and we should free it now
    //

    } else if ((Buffer->Base != NULL) &&
               (Flags & HEAP_USAGE_FREE_BUFFER)) {

        Buffer->ReservedSize = 0;

        Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                      &Buffer->Base,
                                      &Buffer->ReservedSize,
                                      MEM_RELEASE );

        if (!NT_SUCCESS( Status )) {

            return Status;
        }

        RtlZeroMemory( Buffer, sizeof( *Buffer ) );
    }

    //
    //  **** Augment the heap flags again
    //

    Flags |= Heap->ForceFlags;

    try {

        //
        //  Lock the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        //
        //  Scan through the heap segments and for every in-use segment
        //  we add it to the amount of committed and reserved bytes
        //  If the segment is not in use and the heap is growable then
        //  we just add it to the reserved maximum
        //

        for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

            Segment = Heap->Segments[ SegmentIndex ];

            if (Segment) {

                Usage->BytesCommitted += (Segment->NumberOfPages -
                                          Segment->NumberOfUnCommittedPages) * PAGE_SIZE;

                Usage->BytesReserved += Segment->NumberOfPages * PAGE_SIZE;

            } else if (Heap->Flags & HEAP_GROWABLE) {

                Usage->BytesReservedMaximum += Heap->SegmentReserve;
            }
        }

        Usage->BytesReservedMaximum += Usage->BytesReserved;
        Usage->BytesAllocated = Usage->BytesCommitted - (Heap->TotalFreeSize << HEAP_GRANULARITY_SHIFT);

        //
        //  Scan through the big allocations and add those amounts to the
        //  usage statistics
        //

        Head = &Heap->VirtualAllocdBlocks;
        Next = Head->Flink;

        while (Head != Next) {

            VirtualAllocBlock = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );

            Usage->BytesAllocated += VirtualAllocBlock->CommitSize;
            Usage->BytesCommitted += VirtualAllocBlock->CommitSize;

            Next = Next->Flink;
        }

        Status = STATUS_SUCCESS;

        //
        //  Now check if we have a base buffer and we are suppose to account
        //  for allocated blocks
        //

        if ((Buffer->Base != NULL) &&
            (Flags & HEAP_USAGE_ALLOCATED_BLOCKS)) {

            //
            //  Setup a pointer to the old entries, added entries, and removed
            //  entries in the usage struct.  Also drain the added entries
            //  and removed entries list
            //

            pOldEntries = Usage->Entries;

            ppEntries = &Usage->Entries;

            *ppEntries = NULL;

            ppAddedEntries = &Usage->AddedEntries;

            while (*ppAddedEntries = RtlpFreeHeapUsageEntry( Buffer, *ppAddedEntries )) { NOTHING; }

            ppRemovedEntries = &Usage->RemovedEntries;

            while (*ppRemovedEntries = RtlpFreeHeapUsageEntry( Buffer, *ppRemovedEntries )) { NOTHING; }

            //
            //  The way the code works is that ppEntries, ppAddedEntries, and
            //  ppRemovedEntries point to the tail of their respective lists.  If
            //  the list is empty then they point to the head.
            //

            //
            //  Process every segment in the heap
            //

            for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

                Segment = Heap->Segments[ SegmentIndex ];

                //
                //  Only deal with segments that are in use
                //

                if (Segment) {

                    //
                    //  The current block is really the first block in current
                    //  segment.  We need to special case the computation to
                    //  account for the first heap segment.
                    //

                    if (Segment->BaseAddress == Heap) {

                        CurrentBlock = &Heap->Entry;

                    } else {

                        CurrentBlock = &Segment->Entry;
                    }

                    //
                    //  Now for every busy block in the segment we'll check if
                    //  we need to allocate a heap usage entry and put it in the
                    //  the entries list
                    //

                    while (CurrentBlock < Segment->LastValidEntry) {

                        if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {

                            //
                            //  Compute the users data address and size
                            //

                            DataAddress = (CurrentBlock+1);
                            DataSize = (CurrentBlock->Size << HEAP_GRANULARITY_SHIFT) -
                                       CurrentBlock->UnusedBytes;

    keepLookingAtOldEntries:

                            //
                            //  The first time through this routine will have
                            //  both of these variables null so we'll start off
                            //  by looking at new entries.
                            //

                            if (pOldEntries == Buffer->LargeEntriesSentinal) {

                                goto keepLookingAtNewEntries;
                            }

                            //
                            //  Check if this entry hasn't changed.
                            //
                            //  If the old entry is equal to this data block
                            //  then move the old entry back to the entries
                            //  list and go on to the next block.
                            //

                            if ((pOldEntries->Address == DataAddress) &&
                                (pOldEntries->Size == DataSize)) {

                                //
                                //  Same block, keep in entries list
                                //

                                *ppEntries = pOldEntries;
                                pOldEntries = pOldEntries->Next;
                                ppEntries = &(*ppEntries)->Next;

                                *ppEntries = NULL;

                            //
                            //  Check if an entry was removed
                            //
                            //  If this entry is beyond the old entry then move
                            //  the old entry to the removed entry list and keep
                            //  looking at the old entry list without advancing
                            //  the current data block
                            //

                            } else if (pOldEntries->Address <= DataAddress) {

                                *ppRemovedEntries = pOldEntries;
                                pOldEntries = pOldEntries->Next;
                                ppRemovedEntries = &(*ppRemovedEntries)->Next;

                                *ppRemovedEntries = NULL;

                                goto keepLookingAtOldEntries;

                            //
                            //  Otherwise the we want to process the current data block
                            //

                            } else {

    keepLookingAtNewEntries:

                                //
                                //  Allocate a new heap usage entry
                                //

                                pNewEntry = NULL;

                                Status = RtlpAllocateHeapUsageEntry( Buffer, &pNewEntry );

                                if (!NT_SUCCESS( Status )) {

                                    break;
                                }

                                //
                                //  And fill in the new entry
                                //

                                pNewEntry->Address = DataAddress;
                                pNewEntry->Size = DataSize;

                                //
                                //  If there is an extra stuff struct then fill it in
                                //  with the stack backtrace, and appropriate tag index
                                //

                                if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                                    ExtraStuff = RtlpGetExtraStuffPointer( CurrentBlock );

    #if i386

                                    pNewEntry->AllocatorBackTraceIndex = ExtraStuff->AllocatorBackTraceIndex;

    #endif // i386

                                    if (!IS_HEAP_TAGGING_ENABLED()) {

                                        pNewEntry->TagIndex = 0;

                                    } else {

                                        pNewEntry->TagIndex = ExtraStuff->TagIndex;
                                    }

                                } else {

                                    //
                                    //  Otherwise there is no extra stuff so there is
                                    //  no backtrace and the tag is from the small index
                                    //

    #if i386

                                    pNewEntry->AllocatorBackTraceIndex = 0;

    #endif // i386

                                    if (!IS_HEAP_TAGGING_ENABLED()) {

                                        pNewEntry->TagIndex = 0;

                                    } else {

                                        pNewEntry->TagIndex = CurrentBlock->SmallTagIndex;
                                    }
                                }

                                //
                                //  Allocate another new heap usage entry as part of the added
                                //  entry list
                                //

                                Status = RtlpAllocateHeapUsageEntry( Buffer, ppAddedEntries );

                                if (!NT_SUCCESS( Status )) {

                                    break;
                                }

                                //
                                //  Copy over the contents of the new entry to the added entry
                                //

                                **ppAddedEntries = *pNewEntry;

                                //
                                //  Advance the added entry pointer to the next slot
                                //

                                ppAddedEntries = &((*ppAddedEntries)->Next);

                                *ppAddedEntries = NULL;

                                pNewEntry->Next = NULL;

                                //
                                //  Add the new entry to the entries list
                                //

                                *ppEntries = pNewEntry;
                                ppEntries = &pNewEntry->Next;
                            }
                        }

                        //
                        //  Now advance to the next block in the segment
                        //
                        //  If the next block doesn't exist then zoom through the
                        //  uncommitted ranges in the segment until we find a
                        //  match and can recompute the next real block
                        //

                        if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {

                            CurrentBlock += CurrentBlock->Size;

                            if (CurrentBlock < Segment->LastValidEntry) {

                                UnCommittedRange = Segment->UnCommittedRanges;

                                while ((UnCommittedRange != NULL) &&
                                       (UnCommittedRange->Address != (ULONG_PTR)CurrentBlock)) {

                                    UnCommittedRange = UnCommittedRange->Next;
                                }

                                if (UnCommittedRange == NULL) {

                                    CurrentBlock = Segment->LastValidEntry;

                                } else {

                                    CurrentBlock = (PHEAP_ENTRY)(UnCommittedRange->Address +
                                                                 UnCommittedRange->Size);
                                }
                            }

                        } else {

                            //
                            //  Otherwise the next block exists and so point
                            //  directly at it
                            //

                            CurrentBlock += CurrentBlock->Size;
                        }
                    }
                }
            }

            //
            //  At this point we've scanned through every segment in the heap
            //
            //  The first time through we now have two lists one of entries and
            //  another of added entries.  In each case Usage->Entries, and
            //  Usage->AddedEntries points to the start of the list and ppEntries,
            //  and ppAddedEntries points to the tail of the list.  The first
            //  time through we has seem to have a one-to-one correspondence
            //  between Entries and AddedEntries, but the AddedEntries records
            //  do not contain anything useful
            //

            if (NT_SUCCESS( Status )) {

                //
                //  Now we'll examine each big allocation, and for each big allocation
                //  we'll make a heap usage entry
                //

                Head = &Heap->VirtualAllocdBlocks;
                Next = Head->Flink;
                VirtualAllocBlockSeen = FALSE;

                while (Head != Next) {

                    VirtualAllocBlock = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );

                    //
                    //  Allocate a new heap usage entry
                    //

                    pNewEntry = NULL;

                    Status = RtlpAllocateHeapUsageEntry( Buffer, &pNewEntry );

                    if (!NT_SUCCESS( Status )) {

                        break;
                    }

                    VirtualAllocBlockSeen = TRUE;

                    //
                    //  Fill in the new heap usage entry
                    //

                    pNewEntry->Address = (VirtualAllocBlock + 1);
                    pNewEntry->Size = VirtualAllocBlock->CommitSize - VirtualAllocBlock->BusyBlock.Size;

    #if i386

                    pNewEntry->AllocatorBackTraceIndex = VirtualAllocBlock->ExtraStuff.AllocatorBackTraceIndex;

    #endif // i386

                    if (!IS_HEAP_TAGGING_ENABLED()) {

                        pNewEntry->TagIndex = 0;

                    } else {

                        pNewEntry->TagIndex = VirtualAllocBlock->ExtraStuff.TagIndex;
                    }

                    //
                    //  Search the heap usage entries list until we find the address
                    //  that right after the new entry address and then insert
                    //  this new entry.  This will keep the entries list sorted in
                    //  assending addresses
                    //
                    //
                    //  The first time through this function ppEntries will point
                    //  to the tail and so *pp should actually start off as null,
                    //  which means that the big allocation simply get tacked on
                    //  the end of the entries list.  We do not augment the
                    //  AddedEntries list for these big allocations.
                    //

                    pp = ppEntries;

                    while (*pp) {

                        if ((*pp)->Address >= pNewEntry->Address) {

                            break;
                        }

                        pp = &(*pp)->Next;
                    }

                    pNewEntry->Next = *pp;
                    *pp = pNewEntry;

                    //
                    //  Get the next big allocation block
                    //

                    Next = Next->Flink;
                }

                //
                //  At this point we've scanned through the heap segments and the
                //  big allocations.
                //
                //  The first time through this procedure we have built two lists
                //  the Entries and the AddedEntries
                //

                if (NT_SUCCESS( Status )) {

                    pOldEntries = Buffer->LargeEntriesSentinal;
                    Buffer->LargeEntriesSentinal = *ppEntries;

                    //
                    //  Now we'll process the previous large entries sentinal list
                    //
                    //  This path is not taken the first time through this procedure
                    //

                    while (pOldEntries != NULL) {

                        //
                        //  If we have new entries and the entry is equal to the
                        //  entry in the previous large sentinal list then
                        //  we move one down on the new list and remove the previous
                        //  sentinal entry
                        //

                        if ((*ppEntries != NULL) &&
                            (pOldEntries->Address == (*ppEntries)->Address) &&
                            (pOldEntries->Size == (*ppEntries)->Size)) {

                            ppEntries = &(*ppEntries)->Next;

                            pOldEntries = RtlpFreeHeapUsageEntry( Buffer, pOldEntries );

                        //
                        //  If we do now have any new entries or the previous
                        //  sentinal entry is comes before this new entry then
                        //  we'll add the sentinal entry to the remove list
                        //

                        } else if ((*ppEntries == NULL) ||
                                   (pOldEntries->Address < (*ppEntries)->Address)) {

                            *ppRemovedEntries = pOldEntries;

                            pOldEntries = pOldEntries->Next;

                            ppRemovedEntries = &(*ppRemovedEntries)->Next;

                            *ppRemovedEntries = NULL;

                        //
                        //  Otherwise the old sentinal entry is put on the added
                        //  entries list
                        //

                        } else {

                            *ppAddedEntries = pOldEntries;

                            pOldEntries = pOldEntries->Next;

                            **ppAddedEntries = **ppEntries;

                            ppAddedEntries = &(*ppAddedEntries)->Next;

                            *ppAddedEntries = NULL;
                        }
                    }

                    //
                    //  This path is not taken the first time through this procedure
                    //

                    while (pNewEntry = *ppEntries) {

                        Status = RtlpAllocateHeapUsageEntry( Buffer, ppAddedEntries );

                        if (!NT_SUCCESS( Status )) {

                            break;
                        }

                        **ppAddedEntries = *pNewEntry;

                        ppAddedEntries = &(*ppAddedEntries)->Next;

                        *ppAddedEntries = NULL;

                        ppEntries = &pNewEntry->Next;
                    }

                    //
                    //  Tell the user that something has changed between the
                    //  previous call and this one
                    //

                    if ((Usage->AddedEntries != NULL) || (Usage->RemovedEntries != NULL)) {

                        Status = STATUS_MORE_ENTRIES;
                    }
                }
            }
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller
    //

    return Status;
}


//
//  Declared in nturtl.h
//

NTSTATUS
RtlWalkHeap (
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    )

/*++

Routine Description:

    This routine is used to enumerate all the entries within a heap.  For each
    call it returns a new information in entry.

Arguments:

    HeapHandle - Supplies a pointer to the heap being queried

    Entry - Supplies storage for the entry information.  If the DataAddress field
        is null then the enumeration starts over from the beginning otherwise it
        resumes from where it left off

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_SEGMENT Segment;
    UCHAR SegmentIndex;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;
    PLIST_ENTRY Next, Head;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

    //
    //  Check if we should be using the guard page verion of heap
    //

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapWalk( HeapHandle, Entry ));

    //
    //  If this is the debug version of heap then validate the heap
    //  before we go on
    //

    if (DEBUG_HEAP( Heap->Flags )) {

        if (!RtlDebugWalkHeap( HeapHandle, Entry )) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    Status = STATUS_SUCCESS;

    //
    //  If there is an active lookaside list then drain and remove it.
    //  By setting the lookaside field in the heap to null we guarantee
    //  that the call the free heap will not try and use the lookaside
    //  list logic.
    //
    //  We'll actually capture the lookaside pointer from the heap and
    //  only use the captured pointer.  This will take care of the
    //  condition where another walk or lock heap can cause us to check
    //  for a non null pointer and then have it become null when we read
    //  it again.  If it is non null to start with then even if the
    //  user walks or locks the heap via another thread the pointer to
    //  still valid here so we can still try and do a lookaside list pop.
    //

    {
        PHEAP_LOOKASIDE Lookaside = (PHEAP_LOOKASIDE)Heap->Lookaside;

        if (Lookaside != NULL) {

            ULONG i;
            PVOID Block;

            Heap->Lookaside = NULL;

            for (i = 0; i < HEAP_MAXIMUM_FREELISTS; i += 1) {

                while ((Block = RtlpAllocateFromHeapLookaside(&(Lookaside[i]))) != NULL) {

                    RtlFreeHeap( HeapHandle, 0, Block );
                }
            }
        }
    }

    //
    //  Check if this is the first time we've been called to walk the heap
    //

    if (Entry->DataAddress == NULL) {

        //
        //  Start with the first segement in the heap
        //

        SegmentIndex = 0;

nextSegment:

        CurrentBlock = NULL;

        //
        //  Now find the next in use segment for the heap
        //

        Segment = NULL;

        while ((SegmentIndex < HEAP_MAXIMUM_SEGMENTS) &&
               ((Segment = Heap->Segments[ SegmentIndex ]) == NULL)) {

            SegmentIndex += 1;
        }

        //
        //  If there are no more valid segments then we'll try the big
        //  allocation
        //

        if (Segment == NULL) {

            Head = &Heap->VirtualAllocdBlocks;
            Next = Head->Flink;

            if (Next == Head) {

                Status = STATUS_NO_MORE_ENTRIES;

            } else {

                VirtualAllocBlock = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );

                CurrentBlock = &VirtualAllocBlock->BusyBlock;
            }

        //
        //  Otherwise we'll grab information about the segment.  Note that
        //  the current block is still null so when we fall out of this
        //  block we'll return directly to our caller with this segment
        //  information
        //

        } else {

            Entry->DataAddress = Segment;

            Entry->DataSize = 0;

            Entry->OverheadBytes = sizeof( *Segment );

            Entry->Flags = RTL_HEAP_SEGMENT;

            Entry->SegmentIndex = SegmentIndex;

            Entry->Segment.CommittedSize = (Segment->NumberOfPages -
                                            Segment->NumberOfUnCommittedPages) * PAGE_SIZE;

            Entry->Segment.UnCommittedSize = Segment->NumberOfUnCommittedPages * PAGE_SIZE;

            Entry->Segment.FirstEntry = (Segment->FirstEntry->Flags & HEAP_ENTRY_BUSY) ?
                ((PHEAP_ENTRY)Segment->FirstEntry + 1) :
                (PHEAP_ENTRY)((PHEAP_FREE_ENTRY)Segment->FirstEntry + 1);

            Entry->Segment.LastEntry = Segment->LastValidEntry;
        }

    //
    //  This is not the first time through.  Check if last time we gave back
    //  an heap segement or an uncommitted range
    //

    } else if (Entry->Flags & (RTL_HEAP_SEGMENT | RTL_HEAP_UNCOMMITTED_RANGE)) {

        //
        //  Check that the segment index is still valid
        //

        if ((SegmentIndex = Entry->SegmentIndex) >= HEAP_MAXIMUM_SEGMENTS) {

            Status = STATUS_INVALID_ADDRESS;

            CurrentBlock = NULL;

        } else {

            //
            //  Check that the segment is still in use
            //

            Segment = Heap->Segments[ SegmentIndex ];

            if (Segment == NULL) {

                Status = STATUS_INVALID_ADDRESS;

                CurrentBlock = NULL;

            //
            //  The segment is still in use if what we returned last time
            //  as the segment header then this time we'll return the
            //  segments first entry
            //

            } else if (Entry->Flags & RTL_HEAP_SEGMENT) {

                CurrentBlock = (PHEAP_ENTRY)Segment->FirstEntry;

            //
            //  Otherwise what we returned last time as an uncommitted
            //  range so now we need to get the next block
            //

            } else {

                CurrentBlock = (PHEAP_ENTRY)((PCHAR)Entry->DataAddress + Entry->DataSize);

                //
                //  Check if we are beyond this segment and need to get the
                //  next one
                //

                if (CurrentBlock >= Segment->LastValidEntry) {

                    SegmentIndex += 1;

                    goto nextSegment;
                }
            }
        }

    //
    //  Otherwise this is not the first time through and last time we gave back a
    //  valid heap entry
    //

    } else {

        //
        //  Check if the last entry we gave back was in use
        //

        if (Entry->Flags & HEAP_ENTRY_BUSY) {

            //
            //  Get the last entry we returned
            //

            CurrentBlock = ((PHEAP_ENTRY)Entry->DataAddress - 1);

            //
            //  If the last entry was for a big allocation then
            //  get the next big block if there is one otherwise
            //  say there are no more entries
            //

            if (CurrentBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

                Head = &Heap->VirtualAllocdBlocks;

                VirtualAllocBlock = CONTAINING_RECORD( CurrentBlock, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

                Next = VirtualAllocBlock->Entry.Flink;

                if (Next == Head) {

                    Status = STATUS_NO_MORE_ENTRIES;

                } else {

                    VirtualAllocBlock = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );

                    CurrentBlock = &VirtualAllocBlock->BusyBlock;
                }

            //
            //  Our previous result is a busy normal block
            //

            } else {

                //
                //  Get the segment and make sure it it still valid and in use
                //
                //  **** this should also check that segment index is not
                //  **** greater than HEAP MAXIMUM SEGMENTS
                //

                Segment = Heap->Segments[ SegmentIndex = CurrentBlock->SegmentIndex ];

                if (Segment == NULL) {

                    Status = STATUS_INVALID_ADDRESS;

                    CurrentBlock = NULL;

                //
                //  The segment is still in use, check if what we returned
                //  previously was a last entry
                //

                } else if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {

findUncommittedRange:

                    //
                    //  We are at a last entry so now if the segment is done
                    //  then go get another segment
                    //

                    CurrentBlock += CurrentBlock->Size;

                    if (CurrentBlock >= Segment->LastValidEntry) {

                        SegmentIndex += 1;

                        goto nextSegment;
                    }

                    //
                    //  Otherwise we will find the uncommitted range entry that
                    //  immediately follows this last entry
                    //

                    pp = &Segment->UnCommittedRanges;

                    while ((UnCommittedRange = *pp) && UnCommittedRange->Address != (ULONG_PTR)CurrentBlock ) {

                        pp = &UnCommittedRange->Next;
                    }

                    if (UnCommittedRange == NULL) {

                        Status = STATUS_INVALID_PARAMETER;

                    } else {

                        //
                        //  Now fill in the entry to denote that uncommitted
                        //  range information
                        //

                        Entry->DataAddress = (PVOID)UnCommittedRange->Address;

                        Entry->DataSize = UnCommittedRange->Size;

                        Entry->OverheadBytes = 0;

                        Entry->SegmentIndex = SegmentIndex;

                        Entry->Flags = RTL_HEAP_UNCOMMITTED_RANGE;
                    }

                    //
                    //  Null out the current block because we've just filled in
                    //  the entry
                    //

                    CurrentBlock = NULL;

                } else {

                    //
                    //  Otherwise the entry has a following entry so now
                    //  advance to the next entry
                    //

                    CurrentBlock += CurrentBlock->Size;
                }
            }

        //
        //  Otherwise the previous entry we returned is not in use
        //

        } else {

            //
            //  Get the last entry we returned
            //

            CurrentBlock = (PHEAP_ENTRY)((PHEAP_FREE_ENTRY)Entry->DataAddress - 1);

            //
            //  Get the segment and make sure it it still valid and in use
            //
            //  **** this should also check that segment index is not
            //  **** greater than HEAP MAXIMUM SEGMENTS
            //

            Segment = Heap->Segments[ SegmentIndex = CurrentBlock->SegmentIndex ];

            if (Segment == NULL) {

                Status = STATUS_INVALID_ADDRESS;

                CurrentBlock = NULL;

            //
            //  If the block is the last entry then go find the next uncommitted
            //  range or segment
            //

            } else if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {

                goto findUncommittedRange;

            //
            //  Otherwise we'll just move on to the next entry
            //

            } else {

                CurrentBlock += CurrentBlock->Size;
            }
        }
    }

    //
    //  At this point if current block is not null then we've found another
    //  entry to return.  We could also have found a segment or uncommitted
    //  range but those are handled separately above and keep current block
    //  null
    //

    if (CurrentBlock != NULL) {

        //
        //  Check if the block is in use
        //

        if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {

            //
            //  Fill in the entry field for this block
            //

            Entry->DataAddress = (CurrentBlock+1);

            if (CurrentBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

                Entry->DataSize = RtlpGetSizeOfBigBlock( CurrentBlock );

                Entry->OverheadBytes = (UCHAR)( sizeof( *VirtualAllocBlock ) + CurrentBlock->Size);

                Entry->SegmentIndex = HEAP_MAXIMUM_SEGMENTS;

                Entry->Flags = RTL_HEAP_BUSY |  HEAP_ENTRY_VIRTUAL_ALLOC;

            } else {

                Entry->DataSize = (CurrentBlock->Size << HEAP_GRANULARITY_SHIFT) -
                                  CurrentBlock->UnusedBytes;

                Entry->OverheadBytes = CurrentBlock->UnusedBytes;

                Entry->SegmentIndex = CurrentBlock->SegmentIndex;

                Entry->Flags = RTL_HEAP_BUSY;
            }

            if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                ExtraStuff = RtlpGetExtraStuffPointer( CurrentBlock );

                Entry->Block.Settable = ExtraStuff->Settable;
#if i386

                Entry->Block.AllocatorBackTraceIndex = ExtraStuff->AllocatorBackTraceIndex;

#endif // i386

                if (!IS_HEAP_TAGGING_ENABLED()) {

                    Entry->Block.TagIndex = 0;

                } else {

                    Entry->Block.TagIndex = ExtraStuff->TagIndex;
                }

                Entry->Flags |= RTL_HEAP_SETTABLE_VALUE;

            } else {

                if (!IS_HEAP_TAGGING_ENABLED()) {

                    Entry->Block.TagIndex = 0;

                } else {

                    Entry->Block.TagIndex = CurrentBlock->SmallTagIndex;
                }
            }

            Entry->Flags |= CurrentBlock->Flags & HEAP_ENTRY_SETTABLE_FLAGS;

        //
        //  Otherwise the block is not in use
        //

        } else {

            Entry->DataAddress = ((PHEAP_FREE_ENTRY)CurrentBlock+1);

            Entry->DataSize = (CurrentBlock->Size << HEAP_GRANULARITY_SHIFT) -
                              sizeof( HEAP_FREE_ENTRY );

            Entry->OverheadBytes = sizeof( HEAP_FREE_ENTRY );

            Entry->SegmentIndex = CurrentBlock->SegmentIndex;

            Entry->Flags = 0;
        }
    }

    //
    //  And return to our caller
    //

    return Status;
}


//
//  Declared in heappriv.h
//

BOOLEAN
RtlpCheckHeapSignature (
    IN PHEAP Heap,
    IN PCHAR Caller
    )

/*++

Routine Description:

    This routine verifies that it is being called with a properly identified
    heap.

Arguments:

    Heap - Supplies a pointer to the heap being checked

    Caller - Supplies a string that can be used to identify the caller

Return Value:

    BOOLEAN - TRUE if the heap signature is present, and FALSE otherwise

--*/

{
    //
    //  If the heap signature matches then that is the only
    //  checking we do
    //

    if (Heap->Signature == HEAP_SIGNATURE) {

        return TRUE;

    } else {

        //
        //  We have a bad heap signature.  Print out some information, break
        //  into the debugger, and then return false
        //

        HeapDebugPrint(( "Invalid heap signature for heap at %x", Heap ));

        if (Caller != NULL) {

            DbgPrint( ", passed to %s", Caller );
        }

        DbgPrint( "\n" );

        HeapDebugBreak( &Heap->Signature );

        return FALSE;
    }
}


//
//  Declared in heappriv.h
//

PHEAP_FREE_ENTRY
RtlpCoalesceHeap (
    IN PHEAP Heap
    )

/*++

Routine Description:

    This routine scans through heap and coalesces its free blocks

Arguments:

    Heap - Supplies a pointer to the heap being modified

Return Value:

    PHEAP_FREE_ENTRY - returns a pointer to the largest free block
        in the heap

--*/

{
    SIZE_T OldFreeSize;
    SIZE_T FreeSize;
    ULONG n;
    PHEAP_FREE_ENTRY FreeBlock, LargestFreeBlock;
    PLIST_ENTRY FreeListHead, Next;

    RTL_PAGED_CODE();

    LargestFreeBlock = NULL;

    //
    //  For every free list in the heap, going from smallest to
    //  largest and skipping the zero index one we will
    //  scan the free list coalesceing the free blocks
    //

    FreeListHead = &Heap->FreeLists[ 1 ];

    n = HEAP_MAXIMUM_FREELISTS;

    while (n--) {

        //
        //  Scan the individual free list
        //

        Next = FreeListHead->Blink;

        while (FreeListHead != Next) {

            //
            //  Get a pointer to the current free list entry, and remember its
            //  next and size
            //

            FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );

            Next = Next->Flink;
            OldFreeSize = FreeSize = FreeBlock->Size;

            //
            //  Coalesce the block
            //

            FreeBlock = RtlpCoalesceFreeBlocks( Heap,
                                                FreeBlock,
                                                &FreeSize,
                                                TRUE );

            //
            //  If the new free size is not equal to the old free size
            //  then we actually did some changes otherwise the coalesce
            //  calll was essentialy a noop
            //

            if (FreeSize != OldFreeSize) {

                //
                //  Check if we should decommit this block because it is too
                //  large and it is either at the beginning or end of a
                //  committed run.  Otherwise just insert the new sized
                //  block into its corresponding free list.  We'll hit this
                //  block again when we visit larger free lists.
                //

                if (FreeBlock->Size >= (PAGE_SIZE >> HEAP_GRANULARITY_SHIFT)

                        &&

                    (FreeBlock->PreviousSize == 0 ||
                     (FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY))) {

                    RtlpDeCommitFreeBlock( Heap, FreeBlock, FreeSize );

                } else {

                    RtlpInsertFreeBlock( Heap, FreeBlock, FreeSize );
                }

                Next = FreeListHead->Blink;

            } else {

                //
                //  Remember the largest free block we've found so far
                //

                if ((LargestFreeBlock == NULL) ||
                    (LargestFreeBlock->Size < FreeBlock->Size)) {

                    LargestFreeBlock = FreeBlock;
                }
            }
        }

        //
        //  Go to the next free list.  When we hit the largest dedicated
        //  size free list we'll fall back to the [0] index list
        //

        if (n == 1) {

            FreeListHead = &Heap->FreeLists[ 0 ];

        } else {

            FreeListHead++;
        }
    }

    //
    //  And return to our caller
    //

    return LargestFreeBlock;
}


//
//  Declared in heappriv.h
//

VOID
RtlpAddHeapToProcessList (
    IN PHEAP Heap
    )

/*++

Routine Description:

    This routine adds the specified heap to the heap list for the
    current process

Arguments:

    Heap - Supplies a pointer to the heap being added

Return Value:

    None.

--*/

{
    PPEB Peb = NtCurrentPeb();
    PHEAP *NewList;

    //
    //  Lock the processes heap list
    //

    RtlAcquireLockRoutine( &RtlpProcessHeapsListLock.Lock );

    try {

        //
        //  If the processes heap list is already full then we'll
        //  double the size of the heap list for the process
        //

        if (Peb->NumberOfHeaps == Peb->MaximumNumberOfHeaps) {

            //
            //  Double the size
            //

            Peb->MaximumNumberOfHeaps *= 2;

            //
            //  Allocate space for the new list
            //

            NewList = RtlAllocateHeap( RtlProcessHeap(),
                                       0,
                                       Peb->MaximumNumberOfHeaps * sizeof( *NewList ));

            if (NewList == NULL) {

                leave;
            }

            //
            //  Copy over the old buffer to the new buffer
            //

            RtlMoveMemory( NewList,
                           Peb->ProcessHeaps,
                           Peb->NumberOfHeaps * sizeof( *NewList ));

            //
            //  Check if we should free the previous heap list buffer
            //

            if (Peb->ProcessHeaps != RtlpProcessHeapsListBuffer) {

                RtlFreeHeap( RtlProcessHeap(), 0, Peb->ProcessHeaps );
            }

            //
            //  Set the new list
            //

            Peb->ProcessHeaps = NewList;
        }

        //
        //  Add the input heap to the next free heap list slot, and note that
        //  the processes heap list index is really one beyond the actualy
        //  index used to get the processes heap
        //

        Peb->ProcessHeaps[ Peb->NumberOfHeaps++ ] = Heap;
        Heap->ProcessHeapsListIndex = (USHORT)Peb->NumberOfHeaps;

    } finally {

        //
        //  Unlock the processes heap list
        //

        RtlReleaseLockRoutine( &RtlpProcessHeapsListLock.Lock );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Delcared in heappriv.h
//

VOID
RtlpRemoveHeapFromProcessList (
    IN PHEAP Heap
    )

/*++

Routine Description:

    This routine removes the specified heap to the heap list for the
    current process

Arguments:

    Heap - Supplies a pointer to the heap being removed

Return Value:

    None.

--*/

{
    PPEB Peb = NtCurrentPeb();
    PHEAP *p, *p1;
    ULONG n;

    //
    //  Lock the current processes heap list lock
    //

    RtlAcquireLockRoutine( &RtlpProcessHeapsListLock.Lock );

    try {

        //
        //  We only want to the the work if the current process actually has some
        //  heaps, the index stored in the heap is within the range for active
        //  heaps.  Note that the heaps stored index is bias by one.
        //

        if ((Peb->NumberOfHeaps != 0) &&
            (Heap->ProcessHeapsListIndex != 0) &&
            (Heap->ProcessHeapsListIndex <= Peb->NumberOfHeaps)) {

            //
            //  Establish a pointer into the array of process heaps at the
            //  current heap location and one beyond
            //

            p = (PHEAP *)&Peb->ProcessHeaps[ Heap->ProcessHeapsListIndex - 1 ];

            p1 = p + 1;

            //
            //  Calculate the number of heaps that exist beyond the current
            //  heap in the array including the current heap location
            //

            n = Peb->NumberOfHeaps - (Heap->ProcessHeapsListIndex - 1);

            //
            //  For every heap beyond the current one that we are removing
            //  we'll move that heap down to the previous index.
            //

            while (--n) {

                //
                //  Copy the heap process array entry of the next entry to
                //  the current entry, and move p1 to the next next entry
                //

                *p = *p1++;

                //
                //  This is simply a debugging call
                //

                RtlpUpdateHeapListIndex( (*p)->ProcessHeapsListIndex,
                                         (USHORT)((*p)->ProcessHeapsListIndex - 1));

                //
                //  Assign the moved heap its new heap index
                //

                (*p)->ProcessHeapsListIndex -= 1;

                //
                //  Move on to the next heap entry
                //

                p += 1;
            }

            //
            //  Zero out the last process heap pointer, update the count, and
            //  make the heap we just removed realize it has been removed by
            //  zeroing out its process heap list index
            //

            Peb->ProcessHeaps[ --Peb->NumberOfHeaps ] = NULL;
            Heap->ProcessHeapsListIndex = 0;
        }

    } finally {

        //
        //  Unlock the current processes heap list lock
        //

        RtlReleaseLockRoutine( &RtlpProcessHeapsListLock.Lock );
    }

    return;
}


//
//  Local Support routine
//

BOOLEAN
RtlpGrowBlockInPlace (
    IN PHEAP Heap,
    IN ULONG Flags,
    IN PHEAP_ENTRY BusyBlock,
    IN SIZE_T Size,
    IN SIZE_T AllocationIndex
    )

/*++

Routine Description:

    This routine will try and grow a heap allocation block at its current
    location

Arguments:

    Heap - Supplies a pointer to the heap being modified

    Flags - Supplies a set of flags to augment those already enforced by
        the heap

    BusyBlock - Supplies a pointer to the block being resized

    Size - Supplies the size, in bytes, needed by the resized block

    AllocationIndex - Supplies the allocation index for the resized block
        Note that the size variable has not been rounded up to the next
        granular block size, but that allocation index has.

Return Value:

    BOOLEAN - TRUE if the block has been resized and FALSE otherwise

--*/

{
    SIZE_T FreeSize;
    SIZE_T OldSize;
    UCHAR EntryFlags, FreeFlags;
    PHEAP_FREE_ENTRY FreeBlock, SplitBlock, SplitBlock2;
    PHEAP_ENTRY_EXTRA OldExtraStuff, NewExtraStuff;

    //
    //  Check if the allocation index is too large for even the nondedicated
    //  free list (i.e., too large for list [0])
    //

    if (AllocationIndex > Heap->VirtualMemoryThreshold) {

        return FALSE;
    }

    //
    //  Get the flags for the current block and a pointer to the next
    //  block following the current block
    //

    EntryFlags = BusyBlock->Flags;

    FreeBlock = (PHEAP_FREE_ENTRY)(BusyBlock + BusyBlock->Size);

    //
    //  If the current block is the last entry before an uncommitted range
    //  we'll try and extend the uncommitted range to fit our new allocation
    //

    if (EntryFlags & HEAP_ENTRY_LAST_ENTRY) {

        //
        //  Calculate how must more we need beyond the current block
        //  size
        //

        FreeSize = (AllocationIndex - BusyBlock->Size) << HEAP_GRANULARITY_SHIFT;
        FreeSize = ROUND_UP_TO_POWER2( FreeSize, PAGE_SIZE );

        //
        //  Try and commit memory at the desired location
        //

        FreeBlock = RtlpFindAndCommitPages( Heap,
                                            Heap->Segments[ BusyBlock->SegmentIndex ],
                                            &FreeSize,
                                            (PHEAP_ENTRY)FreeBlock );

        //
        //  Check if the commit succeeded
        //

        if (FreeBlock == NULL) {

            return FALSE;
        }

        //
        //  New coalesce this newly committed space with whatever is free
        //  around it
        //

        FreeSize = FreeSize >> HEAP_GRANULARITY_SHIFT;

        FreeBlock = RtlpCoalesceFreeBlocks( Heap, FreeBlock, &FreeSize, FALSE );

        FreeFlags = FreeBlock->Flags;

        //
        //  If the newly allocated space plus the current block size is still
        //  not big enough for our resize effort then put this newly
        //  allocated block into the appropriate free list and tell our caller
        //  that a resize wasn't possible
        //

        if ((FreeSize + BusyBlock->Size) < AllocationIndex) {

            RtlpInsertFreeBlock( Heap, FreeBlock, FreeSize );

            Heap->TotalFreeSize += FreeSize;

            if (DEBUG_HEAP(Flags)) {

                RtlpValidateHeapHeaders( Heap, TRUE );
            }

            return FALSE;
        }

        //
        //  We were able to generate enough space for the resize effort, so
        //  now free size will be the index for the current block plus the
        //  new free space
        //

        FreeSize += BusyBlock->Size;

    } else {

        //
        //  The following block is present so grab its flags and see if
        //  it is free or busy.  If busy then we cannot grow the current
        //  block
        //

        FreeFlags = FreeBlock->Flags;

        if (FreeFlags & HEAP_ENTRY_BUSY) {

            return FALSE;
        }

        //
        //  Compute the index if we combine current block with its following
        //  free block and check if it is big enough
        //

        FreeSize = BusyBlock->Size + FreeBlock->Size;

        if (FreeSize < AllocationIndex) {

            return FALSE;
        }

        //
        //  The two blocks together are big enough so now remove the free
        //  block from its free list, and update the heap's total free size
        //

        RtlpRemoveFreeBlock( Heap, FreeBlock );

        Heap->TotalFreeSize -= FreeBlock->Size;
    }

    //
    //  At this point we have a busy block followed by a free block that
    //  together have enough space for the resize.  The free block has been
    //  removed from its list and free size is the index of the two combined
    //  blocks.
    //
    //  Calculate the number of bytes in use in the old block
    //

    OldSize = (BusyBlock->Size << HEAP_GRANULARITY_SHIFT) - BusyBlock->UnusedBytes;

    //
    //  Calculate the index for whatever excess we'll have when we combine
    //  the two blocks
    //

    FreeSize -= AllocationIndex;

    //
    //  If the excess is not too much then put it back in our allocation
    //  (i.e., we don't want small free pieces left over)
    //

    if (FreeSize <= 2) {

        AllocationIndex += FreeSize;

        FreeSize = 0;
    }

    //
    //  If the busy block has an extra stuff struct present then copy over the
    //  extra stuff
    //

    if (EntryFlags & HEAP_ENTRY_EXTRA_PRESENT) {

        OldExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + BusyBlock->Size - 1);
        NewExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + AllocationIndex - 1);

        *NewExtraStuff = *OldExtraStuff;

        //
        //  If heap tagging is enabled then update the heap tag from the extra
        //  stuff struct
        //

        if (IS_HEAP_TAGGING_ENABLED()) {

            NewExtraStuff->TagIndex =
                RtlpUpdateTagEntry( Heap,
                                    NewExtraStuff->TagIndex,
                                    BusyBlock->Size,
                                    AllocationIndex,
                                    ReAllocationAction );
        }

    //
    //  Otherwise extra stuff is not in use so see if heap tagging is enabled
    //  and if so then update small tag index
    //

    } else if (IS_HEAP_TAGGING_ENABLED()) {

        BusyBlock->SmallTagIndex = (UCHAR)
            RtlpUpdateTagEntry( Heap,
                                BusyBlock->SmallTagIndex,
                                BusyBlock->Size,
                                AllocationIndex,
                                ReAllocationAction );
    }

    //
    //  Check if we will have any free space to give back.
    //

    if (FreeSize == 0) {

        //
        //  No following free space so update the flags, size and byte counts
        //  for the resized block.  If the free block was a last entry
        //  then the busy block must also now be a last entry.
        //

        BusyBlock->Flags |= FreeFlags & HEAP_ENTRY_LAST_ENTRY;

        BusyBlock->Size = (USHORT)AllocationIndex;

        BusyBlock->UnusedBytes = (UCHAR)
            ((AllocationIndex << HEAP_GRANULARITY_SHIFT) - Size);

        //
        //  Update the previous size field of the following block if it exists
        //

        if (!(FreeFlags & HEAP_ENTRY_LAST_ENTRY)) {

            (BusyBlock + BusyBlock->Size)->PreviousSize = BusyBlock->Size;

        } else {

            PHEAP_SEGMENT Segment;

            Segment = Heap->Segments[BusyBlock->SegmentIndex];
            Segment->LastEntryInSegment = BusyBlock;
        }

    //
    //  Otherwise there is some free space to return to the heap
    //

    } else {

        //
        //  Update the size and byte counts for the resized block.
        //

        BusyBlock->Size = (USHORT)AllocationIndex;

        BusyBlock->UnusedBytes = (UCHAR)
            ((AllocationIndex << HEAP_GRANULARITY_SHIFT) - Size);

        //
        //  Determine where the new free block starts and fill in its fields
        //

        SplitBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)BusyBlock + AllocationIndex);

        SplitBlock->PreviousSize = (USHORT)AllocationIndex;

        SplitBlock->SegmentIndex = BusyBlock->SegmentIndex;

        //
        //  If this new free block will be the last entry then update its
        //  flags and size and put it into the appropriate free list
        //

        if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {

            PHEAP_SEGMENT Segment;

            Segment = Heap->Segments[SplitBlock->SegmentIndex];
            Segment->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;

            SplitBlock->Flags = FreeFlags;
            SplitBlock->Size = (USHORT)FreeSize;

            RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

            Heap->TotalFreeSize += FreeSize;

        //
        //  The free block is followed by another valid block
        //

        } else {

            //
            //  Point to the block following our new free block
            //

            SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);

            //
            //  If the block following the new free block is busy then
            //  update the flags and size for the new free block, update
            //  the following blocks previous size, and put the free block
            //  into the appropriate free list
            //

            if (SplitBlock2->Flags & HEAP_ENTRY_BUSY) {

                SplitBlock->Flags = FreeFlags & (~HEAP_ENTRY_LAST_ENTRY);
                SplitBlock->Size = (USHORT)FreeSize;

                //
                //  **** note that this test must be true because we are
                //  **** already in the else clause of the
                //  **** if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) statement
                //

                if (!(FreeFlags & HEAP_ENTRY_LAST_ENTRY)) {

                    ((PHEAP_ENTRY)SplitBlock + FreeSize)->PreviousSize = (USHORT)FreeSize;

                } else {

                    PHEAP_SEGMENT Segment;

                    Segment = Heap->Segments[SplitBlock->SegmentIndex];
                    Segment->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;
                }

                RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                Heap->TotalFreeSize += FreeSize;

            //
            //  Otherwise the following block is also free so we can combine
            //  these two blocks
            //

            } else {

                //
                //  Remember the new free flags from the following block
                //

                FreeFlags = SplitBlock2->Flags;

                //
                //  Remove the following block from its free list
                //

                RtlpRemoveFreeBlock( Heap, SplitBlock2 );

                Heap->TotalFreeSize -= SplitBlock2->Size;

                //
                //  Calculate the size of the new combined free block
                //

                FreeSize += SplitBlock2->Size;

                //
                //  Give the new the its new flags
                //

                SplitBlock->Flags = FreeFlags;

                //
                //  If the combited block is not too large for the dedicated
                //  free lists then that where we'll put it
                //

                if (FreeSize <= HEAP_MAXIMUM_BLOCK_SIZE) {

                    SplitBlock->Size = (USHORT)FreeSize;

                    //
                    //  If present update the previous size for the following block
                    //

                    if (!(FreeFlags & HEAP_ENTRY_LAST_ENTRY)) {

                        ((PHEAP_ENTRY)SplitBlock + FreeSize)->PreviousSize = (USHORT)FreeSize;

                    } else {

                        PHEAP_SEGMENT Segment;

                        Segment = Heap->Segments[SplitBlock->SegmentIndex];
                        Segment->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;
                    }

                    //
                    //  Insert the new combined free block into the free list
                    //

                    RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                    Heap->TotalFreeSize += FreeSize;

                } else {

                    //
                    //  Otherwise the new free block is too large to go into
                    //  a dedicated free list so put it in the general free list
                    //  which might involve breaking it apart.
                    //

                    RtlpInsertFreeBlock( Heap, SplitBlock, FreeSize );
                }
            }
        }
    }

    //
    //  At this point the block has been resized and any extra space has been
    //  returned to the free list
    //
    //  Check if we should zero out the new space
    //

    if (Flags & HEAP_ZERO_MEMORY) {

        //
        //  **** this test is sort of bogus because we're resizing and the new
        //  **** size by definition must be larger than the old size
        //

        if (Size > OldSize) {

            RtlZeroMemory( (PCHAR)(BusyBlock + 1) + OldSize,
                           Size - OldSize );
        }

    //
    //  Check if we should be filling in heap after it as
    //  been freed, and if so then fill in the newly allocated
    //  space beyond the old bytes.
    //

    } else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED) {

        SIZE_T PartialBytes, ExtraSize;

        PartialBytes = OldSize & (sizeof( ULONG ) - 1);

        if (PartialBytes) {

            PartialBytes = 4 - PartialBytes;
        }

        if (Size > (OldSize + PartialBytes)) {

            ExtraSize = (Size - (OldSize + PartialBytes)) & ~(sizeof( ULONG ) - 1);

            if (ExtraSize != 0) {

                RtlFillMemoryUlong( (PCHAR)(BusyBlock + 1) + OldSize + PartialBytes,
                                    ExtraSize,
                                    ALLOC_HEAP_FILL );
            }
        }
    }

    //
    //  If we are going tailing checking then fill in the space right beyond
    //  the new allocation
    //

    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {

        RtlFillMemory( (PCHAR)(BusyBlock + 1) + Size,
                       CHECK_HEAP_TAIL_SIZE,
                       CHECK_HEAP_TAIL_FILL );
    }

    //
    //  Give the resized block any user settable flags send in by the
    //  caller
    //

    BusyBlock->Flags &= ~HEAP_ENTRY_SETTABLE_FLAGS;
    BusyBlock->Flags |= ((Flags & HEAP_SETTABLE_USER_FLAGS) >> 4);

    //
    //  And return to our caller
    //

    return TRUE;
}


//
//  Local support routine
//

PHEAP_TAG_ENTRY
RtlpAllocateTags (
    PHEAP Heap,
    ULONG NumberOfTags
    )

/*++

Routine Description:

    This routine is used to allocate space for additional tags within
    a heap

Arguments:

    Heap - Supplies a pointer to the heap being modified.  If not specified
        then the processes global tag heap is used

    NumberOfTags - Supplies the number of tags that we want stored in the
        heap.  This is the number to grow the tag list by.

Return Value:

    PHEAP_TAG_ENTRY - Returns a pointer to the next available tag entry in the
        heap

--*/

{
    NTSTATUS Status;
    ULONG TagIndex;
    SIZE_T ReserveSize;
    SIZE_T CommitSize;
    PHEAP_TAG_ENTRY TagEntry;
    USHORT CreatorBackTraceIndex;
    USHORT MaximumTagIndex;
    USHORT TagIndexFlag;

    //
    //  Check if the process has a global tag heap.  If not then there is
    //  nothing for us to do
    //

    if (RtlpGlobalTagHeap == NULL) {

        return NULL;
    }

    //
    //  If the user didn't give us a heap then use the processes global
    //  tag heap
    //

    if (Heap == NULL) {

        RtlpGlobalTagHeap->Signature = HEAP_SIGNATURE;

        RtlpGlobalTagHeap->Flags = HEAP_NO_SERIALIZE;

        TagIndexFlag = HEAP_GLOBAL_TAG;

        Heap = RtlpGlobalTagHeap;

    } else {

        TagIndexFlag = 0;
    }

    //
    //  Grab the stack backtrace if possible and if we should
    //

    CreatorBackTraceIndex = 0;

#if i386

    if (Heap->Flags & HEAP_CAPTURE_STACK_BACKTRACES) {

        CreatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();
    }

#endif // i386

    //
    //  If the heap does not already have tag entries then we'll
    //  reserve space for them
    //

    if (Heap->TagEntries == NULL) {

        MaximumTagIndex = HEAP_MAXIMUM_TAG & ~HEAP_GLOBAL_TAG;

        ReserveSize = MaximumTagIndex * sizeof( HEAP_TAG_ENTRY );

        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &Heap->TagEntries,
                                          0,
                                          &ReserveSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE );

        if (!NT_SUCCESS( Status )) {

            return NULL;
        }

        Heap->MaximumTagIndex = MaximumTagIndex;

        Heap->NextAvailableTagIndex = 0;

        //
        // Add one for zero tag, as that is always reserved for heap name
        //

        NumberOfTags += 1;
    }

    //
    //  At this point we have a space reserved for tag entries.  If the number
    //  of tags that we need to grow is too large then tell the user we can't
    //  do it.
    //

    if (NumberOfTags > (ULONG)(Heap->MaximumTagIndex - Heap->NextAvailableTagIndex)) {

        return NULL;
    }

    //
    //  Get a pointer to the next available tag entry, and for
    //  every tag entry that we want to grow by we'll commit
    //  the page containing the tag entry.  We only need to do
    //  this for every page just once.  We'll determine this
    //  by seeing when the tag entry crosses a page boundary
    //

    TagEntry = Heap->TagEntries + Heap->NextAvailableTagIndex;

    for (TagIndex = Heap->NextAvailableTagIndex;
         TagIndex < Heap->NextAvailableTagIndex + NumberOfTags;
         TagIndex++ ) {

        if (((((ULONG_PTR)TagEntry + sizeof(*TagEntry)) & (PAGE_SIZE-1)) <=
            sizeof(*TagEntry))) {

            CommitSize = PAGE_SIZE;

            Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                              &TagEntry,
                                              0,
                                              &CommitSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE );

            if (!NT_SUCCESS( Status )) {

                return NULL;
            }
        }

        //
        //  Bias the tag index if this is the global tag heap
        //

        TagEntry->TagIndex = (USHORT)TagIndex | TagIndexFlag;

        //
        //  Set the stack back trace
        //

        TagEntry->CreatorBackTraceIndex = CreatorBackTraceIndex;

        //
        //  Move on to the next tag entry
        //

        TagEntry += 1;
    }

    //
    //  At this point we've build the new tag list so now pop off the next
    //  available tag entry
    //

    TagEntry = Heap->TagEntries + Heap->NextAvailableTagIndex;

    Heap->NextAvailableTagIndex += (USHORT)NumberOfTags;

    //
    //  And return to our caller
    //

    return TagEntry;
}


//
//  Declared in heappriv.h
//

PWSTR
RtlpGetTagName (
    PHEAP Heap,
    USHORT TagIndex
    )

/*++

Routine Description:

    This routine returns the name of the tag denoted by the heap, tagindex
    tuple.

    This routine is only called by heapdbg when doing a debug print to
    generate a tag name for printing

Arguments:

    Heap - Supplies the tag being queried

    TagIndex - Supplies the index for the tag being queried

Return Value:

    PWSTR - returns the name of the indicated tag

--*/

{
    //
    //  If the processes global tag heap has not been initialized then
    //  not tag has a name
    //

    if (RtlpGlobalTagHeap == NULL) {

        return NULL;
    }

    //
    //  We only deal with non zero tag indices
    //

    if (TagIndex != 0) {

        //
        //  If the tag index is for a pseudo tag then we clear the
        //  the psuedo bit and generate a pseudo tag name
        //

        if (TagIndex & HEAP_PSEUDO_TAG_FLAG) {

            TagIndex &= ~HEAP_PSEUDO_TAG_FLAG;

            //
            //  Check that the tag index is valid and that the heap
            //  has some psuedo tag entries
            //

            if ((TagIndex < HEAP_NUMBER_OF_PSEUDO_TAG) &&
                (Heap->PseudoTagEntries != NULL)) {

                //
                //  A pseudo tag index of zero denote objects
                //

                if (TagIndex == 0) {

                    swprintf( RtlpPseudoTagNameBuffer, L"Objects>%4u",
                              HEAP_MAXIMUM_FREELISTS << HEAP_GRANULARITY_SHIFT );

                //
                //  A psuedo tag index less than the free list maximum
                //  denotes the dedicated free list
                //

                } else if (TagIndex < HEAP_MAXIMUM_FREELISTS) {

                    swprintf( RtlpPseudoTagNameBuffer, L"Objects=%4u", TagIndex << HEAP_GRANULARITY_SHIFT );

                //
                //  Otherwise the pseudo tag is for the big allocations
                //

                } else {

                    swprintf( RtlpPseudoTagNameBuffer, L"VirtualAlloc" );
                }

                return RtlpPseudoTagNameBuffer;
            }

        //
        //  Otherwise if the tag index is for a global tag then we pull
        //  the name off of the global heap.  Provided the index is valid
        //  and the heap does have some tag entries
        //

        } else if (TagIndex & HEAP_GLOBAL_TAG) {

            TagIndex &= ~HEAP_GLOBAL_TAG;

            if ((TagIndex < RtlpGlobalTagHeap->NextAvailableTagIndex) &&
                (RtlpGlobalTagHeap->TagEntries != NULL)) {

                return RtlpGlobalTagHeap->TagEntries[ TagIndex ].TagName;
            }

        //
        //  Otherwise we'll pull the name off of the input heap
        //  provided the index is valid and the heap does have some
        //  tag entries
        //

        } else if ((TagIndex < Heap->NextAvailableTagIndex) &&
                   (Heap->TagEntries != NULL)) {

            return Heap->TagEntries[ TagIndex ].TagName;
        }
    }

    return NULL;
}


//
//  Declared in heappriv.h
//

USHORT
RtlpUpdateTagEntry (
    PHEAP Heap,
    USHORT TagIndex,
    SIZE_T OldSize,              // Only valid for ReAllocation and Free actions
    SIZE_T NewSize,              // Only valid for ReAllocation and Allocation actions
    HEAP_TAG_ACTION Action
    )

/*++

Routine Description:

    This routine is used to modify a tag entry

Arguments:

    Heap - Supplies a pointer to the heap being modified

    TagIndex - Supplies the tag being modified

    OldSize - Supplies the old allocation index of the block associated with the tag

    NewSize - Supplies the new allocation index of the block associated with the tag

    Action - Supplies the type of action being performed on the heap tag

Return Value:

    USHORT - Returns a tag index for the newly updated tag

--*/

{
    PHEAP_TAG_ENTRY TagEntry;

    //
    //  If the processes tag heap does not exist then we'll return a zero index
    //  right away
    //

    if (RtlpGlobalTagHeap == NULL) {

        return 0;
    }

    //
    //  If the action is greater than or equal to free action then it is
    //  either FreeAction, VirtualFreeAction, ReAllocationAction, or
    //  VirtualReAllocationAction.  Which means we already should have a tag
    //  that is simply being modified
    //

    if (Action >= FreeAction) {

        //
        //  If the tag index is zero then there is nothing for us to do
        //

        if (TagIndex == 0) {

            return 0;
        }

        //
        //  If this is a pseudo tag then make sure the rest of the tag index
        //  after we remove the psuedo bit is valid and that the heap is
        //  actually maintaining pseudo tags
        //

        if (TagIndex & HEAP_PSEUDO_TAG_FLAG) {

            TagIndex &= ~HEAP_PSEUDO_TAG_FLAG;

            if ((TagIndex < HEAP_NUMBER_OF_PSEUDO_TAG) &&
                (Heap->PseudoTagEntries != NULL)) {

                TagEntry = (PHEAP_TAG_ENTRY)(Heap->PseudoTagEntries + TagIndex);

                TagIndex |= HEAP_PSEUDO_TAG_FLAG;

            } else {

                return 0;
            }

        //
        //  Otherwise if this is a global tag then make sure the tag index
        //  after we remove the global bit is valid and that the global tag
        //  heap has some tag entries
        //

        } else if (TagIndex & HEAP_GLOBAL_TAG) {

            TagIndex &= ~HEAP_GLOBAL_TAG;

            if ((TagIndex < RtlpGlobalTagHeap->NextAvailableTagIndex) &&
                (RtlpGlobalTagHeap->TagEntries != NULL)) {

                TagEntry = &RtlpGlobalTagHeap->TagEntries[ TagIndex ];

                TagIndex |= HEAP_GLOBAL_TAG;

            } else {

                return 0;
            }

        //
        //  Otherwise we have a regular tag index that we need to make sure
        //  is a valid value and that the heap has some tag entries
        //

        } else if ((TagIndex < Heap->NextAvailableTagIndex) &&
                   (Heap->TagEntries != NULL)) {

            TagEntry = &Heap->TagEntries[ TagIndex ];

        } else {

            return 0;
        }

        //
        //  At this point we have a tag entry and tag index.  Increment the
        //  number of frees we've done on the tag, and decrement the size by
        //  the number of bytes we've just freed
        //

        TagEntry->Frees += 1;

        TagEntry->Size -= OldSize;

        //
        //  Now if the action is either ReAllocationAction or
        //  VirtualReAllocationAction.  Then we get to add back in the
        //  new size and the allocation count
        //

        if (Action >= ReAllocationAction) {

            //
            //  If the this is a pseudo tag then we tag entry goes off the
            //  pseudo tag list
            //

            if (TagIndex & HEAP_PSEUDO_TAG_FLAG) {

                TagIndex = (USHORT)(NewSize < HEAP_MAXIMUM_FREELISTS ?
                                        NewSize :
                                        (Action == VirtualReAllocationAction ? HEAP_MAXIMUM_FREELISTS : 0));

                TagEntry = (PHEAP_TAG_ENTRY)(Heap->PseudoTagEntries + TagIndex);

                TagIndex |= HEAP_PSEUDO_TAG_FLAG;
            }

            TagEntry->Allocs += 1;

            TagEntry->Size += NewSize;
        }

    //
    //  The action is either AllocationAction or VirtualAllocationAction
    //

    } else {

        //
        //  Check if the supplied tag index is a regular tag and that it is
        //  valid for the tags in this heap
        //

        if ((TagIndex != 0) &&
            (TagIndex < Heap->NextAvailableTagIndex) &&
            (Heap->TagEntries != NULL)) {

            TagEntry = &Heap->TagEntries[ TagIndex ];

        //
        //  Otherwise if this is a global tag then make sure that it is a
        //  valid global index
        //

        } else if (TagIndex & HEAP_GLOBAL_TAG) {

            TagIndex &= ~HEAP_GLOBAL_TAG;

            Heap = RtlpGlobalTagHeap;

            if ((TagIndex < Heap->NextAvailableTagIndex) &&
                (Heap->TagEntries != NULL)) {

                TagEntry = &Heap->TagEntries[ TagIndex ];

                TagIndex |= HEAP_GLOBAL_TAG;

            } else {

                return 0;
            }

        //
        //  Otherwise if this is a pseudo tag then build a valid tag index
        //  based on the new size of the allocation
        //

        } else if (Heap->PseudoTagEntries != NULL) {

            TagIndex = (USHORT)(NewSize < HEAP_MAXIMUM_FREELISTS ?
                                    NewSize :
                                    (Action == VirtualAllocationAction ? HEAP_MAXIMUM_FREELISTS : 0));

            TagEntry = (PHEAP_TAG_ENTRY)(Heap->PseudoTagEntries + TagIndex);

            TagIndex |= HEAP_PSEUDO_TAG_FLAG;

        //
        //  Otherwise the user didn't call us with a valid tag
        //

        } else {

            return 0;
        }

        //
        //  At this point we have a valid tag entry and tag index, so
        //  update the tag entry state to reflect this new allocation
        //

        TagEntry->Allocs += 1;

        TagEntry->Size += NewSize;
    }

    //
    //  And return to our caller with the new tag index
    //

    return TagIndex;
}


//
//  Declared in heappriv.h
//

VOID
RtlpResetTags (
    PHEAP Heap
    )

/*++

Routine Description:

    This routine is used to reset all the tag entries in a heap

Arguments:

    Heap - Supplies a pointer to the heap being modified

Return Value:

    None.

--*/

{
    PHEAP_TAG_ENTRY TagEntry;
    PHEAP_PSEUDO_TAG_ENTRY PseudoTagEntry;
    ULONG i;

    //
    //  We only have work to do if the heap has any allocated tag entries
    //

    TagEntry = Heap->TagEntries;

    if (TagEntry != NULL) {

        //
        //  For every tag entry in the heap we will zero out its counters
        //

        for (i=0; i<Heap->NextAvailableTagIndex; i++) {

            TagEntry->Allocs = 0;
            TagEntry->Frees = 0;
            TagEntry->Size = 0;

            //
            //  Advance to the next tag entry
            //

            TagEntry += 1;
        }
    }

    //
    //  We will only reset the pseudo tags if they exist
    //

    PseudoTagEntry = Heap->PseudoTagEntries;

    if (PseudoTagEntry != NULL) {

        //
        //  For every pseudo tag entry in the heap we will zero out its
        //  counters
        //

        for (i=0; i<HEAP_NUMBER_OF_PSEUDO_TAG; i++) {

            PseudoTagEntry->Allocs = 0;
            PseudoTagEntry->Frees = 0;
            PseudoTagEntry->Size = 0;

            //
            //  Advance to the next pseudo tag entry
            //

            PseudoTagEntry += 1;
        }
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Declared in heappriv.h
//

VOID
RtlpDestroyTags (
    PHEAP Heap
    )

/*++

Routine Description:

    This routine is used to completely remove all the normal tag entries
    in use by a heap

Arguments:

    Heap - Supplies a pointer to the heap being modified

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    SIZE_T RegionSize;

    //
    //  We will only do the action if the heap has some tag entries
    //

    if (Heap->TagEntries != NULL) {

        //
        //  Release all the memory used by the tag entries
        //

        RegionSize = 0;

        Status = NtFreeVirtualMemory( NtCurrentProcess(),
                                      &Heap->TagEntries,
                                      &RegionSize,
                                      MEM_RELEASE );

        if (NT_SUCCESS( Status )) {

            Heap->TagEntries = NULL;
        }
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local support routine
//

NTSTATUS
RtlpAllocateHeapUsageEntry (
    PRTL_HEAP_USAGE_INTERNAL Buffer,
    PRTL_HEAP_USAGE_ENTRY *pp
    )

/*++

Routine Description:

    This routine is used to allocate an new heap usage entry
    from the internal heap usage buffer

Arguments:

    Buffer - Supplies a pointer to the internal heap usage
        buffer from which to allocate an entry

    pp - Receives a pointer to the newly allocated heap
        usage entry.  If pp is already pointing to an existing
        heap usage entry then on return we'll have this old
        entry point to the new entry, but still return the new
        entry.

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    NTSTATUS Status;
    PRTL_HEAP_USAGE_ENTRY p;
    PVOID CommitAddress;
    SIZE_T PageSize;

    //
    //  Check if the free list is empty and then we have to allocate more
    //  memory for the free list
    //

    if (Buffer->FreeList == NULL) {

        //
        //  We cannot grow the buffer any larger than the reserved size
        //

        if (Buffer->CommittedSize >= Buffer->ReservedSize) {

            return STATUS_NO_MEMORY;
        }

        //
        //  Try and add one page of committed memory to the buffer
        //  starting right after the currently committed space
        //

        PageSize = PAGE_SIZE;

        CommitAddress = (PCHAR)Buffer->Base + Buffer->CommittedSize;

        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &CommitAddress,
                                          0,
                                          &PageSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );

        if (!NT_SUCCESS( Status )) {

            return Status;
        }

        //
        //  Update the committed buffer size
        //

        Buffer->CommittedSize += PageSize;

        //
        //  Add the newly allocated space to the free list and
        //  build up the free list
        //

        Buffer->FreeList = CommitAddress;

        p = Buffer->FreeList;

        while (PageSize != 0) {

            p->Next = (p+1);
            p += 1;
            PageSize -= sizeof( *p );
        }

        //
        //  Null terminate the next pointer in the last free entry
        //

        p -= 1;
        p->Next = NULL;
    }

    //
    //  At this point the free list contains at least one entry
    //  so simply pop the entry.
    //

    p = Buffer->FreeList;

    Buffer->FreeList = p->Next;

    p->Next = NULL;

    //
    //  Now if the caller supplied an existing heap entry then
    //  we'll make the old heap entry point to this new entry
    //

    if (*pp) {

        (*pp)->Next = p;
    }

    //
    //  And then return the new entry to our caller
    //

    *pp = p;

    return STATUS_SUCCESS;
}


//
//  Local support routine
//

PRTL_HEAP_USAGE_ENTRY
RtlpFreeHeapUsageEntry (
    PRTL_HEAP_USAGE_INTERNAL Buffer,
    PRTL_HEAP_USAGE_ENTRY p
    )

/*++

Routine Description:

    This routine moves a heap usage entry from its current
    list onto the free list and returns a pointer to the
    next heap usage entry in the list.  It is like doing a pop
    of the list denoted by "p"

Arguments:

    Buffer - Supplies a pointer to the internal heap usage buffer
        being modified

    p - Supplies a pointer to the entry being moved.  Okay if
        it's null

Return Value:

    PRTL_HEAP_USAGE_ENTRY - Returns a pointer to the next heap usage
        entry

--*/

{
    PRTL_HEAP_USAGE_ENTRY pTmp;

    //
    //  Check if we have a non null heap entry and if so then add
    //  the entry to the front of the free list and return the next
    //  entry in the list
    //

    if (p != NULL) {

        pTmp = p->Next;

        p->Next = Buffer->FreeList;

        Buffer->FreeList = p;

    } else {

        pTmp = NULL;
    }

    return pTmp;
}


//
//  Declared in heap.h
//

BOOLEAN
RtlpHeapIsLocked (
    IN PVOID HeapHandle
    )

/*++

Routine Description:

    This routine is used to determine if a heap is locked

Arguments:

    HeapHandle - Supplies a pointer to the heap being queried

Return Value:

    BOOLEAN - TRUE if the heap is locked and FALSE otherwise

--*/

{
    PHEAP Heap;

    //
    //  Check if this is guard page version of heap
    //

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapIsLocked( HeapHandle ));

    Heap = (PHEAP)HeapHandle;

    //
    //  The heap is locked if there is a lock variable, and it has an
    //  owning thread or the lockcount is not -1
    //

    return (( Heap->LockVariable != NULL ) &&
            ( Heap->LockVariable->Lock.CriticalSection.OwningThread ||
              Heap->LockVariable->Lock.CriticalSection.LockCount != -1 ));
}

