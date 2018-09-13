/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1989-1995  Microsoft Corporation

Module Name:

    pool.h

Abstract:

    Private executive data structures and procedure prototypes for pool
    allocation.


    There are three pool types:
        1. nonpaged,
        2. paged, and
        3. nonpagedmustsucceed.

    There is only one of each the nonpaged and nonpagedmustsucceed pools.

    There can be more than one paged pool.

Author:

    Lou Perazzoli (loup) 23-Feb-1989

Revision History:

--*/

#ifndef _POOL_
#define _POOL_

#if !DBG && !defined (_WIN64)
#define NO_POOL_CHECKS 1
#endif

#define POOL_CACHE_SUPPORTED 0
#define POOL_CACHE_ALIGN 0

#define NUMBER_OF_POOLS 3

#if defined(NT_UP)
#define NUMBER_OF_PAGED_POOLS 2
#else
#define NUMBER_OF_PAGED_POOLS 4
#endif

#define BASE_POOL_TYPE_MASK 1

#define MUST_SUCCEED_POOL_TYPE_MASK 2

#define CACHE_ALIGNED_POOL_TYPE_MASK 4

#define SESSION_POOL_MASK 32

#define POOL_VERIFIER_MASK 64

#define POOL_DRIVER_MASK 128

//
// WARNING: POOL_QUOTA_MASK is overloaded by POOL_QUOTA_FAIL_INSTEAD_OF_RAISE
//          which is exported from ex.h.
//
// WARNING: POOL_RAISE_IF_ALLOCATION_FAILURE is exported from ex.h with a
//          value of 16.
//
// These definitions are used to control the raising of an exception as the
// result of quota and allocation failures.
//

#define POOL_QUOTA_MASK 8

#define POOL_TYPE_MASK (3)

//#define POOL_TYPE_AND_QUOTA_MASK (15)

//
// Size of a pool page.
//
// This must be greater than or equal to the page size.
//

#define POOL_PAGE_SIZE  PAGE_SIZE

//
// The smallest pool block size must be a multiple of the page size.
//
// Define the block size as 32.
//

#if (PAGE_SIZE == 0x4000)
#define POOL_BLOCK_SHIFT 6
#else
#define POOL_BLOCK_SHIFT 5
#endif

#define POOL_LIST_HEADS (POOL_PAGE_SIZE / (1 << POOL_BLOCK_SHIFT))

#define PAGE_ALIGNED(p) (!(((ULONG_PTR)p) & (POOL_PAGE_SIZE - 1)))

//
// Define page end macro.
//

#if defined(_ALPHA_) || defined(_IA64_)
#define PAGE_END(Address) (((ULONG_PTR)(Address) & (PAGE_SIZE - 1)) == (PAGE_SIZE - (1 << POOL_BLOCK_SHIFT)))
#else
#define PAGE_END(Address) (((ULONG_PTR)(Address) & (PAGE_SIZE - 1)) == 0)
#endif

//
// Define pool descriptor structure.
//

typedef struct _POOL_DESCRIPTOR {
    POOL_TYPE PoolType;
    ULONG PoolIndex;
    ULONG RunningAllocs;
    ULONG RunningDeAllocs;
    ULONG TotalPages;
    ULONG TotalBigPages;
    ULONG Threshold;
    PVOID LockAddress;
    LIST_ENTRY ListHeads[POOL_LIST_HEADS];
} POOL_DESCRIPTOR, *PPOOL_DESCRIPTOR;

//
//      Caveat Programmer:
//
//              The pool header must be QWORD (8 byte) aligned in size.  If it
//              is not, the pool allocation code will trash the allocated
//              buffer
//
//
//
// The layout of the pool header is:
//
//         31              23         16 15             7            0
//         +----------------------------------------------------------+
//         | Current Size |  PoolType+1 |  Pool Index  |Previous Size |
//         +----------------------------------------------------------+
//         |   ProcessBilled   (NULL if not allocated with quota)     |
//         +----------------------------------------------------------+
//         | Zero or more longwords of pad such that the pool header  |
//         | is on a cache line boundary and the pool body is also    |
//         | on a cache line boundary.                                |
//         +----------------------------------------------------------+
//
//      PoolBody:
//
//         +----------------------------------------------------------+
//         |  Used by allocator, or when free FLINK into sized list   |
//         +----------------------------------------------------------+
//         |  Used by allocator, or when free BLINK into sized list   |
//         +----------------------------------------------------------+
//         ... rest of pool block...
//
//
// N.B. The size fields of the pool header are expressed in units of the
//      smallest pool block size.
//

typedef struct _POOL_HEADER {
    union {
        struct {
            UCHAR PreviousSize;
            UCHAR PoolIndex;
            UCHAR PoolType;
            UCHAR BlockSize;
        };
        ULONG Ulong1;                       // used for InterlockedCompareExchange required by Alpha
    };
#ifdef _WIN64
    ULONG PoolTag;
#endif
    union {
        EPROCESS *ProcessBilled;
#ifndef _WIN64
        ULONG PoolTag;
#endif
        struct {
            USHORT AllocatorBackTraceIndex;
            USHORT PoolTagHash;
        };
    };
} POOL_HEADER, *PPOOL_HEADER;

//
// Define size of pool block overhead.
//

#define POOL_OVERHEAD ((LONG)sizeof(POOL_HEADER))

//
// Define size of pool block overhead when the block is on a freelist.
//

#define POOL_FREE_BLOCK_OVERHEAD  (POOL_OVERHEAD + sizeof (LIST_ENTRY))

//
// Define dummy type so computation of pointers is simplified.
//

typedef struct _POOL_BLOCK {
    UCHAR Fill[1 << POOL_BLOCK_SHIFT];
} POOL_BLOCK, *PPOOL_BLOCK;

//
// Define size of smallest pool block.
//

#define POOL_SMALLEST_BLOCK (sizeof(POOL_BLOCK))

//
// Define pool tracking information.
//

#define POOL_BACKTRACEINDEX_PRESENT 0x8000

#if POOL_CACHE_SUPPORTED
#define POOL_BUDDY_MAX PoolBuddyMax
#else
#define POOL_BUDDY_MAX  \
   (POOL_PAGE_SIZE - (POOL_OVERHEAD + POOL_SMALLEST_BLOCK ))
#endif //POOL_CACHE_SUPPORTED

//
// Pool support routine and macro not for general consumption.
// This is only used by the memory manager.
//

VOID
ExpInitializePoolDescriptor(
    IN PPOOL_DESCRIPTOR PoolDescriptor,
    IN POOL_TYPE PoolType,
    IN ULONG PoolIndex,
    IN ULONG Threshold,
    IN PVOID PoolLock
    );

//++
//SIZE_T
//EX_REAL_POOL_USAGE (
//    IN SIZE_T SizeInBytes
//    );
//
// Routine Description:
//
//    This routine determines the real pool cost of the supplied allocation.
//
// Arguments
//
//    SizeInBytes - Supplies the allocation size in bytes.
//
// Return Value:
//
//    TRUE if unused segment trimming should be initiated, FALSE if not.
//
//--

#define EX_REAL_POOL_USAGE(SizeInBytes)                             \
        (((SizeInBytes) > POOL_BUDDY_MAX) ?                         \
            (ROUND_TO_PAGES(SizeInBytes)) :                         \
            (((SizeInBytes) + POOL_OVERHEAD + (POOL_SMALLEST_BLOCK - 1)) & ~(POOL_SMALLEST_BLOCK - 1)))

typedef struct _POOL_TRACKER_TABLE {
    ULONG Key;
    ULONG NonPagedAllocs;
    ULONG NonPagedFrees;
    SIZE_T NonPagedBytes;
    ULONG PagedAllocs;
    ULONG PagedFrees;
    SIZE_T PagedBytes;
} POOL_TRACKER_TABLE, *PPOOL_TRACKER_TABLE;

//
// N.B. The last entry of the pool tracker table is used for all overflow
//      table entries.
//

extern PPOOL_TRACKER_TABLE PoolTrackTable;

typedef struct _POOL_TRACKER_BIG_PAGES {
    PVOID Va;
    ULONG Key;
    ULONG NumberOfPages;
} POOL_TRACKER_BIG_PAGES, *PPOOL_TRACKER_BIG_PAGES;

#endif
