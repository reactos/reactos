/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   allocpag.c

Abstract:

    This module contains the routines which allocate and deallocate
    one or more pages from paged or nonpaged pool.

Author:

    Lou Perazzoli (loup) 6-Apr-1989
    Landy Wang (landyw) 02-June-1997

Revision History:

--*/

#include "mi.h"

VOID
MiInitializeSpecialPool (
    VOID
    );

#ifndef NO_POOL_CHECKS
VOID
MiInitializeSpecialPoolCriteria (
    IN VOID
    );

VOID
MiSpecialPoolTimerDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

PVOID
MmSqueezeBadTags (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    );
#endif

LOGICAL
MmSetSpecialPool (
    IN LOGICAL Enable
    );

PVOID
MiAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    );

VOID
MmFreeSpecialPool (
    IN PVOID P
    );

LOGICAL
MiProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    );

VOID
MiMakeSpecialPoolPagable (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, MiInitializeNonPagedPool)
#pragma alloc_text(INIT, MiInitializeSpecialPool)

#pragma alloc_text(PAGELK, MiFindContiguousMemory)

#pragma alloc_text(PAGEHYDRA, MiCheckSessionPoolAllocations)
#pragma alloc_text(PAGEHYDRA, MiSessionPoolAllocated)
#pragma alloc_text(PAGEHYDRA, MiSessionPoolFreed)
#pragma alloc_text(PAGEHYDRA, MiSessionPoolVector)
#pragma alloc_text(PAGEHYDRA, MiInitializeSessionPool)
#pragma alloc_text(PAGEHYDRA, MiFreeSessionPoolBitMaps)

#pragma alloc_text(PAGESPEC, MmFreeSpecialPool)
#pragma alloc_text(PAGESPEC, MiAllocateSpecialPool)
#pragma alloc_text(PAGESPEC, MiMakeSpecialPoolPagable)
#pragma alloc_text(PAGESPEC, MiProtectSpecialPool)

#pragma alloc_text(POOLMI, MiAllocatePoolPages)
#pragma alloc_text(POOLMI, MiFreePoolPages)

#if DBG || (i386 && !FPO)
#pragma alloc_text(PAGELK, MmSnapShotPool)
#endif // DBG || (i386 && !FPO)
#endif

ULONG MmPagedPoolCommit;        // used by the debugger

PFN_NUMBER MmAllocatedNonPagedPool;
PFN_NUMBER MiEndOfInitialPoolFrame;

PVOID MmNonPagedPoolExpansionStart;

LIST_ENTRY MmNonPagedPoolFreeListHead[MI_MAX_FREE_LIST_HEADS];

extern POOL_DESCRIPTOR NonPagedPoolDescriptor;

extern LOGICAL MmPagedPoolMaximumDesired;

#define MM_SMALL_ALLOCATIONS 4

#if DBG

//
// Set this to a nonzero (ie: 10000) value to cause every pool allocation to
// be checked and an ASSERT fires if the allocation is larger than this value.
//

ULONG MmCheckRequestInPages = 0;

//
// Set this to a nonzero (ie: 0x23456789) value to cause this pattern to be
// written into freed nonpaged pool pages.
//

ULONG MiFillFreedPool = 0;
#endif

extern ULONG MmUnusedSegmentForceFree;

#define MI_MEMORY_MAKER(Thread) \
    ((Thread->StartAddress == (PVOID)MiModifiedPageWriter) || \
     (Thread->StartAddress == (PVOID)MiMappedPageWriter) || \
     (Thread->StartAddress == (PVOID)MiDereferenceSegmentThread))


VOID
MiProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    )

/*++

Routine Description:

    This function protects freed nonpaged pool.

Arguments:

    VirtualAddress - Supplies the freed pool address to protect.

    SizeInPages - Supplies the size of the request in pages.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    MMPTE PteContents;
    PMMPTE PointerPte;

    //
    // Prevent anyone from touching the free non paged pool
    //

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress) == 0) {
        PointerPte = MiGetPteAddress (VirtualAddress);

        for (i = 0; i < SizeInPages; i += 1) {

            PteContents = *PointerPte;

            PteContents.u.Hard.Valid = 0;
            PteContents.u.Soft.Prototype = 1;
    
            KeFlushSingleTb (VirtualAddress,
                             TRUE,
                             TRUE,
                             (PHARDWARE_PTE)PointerPte,
                             PteContents.u.Flush);
            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
            PointerPte += 1;
        }
    }
}


LOGICAL
MiUnProtectFreeNonPagedPool (
    IN PVOID VirtualAddress,
    IN ULONG SizeInPages
    )

/*++

Routine Description:

    This function unprotects freed nonpaged pool.

Arguments:

    VirtualAddress - Supplies the freed pool address to unprotect.

    SizeInPages - Supplies the size of the request in pages - zero indicates
                  to keep going until there are no more protected PTEs (ie: the
                  caller doesn't know how many protected PTEs there are).

Return Value:

    TRUE if pages were unprotected, FALSE if not.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    MMPTE PteContents;
    ULONG PagesDone;

    PagesDone = 0;

    //
    // Unprotect the previously freed pool so it can be manipulated
    //

    if (MI_IS_PHYSICAL_ADDRESS(VirtualAddress) == 0) {

        PointerPte = MiGetPteAddress((PVOID)VirtualAddress);

        PteContents = *PointerPte;

        while (PteContents.u.Hard.Valid == 0 && PteContents.u.Soft.Prototype == 1) {

            PteContents.u.Hard.Valid = 1;
            PteContents.u.Soft.Prototype = 0;
    
            MI_WRITE_VALID_PTE (PointerPte, PteContents);

            PagesDone += 1;

            if (PagesDone == SizeInPages) {
                break;
            }

            PointerPte += 1;
            PteContents = *PointerPte;
        }
    }

    if (PagesDone == 0) {
        return FALSE;
    }

    return TRUE;
}


VOID
MiProtectedPoolInsertList (
    IN PLIST_ENTRY ListHead,
    IN PLIST_ENTRY Entry,
    IN LOGICAL InsertHead
    )

/*++

Routine Description:

    This function inserts the entry into the protected list.

Arguments:

    ListHead - Supplies the list head to add onto.

    Entry - Supplies the list entry to insert.

    InsertHead - If TRUE, insert at the head otherwise at the tail.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    PVOID FreeFlink;
    PVOID FreeBlink;
    PVOID VirtualAddress;

    //
    // Either the flink or the blink may be pointing
    // at protected nonpaged pool.  Unprotect now.
    //

    FreeFlink = (PVOID)0;
    FreeBlink = (PVOID)0;

    if (IsListEmpty(ListHead) == 0) {

        VirtualAddress = (PVOID)ListHead->Flink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeFlink = VirtualAddress;
        }
    }

    if (((PVOID)Entry == ListHead->Blink) == 0) {
        VirtualAddress = (PVOID)ListHead->Blink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeBlink = VirtualAddress;
        }
    }

    if (InsertHead == TRUE) {
        InsertHeadList (ListHead, Entry);
    }
    else {
        InsertTailList (ListHead, Entry);
    }

    if (FreeFlink) {
        //
        // Reprotect the flink.
        //

        MiProtectFreeNonPagedPool (FreeFlink, 1);
    }

    if (FreeBlink) {
        //
        // Reprotect the blink.
        //

        MiProtectFreeNonPagedPool (FreeBlink, 1);
    }
}


VOID
MiProtectedPoolRemoveEntryList (
    IN PLIST_ENTRY Entry
    )

/*++

Routine Description:

    This function unlinks the list pointer from protected freed nonpaged pool.

Arguments:

    Entry - Supplies the list entry to remove.

Return Value:

    None.

Environment:

    Kernel mode.

--*/
{
    PVOID FreeFlink;
    PVOID FreeBlink;
    PVOID VirtualAddress;

    //
    // Either the flink or the blink may be pointing
    // at protected nonpaged pool.  Unprotect now.
    //

    FreeFlink = (PVOID)0;
    FreeBlink = (PVOID)0;

    if (IsListEmpty(Entry) == 0) {

        VirtualAddress = (PVOID)Entry->Flink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeFlink = VirtualAddress;
        }
    }

    if (((PVOID)Entry == Entry->Blink) == 0) {
        VirtualAddress = (PVOID)Entry->Blink;
        if (MiUnProtectFreeNonPagedPool (VirtualAddress, 1) == TRUE) {
            FreeBlink = VirtualAddress;
        }
    }

    RemoveEntryList (Entry);

    if (FreeFlink) {
        //
        // Reprotect the flink.
        //

        MiProtectFreeNonPagedPool (FreeFlink, 1);
    }

    if (FreeBlink) {
        //
        // Reprotect the blink.
        //

        MiProtectFreeNonPagedPool (FreeBlink, 1);
    }
}


POOL_TYPE
MmDeterminePoolType (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function determines which pool a virtual address resides within.

Arguments:

    VirtualAddress - Supplies the virtual address to determine which pool
                     it resides within.

Return Value:

    Returns the POOL_TYPE (PagedPool, NonPagedPool, PagedPoolSession or
            NonPagedPoolSession), it never returns any information about
            MustSucceed pool types.

Environment:

    Kernel Mode Only.

--*/

{
    if ((VirtualAddress >= MmPagedPoolStart) &&
        (VirtualAddress <= MmPagedPoolEnd)) {
        return PagedPool;
    }

    if (MI_IS_SESSION_POOL_ADDRESS (VirtualAddress) == TRUE) {
        return PagedPoolSession;
    }

    return NonPagedPool;
}


PVOID
MiSessionPoolVector(
    VOID
    )

/*++

Routine Description:

    This function returns the session pool descriptor for the current session.

Arguments:

    None.

Return Value:

    Pool descriptor.

--*/

{
    return (PVOID)&MmSessionSpace->PagedPool;
}


VOID
MiSessionPoolAllocated(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function charges the new pool allocation for the current session.
    On session exit, this charge must be zero.

Arguments:

    VirtualAddress - Supplies the allocated pool address.

    NumberOfBytes - Supplies the number of bytes allocated.

    PoolType - Supplies the type of the above pool allocation.

Return Value:

    None.

--*/

{
    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == FALSE);
        MmSessionSpace->NonPagedPoolBytes += NumberOfBytes;
        MmSessionSpace->NonPagedPoolAllocations += 1;
    }
    else {
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == TRUE);
        MmSessionSpace->PagedPoolBytes += NumberOfBytes;
        MmSessionSpace->PagedPoolAllocations += 1;
    }
}


VOID
MiSessionPoolFreed(
    IN PVOID VirtualAddress,
    IN SIZE_T NumberOfBytes,
    IN POOL_TYPE PoolType
    )

/*++

Routine Description:

    This function returns the specified pool allocation for the current session.
    On session exit, this charge must be zero.

Arguments:

    VirtualAddress - Supplies the pool address being freed.

    NumberOfBytes - Supplies the number of bytes being freed.

    PoolType - Supplies the type of the above pool allocation.

Return Value:

    None.

--*/

{
    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == FALSE);
        MmSessionSpace->NonPagedPoolBytes -= NumberOfBytes;
        MmSessionSpace->NonPagedPoolAllocations -= 1;
    }
    else {
        ASSERT (MI_IS_SESSION_POOL_ADDRESS(VirtualAddress) == TRUE);
        MmSessionSpace->PagedPoolBytes -= NumberOfBytes;
        MmSessionSpace->PagedPoolAllocations -= 1;
    }
}


LOGICAL
MmResourcesAvailable (
    IN POOL_TYPE PoolType,
    IN SIZE_T NumberOfBytes,
    IN EX_POOL_PRIORITY Priority
    )

/*++

Routine Description:

    This function examines various resources to determine if this
    pool allocation should be allowed to proceed.

Arguments:

    PoolType - Supplies the type of pool to retrieve information about.

    NumberOfBytes - Supplies the number of bytes to allocate.

    Priority - Supplies an indication as to how important it is that this
               request succeed under low available resource conditions.                       
Return Value:

    TRUE if the pool allocation should be allowed to proceed, FALSE if not.

--*/

{
    KIRQL OldIrql;
    PFN_NUMBER NumberOfPages;
    SIZE_T FreePoolInBytes;
    PETHREAD Thread;
    LOGICAL SignalDereferenceThread;

    ASSERT (Priority != HighPoolPriority);
    ASSERT ((PoolType & MUST_SUCCEED_POOL_TYPE_MASK) == 0);

    NumberOfPages = BYTES_TO_PAGES (NumberOfBytes);

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {
        FreePoolInBytes = MmMaximumNonPagedPoolInBytes - (MmAllocatedNonPagedPool << PAGE_SHIFT);
    }
    else if (PoolType & SESSION_POOL_MASK) {
        FreePoolInBytes = MI_SESSION_POOL_SIZE - MmSessionSpace->PagedPoolBytes;
    }
    else {
        FreePoolInBytes = MmSizeOfPagedPoolInBytes - (MmPagedPoolInfo.AllocatedPagedPool << PAGE_SHIFT);
    }

    //
    // Check available VA space.
    //

    if (Priority == NormalPoolPriority) {
        if ((SIZE_T)NumberOfBytes + 512*1024 > FreePoolInBytes) {
            Thread = PsGetCurrentThread ();
            if (!MI_MEMORY_MAKER(Thread)) {
                goto nopool;
            }
        }
    }
    else {
        if ((SIZE_T)NumberOfBytes + 2*1024*1024 > FreePoolInBytes) {
            Thread = PsGetCurrentThread ();
            if (!MI_MEMORY_MAKER(Thread)) {
                goto nopool;
            }
        }
    }

    //
    // Paged allocations (session and normal) can also fail for lack of commit.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        if (MmTotalCommittedPages + NumberOfPages > MmTotalCommitLimitMaximum) {
            Thread = PsGetCurrentThread ();
            if (!MI_MEMORY_MAKER(Thread)) {
                MiIssuePageExtendRequestNoWait (NumberOfPages);
                goto nopool;
            }
        }
    }

    return TRUE;

nopool:

    //
    // Running low on pool - if this request is not for session pool,
    // force unused segment trimming when appropriate.
    //

    if ((PoolType & SESSION_POOL_MASK) == 0) {

        if (MI_UNUSED_SEGMENTS_SURPLUS()) {
            KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
        }
        else {
            SignalDereferenceThread = FALSE;
            LOCK_PFN2 (OldIrql);
            if (MmUnusedSegmentForceFree == 0) {
                if (!IsListEmpty(&MmUnusedSegmentList)) {
                    SignalDereferenceThread = TRUE;
                    MmUnusedSegmentForceFree = 30;
                }
            }
            UNLOCK_PFN2 (OldIrql);
            if (SignalDereferenceThread == TRUE) {
                KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
            }
        }
    }

    return FALSE;
}


VOID
MiFreeNonPagedPool (
    IN PVOID StartingAddress,
    IN PFN_NUMBER NumberOfPages
    )

/*++

Routine Description:

    This function releases virtually mapped nonpaged expansion pool.

Arguments:

    StartingAddress - Supplies the starting address.

    NumberOfPages - Supplies the number of pages to free.

Return Value:

    None.

Environment:

    These functions are used by the internal Mm page allocation/free routines
    only and should not be called directly.

    Mutexes guarding the pool databases must be held when calling
    this function.

--*/

{
    PFN_NUMBER i;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PFN_NUMBER PageFrameIndex;

    MI_MAKING_MULTIPLE_PTES_INVALID (TRUE);

    PointerPte = MiGetPteAddress (StartingAddress);

    //
    // Return commitment.
    //

    MiReturnCommitment (NumberOfPages);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_NONPAGED_POOL_EXPANSION,
                     NumberOfPages);

    LOCK_PFN2 (OldIrql);

    for (i = 0; i < NumberOfPages; i += 1) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);

        //
        // Set the pointer to the PTE as empty so the page
        // is deleted when the reference count goes to zero.
        //

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        ASSERT (Pfn1->u2.ShareCount == 1);
        Pfn1->u2.ShareCount = 0;
        MI_SET_PFN_DELETED (Pfn1);
#if DBG
        Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
        MiDecrementReferenceCount (PageFrameIndex);

        (VOID)KeFlushSingleTb (StartingAddress,
                               TRUE,
                               TRUE,
                               (PHARDWARE_PTE)PointerPte,
                               ZeroKernelPte.u.Flush);

        StartingAddress = (PVOID)((PCHAR)StartingAddress + PAGE_SIZE);
        PointerPte += 1;
    }

    //
    // Update the count of available resident pages.
    //

    MmResidentAvailablePages += NumberOfPages;
    MM_BUMP_COUNTER(2, NumberOfPages);

    UNLOCK_PFN2(OldIrql);

    PointerPte -= NumberOfPages;

    MiReleaseSystemPtes (PointerPte,
                         (ULONG)NumberOfPages,
                         NonPagedPoolExpansion);
}


PVOID
MiAllocatePoolPages (
    IN POOL_TYPE PoolType,
    IN SIZE_T SizeInBytes,
    IN ULONG IsLargeSessionAllocation
    )

/*++

Routine Description:

    This function allocates a set of pages from the specified pool
-   and returns the starting virtual address to the caller.

    For the NonPagedPoolMustSucceed case, the caller must first
    attempt to get NonPagedPool and if and ONLY IF that fails, then
    MiAllocatePoolPages should be called again with the PoolType of
    NonPagedPoolMustSucceed.

Arguments:

    PoolType - Supplies the type of pool from which to obtain pages.

    SizeInBytes - Supplies the size of the request in bytes.  The actual
                  size returned is rounded up to a page boundary.

    IsLargeSessionAllocation - Supplies nonzero if the allocation is a single
                               large session allocation.  Zero otherwise.

Return Value:

    Returns a pointer to the allocated pool, or NULL if no more pool is
    available.

Environment:

    These functions are used by the general pool allocation routines
    and should not be called directly.

    Mutexes guarding the pool databases must be held when calling
    these functions.

    Kernel mode, IRQL at DISPATCH_LEVEL.

--*/

{
    PFN_NUMBER SizeInPages;
    ULONG StartPosition;
    ULONG EndPosition;
    PMMPTE StartingPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PVOID BaseVa;
    KIRQL OldIrql;
    KIRQL SessionIrql;
    PFN_NUMBER i;
    PLIST_ENTRY Entry;
    PMMFREE_POOL_ENTRY FreePageInfo;
    PMM_SESSION_SPACE SessionSpace;
    PMM_PAGED_POOL_INFO PagedPoolInfo;
    PVOID VirtualAddress;
    ULONG Index;
    PMMPTE SessionPte;
    ULONG WsEntry;
    ULONG WsSwapEntry;
    ULONG PageTableCount;
    LOGICAL FreedPool;
    LOGICAL SignalDereferenceThread;
    PETHREAD Thread;

    SizeInPages = BYTES_TO_PAGES (SizeInBytes);

#if DBG
    if (MmCheckRequestInPages != 0) {
        ASSERT (SizeInPages < MmCheckRequestInPages);
    }
#endif

    if (PoolType & MUST_SUCCEED_POOL_TYPE_MASK) {

        //
        // Pool expansion failed, see if any Must Succeed
        // pool is still left.
        //

        if (MmNonPagedMustSucceed == NULL) {

            //
            // No more pool exists.  Bug Check.
            //

            KeBugCheckEx (MUST_SUCCEED_POOL_EMPTY,
                          SizeInBytes,
                          NonPagedPoolDescriptor.TotalPages,
                          NonPagedPoolDescriptor.TotalBigPages,
                          MmAvailablePages);
        }

        //
        // Remove a page from the must succeed pool.  More than one is illegal.
        //

        if (SizeInBytes > PAGE_SIZE) {
            KeBugCheckEx (BAD_POOL_CALLER,
                          0x98,
                          (ULONG_PTR)SizeInBytes,
                          (ULONG_PTR)SizeInPages,
                          PoolType);
        }

        BaseVa = MmNonPagedMustSucceed;

        if (IsLargeSessionAllocation != 0) {

            //
            // Mark this as a large session allocation in the PFN database.
            //

            if (MI_IS_PHYSICAL_ADDRESS(BaseVa)) {
                PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BaseVa);
            } else {
                PointerPte = MiGetPteAddress(BaseVa);
                ASSERT (PointerPte->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            }
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);

            CONSISTENCY_LOCK_PFN2 (OldIrql);

            Pfn1->u3.e1.LargeSessionAllocation = 1;

            CONSISTENCY_UNLOCK_PFN2 (OldIrql);

            MiSessionPoolAllocated (BaseVa, PAGE_SIZE, NonPagedPool);
        }
        else if (PoolType & POOL_VERIFIER_MASK) {

            if (MI_IS_PHYSICAL_ADDRESS(BaseVa)) {
                PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BaseVa);
            } else {
                PointerPte = MiGetPteAddress(BaseVa);
                ASSERT (PointerPte->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            }

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
            Pfn1->u3.e1.VerifierAllocation = 1;
        }

        MmNonPagedMustSucceed = (PVOID)(*(PULONG_PTR)BaseVa);
        return BaseVa;
    }

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {

        Index = (ULONG)(SizeInPages - 1);

        if (Index >= MI_MAX_FREE_LIST_HEADS) {
            Index = MI_MAX_FREE_LIST_HEADS - 1;
        }

        //
        // NonPaged pool is linked together through the pages themselves.
        //

        while (Index < MI_MAX_FREE_LIST_HEADS) {

            Entry = MmNonPagedPoolFreeListHead[Index].Flink;

            while (Entry != &MmNonPagedPoolFreeListHead[Index]) {

                if (MmProtectFreedNonPagedPool == TRUE) {
                    MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
                }
    
                //
                // The list is not empty, see if this one has enough space.
                //
    
                FreePageInfo = CONTAINING_RECORD(Entry,
                                                 MMFREE_POOL_ENTRY,
                                                 List);
    
                ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);
                if (FreePageInfo->Size >= SizeInPages) {
    
                    //
                    // This entry has sufficient space, remove
                    // the pages from the end of the allocation.
                    //
    
                    FreePageInfo->Size -= SizeInPages;
    
                    BaseVa = (PVOID)((PCHAR)FreePageInfo +
                                            (FreePageInfo->Size  << PAGE_SHIFT));
    
                    if (MmProtectFreedNonPagedPool == FALSE) {
                        RemoveEntryList (&FreePageInfo->List);
                    }
                    else {
                        MiProtectedPoolRemoveEntryList (&FreePageInfo->List);
                    }

                    if (FreePageInfo->Size != 0) {
    
                        //
                        // Insert any remainder into the correct list.
                        //
    
                        Index = (ULONG)(FreePageInfo->Size - 1);
                        if (Index >= MI_MAX_FREE_LIST_HEADS) {
                            Index = MI_MAX_FREE_LIST_HEADS - 1;
                        }

                        if (MmProtectFreedNonPagedPool == FALSE) {
                            InsertTailList (&MmNonPagedPoolFreeListHead[Index],
                                            &FreePageInfo->List);
                        }
                        else {
                            MiProtectedPoolInsertList (&MmNonPagedPoolFreeListHead[Index],
                                                       &FreePageInfo->List,
                                                       FALSE);

                            MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                                       (ULONG)FreePageInfo->Size);
                        }
                    }
    
                    //
                    // Adjust the number of free pages remaining in the pool.
                    //
    
                    MmNumberOfFreeNonPagedPool -= SizeInPages;
                    ASSERT ((LONG)MmNumberOfFreeNonPagedPool >= 0);
    
                    //
                    // Mark start and end of allocation in the PFN database.
                    //
    
                    if (MI_IS_PHYSICAL_ADDRESS(BaseVa)) {
    
                        //
                        // On certain architectures, virtual addresses
                        // may be physical and hence have no corresponding PTE.
                        //
    
                        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (BaseVa);
                    } else {
                        PointerPte = MiGetPteAddress(BaseVa);
                        ASSERT (PointerPte->u.Hard.Valid == 1);
                        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
                    }
                    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    
                    ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                    ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
    
                    CONSISTENCY_LOCK_PFN2 (OldIrql);
    
                    Pfn1->u3.e1.StartOfAllocation = 1;
    
                    if (PoolType & POOL_VERIFIER_MASK) {
                        Pfn1->u3.e1.VerifierAllocation = 1;
                    }

                    CONSISTENCY_UNLOCK_PFN2 (OldIrql);
    
                    //
                    // Mark this as a large session allocation in the PFN database.
                    //
    
                    if (IsLargeSessionAllocation != 0) {
                        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
    
                        CONSISTENCY_LOCK_PFN2 (OldIrql);
    
                        Pfn1->u3.e1.LargeSessionAllocation = 1;
    
                        CONSISTENCY_UNLOCK_PFN2 (OldIrql);
    
                        MiSessionPoolAllocated (BaseVa,
                                                SizeInPages << PAGE_SHIFT,
                                                NonPagedPool);
                    }
    
                    //
                    // Calculate the ending PTE's address.
                    //
    
                    if (SizeInPages != 1) {
                        if (MI_IS_PHYSICAL_ADDRESS(BaseVa)) {
                            Pfn1 += SizeInPages - 1;
                        } else {
                            PointerPte += SizeInPages - 1;
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
                    }
                    else if (MmProtectFreedNonPagedPool == FALSE) {
    
                       //
                       // Map this with KSEG0 if possible.
                       //
#if defined (_X86_)
                       if  ((BaseVa > (PVOID)MM_KSEG2_BASE) &&
                            (PageFrameIndex >= MI_CONVERT_PHYSICAL_TO_PFN(MmSubsectionBase)) &&
                            (PageFrameIndex < MmSubsectionTopPage) &&
                            (MmKseg2Frame != 0))
#elif defined (_ALPHA_)
                       if  ((BaseVa > (PVOID)KSEG2_BASE) &&
                            (PageFrameIndex >= MI_CONVERT_PHYSICAL_TO_PFN(MmSubsectionBase)) &&
                            (PageFrameIndex < MmSubsectionTopPage))
#else
                       if  ((BaseVa > (PVOID)KSEG2_BASE) &&
                            (PageFrameIndex < MmSubsectionTopPage))
#endif
                       {
                           BaseVa = (PVOID)(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
                       }
                    }
    
                    ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
    
                    CONSISTENCY_LOCK_PFN2 (OldIrql);
    
                    Pfn1->u3.e1.EndOfAllocation = 1;
    
                    CONSISTENCY_UNLOCK_PFN2 (OldIrql);
    
                    MmAllocatedNonPagedPool += SizeInPages;
                    return BaseVa;
                }
    
                Entry = FreePageInfo->List.Flink;
    
                if (MmProtectFreedNonPagedPool == TRUE) {
                    MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                               (ULONG)FreePageInfo->Size);
                }
            }
            Index += 1;
        }

        //
        // No more entries on the list, expand nonpaged pool if
        // possible to satisfy this request.
        //

        //
        // Check to see if there are too many unused segments laying
        // around.  If so, set an event so they get deleted.
        //

        if (MI_UNUSED_SEGMENTS_SURPLUS()) {
            KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
        }

        LOCK_PFN2 (OldIrql);

        //
        // Make sure we have 1 more than the number of pages
        // requested available.
        //

        if (MmAvailablePages <= SizeInPages) {

            UNLOCK_PFN2 (OldIrql);

            //
            // There are no free physical pages to expand
            // nonpaged pool.
            //

            return NULL;
        }

        //
        // Try to find system PTEs to expand the pool into.
        //

        StartingPte = MiReserveSystemPtes ((ULONG)SizeInPages,
                                           NonPagedPoolExpansion,
                                           0,
                                           0,
                                           FALSE);

        if (StartingPte == NULL) {

            //
            // There are no free physical PTEs to expand nonpaged pool.
            // If there are any cached expansion PTEs, free them now in
            // an attempt to get enough contiguous VA for our caller.
            //

            if ((SizeInPages > 1) && (MmNumberOfFreeNonPagedPool != 0)) {

                FreedPool = FALSE;

                for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index += 1) {
        
                    Entry = MmNonPagedPoolFreeListHead[Index].Flink;
        
                    while (Entry != &MmNonPagedPoolFreeListHead[Index]) {
        
                        if (MmProtectFreedNonPagedPool == TRUE) {
                            MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
                        }

                        //
                        // The list is not empty, see if this one is virtually
                        // mapped.
                        //
            
                        FreePageInfo = CONTAINING_RECORD(Entry,
                                                         MMFREE_POOL_ENTRY,
                                                         List);
            
                        if ((!MI_IS_PHYSICAL_ADDRESS(FreePageInfo)) &&
                            ((PVOID)FreePageInfo >= MmNonPagedPoolExpansionStart)) {
                            if (MmProtectFreedNonPagedPool == FALSE) {
                                RemoveEntryList (&FreePageInfo->List);
                            }
                            else {
                                MiProtectedPoolRemoveEntryList (&FreePageInfo->List);
                            }

                            MmNumberOfFreeNonPagedPool -= FreePageInfo->Size;
                            ASSERT ((LONG)MmNumberOfFreeNonPagedPool >= 0);
    
                            UNLOCK_PFN2 (OldIrql);

                            FreedPool = TRUE;

                            MiFreeNonPagedPool ((PVOID)FreePageInfo,
                                                FreePageInfo->Size);

                            LOCK_PFN2 (OldIrql);
                            Index = 0;
                            break;
                        }

                        Entry = FreePageInfo->List.Flink;
        
                        if (MmProtectFreedNonPagedPool == TRUE) {
                            MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                                       (ULONG)FreePageInfo->Size);
                        }
                    }
                }

                if (FreedPool == TRUE) {
                    StartingPte = MiReserveSystemPtes ((ULONG)SizeInPages,
                                                       NonPagedPoolExpansion,
                                                       0,
                                                       0,
                                                       FALSE);
            
                    if (StartingPte != NULL) {
                        goto gotpool;
                    }
                }
            }

            UNLOCK_PFN2 (OldIrql);

nopool:

            //
            // Running low on pool - if this request is not for session pool,
            // force unused segment trimming when appropriate.
            //
        
            SignalDereferenceThread = FALSE;
            LOCK_PFN2 (OldIrql);
            if (MmUnusedSegmentForceFree == 0) {
                if (!IsListEmpty(&MmUnusedSegmentList)) {
                    SignalDereferenceThread = TRUE;
                    MmUnusedSegmentForceFree = 30;
                }
            }
            UNLOCK_PFN2 (OldIrql);
            if (SignalDereferenceThread == TRUE) {
                KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
            }

            return NULL;
        }

gotpool:

        //
        // Update the count of available resident pages.
        //

        MmResidentAvailablePages -= SizeInPages;
        MM_BUMP_COUNTER(0, SizeInPages);

        //
        // Charge commitment as non paged pool uses physical memory.
        //

        MM_TRACK_COMMIT (MM_DBG_COMMIT_NONPAGED_POOL_EXPANSION, SizeInPages);

        MiChargeCommitmentCantExpand (SizeInPages, TRUE);

        //
        //  Expand the pool.
        //

        PointerPte = StartingPte;
        TempPte = ValidKernelPte;
        MmAllocatedNonPagedPool += SizeInPages;
        i = SizeInPages;

        do {
            PageFrameIndex = MiRemoveAnyPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

            Pfn1->u3.e2.ReferenceCount = 1;
            Pfn1->u2.ShareCount = 1;
            Pfn1->PteAddress = PointerPte;
            Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
            Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte));

            Pfn1->u3.e1.PageLocation = ActiveAndValid;
            Pfn1->u3.e1.LargeSessionAllocation = 0;
            Pfn1->u3.e1.VerifierAllocation = 0;

            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            MI_WRITE_VALID_PTE (PointerPte, TempPte);
            PointerPte += 1;
            SizeInPages -= 1;
        } while (SizeInPages > 0);

        Pfn1->u3.e1.EndOfAllocation = 1;

        Pfn1 = MI_PFN_ELEMENT (StartingPte->u.Hard.PageFrameNumber);
        Pfn1->u3.e1.StartOfAllocation = 1;

        ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);

        if (PoolType & POOL_VERIFIER_MASK) {
            Pfn1->u3.e1.VerifierAllocation = 1;
        }

        //
        // Mark this as a large session allocation in the PFN database.
        //

        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);

        if (IsLargeSessionAllocation != 0) {
            Pfn1->u3.e1.LargeSessionAllocation = 1;

            MiSessionPoolAllocated(MiGetVirtualAddressMappedByPte (StartingPte),
                                   i << PAGE_SHIFT,
                                   NonPagedPool);
        }

        UNLOCK_PFN2 (OldIrql);

        BaseVa = MiGetVirtualAddressMappedByPte (StartingPte);

        if (i == 1) {

            //
            // Map this with KSEG0 if possible.
            //

#if defined (_X86_)
            if ((PageFrameIndex >= MI_CONVERT_PHYSICAL_TO_PFN(MmSubsectionBase)) &&
                (PageFrameIndex < MmSubsectionTopPage) &&
                (MmKseg2Frame != 0))
#elif defined (_ALPHA_)
            if ((PageFrameIndex >= MI_CONVERT_PHYSICAL_TO_PFN(MmSubsectionBase)) &&
                (PageFrameIndex < MmSubsectionTopPage))
#else
            if (PageFrameIndex < MmSubsectionTopPage)
#endif
            {
                 BaseVa = (PVOID)(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
            }
        }

        return BaseVa;
    }

    //
    // Paged Pool.
    //

    if ((PoolType & SESSION_POOL_MASK) == 0) {
        SessionSpace = (PMM_SESSION_SPACE)0;
        PagedPoolInfo = &MmPagedPoolInfo;
    }
    else {
        SessionSpace = MmSessionSpace;
        PagedPoolInfo = &SessionSpace->PagedPoolInfo;
    }

    StartPosition = RtlFindClearBitsAndSet (
                               PagedPoolInfo->PagedPoolAllocationMap,
                               (ULONG)SizeInPages,
                               PagedPoolInfo->PagedPoolHint
                               );

    if ((StartPosition == 0xFFFFFFFF) && (PagedPoolInfo->PagedPoolHint != 0)) {

        if (MI_UNUSED_SEGMENTS_SURPLUS()) {
            KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
        }

        //
        // No free bits were found, check from the start of
        // the bit map.

        StartPosition = RtlFindClearBitsAndSet (
                                   PagedPoolInfo->PagedPoolAllocationMap,
                                   (ULONG)SizeInPages,
                                   0
                                   );
    }

    //
    // If start position = -1, no room in pool.  Attempt to expand PagedPool.
    //

    if (StartPosition == 0xFFFFFFFF) {

        //
        // Attempt to expand the paged pool.
        //

        StartPosition = (((ULONG)SizeInPages - 1) / PTE_PER_PAGE) + 1;

        //
        // Make sure there is enough space to create the prototype PTEs.
        //

        if (((StartPosition - 1) + PagedPoolInfo->NextPdeForPagedPoolExpansion) >
            MiGetPteAddress (PagedPoolInfo->LastPteForPagedPool)) {

            //
            // Can't expand pool any more.  If this request is not for session
            // pool, force unused segment trimming when appropriate.
            //

            if (SessionSpace == NULL) {
                goto nopool;
            }

            return NULL;
        }

        if (SessionSpace) {
            TempPte = ValidKernelPdeLocal;
            PageTableCount = StartPosition;
        }
        else {
            TempPte = ValidKernelPde;
        }

        LOCK_PFN (OldIrql);

        //
        // Make sure we have 1 more than the number of pages
        // requested available.
        //

        if (MmAvailablePages <= StartPosition) {

            UNLOCK_PFN (OldIrql);

            //
            // There are no free physical pages to expand
            // paged pool.
            //

            return NULL;
        }

        //
        // Update the count of available resident pages.
        //

        MmResidentAvailablePages -= StartPosition;
        MM_BUMP_COUNTER(1, StartPosition);

        //
        //  Expand the pool.
        //

        EndPosition = (ULONG)((PagedPoolInfo->NextPdeForPagedPoolExpansion -
                          MiGetPteAddress(PagedPoolInfo->FirstPteForPagedPool)) *
                          PTE_PER_PAGE);

        RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap,
                      EndPosition,
                      (ULONG) StartPosition * PTE_PER_PAGE);

        PointerPte = PagedPoolInfo->NextPdeForPagedPoolExpansion;
        StartingPte = (PMMPTE)MiGetVirtualAddressMappedByPte(PointerPte);
        PagedPoolInfo->NextPdeForPagedPoolExpansion += StartPosition;

        do {
            ASSERT (PointerPte->u.Hard.Valid == 0);

            MM_TRACK_COMMIT (MM_DBG_COMMIT_PAGED_POOL_PAGETABLE, 1);

            MiChargeCommitmentCantExpand (1, TRUE);

            PageFrameIndex = MiRemoveAnyPage (
                                MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

            TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
            MI_WRITE_VALID_PTE (PointerPte, TempPte);

            //
            // Map valid PDE into system (or session) address space as well.
            //

            VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);

#if defined (_WIN64)

            MiInitializePfn (PageFrameIndex,
                             PointerPte,
                             1);

#else

            if (SessionSpace) {

                Index = (ULONG)(PointerPte - MiGetPdeAddress (MmSessionBase));
                ASSERT (MmSessionSpace->PageTables[Index].u.Long == 0);
                MmSessionSpace->PageTables[Index] = TempPte;

                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                PointerPte,
                                                MmSessionSpace->SessionPageDirectoryIndex);

                MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC1, 1);
            }
            else {
#if !defined (_X86PAE_)
                MmSystemPagePtes [((ULONG_PTR)PointerPte &
                    ((sizeof(MMPTE) * PDE_PER_PAGE) - 1)) / sizeof(MMPTE)] =
                                     TempPte;
                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                PointerPte,
                                                MmSystemPageDirectory);
#else
                MmSystemPagePtes [((ULONG_PTR)PointerPte &
                    (PD_PER_SYSTEM * (sizeof(MMPTE) * PDE_PER_PAGE) - 1)) / sizeof(MMPTE)] =
                                     TempPte;
                MiInitializePfnForOtherProcess (PageFrameIndex,
                                                PointerPte,
                                                MmSystemPageDirectory[(PointerPte - MiGetPdeAddress(0)) / PDE_PER_PAGE]);
#endif
            }
#endif

            KeFillEntryTb ((PHARDWARE_PTE) PointerPte, VirtualAddress, FALSE);

            MiFillMemoryPte (StartingPte,
                             PAGE_SIZE,
                             MM_KERNEL_NOACCESS_PTE);

            PointerPte += 1;
            StartingPte += PAGE_SIZE / sizeof(MMPTE);
            StartPosition -= 1;
        } while (StartPosition > 0);

        UNLOCK_PFN (OldIrql);

        if (SessionSpace) {

            PointerPte -= PageTableCount;

            LOCK_SESSION_SPACE_WS (SessionIrql);

            MmSessionSpace->NonPagablePages += PageTableCount;
            MmSessionSpace->CommittedPages += PageTableCount;

            do {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
    
                ASSERT (Pfn1->u1.Event == 0);
                Pfn1->u1.Event = (PVOID) PsGetCurrentThread ();
    
                SessionPte = MiGetVirtualAddressMappedByPte (PointerPte);
    
                MiAddValidPageToWorkingSet (SessionPte,
                                            PointerPte,
                                            Pfn1,
                                            0);
    
                WsEntry = MiLocateWsle (SessionPte,
                                        MmSessionSpace->Vm.VmWorkingSetList,
                                        Pfn1->u1.WsIndex);
    
                if (WsEntry >= MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic) {
        
                    WsSwapEntry = MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic;
        
                    if (WsEntry != MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic) {
        
                        //
                        // Swap this entry with the one at first dynamic.
                        //
        
                        MiSwapWslEntries (WsEntry, WsSwapEntry, &MmSessionSpace->Vm);
                    }
        
                    MmSessionSpace->Vm.VmWorkingSetList->FirstDynamic += 1;
                }
                else {
                    WsSwapEntry = WsEntry;
                }
        
                //
                // Indicate that the page is locked.
                //
        
                MmSessionSpace->Wsle[WsSwapEntry].u1.e1.LockedInWs = 1;
    
                PointerPte += 1;
                PageTableCount -= 1;
            } while (PageTableCount > 0);
            UNLOCK_SESSION_SPACE_WS (SessionIrql);
        }

        StartPosition = RtlFindClearBitsAndSet (
                                   PagedPoolInfo->PagedPoolAllocationMap,
                                   (ULONG)SizeInPages,
                                   EndPosition
                                   );

        ASSERT (StartPosition != 0xffffffff);
    }

    //
    // This is paged pool, the start and end can't be saved
    // in the PFN database as the page isn't always resident
    // in memory.  The ideal place to save the start and end
    // would be in the prototype PTE, but there are no free
    // bits.  To solve this problem, a bitmap which parallels
    // the allocation bitmap exists which contains set bits
    // in the positions where an allocation ends.  This
    // allows pages to be deallocated with only their starting
    // address.
    //
    // For sanity's sake, the starting address can be verified
    // from the 2 bitmaps as well.  If the page before the starting
    // address is not allocated (bit is zero in allocation bitmap)
    // then this page is obviously a start of an allocation block.
    // If the page before is allocated and the other bit map does
    // not indicate the previous page is the end of an allocation,
    // then the starting address is wrong and a bug check should
    // be issued.
    //

    if (SizeInPages == 1) {
        PagedPoolInfo->PagedPoolHint = StartPosition + (ULONG)SizeInPages;
    }

    if (MiChargeCommitmentCantExpand (SizeInPages, FALSE) == FALSE) {
        Thread = PsGetCurrentThread ();
        if (MI_MEMORY_MAKER(Thread)) {
            MiChargeCommitmentCantExpand (SizeInPages, TRUE);
        }
        else {
            RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap,
                          StartPosition,
                          (ULONG)SizeInPages);
    
            //
            // Could not commit the page(s), return NULL indicating
            // no pool was allocated.  Note that the lack of commit may be due
            // to unused segments and the MmSharedCommit, prototype PTEs, etc
            // associated with them.  So force a reduction now.
            //
    
            MiIssuePageExtendRequestNoWait (SizeInPages);

            SignalDereferenceThread = FALSE;
            LOCK_PFN (OldIrql);
            if (MmUnusedSegmentForceFree == 0) {
                if (!IsListEmpty(&MmUnusedSegmentList)) {
                    SignalDereferenceThread = TRUE;
                    MmUnusedSegmentForceFree = 30;
                }
            }
            UNLOCK_PFN (OldIrql);
            if (SignalDereferenceThread == TRUE) {
                KeSetEvent (&MmUnusedSegmentCleanup, 0, FALSE);
            }

            return NULL;
        }
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_PAGED_POOL_PAGES, SizeInPages);

    if (SessionSpace) {
        LOCK_SESSION_SPACE_WS (OldIrql);
        SessionSpace->CommittedPages += SizeInPages;
        MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_PAGEDPOOL_PAGES, SizeInPages);
        UNLOCK_SESSION_SPACE_WS (OldIrql);
        BaseVa = (PVOID)((PCHAR)SessionSpace->PagedPoolStart +
                                (StartPosition << PAGE_SHIFT));
    }
    else {
        MmPagedPoolCommit += (ULONG)SizeInPages;
        BaseVa = (PVOID)((PUCHAR)MmPageAlignedPoolBase[PagedPool] +
                                (StartPosition << PAGE_SHIFT));
    }

#if DBG
    PointerPte = MiGetPteAddress (BaseVa);
    for (i = 0; i < SizeInPages; i += 1) {
        if (*(ULONG *)PointerPte != MM_KERNEL_NOACCESS_PTE) {
            DbgPrint("MiAllocatePoolPages: PP not zero PTE (%x %x %x)\n",
                BaseVa, PointerPte, *PointerPte);
            DbgBreakPoint();
        }
        PointerPte += 1;
    }
#endif
    PointerPte = MiGetPteAddress (BaseVa);
    MiFillMemoryPte (PointerPte,
                     SizeInPages * sizeof(MMPTE),
                     MM_KERNEL_DEMAND_ZERO_PTE);

    PagedPoolInfo->PagedPoolCommit += SizeInPages;
    EndPosition = StartPosition + (ULONG)SizeInPages - 1;
    RtlSetBits (PagedPoolInfo->EndOfPagedPoolBitmap, EndPosition, 1L);

    //
    // Mark this as a large session allocation in the PFN database.
    //

    if (IsLargeSessionAllocation != 0) {
        RtlSetBits (PagedPoolInfo->PagedPoolLargeSessionAllocationMap,
                    StartPosition,
                    1L);

        MiSessionPoolAllocated (BaseVa,
                                SizeInPages << PAGE_SHIFT,
                                PagedPool);
    }
    else if (PoolType & POOL_VERIFIER_MASK) {
        RtlSetBits (VerifierLargePagedPoolMap,
                    StartPosition,
                    1L);
    }

    PagedPoolInfo->AllocatedPagedPool += SizeInPages;

    return BaseVa;
}

ULONG
MiFreePoolPages (
    IN PVOID StartingAddress
    )

/*++

Routine Description:

    This function returns a set of pages back to the pool from
    which they were obtained.  Once the pages have been deallocated
    the region provided by the allocation becomes available for
    allocation to other callers, i.e. any data in the region is now
    trashed and cannot be referenced.

Arguments:

    StartingAddress - Supplies the starting address which was returned
                      in a previous call to MiAllocatePoolPages.

Return Value:

    Returns the number of pages deallocated.

Environment:

    These functions are used by the general pool allocation routines
    and should not be called directly.

    Mutexes guarding the pool databases must be held when calling
    these functions.

--*/

{
    ULONG StartPosition;
    ULONG Index;
    PFN_NUMBER i;
    PFN_NUMBER NumberOfPages;
    POOL_TYPE PoolType;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    ULONG IsLargeSessionAllocation;
    ULONG IsLargeVerifierAllocation;
    PMMFREE_POOL_ENTRY Entry;
    PMMFREE_POOL_ENTRY NextEntry;
    PMM_PAGED_POOL_INFO PagedPoolInfo;
    PMM_SESSION_SPACE SessionSpace;
    LOGICAL SessionAllocation;
    MMPTE NoAccessPte;
    PFN_NUMBER PagesFreed;

    NumberOfPages = 1;

    //
    // Determine Pool type base on the virtual address of the block
    // to deallocate.
    //
    // This assumes NonPagedPool starts at a higher virtual address
    // then PagedPool.
    //

    if ((StartingAddress >= MmPagedPoolStart) &&
        (StartingAddress <= MmPagedPoolEnd)) {
        PoolType = PagedPool;
        SessionSpace = NULL;
        PagedPoolInfo = &MmPagedPoolInfo;
        StartPosition = (ULONG)(((PCHAR)StartingAddress -
                          (PCHAR)MmPageAlignedPoolBase[PoolType]) >> PAGE_SHIFT);
    }
    else if (MI_IS_SESSION_POOL_ADDRESS (StartingAddress) == TRUE) {
        ASSERT (MiHydra == TRUE);
        PoolType = PagedPool;
        SessionSpace = MmSessionSpace;
        ASSERT (SessionSpace);
        PagedPoolInfo = &SessionSpace->PagedPoolInfo;
        StartPosition = (ULONG)(((PCHAR)StartingAddress -
                          (PCHAR)SessionSpace->PagedPoolStart) >> PAGE_SHIFT);
    }
    else {

        if (StartingAddress < MM_SYSTEM_RANGE_START) {
            KeBugCheckEx (BAD_POOL_CALLER,
                          0x40,
                          (ULONG_PTR)StartingAddress,
                          (ULONG_PTR)MM_SYSTEM_RANGE_START,
                          0);
        }

        PoolType = NonPagedPool;
        SessionSpace = NULL;
        PagedPoolInfo = &MmPagedPoolInfo;
        StartPosition = (ULONG)(((PCHAR)StartingAddress -
                          (PCHAR)MmPageAlignedPoolBase[PoolType]) >> PAGE_SHIFT);
    }

    //
    // Check to ensure this page is really the start of an allocation.
    //

    if (PoolType == NonPagedPool) {

        if (StartPosition < MmMustSucceedPoolBitPosition) {

            PULONG_PTR NextList;

            //
            // This is must succeed pool, don't free it, just
            // add it to the front of the list.
            //
            // Note - only a single page can be released at a time.
            //

            if (MI_IS_PHYSICAL_ADDRESS(StartingAddress)) {
                PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (StartingAddress);
            } else {
                PointerPte = MiGetPteAddress(StartingAddress);
                ASSERT (PointerPte->u.Hard.Valid == 1);
                PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            }
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    
            if (Pfn1->u3.e1.VerifierAllocation == 1) {
                Pfn1->u3.e1.VerifierAllocation = 0;
                VerifierFreeTrackedPool (StartingAddress,
                                         PAGE_SIZE,
                                         NonPagedPool,
                                         FALSE);
            }

            //
            // Check for this being a large session allocation. If it is,
            // we need to return the pool charge accordingly.
            //

            if (Pfn1->u3.e1.LargeSessionAllocation) {
                Pfn1->u3.e1.LargeSessionAllocation = 0;
                MiSessionPoolFreed (StartingAddress,
                                    PAGE_SIZE,
                                    NonPagedPool);
            }

            NextList = (PULONG_PTR)StartingAddress;
            *NextList = (ULONG_PTR)MmNonPagedMustSucceed;
            MmNonPagedMustSucceed = StartingAddress;
            return (ULONG)NumberOfPages;
        }

        if (MI_IS_PHYSICAL_ADDRESS (StartingAddress)) {

            //
            // On certain architectures, virtual addresses
            // may be physical and hence have no corresponding PTE.
            //

            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (StartingAddress));
            if (StartPosition >= MmExpandedPoolBitPosition) {
                PointerPte = Pfn1->PteAddress;
                StartingAddress = MiGetVirtualAddressMappedByPte (PointerPte);
            }
        } else {
            PointerPte = MiGetPteAddress (StartingAddress);
            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
        }

        if (Pfn1->u3.e1.StartOfAllocation == 0) {
            KeBugCheckEx (BAD_POOL_CALLER,
                          0x41,
                          (ULONG_PTR)StartingAddress,
                          (ULONG_PTR)(Pfn1 - MmPfnDatabase),
                          MmHighestPhysicalPage);
        }

        CONSISTENCY_LOCK_PFN2 (OldIrql);

        ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);

        IsLargeVerifierAllocation = Pfn1->u3.e1.VerifierAllocation;
        IsLargeSessionAllocation = Pfn1->u3.e1.LargeSessionAllocation;

        Pfn1->u3.e1.StartOfAllocation = 0;
        Pfn1->u3.e1.VerifierAllocation = 0;
        Pfn1->u3.e1.LargeSessionAllocation = 0;

        CONSISTENCY_UNLOCK_PFN2 (OldIrql);

#if DBG
        if ((Pfn1->u3.e2.ReferenceCount > 1) &&
            (Pfn1->u3.e1.WriteInProgress == 0)) {
            DbgPrint ("MM: MiFreePoolPages - deleting pool locked for I/O %lx\n",
                 Pfn1);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
        }
#endif //DBG

        //
        // Find end of allocation and release the pages.
        //

        while (Pfn1->u3.e1.EndOfAllocation == 0) {
            if (MI_IS_PHYSICAL_ADDRESS(StartingAddress)) {
                Pfn1 += 1;
            } else {
                PointerPte += 1;
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            }
            NumberOfPages += 1;
#if DBG
            if ((Pfn1->u3.e2.ReferenceCount > 1) &&
                (Pfn1->u3.e1.WriteInProgress == 0)) {
                DbgPrint ("MM:MiFreePoolPages - deleting pool locked for I/O %lx\n",
                     Pfn1);
                ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
            }
#endif //DBG
        }

        MmAllocatedNonPagedPool -= NumberOfPages;

        if (IsLargeVerifierAllocation != 0) {
            VerifierFreeTrackedPool (StartingAddress,
                                     NumberOfPages << PAGE_SHIFT,
                                     NonPagedPool,
                                     FALSE);
        }

        if (IsLargeSessionAllocation != 0) {
            MiSessionPoolFreed (StartingAddress,
                                NumberOfPages << PAGE_SHIFT,
                                NonPagedPool);
        }

        CONSISTENCY_LOCK_PFN2 (OldIrql);

        Pfn1->u3.e1.EndOfAllocation = 0;

        CONSISTENCY_UNLOCK_PFN2 (OldIrql);

#if DBG
        if (MiFillFreedPool != 0) {
            RtlFillMemoryUlong (StartingAddress,
                                PAGE_SIZE * NumberOfPages,
                                MiFillFreedPool);
        }
#endif //DBG

        if (StartingAddress > MmNonPagedPoolExpansionStart) {

            //
            // This page was from the expanded pool, should
            // it be freed?
            //
            // NOTE: all pages in the expanded pool area have PTEs
            // so no physical address checks need to be performed.
            //

            if ((NumberOfPages > 3) || (MmNumberOfFreeNonPagedPool > 5)) {

                //
                // Free these pages back to the free page list.
                //

                MiFreeNonPagedPool (StartingAddress, NumberOfPages);

                return (ULONG)NumberOfPages;
            }
        }

        //
        // Add the pages to the list of free pages.
        //

        MmNumberOfFreeNonPagedPool += NumberOfPages;

        //
        // Check to see if the next allocation is free.
        // We cannot walk off the end of nonpaged initial or expansion
        // pages as the highest initial allocation is never freed and
        // the highest expansion allocation is guard-paged.
        //

        i = NumberOfPages;

        ASSERT (MiEndOfInitialPoolFrame != 0);

        if ((PFN_NUMBER)(Pfn1 - MmPfnDatabase) == MiEndOfInitialPoolFrame) {
            PointerPte += 1;
            Pfn1 = NULL;
        }
        else if (MI_IS_PHYSICAL_ADDRESS(StartingAddress)) {
            Pfn1 += 1;
            ASSERT ((PCHAR)StartingAddress + NumberOfPages < (PCHAR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes);
        } else {
            PointerPte += 1;
            ASSERT ((PCHAR)StartingAddress + NumberOfPages <= (PCHAR)MmNonPagedPoolEnd);

            //
            // Unprotect the previously freed pool so it can be merged.
            //

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool (
                    (PVOID)MiGetVirtualAddressMappedByPte(PointerPte),
                    0);
            }

            if (PointerPte->u.Hard.Valid == 1) {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            } else {
                Pfn1 = NULL;
            }
        }

        if ((Pfn1 != NULL) && (Pfn1->u3.e1.StartOfAllocation == 0)) {

            //
            // This range of pages is free.  Remove this entry
            // from the list and add these pages to the current
            // range being freed.
            //

            Entry = (PMMFREE_POOL_ENTRY)((PCHAR)StartingAddress
                                        + (NumberOfPages << PAGE_SHIFT));
            ASSERT (Entry->Signature == MM_FREE_POOL_SIGNATURE);
            ASSERT (Entry->Owner == Entry);
#if DBG
            {
                PMMPTE DebugPte;
                PMMPFN DebugPfn;

                DebugPfn = NULL;

                if (MI_IS_PHYSICAL_ADDRESS(StartingAddress)) {

                    //
                    // On certain architectures, virtual addresses
                    // may be physical and hence have no corresponding PTE.
                    //

                    DebugPfn = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (Entry));
                    DebugPfn += Entry->Size;
                    if ((PFN_NUMBER)((DebugPfn - 1) - MmPfnDatabase) != MiEndOfInitialPoolFrame) {
                        ASSERT (DebugPfn->u3.e1.StartOfAllocation == 1);
                    }
                } else {
                    DebugPte = PointerPte + Entry->Size;
                    if ((DebugPte-1)->u.Hard.Valid == 1) {
                        DebugPfn = MI_PFN_ELEMENT ((DebugPte-1)->u.Hard.PageFrameNumber);
                        if ((PFN_NUMBER)(DebugPfn - MmPfnDatabase) != MiEndOfInitialPoolFrame) {
                            if (DebugPte->u.Hard.Valid == 1) {
                                DebugPfn = MI_PFN_ELEMENT (DebugPte->u.Hard.PageFrameNumber);
                                ASSERT (DebugPfn->u3.e1.StartOfAllocation == 1);
                            }
                        }

                    }
                }
            }
#endif //DBG

            i += Entry->Size;
            if (MmProtectFreedNonPagedPool == FALSE) {
                RemoveEntryList (&Entry->List);
            }
            else {
                MiProtectedPoolRemoveEntryList (&Entry->List);
            }
        }

        //
        // Check to see if the previous page is the end of an allocation.
        // If it is not the end of an allocation, it must be free and
        // therefore this allocation can be tagged onto the end of
        // that allocation.
        //
        // We cannot walk off the beginning of expansion pool because it is
        // guard-paged.  If the initial pool is superpaged instead, we are also
        // safe as the must succeed pages always have EndOfAllocation set.
        //

        Entry = (PMMFREE_POOL_ENTRY)StartingAddress;

        if (MI_IS_PHYSICAL_ADDRESS(StartingAddress)) {
            ASSERT (StartingAddress != MmNonPagedPoolStart);

            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (
                                    (PVOID)((PCHAR)Entry - PAGE_SIZE)));

        } else {
            PointerPte -= NumberOfPages + 1;

            //
            // Unprotect the previously freed pool so it can be merged.
            //

            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool (
                    (PVOID)MiGetVirtualAddressMappedByPte(PointerPte),
                    0);
            }

            if (PointerPte->u.Hard.Valid == 1) {
                Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
            } else {
                Pfn1 = NULL;
            }
        }
        if (Pfn1 != NULL) {
            if (Pfn1->u3.e1.EndOfAllocation == 0) {

                //
                // This range of pages is free, add these pages to
                // this entry.  The owner field points to the address
                // of the list entry which is linked into the free pool
                // pages list.
                //

                Entry = (PMMFREE_POOL_ENTRY)((PCHAR)StartingAddress - PAGE_SIZE);
                ASSERT (Entry->Signature == MM_FREE_POOL_SIGNATURE);
                Entry = Entry->Owner;

                //
                // Unprotect the previously freed pool so we can merge it
                //

                if (MmProtectFreedNonPagedPool == TRUE) {
                    MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
                }

                //
                // If this entry became larger than MM_SMALL_ALLOCATIONS
                // pages, move it to the tail of the list.  This keeps the
                // small allocations at the front of the list.
                //

                if (Entry->Size < MI_MAX_FREE_LIST_HEADS - 1) {

                    if (MmProtectFreedNonPagedPool == FALSE) {
                        RemoveEntryList (&Entry->List);
                    }
                    else {
                        MiProtectedPoolRemoveEntryList (&Entry->List);
                    }

                    //
                    // Add these pages to the previous entry.
                    //
    
                    Entry->Size += i;

                    Index = (ULONG)(Entry->Size - 1);
            
                    if (Index >= MI_MAX_FREE_LIST_HEADS) {
                        Index = MI_MAX_FREE_LIST_HEADS - 1;
                    }

                    if (MmProtectFreedNonPagedPool == FALSE) {
                        InsertTailList (&MmNonPagedPoolFreeListHead[Index],
                                        &Entry->List);
                    }
                    else {
                        MiProtectedPoolInsertList (&MmNonPagedPoolFreeListHead[Index],
                                          &Entry->List,
                                          Entry->Size < MM_SMALL_ALLOCATIONS ?
                                              TRUE : FALSE);
                    }
                }
                else {

                    //
                    // Add these pages to the previous entry.
                    //
    
                    Entry->Size += i;
                }
            }
        }

        if (Entry == (PMMFREE_POOL_ENTRY)StartingAddress) {

            //
            // This entry was not combined with the previous, insert it
            // into the list.
            //

            Entry->Size = i;

            Index = (ULONG)(Entry->Size - 1);
    
            if (Index >= MI_MAX_FREE_LIST_HEADS) {
                Index = MI_MAX_FREE_LIST_HEADS - 1;
            }

            if (MmProtectFreedNonPagedPool == FALSE) {
                InsertTailList (&MmNonPagedPoolFreeListHead[Index],
                                &Entry->List);
            }
            else {
                MiProtectedPoolInsertList (&MmNonPagedPoolFreeListHead[Index],
                                      &Entry->List,
                                      Entry->Size < MM_SMALL_ALLOCATIONS ?
                                          TRUE : FALSE);
            }
        }

        //
        // Set the owner field in all these pages.
        //

        NextEntry = (PMMFREE_POOL_ENTRY)StartingAddress;
        while (i > 0) {
            NextEntry->Owner = Entry;
#if DBG
            NextEntry->Signature = MM_FREE_POOL_SIGNATURE;
#endif

            NextEntry = (PMMFREE_POOL_ENTRY)((PCHAR)NextEntry + PAGE_SIZE);
            i -= 1;
        }

#if DBG
        NextEntry = Entry;
        for (i = 0; i < Entry->Size; i += 1) {
            PMMPTE DebugPte;
            PMMPFN DebugPfn;
            if (MI_IS_PHYSICAL_ADDRESS(StartingAddress)) {

                //
                // On certain architectures, virtual addresses
                // may be physical and hence have no corresponding PTE.
                //

                DebugPfn = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (NextEntry));
            } else {

                DebugPte = MiGetPteAddress (NextEntry);
                DebugPfn = MI_PFN_ELEMENT (DebugPte->u.Hard.PageFrameNumber);
            }
            ASSERT (DebugPfn->u3.e1.StartOfAllocation == 0);
            ASSERT (DebugPfn->u3.e1.EndOfAllocation == 0);
            ASSERT (NextEntry->Owner == Entry);
            NextEntry = (PMMFREE_POOL_ENTRY)((PCHAR)NextEntry + PAGE_SIZE);
        }
#endif

        //
        // Prevent anyone from touching non paged pool after freeing it.
        //

        if (MmProtectFreedNonPagedPool == TRUE) {
            MiProtectFreeNonPagedPool ((PVOID)Entry, (ULONG)Entry->Size);
        }

        return (ULONG)NumberOfPages;

    } else {

        //
        // Paged pool.  Need to verify start of allocation using
        // end of allocation bitmap.
        //

        if (!RtlCheckBit (PagedPoolInfo->PagedPoolAllocationMap, StartPosition)) {
            KeBugCheckEx (BAD_POOL_CALLER,
                          0x50,
                          (ULONG_PTR)StartingAddress,
                          (ULONG_PTR)StartPosition,
                          MmSizeOfPagedPoolInBytes);
        }

#if DBG
        if (StartPosition > 0) {
            if (RtlCheckBit (PagedPoolInfo->PagedPoolAllocationMap, StartPosition - 1)) {
                if (!RtlCheckBit (PagedPoolInfo->EndOfPagedPoolBitmap, StartPosition - 1)) {

                    //
                    // In the middle of an allocation... bugcheck.
                    //

                    DbgPrint("paged pool in middle of allocation\n");
                    KeBugCheckEx (MEMORY_MANAGEMENT,
                                  0x41286,
                                  (ULONG_PTR)PagedPoolInfo->PagedPoolAllocationMap,
                                  (ULONG_PTR)PagedPoolInfo->EndOfPagedPoolBitmap,
                                  StartPosition);
                }
            }
        }
#endif

        i = StartPosition;
        PointerPte = PagedPoolInfo->FirstPteForPagedPool + i;

        //
        // Find the last allocated page and check to see if any
        // of the pages being deallocated are in the paging file.
        //

        while (!RtlCheckBit (PagedPoolInfo->EndOfPagedPoolBitmap, i)) {
            NumberOfPages += 1;
            i += 1;
        }

        NoAccessPte.u.Long = MM_KERNEL_NOACCESS_PTE;

        if (SessionSpace) {

            //
            // This is needed purely to verify no one leaks pool.  This
            // could be removed if we believe everyone was good.
            //

            if (RtlCheckBit (PagedPoolInfo->PagedPoolLargeSessionAllocationMap,
                             StartPosition)) {

                RtlClearBits (PagedPoolInfo->PagedPoolLargeSessionAllocationMap,
                              StartPosition,
                              1L);

                MiSessionPoolFreed (MiGetVirtualAddressMappedByPte (PointerPte),
                                    NumberOfPages << PAGE_SHIFT,
                                    PagedPool);
            }

            SessionAllocation = TRUE;
        }
        else {
            SessionAllocation = FALSE;

            if (VerifierLargePagedPoolMap) {

                if (RtlCheckBit (VerifierLargePagedPoolMap, StartPosition)) {
    
                    RtlClearBits (VerifierLargePagedPoolMap,
                                  StartPosition,
                                  1L);
    
                    VerifierFreeTrackedPool (MiGetVirtualAddressMappedByPte (PointerPte),
                                             NumberOfPages << PAGE_SHIFT,
                                             PagedPool,
                                             FALSE);
                }
            }
        }

        PagesFreed = MiDeleteSystemPagableVm (PointerPte,
                                              NumberOfPages,
                                              NoAccessPte,
                                              SessionAllocation,
                                              NULL);

        ASSERT (PagesFreed == NumberOfPages);

        if (SessionSpace) {
            LOCK_SESSION_SPACE_WS (OldIrql);
            MmSessionSpace->CommittedPages -= NumberOfPages;
    
            MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_COMMIT_POOL_FREED,
                 NumberOfPages);

            UNLOCK_SESSION_SPACE_WS (OldIrql);
        }
        else {
            MmPagedPoolCommit -= (ULONG)NumberOfPages;
        }
    
        MiReturnCommitment (NumberOfPages);

        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PAGED_POOL_PAGES, NumberOfPages);

        //
        // Clear the end of allocation bit in the bit map.
        //

        RtlClearBits (PagedPoolInfo->EndOfPagedPoolBitmap, (ULONG)i, 1L);

        PagedPoolInfo->PagedPoolCommit -= NumberOfPages;
        PagedPoolInfo->AllocatedPagedPool -= NumberOfPages;

        //
        // Clear the allocation bits in the bit map.
        //

        RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap,
                      StartPosition,
                      (ULONG)NumberOfPages
                      );

        if (StartPosition < PagedPoolInfo->PagedPoolHint) {
            PagedPoolInfo->PagedPoolHint = StartPosition;
        }

        return (ULONG)NumberOfPages;
    }
}

VOID
MiInitializeNonPagedPool (
    VOID
    )

/*++

Routine Description:

    This function initializes the NonPaged pool.

    NonPaged Pool is linked together through the pages.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, during initialization.

--*/

{
    ULONG PagesInPool;
    ULONG Size;
    ULONG Index;
    PMMFREE_POOL_ENTRY FreeEntry;
    PMMFREE_POOL_ENTRY FirstEntry;
    PMMPTE PointerPte;
    PFN_NUMBER i;
    PULONG_PTR ThisPage;
    PULONG_PTR NextPage;
    PVOID EndOfInitialPool;
    PFN_NUMBER PageFrameIndex;

    PAGED_CODE();

    //
    // Initialize the list heads for free pages.
    //

    for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index += 1) {
        InitializeListHead (&MmNonPagedPoolFreeListHead[Index]);
    }

    //
    // Initialize the must succeed pool (this occupies the first
    // pages of the pool area).
    //

    //
    // Allocate NonPaged pool for the NonPagedPoolMustSucceed pool.
    //

    MmNonPagedMustSucceed = (PCHAR)MmNonPagedPoolStart;

    i = MmSizeOfNonPagedMustSucceed - PAGE_SIZE;

    MmMustSucceedPoolBitPosition = BYTES_TO_PAGES(MmSizeOfNonPagedMustSucceed);

    ThisPage = (PULONG_PTR)MmNonPagedMustSucceed;

    while (i > 0) {
        NextPage = (PULONG_PTR)((PCHAR)ThisPage + PAGE_SIZE);
        *ThisPage = (ULONG_PTR)NextPage;
        ThisPage = NextPage;
        i -= PAGE_SIZE;
    }
    *ThisPage = 0;

    //
    // Set up the remaining pages as non paged pool pages.
    //

    ASSERT ((MmSizeOfNonPagedMustSucceed & (PAGE_SIZE - 1)) == 0);
    FreeEntry = (PMMFREE_POOL_ENTRY)((PCHAR)MmNonPagedPoolStart +
                                            MmSizeOfNonPagedMustSucceed);
    FirstEntry = FreeEntry;

    PagesInPool = BYTES_TO_PAGES(MmSizeOfNonPagedPoolInBytes -
                                    MmSizeOfNonPagedMustSucceed);

    //
    // Set the location of expanded pool.
    //

    MmExpandedPoolBitPosition = BYTES_TO_PAGES (MmSizeOfNonPagedPoolInBytes);

    MmNumberOfFreeNonPagedPool = PagesInPool;

    Index = (ULONG)(MmNumberOfFreeNonPagedPool - 1);
    if (Index >= MI_MAX_FREE_LIST_HEADS) {
        Index = MI_MAX_FREE_LIST_HEADS - 1;
    }

    InsertHeadList (&MmNonPagedPoolFreeListHead[Index], &FreeEntry->List);

    FreeEntry->Size = PagesInPool;
#if DBG
    FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
#endif
    FreeEntry->Owner = FirstEntry;

    while (PagesInPool > 1) {
        FreeEntry = (PMMFREE_POOL_ENTRY)((PCHAR)FreeEntry + PAGE_SIZE);
#if DBG
        FreeEntry->Signature = MM_FREE_POOL_SIGNATURE;
#endif
        FreeEntry->Owner = FirstEntry;
        PagesInPool -= 1;
    }

    //
    // Set the last nonpaged pool PFN so coalescing on free doesn't go
    // past the end of the initial pool.
    //

    EndOfInitialPool = (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes - 1);

    if (MI_IS_PHYSICAL_ADDRESS(EndOfInitialPool)) {
        PageFrameIndex = MI_CONVERT_PHYSICAL_TO_PFN (EndOfInitialPool);
    } else {
        PointerPte = MiGetPteAddress(EndOfInitialPool);
        ASSERT (PointerPte->u.Hard.Valid == 1);
        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
    }
    MiEndOfInitialPoolFrame = PageFrameIndex;

    //
    // Set up the system PTEs for nonpaged pool expansion.
    //

    PointerPte = MiGetPteAddress (MmNonPagedPoolExpansionStart);
    ASSERT (PointerPte->u.Hard.Valid == 0);

    Size = BYTES_TO_PAGES(MmMaximumNonPagedPoolInBytes -
                            MmSizeOfNonPagedPoolInBytes);

    //
    // Insert a guard PTE at the bottom of expanded nonpaged pool.
    //

    Size -= 1;
    PointerPte += 1;

    MiInitializeSystemPtes (PointerPte,
                            Size,
                            NonPagedPoolExpansion
                            );

    //
    // A guard PTE is built at the top by our caller.  This allows us to
    // freely increment virtual addresses in MiFreePoolPages and just check
    // for a blank PTE.
    //
}

#if DBG || (i386 && !FPO)

//
// This only works on checked builds, because the TraceLargeAllocs array is
// kept in that case to keep track of page size pool allocations.  Otherwise
// we will call ExpSnapShotPoolPages with a page size pool allocation containing
// arbitrary data and it will potentially go off in the weeds trying to
// interpret it as a suballocated pool page.  Ideally, there would be another
// bit map that identified single page pool allocations so
// ExpSnapShotPoolPages would NOT be called for those.
//

NTSTATUS
MmSnapShotPool(
    IN POOL_TYPE PoolType,
    IN PMM_SNAPSHOT_POOL_PAGE SnapShotPoolPage,
    IN PSYSTEM_POOL_INFORMATION PoolInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )
{
    ULONG Index;
    NTSTATUS Status;
    NTSTATUS xStatus;
    PCHAR p, pStart;
    PVOID *pp;
    ULONG Size;
    ULONG BusyFlag;
    ULONG CurrentPage, NumberOfPages;
    PSYSTEM_POOL_ENTRY PoolEntryInfo;
    PLIST_ENTRY Entry;
    PMMFREE_POOL_ENTRY FreePageInfo;
    ULONG StartPosition;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    LOGICAL NeedsReprotect;

    Status = STATUS_SUCCESS;
    PoolEntryInfo = &PoolInformation->Entries[0];

    if (PoolType == PagedPool) {
        PoolInformation->TotalSize = (PCHAR)MmPagedPoolEnd -
                                     (PCHAR)MmPagedPoolStart;
        PoolInformation->FirstEntry = MmPagedPoolStart;
        p = MmPagedPoolStart;
        CurrentPage = 0;
        while (p < (PCHAR)MmPagedPoolEnd) {
            pStart = p;
            BusyFlag = RtlCheckBit (MmPagedPoolInfo.PagedPoolAllocationMap, CurrentPage);
            while (~(BusyFlag ^ RtlCheckBit (MmPagedPoolInfo.PagedPoolAllocationMap, CurrentPage))) {
                p += PAGE_SIZE;
                if (RtlCheckBit (MmPagedPoolInfo.EndOfPagedPoolBitmap, CurrentPage)) {
                    CurrentPage += 1;
                    break;
                }

                CurrentPage += 1;
                if (p > (PCHAR)MmPagedPoolEnd) {
                    break;
               }
            }

            Size = (ULONG)(p - pStart);
            if (BusyFlag) {
                xStatus = (*SnapShotPoolPage)(pStart,
                                              Size,
                                              PoolInformation,
                                              &PoolEntryInfo,
                                              Length,
                                              RequiredLength
                                              );
                if (xStatus != STATUS_COMMITMENT_LIMIT) {
                    Status = xStatus;
                }
            }
            else {
                PoolInformation->NumberOfEntries += 1;
                *RequiredLength += sizeof (SYSTEM_POOL_ENTRY);
                if (Length < *RequiredLength) {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else {
                    PoolEntryInfo->Allocated = FALSE;
                    PoolEntryInfo->Size = Size;
                    PoolEntryInfo->AllocatorBackTraceIndex = 0;
                    PoolEntryInfo->TagUlong = 0;
                    PoolEntryInfo += 1;
                    Status = STATUS_SUCCESS;
                }
            }
        }
    }
    else if (PoolType == NonPagedPool) {
        PoolInformation->TotalSize = MmSizeOfNonPagedPoolInBytes;
        PoolInformation->FirstEntry = MmNonPagedPoolStart;

        p = MmNonPagedPoolStart;
        while (p < (PCHAR)MmNonPagedPoolEnd) {

            //
            // NonPaged pool is linked together through the pages themselves.
            //

            pp = (PVOID *)MmNonPagedMustSucceed;
            while (pp) {
                if (p == (PCHAR)pp) {
                    PoolInformation->NumberOfEntries += 1;
                    *RequiredLength += sizeof( SYSTEM_POOL_ENTRY );
                    if (Length < *RequiredLength) {
                        Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    else {
                        PoolEntryInfo->Allocated = FALSE;
                        PoolEntryInfo->Size = PAGE_SIZE;
                        PoolEntryInfo->AllocatorBackTraceIndex = 0;
                        PoolEntryInfo->TagUlong = 0;
                        PoolEntryInfo += 1;
                        Status = STATUS_SUCCESS;
                    }

                    p += PAGE_SIZE;
                    pp = (PVOID *)MmNonPagedMustSucceed;
                }
                else {
                    pp = (PVOID *)*pp;
                }
            }

            NeedsReprotect = FALSE;

            for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index += 1) {

                Entry = MmNonPagedPoolFreeListHead[Index].Flink;
    
                while (Entry != &MmNonPagedPoolFreeListHead[Index]) {
    
                    if (MmProtectFreedNonPagedPool == TRUE) {
                        MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
                        NeedsReprotect = TRUE;
                    }

                    FreePageInfo = CONTAINING_RECORD( Entry,
                                                      MMFREE_POOL_ENTRY,
                                                      List
                                                    );
    
                    ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);
    
                    if (p == (PCHAR)FreePageInfo) {
    
                        Size = (ULONG)(FreePageInfo->Size << PAGE_SHIFT);
                        PoolInformation->NumberOfEntries += 1;
                        *RequiredLength += sizeof( SYSTEM_POOL_ENTRY );
                        if (Length < *RequiredLength) {
                            Status = STATUS_INFO_LENGTH_MISMATCH;
                        }
                        else {
                            PoolEntryInfo->Allocated = FALSE;
                            PoolEntryInfo->Size = Size;
                            PoolEntryInfo->AllocatorBackTraceIndex = 0;
                            PoolEntryInfo->TagUlong = 0;
                            PoolEntryInfo += 1;
                            Status = STATUS_SUCCESS;
                        }
    
                        p += Size;
                        Index = MI_MAX_FREE_LIST_HEADS;
                        break;
                    }
    
                    Entry = FreePageInfo->List.Flink;
    
                    if (NeedsReprotect == TRUE) {
                        MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                                   (ULONG)FreePageInfo->Size);
                        NeedsReprotect = FALSE;
                    }
                }
            }

            StartPosition = BYTES_TO_PAGES((PCHAR)p -
                  (PCHAR)MmPageAlignedPoolBase[NonPagedPool]);
            if (StartPosition >= MmExpandedPoolBitPosition) {
                if (NeedsReprotect == TRUE) {
                    MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                               (ULONG)FreePageInfo->Size);
                }
                break;
            }

            if (StartPosition < MmMustSucceedPoolBitPosition) {
                ASSERT (NeedsReprotect == FALSE);
                Size = PAGE_SIZE;
                xStatus = (*SnapShotPoolPage) (p,
                                               Size,
                                               PoolInformation,
                                               &PoolEntryInfo,
                                               Length,
                                               RequiredLength
                                              );
                if (xStatus != STATUS_COMMITMENT_LIMIT) {
                    Status = xStatus;
                }
            }
            else {
                if (MI_IS_PHYSICAL_ADDRESS(p)) {
                    //
                    // On certain architectures, virtual addresses
                    // may be physical and hence have no corresponding PTE.
                    //
                    PointerPte = NULL;
                    Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (p));
                } else {
                    PointerPte = MiGetPteAddress (p);
                    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                }
                ASSERT (Pfn1->u3.e1.StartOfAllocation != 0);

                //
                // Find end of allocation and determine size.
                //

                NumberOfPages = 1;
                while (Pfn1->u3.e1.EndOfAllocation == 0) {
                    NumberOfPages += 1;
                    if (PointerPte == NULL) {
                        Pfn1 += 1;
                    }
                    else {
                        PointerPte += 1;
                        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                    }
                }

                Size = NumberOfPages << PAGE_SHIFT;
                xStatus = (*SnapShotPoolPage) (p,
                                               Size,
                                               PoolInformation,
                                               &PoolEntryInfo,
                                               Length,
                                               RequiredLength
                                              );
                if (NeedsReprotect == TRUE) {
                    MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                               (ULONG)FreePageInfo->Size);
                }

                if (xStatus != STATUS_COMMITMENT_LIMIT) {
                    Status = xStatus;
                }
            }

            p += Size;
        }
    }
    else {
        Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

#endif // DBG || (i386 && !FPO)

ULONG MmSpecialPoolTag;
PVOID MmSpecialPoolStart;
PVOID MmSpecialPoolEnd;

ULONG MmSpecialPoolRejected[5];
LOGICAL MmSpecialPoolCatchOverruns = TRUE;


PMMPTE MiSpecialPoolFirstPte;
PMMPTE MiSpecialPoolLastPte;

ULONG MiSpecialPagesNonPaged;
ULONG MiSpecialPagesPagable;
ULONG MmSpecialPagesInUse;      // Used by the debugger

ULONG MiSpecialPagesNonPagedPeak;
ULONG MiSpecialPagesPagablePeak;
ULONG MiSpecialPagesInUsePeak;

ULONG MiSpecialPagesNonPagedMaximum;

ULONG MiSpecialPoolPtes;

LOGICAL MiSpecialPoolEnabled = TRUE;

SIZE_T
MmQuerySpecialPoolBlockSize (
    IN PVOID P
    )

/*++

Routine Description:

    This routine returns the size of a special pool allocation.

Arguments:

    VirtualAddress - Supplies the special pool virtual address to query.

Return Value:

    The size in bytes of the allocation.

Environment:

    Kernel mode, APC_LEVEL or below for pagable addresses, DISPATCH_LEVEL or
    below for nonpaged addresses.

--*/

{
    PPOOL_HEADER Header;

    ASSERT ((P >= MmSpecialPoolStart) && (P < MmSpecialPoolEnd));

    if (((ULONG_PTR)P & (PAGE_SIZE - 1))) {
        Header = PAGE_ALIGN (P);
    }
    else {
        Header = (PPOOL_HEADER)((PCHAR)PAGE_ALIGN (P) + PAGE_SIZE - POOL_OVERHEAD);
    }

    return (SIZE_T)(Header->Ulong1 & ~(MI_SPECIAL_POOL_PAGABLE | MI_SPECIAL_POOL_VERIFIER));
}

VOID
MiMakeSpecialPoolPagable (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte
    );

VOID
MiInitializeSpecialPool (
    VOID
    )

/*++

Routine Description:

    This routine initializes the special pool used to catch pool corruptors.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, no locks held.

--*/

{
    ULONG BugCheckOnFailure;
    KIRQL OldIrql;
    PMMPTE PointerPte;

    if ((MmVerifyDriverBufferLength == (ULONG)-1) &&
        ((MmSpecialPoolTag == 0) || (MmSpecialPoolTag == (ULONG)-1))) {
            return;
    }

#if PFN_CONSISTENCY
    MiUnMapPfnDatabase ();
#endif

    LOCK_PFN (OldIrql);

    //
    // Even though we asked for some number of system PTEs to map special pool,
    // we may not have been given them all.  Large memory systems are
    // autoconfigured so that a large nonpaged pool is the default.
    // x86 systems booted with the 3GB switch and all Alphas don't have enough
    // contiguous virtual address space to support this, so our request may
    // have been trimmed.  Handle that intelligently here so we don't exhaust
    // the system PTE pool and fail to handle thread stacks and I/O.
    //

    if (MmNumberOfSystemPtes < 0x3000) {
        MiSpecialPoolPtes = MmNumberOfSystemPtes / 6;
    }
    else {
        MiSpecialPoolPtes = MmNumberOfSystemPtes / 3;
    }

#if !defined (_WIN64)

    //
    // 32-bit systems are very cramped on virtual address space.  Apply
    // a cap here to prevent overzealousness.
    //

    if (MiSpecialPoolPtes > MM_SPECIAL_POOL_PTES) {
        MiSpecialPoolPtes = MM_SPECIAL_POOL_PTES;
    }
#endif

#ifdef _X86_

    //
    // For x86, we can actually use an additional range of special PTEs to
    // map memory with and so we can raise the limit from 25000 to approximately
    // 96000.
    //

    if ((MiNumberOfExtraSystemPdes != 0) &&
        ((MiHydra == FALSE) || (ExpMultiUserTS == FALSE)) &&
        (MiRequestedSystemPtes != (ULONG)-1)) {

        if (MmPagedPoolMaximumDesired == TRUE) {

            //
            // The low PTEs between 0xA4000000 & 0xC0000000 must be used
            // for both regular system PTE usage and special pool usage.
            //

            MiSpecialPoolPtes = (MiNumberOfExtraSystemPdes / 2) * PTE_PER_PAGE;
        }
        else {

            //
            // The low PTEs between 0xA4000000 & 0xC0000000 can be used
            // exclusively for special pool.
            //

            MiSpecialPoolPtes = MiNumberOfExtraSystemPdes * PTE_PER_PAGE;
        }
    }

#endif

    //
    // A PTE disappears for double mapping the system page directory.
    // When guard paging for system PTEs is enabled, a few more go also.
    // Thus, not being able to get all the PTEs we wanted is not fatal and
    // we just back off a bit and retry.
    //

    //
    // Always request an even number of PTEs so each one can be guard paged.
    //

    MiSpecialPoolPtes &= ~0x1;

    BugCheckOnFailure = FALSE;

    do {
        MiSpecialPoolFirstPte = MiReserveSystemPtes (MiSpecialPoolPtes,
                                                     SystemPteSpace,
                                                     0,
                                                     0,
                                                     BugCheckOnFailure);
        if (MiSpecialPoolFirstPte != NULL) {
            break;
        }

        if (MiSpecialPoolPtes == 0) {
            BugCheckOnFailure = TRUE;
            continue;
        }

        MiSpecialPoolPtes -= 2;
    } while (1);

    //
    // Build the list of PTE pairs.
    //

    MiSpecialPoolLastPte = MiSpecialPoolFirstPte + MiSpecialPoolPtes;
    MmSpecialPoolStart = MiGetVirtualAddressMappedByPte (MiSpecialPoolFirstPte);

    PointerPte = MiSpecialPoolFirstPte;
    while (PointerPte < MiSpecialPoolLastPte) {
        PointerPte->u.List.NextEntry = ((PointerPte + 2) - MmSystemPteBase);
        PointerPte += 2;
    }
    PointerPte -= 2;
    PointerPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;
    MiSpecialPoolLastPte = PointerPte;
    MmSpecialPoolEnd = MiGetVirtualAddressMappedByPte (MiSpecialPoolLastPte + 1);

    //
    // Cap nonpaged special pool based on the memory size.
    //

    MiSpecialPagesNonPagedMaximum = (ULONG)(MmResidentAvailablePages >> 4);

    if (MmNumberOfPhysicalPages > 0x3FFF) {
        MiSpecialPagesNonPagedMaximum = (ULONG)(MmResidentAvailablePages >> 3);
    }

    UNLOCK_PFN (OldIrql);
}

LOGICAL
MmSetSpecialPool (
    IN LOGICAL Enable
    )

/*++

Routine Description:

    This routine enables/disables special pool.  This allows callers to ensure
    that subsequent allocations do not come from special pool.  It is relied
    upon by callers that require KSEG0 addresses.

Arguments:

    Enable - Supplies TRUE to enable special pool, FALSE to disable it.

Return Value:

    Current special pool state (enabled or disabled).

Environment:

    Kernel mode, IRQL of DISPATCH_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    LOGICAL OldEnable;

    LOCK_PFN2 (OldIrql);

    OldEnable = MiSpecialPoolEnabled;

    MiSpecialPoolEnabled = Enable;

    UNLOCK_PFN2 (OldIrql);

    return OldEnable;
}

#ifndef NO_POOL_CHECKS
typedef struct _MI_BAD_TAGS {
    USHORT  Enabled;
    UCHAR   TargetChar;
    UCHAR   AllOthers;
    ULONG   Dispatches;
    ULONG   Allocations;
    ULONG   RandomizerEnabled;
} MI_BAD_TAGS, *PMI_BAD_TAGS;

MI_BAD_TAGS MiBadTags;
KTIMER MiSpecialPoolTimer;
KDPC MiSpecialPoolTimerDpc;
LARGE_INTEGER MiTimerDueTime;

#define MI_THREE_SECONDS     3


VOID
MiSpecialPoolTimerDispatch (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    )

/*++

Routine Description:

    This routine is executed every 3 seconds.  Just toggle the enable bit.
    If not many squeezed allocations have been made then just leave it
    continuously enabled.  Switch to a different tag if it looks like this
    one isn't getting any hits.

    No locks needed.

Arguments:

    Dpc - Supplies a pointer to a control object of type DPC.

    DeferredContext - Optional deferred context;  not used.

    SystemArgument1 - Optional argument 1;  not used.

    SystemArgument2 - Optional argument 2;  not used.

Return Value:

    None.

--*/

{
    UCHAR NewChar;

    UNREFERENCED_PARAMETER (Dpc);
    UNREFERENCED_PARAMETER (DeferredContext);
    UNREFERENCED_PARAMETER (SystemArgument1);
    UNREFERENCED_PARAMETER (SystemArgument2);

    MiBadTags.Dispatches += 1;

    if (MiBadTags.Allocations > 500) {
        MiBadTags.Enabled += 1;
    }
    else if ((MiBadTags.Allocations == 0) && (MiBadTags.Dispatches > 100)) {
        if (MiBadTags.AllOthers == 0) {
            NewChar = (UCHAR)(MiBadTags.TargetChar + 1);
            if (NewChar >= 'a' && NewChar <= 'z') {
                MiBadTags.TargetChar = NewChar;
            }
            else if (NewChar == 'z' + 1) {
                MiBadTags.TargetChar = 'a';
            }
            else if (NewChar >= 'A' && NewChar <= 'Z') {
                MiBadTags.TargetChar = NewChar;
            }
            else {
                MiBadTags.TargetChar = 'A';
            }
        }
    }
}

extern ULONG InitializationPhase;

VOID
MiInitializeSpecialPoolCriteria (
    VOID
    )
{
    LARGE_INTEGER SystemTime;
    TIME_FIELDS TimeFields;

    if (InitializationPhase == 0) {
#if defined (_MI_SPECIAL_POOL_BY_DEFAULT)
        if (MmSpecialPoolTag == 0) {
            MmSpecialPoolTag = (ULONG)-2;
        }
#endif
        return;
    }

    if (MmSpecialPoolTag != (ULONG)-2) {
        return;
    }

    KeQuerySystemTime (&SystemTime);

    RtlTimeToTimeFields (&SystemTime, &TimeFields);

    if (TimeFields.Second <= 25) {
        MiBadTags.TargetChar = (UCHAR)('a' + (UCHAR)TimeFields.Second);
    }
    else if (TimeFields.Second <= 51) {
        MiBadTags.TargetChar = (UCHAR)('A' + (UCHAR)(TimeFields.Second - 26));
    }
    else {
        MiBadTags.AllOthers = 1;
    }

    MiBadTags.RandomizerEnabled = 1;

    //
    // Initialize a periodic timer to go off every three seconds.
    //

    KeInitializeDpc (&MiSpecialPoolTimerDpc, MiSpecialPoolTimerDispatch, NULL);

    KeInitializeTimer (&MiSpecialPoolTimer);

    MiTimerDueTime.QuadPart = Int32x32To64 (MI_THREE_SECONDS, -10000000);

    KeSetTimerEx (&MiSpecialPoolTimer,
                  MiTimerDueTime,
                  MI_THREE_SECONDS * 1000,
                  &MiSpecialPoolTimerDpc);

    MiBadTags.Enabled += 1;
}

PVOID
MmSqueezeBadTags (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    )

/*++

Routine Description:

    This routine squeezes bad tags by forcing them into special pool in a
    systematic fashion.

Arguments:

    NumberOfBytes - Supplies the number of bytes to commit.

    Tag - Supplies the tag of the requested allocation.

    PoolType - Supplies the pool type of the requested allocation.

    SpecialPoolType - Supplies the special pool type of the
                      requested allocation.

                      - 0 indicates overruns.
                      - 1 indicates underruns.
                      - 2 indicates use the systemwide pool policy.

Return Value:

    A non-NULL pointer if the requested allocation was fulfilled from special
    pool.  NULL if the allocation was not made.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    PUCHAR tc;

    if ((MiBadTags.Enabled % 0x10) == 0) {
        return NULL;
    }

    if (MiBadTags.RandomizerEnabled == 0) {
        return NULL;
    }

    tc = (PUCHAR)&Tag;
    if (*tc == MiBadTags.TargetChar) {
        ;
    }
    else if (MiBadTags.AllOthers == 1) {
        if (*tc >= 'a' && *tc <= 'z') {
            return NULL;
        }
        if (*tc >= 'A' && *tc <= 'Z') {
            return NULL;
        }
    }
    else {
        return NULL;
    }

    MiBadTags.Allocations += 1;

    return MmAllocateSpecialPool(NumberOfBytes, Tag, PoolType, SpecialPoolType);
}

VOID
MiEnableRandomSpecialPool (
    IN LOGICAL Enable
    )
{
    MiBadTags.RandomizerEnabled = Enable;
}

#endif

PVOID
MmAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    )

/*++

Routine Description:

    This routine allocates virtual memory from special pool.  This allocation
    is made from the end of a physical page with the next PTE set to no access
    so that any reads or writes will cause an immediate fatal system crash.
    
    This lets us catch components that corrupt pool.

Arguments:

    NumberOfBytes - Supplies the number of bytes to commit.

    Tag - Supplies the tag of the requested allocation.

    PoolType - Supplies the pool type of the requested allocation.

    SpecialPoolType - Supplies the special pool type of the
                      requested allocation.

                      - 0 indicates overruns.
                      - 1 indicates underruns.
                      - 2 indicates use the systemwide pool policy.

Return Value:

    A non-NULL pointer if the requested allocation was fulfilled from special
    pool.  NULL if the allocation was not made.

Environment:

    Kernel mode, no pool locks held.

    Note this is a nonpagable wrapper so that machines without special pool
    can still support drivers allocating nonpaged pool at DISPATCH_LEVEL
    requesting special pool.

--*/

{
    if (MiSpecialPoolPtes == 0) {

        //
        // The special pool allocation code was never initialized.
        //

        return NULL;
    }

    return MiAllocateSpecialPool (NumberOfBytes,
                                  Tag,
                                  PoolType,
                                  SpecialPoolType);
}

PVOID
MiAllocateSpecialPool (
    IN SIZE_T NumberOfBytes,
    IN ULONG Tag,
    IN POOL_TYPE PoolType,
    IN ULONG SpecialPoolType
    )

/*++

Routine Description:

    This routine allocates virtual memory from special pool.  This allocation
    is made from the end of a physical page with the next PTE set to no access
    so that any reads or writes will cause an immediate fatal system crash.
    
    This lets us catch components that corrupt pool.

Arguments:

    NumberOfBytes - Supplies the number of bytes to commit.

    Tag - Supplies the tag of the requested allocation.

    PoolType - Supplies the pool type of the requested allocation.

    SpecialPoolType - Supplies the special pool type of the
                      requested allocation.

                      - 0 indicates overruns.
                      - 1 indicates underruns.
                      - 2 indicates use the systemwide pool policy.

Return Value:

    A non-NULL pointer if the requested allocation was fulfilled from special
    pool.  NULL if the allocation was not made.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PMMPTE PointerPte;
    KIRQL OldIrql;
    PVOID Entry;
    PPOOL_HEADER Header;
    LARGE_INTEGER CurrentTime;
    LOGICAL CatchOverruns;

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {

        if (KeGetCurrentIrql() > APC_LEVEL) {

            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes,
                          0x30);
        }
    }
    else {
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {

            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          PoolType,
                          NumberOfBytes,
                          0x30);
        }
    }

#if defined (_X86_) && !defined (_X86PAE_)

    if (MiNumberOfExtraSystemPdes != 0) {

        extern ULONG MMSECT;

        //
        // Prototype PTEs cannot come from lower special pool because
        // their address is encoded into PTEs and the encoding only covers
        // a max of 1GB from the start of paged pool.  Likewise fork
        // prototype PTEs.
        //

        if (Tag == MMSECT || Tag == 'lCmM') {
            return NULL;
        }
    }

#endif

#if !defined (_WIN64) && !defined (_X86PAE_)

    if (Tag == 'bSmM' || Tag == 'iCmM' || Tag == 'aCmM') {

        //
        // Mm subsections cannot come from this special pool because they
        // get encoded into PTEs - they must come from normal nonpaged pool.
        //

        return NULL;
    }

#endif

    TempPte = ValidKernelPte;

    LOCK_PFN2 (OldIrql);

    if (MiSpecialPoolEnabled == FALSE) {

        //
        // The special pool allocation code is currently disabled.
        //

        UNLOCK_PFN2 (OldIrql);
        return NULL;
    }

    if (MmAvailablePages < 200) {
        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[0] += 1;
        return NULL;
    }

    //
    // Don't get too aggressive until a paging file gets set up.
    //

    if (MmNumberOfPagingFiles == 0 && MmSpecialPagesInUse > MmAvailablePages / 2) {
        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[3] += 1;
        return NULL;
    }

    if (MiSpecialPoolFirstPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[2] += 1;
        return NULL;
    }

    if (MmResidentAvailablePages < 100) {
        UNLOCK_PFN2 (OldIrql);
        MmSpecialPoolRejected[4] += 1;
        return NULL;
    }

    //
    // Cap nonpaged allocations to prevent runaways.
    //

    if ((PoolType & BASE_POOL_TYPE_MASK) == NonPagedPool) {

        if (MiSpecialPagesNonPaged > MiSpecialPagesNonPagedMaximum) {
            UNLOCK_PFN2 (OldIrql);
            MmSpecialPoolRejected[1] += 1;
            return NULL;
        }

        MmResidentAvailablePages -= 1;

        MM_BUMP_COUNTER(31, 1);

        MiSpecialPagesNonPaged += 1;
        if (MiSpecialPagesNonPaged > MiSpecialPagesNonPagedPeak) {
            MiSpecialPagesNonPagedPeak = MiSpecialPagesNonPaged;
        }
    }
    else {
        MiSpecialPagesPagable += 1;
        if (MiSpecialPagesPagable > MiSpecialPagesPagablePeak) {
            MiSpecialPagesPagablePeak = MiSpecialPagesPagable;
        }
    }

    PointerPte = MiSpecialPoolFirstPte;

    ASSERT (MiSpecialPoolFirstPte->u.List.NextEntry != MM_EMPTY_PTE_LIST);

    MiSpecialPoolFirstPte = PointerPte->u.List.NextEntry + MmSystemPteBase;

    PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    MmSpecialPagesInUse += 1;
    if (MmSpecialPagesInUse > MiSpecialPagesInUsePeak) {
        MiSpecialPagesInUsePeak = MmSpecialPagesInUse;
    }

    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        MI_SET_PTE_DIRTY (TempPte);
    }

    MI_WRITE_VALID_PTE (PointerPte, TempPte);
    MiInitializePfn (PageFrameIndex, PointerPte, 1);
    UNLOCK_PFN2 (OldIrql);

    //
    // Fill the page with a random pattern.
    //

    KeQueryTickCount(&CurrentTime);

    Entry = MiGetVirtualAddressMappedByPte (PointerPte);

    RtlFillMemory (Entry, PAGE_SIZE, (UCHAR) CurrentTime.LowPart);

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        MiMakeSpecialPoolPagable (Entry, PointerPte);
        (PointerPte + 1)->u.Soft.PageFileHigh = MI_SPECIAL_POOL_PTE_PAGABLE;
    }
    else {
        (PointerPte + 1)->u.Soft.PageFileHigh = MI_SPECIAL_POOL_PTE_NONPAGABLE;
    }

    if (SpecialPoolType == 0) {
        CatchOverruns = TRUE;
    }
    else if (SpecialPoolType == 1) {
        CatchOverruns = FALSE;
    }
    else if (MmSpecialPoolCatchOverruns == TRUE) {
        CatchOverruns = TRUE;
    }
    else {
        CatchOverruns = FALSE;
    }

    if (CatchOverruns == TRUE) {
        Header = (PPOOL_HEADER) Entry;
        Entry = (PVOID)(((LONG_PTR)(((PCHAR)Entry + (PAGE_SIZE - NumberOfBytes)))) & ~((LONG_PTR)POOL_OVERHEAD - 1));
    }
    else {
        Header = (PPOOL_HEADER) ((PCHAR)Entry + PAGE_SIZE - POOL_OVERHEAD);
    }

    //
    // Zero the header and stash any information needed at release time.
    //

    RtlZeroMemory (Header, POOL_OVERHEAD);

    Header->Ulong1 = (ULONG)NumberOfBytes;

    ASSERT (NumberOfBytes <= PAGE_SIZE - POOL_OVERHEAD && PAGE_SIZE <= 32 * 1024);

    if ((PoolType & BASE_POOL_TYPE_MASK) == PagedPool) {
        Header->Ulong1 |= MI_SPECIAL_POOL_PAGABLE;
    }

    if (PoolType & POOL_VERIFIER_MASK) {
        Header->Ulong1 |= MI_SPECIAL_POOL_VERIFIER;
    }

    Header->BlockSize = (UCHAR) CurrentTime.LowPart;
    Header->PoolTag = Tag;

    MiChargeCommitmentCantExpand (1, TRUE);

    ASSERT ((Header->PoolType & POOL_QUOTA_MASK) == 0);

    return Entry;
}

VOID
MmFreeSpecialPool (
    IN PVOID P
    )

/*++

Routine Description:

    This routine frees a special pool allocation.  The backing page is freed
    and the mapping virtual address is made no access (the next virtual
    address is already no access).

    The virtual address PTE pair is then placed into an LRU queue to provide
    maximum no-access (protection) life to catch components that access
    deallocated pool.

Arguments:

    VirtualAddress - Supplies the special pool virtual address to free.

Return Value:

    None.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    MMPTE PteContents;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    ULONG SlopBytes;
    ULONG NumberOfBytesCalculated;
    ULONG NumberOfBytesRequested;
    POOL_TYPE PoolType;
    MMPTE NoAccessPte;
    PPOOL_HEADER Header;
    PUCHAR Slop;
    ULONG i;
    LOGICAL BufferAtPageEnd;
    PMI_FREED_SPECIAL_POOL AllocationBase;
    LARGE_INTEGER CurrentTime;
    PULONG_PTR StackPointer;

    PointerPte = MiGetPteAddress (P);
    PteContents = *PointerPte;

    //
    // Check the PTE now so we can give a more friendly bugcheck rather than
    // crashing below on a bad reference.
    //

    if (PteContents.u.Hard.Valid == 0) {
        if ((PteContents.u.Soft.Protection == 0) ||
            (PteContents.u.Soft.Protection == MM_NOACCESS)) {
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          (ULONG_PTR)PteContents.u.Long,
                          0,
                          0x20);
        }
    }

    if (((ULONG_PTR)P & (PAGE_SIZE - 1))) {
        Header = PAGE_ALIGN (P);
        BufferAtPageEnd = TRUE;
    }
    else {
        Header = (PPOOL_HEADER)((PCHAR)PAGE_ALIGN (P) + PAGE_SIZE - POOL_OVERHEAD);
        BufferAtPageEnd = FALSE;
    }

    if (Header->Ulong1 & MI_SPECIAL_POOL_PAGABLE) {
        ASSERT ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE);
        if (KeGetCurrentIrql() > APC_LEVEL) {
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          PagedPool,
                          (ULONG_PTR)P,
                          0x31);
        }
        PoolType = PagedPool;
    }
    else {
        ASSERT ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE);
        if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          KeGetCurrentIrql(),
                          NonPagedPool,
                          (ULONG_PTR)P,
                          0x31);
        }
        PoolType = NonPagedPool;
    }

    NumberOfBytesRequested = (ULONG)(USHORT)(Header->Ulong1 & ~(MI_SPECIAL_POOL_PAGABLE | MI_SPECIAL_POOL_VERIFIER));

    //
    // We gave the caller pool-header aligned data, so account for
    // that when checking here.
    //

    if (BufferAtPageEnd == TRUE) {

        NumberOfBytesCalculated = PAGE_SIZE - BYTE_OFFSET(P);
    
        if (NumberOfBytesRequested > NumberOfBytesCalculated) {
    
            //
            // Seems like we didn't give the caller enough - this is an error.
            //
    
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          NumberOfBytesRequested,
                          NumberOfBytesCalculated,
                          0x21);
        }
    
        if (NumberOfBytesRequested + POOL_OVERHEAD < NumberOfBytesCalculated) {
    
            //
            // Seems like we gave the caller too much - also an error.
            //
    
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          NumberOfBytesRequested,
                          NumberOfBytesCalculated,
                          0x22);
        }

        //
        // Check the memory before the start of the caller's allocation.
        //
    
        Slop = (PUCHAR)(Header + 1);
        if (Header->Ulong1 & MI_SPECIAL_POOL_VERIFIER) {
            Slop += sizeof(MI_VERIFIER_POOL_HEADER);
        }

        for ( ; Slop < (PUCHAR)P; Slop += 1) {
    
            if (*Slop != Header->BlockSize) {
    
                KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                              (ULONG_PTR)P,
                              (ULONG_PTR)Slop,
                              Header->Ulong1,
                              0x23);
            }
        }
    }
    else {
        NumberOfBytesCalculated = 0;
    }

    //
    // Check the memory after the end of the caller's allocation.
    //

    Slop = (PUCHAR)P + NumberOfBytesRequested;

    SlopBytes = (ULONG)((PUCHAR)(PAGE_ALIGN(P)) + PAGE_SIZE - Slop);

    if (BufferAtPageEnd == FALSE) {
        SlopBytes -= POOL_OVERHEAD;
        if (Header->Ulong1 & MI_SPECIAL_POOL_VERIFIER) {
            SlopBytes -= sizeof(MI_VERIFIER_POOL_HEADER);
        }
    }

    for (i = 0; i < SlopBytes; i += 1) {

        if (*Slop != Header->BlockSize) {

            //
            // The caller wrote slop between the free alignment we gave and the
            // end of the page (this is not detectable from page protection).
            //
    
            KeBugCheckEx (SPECIAL_POOL_DETECTED_MEMORY_CORRUPTION,
                          (ULONG_PTR)P,
                          (ULONG_PTR)Slop,
                          Header->Ulong1,
                          0x24);
        }
        Slop += 1;
    }

    if (Header->Ulong1 & MI_SPECIAL_POOL_VERIFIER) {
        VerifierFreeTrackedPool (P,
                                 NumberOfBytesRequested,
                                 PoolType,
                                 TRUE);
    }

    AllocationBase = (PMI_FREED_SPECIAL_POOL)(PAGE_ALIGN (P));

    AllocationBase->Signature = MI_FREED_SPECIAL_POOL_SIGNATURE;

    KeQueryTickCount(&CurrentTime);
    AllocationBase->TickCount = CurrentTime.LowPart;

    AllocationBase->NumberOfBytesRequested = NumberOfBytesRequested;
    AllocationBase->Pagable = (ULONG)PoolType;
    AllocationBase->VirtualAddress = P;
    AllocationBase->Thread = PsGetCurrentThread ();

#if defined (_X86_)
    _asm {
        mov StackPointer, esp
    }

    AllocationBase->StackPointer = StackPointer;

    //
    // For now, don't get fancy with copying more than what's in the current
    // stack page.  To do so would require checking the thread stack limits,
    // DPC stack limits, etc.
    //

    AllocationBase->StackBytes = PAGE_SIZE - BYTE_OFFSET(StackPointer);

    if (AllocationBase->StackBytes != 0) {

        if (AllocationBase->StackBytes > MI_STACK_BYTES) {
            AllocationBase->StackBytes = MI_STACK_BYTES;
        }

        RtlCopyMemory (AllocationBase->StackData,
                       StackPointer,
                       AllocationBase->StackBytes);
    }
#else
    AllocationBase->StackPointer = NULL;
    AllocationBase->StackBytes = 0;
#endif

    if (PoolType == PagedPool) {
        NoAccessPte.u.Long = MM_KERNEL_NOACCESS_PTE;
        MiDeleteSystemPagableVm (PointerPte,
                                 1,
                                 NoAccessPte,
                                 FALSE,
                                 NULL);
        LOCK_PFN2 (OldIrql);
        MiSpecialPagesPagable -= 1;
    }
    else {

        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
        LOCK_PFN2 (OldIrql);
        MiSpecialPagesNonPaged -= 1;
        MI_SET_PFN_DELETED (Pfn1);
        MiDecrementShareCount (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte));
        KeFlushSingleTb (PAGE_ALIGN(P),
                         TRUE,
                         TRUE,
                         (PHARDWARE_PTE)PointerPte,
                         ZeroKernelPte.u.Flush);
        MmResidentAvailablePages += 1;
        MM_BUMP_COUNTER(37, 1);
    }

    // 
    // Clear the adjacent PTE to support MmIsSpecialPoolAddressFree().
    // 

    (PointerPte + 1)->u.Long = 0;

    ASSERT (MiSpecialPoolLastPte->u.List.NextEntry == MM_EMPTY_PTE_LIST);
    MiSpecialPoolLastPte->u.List.NextEntry = PointerPte - MmSystemPteBase;

    MiSpecialPoolLastPte = PointerPte;
    MiSpecialPoolLastPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

    MmSpecialPagesInUse -= 1;

    UNLOCK_PFN2 (OldIrql);

    MiReturnCommitment (1);

    return;
}


VOID
MiMakeSpecialPoolPagable (
    IN PVOID VirtualAddress,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    Make a special pool allocation pagable.

Arguments:

    VirtualAddress - Supplies the faulting address.

    PointerPte - Supplies the PTE for the faulting address.

Return Value:

    None.

Environment:

    Kernel mode, no locks (not even pool locks) held.

--*/

{
    PMMPFN Pfn1;
    MMPTE TempPte;
#if defined(_ALPHA_) && !defined(_AXP64_)
    KIRQL OldIrql;
#endif
    KIRQL PreviousIrql;
    PFN_NUMBER PageFrameIndex;
#if PFN_CONSISTENCY
    KIRQL PfnIrql;
#endif

    LOCK_SYSTEM_WS (PreviousIrql);

    //
    // As this page is now allocated, add it to the system working set to
    // make it pagable.
    //

    TempPte = *PointerPte;

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&TempPte);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    ASSERT (Pfn1->u1.Event == 0);

    CONSISTENCY_LOCK_PFN (PfnIrql);

    Pfn1->u1.Event = (PVOID) PsGetCurrentThread();

    CONSISTENCY_UNLOCK_PFN (PfnIrql);

    MiAddValidPageToWorkingSet (VirtualAddress,
                                PointerPte,
                                Pfn1,
                                0);

    ASSERT (KeGetCurrentIrql() == APC_LEVEL);

    if (MmSystemCacheWs.AllowWorkingSetAdjustment == MM_GROW_WSLE_HASH) {
        MiGrowWsleHash (&MmSystemCacheWs);
#if defined(_ALPHA_) && !defined(_AXP64_)
        LOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif
        MmSystemCacheWs.AllowWorkingSetAdjustment = TRUE;
#if defined(_ALPHA_) && !defined(_AXP64_)
        UNLOCK_EXPANSION_IF_ALPHA (OldIrql);
#endif
    }
    UNLOCK_SYSTEM_WS (PreviousIrql);
}


VOID
MiCheckSessionPoolAllocations(
    VOID
    )

/*++

Routine Description:

    Ensure that the current session has no pool allocations since it is about
    to exit.  All session allocations must be freed prior to session exit.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    PMMPTE StartPde;
    PMMPTE EndPde;
    PMMPTE PointerPte;
    PVOID VirtualAddress;

    PAGED_CODE();

    if (MmSessionSpace->NonPagedPoolBytes || MmSessionSpace->PagedPoolBytes) {

        //
        // All page tables for this session's paged pool must be freed by now.
        // Being here means they aren't - this is fatal.  Force in any valid
        // pages so that a debugger can show who the guilty party is.
        //

        StartPde = MiGetPdeAddress (MmSessionSpace->PagedPoolStart);
        EndPde = MiGetPdeAddress (MmSessionSpace->PagedPoolEnd);

        while (StartPde <= EndPde) {

            if (StartPde->u.Long != 0 && StartPde->u.Long != MM_KERNEL_NOACCESS_PTE) {
                //
                // Hunt through the page table page for valid pages and force
                // them in.  Note this also forces in the page table page if
                // it is not already.
                //

                PointerPte = MiGetVirtualAddressMappedByPte (StartPde);

                for (i = 0; i < PTE_PER_PAGE; i += 1) {
                    if (PointerPte->u.Long != 0 && PointerPte->u.Long != MM_KERNEL_NOACCESS_PTE) {
                        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                        *(volatile BOOLEAN *)VirtualAddress = *(volatile BOOLEAN *)VirtualAddress;

#if DBG
                        DbgPrint("MiCheckSessionPoolAllocations: Address %p still valid\n",
                            VirtualAddress);
#endif
                    }
                    PointerPte += 1;
                }

            }

            StartPde += 1;
        }

#if DBG
        DbgPrint ("MiCheckSessionPoolAllocations: This exiting session (ID %d) is leaking pool !\n",  MmSessionSpace->SessionId);

        DbgPrint ("This means win32k.sys, rdpdd.sys, atmfd.sys or a video/font driver is broken\n");

        DbgPrint ("%d nonpaged allocation leaks for %d bytes and %d paged allocation leaks for %d bytes\n",
            MmSessionSpace->NonPagedPoolAllocations,
            MmSessionSpace->NonPagedPoolBytes,
            MmSessionSpace->PagedPoolAllocations,
            MmSessionSpace->PagedPoolBytes);
#endif

        KeBugCheckEx (SESSION_HAS_VALID_POOL_ON_EXIT,
                      (ULONG_PTR)MmSessionSpace->SessionId,
                      MmSessionSpace->PagedPoolBytes,
                      MmSessionSpace->NonPagedPoolBytes,
#if defined (_WIN64)
                      (MmSessionSpace->NonPagedPoolAllocations << 32) |
                        (MmSessionSpace->PagedPoolAllocations)
#else
                      (MmSessionSpace->NonPagedPoolAllocations << 16) |
                        (MmSessionSpace->PagedPoolAllocations)
#endif
                    );
    }

    ASSERT (MmSessionSpace->NonPagedPoolAllocations == 0);
    ASSERT (MmSessionSpace->PagedPoolAllocations == 0);
}


NTSTATUS
MiInitializeSessionPool(
    VOID
    )

/*++

Routine Description:

    Initialize the current session's pool structure.

Arguments:

    None.

Return Value:

    Status of the pool initialization.

Environment:

    Kernel mode.

--*/

{
    ULONG Index;
    MMPTE TempPte;
    PMMPTE PointerPde, PointerPte;
    PFN_NUMBER PageFrameIndex;
    PPOOL_DESCRIPTOR PoolDescriptor;
    PMM_SESSION_SPACE SessionGlobal;
    PMM_PAGED_POOL_INFO PagedPoolInfo;
    KIRQL OldIrql;
    PMMPFN Pfn1;
    MMPTE PreviousPte;
#if DBG
    PMMPTE StartPde, EndPde;
#endif

    SessionGlobal = SESSION_GLOBAL(MmSessionSpace);

    ExInitializeFastMutex (&SessionGlobal->PagedPoolMutex);

    PoolDescriptor = &MmSessionSpace->PagedPool;

    ExpInitializePoolDescriptor (PoolDescriptor,
                                 PagedPoolSession,
                                 0,
                                 0,
                                 &SessionGlobal->PagedPoolMutex);

    MmSessionSpace->PagedPoolStart = (PVOID)MI_SESSION_POOL;

    MmSessionSpace->PagedPoolEnd = (PVOID)((MI_SESSION_POOL + MI_SESSION_POOL_SIZE)-1);

    PagedPoolInfo = &MmSessionSpace->PagedPoolInfo;
    PagedPoolInfo->PagedPoolCommit = 0;
    PagedPoolInfo->PagedPoolHint = 0;
    PagedPoolInfo->AllocatedPagedPool = 0;

    //
    // Build the page table page for paged pool.
    //

    PointerPde = MiGetPdeAddress (MmSessionSpace->PagedPoolStart);
    MmSessionSpace->PagedPoolBasePde = PointerPde;

    PointerPte = MiGetPteAddress (MmSessionSpace->PagedPoolStart);

    PagedPoolInfo->FirstPteForPagedPool = PointerPte;
    PagedPoolInfo->LastPteForPagedPool = MiGetPteAddress (MmSessionSpace->PagedPoolEnd);

#if DBG
    //
    // Session pool better be unused.
    //

    StartPde = MiGetPdeAddress (MmSessionSpace->PagedPoolStart);
    EndPde = MiGetPdeAddress (MmSessionSpace->PagedPoolEnd);

    while (StartPde <= EndPde) {
        ASSERT (StartPde->u.Long == 0);
        StartPde += 1;
    }
#endif

    //
    // Mark all PDEs as empty.
    //

    MiFillMemoryPte (PointerPde,
                     sizeof(MMPTE) *
                         (1 + MiGetPdeAddress (MmSessionSpace->PagedPoolEnd) - PointerPde),
                     ZeroKernelPte.u.Long);

    if (MiChargeCommitment (1, NULL) == FALSE) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_COMMIT);
        return STATUS_NO_MEMORY;
    }

    MM_TRACK_COMMIT (MM_DBG_COMMIT_SESSION_POOL_PAGE_TABLES, 1);

    TempPte = ValidKernelPdeLocal;

    LOCK_PFN (OldIrql);

    if (MmAvailablePages <= 1) {
        UNLOCK_PFN (OldIrql);
        MiReturnCommitment (1);
        MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_SESSION_POOL_PAGE_TABLES, 1);
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_RESIDENT);
        return STATUS_NO_MEMORY;
    }

    MmResidentAvailablePages -= 1;
    MM_BUMP_COUNTER(42, 1);
    MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_ALLOC, 1);

    MiEnsureAvailablePageOrWait (NULL, NULL);

    //
    // Allocate and map in the initial page table page for session pool.
    //

    PageFrameIndex = MiRemoveAnyPage (MI_GET_PAGE_COLOR_FROM_PTE (PointerPde));
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    MI_WRITE_VALID_PTE (PointerPde, TempPte);

    MiInitializePfnForOtherProcess (PageFrameIndex,
                                    PointerPde,
                                    MmSessionSpace->SessionPageDirectoryIndex);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    //
    // This page will be locked into working set and assigned an index when
    // the working set is set up on return.
    //

    ASSERT (Pfn1->u1.WsIndex == 0);

    UNLOCK_PFN (OldIrql);

    KeFillEntryTb ((PHARDWARE_PTE) PointerPde, PointerPte, FALSE);

#if !defined (_WIN64)

    Index = MiGetPdeSessionIndex (MmSessionSpace->PagedPoolStart);

    ASSERT (MmSessionSpace->PageTables[Index].u.Long == 0);
    MmSessionSpace->PageTables[Index] = TempPte;

#endif

    MmSessionSpace->NonPagablePages += 1;
    MmSessionSpace->CommittedPages += 1;

    MiFillMemoryPte (PointerPte, PAGE_SIZE, MM_KERNEL_NOACCESS_PTE);

    PagedPoolInfo->NextPdeForPagedPoolExpansion = PointerPde + 1;

    //
    // Initialize the bitmaps.
    //

    MiCreateBitMap (&PagedPoolInfo->PagedPoolAllocationMap,
                    MI_SESSION_POOL_SIZE >> PAGE_SHIFT,
                    NonPagedPool);

    if (PagedPoolInfo->PagedPoolAllocationMap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        goto Failure;
    }

    //
    // We start with all pages in the virtual address space as "busy", and
    // clear bits to make pages available as we dynamically expand the pool.
    //

    RtlSetAllBits( PagedPoolInfo->PagedPoolAllocationMap );

    //
    // Indicate first page worth of PTEs are available.
    //

    RtlClearBits (PagedPoolInfo->PagedPoolAllocationMap, 0, PTE_PER_PAGE);

    //
    // Create the end of allocation range bitmap.
    //

    MiCreateBitMap (&PagedPoolInfo->EndOfPagedPoolBitmap,
                    MI_SESSION_POOL_SIZE >> PAGE_SHIFT,
                    NonPagedPool);

    if (PagedPoolInfo->EndOfPagedPoolBitmap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        goto Failure;
    }

    RtlClearAllBits (PagedPoolInfo->EndOfPagedPoolBitmap);

    //
    // Create the large session allocation bitmap.
    //

    MiCreateBitMap (&PagedPoolInfo->PagedPoolLargeSessionAllocationMap,
                    MI_SESSION_POOL_SIZE >> PAGE_SHIFT,
                    NonPagedPool);

    if (PagedPoolInfo->PagedPoolLargeSessionAllocationMap == NULL) {
        MM_BUMP_SESSION_FAILURES (MM_SESSION_FAILURE_NO_NONPAGED_POOL);
        goto Failure;
    }

    RtlClearAllBits (PagedPoolInfo->PagedPoolLargeSessionAllocationMap);

    return STATUS_SUCCESS;

Failure:

    MiFreeSessionPoolBitMaps ();

    LOCK_PFN (OldIrql);

    ASSERT (MmSessionSpace->SessionPageDirectoryIndex == Pfn1->PteFrame);
    ASSERT (Pfn1->u2.ShareCount == 1);
    MiDecrementShareAndValidCount (Pfn1->PteFrame);
    MI_SET_PFN_DELETED (Pfn1);
    MiDecrementShareCountOnly (PageFrameIndex);

    MI_FLUSH_SINGLE_SESSION_TB (MiGetPteAddress(PointerPde),
                                TRUE,
                                TRUE,
                                (PHARDWARE_PTE)PointerPde,
                                ZeroKernelPte.u.Flush,
                                PreviousPte);

    MmSessionSpace->NonPagablePages -= 1;
    MmSessionSpace->CommittedPages -= 1;

    MmResidentAvailablePages += 1;
    MM_BUMP_COUNTER(51, 1);
    MM_BUMP_SESS_COUNTER(MM_DBG_SESSION_PAGEDPOOL_PAGETABLE_FREE_FAIL1, 1);

    UNLOCK_PFN (OldIrql);

    MiReturnCommitment (1);

    MM_TRACK_COMMIT (MM_DBG_COMMIT_RETURN_PAGED_POOL_PAGES, 1);

    return STATUS_NO_MEMORY;
}


VOID
MiFreeSessionPoolBitMaps(
    VOID
    )

/*++

Routine Description:

    Free the current session's pool bitmap structures.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PAGED_CODE();

    if (MmSessionSpace->PagedPoolInfo.PagedPoolAllocationMap ) {
        ExFreePool (MmSessionSpace->PagedPoolInfo.PagedPoolAllocationMap);
        MmSessionSpace->PagedPoolInfo.PagedPoolAllocationMap = NULL;
    }

    if (MmSessionSpace->PagedPoolInfo.EndOfPagedPoolBitmap ) {
        ExFreePool (MmSessionSpace->PagedPoolInfo.EndOfPagedPoolBitmap);
        MmSessionSpace->PagedPoolInfo.EndOfPagedPoolBitmap = NULL;
    }

    if (MmSessionSpace->PagedPoolInfo.PagedPoolLargeSessionAllocationMap) {
        ExFreePool (MmSessionSpace->PagedPoolInfo.PagedPoolLargeSessionAllocationMap);
        MmSessionSpace->PagedPoolInfo.PagedPoolLargeSessionAllocationMap = NULL;
    }
}

#if DBG

#define MI_LOG_CONTIGUOUS  100

typedef struct _MI_CONTIGUOUS_ALLOCATORS {
    PVOID BaseAddress;
    SIZE_T NumberOfBytes;
    PVOID CallingAddress;
} MI_CONTIGUOUS_ALLOCATORS, *PMI_CONTIGUOUS_ALLOCATORS;

ULONG MiContiguousIndex;
MI_CONTIGUOUS_ALLOCATORS MiContiguousAllocators[MI_LOG_CONTIGUOUS];

VOID
MiInsertContiguousTag (
    IN PVOID BaseAddress,
    IN SIZE_T NumberOfBytes,
    IN PVOID CallingAddress
    )
{
    KIRQL OldIrql;

#if !DBG
    if ((NtGlobalFlag & FLG_POOL_ENABLE_TAGGING) == 0) {
        return;
    }
#endif

    OldIrql = ExLockPool (NonPagedPool);

    if (MiContiguousIndex >= MI_LOG_CONTIGUOUS) {
        MiContiguousIndex = 0;
    }

    MiContiguousAllocators[MiContiguousIndex].BaseAddress = BaseAddress;
    MiContiguousAllocators[MiContiguousIndex].NumberOfBytes = NumberOfBytes;
    MiContiguousAllocators[MiContiguousIndex].CallingAddress = CallingAddress;

    MiContiguousIndex += 1;

    ExUnlockPool (NonPagedPool, OldIrql);
}
#else
#define MiInsertContiguousTag(a, b, c)
#endif


PVOID
MiFindContiguousMemory (
    IN PFN_NUMBER LowestPfn,
    IN PFN_NUMBER HighestPfn,
    IN PFN_NUMBER BoundaryPfn,
    IN PFN_NUMBER SizeInPages,
    IN PVOID CallingAddress
    )

/*++

Routine Description:

    This function searches nonpaged pool and the free, zeroed,
    and standby lists for contiguous pages that satisfy the
    request.

Arguments:

    LowestPfn - Supplies the lowest acceptable physical page number.

    HighestPfn - Supplies the highest acceptable physical page number.

    BoundaryPfn - Supplies the page frame number multiple the allocation must
                  not cross.  0 indicates it can cross any boundary.

    SizeInPages - Supplies the number of pages to allocate.

    CallingAddress - Supplies the calling address of the allocator.

Return Value:

    NULL - a contiguous range could not be found to satisfy the request.

    NON-NULL - Returns a pointer (virtual address in the nonpaged portion
               of the system) to the allocated physically contiguous
               memory.

Environment:

    Kernel mode, IRQL of APC_LEVEL or below.

--*/
{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PVOID BaseAddress;
    PVOID BaseAddress2;
    KIRQL OldIrql;
    KIRQL OldIrql2;
    PMMFREE_POOL_ENTRY FreePageInfo;
    PLIST_ENTRY Entry;
    ULONG start;
    ULONG Index;
    PFN_NUMBER count;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_NUMBER found;
    PFN_NUMBER BoundaryMask;
    MMPTE TempPte;
    ULONG PageColor;
    ULONG AllocationPosition;
    PVOID Va;
    LOGICAL AddressIsPhysical;
    PFN_NUMBER SpanInPages;
    PFN_NUMBER SpanInPages2;

    PAGED_CODE ();

    BaseAddress = NULL;

    BoundaryMask = ~(BoundaryPfn - 1);

    //
    // A suitable pool page was not allocated via the pool allocator.
    // Grab the pool lock and manually search for a page which meets
    // the requirements.
    //

    MmLockPagableSectionByHandle (ExPageLockHandle);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    OldIrql = ExLockPool (NonPagedPool);

    //
    // Trace through the page allocator's pool headers for a page which
    // meets the requirements.
    //

    //
    // NonPaged pool is linked together through the pages themselves.
    //

    Index = (ULONG)(SizeInPages - 1);

    if (Index >= MI_MAX_FREE_LIST_HEADS) {
        Index = MI_MAX_FREE_LIST_HEADS - 1;
    }

    while (Index < MI_MAX_FREE_LIST_HEADS) {

        Entry = MmNonPagedPoolFreeListHead[Index].Flink;
    
        while (Entry != &MmNonPagedPoolFreeListHead[Index]) {
    
            if (MmProtectFreedNonPagedPool == TRUE) {
                MiUnProtectFreeNonPagedPool ((PVOID)Entry, 0);
            }
    
            //
            // The list is not empty, see if this one meets the physical
            // requirements.
            //
    
            FreePageInfo = CONTAINING_RECORD(Entry,
                                             MMFREE_POOL_ENTRY,
                                             List);
    
            ASSERT (FreePageInfo->Signature == MM_FREE_POOL_SIGNATURE);
            if (FreePageInfo->Size >= SizeInPages) {
    
                //
                // This entry has sufficient space, check to see if the
                // pages meet the physical requirements.
                //
    
                Va = MiCheckForContiguousMemory (PAGE_ALIGN(Entry),
                                                 FreePageInfo->Size,
                                                 SizeInPages,
                                                 LowestPfn,
                                                 HighestPfn,
                                                 BoundaryPfn);
     
                if (Va != NULL) {

                    //
                    // These pages meet the requirements.  The returned
                    // address may butt up on the end, the front or be
                    // somewhere in the middle.  Split the Entry based
                    // on which case it is.
                    //

                    Entry = PAGE_ALIGN(Entry);
                    if (MmProtectFreedNonPagedPool == FALSE) {
                        RemoveEntryList (&FreePageInfo->List);
                    }
                    else {
                        MiProtectedPoolRemoveEntryList (&FreePageInfo->List);
                    }
    
                    //
                    // Adjust the number of free pages remaining in the pool.
                    // The TotalBigPages calculation appears incorrect for the
                    // case where we're splitting a block, but it's done this
                    // way because ExFreePool corrects it when we free the
                    // fragment block below.  Likewise for
                    // MmAllocatedNonPagedPool and MmNumberOfFreeNonPagedPool
                    // which is corrected by MiFreePoolPages for the fragment.
                    //
    
                    NonPagedPoolDescriptor.TotalBigPages += (ULONG)FreePageInfo->Size;
                    MmAllocatedNonPagedPool += FreePageInfo->Size;
                    MmNumberOfFreeNonPagedPool -= FreePageInfo->Size;
    
                    ASSERT ((LONG)MmNumberOfFreeNonPagedPool >= 0);
    
                    if (Va == Entry) {

                        //
                        // Butted against the front.
                        //

                        AllocationPosition = 0;
                    }
                    else if (((PCHAR)Va + (SizeInPages << PAGE_SHIFT)) == ((PCHAR)Entry + (FreePageInfo->Size << PAGE_SHIFT))) {

                        //
                        // Butted against the end.
                        //

                        AllocationPosition = 2;
                    }
                    else {

                        //
                        // Somewhere in the middle.
                        //

                        AllocationPosition = 1;
                    }

                    //
                    // Pages are being removed from the front of
                    // the list entry and the whole list entry
                    // will be removed and then the remainder inserted.
                    //
    
                    //
                    // Mark start and end for the block at the top of the
                    // list.
                    //
    
                    if (MI_IS_PHYSICAL_ADDRESS(Va)) {
    
                        //
                        // On certain architectures, virtual addresses
                        // may be physical and hence have no corresponding PTE.
                        //
    
                        AddressIsPhysical = TRUE;
                        Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (Va));
                    } else {
                        AddressIsPhysical = FALSE;
                        PointerPte = MiGetPteAddress(Va);
                        ASSERT (PointerPte->u.Hard.Valid == 1);
                        Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                    }
    
                    ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
                    ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                    ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                    Pfn1->u3.e1.StartOfAllocation = 1;
    
                    //
                    // Calculate the ending PFN address, note that since
                    // these pages are contiguous, just add to the PFN.
                    //
    
                    Pfn1 += SizeInPages - 1;
                    ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
                    ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                    ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                    Pfn1->u3.e1.EndOfAllocation = 1;
    
                    if (SizeInPages == FreePageInfo->Size) {
    
                        //
                        // Unlock the pool and return.
                        //
                        BaseAddress = (PVOID)Va;
                        goto Done;
                    }
    
                    BaseAddress = NULL;

                    if (AllocationPosition != 2) {

                        //
                        // The end piece needs to be freed as the removal
                        // came from the front or the middle.
                        //

                        BaseAddress = (PVOID)((PCHAR)Va + (SizeInPages << PAGE_SHIFT));
                        SpanInPages = FreePageInfo->Size - SizeInPages -
                            (((ULONG_PTR)Va - (ULONG_PTR)Entry) >> PAGE_SHIFT);
    
                        //
                        // Mark start and end of the allocation in the PFN database.
                        //
        
                        if (AddressIsPhysical == TRUE) {
        
                            //
                            // On certain architectures, virtual addresses
                            // may be physical and hence have no corresponding PTE.
                            //
        
                            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress));
                        } else {
                            PointerPte = MiGetPteAddress(BaseAddress);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
        
                        ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
                        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                        ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                        Pfn1->u3.e1.StartOfAllocation = 1;
        
                        //
                        // Calculate the ending PTE's address, can't depend on
                        // these pages being physically contiguous.
                        //
        
                        if (AddressIsPhysical == TRUE) {
                            Pfn1 += (SpanInPages - 1);
                        } else {
                            PointerPte += (SpanInPages - 1);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
                        ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                        Pfn1->u3.e1.EndOfAllocation = 1;
        
                        ASSERT (((ULONG_PTR)BaseAddress & (PAGE_SIZE -1)) == 0);
        
                        SpanInPages2 = SpanInPages;
                    }
        
                    BaseAddress2 = BaseAddress;
                    BaseAddress = NULL;

                    if (AllocationPosition != 0) {

                        //
                        // The front piece needs to be freed as the removal
                        // came from the middle or the end.
                        //

                        BaseAddress = (PVOID)Entry;

                        SpanInPages = ((ULONG_PTR)Va - (ULONG_PTR)Entry) >> PAGE_SHIFT;
    
                        //
                        // Mark start and end of the allocation in the PFN database.
                        //
        
                        if (AddressIsPhysical == TRUE) {
        
                            //
                            // On certain architectures, virtual addresses
                            // may be physical and hence have no corresponding PTE.
                            //
        
                            Pfn1 = MI_PFN_ELEMENT (MI_CONVERT_PHYSICAL_TO_PFN (BaseAddress));
                        } else {
                            PointerPte = MiGetPteAddress(BaseAddress);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
        
                        ASSERT (Pfn1->u3.e1.VerifierAllocation == 0);
                        ASSERT (Pfn1->u3.e1.LargeSessionAllocation == 0);
                        ASSERT (Pfn1->u3.e1.StartOfAllocation == 0);
                        Pfn1->u3.e1.StartOfAllocation = 1;
        
                        //
                        // Calculate the ending PTE's address, can't depend on
                        // these pages being physically contiguous.
                        //
        
                        if (AddressIsPhysical == TRUE) {
                            Pfn1 += (SpanInPages - 1);
                        } else {
                            PointerPte += (SpanInPages - 1);
                            ASSERT (PointerPte->u.Hard.Valid == 1);
                            Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);
                        }
                        ASSERT (Pfn1->u3.e1.EndOfAllocation == 0);
                        Pfn1->u3.e1.EndOfAllocation = 1;
        
                        ASSERT (((ULONG_PTR)BaseAddress & (PAGE_SIZE -1)) == 0);
                    }
        
                    //
                    // Unlock the pool.
                    //
    
                    ExUnlockPool (NonPagedPool, OldIrql);
    
                    ExReleaseFastMutex (&MmDynamicMemoryMutex);

                    //
                    // Free the split entry at BaseAddress back into the pool.
                    // Note that we have overcharged the pool - the entire free
                    // chunk has been billed.  Here we return the piece we
                    // didn't use and correct the momentary overbilling.
                    //
                    // The start and end allocation bits of this split entry
                    // which we just set up enable ExFreePool and his callees
                    // to correctly adjust the billing.
                    //
    
                    if (BaseAddress) {
                        ExInsertPoolTag ('tnoC',
                                         BaseAddress,
                                         SpanInPages << PAGE_SHIFT,
                                         NonPagedPool);
                        ExFreePool (BaseAddress);
                    }
                    if (BaseAddress2) {
                        ExInsertPoolTag ('tnoC',
                                         BaseAddress2,
                                         SpanInPages2 << PAGE_SHIFT,
                                         NonPagedPool);
                        ExFreePool (BaseAddress2);
                    }
                    BaseAddress = Va;
                    goto Done1;
                }
            }
            Entry = FreePageInfo->List.Flink;
            if (MmProtectFreedNonPagedPool == TRUE) {
                MiProtectFreeNonPagedPool ((PVOID)FreePageInfo,
                                           (ULONG)FreePageInfo->Size);
            }
        }
        Index += 1;
    }

    //
    // No entry was found in free nonpaged pool that meets the requirements.
    // Search the PFN database for pages that meet the requirements.
    //

    start = 0;
    do {

        count = MmPhysicalMemoryBlock->Run[start].PageCount;
        Page = MmPhysicalMemoryBlock->Run[start].BasePage;

        //
        // Close the gaps, then examine the range for a fit.
        //

        LastPage = Page + count; 

        if (LastPage - 1 > HighestPfn) {
            LastPage = HighestPfn + 1;
        }
    
        if (Page < LowestPfn) {
            Page = LowestPfn;
        }

        if ((count != 0) && (Page + SizeInPages <= LastPage)) {
    
            //
            // A fit may be possible in this run, check whether the pages
            // are on the right list.
            //

            found = 0;

            Pfn1 = MI_PFN_ELEMENT (Page);
            LOCK_PFN2 (OldIrql2);
            do {

                if ((Pfn1->u3.e1.PageLocation == ZeroedPageList) ||
                    (Pfn1->u3.e1.PageLocation == FreePageList) ||
                    (Pfn1->u3.e1.PageLocation == StandbyPageList)) {

                    if ((Pfn1->u1.Flink != 0) &&
                        (Pfn1->u2.Blink != 0) &&
                        (Pfn1->u3.e2.ReferenceCount == 0)) {
    
                        //
                        // Before starting a new run, ensure that it
                        // can satisfy the boundary requirements (if any).
                        //
                        
                        if ((found == 0) && (BoundaryPfn != 0)) {
                            if (((Page ^ (Page + SizeInPages - 1)) & BoundaryMask) != 0) {
                                //
                                // This run's physical address does not meet the
                                // requirements.
                                //

                                goto NextPage;
                            }
                        }

                        found += 1;
                        if (found == SizeInPages) {

                            //
                            // A match has been found, remove these
                            // pages, add them to the free pool and
                            // return.
                            //

                            Page = 1 + Page - found;

                            //
                            // Try to find system PTES to expand the pool into.
                            //

                            PointerPte = MiReserveSystemPtes ((ULONG)SizeInPages,
                                                              NonPagedPoolExpansion,
                                                              0,
                                                              0,
                                                              FALSE);

                            if (PointerPte == NULL) {
                                UNLOCK_PFN2 (OldIrql2);
                                goto Done;
                            }

                            MmResidentAvailablePages -= SizeInPages;
                            MM_BUMP_COUNTER(3, SizeInPages);
                            MiChargeCommitmentCantExpand (SizeInPages, TRUE);
                            MM_TRACK_COMMIT (MM_DBG_COMMIT_CONTIGUOUS_PAGES, SizeInPages);
                            BaseAddress = MiGetVirtualAddressMappedByPte (PointerPte);
                            PageColor = MI_GET_PAGE_COLOR_FROM_VA(BaseAddress);
                            TempPte = ValidKernelPte;
                            MmAllocatedNonPagedPool += SizeInPages;
                            NonPagedPoolDescriptor.TotalBigPages += (ULONG)SizeInPages;
                            Pfn1 = MI_PFN_ELEMENT (Page - 1);

                            do {
                                Pfn1 += 1;
                                if (Pfn1->u3.e1.PageLocation == StandbyPageList) {
                                    MiUnlinkPageFromList (Pfn1);
                                    MiRestoreTransitionPte (Page);
                                } else {
                                    MiUnlinkFreeOrZeroedPage (Page);
                                }

                                MI_CHECK_PAGE_ALIGNMENT(Page,
                                                        PageColor & MM_COLOR_MASK);
                                Pfn1->u3.e1.PageColor = PageColor & MM_COLOR_MASK;
                                PageColor += 1;
                                TempPte.u.Hard.PageFrameNumber = Page;
                                MI_WRITE_VALID_PTE (PointerPte, TempPte);

                                Pfn1->u3.e2.ReferenceCount = 1;
                                Pfn1->u2.ShareCount = 1;
                                Pfn1->PteAddress = PointerPte;
                                Pfn1->OriginalPte.u.Long = MM_DEMAND_ZERO_WRITE_PTE;
                                Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress(PointerPte));
                                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                                Pfn1->u3.e1.VerifierAllocation = 0;
                                Pfn1->u3.e1.LargeSessionAllocation = 0;

                                if (found == SizeInPages) {
                                    Pfn1->u3.e1.StartOfAllocation = 1;
                                }
                                PointerPte += 1;
                                Page += 1;
                                found -= 1;
                            } while (found);

                            Pfn1->u3.e1.EndOfAllocation = 1;
                            UNLOCK_PFN2 (OldIrql2);
                            goto Done;
                        }
                    } else {
                        found = 0;
                    }
                } else {
                    found = 0;
                }
NextPage:
                Page += 1;
                Pfn1 += 1;
            } while (Page < LastPage);
            UNLOCK_PFN2 (OldIrql2);
        }
        start += 1;
    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

Done:

    ExUnlockPool (NonPagedPool, OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

Done1:

    MmUnlockPagableImageSection (ExPageLockHandle);

    if (BaseAddress) {

        MiInsertContiguousTag (BaseAddress,
                               SizeInPages << PAGE_SHIFT,
                               CallingAddress);

        ExInsertPoolTag ('tnoC',
                         BaseAddress,
                         SizeInPages << PAGE_SHIFT,
                         NonPagedPool);
    }

    return BaseAddress;
}

LOGICAL
MmIsHydraAddress (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function returns TRUE if a Hydra address is specified.
    FALSE is returned if not.

Arguments:

    VirtualAddress - Supplies the address in question.

Return Value:

    See above.

Environment:

    Kernel mode.  Note this routine is present and nonpaged for both Hydra
    and non-Hydra systems.

--*/

{
    return MI_IS_SESSION_ADDRESS (VirtualAddress);
}

LOGICAL
MmIsSpecialPoolAddressFree (
    IN PVOID VirtualAddress
    )

/*++

Routine Description:

    This function returns TRUE if a special pool address has been freed.
    FALSE is returned if it is inuse (ie: the caller overran).

Arguments:

    VirtualAddress - Supplies the special pool address in question.

Return Value:

    See above.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;

    //
    // Caller must check that the address in in special pool.
    //

    ASSERT (VirtualAddress >= MmSpecialPoolStart && VirtualAddress < MmSpecialPoolEnd);

    PointerPte = MiGetPteAddress(VirtualAddress);

    //
    // Take advantage of the fact that adjacent PTEs have the paged/nonpaged
    // bits set when in use and these bits are cleared on free.  Note also
    // that freed pages get their PTEs chained together through PageFileHigh.
    //

    if ((PointerPte->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE) ||
        (PointerPte->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE)) {
            return FALSE;
    }

    return TRUE;
}

LOGICAL
MmProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    )

/*++

Routine Description:

    This function protects a special pool allocation.

Arguments:

    VirtualAddress - Supplies the special pool address to protect.

    NewProtect - Supplies the protection to set the pages to (PAGE_XX).

Return Value:

    TRUE if the protection was successfully applied, FALSE if not.

Environment:

    Kernel mode, IRQL at APC_LEVEL or below for pagable pool, DISPATCH or
    below for nonpagable pool.

    Note that setting an allocation to NO_ACCESS implies that an accessible
    protection must be applied by the caller prior to this allocation being
    freed.

    Note this is a nonpagable wrapper so that machines without special pool
    can still support code attempting to protect special pool at
    DISPATCH_LEVEL.

--*/

{
    if (MiSpecialPoolPtes == 0) {

        //
        // The special pool allocation code was never initialized.
        //

        return (ULONG)-1;
    }

    return MiProtectSpecialPool (VirtualAddress, NewProtect);
}

LOGICAL
MiProtectSpecialPool (
    IN PVOID VirtualAddress,
    IN ULONG NewProtect
    )

/*++

Routine Description:

    This function protects a special pool allocation.

Arguments:

    VirtualAddress - Supplies the special pool address to protect.

    NewProtect - Supplies the protection to set the pages to (PAGE_XX).

Return Value:

    TRUE if the protection was successfully applied, FALSE if not.

Environment:

    Kernel mode, IRQL at APC_LEVEL or below for pagable pool, DISPATCH or
    below for nonpagable pool.

    Note that setting an allocation to NO_ACCESS implies that an accessible
    protection must be applied by the caller prior to this allocation being
    freed.

--*/

{
    KIRQL OldIrql;
    KIRQL OldIrql2;
    MMPTE PteContents;
    MMPTE NewPteContents;
    MMPTE PreviousPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    ULONG ProtectionMask;
    WSLE_NUMBER WsIndex;
    LOGICAL SystemWsLocked;

    if ((VirtualAddress < MmSpecialPoolStart) || (VirtualAddress >= MmSpecialPoolEnd)) {
        return (ULONG)-1;
    }

    try {
        ProtectionMask = MiMakeProtectionMask (NewProtect);
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return (ULONG)-1;
    }

    SystemWsLocked = FALSE;

    PointerPte = MiGetPteAddress (VirtualAddress);

    if ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE) {
        LOCK_SYSTEM_WS (OldIrql);
        SystemWsLocked = TRUE;
    }

    PteContents = *PointerPte;

    if (ProtectionMask == MM_NOACCESS) {

        if ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_PAGABLE) {
retry1:
            ASSERT (SystemWsLocked == TRUE);
            if (PteContents.u.Hard.Valid == 1) {

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
                WsIndex = Pfn1->u1.WsIndex;
                ASSERT (WsIndex != 0);
                Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                MiRemovePageFromWorkingSet (PointerPte,
                                            Pfn1,
                                            &MmSystemCacheWs);
            }
            else if (PteContents.u.Soft.Transition == 1) {

                LOCK_PFN2 (OldIrql2);

                PteContents = *(volatile MMPTE *)PointerPte;

                if (PteContents.u.Soft.Transition == 0) {
                    UNLOCK_PFN2 (OldIrql2);
                    goto retry1;
                }

                Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
                Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
                PointerPte->u.Soft.Protection = ProtectionMask;
                UNLOCK_PFN2(OldIrql2);
            }
            else {
    
                //
                // Must be page file space or demand zero.
                //
    
                PointerPte->u.Soft.Protection = ProtectionMask;
            }
            ASSERT (SystemWsLocked == TRUE);
            UNLOCK_SYSTEM_WS (OldIrql);
        }
        else {

            ASSERT (SystemWsLocked == FALSE);

            //
            // Make it no access regardless of its previous protection state.
            // Note that the page frame number is preserved.
            //

            PteContents.u.Hard.Valid = 0;
            PteContents.u.Soft.Prototype = 0;
            PteContents.u.Soft.Protection = MM_NOACCESS;
    
            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            LOCK_PFN2 (OldIrql2);

            Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

            PreviousPte.u.Flush = KeFlushSingleTb (VirtualAddress,
                                                   TRUE,
                                                   TRUE,
                                                   (PHARDWARE_PTE)PointerPte,
                                                   PteContents.u.Flush);

            MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn1);

            UNLOCK_PFN2(OldIrql2);
        }

        return TRUE;
    }

    //
    // No guard pages, noncached pages or copy-on-write for special pool.
    //

    if ((ProtectionMask >= MM_NOCACHE) || (ProtectionMask == MM_WRITECOPY) || (ProtectionMask == MM_EXECUTE_WRITECOPY)) {
        if (SystemWsLocked == TRUE) {
            UNLOCK_SYSTEM_WS (OldIrql);
        }
        return FALSE;
    }

    //
    // Set accessible permissions - the page may already be protected or not.
    //

    if ((PointerPte + 1)->u.Soft.PageFileHigh == MI_SPECIAL_POOL_PTE_NONPAGABLE) {

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

        MI_MAKE_VALID_PTE (NewPteContents,
                           PteContents.u.Hard.PageFrameNumber,
                           ProtectionMask,
                           PointerPte);

        MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, NewPteContents);

        ASSERT (SystemWsLocked == FALSE);
        return TRUE;
    }

retry2:

    ASSERT (SystemWsLocked == TRUE);

    if (PteContents.u.Hard.Valid == 1) {

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
        ASSERT (Pfn1->u1.WsIndex != 0);

        LOCK_PFN2 (OldIrql2);

        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;

        MI_MAKE_VALID_PTE (PteContents,
                           PteContents.u.Hard.PageFrameNumber,
                           ProtectionMask,
                           PointerPte);

        PreviousPte.u.Flush = KeFlushSingleTb (VirtualAddress,
                              TRUE,
                              TRUE,
                              (PHARDWARE_PTE)PointerPte,
                              PteContents.u.Flush);

        MI_CAPTURE_DIRTY_BIT_TO_PFN (&PreviousPte, Pfn1);

        UNLOCK_PFN2 (OldIrql2);
    }
    else if (PteContents.u.Soft.Transition == 1) {

        LOCK_PFN2 (OldIrql2);

        PteContents = *(volatile MMPTE *)PointerPte;

        if (PteContents.u.Soft.Transition == 0) {
            UNLOCK_PFN2 (OldIrql2);
            goto retry2;
        }

        Pfn1 = MI_PFN_ELEMENT (PteContents.u.Trans.PageFrameNumber);
        Pfn1->OriginalPte.u.Soft.Protection = ProtectionMask;
        PointerPte->u.Soft.Protection = ProtectionMask;
        UNLOCK_PFN2(OldIrql2);
    }
    else {

        //
        // Must be page file space or demand zero.
        //

        PointerPte->u.Soft.Protection = ProtectionMask;
    }

    UNLOCK_SYSTEM_WS (OldIrql);
    return TRUE;
}
