#include "precomp.h"
#pragma hdrstop

//
// SEGMENT_HEADER is the header that resides at the beginning of every
// segment of memory managed by this package.  For non-growable heaps,
// there is only one segment for the entire heap.  For growable heaps,
// segments are created as needed whenever there is not enough free space
// to satisfy an allocation request.
//
typedef struct _SEGMENT_HEADER {
    struct _SEGMENT_HEADER *Next;
    ULONG Size;
    ULONG Spare[ 2 ];                   // Make sizeof match granularity
} SEGMENT_HEADER, *PSEGMENT_HEADER;

//
// FREE_HEADER is the header that resides at the beginning of every
// free block of memory managed by this package.  All free blocks are
// linked together on a free list rooted in the heap header.  The segment
// field of a free header prevents free blocks in different segments from
// being coalesced.
//
typedef struct _FREE_HEADER {
    struct _FREE_HEADER *Next;
    ULONG Size;
    struct _SEGMENT_HEADER *Segment;
    ULONG Spare;
} FREE_HEADER, *PFREE_HEADER;

//
// BUSY_HEADER is the header that resides at the beginning of allocated
// block of memory managed by this package.  When the block is
// allocated, the Busy structure is valid.  The address that the user
// sees actually points to the byte following the header.  When the
// block is on the free list, the Free structure is valid.
//
typedef struct _BUSY_HEADER {
    struct _SEGMENT_HEADER *Segment;
    ULONG Size;
    HANDLE HandleValue;
    ULONG Spare;
} BUSY_HEADER, *PBUSY_HEADER;

//
// Flags are stored in the low order two bits of the first word of the
// header.  This is possible, since all blocks are aligned on 16 byte
// boundaries.  To make walking the free list fast, the flag value for
// a free block is zero, so we can use the Next pointer without modification.
//
#define FLAGS_FREE        0x00000000
#define FLAGS_BUSY        0x00000001
#define FLAGS_MASK        0x00000003

//
// All allocations are made as a multiple of ALLOCATION_GRANULARITY.
// The size requested is rounded up to a multiple of the allocation
// granularity.  The size of an allocation header is then added and
// that is the amount of memory that is actually allocated.
//
#define ALLOCATION_GRANULARITY 16 // bytes

//
// HEAP_HEADER is the header for a heap.  All access to the heap is
// synchronized by the Lock field.
//
typedef struct _HEAP_HEADER {
    //
    // Heap routines use Length to determine if the heap is valid.
    //
    ULONG Length;

    //
    // If the ZeroExtraMemory field is true, then the heap allocation
    // logic will zero initialize any extra space at the end of an allocated
    // block, due to rounding up to the ALIGNMENT_GRANULARITY amount.
    //
    BOOLEAN ZeroExtraMemory;

    //
    // The address of the first valid address at the begining of the
    // heap.  Used for validating pointers passed to AlFreeHeap
    //
    PVOID ValidAddress;

    //
    // The address of the first address of memory beyond the end of the heap
    //
    PVOID EndAddress;

    //
    // FreeList is the header for the heap free list.
    //
    ULONG Spare;         // Make free list align on granularity
    FREE_HEADER FreeList;
} HEAP_HEADER, *PHEAP_HEADER;


//
// The heap is constructed out of a memory descriptor
// block that is below the descriptor for the loaded program.  This block
// also accomodates the loaded program stack.  It is essential therefore
// to estimate the stack space requirement for your arc program.  (16 pages
// should be enough.)  The StackPages and HeapPages are 4K each.
//

#define HEAP_ZERO_EXTRA_MEMORY   0x00000008
//
// Define memory allocation descriptor listhead and heap storage variables.
//

ULONG AlHeapFree;
ULONG AlHeapLimit;

PVOID  HeapHandle;


PVOID
AlRtCreateHeap(
    IN ULONG Flags,
    IN PVOID HeapBase,
    IN ULONG Size
    )

/*++

Routine Description:

    This routine initializes a heap.

Arguments:

    Flags - Specifies optional attributes of the heap.

        Valid Flags Values:
        HEAP_ZERO_EXTRA_MEMORY. to make sure that extra memory passed
                    in is zeroed out.

    HeapBase - if not NULL, this specifies the base address for memory
    to use as the heap.  If NULL, memory is allocated by these routines.

    Size - Size of the block of memory passed in to be used as a heap

Return Value:

    PVOID - a pointer to be used in accessing the created heap.

--*/

{
    PHEAP_HEADER Heap = NULL;
    PFREE_HEADER FreeBlock;


    Heap          = (PHEAP_HEADER)HeapBase;
    Heap->Length      = sizeof( HEAP_HEADER );
    Heap->ZeroExtraMemory = (BOOLEAN)((Flags & HEAP_ZERO_EXTRA_MEMORY) ? TRUE : FALSE);
    Heap->ValidAddress    = (PCH)Heap + ((sizeof(*Heap) + (ALLOCATION_GRANULARITY-1)) & ~(ALLOCATION_GRANULARITY-1));
    Heap->EndAddress      = (PCH)Heap + Size;

    //
    // Initialize the free list to be a single block that starts immediately
    // after the heap header.
    //

    FreeBlock = (PFREE_HEADER)Heap->ValidAddress;
    FreeBlock->Next = NULL;
    FreeBlock->Size = (ULONG)Heap->EndAddress -
                      (ULONG)FreeBlock;
    FreeBlock->Segment = NULL;

    Heap->FreeList.Next = FreeBlock;
    Heap->FreeList.Size = 0;
    Heap->FreeList.Segment = NULL;

    return( (PVOID)Heap );
} // AlRtCreateHeap



#if DBG

BOOLEAN
DumpHeapSegment(
    BOOLEAN DumpHeap,
    PHEAP_HEADER Heap,
    PVOID FirstValidBlock,
    PVOID FirstInvalidBlock,
    PULONG CountOfFreeBlocks
    )
{
    PVOID CurrentBlock = FirstValidBlock;
    PFREE_HEADER FreeBlock;
    PBUSY_HEADER BusyBlock;

    while (CurrentBlock < FirstInvalidBlock) {
        BusyBlock = CurrentBlock;
        FreeBlock = CurrentBlock;
        if (((ULONG)BusyBlock->Segment & FLAGS_MASK) == FLAGS_BUSY) {
            if (DumpHeap) {
                AlPrint( "    %08lx: BUSY Flags=%lx  Size: %lx  Segment: %lx\n",
                          BusyBlock,
                          (ULONG)BusyBlock->Segment & FLAGS_MASK,
                          BusyBlock->Size,
                          (ULONG)BusyBlock->Segment & ~FLAGS_MASK
                        );
                }

            CurrentBlock = (PCH)CurrentBlock + BusyBlock->Size;
            }
        else
        if (((ULONG)FreeBlock->Next & FLAGS_MASK) == FLAGS_FREE) {
            *CountOfFreeBlocks += 1;
            if (DumpHeap) {
                AlPrint( "    %08lx: FREE Next=%lx  Size: %lx  Segment: %lx\n",
                          FreeBlock,
                          FreeBlock->Next,
                          FreeBlock->Size,
                          FreeBlock->Segment
                        );
                }

            CurrentBlock = (PCH)CurrentBlock + FreeBlock->Size;
            }
        else {
            if (DumpHeap) {
                AlPrint( "*** Invalid heap block at %lx\n", CurrentBlock );
                }

            return( FALSE );
            }

        }

    if (CurrentBlock != FirstInvalidBlock) {
        if (DumpHeap) {
            AlPrint( "*** Heap segment ends at %lx instead of %lx\n",
                      CurrentBlock, FirstInvalidBlock
                    );
            }

        return( FALSE );
        }

    return( TRUE );
}


BOOLEAN
AlRtValidateHeap(
    IN PVOID HeapHandle,
    IN BOOLEAN DumpHeap
    )
{
    PHEAP_HEADER Heap = (PHEAP_HEADER)HeapHandle;
    PFREE_HEADER FreeBlock;
    BOOLEAN HeapValid = TRUE;
    ULONG LengthOfFreeList;
    ULONG CountOfFreeBlocks;

    //
    // Validate that HeapAddress points to a HEAP_HEADER structure.
    //

    if (Heap->Length != sizeof( HEAP_HEADER )) {
        if (DumpHeap) {
            AlPrint( "AlRtHEAP: Invalid heap header- %lx\n", Heap );
            }

        HeapValid = FALSE;
        goto exit;
        }


    if (DumpHeap) {
        AlPrint( "Heap at %lx,  Length=%lx\n", Heap, Heap->Length );
        AlPrint( "  FreeList: Head=%lx\n", Heap->FreeList.Next );
        AlPrint( "  Heap: End Address = %lx\n",Heap->EndAddress);
        }


    if (Heap->FreeList.Size != 0) {
        if (DumpHeap) {
            AlPrint( "*** head of free list is invalid (size)\n" );
            }

        HeapValid = FALSE;
        goto exit;
        }

    LengthOfFreeList = 0;
    FreeBlock = Heap->FreeList.Next;
    while (FreeBlock) {
        if (DumpHeap) {
            AlPrint( "    %08lx: Next=%lx  Size: %lx  Segment: %lx\n",
                      FreeBlock,
                      FreeBlock->Next,
                      FreeBlock->Size,
                      FreeBlock->Segment
                    );
            }

        if (((ULONG)FreeBlock->Next & FLAGS_MASK) != FLAGS_FREE) {
            if (DumpHeap) {
                AlPrint( "*** free list element is not a valid free block\n" );
                }

            HeapValid = FALSE;
            goto exit;
            }

        LengthOfFreeList += 1;
        FreeBlock = FreeBlock->Next;
        }

    CountOfFreeBlocks = 0;
    if (DumpHeap) {
        AlPrint( "  Heap Blocks starting at %lx:\n", Heap->ValidAddress );
        }

    HeapValid = DumpHeapSegment( DumpHeap,
                                 Heap,
                                 Heap->ValidAddress,
                                 Heap->EndAddress,
                                 &CountOfFreeBlocks
                               );
    if (!HeapValid) {
        goto exit;
        }

    if (LengthOfFreeList != CountOfFreeBlocks) {
        if (DumpHeap) {
            AlPrint( "*** Number of free blocks in arena (%ld) does not match number in the free list (%ld)\n",
                      CountOfFreeBlocks,
                      LengthOfFreeList
                    );
            }

        HeapValid = FALSE;
        goto exit;
        }

exit:
    return( HeapValid );

} // AlRtValidateHeap


#else

BOOLEAN
AlRtValidateHeap(
    IN PVOID HeapHandle,
    IN BOOLEAN DumpHeap
    )
{
    return( TRUE );
}

#endif



PVOID
AlRtAllocateHeap(
    IN PVOID HeapHandle,
    IN ULONG Size
    )
{
    PHEAP_HEADER Heap = (PHEAP_HEADER)HeapHandle;
    ULONG allocationSize;
//    PSEGMENT_HEADER Segment;
    PFREE_HEADER FreeBlock;
    PFREE_HEADER PreviousFreeBlock;
    PFREE_HEADER NewFreeBlock;
    PBUSY_HEADER BusyBlock;


    //
    // Validate that HeapAddress points to a HEAP_HEADER structure.
    //

    if (Heap->Length != sizeof( HEAP_HEADER )) {
#if DBG
    AlPrint( "ALHEAP: Invalid heap header- %lx\n", Heap );
#endif // DBG
        return( NULL );
        }

    //
    // Additional check, see if the heap is valid, call the heap validation
    // code, requesting it to not dump stuff.
    //
    if(!AlRtValidateHeap( HeapHandle, FALSE)) {

#if DBG
        AlPrint("Heap validation failed\n");
#endif
        return ( NULL );
    }


    //
    // Round the requested size up to the allocation granularity.  Note
    // that if the request is for 0 bytes, we still allocate memory.
    //

    allocationSize = ((Size ? Size : ALLOCATION_GRANULARITY) +
                      sizeof( BUSY_HEADER ) +
                      ALLOCATION_GRANULARITY -
                      1
                     ) & ~(ALLOCATION_GRANULARITY - 1);
    if (allocationSize < Size) {
#if DBG
    AlPrint( "ALHEAP: Invalid heap size - %lx\n", Size );
//   RtlpBreakPointHeap();
#endif // DBG
        return( NULL );
    }

    //
    // Point to the free list header.
    //

    FreeBlock = &Heap->FreeList;
#if DBG
    if (FreeBlock->Size != 0) {
    AlPrint( "ALHEAP: Heap free list HEAD hosed at %lx\n", FreeBlock );
//   RtlpBreakPointHeap();

    return( NULL );
        }
#endif // DBG

    //
    // Continuous loop.  We'll break out of the loop when we've found
    // (or created) some free memory.
    //

    while (TRUE) {
        //
        // Have we reached the end of the free list?
        //

    if (FreeBlock->Next == NULL)
        return( NULL );
        else {
            //
            // Point to the next free block, saving a pointer to the
            // previous one.
            //

            PreviousFreeBlock = FreeBlock;
            FreeBlock = PreviousFreeBlock->Next;
            }

#if DBG
        if (FreeBlock->Size == 0) {
        AlPrint( "ALHEAP: Heap free list ENTRY hosed at %lx\n",
                      FreeBlock
                    );
//       RtlpBreakPointHeap();
        return( NULL );
            }
#endif // DBG

        //
        // We haven't exhausted the free list yet.  If this block is
        // large enough for what we need, break out of the while loop.
        //

        if (FreeBlock->Size >= allocationSize) {
            break;
            }

        } // while ( TRUE )

    //
    // We have found a free block that is large enough to hold what the
    // user requested.  If it's exactly the right size, simply point the
    // previous free block to the successor of this free block.  If it's
    // larger than what we want, we allocate from the front of the block,
    // leaving the trailing part free.  Exactly the right size is fuzzy,
    // as if we decide to split we need at least enough extra space for
    // a free header plus some space to statisfy an allocation.
    //

    if ((FreeBlock->Size - allocationSize) < (2 * sizeof( FREE_HEADER ))) {
        //
        // If the amount of extra space is less than twice the size of
        // a free header, just give the caller all the space, as the
        // extra amount is too small to waste a free block on.
        //

        allocationSize = FreeBlock->Size;

        //
        // Exactly the right size.  Point previous free block to the
        // next free block.
        //

        PreviousFreeBlock->Next = FreeBlock->Next;

        }
    else {

        //
        // More memory than we need.  Make the trailing part of the block
        // into a free block.  Point the previous block to the new block
        // and the new block to the next block.
        //

        NewFreeBlock = (PFREE_HEADER)((PCH)FreeBlock + allocationSize);
        PreviousFreeBlock->Next = NewFreeBlock;
        NewFreeBlock->Next = FreeBlock->Next;
        NewFreeBlock->Size = FreeBlock->Size - allocationSize;
        NewFreeBlock->Segment = FreeBlock->Segment;
        }

    //
    // Set up the header for the allocated block.
    //

    BusyBlock = (PBUSY_HEADER)FreeBlock;
    BusyBlock->Segment = (PSEGMENT_HEADER)((ULONG)FreeBlock->Segment |
                                           FLAGS_BUSY
                                          );
    BusyBlock->Size = allocationSize;
    BusyBlock->HandleValue = NULL;

    if (Heap->ZeroExtraMemory) {
     ULONG extraSize;

     extraSize = allocationSize - Size - sizeof( BUSY_HEADER );
     memset( (PCHAR)BusyBlock + (allocationSize - extraSize),
            0,
            extraSize
              );
     }

#if DBG
    BusyBlock->Spare = 0;
#endif

    //
    // Return the address of the user portion of the allocated block.
    // This is the byte following the header.
    //

    return( (PVOID)(BusyBlock + 1) );
} // AlRtAllocateHeap


PVOID
AlRtFreeHeap(
    IN PVOID HeapHandle,
    IN PVOID BaseAddress
    )
{
    PHEAP_HEADER Heap = (PHEAP_HEADER)HeapHandle;
    PFREE_HEADER FreeBlock;
    PFREE_HEADER PreviousFreeBlock;
    PFREE_HEADER SecondPrevFreeBlock;
    PBUSY_HEADER BusyBlock;
    PSEGMENT_HEADER BusySegment;
    ULONG BusyFlags;
    ULONG BusySize;

    if (BaseAddress == NULL) {
        return( NULL );
        }

    //
    // Validate that HeapAddress points to a HEAP_HEADER structure.
    //

    if (Heap->Length != sizeof( HEAP_HEADER )) {
#if DBG
    AlPrint( "ALHEAP: Invalid heap header- %lx\n", Heap );
//   RtlpBreakPointHeap();
#endif // DBG
        return( BaseAddress );
        }

    //
    // Additional check, see if the heap is valid, call the heap validation
    // code, requesting it to not dump stuff.
    //
    if(!AlRtValidateHeap( HeapHandle, FALSE)) {

#if DBG
        AlPrint("Heap validation failed\n");
#endif
        return ( BaseAddress );
    }


    //
    // Get the 'real' address of the allocation unit.  (That is, the
    // address of the allocation header.)  Make sure the address lies
    // within the bounds of the valid portion of the heap.
    //

    BusyBlock = (PBUSY_HEADER)BaseAddress - 1;
    BusyFlags = (ULONG)BusyBlock->Segment & FLAGS_MASK;
    BusySegment = (PSEGMENT_HEADER)((ULONG)BusyBlock->Segment & ~FLAGS_MASK);
    BusySize = BusyBlock->Size;

    if (BusyFlags != FLAGS_BUSY
#if DBG
        || (BusySegment == NULL &&
            ((PCHAR)BusyBlock < (PCHAR)Heap->ValidAddress ||
         (PCHAR)BusyBlock >= (PCHAR)Heap->EndAddress
            )
        ) ||
        (BusySegment != NULL &&
            (BusyBlock < (PBUSY_HEADER)(BusySegment+1) ||
             BusyBlock >= (PBUSY_HEADER)((ULONG)BusySegment + BusySegment->Size)
            )
        ) ||
        (BusySize < ALLOCATION_GRANULARITY
        ) ||
        (BusySize & (ALLOCATION_GRANULARITY-1) != 0
        )
#endif // DBG
       ) {
#if DBG
    AlPrint( "ALHEAP: Invalid Address specified to AlRtFreeHeap( %lx, %lx )\n",
                  Heap,
                  BaseAddress
                );
//   RtlpBreakPointHeap();
#endif // DBG
        return( BaseAddress );
        }


    //
    // Free blocks are stored in the free list in ascending order by
    // base address.  As we search the free list to find the place for
    // this block, we remember the previous two free blocks that we
    // passed through.  These are used during combining of adjacent
    // free blocks.
    //

    SecondPrevFreeBlock = NULL;
    PreviousFreeBlock = &Heap->FreeList;
#if DBG
    if (PreviousFreeBlock->Size != 0) {
    AlPrint( "ALHEAP: Heap free list HEAD hosed at %lx\n",
                  PreviousFreeBlock
                );
//   RtlpBreakPointHeap();

        return( BaseAddress );
        }
#endif // DBG

    //
    // Continuous loop.  We'll break out of the loop when we've found
    // the first block of free memory whose address is larger than the
    // address of the block being freed.  (Or the end of the free list.)
    //

    while (TRUE) {

        //
        // Get the address of the next free block.  If we've exhausted
        // the free list, break out of the loop -- the block we're
        // freeing goes at the end of the list.
        //

        FreeBlock = PreviousFreeBlock->Next;

        if (FreeBlock == NULL) {
            break;
            }

#if DBG
        if (FreeBlock->Size == 0) {
        AlPrint( "ALHEAP: Heap free list ENTRY hosed at %lx\n",
                      FreeBlock
                    );
//       RtlpBreakPointHeap();

            return( BaseAddress );
            }
#endif // DBG

        //
        // If the address of the current block is higher than the
        // address of the block we're freeing, break out of the loop.
        // The freed block goes immediately before the current block.
        //

        if (FreeBlock > (PFREE_HEADER)BusyBlock) {
            break;
            }

        //
        // We haven't found the spot yet.  Remember the last two blocks.
        //

        SecondPrevFreeBlock = PreviousFreeBlock;
        PreviousFreeBlock = FreeBlock;

        } // while ( TRUE )


    //
    // We've found the place for the block we're freeing.  If the previous
    // block is adjacent to this one, merge the two by summing their sizes,
    // adjusting the address of the block being freed, and making the second
    // previous block the first previous block.  (Note that the previous
    // block may actually be the listhead.  In this case, the if condition
    // will never be true, because the Size of the listhead is 0.)
    //

    if (((PCH)PreviousFreeBlock + PreviousFreeBlock->Size) == (PCH)BusyBlock &&
        PreviousFreeBlock->Segment == BusySegment
       ) {
        BusySize += PreviousFreeBlock->Size;
        BusyBlock = (PBUSY_HEADER)PreviousFreeBlock;
        PreviousFreeBlock = SecondPrevFreeBlock;
#if DBG
        }
    else
    if ((PreviousFreeBlock != &Heap->FreeList) &&
        ((PCH)PreviousFreeBlock + PreviousFreeBlock->Size) > (PCH)BusyBlock
       ) {
    AlPrint( "ALHEAP: Heap free list overlaps freed block at %lx\n",
                  BusyBlock
                );
//   RtlpBreakPointHeap();

    return( BaseAddress );
#endif // DBG
        }

    //
    // If the block being freed is adjacent to the current block, merge
    // the two by summing their sizes and making the next block the
    // current block.  (Note that the current block may not exist, in
    // which case FreeBlock == NULL, and the if condition will not be
    // true.)
    //*** There is an assumption here that we'll never EVER use the
    //*** very highest part of the address space for user mode allocatable
    //*** memory!
    //

    if (((PCH)BusyBlock + BusySize) == (PCH)FreeBlock &&
        FreeBlock->Segment == BusySegment
       ) {
        BusySize += FreeBlock->Size;
        FreeBlock = FreeBlock->Next;
#if DBG
        if (FreeBlock != NULL) {
            if (FreeBlock->Size == 0) {
        AlPrint( "ALHEAP: Heap free list ENTRY hosed at %lx\n",
                          FreeBlock
                        );
//       RtlpBreakPointHeap();

        return( BaseAddress );
                }
            }
        }
    else
    if ((FreeBlock != NULL) &&
        ((PCH)BusyBlock + BusySize) > (PCH)FreeBlock
       ) {
    AlPrint( "ALHEAP: Freed block overlaps heap free list at %lx\n",
                  BusyBlock
                );
//   RtlpBreakPointHeap();

    return( BaseAddress );
#endif // DBG
        }

    //
    // Done merging.  Update the free list and the free block header.
    //*** May want to reclaim (i.e., release) pages sometime.  That is,
    //*** if we find ourselves with oodles of contiguous pages on the
    //*** free list, we could delete them from our address space.  On
    //*** the other hand, it probably doesn't cost very much to keep
    //*** them around, and if the process needed that much memory once,
    //*** it's likely to need it again.
    //

    PreviousFreeBlock->Next = (PFREE_HEADER)BusyBlock;
    ((PFREE_HEADER)BusyBlock)->Next = FreeBlock;
    ((PFREE_HEADER)BusyBlock)->Size = BusySize;
    ((PFREE_HEADER)BusyBlock)->Segment = BusySegment;

    //
    // Release the free list lock and return to the caller.
    //


    return( NULL );
} // AlRtFreeHeap



PVOID
AlRtReAllocateHeap(
    IN PVOID HeapHandle,
    IN PVOID BaseAddress,
    IN ULONG Size
    )
{
    PHEAP_HEADER Heap = (PHEAP_HEADER)HeapHandle;
    PVOID NewBaseAddress;
    ULONG allocationSize;
    PBUSY_HEADER BusyBlock;
    PBUSY_HEADER ExtraBusyBlock;
    PSEGMENT_HEADER BusySegment;
    ULONG BusyFlags;
    ULONG BusySize;
    LONG DeltaSize;

    //
    // Validate that HeapAddress points to a HEAP_HEADER structure.
    //

    if (Heap->Length != sizeof( HEAP_HEADER )) {
#if DBG
    AlPrint( "ALHEAP: Invalid heap header- %lx\n", Heap );
//   RtlpBreakPointHeap();
#endif // DBG
        return( NULL );
        }

    //
    // Additional check, see if the heap is valid, call the heap validation
    // code, requesting it to not dump stuff.
    //
    if(!AlRtValidateHeap( HeapHandle, FALSE)) {

#if DBG
        AlPrint("Heap validation failed\n");
#endif
        return ( NULL );
    }


    //
    // Round the requested size up to the allocation granularity.  Note
    // that if the request is for 0 bytes, we still allocate memory.
    //

    allocationSize = ((Size ? Size : ALLOCATION_GRANULARITY) +
                      sizeof( BUSY_HEADER ) +
                      ALLOCATION_GRANULARITY -
                      1
                     ) & ~(ALLOCATION_GRANULARITY - 1);

    if (allocationSize < Size) {
#if DBG
    AlPrint( "ALHEAP: Invalid heap size - %lx\n", Size );
//   RtlpBreakPointHeap();
#endif // DBG
        return( NULL );
        }

    if (BaseAddress == NULL) {
#if DBG
    AlPrint( "ALHEAP: Invalid heap address - %lx\n", BaseAddress );
//   RtlpBreakPointHeap();
#endif // DBG
        return( NULL );
        }

    //
    // Get the 'real' address of the allocation unit.  (That is, the
    // address of the allocation header.)  Make sure the address lies
    // within the bounds of the valid portion of the heap.
    //

    BusyBlock = (PBUSY_HEADER)BaseAddress - 1;
    BusySize = BusyBlock->Size;
    BusySegment = (PSEGMENT_HEADER)((ULONG)BusyBlock->Segment & ~FLAGS_MASK);
    BusyFlags = (ULONG)BusyBlock->Segment & FLAGS_MASK;

    if (BusyFlags != FLAGS_BUSY
#if DBG
        || (BusySegment == NULL &&
            ((PCHAR)BusyBlock < (PCHAR)Heap->ValidAddress ||
         (PCHAR)BusyBlock >= (PCHAR)Heap->EndAddress
            )
        ) ||
        (BusySegment != NULL &&
            (BusyBlock < (PBUSY_HEADER)(BusySegment+1) ||
             BusyBlock >= (PBUSY_HEADER)((ULONG)BusySegment + BusySegment->Size)
            )
        ) ||
        (BusySize < ALLOCATION_GRANULARITY
        ) ||
        (BusySize & (ALLOCATION_GRANULARITY-1) != 0
        )
#endif // DBG
       ) {
#if DBG
    AlPrint( "ALHEAP: Invalid Address specified to AlRtFreeHeap( %lx, %lx )\n",
                  Heap,
                  BaseAddress
                );
//   RtlpBreakPointHeap();
#endif // DBG
        return( NULL );
        }


    //
    // See if new size less than or equal to the current size.
    //

    DeltaSize = (LONG)(BusySize - allocationSize);
    if (DeltaSize >= 0) {
        //
        // Then shrinking block.  If amount of shrinkage is less than
        // the size of a free block, then nothing to do.
        //

        if (DeltaSize < sizeof( FREE_HEADER )) {
            if (Heap->ZeroExtraMemory) {
        memset( (PCHAR)BusyBlock + (allocationSize - DeltaSize),0,
                               DeltaSize
                             );
                }

            return( BaseAddress );
            }

        //
        // Otherwise, shrink size of this block to new size, and make extra
    // space at end look like another busy block and call AlRtFreeHeap
        // to free it.
        //

        BusyBlock->Size = allocationSize;
        ExtraBusyBlock = (PBUSY_HEADER)((PCH)BusyBlock + allocationSize);
        ExtraBusyBlock->Segment = BusyBlock->Segment;
        ExtraBusyBlock->Size = (ULONG)(DeltaSize);
#if DBG
    ExtraBusyBlock->Spare = 0;
#endif

    AlRtFreeHeap( HeapHandle, (PVOID)(ExtraBusyBlock+1) );

        if (Heap->ZeroExtraMemory) {
            ULONG extraSize;

            extraSize = allocationSize - Size - sizeof( BUSY_HEADER );
        memset( (PCHAR)BusyBlock + (allocationSize - extraSize),0,
                           extraSize
                         );
            }

        return( BaseAddress );
        }

    //
    // Otherwise growing block, so allocate a new block with the bigger
    // size, copy the contents of the old block to the new block and then
    // free the old block.  Return the address of the new block.
    //

    NewBaseAddress = AlRtAllocateHeap( HeapHandle, Size );
    if (NewBaseAddress != NULL) {
#if DBG
        ExtraBusyBlock = (PBUSY_HEADER)NewBaseAddress - 1;
    ExtraBusyBlock->Spare = 0;
#endif
    memmove( NewBaseAddress,
                       BaseAddress,
                       BusySize - sizeof( BUSY_HEADER )
                     );

    AlRtFreeHeap( HeapHandle, BaseAddress );
        }

    return( NewBaseAddress );
}




ARC_STATUS
AlMemoryInitialize (
    ULONG StackPages,
    ULONG HeapPages
    )

/*++

Routine Description:

    This routine allocates stack space for the OS loader, initializes
    heap storage, and initializes the memory allocation list.

Arguments:

    None.

Return Value:

    ESUCCESS is returned if the initialization is successful. Otherwise,
    ENOMEM is returned.

--*/

{

    PMEMORY_DESCRIPTOR FreeDescriptor;
    PMEMORY_DESCRIPTOR ProgramDescriptor;

    //
    // Find the memory descriptor that describes the allocation for the OS
    // loader itself.
    //

    ProgramDescriptor = NULL;
    while ((ProgramDescriptor = ArcGetMemoryDescriptor(ProgramDescriptor)) != NULL) {
        if (ProgramDescriptor->MemoryType == MemoryLoadedProgram) {
            break;
        }
    }

    //
    // If a loaded program memory descriptor was found, then it must be
    // for the OS loader since that is the only program that can be loaded.
    // If a loaded program memory descriptor was not found, then firmware
    // is not functioning properly and an unsuccessful status is returned.
    //

    if (ProgramDescriptor == NULL) {
        return ENOMEM;
    }

    //
    // Find the free memory descriptor that is just below the loaded
    // program in memory. There should be several megabytes of free
    // memory just preceeding the OS loader.
    //

    FreeDescriptor = NULL;
    while ((FreeDescriptor = ArcGetMemoryDescriptor(FreeDescriptor)) != NULL) {
        if ((FreeDescriptor->MemoryType == MemoryFree) &&
            (FreeDescriptor->PageCount >= (StackPages+HeapPages))) {
            break;
        }
    }

    //
    // If a free memory descriptor was not found that describes the free
    // memory just below the OS loader, then firmware is not functioning
    // properly and an unsuccessful status is returned.
    //

    if (FreeDescriptor == NULL) {
        return ENOMEM;
    }

    //
    // Check to determine if enough free memory is available for the OS
    // loader stack and the heap area. If enough memory is not available,
    // then return an unsuccessful status.
    //

    if (FreeDescriptor->PageCount < (StackPages + HeapPages)) {
        return ENOMEM;
    }

    //
    // Compute the address of the loader heap, initialize the heap
    // allocation variables, and zero the heap memory.
    //

    AlHeapFree = KSEG0_BASE | ((ProgramDescriptor->BasePage -
                (StackPages + HeapPages)) << PAGE_SHIFT);

    AlHeapLimit = AlHeapFree + (HeapPages << PAGE_SHIFT);

    memset((PVOID)AlHeapFree, 0,HeapPages << PAGE_SHIFT);


    //
    // Changed to new heap allocater
    //

    if ((HeapHandle = AlRtCreateHeap
            (
            HEAP_ZERO_EXTRA_MEMORY,
            (PVOID)AlHeapFree,
            HeapPages << PAGE_SHIFT
            ))
            == NULL)
       return ENOMEM;
    else
       return ESUCCESS;

}


//
// AlAllocateHeap.
//
//    Heap space allocator.  Size is in bytes required.

PVOID
AlAllocateHeap (
    IN ULONG Size
    )
{
    return (AlRtAllocateHeap
         (
         HeapHandle,
         Size
         ));

}



// 3. AlDeallocateHeap
//
//    Heap Deallocation needs to be defined and implemented.
//
//

PVOID
AlDeallocateHeap (
    IN PVOID HeapAddress
    )
{
    return (AlRtFreeHeap
        (
        HeapHandle,
        HeapAddress
        ));
}


//
// 4. AlReallocateHeap
//
//
//

PVOID
AlReallocateHeap (
    IN PVOID HeapAddress,
    IN ULONG NewSize
    )
{
    return (AlRtReAllocateHeap
        (
        HeapHandle,
        HeapAddress,
        NewSize
        ));
}

//
// 5. AlValidateHeap
//
//    Heap validation
//
//

BOOLEAN
AlValidateHeap(
    IN BOOLEAN DumpHeap
    )
{
    return (AlRtValidateHeap
                (
                HeapHandle,
                DumpHeap
                ));
}

