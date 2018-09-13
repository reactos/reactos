/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    heappriv.h

Abstract:

    Private include file used by heap allocator (heap.c, heapdll.c and
    heapdbg.c)

Author:

    Steve Wood (stevewo) 25-Oct-1994

Revision History:

--*/

#ifndef _RTL_HEAP_PRIVATE_
#define _RTL_HEAP_PRIVATE_

#include "heappage.h"

//
//  Disable FPO optimization so even retail builds get somewhat reasonable
//  stack backtraces
//

#if i386
// #pragma optimize("y",off)
#endif

#if DBG
#define HEAPASSERT(exp) if (!(exp)) RtlAssert( #exp, __FILE__, __LINE__, NULL )
#else
#define HEAPASSERT(exp)
#endif

//
//  This variable contains the fill pattern used for heap tail checking
//

UCHAR CheckHeapFillPattern[ CHECK_HEAP_TAIL_SIZE ];


//
//  Here are the locking routines for the heap (kernel and user)
//

#ifdef NTOS_KERNEL_RUNTIME

//
//  Kernel mode heap uses the kernel resource package for locking
//

#define RtlInitializeLockRoutine(L) ExInitializeResource((PERESOURCE)(L))
#define RtlAcquireLockRoutine(L)    ExAcquireResourceExclusive((PERESOURCE)(L),TRUE)
#define RtlReleaseLockRoutine(L)    ExReleaseResource((PERESOURCE)(L))
#define RtlDeleteLockRoutine(L)     ExDeleteResource((PERESOURCE)(L))
#define RtlOkayToLockRoutine(L)     ExOkayToLockRoutine((PERESOURCE)(L))

#else // #ifdef NTOS_KERNEL_ROUTINE

//
//  User mode heap uses the critical section package for locking
//

#ifndef PREALLOCATE_EVENT_MASK

#define PREALLOCATE_EVENT_MASK  0x80000000  // **** defined only in dll\resource.c

#endif // PREALLOCATE_EVENT_MASK

#define RtlInitializeLockRoutine(L) RtlInitializeCriticalSectionAndSpinCount((PRTL_CRITICAL_SECTION)(L),(PREALLOCATE_EVENT_MASK | 4000))
#define RtlAcquireLockRoutine(L)    RtlEnterCriticalSection((PRTL_CRITICAL_SECTION)(L))
#define RtlReleaseLockRoutine(L)    RtlLeaveCriticalSection((PRTL_CRITICAL_SECTION)(L))
#define RtlDeleteLockRoutine(L)     RtlDeleteCriticalSection((PRTL_CRITICAL_SECTION)(L))
#define RtlOkayToLockRoutine(L)     NtdllOkayToLockRoutine((PVOID)(L))

#endif // #ifdef NTOS_KERNEL_RUNTIME


//
//  Here are some debugging macros for the heap
//

#ifdef NTOS_KERNEL_RUNTIME

#define HEAP_DEBUG_FLAGS   0
#define DEBUG_HEAP(F)      FALSE
#define SET_LAST_STATUS(S) NOTHING;

#else // #ifdef NTOS_KERNEL_ROUTINE

#define HEAP_DEBUG_FLAGS   (HEAP_VALIDATE_PARAMETERS_ENABLED | \
                            HEAP_VALIDATE_ALL_ENABLED        | \
                            HEAP_CAPTURE_STACK_BACKTRACES    | \
                            HEAP_CREATE_ENABLE_TRACING       | \
                            HEAP_FLAG_PAGE_ALLOCS)
#define DEBUG_HEAP(F)      ((F & HEAP_DEBUG_FLAGS) && !(F & HEAP_SKIP_VALIDATION_CHECKS))
#define SET_LAST_STATUS(S) {NtCurrentTeb()->LastErrorValue = RtlNtStatusToDosError( NtCurrentTeb()->LastStatusValue = (ULONG)(S) );}

#endif // #ifdef NTOS_KERNEL_RUNTIME


//
//  Here are the macros used for debug printing and breakpoints
//

#ifdef NTOS_KERNEL_RUNTIME

#define HeapDebugPrint( _x_ ) {DbgPrint _x_;}

#define HeapDebugBreak( _x_ ) {if (KdDebuggerEnabled) DbgBreakPoint();}

#else // #ifdef NTOS_KERNEL_ROUTINE

#define HeapDebugPrint( _x_ )                                   \
{                                                               \
    PLIST_ENTRY _Module;                                        \
    PLDR_DATA_TABLE_ENTRY _Entry;                               \
                                                                \
    _Module = NtCurrentPeb()->Ldr->InLoadOrderModuleList.Flink; \
    _Entry = CONTAINING_RECORD( _Module,                        \
                                LDR_DATA_TABLE_ENTRY,           \
                                InLoadOrderLinks);              \
    DbgPrint("HEAP[%wZ]: ", &_Entry->BaseDllName);              \
    DbgPrint _x_;                                               \
}

#define HeapDebugBreak( _x_ )                    \
{                                                \
    VOID RtlpBreakPointHeap( PVOID BadAddress ); \
                                                 \
    RtlpBreakPointHeap( (_x_) );                 \
}

#endif // #ifdef NTOS_KERNEL_RUNTIME


//
//  Implemented in heap.c
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
    );

PHEAP_FREE_ENTRY
RtlpCoalesceFreeBlocks (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN OUT PSIZE_T FreeSize,
    IN BOOLEAN RemoveFromFreeList
    );

VOID
RtlpDeCommitFreeBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN SIZE_T FreeSize
    );

VOID
RtlpInsertFreeBlock (
    IN PHEAP Heap,
    IN PHEAP_FREE_ENTRY FreeBlock,
    IN SIZE_T FreeSize
    );

PHEAP_FREE_ENTRY
RtlpFindAndCommitPages (
    IN PHEAP Heap,
    IN PHEAP_SEGMENT Segment,
    IN OUT PSIZE_T Size,
    IN PVOID AddressWanted OPTIONAL
    );

PVOID
RtlAllocateHeapSlowly (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN SIZE_T Size
    );

BOOLEAN
RtlFreeHeapSlowly (
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

SIZE_T
RtlpGetSizeOfBigBlock (
    IN PHEAP_ENTRY BusyBlock
    );

PHEAP_ENTRY_EXTRA
RtlpGetExtraStuffPointer (
    PHEAP_ENTRY BusyBlock
    );

BOOLEAN
RtlpCheckBusyBlockTail (
    IN PHEAP_ENTRY BusyBlock
    );


//
//  Implemented in heapdll.c
//

VOID
RtlpAddHeapToProcessList (
    IN PHEAP Heap
    );

VOID
RtlpRemoveHeapFromProcessList (
    IN PHEAP Heap
    );

PHEAP_FREE_ENTRY
RtlpCoalesceHeap (
    IN PHEAP Heap
    );

BOOLEAN
RtlpCheckHeapSignature (
    IN PHEAP Heap,
    IN PCHAR Caller
    );


//
//  Implemented in heapdbg.c
//

BOOLEAN
RtlpValidateHeapEntry (
    IN PHEAP Heap,
    IN PHEAP_ENTRY BusyBlock,
    IN PCHAR Reason
    );

BOOLEAN
RtlpValidateHeap (
    IN PHEAP Heap,
    IN BOOLEAN AlwaysValidate
    );

VOID
RtlpUpdateHeapListIndex (
    USHORT OldIndex,
    USHORT NewIndex
    );

BOOLEAN
RtlpValidateHeapHeaders(
    IN PHEAP Heap,
    IN BOOLEAN Recompute
    );


//
//  Macro for setting a bit in the freelist vector to indicate entries are
//  present.
//

#define SET_FREELIST_BIT( H, FB )                                     \
{                                                                     \
    ULONG _Index_;                                                    \
    ULONG _Bit_;                                                      \
                                                                      \
    HEAPASSERT((FB)->Size < HEAP_MAXIMUM_FREELISTS);                  \
                                                                      \
    _Index_ = (FB)->Size >> 3;                                        \
    _Bit_ = (1 << ((FB)->Size & 7));                                  \
                                                                      \
    HEAPASSERT(((H)->u.FreeListsInUseBytes[ _Index_ ] & _Bit_) == 0); \
                                                                      \
    (H)->u.FreeListsInUseBytes[ _Index_ ] |= _Bit_;                   \
}

//
//  Macro for clearing a bit in the freelist vector to indicate entries are
//  not present.
//

#define CLEAR_FREELIST_BIT( H, FB )                            \
{                                                              \
    ULONG _Index_;                                             \
    ULONG _Bit_;                                               \
                                                               \
    HEAPASSERT((FB)->Size < HEAP_MAXIMUM_FREELISTS);           \
                                                               \
    _Index_ = (FB)->Size >> 3;                                 \
    _Bit_ = (1 << ((FB)->Size & 7));                           \
                                                               \
    HEAPASSERT((H)->u.FreeListsInUseBytes[ _Index_ ] & _Bit_); \
    HEAPASSERT(IsListEmpty(&(H)->FreeLists[ (FB)->Size ]));    \
                                                               \
    (H)->u.FreeListsInUseBytes[ _Index_ ] ^= _Bit_;            \
}


//
//  This macro inserts a free block into the appropriate free list including
//  the [0] index list with entry filling if necessary
//

#define RtlpInsertFreeBlockDirect( H, FB, SIZE )                          \
{                                                                         \
    PLIST_ENTRY _HEAD, _NEXT;                                             \
    PHEAP_FREE_ENTRY _FB1;                                                \
                                                                          \
    HEAPASSERT((FB)->Size == (SIZE));                                     \
    (FB)->Flags &= ~(HEAP_ENTRY_FILL_PATTERN |                            \
                     HEAP_ENTRY_EXTRA_PRESENT |                           \
                     HEAP_ENTRY_BUSY);                                    \
                                                                          \
    if ((H)->Flags & HEAP_FREE_CHECKING_ENABLED) {                        \
                                                                          \
        RtlFillMemoryUlong( (PCHAR)((FB) + 1),                            \
                            ((SIZE) << HEAP_GRANULARITY_SHIFT) -          \
                                sizeof( *(FB) ),                          \
                            FREE_HEAP_FILL );                             \
                                                                          \
        (FB)->Flags |= HEAP_ENTRY_FILL_PATTERN;                           \
    }                                                                     \
                                                                          \
    if ((SIZE) < HEAP_MAXIMUM_FREELISTS) {                                \
                                                                          \
        _HEAD = &(H)->FreeLists[ (SIZE) ];                                \
                                                                          \
        if (IsListEmpty(_HEAD)) {                                         \
                                                                          \
            SET_FREELIST_BIT( H, FB );                                    \
        }                                                                 \
                                                                          \
    } else {                                                              \
                                                                          \
        _HEAD = &(H)->FreeLists[ 0 ];                                     \
        _NEXT = _HEAD->Flink;                                             \
                                                                          \
        while (_HEAD != _NEXT) {                                          \
                                                                          \
            _FB1 = CONTAINING_RECORD( _NEXT, HEAP_FREE_ENTRY, FreeList ); \
                                                                          \
            if ((SIZE) <= _FB1->Size) {                                   \
                                                                          \
                break;                                                    \
                                                                          \
            } else {                                                      \
                                                                          \
                _NEXT = _NEXT->Flink;                                     \
            }                                                             \
        }                                                                 \
                                                                          \
        _HEAD = _NEXT;                                                    \
    }                                                                     \
                                                                          \
    InsertTailList( _HEAD, &(FB)->FreeList );                             \
}

//
//  This version of RtlpInsertFreeBlockDirect does no filling.
//

#define RtlpFastInsertFreeBlockDirect( H, FB, SIZE )              \
{                                                                 \
    if ((SIZE) < HEAP_MAXIMUM_FREELISTS) {                        \
                                                                  \
        RtlpFastInsertDedicatedFreeBlockDirect( H, FB, SIZE );    \
                                                                  \
    } else {                                                      \
                                                                  \
        RtlpFastInsertNonDedicatedFreeBlockDirect( H, FB, SIZE ); \
    }                                                             \
}

//
//  This version of RtlpInsertFreeBlockDirect only works for dedicated free
//  lists and doesn't do any filling.
//

#define RtlpFastInsertDedicatedFreeBlockDirect( H, FB, SIZE )             \
{                                                                         \
    PLIST_ENTRY _HEAD;                                                    \
                                                                          \
    HEAPASSERT((FB)->Size == (SIZE));                                     \
                                                                          \
    if (!((FB)->Flags & HEAP_ENTRY_LAST_ENTRY)) {                         \
                                                                          \
        HEAPASSERT(((PHEAP_ENTRY)(FB) + (SIZE))->PreviousSize == (SIZE)); \
    }                                                                     \
                                                                          \
    (FB)->Flags &= HEAP_ENTRY_LAST_ENTRY;                                 \
                                                                          \
    _HEAD = &(H)->FreeLists[ (SIZE) ];                                    \
                                                                          \
    if (IsListEmpty(_HEAD)) {                                             \
                                                                          \
        SET_FREELIST_BIT( H, FB );                                        \
    }                                                                     \
                                                                          \
    InsertTailList( _HEAD, &(FB)->FreeList );                             \
}

//
//  This version of RtlpInsertFreeBlockDirect only works for nondedicated free
//  lists and doesn't do any filling.
//

#define RtlpFastInsertNonDedicatedFreeBlockDirect( H, FB, SIZE )          \
{                                                                         \
    PLIST_ENTRY _HEAD, _NEXT;                                             \
    PHEAP_FREE_ENTRY _FB1;                                                \
                                                                          \
    HEAPASSERT((FB)->Size == (SIZE));                                     \
                                                                          \
    if (!((FB)->Flags & HEAP_ENTRY_LAST_ENTRY)) {                         \
                                                                          \
        HEAPASSERT(((PHEAP_ENTRY)(FB) + (SIZE))->PreviousSize == (SIZE)); \
    }                                                                     \
                                                                          \
    (FB)->Flags &= (HEAP_ENTRY_LAST_ENTRY);                               \
                                                                          \
    _HEAD = &(H)->FreeLists[ 0 ];                                         \
    _NEXT = _HEAD->Flink;                                                 \
                                                                          \
    while (_HEAD != _NEXT) {                                              \
                                                                          \
        _FB1 = CONTAINING_RECORD( _NEXT, HEAP_FREE_ENTRY, FreeList );     \
                                                                          \
        if ((SIZE) <= _FB1->Size) {                                       \
                                                                          \
            break;                                                        \
                                                                          \
        } else {                                                          \
                                                                          \
            _NEXT = _NEXT->Flink;                                         \
        }                                                                 \
    }                                                                     \
                                                                          \
    InsertTailList( _NEXT, &(FB)->FreeList );                             \
}


//
//  This macro removes a block from its free list with fill checking if
//  necessary
//

#define RtlpRemoveFreeBlock( H, FB )                                              \
{                                                                                 \
    RtlpFastRemoveFreeBlock( H, FB )                                              \
                                                                                  \
    if ((FB)->Flags & HEAP_ENTRY_FILL_PATTERN) {                                  \
                                                                                  \
        SIZE_T cb, cbEqual;                                                       \
        PVOID p;                                                                  \
                                                                                  \
        cb = ((FB)->Size << HEAP_GRANULARITY_SHIFT) - sizeof( *(FB) );            \
                                                                                  \
        if ((FB)->Flags & HEAP_ENTRY_EXTRA_PRESENT &&                             \
            cb > sizeof( HEAP_FREE_ENTRY_EXTRA )) {                               \
                                                                                  \
            cb -= sizeof( HEAP_FREE_ENTRY_EXTRA );                                \
        }                                                                         \
                                                                                  \
        cbEqual = RtlCompareMemoryUlong( (PCHAR)((FB) + 1),                       \
                                                 cb,                              \
                                                 FREE_HEAP_FILL );                \
                                                                                  \
        if (cbEqual != cb) {                                                      \
                                                                                  \
            HeapDebugPrint((                                                      \
                "HEAP: Free Heap block %lx modified at %lx after it was freed\n", \
                (FB),                                                             \
                (PCHAR)((FB) + 1) + cbEqual ));                                   \
                                                                                  \
            HeapDebugBreak((FB));                                                 \
        }                                                                         \
    }                                                                             \
}

//
//  This version of RtlpRemoveFreeBlock does no fill checking
//

#define RtlpFastRemoveFreeBlock( H, FB )         \
{                                                \
    PLIST_ENTRY _EX_Blink;                       \
    PLIST_ENTRY _EX_Flink;                       \
                                                 \
    _EX_Flink = (FB)->FreeList.Flink;            \
    _EX_Blink = (FB)->FreeList.Blink;            \
                                                 \
    _EX_Blink->Flink = _EX_Flink;                \
    _EX_Flink->Blink = _EX_Blink;                \
                                                 \
    if ((_EX_Flink == _EX_Blink) &&              \
        ((FB)->Size < HEAP_MAXIMUM_FREELISTS)) { \
                                                 \
        CLEAR_FREELIST_BIT( H, FB );             \
    }                                            \
}

//
//  This version of RtlpRemoveFreeBlock only works for dedicated free lists
//  (where we know that (FB)->Mask != 0) and doesn't do any fill checking
//

#define RtlpFastRemoveDedicatedFreeBlock( H, FB ) \
{                                                 \
    PLIST_ENTRY _EX_Blink;                        \
    PLIST_ENTRY _EX_Flink;                        \
                                                  \
    _EX_Flink = (FB)->FreeList.Flink;             \
    _EX_Blink = (FB)->FreeList.Blink;             \
                                                  \
    _EX_Blink->Flink = _EX_Flink;                 \
    _EX_Flink->Blink = _EX_Blink;                 \
                                                  \
    if (_EX_Flink == _EX_Blink) {                 \
                                                  \
        CLEAR_FREELIST_BIT( H, FB );              \
    }                                             \
}

//
//  This version of RtlpRemoveFreeBlock only works for dedicated free lists
//  (where we know that (FB)->Mask == 0) and doesn't do any fill checking
//

#define RtlpFastRemoveNonDedicatedFreeBlock( H, FB ) \
{                                                    \
    RemoveEntryList(&(FB)->FreeList)                 \
}


//
//  Heap tagging routines implemented in heapdll.c
//

#if DBG

#define IS_HEAP_TAGGING_ENABLED() (TRUE)

#else

#define IS_HEAP_TAGGING_ENABLED() (RtlGetNtGlobalFlags() & FLG_HEAP_ENABLE_TAGGING)

#endif // DBG

//
//  ORDER IS IMPORTANT HERE...SEE RtlpUpdateTagEntry sources
//

typedef enum _HEAP_TAG_ACTION {

    AllocationAction,
    VirtualAllocationAction,
    FreeAction,
    VirtualFreeAction,
    ReAllocationAction,
    VirtualReAllocationAction

} HEAP_TAG_ACTION;

PWSTR
RtlpGetTagName (
    PHEAP Heap,
    USHORT TagIndex
    );

USHORT
RtlpUpdateTagEntry (
    PHEAP Heap,
    USHORT TagIndex,
    SIZE_T OldSize,      // Only valid for ReAllocation and Free actions
    SIZE_T NewSize,      // Only valid for ReAllocation and Allocation actions
    HEAP_TAG_ACTION Action
    );

VOID
RtlpResetTags (
    PHEAP Heap
    );

VOID
RtlpDestroyTags (
    PHEAP Heap
    );


//
// Define heap lookaside list allocation functions.
//

typedef struct _HEAP_LOOKASIDE {
    SLIST_HEADER ListHead;

    USHORT Depth;
    USHORT MaximumDepth;

    ULONG TotalAllocates;
    ULONG AllocateMisses;
    ULONG TotalFrees;
    ULONG FreeMisses;

    ULONG LastTotalAllocates;
    ULONG LastAllocateMisses;

    ULONG Future[2];

} HEAP_LOOKASIDE, *PHEAP_LOOKASIDE;

NTKERNELAPI
VOID
RtlpInitializeHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
RtlpDeleteHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    );

VOID
RtlpAdjustHeapLookasideDepth (
    IN PHEAP_LOOKASIDE Lookaside
    );

NTKERNELAPI
PVOID
RtlpAllocateFromHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside
    );

NTKERNELAPI
BOOLEAN
RtlpFreeToHeapLookaside (
    IN PHEAP_LOOKASIDE Lookaside,
    IN PVOID Entry
    );

#endif // _RTL_HEAP_PRIVATE_
