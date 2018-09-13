/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    pool.c

Abstract:

    Implementation of the binary buddy pool allocator for the NT executive.

Author:

    Mark Lucovsky     16-feb-1989

Environment:

    kernel mode only

Revision History:

--*/


#if DBG
//#define TRACE_ALLOC 1

#ifdef i386
#define DEADBEEF 1
#endif

#endif // DBG

#include "exp.h"
#include "..\mm\mi.h"

//
// Global Variables
//

#ifdef TRACE_ALLOC
PULONG RtlpGetFramePointer(VOID);

KSEMAPHORE TracePoolLock;
LIST_ENTRY TracePoolListHead[MaxPoolType];

typedef struct _TRACEBUFF {
    PVOID BufferAddress;
    PETHREAD Thread;
    PULONG xR1;
    PULONG xPrevR1;
} TRACEBUFF, *PTRACEBUFF;

#define MAXTRACE 1024

ULONG NextAllocTrace;
ULONG NextDeallocTrace;

TRACEBUFF AllocTrace[MAXTRACE];
TRACEBUFF DeallocTrace[MAXTRACE];


#endif //TRACE_ALLOC

#define POOL_PAGE_SIZE  0x1000
#define POOL_LOG_PAGE   12
#define POOL_LIST_HEADS 8
#define POOL_LOG_MIN    (POOL_LOG_PAGE - POOL_LIST_HEADS + 1)
#define POOL_MIN_ROUND  ( (1<<POOL_LOG_MIN) - 1 )
#define PAGE_ALIGNED(p) (!(((ULONG)p) & (POOL_PAGE_SIZE - 1)))

CHAR PoolIndexTable[128] = { 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4,
                             5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                             7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
                           };

//
// This structure exists in the pool descriptor structure.  There is one of
// these for each pool block size
//

typedef struct _POOL_LIST_HEAD {
    ULONG CurrentFreeLength;
    ULONG Reserved;
    LIST_ENTRY ListHead;
} POOL_LIST_HEAD;
typedef POOL_LIST_HEAD *PPOOL_LIST_HEAD;

typedef struct _POOL_DESCRIPTOR {
    POOL_TYPE PoolType;
    ULONG TotalPages;
    ULONG Threshold;
    PVOID LockAddress;
    POOL_LIST_HEAD ListHeads[POOL_LIST_HEADS];
} POOL_DESCRIPTOR;
typedef POOL_DESCRIPTOR *PPOOL_DESCRIPTOR;

typedef struct _POOL_HEADER {
    USHORT LogAllocationSize;
    USHORT PoolType;
#ifdef TRACE_ALLOC
    LIST_ENTRY TraceLinks;
    PULONG xR1;
    PULONG xPrevR1;
#endif // TRACE_ALLOC
    EPROCESS *ProcessBilled;
} POOL_HEADER;
typedef POOL_HEADER *PPOOL_HEADER;

#define POOL_OVERHEAD sizeof(POOL_HEADER)
#define POOL_BUDDY_MAX (POOL_PAGE_SIZE - POOL_OVERHEAD)

POOL_DESCRIPTOR NonPagedPoolDescriptor,PagedPoolDescriptor;
POOL_DESCRIPTOR NonPagedPoolDescriptorMS,PagedPoolDescriptorMS;

KSPIN_LOCK NonPagedPoolLock;
KMUTEX PagedPoolLock;


PPOOL_DESCRIPTOR PoolVector[MaxPoolType] = {
    &NonPagedPoolDescriptor,
    &PagedPoolDescriptor,
    &NonPagedPoolDescriptorMS,
    &PagedPoolDescriptorMS
    };

POOL_TYPE BasePoolTypeTable[MaxPoolType] = {
    NonPagedPool,
    PagedPool,
    NonPagedPool,
    PagedPool
    };

BOOLEAN MustSucceedPoolTable[MaxPoolType] = {
    FALSE,
    FALSE,
    TRUE,
    TRUE
    };


PPOOL_HEADER
AllocatePoolInternal(
    IN PPOOL_DESCRIPTOR PoolDesc,
    IN LONG Index
    );

VOID
DeallocatePoolInternal(
    IN PPOOL_DESCRIPTOR PoolDesc,
    IN PPOOL_HEADER Entry
    );

PLIST_ENTRY
ExpInterlockedTryAllocatePool(
    IN PLIST_ENTRY List,
    IN KSPIN_LOCK Lock,
    IN ULONG Size,
    IN LONG SizeOffset
    );


//
// LOCK_POOL is used as a macro only within this module
//

#define LOCK_POOL(Lock,PoolType,LockHandle)                                   \
    {                                                                         \
        if ( (PoolType) == (NonPagedPool) || (PoolType) == NonPagedPoolMustSucceed ) {                                 \
            KeAcquireSpinLock((PKSPIN_LOCK)Lock,&LockHandle);                 \
        } else {                                                              \
            KeRaiseIrql(APC_LEVEL,&LockHandle);                               \
            KeWaitForSingleObject(                                            \
                Lock,                                                         \
                PoolAllocation,                                               \
                KernelMode,                                                   \
                FALSE,                                                        \
                NULL                                                          \
                );                                                            \
        }                                                                     \
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

    Opaque - Returns a lock handle that must be returned in a subsequent
             call to ExUnlockPool.

--*/

{

    KIRQL Irql;

    if ( PoolType == NonPagedPool || PoolType == NonPagedPoolMustSucceed ) {

        //
        // Spin Lock locking
        //

        KeAcquireSpinLock((PKSPIN_LOCK)PoolVector[PoolType]->LockAddress, &Irql);
        return Irql;
    } else {

        //
        // Mutex Locking
        //

        KeRaiseIrql(APC_LEVEL, &Irql);

        KeWaitForSingleObject(
            PoolVector[PoolType]->LockAddress,
            PoolAllocation,
            KernelMode,
            FALSE,
            NULL
            );

        return Irql;
    }

}

//
// UNLOCK_POOL is used as a macro only within this module
//

#define UNLOCK_POOL(Lock,PoolType,LockHandle,Wait)                               \
    {                                                                       \
        if ( PoolType == NonPagedPool || (PoolType) == NonPagedPoolMustSucceed ) {                                   \
            KeReleaseSpinLock(                                              \
                Lock,                                                       \
                (KIRQL)LockHandle                                           \
                );                                                          \
        } else {                                                            \
            KeReleaseMutex((PKMUTEX)Lock,Wait);                             \
            KeLowerIrql(LockHandle);                                        \
        }                                                                   \
    }

VOID
ExUnlockPool(
    IN POOL_TYPE PoolType,
    IN KIRQL LockHandle,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function unlocks the pool specified by pool type.  If the value
    of the Wait parameter is true, then the pool's lock is released
    using "wait == true".


Arguments:

    PoolType - Specifies the pool that should be unlocked.

    LockHandle - Specifies the lock handle from a previous call to
                 ExLockPool.

    Wait - Supplies a boolean value that signifies whether the call to
           ExUnlockPool will be immediately followed by a call to one of
           the kernel Wait functions.

Return Value:

    None.

--*/

{

    if ( PoolType == NonPagedPool || (PoolType) == NonPagedPoolMustSucceed ) {

        //
        // Spin Lock locking
        //

        KeReleaseSpinLock(
            (PKSPIN_LOCK)PoolVector[PoolType]->LockAddress,
            LockHandle
            );

    } else {

        //
        // Mutex Locking
        //

        KeReleaseMutex((PKMUTEX)PoolVector[PoolType]->LockAddress,Wait);

        //
        // This could be a problem if wait == true is specified !
        //

        KeLowerIrql(LockHandle);
    }

}

VOID
InitializePool(
    IN POOL_TYPE PoolType,
    IN ULONG Threshold
    )

/*++

Routine Description:

    This procedure initializes a pool descriptor for a binary buddy pool
    type.  Once initialized, the pool may be used for allocation and
    deallocation.

    This function should be called once for each base pool type during
    system initialization.

    Each pool descriptor contains an array of list heads for free
    blocks.  Each list head holds blocks of a particular size.  One list
    head contains page-sized blocks.  The other list heads contain 1/2-
    page-sized blocks, 1/4-page-sized blocks....  A threshold is
    associated with the page-sized list head.  The number of free blocks
    on this list will not grow past the specified threshold.  When a
    deallocation occurs that would cause the threshold to be exceeded,
    the page is returned to the page-aliged pool allocator.

Arguments:

    PoolType - Supplies the type of pool being initialized (e.g.
               nonpaged pool, paged pool...).

    Threshold - Supplies the threshold value for the specified pool.

Return Value:

    None.

--*/

{
    int i;
    POOL_TYPE BasePoolType, MustSucceedPoolType;

    if ( MustSucceedPoolTable[PoolType] ) {
        KeBugCheck(PHASE0_INITIALIZATION_FAILED);
    }

    if (PoolType == NonPagedPool) {

        BasePoolType = NonPagedPool;
        MustSucceedPoolType = NonPagedPoolMustSucceed;

        KeInitializeSpinLock(&PsGetCurrentProcess()->StatisticsLock);
        KeInitializeSpinLock(&NonPagedPoolLock);
        NonPagedPoolDescriptor.LockAddress = (PVOID)&NonPagedPoolLock;
        NonPagedPoolDescriptorMS.LockAddress = (PVOID)&NonPagedPoolLock;

        KeInitializeMutex(&PagedPoolLock,MUTEX_LEVEL_EX_PAGED_POOL);
        PagedPoolDescriptor.LockAddress = (PVOID)&PagedPoolLock;
        PagedPoolDescriptorMS.LockAddress = (PVOID)&PagedPoolLock;

#ifdef TRACE_ALLOC

        KeInitializeSemaphore(&TracePoolLock,1L,1L);
        InitializeListHead(&TracePoolListHead[NonPagedPool]);
        InitializeListHead(&TracePoolListHead[PagedPool]);

#endif // TRACE_ALLOC
    } else {
        BasePoolType = PagedPool;
        MustSucceedPoolType = PagedPoolMustSucceed;
    }

    PoolVector[BasePoolType]->TotalPages = 0;
    PoolVector[BasePoolType]->Threshold = Threshold;
    PoolVector[BasePoolType]->PoolType = BasePoolType;
    PoolVector[MustSucceedPoolType]->TotalPages = 0;
    PoolVector[MustSucceedPoolType]->Threshold = 0;
    PoolVector[MustSucceedPoolType]->PoolType = MustSucceedPoolType;
    for (i=0; i<POOL_LIST_HEADS ;i++ ) {
        InitializeListHead(&PoolVector[BasePoolType]->ListHeads[i].ListHead);
        PoolVector[BasePoolType]->ListHeads[i].CurrentFreeLength = 0;
        InitializeListHead(&PoolVector[MustSucceedPoolType]->ListHeads[i].ListHead);
        PoolVector[MustSucceedPoolType]->ListHeads[i].CurrentFreeLength = 0;
    }
    return;
}

PVOID
ExAllocatePool(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
    )

/*++

Routine Description:

    This function allocates a block of pool of the specified type and
    returns a pointer to the allocated block.  This function is used to
    access both the page-aligned pools, and the binary buddy (less than
    a page) pools.

    If the number of bytes specifies a size that is too large to be
    satisfied by the appropriate binary buddy pool, then the page-aligned
    pool allocator is used.  The allocated block will be page-aligned
    and a page-sized multiple.

    Otherwise, the appropriate binary buddy pool is used.  The allocated
    block will be 64-bit aligned, but will not be page aligned.  The
    binary buddy allocator calculates the smallest block size that is a
    power of two and that can be used to satisfy the request.  If there
    are no blocks available of this size, then a block of the next
    larger block size is allocated and split in half.  One piece is
    placed back into the pool, and the other piece is used to satisfy
    the request.  If the allocator reaches the paged-sized block list,
    and nothing is there, the page-aligned pool allocator is called.
    The page is added to the binary buddy pool...

Arguments:

    PoolType - Supplies the type of pool to allocate.  If the pool type
        is one of the "MustSucceed" pool types, then this call will
        always succeed and return a pointer to allocated pool.
        Otherwise, if the system can not allocate the requested amount
        of memory a NULL is returned.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NULL - The PoolType is not one of the "MustSucceed" pool types, and
        not enough pool exists to satisfy the request.

    NON-NULL - Returns a pointer to the allocated pool.

--*/

{
    PPOOL_HEADER Entry;
    PEPROCESS CurrentProcess;
    KIRQL LockHandle;
    PPOOL_DESCRIPTOR PoolDesc;
    PVOID Lock;
    LONG Index;
    PVOID Block;

    KIRQL OldIrql;
    PMMPTE PointerPte;
    ULONG PageFrameIndex;
    MMPTE TempPte;
    BOOLEAN ReleaseSpinLock = TRUE;

#if defined(TRACE_ALLOC) || defined (DEADBEEF)

    PULONG BadFood;
#endif //TRACE_ALLOC

#ifdef TRACE_ALLOC
    PULONG xFp, xPrevFp, xR1, xPrevR1, xPrevPrevR1;

    xFp = RtlpGetFramePointer();
    xR1 = (PULONG)*(xFp+1);
    xPrevFp = (PULONG)*xFp;
    xPrevR1 = (PULONG)*(xPrevFp+1);
    xPrevFp = (PULONG)*xPrevFp;
    xPrevPrevR1 = (PULONG)*(xPrevFp+1);

#endif // TRACE_ALLOC



    PoolDesc = PoolVector[PoolType];
    Lock = PoolDesc->LockAddress;

    if (NumberOfBytes > POOL_BUDDY_MAX) {

        LOCK_POOL(Lock,PoolType,LockHandle);

        Entry = (PPOOL_HEADER)MiAllocatePoolPages (
                                        BasePoolTypeTable[PoolType],
                                        NumberOfBytes
                                        );
        if ( !Entry && MustSucceedPoolTable[PoolType] ) {
            Entry = (PPOOL_HEADER)MiAllocatePoolPages (
                                            PoolType,
                                            NumberOfBytes
                                            );
        }
        UNLOCK_POOL(Lock,PoolType,LockHandle,FALSE);

        return Entry;
    }

    if (KeGetCurrentIrql() >= 2) {
        DbgPrint("allocating pool at irql >= 2\n");
        ReleaseSpinLock = FALSE;

    } else {
        KeAcquireQueuedSpinLock(LockQueuePfnLock);
    }

    PointerPte = MiReserveSystemPtes (2, SystemPteSpace, 0, 0, TRUE);

            ASSERT (MmAvailablePages > 0);
            PageFrameIndex = MiRemoveAnyPage ();
            TempPte = ValidKernelPte;
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            *PointerPte = TempPte;
            MiInitializePfn (PageFrameIndex, PointerPte, 1L, 1);

    if (ReleaseSpinLock) {
        KeReleaseSpinLock ( &MmPfnLock, OldIrql );
    }

    Entry = (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);

    Entry = (PVOID)(((ULONG)Entry + (PAGE_SIZE - (NumberOfBytes))) &
            0xfffffff8L);

    return Entry;


    Index = ( (NumberOfBytes+POOL_OVERHEAD+POOL_MIN_ROUND) >> POOL_LOG_MIN) - 1;

    Index = PoolIndexTable[Index];

    LOCK_POOL(Lock,PoolType,LockHandle);

    if ( !IsListEmpty(&PoolDesc->ListHeads[Index].ListHead) ) {

        Block = RemoveHeadList(&PoolDesc->ListHeads[Index].ListHead);
        Entry = (PPOOL_HEADER) ((PCH)Block - POOL_OVERHEAD);
        Entry->ProcessBilled = (PEPROCESS)NULL;
        Entry->LogAllocationSize = (USHORT)POOL_LOG_MIN + Index;
        Entry->PoolType = (USHORT)PoolType;
#if defined(TRACE_ALLOC) || defined (DEADBEEF)
        BadFood = (PULONG)Block;
        *BadFood = 0xBAADF00D;
        *(BadFood+1) = 0xBAADF00D;
#endif // TRACE_ALLOC

    } else {

        Entry = AllocatePoolInternal(PoolDesc,Index);

        if ( !Entry ) {
#if DBG
            DbgPrint("EX: ExAllocatePool returning NULL\n");
            DbgBreakPoint();
#endif // DBG
            UNLOCK_POOL(Lock,PoolType,LockHandle,FALSE);
            return NULL;
        }
    }

    UNLOCK_POOL(Lock,PoolType,LockHandle,FALSE);

#ifdef TRACE_ALLOC
        {
                KIRQL xIrql;

                KeRaiseIrql(APC_LEVEL, &xIrql);

                KeWaitForSingleObject(
                    &TracePoolLock,
                    PoolAllocation,
                    KernelMode,
                    FALSE,
                    NULL
                    );

                InsertTailList(&TracePoolListHead[PoolType],&Entry->TraceLinks);

                Entry->xR1 = xR1;
                Entry->xPrevR1 = xPrevR1;
                Entry->ProcessBilled = (PEPROCESS)xPrevPrevR1;

                //Entry->ProcessBilled = (PEPROCESS)NextAllocTrace;

                AllocTrace[NextAllocTrace].BufferAddress = (PCH)Entry + POOL_OVERHEAD;
                AllocTrace[NextAllocTrace].Thread = PsGetCurrentThread();
                AllocTrace[NextAllocTrace].xR1 = xR1;
                AllocTrace[NextAllocTrace++].xPrevR1 = xPrevR1;
                if ( NextAllocTrace >= MAXTRACE ) {
                    NextAllocTrace = 0;
                }


                (VOID) KeReleaseSemaphore(
                            &TracePoolLock,
                            0L,
                            1L,
                            FALSE
                            );
                KeLowerIrql(xIrql);

        }
#endif // TRACE_ALLOC

#if defined(TRACE_ALLOC) || defined (DEADBEEF)
    {
        PULONG NewPool;
        ULONG LongCount;

        Block = (PULONG)((PCH)Entry + POOL_OVERHEAD);
        NewPool = (PULONG) ((PCH)Entry + POOL_OVERHEAD);
        LongCount = (1 << Entry->LogAllocationSize) >> 2;
        LongCount -= (POOL_OVERHEAD>>2);

        while(LongCount--) {
            if ( *NewPool != 0xBAADF00D ) {
                DbgPrint("ExAllocatePool: No BAADF00D Block %lx at %lx\n",
                    Block,
                    NewPool
                    );
                KeBugCheck(0xBADF00D2);
            }
            *NewPool++ = 0xDEADBEEF;
        }
    }
#endif //TRACE_ALLOC

    return ((PCH)Entry + POOL_OVERHEAD);
}

ULONG
ExpAllocatePoolWithQuotaHandler(
    IN NTSTATUS ExceptionCode,
    IN PVOID PoolAddress
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
    return EXCEPTION_CONTINUE_SEARCH;
}



PVOID
ExAllocatePoolWithQuota(
    IN POOL_TYPE PoolType,
    IN ULONG NumberOfBytes
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
        to allocated pool.  Otherwise, if the system can not allocate
        the requested amount of memory a STATUS_INSUFFICIENT_RESOURCES
        status is raised.

    NumberOfBytes - Supplies the number of bytes to allocate.

Return Value:

    NON-NULL - Returns a pointer to the allocated pool.

    Unspecified - If insuffient quota exists to complete the pool
        allocation, the return value is unspecified.

--*/

{
    PVOID p;
    PEPROCESS Process;
    PPOOL_HEADER Entry;
    ULONG AllocationSize;

    p = ExAllocatePool(PoolType,NumberOfBytes);


#ifndef TRACE_ALLOC
    if ( p && !PAGE_ALIGNED(p) ) {

        Entry = (PPOOL_HEADER)((PCH)p - POOL_OVERHEAD);

        Process = PsGetCurrentProcess();

        //
        // Catch exception and back out allocation if necessary
        //

        try {

            PsChargePoolQuota(Process,BasePoolTypeTable[PoolType],(1 << Entry->LogAllocationSize));
            ObReferenceObject(Process);
            Entry->ProcessBilled = Process;

        } except ( ExpAllocatePoolWithQuotaHandler(GetExceptionCode(),p)) {
            KeBugCheck(GetExceptionCode());
        }

    } else {
        if ( !p ) {
            ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
#endif // TRACE_ALLOC

    return p;
}

PPOOL_HEADER
AllocatePoolInternal(
    IN PPOOL_DESCRIPTOR PoolDesc,
    IN LONG Index
    )

/*++

Routine Description:

    This function implements the guts of the binary buddy pool
    allocator.  It is a recursive function.  It assumes that the caller
    (ExAllocatePool) chose the correct pool descriptor, and that the
    allocation should be satisfied from the binary buddy allocator.

    The binary buddy allocator calculates the smallest block size that
    is a power of two and that can be used to satisfy the request.  If
    there are no blocks available of this size, then a block of the next
    larger block size is allocated and split in half.  One piece is
    placed back into the pool, and the other piece is used to satisfy
    the request.  If the allocator reaches the paged sized block list,
    and nothing is there, the page aligned pool allocator is called.
    The page is added to the binary buddy pool...

Arguments:

    PoolDesc - Supplies the address of the pool descriptor to use to
               satisfy the request.

    Index - Supplies the index into the pool descriptor list head array
            that should satisfy an allocation.

Return Value:

    Non-Null - Returns a pointer to the allocated pool header.  Before
               this can be returned to the caller of ExAllocatePool or
               ExAllocatePoolWithQuota, the value must be adjusted
               (incremented) to account for the 64bit binary buddy pool
               overhead.

--*/

{
    LONG Log2N,ShiftedN;
    PPOOL_HEADER Entry,Buddy;
    PPOOL_LIST_HEAD PoolListHead;

#if defined(TRACE_ALLOC) || defined (DEADBEEF)
    PULONG BadFood;
#endif // TRACE_ALLOC

    Log2N = POOL_LOG_MIN + Index;
    ShiftedN = 1 << Log2N;

    PoolListHead = &PoolDesc->ListHeads[Index];

    //
    // this is the correct list head.  See if anything is on the
    // list if so, delink and mark as allocated.  Otherwise,
    // recurse and split.
    //

    if ( !IsListEmpty(&PoolListHead->ListHead) ) {

        //
        // list has an entry, so grab it
        //

        Entry = (PPOOL_HEADER)RemoveHeadList(
                                &PoolListHead->ListHead);

#if defined(TRACE_ALLOC) || defined (DEADBEEF)
        BadFood = (PULONG)Entry;
        *BadFood = 0xBAADF00D;
        *(BadFood+1) = 0xBAADF00D;
#endif // TRACE_ALLOC

        Entry = (PPOOL_HEADER) ((PCH)Entry - POOL_OVERHEAD);

        //
        // allocated entries have a size field set and pool type set
        //

        Entry->LogAllocationSize = (USHORT)Log2N;
        Entry->PoolType = PoolDesc->PoolType;
        Entry->ProcessBilled = (PEPROCESS)NULL;

        return (PVOID)Entry;

    } else {

        if ( Index != (POOL_LIST_HEADS - 1) ) {

            //
            // This is the right list head, but since it is empty
            // must recurse.  Allocate from the next highest entry.
            // The resulting entry is then split in half.  One half
            // is marked as free and added to the list.  The other
            // half is returned
            //

            Entry = (PPOOL_HEADER)AllocatePoolInternal(PoolDesc,Index+1);

            if ( !Entry ) {
                return NULL;
            }

#if defined(TRACE_ALLOC) || defined (DEADBEEF)
            BadFood = (PULONG)((PCH)Entry + POOL_OVERHEAD);
            *BadFood = 0xBAADF00D;
            *(BadFood+1) = 0xBAADF00D;
#endif // TRACE_ALLOC

            Buddy = (PPOOL_HEADER)((PCH)Entry + ShiftedN);

            //
            // mark buddy as free and entry as allocated
            //

            Entry->LogAllocationSize = (USHORT) Log2N;
            Entry->PoolType = PoolDesc->PoolType;
            Entry->ProcessBilled = (PEPROCESS)NULL;

            Buddy->LogAllocationSize = 0;
            Buddy->PoolType = Index;
            Buddy->ProcessBilled = (PEPROCESS)NULL;

            InsertTailList(
               &PoolListHead->ListHead,
               (PLIST_ENTRY)(((PCH)Buddy + POOL_OVERHEAD))
               );

            return (PVOID)Entry;

        } else {

            //
            // Need to call page allocator for a page to add to the pool
            //

            Entry = (PPOOL_HEADER)MiAllocatePoolPages (
                                            BasePoolTypeTable[PoolDesc->PoolType],
                                            PAGE_SIZE
                                            );

            if ( !Entry ) {
                if ( MustSucceedPoolTable[PoolDesc->PoolType] ) {
                    Entry = (PPOOL_HEADER)MiAllocatePoolPages (
                                            PoolDesc->PoolType,
                                            PAGE_SIZE
                                            );
                    ASSERT(Entry);
                } else {
                    return NULL;
                }
            }

            Entry->LogAllocationSize = (USHORT) Log2N;
            Entry->PoolType = PoolDesc->PoolType;
            Entry->ProcessBilled = (PEPROCESS)NULL;

#if defined(TRACE_ALLOC) || defined (DEADBEEF)
            {
                PULONG NewPool;
                ULONG LongCount;

                NewPool = (PULONG) ((PCH)Entry + POOL_OVERHEAD);
                LongCount = (1 << Entry->LogAllocationSize) >> 2;
                LongCount -= (POOL_OVERHEAD>>2);

                while(LongCount--) {
                    *NewPool++ = 0xBAADF00D;
                }
            }
#endif //TRACE_ALLOC

            PoolDesc->TotalPages++;

            return (PVOID)Entry;

        }
    }
}

VOID
ExFreePool(
    IN PVOID P
    )

/*++

Routine Description:

    This function deallocates a block of pool.  This function is used to
    deallocate to both the page aligned pools, and the binary buddy
    (less than a page) pools.

    If the address of the block being deallocated is page-aligned, then
    the page-aliged pool deallocator is used.

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

{
    PPOOL_HEADER Entry;
    POOL_TYPE PoolType;
    KIRQL LockHandle;
    PVOID Lock;
    PPOOL_DESCRIPTOR PoolDesc;

    KIRQL OldIrql;
    BOOLEAN ReleaseSpinLock = TRUE;
    PMMPTE PointerPte;
    PMMPFN Pfn1;

#ifdef TRACE_ALLOC

    PULONG xFp, xPrevFp, xR1, xPrevR1;

    xFp = RtlpGetFramePointer();
    xR1 = (PULONG)*(xFp+1);
    xPrevFp = (PULONG)*xFp;
    xPrevR1 = (PULONG)*(xPrevFp+1);

#endif // TRACE_ALLOC

    //
    // If Entry is page aligned, then call page aligned pool
    //

    if ( PAGE_ALIGNED(P) ) {

        PoolType = MmDeterminePoolType(P);

        Lock = PoolVector[PoolType]->LockAddress;

        LOCK_POOL(Lock,PoolType,LockHandle);
        MiFreePoolPages (P);
        UNLOCK_POOL(Lock,PoolType,LockHandle,FALSE);
        return;
    }

    PointerPte = MiGetPteAddress (P);

    if (PointerPte->u.Hard.Valid == 0) {
        DbgPrint("bad pool deallocation\n");
        KeBugCheck (12345);
    }

    if (KeGetCurrentIrql() >= 2) {
        DbgPrint("deallocating pool at irql >= 2\n");
        ReleaseSpinLock = FALSE;
    } else {
        KeAcquireSpinLock ( &MmPfnLock, &OldIrql);
    }

    KeSweepDcache(TRUE);

    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
    Pfn1->PteAddress = (PMMPTE)MM_EMPTY_LIST;
    MiDecrementShareCountOnly (PointerPte->u.Hard.PageFrameNumber);
    *PointerPte = ZeroPte;
    MiReleaseSystemPtes (PointerPte, 2, SystemPteSpace);
    //
    // BUGBUG Fix for MP.
    //
    KiFlushSingleTb (P,TRUE);
    if (ReleaseSpinLock) {
        KeReleaseSpinLock ( &MmPfnLock, OldIrql );
    }
    return;

    Entry = (PPOOL_HEADER)((PCH)P - POOL_OVERHEAD);

    PoolType = Entry->PoolType;

    PoolDesc = PoolVector[PoolType];
    Lock = PoolDesc->LockAddress;

    //
    // Sanity Check pool header
    //

    if ( (Entry->LogAllocationSize == 0) || (Entry->PoolType >= MaxPoolType) ) {
        DbgPrint("Invalid pool header 0x%lx 0x%lx\n",P,*(PULONG)P);
        KeBugCheck(BAD_POOL_HEADER);
        return;
    }

    if ( (ULONG)P & 0x0000000f != 8 ) {
        DbgPrint("Misaligned Deallocation 0x%lx\n",P);
        KeBugCheck(BAD_POOL_HEADER);
        return;
    }

#ifdef TRACE_ALLOC
    {
        KIRQL xIrql;
        PLIST_ENTRY Next, Target;
        BOOLEAN Found;

        KeRaiseIrql(APC_LEVEL, &xIrql);

        KeWaitForSingleObject(
            &TracePoolLock,
            PoolAllocation,
            KernelMode,
            FALSE,
            NULL
            );

        Found = FALSE;
        Target = &Entry->TraceLinks;
        Next = TracePoolListHead[PoolType].Flink;
        while( Next != &TracePoolListHead[PoolType] ){
            if ( Next == Target ) {

                RemoveEntryList(&Entry->TraceLinks);
                Found = TRUE;
                break;
            }
            Next = Next->Flink;
        }

        if ( !Found ) {
            DbgPrint("Block Not in Allocated Pool List 0x%lx\n",P);
            KeBugCheck(BAD_POOL_HEADER);
            return;
        }

        DeallocTrace[NextDeallocTrace].BufferAddress = P;
        DeallocTrace[NextDeallocTrace].Thread = PsGetCurrentThread();
        DeallocTrace[NextDeallocTrace].xR1 = xR1;
        DeallocTrace[NextDeallocTrace++].xPrevR1 = xPrevR1;
        if ( NextDeallocTrace >= MAXTRACE ) {
            NextDeallocTrace = 0;
        }

        (VOID) KeReleaseSemaphore(
                    &TracePoolLock,
                    0L,
                    1L,
                    FALSE
                    );
        KeLowerIrql(xIrql);

    }
#endif // TRACE_ALLOC

#ifndef TRACE_ALLOC

    //
    // Check ProcessBilled flag to see if quota was charged on
    // this allocation
    //

    if ( Entry->ProcessBilled ) {

        PsReturnPoolQuota(
            Entry->ProcessBilled,
            BasePoolTypeTable[PoolType],
            (1 << Entry->LogAllocationSize)
            );

        ObDereferenceObject(Entry->ProcessBilled);

    }
#endif // TRACE_ALLOC

    LOCK_POOL(Lock,PoolType,LockHandle);

    DeallocatePoolInternal(PoolDesc,Entry);

    UNLOCK_POOL(Lock,PoolType,LockHandle,FALSE);

}

VOID
DeallocatePoolInternal(
    IN PPOOL_DESCRIPTOR PoolDesc,
    IN PPOOL_HEADER Entry
    )

/*++

Routine Description:

    This function implements the guts of the binary buddy pool
    deallocator.  It is a recursive function.  It assumes that the
    caller (ExFreePool) chose the correct pool descriptor, and that
    the deallocation should be done using the binary buddy deallocator.

    The binary buddy deallocator automatically collapses adjacent free
    blocks of the same size into a single free block of the next larger
    size.  This is a recursive operation.  This does not occur on a
    deallocation of a page sized block.  If a page sized block is being
    deallocated, then the pool descriptor's threshold is examined.  If
    the deallocation would cause the threshold to be exceeded, the page
    sized block is returned to the page aligned pool allocator.

Arguments:

    PoolDesc - Supplies the address of the pool descriptor to use to
               satisfy the request.

    Entry - Supplies the address of the pool header of the block of pool
            being deallocated.

Return Value:

    None.

--*/

{
    PPOOL_HEADER Buddy,Base;
    LONG index;
    ULONG EntrySize;

    //
    // Locate buddy of Entry being deallocated.  Page size entries have no
    // buddy
    //


    index = Entry->LogAllocationSize - POOL_LOG_MIN;

    EntrySize = (1L << Entry->LogAllocationSize);

    if ( EntrySize < POOL_PAGE_SIZE ) {

        //
        // buddy of Entry is LogAllocationSize bytes from Entry rounded
        // down on a 2*LogAllocationSize boundry
        //

        Buddy = (PPOOL_HEADER)((ULONG)Entry ^ EntrySize);

        //
        // see if buddy is free.  If so, then join buddy and Entry and
        // deallocate the joined Entry
        //

        if ( Buddy->LogAllocationSize == 0 &&
             Buddy->PoolType == index ) {

            //
            // buddy is free.  Figure out which Entry is the base.
            // Convert the size of the base to next larger size and
            // deallocate the new Entry
            //

            Base = ((ULONG)Entry & EntrySize) ? Buddy : Entry;

            RemoveEntryList((PLIST_ENTRY)((PCH)Buddy + POOL_OVERHEAD));

            //
            // Update statistics for buddy's list head.
            //

            PoolDesc->ListHeads[index].CurrentFreeLength--;

            //
            // mark base as allocated as next size Entry and then deallocate it
            //

            Base->LogAllocationSize = Entry->LogAllocationSize + 1;

            DeallocatePoolInternal(PoolDesc,Base);

        } else {

            //
            // Buddy is not free, so just mark Entry as free and return to
            // appropriate list head
            //

#if defined(TRACE_ALLOC) || defined (DEADBEEF)
            {
                PULONG OldPool;
                ULONG LongCount;

                OldPool = (PULONG)((PCH)Entry + POOL_OVERHEAD);
                LongCount = EntrySize >> 2;
                LongCount -= (POOL_OVERHEAD>>2);

                while(LongCount--) {
                    *OldPool++ = 0xBAADF00D;
                }
            }
#endif //TRACE_ALLOC

            InsertTailList(
                &PoolDesc->ListHeads[index].ListHead,
                (PLIST_ENTRY)((PCH)Entry + POOL_OVERHEAD)
                );

            PoolDesc->ListHeads[index].CurrentFreeLength++;

            Entry->LogAllocationSize = 0;
            Entry->PoolType = index;
        }

    } else {

        //
        // Page Sized Entry.  Check threshold.  If deallocating Entry
        // would cross threshold then give back the page.  Otherwise put
        // it on the free list
        //

        if ( PoolDesc->ListHeads[index].CurrentFreeLength == PoolDesc->Threshold ) {

            MiFreePoolPages (Entry);

        } else {

            //
            // so just mark Entry as free and return to appropriate list head
            //

#if defined(TRACE_ALLOC) || defined (DEADBEEF)
            {
                PULONG OldPool;
                ULONG LongCount;

                OldPool = (PULONG)((PCH)Entry + POOL_OVERHEAD);
                LongCount = EntrySize >> 2;
                LongCount -= (POOL_OVERHEAD>>2);

                while(LongCount--) {
                    *OldPool++ = 0xBAADF00D;
                }
            }
#endif //TRACE_ALLOC

            InsertTailList(
                &PoolDesc->ListHeads[index].ListHead,
                (PLIST_ENTRY)((PCH)Entry + POOL_OVERHEAD)
                );

            PoolDesc->ListHeads[index].CurrentFreeLength++;

            Entry->LogAllocationSize = 0;
            Entry->PoolType = index;

        }
    }
}


VOID
DumpPool(
    IN PSZ s,
    IN POOL_TYPE pt
    )
{
    PPOOL_DESCRIPTOR pd;
    PPOOL_HEADER ph,bph;
    PPOOL_LIST_HEAD plh;
    PLIST_ENTRY lh,next;
    LONG i;
    ULONG size;

    pd = PoolVector[pt];

    DbgPrint("\n\n%s\n",s);

    DbgPrint("PoolType: 0x%lx\n",(ULONG)pd->PoolType);
    DbgPrint("TotalPages: 0x%lx\n",pd->TotalPages);
    DbgPrint("Threshold: 0x%lx\n",pd->Threshold);
    for (i=0; i<POOL_LIST_HEADS; i++ ) {
        plh = &pd->ListHeads[i];
        size = (1 << (i + POOL_LOG_MIN));
        DbgPrint("\npd_list_head[0x%lx] size 0x%lx\n",i,size);
        DbgPrint("\tCurrentFreeLength 0x%lx\n",plh->CurrentFreeLength);
        DbgPrint("\t&ListHead 0x%lx\n",&plh->ListHead);
        lh = &plh->ListHead;
        DbgPrint("\t\tpFlink 0x%lx\n",lh->Flink);
        DbgPrint("\t\tpBlink 0x%lx\n",lh->Blink);
        next = lh->Flink;
        while ( next != lh ) {
            ph = (PPOOL_HEADER)((PCH)next - POOL_OVERHEAD);
            DbgPrint("\t\t\tpool header at 0x%lx list 0x%lx\n",ph,next);
            DbgPrint("\t\t\tLogAllocationSize 0x%lx 0x%lx\n",(ULONG)ph->LogAllocationSize,(ULONG)(1<<ph->LogAllocationSize));
            DbgPrint("\t\t\tPoolType 0x%lx\n",(ULONG)ph->PoolType);
            DbgPrint("\t\t\tProcessBilled 0x%lx\n",ph->ProcessBilled);
            if ( size != POOL_PAGE_SIZE ) {
                bph = (PPOOL_HEADER)((ULONG)ph ^ size);
                DbgPrint("\t\t\t\tBuddy pool header at 0x%lx\n",bph);
                DbgPrint("\t\t\t\tBuddy LogAllocationSize 0x%lx 0x%lx\n",(ULONG)bph->LogAllocationSize,(ULONG)(1<<bph->LogAllocationSize));
                DbgPrint("\t\t\t\tBuddy PoolType 0x%lx\n",(ULONG)bph->PoolType);
                DbgPrint("\t\t\t\tBuddy ProcessBilled 0x%lx\n",bph->ProcessBilled);
            }
            next = next->Flink;
        }
    }
}


VOID
CheckPool()
{
    PPOOL_DESCRIPTOR pd;
    PPOOL_HEADER ph,bph;
    PPOOL_LIST_HEAD plh;
    PLIST_ENTRY lh,next,lh2,next2;
    BOOLEAN buddyinlist;
    LONG i,j;
    ULONG size;

    pd = PoolVector[NonPagedPool];

    if ( pd->PoolType != NonPagedPool ) {
        DbgPrint("pd = %8lx\n", pd);
        KeBugCheck(0x70000001);
    }

    if ( (LONG) pd->TotalPages < 0 ) {
        DbgPrint("pd = %8lx\n", pd);
        KeBugCheck(0x70000002);
    }

    for (i=0; i<POOL_LIST_HEADS; i++ ) {
        plh = &pd->ListHeads[i];
        size = (1 << (i + POOL_LOG_MIN));

        if ( !IsListEmpty(&plh->ListHead) ) {

            lh = &plh->ListHead;
            next = lh->Flink;
            while ( next != lh ) {
                ph = (PPOOL_HEADER)((PCH)next - POOL_OVERHEAD);

                if ( MmDeterminePoolType(ph) != NonPagedPool ) {
                    DbgPrint("ph = %8lx\n", ph);
                    KeBugCheck(0x70000004);
                }
                if ( size != POOL_PAGE_SIZE ) {
                    bph = (PPOOL_HEADER)((ULONG)ph ^ size);
                    if ( bph->LogAllocationSize == 0 &&
                         bph->PoolType == i ) {
                        lh2 = &plh->ListHead;
                        next2 = lh2->Flink;
                        buddyinlist = FALSE;
                        while ( next2 != lh2 ) {
                            ph = (PPOOL_HEADER)((PCH)next - POOL_OVERHEAD);
                            if ( bph == ph ) {
                                buddyinlist = TRUE;
                            }
                            next2 = next2->Flink;
                        }
                        if ( !buddyinlist ) {
                            KeBugCheck(0x70000005);
                        }
                    }
                }
                if ( next == next->Flink ) {
                    DbgPrint("next = %8lx\n", next);
                    KeBugCheck(0x70000006);
                }
                next = next->Flink;
            }
        }

    }
}
VOID
DumpAllocatedPool(
    IN ULONG DumpOrFlush
    )
{
    VOID MiFlushUnusedSections( VOID );
    POOL_TYPE pt;
    ULONG PoolUsage[MaxPoolType];
    ULONG PoolFree[MaxPoolType];

    PPOOL_DESCRIPTOR pd;
    PPOOL_LIST_HEAD plh;
    PLIST_ENTRY lh,next;
    LONG i,j,k;
    ULONG size;

    if ( DumpOrFlush ) {
        MiFlushUnusedSections();
        return;
    }

    DbgPrint ("PoolHack does not work with POOL command\n");

    return;
}

VOID
ExQueryPoolUsage(
    OUT PULONG PagedPoolPages,
    OUT PULONG NonPagedPoolPages
    )
{
    PPOOL_DESCRIPTOR pd;

    pd = PoolVector[PagedPool];
    *PagedPoolPages = pd->TotalPages;
    pd = PoolVector[PagedPoolMustSucceed];
    *PagedPoolPages += pd->TotalPages;

    pd = PoolVector[NonPagedPool];
    *NonPagedPoolPages = pd->TotalPages;
    pd = PoolVector[NonPagedPoolMustSucceed];
    *NonPagedPoolPages += pd->TotalPages;

}
