/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   sysptes.c

Abstract:

    This module contains the routines which reserve and release
    system wide PTEs reserved within the non paged portion of the
    system space.  These PTEs are used for mapping I/O devices
    and mapping kernel stacks for threads.

Author:

    Lou Perazzoli (loup) 6-Apr-1989

Revision History:

--*/

#include "mi.h"

VOID
MiFeedSysPtePool (
    IN ULONG Index
    );

PMMPTE
MiReserveSystemPtes2 (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
    IN ULONG Alignment,
    IN ULONG Offset,
    IN ULONG BugCheckOnFailure
    );

ULONG
MiGetSystemPteListCount (
    IN ULONG ListSize
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeSystemPtes)
#pragma alloc_text(INIT,MiAddSystemPtes)
#pragma alloc_text(MISYSPTE,MiReserveSystemPtes)
#pragma alloc_text(MISYSPTE,MiReserveSystemPtes2)
#pragma alloc_text(MISYSPTE,MiFeedSysPtePool)
#pragma alloc_text(MISYSPTE,MiReleaseSystemPtes)
#pragma alloc_text(MISYSPTE,MiGetSystemPteListCount)
#endif

#ifdef _MI_GUARD_PTE_
#define _MI_SYSPTE_DEBUG_ 1
#endif

#ifdef _MI_SYSPTE_DEBUG_
typedef struct _MMPTE_TRACKER {
    USHORT   NumberOfPtes;
    BOOLEAN  InUse;
    BOOLEAN  Reserved;
} MMPTE_TRACKER, *PMMPTE_TRACKER;

PMMPTE_TRACKER MiPteTracker;
#endif

ULONG MmTotalFreeSystemPtes[MaximumPtePoolTypes];
PMMPTE MmSystemPtesStart[MaximumPtePoolTypes];
PMMPTE MmSystemPtesEnd[MaximumPtePoolTypes];

#define MM_MIN_SYSPTE_FREE 500
#define MM_MAX_SYSPTE_FREE 3000

PMMPTE MmFlushPte1;

ULONG MmFlushCounter;

//
// PTEs are binned at sizes 1, 2, 4, 8, and 16.
//

#ifdef _ALPHA_

//
// Alpha has an 8k page size and stacks consume 9 pages (including guard page).
//

ULONG MmSysPteIndex[MM_SYS_PTE_TABLES_MAX] = {1,2,4,9,16};

UCHAR MmSysPteTables[17] = {0,0,1,2,2,3,3,3,3,3,4,4,4,4,4,4,4};

#else

ULONG MmSysPteIndex[MM_SYS_PTE_TABLES_MAX] = {1,2,4,8,16};

UCHAR MmSysPteTables[17] = {0,0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4};
#endif

MMPTE MmFreeSysPteListBySize [MM_SYS_PTE_TABLES_MAX];
PMMPTE MmLastSysPteListBySize [MM_SYS_PTE_TABLES_MAX];
ULONG MmSysPteListBySizeCount [MM_SYS_PTE_TABLES_MAX];
ULONG MmSysPteMinimumFree [MM_SYS_PTE_TABLES_MAX] = {100,50,30,20,20};

//
// Initial sizes for PTE lists.
//

#define MM_PTE_LIST_1  400
#define MM_PTE_LIST_2  100
#define MM_PTE_LIST_4   60
#define MM_PTE_LIST_8   50
#define MM_PTE_LIST_16  40

#define MM_PTE_TABLE_LIMIT 16

PMMPTE
MiReserveSystemPtes2 (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
    IN ULONG Alignment,
    IN ULONG Offset,
    IN ULONG BugCheckOnFailure
    );

VOID
MiFeedSysPtePool (
    IN ULONG Index
    );

VOID
MiDumpSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    );

ULONG
MiCountFreeSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    );

PVOID
MiGetHighestPteConsumer (
    OUT PULONG_PTR NumberOfPtes
    );

#ifdef _MI_SYSPTE_DEBUG_
ULONG MiPtesChecked[0x10];


VOID
MiValidateSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes
    )

/*++

Routine Description:

    This function validates the PTE range passed in, guaranteeing that it
    doesn't overlap with any free system PTEs.  This enables us to catch
    callers that are freeing one too many PTEs.

Arguments:

    StartingPte - Supplies the first PTE being freed.

    NumberOfPtes - Supplies the number of PTEs being freed.

Return Value:

    None.

Environment:

    Kernel mode, DISPATCH_LEVEL or below.

--*/

{
    KIRQL OldIrql;
    ULONG Index;
    PMMPTE EndingPte;
    PMMPTE PointerPte1;
    PMMPTE PointerFreedPte;
    ULONG PtesInThisBucket;
    ULONG j;

    ASSERT (NumberOfPtes != 0);

    EndingPte = StartingPte + NumberOfPtes - 1;

    MiLockSystemSpace(OldIrql);

    for (Index = 0; Index < MM_SYS_PTE_TABLES_MAX; Index += 1) {

        PointerPte1 = &MmFreeSysPteListBySize[Index];

        while (PointerPte1->u.List.NextEntry != MM_EMPTY_PTE_LIST) {

            MiPtesChecked[0] += 1;

            PointerPte1 = MmSystemPteBase + PointerPte1->u.List.NextEntry;

            PtesInThisBucket = MmSysPteIndex[Index];

            if (StartingPte >= PointerPte1 && StartingPte < PointerPte1 + PtesInThisBucket) {
                DbgPrint("MiValidateSystemPtes1: %x %x %x %x %x %x\n",
                    StartingPte,
                    EndingPte,
                    NumberOfPtes,
                    PointerPte1,
                    PtesInThisBucket,
                    Index);
                DbgBreakPoint ();
            }

            if (EndingPte >= PointerPte1 && EndingPte < PointerPte1 + PtesInThisBucket) {
                DbgPrint("MiValidateSystemPtes2: %x %x %x %x %x %x\n",
                    StartingPte,
                    EndingPte,
                    NumberOfPtes,
                    PointerPte1,
                    PtesInThisBucket,
                    Index);
                DbgBreakPoint ();
            }

            if (StartingPte < PointerPte1 && EndingPte >= PointerPte1 + PtesInThisBucket) {
                DbgPrint("MiValidateSystemPtes3: %x %x %x %x %x %x\n",
                    StartingPte,
                    EndingPte,
                    NumberOfPtes,
                    PointerPte1,
                    PtesInThisBucket,
                    Index);
                DbgBreakPoint ();
            }

            PointerFreedPte = PointerPte1;
            for (j = 0; j < MmSysPteIndex[Index]; j++) {
                ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                PointerFreedPte += 1;
                MiPtesChecked[1] += 1;
            }
        }
    }

    MiUnlockSystemSpace(OldIrql);
}


VOID
MiCheckPteAllocation (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN ULONG CallerId,
    IN ULONG Context1
    )

/*++

Routine Description:

    This function validates the PTE range passed in, guaranteeing that it
    is not already allocated.

Arguments:

    StartingPte - Supplies the first PTE being allocated.

    NumberOfPtes - Supplies the number of PTEs being allocated.

    CallerId - Supplies the caller's ID.

    Context1 - Opaque.

Return Value:

    None.

Environment:

    Kernel mode, MmSystemSpaceLock held.

--*/

{
    ULONG i;
    ULONG_PTR Index;
    PMMPTE BasePte;
    PMMPTE_TRACKER Tracker;

    //
    // Don't track nonpaged pool expansion.
    //

    if (StartingPte < MmSystemPtesStart[SystemPteSpace] ||
        StartingPte + NumberOfPtes > MmSystemPtesEnd[SystemPteSpace] + 1) {
            return;
    }

    if (MiPteTracker == NULL) {
            return;
    }

    Index = StartingPte - MmSystemPtesStart[SystemPteSpace];

    Tracker = &MiPteTracker[Index];

    for (i = 0; i < NumberOfPtes; i += 1) {

        if (Tracker->InUse == TRUE) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          7,
                          (ULONG_PTR)StartingPte,
                          NumberOfPtes,
                          CallerId);
        }

        Tracker += 1;
    }

    Tracker = &MiPteTracker[Index];

    ASSERT (NumberOfPtes < 0x10000);

    Tracker->NumberOfPtes = (USHORT)NumberOfPtes;

    for (i = 0; i < NumberOfPtes; i += 1) {
        Tracker->InUse = TRUE;
        Tracker += 1;
    }

    MiPtesChecked[0xE] += 1;
}


VOID
MiCheckPteRelease (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes
    )

/*++

Routine Description:

    This function validates the PTE range passed in, guaranteeing that it
    is not already free.

Arguments:

    StartingPte - Supplies the first PTE being allocated.

    NumberOfPtes - Supplies the number of PTEs being allocated.

Return Value:

    None.

Environment:

    Kernel mode, MmSystemSpaceLock held.

--*/

{
    ULONG i;
    ULONG_PTR Index;
    PMMPTE BasePte;
    PMMPTE_TRACKER Tracker;

    //
    // Don't track nonpaged pool expansion.
    //

    if (StartingPte < MmSystemPtesStart[SystemPteSpace] ||
        StartingPte + NumberOfPtes > MmSystemPtesEnd[SystemPteSpace] + 1) {
            return;
    }

    if (MiPteTracker == NULL) {
            return;
    }

    ASSERT (NumberOfPtes < 0x10000);

    Index = StartingPte - MmSystemPtesStart[SystemPteSpace];

    Tracker = &MiPteTracker[Index];

    if ((NumberOfPtes == 0) ||
        (Tracker->NumberOfPtes != NumberOfPtes)) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      8,
                      (ULONG_PTR)StartingPte,
                      NumberOfPtes,
                      Tracker->NumberOfPtes);
    }

    Tracker->NumberOfPtes = 0;

    for (i = 0; i < NumberOfPtes; i += 1) {

        if (Tracker->InUse == FALSE) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          9,
                          (ULONG_PTR)StartingPte,
                          NumberOfPtes,
                          i);
        }

        Tracker += 1;
    }

    Tracker = &MiPteTracker[Index];

    for (i = 0; i < NumberOfPtes; i += 1) {
        Tracker->InUse = FALSE;
        Tracker += 1;
    }

    MiPtesChecked[0xF] += 1;
}


VOID
MiRebuildPteTracker (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes
    )

/*++

Routine Description:

    This function resizes the PTE tracking data structures to account for
    system PTE table growth.

    Note that the original PTE tracker structure cannot be freed unless
    MiFreePoolPages is modified to deal with the boundary allocations being
    freed - since this never happens in a normal system, there are no bounds
    checks in place and it will coalesce past the ends of pool.

Arguments:

    StartingPte - Supplies the first PTE for the system PTE pool.

    NumberOfPtes - Supplies the number of PTEs in the system PTE pool.

Return Value:

    None.

Environment:

    Kernel mode, MmSystemSpaceLock held.

--*/

{
    KIRQL OldIrql;
    ULONG_PTR OldPtes;
    ULONG_PTR NewPtes;
    PMMPTE_TRACKER OldTracker;
    PMMPTE_TRACKER NewTracker;

    //
    // If we've already discovered a discontiguity don't coalesce now.
    //

    if (MiPteTracker == (PMMPTE_TRACKER) 0) {
        return;
    }

    if (StartingPte + NumberOfPtes != MmSystemPtesStart[SystemPteSpace]) {
#if 0
        MiFreePoolPages ((PVOID)MiPteTracker);
#endif
        MiPteTracker = (PMMPTE_TRACKER) 0;
        return;
    }

    if (MiPteTracker) {
        OldPtes = MmSystemPtesEnd[SystemPteSpace] - MmSystemPtesStart[SystemPteSpace] + 1;

        NewPtes = OldPtes + NumberOfPtes;

        NewTracker = (PMMPTE_TRACKER) ExAllocatePoolWithTag (
                                          NonPagedPool,
                                          (ULONG_PTR)(NewPtes * sizeof (MMPTE_TRACKER)),
                                          '  mM');

        if (NewTracker == (PMMPTE_TRACKER) 0) {
            MiPteTracker = (PMMPTE_TRACKER) 0;
            return;
        }

        RtlZeroMemory (NewTracker, NewPtes * sizeof (MMPTE_TRACKER));

        OldTracker = MiPteTracker;

        MiLockSystemSpace(OldIrql);

        RtlCopyMemory (NewTracker + NumberOfPtes, OldTracker, OldPtes * sizeof (MMPTE_TRACKER));

        MiPteTracker = NewTracker;

        MiUnlockSystemSpace(OldIrql);

#if 0
        MiFreePoolPages ((PVOID)OldTracker);
#endif

        MiPtesChecked[0xD] += 1;
    }
}
#endif


PMMPTE
MiReserveSystemPtes (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
    IN ULONG Alignment,
    IN ULONG Offset,
    IN ULONG BugCheckOnFailure
    )

/*++

Routine Description:

    This function locates the specified number of unused PTEs to locate
    within the non paged portion of system space.

Arguments:

    NumberOfPtes - Supplies the number of PTEs to locate.

    SystemPtePoolType - Supplies the PTE type of the pool to expand, one of
                        SystemPteSpace or NonPagedPoolExpansion.

    Alignment - Supplies the virtual address alignment for the address
                the returned PTE maps. For example, if the value is 64K,
                the returned PTE will map an address on a 64K boundary.
                An alignment of zero means to align on a page boundary.

    Offset - Supplies the offset into the alignment for the virtual address.
             For example, if the Alignment is 64k and the Offset is 4k,
             the returned address will be 4k above a 64k boundary.

    BugCheckOnFailure - Supplies FALSE if NULL should be returned if
                        the request cannot be satisfied, TRUE if
                        a bugcheck should be issued.

Return Value:

    Returns the address of the first PTE located.
    NULL if no system PTEs can be located and BugCheckOnFailure is FALSE.

Environment:

    Kernel mode, DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE Previous;
    KIRQL OldIrql;
    ULONG PteMask;
    ULONG MaskSize;
    ULONG Index;

#ifdef _MI_GUARD_PTE_
    ULONG ExactPtes;

    if (NumberOfPtes == 0) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0xA,
                      BugCheckOnFailure,
                      NumberOfPtes,
                      SystemPtePoolType);
    }

    if (SystemPtePoolType == SystemPteSpace) {
        NumberOfPtes += 1;
    }

    ExactPtes = NumberOfPtes;
#endif

    if (SystemPtePoolType == SystemPteSpace) {

        MaskSize = (Alignment - 1) >> (PAGE_SHIFT - PTE_SHIFT);
        PteMask = MaskSize & (Offset >> (PAGE_SHIFT - PTE_SHIFT));

        //
        // Acquire the system space lock to synchronize access to this
        // routine.
        //

        MiLockSystemSpace(OldIrql);

        if (NumberOfPtes <= MM_PTE_TABLE_LIMIT) {
            Index = MmSysPteTables [NumberOfPtes];
            ASSERT (NumberOfPtes <= MmSysPteIndex[Index]);
            PointerPte = &MmFreeSysPteListBySize[Index];
#if DBG
            if (MmDebug & MM_DBG_SYS_PTES) {
                PMMPTE PointerPte1;
                PointerPte1 = &MmFreeSysPteListBySize[Index];
                while (PointerPte1->u.List.NextEntry != MM_EMPTY_PTE_LIST) {
                    PMMPTE PointerFreedPte;
                    ULONG j;

                    PointerPte1 = MmSystemPteBase + PointerPte1->u.List.NextEntry;
                    PointerFreedPte = PointerPte1;
                    for (j = 0; j < MmSysPteIndex[Index]; j++) {
                        ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                        PointerFreedPte++;
                    }
                }
            }
#endif //DBG

            Previous = PointerPte;

            while (PointerPte->u.List.NextEntry != MM_EMPTY_PTE_LIST) {

                //
                //  Try to find suitable PTEs with the proper alignment.
                //

                Previous = PointerPte;
                PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
#if !defined(_IA64_)
                if (PointerPte == MmFlushPte1) {
                    KeFlushEntireTb (TRUE, TRUE);
                    MmFlushCounter = (MmFlushCounter + 1) & MM_FLUSH_COUNTER_MASK;
                    MmFlushPte1 = NULL;
                }
#endif
                if ((Alignment == 0) ||
                    (((ULONG_PTR)PointerPte & MaskSize) == PteMask)) {

#ifdef _MI_SYSPTE_DEBUG_
                    MiCheckPteAllocation (PointerPte,
                                          NumberOfPtes,
                                          0,
                                          MmSysPteIndex[Index]);
#endif
                    //
                    // Proper alignment and offset, update list index.
                    //

                    ASSERT ((PointerPte->u.List.NextEntry + MmSystemPteBase) >=
                             MmSystemPtesStart[SystemPtePoolType] ||
                             PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST);
                    ASSERT ((PointerPte->u.List.NextEntry + MmSystemPteBase) <=
                             MmSystemPtesEnd[SystemPtePoolType] ||
                             PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST);

                    Previous->u.List.NextEntry = PointerPte->u.List.NextEntry;
                    MmSysPteListBySizeCount [Index] -= 1;

#if !defined(_IA64_)
                    if (NumberOfPtes != 1) {

                        //
                        // Check to see if the TB should be flushed.
                        //

                        if ((PointerPte + 1)->u.List.NextEntry == MmFlushCounter) {
                            KeFlushEntireTb (TRUE, TRUE);
                            MmFlushCounter = (MmFlushCounter + 1) &
                                                    MM_FLUSH_COUNTER_MASK;
                            MmFlushPte1 = NULL;
                        }
                    }
#endif
                    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
                        MmLastSysPteListBySize[Index] = Previous;
                    }
#if DBG

                    if (MmDebug & MM_DBG_SYS_PTES) {
                        PMMPTE PointerPte1;
                        PointerPte1 = &MmFreeSysPteListBySize[Index];
                        while (PointerPte1->u.List.NextEntry != MM_EMPTY_PTE_LIST) {
                            PMMPTE PointerFreedPte;
                            ULONG j;

                            PointerPte1 = MmSystemPteBase + PointerPte1->u.List.NextEntry;
                            PointerFreedPte = PointerPte1;
                            for (j = 0; j < MmSysPteIndex[Index]; j++) {
                                ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                                PointerFreedPte++;
                            }
                        }
                    }
#endif //DBG
                    MiUnlockSystemSpace(OldIrql);

#if DBG
                    PointerPte->u.List.NextEntry = 0xABCDE;
                    if (MmDebug & MM_DBG_SYS_PTES) {

                        PMMPTE PointerFreedPte;
                        ULONG j;

                        PointerFreedPte = PointerPte;
                        for (j = 0; j < MmSysPteIndex[Index]; j++) {
                            ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                            PointerFreedPte++;
                        }
                    }
                    if (PointerPte < MmSystemPtesStart[SystemPtePoolType]) {
                        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                      0xB,
                                      (ULONG_PTR)PointerPte,
                                      NumberOfPtes,
                                      SystemPtePoolType);
                    }
                    if (PointerPte > MmSystemPtesEnd[SystemPtePoolType]) {
                        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                                      0xC,
                                      (ULONG_PTR)PointerPte,
                                      NumberOfPtes,
                                      SystemPtePoolType);
                    }
#endif //DBG

                    if (MmSysPteListBySizeCount[Index] <
                                            MmSysPteMinimumFree[Index]) {
                        MiFeedSysPtePool (Index);
                    }
#ifdef _MI_GUARD_PTE_
                    (PointerPte + ExactPtes - 1)->u.Long = MM_KERNEL_NOACCESS_PTE;
#endif
                    return PointerPte;
                }
            }
            NumberOfPtes = MmSysPteIndex [Index];
        }
        MiUnlockSystemSpace(OldIrql);
    }

#ifdef _MI_GUARD_PTE_
    NumberOfPtes = ExactPtes;
#endif

    PointerPte = MiReserveSystemPtes2 (NumberOfPtes,
                                       SystemPtePoolType,
                                       Alignment,
                                       Offset,
                                       BugCheckOnFailure);

#if DBG
    if (MmDebug & MM_DBG_SYS_PTES) {

        PMMPTE PointerFreedPte;
        ULONG j;

        if (PointerPte) {
            PointerFreedPte = PointerPte;
            for (j = 0; j < NumberOfPtes; j++) {
                ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                PointerFreedPte++;
            }
        }
    }
#endif //DBG

#ifdef _MI_GUARD_PTE_
    if (PointerPte) {
        (PointerPte + ExactPtes - 1)->u.Long = MM_KERNEL_NOACCESS_PTE;
    }
#endif

    return PointerPte;
}

VOID
MiFeedSysPtePool (
    IN ULONG Index
    )

/*++

Routine Description:

    This routine adds PTEs to the look aside lists.

Arguments:

    Index - Supplies the index for the look aside list to fill.

Return Value:

    None.


Environment:

    Kernel mode, internal to SysPtes.

--*/

{
#ifdef _MI_GUARD_PTE_
    UNREFERENCED_PARAMETER (Index);
#else
    ULONG i;
    PMMPTE PointerPte;

    if (MmTotalFreeSystemPtes[SystemPteSpace] < MM_MIN_SYSPTE_FREE) {
        return;
    }

    for (i = 0; i < 10 ; i++ ) {
        PointerPte = MiReserveSystemPtes2 (MmSysPteIndex [Index],
                                           SystemPteSpace,
                                           0,
                                           0,
                                           FALSE);
        if (PointerPte == NULL) {
            return;
        }
        MiReleaseSystemPtes (PointerPte,
                             MmSysPteIndex [Index],
                             SystemPteSpace);
    }
#endif
    return;
}


PMMPTE
MiReserveSystemPtes2 (
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType,
    IN ULONG Alignment,
    IN ULONG Offset,
    IN ULONG BugCheckOnFailure
    )

/*++

Routine Description:

    This function locates the specified number of unused PTEs to locate
    within the non paged portion of system space.

Arguments:

    NumberOfPtes - Supplies the number of PTEs to locate.

    SystemPtePoolType - Supplies the PTE type of the pool to expand, one of
                        SystemPteSpace or NonPagedPoolExpansion.

    Alignment - Supplies the virtual address alignment for the address
                the returned PTE maps. For example, if the value is 64K,
                the returned PTE will map an address on a 64K boundary.
                An alignment of zero means to align on a page boundary.

    Offset - Supplies the offset into the alignment for the virtual address.
             For example, if the Alignment is 64k and the Offset is 4k,
             the returned address will be 4k above a 64k boundary.

    BugCheckOnFailure - Supplies FALSE if NULL should be returned if
                        the request cannot be satisfied, TRUE if
                        a bugcheck should be issued.

Return Value:

    Returns the address of the first PTE located.
    NULL if no system PTEs can be located and BugCheckOnFailure is FALSE.

Environment:

    Kernel mode, DISPATCH_LEVEL or below.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerFollowingPte;
    PMMPTE Previous;
    ULONG_PTR SizeInSet;
    KIRQL OldIrql;
    ULONG MaskSize;
    ULONG NumberOfRequiredPtes;
    ULONG OffsetSum;
    ULONG PtesToObtainAlignment;
    PMMPTE NextSetPointer;
    ULONG_PTR LeftInSet;
    ULONG_PTR PteOffset;
    MMPTE_FLUSH_LIST PteFlushList;
    PVOID HighConsumer;
    ULONG_PTR HighPteUse;

    MaskSize = (Alignment - 1) >> (PAGE_SHIFT - PTE_SHIFT);

    OffsetSum = (Offset >> (PAGE_SHIFT - PTE_SHIFT)) |
                            (Alignment >> (PAGE_SHIFT - PTE_SHIFT));

    MiLockSystemSpace(OldIrql);

    //
    // The nonpaged PTE pool uses the invalid PTEs to define the pool
    // structure.   A global pointer points to the first free set
    // in the list, each free set contains the number free and a pointer
    // to the next free set.  The free sets are kept in an ordered list
    // such that the pointer to the next free set is always greater
    // than the address of the current free set.
    //
    // As to not limit the size of this pool, two PTEs are used
    // to define a free region.  If the region is a single PTE, the
    // prototype field within the PTE is set indicating the set
    // consists of a single PTE.
    //
    // The page frame number field is used to define the next set
    // and the number free.  The two flavors are:
    //
    //                           o          V
    //                           n          l
    //                           e          d
    //  +-----------------------+-+----------+
    //  |  next set             |0|0        0|
    //  +-----------------------+-+----------+
    //  |  number in this set   |0|0        0|
    //  +-----------------------+-+----------+
    //
    //
    //  +-----------------------+-+----------+
    //  |  next set             |1|0        0|
    //  +-----------------------+-+----------+
    //  ...
    //

    //
    // Acquire the system space lock to synchronize access to this routine.
    //

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    Previous = PointerPte;

    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

        //
        // End of list and none found, return NULL or bugcheck.
        //

        if (BugCheckOnFailure) {
            goto IssueBugcheck;
        }

        MiUnlockSystemSpace(OldIrql);
        return NULL;
    }

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    if (Alignment <= PAGE_SIZE) {

        //
        // Don't deal with alignment issues.
        //

        while (TRUE) {

            if (PointerPte->u.List.OneEntry) {
                SizeInSet = 1;

            } else {

                PointerFollowingPte = PointerPte + 1;
                SizeInSet = (ULONG_PTR) PointerFollowingPte->u.List.NextEntry;
            }

            if (NumberOfPtes < SizeInSet) {

                //
                // Get the PTEs from this set and reduce the size of the
                // set.  Note that the size of the current set cannot be 1.
                //

                if ((SizeInSet - NumberOfPtes) == 1) {

                    //
                    // Collapse to the single PTE format.
                    //

                    PointerPte->u.List.OneEntry = 1;

                } else {

                    PointerFollowingPte->u.List.NextEntry = SizeInSet - NumberOfPtes;

                    //
                    // Get the required PTEs from the end of the set.
                    //

#if 0
                    if (MmDebug & MM_DBG_SYS_PTES) {
                        MiDumpSystemPtes(SystemPtePoolType);
                        PointerFollowingPte = PointerPte + (SizeInSet - NumberOfPtes);
                        DbgPrint("allocated 0x%lx Ptes at %lx\n",NumberOfPtes,PointerFollowingPte);
                    }
#endif //0
                }

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG

#ifdef _MI_SYSPTE_DEBUG_
                MiCheckPteAllocation (PointerPte + (SizeInSet - NumberOfPtes),
                                      NumberOfPtes,
                                      1,
                                      0);
#endif

                MiUnlockSystemSpace(OldIrql);

                PointerPte =  PointerPte + (SizeInSet - NumberOfPtes);
                goto Flush;
            }

            if (NumberOfPtes == SizeInSet) {

                //
                // Satisfy the request with this complete set and change
                // the list to reflect the fact that this set is gone.
                //

                Previous->u.List.NextEntry = PointerPte->u.List.NextEntry;

                //
                // Release the system PTE lock.
                //

#if 0
                if (MmDebug & MM_DBG_SYS_PTES) {
                        MiDumpSystemPtes(SystemPtePoolType);
                        PointerFollowingPte = PointerPte + (SizeInSet - NumberOfPtes);
                        DbgPrint("allocated 0x%lx Ptes at %lx\n",NumberOfPtes,PointerFollowingPte);
                }
#endif //0

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG

#ifdef _MI_SYSPTE_DEBUG_
                MiCheckPteAllocation (PointerPte,
                                      NumberOfPtes,
                                      2,
                                      0);
#endif

                MiUnlockSystemSpace(OldIrql);
                goto Flush;
            }

            //
            // Point to the next set and try again
            //

            if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

                //
                // End of list and none found, return NULL or bugcheck.
                //

                if (BugCheckOnFailure) {
                    goto IssueBugcheck;
                }

                MiUnlockSystemSpace(OldIrql);
                return NULL;
            }
            Previous = PointerPte;
            PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
            ASSERT (PointerPte > Previous);
        }

    } else {

        //
        // Deal with the alignment issues.
        //

        while (TRUE) {

            if (PointerPte->u.List.OneEntry) {
                SizeInSet = 1;

            } else {

                PointerFollowingPte = PointerPte + 1;
                SizeInSet = (ULONG_PTR) PointerFollowingPte->u.List.NextEntry;
            }

            PtesToObtainAlignment = (ULONG)
                (((OffsetSum - ((ULONG_PTR)PointerPte & MaskSize)) & MaskSize) >>
                    PTE_SHIFT);

            NumberOfRequiredPtes = NumberOfPtes + PtesToObtainAlignment;

            if (NumberOfRequiredPtes < SizeInSet) {

                //
                // Get the PTEs from this set and reduce the size of the
                // set.  Note that the size of the current set cannot be 1.
                //
                // This current block will be slit into 2 blocks if
                // the PointerPte does not match the alignment.
                //

                //
                // Check to see if the first PTE is on the proper
                // alignment, if so, eliminate this block.
                //

                LeftInSet = SizeInSet - NumberOfRequiredPtes;

                //
                // Set up the new set at the end of this block.
                //

                NextSetPointer = PointerPte + NumberOfRequiredPtes;
                NextSetPointer->u.List.NextEntry =
                                       PointerPte->u.List.NextEntry;

                PteOffset = (ULONG_PTR)(NextSetPointer - MmSystemPteBase);

                if (PtesToObtainAlignment == 0) {

                    Previous->u.List.NextEntry += NumberOfRequiredPtes;

                } else {

                    //
                    // Point to the new set at the end of the block
                    // we are giving away.
                    //

                    PointerPte->u.List.NextEntry = PteOffset;

                    //
                    // Update the size of the current set.
                    //

                    if (PtesToObtainAlignment == 1) {

                        //
                        // Collapse to the single PTE format.
                        //

                        PointerPte->u.List.OneEntry = 1;

                    } else {

                        //
                        // Set the set size in the next PTE.
                        //

                        PointerFollowingPte->u.List.NextEntry =
                                                        PtesToObtainAlignment;
                    }
                }

                //
                // Set up the new set at the end of the block.
                //

                if (LeftInSet == 1) {
                    NextSetPointer->u.List.OneEntry = 1;
                } else {
                    NextSetPointer->u.List.OneEntry = 0;
                    NextSetPointer += 1;
                    NextSetPointer->u.List.NextEntry = LeftInSet;
                }
                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG

#ifdef _MI_SYSPTE_DEBUG_
                MiCheckPteAllocation (PointerPte + PtesToObtainAlignment,
                                      NumberOfPtes,
                                      3,
                                      0);
#endif

                MiUnlockSystemSpace(OldIrql);

                PointerPte = PointerPte + PtesToObtainAlignment;
                goto Flush;
            }

            if (NumberOfRequiredPtes == SizeInSet) {

                //
                // Satisfy the request with this complete set and change
                // the list to reflect the fact that this set is gone.
                //

                if (PtesToObtainAlignment == 0) {

                    //
                    // This block exactly satisfies the request.
                    //

                    Previous->u.List.NextEntry =
                                            PointerPte->u.List.NextEntry;

                } else {

                    //
                    // A portion at the start of this block remains.
                    //

                    if (PtesToObtainAlignment == 1) {

                        //
                        // Collapse to the single PTE format.
                        //

                        PointerPte->u.List.OneEntry = 1;

                    } else {
                      PointerFollowingPte->u.List.NextEntry =
                                                        PtesToObtainAlignment;

                    }
                }

                MmTotalFreeSystemPtes[SystemPtePoolType] -= NumberOfPtes;
#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                             MiCountFreeSystemPtes (SystemPtePoolType));
                }
#endif //DBG

#ifdef _MI_SYSPTE_DEBUG_
                MiCheckPteAllocation (PointerPte + PtesToObtainAlignment,
                                      NumberOfPtes,
                                      4,
                                      0);
#endif

                MiUnlockSystemSpace(OldIrql);

                PointerPte = PointerPte + PtesToObtainAlignment;
                goto Flush;
            }

            //
            // Point to the next set and try again
            //

            if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {

                //
                // End of list and none found, return NULL or bugcheck.
                //

                if (BugCheckOnFailure) {
                    goto IssueBugcheck;
                }

                MiUnlockSystemSpace(OldIrql);
                return NULL;
            }
            Previous = PointerPte;
            PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
            ASSERT (PointerPte > Previous);
        }
    }
Flush:

    if (SystemPtePoolType == SystemPteSpace) {
        PVOID BaseAddress;
        ULONG j;

        PteFlushList.Count = 0;
        Previous = PointerPte;
        BaseAddress = MiGetVirtualAddressMappedByPte (Previous);

        for (j = 0; j < NumberOfPtes ; j++) {
            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushPte[PteFlushList.Count] = Previous;
                PteFlushList.FlushVa[PteFlushList.Count] = BaseAddress;
                PteFlushList.Count += 1;
            }

            //
            // PTEs being freed better be invalid.
            //
            ASSERT (Previous->u.Hard.Valid == 0);

            *Previous = ZeroKernelPte;
            BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
            Previous++;
        }

        KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
        MiFlushPteList (&PteFlushList, TRUE, ZeroKernelPte);
        KeLowerIrql (OldIrql);
    }
    return PointerPte;

IssueBugcheck:

    if (SystemPtePoolType == SystemPteSpace) {

        HighConsumer = MiGetHighestPteConsumer (&HighPteUse);

        if (HighConsumer != NULL) {
            KeBugCheckEx (DRIVER_USED_EXCESSIVE_PTES,
                          (ULONG_PTR)HighConsumer,
                          HighPteUse,
                          MmTotalFreeSystemPtes[SystemPtePoolType],
                          MmNumberOfSystemPtes);
        }
    }

    KeBugCheckEx (NO_MORE_SYSTEM_PTES,
                  (ULONG_PTR)SystemPtePoolType,
                  NumberOfPtes,
                  MmTotalFreeSystemPtes[SystemPtePoolType],
                  MmNumberOfSystemPtes);
}

VOID
MiReleaseSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This function releases the specified number of PTEs
    within the non paged portion of system space.

    Note that the PTEs must be invalid and the page frame number
    must have been set to zero.

Arguments:

    StartingPte - Supplies the address of the first PTE to release.

    NumberOfPtes - Supplies the number of PTEs to release.

    SystemPtePoolType - Supplies the PTE type of the pool to release PTEs to,
                        one of SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    none.

Environment:

    Kernel mode.

--*/

{
    ULONG_PTR Size;
    ULONG i;
    ULONG_PTR PteOffset;
    PMMPTE PointerPte;
    PMMPTE PointerFollowingPte;
    PMMPTE NextPte;
    KIRQL OldIrql;
    ULONG Index;
#if defined(_IA64_)
    MMPTE_FLUSH_LIST PteFlushList;
    PMMPTE Previous;
    PVOID BaseAddress;
#endif

    //
    // Check to make sure the PTEs don't map anything.
    //

    ASSERT (NumberOfPtes != 0);

#ifdef _MI_GUARD_PTE_
    if (NumberOfPtes == 0) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0xD,
                      (ULONG_PTR)StartingPte,
                      NumberOfPtes,
                      SystemPtePoolType);
    }

    if (SystemPtePoolType == SystemPteSpace) {
        if ((StartingPte + NumberOfPtes)->u.Long != MM_KERNEL_NOACCESS_PTE) {
            KeBugCheckEx (SYSTEM_PTE_MISUSE,
                          0xE,
                          (ULONG_PTR)StartingPte,
                          NumberOfPtes,
                          SystemPtePoolType);
        }
        NumberOfPtes += 1;
    }
#endif

#if DBG
    if (StartingPte < MmSystemPtesStart[SystemPtePoolType]) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0xF,
                      (ULONG_PTR)StartingPte,
                      NumberOfPtes,
                      SystemPtePoolType);
    }

    if (StartingPte > MmSystemPtesEnd[SystemPtePoolType]) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x10,
                      (ULONG_PTR)StartingPte,
                      NumberOfPtes,
                      SystemPtePoolType);
    }
#endif //DBG

#if 0
    if (MmDebug & MM_DBG_SYS_PTES) {
        DbgPrint("releasing 0x%lx system PTEs at location %lx\n",NumberOfPtes,StartingPte);
    }
#endif //0

#if defined(_IA64_)

    PteFlushList.Count = 0;
    Previous = StartingPte;
    BaseAddress = MiGetVirtualAddressMappedByPte (Previous);
    for (i = 0; i < NumberOfPtes ; i++) {
        if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
            PteFlushList.FlushPte[PteFlushList.Count] = Previous;
            PteFlushList.FlushVa[PteFlushList.Count] = BaseAddress;
            PteFlushList.Count += 1;
        }
        *Previous = ZeroKernelPte;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        Previous++;
    }

    KeRaiseIrql (DISPATCH_LEVEL, &OldIrql);
    MiFlushPteList (&PteFlushList, TRUE, ZeroKernelPte);
    KeLowerIrql (OldIrql);

#else

    //
    // Zero PTEs.
    //

#ifdef _MI_SYSPTE_DEBUG_
    MiValidateSystemPtes (StartingPte, NumberOfPtes);
#endif

    MiFillMemoryPte (StartingPte,
                     NumberOfPtes * sizeof (MMPTE),
                     ZeroKernelPte.u.Long);

#ifdef _MI_SYSPTE_DEBUG_

    //
    // Invalidate any TB entries that might have been mapped to
    // immediately prevent bad callers from corrupting the system.
    //

    KeFlushEntireTb (TRUE, TRUE);
#endif

#endif

    //
    // Acquire system space spin lock to synchronize access.
    //

    PteOffset = (ULONG_PTR)(StartingPte - MmSystemPteBase);

#ifdef _MI_SYSPTE_DEBUG_
    if (PteOffset == 0) {
        KeBugCheckEx (SYSTEM_PTE_MISUSE,
                      0x11,
                      (ULONG_PTR)StartingPte,
                      NumberOfPtes,
                      SystemPtePoolType);
    }
#endif

    MiLockSystemSpace(OldIrql);

#ifdef _MI_SYSPTE_DEBUG_
    MiCheckPteRelease (StartingPte, NumberOfPtes);
#endif

#ifndef _MI_GUARD_PTE_
    if ((SystemPtePoolType == SystemPteSpace) &&
        (NumberOfPtes <= MM_PTE_TABLE_LIMIT)) {

        Index = MmSysPteTables [NumberOfPtes];
        NumberOfPtes = MmSysPteIndex [Index];

        if (MmTotalFreeSystemPtes[SystemPteSpace] >= MM_MIN_SYSPTE_FREE) {

            //
            // Don't add to the pool if the size is greater than 15 + the minimum.
            //

            i = MmSysPteMinimumFree[Index];
            if (MmTotalFreeSystemPtes[SystemPteSpace] >= MM_MAX_SYSPTE_FREE) {

                //
                // Lots of free PTEs, quadruple the limit.
                //

                i = i * 4;
            }
            i += 15;
            if (MmSysPteListBySizeCount[Index] <= i) {

#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    PMMPTE PointerPte1;

                    PointerPte1 = &MmFreeSysPteListBySize[Index];
                    while (PointerPte1->u.List.NextEntry != MM_EMPTY_PTE_LIST) {
                        PMMPTE PointerFreedPte;
                        ULONG j;

                        PointerPte1 = MmSystemPteBase + PointerPte1->u.List.NextEntry;
                        PointerFreedPte = PointerPte1;
                        for (j = 0; j < MmSysPteIndex[Index]; j++) {
                            ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                            PointerFreedPte++;
                        }
                    }
                }
#endif //DBG
                MmSysPteListBySizeCount [Index] += 1;
                PointerPte = MmLastSysPteListBySize[Index];
                ASSERT (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST);
                PointerPte->u.List.NextEntry = PteOffset;
                MmLastSysPteListBySize[Index] = StartingPte;
                StartingPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

#if DBG
                if (MmDebug & MM_DBG_SYS_PTES) {
                    PMMPTE PointerPte1;
                    PointerPte1 = &MmFreeSysPteListBySize[Index];
                    while (PointerPte1->u.List.NextEntry != MM_EMPTY_PTE_LIST) {
                        PMMPTE PointerFreedPte;
                        ULONG j;

                        PointerPte1 = MmSystemPteBase + PointerPte1->u.List.NextEntry;
                        PointerFreedPte = PointerPte1;
                        for (j = 0; j < MmSysPteIndex[Index]; j++) {
                            ASSERT (PointerFreedPte->u.Hard.Valid == 0);
                            PointerFreedPte++;
                        }
                    }
                }
#endif //DBG
                if (NumberOfPtes == 1) {
                    if (MmFlushPte1 == NULL) {
                        MmFlushPte1 = StartingPte;
                    }
                } else {
                    (StartingPte + 1)->u.List.NextEntry = MmFlushCounter;
                }

                MiUnlockSystemSpace(OldIrql);
                return;
            }
        }
    }
#endif

    MmTotalFreeSystemPtes[SystemPtePoolType] += NumberOfPtes;

    PteOffset = (ULONG_PTR)(StartingPte - MmSystemPteBase);
    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];

    while (TRUE) {
        NextPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
        if (PteOffset < PointerPte->u.List.NextEntry) {

            //
            // Insert in the list at this point.  The
            // previous one should point to the new freed set and
            // the new freed set should point to the place
            // the previous set points to.
            //
            // Attempt to combine the clusters before we
            // insert.
            //
            // Locate the end of the current structure.
            //

            ASSERT (((StartingPte + NumberOfPtes) <= NextPte) ||
                    (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST));

            PointerFollowingPte = PointerPte + 1;
            if (PointerPte->u.List.OneEntry) {
                Size = 1;
            } else {
                Size = (ULONG_PTR) PointerFollowingPte->u.List.NextEntry;
            }
            if ((PointerPte + Size) == StartingPte) {

                //
                // We can combine the clusters.
                //

                NumberOfPtes += (ULONG)Size;
                PointerFollowingPte->u.List.NextEntry = NumberOfPtes;
                PointerPte->u.List.OneEntry = 0;

                //
                // Point the starting PTE to the beginning of
                // the new free set and try to combine with the
                // following free cluster.
                //

                StartingPte = PointerPte;

            } else {

                //
                // Can't combine with previous. Make this Pte the
                // start of a cluster.
                //

                //
                // Point this cluster to the next cluster.
                //

                StartingPte->u.List.NextEntry = PointerPte->u.List.NextEntry;

                //
                // Point the current cluster to this cluster.
                //

                PointerPte->u.List.NextEntry = PteOffset;

                //
                // Set the size of this cluster.
                //

                if (NumberOfPtes == 1) {
                    StartingPte->u.List.OneEntry = 1;

                } else {
                    StartingPte->u.List.OneEntry = 0;
                    PointerFollowingPte = StartingPte + 1;
                    PointerFollowingPte->u.List.NextEntry = NumberOfPtes;
                }
            }

            //
            // Attempt to combine the newly created cluster with
            // the following cluster.
            //

            if ((StartingPte + NumberOfPtes) == NextPte) {

                //
                // Combine with following cluster.
                //

                //
                // Set the next cluster to the value contained in the
                // cluster we are merging into this one.
                //

                StartingPte->u.List.NextEntry = NextPte->u.List.NextEntry;
                StartingPte->u.List.OneEntry = 0;
                PointerFollowingPte = StartingPte + 1;

                if (NextPte->u.List.OneEntry) {
                    Size = 1;

                } else {
                    NextPte++;
                    Size = (ULONG_PTR) NextPte->u.List.NextEntry;
                }
                PointerFollowingPte->u.List.NextEntry = NumberOfPtes + Size;
            }
#if 0
            if (MmDebug & MM_DBG_SYS_PTES) {
                MiDumpSystemPtes(SystemPtePoolType);
            }
#endif //0

#if DBG
            if (MmDebug & MM_DBG_SYS_PTES) {
                ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                         MiCountFreeSystemPtes (SystemPtePoolType));
            }
#endif //DBG
            MiUnlockSystemSpace(OldIrql);
            return;
        }

        //
        // Point to next freed cluster.
        //

        PointerPte = NextPte;
    }
}

VOID
MiInitializeSystemPtes (
    IN PMMPTE StartingPte,
    IN ULONG NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This routine initializes the system PTE pool.

Arguments:

    StartingPte - Supplies the address of the first PTE to put in the pool.

    NumberOfPtes - Supplies the number of PTEs to put in the pool.

    SystemPtePoolType - Supplies the PTE type of the pool to initialize, one of
                        SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    none.

Environment:

    Kernel mode.

--*/

{
    LONG i;
    LONG j;
#ifdef _MI_SYSPTE_DEBUG_
    PMMPTE_TRACKER Tracker;
#endif

    //
    // Set the base of the system PTE pool to this PTE.  This takes into
    // account that systems may have additional PTE pools below the PTE_BASE.
    //

    MmSystemPteBase = MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS;

    MmSystemPtesStart[SystemPtePoolType] = StartingPte;
    MmSystemPtesEnd[SystemPtePoolType] = StartingPte + NumberOfPtes - 1;

    //
    // If there are no PTEs specified, then make a valid chain by indicating
    // that the list is empty.
    //

    if (NumberOfPtes == 0) {
        MmFirstFreeSystemPte[SystemPtePoolType] = ZeroKernelPte;
        MmFirstFreeSystemPte[SystemPtePoolType].u.List.NextEntry =
                                                                MM_EMPTY_LIST;
        return;
    }

    //
    // Initialize the specified system pte pool.
    //

    MiFillMemoryPte (StartingPte,
                     NumberOfPtes * sizeof (MMPTE),
                     ZeroKernelPte.u.Long);

    //
    // The page frame field points to the next cluster.  As we only
    // have one cluster at initialization time, mark it as the last
    // cluster.
    //

    StartingPte->u.List.NextEntry = MM_EMPTY_LIST;

    MmFirstFreeSystemPte[SystemPtePoolType] = ZeroKernelPte;
    MmFirstFreeSystemPte[SystemPtePoolType].u.List.NextEntry =
                                                StartingPte - MmSystemPteBase;

    //
    // If there is only one PTE in the pool, then mark it as a one entry
    // PTE. Otherwise, store the cluster size in the following PTE.
    //

    if (NumberOfPtes == 1) {
        StartingPte->u.List.OneEntry = TRUE;

    } else {
        StartingPte += 1;
        MI_WRITE_INVALID_PTE (StartingPte, ZeroKernelPte);
        StartingPte->u.List.NextEntry = NumberOfPtes;
    }

    //
    // Set the total number of free PTEs for the specified type.
    //

    MmTotalFreeSystemPtes[SystemPtePoolType] = NumberOfPtes;

    ASSERT (MmTotalFreeSystemPtes[SystemPtePoolType] ==
                         MiCountFreeSystemPtes (SystemPtePoolType));

    if (SystemPtePoolType == SystemPteSpace) {

        ULONG Lists[MM_SYS_PTE_TABLES_MAX] = {MM_PTE_LIST_1, MM_PTE_LIST_2, MM_PTE_LIST_4, MM_PTE_LIST_8, MM_PTE_LIST_16};
        PMMPTE PointerPte;
        ULONG total;

#ifdef _MI_SYSPTE_DEBUG_
        Tracker = (PMMPTE_TRACKER) MiAllocatePoolPages (
                                          NonPagedPool,
                                          NumberOfPtes * sizeof (MMPTE_TRACKER),
                                          0);

        if (Tracker) {
            RtlZeroMemory (Tracker, NumberOfPtes * sizeof (MMPTE_TRACKER));
        }
#endif

        for (j = 0; j < MM_SYS_PTE_TABLES_MAX ; j++) {
            MmFreeSysPteListBySize [j].u.List.NextEntry = MM_EMPTY_PTE_LIST;
            MmLastSysPteListBySize [j] = &MmFreeSysPteListBySize [j];
        }
        MmFlushCounter += 1;

#ifndef _MI_GUARD_PTE_

        //
        // Initialize the by size lists.
        //

        total = MM_PTE_LIST_1 * MmSysPteIndex[0] +
                MM_PTE_LIST_2 * MmSysPteIndex[1] +
                MM_PTE_LIST_4 * MmSysPteIndex[2] +
                MM_PTE_LIST_8 * MmSysPteIndex[3] +
                MM_PTE_LIST_16 * MmSysPteIndex[4];

        PointerPte = MiReserveSystemPtes (total,
                                          SystemPteSpace,
                                          64*1024,
                                          0,
                                          TRUE);

        for (i = (MM_SYS_PTE_TABLES_MAX - 1); i >= 0; i--) {
            do {
                Lists[i] -= 1;
                MiReleaseSystemPtes (PointerPte,
                                     MmSysPteIndex[i],
                                     SystemPteSpace);
                PointerPte += MmSysPteIndex[i];
            } while (Lists[i] != 0  );
        }
#endif

        MmFlushCounter += 1;
        MmFlushPte1 = NULL;

#ifdef _MI_SYSPTE_DEBUG_
        MiPteTracker = Tracker;
#endif
    }

    return;
}

VOID
MiAddSystemPtes(
    IN PMMPTE StartingPte,
    IN ULONG  NumberOfPtes,
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

/*++

Routine Description:

    This routine adds newly created PTEs to the specified pool.

Arguments:

    StartingPte - Supplies the address of the first PTE to put in the pool.

    NumberOfPtes - Supplies the number of PTEs to put in the pool.

    SystemPtePoolType - Supplies the PTE type of the pool to expand, one of
                        SystemPteSpace or NonPagedPoolExpansion.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPTE EndingPte;

    EndingPte = StartingPte + NumberOfPtes - 1;

#ifdef _MI_SYSPTE_DEBUG_
    MiRebuildPteTracker (StartingPte, NumberOfPtes);
#endif

    if (StartingPte < MmSystemPtesStart[SystemPtePoolType]) {
        MmSystemPtesStart[SystemPtePoolType] = StartingPte;
    }

    if (EndingPte > MmSystemPtesEnd[SystemPtePoolType]) {
        MmSystemPtesEnd[SystemPtePoolType] = EndingPte;
    }

#ifdef _MI_SYSPTE_DEBUG_

    if (SystemPtePoolType == SystemPteSpace && MiPteTracker != NULL) {

        ULONG i;
        ULONG_PTR Index;
        PMMPTE_TRACKER Tracker;

        Index = StartingPte - MmSystemPtesStart[SystemPteSpace];
        Tracker = &MiPteTracker[Index];

        ASSERT (NumberOfPtes < 0x10000);
        Tracker->NumberOfPtes = (USHORT)NumberOfPtes;

        for (i = 0; i < NumberOfPtes; i += 1) {
            Tracker->InUse = TRUE;
            Tracker += 1;
        }
    }

    MiFillMemoryPte (StartingPte, NumberOfPtes * sizeof (MMPTE), MM_KERNEL_NOACCESS_PTE);
#endif

#ifdef _MI_GUARD_PTE_
    MiReleaseSystemPtes (StartingPte, NumberOfPtes - 1, SystemPtePoolType);
#else
    MiReleaseSystemPtes (StartingPte, NumberOfPtes, SystemPtePoolType);
#endif
}


ULONG
MiGetSystemPteListCount (
    IN ULONG ListSize
    )

/*++

Routine Description:

    This routine returns the number of free entries of the list which
    covers the specified size.  The size must be less than or equal to the
    largest list index.

Arguments:

    ListSize - Supplies the number of PTEs needed.

Return Value:

    Number of free entries on the list which contains ListSize PTEs.

Environment:

    Kernel mode.

--*/

{
#ifdef _MI_GUARD_PTE_
    UNREFERENCED_PARAMETER (ListSize);

    return 8;
#else
    ULONG Index;

    ASSERT (ListSize <= MM_PTE_TABLE_LIMIT);

    Index = MmSysPteTables [ListSize];
    return MmSysPteListBySizeCount[Index];
#endif
}


LOGICAL
MiGetSystemPteAvailability (
    IN ULONG NumberOfPtes,
    IN MM_PAGE_PRIORITY Priority
    )

/*++

Routine Description:

    This routine checks how many SystemPteSpace PTEs are available for the
    requested size.  If plenty are available then TRUE is returned.
    If we are reaching a low resource situation, then the request is evaluated
    based on the argument priority.

Arguments:

    NumberOfPtes - Supplies the number of PTEs needed.

    Priority - Supplies the priority of the request.

Return Value:

    TRUE if the caller should allocate the PTEs, FALSE if not.

Environment:

    Kernel mode.

--*/

{
    ULONG Index;
    ULONG FreePtes;
    ULONG FreeBinnedPtes;

    if (Priority == HighPagePriority) {
        return TRUE;
    }

#ifdef _MI_GUARD_PTE_
    NumberOfPtes += 1;
#endif

    FreePtes = MmTotalFreeSystemPtes[SystemPteSpace];

    if (NumberOfPtes <= MM_PTE_TABLE_LIMIT) {
        Index = MmSysPteTables [NumberOfPtes];
        FreeBinnedPtes = MmSysPteListBySizeCount[Index];

        if (FreeBinnedPtes > MmSysPteMinimumFree[Index]) {
            return TRUE;
        }
        if (FreeBinnedPtes != 0) {
            if (Priority == NormalPagePriority) {
                if (FreeBinnedPtes > 1 || FreePtes > 512) {
                    return TRUE;
                }
                return FALSE;
            }
            if (FreePtes > 2048) {
                return TRUE;
            }
            return FALSE;
        }
    }

    if (Priority == NormalPagePriority) {
        if ((LONG)NumberOfPtes < (LONG)FreePtes - 512) {
            return TRUE;
        }
        return FALSE;
    }

    if ((LONG)NumberOfPtes < (LONG)FreePtes - 2048) {
        return TRUE;
    }
    return FALSE;
}

#if DBG

VOID
MiDumpSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )


{
    PMMPTE PointerPte;
    PMMPTE PointerNextPte;
    ULONG_PTR ClusterSize;
    PMMPTE EndOfCluster;

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        return;
    }

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    for (;;) {
        if (PointerPte->u.List.OneEntry) {
            ClusterSize = 1;
        } else {
            PointerNextPte = PointerPte + 1;
            ClusterSize = (ULONG_PTR) PointerNextPte->u.List.NextEntry;
        }

        EndOfCluster = PointerPte + (ClusterSize - 1);

        DbgPrint("System Pte at %p for %p entries (%p)\n",
                PointerPte, ClusterSize, EndOfCluster);

        if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
            break;
        }

        PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
    }
    return;
}

ULONG
MiCountFreeSystemPtes (
    IN MMSYSTEM_PTE_POOL_TYPE SystemPtePoolType
    )

{
    PMMPTE PointerPte;
    PMMPTE PointerNextPte;
    ULONG_PTR ClusterSize;
    ULONG_PTR FreeCount;

    PointerPte = &MmFirstFreeSystemPte[SystemPtePoolType];
    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        return 0;
    }

    FreeCount = 0;

    PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;

    for (;;) {
        if (PointerPte->u.List.OneEntry) {
            ClusterSize = 1;

        } else {
            PointerNextPte = PointerPte + 1;
            ClusterSize = (ULONG_PTR) PointerNextPte->u.List.NextEntry;
        }

        FreeCount += ClusterSize;
        if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
            break;
        }

        PointerPte = MmSystemPteBase + PointerPte->u.List.NextEntry;
    }

    return (ULONG)FreeCount;
}

#endif //DBG

