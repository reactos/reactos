/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    heapdbg.c

Abstract:

    This module implements a debugging layer on top of heap allocator.

Author:

    Steve Wood (stevewo) 20-Sep-1994

Revision History:

--*/

#include "ntrtlp.h"
#include "heap.h"
#include "heappriv.h"

BOOLEAN RtlpValidateHeapHdrsEnable = FALSE; // Set to TRUE if headers are being corrupted
BOOLEAN RtlpValidateHeapTagsEnable;         // Set to TRUE if tag counts are off and you want to know why

HEAP_STOP_ON_VALUES RtlpHeapStopOn;


const struct {

    ULONG Offset;
    LPSTR Description;

} RtlpHeapHeaderFieldOffsets[] = {

    FIELD_OFFSET( HEAP, Entry ),                        "Entry",
    FIELD_OFFSET( HEAP, Signature ),                    "Signature",
    FIELD_OFFSET( HEAP, Flags ),                        "Flags",
    FIELD_OFFSET( HEAP, ForceFlags ),                   "ForceFlags",
    FIELD_OFFSET( HEAP, VirtualMemoryThreshold ),       "VirtualMemoryThreshold",
    FIELD_OFFSET( HEAP, SegmentReserve ),               "SegmentReserve",
    FIELD_OFFSET( HEAP, SegmentCommit ),                "SegmentCommit",
    FIELD_OFFSET( HEAP, DeCommitFreeBlockThreshold ),   "DeCommitFreeBlockThreshold",
    FIELD_OFFSET( HEAP, DeCommitTotalFreeThreshold ),   "DeCommitTotalFreeThreshold",
    FIELD_OFFSET( HEAP, TotalFreeSize ),                "TotalFreeSize",
    FIELD_OFFSET( HEAP, MaximumAllocationSize ),        "MaximumAllocationSize",
    FIELD_OFFSET( HEAP, ProcessHeapsListIndex ),        "ProcessHeapsListIndex",
    FIELD_OFFSET( HEAP, HeaderValidateLength ),         "HeaderValidateLength",
    FIELD_OFFSET( HEAP, HeaderValidateCopy ),           "HeaderValidateCopy",
    FIELD_OFFSET( HEAP, NextAvailableTagIndex ),        "NextAvailableTagIndex",
    FIELD_OFFSET( HEAP, MaximumTagIndex ),              "MaximumTagIndex",
    FIELD_OFFSET( HEAP, TagEntries ),                   "TagEntries",
    FIELD_OFFSET( HEAP, UCRSegments ),                  "UCRSegments",
    FIELD_OFFSET( HEAP, UnusedUnCommittedRanges ),      "UnusedUnCommittedRanges",
    FIELD_OFFSET( HEAP, AlignRound ),                   "AlignRound",
    FIELD_OFFSET( HEAP, AlignMask ),                    "AlignMask",
    FIELD_OFFSET( HEAP, VirtualAllocdBlocks ),          "VirtualAllocdBlocks",
    FIELD_OFFSET( HEAP, Segments ),                     "Segments",
    FIELD_OFFSET( HEAP, u ),                            "FreeListsInUse",
    FIELD_OFFSET( HEAP, FreeListsInUseTerminate ),      "FreeListsInUseTerminate",
    FIELD_OFFSET( HEAP, AllocatorBackTraceIndex ),      "AllocatorBackTraceIndex",
    FIELD_OFFSET( HEAP, Reserved1 ),                    "Reserved1",
    FIELD_OFFSET( HEAP, PseudoTagEntries ),             "PseudoTagEntries",
    FIELD_OFFSET( HEAP, FreeLists ),                    "FreeLists",
    FIELD_OFFSET( HEAP, LockVariable ),                 "LockVariable",
    FIELD_OFFSET( HEAP, Lookaside ),                    "Lookaside",
    FIELD_OFFSET( HEAP, LookasideLockCount ),           "LookasideLockCount",
    sizeof( HEAP ),                                     "Uncommitted Ranges",
    0xFFFF, NULL
};


VOID
RtlpUpdateHeapListIndex (
    USHORT OldIndex,
    USHORT NewIndex
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    if (RtlpHeapStopOn.AllocTag.HeapIndex == OldIndex) {

        RtlpHeapStopOn.AllocTag.HeapIndex = NewIndex;
    }

    if (RtlpHeapStopOn.ReAllocTag.HeapIndex == OldIndex) {

        RtlpHeapStopOn.ReAllocTag.HeapIndex = NewIndex;
    }

    if (RtlpHeapStopOn.FreeTag.HeapIndex == OldIndex) {

        RtlpHeapStopOn.FreeTag.HeapIndex = NewIndex;
    }

    return;
}


BOOLEAN
RtlpValidateHeapHeaders (
    IN PHEAP Heap,
    IN BOOLEAN Recompute
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    ULONG i;
    SIZE_T n;
    SIZE_T nEqual;
    NTSTATUS Status;

    if (!RtlpValidateHeapHdrsEnable) {

        return TRUE;
    }

    if (Heap->HeaderValidateCopy == NULL) {

        n = Heap->HeaderValidateLength;

        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &Heap->HeaderValidateCopy,
                                          0,
                                          &n,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );

        if (!NT_SUCCESS( Status )) {

            return TRUE;
        }

        Recompute = TRUE;
    }

    n = Heap->HeaderValidateLength;

    if (!Recompute) {

        nEqual = RtlCompareMemory( Heap,
                                   Heap->HeaderValidateCopy,
                                   n );

    } else {

        RtlMoveMemory( Heap->HeaderValidateCopy,
                       Heap,
                       n );

        nEqual = n;
    }

    if (n != nEqual) {

        HeapDebugPrint(( "Heap %x - headers modified (%x is %x instead of %x)\n",
                         Heap,
                         (PCHAR)Heap + nEqual,
                         *(PULONG)((PCHAR)Heap + nEqual),
                         *(PULONG)((PCHAR)Heap->HeaderValidateCopy + nEqual)));

        for (i=0; RtlpHeapHeaderFieldOffsets[ i ].Description != NULL; i++) {

            if ((nEqual >= RtlpHeapHeaderFieldOffsets[ i ].Offset) &&
                (nEqual < RtlpHeapHeaderFieldOffsets[ i+1 ].Offset)) {

                DbgPrint( "    This is located in the %s field of the heap header.\n",
                                 RtlpHeapHeaderFieldOffsets[ i ].Description );

                break;
            }
        }

        return FALSE;

    } else {

        return TRUE;
    }
}


PVOID
RtlDebugCreateHeap (
    IN ULONG Flags,
    IN PVOID HeapBase OPTIONAL,
    IN SIZE_T ReserveSize OPTIONAL,
    IN SIZE_T CommitSize OPTIONAL,
    IN PVOID Lock OPTIONAL,
    IN PRTL_HEAP_PARAMETERS Parameters
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap;
    NTSTATUS Status;
    MEMORY_BASIC_INFORMATION MemoryInformation;

    if (ReserveSize <= sizeof( HEAP_ENTRY )) {

        HeapDebugPrint(( "Invalid ReserveSize parameter - %lx\n", ReserveSize ));
        HeapDebugBreak( NULL );

        return NULL;
    }

    if (ReserveSize < CommitSize) {

        HeapDebugPrint(( "Invalid CommitSize parameter - %lx\n", CommitSize ));
        HeapDebugBreak( NULL );

        return NULL;
    }

    if ((Flags & HEAP_NO_SERIALIZE) && ARGUMENT_PRESENT( Lock )) {

        HeapDebugPrint(( "May not specify Lock parameter with HEAP_NO_SERIALIZE\n" ));
        HeapDebugBreak( NULL );

        return NULL;
    }

    if (ARGUMENT_PRESENT( HeapBase )) {

        Status = NtQueryVirtualMemory( NtCurrentProcess(),
                                       HeapBase,
                                       MemoryBasicInformation,
                                       &MemoryInformation,
                                       sizeof( MemoryInformation ),
                                       NULL );

        if (!NT_SUCCESS( Status )) {

            HeapDebugPrint(( "Specified HeapBase (%lx) invalid,  Status = %lx\n",
                             HeapBase,
                             Status ));

            HeapDebugBreak( NULL );

            return NULL;
        }

        if (MemoryInformation.BaseAddress != HeapBase) {

            HeapDebugPrint(( "Specified HeapBase (%lx) != to BaseAddress (%lx)\n",
                             HeapBase,
                             MemoryInformation.BaseAddress ));

            HeapDebugBreak( NULL );

            return NULL;
        }

        if (MemoryInformation.State == MEM_FREE) {

            HeapDebugPrint(( "Specified HeapBase (%lx) is free or not writable\n",
                             MemoryInformation.BaseAddress ));

            HeapDebugBreak( NULL );

            return NULL;
        }
    }

    Heap = RtlCreateHeap( Flags |
                            HEAP_SKIP_VALIDATION_CHECKS |
                            HEAP_TAIL_CHECKING_ENABLED  |
                            HEAP_FREE_CHECKING_ENABLED,
                          HeapBase,
                          ReserveSize,
                          CommitSize,
                          Lock,
                          Parameters );

    if (Heap != NULL) {

#if i386

        if (Heap->Flags & HEAP_CAPTURE_STACK_BACKTRACES) {

            Heap->AllocatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();
        }

#endif // i386

        RtlpValidateHeapHeaders( Heap, TRUE );
    }

    return Heap;
}


BOOLEAN
RtlpSerializeHeap (
    IN PVOID HeapHandle
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_LOCK Lock;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapSerialize( HeapHandle ));

    //
    //  Validate that HeapAddress points to a HEAP structure.
    //

    if (!RtlpCheckHeapSignature( Heap, "RtlpSerializeHeap" )) {

        return FALSE;
    }

    //
    //  Lock the heap.
    //

    if (Heap->Flags & HEAP_NO_SERIALIZE) {

        Lock = RtlAllocateHeap( HeapHandle, HEAP_NO_SERIALIZE, sizeof( *Lock ) );

        if ( Lock == NULL ) {

            return FALSE;
        }
    
        Status = RtlInitializeLockRoutine( Lock );

        if (!NT_SUCCESS( Status )) {

            RtlFreeHeap( HeapHandle, HEAP_NO_SERIALIZE, Lock );

            return FALSE;
        }

        Heap->LockVariable = Lock;
        Heap->Flags &= ~HEAP_NO_SERIALIZE;
        Heap->ForceFlags &= ~HEAP_NO_SERIALIZE;

        RtlpValidateHeapHeaders( Heap, TRUE );
    }

    return TRUE;
}


BOOLEAN
RtlDebugDestroyHeap (
    IN PVOID HeapHandle
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    LIST_ENTRY ListEntry;
    SIZE_T n;

    if (HeapHandle == NtCurrentPeb()->ProcessHeap) {

        HeapDebugPrint(( "May not destroy the process heap at %x\n", HeapHandle ));

        return FALSE;
    }

    if (!RtlpCheckHeapSignature( Heap, "RtlDestroyHeap" )) {

        return FALSE;
    }

    if (!RtlpValidateHeap( Heap, FALSE )) {

        return FALSE;
    }

    //
    //  Now mark the heap as invalid by zeroing the signature field.
    //

    Heap->Signature = 0;

    if (Heap->HeaderValidateCopy != NULL) {

        n = 0;
        NtFreeVirtualMemory( NtCurrentProcess(),
                             &Heap->HeaderValidateCopy,
                             &n,
                             MEM_RELEASE );
    }

    return TRUE;
}


PVOID
RtlDebugAllocateHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    PVOID ReturnValue = NULL;
    SIZE_T AllocationSize;
    USHORT TagIndex;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapAllocate( HeapHandle, Flags, Size ));

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlAllocateHeap" )) {

                ReturnValue = NULL;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SETTABLE_USER_VALUE | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Verify that the size did not wrap or exceed the limit for this heap.
            //

            AllocationSize = (((Size ? Size : 1) + Heap->AlignRound) & Heap->AlignMask) +
                             sizeof( HEAP_ENTRY_EXTRA );

            if ((AllocationSize < Size) || (AllocationSize > Heap->MaximumAllocationSize)) {

                HeapDebugPrint(( "Invalid allocation size - %lx (exceeded %x)\n",
                                 Size,
                                 Heap->MaximumAllocationSize ));

                ReturnValue = NULL;
                leave;
            }

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            RtlpValidateHeap( Heap, FALSE );

            ReturnValue = RtlAllocateHeapSlowly( HeapHandle, Flags, Size );

            RtlpValidateHeapHeaders( Heap, TRUE );

            if (ReturnValue != NULL) {

                BusyBlock = (PHEAP_ENTRY)ReturnValue - 1;

                if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                    ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

    #if i386

                    if (Heap->Flags & HEAP_CAPTURE_STACK_BACKTRACES) {

                        ExtraStuff->AllocatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();

                    } else {

                        ExtraStuff->AllocatorBackTraceIndex = 0;
                    }

    #endif // i386

                    TagIndex = ExtraStuff->TagIndex;

                } else {

                    TagIndex = BusyBlock->SmallTagIndex;
                }

                if (Heap->Flags & HEAP_VALIDATE_ALL_ENABLED) {

                    RtlpValidateHeap( Heap, FALSE );
                }
            }

            if (ReturnValue != NULL) {

                if ((ULONG_PTR)ReturnValue == RtlpHeapStopOn.AllocAddress) {

                    HeapDebugPrint(( "Just allocated block at %lx for 0x%x bytes\n",
                                     RtlpHeapStopOn.AllocAddress,
                                     Size ));

                    HeapDebugBreak( NULL );

                } else if ((IS_HEAP_TAGGING_ENABLED()) &&
                           (TagIndex != 0) &&
                           (TagIndex == RtlpHeapStopOn.AllocTag.TagIndex) &&
                           (Heap->ProcessHeapsListIndex == RtlpHeapStopOn.AllocTag.HeapIndex)) {

                    HeapDebugPrint(( "Just allocated block at %lx for 0x%x bytes with tag %ws\n",
                                     ReturnValue,
                                     Size,
                                     RtlpGetTagName( Heap, TagIndex )));

                    HeapDebugBreak( NULL );
                }
            }

        } except( GetExceptionCode() == STATUS_NO_MEMORY ? EXCEPTION_CONTINUE_SEARCH :
                                                           EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );

            ReturnValue = NULL;
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return ReturnValue;
}


PVOID
RtlDebugReAllocateHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN SIZE_T Size
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    SIZE_T AllocationSize;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    BOOLEAN LockAcquired = FALSE;
    PVOID ReturnValue = NULL;
    USHORT TagIndex;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapReAllocate( HeapHandle, Flags, BaseAddress, Size ));

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlReAllocateHeap" )) {

                ReturnValue = NULL;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SETTABLE_USER_VALUE | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Verify that the size did not wrap or exceed the limit for this heap.
            //

            AllocationSize = (((Size ? Size : 1) + Heap->AlignRound) & Heap->AlignMask) +
                             sizeof( HEAP_ENTRY_EXTRA );

            if (AllocationSize < Size || AllocationSize > Heap->MaximumAllocationSize) {

                HeapDebugPrint(( "Invalid allocation size - %lx (exceeded %x)\n",
                                 Size,
                                 Heap->MaximumAllocationSize ));

                HeapDebugBreak( NULL );

                ReturnValue = NULL;
                leave;
            }

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            RtlpValidateHeap( Heap, FALSE );
            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            if (RtlpValidateHeapEntry( Heap, BusyBlock, "RtlReAllocateHeap" )) {

                if ((ULONG_PTR)BaseAddress == RtlpHeapStopOn.ReAllocAddress) {

                    HeapDebugPrint(( "About to reallocate block at %lx to 0x%x bytes\n",
                                     RtlpHeapStopOn.ReAllocAddress,
                                     Size ));

                    HeapDebugBreak( NULL );

                } else if (IS_HEAP_TAGGING_ENABLED() && RtlpHeapStopOn.ReAllocTag.HeapAndTagIndex != 0) {

                    if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                        ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );
                        TagIndex = ExtraStuff->TagIndex;

                    } else {

                        TagIndex = BusyBlock->SmallTagIndex;
                    }

                    if ((TagIndex != 0) &&
                        (TagIndex == RtlpHeapStopOn.ReAllocTag.TagIndex) &&
                        (Heap->ProcessHeapsListIndex == RtlpHeapStopOn.ReAllocTag.HeapIndex)) {

                        HeapDebugPrint(( "About to rellocate block at %lx to 0x%x bytes with tag %ws\n",
                                         BaseAddress,
                                         Size,
                                         RtlpGetTagName( Heap, TagIndex )));

                        HeapDebugBreak( NULL );
                    }
                }

                ReturnValue = RtlReAllocateHeap( HeapHandle, Flags, BaseAddress, Size );

                if (ReturnValue != NULL) {

                    BusyBlock = (PHEAP_ENTRY)ReturnValue - 1;

                    if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                        ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

    #if i386

                        if (Heap->Flags & HEAP_CAPTURE_STACK_BACKTRACES) {

                            ExtraStuff->AllocatorBackTraceIndex = (USHORT)RtlLogStackBackTrace();

                        } else {

                            ExtraStuff->AllocatorBackTraceIndex = 0;
                        }

    #endif // i386

                        TagIndex = ExtraStuff->TagIndex;

                    } else {

                        TagIndex = BusyBlock->SmallTagIndex;
                    }
                }

                RtlpValidateHeapHeaders( Heap, TRUE );
                RtlpValidateHeap( Heap, FALSE );
            }

            if (ReturnValue != NULL) {

                if ((ULONG_PTR)ReturnValue == RtlpHeapStopOn.ReAllocAddress) {

                    HeapDebugPrint(( "Just reallocated block at %lx to 0x%x bytes\n",
                                     RtlpHeapStopOn.ReAllocAddress,
                                     Size ));

                    HeapDebugBreak( NULL );

                } else if ((IS_HEAP_TAGGING_ENABLED()) &&
                           (TagIndex == RtlpHeapStopOn.ReAllocTag.TagIndex) &&
                           (Heap->ProcessHeapsListIndex == RtlpHeapStopOn.ReAllocTag.HeapIndex)) {

                    HeapDebugPrint(( "Just reallocated block at %lx to 0x%x bytes with tag %ws\n",
                                     ReturnValue,
                                     Size,
                                     RtlpGetTagName( Heap, TagIndex )));

                    HeapDebugBreak( NULL );
                }
            }

        } except( GetExceptionCode() == STATUS_NO_MEMORY ? EXCEPTION_CONTINUE_SEARCH :
                                                           EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );

            ReturnValue = NULL;
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return ReturnValue;
}


BOOLEAN
RtlDebugFreeHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    SIZE_T Size;
    BOOLEAN Result = FALSE;
    BOOLEAN LockAcquired = FALSE;
    USHORT TagIndex;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapFree( HeapHandle, Flags, BaseAddress ));

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlFreeHeap" )) {

                Result = FALSE;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            RtlpValidateHeap( Heap, FALSE );

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;
            Size = BusyBlock->Size << HEAP_GRANULARITY_SHIFT;

            if (RtlpValidateHeapEntry( Heap, BusyBlock, "RtlFreeHeap" )) {

                if ((ULONG_PTR)BaseAddress == RtlpHeapStopOn.FreeAddress) {

                    HeapDebugPrint(( "About to free block at %lx\n",
                                     RtlpHeapStopOn.FreeAddress ));

                    HeapDebugBreak( NULL );

                } else if ((IS_HEAP_TAGGING_ENABLED()) && (RtlpHeapStopOn.FreeTag.HeapAndTagIndex != 0)) {

                    if (BusyBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                        ExtraStuff = RtlpGetExtraStuffPointer( BusyBlock );

                        TagIndex = ExtraStuff->TagIndex;

                    } else {

                        TagIndex = BusyBlock->SmallTagIndex;
                    }

                    if ((TagIndex != 0) &&
                        (TagIndex == RtlpHeapStopOn.FreeTag.TagIndex) &&
                        (Heap->ProcessHeapsListIndex == RtlpHeapStopOn.FreeTag.HeapIndex)) {

                        HeapDebugPrint(( "About to free block at %lx with tag %ws\n",
                                         BaseAddress,
                                         RtlpGetTagName( Heap, TagIndex )));

                        HeapDebugBreak( NULL );
                    }
                }

                Result = RtlFreeHeapSlowly( HeapHandle, Flags, BaseAddress );

                RtlpValidateHeapHeaders( Heap, TRUE );
                RtlpValidateHeap( Heap, FALSE );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );

            Result = FALSE;
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Result;
}


BOOLEAN
RtlDebugGetUserInfoHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    OUT PVOID *UserValue OPTIONAL,
    OUT PULONG UserFlags OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN Result = FALSE;
    BOOLEAN LockAcquired = FALSE;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapGetUserInfo( HeapHandle, Flags, BaseAddress, UserValue, UserFlags ));

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlGetUserInfoHeap" )) {

                Result = FALSE;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            RtlpValidateHeap( Heap, FALSE );

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            if (RtlpValidateHeapEntry( Heap, BusyBlock, "RtlGetUserInfoHeap" )) {

                Result = RtlGetUserInfoHeap( HeapHandle, Flags, BaseAddress, UserValue, UserFlags );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Result;
}


BOOLEAN
RtlDebugSetUserValueHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN PVOID UserValue
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN Result = FALSE;
    BOOLEAN LockAcquired = FALSE;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapSetUserValue( HeapHandle, Flags, BaseAddress, UserValue ));

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlSetUserValueHeap" )) {

                Result = FALSE;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            RtlpValidateHeap( Heap, FALSE );

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            if (RtlpValidateHeapEntry( Heap, BusyBlock, "RtlSetUserValueHeap" )) {

                Result = RtlSetUserValueHeap( HeapHandle, Flags, BaseAddress, UserValue );

                RtlpValidateHeap( Heap, FALSE );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Result;
}


BOOLEAN
RtlDebugSetUserFlagsHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress,
    IN ULONG UserFlagsReset,
    IN ULONG UserFlagsSet
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN Result = FALSE;
    BOOLEAN LockAcquired = FALSE;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapSetUserFlags( HeapHandle, Flags, BaseAddress, UserFlagsReset, UserFlagsSet ));

    if ((UserFlagsReset & ~HEAP_SETTABLE_USER_FLAGS) ||
        (UserFlagsSet & ~HEAP_SETTABLE_USER_FLAGS)) {

        return FALSE;
    }

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlSetUserFlagsHeap" )) {

                Result = FALSE;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            RtlpValidateHeap( Heap, FALSE );

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            if (RtlpValidateHeapEntry( Heap, BusyBlock, "RtlSetUserFlagsHeap" )) {

                Result = RtlSetUserFlagsHeap( HeapHandle, Flags, BaseAddress, UserFlagsReset, UserFlagsSet );

                RtlpValidateHeap( Heap, FALSE );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Result;
}


SIZE_T
RtlDebugSizeHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    PHEAP_ENTRY BusyBlock;
    BOOLEAN LockAcquired = FALSE;
    SIZE_T BusySize;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapSize( HeapHandle, Flags, BaseAddress ));

    BusySize = 0xFFFFFFFF;

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlSizeHeap" )) {

                BusySize = FALSE;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                Flags |= HEAP_NO_SERIALIZE;

                LockAcquired = TRUE;
            }

            RtlpValidateHeap( Heap, FALSE );

            BusyBlock = (PHEAP_ENTRY)BaseAddress - 1;

            if (RtlpValidateHeapEntry( Heap, BusyBlock, "RtlSizeHeap" )) {

                BusySize = RtlSizeHeap( HeapHandle, Flags, BaseAddress );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return BusySize;
}


SIZE_T
RtlDebugCompactHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    SIZE_T LargestFreeSize;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapCompact( HeapHandle, Flags ));

    LargestFreeSize = 0;

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlCompactHeap" )) {

                LargestFreeSize = 0;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            RtlpValidateHeap( Heap, FALSE );

            LargestFreeSize = RtlCompactHeap( HeapHandle, Flags );

            RtlpValidateHeapHeaders( Heap, TRUE );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return LargestFreeSize;
}


NTSTATUS
RtlDebugZeroHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    SIZE_T LargestFreeSize;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapZero( HeapHandle, Flags ));

    Status = STATUS_SUCCESS;
    LargestFreeSize = 0;

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlZeroHeap" )) {

                Status = STATUS_INVALID_PARAMETER;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            if (!RtlpValidateHeap( Heap, FALSE )) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = RtlZeroHeap( HeapHandle, Flags );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Status;
}


NTSTATUS
RtlDebugCreateTagHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PWSTR TagPrefix OPTIONAL,
    IN PWSTR TagNames
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    ULONG TagIndex;

    TagIndex = 0;

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (RtlpCheckHeapSignature( Heap, "RtlCreateTagHeap" )) {

                Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

                //
                //  Lock the heap
                //

                if (!(Flags & HEAP_NO_SERIALIZE)) {

                    RtlAcquireLockRoutine( Heap->LockVariable );

                    LockAcquired = TRUE;

                    Flags |= HEAP_NO_SERIALIZE;
                }

                if (RtlpValidateHeap( Heap, FALSE )) {

                    TagIndex = RtlCreateTagHeap( HeapHandle, Flags, TagPrefix, TagNames );
                }

                RtlpValidateHeapHeaders( Heap, TRUE );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return TagIndex;
}


NTSYSAPI
PWSTR
NTAPI
RtlDebugQueryTagHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN USHORT TagIndex,
    IN BOOLEAN ResetCounters,
    OUT PRTL_HEAP_TAG_INFO TagInfo OPTIONAL
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN LockAcquired = FALSE;
    PWSTR Result;

    Result = NULL;

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (RtlpCheckHeapSignature( Heap, "RtlQueryTagHeap" )) {

                Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

                //
                //  Lock the heap
                //

                if (!(Flags & HEAP_NO_SERIALIZE)) {

                    RtlAcquireLockRoutine( Heap->LockVariable );

                    LockAcquired = TRUE;

                    Flags |= HEAP_NO_SERIALIZE;
                }

                if (RtlpValidateHeap( Heap, FALSE )) {

                    Result = RtlQueryTagHeap( HeapHandle, Flags, TagIndex, ResetCounters, TagInfo );
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            SET_LAST_STATUS( GetExceptionCode() );
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Result;
}


NTSTATUS
RtlDebugUsageHeap (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN OUT PRTL_HEAP_USAGE Usage
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    NTSTATUS Status;
    BOOLEAN LockAcquired = FALSE;

    IF_DEBUG_PAGE_HEAP_THEN_RETURN( HeapHandle,
                                    RtlpDebugPageHeapUsage( HeapHandle, Flags, Usage ));

    Status = STATUS_SUCCESS;

    try {

        try {

            //
            //  Validate that HeapAddress points to a HEAP structure.
            //

            if (!RtlpCheckHeapSignature( Heap, "RtlUsageHeap" )) {

                Status = STATUS_INVALID_PARAMETER;
                leave;
            }

            Flags |= Heap->ForceFlags | HEAP_SKIP_VALIDATION_CHECKS;

            //
            //  Lock the heap
            //

            if (!(Flags & HEAP_NO_SERIALIZE)) {

                RtlAcquireLockRoutine( Heap->LockVariable );

                LockAcquired = TRUE;

                Flags |= HEAP_NO_SERIALIZE;
            }

            if (!RtlpValidateHeap( Heap, FALSE )) {

                Status = STATUS_INVALID_PARAMETER;

            } else {

                Status = RtlUsageHeap( HeapHandle, Flags, Usage );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

    } finally {

        if (LockAcquired) {

            RtlReleaseLockRoutine( Heap->LockVariable );
        }
    }

    return Status;
}


BOOLEAN
RtlDebugWalkHeap (
    IN PVOID HeapHandle,
    IN OUT PRTL_HEAP_WALK_ENTRY Entry
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP Heap = (PHEAP)HeapHandle;
    BOOLEAN Result;

    //
    //  Assumed the caller has serialized via RtlLockHeap or their own locking mechanism.
    //

    Result = FALSE;

    try {

        if (RtlpCheckHeapSignature( Heap, "RtlWalkHeap" )) {

            Result = RtlpValidateHeap( Heap, FALSE );
        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        SET_LAST_STATUS( GetExceptionCode() );
    }

    return Result;
}


BOOLEAN
RtlpValidateHeapEntry (
    IN PHEAP Heap,
    IN PHEAP_ENTRY BusyBlock,
    IN PCHAR Reason
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP_SEGMENT Segment;
    UCHAR SegmentIndex;
    BOOLEAN Result;

    if ((BusyBlock == NULL)

            ||

        ((ULONG_PTR)BusyBlock & (HEAP_GRANULARITY-1))

            ||

        ((BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
         ((ULONG_PTR)BusyBlock & (PAGE_SIZE-1)) != FIELD_OFFSET( HEAP_VIRTUAL_ALLOC_ENTRY, BusyBlock ))

            ||

        (!(BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) &&
         ((BusyBlock->SegmentIndex >= HEAP_MAXIMUM_SEGMENTS) ||
          !(Segment = Heap->Segments[ BusyBlock->SegmentIndex ]) ||
          (BusyBlock < Segment->FirstEntry) ||
          (BusyBlock >= Segment->LastValidEntry)))

            ||

        !(BusyBlock->Flags & HEAP_ENTRY_BUSY)

            ||

        ((BusyBlock->Flags & HEAP_ENTRY_FILL_PATTERN) && !RtlpCheckBusyBlockTail( BusyBlock ))) {

InvalidBlock:

        HeapDebugPrint(( "Invalid Address specified to %s( %lx, %lx )\n",
                         Reason,
                         Heap,
                         BusyBlock + 1 ));

        HeapDebugBreak( BusyBlock );

        return FALSE;

    } else {

        if (BusyBlock->Flags & HEAP_ENTRY_VIRTUAL_ALLOC) {

            Result = TRUE;

        } else {

            for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

                Segment = Heap->Segments[ SegmentIndex ];

                if (Segment) {

                    if ((BusyBlock >= Segment->FirstEntry) &&
                        (BusyBlock < Segment->LastValidEntry)) {

                        Result = TRUE;
                        break;
                    }
                }
            }
        }

        if (!Result) {

            goto InvalidBlock;
        }

        return TRUE;
    }
}


BOOLEAN
RtlpValidateHeapSegment (
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN UCHAR SegmentIndex,
    IN OUT PULONG CountOfFreeBlocks,
    IN OUT PSIZE_T TotalFreeSize,
    OUT PVOID *BadAddress,
    IN OUT PSIZE_T ComputedTagEntries,
    IN OUT PSIZE_T ComputedPseudoTagEntries
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    PHEAP_ENTRY CurrentBlock, PreviousBlock;
    SIZE_T Size;
    USHORT PreviousSize, TagIndex;
    PHEAP_UNCOMMMTTED_RANGE UnCommittedRange;
    PHEAP_ENTRY_EXTRA ExtraStuff;
    ULONG NumberOfUnCommittedPages;
    ULONG NumberOfUnCommittedRanges;

    RTL_PAGED_CODE();

    NumberOfUnCommittedPages = 0;
    NumberOfUnCommittedRanges = 0;

    UnCommittedRange = Segment->UnCommittedRanges;

    if (Segment->BaseAddress == Heap) {

        CurrentBlock = &Heap->Entry;

    } else {

        CurrentBlock = &Segment->Entry;
    }

    while (CurrentBlock < Segment->LastValidEntry) {

        *BadAddress = CurrentBlock;

        if ((UnCommittedRange != NULL) &&
            ((ULONG_PTR)CurrentBlock >= UnCommittedRange->Address)) {

            HeapDebugPrint(( "Heap entry %lx is beyond uncommited range [%x .. %x)\n",
                             CurrentBlock,
                             UnCommittedRange->Address,
                             (PCHAR)UnCommittedRange->Address + UnCommittedRange->Size ));

            return FALSE;
        }

        PreviousSize = 0;

        while (CurrentBlock < Segment->LastValidEntry) {

            *BadAddress = CurrentBlock;

            if (PreviousSize != CurrentBlock->PreviousSize) {

                HeapDebugPrint(( "Heap entry %lx has incorrect PreviousSize field (%04x instead of %04x)\n",
                                 CurrentBlock, CurrentBlock->PreviousSize, PreviousSize ));

                return FALSE;
            }

            PreviousSize = CurrentBlock->Size;
            Size = (ULONG_PTR)CurrentBlock->Size << HEAP_GRANULARITY_SHIFT;

            if (CurrentBlock->Flags & HEAP_ENTRY_BUSY) {

                if (ComputedTagEntries != NULL) {

                    if (CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) {

                        ExtraStuff = RtlpGetExtraStuffPointer( CurrentBlock );
                        TagIndex = ExtraStuff->TagIndex;

                    } else {

                        TagIndex = CurrentBlock->SmallTagIndex;
                    }

                    if (TagIndex != 0) {

                        if (TagIndex & HEAP_PSEUDO_TAG_FLAG) {

                            TagIndex &= ~HEAP_PSEUDO_TAG_FLAG;

                            if (TagIndex < HEAP_NUMBER_OF_PSEUDO_TAG) {

                                ComputedPseudoTagEntries[ TagIndex ] += CurrentBlock->Size;
                            }

                        } else if (TagIndex & HEAP_GLOBAL_TAG) {

                            //
                            //  Ignore these since they are global across more than
                            //  one heap.
                            //

                        } else if (TagIndex < Heap->NextAvailableTagIndex) {

                            ComputedTagEntries[ TagIndex ] += CurrentBlock->Size;
                        }
                    }
                }

                if (CurrentBlock->Flags & HEAP_ENTRY_FILL_PATTERN) {

                    if (!RtlpCheckBusyBlockTail( CurrentBlock )) {

                        return FALSE;
                    }
                }

            } else {

                *CountOfFreeBlocks += 1;
                *TotalFreeSize += CurrentBlock->Size;

                if ((Heap->Flags & HEAP_FREE_CHECKING_ENABLED) &&
                    (CurrentBlock->Flags & HEAP_ENTRY_FILL_PATTERN)) {

                    SIZE_T cb, cbEqual;

                    cb = Size - sizeof( HEAP_FREE_ENTRY );

                    if ((CurrentBlock->Flags & HEAP_ENTRY_EXTRA_PRESENT) &&
                        (cb > sizeof( HEAP_FREE_ENTRY_EXTRA ))) {

                        cb -= sizeof( HEAP_FREE_ENTRY_EXTRA );
                    }

                    cbEqual = RtlCompareMemoryUlong( (PCHAR)((PHEAP_FREE_ENTRY)CurrentBlock + 1),
                                                     cb,
                                                     FREE_HEAP_FILL );

                    if (cbEqual != cb) {

                        HeapDebugPrint(( "Free Heap block %lx modified at %lx after it was freed\n",
                                         CurrentBlock,
                                         (PCHAR)(CurrentBlock + 1) + cbEqual ));

                        return FALSE;
                    }
                }
            }

            if (CurrentBlock->SegmentIndex != SegmentIndex) {

                HeapDebugPrint(( "Heap block at %lx has incorrect segment index (%x)\n",
                                 CurrentBlock,
                                 SegmentIndex ));

                return FALSE;
            }

            if (CurrentBlock->Flags & HEAP_ENTRY_LAST_ENTRY) {

                CurrentBlock = (PHEAP_ENTRY)((PCHAR)CurrentBlock + Size);

                if (UnCommittedRange == NULL) {

                    if (CurrentBlock != Segment->LastValidEntry) {

                        HeapDebugPrint(( "Heap block at %lx is not last block in segment (%x)\n",
                                         CurrentBlock,
                                         Segment->LastValidEntry ));

                        return FALSE;
                    }

                } else if ((ULONG_PTR)CurrentBlock != UnCommittedRange->Address) {

                    HeapDebugPrint(( "Heap block at %lx does not match address of next uncommitted address (%x)\n",
                                     CurrentBlock,
                                     UnCommittedRange->Address ));

                    return FALSE;

                } else {

                    NumberOfUnCommittedPages += (ULONG) (UnCommittedRange->Size / PAGE_SIZE);
                    NumberOfUnCommittedRanges += 1;

                    CurrentBlock = (PHEAP_ENTRY)
                        ((PCHAR)UnCommittedRange->Address + UnCommittedRange->Size);

                    UnCommittedRange = UnCommittedRange->Next;
                }

                break;
            }

            CurrentBlock = (PHEAP_ENTRY)((PCHAR)CurrentBlock + Size);
        }
    }

    *BadAddress = Segment;

    if (Segment->NumberOfUnCommittedPages != NumberOfUnCommittedPages) {

        HeapDebugPrint(( "Heap Segment at %lx contains invalid NumberOfUnCommittedPages (%x != %x)\n",
                         Segment,
                         Segment->NumberOfUnCommittedPages,
                         NumberOfUnCommittedPages ));

        return FALSE;
    }

    if (Segment->NumberOfUnCommittedRanges != NumberOfUnCommittedRanges) {

        HeapDebugPrint(( "Heap Segment at %lx contains invalid NumberOfUnCommittedRanges (%x != %x)\n",
                         Segment,
                         Segment->NumberOfUnCommittedRanges,
                         NumberOfUnCommittedRanges ));

        return FALSE;
    }

    return TRUE;
}


BOOLEAN
RtlpValidateHeap (
    IN PHEAP Heap,
    IN BOOLEAN AlwaysValidate
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    NTSTATUS Status;
    PHEAP_SEGMENT Segment;
    PLIST_ENTRY Head, Next;
    PHEAP_FREE_ENTRY FreeBlock;
    BOOLEAN EmptyFreeList;
    ULONG NumberOfFreeListEntries;
    ULONG CountOfFreeBlocks;
    SIZE_T TotalFreeSize;
    SIZE_T Size;
    USHORT PreviousSize;
    UCHAR SegmentIndex;
    PVOID BadAddress;
    PSIZE_T ComputedTagEntries = NULL;
    PSIZE_T ComputedPseudoTagEntries = NULL;
    PHEAP_VIRTUAL_ALLOC_ENTRY VirtualAllocBlock;
    USHORT TagIndex;

    RTL_PAGED_CODE();

    BadAddress = Heap;

    if (!RtlpValidateHeapHeaders( Heap, FALSE )) {

        goto errorExit;
    }

    if (!AlwaysValidate && !(Heap->Flags & HEAP_VALIDATE_ALL_ENABLED)) {

        goto exit;
    }

    NumberOfFreeListEntries = 0;
    Head = &Heap->FreeLists[ 0 ];

    for (Size = 0; Size < HEAP_MAXIMUM_FREELISTS; Size++) {

        if (Size != 0) {

            EmptyFreeList = (BOOLEAN)(IsListEmpty( Head ));
            BadAddress = &Heap->u.FreeListsInUseBytes[ Size / 8 ];

            if (Heap->u.FreeListsInUseBytes[ Size / 8 ] & (1 << (Size & 7)) ) {

                if (EmptyFreeList) {

                    HeapDebugPrint(( "dedicated (%04x) free list empty but marked as non-empty\n",
                                     Size ));

                    goto errorExit;
                }

            } else {

                if (!EmptyFreeList) {

                    HeapDebugPrint(( "dedicated (%04x) free list non-empty but marked as empty\n",
                                     Size ));

                    goto errorExit;
                }
            }
        }

        Next = Head->Flink;
        PreviousSize = 0;

        while (Head != Next) {

            FreeBlock = CONTAINING_RECORD( Next, HEAP_FREE_ENTRY, FreeList );
            Next = Next->Flink;

            BadAddress = FreeBlock;

            if (FreeBlock->Flags & HEAP_ENTRY_BUSY) {

                HeapDebugPrint(( "dedicated (%04x) free list element %lx is marked busy\n",
                                 Size,
                                 FreeBlock ));

                goto errorExit;
            }

            if ((Size != 0) && (FreeBlock->Size != Size)) {

                HeapDebugPrint(( "Dedicated (%04x) free list element %lx is wrong size (%04x)\n",
                                 Size,
                                 FreeBlock,
                                 FreeBlock->Size ));

                goto errorExit;

            } else if ((Size == 0) && (FreeBlock->Size < HEAP_MAXIMUM_FREELISTS)) {

                HeapDebugPrint(( "Non-Dedicated free list element %lx with too small size (%04x)\n",
                                 FreeBlock,
                                 FreeBlock->Size ));

                goto errorExit;

            } else if ((Size == 0) && (FreeBlock->Size < PreviousSize)) {

                HeapDebugPrint(( "Non-Dedicated free list element %lx is out of order\n",
                                 FreeBlock ));

                goto errorExit;

            } else {

                PreviousSize = FreeBlock->Size;
            }

            NumberOfFreeListEntries++;
        }

        Head++;
    }

    Size = (HEAP_NUMBER_OF_PSEUDO_TAG + Heap->NextAvailableTagIndex + 1) * sizeof( SIZE_T );

    if ((RtlpValidateHeapTagsEnable) && (Heap->PseudoTagEntries != NULL)) {

        Status = NtAllocateVirtualMemory( NtCurrentProcess(),
                                          &ComputedPseudoTagEntries,
                                          0,
                                          &Size,
                                          MEM_COMMIT,
                                          PAGE_READWRITE );

        if (NT_SUCCESS( Status )) {

            ComputedTagEntries = ComputedPseudoTagEntries + HEAP_NUMBER_OF_PSEUDO_TAG;
        }
    }

    Head = &Heap->VirtualAllocdBlocks;
    Next = Head->Flink;

    while (Head != Next) {

        VirtualAllocBlock = CONTAINING_RECORD( Next, HEAP_VIRTUAL_ALLOC_ENTRY, Entry );

        if (ComputedTagEntries != NULL) {

            TagIndex = VirtualAllocBlock->ExtraStuff.TagIndex;

            if (TagIndex != 0) {

                if (TagIndex & HEAP_PSEUDO_TAG_FLAG) {

                    TagIndex &= ~HEAP_PSEUDO_TAG_FLAG;

                    if (TagIndex < HEAP_NUMBER_OF_PSEUDO_TAG) {

                        ComputedPseudoTagEntries[ TagIndex ] +=
                            VirtualAllocBlock->CommitSize >> HEAP_GRANULARITY_SHIFT;
                    }

                } else if (TagIndex & HEAP_GLOBAL_TAG) {

                    //
                    //  Ignore these since they are global across more than
                    //  one heap.
                    //

                } else if (TagIndex < Heap->NextAvailableTagIndex) {

                    ComputedTagEntries[ TagIndex ] +=
                        VirtualAllocBlock->CommitSize >> HEAP_GRANULARITY_SHIFT;
                }
            }
        }

        if (VirtualAllocBlock->BusyBlock.Flags & HEAP_ENTRY_FILL_PATTERN) {

            if (!RtlpCheckBusyBlockTail( &VirtualAllocBlock->BusyBlock )) {

                return FALSE;
            }
        }

        Next = Next->Flink;
    }

    CountOfFreeBlocks = 0;
    TotalFreeSize = 0;

    for (SegmentIndex=0; SegmentIndex<HEAP_MAXIMUM_SEGMENTS; SegmentIndex++) {

        Segment = Heap->Segments[ SegmentIndex ];

        if (Segment) {

            if (!RtlpValidateHeapSegment( Heap,
                                          Segment,
                                          SegmentIndex,
                                          &CountOfFreeBlocks,
                                          &TotalFreeSize,
                                          &BadAddress,
                                          ComputedTagEntries,
                                          ComputedPseudoTagEntries )) {

                goto errorExit;
            }
        }
    }

    BadAddress = Heap;

    if (NumberOfFreeListEntries != CountOfFreeBlocks) {

        HeapDebugPrint(( "Number of free blocks in arena (%ld) does not match number in the free lists (%ld)\n",
                         CountOfFreeBlocks,
                         NumberOfFreeListEntries ));

        goto errorExit;
    }

    if (Heap->TotalFreeSize != TotalFreeSize) {

        HeapDebugPrint(( "Total size of free blocks in arena (%ld) does not match number total in heap header (%ld)\n",
                         TotalFreeSize,
                         Heap->TotalFreeSize ));

        goto errorExit;
    }

    if (ComputedPseudoTagEntries != NULL) {

        PHEAP_PSEUDO_TAG_ENTRY PseudoTagEntries;
        PHEAP_TAG_ENTRY TagEntries;
        USHORT TagIndex;

        PseudoTagEntries = Heap->PseudoTagEntries;

        if (PseudoTagEntries != NULL) {

            for (TagIndex=1; TagIndex<HEAP_NUMBER_OF_PSEUDO_TAG; TagIndex++) {

                PseudoTagEntries += 1;

                if (ComputedPseudoTagEntries[ TagIndex ] != PseudoTagEntries->Size) {

                    HeapDebugPrint(( "Pseudo Tag %04x size incorrect (%x != %x) %x\n",
                                     TagIndex,
                                     PseudoTagEntries->Size,
                                     ComputedPseudoTagEntries[ TagIndex ]
                                     &ComputedPseudoTagEntries[ TagIndex ] ));

                    goto errorExit;
                }
            }
        }

        TagEntries = Heap->TagEntries;

        if (TagEntries != NULL) {

            for (TagIndex=1; TagIndex<Heap->NextAvailableTagIndex; TagIndex++) {

                TagEntries += 1;

                if (ComputedTagEntries[ TagIndex ] != TagEntries->Size) {

                    HeapDebugPrint(( "Tag %04x (%ws) size incorrect (%x != %x) %x\n",
                                     TagIndex,
                                     TagEntries->TagName,
                                     TagEntries->Size,
                                     ComputedTagEntries[ TagIndex ],
                                     &ComputedTagEntries[ TagIndex ] ));

                    goto errorExit;
                }
            }
        }

        Size = 0;

        NtFreeVirtualMemory( NtCurrentProcess(),
                             &ComputedPseudoTagEntries,
                             &Size,
                             MEM_RELEASE );
    }

exit:

    return TRUE;

errorExit:

    HeapDebugBreak( BadAddress );

    if (ComputedPseudoTagEntries != NULL) {

        Size = 0;

        NtFreeVirtualMemory( NtCurrentProcess(),
                             &ComputedPseudoTagEntries,
                             &Size,
                             MEM_RELEASE );
    }

    return FALSE;

}


BOOLEAN RtlpHeapInvalidBreakPoint;
PVOID RtlpHeapInvalidBadAddress;

VOID
RtlpBreakPointHeap (
    IN PVOID BadAddress
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    if (NtCurrentPeb()->BeingDebugged) {

        *(BOOLEAN volatile *)&RtlpHeapInvalidBreakPoint = TRUE;

        RtlpHeapInvalidBadAddress = BadAddress;

        DbgBreakPoint();

        *(BOOLEAN volatile *)&RtlpHeapInvalidBreakPoint = FALSE;
    }
}
