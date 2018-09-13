/*++ BUILD Version: 0007    // Increment this if a change has global effects

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ex.h

Abstract:

    Public executive data structures and procedure prototypes.

Author:

    Mark Lucovsky (markl) 23-Feb-1989

Revision History:

--*/

#ifndef _EX_
#define _EX_

//
// Define caller count hash table structures and function prototypes.
//

#define CALL_HASH_TABLE_SIZE 64

typedef struct _CALL_HASH_ENTRY {
    LIST_ENTRY ListEntry;
    PVOID CallersAddress;
    PVOID CallersCaller;
    ULONG CallCount;
} CALL_HASH_ENTRY, *PCALL_HASH_ENTRY;

typedef struct _CALL_PERFORMANCE_DATA {
    KSPIN_LOCK SpinLock;
    LIST_ENTRY HashTable[CALL_HASH_TABLE_SIZE];
} CALL_PERFORMANCE_DATA, *PCALL_PERFORMANCE_DATA;

VOID
ExInitializeCallData(
    IN PCALL_PERFORMANCE_DATA CallData
    );

VOID
ExRecordCallerInHashTable(
    IN PCALL_PERFORMANCE_DATA CallData,
    IN PVOID CallersAddress,
    IN PVOID CallersCaller
    );

#define RECORD_CALL_DATA(Table)                                            \
    {                                                                      \
        PVOID CallersAddress;                                              \
        PVOID CallersCaller;                                               \
        RtlGetCallersAddress(&CallersAddress, &CallersCaller);             \
        ExRecordCallerInHashTable((Table), CallersAddress, CallersCaller); \
    }

//
// Define executive event pair object structure.
//

typedef struct _EEVENT_PAIR {
    KEVENT_PAIR KernelEventPair;
} EEVENT_PAIR, *PEEVENT_PAIR;

//
// empty struct def so we can forward reference ETHREAD
//

struct _ETHREAD;

//
// System Initialization procedure for EX subcomponent of NTOS (in exinit.c)
//

NTKERNELAPI
BOOLEAN
ExInitSystem(
    VOID
    );

NTKERNELAPI
VOID
ExInitSystemPhase2(
    VOID
    );

VOID
ExInitPoolLookasidePointers (
    VOID
    );

ULONG
ExComputeTickCountMultiplier (
    IN ULONG TimeIncrement
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis
//
// Pool Allocation routines (in pool.c)
//

typedef enum _POOL_TYPE {
    NonPagedPool,
    PagedPool,
    NonPagedPoolMustSucceed,
    DontUseThisType,
    NonPagedPoolCacheAligned,
    PagedPoolCacheAligned,
    NonPagedPoolCacheAlignedMustS,
    MaxPoolType

    // end_wdm
    ,
    //
    // Note these per session types are carefully chosen so that the appropriate
    // masking still applies as well as MaxPoolType above.
    //

    NonPagedPoolSession = 32,
    PagedPoolSession = NonPagedPoolSession + 1,
    NonPagedPoolMustSucceedSession = PagedPoolSession + 1,
    DontUseThisTypeSession = NonPagedPoolMustSucceedSession + 1,
    NonPagedPoolCacheAlignedSession = DontUseThisTypeSession + 1,
    PagedPoolCacheAlignedSession = NonPagedPoolCacheAlignedSession + 1,
    NonPagedPoolCacheAlignedMustSSession = PagedPoolCacheAlignedSession + 1,

    // begin_wdm

    } POOL_TYPE;

// end_ntddk end_wdm end_nthal end_ntifs end_ntndis

//
// The following two definitions control the raising of exceptions on quota
// and allocation failures.
//

#define POOL_QUOTA_FAIL_INSTEAD_OF_RAISE 8
#define POOL_RAISE_IF_ALLOCATION_FAILURE 16               // ntifs

VOID
InitializePool(
    IN POOL_TYPE PoolType,
    IN ULONG Threshold
    );

//
// These routines are private to the pool manager and the memory manager.
//

VOID
ExInsertPoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes,
    POOL_TYPE PoolType
    );

VOID
ExAllocatePoolSanityChecks(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

VOID
ExFreePoolSanityChecks(
    IN PVOID P
    );

// begin_ntddk begin_nthal begin_ntifs begin_wdm

NTKERNELAPI
PVOID
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
PVOID
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    );

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

// end_wdm

//
// _EX_POOL_PRIORITY_ provides a method for the system to handle requests
// intelligently in low resource conditions.
//
// LowPoolPriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is low on resources.  An example of
// this could be for a non-critical network connection where the driver can
// handle the failure case when system resources are close to being depleted.
//
// NormalPoolPriority should be used when it is acceptable to the driver for the
// mapping request to fail if the system is very low on resources.  An example
// of this could be for a non-critical local filesystem request.
//
// HighPoolPriority should be used when it is unacceptable to the driver for the
// mapping request to fail unless the system is completely out of resources.
// An example of this would be the paging file path in a driver.
//
// SpecialPool can be specified to bound the allocation at a page end (or
// beginning).  This should only be done on systems being debugged as the
// memory cost is expensive.
//
// N.B.  These values are very carefully chosen so that the pool allocation
//       code can quickly crack the priority request.
//

typedef enum _EX_POOL_PRIORITY {
    LowPoolPriority,
    LowPoolPrioritySpecialPoolOverrun = 8,
    LowPoolPrioritySpecialPoolUnderrun = 9,
    NormalPoolPriority = 16,
    NormalPoolPrioritySpecialPoolOverrun = 24,
    NormalPoolPrioritySpecialPoolUnderrun = 25,
    HighPoolPriority = 32,
    HighPoolPrioritySpecialPoolOverrun = 40,
    HighPoolPrioritySpecialPoolUnderrun = 41

    } EX_POOL_PRIORITY;

NTKERNELAPI
PVOID
NTAPI
ExAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    );

// begin_wdm

#ifndef POOL_TAGGING
#define ExAllocatePoolWithTag(a,b,c) ExAllocatePool(a,b)
#endif //POOL_TAGGING

NTKERNELAPI
PVOID
ExAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

#ifndef POOL_TAGGING
#define ExAllocatePoolWithQuotaTag(a,b,c) ExAllocatePoolWithQuota(a,b)
#endif //POOL_TAGGING

NTKERNELAPI
VOID
NTAPI
ExFreePool(
    IN PVOID P
    );

// end_ntddk end_wdm end_nthal end_ntifs

//
// If high order bit in Pool tag is set, then must use ExFreePoolWithTag to free
//

#define PROTECTED_POOL 0x80000000

NTKERNELAPI
VOID
ExFreePoolWithTag(
    IN PVOID P,
    IN ULONG Tag
    );

#ifndef POOL_TAGGING
#define ExFreePoolWithTag(a,b) ExFreePool(a)
#endif //POOL_TAGGING


NTKERNELAPI
KIRQL
ExLockPool(
    IN POOL_TYPE PoolType
    );

NTKERNELAPI
VOID
ExUnlockPool(
    IN POOL_TYPE PoolType,
    IN KIRQL LockHandle
    );

NTKERNELAPI                                     // ntifs
ULONG                                           // ntifs
ExQueryPoolBlockSize (                          // ntifs
    IN PVOID PoolBlock,                         // ntifs
    OUT PBOOLEAN QuotaCharged                   // ntifs
    );                                          // ntifs

NTKERNELAPI
VOID
ExQueryPoolUsage(
    OUT PULONG PagedPoolPages,
    OUT PULONG NonPagedPoolPages,
    OUT PULONG PagedPoolAllocs,
    OUT PULONG PagedPoolFrees,
    OUT PULONG PagedPoolLookasideHits,
    OUT PULONG NonPagedPoolAllocs,
    OUT PULONG NonPagedPoolFrees,
    OUT PULONG NonPagedPoolLookasideHits
    );

VOID
ExReturnPoolQuota (
    IN PVOID P
    );

#if DBG || (i386 && !FPO)
NTKERNELAPI
NTSTATUS
ExSnapShotPool(
    IN POOL_TYPE PoolType,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    );
#endif // DBG || (i386 && !FPO)


// begin_ntifs begin_ntddk begin_wdm begin_nthal
//
// Routines to support fast mutexes.
//

typedef struct _FAST_MUTEX {
    LONG Count;
    PKTHREAD Owner;
    ULONG Contention;
    KEVENT Event;
    ULONG OldIrql;
} FAST_MUTEX, *PFAST_MUTEX;

#if DBG
#define ExInitializeFastMutex(_FastMutex)                            \
    (_FastMutex)->Count = 1;                                         \
    (_FastMutex)->Owner = NULL;                                      \
    (_FastMutex)->Contention = 0;                                    \
    KeInitializeEvent(&(_FastMutex)->Event,                          \
                      SynchronizationEvent,                          \
                      FALSE);
#else
#define ExInitializeFastMutex(_FastMutex)                            \
    (_FastMutex)->Count = 1;                                         \
    (_FastMutex)->Contention = 0;                                    \
    KeInitializeEvent(&(_FastMutex)->Event,                          \
                      SynchronizationEvent,                          \
                      FALSE);
#endif // DBG

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutexUnsafe (
    IN PFAST_MUTEX FastMutex
    );

#if defined(_ALPHA_) || defined(_IA64_)

NTKERNELAPI
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTKERNELAPI
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

// end_wdm

NTKERNELAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

// begin_wdm

#elif defined(_X86_)

NTHALAPI
VOID
FASTCALL
ExAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

NTHALAPI
VOID
FASTCALL
ExReleaseFastMutex (
    IN PFAST_MUTEX FastMutex
    );

// end_wdm

NTHALAPI
BOOLEAN
FASTCALL
ExTryToAcquireFastMutex (
    IN PFAST_MUTEX FastMutex
    );

// begin_wdm

#else

#error "Target architecture not defined"

#endif

// end_ntifs end_ntddk end_wdm end_nthal

//
// Interlocked support routine definitions.
//
// begin_ntddk begin_wdm begin_nthal begin_ntifs begin_ntndis
//

NTKERNELAPI
VOID
FASTCALL
ExInterlockedAddLargeStatistic (
    IN PLARGE_INTEGER Addend,
    IN ULONG Increment
    );

// end_ntndis

NTKERNELAPI
LARGE_INTEGER
ExInterlockedAddLargeInteger (
    IN PLARGE_INTEGER Addend,
    IN LARGE_INTEGER Increment,
    IN PKSPIN_LOCK Lock
    );

//  end_wdm end_ntifs end_ntddk end_nthal

#if defined(NT_UP) && !defined(_NTHAL_) && !defined(_NTDDK_) && !defined(_NTIFS_)

#undef ExInterlockedAddUlong
#define ExInterlockedAddUlong(x, y, z) InterlockedExchangeAdd((PLONG)(x), (LONG)(y))

#else

//  begin_wdm begin_ntddk begin_nthal begin_ntifs

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedAddUlong (
    IN PULONG Addend,
    IN ULONG Increment,
    IN PKSPIN_LOCK Lock
    );

// end_wdm end_ntifs end_ntddk end_nthal

#endif

// begin_wdm begin_ntddk begin_nthal begin_ntifs

#if defined(_AXP64_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    InterlockedCompareExchange64(Destination, *(Exchange), *(Comperand))

#elif defined(_ALPHA_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    ExpInterlockedCompareExchange64(Destination, Exchange, Comperand)

#elif defined(_IA64_)

#define ExInterlockedCompareExchange64(Destination, Exchange, Comperand, Lock) \
    InterlockedCompareExchange64(Destination, *(Exchange), *(Comperand))

#else

NTKERNELAPI
LONGLONG
FASTCALL
ExInterlockedCompareExchange64 (
    IN PLONGLONG Destination,
    IN PLONGLONG Exchange,
    IN PLONGLONG Comperand,
    IN PKSPIN_LOCK Lock
    );

#endif

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertHeadList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedInsertTailList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PLIST_ENTRY
FASTCALL
ExInterlockedRemoveHeadList (
    IN PLIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntryList (
    IN PSINGLE_LIST_ENTRY ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

// begin_ntndis

//
// Define interlocked sequenced listhead functions.
//
// A sequenced interlocked list is a singly linked list with a header that
// contains the current depth and a sequence number. Each time an entry is
// inserted or removed from the list the depth is updated and the sequence
// number is incremented. This enables MIPS, Alpha, and Pentium and later
// machines to insert and remove from the list without the use of spinlocks.
// The PowerPc, however, must use a spinlock to synchronize access to the
// list.
//
// N.B. A spinlock must be specified with SLIST operations. However, it may
//      not actually be used.
//

/*++

VOID
ExInitializeSListHead (
    IN PSLIST_HEADER SListHead
    )

Routine Description:

    This function initializes a sequenced singly linked listhead.

Arguments:

    SListHead - Supplies a pointer to a sequenced singly linked listhead.

Return Value:

    None.

--*/

#define ExInitializeSListHead(_listhead_) (_listhead_)->Alignment = 0

/*++

USHORT
ExQueryDepthSList (
    IN PSLIST_HEADERT SListHead
    )

Routine Description:

    This function queries the current number of entries contained in a
    sequenced single linked list.

Arguments:

    SListHead - Supplies a pointer to the sequenced listhead which is
        be queried.

Return Value:

    The current number of entries in the sequenced singly linked list is
    returned as the function value.

--*/

#define ExQueryDepthSList(_listhead_) (USHORT)(_listhead_)->Depth

#if defined(_MIPS_) || defined(_ALPHA_) || defined(_IA64_)

#define ExInterlockedPopEntrySList(Head, Lock) \
    ExpInterlockedPopEntrySList(Head)

#define ExInterlockedPushEntrySList(Head, Entry, Lock) \
    ExpInterlockedPushEntrySList(Head, Entry)

#define ExInterlockedFlushSList(Head) \
    ExpInterlockedFlushSList(Head)

NTKERNELAPI
PSINGLE_LIST_ENTRY
ExpInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
ExpInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
ExpInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

#else

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry,
    IN PKSPIN_LOCK Lock
    );

NTKERNELAPI
PSINGLE_LIST_ENTRY
FASTCALL
ExInterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

#endif

// end_ntddk end_wdm
//
// Define interlocked lookaside list structure and allocation functions.
//

VOID
ExAdjustLookasideDepth (
    VOID
    );

// begin_ntddk begin_wdm

typedef
PVOID
(*PALLOCATE_FUNCTION) (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    );

typedef
VOID
(*PFREE_FUNCTION) (
    IN PVOID Buffer
    );

typedef struct _GENERAL_LOOKASIDE {
    SLIST_HEADER ListHead;
    USHORT Depth;
    USHORT MaximumDepth;
    ULONG TotalAllocates;
    union {
        ULONG AllocateMisses;
        ULONG AllocateHits;
    };

    ULONG TotalFrees;
    union {
        ULONG FreeMisses;
        ULONG FreeHits;
    };

    POOL_TYPE Type;
    ULONG Tag;
    ULONG Size;
    PALLOCATE_FUNCTION Allocate;
    PFREE_FUNCTION Free;
    LIST_ENTRY ListEntry;
    ULONG LastTotalAllocates;
    union {
        ULONG LastAllocateMisses;
        ULONG LastAllocateHits;
    };

    ULONG Future[2];
} GENERAL_LOOKASIDE, *PGENERAL_LOOKASIDE;

typedef struct _NPAGED_LOOKASIDE_LIST {
    GENERAL_LOOKASIDE L;
    KSPIN_LOCK Lock;
} NPAGED_LOOKASIDE_LIST, *PNPAGED_LOOKASIDE_LIST;


NTKERNELAPI
VOID
ExInitializeNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
ExDeleteNPagedLookasideList (
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    );

__inline
PVOID
ExAllocateFromNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;
    Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead, &Lookaside->Lock);
    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

__inline
VOID
ExFreeToNPagedLookasideList(
    IN PNPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    nonpaged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {
        ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                    (PSINGLE_LIST_ENTRY)Entry,
                                    &Lookaside->Lock);
    }

    return;
}

// end_ntndis

typedef struct _PAGED_LOOKASIDE_LIST {
    GENERAL_LOOKASIDE L;
    FAST_MUTEX Lock;
} PAGED_LOOKASIDE_LIST, *PPAGED_LOOKASIDE_LIST;

NTKERNELAPI
VOID
ExInitializePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PALLOCATE_FUNCTION Allocate,
    IN PFREE_FUNCTION Free,
    IN ULONG Flags,
    IN SIZE_T Size,
    IN ULONG Tag,
    IN USHORT Depth
    );

NTKERNELAPI
VOID
ExDeletePagedLookasideList (
    IN PPAGED_LOOKASIDE_LIST Lookaside
    );

#if defined(_X86_)

NTKERNELAPI
PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside
    );

NTKERNELAPI
VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    );

#else

__inline
PVOID
ExAllocateFromPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a paged lookaside list structure.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;

    Lookaside->L.TotalAllocates += 1;
    Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead, NULL);
    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Entry = (Lookaside->L.Allocate)(Lookaside->L.Type,
                                        Lookaside->L.Size,
                                        Lookaside->L.Tag);
    }

    return Entry;
}

__inline
VOID
ExFreeToPagedLookasideList(
    IN PPAGED_LOOKASIDE_LIST Lookaside,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    paged lookaside list.

Arguments:

    Lookaside - Supplies a pointer to a nonpaged lookaside list structure.

    Entry - Supples a pointer to the entry that is inserted in the
        lookaside list.

Return Value:

    None.

--*/

{

    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        (Lookaside->L.Free)(Entry);

    } else {
        ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                    (PSINGLE_LIST_ENTRY)Entry,
                                    NULL);
    }

    return;
}

#endif

// end_ntddk end_nthal end_ntifs end_wdm

//
// Define per processor nonpage lookaside list structures.
//

typedef enum _PP_NPAGED_LOOKASIDE_NUMBER {
    LookasideSmallIrpList,
    LookasideLargeIrpList,
    LookasideMdlList,
    LookasideCreateInfoList,
    LookasideNameBufferList,
    LookasideTwilightList,
    LookasideCompletionList,
    LookasideMaximumList
} PP_NPAGED_LOOKASIDE_NUMBER, *PPP_NPAGED_LOOKASIDE_NUMBER;

#if !defined(_CROSS_PLATFORM_)

__inline
PVOID
ExAllocateFromPPNPagedLookasideList(
    IN PP_NPAGED_LOOKASIDE_NUMBER Number
    )

/*++

Routine Description:

    This function removes (pops) the first entry from the specified
    nonpaged per processor lookaside list.

Arguments:

    Number - Supplies the per processor nonpaged lookaside list number.

Return Value:

    If an entry is removed from the specified lookaside list, then the
    address of the entry is returned as the function value. Otherwise,
    NULL is returned.

--*/

{

    PVOID Entry;
    PNPAGED_LOOKASIDE_LIST Lookaside;
    PKPRCB Prcb;

    //
    // Get address of current processor block.
    //
    // N.B. It is possible to context switch during the allocation from a
    //      per processor nonpaged lookaside list, but this should happen
    //      infrequently and should not aversely effect the benefits of
    //      per processor lookaside lists.
    //

    ASSERT((Number >= 0) && (Number < LookasideMaximumList));

    Prcb = KeGetCurrentPrcb();

    //
    // Attempt to allocate from the per processor lookaside list.
    //

    Lookaside = Prcb->PPLookasideList[Number].P;
    Lookaside->L.TotalAllocates += 1;
    Entry = ExInterlockedPopEntrySList(&Lookaside->L.ListHead, &Lookaside->Lock);

    //
    // If the per processor allocation attempt failed, then attempt to
    // allocate from the system lookaside list.
    //

    if (Entry == NULL) {
        Lookaside->L.AllocateMisses += 1;
        Lookaside = Prcb->PPLookasideList[Number].L;
        Entry = ExAllocateFromNPagedLookasideList(Lookaside);
    }

    return Entry;
}

__inline
VOID
ExFreeToPPNPagedLookasideList(
    IN PP_NPAGED_LOOKASIDE_NUMBER Number,
    IN PVOID Entry
    )

/*++

Routine Description:

    This function inserts (pushes) the specified entry into the specified
    nonpaged per processor lookaside list.

Arguments:

    Number - Supplies the per processor nonpaged lookaside list number.

    Entry - Supples a pointer to the entry that is inserted in the nonpaged
        per processor lookaside list.

Return Value:

    None.

--*/

{

    PNPAGED_LOOKASIDE_LIST Lookaside;
    PKPRCB Prcb;

    //
    // Get address of current processor block.
    //
    // N.B. It is possible to context switch during the free to a per
    //      processor nonpaged lookaside list, but this should happen
    //      infrequently and should not aversely effect the benefits of
    //      per processor lookaside lists.
    //

    ASSERT((Number >= 0) && (Number < LookasideMaximumList));

    Prcb = KeGetCurrentPrcb();

    //
    // If the current depth is less than of equal to the maximum depth, then
    // free the specified entry to the per processor lookaside list. Otherwise,
    // free the entry to the system lookaside list;
    //
    //

    Lookaside = Prcb->PPLookasideList[Number].P;
    Lookaside->L.TotalFrees += 1;
    if (ExQueryDepthSList(&Lookaside->L.ListHead) >= Lookaside->L.Depth) {
        Lookaside->L.FreeMisses += 1;
        Lookaside = Prcb->PPLookasideList[Number].L;
        ExFreeToNPagedLookasideList(Lookaside, Entry);

    } else {
        ExInterlockedPushEntrySList(&Lookaside->L.ListHead,
                                    (PSINGLE_LIST_ENTRY)Entry,
                                    &Lookaside->Lock);
    }

    return;
}

#endif

#if i386 && !FPO

NTSTATUS
ExQuerySystemBackTraceInformation(
    OUT PRTL_PROCESS_BACKTRACES BackTraceInformation,
    IN ULONG BackTraceInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

NTKERNELAPI
USHORT
ExGetPoolBackTraceIndex(
    IN PVOID P
    );

#endif // i386 && !FPO

NTKERNELAPI
PVOID
ExLockUserBuffer(
    IN PVOID Buffer,
    IN ULONG Length,
    OUT PVOID *LockVariable
    );

NTKERNELAPI
VOID
ExUnlockUserBuffer(
    IN PVOID LockVariable
    );



// begin_ntddk begin_wdm begin_ntifs

NTKERNELAPI
VOID
NTAPI
ProbeForRead(
    IN CONST VOID *Address,
    IN ULONG Length,
    IN ULONG Alignment
    );

// end_ntddk end_wdm end_ntifs

#if !defined(_NTHAL_) && !defined(_NTDDK_) && !defined(_NTIFS_)

//
// Probe function definitions
//
// Probe for read functions.
//
//++
//
// VOID
// ProbeForRead(
//     IN PVOID Address,
//     IN ULONG Length,
//     IN ULONG Alignment
//     )
//
//--

#define ProbeForRead(Address, Length, Alignment)                             \
    ASSERT(((Alignment) == 1) || ((Alignment) == 2) ||                       \
           ((Alignment) == 4) || ((Alignment) == 8) ||                       \
           ((Alignment) == 16));                                             \
                                                                             \
    if ((Length) != 0) {                                                     \
        if (((ULONG_PTR)(Address) & ((Alignment) - 1)) != 0) {                   \
            ExRaiseDatatypeMisalignment();                                   \
                                                                             \
        } else if ((((ULONG_PTR)(Address) + (Length)) < (ULONG_PTR)(Address)) ||                                  \
                   (((ULONG_PTR)(Address) + (Length)) > (ULONG_PTR)MM_USER_PROBE_ADDRESS)) { \
            ExRaiseAccessViolation();                                        \
        }                                                                    \
    }

#endif

//++
//
// BOOLEAN
// ProbeAndReadBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeAndReadBoolean(Address) \
    (((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS) : (*(volatile BOOLEAN *)(Address)))

//++
//
// CHAR
// ProbeAndReadChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeAndReadChar(Address) \
    (((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile CHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile CHAR *)(Address)))

//++
//
// UCHAR
// ProbeAndReadUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeAndReadUchar(Address) \
    (((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UCHAR * const)MM_USER_PROBE_ADDRESS) : (*(volatile UCHAR *)(Address)))

//++
//
// SHORT
// ProbeAndReadShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeAndReadShort(Address) \
    (((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile SHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile SHORT *)(Address)))

//++
//
// USHORT
// ProbeAndReadUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeAndReadUshort(Address) \
    (((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile USHORT * const)MM_USER_PROBE_ADDRESS) : (*(volatile USHORT *)(Address)))

//++
//
// HANDLE
// ProbeAndReadHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndReadHandle(Address) \
    (((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile HANDLE * const)MM_USER_PROBE_ADDRESS) : (*(volatile HANDLE *)(Address)))

//++
//
// PVOID
// ProbeAndReadPointer(
//     IN PVOID *Address
//     )
//
//--

#define ProbeAndReadPointer(Address) \
    (((Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile PVOID * const)MM_USER_PROBE_ADDRESS) : (*(volatile PVOID *)(Address)))

//++
//
// LONG
// ProbeAndReadLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeAndReadLong(Address) \
    (((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile LONG *)(Address)))

//++
//
// ULONG
// ProbeAndReadUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeAndReadUlong(Address) \
    (((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULONG *)(Address)))

//++
//
// ULONG_PTR
// ProbeAndReadUlong_ptr(
//     IN PULONG_PTR Address
//     )
//
//--

#define ProbeAndReadUlong_ptr(Address) \
    (((Address) >= (ULONG_PTR * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULONG_PTR * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULONG_PTR *)(Address)))

//++
//
// QUAD
// ProbeAndReadQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeAndReadQuad(Address) \
    (((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile QUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile QUAD *)(Address)))

//++
//
// UQUAD
// ProbeAndReadUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeAndReadUquad(Address) \
    (((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UQUAD * const)MM_USER_PROBE_ADDRESS) : (*(volatile UQUAD *)(Address)))

//++
//
// LARGE_INTEGER
// ProbeAndReadLargeInteger(
//     IN PLARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadLargeInteger(Source)  \
    (((Source) >= (LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile LARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile LARGE_INTEGER *)(Source)))

//++
//
// ULARGE_INTEGER
// ProbeAndReadUlargeInteger(
//     IN PULARGE_INTEGER Source
//     )
//
//--

#define ProbeAndReadUlargeInteger(Source)  \
    (((Source) >= (ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile ULARGE_INTEGER * const)MM_USER_PROBE_ADDRESS) : (*(volatile ULARGE_INTEGER *)(Source)))

//++
//
// UNICODE_STRING
// ProbeAndReadUnicodeString(
//     IN PUNICODE_STRING Source
//     )
//
//--

#define ProbeAndReadUnicodeString(Source)  \
    (((Source) >= (UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) ? \
        (*(volatile UNICODE_STRING * const)MM_USER_PROBE_ADDRESS) : (*(volatile UNICODE_STRING *)(Source)))

//++
//
// <STRUCTURE>
// ProbeAndReadStructure(
//     IN P<STRUCTURE> Source
//     <STRUCTURE>
//     )
//
//--

#define ProbeAndReadStructure(Source,STRUCTURE)  \
    (((Source) >= (STRUCTURE * const)MM_USER_PROBE_ADDRESS) ? \
        (*(STRUCTURE * const)MM_USER_PROBE_ADDRESS) : (*(STRUCTURE *)(Source)))

//
// Probe for write functions definitions.
//
//++
//
// VOID
// ProbeForWriteBoolean(
//     IN PBOOLEAN Address
//     )
//
//--

#define ProbeForWriteBoolean(Address) {                                      \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(volatile BOOLEAN *)(Address) = *(volatile BOOLEAN *)(Address);         \
}

//++
//
// VOID
// ProbeForWriteChar(
//     IN PCHAR Address
//     )
//
//--

#define ProbeForWriteChar(Address) {                                         \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile CHAR *)(Address) = *(volatile CHAR *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUchar(
//     IN PUCHAR Address
//     )
//
//--

#define ProbeForWriteUchar(Address) {                                        \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UCHAR *)(Address) = *(volatile UCHAR *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteIoStatus(
//     IN PIO_STATUS_BLOCK Address
//     )
//
//--

#define ProbeForWriteIoStatus(Address) {                                     \
    if ((Address) >= (IO_STATUS_BLOCK * const)MM_USER_PROBE_ADDRESS) {       \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile IO_STATUS_BLOCK *)(Address) = *(volatile IO_STATUS_BLOCK *)(Address); \
}

#ifdef  _WIN64
#define ProbeForWriteIoStatusEx(Address, Cookie) {                                          \
    if ((Address) >= (IO_STATUS_BLOCK * const)MM_USER_PROBE_ADDRESS) {                      \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                                 \
    }                                                                                       \
    if ((ULONG_PTR)(Cookie) & (ULONG)1) {                                                            \
        *(volatile IO_STATUS_BLOCK32 *)(Address) = *(volatile IO_STATUS_BLOCK32 *)(Address);\
    } else {                                                                                \
        *(volatile IO_STATUS_BLOCK *)(Address) = *(volatile IO_STATUS_BLOCK *)(Address);    \
    }                                                                                       \
}
#else
#define ProbeForWriteIoStatusEx(Address, Cookie)    ProbeForWriteIoStatus(Address)
#endif

//++
//
// VOID
// ProbeForWriteShort(
//     IN PSHORT Address
//     )
//
//--

#define ProbeForWriteShort(Address) {                                        \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile SHORT *)(Address) = *(volatile SHORT *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteUshort(
//     IN PUSHORT Address
//     )
//
//--

#define ProbeForWriteUshort(Address) {                                       \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile USHORT *)(Address) = *(volatile USHORT *)(Address);           \
}

//++
//
// VOID
// ProbeForWriteHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeForWriteHandle(Address) {                                       \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = *(volatile HANDLE *)(Address);           \
}

//++
//
// VOID
// ProbeAndZeroHandle(
//     IN PHANDLE Address
//     )
//
//--

#define ProbeAndZeroHandle(Address) {                                        \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(volatile HANDLE *)(Address) = 0;                                       \
}

//++
//
// VOID
// ProbeForWritePointer(
//     IN PVOID Address
//     )
//
//--

#define ProbeForWritePointer(Address) {                                      \
    if ((PVOID *)(Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) {        \
        *(volatile PVOID * const)MM_USER_PROBE_ADDRESS = NULL;               \
    }                                                                        \
                                                                             \
    *(volatile PVOID *)(Address) = *(volatile PVOID *)(Address);             \
}

//++
//
// VOID
// ProbeAndNullPointer(
//     IN PVOID *Address
//     )
//
//--

#define ProbeAndNullPointer(Address) {                                       \
    if ((PVOID *)(Address) >= (PVOID * const)MM_USER_PROBE_ADDRESS) {        \
        *(volatile PVOID * const)MM_USER_PROBE_ADDRESS = NULL;               \
    }                                                                        \
                                                                             \
    *(volatile PVOID *)(Address) = NULL;                                     \
}

//++
//
// VOID
// ProbeForWriteLong(
//     IN PLONG Address
//     )
//
//--

#define ProbeForWriteLong(Address) {                                        \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                       \
                                                                            \
    *(volatile LONG *)(Address) = *(volatile LONG *)(Address);              \
}

//++
//
// VOID
// ProbeForWriteUlong(
//     IN PULONG Address
//     )
//
//--

#define ProbeForWriteUlong(Address) {                                        \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile ULONG *)(Address) = *(volatile ULONG *)(Address);             \
}

//++
//
// VOID
// ProbeForWriteUlong_ptr(
//     IN PULONG_PTR Address
//     )
//
//--

#define ProbeForWriteUlong_ptr(Address) {                                    \
    if ((Address) >= (ULONG_PTR * const)MM_USER_PROBE_ADDRESS) {             \
        *(volatile ULONG_PTR * const)MM_USER_PROBE_ADDRESS = 0;              \
    }                                                                        \
                                                                             \
    *(volatile ULONG_PTR *)(Address) = *(volatile ULONG_PTR *)(Address);     \
}

//++
//
// VOID
// ProbeForWriteQuad(
//     IN PQUAD Address
//     )
//
//--

#define ProbeForWriteQuad(Address) {                                         \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(volatile QUAD *)(Address) = *(volatile QUAD *)(Address);               \
}

//++
//
// VOID
// ProbeForWriteUquad(
//     IN PUQUAD Address
//     )
//
//--

#define ProbeForWriteUquad(Address) {                                        \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(volatile UQUAD *)(Address) = *(volatile UQUAD *)(Address);             \
}

//
// Probe and write functions definitions.
//
//++
//
// VOID
// ProbeAndWriteBoolean(
//     IN PBOOLEAN Address,
//     IN BOOLEAN Value
//     )
//
//--

#define ProbeAndWriteBoolean(Address, Value) {                               \
    if ((Address) >= (BOOLEAN * const)MM_USER_PROBE_ADDRESS) {               \
        *(volatile BOOLEAN * const)MM_USER_PROBE_ADDRESS = 0;                \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteChar(
//     IN PCHAR Address,
//     IN CHAR Value
//     )
//
//--

#define ProbeAndWriteChar(Address, Value) {                                  \
    if ((Address) >= (CHAR * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile CHAR * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUchar(
//     IN PUCHAR Address,
//     IN UCHAR Value
//     )
//
//--

#define ProbeAndWriteUchar(Address, Value) {                                 \
    if ((Address) >= (UCHAR * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile UCHAR * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteShort(
//     IN PSHORT Address,
//     IN SHORT Value
//     )
//
//--

#define ProbeAndWriteShort(Address, Value) {                                 \
    if ((Address) >= (SHORT * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile SHORT * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUshort(
//     IN PUSHORT Address,
//     IN USHORT Value
//     )
//
//--

#define ProbeAndWriteUshort(Address, Value) {                                \
    if ((Address) >= (USHORT * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile USHORT * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteHandle(
//     IN PHANDLE Address,
//     IN HANDLE Value
//     )
//
//--

#define ProbeAndWriteHandle(Address, Value) {                                \
    if ((Address) >= (HANDLE * const)MM_USER_PROBE_ADDRESS) {                \
        *(volatile HANDLE * const)MM_USER_PROBE_ADDRESS = 0;                 \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteLong(
//     IN PLONG Address,
//     IN LONG Value
//     )
//
//--

#define ProbeAndWriteLong(Address, Value) {                                  \
    if ((Address) >= (LONG * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUlong(
//     IN PULONG Address,
//     IN ULONG Value
//     )
//
//--

#define ProbeAndWriteUlong(Address, Value) {                                 \
    if ((Address) >= (ULONG * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteQuad(
//     IN PQUAD Address,
//     IN QUAD Value
//     )
//
//--

#define ProbeAndWriteQuad(Address, Value) {                                  \
    if ((Address) >= (QUAD * const)MM_USER_PROBE_ADDRESS) {                  \
        *(volatile LONG * const)MM_USER_PROBE_ADDRESS = 0;                   \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteUquad(
//     IN PUQUAD Address,
//     IN UQUAD Value
//     )
//
//--

#define ProbeAndWriteUquad(Address, Value) {                                 \
    if ((Address) >= (UQUAD * const)MM_USER_PROBE_ADDRESS) {                 \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

//++
//
// VOID
// ProbeAndWriteSturcture(
//     IN P<STRUCTURE> Address,
//     IN <STRUCTURE> Value,
//     <STRUCTURE>
//     )
//
//--

#define ProbeAndWriteStructure(Address, Value,STRUCTURE) {                   \
    if ((STRUCTURE * const)(Address) >= (STRUCTURE * const)MM_USER_PROBE_ADDRESS) {    \
        *(volatile ULONG * const)MM_USER_PROBE_ADDRESS = 0;                  \
    }                                                                        \
                                                                             \
    *(Address) = (Value);                                                    \
}

// begin_ntifs begin_ntddk begin_wdm
//
// Common probe for write functions.
//

NTKERNELAPI
VOID
NTAPI
ProbeForWrite (
    IN PVOID Address,
    IN ULONG Length,
    IN ULONG Alignment
    );

// end_ntifs end_ntddk end_wdm

//
// Timer Rundown
//

NTKERNELAPI
VOID
ExTimerRundown (
    VOID
    );

// begin_ntddk begin_wdm begin_nthal begin_ntifs
//
// Worker Thread
//

typedef enum _WORK_QUEUE_TYPE {
    CriticalWorkQueue,
    DelayedWorkQueue,
    HyperCriticalWorkQueue,
    MaximumWorkQueue
} WORK_QUEUE_TYPE;

typedef
VOID
(*PWORKER_THREAD_ROUTINE)(
    IN PVOID Parameter
    );

typedef struct _WORK_QUEUE_ITEM {
    LIST_ENTRY List;
    PWORKER_THREAD_ROUTINE WorkerRoutine;
    PVOID Parameter;
} WORK_QUEUE_ITEM, *PWORK_QUEUE_ITEM;


#define ExInitializeWorkItem(Item, Routine, Context) \
    (Item)->WorkerRoutine = (Routine);               \
    (Item)->Parameter = (Context);                   \
    (Item)->List.Flink = NULL;

NTKERNELAPI
VOID
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    );

//  end_wdm

NTKERNELAPI
BOOLEAN
ExIsProcessorFeaturePresent(
    ULONG ProcessorFeature
    );

// end_ntddk end_nthal end_ntifs

typedef struct _EX_WORK_QUEUE {

    //
    // Queue objects that that are used to hold work queue entries and
    // synchronize worker thread activity.
    //

    KQUEUE WorkerQueue;

    //
    // Number of dynamic worker threads that have been created "on the fly"
    // as part of worker thread deadlock prevention
    //

    ULONG DynamicThreadCount;

    //
    // Count of the number of work items processed.
    //

    ULONG WorkItemsProcessed;

    //
    // Used for deadlock detection, WorkItemsProcessedLastPass equals the value
    // of WorkItemsProcessed the last time ExpDetectWorkerThreadDeadlock()
    // ran.
    //

    ULONG WorkItemsProcessedLastPass;

    //
    // QueueDepthLastPass is also part of the worker queue state snapshot
    // taken by ExpDetectWorkerThreadDeadlock().
    //

    ULONG QueueDepthLastPass;

    //
    // MakeThreadsAsNecessary indicates whether this work queue is elligible
    // for dynamic creation of threads not just for deadlock detection,
    // but to ensure that the CPUs are all kept busy clearing any work
    // item backlog.
    //

    BOOLEAN MakeThreadsAsNecessary;

} EX_WORK_QUEUE, *PEX_WORK_QUEUE;

extern EX_WORK_QUEUE ExWorkerQueue[];


// begin_ntddk begin_nthal begin_ntifs
//
// Zone Allocation
//

typedef struct _ZONE_SEGMENT_HEADER {
    SINGLE_LIST_ENTRY SegmentList;
    PVOID Reserved;
} ZONE_SEGMENT_HEADER, *PZONE_SEGMENT_HEADER;

typedef struct _ZONE_HEADER {
    SINGLE_LIST_ENTRY FreeList;
    SINGLE_LIST_ENTRY SegmentList;
    ULONG BlockSize;
    ULONG TotalSegmentSize;
} ZONE_HEADER, *PZONE_HEADER;


NTKERNELAPI
NTSTATUS
ExInitializeZone(
    IN PZONE_HEADER Zone,
    IN ULONG BlockSize,
    IN PVOID InitialSegment,
    IN ULONG InitialSegmentSize
    );

NTKERNELAPI
NTSTATUS
ExExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize
    );

NTKERNELAPI
NTSTATUS
ExInterlockedExtendZone(
    IN PZONE_HEADER Zone,
    IN PVOID Segment,
    IN ULONG SegmentSize,
    IN PKSPIN_LOCK Lock
    );

//++
//
// PVOID
// ExAllocateFromZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#define ExAllocateFromZone(Zone) \
    (PVOID)((Zone)->FreeList.Next); \
    if ( (Zone)->FreeList.Next ) (Zone)->FreeList.Next = (Zone)->FreeList.Next->Next


//++
//
// PVOID
// ExFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#define ExFreeToZone(Zone,Block)                                    \
    ( ((PSINGLE_LIST_ENTRY)(Block))->Next = (Zone)->FreeList.Next,  \
      (Zone)->FreeList.Next = ((PSINGLE_LIST_ENTRY)(Block)),        \
      ((PSINGLE_LIST_ENTRY)(Block))->Next                           \
    )

//++
//
// BOOLEAN
// ExIsFullZone(
//     IN PZONE_HEADER Zone
//     )
//
// Routine Description:
//
//     This routine determines if the specified zone is full or not.  A zone
//     is considered full if the free list is empty.
//
// Arguments:
//
//     Zone - Pointer to the zone header to be tested.
//
// Return Value:
//
//     TRUE if the zone is full and FALSE otherwise.
//
//--

#define ExIsFullZone(Zone) \
    ( (Zone)->FreeList.Next == (PSINGLE_LIST_ENTRY)NULL )

//++
//
// PVOID
// ExInterlockedAllocateFromZone(
//     IN PZONE_HEADER Zone,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine removes an entry from the zone and returns a pointer to it.
//     The removal is performed with the specified lock owned for the sequence
//     to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage from which the
//         entry is to be allocated.
//
//     Lock - Pointer to the spin lock which should be obtained before removing
//         the entry from the allocation list.  The lock is released before
//         returning to the caller.
//
// Return Value:
//
//     The function value is a pointer to the storage allocated from the zone.
//
//--

#define ExInterlockedAllocateFromZone(Zone,Lock) \
    (PVOID) ExInterlockedPopEntryList( &(Zone)->FreeList, Lock )

//++
//
// PVOID
// ExInterlockedFreeToZone(
//     IN PZONE_HEADER Zone,
//     IN PVOID Block,
//     IN PKSPIN_LOCK Lock
//     )
//
// Routine Description:
//
//     This routine places the specified block of storage back onto the free
//     list in the specified zone.  The insertion is performed with the lock
//     owned for the sequence to make it MP-safe.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         entry is to be inserted.
//
//     Block - Pointer to the block of storage to be freed back to the zone.
//
//     Lock - Pointer to the spin lock which should be obtained before inserting
//         the entry onto the free list.  The lock is released before returning
//         to the caller.
//
// Return Value:
//
//     Pointer to previous block of storage that was at the head of the free
//         list.  NULL implies the zone went from no available free blocks to
//         at least one free block.
//
//--

#define ExInterlockedFreeToZone(Zone,Block,Lock) \
    ExInterlockedPushEntryList( &(Zone)->FreeList, ((PSINGLE_LIST_ENTRY) (Block)), Lock )


//++
//
// BOOLEAN
// ExIsObjectInFirstZoneSegment(
//     IN PZONE_HEADER Zone,
//     IN PVOID Object
//     )
//
// Routine Description:
//
//     This routine determines if the specified pointer lives in the zone.
//
// Arguments:
//
//     Zone - Pointer to the zone header controlling the storage to which the
//         object may belong.
//
//     Object - Pointer to the object in question.
//
// Return Value:
//
//     TRUE if the Object came from the first segment of zone.
//
//--

#define ExIsObjectInFirstZoneSegment(Zone,Object) ((BOOLEAN)     \
    (((PUCHAR)(Object) >= (PUCHAR)(Zone)->SegmentList.Next) &&   \
     ((PUCHAR)(Object) < (PUCHAR)(Zone)->SegmentList.Next +      \
                         (Zone)->TotalSegmentSize))              \
)

// end_ntddk end_nthal end_ntifs



// begin_ntifs begin_ntddk
//
//  Define executive resource data structures.
//

typedef ULONG_PTR ERESOURCE_THREAD;
typedef ERESOURCE_THREAD *PERESOURCE_THREAD;

typedef struct _OWNER_ENTRY {
    ERESOURCE_THREAD OwnerThread;
    union {
        LONG OwnerCount;
        ULONG TableSize;
    };

} OWNER_ENTRY, *POWNER_ENTRY;

typedef struct _ERESOURCE {
    LIST_ENTRY SystemResourcesList;
    POWNER_ENTRY OwnerTable;
    SHORT ActiveCount;
    USHORT Flag;
    PKSEMAPHORE SharedWaiters;
    PKEVENT ExclusiveWaiters;
    OWNER_ENTRY OwnerThreads[2];
    ULONG ContentionCount;
    USHORT NumberOfSharedWaiters;
    USHORT NumberOfExclusiveWaiters;
    union {
        PVOID Address;
        ULONG_PTR CreatorBackTraceIndex;
    };

    KSPIN_LOCK SpinLock;
} ERESOURCE, *PERESOURCE;

//
//  Values for ERESOURCE.Flag
//

#define ResourceNeverExclusive       0x10
#define ResourceReleaseByOtherThread 0x20
#define ResourceOwnedExclusive       0x80

#define RESOURCE_HASH_TABLE_SIZE 64

typedef struct _RESOURCE_HASH_ENTRY {
    LIST_ENTRY ListEntry;
    PVOID Address;
    ULONG ContentionCount;
    ULONG Number;
} RESOURCE_HASH_ENTRY, *PRESOURCE_HASH_ENTRY;

typedef struct _RESOURCE_PERFORMANCE_DATA {
    ULONG ActiveResourceCount;
    ULONG TotalResourceCount;
    ULONG ExclusiveAcquire;
    ULONG SharedFirstLevel;
    ULONG SharedSecondLevel;
    ULONG StarveFirstLevel;
    ULONG StarveSecondLevel;
    ULONG WaitForExclusive;
    ULONG OwnerTableExpands;
    ULONG MaximumTableExpand;
    LIST_ENTRY HashTable[RESOURCE_HASH_TABLE_SIZE];
} RESOURCE_PERFORMANCE_DATA, *PRESOURCE_PERFORMANCE_DATA;

//
// Define executive resource function prototypes.
//

NTKERNELAPI
NTSTATUS
ExInitializeResourceLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
NTSTATUS
ExReinitializeResourceLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
BOOLEAN
ExAcquireResourceSharedLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireResourceExclusiveLite(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireSharedStarveExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExAcquireSharedWaitForExclusive(
    IN PERESOURCE Resource,
    IN BOOLEAN Wait
    );

NTKERNELAPI
BOOLEAN
ExTryToAcquireResourceExclusiveLite(
    IN PERESOURCE Resource
    );

//
//  VOID
//  ExReleaseResource(
//      IN PERESOURCE Resource
//      );
//

#define ExReleaseResource(R) (ExReleaseResourceLite(R))

NTKERNELAPI
VOID
FASTCALL
ExReleaseResourceLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
VOID
ExReleaseResourceForThreadLite(
    IN PERESOURCE Resource,
    IN ERESOURCE_THREAD ResourceThreadId
    );

NTKERNELAPI
VOID
ExSetResourceOwnerPointer(
    IN PERESOURCE Resource,
    IN PVOID OwnerPointer
    );

NTKERNELAPI
VOID
ExConvertExclusiveToSharedLite(
    IN PERESOURCE Resource
    );

NTKERNELAPI
NTSTATUS
ExDeleteResourceLite (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExGetExclusiveWaiterCount (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExGetSharedWaiterCount (
    IN PERESOURCE Resource
    );

// end_ntddk

NTKERNELAPI
VOID
ExDisableResourceBoostLite (
    IN PERESOURCE Resource
    );

// begin_ntddk
//
//  ERESOURCE_THREAD
//  ExGetCurrentResourceThread(
//      );
//

#define ExGetCurrentResourceThread() ((ULONG_PTR)PsGetCurrentThread())

NTKERNELAPI
BOOLEAN
ExIsResourceAcquiredExclusiveLite (
    IN PERESOURCE Resource
    );

NTKERNELAPI
ULONG
ExIsResourceAcquiredSharedLite (
    IN PERESOURCE Resource
    );

//
//  ntddk.h stole the entrypoints we wanted so fix them up here.
//

#define ExInitializeResource ExInitializeResourceLite
#define ExAcquireResourceShared ExAcquireResourceSharedLite
#define ExAcquireResourceExclusive ExAcquireResourceExclusiveLite
#define ExReleaseResourceForThread ExReleaseResourceForThreadLite
#define ExConvertExclusiveToShared ExConvertExclusiveToSharedLite
#define ExDeleteResource ExDeleteResourceLite
#define ExIsResourceAcquiredExclusive ExIsResourceAcquiredExclusiveLite
#define ExIsResourceAcquiredShared ExIsResourceAcquiredSharedLite
// end_ntddk
#define ExDisableResourceBoost ExDisableResourceBoostLite
// end_ntifs

#if DEVL
NTKERNELAPI
NTSTATUS
ExQuerySystemLockInformation(
    OUT struct _RTL_PROCESS_LOCKS *LockInformation,
    IN ULONG LockInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );
#endif // DEVL

//
//  Shared resource function definitions (in resource.c).
//
//  This definition here matches the ntddk one defined above.  It allows
//  the resource package to deal with these antiquated objects.
//

typedef struct _NTDDK_ERESOURCE {

    //
    //  First 8 bytes are used to align the next part of the structure
    //  onto 16 bytes.  (typical case)
    //

    LIST_ENTRY          SystemResourcesList;

    //
    //  Next 128 bits of this structure are field which we know
    //  we will hit to obtain this resource either shared or exclusive
    //

    PERESOURCE_THREAD   OwnerThreads;
    PUCHAR              OwnerCounts;

    USHORT              TableSize;
    USHORT              ActiveCount;

    USHORT              Flag;
    USHORT              TableRover;                 // (0 - 128 bits)

    //
    //  Next 128 bits contain the initial counters and at least the
    //  first initial thread (which is also highly updated)
    //

    UCHAR               InitialOwnerCounts[4];
    ERESOURCE_THREAD    InitialOwnerThreads[4];

    ULONG               Spare1;

    //
    //  The rest is what ever was left.  The spinlock is in with
    //  a part of the structure we normally don't touch in the
    //  hot paths (read or write)
    //

    ULONG               ContentionCount;

    USHORT              NumberOfExclusiveWaiters;
    USHORT              NumberOfSharedWaiters;

    KSEMAPHORE          SharedWaiters;
    KEVENT              ExclusiveWaiters;

    KSPIN_LOCK          SpinLock;

    USHORT CreatorBackTraceIndex;
    USHORT Spare2;

} NTDDK_ERESOURCE;
typedef NTDDK_ERESOURCE *PNTDDK_ERESOURCE;

//
//  These are routines that were unfortunately exported to ntddk.h
//
//  They live in ntos\ex\ddkresrc.c
//

//
//  NTKERNELAPI
//  NTSTATUS
//  ExInitializeResource(
//      IN PNTDDK_ERESOURCE Resource
//      );
//
//  NTKERNELAPI
//  BOOLEAN
//  ExAcquireResourceExclusive(
//      IN PNTDDK_ERESOURCE Resource,
//      IN BOOLEAN Wait
//      );
//
//  NTKERNELAPI
//  VOID
//  ExReleaseResourceForThread(
//      IN PNTDDK_ERESOURCE Resource,
//      IN ERESOURCE_THREAD ResourceThreadId
//      );
//
//  NTKERNELAPI
//  NTSTATUS
//      ExDeleteResource (
//      IN PNTDDK_ERESOURCE Resource
//      );
//


//
//  The Ex/Ob handle table interface package (in handle.c)
//

//
//  The Ex/Ob handle table package uses a common handle definition.  The actual
//  type definition for a handle is a pvoid and is declared in sdk/inc.  This
//  package uses only the low 32 bits of the pvoid pointer.
//
//  For simplicity we declare a new typedef called an exhandle
//
//  The 2 bits of an EXHANDLE is available to the application and is
//  ignored by the system.  The next 24 bits store the handle table entry
//  index and is used to refer to a particular entry in a handle table.
//
//  Note that this format is immutable because there are outside programs with
//  hardwired code that already assumes the format of a handle.
//

typedef struct _EXHANDLE {

    union {

        struct {

            //
            //  Application available tag bits
            //

            ULONG TagBits : 2;

            //
            //  The handle table entry index
            //

            ULONG Index : 30;
        };

        HANDLE GenericHandleOverlay;
    };

} EXHANDLE, *PEXHANDLE;

//
//  A handle table stores multiple handle table entries, each entry is looked
//  up by its exhandle.  A handle table entry has really two fields.
//
//  The first field contains a pointer object and is overloaded with the three
//  low order bits used by ob to denote inherited, protected, and audited
//  objects.   The upper bit used as a handle table entry lock.  Note, this
//  means that all valid object pointers must be at least longword aligned and
//  have their sign bit set (i.e., be negative).
//
//  The next field contains the acces mask (sometimes in the form of a granted
//  access index, and creator callback trace) if the entry is in use or a
//  pointer in the free list if the entry is free.
//
//  Two things to note:
//
//  1. An entry is free if the object pointer is null, this means that the
//     following field contains the FreeTableEntryList.
//
//  2. An entry is unlocked if the object pointer is positive and locked if its
//     negative.  The handle package through callbacks and Map Handle to
//     Pointer will lock the entry (thus making the pointer valid) outside
//     routines can then read and reset the attributes field and the object
//     provided they don't unlock the entry.  When the callbacks return the
//     entry will be unlocked and the callers or MapHandleToPointer will need
//     to call UnlockHandleTableEntry explicitly.
//

typedef struct _HANDLE_TABLE_ENTRY {

    //
    //  The pointer to the object overloaded with three ob attributes bits in
    //  the lower order and the high bit to denote locked or unlocked entries
    //

    union {

        PVOID Object;

        ULONG ObAttributes;
    };

    //
    //  This field either contains the granted access mask for the handle or an
    //  ob variation that also stores the same information.  Or in the case of
    //  a free entry the field stores the index for the next free entry in the
    //  free list.  This is like a FAT chain, and is used instead of pointers
    //  to make table duplication easier, because the entries can just be
    //  copied without needing to modify pointers.
    //

    union {

        union {

            ACCESS_MASK GrantedAccess;

            struct {

                USHORT GrantedAccessIndex;
                USHORT CreatorBackTraceIndex;
            };
        };

        LONG NextFreeTableEntry;
    };

} HANDLE_TABLE_ENTRY, *PHANDLE_TABLE_ENTRY;

//
//  One handle table exists per process.  Unless otherwise specified, via a
//  call to RemoveHandleTable, all handle tables are linked together in a
//  global list.  This list is used by the snapshot handle tables call.
//

typedef struct _HANDLE_TABLE {

    //
    //  A set of flags used to denote the state or attributes of this
    //  particular handle table
    //

    ULONG Flags;

    //
    //  The number of handle table entries in use.
    //

    LONG HandleCount;

    //
    //  A pointer to the top level handle table tree node.
    //

    PHANDLE_TABLE_ENTRY **Table;

    //
    //  The process who is being charged quota for this handle table and a
    //  unique process id to use in our callbacks
    //

    struct _EPROCESS *QuotaProcess;
    HANDLE UniqueProcessId;

    //
    //  This is a singly linked list of free table entries.  We don't actually
    //  use pointers, but have each store the index of the next free entry
    //  in the list.  The list is managed as a lifo list.  We also keep track
    //  of the next index that we have to allocate pool to hold.
    //

    LONG FirstFreeTableEntry;
    LONG NextIndexNeedingPool;

    //
    //  This is the lock used to protect the fields in the record, and the
    //  handle table tree in general.  Individual handle table entries that are
    //  not free have their own lock
    //

    ERESOURCE HandleTableLock;

    //
    //  The list of global handle tables.  This field is protected by a global
    //  lock.
    //

    LIST_ENTRY HandleTableList;

    //
    //  The following field is used to loosely synchronize thread contention
    //  on a handle.  If a thread wants to wait for a handle to be unlocked
    //  it will wait on this event with a short timeout.  Any handle unlock
    //  operation will pulse this event if there are threads waiting on it
    //

    KEVENT HandleContentionEvent;

} HANDLE_TABLE, *PHANDLE_TABLE;

//
//  Routines for handle manipulation.
//

//
//  Functions for locking and unlocking the handle table, and for locking and
//  locking handle table entries
//

NTKERNELAPI
VOID
ExLockHandleTableShared (
    PHANDLE_TABLE HandleTable
    );

NTKERNELAPI
VOID
ExLockHandleTableExclusive (
    PHANDLE_TABLE HandleTable
    );

NTKERNELAPI
VOID
ExUnlockHandleTableShared (
    PHANDLE_TABLE HandleTable
    );

NTKERNELAPI
VOID
ExUnlockHandleTableExclusive (
    PHANDLE_TABLE HandleTable
    );

NTKERNELAPI
BOOLEAN
ExLockHandleTableEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    );

NTKERNELAPI
VOID
ExUnlockHandleTableEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    );

//
//  A global initialization function called on at system start up
//

NTKERNELAPI
VOID
ExInitializeHandleTablePackage (
    VOID
    );

//
//  Functions to create, remove, and destroy handle tables per process.  The
//  destroy function uses a callback.
//

NTKERNELAPI
PHANDLE_TABLE
ExCreateHandleTable (
    IN struct _EPROCESS *Process OPTIONAL
    );

NTKERNELAPI
VOID
ExRemoveHandleTable (
    IN PHANDLE_TABLE HandleTable
    );

typedef VOID (*EX_DESTROY_HANDLE_ROUTINE)(
    IN HANDLE Handle
    );

NTKERNELAPI
VOID
ExDestroyHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_DESTROY_HANDLE_ROUTINE DestroyHandleProcedure
    );

//
//  A function to enumerate through the handle table of a process using a
//  callback.
//

typedef BOOLEAN (*EX_ENUMERATE_HANDLE_ROUTINE)(
    IN PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN HANDLE Handle,
    IN PVOID EnumParameter
    );

NTKERNELAPI
BOOLEAN
ExEnumHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter,
    OUT PHANDLE Handle OPTIONAL
    );

//
//  A function to duplicate the handle table of a process using a callback
//

typedef BOOLEAN (*EX_DUPLICATE_HANDLE_ROUTINE)(
    IN struct _EPROCESS *Process OPTIONAL,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    );

NTKERNELAPI
PHANDLE_TABLE
ExDupHandleTable (
    IN struct _EPROCESS *Process OPTIONAL,
    IN PHANDLE_TABLE OldHandleTable,
    IN EX_DUPLICATE_HANDLE_ROUTINE DupHandleProcedure OPTIONAL
    );

//
//  A function that enumerates all the handles in all the handle tables
//  throughout the system using a callback.
//

typedef NTSTATUS (*PEX_SNAPSHOT_HANDLE_ENTRY)(
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PHANDLE_TABLE_ENTRY HandleEntry,
    IN HANDLE Handle,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

NTKERNELAPI
NTSTATUS
ExSnapShotHandleTables (
    IN PEX_SNAPSHOT_HANDLE_ENTRY SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

//
//  Functions to create, destroy, and modify handle table entries the modify
//  function using a callback
//

NTKERNELAPI
HANDLE
ExCreateHandle (
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    );


NTKERNELAPI
BOOLEAN
ExDestroyHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry OPTIONAL
    );


typedef BOOLEAN (*PEX_CHANGE_HANDLE_ROUTINE) (
    IN OUT PHANDLE_TABLE_ENTRY HandleTableEntry,
    IN ULONG_PTR Parameter
    );

NTKERNELAPI
BOOLEAN
ExChangeHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_ROUTINE ChangeRoutine,
    IN ULONG_PTR Parameter
    );

//
//  A function that takes a handle value and returns a pointer to the
//  associated handle table entry.
//

NTKERNELAPI
PHANDLE_TABLE_ENTRY
ExMapHandleToPointer (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle
    );

//
//  Macros for resetting the owner of the handle table, and current
//  noop macro for setting fifo/lifo behaviour of the table
//

#define ExSetHandleTableOwner(ht,id) {(ht)->UniqueProcessId = (id);}

#define ExSetHandleTableOrder(ht,or) {NOTHING;}


//
// Locally Unique Identifier Services
//

NTKERNELAPI
BOOLEAN
ExLuidInitialization (
    VOID
    );

//
// VOID
// ExAllocateLocallyUniqueId (
//     PLUID Luid
//     )
//
//*++
//
// Routine Description:
//
//     This function returns an LUID value that is unique since the system
//     was last rebooted. It is unique only on the system it is generated on
//     and not network wide.
//
//     N.B. A LUID is a 64-bit value and for all practical purposes will
//          never carry in the lifetime of a single boot of the system.
//          At an increment rate of 1ns, the value would carry to zero in
//          approximately 126 years.
//
// Arguments:
//
//     Luid - Supplies a pointer to a variable that receives the allocated
//          locally unique Id.
//
// Return Value:
//
//     The allocated LUID value.
//
// --*/


extern LARGE_INTEGER ExpLuid;
extern LARGE_INTEGER ExpLuidIncrement;
extern KSPIN_LOCK ExpLuidLock;


#define ExAllocateLocallyUniqueId(Luid)                                 \
    {                                                                   \
    LARGE_INTEGER _TempLi;                                              \
                                                                        \
    _TempLi = ExInterlockedExchangeAddLargeInteger(&ExpLuid,            \
                                                   ExpLuidIncrement,    \
                                                   &ExpLuidLock);       \
    (Luid)->LowPart = _TempLi.LowPart;                                  \
    (Luid)->HighPart = _TempLi.HighPart;                                \
    }


// begin_ntddk begin_wdm begin_ntifs
//
// Get previous mode
//

NTKERNELAPI
KPROCESSOR_MODE
ExGetPreviousMode(
    VOID
    );
// end_ntddk end_wdm end_ntifs

//
// Raise exception from kernel mode.
//

NTKERNELAPI
VOID
NTAPI
ExRaiseException (
    PEXCEPTION_RECORD ExceptionRecord
    );

// begin_ntddk begin_wdm begin_ntifs
//
// Raise status from kernel mode.
//

NTKERNELAPI
VOID
NTAPI
ExRaiseStatus (
    IN NTSTATUS Status
    );

// end_wdm

NTKERNELAPI
VOID
ExRaiseDatatypeMisalignment (
    VOID
    );

NTKERNELAPI
VOID
ExRaiseAccessViolation (
    VOID
    );

// end_ntddk end_ntifs

extern BOOLEAN ExReadyForErrors;

NTKERNELAPI
NTSTATUS
ExRaiseHardError(
    IN NTSTATUS ErrorStatus,
    IN ULONG NumberOfParameters,
    IN ULONG UnicodeStringParameterMask,
    IN PULONG_PTR Parameters,
    IN ULONG ValidResponseOptions,
    OUT PULONG Response
    );

int
ExSystemExceptionFilter(
    VOID
    );

//
// The following are global counters used by the EX component to indicate
// the amount of EventPair transactions being performed in the system.
//

extern ULONG EvPrSetHigh;
extern ULONG EvPrSetLow;


//
// Debug event logging facility
//

#define EX_DEBUG_LOG_FORMAT_NONE     (UCHAR)0
#define EX_DEBUG_LOG_FORMAT_ULONG    (UCHAR)1
#define EX_DEBUG_LOG_FORMAT_PSZ      (UCHAR)2
#define EX_DEBUG_LOG_FORMAT_PWSZ     (UCHAR)3
#define EX_DEBUG_LOG_FORMAT_STRING   (UCHAR)4
#define EX_DEBUG_LOG_FORMAT_USTRING  (UCHAR)5
#define EX_DEBUG_LOG_FORMAT_OBJECT   (UCHAR)6
#define EX_DEBUG_LOG_FORMAT_HANDLE   (UCHAR)7

#define EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES 4
#define EX_DEBUG_LOG_NUMBER_OF_BACK_TRACES 4

typedef struct _EX_DEBUG_LOG_TAG {
    UCHAR Format[ EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES ];
    PCHAR Name;
} EX_DEBUG_LOG_TAG, *PEX_DEBUG_LOG_TAG;

typedef struct _EX_DEBUG_LOG_EVENT {
    USHORT ThreadId;
    USHORT ProcessId;
    ULONG Time : 24;
    ULONG Tag : 8;
    ULONG BackTrace[ EX_DEBUG_LOG_NUMBER_OF_BACK_TRACES ];
    ULONG Data[ EX_DEBUG_LOG_NUMBER_OF_DATA_VALUES ];
} EX_DEBUG_LOG_EVENT, *PEX_DEBUG_LOG_EVENT;

typedef struct _EX_DEBUG_LOG {
    KSPIN_LOCK Lock;
    ULONG NumberOfTags;
    ULONG MaximumNumberOfTags;
    PEX_DEBUG_LOG_TAG Tags;
    ULONG CountOfEventsLogged;
    PEX_DEBUG_LOG_EVENT First;
    PEX_DEBUG_LOG_EVENT Last;
    PEX_DEBUG_LOG_EVENT Next;
} EX_DEBUG_LOG, *PEX_DEBUG_LOG;


NTKERNELAPI
PEX_DEBUG_LOG
ExCreateDebugLog(
    IN UCHAR MaximumNumberOfTags,
    IN ULONG MaximumNumberOfEvents
    );

NTKERNELAPI
UCHAR
ExCreateDebugLogTag(
    IN PEX_DEBUG_LOG Log,
    IN PCHAR Name,
    IN UCHAR Format1,
    IN UCHAR Format2,
    IN UCHAR Format3,
    IN UCHAR Format4
    );

NTKERNELAPI
VOID
ExDebugLogEvent(
    IN PEX_DEBUG_LOG Log,
    IN UCHAR Tag,
    IN ULONG Data1,
    IN ULONG Data2,
    IN ULONG Data3,
    IN ULONG Data4
    );

VOID
ExShutdownSystem(
    VOID
    );

VOID
ExAcquireTimeRefreshLock(
    VOID
    );

VOID
ExReleaseTimeRefreshLock(
    VOID
    );

VOID
ExUpdateSystemTimeFromCmos (
    IN BOOLEAN UpdateInterruptTime,
    IN ULONG   MaxSepInSeconds
    );

VOID
ExGetNextWakeTime (
    OUT PULONGLONG      DueTime,
    OUT PTIME_FIELDS    TimeFields,
    OUT PVOID           *TimerObject
    );

// begin_ntddk begin_wdm begin_ntifs
//
// Set timer resolution.
//

NTKERNELAPI
ULONG
ExSetTimerResolution (
    IN ULONG DesiredTime,
    IN BOOLEAN SetResolution
    );

// end_wdm
//
// Subtract time zone bias from system time to get local time.
//

NTKERNELAPI
VOID
ExSystemTimeToLocalTime (
    IN PLARGE_INTEGER SystemTime,
    OUT PLARGE_INTEGER LocalTime
    );

//
// Add time zone bias to local time to get system time.
//

NTKERNELAPI
VOID
ExLocalTimeToSystemTime (
    IN PLARGE_INTEGER LocalTime,
    OUT PLARGE_INTEGER SystemTime
    );

// end_ntddk end_ntifs

NTKERNELAPI
VOID
ExInitializeTimeRefresh(
    VOID
    );

// begin_ntddk begin_wdm begin_ntifs begin_nthal begin_ntminiport

//
// Define the type for Callback function.
//

typedef struct _CALLBACK_OBJECT *PCALLBACK_OBJECT;

typedef VOID (*PCALLBACK_FUNCTION ) (
    IN PVOID CallbackContext,
    IN PVOID Argument1,
    IN PVOID Argument2
    );


NTKERNELAPI
NTSTATUS
ExCreateCallback (
    OUT PCALLBACK_OBJECT *CallbackObject,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN BOOLEAN Create,
    IN BOOLEAN AllowMultipleCallbacks
    );

NTKERNELAPI
PVOID
ExRegisterCallback (
    IN PCALLBACK_OBJECT CallbackObject,
    IN PCALLBACK_FUNCTION CallbackFunction,
    IN PVOID CallbackContext
    );

NTKERNELAPI
VOID
ExUnregisterCallback (
    IN PVOID CallbackRegistration
    );

NTKERNELAPI
VOID
ExNotifyCallback (
    IN PVOID CallbackObject,
    IN PVOID Argument1,
    IN PVOID Argument2
    );

// end_ntddk end_wdm end_ntifs end_nthal end_ntminiport

//
// The current bias from GMT to LocalTime
//

extern LARGE_INTEGER ExpTimeZoneBias;
extern LONG ExpLastTimeZoneBias;
extern LONG ExpAltTimeZoneBias;
extern ULONG ExpCurrentTimeZoneId;
extern ULONG ExpRealTimeIsUniversal;
extern ULONG ExCriticalWorkerThreads;
extern ULONG ExDelayedWorkerThreads;
extern ULONG ExpTickCountMultiplier;

//
// The lock handle for PAGELK section, initialized in init\init.c
//

extern PVOID ExPageLockHandle;

//
// Global executive callbacks
//

extern PCALLBACK_OBJECT ExCbSetSystemTime;
extern PCALLBACK_OBJECT ExCbSetSystemState;
extern PCALLBACK_OBJECT ExCbPowerState;


typedef
PVOID
(*PKWIN32_GLOBALATOMTABLE_CALLOUT) ( void );

extern PKWIN32_GLOBALATOMTABLE_CALLOUT ExGlobalAtomTableCallout;

// begin_ntddk

//
// UUID Generation
//

typedef GUID UUID;

NTKERNELAPI
NTSTATUS
ExUuidCreate(
    OUT UUID *Uuid
    );

// end_ntddk

//
// suite support
//


NTKERNELAPI
BOOLEAN
ExVerifySuite(
    SUITE_TYPE SuiteType
    );

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedSetBits (
    IN OUT PULONG Flags,
    IN ULONG Flag
    );

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedClearBits (
    IN OUT PULONG Flags,
    IN ULONG Flag
    );

NTKERNELAPI
ULONG
FASTCALL
ExInterlockedSetClearBits (
    IN OUT PULONG Flags,
    IN ULONG sFlag,
    IN ULONG cFlag
    );
#endif /* _EX_ */
