/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    heap.c

Abstract:

    This module implements a heap allocator.

Author:

    Steve Wood (stevewo) 20-Sep-1989 (Adapted from URTL\alloc.c)

Revision History:

--*/

#include "ntrtlp.h"
#include "heap.h"
#include "heappriv.h"

#ifdef NTHEAP_ENABLED
#include "heapp.h"
#endif // NTHEAP_ENABLED

ULONG RtlpDisableHeapLookaside = 0;

//
//  If any of these flags are set, the fast allocator punts
//  to the slow do-everything allocator.
//

#define HEAP_SLOW_FLAGS (HEAP_DEBUG_FLAGS           | \
                         HEAP_SETTABLE_USER_FLAGS   | \
                         HEAP_NEED_EXTRA_FLAGS      | \
                         HEAP_CREATE_ALIGN_16       | \
                         HEAP_FREE_CHECKING_ENABLED | \
                         HEAP_TAIL_CHECKING_ENABLED)

UCHAR CheckHeapFillPattern[ CHECK_HEAP_TAIL_SIZE ] = {
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
#ifdef _WIN64
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
    CHECK_HEAP_TAIL_FILL,
#endif
    CHECK_HEAP_TAIL_FILL
};

//
//  An extra bitmap manipulation routine
//

#define RtlFindFirstSetRightMember(Set)                     \
    (((Set) & 0xFFFF) ?                                     \
        (((Set) & 0xFF) ?                                   \
            RtlpBitsClearLow[(Set) & 0xFF] :                \
            RtlpBitsClearLow[((Set) >> 8) & 0xFF] + 8) :    \
        ((((Set) >> 16) & 0xFF) ?                           \
            RtlpBitsClearLow[ ((Set) >> 16) & 0xFF] + 16 :  \
            RtlpBitsClearLow[ (Set) >> 24] + 24)            \
    )


//
//  These are procedure prototypes exported by heapdbg.c
//

#ifndef NTOS_KERNEL_RUNTIME

PVOID
RtlDebugCreateHeap (
    IN ULONG Flags,
    IN PVOID HeapBase OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    );

BOOLEAN
RtlDebugDestroyHeap (
    IN PVOID HeapHandle
    );

PVOID
RtlDebugAllocateHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

BOOLEAN
RtlDebugFreeHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

ULONG
RtlDebugSizeHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

NTSTATUS
RtlDebugZeroHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags
    );

#endif // NTOS_KERNEL_RUNTIME


//
//  Local procedure prototypes
//

PHEAP_UNCOMMMTTED_RANGE
RtlpCreateUnCommittedRange (
    IN PHEAP_SEGMENT Segment
    );

VOID
RtlpDestroyUnCommittedRange (
    IN PHEAP_SEGMENT Segment,
    IN PHEAP_UNCOMMMTTED_RANGE UnCommittedRange
    );

VOID
RtlpInsertUnCommittedPages (
    IN PHEAP_SEGMENT Segment,
    IN ULONG_PTR Address,
    IN SIZE_T Size
    );

NTSTATUS
RtlpDestroyHeapSegment (
    IN PHEAP_SEGMENT Segment
    );

PHEAP_FREE_ENTRY
RtlpExtendHeap (
    IN PHEAP Heap,
    IN SIZE_T AllocationSize
    );

#ifdef ALLOC_PRAGMA

#pragma alloc_text(PAGE, RtlCreateHeap)
#pragma alloc_text(PAGE, RtlDestroyHeap)
#pragma alloc_text(PAGE, RtlAllocateHeap)
#pragma alloc_text(PAGE, RtlFreeHeap)
#pragma alloc_text(PAGE, RtlSizeHeap)
#pragma alloc_text(PAGE, RtlZeroHeap)

#pragma alloc_text(PAGE, RtlpCreateUnCommittedRange)
#pragma alloc_text(PAGE, RtlpDestroyUnCommittedRange)
#pragma alloc_text(PAGE, RtlpInsertUnCommittedPages)
#pragma alloc_text(PAGE, RtlpDestroyHeapSegment)
#pragma alloc_text(PAGE, RtlpExtendHeap)

#pragma alloc_text(PAGE, RtlpFindAndCommitPages)
#pragma alloc_text(PAGE, RtlpInitializeHeapSegment)
#pragma alloc_text(PAGE, RtlpCoalesceFreeBlocks)
#pragma alloc_text(PAGE, RtlpDeCommitFreeBlock)
#pragma alloc_text(PAGE, RtlpInsertFreeBlock)
#pragma alloc_text(PAGE, RtlpGetSizeOfBigBlock)
#pragma alloc_text(PAGE, RtlpCheckBusyBlockTail)

#endif // ALLOC_PRAGMA


PVOID

RtlCreateHeap (
    IN ULONG Flags,
    IN PVOID HeapBase OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters OPTIONAL
    )

/*++

Routine Description:

    This routine initializes a heap.

Arguments:

    Flags - Specifies optional attributes of the heap.

        Valid Flags Values:

        HEAP_NO_SERIALIZE - if set, then allocations and deallocations on
                         this heap are NOT synchronized by these routines.

        HEAP_GROWABLE - if set, then the heap is a "sparse" heap where
                        memory is committed only as necessary instead of
                        being preallocated.

    HeapBase - if not NULL, this specifies the base address for memory
        to use as the heap.  If NULL, memory is allocated by these routines.

    ReserveSize - if not zero, this specifies the amount of virtual address
        space to reserve for the heap.

    CommitSize - if not zero, this specifies the amount of virtual address
        space to commit for the heap.  Must be less than ReserveSize.  If
        zero, then defaults to one page.

    Lock - if not NULL, this parameter points to the resource lock to
        use.  Only valid if HEAP_NO_SERIALIZE is NOT set.

    Parameters - optional heap parameters.

Return Value:

    PVOID - a pointer to be used in accessing the created heap.

--*/

{
    ULONG_PTR HighestUserAddress;
    NTSTATUS Status;
    PHEAP Heap = NULL;
    PHEAP_SEGMENT Segment = NULL;
    PLIST_ENTRY FreeListHead;
    ULONG SizeOfHeapHeader;
    ULONG SegmentFlags;
    PVOID CommittedBase;
    PVOID UnCommittedBase;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    SYSTEM_BASIC_INFORMATION SystemInformation;
    ULONG n;
    ULONG InitialCountOfUnusedUnCommittedRanges;
    SIZE_T MaximumHeapBlockSize;
    PVOID NextHeapHeaderAddress;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;
    RTL_HEAP_PARAMETERS TempParameters;
    ULONG NtGlobalFlag = RtlGetNtGlobalFlags();

#ifndef NTOS_KERNEL_RUNTIME

    PPEB Peb;

#else // NTOS_KERNEL_RUNTIME

    extern SIZE_T MmHeapSegmentReserve;
    extern SIZE_T MmHeapSegmentCommit;
    extern SIZE_T MmHeapDeCommitTotalFreeThreshold;
    extern SIZE_T MmHeapDeCommitFreeBlockThreshold;

#endif // NTOS_KERNEL_RUNTIME

    RTL_PAGED_CODE();

#ifndef NTOS_KERNEL_RUNTIME
#ifdef NTHEAP_ENABLED
    {
        if (Flags & NTHEAP_ENABLED_FLAG) {

            Heap = RtlCreateNtHeap( Flags, NULL );

            if (Heap != NULL) {

                return Heap;
            }

            Flags &= ~NTHEAP_ENABLED_FLAG;
        }
    }
#endif // NTHEAP_ENABLED
#endif // NTOS_KERNEL_RUNTIME

    //
    //  Check if we should be using the page heap code.  If not then turn
    //  off any of the page heap flags before going on
    //

#ifdef DEBUG_PAGE_HEAP

    if ( RtlpDebugPageHeap && ( HeapBase == NULL ) && ( Lock == NULL )) {

        PVOID PageHeap;

        PageHeap = RtlpDebugPageHeapCreate(

            Flags,
            HeapBase,
            ReserveSize,
            CommitSize,
            Lock,
            Parameters );

        if (PageHeap != NULL) {
            return PageHeap;
        }

        //
        // A `-1' value signals a recursive call from page heap
        // manager. We set this to null and continue creating
        // a normal heap. This small hack is required so that we
        // minimize the dependencies between the normal and the page
        // heap manager.
        //

        if ((SIZE_T)Parameters == (SIZE_T)-1) {

            Parameters = NULL;
        }
    }

    Flags &= ~( HEAP_PROTECTION_ENABLED |
        HEAP_BREAK_WHEN_OUT_OF_VM |
        HEAP_NO_ALIGNMENT );

#endif // DEBUG_PAGE_HEAP

    //
    //  If the caller does not want to skip heap validiation checks then we
    //  need to validate the rest of the flags but simply masking out only
    //  those flags that want on a create heap call
    //

    if (!(Flags & HEAP_SKIP_VALIDATION_CHECKS)) {

        if (Flags & ~HEAP_CREATE_VALID_MASK) {

            HeapDebugPrint(( "Invalid flags (%08x) specified to RtlCreateHeap\n", Flags ));
            HeapDebugBreak( NULL );

            Flags &= HEAP_CREATE_VALID_MASK;
        }
    }

    //
    //  The maximum heap block size is really 0x7f000 which is 0x80000 minus a
    //  page.  Maximum block size is 0xfe00 and granularity shift is 3.
    //

    MaximumHeapBlockSize = HEAP_MAXIMUM_BLOCK_SIZE << HEAP_GRANULARITY_SHIFT;

    //
    //  Assume we're going to be successful until we're shown otherwise
    //

    Status = STATUS_SUCCESS;

    //
    //  This part of the routine builds up local variable containing all the
    //  parameters used to initialize the heap.  First thing we do is zero
    //  it out.
    //

    RtlZeroMemory( &TempParameters, sizeof( TempParameters ) );

    //
    //  If our caller supplied the optional heap parameters then we'll
    //  make sure the size is good and copy over them over to our
    //  local copy
    //

    if (ARGUMENT_PRESENT( Parameters )) {

        try {

            if (Parameters->Length == sizeof( *Parameters )) {

                RtlMoveMemory( &TempParameters, Parameters, sizeof( *Parameters ) );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

        if (!NT_SUCCESS( Status )) {

            return NULL;
        }
    }

    //
    //  Set the parameter block to the local copy
    //

    Parameters = &TempParameters;

    //
    //  If nt global flags tells us to always do tail or free checking
    //  or to disable coalescing then force those bits set in the user
    //  specified flags
    //

    if (NtGlobalFlag & FLG_HEAP_ENABLE_TAIL_CHECK) {

        Flags |= HEAP_TAIL_CHECKING_ENABLED;
    }

    if (NtGlobalFlag & FLG_HEAP_ENABLE_FREE_CHECK) {

        Flags |= HEAP_FREE_CHECKING_ENABLED;
    }

    if (NtGlobalFlag & FLG_HEAP_DISABLE_COALESCING) {

        Flags |= HEAP_DISABLE_COALESCE_ON_FREE;
    }

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case we also check if we should
    //  validate parameters, validate all, or do stack backtraces
    //

    Peb = NtCurrentPeb();

    if (NtGlobalFlag & FLG_HEAP_VALIDATE_PARAMETERS) {

        Flags |= HEAP_VALIDATE_PARAMETERS_ENABLED;
    }

    if (NtGlobalFlag & FLG_HEAP_VALIDATE_ALL) {

        Flags |= HEAP_VALIDATE_ALL_ENABLED;
    }

    if (NtGlobalFlag & FLG_USER_STACK_TRACE_DB) {

        Flags |= HEAP_CAPTURE_STACK_BACKTRACES;
    }

    //
    //  Also in the non kernel case the PEB will have some state
    //  variables that we need to set if the user hasn't specified
    //  otherwise
    //

    if (Parameters->SegmentReserve == 0) {

        Parameters->SegmentReserve = Peb->HeapSegmentReserve;
    }

    if (Parameters->SegmentCommit == 0) {

        Parameters->SegmentCommit = Peb->HeapSegmentCommit;
    }

    if (Parameters->DeCommitFreeBlockThreshold == 0) {

        Parameters->DeCommitFreeBlockThreshold = Peb->HeapDeCommitFreeBlockThreshold;
    }

    if (Parameters->DeCommitTotalFreeThreshold == 0) {

        Parameters->DeCommitTotalFreeThreshold = Peb->HeapDeCommitTotalFreeThreshold;
    }
#else // NTOS_KERNEL_RUNTIME

    //
    //  In the kernel case Mm has some global variables that we set
    //  into the paramters if the user hasn't specified otherwise
    //

    if (Parameters->SegmentReserve == 0) {

        Parameters->SegmentReserve = MmHeapSegmentReserve;
    }

    if (Parameters->SegmentCommit == 0) {

        Parameters->SegmentCommit = MmHeapSegmentCommit;
    }

    if (Parameters->DeCommitFreeBlockThreshold == 0) {

        Parameters->DeCommitFreeBlockThreshold = MmHeapDeCommitFreeBlockThreshold;
    }

    if (Parameters->DeCommitTotalFreeThreshold == 0) {

        Parameters->DeCommitTotalFreeThreshold = MmHeapDeCommitTotalFreeThreshold;
    }
#endif // NTOS_KERNEL_RUNTIME

    //
    //  Get the highest user address
    //

    if (!NT_SUCCESS(ZwQuerySystemInformation(SystemBasicInformation,
                                             &SystemInformation,
                                             sizeof(SystemInformation),
                                             NULL))) {
        return NULL;
    }
    HighestUserAddress = SystemInformation.MaximumUserModeAddress;

    //
    //  If the user hasn't said what the largest allocation size is then
    //  we should compute it as the difference between the highest and lowest
    //  address less one page
    //

    if (Parameters->MaximumAllocationSize == 0) {

        Parameters->MaximumAllocationSize = (HighestUserAddress -
                                             (ULONG_PTR)MM_LOWEST_USER_ADDRESS -
                                             PAGE_SIZE );
    }

    //
    //  Set the virtual memory threshold to be non zero and not more than the
    //  maximum heap block size of 0x7f000.  If the user specified one that is
    //  too large we automatically and silently drop it down.
    //

    if ((Parameters->VirtualMemoryThreshold == 0) ||
        (Parameters->VirtualMemoryThreshold > MaximumHeapBlockSize)) {

        Parameters->VirtualMemoryThreshold = MaximumHeapBlockSize;
    }

    //
    //  The default commit size is one page and the default reserve size is
    //  64 pages.
    //
    //  **** this doesn't check that commit size if specified is less than
    //  **** reserved size if specified
    //

    if (!ARGUMENT_PRESENT( CommitSize )) {

        CommitSize = PAGE_SIZE;

        if (!ARGUMENT_PRESENT( ReserveSize )) {

            ReserveSize = 64 * CommitSize;

        } else {

            ReserveSize = ROUND_UP_TO_POWER2( ReserveSize, PAGE_SIZE );
        }

    } else {

        //
        //  The heap actually uses space that is reserved and commited
        //  to store internal data structures (the LOCK,
        //  the HEAP_PSEUDO_TAG, etc.). These structures can be larger than
        //  4K especially on a 64-bit build. So, make sure the commit
        //  is at least 8K in length which is the minimal page size for
        //  64-bit systems
        //

        CommitSize = ROUND_UP_TO_POWER2(CommitSize, PAGE_SIZE);

        if (!ARGUMENT_PRESENT( ReserveSize )) {

            ReserveSize = ROUND_UP_TO_POWER2( CommitSize, 16 * PAGE_SIZE );

        } else {

            ReserveSize = ROUND_UP_TO_POWER2( ReserveSize, PAGE_SIZE );
        }

    }

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case check if we are creating a debug heap
    //  the test checks that skip validation checks is false.
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugCreateHeap( Flags,
                                   HeapBase,
                                   ReserveSize,
                                   CommitSize,
                                   Lock,
                                   Parameters );
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  Compute the size of the heap which will be the
    //  heap struct itself and if we are to seralize with
    //  out own lock then add room for the lock.  If the
    //  user did not supply the lock then set the lock
    //  variable to -1.
    //

    SizeOfHeapHeader = sizeof( HEAP );

    if (!(Flags & HEAP_NO_SERIALIZE)) {

        if (ARGUMENT_PRESENT( Lock )) {

            Flags |= HEAP_LOCK_USER_ALLOCATED;

        } else {

            SizeOfHeapHeader += sizeof( HEAP_LOCK );
            Lock = (PHEAP_LOCK)-1;
        }

    } else if (ARGUMENT_PRESENT( Lock )) {

        //
        //  In this error case the call said not to seralize but also fed us
        //  a lock
        //

        return NULL;
    }

    //
    //  See if caller allocate the space for the heap.
    //

    if (ARGUMENT_PRESENT( HeapBase )) {

        //
        //  The call specified a heap base now check if there is
        //  a caller supplied commit routine
        //

        if (Parameters->CommitRoutine != NULL) {

            //
            //  The caller specified a commit routine so he caller
            //  also needs to have given us certain parameters and make
            //  sure the heap is not growable.  Otherwise it is an error
            //

            if ((Parameters->InitialCommit == 0) ||
                (Parameters->InitialReserve == 0) ||
                (Parameters->InitialCommit > Parameters->InitialReserve) ||
                (Flags & HEAP_GROWABLE)) {

                return NULL;
            }

            //
            //  Set the commited base and the uncommited base to the
            //  proper pointers within the heap.
            //

            CommittedBase = HeapBase;
            UnCommittedBase = (PCHAR)CommittedBase + Parameters->InitialCommit;
            ReserveSize = Parameters->InitialReserve;

            //
            //  Zero out a page of the heap where our first part goes
            //
            //  **** what if the size is less than a page
            //

            RtlZeroMemory( CommittedBase, PAGE_SIZE );

        } else {

            //
            //  The user gave us space but not commit routine
            //  So query the base to get its size
            //

            Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                           HeapBase,
                                           MemoryBasicInformation,
                                           &MemoryInformation,
                                           sizeof( MemoryInformation ),
                                           NULL );

            if (!NT_SUCCESS( Status )) {

                return NULL;
            }

            //
            //  Make sure the user gave us a base address for this block
            //  and that the memory is not free
            //

            if (MemoryInformation.BaseAddress != HeapBase) {

                return NULL;
            }

            if (MemoryInformation.State == MEM_FREE) {

                return NULL;
            }

            //
            //  Set our commit base to the start of the range
            //

            CommittedBase = MemoryInformation.BaseAddress;

            //
            //  If the memory is commmitted then
            //  we can zero out a page worth
            //

            if (MemoryInformation.State == MEM_COMMIT) {

                RtlZeroMemory( CommittedBase, PAGE_SIZE );

                //
                //  Set the commit size and uncommited base according
                //  to the start of the vm
                //

                CommitSize = MemoryInformation.RegionSize;
                UnCommittedBase = (PCHAR)CommittedBase + CommitSize;

                //
                //  Find out the uncommited base is reserved and if so
                //  the update the reserve size accordingly.
                //

                Status = ZwQueryVirtualMemory( NtCurrentProcess(),
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
                //  The memory the user gave us is not committed so dummy
                //  up these small nummbers
                //

                CommitSize = PAGE_SIZE;
                UnCommittedBase = CommittedBase;
            }
        }

        //
        //  This user gave us a base and we've just taken care of the committed
        //  bookkeeping.  So mark this segment as user supplied and set the
        //  heap
        //

        SegmentFlags = HEAP_SEGMENT_USER_ALLOCATED;
        Heap = (PHEAP)HeapBase;

    } else {

        //
        //  The user did not specify a heap base so we have to allocate the
        //  vm here.  First make sure the user did not give us a commit routine
        //

        if (Parameters->CommitRoutine != NULL) {

            return NULL;
        }

        //
        //  Reserve the amount of virtual address space requested.
        //

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&Heap,
                                          0,
                                          &ReserveSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE );

        if (!NT_SUCCESS( Status )) {

            return NULL;
        }

        //
        //  Indicate that this segment is not user supplied
        //

        SegmentFlags = 0;

        //
        //  Set the default commit size to one page
        //

        if (!ARGUMENT_PRESENT( CommitSize )) {

            CommitSize = PAGE_SIZE;
        }

        //
        //  Set the committed and uncommitted base to be the same the following
        //  code will actually commit the page for us
        //

        CommittedBase = Heap;
        UnCommittedBase = Heap;
    }

    //
    //  At this point we have a heap pointer, committed base, uncommitted base,
    //  segment flags, commit size, and reserve size.  If the committed and
    //  uncommited base are the same then we need to commit the amount
    //  specified by the commit size
    //

    if (CommittedBase == UnCommittedBase) {

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&CommittedBase,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );

        //
        //  In the non successful case we need to back out any vm reservation
        //  we did earlier
        //

        if (!NT_SUCCESS( Status )) {

            if (!ARGUMENT_PRESENT(HeapBase)) {

                //
                //  Return the reserved virtual address space.
                //

                ZwFreeVirtualMemory( NtCurrentProcess(),
                                     (PVOID *)&Heap,
                                     &ReserveSize,
                                     MEM_RELEASE );

            }

            return NULL;
        }

        //
        //  The new uncommitted base is not adjusted above what we just
        //  committed
        //

        UnCommittedBase = (PVOID)((PCHAR)UnCommittedBase + CommitSize);
    }

    //
    //  At this point we have memory for the start of the heap committed and
    //  ready to be initialized.  So now we need initialize the heap
    //

    //
    //  Calculate the end of the heap header and make room for 8 uncommitted
    //  range structures.  Once we have the room for them then chain them
    //  together and null terminate the chain
    //

    NextHeapHeaderAddress = Heap + 1;

    UnCommittedRange = (PHEAP_UNCOMMMTTED_RANGE)ROUND_UP_TO_POWER2( NextHeapHeaderAddress,
                                                                    sizeof( QUAD ) );

    InitialCountOfUnusedUnCommittedRanges = 8;

    SizeOfHeapHeader += InitialCountOfUnusedUnCommittedRanges * sizeof( *UnCommittedRange );

    //
    //  **** what a hack Pp is really a pointer to the next field of the
    //  **** uncommmtted range structure.  So we set next by setting through Pp
    //

    pp = &Heap->UnusedUnCommittedRanges;

    while (InitialCountOfUnusedUnCommittedRanges--) {

        *pp = UnCommittedRange;
        pp = &UnCommittedRange->Next;
        UnCommittedRange += 1;
    }

    NextHeapHeaderAddress = UnCommittedRange;

    *pp = NULL;

    //
    //  Check if tagging is enabled in global flags.  This check is always true
    //  in a debug build.
    //
    //  If tagging is enabled then make room for 129 pseudo tag heap entry.
    //  Which is one more than the number of free lists.  Also point the heap
    //  header to this array of pseudo tags entries.
    //

    if (IS_HEAP_TAGGING_ENABLED()) {

        Heap->PseudoTagEntries = (PHEAP_PSEUDO_TAG_ENTRY)ROUND_UP_TO_POWER2( NextHeapHeaderAddress,
                                                                             sizeof( QUAD ) );

        SizeOfHeapHeader += HEAP_NUMBER_OF_PSEUDO_TAG * sizeof( HEAP_PSEUDO_TAG_ENTRY );

        //
        //  **** this advancement of the next heap address doesn't seem right
        //  **** given that a pseudo heap entry is 12 ulongs in length and not
        //  **** a single byte

        NextHeapHeaderAddress = Heap->PseudoTagEntries + HEAP_NUMBER_OF_PSEUDO_TAG;
    }

    //
    //  Round the size of the heap header to the next 8 byte boundary
    //

    SizeOfHeapHeader = (ULONG) ROUND_UP_TO_POWER2( SizeOfHeapHeader,
                                                   HEAP_GRANULARITY );

    //
    //  If the sizeof the heap header is larger than the native
    //  page size, you have a problem. Further, if the CommitSize passed
    //  in was smaller than the SizeOfHeapHeader, you may not even make it
    //  this far before death...
    //
    //  HeapDbgPrint() doesn't work for IA64 yet.
    //
    //  HeapDbgPrint(("Size of the heap header is %u bytes, commit was %u bytes\n", SizeOfHeapHeader, (ULONG) CommitSize));
    //

    //
    //  Fill in the heap header fields
    //

    Heap->Entry.Size = (USHORT)(SizeOfHeapHeader >> HEAP_GRANULARITY_SHIFT);
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

    Heap->FreeListsInUseTerminate = 0xFFFF;
    Heap->HeaderValidateLength = (USHORT)((PCHAR)NextHeapHeaderAddress - (PCHAR)Heap);
    Heap->HeaderValidateCopy = NULL;

    //
    //  Initialize the free list to be all empty
    //

    FreeListHead = &Heap->FreeLists[ 0 ];
    n = HEAP_MAXIMUM_FREELISTS;

    while (n--) {

        InitializeListHead( FreeListHead );
        FreeListHead++;
    }

    //
    //  Make it so that there a no big block allocations
    //

    InitializeListHead( &Heap->VirtualAllocdBlocks );

    //
    //  Initialize the cricital section that controls access to
    //  the free list.  If the lock variable is -1 then the caller
    //  did not supply a lock so we need to make room for one
    //  and initialize it.
    //

    if (Lock == (PHEAP_LOCK)-1) {

        Lock = (PHEAP_LOCK)NextHeapHeaderAddress;

        Status = RtlInitializeLockRoutine( Lock );

        if (!NT_SUCCESS( Status )) {

            return NULL;
        }

        NextHeapHeaderAddress = (PHEAP_LOCK)Lock + 1;
    }

    Heap->LockVariable = Lock;


    //
    //  Initialize the first segment for the heap
    //

    if (!RtlpInitializeHeapSegment( Heap,
                                    (PHEAP_SEGMENT)((PCHAR)Heap + SizeOfHeapHeader),
                                    0,
                                    SegmentFlags,
                                    CommittedBase,
                                    UnCommittedBase,
                                    (PCHAR)CommittedBase + ReserveSize )) {

        return NULL;
    }

    //
    //  Fill in additional heap entry fields
    //

    Heap->ProcessHeapsListIndex = 0;
    Heap->SegmentReserve = Parameters->SegmentReserve;
    Heap->SegmentCommit = Parameters->SegmentCommit;
    Heap->DeCommitFreeBlockThreshold = Parameters->DeCommitFreeBlockThreshold >> HEAP_GRANULARITY_SHIFT;
    Heap->DeCommitTotalFreeThreshold = Parameters->DeCommitTotalFreeThreshold >> HEAP_GRANULARITY_SHIFT;
    Heap->MaximumAllocationSize = Parameters->MaximumAllocationSize;

    Heap->VirtualMemoryThreshold = (ULONG) (ROUND_UP_TO_POWER2( Parameters->VirtualMemoryThreshold,
                                                       HEAP_GRANULARITY ) >> HEAP_GRANULARITY_SHIFT);

    Heap->CommitRoutine = Parameters->CommitRoutine;

    //
    //  We either align the heap at 16 or 8 byte boundaries.  The AlignRound
    //  and AlignMask are used to bring allocation sizes up to the next
    //  boundary.  The align round includes the heap header and the optional
    //  check tail size
    //

    if (Flags & HEAP_CREATE_ALIGN_16) {

        Heap->AlignRound = 15 + sizeof( HEAP_ENTRY );
        Heap->AlignMask = (ULONG)~15;

    } else {

        Heap->AlignRound = HEAP_GRANULARITY - 1 + sizeof( HEAP_ENTRY );
        Heap->AlignMask = (ULONG)~(HEAP_GRANULARITY - 1);
    }

    if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {

        Heap->AlignRound += CHECK_HEAP_TAIL_SIZE;
    }

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case we need to add this heap to the processes heap
    //  list
    //

    RtlpAddHeapToProcessList( Heap );

    //
    //  Initialize the heap lookaide lists.  This is only for the user mode
    //  heap and the heap contains a pointer to the lookaside list array.
    //  The array is sized the same as the dedicated free list.  First we
    //  allocate space for the lookaside list and then we initialize each
    //  lookaside list.
    //
    //  But the caller asked for no serialize or asked for non growable
    //  heap then we won't enable the lookaside lists.
    //

    Heap->Lookaside = NULL;
    Heap->LookasideLockCount = 0;

    if ((!(Flags & HEAP_NO_SERIALIZE)) &&
        ( (Flags & HEAP_GROWABLE)) &&
        (!(RtlpDisableHeapLookaside))) {

        ULONG i;

        Heap->Lookaside = RtlAllocateHeap( Heap,
                                           Flags,
                                           sizeof(HEAP_LOOKASIDE) * HEAP_MAXIMUM_FREELISTS );

        if (Heap->Lookaside != NULL) {

            for (i = 0; i < HEAP_MAXIMUM_FREELISTS; i += 1) {

                RtlpInitializeHeapLookaside( &(((PHEAP_LOOKASIDE)(Heap->Lookaside))[i]),
                                             32 );
            }
        }
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  And return the fully initialized heap to our caller
    //

    return (PVOID)Heap;
}


PVOID
RtlDestroyHeap (
    IN PVOID HeapHandle
    )

/*++

Routine Description:

    This routine is the opposite of Rtl Create Heap.  It tears down an
    existing heap structure.

Arguments:

    HeapHandle - Supplies a pointer to the heap being destroyed

Return Value:

    PVOID - Returns null if the heap was destroyed completely and a
        pointer back to the heap if for some reason the heap could
        not be destroyed.

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_SEGMENT Segment;
    PHEAP_UCR_SEGMENT UCRSegments;
    PLIST_ENTRY Head, Next;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    UCHAR SegmentIndex;

    //
    //  Validate that HeapAddress points to a HEAP structure.
    //

    RTL_PAGED_CODE();

    if (HeapHandle == NULL) {

        HeapDebugPrint(( "Ignoring RtlDestroyHeap( NULL )\n" ));

        return NULL;
    }

#ifndef NTOS_KERNEL_RUNTIME
#ifdef NTHEAP_ENABLED
    {
        if (Heap->Flags & NTHEAP_ENABLED_FLAG) {

            return RtlDestroyNtHeap( HeapHandle );
        }
    }
#endif // NTHEAP_ENABLED
#endif // NTOS_KERNEL_RUNTIME

    //
    //  Check if this is the debug version of heap using page allocation
    //  with guard pages
    //

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapDestroy( HeapHandle ));

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case check if this is the debug version of heap
    //  and of so then call the debug version to do the teardown
    //

    if (DEBUG_HEAP( Heap->Flags )) {

        if (!RtlDebugDestroyHeap( HeapHandle )) {

            return HeapHandle;
        }
    }

    //
    //  We are not allowed to destroy the process heap
    //

    if (HeapHandle == NtCurrentPeb()->ProcessHeap) {

        return HeapHandle;
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  For every big allocation we remove it from the list and free the
    //  vm
    //

    Head = &Heap->VirtualAllocdBlocks;
    Next = Head->Flink;

    while (Head != Next) {

        BaseAddress = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );

        Next = Next->Flink;
        RegionSize = 0;

        ZwFreeVirtualMemory( NtCurrentProcess(),
                             (PVOID *)&BaseAddress,
                             &RegionSize,
                             MEM_RELEASE );
    }

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case we need to destory any heap tags we have setup
    //  and remove this heap from the process heap list
    //

    RtlpDestroyTags( Heap );
    RtlpRemoveHeapFromProcessList( Heap );

#endif // NTOS_KERNEL_RUNTIME

    //
    //  If the heap is serialized, delete the critical section created
    //  by RtlCreateHeap.
    //

    if (!(Heap->Flags & HEAP_NO_SERIALIZE)) {

        if (!(Heap->Flags & HEAP_LOCK_USER_ALLOCATED)) {

            (VOID)RtlDeleteLockRoutine( Heap->LockVariable );
        }

        Heap->LockVariable = NULL;
    }

    //
    //  For every uncommitted segment we free its vm
    //

    UCRSegments = Heap->UCRSegments;
    Heap->UCRSegments = NULL;

    while (UCRSegments) {

        BaseAddress = UCRSegments;
        UCRSegments = UCRSegments->Next;
        RegionSize = 0;

        ZwFreeVirtualMemory( NtCurrentProcess(),
                             &BaseAddress,
                             &RegionSize,
                             MEM_RELEASE );
    }

    //
    //  For every segment in the heap we call a worker routine to
    //  destory the segment
    //

    SegmentIndex = HEAP_MAXIMUM_SEGMENTS;

    while (SegmentIndex--) {

        Segment = Heap->Segments[ SegmentIndex ];

        if (Segment) {

            RtlpDestroyHeapSegment( Segment );
        }
    }

    //
    //  And we return to our caller
    //

    return NULL;
}


PVOID
RtlAllocateHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    )

/*++

Routine Description:

    This routine allocates a memory of the specified size from the specified
    heap.

Arguments:

    HeapHandle - Supplies a pointer to an initialized heap structure

    Flags - Specifies the set of flags to use to control the allocation

    Size - Specifies the size, in bytes, of the allocation

Return Value:

    PVOID - returns a pointer to the newly allocated block

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PULONG FreeListsInUse;
    ULONG FreeListsInUseUlong;
    SIZE_T AllocationSize;
    SIZE_T FreeSize, AllocationIndex;
    PLIST_ENTRY FreeListHead, Next;
    PHEAP_ENTRY BusyBlock;
    PHEAP_FREE_ENTRY FreeBlock, SplitBlock, SplitBlock2;
    ULONG InUseIndex;
    UCHAR FreeFlags;
    NTSTATUS Status;
    EXCEPTION_RECORD ExceptionRecord;
    PVOID ReturnValue;
    BOOLEAN LockAcquired = FALSE;

    RTL_PAGED_CODE();


#ifndef NTOS_KERNEL_RUNTIME
#ifdef NTHEAP_ENABLED
    {
        if (Heap->Flags & NTHEAP_ENABLED_FLAG) {

            return RtlAllocateNtHeap( HeapHandle,
                                      Flags,
                                      Size);
        }
    }
#endif // NTHEAP_ENABLED
#endif // NTOS_KERNEL_RUNTIME


    //
    //  Take the callers flags and add in the flags that we must forcibly set
    //  in the heap
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check for special features that force us to call the slow, do-everything
    //  version.  We do everything slow for any of the following flags.
    //
    //    HEAP_SLOW_FLAGS defined as           0x6f030f60
    //
    //      HEAP_DEBUG_FLAGS, defined as       0x69020000 (heappriv.h)
    //
    //        HEAP_VALIDATE_PARAMETERS_ENABLED 0x40000000 (heap.h)
    //
    //        HEAP_VALIDATE_ALL_ENABLED        0x20000000 (heap.h)
    //
    //        HEAP_CAPTURE_STACK_BACKTRACES    0x08000000 (heap.h)
    //
    //        HEAP_CREATE_ENABLE_TRACING       0x00020000 (ntrtl.h winnt obsolete)
    //
    //        HEAP_FLAG_PAGE_ALLOCS            0x01000000 (heappage.h)
    //
    //      HEAP_SETTABLE_USER_FLAGS           0x00000E00 (ntrtl.h)
    //
    //      HEAP_NEED_EXTRA_FLAGS              0x0f000100 (heap.h)
    //
    //      HEAP_CREATE_ALIGN_16               0x00010000 (ntrtl.h winnt obsolete)
    //
    //      HEAP_FREE_CHECKING_ENABLED         0x00000040 (ntrtl.h winnt)
    //
    //      HEAP_TAIL_CHECKING_ENABLED         0x00000020 (ntrtl.h winnt )
    //
    //  We also do everything slow if the size is greater than max long
    //

    if ((Flags & HEAP_SLOW_FLAGS) || (Size >= 0x80000000)) {

        return RtlAllocateHeapSlowly( HeapHandle, Flags, Size );
    }

    //
    //  At this point we know we are doing everything in this routine
    //  and not taking the slow route.
    //
    //  Round the requested size up to the allocation granularity.  Note
    //  that if the request is for 0 bytes, we still allocate memory, because
    //  we add in an extra 1 byte to protect ourselves from idiots.
    //
    //      Allocation size will be either 16, 24, 32, ...
    //      Allocation index will be 2, 3, 4, ...
    //
    //  Note that allocation size 8 is skipped and are indices 0 and 1
    //

    AllocationSize = ((Size ? Size : 1) + HEAP_GRANULARITY - 1 + sizeof( HEAP_ENTRY ))
        & ~(HEAP_GRANULARITY -1);
    AllocationIndex = AllocationSize >>  HEAP_GRANULARITY_SHIFT;

    //
    //  If there is a lookaside list and the index is within limits then
    //  try and allocate from the lookaside list.  We'll actually capture
    //  the lookaside pointer from the heap and only use the captured pointer.
    //  This will take care of the condition where a walk or lock heap can
    //  cause us to check for a non null pointer and then have it become null
    //  when we read it again.  If it is non null to start with then even if
    //  the user walks or locks the heap via another thread the pointer to
    //  still valid here so we can still try and do a lookaside list pop.
    //

#ifndef NTOS_KERNEL_RUNTIME

    {
        PHEAP_LOOKASIDE Lookaside = (PHEAP_LOOKASIDE)Heap->Lookaside;

        if ((Lookaside != NULL) &&
            (Heap->LookasideLockCount == 0) &&
            (AllocationIndex < HEAP_MAXIMUM_FREELISTS)) {

            //
            //  If the number of operation elapsed operations is 128 times the
            //  lookaside depth then it is time to adjust the depth
            //

            if ((LONG)(Lookaside[AllocationIndex].TotalAllocates - Lookaside[AllocationIndex].LastTotalAllocates) >=
                      (Lookaside[AllocationIndex].Depth * 128)) {

                RtlpAdjustHeapLookasideDepth(&(Lookaside[AllocationIndex]));
            }

            ReturnValue = RtlpAllocateFromHeapLookaside(&(Lookaside[AllocationIndex]));

            if (ReturnValue != NULL) {

                PHEAP_ENTRY BusyBlock;

                BusyBlock = ((PHEAP_ENTRY)ReturnValue) - 1;
                BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);
                BusyBlock->SmallTagIndex = 0;

                if (Flags & HEAP_ZERO_MEMORY) {

                    RtlZeroMemory( ReturnValue, Size );
                }

                return ReturnValue;
            }
        }
    }

#endif // NTOS_KERNEL_RUNTIME

    try {

        //
        //  Check if we need to serialize our access to the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            //
            //  Lock the free list.
            //

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        //
        //  If the allocation index is less than the maximum free list size
        //  then we can use the index to check the free list otherwise we have
        //  to either pull the entry off of the [0] index list or allocate
        //  memory directly for this request.
        //

        if (AllocationIndex < HEAP_MAXIMUM_FREELISTS) {

            //
            //  With a size that matches a free list size grab the head
            //  of the list and check if there is an available entry
            //

            FreeListHead = &Heap->FreeLists[ AllocationIndex ];

            if ( !IsListEmpty( FreeListHead ))  {

                //
                //  We're in luck the list has an entry so now get the free
                //  entry,  copy its flags, remove it from the free list
                //

                FreeBlock = CONTAINING_RECORD( FreeListHead->Blink,
                                               HEAP_FREE_ENTRY,
                                               FreeList );

                FreeFlags = FreeBlock->Flags;

                RtlpFastRemoveDedicatedFreeBlock( Heap, FreeBlock );

                //
                //  Adjust the total number of bytes free in the heap
                //

                Heap->TotalFreeSize -= AllocationIndex;

                //
                //  Mark the block as busy and and set the number of bytes
                //  unused and tag index.  Also if it is the last entry
                //  then keep that flag.
                //

                BusyBlock = (PHEAP_ENTRY)FreeBlock;
                BusyBlock->Flags = HEAP_ENTRY_BUSY | (FreeFlags & HEAP_ENTRY_LAST_ENTRY);
                BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);
                BusyBlock->SmallTagIndex = 0;

            } else {

                //
                //  The free list that matches our request is empty
                //
                //  Scan the free list in use vector to find the smallest
                //  available free block large enough for our allocations.
                //

                //
                //  Compute the index of the ULONG where the scan should begin
                //

                InUseIndex = (ULONG) (AllocationIndex >> 5);
                FreeListsInUse = &Heap->u.FreeListsInUseUlong[InUseIndex];

                //
                //  Mask off the bits in the first ULONG that represent allocations
                //  smaller than we need.
                //

                FreeListsInUseUlong = *FreeListsInUse++ & ~((1 << ((ULONG) AllocationIndex & 0x1f)) - 1);

                //
                //  Begin unrolled loop to scan bit vector.
                //

                switch (InUseIndex) {

                case 0:

                    if (FreeListsInUseUlong) {

                        FreeListHead = &Heap->FreeLists[0];
                        break;
                    }

                    FreeListsInUseUlong = *FreeListsInUse++;

                    //
                    //  deliberate fallthrough to next ULONG
                    //

                case 1:

                    if (FreeListsInUseUlong) {

                        FreeListHead = &Heap->FreeLists[32];
                        break;
                    }

                    FreeListsInUseUlong = *FreeListsInUse++;

                    //
                    //  deliberate fallthrough to next ULONG
                    //

                case 2:

                    if (FreeListsInUseUlong) {

                        FreeListHead = &Heap->FreeLists[64];
                        break;
                    }

                    FreeListsInUseUlong = *FreeListsInUse++;

                    //
                    //  deliberate fallthrough to next ULONG
                    //

                case 3:

                    if (FreeListsInUseUlong) {

                        FreeListHead = &Heap->FreeLists[96];
                        break;
                    }

                    //
                    //  deliberate fallthrough to non dedicated list
                    //

                default:

                    //
                    //  No suitable entry on the free list was found.
                    //

                    goto LookInNonDedicatedList;
                }

                //
                //  A free list has been found with a large enough allocation.
                //  FreeListHead contains the base of the vector it was found in.
                //  FreeListsInUseUlong contains the vector.
                //

                FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );

                //
                //  Grab the free block and remove it from the free list
                //

                FreeBlock = CONTAINING_RECORD( FreeListHead->Blink,
                                               HEAP_FREE_ENTRY,
                                               FreeList );

                RtlpFastRemoveDedicatedFreeBlock( Heap, FreeBlock );

    SplitFreeBlock:

                //
                //  Save the blocks flags and decrement the amount of
                //  free space left in the heap
                //

                FreeFlags = FreeBlock->Flags;
                Heap->TotalFreeSize -= FreeBlock->Size;

                //
                //  Mark the block busy
                //

                BusyBlock = (PHEAP_ENTRY)FreeBlock;
                BusyBlock->Flags = HEAP_ENTRY_BUSY;

                //
                //  Compute the size (i.e., index) of the amount from this block
                //  that we don't need and can return to the free list
                //

                FreeSize = BusyBlock->Size - AllocationIndex;

                //
                //  Finish setting up the rest of the new busy block
                //

                BusyBlock->Size = (USHORT)AllocationIndex;
                BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);
                BusyBlock->SmallTagIndex = 0;

                //
                //  Now if the size that we are going to free up is not zero
                //  then lets get to work and to the split.
                //

                if (FreeSize != 0) {

                    //
                    //  But first we won't ever bother doing a split that only
                    //  gives us 8 bytes back.  So if free size is one then just
                    //  bump up the size of the new busy block
                    //

                    if (FreeSize == 1) {

                        BusyBlock->Size += 1;
                        BusyBlock->UnusedBytes += sizeof( HEAP_ENTRY );

                    } else {

                        //
                        //  Get a pointer to where the new free block will be.
                        //  When we split a block the first part goes to the new
                        //  busy block and the second part goes back to the free
                        //  list
                        //

                        SplitBlock = (PHEAP_FREE_ENTRY)(BusyBlock + AllocationIndex);

                        //
                        //  Reset the flags that we copied from the original free list
                        //  header, and set it other size fields.
                        //

                        SplitBlock->Flags = FreeFlags;
                        SplitBlock->PreviousSize = (USHORT)AllocationIndex;
                        SplitBlock->SegmentIndex = BusyBlock->SegmentIndex;
                        SplitBlock->Size = (USHORT)FreeSize;

                        //
                        //  If nothing else follows this entry then we will insert
                        //  this into the corresponding free list (and update
                        //  Segment->LastEntryInSegment)
                        //

                        if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {

                            RtlpFastInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize);
                            Heap->TotalFreeSize += FreeSize;

                        } else {

                            //
                            //  Otherwise we need to check the following block
                            //  and if it is busy then update its previous size
                            //  before inserting our new free block into the
                            //  free list
                            //

                            SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);

                            if (SplitBlock2->Flags & HEAP_ENTRY_BUSY) {

                                SplitBlock2->PreviousSize = (USHORT)FreeSize;

                                RtlpFastInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                                Heap->TotalFreeSize += FreeSize;

                            } else {

                                //
                                //  The following block is free so we'll merge
                                //  these to blocks. by first merging the flags
                                //

                                SplitBlock->Flags = SplitBlock2->Flags;

                                //
                                //  Removing the second block from its free list
                                //

                                RtlpFastRemoveFreeBlock( Heap, SplitBlock2 );

                                //
                                //  Updating the free total number of free bytes
                                //  in the heap and updating the size of the new
                                //  free block
                                //

                                Heap->TotalFreeSize -= SplitBlock2->Size;
                                FreeSize += SplitBlock2->Size;

                                //
                                //  If the new free block is still less than the
                                //  maximum heap block size then we'll simply
                                //  insert it back in the free list
                                //

                                if (FreeSize <= HEAP_MAXIMUM_BLOCK_SIZE) {

                                    SplitBlock->Size = (USHORT)FreeSize;

                                    //
                                    //  Again check if the new following block
                                    //  exists and if so then updsate is previous
                                    //  size
                                    //

                                    if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                                        ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                                    }

                                    //
                                    //  Insert the new free block into the free
                                    //  list and update the free heap size
                                    //

                                    RtlpFastInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );
                                    Heap->TotalFreeSize += FreeSize;

                                } else {

                                    //
                                    //  The new free block is pretty large so we
                                    //  need to call a private routine to do the
                                    //  insert
                                    //

                                    RtlpInsertFreeBlock( Heap, SplitBlock, FreeSize );
                                }
                            }
                        }

                        //
                        //  Now that free flags made it back into a free block
                        //  we can zero out what we saved.
                        //

                        FreeFlags = 0;

                        //
                        //  If splitblock now last, update LastEntryInSegment
                        //

                        if (SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {

                            PHEAP_SEGMENT Segment;

                            Segment = Heap->Segments[SplitBlock->SegmentIndex];
                            Segment->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;
                        }
                    }
                }

                //
                //  If there are no following entries then mark the new block as
                //  such
                //

                if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {

                    BusyBlock->Flags |= HEAP_ENTRY_LAST_ENTRY;
                }
            }

            //
            //  Return the address of the user portion of the allocated block.
            //  This is the byte following the header.
            //

            ReturnValue = BusyBlock + 1;

            //
            //  **** Release the lock before the zero memory call
            //

            if (LockAcquired) {

                RtlReleaseLockRoutine( Heap->LockVariable );

                LockAcquired = FALSE;
            }
            
            //
            //  If the flags indicate that we should zero memory then do it now
            //

            if (Flags & HEAP_ZERO_MEMORY) {

                RtlZeroMemory( ReturnValue, Size );
            }

            //
            //  And return the allocated block to our caller
            //

            leave;

        //
        //  Otherwise the allocation request is bigger than the last dedicated
        //  free list size.  Now check if the size is within our threshold.
        //  Meaning that it could be in the [0] free list
        //

        } else if (AllocationIndex <= Heap->VirtualMemoryThreshold) {

    LookInNonDedicatedList:

            //
            //  The following code cycles through the [0] free list until
            //  it finds a block that satisfies the request.  The list
            //  is sorted so the search is can be terminated early on success
            //

            FreeListHead = &Heap->FreeLists[0];
            
            //
            //  Check if the largest block in the list is smaller than the request
            //

            Next = FreeListHead->Blink;

            if (FreeListHead != Next) {
                
                FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );

                if (FreeBlock->Size >= AllocationIndex) {

                    //
                    //  Here we are sure there is at least a block here larger than
                    //  the requested size. Start searching from the first block
                    //

                    Next = FreeListHead->Flink;
                    
                    while (FreeListHead != Next) {

                        FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );

                        if (FreeBlock->Size >= AllocationIndex) {

                            //
                            //  We've found something that we can use so now remove
                            //  it from the free list and go to where we treat spliting
                            //  a free block.  Note that the block we found here might
                            //  actually be the exact size we need and that is why
                            //  in the split free block case we have to consider having
                            //  nothing free after the split
                            //

                            RtlpFastRemoveNonDedicatedFreeBlock( Heap, FreeBlock );

                            goto SplitFreeBlock;
                        }

                        Next = Next->Flink;
                    }
                }
            }

            //
            //  The [0] list is either empty or everything is too small
            //  so now extend the heap which should get us something less
            //  than or equal to the virtual memory threshold
            //

            FreeBlock = RtlpExtendHeap( Heap, AllocationSize );

            //
            //  And provided we got something we'll treat it just like the previous
            //  split free block cases
            //

            if (FreeBlock != NULL) {

                RtlpFastRemoveNonDedicatedFreeBlock( Heap, FreeBlock );

                goto SplitFreeBlock;
            }

            //
            //  We weren't able to extend the heap so we must be out of memory
            //

            Status = STATUS_NO_MEMORY;

        //
        //  At this point the allocation is way too big for any of the free lists
        //  and we can only satisfy this request if the heap is growable
        //

        } else if (Heap->Flags & HEAP_GROWABLE) {

            PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

            VirtualAllocBlock = NULL;

            //
            //  Compute how much memory we will need for this allocation which
            //  will include the allocation size plus a header, and then go
            //  get the committed memory
            //

            AllocationSize += FIELD_OFFSET( HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              (PVOID *)&VirtualAllocBlock,
                                              0,
                                              &AllocationSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE );

            if (NT_SUCCESS(Status)) {

                //
                //  Just committed, already zero.  Fill in the new block
                //  and insert it in the list of big allocation
                //

                VirtualAllocBlock->BusyBlock.Size = (USHORT)(AllocationSize - Size);
                VirtualAllocBlock->BusyBlock.Flags = HEAP_ENTRY_VIRTUAL_ALLOC | HEAP_ENTRY_EXTRA_PRESENT | HEAP_ENTRY_BUSY;
                VirtualAllocBlock->CommitSize = AllocationSize;
                VirtualAllocBlock->ReserveSize = AllocationSize;

                InsertTailList( &Heap->VirtualAllocdBlocks, (PLIST_ENTRY)VirtualAllocBlock );

                //
                //  Return the address of the user portion of the allocated block.
                //  This is the byte following the header.
                //

                ReturnValue = (PHEAP_ENTRY)(VirtualAllocBlock + 1);

                leave;
            }

        } else {

            Status = STATUS_BUFFER_TOO_SMALL;
        }

        //
        //  This is the error return.
        //

        if (Flags & HEAP_GENERATE_EXCEPTIONS) {

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

        SET_LAST_STATUS(Status);

        ReturnValue = NULL;

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return ReturnValue;
}


PVOID
RtlAllocateHeapSlowly (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    )

/*++

Routine Description:

    This routine does the equivalent of Rtl Allocate Heap but it does it will
    additional heap consistency checking logic and tagging.

Arguments:

    HeapHandle - Supplies a pointer to an initialized heap structure

    Flags - Specifies the set of flags to use to control the allocation

    Size - Specifies the size, in bytes, of the allocation

Return Value:

    PVOID - returns a pointer to the newly allocated block

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    PVOID ReturnValue = NULL;
    PULONG FreeListsInUse;
    ULONG FreeListsInUseUlong;
    SIZE_T AllocationSize;
    SIZE_T FreeSize, AllocationIndex;
    UCHAR EntryFlags, FreeFlags;
    PLIST_ENTRY FreeListHead, Next;
    PHEAP_ENTRY BusyBlock;
    PHEAP_FREE_ENTRY FreeBlock, SplitBlock, SplitBlock2;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    NTSTATUS Status;
    EXCEPTION_RECORD ExceptionRecord;
    SIZE_T ZeroSize = 0;

    RTL_PAGED_CODE();

    //
    //  Note that Flags has already been OR'd with Heap->ForceFlags.
    //

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case check if we should be using the debug version
    //  of heap allocation
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugAllocateHeap( HeapHandle, Flags, Size );
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  If the size is greater than maxlong then say we can't allocate that
    //  much and return the error to our caller
    //

    if (Size > 0x7fffffff) {

        SET_LAST_STATUS( STATUS_NO_MEMORY );

        return NULL;
    }

    //
    //  Round up the requested size to the allocation granularity.  Note
    //  that if the request is for zero bytes we will still allocate memory,
    //
    //      Allocation size will be either 16, 24, 32, ...
    //      Allocation index will be 2, 3, 4, ...
    //

    AllocationSize = ((Size ? Size : 1) + Heap->AlignRound) & Heap->AlignMask;

    //
    //  Generate the flags needed for this heap entry.  Mark it busy and add
    //  any user settable bits.  Also if the input flag indicates any entry
    //  extra fields and we have a tag to use then make room for the extra
    //  fields in the heap entry
    //

    EntryFlags = (UCHAR)(HEAP_ENTRY_BUSY | ((Flags & HEAP_SETTABLE_USER_FLAGS) >> 4));

    if ((Flags & HEAP_NEED_EXTRA_FLAGS) || (Heap->PseudoTagEntries != NULL)) {

        EntryFlags |= HEAP_ENTRY_EXTRA_PRESENT;
        AllocationSize += sizeof( HEAP_ENTRY_EXTRA );
    }

    AllocationIndex = AllocationSize >> HEAP_GRANULARITY_SHIFT;

    try {

        //
        //  Lock the free list.
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        //
        //  Do all the actual heap work under the protection of a try-except clause
        //  to protect us from corruption
        //

        try {

            //
            //  If the allocation index is less than the maximum free list size
            //  then we can use the index to check the free list otherwise we have
            //  to either pull the entry off of the [0] index list or allocate
            //  memory directly for this request.
            //

            if (AllocationIndex < HEAP_MAXIMUM_FREELISTS) {

                //
                //  With a size that matches a free list size grab the head
                //  of the list and check if there is an available entry
                //

                FreeListHead = &Heap->FreeLists[ AllocationIndex ];

                if ( !IsListEmpty( FreeListHead ))  {

                    //
                    //  We're in luck the list has an entry so now get the free
                    //  entry,  copy its flags, remove it from the free list
                    //

                    FreeBlock = CONTAINING_RECORD( FreeListHead->Flink,
                                                   HEAP_FREE_ENTRY,
                                                   FreeList );

                    FreeFlags = FreeBlock->Flags;

                    RtlpRemoveFreeBlock( Heap, FreeBlock );

                    //
                    //  Adjust the total number of bytes free in the heap
                    //

                    Heap->TotalFreeSize -= AllocationIndex;

                    //
                    //  Mark the block as busy and and set the number of bytes
                    //  unused and tag index.  Also if it is the last entry
                    //  then keep that flag.
                    //

                    BusyBlock = (PHEAP_ENTRY)FreeBlock;
                    BusyBlock->Flags = EntryFlags | (FreeFlags & HEAP_ENTRY_LAST_ENTRY);
                    BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);

                } else {

                    //
                    //  The free list that matches our request is empty.  We know
                    //  that there are 128 free lists managed by a 4 ulong bitmap.
                    //  The next big if-else-if statement will decide which ulong
                    //  we tackle
                    //
                    //  Check if the requested allocation index within the first
                    //  quarter of the free lists.
                    //

                    if (AllocationIndex < (HEAP_MAXIMUM_FREELISTS * 1) / 4) {

                        //
                        //  Grab a pointer to the corresponding bitmap ulong, and
                        //  then get the bit we're actually interested in to be the
                        //  first bit of the ulong.
                        //

                        FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 0 ];
                        FreeListsInUseUlong = *FreeListsInUse++ >> ((ULONG) AllocationIndex & 0x1F);

                        //
                        //  If the remaining bitmap has any bits set then we know
                        //  there is a non empty list that is larger than our
                        //  requested index so find that bit and compute the list
                        //  head of the next non empty list
                        //

                        if (FreeListsInUseUlong) {

                            FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );

                        } else {

                            //
                            //  The rest of the first ulong is all zeros so we need
                            //  to move to the second ulong
                            //

                            FreeListsInUseUlong = *FreeListsInUse++;

                            //
                            //  Check if the second ulong has any bits set and if
                            //  so then compute the list head of the next non empty
                            //  list
                            //

                            if (FreeListsInUseUlong) {

                                FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 1) / 4) -
                                    (AllocationIndex & 0x1F)  +
                                    RtlFindFirstSetRightMember( FreeListsInUseUlong );

                            } else {

                                //
                                //  Do the same test for the third ulong
                                //

                                FreeListsInUseUlong = *FreeListsInUse++;

                                if (FreeListsInUseUlong) {

                                    FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 2) / 4) -
                                        (AllocationIndex & 0x1F) +
                                        RtlFindFirstSetRightMember( FreeListsInUseUlong );

                                } else {

                                    //
                                    //  Repeat the test for the forth ulong, and if
                                    //  that one is also empty then we need to grab
                                    //  the allocation off of the [0] index list
                                    //

                                    FreeListsInUseUlong = *FreeListsInUse++;

                                    if (FreeListsInUseUlong) {

                                        FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 3) / 4) -
                                            (AllocationIndex & 0x1F)  +
                                            RtlFindFirstSetRightMember( FreeListsInUseUlong );

                                    } else {

                                        goto LookInNonDedicatedList;
                                    }
                                }
                            }
                        }

                    //
                    //  Otherwise check if the requested allocation index lies
                    //  within the second quarter of the free lists.  We repeat the
                    //  test just like we did above on the second, third, and forth
                    //  bitmap ulongs.
                    //

                    } else if (AllocationIndex < (HEAP_MAXIMUM_FREELISTS * 2) / 4) {

                        FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 1 ];
                        FreeListsInUseUlong = *FreeListsInUse++ >> ((ULONG) AllocationIndex & 0x1F);

                        if (FreeListsInUseUlong) {

                            FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );

                        } else {

                            FreeListsInUseUlong = *FreeListsInUse++;

                            if (FreeListsInUseUlong) {

                                FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 1) / 4) -
                                    (AllocationIndex & 0x1F)  +
                                    RtlFindFirstSetRightMember( FreeListsInUseUlong );

                            } else {

                                FreeListsInUseUlong = *FreeListsInUse++;

                                if (FreeListsInUseUlong) {

                                    FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 2) / 4) -
                                        (AllocationIndex & 0x1F)  +
                                        RtlFindFirstSetRightMember( FreeListsInUseUlong );

                                } else {

                                    goto LookInNonDedicatedList;
                                }
                            }
                        }

                    //
                    //  Otherwise check if the requested allocation index lies
                    //  within the third quarter of the free lists. We repeat the
                    //  test just like we did above on the third and forth bitmap
                    //  ulongs
                    //

                    } else if (AllocationIndex < (HEAP_MAXIMUM_FREELISTS * 3) / 4) {

                        FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 2 ];
                        FreeListsInUseUlong = *FreeListsInUse++ >> ((ULONG) AllocationIndex & 0x1F);

                        if (FreeListsInUseUlong) {

                            FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );

                        } else {

                            FreeListsInUseUlong = *FreeListsInUse++;

                            if (FreeListsInUseUlong) {

                                FreeListHead += ((HEAP_MAXIMUM_FREELISTS * 1) / 4) -
                                    (AllocationIndex & 0x1F)  +
                                    RtlFindFirstSetRightMember( FreeListsInUseUlong );

                            } else {

                                goto LookInNonDedicatedList;
                            }
                        }

                    //
                    //  Lastly the requested allocation index must lie within the
                    //  last quarter of the free lists.  We repeat the test just
                    //  like we did above on the forth ulong
                    //

                    } else {

                        FreeListsInUse = &Heap->u.FreeListsInUseUlong[ 3 ];
                        FreeListsInUseUlong = *FreeListsInUse++ >> ((ULONG) AllocationIndex & 0x1F);

                        if (FreeListsInUseUlong) {

                            FreeListHead += RtlFindFirstSetRightMember( FreeListsInUseUlong );

                        } else {

                            goto LookInNonDedicatedList;
                        }
                    }

                    //
                    //  At this point the free list head points to a non empty free
                    //  list that is of greater size than we need.
                    //

                    FreeBlock = CONTAINING_RECORD( FreeListHead->Flink,
                                                   HEAP_FREE_ENTRY,
                                                   FreeList );

    SplitFreeBlock:

                    //
                    //  Remember the flags that go with this block and remove it
                    //  from its list
                    //

                    FreeFlags = FreeBlock->Flags;

                    RtlpRemoveFreeBlock( Heap, FreeBlock );

                    //
                    //  Adjust the amount free in the heap
                    //

                    Heap->TotalFreeSize -= FreeBlock->Size;

                    //
                    //  Mark the block busy
                    //

                    BusyBlock = (PHEAP_ENTRY)FreeBlock;
                    BusyBlock->Flags = EntryFlags;

                    //
                    //  Compute the size (i.e., index) of the amount from this
                    //  block that we don't need and can return to the free list
                    //

                    FreeSize = BusyBlock->Size - AllocationIndex;

                    //
                    //  Finish setting up the rest of the new busy block
                    //

                    BusyBlock->Size = (USHORT)AllocationIndex;
                    BusyBlock->UnusedBytes = (UCHAR)(AllocationSize - Size);

                    //
                    //  Now if the size that we are going to free up is not zero
                    //  then lets get to work and to the split.
                    //

                    if (FreeSize != 0) {

                        //
                        //  But first we won't ever bother doing a split that only
                        //  gives us 8 bytes back.  So if free size is one then
                        //  just bump up the size of the new busy block
                        //

                        if (FreeSize == 1) {

                            BusyBlock->Size += 1;
                            BusyBlock->UnusedBytes += sizeof( HEAP_ENTRY );

                        } else {

                            //
                            //  Get a pointer to where the new free block will be.
                            //  When we split a block the first part goes to the
                            //  new busy block and the second part goes back to the
                            //  free list
                            //

                            SplitBlock = (PHEAP_FREE_ENTRY)(BusyBlock + AllocationIndex);

                            //
                            //  Reset the flags that we copied from the original
                            //  free list header, and set it other size fields.
                            //

                            SplitBlock->Flags = FreeFlags;
                            SplitBlock->PreviousSize = (USHORT)AllocationIndex;
                            SplitBlock->SegmentIndex = BusyBlock->SegmentIndex;
                            SplitBlock->Size = (USHORT)FreeSize;

                            //
                            //  If nothing else follows this entry then we will
                            //  insert this into the corresponding free list
                            //

                            if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {

                                RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                                Heap->TotalFreeSize += FreeSize;

                            } else {

                                //
                                //  Otherwise we need to check the following block
                                //  and if it is busy then update its previous size
                                //  before inserting our new free block into the
                                //  free list
                                //

                                SplitBlock2 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize);

                                if (SplitBlock2->Flags & HEAP_ENTRY_BUSY) {

                                    SplitBlock2->PreviousSize = (USHORT)FreeSize;

                                    RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                                    Heap->TotalFreeSize += FreeSize;

                                } else {

                                    //
                                    //  The following block is free so we'll merge
                                    //  these to blocks. by first merging the flags
                                    //

                                    SplitBlock->Flags = SplitBlock2->Flags;

                                    //
                                    //  Removing the second block from its free
                                    //  list
                                    //

                                    RtlpRemoveFreeBlock( Heap, SplitBlock2 );

                                    //
                                    //  Updating the free total number of free
                                    //  bytes in the heap and updating the size of
                                    //  the new free block
                                    //

                                    Heap->TotalFreeSize -= SplitBlock2->Size;
                                    FreeSize += SplitBlock2->Size;

                                    //
                                    //  If the new free block is still less than
                                    //  the maximum heap block size then we'll
                                    //  simply insert it back in the free list
                                    //

                                    if (FreeSize <= HEAP_MAXIMUM_BLOCK_SIZE) {

                                        SplitBlock->Size = (USHORT)FreeSize;

                                        //
                                        //  Again check if the new following block
                                        //  exists and if so then updsate is
                                        //  previous size
                                        //

                                        if (!(SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                                            ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)SplitBlock + FreeSize))->PreviousSize = (USHORT)FreeSize;
                                        }

                                        //
                                        //  Insert the new free block into the free
                                        //  list and update the free heap size
                                        //

                                        RtlpInsertFreeBlockDirect( Heap, SplitBlock, (USHORT)FreeSize );

                                        Heap->TotalFreeSize += FreeSize;

                                    } else {

                                        //
                                        //  The new free block is pretty large so
                                        //  we need to call a private routine to do
                                        //  the insert
                                        //

                                        RtlpInsertFreeBlock( Heap, SplitBlock, FreeSize );
                                    }
                                }
                            }

                            //
                            //  Now that free flags made it back into a free block
                            //  we can zero out what we saved.
                            //

                            FreeFlags = 0;

                            //
                            //  If splitblock now last, update LastEntryInSegment
                            //

                            if (SplitBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {

                                PHEAP_SEGMENT Segment;

                                Segment = Heap->Segments[SplitBlock->SegmentIndex];
                                Segment->LastEntryInSegment = (PHEAP_ENTRY)SplitBlock;
                            }

                        }
                    }

                    //
                    //  If there are no following entries then mark the new block
                    //  as such
                    //

                    if (FreeFlags & HEAP_ENTRY_LAST_ENTRY) {

                        BusyBlock->Flags |= HEAP_ENTRY_LAST_ENTRY;
                    }
                }

                //
                //  Return the address of the user portion of the allocated block.
                //  This is the byte following the header.
                //

                ReturnValue = BusyBlock + 1;

                //
                //  If the flags indicate that we should zero memory then
                //  remember how much to zero.  We'll do the zeroing later
                //

                if (Flags & HEAP_ZERO_MEMORY) {

                    ZeroSize = Size;

                //
                //  Otherwise if the flags indicate that we should fill heap then
                //  it it now.
                //

                } else if (Heap->Flags & HEAP_FREE_CHECKING_ENABLED) {

                    RtlFillMemoryUlong( (PCHAR)(BusyBlock + 1), Size & ~0x3, ALLOC_HEAP_FILL );
                }

                //
                //  If the flags indicate that we should do tail checking then copy
                //  the fill pattern right after the heap block.
                //

                if (Heap->Flags & HEAP_TAIL_CHECKING_ENABLED) {

                    RtlFillMemory( (PCHAR)ReturnValue + Size,
                                   CHECK_HEAP_TAIL_SIZE,
                                   CHECK_HEAP_TAIL_FILL );

                    BusyBlock->Flags |= HEAP_ENTRY_FILL_PATTERN;
                }

                BusyBlock->SmallTagIndex = 0;

                //
                //  If the flags indicate that there is an extra block persent then
                //  we'll fill it in
                //

                if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                    ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

                    RtlZeroMemory( ExtraStuff, sizeof( *ExtraStuff ));

    #ifndef NTOS_KERNEL_RUNTIME

                //
                //  In the non kernel case the tagging goes in either the extra
                //  stuff of the busy block small tag index
                //

                    if (IS_HEAP_TAGGING_ENABLED()) {

                        ExtraStuff->TagIndex = RtlpUpdateTagEntry( Heap,
                                                                   (USHORT)((Flags & HEAP_TAG_MASK) >> HEAP_TAG_SHIFT),
                                                                   0,
                                                                   BusyBlock->Size,
                                                                   AllocationAction );
                    }

                } else if (IS_HEAP_TAGGING_ENABLED()) {

                    BusyBlock->SmallTagIndex = (UCHAR)RtlpUpdateTagEntry( Heap,
                                                                          (USHORT)((Flags & HEAP_SMALL_TAG_MASK) >> HEAP_TAG_SHIFT),
                                                                          0,
                                                                          BusyBlock->Size,
                                                                          AllocationAction );

    #endif // NTOS_KERNEL_RUNTIME

                }

                //
                //  Return the address of the user portion of the allocated block.
                //  This is the byte following the header.
                //

                leave;

            //
            //  Otherwise the allocation request is bigger than the last dedicated
            //  free list size.  Now check if the size is within our threshold.
            //  Meaning that it could be in the [0] free list
            //

            } else if (AllocationIndex <= Heap->VirtualMemoryThreshold) {

    LookInNonDedicatedList:

                //
                //  The following code cycles through the [0] free list until
                //  it finds a block that satisfies the request.  The list
                //  is sorted so the search is can be terminated early on success
                //

                FreeListHead = &Heap->FreeLists[ 0 ];
                Next = FreeListHead->Flink;

                while (FreeListHead != Next) {

                    FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );

                    if (FreeBlock->Size >= AllocationIndex) {

                        //
                        //  We've found something that we can use so now go to
                        //  where we treat spliting a free block.  Note that
                        //  the block we found here might actually be the exact
                        //  size we need and that is why in the split free block
                        //  case we have to consider having nothing free after the
                        //  split
                        //

                        goto SplitFreeBlock;

                    } else {

                        Next = Next->Flink;
                    }
                }

                //
                //  The [0] list is either empty or everything is too small
                //  so now extend the heap which should get us something less
                //  than or equal to the virtual memory threshold
                //

                FreeBlock = RtlpExtendHeap( Heap, AllocationSize );

                //
                //  And provided we got something we'll treat it just like the
                //  previous split free block cases
                //

                if (FreeBlock != NULL) {

                    goto SplitFreeBlock;
                }

                //
                //  We weren't able to extend the heap so we must be out of memory
                //

                Status = STATUS_NO_MEMORY;

            //
            //  At this point the allocation is way too big for any of the free
            //  lists and we can only satisfy this request if the heap is growable
            //

            } else if (Heap->Flags & HEAP_GROWABLE) {

                PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

                VirtualAllocBlock = NULL;

                //
                //  Compute how much memory we will need for this allocation which
                //  will include the allocation size plus a header, and then go
                //  get the committed memory
                //

                AllocationSize += FIELD_OFFSET( HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

                Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                                  (PVOID *)&VirtualAllocBlock,
                                                  0,
                                                  &AllocationSize,
                                                  MEM_COMMIT,
                                                  PAGE_READWRITE );

                if (NT_SUCCESS( Status )) {

                    //
                    //  Just committed, already zero.  Fill in the new block
                    //  and insert it in the list of big allocation
                    //

                    VirtualAllocBlock->BusyBlock.Size = (USHORT)(AllocationSize - Size);
                    VirtualAllocBlock->BusyBlock.Flags = EntryFlags | HEAP_ENTRY_VIRTUAL_ALLOC | HEAP_ENTRY_EXTRA_PRESENT;
                    VirtualAllocBlock->CommitSize = AllocationSize;
                    VirtualAllocBlock->ReserveSize = AllocationSize;

    #ifndef NTOS_KERNEL_RUNTIME

                    //
                    //  In the non kernel case see if we need to add heap tagging
                    //

                    if (IS_HEAP_TAGGING_ENABLED()) {

                        VirtualAllocBlock->ExtraStuff.TagIndex =
                            RtlpUpdateTagEntry( Heap,
                                                (USHORT)((Flags & HEAP_SMALL_TAG_MASK) >> HEAP_TAG_SHIFT),
                                                0,
                                                VirtualAllocBlock->CommitSize >> HEAP_GRANULARITY_SHIFT,
                                                VirtualAllocationAction );
                    }

    #endif // NTOS_KERNEL_RUNTIME

                    InsertTailList( &Heap->VirtualAllocdBlocks, (PLIST_ENTRY)VirtualAllocBlock );

                    //
                    //  Return the address of the user portion of the allocated
                    //  block.  This is the byte following the header.
                    //

                    ReturnValue = (PHEAP_ENTRY)(VirtualAllocBlock + 1);

                    leave;
                }

            //
            //  Otherwise we have an error condition
            //

            } else {

                Status = STATUS_BUFFER_TOO_SMALL;
            }

            SET_LAST_STATUS( Status );

            if (Flags & HEAP_GENERATE_EXCEPTIONS) {

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
        }

        //
        //  Check if there is anything to zero out
        //

        if ( ZeroSize ) {

            RtlZeroMemory( ReturnValue, ZeroSize );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  And return to our caller
    //

    return ReturnValue;
}


BOOLEAN
RtlFreeHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This routine returns a previously allocated block back to its heap

Arguments:

    HeapHandle - Supplies a pointer to the owning heap structure

    Flags - Specifies the set of flags to use in the deallocation

    BaseAddress - Supplies a pointer to the block being freed

Return Value:

    BOOLEAN - TRUE if the block was properly freed and FALSE otherwise

--*/

{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    SIZE_T FreeSize;
    BOOLEAN LockAcquired = FALSE;
    BOOLEAN ReturnValue = TRUE;

    RTL_PAGED_CODE();

    //
    //  First check if the address we're given is null and if so then
    //  there is really nothing to do so just return success
    //

    if (BaseAddress == NULL) {

        return TRUE;
    }

#ifndef NTOS_KERNEL_RUNTIME
#ifdef NTHEAP_ENABLED
    {
        if (Heap->Flags & NTHEAP_ENABLED_FLAG) {

            return RtlFreeNtHeap( HeapHandle,
                                  Flags,
                                  BaseAddress);
        }
    }
#endif // NTHEAP_ENABLED
#endif // NTOS_KERNEL_RUNTIME


    //
    //  Compliment the input flags with those enforced by the heap
    //

    Flags |= Heap->ForceFlags;

    //
    //  Now check if we should go the slow route
    //

    if (Flags & HEAP_SLOW_FLAGS) {

        return RtlFreeHeapSlowly(HeapHandle, Flags, BaseAddress);
    }

    //
    //  We can do everything in this routine. So now backup to get
    //  a pointer to the start of the block
    //

    BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

    //
    //  Protect ourselves from idiots by refusing to free blocks
    //  that do not have the busy bit set.
    //
    //  Also refuse to free blocks that are not eight-byte aligned.
    //  The specific idiot in this case is Office95, which likes
    //  to free a random pointer when you start Word95 from a desktop
    //  shortcut.
    //
    //  As further insurance against idiots, check the segment index
    //  to make sure it is less than HEAP_MAXIMUM_SEGMENTS (16). This
    //  should fix all the dorks who have ASCII or Unicode where the
    //  heap header is supposed to be.
    //

    try {
        if ((!(BusyBlock->Flags & HEAP_ENTRY_BUSY)) ||
            (((ULONG_PTR)BaseAddress & 0x7) != 0) ||
            (BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS)) {

            //
            //  Not a busy block, or it's not aligned or the segment is
            //  to big, meaning it's corrupt
            //

            SET_LAST_STATUS( STATUS_INVALID_PARAMETER );

            return FALSE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {

        SET_LAST_STATUS( STATUS_INVALID_PARAMETER );
        return FALSE;
    }

    //
    //  If there is a lookaside list and the block is not a big allocation
    //  and the index is for a dedicated list then free the block to the
    //  lookaside list.  We'll actually capture
    //  the lookaside pointer from the heap and only use the captured pointer.
    //  This will take care of the condition where a walk or lock heap can
    //  cause us to check for a non null pointer and then have it become null
    //  when we read it again.  If it is non null to start with then even if
    //  the user walks or locks the heap via another thread the pointer to
    //  still valid here so we can still try and do a lookaside list push
    //

#ifndef NTOS_KERNEL_RUNTIME

    {
        PHEAP_LOOKASIDE Lookaside = (PHEAP_LOOKASIDE)Heap->Lookaside;

        if ((Lookaside != NULL) &&
            (Heap->LookasideLockCount == 0) &&
            (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)) &&
            ((FreeSize = BusyBlock->Size) < HEAP_MAXIMUM_FREELISTS)) {

            if (RtlpFreeToHeapLookaside( &Lookaside[FreeSize], BaseAddress)) {

                return TRUE;
            }
        }
    }

#endif // NTOS_KERNEL_RUNTIME

    try {

        //
        //  Check if we need to lock the heap
        //

        if (!(Flags & HEAP_NO_SERIALIZE)) {

            RtlAcquireLockRoutine( Heap->LockVariable );

            LockAcquired = TRUE;
        }

        //
        //  Check if this is not a virtual block allocation meaning
        //  that we it is part of the heap free list structure and not
        //  one huge allocation that we got from vm
        //

        if (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC)) {

            //
            //  This block is not a big allocation so we need to
            //  to get its size, and coalesce the blocks note that
            //  the user mode heap does this conditionally on a heap
            //  flag.  The coalesce function returns the newly formed
            //  free block and the new size.
            //

            FreeSize = BusyBlock->Size;

    #ifdef NTOS_KERNEL_RUNTIME

            BusyBlock = (PHEAP_ENTRY)RtlpCoalesceFreeBlocks( Heap,
                                                             (PHEAP_FREE_ENTRY)BusyBlock,
                                                             &FreeSize,
                                                             FALSE );

    #else // NTOS_KERNEL_RUNTIME

            if (!(Heap->Flags & HEAP_DISABLE_COALESCE_ON_FREE)) {

                BusyBlock = (PHEAP_ENTRY)RtlpCoalesceFreeBlocks( Heap,
                                                                 (PHEAP_FREE_ENTRY)BusyBlock,
                                                                 &FreeSize,
                                                                 FALSE );
            }

    #endif // NTOS_KERNEL_RUNTIME

            //
            //  Check for a small allocation that can go on a freelist
            //  first, these should never trigger a decommit.
            //

            HEAPASSERT(HEAP_MAXIMUM_FREELISTS < Heap->DeCommitFreeBlockThreshold);

            //
            //  If the allocation fits on a free list then insert it on
            //  the appropriate free list.  If the block is not the last
            //  entry then make sure that the next block knows our correct
            //  size, and update the heap free space counter.
            //

            if (FreeSize < HEAP_MAXIMUM_FREELISTS) {

                RtlpFastInsertDedicatedFreeBlockDirect( Heap,
                                                        (PHEAP_FREE_ENTRY)BusyBlock,
                                                        (USHORT)FreeSize );

                if (!(BusyBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                    HEAPASSERT((BusyBlock + FreeSize)->PreviousSize == (USHORT)FreeSize);
                }

                Heap->TotalFreeSize += FreeSize;

            //
            //  Otherwise the block is to big for one of the dedicated free list so
            //  see if the free size is under the decommit threshold by itself
            //  or the total free in the heap is under the decomit threshold then
            //  we'll put this into a free list
            //

            } else if ((FreeSize < Heap->DeCommitFreeBlockThreshold) ||
                       ((Heap->TotalFreeSize + FreeSize) < Heap->DeCommitTotalFreeThreshold)) {

                //
                //  Check if the block can go into the [0] index free list, and if
                //  so then do the insert and make sure the following block is
                //  needed knows our correct size, and update the heaps free space
                //  counter
                //

                if (FreeSize <= (ULONG)HEAP_MAXIMUM_BLOCK_SIZE) {

                    RtlpFastInsertNonDedicatedFreeBlockDirect( Heap,
                                                               (PHEAP_FREE_ENTRY)BusyBlock,
                                                               (USHORT)FreeSize );

                    if (!(BusyBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                        HEAPASSERT((BusyBlock + FreeSize)->PreviousSize == (USHORT)FreeSize);
                    }

                    Heap->TotalFreeSize += FreeSize;

                } else {

                    //
                    //  The block is too big to go on a free list in its
                    //  entirety but we don't want to decommit anything so
                    //  simply call a worker routine to hack up the block
                    //  into pieces that will fit on the free lists.
                    //

                    RtlpInsertFreeBlock( Heap, (PHEAP_FREE_ENTRY)BusyBlock, FreeSize );
                }

            //
            //  Otherwise the block is to big for any lists and we should decommit
            //  the block
            //

            } else {

                RtlpDeCommitFreeBlock( Heap, (PHEAP_FREE_ENTRY)BusyBlock, FreeSize );
            }

        } else {

            //
            //  This is a big virtual block allocation.  To free it we only have to
            //  remove it from the heaps list of virtual allocated blocks, unlock
            //  the heap, and return the block to vm
            //

            PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

            VirtualAllocBlock = CONTAINING_RECORD( BusyBlock, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

            RemoveEntryList( &VirtualAllocBlock->Entry );

            //
            //  Release lock here as there is no reason to hold it across
            //  the system call.
            //

            if (LockAcquired) {

                RtlReleaseLockRoutine( Heap->LockVariable );
                LockAcquired = FALSE;
            }

            FreeSize = 0;

            Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&VirtualAllocBlock,
                                          &FreeSize,
                                          MEM_RELEASE );


            //
            //  Check if we had trouble freeing the block back to vm
            //  and return an error if necessary
            //

            if (!NT_SUCCESS( Status )) {

                SET_LAST_STATUS( Status );

                ReturnValue = FALSE;
            }
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    //
    //  The block was freed successfully so return success to our
    //  caller
    //

    return ReturnValue;
}


BOOLEAN
RtlFreeHeapSlowly (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This routine returns a previously allocated block back to its heap.
    It is the slower version of Rtl Free Heap and does more checking and
    tagging control.

Arguments:

    HeapHandle - Supplies a pointer to the owning heap structure

    Flags - Specifies the set of flags to use in the deallocation

    BaseAddress - Supplies a pointer to the block being freed

Return Value:

    BOOLEAN - TRUE if the block was properly freed and FALSE otherwise

--*/

{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    SIZE_T FreeSize;
    BOOLEAN Result;
    BOOLEAN LockAcquired = FALSE;

#ifndef NTOS_KERNEL_RUNTIME

    USHORT TagIndex;

#endif // NTOS_KERNEL_RUNTIME

    RTL_PAGED_CODE();

    //
    //  Note that Flags has already been OR'd with Heap->ForceFlags.
    //

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case see if we should be calling the debug version to
    //  free the heap
    //

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugFreeHeap( HeapHandle, Flags, BaseAddress );
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  Until we figure out otherwise we'll assume that this call will fail
    //

    Result = FALSE;

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
            //  Backup to get a pointer to the start of the block
            //

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            //
            //  Protect ourselves from idiots by refusing to free blocks
            //  that do not have the busy bit set.
            //
            //  Also refuse to free blocks that are not eight-byte aligned.
            //  The specific idiot in this case is Office95, which likes
            //  to free a random pointer when you start Word95 from a desktop
            //  shortcut.
            //
            //  As further insurance against idiots, check the segment index
            //  to make sure it is less than HEAP_MAXIMUM_SEGMENTS (16). This
            //  should fix all the dorks who have ASCII or Unicode where the
            //  heap header is supposed to be.
            //
            //  Note that this test is just opposite from the test used in
            //  Rtl Free Heap
            //

            if ((BusyBlock->Flags & HEAP_ENTRY_BUSY) &&
                (((ULONG_PTR)BaseAddress & 0x7) == 0) &&
                (BusyBlock->SegmentIndex < HEAP_MAXIMUM_SEGMENTS)) {

                //
                //  Check if this is a virtual block allocation
                //

                if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

                    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

                    //
                    //  This is a big virtual block allocation.  To free it
                    //  we only have to remove it from the heaps list of
                    //  virtual allocated blocks, unlock the heap, and return
                    //  the block to vm
                    //

                    VirtualAllocBlock = CONTAINING_RECORD( BusyBlock, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

                    RemoveEntryList( &VirtualAllocBlock->Entry );

    #ifndef NTOS_KERNEL_RUNTIME

                    //
                    //  In the non kernel case see if we need to free the tag
                    //

                    if (IS_HEAP_TAGGING_ENABLED()) {

                        RtlpUpdateTagEntry( Heap,
                                            VirtualAllocBlock->ExtraStuff.TagIndex,
                                            VirtualAllocBlock->CommitSize >> HEAP_GRANULARITY_SHIFT,
                                            0,
                                            VirtualFreeAction );
                    }

    #endif // NTOS_KERNEL_RUNTIME

                    FreeSize = 0;

                    Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                                  (PVOID *)&VirtualAllocBlock,
                                                  &FreeSize,
                                                  MEM_RELEASE );

                    //
                    //  Check if everything worked okay, if we had trouble freeing
                    //  the block back to vm return an error if necessary,
                    //

                    if (NT_SUCCESS( Status )) {

                        Result = TRUE;

                    } else {

                        SET_LAST_STATUS( Status );
                    }

                } else {

                    //
                    //  This block is not a big allocation so we need to
                    //  to get its size, and coalesce the blocks note that
                    //  the user mode heap does this conditionally on a heap
                    //  flag.  The coalesce function returns the newly formed
                    //  free block and the new size.
                    //

    #ifndef NTOS_KERNEL_RUNTIME

                    //
                    //  First in the non kernel case remove any tagging we might
                    //  have been using.  Note that the will either be in
                    //  the heap header, or in the extra block if present
                    //

                    if (IS_HEAP_TAGGING_ENABLED()) {

                        if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                            ExtraStuff = (PHEAP_ENTRY_EXTRA)(BusyBlock + BusyBlock->Size - 1);

                            TagIndex = RtlpUpdateTagEntry( Heap,
                                                           ExtraStuff->TagIndex,
                                                           BusyBlock->Size,
                                                           0,
                                                           FreeAction );

                        } else {

                            TagIndex = RtlpUpdateTagEntry( Heap,
                                                           BusyBlock->SmallTagIndex,
                                                           BusyBlock->Size,
                                                           0,
                                                           FreeAction );
                        }

                    } else {

                        TagIndex = 0;
                    }

    #endif // NTOS_KERNEL_RUNTIME

                    //
                    //  This is the size of the block we are freeing
                    //

                    FreeSize = BusyBlock->Size;

    #ifndef NTOS_KERNEL_RUNTIME

                    //
                    //  In the non kernel case see if we should coalesce on free
                    //

                    if (!(Heap->Flags & HEAP_DISABLE_COALESCE_ON_FREE)) {

    #endif // NTOS_KERNEL_RUNTIME

                        //
                        //  In kernel case and in the tested user mode case we
                        //  now coalesce free blocks
                        //

                        BusyBlock = (PHEAP_ENTRY)RtlpCoalesceFreeBlocks( Heap, (PHEAP_FREE_ENTRY)BusyBlock, &FreeSize, FALSE );

    #ifndef NTOS_KERNEL_RUNTIME

                    }

    #endif // NTOS_KERNEL_RUNTIME

                    //
                    //  If the block should not be decommit then try and put it
                    //  on a free list
                    //

                    if ((FreeSize < Heap->DeCommitFreeBlockThreshold) ||
                        ((Heap->TotalFreeSize + FreeSize) < Heap->DeCommitTotalFreeThreshold)) {

                        //
                        //  Check if the block can fit on one of the dedicated free
                        //  lists
                        //

                        if (FreeSize <= (ULONG)HEAP_MAXIMUM_BLOCK_SIZE) {

                            //
                            //  It can fit on a dedicated free list so insert it on
                            //

                            RtlpInsertFreeBlockDirect( Heap, (PHEAP_FREE_ENTRY)BusyBlock, (USHORT)FreeSize );

                            //
                            //  If there is a following entry then make sure the
                            //  sizes agree
                            //

                            if (!(BusyBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                                HEAPASSERT((BusyBlock + FreeSize)->PreviousSize == (USHORT)FreeSize);
                            }

                            //
                            //  Update the heap with the amount of free space
                            //  available
                            //

                            Heap->TotalFreeSize += FreeSize;

                        } else {

                            //
                            //  The block goes on the non dedicated free list
                            //

                            RtlpInsertFreeBlock( Heap, (PHEAP_FREE_ENTRY)BusyBlock, FreeSize );
                        }

    #ifndef NTOS_KERNEL_RUNTIME

                        //
                        //  In the non kernel case see if the there was tag and if
                        //  so then update the entry to show that it's been freed
                        //

                        if (TagIndex != 0) {

                            PHEAP_FREE_ENTRY_EXTRA FreeExtra;

                            BusyBlock->Flags |= HEAP_ENTRY_EXTRA_PRESENT;

                            FreeExtra = (PHEAP_FREE_ENTRY_EXTRA)(BusyBlock + BusyBlock->Size) - 1;

                            FreeExtra->TagIndex = TagIndex;
                            FreeExtra->FreeBackTraceIndex = 0;

    #if i386

                            //
                            //  In the x86 case we can also capture the stack
                            //  backtrace
                            //

                            if (Heap->Flags & HEAP_CAPTURE_STACK_BACKTRACES) {

                                FreeExtra->FreeBackTraceIndex = (USHORT)RtlLogStackBackTrace();
                            }

    #endif // i386

                        }

    #endif // NTOS_KERNEL_RUNTIME

                    } else {

                        //
                        //  Otherwise the block is big enough to decommit so have a
                        //  worker routine to do the decommit
                        //

                        RtlpDeCommitFreeBlock( Heap, (PHEAP_FREE_ENTRY)BusyBlock, FreeSize );
                    }

                    //
                    //  And say the free worked fine
                    //

                    Result = TRUE;
                }

            } else {

                //
                //  Not a busy block, or it's not aligned or the segment is
                //  to big, meaning it's corrupt
                //

                SET_LAST_STATUS( STATUS_INVALID_PARAMETER );
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


SIZE_T
RtlSizeHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )

/*++

Routine Description:

    This routine returns the size, in bytes, of the indicated block
    of heap storage.  The size only includes the number of bytes the
    original caller used to allocate the block and not any unused
    bytes at the end of the block.

Arguments:

    HeapHandle - Supplies a pointer to the heap that owns the block
        being queried

    Flags - Supplies a set of flags used to allocate the block

    BaseAddress - Supplies the address of the block being queried

Return Value:

    SIZE_T - returns the size, in bytes, of the queried block, or -1
        if the block is not in use.

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    SIZE_T BusySize;

    //
    //  Compliment the input flags with those enforced by the heap
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check if this is the nonkernel debug version of heap
    //

#ifndef NTOS_KERNEL_RUNTIME

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugSizeHeap( HeapHandle, Flags, BaseAddress );
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  No lock is required since nothing is modified and nothing
    //  outside the busy block is read.  Backup to get a pointer
    //  to the heap entry
    //

    BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

    //
    //  If the block is not in use then the answer is -1 and
    //  we'll set the error status for the user mode thread
    //

    if (!(BusyBlock->Flags & HEAP_ENTRY_BUSY)) {

        BusySize = -1;

        SET_LAST_STATUS( STATUS_INVALID_PARAMETER );

    //
    //  Otherwise if the block is from our large allocation then
    //  we'll get the result from that routine
    //

    } else if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

        BusySize = RtlpGetSizeOfBigBlock( BusyBlock );

    //
    //  Otherwise the block must be one that we can handle so
    //  calculate its block size and then subtract what's not being
    //  used by the caller.
    //
    //  **** this seems to include the heap entry header in its
    //  **** calculation.  Is that what we really want?
    //

    } else {

        BusySize = (BusyBlock->Size << HEAP_GRANULARITY_SHIFT) -
                   BusyBlock->UnusedBytes;
    }

    //
    //  And return to our caller
    //

    return BusySize;
}


NTSTATUS
RtlZeroHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags
    )

/*++

Routine Description:

    This routine zero's (or fills) in all the free blocks in a heap.
    It does not touch big allocations.

Arguments:

    HeapHandle - Supplies a pointer to the heap being zeroed

    Flags - Supplies a set of heap flags to compliment those already
        set in the heap

Return Value:

    NTSTATUS - An appropriate status code

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    NTSTATUS Status;
    BOOLEAN LockAcquired = FALSE;
    PHEAP_SEGMENT Segment;
    ULONG SegmentIndex;
    PHEAP_ENTRY CurrentBlock;
    PHEAP_FREE_ENTRY FreeBlock;
    SIZE_T Size;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;

    RTL_PAGED_CODE();

    //
    //  Compliment the input flags with those enforced by the heap
    //

    Flags |= Heap->ForceFlags;

    //
    //  Check if this is the nonkernel debug version of heap
    //

#ifndef NTOS_KERNEL_RUNTIME

    if (DEBUG_HEAP( Flags )) {

        return RtlDebugZeroHeap( HeapHandle, Flags );
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  Unless something happens otherwise we'll assume that we'll
    //  be successful
    //

    Status = STATUS_SUCCESS;

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
            //  Zero fill all the free blocks in all the segements
            //

            for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

                Segment = Heap->Segments[ SegmentIndex ];

                if (!Segment) {

                    continue;
                }

                UnCommittedRange = Segment->UnCommittedRanges;
                CurrentBlock = Segment->FirstEntry;

                //
                //  With the current segment we'll zoom through the
                //  blocks until we reach the end
                //

                while (CurrentBlock < Segment->LastValidEntry) {

                    Size = CurrentBlock->Size << HEAP_GRANULARITY_SHIFT;

                    //
                    //  If the block is not in use then we'll either zero
                    //  it or fill it.
                    //

                    if (!(CurrentBlock->Flags & HEAP_ENTRY_BUSY)) {

                        FreeBlock = (PHEAP_FREE_ENTRY)CurrentBlock;

                        if ((Heap->Flags & HEAP_FREE_CHECKING_ENABLED) &&
                            (CurrentBlock->Flags & HEAP_ENTRY_FILL_PATTERN)) {

                            RtlFillMemoryUlong( FreeBlock + 1,
                                                Size - sizeof( *FreeBlock ),
                                                FREE_HEAP_FILL );

                        } else {

                            RtlFillMemoryUlong( FreeBlock + 1,
                                                Size - sizeof( *FreeBlock ),
                                                0 );
                        }
                    }

                    //
                    //  If the following entry is uncommited then we need to
                    //  skip over it.  This code strongly implies that the
                    //  uncommitted range list is in perfect sync with the
                    //  blocks in the segement
                    //

                    if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {

                        CurrentBlock += CurrentBlock->Size;

                        //
                        //  Check if the we've reached the end of the segment
                        //  and should just break out of the while loop
                        //
                        //
                        //  **** "break;" would probably be more clear here
                        //

                        if (UnCommittedRange == NULL) {

                            CurrentBlock = Segment->LastValidEntry;

                        //
                        //  Otherwise skip over the uncommitted range
                        //

                        } else {

                            CurrentBlock = (PHEAP_ENTRY)
                                ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);

                            UnCommittedRange = UnCommittedRange->Next;
                        }

                    //
                    //  Otherwise the next block exists so advance to it
                    //

                    } else {

                        CurrentBlock += CurrentBlock->Size;
                    }
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

    } finally {

        //
        //  Unlock the heap
        //

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Status;
}


//
//  Local Support Routine
//

PHEAP_UNCOMMMTTED_RANGE
RtlpCreateUnCommittedRange (
    IN PHEAP_SEGMENT Segment
    )

/*++

Routine Description:

    This routine add a new uncommitted range structure to the specified heap
    segment.  This routine works by essentially doing a pop of the stack of
    unused uncommitted range structures located off the heap structure.  If
    the stack is empty then we'll create some more before doing the pop.

Arguments:

    Segment - Supplies the heap segment being modified

Return Value:

    PHEAP_UNCOMMITTED_RANGE - returns a pointer to the newly created
        uncommitted range structure

--*/

{
    NTSTATUS Status;
    PVOID FirstEntry, LastEntry;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;
    SIZE_T ReserveSize, CommitSize;
    PHEAP_UCR_SEGMENT UCRSegment;

    RTL_PAGED_CODE();

    //
    //  Get a pointer to the unused uncommitted range structures for
    //  the specified heap
    //

    pp = &Segment->Heap->UnusedUnCommittedRanges;

    //
    //  If the list is null then we need to allocate some more to
    //  put on the list
    //

    if (*pp == NULL) {

        //
        //  Get the next uncommitted range segment from the heap
        //

        UCRSegment = Segment->Heap->UCRSegments;

        //
        //  If there are no more uncommitted range segments or
        //  the segemtns commited and reserved sizes are equal (meaning
        //  it's all used up) then we need to allocate another uncommitted
        //  range segment
        //

        if ((UCRSegment == NULL) ||
            (UCRSegment->CommittedSize == UCRSegment->ReservedSize)) {

            //
            //  We'll reserve 16 pages of memory and commit at this
            //  time one page of it.
            //

            ReserveSize = PAGE_SIZE * 16;
            UCRSegment = NULL;

            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              &UCRSegment,
                                              0,
                                              &ReserveSize,
                                              MEM_RESERVE,
                                              PAGE_READWRITE );

            if (!NT_SUCCESS( Status )) {

                return NULL;
            }

            CommitSize = PAGE_SIZE;

            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              &UCRSegment,
                                              0,
                                              &CommitSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE );

            if (!NT_SUCCESS( Status )) {

                ZwFreeVirtualMemory( NtCurrentProcess(),
                                     &UCRSegment,
                                     &ReserveSize,
                                     MEM_RELEASE );

                return NULL;
            }

            //
            //  Add this new segment to the front of the UCR segments
            //

            UCRSegment->Next = Segment->Heap->UCRSegments;
            Segment->Heap->UCRSegments = UCRSegment;

            //
            //  Set the segments commit and reserve size
            //

            UCRSegment->ReservedSize = ReserveSize;
            UCRSegment->CommittedSize = CommitSize;

            //
            //  Point to the first free spot in the segment
            //

            FirstEntry = (PCHAR)(UCRSegment + 1);

        } else {

            //
            //  We have an existing UCR segment with available space
            //  So now try and commit another PAGE_SIZE bytes.  When we are done
            //  FirstEntry will point to the newly committed space
            //

            CommitSize = PAGE_SIZE;
            FirstEntry = (PCHAR)UCRSegment + UCRSegment->CommittedSize;

            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              &FirstEntry,
                                              0,
                                              &CommitSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE );

            if (!NT_SUCCESS( Status )) {

                return NULL;
            }

            //
            //  And update the amount committed in the segment
            //

            UCRSegment->CommittedSize += CommitSize;
        }

        //
        //  At this point UCR segment exists and First Entry points to the
        //  start of the available committed space.  We'll make Last Entry
        //  point to the end of the committed space
        //

        LastEntry = (PCHAR)UCRSegment + UCRSegment->CommittedSize;

        //
        //  Now the task is to push all of this new space unto the
        //  unused uncommitted range list off the heap, then we can
        //  do a regular pop
        //

        UnCommittedRange = (PHEAP_UNCOMMMTTED_RANGE)FirstEntry;

        pp = &Segment->Heap->UnusedUnCommittedRanges;

        while ((PCHAR)UnCommittedRange < (PCHAR)LastEntry) {

            *pp = UnCommittedRange;
            pp = &UnCommittedRange->Next;
            UnCommittedRange += 1;
        }

        //
        //  Null terminate the list
        //

        *pp = NULL;

        //
        //  And have Pp point the new top of the list
        //

        pp = &Segment->Heap->UnusedUnCommittedRanges;
    }

    //
    //  At this point the Pp points to a non empty list of unused uncommitted
    //  range structures.  So we pop the list and return the top to our caller
    //

    UnCommittedRange = *pp;
    *pp = UnCommittedRange->Next;

    return UnCommittedRange;
}


//
//  Local Support Routine
//

VOID
RtlpDestroyUnCommittedRange (
    IN PHEAP_SEGMENT Segment,
    IN PHEAP_UNCOMMMTTED_RANGE UnCommittedRange
    )

/*++

Routine Description:

    This routine returns an uncommitted range structure back to the unused
    uncommitted range list

Arguments:

    Segment - Supplies any segment in the heap being modified.  Most likely but
        not necessarily the segment containing the uncommitted range structure

    UnCommittedRange - Supplies a pointer to the uncommitted range structure
        being decommissioned.

Return Value:

    None.

--*/

{
    RTL_PAGED_CODE();

    //
    //  This routine simply does a "push" of the uncommitted range structure
    //  onto the heap's stack of unused uncommitted ranges
    //

    UnCommittedRange->Next = Segment->Heap->UnusedUnCommittedRanges;
    Segment->Heap->UnusedUnCommittedRanges = UnCommittedRange;

    //
    //  For safety sake we'll also zero out the fields in the decommissioned
    //  structure
    //

    UnCommittedRange->Address = 0;
    UnCommittedRange->Size = 0;

    //
    //  And return to our caller
    //

    return;
}


//
//  Local Support Routine
//

VOID
RtlpInsertUnCommittedPages (
    IN PHEAP_SEGMENT Segment,
    IN ULONG_PTR Address,
    IN SIZE_T Size
    )

/*++

Routine Description:

    This routine adds the specified range to the list of uncommitted pages
    in the segment.  When done the information will hang off the segments
    uncommitted ranges list.

Arguments:

    Segment - Supplies a segment whose uncommitted range is being modified

    Address - Supplies the base (start) address for the uncommitted range

    Size - Supplies the size, in bytes, of the uncommitted range

Return Value:

    None.

--*/

{
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange, *pp;

    RTL_PAGED_CODE();

    //
    //  Get a pointer to the front of the segments uncommitted range list
    //  The list is sorted by ascending address
    //

    pp = &Segment->UnCommittedRanges;

    //
    //  While we haven't reached the end of the list we'll zoom through
    //  trying to find a fit
    //

    while (UnCommittedRange = *pp) {

        //
        //  If address we want is less than what we're pointing at then
        //  we've found where this new entry goes
        //

        if (UnCommittedRange->Address > Address) {

            //
            //  If the new block matches right up to the existing block
            //  then we can simply backup the existing block and add
            //  to its size
            //

            if ((Address + Size) == UnCommittedRange->Address) {

                UnCommittedRange->Address = Address;
                UnCommittedRange->Size += Size;

                //
                //  Check if we need to update our notion of what the
                //  largest uncommitted range is
                //

                if (UnCommittedRange->Size > Segment->LargestUnCommittedRange) {

                    Segment->LargestUnCommittedRange = UnCommittedRange->Size;
                }

                //
                //  And return to our caller
                //

                return;
            }

            //
            //  Pp is the address of the block right before us, and *Pp is the
            //  address of the block right after us.  So now fall out to where
            //  the insertion takes place.
            //

            break;

        //
        //  Otherwise if this existing block stops right where the new block
        //  starts then we get to modify this entry.
        //

        } else if ((UnCommittedRange->Address + UnCommittedRange->Size) == Address) {

            //
            //  Remember the starting address and compute the new larger size
            //

            Address = UnCommittedRange->Address;
            Size += UnCommittedRange->Size;

            //
            //  Remove this entry from the list and then return it to the
            //  unused uncommitted list
            //

            *pp = UnCommittedRange->Next;

            RtlpDestroyUnCommittedRange( Segment, UnCommittedRange );

            //
            //  Modify the segemnt counters and largest size state.  The next
            //  time through the loop should hit the first case above where
            //  we'll either merge with a list following us or add a new
            //  entry
            //

            Segment->NumberOfUnCommittedRanges -= 1;

            if (Size > Segment->LargestUnCommittedRange) {

                Segment->LargestUnCommittedRange = Size;
            }

        //
        //  Otherwise we'll continue search down the list
        //

        } else {

            pp = &UnCommittedRange->Next;
        }
    }

    //
    //  If we reach this point that means we've either fallen off the end of the
    //  list, or the list is empty, or we've located the spot where a new uncommitted
    //  range structure belongs.  So allocate a new uncommitted range structure,
    //  and make sure we got one.
    //
    //  Pp is the address of the block right before us and *Pp is the address of the
    //  block right after us
    //

    UnCommittedRange = RtlpCreateUnCommittedRange( Segment );

    if (UnCommittedRange == NULL) {

        HeapDebugPrint(( "Abandoning uncommitted range (%x for %x)\n", Address, Size ));
        // HeapDebugBreak( NULL );

        return;
    }

    //
    //  Fill in the new uncommitted range structure
    //

    UnCommittedRange->Address = Address;
    UnCommittedRange->Size = Size;

    //
    //  Insert it in the list for the segment
    //

    UnCommittedRange->Next = *pp;
    *pp = UnCommittedRange;

    //
    //  Update the segment counters and notion of the largest uncommitted range
    //

    Segment->NumberOfUnCommittedRanges += 1;

    if (Size >= Segment->LargestUnCommittedRange) {

        Segment->LargestUnCommittedRange = Size;
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Declared in heappriv.h
//

PHEAP_FREE_ENTRY
RtlpFindAndCommitPages (
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN OUT PSIZE_T Size,
    IN PVOID AddressWanted OPTIONAL
    )

/*++

Routine Description:

    This function searches the supplied segment for an uncommitted range that
    satisfies the specified size.  It commits the range and returns a heap entry
    for the range.

Arguments:

    Heap - Supplies the heap being maniuplated

    Segment - Supplies the segment being searched

    Size - Supplies the size of what we need to look for, on return it contains
        the size of what we're just found and committed.

    AddressWanted - Optionally gives an address where we would like the pages
        based.  If supplied the entry must start at this address

Return Value:

    PHEAP_FREE_ENTRY - Returns a pointer to the newly committed range that
        satisfies the given size requirement, or NULL if we could not find
        something large enough and/or based at the address wanted.

--*/

{
    NTSTATUS Status;
    PHEAP_ENTRY FirstEntry, LastEntry, PreviousLastEntry;
    PHEAP_UNCOMMMTTED_RANGE PreviousUnCommittedRange, UnCommittedRange, *pp;
    ULONG_PTR Address;
    SIZE_T Length;

    RTL_PAGED_CODE();

    //
    //  What the outer loop does is cycle through the uncommited ranges
    //  stored in in the specified segment
    //

    PreviousUnCommittedRange = NULL;
    pp = &Segment->UnCommittedRanges;

    while (UnCommittedRange = *pp) {

        //
        //  Check for the best of worlds, where the size of this current
        //  uncommitted range satisfies our size request and either the user
        //  didn't specify an address or the address match
        //

        if ((UnCommittedRange->Size >= *Size) &&
            (!ARGUMENT_PRESENT( AddressWanted ) || (UnCommittedRange->Address == (ULONG_PTR)AddressWanted ))) {

            //
            //  Calculate an address
            //

            Address = UnCommittedRange->Address;

            //
            //  Commit the memory.  If the heap doesn't have a commit
            //  routine then use the default mm supplied routine.
            //

            if (Heap->CommitRoutine != NULL) {

                Status = (Heap->CommitRoutine)( Heap,
                                                (PVOID *)&Address,
                                                Size );

            } else {

                Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                                  (PVOID *)&Address,
                                                  0,
                                                  Size,
                                                  MEM_COMMIT,
                                                  PAGE_READWRITE );

            }

            if (!NT_SUCCESS( Status )) {

                return NULL;
            }

            //
            //  At this point we have some committed memory, with Address and Size
            //  giving us the necessary details
            //
            //  Update the number of uncommitted pages in the segment and if necessary
            //  mark down the largest uncommitted range
            //

            Segment->NumberOfUnCommittedPages -= (ULONG) (*Size / PAGE_SIZE);

            if (Segment->LargestUnCommittedRange == UnCommittedRange->Size) {

                Segment->LargestUnCommittedRange = 0;
            }

            //
            //  First entry is the start of the newly committed range
            //

            FirstEntry = (PHEAP_ENTRY)Address;

            //
            //  We want last entry to point to the last real entry before
            //  this newly committed spot.  To do this we start by
            //  setting last entry to either the first entry for the
            //  segment or (if we can do better), to right after the last
            //  uncommitted range we examined.  Either way it points to
            //  some committed range
            //

            if ((Segment->LastEntryInSegment->Flags & HEAP_ENTRY_LAST_ENTRY) &&
                (ULONG_PTR)(Segment->LastEntryInSegment + Segment->LastEntryInSegment->Size) == UnCommittedRange->Address) {

                LastEntry = Segment->LastEntryInSegment;

            } else {

                if (PreviousUnCommittedRange == NULL) {

                    LastEntry = Segment->FirstEntry;

                } else {

                    LastEntry = (PHEAP_ENTRY)(PreviousUnCommittedRange->Address +
                                              PreviousUnCommittedRange->Size);
                }

                //
                //  Now we zoom through the entries until we find the one
                //  marked last
                //

                while (!(LastEntry->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                    PreviousLastEntry = LastEntry;
                    LastEntry += LastEntry->Size;

                    if (((PCHAR)LastEntry >= (PCHAR)Segment->LastValidEntry) || (LastEntry->Size == 0)) {

                        //
                        //  Check for the situation where the last entry in the
                        //  segment isn't marked as a last entry but does put
                        //  us right where the have a new committed range
                        //

                        if (LastEntry == (PHEAP_ENTRY)Address) {

                            LastEntry = PreviousLastEntry;

                            break;
                        }

                        HeapDebugPrint(( "Heap missing last entry in committed range near %x\n", PreviousLastEntry ));
                        HeapDebugBreak( PreviousLastEntry );

                        return NULL;
                    }
                }
            }

            //
            //  Turn off the last bit on this entry because what's following
            //  is no longer uncommitted
            //

            LastEntry->Flags &= ~HEAP_ENTRY_LAST_ENTRY;

            //
            //  Shrink the uncommited range by the size we've committed
            //

            UnCommittedRange->Address += *Size;
            UnCommittedRange->Size -= *Size;

            //
            //  Now if the size is zero then we've committed everything that there
            //  was in the range.  Otherwise make sure the first entry of what
            //  we've just committed knows that an uncommitted range follows.
            //

            if (UnCommittedRange->Size == 0) {

                //
                //  This uncommitted range is about to vanish.  Base on if the
                //  range is the last one in the segemnt then we know how to
                //  mark the committed range as being last or not.
                //

                if (UnCommittedRange->Address == (ULONG_PTR)Segment->LastValidEntry) {

                    FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;

                    Segment->LastEntryInSegment = FirstEntry;

                } else {

                    FirstEntry->Flags = 0;

                    Segment->LastEntryInSegment = Segment->FirstEntry;
                }

                //
                //  Remove this zero sized range from the uncommitted range
                //  list, and update the segment counters
                //

                *pp = UnCommittedRange->Next;

                RtlpDestroyUnCommittedRange( Segment, UnCommittedRange );

                Segment->NumberOfUnCommittedRanges -= 1;

            } else {

                //
                //  Otherwise the range is not empty so we know what we committed
                //  is immediately followed by an uncommitted range
                //

                FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;

                Segment->LastEntryInSegment = FirstEntry;
            }

            //
            //  Update the fields in the first entry, and optional
            //  following entry.
            //

            FirstEntry->SegmentIndex = LastEntry->SegmentIndex;
            FirstEntry->Size = (USHORT)(*Size >> HEAP_GRANULARITY_SHIFT);
            FirstEntry->PreviousSize = LastEntry->Size;

            if (!(FirstEntry->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                (FirstEntry + FirstEntry->Size)->PreviousSize = FirstEntry->Size;
            }

            //
            //  Now if we adjusted the largest uncommitted range to zero then
            //  we need to go back and find the largest uncommitted range
            //  To do that we simply zoom down the uncommitted range list
            //  remembering the largest one
            //

            if (Segment->LargestUnCommittedRange == 0) {

                UnCommittedRange = Segment->UnCommittedRanges;

                while (UnCommittedRange != NULL) {

                    if (UnCommittedRange->Size >= Segment->LargestUnCommittedRange) {

                        Segment->LargestUnCommittedRange = UnCommittedRange->Size;
                    }

                    UnCommittedRange = UnCommittedRange->Next;
                }
            }

            //
            //  And return the heap entry to our caller
            //

            return (PHEAP_FREE_ENTRY)FirstEntry;

        } else {

            //
            //  Otherwise the current uncommited range is too small or
            //  doesn't have the right address so go to the next uncommitted
            //  range entry
            //

            PreviousUnCommittedRange = UnCommittedRange;
            pp = &UnCommittedRange->Next;
        }
    }

    //
    //  At this point we did not find an uncommitted range entry that satisfied
    //  our requirements either because of size and/or address.  So return null
    //  to tell the user we didn't find anything.
    //

    return NULL;
}


//
//  Declared in heappriv.h
//

BOOLEAN
RtlpInitializeHeapSegment (
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN UCHAR SegmentIndex,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UnCommittedAddress,
    IN PVOID CommitLimitAddress
    )

/*++

Routine Description:

    This routines initializes the internal structures for a heap segment.
    The caller supplies the heap and the memory for the segment being
    initialized

Arguments:

    Heap - Supplies the address of the heap owning this segment

    Segment - Supplies a pointer to the segment being initialized

    SegmentIndex - Supplies the segement index within the heap that this
        new segment is being assigned

    Flags - Supplies flags controlling the initialization of the segment
        Valid flags are:

            HEAP_SEGMENT_USER_ALLOCATED

    BaseAddress - Supplies the base address for the segment

    UnCommittedAddress - Supplies the address where the uncommited range starts

    CommitLimitAddress - Supplies the top address available to the segment

Return Value:

    BOOLEAN - TRUE if the initialization is successful and FALSE otherwise

--*/

{
    NTSTATUS Status;
    PHEAP_ENTRY FirstEntry;
    USHORT PreviousSize, Size;
    ULONG NumberOfPages;
    ULONG NumberOfCommittedPages;
    ULONG NumberOfUnCommittedPages;
    SIZE_T CommitSize;
    ULONG NtGlobalFlag = RtlGetNtGlobalFlags();

    RTL_PAGED_CODE();

    //
    //  Compute the total number of pages possible in this segment
    //

    NumberOfPages = (ULONG) (((PCHAR)CommitLimitAddress - (PCHAR)BaseAddress) / PAGE_SIZE);

    //
    //  First entry points to the first possible segment entry after
    //  the segment header
    //

    FirstEntry = (PHEAP_ENTRY)ROUND_UP_TO_POWER2( Segment + 1,
                                                  HEAP_GRANULARITY );

    //
    //  Now if the heap is equal to the base address for the segment which
    //  it the case for the segment zero then the previous size is the
    //  heap header.  Otherwise there isn't a previous entry
    //

    if ((PVOID)Heap == BaseAddress) {

        PreviousSize = Heap->Entry.Size;

    } else {

        PreviousSize = 0;
    }

    //
    //  Compute the index size of the segement header
    //

    Size = (USHORT)(((PCHAR)FirstEntry - (PCHAR)Segment) >> HEAP_GRANULARITY_SHIFT);

    //
    //  If the first available heap entry is not committed and
    //  it is beyond the heap limit then we cannot initialize
    //

    if ((PCHAR)(FirstEntry + 1) >= (PCHAR)UnCommittedAddress) {

        if ((PCHAR)(FirstEntry + 1) >= (PCHAR)CommitLimitAddress) {

            return FALSE;
        }

        //
        //  Enough of the segment has not been committed so we
        //  will commit enough now to handle the first entry
        //

        CommitSize = (PCHAR)(FirstEntry + 1) - (PCHAR)UnCommittedAddress;

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&UnCommittedAddress,
                                          0,
                                          &CommitSize,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );

        if (!NT_SUCCESS( Status )) {

            return FALSE;
        }

        //
        //  Because we had to commit some memory we need to adjust
        //  the uncommited address
        //

        UnCommittedAddress = (PVOID)((PCHAR)UnCommittedAddress + CommitSize);
    }

    //
    //  At this point we know there is enough memory committed to handle the
    //  segment header and one heap entry
    //
    //  Now compute the number of uncommited pages and the number of committed
    //  pages
    //

    NumberOfUnCommittedPages = (ULONG)(((PCHAR)CommitLimitAddress - (PCHAR)UnCommittedAddress) / PAGE_SIZE);
    NumberOfCommittedPages = NumberOfPages - NumberOfUnCommittedPages;

    //
    //  Initialize the heap segment heap entry.  We
    //  calculated earlier if there was a previous entry
    //

    Segment->Entry.PreviousSize = PreviousSize;
    Segment->Entry.Size = Size;
    Segment->Entry.Flags = HEAP_ENTRY_BUSY;
    Segment->Entry.SegmentIndex = SegmentIndex;

#if i386 && !NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel x86 case see if we need to capture the callers stack
    //  backtrace
    //

    if (NtGlobalFlag & FLG_USER_STACK_TRACE_DB) {

        Segment->AllocatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();
    }

#endif // i386 && !NTOS_KERNEL_RUNTIME

    //
    //  Now initializes the heap segment
    //

    Segment->Signature = HEAP_SEGMENT_SIGNATURE;
    Segment->Flags = Flags;
    Segment->Heap = Heap;
    Segment->BaseAddress = BaseAddress;
    Segment->FirstEntry = FirstEntry;
    Segment->LastValidEntry = (PHEAP_ENTRY)((PCHAR)BaseAddress + (NumberOfPages * PAGE_SIZE));
    Segment->NumberOfPages = NumberOfPages;
    Segment->NumberOfUnCommittedPages = NumberOfUnCommittedPages;

    //
    //  If there are uncommitted pages then we need to insert them
    //  into the uncommitted ranges list
    //

    if (NumberOfUnCommittedPages) {

        RtlpInsertUnCommittedPages( Segment,
                                    (ULONG_PTR)UnCommittedAddress,
                                    NumberOfUnCommittedPages * PAGE_SIZE );
    }

    //
    //  Have the containing heap point to this segment via the specified index
    //

    Heap->Segments[ SegmentIndex ] = Segment;

    //
    //  Initialize the first free heap entry after the heap segment header and
    //  put it in the free list.  This first entry will be for whatever is left
    //  of the committed range
    //

    PreviousSize = Segment->Entry.Size;
    FirstEntry->Flags = HEAP_ENTRY_LAST_ENTRY;

    Segment->LastEntryInSegment = FirstEntry;

    FirstEntry->PreviousSize = PreviousSize;
    FirstEntry->SegmentIndex = SegmentIndex;

    RtlpInsertFreeBlock( Heap,
                         (PHEAP_FREE_ENTRY)FirstEntry,
                         (PHEAP_ENTRY)UnCommittedAddress - FirstEntry);

    //
    //  And return to our caller
    //

    return TRUE;
}


//
//  Local Support Routine
//

NTSTATUS
RtlpDestroyHeapSegment (
    IN PHEAP_SEGMENT Segment
    )

/*++

Routine Description:

    This routine removes an existing heap segment.  After the call it
    is as if the segment never existed

Arguments:

    Segment - Supplies a pointer to the heap segment being destroyed

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    PVOID BaseAddress;
    SIZE_T BytesToFree;

    RTL_PAGED_CODE();

    //
    //  We actually only have work to do if the segment is not
    //  user allocated.  If the segement is user allocated then
    //  we'll assume knows how to get rid of the memory
    //

    if (!(Segment->Flags & HEAP_SEGMENT_USER_ALLOCATED)) {

        BaseAddress = Segment->BaseAddress;
        BytesToFree = 0;

        //
        //  Free all the virtual memory for the segment and return
        //  to our caller.
        //

        return ZwFreeVirtualMemory( NtCurrentProcess(),
                                    (PVOID *)&BaseAddress,
                                    &BytesToFree,
                                    MEM_RELEASE );

    } else {

        //
        //  User allocated segments are a noop
        //

        return STATUS_SUCCESS;
    }
}



//
//  Local Support Routine
//

PHEAP_FREE_ENTRY
RtlpExtendHeap (
    IN PHEAP Heap,
    IN SIZE_T AllocationSize
    )

/*++

Routine Description:

    This routine is used to extend the amount of committed memory in a heap

Arguments:

    Heap - Supplies the heap being modified

    AllocationSize - Supplies the size, in bytes, that we need to extend the
        heap

Return Value:

    PHEAP_FREE_ENTRY - Returns a pointer to the newly created heap entry
        of the specified size, or NULL if we weren't able to extend the heap

--*/

{
    NTSTATUS Status;
    PHEAP_SEGMENT Segment;
    PHEAP_FREE_ENTRY FreeBlock;
    UCHAR SegmentIndex, EmptySegmentIndex;
    ULONG NumberOfPages;
    SIZE_T CommitSize;
    SIZE_T ReserveSize;
    SIZE_T FreeSize;

    RTL_PAGED_CODE();

    //
    //  Compute the number of pages need to hold this extension
    //  And then compute the real free, still in bytes, based on
    //  the page count
    //

    NumberOfPages = (ULONG) ((AllocationSize + PAGE_SIZE - 1) / PAGE_SIZE);
    FreeSize = NumberOfPages * PAGE_SIZE;

    //
    //  For every segment we're either going to look for an existing
    //  heap segment that we can get some pages out of or we will
    //  identify a free heap segment index where we'll try and create a new
    //  segment
    //

    EmptySegmentIndex = HEAP_MAXIMUM_SEGMENTS;

    for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

        Segment = Heap->Segments[ SegmentIndex ];

        //
        //  If the segment exists and number of uncommitted pages will
        //  satisfy our reguest and the largest uncommitted range will
        //  also satisfy our request then we'll try and segment
        //
        //  **** note that this second test seems unnecessary given that
        //  **** the largest uncommitted range is also being tested
        //

        if ((Segment) &&
            (NumberOfPages <= Segment->NumberOfUnCommittedPages) &&
            (FreeSize <= Segment->LargestUnCommittedRange)) {

            //
            //  Looks like a good segment so try and commit the
            //  amount we need
            //

            FreeBlock = RtlpFindAndCommitPages( Heap,
                                                Segment,
                                                &FreeSize,
                                                NULL );

            //
            //  If we were successful the we will coalesce it with adjacent
            //  free blocks and put it in the free list then return the
            //  the free block
            //

            if (FreeBlock != NULL) {

                //
                //  **** gdk ****
                //  **** this doesn't seem right given that coalesece should take
                //  **** byte count and not heap entry count
                //

                FreeSize = FreeSize >> HEAP_GRANULARITY_SHIFT;

                FreeBlock = RtlpCoalesceFreeBlocks( Heap, FreeBlock, &FreeSize, FALSE );

                RtlpInsertFreeBlock( Heap, FreeBlock, FreeSize );

                return FreeBlock;
            }

        //
        //  Otherwise if the segment index is not in use and we haven't
        //  yet identified a unused segment index then remembeer this
        //  index
        //

        } else if ((Segment == NULL) &&
                   (EmptySegmentIndex == HEAP_MAXIMUM_SEGMENTS)) {

            EmptySegmentIndex = SegmentIndex;
        }
    }

    //
    //  At this point we weren't able to get the memory from an existing
    //  heap segment so now check if we found an unused segment index
    //  and if we're alowed to grow the heap.
    //

    if ((EmptySegmentIndex != HEAP_MAXIMUM_SEGMENTS) &&
        (Heap->Flags & HEAP_GROWABLE)) {

        Segment = NULL;

        //
        //  Calculate a reserve size for the new segment, we might
        //  need to fudge it up if the allocation size we're going for
        //  right now is already beyond the default reserve size
        //

        if ((AllocationSize + PAGE_SIZE) > Heap->SegmentReserve) {

            ReserveSize = AllocationSize + PAGE_SIZE;

        } else {

            ReserveSize = Heap->SegmentReserve;
        }

        //
        //  Try and reserve some vm
        //

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          (PVOID *)&Segment,
                                          0,
                                          &ReserveSize,
                                          MEM_RESERVE,
                                          PAGE_READWRITE );

        //
        //  If we get back status no memory then we should trim back the
        //  request to something reasonable and try again.  We'll half
        //  the amount until we it either succeeds or until we reach
        //  the allocation size.  In the latter case we are really
        //  out of memory.
        //

        while ((!NT_SUCCESS( Status )) && (ReserveSize != (AllocationSize + PAGE_SIZE))) {

            ReserveSize = ReserveSize / 2;

            if( ReserveSize < (AllocationSize + PAGE_SIZE) ) {

                ReserveSize = (AllocationSize + PAGE_SIZE);
            }

            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              (PVOID *)&Segment,
                                              0,
                                              &ReserveSize,
                                              MEM_RESERVE,
                                              PAGE_READWRITE );
        }

        if (NT_SUCCESS( Status )) {

            //
            //  Adjust the heap state information
            //

            Heap->SegmentReserve += ReserveSize;

            //
            //  Compute the commit size to be either the default, or if
            //  that's not big enough then make it big enough to handle
            //  this current request
            //

            if ((AllocationSize + PAGE_SIZE) > Heap->SegmentCommit) {

                CommitSize = AllocationSize + PAGE_SIZE;

            } else {

                CommitSize = Heap->SegmentCommit;
            }

            //
            //  Try and commit the memory
            //

            Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                              (PVOID *)&Segment,
                                              0,
                                              &CommitSize,
                                              MEM_COMMIT,
                                              PAGE_READWRITE );

            //
            //  If the commit is successful but we were not able to
            //  initialize the heap segment then still make the status
            //  and error value
            //

            if (NT_SUCCESS( Status ) &&
                !RtlpInitializeHeapSegment( Heap,
                                            Segment,
                                            EmptySegmentIndex,
                                            0,
                                            Segment,
                                            (PCHAR)Segment + CommitSize,
                                            (PCHAR)Segment + ReserveSize)) {

                Status = STATUS_NO_MEMORY;
            }

            //
            //  If we've been successful so far then we're done and we
            //  can return the first entry in the segment to our caller
            //

            if (NT_SUCCESS(Status)) {

                return (PHEAP_FREE_ENTRY)Segment->FirstEntry;
            }

            //
            //  Otherwise either the commit or heap segment initialization failed
            //  so we'll release the memory which will also decommit it if necessary
            //

            ZwFreeVirtualMemory( NtCurrentProcess(),
                                 (PVOID *)&Segment,
                                 &ReserveSize,
                                 MEM_RELEASE );
        }
    }

#ifndef NTOS_KERNEL_RUNTIME

    //
    //  In the non kernel case we disabled coaleseing on free then what we'll
    //  do as a last resort is coalesce the heap and see if a block comes out
    //  that we can use
    //

    if (Heap->Flags & HEAP_DISABLE_COALESCE_ON_FREE) {

        FreeBlock = RtlpCoalesceHeap( Heap );

        if ((FreeBlock != NULL) && (FreeBlock->Size >= AllocationSize)) {

            return FreeBlock;
        }
    }

#endif // NTOS_KERNEL_RUNTIME

    //
    //  Either the heap cannot grow or we out of resources of some type
    //  so we're going to return null
    //

    return NULL;
}


//
//  Declared in heappriv.h
//

PHEAP_FREE_ENTRY
RtlpCoalesceFreeBlocks (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN OUT PSIZE_T FreeSize,
    IN BOOLEAN RemoveFromFreeList
    )

/*++

Routine Description:

    This routine coalesces the free block together.

Arguments:

    Heap - Supplies a pointer to the heap being manipulated

    FreeBlock - Supplies a pointer to the free block that we want coalesced

    FreeSize - Supplies the size, in bytes, of the free block.  On return it
        contains the size, in bytes, of the of the newly coalesced free block

    RemoveFromFreeList - Indicates if the input free block is already on a
        free list and needs to be removed to before coalescing

Return Value:

    PHEAP_FREE_ENTRY - returns a pointer to the newly coalesced free block

--*/

{
    PHEAP_FREE_ENTRY FreeBlock1, NextFreeBlock;

    RTL_PAGED_CODE();

    //
    //  Point to the preceding block
    //

    FreeBlock1 = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock - FreeBlock->PreviousSize);

    //
    //  Check if there is a preceding block, and if it is free, and the two sizes
    //  put together will still fit on a free lists.
    //

    if ((FreeBlock1 != FreeBlock) &&
        !(FreeBlock1->Flags & HEAP_ENTRY_BUSY) &&
        ((*FreeSize + FreeBlock1->Size) <= HEAP_MAXIMUM_BLOCK_SIZE)) {

        //
        //  We are going to merge ourselves with the preceding block
        //

        HEAPASSERT(FreeBlock->PreviousSize == FreeBlock1->Size);

        //
        //  Check if we need to remove the input block from the free list
        //

        if (RemoveFromFreeList) {

            RtlpRemoveFreeBlock( Heap, FreeBlock );

            Heap->TotalFreeSize -= FreeBlock->Size;

            //
            //  We're removed so we don't have to do it again
            //

            RemoveFromFreeList = FALSE;
        }

        //
        //  Remove the preceding block from its free list
        //

        RtlpRemoveFreeBlock( Heap, FreeBlock1 );

        //
        //  Copy over the last entry flag if necessary from what we're freeing
        //  to the preceding block
        //

        FreeBlock1->Flags = FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY;

        if( FreeBlock1->Flags & HEAP_ENTRY_LAST_ENTRY ) {

            PHEAP_SEGMENT Segment;

            Segment = Heap->Segments[FreeBlock1->SegmentIndex];
            Segment->LastEntryInSegment = (PHEAP_ENTRY)FreeBlock1;
        }

        //
        //  Point to the preceding block, and adjust the sizes for the
        //  new free block.  It is the total of both blocks.
        //

        FreeBlock = FreeBlock1;

        *FreeSize += FreeBlock1->Size;

        Heap->TotalFreeSize -= FreeBlock1->Size;

        FreeBlock->Size = (USHORT)*FreeSize;

        //
        //  Check if we need to update the previous size of the next
        //  entry
        //

        if (!(FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

            ((PHEAP_ENTRY)FreeBlock + *FreeSize)->PreviousSize = (USHORT)*FreeSize;
        }
    }

    //
    //  Check if there is a following block.
    //

    if (!(FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

        //
        //  There is a following block so now get a pointer to it
        //  and check if it is free and if putting the two blocks together
        //  still fits on a free list
        //

        NextFreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock + *FreeSize);

        if (!(NextFreeBlock->Flags & HEAP_ENTRY_BUSY) &&
            ((*FreeSize + NextFreeBlock->Size) <= HEAP_MAXIMUM_BLOCK_SIZE)) {

            //
            //  We are going to merge ourselves with the following block
            //

            HEAPASSERT(*FreeSize == NextFreeBlock->PreviousSize);

            //
            //  Check if we need to remove the input block from the free list
            //

            if (RemoveFromFreeList) {

                RtlpRemoveFreeBlock( Heap, FreeBlock );

                Heap->TotalFreeSize -= FreeBlock->Size;

                //
                //  **** this assignment isn't necessary because there isn't
                //  **** any more merging after this one
                //

                RemoveFromFreeList = FALSE;
            }

            //
            //  Copy up the last entry flag if necessary from the following
            //  block to our input block
            //

            FreeBlock->Flags = NextFreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY;

            if( FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY ) {

                PHEAP_SEGMENT Segment;

                Segment = Heap->Segments[FreeBlock->SegmentIndex];
                Segment->LastEntryInSegment = (PHEAP_ENTRY)FreeBlock;
            }

            //
            //  Remove the following block from its free list
            //

            RtlpRemoveFreeBlock( Heap, NextFreeBlock );

            //
            //  Adjust the size for the newly combined block
            //

            *FreeSize += NextFreeBlock->Size;

            Heap->TotalFreeSize -= NextFreeBlock->Size;

            FreeBlock->Size = (USHORT)*FreeSize;

            //
            //  Check if we need to update the previous size of the next block
            //

            if (!(FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

                ((PHEAP_ENTRY)FreeBlock + *FreeSize)->PreviousSize = (USHORT)*FreeSize;
            }
        }
    }

    //
    //  And return the free block to our caller
    //

    return FreeBlock;
}


//
//  Declared in heappriv.h
//

VOID
RtlpDeCommitFreeBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN SIZE_T FreeSize
    )

/*++

Routine Description:

    This routine takes a free block and decommits it.  This is usually called
    because the block is beyond the decommit threshold

Arguments:

    Heap - Supplies a pointer to the heap being manipulated

    FreeBlock - Supplies a pointer to the block being decommitted

    FreeSize - Supplies the size, in bytes, of the free block being decommitted

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    ULONG_PTR DeCommitAddress;
    SIZE_T DeCommitSize;
    USHORT LeadingFreeSize, TrailingFreeSize;
    PHEAP_SEGMENT Segment;
    PHEAP_FREE_ENTRY LeadingFreeBlock, TrailingFreeBlock;
    PHEAP_ENTRY LeadingBusyBlock, TrailingBusyBlock;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;

    RTL_PAGED_CODE();

    //
    //  If the heap has a user specified decommit routine then we won't really
    //  decommit anything instead we'll call a worker routine to chop it up
    //  into pieces that will fit on the free lists
    //

    if (Heap->CommitRoutine != NULL) {

        RtlpInsertFreeBlock( Heap, FreeBlock, FreeSize );

        return;
    }

    //
    //  Get a pointer to the owning segment
    //

    Segment = Heap->Segments[ FreeBlock->SegmentIndex ];

    //
    //  The leading busy block identifies the preceding in use block before
    //  what we are trying to decommit.  It is only used if what we are trying
    //  decommit is right on a page boundary and then it is the block right
    //  before us if it exists.
    //
    //  The leading free block is used to identify whatever space is needed
    //  to round up the callers specified address to a page address.  If the
    //  caller already gave us a page aligned address then the free block
    //  address is identical to what the caller supplied.
    //

    LeadingBusyBlock = NULL;
    LeadingFreeBlock = FreeBlock;

    //
    //  Make sure the block we are trying to decommit start on the next full
    //  page boundary.  The leading free size is the size of whatever it takes
    //  to round up the free block to the next page specified in units of
    //  heap entries.
    //

    DeCommitAddress = ROUND_UP_TO_POWER2( LeadingFreeBlock, PAGE_SIZE );
    LeadingFreeSize = (USHORT)((PHEAP_ENTRY)DeCommitAddress - (PHEAP_ENTRY)LeadingFreeBlock);

    //
    //  If we leading free size only has space for one heap entry then we'll
    //  bump it up to include the next page, because we don't want to leave
    //  anything that small laying around.  Otherwise if we have a preceding
    //  block and the leading free size is zero then identify the preceding
    //  block as the leading busy block
    //

    if (LeadingFreeSize == 1) {

        DeCommitAddress += PAGE_SIZE;
        LeadingFreeSize += PAGE_SIZE >> HEAP_GRANULARITY_SHIFT;

    } else if (LeadingFreeBlock->PreviousSize != 0) {

        if (DeCommitAddress == (ULONG_PTR)LeadingFreeBlock) {

            LeadingBusyBlock = (PHEAP_ENTRY)LeadingFreeBlock - LeadingFreeBlock->PreviousSize;
        }
    }

    //
    //  The trailing busy block identifies the block immediately after the one
    //  we are trying to decommit provided what we are decommitting ends right
    //  on a page boundary otherwise the trailing busy block stays null and
    //  the trailing free block value is used.
    //
    //  **** gdk ****
    //  **** the assignment of tailing free block doesn't seem right because
    //  **** Free size should be in bytes, and not heap entries
    //

    TrailingBusyBlock = NULL;
    TrailingFreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock + FreeSize);

    //
    //  Make sure the block we are trying to decommit ends on a page boundary.
    //
    //  And compute how many heap entries we had to backup to make it land on a
    //  page boundary.
    //

    DeCommitSize = ROUND_DOWN_TO_POWER2( (ULONG_PTR)TrailingFreeBlock, PAGE_SIZE );
    TrailingFreeSize = (USHORT)((PHEAP_ENTRY)TrailingFreeBlock - (PHEAP_ENTRY)DeCommitSize);

    //
    //  If the trailing free size is exactly one heap in size then we will
    //  nibble off a bit more from the decommit size because free block of
    //  exactly one heap entry in size are useless.  Otherwise if we actually
    //  ended on a page boundary and there is a block after us then indicate
    //  that we have a trailing busy block
    //

    if (TrailingFreeSize == (sizeof( HEAP_ENTRY ) >> HEAP_GRANULARITY_SHIFT)) {

        DeCommitSize -= PAGE_SIZE;
        TrailingFreeSize += PAGE_SIZE >> HEAP_GRANULARITY_SHIFT;

    } else if ((TrailingFreeSize == 0) && !(FreeBlock->Flags & HEAP_ENTRY_LAST_ENTRY)) {

        TrailingBusyBlock = (PHEAP_ENTRY)TrailingFreeBlock;
    }

    //
    //  Now adjust the trailing free block to compensate for the trailing free size
    //  we just computed.
    //

    TrailingFreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)TrailingFreeBlock - TrailingFreeSize);

    //
    //  Right now DeCommit size is really a pointer.  If it points at is beyond
    //  the decommit address then make the size really be just the byte count
    //  to decommit.  Otherwise the decommit size is zero.
    //

    if (DeCommitSize > DeCommitAddress) {

        DeCommitSize -= DeCommitAddress;

    } else {

        DeCommitSize = 0;
    }

    //
    //  **** this next test is bogus given the if-then-else that just preceded it
    //
    //  Now check if we still have something to decommit
    //

    if (DeCommitSize != 0) {

        //
        //  Before freeing the memory to MM we have to be sure we can create 
        //  a PHEAP_UNCOMMMTTED_RANGE later. So we do it right now
        //

        UnCommittedRange = RtlpCreateUnCommittedRange(Segment);

        if (UnCommittedRange == NULL) {
            
            HeapDebugPrint(( "Failing creating uncommitted range (%x for %x)\n", DeCommitAddress, DeCommitSize ));

            //
            //  We weren't successful in the decommit so now simply
            //  add the leading free block to the free list
            //

            RtlpInsertFreeBlock( Heap, LeadingFreeBlock, FreeSize );

            return;
        }

        //
        //  Decommit the memory
        //

        Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                      (PVOID *)&DeCommitAddress,
                                      &DeCommitSize,
                                      MEM_DECOMMIT );

        //
        //  Push back the UnCommittedRange structure. Now the insert cannot fail
        //

        RtlpDestroyUnCommittedRange( Segment, UnCommittedRange );

        if (NT_SUCCESS( Status )) {

            //
            //  Insert information regarding the pages we just decommitted
            //  to the lsit of uncommited pages in the segment
            //

            RtlpInsertUnCommittedPages( Segment,
                                        DeCommitAddress,
                                        DeCommitSize );
            //
            //  Adjust the segments count of uncommitted pages
            //

            Segment->NumberOfUnCommittedPages += (ULONG)(DeCommitSize / PAGE_SIZE);

            //
            //  If we have a leading free block then mark its proper state
            //  update the heap, and put it on the free list
            //

            if (LeadingFreeSize != 0) {

                LeadingFreeBlock->Flags = HEAP_ENTRY_LAST_ENTRY;
                LeadingFreeBlock->Size = LeadingFreeSize;
                Heap->TotalFreeSize += LeadingFreeSize;

                Segment->LastEntryInSegment = (PHEAP_ENTRY)LeadingFreeBlock;

                RtlpInsertFreeBlockDirect( Heap, LeadingFreeBlock, LeadingFreeSize );

            //
            //  Otherwise if we actually have a leading busy block then
            //  make sure the busy block knows we're uncommitted
            //

            } else if (LeadingBusyBlock != NULL) {

                LeadingBusyBlock->Flags |= HEAP_ENTRY_LAST_ENTRY;

                Segment->LastEntryInSegment = LeadingBusyBlock;

            } else if ((Segment->LastEntryInSegment >= (PHEAP_ENTRY)DeCommitAddress)
                            &&
                       ((PCHAR)Segment->LastEntryInSegment < ((PCHAR)DeCommitAddress + DeCommitSize))) {

                     Segment->LastEntryInSegment = Segment->FirstEntry;
            }

            //
            //  If there is a trailing free block then sets its state,
            //  update the heap, and insert it on a free list
            //

            if (TrailingFreeSize != 0) {

                TrailingFreeBlock->PreviousSize = 0;
                TrailingFreeBlock->SegmentIndex = Segment->Entry.SegmentIndex;
                TrailingFreeBlock->Flags = 0;
                TrailingFreeBlock->Size = TrailingFreeSize;

                ((PHEAP_FREE_ENTRY)((PHEAP_ENTRY)TrailingFreeBlock + TrailingFreeSize))->PreviousSize = (USHORT)TrailingFreeSize;

                RtlpInsertFreeBlockDirect( Heap, TrailingFreeBlock, TrailingFreeSize );

                Heap->TotalFreeSize += TrailingFreeSize;

            //
            //  Otherwise if we actually have a succeeding block then
            //  make it know we are uncommitted
            //

            } else if (TrailingBusyBlock != NULL) {

                TrailingBusyBlock->PreviousSize = 0;
            }

        } else {

            //
            //  We weren't successful in the decommit so now simply
            //  add the leading free block to the free list
            //

            RtlpInsertFreeBlock( Heap, LeadingFreeBlock, FreeSize );
        }

    } else {

        //
        //  There is nothing left to decommit to take our leading free block
        //  and put it on a free list
        //

        RtlpInsertFreeBlock( Heap, LeadingFreeBlock, FreeSize );
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
RtlpInsertFreeBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN SIZE_T FreeSize
    )

/*++

Routine Description:

    This routines take a piece of committed memory and adds to the
    the appropriate free lists for the heap.  If necessary this
    routine will divide up the free block to sizes that fit
    on the free list


Arguments:

    Heap - Supplies a pointer to the owning heap

    FreeBlock - Supplies a pointer to the block being freed

    FreeSize - Supplies the size, in bytes, of the block being freed

Return Value:

    None.

--*/

{
    USHORT PreviousSize, Size;
    UCHAR Flags;
    UCHAR SegmentIndex;
    PHEAP_SEGMENT Segment;

    RTL_PAGED_CODE();

    //
    //  Get the size of the previous block, the index of the segment
    //  containing this block, and the flags specific to the block
    //

    PreviousSize = FreeBlock->PreviousSize;

    SegmentIndex = FreeBlock->SegmentIndex;
    Segment = Heap->Segments[ SegmentIndex ];

    Flags = FreeBlock->Flags;

    //
    //  Adjust the total amount free in the heap
    //

    Heap->TotalFreeSize += FreeSize;

    //
    //  Now, while there is still something left to add to the free list
    //  we'll process the information
    //

    while (FreeSize != 0) {

        //
        //  If the size is too big for our free lists then we'll
        //  chop it down.
        //

        if (FreeSize > (ULONG)HEAP_MAXIMUM_BLOCK_SIZE) {

            Size = HEAP_MAXIMUM_BLOCK_SIZE;

            //
            //  This little adjustment is so that we don't have a remainder
            //  that is too small to be useful on the next iteration
            //  through the loop
            //

            if (FreeSize == ((ULONG)HEAP_MAXIMUM_BLOCK_SIZE + 1)) {

                Size -= 16;
            }

            //
            //  Guarantee that Last entry does not get set in this
            //  block.
            //

            FreeBlock->Flags = 0;

        } else {

            Size = (USHORT)FreeSize;

            //
            //  This could propagate the last entry flag
            //

            FreeBlock->Flags = Flags;
        }

        //
        //  Update the block sizes and then insert this
        //  block into a free list
        //

        FreeBlock->PreviousSize = PreviousSize;
        FreeBlock->SegmentIndex = SegmentIndex;
        FreeBlock->Size = Size;

        RtlpInsertFreeBlockDirect( Heap, FreeBlock, Size );

        //
        //  Note the size of what we just freed, and then update
        //  our state information for the next time through the
        //  loop
        //

        PreviousSize = Size;

        FreeSize -= Size;
        FreeBlock = (PHEAP_FREE_ENTRY)((PHEAP_ENTRY)FreeBlock + Size);

        //
        //  Check if we're done with the free block based on the
        //  segment information, otherwise go back up and check size
        //  Note that is means that we can get called with a very
        //  large size and still work.
        //

        if ((PHEAP_ENTRY)FreeBlock >= Segment->LastValidEntry) {

            return;
        }
    }

    //
    //  If the block we're freeing did not think it was the last entry
    //  then tell the next block our real size.
    //

    if (!(Flags & HEAP_ENTRY_LAST_ENTRY)) {

        FreeBlock->PreviousSize = PreviousSize;
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Declared in heappriv.h
//

PHEAP_ENTRY_EXTRA
RtlpGetExtraStuffPointer (
    PHEAP_ENTRY BusyBlock
    )

/*++

Routine Description:

    This routine calculates where the extra stuff record will be given
    the busy block and returns a pointer to it.  The caller must have
    already checked that the entry extry field is present

Arguments:

    BusyBlock - Supplies the busy block whose extra stuff we are seeking

Return Value:

    PHEAP_ENTRY_EXTRA - returns a pointer to the extra stuff record.

--*/

{
    ULONG AllocationIndex;

    //
    //  On big blocks the extra stuff is automatically part of the
    //  block
    //

    if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

        PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

        VirtualAllocBlock = CONTAINING_RECORD( BusyBlock, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

        return &VirtualAllocBlock->ExtraStuff;

    } else {

        //
        //  On non big blocks the extra stuff follows immediately after
        //  the allocation itself.
        //
        //  **** What a hack
        //  **** We do some funny math here because the busy block
        //  **** stride is 8 bytes we know we can stride it by its
        //  **** index minus one to get to the end of the allocation
        //

        AllocationIndex = BusyBlock->Size;

        return (PHEAP_ENTRY_EXTRA)(BusyBlock + AllocationIndex - 1);
    }
}


//
//  Declared in heappriv.h
//

SIZE_T
RtlpGetSizeOfBigBlock (
    IN PHEAP_ENTRY BusyBlock
    )

/*++

Routine Description:

    This routine returns the size, in bytes, of the big allocation block

Arguments:

    BusyBlock - Supplies a pointer to the block being queried

Return Value:

    SIZE_T - Returns the size, in bytes, that was allocated to the big
        block

--*/

{
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;

    RTL_PAGED_CODE();

    //
    //  Get a pointer to the block header itself
    //

    VirtualAllocBlock = CONTAINING_RECORD( BusyBlock, HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock );

    //
    //  The size allocated to the block is actually the difference between the
    //  commit size stored in the virtual alloc block and the size stored in
    //  in the block.
    //

    return VirtualAllocBlock->CommitSize - BusyBlock->Size;
}


//
//  Declared in heappriv.h
//

BOOLEAN
RtlpCheckBusyBlockTail (
    IN PHEAP_ENTRY BusyBlock
    )

/*++

Routine Description:

    This routine checks to see if the bytes beyond the user specified
    allocation have been modified.  It does this by checking for a tail
    fill pattern

Arguments:

    BusyBlock - Supplies the heap block being queried

Return Value:

    BOOLEAN - TRUE if the tail is still okay and FALSE otherwise

--*/

{
    PCHAR Tail;
    SIZE_T Size, cbEqual;

    RTL_PAGED_CODE();

    //
    //  Compute the user allocated size of the input heap block
    //

    if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

        Size = RtlpGetSizeOfBigBlock( BusyBlock );

    } else {

        Size = (BusyBlock->Size << HEAP_GRANULARITY_SHIFT) - BusyBlock->UnusedBytes;
    }

    //
    //  Compute a pointer to the tail of the input block.  This would
    //  be the space right after the user allocated portion
    //

    Tail = (PCHAR)(BusyBlock + 1) + Size;

    //
    //  Check if the tail fill pattern is still there
    //

    cbEqual = RtlCompareMemory( Tail,
                                CheckHeapFillPattern,
                                CHECK_HEAP_TAIL_SIZE );

    //
    //  If the number we get back isn't equal to the tail size then
    //  someone modified the block beyond its user specified allocation
    //  size
    //

    if (cbEqual != CHECK_HEAP_TAIL_SIZE) {

        //
        //  Do some debug printing
        //

        HeapDebugPrint(( "Heap block at %p modified at %p past requested size of %lx\n",
                         BusyBlock,
                         Tail + cbEqual,
                         Size ));

        HeapDebugBreak( BusyBlock );

        //
        //  And tell our caller there was an error
        //

        return FALSE;

    } else {

        //
        //  And return to our caller that the tail is fine
        //

        return TRUE;
    }
}
