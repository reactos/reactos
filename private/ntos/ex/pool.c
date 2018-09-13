/*++

Copyright (c) 1989-1994  Microsoft Corporation

Module Name:

    pool.c

Abstract:

    This module implements the NT executive pool allocator.

Author:

    Mark Lucovsky     16-feb-1989
    Lou Perazzoli     31-Aug-1991 (change from binary buddy)
    David N. Cutler (davec) 27-May-1994
    Landy Wang        17-Oct-1997

Environment:

    Kernel mode only

Revision History:

--*/

#include "exp.h"

#pragma hdrstop

#undef ExAllocatePoolWithTag
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#undef ExAllocatePoolWithQuotaTag
#undef ExFreePoolWithTag

#if defined (_WIN64)
#define POOL_QUOTA_ENABLED (TRUE)
#else
#define POOL_QUOTA_ENABLED (PoolTrackTable == NULL)
#endif

//
// These bitfield definitions are based on EX_POOL_PRIORITY in inc\ex.h.
//

#define POOL_SPECIAL_POOL_BIT               0x8
#define POOL_SPECIAL_POOL_UNDERRUN_BIT      0x1

//
// FREE_CHECK_ERESOURCE - If enabled causes each free pool to verify
// no active ERESOURCEs are in the pool block being freed.
//
// FREE_CHECK_KTIMER - If enabled causes each free pool to verify no
// active KTIMERs are in the pool block being freed.
//

#if DBG

#define FREE_CHECK_ERESOURCE(Va, NumberOfBytes) \
            ExpCheckForResource(Va, NumberOfBytes)

#define FREE_CHECK_KTIMER(Va, NumberOfBytes) \
            KeCheckForTimer(Va, NumberOfBytes)

#define FREE_CHECK_WORKER(Va, NumberOfBytes) \
            ExpCheckForWorker(Va, NumberOfBytes)

#else

#define FREE_CHECK_ERESOURCE(Va, NumberOfBytes)
#define FREE_CHECK_KTIMER(Va, NumberOfBytes)
#define FREE_CHECK_WORKER(Va, NumberOfBytes)

#endif


#if defined(_ALPHA_) && !defined(_AXP64_)

//
// On Alpha32, Entry->PoolType cannot be updated without
// synchronizing with updates to Entry->PreviousSize.
// Otherwise, the lack of byte granularity can cause one
// update to get lost.
//

#define _POOL_LOCK_GRANULAR_ 1

#define LOCK_POOL_GRANULAR(PoolDesc, LockHandle)        \
            LOCK_POOL(PoolDesc, LockHandle);

#define UNLOCK_POOL_GRANULAR(PoolDesc, LockHandle)      \
            UNLOCK_POOL(PoolDesc, LockHandle);

#else

#define LOCK_POOL_GRANULAR(PoolDesc, LockHandle)
#define UNLOCK_POOL_GRANULAR(PoolDesc, LockHandle)

#endif


//
// We redefine the LIST_ENTRY macros to have each pointer biased
// by one so any rogue code using these pointers will access
// violate.  See \nt\public\sdk\inc\ntrtl.h for the original
// definition of these macros.
//
// This is turned off in the shipping product.
//

#ifndef NO_POOL_CHECKS

ULONG ExpPoolBugCheckLine;

PVOID
MmSqueezeBadTags (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    );

#define DecodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link) & ~1))
#define EncodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link) |  1))

#define PrivateInitializeListHead(ListHead) (                     \
    (ListHead)->Flink = (ListHead)->Blink = EncodeLink(ListHead))

#define PrivateIsListEmpty(ListHead)              \
    (DecodeLink((ListHead)->Flink) == (ListHead))

#define PrivateRemoveHeadList(ListHead)                     \
    DecodeLink((ListHead)->Flink);                          \
    {PrivateRemoveEntryList(DecodeLink((ListHead)->Flink))}

#define PrivateRemoveTailList(ListHead)                     \
    DecodeLink((ListHead)->Blink);                          \
    {PrivateRemoveEntryList(DecodeLink((ListHead)->Blink))}

#define PrivateRemoveEntryList(Entry) {       \
    PLIST_ENTRY _EX_Blink;                    \
    PLIST_ENTRY _EX_Flink;                    \
    _EX_Flink = DecodeLink((Entry)->Flink);   \
    _EX_Blink = DecodeLink((Entry)->Blink);   \
    _EX_Blink->Flink = EncodeLink(_EX_Flink); \
    _EX_Flink->Blink = EncodeLink(_EX_Blink); \
    }

#define PrivateInsertTailList(ListHead,Entry) {  \
    PLIST_ENTRY _EX_Blink;                       \
    PLIST_ENTRY _EX_ListHead;                    \
    _EX_ListHead = (ListHead);                   \
    _EX_Blink = DecodeLink(_EX_ListHead->Blink); \
    (Entry)->Flink = EncodeLink(_EX_ListHead);   \
    (Entry)->Blink = EncodeLink(_EX_Blink);      \
    _EX_Blink->Flink = EncodeLink(Entry);        \
    _EX_ListHead->Blink = EncodeLink(Entry);     \
    }

#define PrivateInsertHeadList(ListHead,Entry) {  \
    PLIST_ENTRY _EX_Flink;                       \
    PLIST_ENTRY _EX_ListHead;                    \
    _EX_ListHead = (ListHead);                   \
    _EX_Flink = DecodeLink(_EX_ListHead->Flink); \
    (Entry)->Flink = EncodeLink(_EX_Flink);      \
    (Entry)->Blink = EncodeLink(_EX_ListHead);   \
    _EX_Flink->Blink = EncodeLink(Entry);        \
    _EX_ListHead->Flink = EncodeLink(Entry);     \
    }

#define CHECK_LIST(LINE,LIST,ENTRY)                                         \
    if ((DecodeLink(DecodeLink((LIST)->Flink)->Blink) != (LIST)) ||         \
        (DecodeLink(DecodeLink((LIST)->Blink)->Flink) != (LIST))) {         \
            ExpPoolBugCheckLine = LINE;                                     \
            KeBugCheckEx (BAD_POOL_HEADER,                                  \
                          3,                                                \
                          (ULONG_PTR)LIST,                                  \
                          (ULONG_PTR)DecodeLink(DecodeLink((LIST)->Flink)->Blink),     \
                          (ULONG_PTR)DecodeLink(DecodeLink((LIST)->Blink)->Flink));    \
    }

#define CHECK_POOL_HEADER(LINE,ENTRY) {                                                 \
    PPOOL_HEADER PreviousEntry;                                                         \
    PPOOL_HEADER NextEntry;                                                             \
    if ((ENTRY)->PreviousSize != 0) {                                                   \
        PreviousEntry = (PPOOL_HEADER)((PPOOL_BLOCK)(ENTRY) - (ENTRY)->PreviousSize);   \
        if ((PreviousEntry->BlockSize != (ENTRY)->PreviousSize) ||                      \
            (DECODE_POOL_INDEX(PreviousEntry) != DECODE_POOL_INDEX(ENTRY))) {           \
            ExpPoolBugCheckLine = LINE;                                     \
            KeBugCheckEx(BAD_POOL_HEADER, 5, (ULONG_PTR)PreviousEntry, LINE, (ULONG_PTR)ENTRY); \
        }                                                                               \
    }                                                                                   \
    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)(ENTRY) + (ENTRY)->BlockSize);              \
    if (!PAGE_END(NextEntry)) {                                                         \
        if ((NextEntry->PreviousSize != (ENTRY)->BlockSize) ||                          \
            (DECODE_POOL_INDEX(NextEntry) != DECODE_POOL_INDEX(ENTRY))) {               \
            ExpPoolBugCheckLine = LINE;                                     \
            KeBugCheckEx(BAD_POOL_HEADER, 5, (ULONG_PTR)NextEntry, LINE, (ULONG_PTR)ENTRY);     \
        }                                                                               \
    }                                                                                   \
}

#define ASSERT_ALLOCATE_IRQL(_PoolType, _NumberOfBytes)                 \
    if ((_PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {               \
        if (KeGetCurrentIrql() > APC_LEVEL) {                           \
            KeBugCheckEx (BAD_POOL_CALLER, 8, KeGetCurrentIrql(), _PoolType, _NumberOfBytes);                                                           \
        }                                                               \
    }                                                                   \
    else {                                                              \
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {                      \
            KeBugCheckEx (BAD_POOL_CALLER, 8, KeGetCurrentIrql(), _PoolType, _NumberOfBytes);                                                           \
        }                                                               \
    }

#define ASSERT_FREE_IRQL(_PoolType, _P)                                 \
    if ((_PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {               \
        if (KeGetCurrentIrql() > APC_LEVEL) {                           \
            KeBugCheckEx (BAD_POOL_CALLER, 9, KeGetCurrentIrql(), _PoolType, (ULONG_PTR)_P);                                                            \
        }                                                               \
    }                                                                   \
    else {                                                              \
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {                      \
            KeBugCheckEx (BAD_POOL_CALLER, 9, KeGetCurrentIrql(), _PoolType, (ULONG_PTR)P);                                                             \
        }                                                               \
    }

#define ASSERT_POOL_NOT_FREE(_Entry)                                    \
    if ((_Entry->PoolType & POOL_TYPE_MASK) == 0) {                     \
        KeBugCheckEx (BAD_POOL_CALLER, 6, __LINE__, (ULONG_PTR)_Entry, _Entry->Ulong1);                                                                 \
    }

#define ASSERT_POOL_TYPE_NOT_ZERO(_Entry)                               \
    if (_Entry->PoolType == 0) {                                        \
        KeBugCheckEx(BAD_POOL_CALLER, 1, (ULONG_PTR)_Entry, (ULONG_PTR)(*(PULONG)_Entry), 0);                                                           \
    }

#define CHECK_LOOKASIDE_LIST(LINE,LIST,ENTRY) {NOTHING;}

#else

#define DecodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link)))
#define EncodeLink(Link) ((PLIST_ENTRY)((ULONG_PTR)(Link)))
#define PrivateInitializeListHead InitializeListHead
#define PrivateIsListEmpty        IsListEmpty
#define PrivateRemoveHeadList     RemoveHeadList
#define PrivateRemoveTailList     RemoveTailList
#define PrivateRemoveEntryList    RemoveEntryList
#define PrivateInsertTailList     InsertTailList
#define PrivateInsertHeadList     InsertHeadList

#define ASSERT_ALLOCATE_IRQL(_PoolType, _P) {NOTHING;}
#define ASSERT_FREE_IRQL(_PoolType, _P)     {NOTHING;}
#define ASSERT_POOL_NOT_FREE(_Entry)        {NOTHING;}
#define ASSERT_POOL_TYPE_NOT_ZERO(_Entry)   {NOTHING;}

//
// The check list macros come in two flavors - there is one in the checked
// and free build that will bugcheck the system if a list is ill-formed, and
// there is one for the final shipping version that has all the checked
// disabled.
//
// The check lookaside list macros also comes in two flavors and is used to
// verify that the look aside lists are well formed.
//
// The check pool header macro (two flavors) verifies that the specified
// pool header matches the preceeding and succeeding pool headers.
//

#define CHECK_LIST(LINE,LIST,ENTRY)         {NOTHING;}
#define CHECK_POOL_HEADER(LINE,ENTRY)       {NOTHING;}

#define CHECK_LOOKASIDE_LIST(LINE,LIST,ENTRY) {NOTHING;}

#define CHECK_POOL_PAGE(PAGE) \
    {                                                                         \
        PPOOL_HEADER P = (PPOOL_HEADER)(((ULONG_PTR)(PAGE)) & ~(PAGE_SIZE-1));    \
        ULONG SIZE, LSIZE;                                                    \
        LOGICAL FOUND=FALSE;                                                  \
        LSIZE = 0;                                                            \
        SIZE = 0;                                                             \
        do {                                                                  \
            if (P == (PPOOL_HEADER)PAGE) {                                    \
                FOUND = TRUE;                                                 \
            }                                                                 \
            if (P->PreviousSize != LSIZE) {                                   \
                DbgPrint("POOL: Inconsistent size: ( %lx ) - %lx->%u != %u\n",\
                         PAGE, P, P->PreviousSize, LSIZE);                    \
                DbgBreakPoint();                                              \
            }                                                                 \
            LSIZE = P->BlockSize;                                             \
            SIZE += LSIZE;                                                    \
            P = (PPOOL_HEADER)((PPOOL_BLOCK)P + LSIZE);                       \
        } while ((SIZE < (PAGE_SIZE / POOL_SMALLEST_BLOCK)) &&                \
                 (PAGE_END(P) == FALSE));                                     \
        if ((PAGE_END(P) == FALSE) || (FOUND == FALSE)) {                     \
            DbgPrint("POOL: Inconsistent page: %lx\n",P);                     \
            DbgBreakPoint();                                                  \
        }                                                                     \
    }

#endif



//
// Define forward referenced function prototypes.
//

VOID
ExpInitializePoolDescriptor(
    IN PPOOL_DESCRIPTOR PoolDescriptor,
    IN POOL_TYPE PoolType,
    IN ULONG PoolIndex,
    IN ULONG Threshold,
    IN PVOID PoolLock
    );

NTSTATUS
ExpSnapShotPoolPages(
    IN PVOID Address,
    IN ULONG Size,
    IN OUT PSYSTEM_POOL_INFORMATION PoolInformation,
    IN OUT PSYSTEM_POOL_ENTRY *PoolEntryInfo,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, InitializePool)
#pragma alloc_text(PAGE, ExpInitializePoolDescriptor)
#pragma alloc_text(PAGEVRFY, ExAllocatePoolSanityChecks)
#pragma alloc_text(PAGEVRFY, ExFreePoolSanityChecks)
#pragma alloc_text(POOLCODE, ExAllocatePoolWithTag)
#pragma alloc_text(POOLCODE, ExFreePool)
#pragma alloc_text(POOLCODE, ExFreePoolWithTag)
#if DBG
#pragma alloc_text(PAGELK, ExSnapShotPool)
#pragma alloc_text(PAGELK, ExpSnapShotPoolPages)
#endif // DBG
#endif

#define MAX_TRACKER_TABLE   1025
#define MAX_BIGPAGE_TABLE   4096
// #define MAX_TRACKER_TABLE   5
// #define MAX_BIGPAGE_TABLE   4

PPOOL_DESCRIPTOR ExpSessionPoolDescriptor;
ULONG FirstPrint;

PPOOL_TRACKER_TABLE PoolTrackTable;
SIZE_T PoolTrackTableSize;
SIZE_T PoolTrackTableMask;

PPOOL_TRACKER_BIG_PAGES PoolBigPageTable;
SIZE_T PoolBigPageTableSize;
SIZE_T PoolBigPageTableHash;

ULONG PoolHitTag = 0xffffff0f;

VOID
ExpInsertPoolTracker (
    IN ULONG Key,
    IN SIZE_T Size,
    IN POOL_TYPE PoolType
    );

VOID
ExpRemovePoolTracker (
    IN ULONG Key,
    IN ULONG Size,
    IN POOL_TYPE PoolType
    );

LOGICAL
ExpAddTagForBigPages (
    IN PVOID Va,
    IN ULONG Key,
    IN ULONG NumberOfPages
    );

ULONG
ExpFindAndRemoveTagBigPages (
    IN PVOID Va
    );

PVOID
ExpAllocateStringRoutine(
    IN SIZE_T NumberOfBytes
    )
{
    return ExAllocatePoolWithTag(PagedPool,NumberOfBytes,'grtS');
}

BOOLEAN
ExOkayToLockRoutine(
    IN PVOID Lock
    )
{
    UNREFERENCED_PARAMETER (Lock);

    if (KeIsExecutingDpc()) {
        return FALSE;
    } else {
        return TRUE;
    }
}

PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = ExpAllocateStringRoutine;
PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = (PRTL_FREE_STRING_ROUTINE)ExFreePool;

//
// Define macros to pack and unpack a pool index.
//

#define ENCODE_POOL_INDEX(POOLHEADER,INDEX) {(POOLHEADER)->PoolIndex = (UCHAR)((((POOLHEADER)->PoolIndex & 0x80) | ((UCHAR)(INDEX))));}
#define DECODE_POOL_INDEX(POOLHEADER)       ((ULONG)((POOLHEADER)->PoolIndex & 0x7f))

#define MARK_POOL_HEADER_ALLOCATED(POOLHEADER)      {(POOLHEADER)->PoolIndex |= 0x80;}
#define MARK_POOL_HEADER_FREED(POOLHEADER)          {(POOLHEADER)->PoolIndex &= 0x7f;}
#define IS_POOL_HEADER_MARKED_ALLOCATED(POOLHEADER) ((POOLHEADER)->PoolIndex & 0x80)

//
// Define the number of paged pools. This value may be overridden at boot
// time.
//

ULONG ExpNumberOfPagedPools = NUMBER_OF_PAGED_POOLS;

//
// Pool descriptors for nonpaged pool and nonpaged pool must succeed are
// static. The pool descriptors for paged pool are dynamically allocated
// since there can be more than one paged pool. There is always one more
// paged pool descriptor than there are paged pools. This descriptor is
// used when a page allocation is done for a paged pool and is the first
// descriptor in the paged pool descriptor array.
//

POOL_DESCRIPTOR NonPagedPoolDescriptor;
POOL_DESCRIPTOR NonPagedPoolDescriptorMS;

//
// The pool vector contains an array of pointers to pool descriptors. For
// nonpaged pool and nonpaged pool must succeed, this is a pointer to a
// single descriptor. For page pool, this is a pointer to an array of pool
// descriptors. The pointer to the paged pool descriptor is duplicated so
// if can be found easily by the kernel debugger.
//

PPOOL_DESCRIPTOR PoolVector[NUMBER_OF_POOLS];
PPOOL_DESCRIPTOR ExpPagedPoolDescriptor;

extern KSPIN_LOCK NonPagedPoolLock;

#define ExpLockNonPagedPool(OldIrql) \
    ExAcquireSpinLock(&NonPagedPoolLock, &OldIrql)

#define ExpUnlockNonPagedPool(OldIrql) \
    ExReleaseSpinLock(&NonPagedPoolLock, OldIrql)

volatile ULONG ExpPoolIndex = 1;
KSPIN_LOCK ExpTaggedPoolLock;

#if DBG
PSZ PoolTypeNames[MaxPoolType] = {
    "NonPaged",
    "Paged",
    "NonPagedMustSucceed",
    "NotUsed",
    "NonPagedCacheAligned",
    "PagedCacheAligned",
    "NonPagedCacheAlignedMustS"
    };

#endif //DBG


//
// Define paged and nonpaged pool lookaside descriptors.
//

NPAGED_LOOKASIDE_LIST ExpSmallNPagedPoolLookasideLists[POOL_SMALL_LISTS];

NPAGED_LOOKASIDE_LIST ExpSmallPagedPoolLookasideLists[POOL_SMALL_LISTS];


//
// LOCK_POOL and LOCK_IF_PAGED_POOL are only used within this module.
//

#define LOCK_POOL(PoolDesc, LockHandle) {                                      \
    if ((PoolDesc->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {          \
        ExpLockNonPagedPool(LockHandle);                                       \
    } else {                                                                   \
        ExAcquireFastMutex((PFAST_MUTEX)PoolDesc->LockAddress);                \
    }                                                                          \
}

#define LOCK_IF_PAGED_POOL(_CheckType, _GlobalPool)                            \
    if (_CheckType == PagedPool && _GlobalPool == TRUE) {                      \
        ExAcquireFastMutex((PFAST_MUTEX)PoolVector[PagedPool]->LockAddress);   \
    }

KIRQL
ExLockPool(
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function locks the pool specified by pool type.

Arguments:

    PoolType - Specifies the pool that should be locked.

Return Value:

    The previous IRQL is returned as the function value.

--*/

{

    KIRQL OldIrql;

    //
    // If the pool type is nonpaged, then use a spinlock to lock the
    // pool. Otherwise, use a fast mutex to lock the pool.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ExAcquireSpinLock(NonPagedPoolDescriptor.LockAddress, &OldIrql);

    } else {
        ExAcquireFastMutex((PFAST_MUTEX)PoolVector[PagedPool]->LockAddress);
        OldIrql = (KIRQL)((PFAST_MUTEX)(PoolVector[PagedPool]->LockAddress))->OldIrql;
    }

    return OldIrql;
}


//
// UNLOCK_POOL and UNLOCK_IF_PAGED_POOL are only used within this module.
//

#define UNLOCK_POOL(PoolDesc, LockHandle) {                                    \
    if ((PoolDesc->PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {          \
        ExpUnlockNonPagedPool((KIRQL)LockHandle);                              \
    } else {                                                                   \
        ExReleaseFastMutex((PFAST_MUTEX)PoolDesc->LockAddress);                \
    }                                                                          \
}

#define UNLOCK_IF_PAGED_POOL(_CheckType, _GlobalPool)                          \
    if (_CheckType == PagedPool && _GlobalPool == TRUE) {                      \
        ExReleaseFastMutex((PFAST_MUTEX)PoolVector[PagedPool]->LockAddress);   \
    }

VOID
ExUnlockPool(
    IN POOL_TYPE PoolType,
    IN KIRQL LockHandle
    )

/*++

Routine Description:

    This function unlocks the pool specified by pool type.


Arguments:

    PoolType - Specifies the pool that should be unlocked.

    LockHandle - Specifies the lock handle from a previous call to
                 ExLockPool.

Return Value:

    None.

--*/

{

    //
    // If the pool type is nonpaged, then use a spinlock to unlock the
    // pool. Otherwise, use a fast mutex to unlock the pool.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ExReleaseSpinLock(&NonPagedPoolLock, LockHandle);

    } else {
        ExReleaseFastMutex((PFAST_MUTEX)PoolVector[PagedPool]->LockAddress);
    }

    return;
}


VOID
ExpInitializePoolDescriptor(
    IN PPOOL_DESCRIPTOR PoolDescriptor,
    IN POOL_TYPE PoolType,
    IN ULONG PoolIndex,
    IN ULONG Threshold,
    IN PVOID PoolLock
    )

/*++

Routine Description:

    This function initializes a pool descriptor.

    Note that this routine is called directly by the memory manager.

Arguments:

    PoolDescriptor - Supplies a pointer to the pool descriptor.

    PoolType - Supplies the type of the pool.

    PoolIndex - Supplies the pool descriptor index.

    Threshold - Supplies the threshold value for the specified pool.

    PoolLock - Supplies a point to the lock for the specified pool.

Return Value:

    None.

--*/

{

    ULONG Index;

    //
    // Initialize statistics fields, the pool type, the threshold value,
    // and the lock address.
    //

    PoolDescriptor->PoolType = PoolType;
    PoolDescriptor->PoolIndex = PoolIndex;
    PoolDescriptor->RunningAllocs = 0;
    PoolDescriptor->RunningDeAllocs = 0;
    PoolDescriptor->TotalPages = 0;
    PoolDescriptor->TotalBigPages = 0;
    PoolDescriptor->Threshold = Threshold;
    PoolDescriptor->LockAddress = PoolLock;

    //
    // Initialize the allocation listheads.
    //

    for (Index = 0; Index < POOL_LIST_HEADS; Index += 1) {
        PrivateInitializeListHead(&PoolDescriptor->ListHeads[Index]);
    }

    if ((PoolType == PagedPoolSession) && (ExpSessionPoolDescriptor == NULL)) {
        ExpSessionPoolDescriptor = (PPOOL_DESCRIPTOR) MiSessionPoolVector ();
    }

    return;
}

VOID
InitializePool(
    IN POOL_TYPE PoolType,
    IN ULONG Threshold
    )

/*++

Routine Description:

    This procedure initializes a pool descriptor for the specified pool
    type.  Once initialized, the pool may be used for allocation and
    deallocation.

    This function should be called once for each base pool type during
    system initialization.

    Each pool descriptor contains an array of list heads for free
    blocks.  Each list head holds blocks which are a multiple of
    the POOL_BLOCK_SIZE.  The first element on the list [0] links
    together free entries of size POOL_BLOCK_SIZE, the second element
    [1] links together entries of POOL_BLOCK_SIZE * 2, the third
    POOL_BLOCK_SIZE * 3, etc, up to the number of blocks which fit
    into a page.

Arguments:

    PoolType - Supplies the type of pool being initialized (e.g.
               nonpaged pool, paged pool...).

    Threshold - Supplies the threshold value for the specified pool.

Return Value:

    None.

--*/

{

    PPOOL_DESCRIPTOR Descriptor;
    ULONG Index;
    PFAST_MUTEX FastMutex;
    SIZE_T Size;

    ASSERT((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0);

    if (PoolType == NonPagedPool) {

        //
        // Initialize nonpaged pools.
        //

#if !DBG
        if (NtGlobalFlag & FLG_POOL_ENABLE_TAGGING) {
#endif  //!DBG
            PoolTrackTableSize = MAX_TRACKER_TABLE;
            PoolTrackTableMask = PoolTrackTableSize - 2;
            PoolTrackTable = MiAllocatePoolPages(NonPagedPool,
                                                 PoolTrackTableSize *
                                                   sizeof(POOL_TRACKER_TABLE),
                                                 FALSE);

            RtlZeroMemory(PoolTrackTable, PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE));

            PoolBigPageTableSize = MAX_BIGPAGE_TABLE;
            PoolBigPageTableHash = PoolBigPageTableSize - 1;
            PoolBigPageTable = MiAllocatePoolPages(NonPagedPool,
                                                   PoolBigPageTableSize *
                                                     sizeof(POOL_TRACKER_BIG_PAGES),
                                                   FALSE);

            RtlZeroMemory(PoolBigPageTable, PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES));
#if !DBG
        }
#endif  //!DBG

        //
        // Initialize the spinlocks for nonpaged pool.
        //

        KeInitializeSpinLock (&ExpTaggedPoolLock);
        KeInitializeSpinLock(&NonPagedPoolLock);

        //
        // Initialize the nonpaged pool descriptor.
        //

        PoolVector[NonPagedPool] = &NonPagedPoolDescriptor;
        ExpInitializePoolDescriptor(&NonPagedPoolDescriptor,
                                    NonPagedPool,
                                    0,
                                    Threshold,
                                    (PVOID)&NonPagedPoolLock);

        //
        // Initialize the nonpaged must succeed pool descriptor.
        //

        PoolVector[NonPagedPoolMustSucceed] = &NonPagedPoolDescriptorMS;
        ExpInitializePoolDescriptor(&NonPagedPoolDescriptorMS,
                                    NonPagedPoolMustSucceed,
                                    0,
                                    0,
                                    (PVOID)&NonPagedPoolLock);

    } else {

        //
        // Allocate memory for the paged pool descriptors and fast mutexes.
        //

        Size = (ExpNumberOfPagedPools + 1) * (sizeof(FAST_MUTEX) + sizeof(POOL_DESCRIPTOR));
        Descriptor = (PPOOL_DESCRIPTOR)ExAllocatePoolWithTag (NonPagedPool,
                                                              Size,
                                                              'looP');
        if (Descriptor == NULL) {
            KeBugCheckEx (MUST_SUCCEED_POOL_EMPTY,
                          Size,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)-1);
        }

        if (PoolTrackTable) {
            ExpInsertPoolTracker('looP',
                                  (ULONG) ROUND_TO_PAGES(PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE)),
                                 NonPagedPool);

            ExpInsertPoolTracker('looP',
                                  (ULONG) ROUND_TO_PAGES(PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES)),
                                 NonPagedPool);
        }

        FastMutex = (PFAST_MUTEX)(Descriptor + ExpNumberOfPagedPools + 1);
        PoolVector[PagedPool] = Descriptor;
        ExpPagedPoolDescriptor = Descriptor;
        for (Index = 0; Index < (ExpNumberOfPagedPools + 1); Index += 1) {
            ExInitializeFastMutex(FastMutex);
            ExpInitializePoolDescriptor(Descriptor,
                                        PagedPool,
                                        Index,
                                        Threshold,
                                        (PVOID)FastMutex);

            Descriptor += 1;
            FastMutex += 1;
        }
    }

    //
    // The maximum cache alignment must be less than the size of the
    // smallest pool block because the lower bits are being cleared
    // in ExFreePool to find the entry's address.
    //

#if POOL_CACHE_SUPPORTED

    //
    // Compute pool cache information.
    //

    PoolCacheSize = HalGetDmaAlignmentRequirement();

    ASSERT(PoolCacheSize >= POOL_OVERHEAD);

    PoolCacheOverhead = PoolCacheSize + PoolCacheSize - (sizeof(POOL_HEADER) + 1);

    PoolBuddyMax =
       (POOL_PAGE_SIZE - (POOL_OVERHEAD + (3*POOL_SMALLEST_BLOCK) + 2*PoolCacheSize));

#endif //POOL_CACHE_SUPPORTED

}


LOGICAL
ExpCheckSingleFilter (
    ULONG Tag,
    ULONG Filter
    )

/*++

Routine Description:

    This function checks if a pool tag matches a given pattern.  Pool
    protection is ignored in the tag.

        ? - matches a single character
        * - terminates match with TRUE

    N.B.: ability inspired by the !poolfind debugger extension.

Arguments:

    Tag - a pool tag

    Filter - a globish pattern (chars and/or ?,*)

Return Value:

    TRUE if a match exists, FALSE otherwise.

--*/

{
    ULONG i;
    PUCHAR tc;
    PUCHAR fc;

    tc = (PUCHAR) &Tag;
    fc = (PUCHAR) &Filter;

    for (i = 0; i < 4; i += 1, tc += 1, fc += 1) {

        if (*fc == '*') {
            break;
        }
        if (*fc == '?') {
            continue;
        }
        if (i == 3 && ((*tc) & ~(PROTECTED_POOL >> 24)) == *fc) {
            continue;
        }
        if (*tc != *fc) {
            return FALSE;
        }
    }
    return TRUE;
}


PVOID
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type and
    returns a pointer to the allocated block.  This function is used to
    access both the page-aligned pools, and the list head entries (less than
    a page) pools.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate list, then the page-aligned
    pool allocator is used.  The allocated block will be page-aligned
    and a page-sized multiple.

    Otherwise, the appropriate pool list entry is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  The
    pool allocator calculates the smallest number of POOL_BLOCK_SIZE
    that can be used to satisfy the request.  If there are no blocks
    available of this size, then a block of the next larger block size
    is allocated and split.  One piece is placed back into the pool, and
    the other piece is used to satisfy the request.  If the allocator
    reaches the paged-sized block list, and nothing is there, the
    page-aligned pool allocator is called.  The page is split and added
    to the pool...

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types, then this call will
        always succeed and return a pointer to allocated pool.
        Otherwise, if the system cannot allocate the requested amount
        of memory a NULL is returned.

        Valid pool types:

        NonPagedPool
        PagedPool
        NonPagedPoolMustSucceed,
        NonPagedPoolCacheAligned
        PagedPoolCacheAligned
        NonPagedPoolCacheAlignedMustS

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NULL - The PoolType is not one of the "MustSucceed" pool types, and
        not enough pool exists to satisfy the request.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    return ExAllocatePoolWithTag (PoolType,
                                  NumberOfBytes,
                                  'enoN');
}

#define _NTOSKRNL_VERIFIER_ 1       // LWFIX: disable for ship

#ifdef _NTOSKRNL_VERIFIER_

PVOID
VeAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority,
    IN PVOID CallingAddress
    );

extern LOGICAL KernelVerifier;

#endif


PVOID
ExAllocatePoolWithTagPriority(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN EX_POOL_PRIORITY Priority
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type and
    returns a pointer to the allocated block.  This function is used to
    access both the page-aligned pools, and the list head entries (less than
    a page) pools.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate list, then the page-aligned
    pool allocator is used.  The allocated block will be page-aligned
    and a page-sized multiple.

    Otherwise, the appropriate pool list entry is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  The
    pool allocator calculates the smallest number of POOL_BLOCK_SIZE
    that can be used to satisfy the request.  If there are no blocks
    available of this size, then a block of the next larger block size
    is allocated and split.  One piece is placed back into the pool, and
    the other piece is used to satisfy the request.  If the allocator
    reaches the paged-sized block list, and nothing is there, the
    page-aligned pool allocator is called.  The page is split and added
    to the pool...

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types, then this call will
        always succeed and return a pointer to allocated pool.
        Otherwise, if the system cannot allocate the requested amount
        of memory a NULL is returned.

        Valid pool types:

        NonPagedPool
        PagedPool
        NonPagedPoolMustSucceed,
        NonPagedPoolCacheAligned
        PagedPoolCacheAligned
        NonPagedPoolCacheAlignedMustS

    NumberOfBytes - Supplies the number of bytes to allocate.

    Tag - Supplies the caller's identifying tag.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available pool conditions.  This
               can also be used to specify special pool.

Return Value:

    NULL - The PoolType is not one of the "MustSucceed" pool types, and
        not enough pool exists to satisfy the request.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    PVOID Entry;

    if ((Priority & POOL_SPECIAL_POOL_BIT) && (NumberOfBytes <= POOL_BUDDY_MAX)) {
        Entry = MmAllocateSpecialPool (NumberOfBytes,
                                       Tag,
                                       PoolType & (BASE_POOL_TYPE_MASK | POOL_VERIFIER_MASK),
                                       (Priority & POOL_SPECIAL_POOL_UNDERRUN_BIT) ? 1 : 0);

        if (Entry != NULL) {
            return Entry;
        }
        Priority &= ~(POOL_SPECIAL_POOL_BIT | POOL_SPECIAL_POOL_UNDERRUN_BIT);
    }

    //
    // Pool and other resources can be allocated directly through the Mm
    // without the pool code knowing - so always call the Mm for the
    // up-to-date counters.
    //

    if ((Priority != HighPoolPriority) && ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0)) {

        if (ExpSessionPoolDescriptor == NULL) {
            PoolType &= ~SESSION_POOL_MASK;
        }

        if (MmResourcesAvailable (PoolType, NumberOfBytes, Priority) == FALSE) {
            return NULL;
        }
    }

    //
    // There is a window between determining whether to proceed and actually
    // doing the allocation.  In this window the pool may deplete.  This is not
    // worth closing at this time.
    //

    return ExAllocatePoolWithTag (PoolType,
                                  NumberOfBytes,
                                  Tag);
}


PVOID
ExAllocatePoolWithTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type and
    returns a pointer to the allocated block. This function is used to
    access both the page-aligned pools and the list head entries (less
    than a page) pools.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate list, then the page-aligned pool
    allocator is used. The allocated block will be page-aligned and a
    page-sized multiple.

    Otherwise, the appropriate pool list entry is used. The allocated
    block will be 64-bit aligned, but will not be page aligned. The
    pool allocator calculates the smallest number of POOL_BLOCK_SIZE
    that can be used to satisfy the request. If there are no blocks
    available of this size, then a block of the next larger block size
    is allocated and split. One piece is placed back into the pool, and
    the other piece is used to satisfy the request. If the allocator
    reaches the paged-sized block list, and nothing is there, the
    page-aligned pool allocator is called. The page is split and added
    to the pool.

Arguments:

    PoolType - Supplies the type of pool to allocate. If the pool type
        is one of the "MustSucceed" pool types, then this call will
        always succeed and return a pointer to allocated pool. Otherwise,
        if the system cannot allocate the requested amount of memory a
        NULL is returned.

        Valid pool types:

        NonPagedPool
        PagedPool
        NonPagedPoolMustSucceed,
        NonPagedPoolCacheAligned
        PagedPoolCacheAligned
        NonPagedPoolCacheAlignedMustS

    Tag - Supplies the caller's identifying tag.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NULL - The PoolType is not one of the "MustSucceed" pool types, and
        not enough pool exists to satisfy the request.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    PVOID Block;
    PPOOL_HEADER Entry;
    PNPAGED_LOOKASIDE_LIST LookasideList;
    PPOOL_HEADER NextEntry;
    PPOOL_HEADER SplitEntry;
    KIRQL LockHandle;
    PPOOL_DESCRIPTOR PoolDesc;
    ULONG Index;
    ULONG ListNumber;
    ULONG NeededSize;
    ULONG PoolIndex;
    POOL_TYPE CheckType;
    POOL_TYPE RequestType;
    PLIST_ENTRY ListHead;
    POOL_TYPE NewPoolType;
    LOGICAL GlobalSpace;
    ULONG IsLargeSessionAllocation;
    PKPRCB Prcb;
    ULONG NumberOfPages;
    PVOID CallingAddress;
    PVOID CallersCaller;

#if POOL_CACHE_SUPPORTED
    ULONG CacheOverhead;
#else
#define CacheOverhead POOL_OVERHEAD
#endif

    PERFINFO_EXALLOCATEPOOLWITHTAG_DECL();

#ifdef _NTOSKRNL_VERIFIER_

    if (KernelVerifier == TRUE) {
#if defined (_X86_)
        RtlGetCallersAddress(&CallingAddress, &CallersCaller);
#else
        CallingAddress = (PVOID)_ReturnAddress();
#endif

        ASSERT(NumberOfBytes != 0);
        ASSERT_ALLOCATE_IRQL(PoolType, NumberOfBytes);

        if ((PoolType & POOL_DRIVER_MASK) == 0) {

            //
            // Use the Driver Verifier pool framework.  Note this will
            // result in a recursive callback to this routine.
            //

            return VeAllocatePoolWithTagPriority (PoolType | POOL_DRIVER_MASK,
                                                  NumberOfBytes,
                                                  Tag,
                                                  HighPoolPriority,
                                                  CallingAddress);
        }
        PoolType &= ~POOL_DRIVER_MASK;
    }

#else

    ASSERT(NumberOfBytes != 0);
    ASSERT_ALLOCATE_IRQL(PoolType, NumberOfBytes);

#endif

    //
    // Isolate the base pool type and select a pool from which to allocate
    // the specified block size.
    //

    CheckType = PoolType & BASE_POOL_TYPE_MASK;

    //
    // Currently only Hydra paged pool allocations come from the per session
    // pools.  Nonpaged Hydra pool allocations still come from global pool.
    //

    if (PoolType & SESSION_POOL_MASK) {
        if (ExpSessionPoolDescriptor == NULL) {

            //
            // Promote this down to support common binaries.
            //

            PoolType &= ~SESSION_POOL_MASK;
            PoolDesc = PoolVector[CheckType];
            GlobalSpace = TRUE;
        }
        else {
            GlobalSpace = FALSE;
            if (CheckType == NonPagedPool) {
                PoolDesc = PoolVector[CheckType];
            }
            else {
                PoolDesc = ExpSessionPoolDescriptor;
            }
        }
    }
    else {
        PoolDesc = PoolVector[CheckType];
        GlobalSpace = TRUE;
    }

    //
    // Check to determine if the requested block can be allocated from one
    // of the pool lists or must be directly allocated from virtual memory.
    //

    if (NumberOfBytes > POOL_BUDDY_MAX) {

        //
        // The requested size is greater than the largest block maintained
        // by allocation lists.
        //

        ASSERT ((NumberOfBytes <= PAGE_SIZE) ||
                (ExpPagedPoolDescriptor == (PPOOL_DESCRIPTOR)0) ||
                ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0));

        LOCK_POOL(PoolDesc, LockHandle);

        PoolDesc->RunningAllocs += 1;

        IsLargeSessionAllocation = (PoolType & SESSION_POOL_MASK);

        RequestType = (PoolType & (BASE_POOL_TYPE_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK));

RetryWithMustSucceed:
        Entry = (PPOOL_HEADER) MiAllocatePoolPages (RequestType,
                                                    NumberOfBytes,
                                                    IsLargeSessionAllocation);

        //
        // Large session pool allocations are accounted for directly by
        // the memory manager so no need to call MiSessionPoolAllocated here.
        //

        if (Entry != NULL) {

            NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);
            PoolDesc->TotalBigPages += NumberOfPages;

            UNLOCK_POOL(PoolDesc, LockHandle);

            if ((PoolBigPageTable) && (IsLargeSessionAllocation == 0)) {

                if (ExpAddTagForBigPages((PVOID)Entry,
                                         Tag,
                                         NumberOfPages) == FALSE) {
                    Tag = ' GIB';
                }

                ExpInsertPoolTracker (Tag,
                                      (ULONG) ROUND_TO_PAGES(NumberOfBytes),
                                      PoolType);
            }

        } else {
            if (PoolType & MUST_SUCCEED_POOL_TYPE_MASK) {
                RequestType |= MUST_SUCCEED_POOL_TYPE_MASK;
                goto RetryWithMustSucceed;
            }

            UNLOCK_POOL(PoolDesc, LockHandle);

            KdPrint(("EX: ExAllocatePool (%p, 0x%x ) returning NULL\n",
                NumberOfBytes,
                PoolType));

            if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
                ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
            }
        }

        PERFINFO_BIGPOOLALLOC(PoolType, Tag, NumberOfBytes, Entry);

        return Entry;
    }

    //
    // The requested size is less than or equal to the size of the
    // maximum block maintained by the allocation lists.
    //

    PERFINFO_POOLALLOC(PoolType, Tag, NumberOfBytes);

    //
    // Check for a special pool tag match by actual tag.
    //

    if (MmSpecialPoolTag != 0 && NumberOfBytes != 0) {

#ifndef NO_POOL_CHECKS
        Entry = MmSqueezeBadTags (NumberOfBytes, Tag, CheckType, 2);
        if (Entry != NULL) {
            return (PVOID)Entry;
        }
#endif

        //
        // Check for a special pool tag match by tag string and size ranges.
        //

        if ((ExpCheckSingleFilter(Tag, MmSpecialPoolTag)) ||
            ((MmSpecialPoolTag >= (NumberOfBytes + POOL_OVERHEAD)) &&
             (MmSpecialPoolTag < (NumberOfBytes + POOL_OVERHEAD + POOL_SMALLEST_BLOCK)))) {

            Entry = MmAllocateSpecialPool (NumberOfBytes,
                                           Tag,
                                           PoolType & (BASE_POOL_TYPE_MASK | POOL_VERIFIER_MASK),
                                           2);
            if (Entry != NULL) {
                return (PVOID)Entry;
            }
        }
    }

    //
    // If the request is for cache aligned memory adjust the number of
    // bytes.
    //

#if POOL_CACHE_SUPPORTED

    CacheOverhead = POOL_OVERHEAD;
    if (PoolType & CACHE_ALIGNED_POOL_TYPE_MASK) {
        NumberOfBytes += PoolCacheOverhead;
        CacheOverhead = PoolCacheSize;
    }

#endif //POOL_CACHE_SUPPORTED

    //
    // Compute the Index of the listhead for blocks of the requested
    // size.
    //

    ListNumber = (ULONG)((NumberOfBytes + POOL_OVERHEAD + (POOL_SMALLEST_BLOCK - 1)) >> POOL_BLOCK_SHIFT);

    NeededSize = ListNumber;

    //
    // If the pool type is paged, then pick a starting pool number and
    // attempt to lock each paged pool in circular succession. Otherwise,
    // lock the nonpaged pool as the same lock is used for both nonpaged
    // and nonpaged must succeed.
    //
    // N.B. The paged pool is selected in a round robin fashion using a
    //      simple counter. Note that the counter is incremented using a
    //      a noninterlocked sequence, but the pool index is never allowed
    //      to get out of range.
    //

    if (CheckType == PagedPool) {

        //
        // If the requested pool block is a small block, then attempt to
        // allocate the requested pool from the per processor lookaside
        // list. If the attempt fails, then attempt to allocate from the
        // system lookaside list. If the attempt fails, then select a
        // pool to allocate from and allocate the block normally.
        //
        // Session space allocations do not currently use lookaside lists.
        //

        if ((GlobalSpace == TRUE) &&
            (NeededSize <= POOL_SMALL_LISTS) &&
            (Isx86FeaturePresent(KF_CMPXCHG8B))) {

            Prcb = KeGetCurrentPrcb();
            LookasideList = Prcb->PPPagedLookasideList[NeededSize - 1].P;
            LookasideList->L.TotalAllocates += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

            Entry = (PPOOL_HEADER)
                ExInterlockedPopEntrySList (&LookasideList->L.ListHead,
                                            &LookasideList->Lock);

            if (Entry == NULL) {
                LookasideList = Prcb->PPPagedLookasideList[NeededSize - 1].L;
                LookasideList->L.TotalAllocates += 1;

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

                Entry = (PPOOL_HEADER)
                    ExInterlockedPopEntrySList (&LookasideList->L.ListHead,
                                                &LookasideList->Lock);
            }

            if (Entry != NULL) {

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

                Entry -= 1;
                LookasideList->L.AllocateHits += 1;
                NewPoolType = (PoolType & (BASE_POOL_TYPE_MASK | POOL_QUOTA_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK)) + 1;

#if _POOL_LOCK_GRANULAR_
                PoolDesc = &PoolDesc[DECODE_POOL_INDEX(Entry)];
#endif

                LOCK_POOL_GRANULAR(PoolDesc, LockHandle);

                Entry->PoolType = (UCHAR)NewPoolType;
                MARK_POOL_HEADER_ALLOCATED(Entry);

                UNLOCK_POOL_GRANULAR(PoolDesc, LockHandle);

                Entry->PoolTag = Tag;

                if ((PoolTrackTable != NULL) &&
                    ((PoolType & SESSION_POOL_MASK) == 0)) {

                    ExpInsertPoolTracker (Tag,
                                          Entry->BlockSize << POOL_BLOCK_SHIFT,
                                          PoolType);
                }

                //
                // Zero out any back pointer to our internal structures
                // to stop someone from corrupting us via an
                // uninitialized pointer.
                //

                ((PULONG)((PCHAR)Entry + CacheOverhead))[0] = 0;

                PERFINFO_POOLALLOC_ADDR((PUCHAR)Entry + CacheOverhead);

                return (PUCHAR)Entry + CacheOverhead;
            }
        }

        //
        // If there is more than one paged pool, then attempt to find
        // one that can be immediately locked.
        //

        if (GlobalSpace == TRUE) {

            PVOID Lock;

            PoolIndex = 1;
            if (ExpNumberOfPagedPools != PoolIndex) {
                ExpPoolIndex += 1;
                PoolIndex = ExpPoolIndex;
                if (PoolIndex > ExpNumberOfPagedPools) {
                    PoolIndex = 1;
                    ExpPoolIndex = 1;
                }

                Index = PoolIndex;
                do {
                    Lock = PoolDesc[PoolIndex].LockAddress;
                    if (ExTryToAcquireFastMutex((PFAST_MUTEX)Lock) == TRUE) {
                        PoolDesc = &PoolDesc[PoolIndex];
                        goto PoolLocked;
                    }

                    PoolIndex += 1;
                    if (PoolIndex > ExpNumberOfPagedPools) {
                        PoolIndex = 1;
                    }

                } while (PoolIndex != Index);
            }
            PoolDesc = &PoolDesc[PoolIndex];
        }
        else {

            //
            // Only one paged pool is currently available per session.
            //

            PoolIndex = 0;
            ASSERT (PoolDesc == ExpSessionPoolDescriptor);
        }

        //
        // None of the paged pools could be conditionally locked or there
        // is only one paged pool. The first pool considered is picked as
        // the victim to wait on.
        //

        ExAcquireFastMutex((PFAST_MUTEX)PoolDesc->LockAddress);
PoolLocked:

        GlobalSpace = GlobalSpace;
#if DBG
        if (GlobalSpace == TRUE) {
            ASSERT(PoolIndex == PoolDesc->PoolIndex);
        }
        else {
            ASSERT(PoolIndex == 0);
            ASSERT(PoolDesc->PoolIndex == 0);
        }
#endif

    } else {

        //
        // If the requested pool block is a small block, then attempt to
        // allocate the requested pool from the per processor lookaside
        // list. If the attempt fails, then attempt to allocate from the
        // system lookaside list. If the attempt fails, then select a
        // pool to allocate from and allocate the block normally.
        //

        if (GlobalSpace == TRUE && NeededSize <= POOL_SMALL_LISTS) {
            Prcb = KeGetCurrentPrcb();
            LookasideList = Prcb->PPNPagedLookasideList[NeededSize - 1].P;
            LookasideList->L.TotalAllocates += 1;

            CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, 0);

            Entry = (PPOOL_HEADER)
                        ExInterlockedPopEntrySList (&LookasideList->L.ListHead,
                                                    &LookasideList->Lock);

            if (Entry == NULL) {
                LookasideList = Prcb->PPNPagedLookasideList[NeededSize - 1].L;
                LookasideList->L.TotalAllocates += 1;

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, 0);

                Entry = (PPOOL_HEADER)
                        ExInterlockedPopEntrySList (&LookasideList->L.ListHead,
                                                    &LookasideList->Lock);
            }

            if (Entry != NULL) {

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, Entry);

                Entry -= 1;
                LookasideList->L.AllocateHits += 1;
                NewPoolType = (PoolType & (BASE_POOL_TYPE_MASK | POOL_QUOTA_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK)) + 1;

                LOCK_POOL_GRANULAR(PoolDesc, LockHandle);

                Entry->PoolType = (UCHAR)NewPoolType;
                MARK_POOL_HEADER_ALLOCATED(Entry);

                UNLOCK_POOL_GRANULAR(PoolDesc, LockHandle);

                Entry->PoolTag = Tag;

                if (PoolTrackTable != NULL) {

                    ExpInsertPoolTracker (Tag,
                                          Entry->BlockSize << POOL_BLOCK_SHIFT,
                                          PoolType);
                }

                //
                // Zero out any back pointer to our internal structures
                // to stop someone from corrupting us via an
                // uninitialized pointer.
                //

                ((PULONG)((PCHAR)Entry + CacheOverhead))[0] = 0;

                PERFINFO_POOLALLOC_ADDR((PUCHAR)Entry + CacheOverhead);

                return (PUCHAR)Entry + CacheOverhead;
            }
        }

        PoolIndex = 0;
        ExAcquireSpinLock(&NonPagedPoolLock, &LockHandle);

        ASSERT(PoolIndex == PoolDesc->PoolIndex);
    }

    //
    // The following code has an outer loop and an inner loop.
    //
    // The outer loop is utilized to repeat a nonpaged must succeed
    // allocation if necessary.
    //
    // The inner loop is used to repeat an allocation attempt if there
    // are no entries in any of the pool lists.
    //

    RequestType = PoolType & (BASE_POOL_TYPE_MASK | SESSION_POOL_MASK);

    PoolDesc->RunningAllocs += 1;
    ListHead = &PoolDesc->ListHeads[ListNumber];

    do {

        //
        // Attempt to allocate the requested block from the current free
        // blocks.
        //

        do {

            //
            // If the list is not empty, then allocate a block from the
            // selected list.
            //

            if (PrivateIsListEmpty(ListHead) == FALSE) {

                CHECK_LIST( __LINE__, ListHead, 0 );
                Block = PrivateRemoveHeadList(ListHead);
                CHECK_LIST( __LINE__, ListHead, 0 );
                Entry = (PPOOL_HEADER)((PCHAR)Block - POOL_OVERHEAD);

                ASSERT(Entry->BlockSize >= NeededSize);

                ASSERT(DECODE_POOL_INDEX(Entry) == PoolIndex);

                ASSERT(Entry->PoolType == 0);

                if (Entry->BlockSize != NeededSize) {

                    //
                    // The selected block is larger than the allocation
                    // request. Split the block and insert the remaining
                    // fragment in the appropriate list.
                    //
                    // If the entry is at the start of a page, then take
                    // the allocation from the front of the block so as
                    // to minimize fragmentation. Otherwise, take the
                    // allocation from the end of the block which may
                    // also reduce fragmentation if the block is at the
                    // end of a page.
                    //

                    if (Entry->PreviousSize == 0) {

                        //
                        // The entry is at the start of a page.
                        //

                        SplitEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);
                        SplitEntry->BlockSize = (UCHAR)(Entry->BlockSize - (UCHAR)NeededSize);
                        SplitEntry->PreviousSize = (UCHAR)NeededSize;

                        //
                        // If the allocated block is not at the end of a
                        // page, then adjust the size of the next block.
                        //

                        NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)SplitEntry + SplitEntry->BlockSize);
                        if (PAGE_END(NextEntry) == FALSE) {
                            NextEntry->PreviousSize = SplitEntry->BlockSize;
                        }

                    } else {

                        //
                        // The entry is not at the start of a page.
                        //

                        SplitEntry = Entry;
                        Entry->BlockSize -= (UCHAR)NeededSize;
                        Entry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + Entry->BlockSize);
                        Entry->PreviousSize = SplitEntry->BlockSize;

                        //
                        // If the allocated block is not at the end of a
                        // page, then adjust the size of the next block.
                        //

                        NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);
                        if (PAGE_END(NextEntry) == FALSE) {
                            NextEntry->PreviousSize = (UCHAR)NeededSize;
                        }
                    }

                    //
                    // Set the size of the allocated entry, clear the pool
                    // type of the split entry, set the index of the split
                    // entry, and insert the split entry in the appropriate
                    // free list.
                    //

                    Entry->BlockSize = (UCHAR)NeededSize;
                    ENCODE_POOL_INDEX(Entry, PoolIndex);
                    SplitEntry->PoolType = 0;
                    ENCODE_POOL_INDEX(SplitEntry, PoolIndex);
                    Index = SplitEntry->BlockSize;

                    CHECK_LIST(__LINE__, &PoolDesc->ListHeads[Index - 1], 0);
                    PrivateInsertTailList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((PCHAR)SplitEntry + POOL_OVERHEAD)));
                    CHECK_LIST(__LINE__, &PoolDesc->ListHeads[Index - 1], 0);
                    CHECK_LIST(__LINE__, ((PLIST_ENTRY)((PCHAR)SplitEntry + POOL_OVERHEAD)), 0);
                }

                Entry->PoolType = (UCHAR)((PoolType & (BASE_POOL_TYPE_MASK | POOL_QUOTA_MASK | SESSION_POOL_MASK | POOL_VERIFIER_MASK)) + 1);

                MARK_POOL_HEADER_ALLOCATED(Entry);

                CHECK_POOL_HEADER(__LINE__, Entry);

                //
                // Notify the memory manager of session pool allocations
                // so leaked allocations can be caught on session exit.
                // This call must be made with the relevant pool locked.
                //

                if (PoolType & SESSION_POOL_MASK) {
                    MiSessionPoolAllocated(
                        (PVOID)((PCHAR)Entry + CacheOverhead),
                        (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT),
                        PoolType);
                }

                UNLOCK_POOL(PoolDesc, LockHandle);

                Entry->PoolTag = Tag;

                if ((PoolTrackTable != NULL) &&
                    ((PoolType & SESSION_POOL_MASK) == 0)) {

                    ExpInsertPoolTracker (Tag,
                                          Entry->BlockSize << POOL_BLOCK_SHIFT,
                                          PoolType);
                }

                //
                // Zero out any back pointer to our internal structures
                // to stop someone from corrupting us via an
                // uninitialized pointer.
                //

                ((PULONGLONG)((PCHAR)Entry + CacheOverhead))[0] = 0;

                PERFINFO_POOLALLOC_ADDR((PUCHAR)Entry + CacheOverhead);
                return (PCHAR)Entry + CacheOverhead;

            }
            ListHead += 1;

        } while (ListHead != &PoolDesc->ListHeads[POOL_LIST_HEADS]);

        //
        // A block of the desired size does not exist and there are
        // no large blocks that can be split to satisfy the allocation.
        // Attempt to expand the pool by allocating another page to be
        // added to the pool.
        //
        // If the pool type is paged pool, then the paged pool page lock
        // must be held during the allocation of the pool pages.
        //

        LOCK_IF_PAGED_POOL(CheckType, GlobalSpace);

        Entry = (PPOOL_HEADER)MiAllocatePoolPages (RequestType,
                                                   PAGE_SIZE,
                                                   FALSE);

        UNLOCK_IF_PAGED_POOL(CheckType, GlobalSpace);

        if (Entry == NULL) {
            if ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) != 0) {

                //
                // Must succeed pool was requested. Reset the type,
                // the pool descriptor address, and continue the search.
                //

                CheckType = NonPagedPoolMustSucceed;
                RequestType = RequestType | MUST_SUCCEED_POOL_TYPE_MASK;
                PoolDesc = PoolVector[NonPagedPoolMustSucceed];
                ListHead = &PoolDesc->ListHeads[ListNumber];
                continue;

            } else {

                //
                // No more pool of the specified type is available.
                //

                KdPrint(("EX: ExAllocatePool (%p, 0x%x ) returning NULL\n",
                    NumberOfBytes,
                    PoolType));

                UNLOCK_POOL(PoolDesc, LockHandle);

                if ((PoolType & POOL_RAISE_IF_ALLOCATION_FAILURE) != 0) {
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                return NULL;
            }
        }

        //
        // Insert the allocated page in the last allocation list.
        //

        PoolDesc->TotalPages += 1;
        Entry->PoolType = 0;
        ENCODE_POOL_INDEX(Entry, PoolIndex);

        PERFINFO_ADDPOOLPAGE(CheckType, PoolIndex, Entry, PoolDesc);

        //
        // N.B. A byte is used to store the block size in units of the
        //      smallest block size. Therefore, if the number of small
        //      blocks in the page is greater than 255, the block size
        //      is set to 255.
        //

        if ((PAGE_SIZE / POOL_SMALLEST_BLOCK) > 255) {
            Entry->BlockSize = 255;

        } else {
            Entry->BlockSize = (UCHAR)(PAGE_SIZE / POOL_SMALLEST_BLOCK);
        }

        Entry->PreviousSize = 0;
        ListHead = &PoolDesc->ListHeads[POOL_LIST_HEADS - 1];

        CHECK_LIST(__LINE__, ListHead, 0);
        PrivateInsertHeadList(ListHead, ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)));
        CHECK_LIST(__LINE__, ListHead, 0);
        CHECK_LIST(__LINE__, ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)), 0);

    } while (TRUE);
}

VOID
ExInsertPoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes,
    POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function inserts a pool tag in the tag table and increments the
    number of allocates and updates the total allocation size.

    This function also inserts the pool tag in the big page tag table.

    N.B. This function is for use by memory management ONLY.

Arguments:

    Tag - Supplies the tag used to insert an entry in the tag table.

    Va - Supplies the allocated virtual address.

    NumberOfBytes - Supplies the allocation size in bytes.

    PoolType - Supplies the pool type.

Return Value:

    None.

Environment:

    No pool locks held so pool may be freely allocated here as needed.

--*/

{
    ULONG NumberOfPages;

    ASSERT ((PoolType & SESSION_POOL_MASK) == 0);

    if ((PoolBigPageTable) && (NumberOfBytes >= PAGE_SIZE)) {

        NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);

        if (ExpAddTagForBigPages((PVOID)Va, Tag, NumberOfPages) == FALSE) {
            Tag = ' GIB';
        }
    }

    if (PoolTrackTable != NULL) {
        ExpInsertPoolTracker (Tag, NumberOfBytes, NonPagedPool);
    }
}

VOID
ExRemovePoolTag (
    ULONG Tag,
    PVOID Va,
    SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function removes a pool tag from the tag table and increments the
    number of frees and updates the total allocation size.

    This function also removes the pool tag from the big page tag table.

    N.B. This function is for use by memory management ONLY.

Arguments:

    Tag - Supplies the tag used to remove an entry in the tag table.

    Va - Supplies the allocated virtual address.

    NumberOfBytes - Supplies the allocation size in bytes.

Return Value:

    None.

Environment:

    No pool locks held so pool may be freely allocated here as needed.

--*/

{
    if ((PoolBigPageTable) && (NumberOfBytes >= PAGE_SIZE)) {
        ExpFindAndRemoveTagBigPages (Va);
    }

    if (PoolTrackTable != NULL) {
        ExpRemovePoolTracker(Tag,
                             (ULONG)NumberOfBytes,
                             NonPagedPool);
    }
}


VOID
ExpInsertPoolTracker (
    IN ULONG Key,
    IN SIZE_T Size,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function inserts a pool tag in the tag table and increments the
    number of allocates and updates the total allocation size.

Arguments:

    Key - Supplies the key value used to locate a matching entry in the
          tag table.

    Size - Supplies the allocation size.

    PoolType - Supplies the pool type.

Return Value:

    None.

Environment:

    No pool locks held so pool may be freely allocated here as needed.

--*/

{
    USHORT Result;
    ULONG Hash;
    ULONG OriginalKey;
    ULONG OriginalHash;
    ULONG Index;
    KIRQL OldIrql;
    KIRQL LockHandle;
    ULONG BigPages;
    LOGICAL HashedIt;
    SIZE_T NewSize;
    SIZE_T SizeInBytes;
    SIZE_T NewSizeInBytes;
    SIZE_T NewSizeMask;
    PPOOL_TRACKER_TABLE OldTable;
    PPOOL_TRACKER_TABLE NewTable;

    ASSERT ((PoolType & SESSION_POOL_MASK) == 0);

    //
    // Ignore protected pool bit except for returned hash index.
    //

    if (Key & PROTECTED_POOL) {
        Key &= ~PROTECTED_POOL;
        Result = (USHORT)(PROTECTED_POOL >> 16);
    } else {
        Result = 0;
    }

    if (Key == PoolHitTag) {
        DbgBreakPoint();
    }

retry:

    //
    // Compute hash index and search for pool tag.
    //

    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);

    Hash = ((40543*((((((((PUCHAR)&Key)[0]<<2)^((PUCHAR)&Key)[1])<<2)^((PUCHAR)&Key)[2])<<2)^((PUCHAR)&Key)[3]))>>2) & (ULONG)PoolTrackTableMask;
    Index = Hash;

    do {
        if (PoolTrackTable[Hash].Key == Key) {
            PoolTrackTable[Hash].Key = Key;
            goto EntryFound;
        }

        if (PoolTrackTable[Hash].Key == 0 && Hash != PoolTrackTableSize - 1) {
            PoolTrackTable[Hash].Key = Key;
            goto EntryFound;
        }

        Hash = (Hash + 1) & (ULONG)PoolTrackTableMask;
    } while (Hash != Index);

    //
    // No matching entry and no free entry was found.
    // If the overflow bucket has been used then expansion of the tracker table
    // is not allowed because a subsequent free of a tag can go negative as the
    // original allocation is in overflow and a newer allocation may be
    // distinct.
    //

    NewSize = ((PoolTrackTableSize - 1) << 1) + 1;
    NewSizeInBytes = NewSize * sizeof(POOL_TRACKER_TABLE);

    SizeInBytes = PoolTrackTableSize * sizeof(POOL_TRACKER_TABLE);

    if ((NewSizeInBytes > SizeInBytes) &&
        (PoolTrackTable[PoolTrackTableSize - 1].Key == 0)) {

        ExpLockNonPagedPool(LockHandle);

        NewTable = MiAllocatePoolPages (NonPagedPool,
                                        NewSizeInBytes,
                                        FALSE);

        ExpUnlockNonPagedPool(LockHandle);

        if (NewTable != NULL) {

            OldTable = (PVOID)PoolTrackTable;

            KdPrint(("POOL:grew track table (%p, %p, %p)\n",
                OldTable,
                PoolTrackTableSize,
                NewTable));

            RtlZeroMemory ((PVOID)NewTable, NewSizeInBytes);

            //
            // Rehash all the entries into the new table.
            //

            NewSizeMask = NewSize - 2;

            for (OriginalHash = 0; OriginalHash < PoolTrackTableSize; OriginalHash += 1) {
                OriginalKey = PoolTrackTable[OriginalHash].Key;

                if (OriginalKey == 0) {
                    continue;
                }

                Hash = (ULONG)((40543*((((((((PUCHAR)&OriginalKey)[0]<<2)^((PUCHAR)&OriginalKey)[1])<<2)^((PUCHAR)&OriginalKey)[2])<<2)^((PUCHAR)&OriginalKey)[3]))>>2) & (ULONG)NewSizeMask;
                Index = Hash;
            
                HashedIt = FALSE;
                do {
                    if (NewTable[Hash].Key == 0 && Hash != NewSize - 1) {
                        RtlCopyMemory ((PVOID)&NewTable[Hash],
                                       (PVOID)&PoolTrackTable[OriginalHash],
                                       sizeof(POOL_TRACKER_TABLE));
                        HashedIt = TRUE;
                        break;
                    }
            
                    Hash = (Hash + 1) & (ULONG)NewSizeMask;
                } while (Hash != Index);
        
                //
                // No matching entry and no free entry was found, have to bail.
                //

                if (HashedIt == FALSE) {
                    KdPrint(("POOL:rehash of track table failed (%p, %p, %p %p)\n",
                        OldTable,
                        PoolTrackTableSize,
                        NewTable,
                        OriginalKey));
        
                    ExpLockNonPagedPool(LockHandle);

                    MiFreePoolPages (NewTable);

                    ExpUnlockNonPagedPool(LockHandle);

                    goto overflow;
                }
            }

            PoolTrackTable = NewTable;
            PoolTrackTableSize = NewSize;
            PoolTrackTableMask = NewSizeMask;

            ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

            ExpLockNonPagedPool(LockHandle);

            BigPages = MiFreePoolPages (OldTable);

            ExpUnlockNonPagedPool(LockHandle);

            ExpRemovePoolTracker ('looP',
                                  BigPages * PAGE_SIZE,
                                  NonPagedPool);

            ExpInsertPoolTracker ('looP',
                                  (ULONG) ROUND_TO_PAGES(NewSizeInBytes),
                                  NonPagedPool);

            goto retry;
        }
    }

overflow:

    //
    // Use the very last entry as a bit bucket for overflows.
    //

    Hash = (ULONG)PoolTrackTableSize - 1;

    PoolTrackTable[Hash].Key = 'lfvO';

    //
    // Update pool tracker table entry.
    //

EntryFound:

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        PoolTrackTable[Hash].PagedAllocs += 1;
        PoolTrackTable[Hash].PagedBytes += Size;

    } else {
        PoolTrackTable[Hash].NonPagedAllocs += 1;
        PoolTrackTable[Hash].NonPagedBytes += Size;
    }
    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

    return;
}


VOID
ExpRemovePoolTracker (
    IN ULONG Key,
    IN ULONG Size,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function increments the number of frees and updates the total
    allocation size.

Arguments:

    Key - Supplies the key value used to locate a matching entry in the
          tag table.

    Size - Supplies the allocation size.

    PoolType - Supplies the pool type.

Return Value:

    None.

--*/

{
    ULONG Hash;
    ULONG Index;
    KIRQL OldIrql;

    //
    // Ignore protected pool bit
    //

    Key &= ~PROTECTED_POOL;
    if (Key == PoolHitTag) {
        DbgBreakPoint();
    }

    //
    // Compute hash index and search for pool tag.
    //

    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);

    Hash = ((40543*((((((((PUCHAR)&Key)[0]<<2)^((PUCHAR)&Key)[1])<<2)^((PUCHAR)&Key)[2])<<2)^((PUCHAR)&Key)[3]))>>2) & (ULONG)PoolTrackTableMask;
    Index = Hash;

    do {
        if (PoolTrackTable[Hash].Key == Key) {
            goto EntryFound;
        }

        if (PoolTrackTable[Hash].Key == 0 && Hash != PoolTrackTableSize - 1) {
            KdPrint(("POOL: Unable to find tracker %lx, table corrupted\n", Key));
            ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
            return;
        }

        Hash = (Hash + 1) & (ULONG)PoolTrackTableMask;
    } while (Hash != Index);

    //
    // No matching entry and no free entry was found.
    //

    Hash = (ULONG)PoolTrackTableSize - 1;

    //
    // Update pool tracker table entry.
    //

EntryFound:

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        PoolTrackTable[Hash].PagedBytes -= Size;
        PoolTrackTable[Hash].PagedFrees += 1;

    } else {
        PoolTrackTable[Hash].NonPagedBytes -= Size;
        PoolTrackTable[Hash].NonPagedFrees += 1;
    }

    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

    return;
}


LOGICAL
ExpAddTagForBigPages (
    IN PVOID Va,
    IN ULONG Key,
    IN ULONG NumberOfPages
    )
/*++

Routine Description:

    This function inserts a pool tag in the big page tag table.

Arguments:

    Va - Supplies the allocated virtual address.

    Key - Supplies the key value used to locate a matching entry in the
        tag table.

    NumberOfPages - Supplies the number of pages that were allocated.

Return Value:

    TRUE if an entry was allocated, FALSE if not.

Environment:

    No pool locks held so the table may be freely expanded here as needed.

--*/
{
    ULONG Hash;
    ULONG BigPages;
    PVOID OldTable;
    LOGICAL Inserted;
    KIRQL OldIrql;
    KIRQL LockHandle;
    SIZE_T SizeInBytes;
    SIZE_T NewSizeInBytes;
    PPOOL_TRACKER_BIG_PAGES NewTable;
    PPOOL_TRACKER_BIG_PAGES p;

retry:

    Inserted = TRUE;
    Hash = (ULONG)(((ULONG_PTR)Va >> PAGE_SHIFT) & PoolBigPageTableHash);
    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);
    while ((LONG_PTR)PoolBigPageTable[Hash].Va < 0) {
        Hash += 1;
        if (Hash >= PoolBigPageTableSize) {
            if (!Inserted) {

                //
                // Try to expand the tracker table.
                //

                SizeInBytes = PoolBigPageTableSize * sizeof(POOL_TRACKER_BIG_PAGES);
                NewSizeInBytes = (SizeInBytes << 1);

                if (NewSizeInBytes > SizeInBytes) {

                    ExpLockNonPagedPool(LockHandle);

                    NewTable = MiAllocatePoolPages (NonPagedPool,
                                                    NewSizeInBytes,
                                                    FALSE);

                    ExpUnlockNonPagedPool(LockHandle);

                    if (NewTable != NULL) {
    
                        OldTable = (PVOID)PoolBigPageTable;

                        KdPrint(("POOL:grew big table (%p, %p, %p)\n",
                            OldTable,
                            PoolBigPageTableSize,
                            NewTable));

                        RtlCopyMemory ((PVOID)NewTable,
                                       OldTable,
                                       SizeInBytes);

                        RtlZeroMemory ((PVOID)(NewTable + PoolBigPageTableSize),
                                       NewSizeInBytes - SizeInBytes);

                        PoolBigPageTable = NewTable;
                        PoolBigPageTableSize <<= 1;
                        PoolBigPageTableHash = PoolBigPageTableSize - 1;

                        ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

                        ExpLockNonPagedPool(LockHandle);

                        BigPages = MiFreePoolPages (OldTable);

                        ExpUnlockNonPagedPool(LockHandle);

                        ExpRemovePoolTracker ('looP',
                                              BigPages * PAGE_SIZE,
                                              NonPagedPool);

                        ExpInsertPoolTracker ('looP',
                                              (ULONG) ROUND_TO_PAGES(NewSizeInBytes),
                                              NonPagedPool);

                        goto retry;
                    }
                }

                if (!FirstPrint) {
                    KdPrint(("POOL:unable to insert big page slot %lx\n",Key));
                    FirstPrint = TRUE;
                }

                ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
                return FALSE;
            }

            Hash = 0;
            Inserted = FALSE;
        }
    }

    p = &PoolBigPageTable[Hash];

    ASSERT ((LONG_PTR)Va < 0);

    p->Va = Va;
    p->Key = Key;
    p->NumberOfPages = NumberOfPages;

    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);

    return TRUE;
}


ULONG
ExpFindAndRemoveTagBigPages (
    IN PVOID Va
    )

{
    ULONG Hash;
    LOGICAL Inserted;
    KIRQL OldIrql;
    ULONG ReturnKey;

    Inserted = TRUE;
    Hash = (ULONG)(((ULONG_PTR)Va >> PAGE_SHIFT) & PoolBigPageTableHash);
    ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);
    while (PoolBigPageTable[Hash].Va != Va) {
        Hash += 1;
        if (Hash >= PoolBigPageTableSize) {
            if (!Inserted) {
                if (!FirstPrint) {
                    KdPrint(("POOL:unable to find big page slot %lx\n",Va));
                    FirstPrint = TRUE;
                }

                ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
                return ' GIB';
            }

            Hash = 0;
            Inserted = FALSE;
        }
    }

    ASSERT ((LONG_PTR)Va < 0);
    (ULONG_PTR)PoolBigPageTable[Hash].Va &= MAXLONG_PTR;

    ReturnKey = PoolBigPageTable[Hash].Key;
    ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
    return ReturnKey;
}


ULONG
ExpAllocatePoolWithQuotaHandler(
    IN NTSTATUS ExceptionCode,
    IN PVOID PoolAddress,
    IN LOGICAL ContinueSearch
    )

/*++

Routine Description:

    This function is called when an exception occurs in ExFreePool
    while quota is being charged to a process.

    Its function is to deallocate the pool block and continue the search
    for an exception handler.

Arguments:

    ExceptionCode - Supplies the exception code that caused this
        function to be entered.

    PoolAddress - Supplies the address of a pool block that needs to be
        deallocated.

    ContinueSearch - Supplies a value that if TRUE causes the exception
        search to continue.  This is used in allocate pool with quota
        calls that do not contain the pool quota mask bit set.

Return Value:

    EXCEPTION_CONTINUE_SEARCH - The exception should be propagated to the
        caller of ExAllocatePoolWithQuota.

--*/

{
    if ( PoolAddress ) {
        ASSERT(ExceptionCode == STATUS_QUOTA_EXCEEDED);
        ExFreePool(PoolAddress);

    } else {
        ASSERT(ExceptionCode == STATUS_INSUFFICIENT_RESOURCES);
    }

    return ContinueSearch ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
}

PVOID
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type,
    returns a pointer to the allocated block, and if the binary buddy
    allocator was used to satisfy the request, charges pool quota to the
    current process.  This function is used to access both the
    page-aligned pools, and the binary buddy.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate binary buddy pool, then the
    page-aligned pool allocator is used.  The allocated block will be
    page-aligned and a page-sized multiple.  No quota is charged to the
    current process if this is the case.

    Otherwise, the appropriate binary buddy pool is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  After
    the allocation completes, an attempt will be made to charge pool
    quota (of the appropriate type) to the current process object.  If
    the quota charge succeeds, then the pool block's header is adjusted
    to point to the current process.  The process object is not
    dereferenced until the pool is deallocated and the appropriate
    amount of quota is returned to the process.  Otherwise, the pool is
    deallocated, a "quota exceeded" condition is raised.

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types and sufficient quota
        exists, then this call will always succeed and return a pointer
        to allocated pool.  Otherwise, if the system cannot allocate
        the requested amount of memory a STATUS_INSUFFICIENT_RESOURCES
        status is raised.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NON-NULL - Returns a pointer to the allocated pool.

    Unspecified - If insufficient quota exists to complete the pool
        allocation, the return value is unspecified.

--*/

{
    return (ExAllocatePoolWithQuotaTag (PoolType, NumberOfBytes, 'enoN'));
}


PVOID
ExAllocatePoolWithQuotaTag(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type,
    returns a pointer to the allocated block, and if the binary buddy
    allocator was used to satisfy the request, charges pool quota to the
    current process.  This function is used to access both the
    page-aligned pools, and the binary buddy.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate binary buddy pool, then the
    page-aligned pool allocator is used.  The allocated block will be
    page-aligned and a page-sized multiple.  No quota is charged to the
    current process if this is the case.

    Otherwise, the appropriate binary buddy pool is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  After
    the allocation completes, an attempt will be made to charge pool
    quota (of the appropriate type) to the current process object.  If
    the quota charge succeeds, then the pool block's header is adjusted
    to point to the current process.  The process object is not
    dereferenced until the pool is deallocated and the appropriate
    amount of quota is returned to the process.  Otherwise, the pool is
    deallocated, a "quota exceeded" condition is raised.

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types and sufficient quota
        exists, then this call will always succeed and return a pointer
        to allocated pool.  Otherwise, if the system cannot allocate
        the requested amount of memory a STATUS_INSUFFICIENT_RESOURCES
        status is raised.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NON-NULL - Returns a pointer to the allocated pool.

    Unspecified - If insufficient quota exists to complete the pool
        allocation, the return value is unspecified.

--*/

{
    PVOID p;
    PEPROCESS Process;
    PPOOL_HEADER Entry;
    LOGICAL IgnoreQuota;
    LOGICAL RaiseOnQuotaFailure;

    IgnoreQuota = FALSE;
    RaiseOnQuotaFailure = TRUE;

    if ( PoolType & POOL_QUOTA_FAIL_INSTEAD_OF_RAISE ) {
        RaiseOnQuotaFailure = FALSE;
        PoolType &= ~POOL_QUOTA_FAIL_INSTEAD_OF_RAISE;
    }

    if ((POOL_QUOTA_ENABLED == FALSE)
#if i386 && !FPO
            || (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB)
#endif // i386 && !FPO
       ) {
        IgnoreQuota = TRUE;
    } else {
        PoolType = (POOL_TYPE)((UCHAR)PoolType + POOL_QUOTA_MASK);
    }

    p = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);

    //
    // Note - NULL is page aligned.
    //

    if (!PAGE_ALIGNED(p) && !IgnoreQuota) {

        if ((p >= MmSpecialPoolStart) && (p < MmSpecialPoolEnd)) {
            return p;
        }

#if POOL_CACHE_SUPPORTED

        //
        // Align entry on pool allocation boundary.
        //

        if (((ULONG)p & POOL_CACHE_CHECK) == 0) {
            Entry = (PPOOL_HEADER)((ULONG)p - PoolCacheSize);
        } else {
            Entry = (PPOOL_HEADER)((PCH)p - POOL_OVERHEAD);
        }

#else
        Entry = (PPOOL_HEADER)((PCH)p - POOL_OVERHEAD);
#endif //POOL_CACHE_SUPPORTED

        Process = PsGetCurrentProcess();

        //
        // Catch exception and back out allocation if necessary
        //

        try {

            Entry->ProcessBilled = NULL;

            if (Process != PsInitialSystemProcess) {

                PsChargePoolQuota(Process,
                                 PoolType & BASE_POOL_TYPE_MASK,
                                 (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));

                ObReferenceObject(Process);
                Entry->ProcessBilled = Process;
            }

        } except ( ExpAllocatePoolWithQuotaHandler(GetExceptionCode(),p,RaiseOnQuotaFailure)) {
            if ( RaiseOnQuotaFailure ) {
                KeBugCheck(GetExceptionCode());
            }
            else {
                p = NULL;
            }
        }

    } else {
        if ( !p && RaiseOnQuotaFailure ) {
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    return p;
}

VOID
ExFreePool(
    IN PVOID P
    )
{
    ExFreePoolWithTag(P, 0);
    return;
}

VOID
ExFreePoolWithTag(
    IN PVOID P,
    IN ULONG TagToFree
    )
{

/*++

Routine Description:

    This function deallocates a block of pool. This function is used to
    deallocate to both the page aligned pools and the buddy (less than
    a page) pools.

    If the address of the block being deallocated is page-aligned, then
    the page-aligned pool deallocator is used.

    Otherwise, the binary buddy pool deallocator is used.  Deallocation
    looks at the allocated block's pool header to determine the pool
    type and block size being deallocated.  If the pool was allocated
    using ExAllocatePoolWithQuota, then after the deallocation is
    complete, the appropriate process's pool quota is adjusted to reflect
    the deallocation, and the process object is dereferenced.

Arguments:

    P - Supplies the address of the block of pool being deallocated.

Return Value:

    None.

--*/

    POOL_TYPE CheckType;
    PPOOL_HEADER Entry;
    ULONG Index;
    KIRQL LockHandle;
    PNPAGED_LOOKASIDE_LIST LookasideList;
    PPOOL_HEADER NextEntry;
    ULONG PoolIndex;
    POOL_TYPE PoolType;
    PPOOL_DESCRIPTOR PoolDesc;
    PEPROCESS ProcessBilled;
    LOGICAL Combined;
    ULONG BigPages;
    ULONG Tag;
    LOGICAL GlobalSpace;
    PKPRCB Prcb;
    PERFINFO_EXFREEPOOLWITHTAG_DECL();

    PERFINFO_FREEPOOL(P);

    if ((P >= MmSpecialPoolStart) && (P < MmSpecialPoolEnd)) {
        MmFreeSpecialPool (P);
        return;
    }

    ProcessBilled = NULL;

    //
    // If entry is page aligned, then call free block to the page aligned
    // pool. Otherwise, free the block to the allocation lists.
    //

    if (PAGE_ALIGNED(P)) {

        PoolType = MmDeterminePoolType(P);

        ASSERT_FREE_IRQL(PoolType, P);

        CheckType = PoolType & BASE_POOL_TYPE_MASK;

        if (PoolType == PagedPoolSession) {
            PoolDesc = ExpSessionPoolDescriptor;
        }
        else {
            PoolDesc = PoolVector[PoolType];
        }

        if ((PoolTrackTable != NULL) && (PoolType != PagedPoolSession)) {
            Tag = ExpFindAndRemoveTagBigPages(P);
        }

        LOCK_POOL(PoolDesc, LockHandle);

        PoolDesc->RunningDeAllocs += 1;

        //
        // Large session pool allocations are accounted for directly by
        // the memory manager so no need to call MiSessionPoolFreed here.
        //

        BigPages = MiFreePoolPages(P);

        if ((PoolTrackTable != NULL) && (PoolType != PagedPoolSession)) {
            if (Tag & PROTECTED_POOL) {
                Tag &= ~PROTECTED_POOL;
                TagToFree &= ~PROTECTED_POOL;
                if (Tag != TagToFree) {
                    DbgPrint( "EX: Invalid attempt to free protected pool block %x (%c%c%c%c)\n",
                              P,
                              Tag,
                              Tag >> 8,
                              Tag >> 16,
                              Tag >> 24
                            );
                    DbgBreakPoint();
                }
            }

            ExpRemovePoolTracker(Tag,
                                 BigPages * PAGE_SIZE,
                                 PoolType);
        }

        //
        // Check if an ERESOURCE is currently active in this memory block.
        //

        FREE_CHECK_ERESOURCE (P, BigPages << PAGE_SHIFT);

        //
        // Check if a KTIMER is currently active in this memory block
        //

        FREE_CHECK_KTIMER(P, BigPages << PAGE_SHIFT);

        //
        // Search worker queues for work items still queued
        //
        FREE_CHECK_WORKER(P, BigPages << PAGE_SHIFT);

        PoolDesc->TotalBigPages -= BigPages;

        UNLOCK_POOL(PoolDesc, LockHandle);

        return;
    }

    //
    // Align the entry address to a pool allocation boundary.
    //

#if POOL_CACHE_SUPPORTED

    if (((ULONG)P & POOL_CACHE_CHECK) == 0) {
        Entry = (PPOOL_HEADER)((ULONG)P - PoolCacheSize);

    } else {
        Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);
    }

#else

    Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);

#endif //POOL_CACHE_SUPPORTED

    ASSERT_POOL_NOT_FREE(Entry);

    PoolType = (Entry->PoolType & POOL_TYPE_MASK) - 1;

    CheckType = PoolType & BASE_POOL_TYPE_MASK;

    ASSERT_FREE_IRQL(PoolType, P);

    if (Entry->PoolType & POOL_VERIFIER_MASK) {
        VerifierFreeTrackedPool (P,
                                 Entry->BlockSize << POOL_BLOCK_SHIFT,
                                 CheckType,
                                 FALSE);
    }

    PoolDesc = PoolVector[PoolType];
    GlobalSpace = TRUE;

    if (Entry->PoolType & SESSION_POOL_MASK) {
        if (CheckType == PagedPool) {
            PoolDesc = ExpSessionPoolDescriptor;
        }
        GlobalSpace = FALSE;
    }
    else if (CheckType == PagedPool) {
        PoolDesc = &PoolDesc[DECODE_POOL_INDEX(Entry)];
    }

    LOCK_POOL(PoolDesc, LockHandle);

    if (!IS_POOL_HEADER_MARKED_ALLOCATED(Entry)) {
        KeBugCheckEx (BAD_POOL_CALLER, 7, __LINE__, (ULONG_PTR)Entry, (ULONG_PTR)P);
    }

    MARK_POOL_HEADER_FREED(Entry);

    //
    // If this allocation was in session space, let the memory
    // manager know to delete it so it won't be considered in use on
    // session exit.  Note this call must be made with the
    // relevant pool still locked.
    //

    if (GlobalSpace == FALSE) {
        MiSessionPoolFreed(P, Entry->BlockSize << POOL_BLOCK_SHIFT, CheckType);
    }

    UNLOCK_POOL(PoolDesc, LockHandle);

    ASSERT_POOL_TYPE_NOT_ZERO(Entry);

    //
    // Check if an ERESOURCE is currently active in this memory block.
    //

    FREE_CHECK_ERESOURCE (Entry, (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));

    //
    // Check if a KTIMER is currently active in this memory block.
    //

    FREE_CHECK_KTIMER(Entry, (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));

    //
    // Look for work items still queued
    //

    FREE_CHECK_WORKER(Entry, (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));


#if DBG

    //
    // Check if the pool index field is defined correctly.
    //

    if (CheckType == NonPagedPool) {
        if (DECODE_POOL_INDEX(Entry) != 0) {
            KeBugCheckEx(BAD_POOL_CALLER, 2, (ULONG_PTR)Entry, (ULONG_PTR)(*(PULONG)Entry), 0);
        }
    }
    else {
        if (GlobalSpace == FALSE) {
            //
            // All session space allocations have an index of 0.
            //
            ASSERT (DECODE_POOL_INDEX(Entry) == 0);
        }
        else if (DECODE_POOL_INDEX(Entry) == 0) {
            KeBugCheckEx(BAD_POOL_CALLER, 4, (ULONG_PTR)Entry, *(PULONG)Entry, 0);
        }
    }

#endif // DBG

    //
    // If pool tagging is enabled, then update the pool tracking database.
    // Otherwise, check to determine if quota was charged when the pool
    // block was allocated.
    //

#if defined (_WIN64)
    if (Entry->PoolType & POOL_QUOTA_MASK) {
        ProcessBilled = Entry->ProcessBilled;
    }

    Tag = Entry->PoolTag;
    if (Tag & PROTECTED_POOL) {
        Tag &= ~PROTECTED_POOL;
        TagToFree &= ~PROTECTED_POOL;
        if (Tag != TagToFree) {
            DbgPrint( "EX: Invalid attempt to free protected pool block %x (%c%c%c%c)\n",
                      P,
                      Tag,
                      Tag >> 8,
                      Tag >> 16,
                      Tag >> 24
                    );
            DbgBreakPoint();
        }
    }
    if (PoolTrackTable != NULL) {
        if (GlobalSpace == TRUE) {
            ExpRemovePoolTracker(Tag,
                                 Entry->BlockSize << POOL_BLOCK_SHIFT,
                                 PoolType);

        }
    }
    if (ProcessBilled != NULL) {
        PsReturnPoolQuota(ProcessBilled,
                          PoolType & BASE_POOL_TYPE_MASK,
                          (ULONG)Entry->BlockSize << POOL_BLOCK_SHIFT);
        ObDereferenceObject(ProcessBilled);
    }
#else
    if (Entry->PoolType & POOL_QUOTA_MASK) {
        if (PoolTrackTable == NULL) {
            ProcessBilled = Entry->ProcessBilled;
            Entry->PoolTag = 'atoQ';
        }
    }

    if (PoolTrackTable != NULL) {
        Tag = Entry->PoolTag;
        if (Tag & PROTECTED_POOL) {
            Tag &= ~PROTECTED_POOL;
            TagToFree &= ~PROTECTED_POOL;
            if (Tag != TagToFree) {
                DbgPrint( "EX: Invalid attempt to free protected pool block %x (%c%c%c%c)\n",
                          P,
                          Tag,
                          Tag >> 8,
                          Tag >> 16,
                          Tag >> 24
                        );
                DbgBreakPoint();
            }
        }
        if (GlobalSpace == TRUE) {
            ExpRemovePoolTracker(Tag,
                                 Entry->BlockSize << POOL_BLOCK_SHIFT ,
                                 PoolType);
        }
    } else if (ProcessBilled != NULL) {
        PsReturnPoolQuota(ProcessBilled,
                          PoolType & BASE_POOL_TYPE_MASK,
                          (ULONG)Entry->BlockSize << POOL_BLOCK_SHIFT);
        ObDereferenceObject(ProcessBilled);
    }
#endif

    //
    // If the pool block is a small block, then attempt to free the block
    // to the single entry lookaside list. If the free attempt fails, then
    // free the block by merging it back into the pool data structures.
    //

    PoolIndex = DECODE_POOL_INDEX(Entry);

    Index = Entry->BlockSize;

    if (Index <= POOL_SMALL_LISTS && GlobalSpace == TRUE) {

        //
        // Attempt to free the small block to a per processor lookaside
        // list.
        //

        if (CheckType == PagedPool) {
            if (Isx86FeaturePresent(KF_CMPXCHG8B)) {
                Prcb = KeGetCurrentPrcb();
                LookasideList = Prcb->PPPagedLookasideList[Index - 1].P;
                LookasideList->L.TotalFrees += 1;

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                if (ExQueryDepthSList(&LookasideList->L.ListHead) < LookasideList->L.Depth) {
                    LookasideList->L.FreeHits += 1;
                    Entry += 1;
                    ExInterlockedPushEntrySList(&LookasideList->L.ListHead,
                                                (PSINGLE_LIST_ENTRY)Entry,
                                                &LookasideList->Lock);

                    CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                    return;

                } else {
                    LookasideList = Prcb->PPPagedLookasideList[Index - 1].L;
                    LookasideList->L.TotalFrees += 1;

                    CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                    if (ExQueryDepthSList(&LookasideList->L.ListHead) < LookasideList->L.Depth) {
                        LookasideList->L.FreeHits += 1;
                        Entry += 1;
                        ExInterlockedPushEntrySList(&LookasideList->L.ListHead,
                                                    (PSINGLE_LIST_ENTRY)Entry,
                                                    &LookasideList->Lock);

                        CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                        return;
                    }
                }
            }

        } else {

            //
            // Make sure we don't put a must succeed buffer into the
            // regular nonpaged pool list.
            //

            if (PoolType != NonPagedPoolMustSucceed) {
                Prcb = KeGetCurrentPrcb();
                LookasideList = Prcb->PPNPagedLookasideList[Index - 1].P;
                LookasideList->L.TotalFrees += 1;

                CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                if (ExQueryDepthSList(&LookasideList->L.ListHead) < LookasideList->L.Depth) {
                    LookasideList->L.FreeHits += 1;
                    Entry += 1;
                    ExInterlockedPushEntrySList(&LookasideList->L.ListHead,
                                                (PSINGLE_LIST_ENTRY)Entry,
                                                &LookasideList->Lock);

                    CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                    return;

                } else {
                    LookasideList = Prcb->PPNPagedLookasideList[Index - 1].L;
                    LookasideList->L.TotalFrees += 1;

                    CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                    if (ExQueryDepthSList(&LookasideList->L.ListHead) < LookasideList->L.Depth) {
                        LookasideList->L.FreeHits += 1;
                        Entry += 1;
                        ExInterlockedPushEntrySList(&LookasideList->L.ListHead,
                                                    (PSINGLE_LIST_ENTRY)Entry,
                                                    &LookasideList->Lock);

                        CHECK_LOOKASIDE_LIST(__LINE__, LookasideList, P);

                        return;
                    }
                }
            }
        }
    }

    ASSERT(PoolIndex == PoolDesc->PoolIndex);

    LOCK_POOL(PoolDesc, LockHandle);

    CHECK_POOL_HEADER(__LINE__, Entry);

    PoolDesc->RunningDeAllocs += 1;

    //
    // Free the specified pool block.
    //
    // Check to see if the next entry is free.
    //

    Combined = FALSE;
    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + Entry->BlockSize);
    if (PAGE_END(NextEntry) == FALSE) {

        if (NextEntry->PoolType == 0) {

            //
            // This block is free, combine with the released block.
            //

            Combined = TRUE;

            CHECK_LIST(__LINE__, ((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)), P);
            PrivateRemoveEntryList(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
            CHECK_LIST(__LINE__, DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Flink), P);
            CHECK_LIST(__LINE__, DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Blink), P);

            Entry->BlockSize += NextEntry->BlockSize;
        }
    }

    //
    // Check to see if the previous entry is free.
    //

    if (Entry->PreviousSize != 0) {
        NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry - Entry->PreviousSize);
        if (NextEntry->PoolType == 0) {

            //
            // This block is free, combine with the released block.
            //

            Combined = TRUE;

            CHECK_LIST(__LINE__, ((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)), P);
            PrivateRemoveEntryList(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD)));
            CHECK_LIST(__LINE__, DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Flink), P);
            CHECK_LIST(__LINE__, DecodeLink(((PLIST_ENTRY)((PCHAR)NextEntry + POOL_OVERHEAD))->Blink), P);

            NextEntry->BlockSize += Entry->BlockSize;
            Entry = NextEntry;
        }
    }

    //
    // If the block being freed has been combined into a full page,
    // then return the free page to memory management.
    //

    if (PAGE_ALIGNED(Entry) &&
        (PAGE_END((PPOOL_BLOCK)Entry + Entry->BlockSize) != FALSE)) {

        //
        // If the pool type is paged pool, then the paged pool page lock
        // must be held during the free of the pool pages.
        //

        LOCK_IF_PAGED_POOL(CheckType, GlobalSpace);

        PERFINFO_FREEPOOLPAGE(CheckType, PoolIndex, Entry, PoolDesc);

        MiFreePoolPages(Entry);

        UNLOCK_IF_PAGED_POOL(CheckType, GlobalSpace);

        PoolDesc->TotalPages -= 1;

    } else {

        //
        // Insert this element into the list.
        //

        Entry->PoolType = 0;
        ENCODE_POOL_INDEX(Entry, PoolIndex);
        Index = Entry->BlockSize;

        //
        // If the freed block was combined with any other block, then
        // adjust the size of the next block if necessary.
        //

        if (Combined != FALSE) {

            //
            // The size of this entry has changed, if this entry is
            // not the last one in the page, update the pool block
            // after this block to have a new previous allocation size.
            //

            NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + Entry->BlockSize);
            if (PAGE_END(NextEntry) == FALSE) {
                NextEntry->PreviousSize = Entry->BlockSize;
            }

            //
            // Reduce fragmentation and insert at the tail in hopes
            // neighbors for this will be freed before this is reallocated.
            //

            CHECK_LIST(__LINE__, &PoolDesc->ListHeads[Index - 1], P);
            PrivateInsertTailList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)));
            CHECK_LIST(__LINE__, &PoolDesc->ListHeads[Index - 1], P);
            CHECK_LIST(__LINE__, ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)), P);

        } else {

            CHECK_LIST(__LINE__, &PoolDesc->ListHeads[Index - 1], P);
            PrivateInsertHeadList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)));
            CHECK_LIST(__LINE__, &PoolDesc->ListHeads[Index - 1], P);
            CHECK_LIST(__LINE__, ((PLIST_ENTRY)((PCHAR)Entry + POOL_OVERHEAD)), P);
        }
    }

    UNLOCK_POOL(PoolDesc, LockHandle);
}


ULONG
ExQueryPoolBlockSize (
    IN PVOID PoolBlock,
    OUT PBOOLEAN QuotaCharged
    )

/*++

Routine Description:

    This function returns the size of the pool block.

Arguments:

    PoolBlock - Supplies the address of the block of pool.

    QuotaCharged - Supplies a BOOLEAN variable to receive whether or not the
        pool block had quota charged.

    NOTE: If the entry is bigger than a page, the value PAGE_SIZE is returned
          rather than the correct number of bytes.

Return Value:

    Size of pool block.

--*/

{
    PPOOL_HEADER Entry;
    ULONG size;

    if ((PoolBlock >= MmSpecialPoolStart) && (PoolBlock < MmSpecialPoolEnd)) {
        *QuotaCharged = FALSE;
        return (ULONG)MmQuerySpecialPoolBlockSize (PoolBlock);
    }

    if (PAGE_ALIGNED(PoolBlock)) {
        *QuotaCharged = FALSE;
        return PAGE_SIZE;
    }

#if POOL_CACHE_SUPPORTED

    //
    // Align entry on pool allocation boundary.
    //

    if (((ULONG)PoolBlock & POOL_CACHE_CHECK) == 0) {
        Entry = (PPOOL_HEADER)((ULONG)PoolBlock - PoolCacheSize);
        size = (Entry->BlockSize << POOL_BLOCK_SHIFT) - PoolCacheSize;

    } else {
        Entry = (PPOOL_HEADER)((PCHAR)PoolBlock - POOL_OVERHEAD);
        size = (Entry->BlockSize << POOL_BLOCK_SHIFT) - POOL_OVERHEAD;
    }

#else

    Entry = (PPOOL_HEADER)((PCHAR)PoolBlock - POOL_OVERHEAD);
    size = (ULONG)((Entry->BlockSize << POOL_BLOCK_SHIFT) - POOL_OVERHEAD);

#endif //POOL_CACHE_SUPPORTED

#ifdef _WIN64
    *QuotaCharged = (BOOLEAN) (Entry->ProcessBilled != NULL);
#else
    if ( PoolTrackTable ) {
        *QuotaCharged = FALSE;
    }
    else {
        *QuotaCharged = (BOOLEAN) (Entry->ProcessBilled != NULL);
    }
#endif
    return size;
}

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
    )

{
    ULONG Index;
    PNPAGED_LOOKASIDE_LIST Lookaside;
    PLIST_ENTRY NextEntry;
    PPOOL_DESCRIPTOR pd;

    //
    // Sum all the paged pool usage.
    //

    pd = PoolVector[PagedPool];
    *PagedPoolPages = 0;
    *PagedPoolAllocs = 0;
    *PagedPoolFrees = 0;

    for (Index = 0; Index < ExpNumberOfPagedPools + 1; Index += 1) {
        *PagedPoolPages += pd[Index].TotalPages + pd[Index].TotalBigPages;
        *PagedPoolAllocs += pd[Index].RunningAllocs;
        *PagedPoolFrees += pd[Index].RunningDeAllocs;
    }

    //
    // Sum all the nonpaged pool usage.
    //

    pd = PoolVector[NonPagedPool];
    *NonPagedPoolPages = pd->TotalPages + pd->TotalBigPages;
    *NonPagedPoolAllocs = pd->RunningAllocs;
    *NonPagedPoolFrees = pd->RunningDeAllocs;

    //
    // Sum all the nonpaged must succeed usage.
    //

    pd = PoolVector[NonPagedPoolMustSucceed];
    *NonPagedPoolPages += pd->TotalPages + pd->TotalBigPages;
    *NonPagedPoolAllocs += pd->RunningAllocs;
    *NonPagedPoolFrees += pd->RunningDeAllocs;

    //
    // Sum all the lookaside hits for paged and nonpaged pool.
    //

    NextEntry = ExPoolLookasideListHead.Flink;
    while (NextEntry != &ExPoolLookasideListHead) {
        Lookaside = CONTAINING_RECORD(NextEntry,
                                      NPAGED_LOOKASIDE_LIST,
                                      L.ListEntry);

        if (Lookaside->L.Type == NonPagedPool) {
            *NonPagedPoolLookasideHits += Lookaside->L.AllocateHits;

        } else {
            *PagedPoolLookasideHits += Lookaside->L.AllocateHits;
        }

        NextEntry = NextEntry->Flink;
    }

    return;
}


VOID
ExReturnPoolQuota(
    IN PVOID P
    )

/*++

Routine Description:

    This function returns quota charged to a subject process when the
    specified pool block was allocated.

Arguments:

    P - Supplies the address of the block of pool being deallocated.

Return Value:

    None.

--*/

{

    PPOOL_HEADER Entry;
    POOL_TYPE PoolType;
    PEPROCESS Process;
#if defined(_ALPHA_) && !defined(_AXP64_)
    PPOOL_DESCRIPTOR PoolDesc;
    KIRQL LockHandle;
#endif

    //
    //  Do nothing for special pool. No quota was charged.
    //
    
    if ((P >= MmSpecialPoolStart) && (P < MmSpecialPoolEnd)) {
        
        return;
    }

    //
    // Align the entry address to a pool allocation boundary.
    //

#if POOL_CACHE_SUPPORTED

    if (((ULONG)P & POOL_CACHE_CHECK) == 0) {
        Entry = (PPOOL_HEADER)((ULONG)P - PoolCacheSize);

    } else {
        Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);
    }

#else

    Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);

#endif //POOL_CACHE_SUPPORTED

    //
    // If quota was charged, then return the appropriate quota to the
    // subject process.
    //

    if ((Entry->PoolType & POOL_QUOTA_MASK) && POOL_QUOTA_ENABLED) {

        PoolType = (Entry->PoolType & POOL_TYPE_MASK) - 1;

#if _POOL_LOCK_GRANULAR_
        PoolDesc = PoolVector[PoolType];
        if (PoolType == PagedPool) {
            PoolDesc = &PoolDesc[DECODE_POOL_INDEX(Entry)];
        }
#endif

        LOCK_POOL_GRANULAR(PoolDesc, LockHandle);

        Entry->PoolType &= ~POOL_QUOTA_MASK;

        UNLOCK_POOL_GRANULAR(PoolDesc, LockHandle);

        Process = Entry->ProcessBilled;

#if !defined (_WIN64)
        Entry->PoolTag = 'atoQ';
#endif

        if (Process != NULL) {
            PsReturnPoolQuota(Process,
                              PoolType & BASE_POOL_TYPE_MASK,
                              (ULONG)Entry->BlockSize << POOL_BLOCK_SHIFT);

            ObDereferenceObject(Process);
        }

    }

    return;
}

#if DBG || (i386 && !FPO)

//
// Only works on checked builds or free x86 builds with FPO turned off
// See comment in mm\allocpag.c
//

NTSTATUS
ExpSnapShotPoolPages(
    IN PVOID Address,
    IN ULONG Size,
    IN OUT PSYSTEM_POOL_INFORMATION PoolInformation,
    IN OUT PSYSTEM_POOL_ENTRY *PoolEntryInfo,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )
{
    NTSTATUS Status;
    CLONG i;
    PPOOL_HEADER p;
    PPOOL_TRACKER_BIG_PAGES PoolBig;
    LOGICAL ValidSplitBlock;
    ULONG EntrySize;
    KIRQL OldIrql;

    if (PAGE_ALIGNED(Address) && PoolBigPageTable) {

        ExAcquireSpinLock(&ExpTaggedPoolLock, &OldIrql);

        PoolBig = PoolBigPageTable;

        for (i = 0; i < PoolBigPageTableSize; i += 1, PoolBig += 1) {

            if (PoolBig->NumberOfPages == 0 || PoolBig->Va != Address) {
                continue;
            }

            PoolInformation->NumberOfEntries += 1;
            *RequiredLength += sizeof(SYSTEM_POOL_ENTRY);

            if (Length < *RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else {
                (*PoolEntryInfo)->Allocated = TRUE;
                (*PoolEntryInfo)->Size = PoolBig->NumberOfPages << PAGE_SHIFT;
                (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
                (*PoolEntryInfo)->ProcessChargedQuota = 0;
#if !DBG
                if (NtGlobalFlag & FLG_POOL_ENABLE_TAGGING)
#endif  //!DBG
                (*PoolEntryInfo)->TagUlong = PoolBig->Key;
                (*PoolEntryInfo) += 1;
                Status = STATUS_SUCCESS;
            }

            ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
            return  Status;
        }
        ExReleaseSpinLock(&ExpTaggedPoolLock, OldIrql);
    }

    p = (PPOOL_HEADER)Address;
    ValidSplitBlock = FALSE;

    if (Size == PAGE_SIZE && p->PreviousSize == 0 && p->BlockSize != 0) {
        PPOOL_HEADER PoolAddress;
        PPOOL_HEADER EndPoolAddress;

        //
        // Validate all the pool links before we regard this as a page that
        // has been split into small pool blocks.
        //

        PoolAddress = p;
        EndPoolAddress = (PPOOL_HEADER)((PCHAR) p + PAGE_SIZE);

        do {
            EntrySize = PoolAddress->BlockSize << POOL_BLOCK_SHIFT;
            PoolAddress = (PPOOL_HEADER)((PCHAR)PoolAddress + EntrySize);
            if (PoolAddress == EndPoolAddress) {
                ValidSplitBlock = TRUE;
                break;
            }
            if (PoolAddress > EndPoolAddress) {
                break;
            }
            if (PoolAddress->PreviousSize != EntrySize) {
                break;
            }
        } while (EntrySize != 0);
    }

    if (ValidSplitBlock == TRUE) {

        p = (PPOOL_HEADER)Address;

        do {
            EntrySize = p->BlockSize << POOL_BLOCK_SHIFT;

            if (EntrySize == 0) {
                return STATUS_COMMITMENT_LIMIT;
            }

            PoolInformation->NumberOfEntries += 1;
            *RequiredLength += sizeof(SYSTEM_POOL_ENTRY);

            if (Length < *RequiredLength) {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else {
                (*PoolEntryInfo)->Size = EntrySize;
                if (p->PoolType != 0) {
                    (*PoolEntryInfo)->Allocated = TRUE;
                    (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
                    (*PoolEntryInfo)->ProcessChargedQuota = 0;
#if !DBG
                    if (NtGlobalFlag & FLG_POOL_ENABLE_TAGGING)
#endif  //!DBG
                    (*PoolEntryInfo)->TagUlong = p->PoolTag;
                }
                else {
                    (*PoolEntryInfo)->Allocated = FALSE;
                    (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
                    (*PoolEntryInfo)->ProcessChargedQuota = 0;

#if !defined(DBG) && !defined(_WIN64)
                    if (NtGlobalFlag & FLG_POOL_ENABLE_TAGGING)
#endif  //!DBG
                    (*PoolEntryInfo)->TagUlong = p->PoolTag;
                }

                (*PoolEntryInfo) += 1;
                Status = STATUS_SUCCESS;
            }

            p = (PPOOL_HEADER)((PCHAR)p + EntrySize);
        }
        while (PAGE_END(p) == FALSE);

    }
    else {

        PoolInformation->NumberOfEntries += 1;
        *RequiredLength += sizeof(SYSTEM_POOL_ENTRY);
        if (Length < *RequiredLength) {
            Status = STATUS_INFO_LENGTH_MISMATCH;

        } else {
            (*PoolEntryInfo)->Allocated = TRUE;
            (*PoolEntryInfo)->Size = Size;
            (*PoolEntryInfo)->AllocatorBackTraceIndex = 0;
            (*PoolEntryInfo)->ProcessChargedQuota = 0;
            (*PoolEntryInfo) += 1;
            Status = STATUS_SUCCESS;
        }
    }

    return Status;
}

NTSTATUS
ExSnapShotPool(
    IN POOL_TYPE PoolType,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )
{
    ULONG Index;
    PVOID Lock;
    KIRQL LockHandle;
    PPOOL_DESCRIPTOR PoolDesc;
    ULONG RequiredLength;
    NTSTATUS Status;

    RequiredLength = FIELD_OFFSET(SYSTEM_POOL_INFORMATION, Entries);
    if (Length < RequiredLength) {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    try {

        //
        // If the pool type is paged, then lock all of the paged pools.
        // Otherwise, lock the nonpaged pool.
        //

        PoolDesc = PoolVector[PoolType];
        if (PoolType == PagedPool) {
            Index = 0;
            KeRaiseIrql(APC_LEVEL, &LockHandle);                                   \
            do {
                Lock = PoolDesc[Index].LockAddress;
                ExAcquireFastMutex((PFAST_MUTEX)Lock);
                Index += 1;
            } while (Index < ExpNumberOfPagedPools);

        } else {
            ExAcquireSpinLock(&NonPagedPoolLock, &LockHandle);
        }

        PoolInformation->EntryOverhead = POOL_OVERHEAD;
        PoolInformation->NumberOfEntries = 0;

#if POOL_CACHE_SUPPORTED
        if (PoolType & CACHE_ALIGNED_POOL_TYPE_MASK) {
            PoolInformation->EntryOverhead = (USHORT)PoolCacheSize;
        }
#endif //POOL_CACHE_SUPPORTED

        Status = MmSnapShotPool(PoolType,
                                ExpSnapShotPoolPages,
                                PoolInformation,
                                Length,
                                &RequiredLength);

    } finally {

        //
        // If the pool type is paged, then unlock all of the paged pools.
        // Otherwise, unlock the nonpaged pool.
        //

        if (PoolType == PagedPool) {
            Index = 0;
            do {
                Lock = PoolDesc[Index].LockAddress;
                ExReleaseFastMutex((PFAST_MUTEX)Lock);
                Index += 1;
            } while (Index < ExpNumberOfPagedPools);

            KeLowerIrql(LockHandle);

        } else {
            ExReleaseSpinLock(&NonPagedPoolLock, LockHandle);
        }
    }

    if (ARGUMENT_PRESENT(ReturnLength)) {
        *ReturnLength = RequiredLength;
    }

    return Status;
}
#endif // DBG || (i386 && !FPO)

VOID
ExAllocatePoolSanityChecks(
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes
    )

/*++

Routine Description:

    This function performs sanity checks on the caller.

Return Value:

    None.

Environment:

    Only enabled as part of the driver verification package.

--*/

{
    if (NumberOfBytes == 0) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x0,
                      KeGetCurrentIrql(),
                      PoolType,
                      NumberOfBytes);
    }

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {

        if (KeGetCurrentIrql() > APC_LEVEL) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x1,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes);
        }
    }
    else {
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {

            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x2,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes);
        }
    }

    if (PoolType & MUST_SUCCEED_POOL_TYPE_MASK) {
        if (NumberOfBytes > PAGE_SIZE) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x3,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes);
        }
    }
}

VOID
ExFreePoolSanityChecks(
    IN PVOID P
    )

/*++

Routine Description:

    This function performs sanity checks on the caller.

Return Value:

    None.

Environment:

    Only enabled as part of the driver verification package.

--*/

{
    PPOOL_HEADER Entry;
    POOL_TYPE PoolType;
    PVOID StillQueued;

    if (P <= (PVOID)(MM_HIGHEST_USER_ADDRESS)) {
        KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                      0x10,
                      (ULONG_PTR)P,
                      0,
                      0);
    }

    if ((P >= MmSpecialPoolStart) && (P < MmSpecialPoolEnd)) {
        StillQueued = KeCheckForTimer(P, PAGE_SIZE - BYTE_OFFSET (P));
        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x15,
                          (ULONG_PTR)StillQueued,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)P);
        }

        //
        // Check if an ERESOURCE is currently active in this memory block.
        //

        StillQueued = ExpCheckForResource(P, PAGE_SIZE - BYTE_OFFSET (P));
        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x17,
                          (ULONG_PTR)StillQueued,
                          (ULONG_PTR)-1,
                          (ULONG_PTR)P);
        }

        ExpCheckForWorker (P, PAGE_SIZE - BYTE_OFFSET (P)); // bugchecks inside
        return;
    }

    if (PAGE_ALIGNED(P)) {
        PoolType = MmDeterminePoolType(P);

        if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
            if (KeGetCurrentIrql() > APC_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x11,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }
        else {
            if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x12,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }

        //
        // Just check the first page.
        //

        StillQueued = KeCheckForTimer(P, PAGE_SIZE);
        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x15,
                          (ULONG_PTR)StillQueued,
                          PoolType,
                          (ULONG_PTR)P);
        }

        //
        // Check if an ERESOURCE is currently active in this memory block.
        //

        StillQueued = ExpCheckForResource(P, PAGE_SIZE);

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x17,
                          (ULONG_PTR)StillQueued,
                          PoolType,
                          (ULONG_PTR)P);
        }
    }
    else {

#if !defined (_WIN64)
        if (((ULONG_PTR)P & 0x17) != 0) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x16,
                          __LINE__,
                          (ULONG_PTR)P,
                          0);
        }
#endif

        Entry = (PPOOL_HEADER)((PCHAR)P - POOL_OVERHEAD);

        if ((Entry->PoolType & POOL_TYPE_MASK) == 0) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x13,
                          __LINE__,
                          (ULONG_PTR)Entry,
                          Entry->Ulong1);
        }

        PoolType = (Entry->PoolType & POOL_TYPE_MASK) - 1;

        if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
            if (KeGetCurrentIrql() > APC_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x11,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }
        else {
            if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
                KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                              0x12,
                              KeGetCurrentIrql(),
                              PoolType,
                              (ULONG_PTR)P);
            }
        }

        if (!IS_POOL_HEADER_MARKED_ALLOCATED(Entry)) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x14,
                          __LINE__,
                          (ULONG_PTR)Entry,
                          0);
        }
        StillQueued = KeCheckForTimer(Entry, (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));
        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x15,
                          (ULONG_PTR)StillQueued,
                          PoolType,
                          (ULONG_PTR)P);
        }

        //
        // Check if an ERESOURCE is currently active in this memory block.
        //

        StillQueued = ExpCheckForResource(Entry, (ULONG)(Entry->BlockSize << POOL_BLOCK_SHIFT));

        if (StillQueued != NULL) {
            KeBugCheckEx (DRIVER_VERIFIER_DETECTED_VIOLATION,
                          0x17,
                          (ULONG_PTR)StillQueued,
                          PoolType,
                          (ULONG_PTR)P);
        }
    }
}
