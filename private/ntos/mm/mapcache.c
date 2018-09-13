/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    mapcache.c

Abstract:

    This module contains the routines which implement mapping views
    of sections into the system-wide cache.

Author:

    Lou Perazzoli (loup) 22-May-1990
    Landy Wang (landyw) 02-Jun-1997

Revision History:

--*/


#include "mi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,MiInitializeSystemCache )
#endif

extern ULONG MmFrontOfList;

VOID
MiFreeInPageSupportBlock (
    IN PMMINPAGE_SUPPORT Support
    );

VOID
MiRemoveMappedPtes (
    IN PVOID BaseAddress,
    IN ULONG NumberOfPtes,
    IN PCONTROL_AREA ControlArea,
    IN PMMSUPPORT WorkingSetInfo
    );

#define X256K 0x40000

PMMPTE MmFirstFreeSystemCache;

PMMPTE MmLastFreeSystemCache;

PMMPTE MmFlushSystemCache;

PMMPTE MmSystemCachePteBase;


LONG
MiMapCacheExceptionFilter (
    IN PNTSTATUS Status,
    IN PEXCEPTION_POINTERS ExceptionPointer
    );

NTSTATUS
MmMapViewInSystemCache (
    IN PVOID SectionToMap,
    OUT PVOID *CapturedBase,
    IN OUT PLARGE_INTEGER SectionOffset,
    IN OUT PULONG CapturedViewSize
    )

/*++

Routine Description:

    This function maps a view in the specified subject process to
    the section object.  The page protection is identical to that
    of the prototype PTE.

    This function is a kernel mode interface to allow LPC to map
    a section given the section pointer to map.

    This routine assumes all arguments have been probed and captured.

Arguments:

    SectionToMap - Supplies a pointer to the section object.

    BaseAddress - Supplies a pointer to a variable that will receive
         the base address of the view. If the initial value
         of this argument is not null, then the view will
         be allocated starting at the specified virtual
         address rounded down to the next 64kb address
         boundary. If the initial value of this argument is
         null, then the operating system will determine
         where to allocate the view using the information
         specified by the ZeroBits argument value and the
         section allocation attributes (i.e. based and
         tiled).

    SectionOffset - Supplies the offset from the beginning of the
         section to the view in bytes. This value must be a multiple
         of 256k.

    ViewSize - Supplies a pointer to a variable that will receive
         the actual size in bytes of the view.
         The initial values of this argument specifies the
         size of the view in bytes and is rounded up to the
         next host page size boundary and must be less than or equal
         to 256k.

Return Value:

    Returns the status

    TBS

Environment:

    Kernel mode.

--*/

{
    PSECTION Section;
    ULONG PteOffset;
    KIRQL OldIrql;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PMMPTE ProtoPte;
    PMMPTE LastProto;
    PSUBSECTION Subsection;
    PVOID EndingVa;
    PCONTROL_AREA ControlArea;

    Section = SectionToMap;

    //
    // Assert the view size is less 256kb and the section offset
    // is aligned on a 256k boundary.
    //

    ASSERT (*CapturedViewSize <= 256L*1024L);
    ASSERT ((SectionOffset->LowPart & (256L*1024L - 1)) == 0);

    //
    // Make sure the section is not an image section or a page file
    // backed section.
    //

    if (Section->u.Flags.Image) {
        return STATUS_NOT_MAPPED_DATA;
    }

    ControlArea = Section->Segment->ControlArea;

    ASSERT (*CapturedViewSize != 0);

    ASSERT (ControlArea->u.Flags.GlobalOnlyPerSession == 0);

    Subsection = (PSUBSECTION)(ControlArea + 1);

    LOCK_PFN (OldIrql);

    ASSERT (ControlArea->u.Flags.BeingCreated == 0);
    ASSERT (ControlArea->u.Flags.BeingDeleted == 0);
    ASSERT (ControlArea->u.Flags.BeingPurged == 0);

    //
    // Find a free 256k base in the cache.
    //

    if (MmFirstFreeSystemCache == (PMMPTE)MM_EMPTY_LIST) {
        UNLOCK_PFN (OldIrql);
        return STATUS_NO_MEMORY;
    }

    if (MmFirstFreeSystemCache == MmFlushSystemCache) {

        //
        // All system cache PTEs have been used, flush the entire
        // TB to remove any stale TB entries.
        //

        KeFlushEntireTb (TRUE, TRUE);
        MmFlushSystemCache = NULL;
    }

    PointerPte = MmFirstFreeSystemCache;

    //
    // Update next free entry.
    //

    ASSERT (PointerPte->u.Hard.Valid == 0);

    if (PointerPte->u.List.NextEntry == MM_EMPTY_PTE_LIST) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x778,
                      (ULONG_PTR)PointerPte,
                      0,
                      0);
        MmFirstFreeSystemCache = (PMMPTE)MM_EMPTY_LIST;
    }
    else {
        MmFirstFreeSystemCache = MmSystemCachePteBase + PointerPte->u.List.NextEntry;
        ASSERT (MmFirstFreeSystemCache <= MiGetPteAddress (MmSystemCacheEnd));
    }

    //
    // Increment the count of the number of views for the
    // section object.  This requires the PFN lock to be held.
    //

    ControlArea->NumberOfMappedViews += 1;
    ControlArea->NumberOfSystemCacheViews += 1;
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    UNLOCK_PFN (OldIrql);

    *CapturedBase = MiGetVirtualAddressMappedByPte (PointerPte);

    EndingVa = (PVOID)(((ULONG_PTR)*CapturedBase +
                                *CapturedViewSize - 1L) | (PAGE_SIZE - 1L));

    //
    // An unoccupied address range has been found, put the PTEs in
    // the range into prototype PTEs.
    //

#if DBG

    //
    //  Zero out the next pointer field.
    //

    PointerPte->u.List.NextEntry = 0;
#endif //DBG

    LastPte = MiGetPteAddress (EndingVa);

    //
    // Calculate the first prototype PTE address.
    //

    PteOffset = (ULONG)(SectionOffset->QuadPart >> PAGE_SHIFT);

    //
    // Make sure the PTEs are not in the extended part of the
    // segment.
    //

    while (PteOffset >= Subsection->PtesInSubsection) {
        PteOffset -= Subsection->PtesInSubsection;
        Subsection = Subsection->NextSubsection;
    }

    ProtoPte = &Subsection->SubsectionBase[PteOffset];

    LastProto = &Subsection->SubsectionBase[Subsection->PtesInSubsection];

    while (PointerPte <= LastPte) {

        if (ProtoPte >= LastProto) {

            //
            // Handle extended subsections.
            //

            Subsection = Subsection->NextSubsection;
            ProtoPte = Subsection->SubsectionBase;
            LastProto = &Subsection->SubsectionBase[
                                        Subsection->PtesInSubsection];
        }
        ASSERT (PointerPte->u.Long == ZeroKernelPte.u.Long);
        PointerPte->u.Long = MiProtoAddressForKernelPte (ProtoPte);

        ASSERT (((ULONG_PTR)PointerPte & (MM_COLOR_MASK << PTE_SHIFT)) ==
                 (((ULONG_PTR)ProtoPte  & (MM_COLOR_MASK << PTE_SHIFT))));

        PointerPte += 1;
        ProtoPte += 1;
    }

    return STATUS_SUCCESS;
}

VOID
MiAddMappedPtes (
    IN PMMPTE FirstPte,
    IN ULONG NumberOfPtes,
    IN PCONTROL_AREA ControlArea
    )

/*++

Routine Description:

    This function maps a view in the current address space to the
    specified control area.  The page protection is identical to that
    of the prototype PTE.

    This routine assumes the caller has called MiCheckPurgeAndUpMapCount,
    hence the PFN lock is not needed here.

Arguments:

    FirstPte - Supplies a pointer to the first PTE of the current address
               space to initialize.

    NumberOfPtes - Supplies the number of PTEs to initialize.

    ControlArea - Supplies the control area to point the PTEs at.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    PMMPTE ProtoPte;
    PMMPTE LastProto;
    PMMPTE LastPte;
    PSUBSECTION Subsection;

    if (ControlArea->u.Flags.GlobalOnlyPerSession == 0) {
        Subsection = (PSUBSECTION)(ControlArea + 1);
    }
    else {
        Subsection = (PSUBSECTION)((PLARGE_CONTROL_AREA)ControlArea + 1);
    }

    PointerPte = FirstPte;
    ASSERT (NumberOfPtes != 0);
    LastPte = FirstPte + NumberOfPtes;

    ASSERT (ControlArea->NumberOfMappedViews >= 1);
    ASSERT (ControlArea->NumberOfUserReferences >= 1);
    ASSERT (ControlArea->u.Flags.HadUserReference == 1);
    ASSERT (ControlArea->NumberOfSectionReferences != 0);

    ASSERT (ControlArea->u.Flags.BeingCreated == 0);
    ASSERT (ControlArea->u.Flags.BeingDeleted == 0);
    ASSERT (ControlArea->u.Flags.BeingPurged == 0);

    ProtoPte = Subsection->SubsectionBase;

    LastProto = &Subsection->SubsectionBase[Subsection->PtesInSubsection];

    while (PointerPte < LastPte) {

        if (ProtoPte >= LastProto) {

            //
            // Handle extended subsections.
            //

            Subsection = Subsection->NextSubsection;
            ProtoPte = Subsection->SubsectionBase;
            LastProto = &Subsection->SubsectionBase[
                                        Subsection->PtesInSubsection];
        }
        ASSERT (PointerPte->u.Long == ZeroKernelPte.u.Long);
        PointerPte->u.Long = MiProtoAddressForKernelPte (ProtoPte);

        ASSERT (((ULONG_PTR)PointerPte & (MM_COLOR_MASK << PTE_SHIFT)) ==
                 (((ULONG_PTR)ProtoPte  & (MM_COLOR_MASK << PTE_SHIFT))));

        PointerPte += 1;
        ProtoPte += 1;
    }

    return;
}

VOID
MmUnmapViewInSystemCache (
    IN PVOID BaseAddress,
    IN PVOID SectionToUnmap,
    IN ULONG AddToFront
    )

/*++

Routine Description:

    This function unmaps a view from the system cache.

    NOTE: When this function is called, no pages may be locked in
    the cache for the specified view.

Arguments:

    BaseAddress - Supplies the base address of the section in the
                  system cache.

    SectionToUnmap - Supplies a pointer to the section which the
                     base address maps.

    AddToFront - Supplies TRUE if the unmapped pages should be
                 added to the front of the standby list (i.e., their
                 value in the cache is low).  FALSE otherwise

Return Value:

    none.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    PMMPTE FirstPte;
    MMPTE PteContents;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;
    PFN_NUMBER i;
    WSLE_NUMBER WorkingSetIndex;
    PCONTROL_AREA ControlArea;
    ULONG WsHeld;
    PFN_NUMBER PdeFrameNumber;

    WsHeld = FALSE;

    ASSERT (KeGetCurrentIrql() <= APC_LEVEL);

    PointerPte = MiGetPteAddress (BaseAddress);
    FirstPte = PointerPte;
    ControlArea = ((PSECTION)SectionToUnmap)->Segment->ControlArea;
    PdeFrameNumber = MI_GET_PAGE_FRAME_FROM_PTE (MiGetPteAddress (PointerPte));

    //
    // Get the control area for the segment which is mapped here.
    //

    i = 0;

    do {

        //
        // The cache is organized in chunks of 256k bytes, clear
        // the first chunk then check to see if this is the last
        // chunk.
        //

        //
        // The page table page is always resident for the system cache.
        // Check each PTE, it is in one of two states, either valid or
        // prototype PTE format.
        //

        PteContents = *(volatile MMPTE *)PointerPte;
        if (PteContents.u.Hard.Valid == 1) {

            if (!WsHeld) {
                WsHeld = TRUE;
                LOCK_SYSTEM_WS (OldIrqlWs);
                continue;
            }

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            WorkingSetIndex = MiLocateWsle (BaseAddress,
                                            MmSystemCacheWorkingSetList,
                                            Pfn1->u1.WsIndex );
            MiRemoveWsle (WorkingSetIndex,
                          MmSystemCacheWorkingSetList );
            MiReleaseWsle (WorkingSetIndex, &MmSystemCacheWs);

            MI_SET_PTE_IN_WORKING_SET (PointerPte, 0);

            //
            // The PTE is valid.
            //

            LOCK_PFN (OldIrql);

            //
            // Capture the state of the modified bit for this PTE.
            //

            MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            MiDecrementShareAndValidCount (PdeFrameNumber);

            //
            // Decrement the share count for the physical page.
            //

#if DBG
            if (ControlArea->NumberOfMappedViews == 1) {
                PMMPFN Pfn;
                Pfn = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);
                ASSERT (Pfn->u2.ShareCount == 1);
            }
#endif //DBG


            MmFrontOfList = AddToFront;
            MiDecrementShareCount (MI_GET_PAGE_FRAME_FROM_PTE (&PteContents));
            MmFrontOfList = FALSE;
            UNLOCK_PFN (OldIrql);
        } else {
            if (WsHeld) {
                UNLOCK_SYSTEM_WS (OldIrqlWs);
                WsHeld = FALSE;
            }

            ASSERT ((PteContents.u.Long == ZeroKernelPte.u.Long) ||
                    (PteContents.u.Soft.Prototype == 1));
            NOTHING;
        }
        MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

        PointerPte += 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        i += 1;
    } while (i < (X256K / PAGE_SIZE));

    if (WsHeld) {
        UNLOCK_SYSTEM_WS (OldIrqlWs);
    }

    FirstPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;

    LOCK_PFN (OldIrql);

    //
    // Free this entry to the end of the list.
    //

    if (MmFlushSystemCache == NULL) {

        //
        // If there is no entry marked to initiate a TB flush when
        // reused, mark this entry as the one.  This way the TB
        // only needs to be flushed when the list wraps.
        //

        MmFlushSystemCache = FirstPte;
    }

    MmLastFreeSystemCache->u.List.NextEntry = FirstPte - MmSystemCachePteBase;
    MmLastFreeSystemCache = FirstPte;

    //
    // Decrement the number of mapped views for the segment
    // and check to see if the segment should be deleted.
    //

    ControlArea->NumberOfMappedViews -= 1;
    ControlArea->NumberOfSystemCacheViews -= 1;

    //
    // Check to see if the control area (segment) should be deleted.
    // This routine releases the PFN lock.
    //

    MiCheckControlArea (ControlArea, NULL, OldIrql);

    return;
}


VOID
MiRemoveMappedPtes (
    IN PVOID BaseAddress,
    IN ULONG NumberOfPtes,
    IN PCONTROL_AREA ControlArea,
    IN PMMSUPPORT WorkingSetInfo
    )

/*++

Routine Description:

    This function unmaps a view from the system cache or a session space.

    NOTE: When this function is called, no pages may be locked in
    the cache (or session space) for the specified view.

Arguments:

    BaseAddress - Supplies the base address of the section in the
                  system cache or session space.

    NumberOfPtes - Supplies the number of PTEs to unmap.

    ControlArea - Supplies the control area mapping the view.

    WorkingSetInfo - Supplies the charged working set structures.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPFN Pfn1;
    PMMPTE FirstPte;
    MMPTE PteContents;
    KIRQL OldIrql;
    KIRQL OldIrqlWs;
    ULONG WorkingSetIndex;
    ULONG DereferenceSegment;
    MMPTE_FLUSH_LIST PteFlushList;
    ULONG WsHeld;

    DereferenceSegment = FALSE;
    WsHeld = FALSE;

    PteFlushList.Count = 0;
    PointerPte = MiGetPteAddress (BaseAddress);
    FirstPte = PointerPte;

    //
    // Get the control area for the segment which is mapped here.
    //

    while (NumberOfPtes) {

        //
        // The cache is organized in chunks of 256k bytes, clear
        // the first chunk, then check to see if this is the last
        // chunk.
        //

        //
        // The page table page is always resident for the system cache (and
        // for a session space map).
        //
        // Check each PTE, it is in one of two states, either valid or
        // prototype PTE format.
        //

        PteContents = *PointerPte;
        if (PteContents.u.Hard.Valid == 1) {

            //
            // The system cache is locked by us, all others are locked by
            // the caller.
            //

            if (WorkingSetInfo == &MmSystemCacheWs) {
                if (!WsHeld) {
                    WsHeld = TRUE;
                    LOCK_SYSTEM_WS (OldIrqlWs);
                    continue;
                }
            }

            Pfn1 = MI_PFN_ELEMENT (PteContents.u.Hard.PageFrameNumber);

            WorkingSetIndex = MiLocateWsle (BaseAddress,
                                            WorkingSetInfo->VmWorkingSetList,
                                            Pfn1->u1.WsIndex );
            ASSERT (WorkingSetIndex != WSLE_NULL_INDEX);

            MiRemoveWsle (WorkingSetIndex,
                          WorkingSetInfo->VmWorkingSetList );
            MiReleaseWsle (WorkingSetIndex, WorkingSetInfo);

            MI_SET_PTE_IN_WORKING_SET (PointerPte, 0);

            LOCK_PFN (OldIrql);

            //
            // The PTE is valid.
            //

            //
            // Capture the state of the modified bit for this PTE.
            //

            MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

            //
            // Flush the TB for this page.
            //

            if (PteFlushList.Count != MM_MAXIMUM_FLUSH_COUNT) {
                PteFlushList.FlushPte[PteFlushList.Count] = PointerPte;
                PteFlushList.FlushVa[PteFlushList.Count] = BaseAddress;
                PteFlushList.Count += 1;
            }

            PointerPde = MiGetPteAddress (PointerPte);

#if !defined (_WIN64)

            //
            // The PDE must be carefully checked against the master table
            // because the PDEs are all zeroed in process creation.  If this
            // process has never faulted on any address in this range (all
            // references prior and above were filled directly by the TB as
            // the PTEs are global on non-Hydra), then the PDE reference
            // below to determine the page table frame will be zero.
            //
            // Note this cannot happen on NT64 as no master table is used.
            //

            if (PointerPde->u.Long == 0) {

                PMMPTE MasterPde;

                ASSERT (MiHydra == FALSE);

#if !defined (_X86PAE_)
                MasterPde = &MmSystemPagePtes [((ULONG_PTR)PointerPde &
                             ((sizeof(MMPTE) * PDE_PER_PAGE) - 1)) / sizeof(MMPTE)];
#else
                MasterPde = &MmSystemPagePtes [((ULONG_PTR)PointerPde &
                             (PD_PER_SYSTEM * (sizeof(MMPTE) * PDE_PER_PAGE) - 1)) / sizeof(MMPTE)];
#endif
                ASSERT (MasterPde->u.Hard.Valid == 1);
                MI_WRITE_VALID_PTE (PointerPde, *MasterPde);
            }
#endif

            //
            // Decrement the share and valid counts of the page table
            // page which maps this PTE.
            //

            MiDecrementShareAndValidCount (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde));

            //
            // Decrement the share count for the physical page.
            //

            MiDecrementShareCount (MI_GET_PAGE_FRAME_FROM_PTE (&PteContents));
            UNLOCK_PFN (OldIrql);

        } else {
            if (WorkingSetInfo == &MmSystemCacheWs) {
                if (WsHeld) {
                    UNLOCK_SYSTEM_WS (OldIrqlWs);
                    WsHeld = FALSE;
                }
            }

            ASSERT ((PteContents.u.Long == ZeroKernelPte.u.Long) ||
                    (PteContents.u.Soft.Prototype == 1));
            NOTHING;
        }
        MI_WRITE_INVALID_PTE (PointerPte, ZeroKernelPte);

        PointerPte += 1;
        BaseAddress = (PVOID)((PCHAR)BaseAddress + PAGE_SIZE);
        NumberOfPtes -= 1;
    }

    if (WorkingSetInfo == &MmSystemCacheWs) {
        if (WsHeld) {
            UNLOCK_SYSTEM_WS (OldIrqlWs);
        }
    }
    LOCK_PFN (OldIrql);

    MiFlushPteList (&PteFlushList, TRUE, ZeroKernelPte);

    if (WorkingSetInfo != &MmSystemCacheWs) {

        //
        // Session space has no ASN - flush the entire TB.
        //
    
        MI_FLUSH_ENTIRE_SESSION_TB (TRUE, TRUE);
    }

    //
    // Decrement the number of user references as the caller upped them
    // via MiCheckPurgeAndUpMapCount when this was originally mapped.
    //

    ControlArea->NumberOfUserReferences -= 1;

    //
    // Decrement the number of mapped views for the segment
    // and check to see if the segment should be deleted.
    //

    ControlArea->NumberOfMappedViews -= 1;

    //
    // Check to see if the control area (segment) should be deleted.
    // This routine releases the PFN lock.
    //

    MiCheckControlArea (ControlArea, NULL, OldIrql);
}

VOID
MiInitializeSystemCache (
    IN ULONG MinimumWorkingSet,
    IN ULONG MaximumWorkingSet
    )

/*++

Routine Description:

    This routine initializes the system cache working set and
    data management structures.

Arguments:

    MinimumWorkingSet - Supplies the minimum working set for the system
                        cache.

    MaximumWorkingSet - Supplies the maximum working set size for the
                        system cache.

Return Value:

    None.

Environment:

    Kernel mode, called only at phase 0 initialization.

--*/

{
    ULONG SizeOfSystemCacheInPages;
    ULONG HunksOf256KInCache;
    PMMWSLE WslEntry;
    ULONG NumberOfEntriesMapped;
    PFN_NUMBER i;
    MMPTE PteContents;
    PMMPTE PointerPte;
    KIRQL OldIrql;

    PointerPte = MiGetPteAddress (MmSystemCacheWorkingSetList);

    PteContents = ValidKernelPte;

    LOCK_PFN (OldIrql);

    i = MiRemoveZeroPage(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));

    PteContents.u.Hard.PageFrameNumber = i;

    MI_WRITE_VALID_PTE (PointerPte, PteContents);

    MiInitializePfn (i, PointerPte, 1L);

    UNLOCK_PFN (OldIrql);

#if defined (_WIN64)
    MmSystemCacheWsle = (PMMWSLE)(MmSystemCacheWorkingSetList + 1);
#else
    MmSystemCacheWsle =
            (PMMWSLE)(&MmSystemCacheWorkingSetList->UsedPageTableEntries[0]);
#endif

    MmSystemCacheWs.VmWorkingSetList = MmSystemCacheWorkingSetList;
    MmSystemCacheWs.WorkingSetSize = 0;
    MmSystemCacheWs.MinimumWorkingSetSize = MinimumWorkingSet;
    MmSystemCacheWs.MaximumWorkingSetSize = MaximumWorkingSet;
    InsertTailList (&MmWorkingSetExpansionHead.ListHead,
                    &MmSystemCacheWs.WorkingSetExpansionLinks);

    MmSystemCacheWs.AllowWorkingSetAdjustment = TRUE;

    //
    // Don't use entry 0 as an index of zero in the PFN database
    // means that the page can be assigned to a slot.  This is not
    // a problem for process working sets as page 0 is private.
    //

    MmSystemCacheWorkingSetList->FirstFree = 1;
    MmSystemCacheWorkingSetList->FirstDynamic = 1;
    MmSystemCacheWorkingSetList->NextSlot = 1;
    MmSystemCacheWorkingSetList->LastEntry = (ULONG)MmSystemCacheWsMinimum;
    MmSystemCacheWorkingSetList->Quota = MmSystemCacheWorkingSetList->LastEntry;
    MmSystemCacheWorkingSetList->HashTable = NULL;
    MmSystemCacheWorkingSetList->HashTableSize = 0;
    MmSystemCacheWorkingSetList->Wsle = MmSystemCacheWsle;

    MmSystemCacheWorkingSetList->HashTableStart = 
       (PVOID)((PCHAR)PAGE_ALIGN (&MmSystemCacheWorkingSetList->Wsle[MM_MAXIMUM_WORKING_SET]) + PAGE_SIZE);

    MmSystemCacheWorkingSetList->HighestPermittedHashAddress = (PVOID)(MM_SYSTEM_CACHE_START);

    NumberOfEntriesMapped = (ULONG)(((PMMWSLE)((PCHAR)MmSystemCacheWorkingSetList +
                                PAGE_SIZE)) - MmSystemCacheWsle);

    LOCK_PFN (OldIrql);

    while (NumberOfEntriesMapped < MmSystemCacheWsMaximum) {

        PointerPte += 1;
        i = MiRemoveZeroPage(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
        PteContents.u.Hard.PageFrameNumber = i;
        MI_WRITE_VALID_PTE (PointerPte, PteContents);
        MiInitializePfn (i, PointerPte, 1L);
        NumberOfEntriesMapped += PAGE_SIZE / sizeof(MMWSLE);
    }

    UNLOCK_PFN (OldIrql);

    //
    // Initialize the following slots as free.
    //

    WslEntry = MmSystemCacheWsle + 1;

    for (i = 1; i < NumberOfEntriesMapped; i++) {

        //
        // Build the free list, note that the first working
        // set entries (CurrentEntry) are not on the free list.
        // These entries are reserved for the pages which
        // map the working set and the page which contains the PDE.
        //

        WslEntry->u1.Long = (i + 1) << MM_FREE_WSLE_SHIFT;
        WslEntry += 1;
    }

    WslEntry -= 1;
    WslEntry->u1.Long = WSLE_NULL_INDEX << MM_FREE_WSLE_SHIFT;  // End of list.

    MmSystemCacheWorkingSetList->LastInitializedWsle = NumberOfEntriesMapped - 1;

    //
    // Build a free list structure in the PTEs for the system cache.
    //

    MmSystemCachePteBase = MI_PTE_BASE_FOR_LOWEST_KERNEL_ADDRESS;

    SizeOfSystemCacheInPages = COMPUTE_PAGES_SPANNED (MmSystemCacheStart,
                                (PCHAR)MmSystemCacheEnd - (PCHAR)MmSystemCacheStart + 1);

    HunksOf256KInCache = SizeOfSystemCacheInPages / (X256K / PAGE_SIZE);

    PointerPte = MiGetPteAddress (MmSystemCacheStart);

    MmFirstFreeSystemCache = PointerPte;

    for (i = 0; i < HunksOf256KInCache; i += 1) {
        PointerPte->u.List.NextEntry = (PointerPte + (X256K / PAGE_SIZE)) - MmSystemCachePteBase;
        PointerPte += X256K / PAGE_SIZE;
    }

    PointerPte -= X256K / PAGE_SIZE;

#if defined(_X86_)

    //
    // Add any extended ranges.
    //

    if (MiSystemCacheEndExtra != MmSystemCacheEnd) {

        SizeOfSystemCacheInPages = COMPUTE_PAGES_SPANNED (MiSystemCacheStartExtra,
                                    (PCHAR)MiSystemCacheEndExtra - (PCHAR)MiSystemCacheStartExtra + 1);
    
        HunksOf256KInCache = SizeOfSystemCacheInPages / (X256K / PAGE_SIZE);
    
        if (HunksOf256KInCache) {

            PMMPTE PointerPteExtended;
    
            PointerPteExtended = MiGetPteAddress (MiSystemCacheStartExtra);
            PointerPte->u.List.NextEntry = PointerPteExtended - MmSystemCachePteBase;
            PointerPte = PointerPteExtended;

            for (i = 0; i < HunksOf256KInCache; i += 1) {
                PointerPte->u.List.NextEntry = (PointerPte + (X256K / PAGE_SIZE)) - MmSystemCachePteBase;
                PointerPte += X256K / PAGE_SIZE;
            }
    
            PointerPte -= X256K / PAGE_SIZE;
        }
    }
#endif

    PointerPte->u.List.NextEntry = MM_EMPTY_PTE_LIST;
    MmLastFreeSystemCache = PointerPte;

    if (MaximumWorkingSet > ((1536*1024) >> PAGE_SHIFT)) {

        //
        // The working set list consists of more than a single page.
        //

        LOCK_SYSTEM_WS (OldIrql);
        MiGrowWsleHash (&MmSystemCacheWs);
        UNLOCK_SYSTEM_WS (OldIrql);
    }
}

BOOLEAN
MmCheckCachedPageState (
    IN PVOID Address,
    IN BOOLEAN SetToZero
    )

/*++

Routine Description:

    This routine checks the state of the specified page that is mapped in
    the system cache.  If the specified virtual address can be made valid
    (i.e., the page is already in memory), it is made valid and the value
    TRUE is returned.

    If the page is not in memory, and SetToZero is FALSE, the
    value FALSE is returned.  However, if SetToZero is TRUE, a page of
    zeroes is materialized for the specified virtual address and the address
    is made valid and the value TRUE is returned.

    This routine is for usage by the cache manager.

Arguments:

    Address - Supplies the address of a page mapped in the system cache.

    SetToZero - Supplies TRUE if a page of zeroes should be created in the
                case where no page is already mapped.

Return Value:

    FALSE if there if touching this page would cause a page fault resulting
          in a page read.

    TRUE if there is a physical page in memory for this address.

Environment:

    Kernel mode.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE ProtoPte;
    PFN_NUMBER PageFrameIndex;
    WSLE_NUMBER WorkingSetIndex;
    MMPTE TempPte;
    MMPTE ProtoPteContents;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    LOGICAL BarrierNeeded;
    ULONG BarrierStamp;

    BarrierNeeded = FALSE;

    PointerPte = MiGetPteAddress (Address);

    //
    // Make the PTE valid if possible.
    //

    if (PointerPte->u.Hard.Valid == 1) {
        return TRUE;
    }

    LOCK_PFN (OldIrql);

    if (PointerPte->u.Hard.Valid == 1) {
        goto UnlockAndReturnTrue;
    }

    ASSERT (PointerPte->u.Soft.Prototype == 1);

    ProtoPte = MiPteToProto (PointerPte);

    //
    // Pte is not valid, check the state of the prototype PTE.
    //

    if (MiMakeSystemAddressValidPfn (ProtoPte)) {

        //
        // If page fault occurred, recheck state of original PTE.
        //

        if (PointerPte->u.Hard.Valid == 1) {
            goto UnlockAndReturnTrue;
        }
    }

    ProtoPteContents = *ProtoPte;

    if (ProtoPteContents.u.Hard.Valid == 1) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // The prototype PTE is valid, make the cache PTE
        // valid and add it to the working set.
        //

        TempPte = ProtoPteContents;

    } else if ((ProtoPteContents.u.Soft.Transition == 1) &&
               (ProtoPteContents.u.Soft.Prototype == 0)) {

        //
        // Prototype PTE is in the transition state.  Remove the page
        // from the page list and make it valid.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        if ((Pfn1->u3.e1.ReadInProgress) ||
            (Pfn1->u3.e1.InPageError)) {

            //
            // Collided page fault, return.
            //

            goto UnlockAndReturnTrue;
        }

        MiUnlinkPageFromList (Pfn1);

        Pfn1->u3.e2.ReferenceCount += 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL );

        MI_WRITE_VALID_PTE (ProtoPte, TempPte);

        //
        // Increment the valid PTE count for the page containing
        // the prototype PTE.
        //

        Pfn2 = MI_PFN_ELEMENT (Pfn1->PteFrame);

    } else {

        //
        // Page is not in memory, if a page of zeroes is requested,
        // get a page of zeroes and make it valid.
        //

        if ((SetToZero == FALSE) || (MmAvailablePages < 8)) {
            UNLOCK_PFN (OldIrql);

            //
            // Fault the page into memory.
            //

            MmAccessFault (FALSE, Address, KernelMode, (PVOID)0);
            return FALSE;
        }

        //
        // Increment the count of Pfn references for the control area
        // corresponding to this file.
        //

        MiGetSubsectionAddress (
                    ProtoPte)->ControlArea->NumberOfPfnReferences += 1;

        PageFrameIndex = MiRemoveZeroPage(MI_GET_PAGE_COLOR_FROM_PTE (ProtoPte));

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // This barrier check is needed after zeroing the page and
        // before setting the PTE (not the prototype PTE) valid.
        // Capture it now, check it at the last possible moment.
        //

        BarrierNeeded = TRUE;
        BarrierStamp = (ULONG)Pfn1->PteFrame;

        MiInitializePfn (PageFrameIndex, ProtoPte, 1);
        Pfn1->u2.ShareCount = 0;
        Pfn1->u3.e1.PrototypePte = 1;

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL );

        MI_WRITE_VALID_PTE (ProtoPte, TempPte);
    }

    //
    // Increment the share count since the page is being put into a working
    // set.
    //

    Pfn1->u2.ShareCount += 1;

    if (Pfn1->u1.Event == NULL) {
        Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
    }

    //
    // Increment the reference count of the page table
    // page for this PTE.
    //

    PointerPde = MiGetPteAddress (PointerPte);
    Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

    Pfn2->u2.ShareCount += 1;

    MI_SET_GLOBAL_STATE (TempPte, 1);

#if defined (_WIN64)
    if (MI_DETERMINE_OWNER (PointerPte) == 0) {
        TempPte.u.Long &= ~MM_PTE_OWNER_MASK;
    }
#else
    TempPte.u.Hard.Owner = MI_DETERMINE_OWNER (PointerPte);
#endif

    if (BarrierNeeded) {
        MI_BARRIER_SYNCHRONIZE (BarrierStamp);
    }

    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    UNLOCK_PFN (OldIrql);

    LOCK_SYSTEM_WS (OldIrql);

    WorkingSetIndex = MiLocateAndReserveWsle (&MmSystemCacheWs);

    MiUpdateWsle (&WorkingSetIndex,
                  MiGetVirtualAddressMappedByPte (PointerPte),
                  MmSystemCacheWorkingSetList,
                  Pfn1);

    MmSystemCacheWsle[WorkingSetIndex].u1.e1.SameProtectAsProto = 1;

    MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);

    UNLOCK_SYSTEM_WS (OldIrql);

    return TRUE;

UnlockAndReturnTrue:
    UNLOCK_PFN (OldIrql);
    return TRUE;
}

NTSTATUS
MmCopyToCachedPage (
    IN PVOID Address,
    IN PVOID UserBuffer,
    IN ULONG Offset,
    IN SIZE_T CountInBytes,
    IN BOOLEAN DontZero
    )

/*++

Routine Description:

    This routine checks the state of the specified page that is mapped in
    the system cache.  If the specified virtual address can be made valid
    (i.e., the page is already in memory), it is made valid and the value
    TRUE is returned.

    If the page is not in memory, and SetToZero is FALSE, the
    value FALSE is returned.  However, if SetToZero is TRUE, a page of
    zeroes is materialized for the specified virtual address and the address
    is made valid and the value TRUE is returned.

    This routine is for usage by the cache manager.

Arguments:

    Address - Supplies the address of a page mapped in the system cache.
              This MUST be a page aligned address!

    UserBuffer - Supplies the address of a user buffer to copy into the
                 system cache at the specified address + offset.

    Offset - Supplies the offset into the UserBuffer to copy the data.

    CountInBytes - Supplies the byte count to copy from the user buffer.

    DontZero - Supplies TRUE if the buffer should not be zeroed (the
               caller will track zeroing).  FALSE if it should be zeroed.

Return Value:

    Returns the status of the copy.

Environment:

    Kernel mode, <= APC_LEVEL.

--*/

{
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPTE ProtoPte;
    PFN_NUMBER PageFrameIndex;
    WSLE_NUMBER WorkingSetIndex;
    MMPTE TempPte;
    MMPTE ProtoPteContents;
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    KIRQL OldIrql;
    ULONG TransitionState;
    ULONG AddToWorkingSet;
    LOGICAL ShareCountUpped;
    SIZE_T EndFill;
    PVOID Buffer;
    NTSTATUS status;
    PMMINPAGE_SUPPORT Event;
    PCONTROL_AREA ControlArea;
    PETHREAD Thread;
    ULONG SavedState;
    LOGICAL ApcsExplicitlyBlocked;
    LOGICAL ApcNeeded;

    TransitionState = FALSE;
    AddToWorkingSet = FALSE;
    ApcsExplicitlyBlocked = FALSE;
    ApcNeeded = FALSE;

    ASSERT (((ULONG_PTR)Address & (PAGE_SIZE - 1)) == 0);
    ASSERT ((CountInBytes + Offset) <= PAGE_SIZE);
    ASSERT (KeGetCurrentIrql() < DISPATCH_LEVEL);

    PointerPte = MiGetPteAddress (Address);

    if (PointerPte->u.Hard.Valid == 1) {
        goto Copy;
    }

    //
    // Touch the user's buffer to make it resident.  This is required in
    // order to safely detect the case where both the system and user
    // address are pointing at the same physical page.  This case causes
    // a deadlock during the RtlCopyBytes if the inpage support block needed
    // to be allocated and the PTE for the user page is not valid.  This
    // potential deadlock is resolved because if the user page causes a
    // collided fault, the initiator thread is checked for.  If they are
    // the same, then an exception is thrown by the pager.
    //

    try {

        *(volatile CHAR *)UserBuffer;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    //
    // Make the PTE valid if possible.
    //

    LOCK_PFN (OldIrql);

Recheck:

    if (PointerPte->u.Hard.Valid == 1) {
        goto UnlockAndCopy;
    }

    ASSERT (PointerPte->u.Soft.Prototype == 1);

    ProtoPte = MiPteToProto (PointerPte);

    //
    // Pte is not valid, check the state of the prototype PTE.
    //

    if (MiMakeSystemAddressValidPfn (ProtoPte)) {

        //
        // If page fault occurred, recheck state of original PTE.
        //

        if (PointerPte->u.Hard.Valid == 1) {
            goto UnlockAndCopy;
        }
    }

    ShareCountUpped = FALSE;
    ProtoPteContents = *ProtoPte;

    if (ProtoPteContents.u.Hard.Valid == 1) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Increment the share count so the prototype PTE will remain
        // valid until this can be added into the system's working set.
        //

        Pfn1->u2.ShareCount += 1;
        ShareCountUpped = TRUE;

        //
        // The prototype PTE is valid, make the cache PTE
        // valid and add it to the working set.
        //

        TempPte = ProtoPteContents;

    } else if ((ProtoPteContents.u.Soft.Transition == 1) &&
               (ProtoPteContents.u.Soft.Prototype == 0)) {

        //
        // Prototype PTE is in the transition state.  Remove the page
        // from the page list and make it valid.
        //

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (&ProtoPteContents);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
        if ((Pfn1->u3.e1.ReadInProgress) ||
            (Pfn1->u3.e1.InPageError)) {

            //
            // Collided page fault or in page error, try the copy
            // operation incurring a page fault.
            //

            goto UnlockAndCopy;
        }

        MiUnlinkPageFromList (Pfn1);

        Pfn1->u3.e2.ReferenceCount += 1;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.Modified = 1;
        ASSERT (Pfn1->u2.ShareCount == 0);
        Pfn1->u2.ShareCount += 1;
        ShareCountUpped = TRUE;

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL );
        MI_SET_PTE_DIRTY (TempPte);

        MI_WRITE_VALID_PTE (ProtoPte, TempPte);

        //
        // Increment the valid pte count for the page containing
        // the prototype PTE.
        //

    } else {

        //
        // Page is not in memory, if a page of zeroes is requested,
        // get a page of zeroes and make it valid.
        //

        if (MiEnsureAvailablePageOrWait (NULL, NULL)) {

            //
            // A wait operation occurred which could have changed the
            // state of the PTE.  Recheck the PTE state.
            //

            goto Recheck;
        }

        Event = MiGetInPageSupportBlock ();
        if (Event == NULL) {
            UNLOCK_PFN (OldIrql);
            KeDelayExecutionThread (KernelMode, FALSE, (PLARGE_INTEGER)&MmShortTime);
            LOCK_PFN (OldIrql);
            goto Recheck;
        }

        //
        // Increment the count of Pfn references for the control area
        // corresponding to this file.
        //

        ControlArea = MiGetSubsectionAddress (ProtoPte)->ControlArea;
        ControlArea->NumberOfPfnReferences += 1;
        if (ControlArea->NumberOfUserReferences > 0) {

            //
            // There is a user reference to this file, always zero ahead.
            //

            DontZero = FALSE;
        }

        //
        // Remove any page from the list and turn it into a transition
        // page in the cache with read in progress set.  This causes
        // any other references to this page to block on the specified
        // event while the copy operation to the cache is on-going.
        //

        PageFrameIndex = MiRemoveAnyPage(MI_GET_PAGE_COLOR_FROM_PTE (ProtoPte));

        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Increment the valid PTE count for the page containing
        // the prototype PTE.
        //

        MiInitializeTransitionPfn (PageFrameIndex, ProtoPte, 0xFFFFFFFF);

        Pfn1->u2.ShareCount = 0;

        Pfn1->u3.e2.ReferenceCount = 0;     // for the add_locked_page macro
        MI_ADD_LOCKED_PAGE_CHARGE_FOR_MODIFIED_PAGE (Pfn1, 24);

        Pfn1->u3.e2.ReferenceCount = 1;
        Pfn1->u3.e1.PrototypePte = 1;
        Pfn1->u3.e1.Modified = 1;
        Pfn1->u3.e1.ReadInProgress = 1;
        Pfn1->u1.Event = &Event->Event;
        Event->Pfn = Pfn1;

        //
        // This is needed in case a special kernel APC fires that ends up
        // referencing the same page (this may even be through a different
        // virtual address from the user/system one here).
        //

        Thread = PsGetCurrentThread ();
        ASSERT (Thread->NestedFaultCount <= 1);
        Thread->NestedFaultCount += 1;

        TransitionState = TRUE;

        MI_MAKE_VALID_PTE (TempPte,
                           PageFrameIndex,
                           Pfn1->OriginalPte.u.Soft.Protection,
                           NULL);
        MI_SET_PTE_DIRTY (TempPte);

        //
        // APCs must be explicitly disabled to prevent suspend APCs from
        // interrupting this thread before the RtlCopyBytes completes.
        // Otherwise this page can remain in transition indefinitely (until
        // the suspend APC is released) which blocks any other threads that
        // may reference it.
        //

        KeEnterCriticalRegion();
        ApcsExplicitlyBlocked = TRUE;
    }

    //
    // Increment the share count of the page table page for this PTE.
    //

    PointerPde = MiGetPteAddress (PointerPte);
    Pfn2 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);

    Pfn2->u2.ShareCount += 1;

    MI_SET_GLOBAL_STATE (TempPte, 1);
#if defined (_WIN64)
    if (MI_DETERMINE_OWNER (PointerPte) == 0) {
        TempPte.u.Long &= ~MM_PTE_OWNER_MASK;
    }
#else
    TempPte.u.Hard.Owner = MI_DETERMINE_OWNER (PointerPte);
#endif
    MI_WRITE_VALID_PTE (PointerPte, TempPte);

    AddToWorkingSet = TRUE;

UnlockAndCopy:

    //
    // Unlock the PFN database and perform the copy.
    //

    UNLOCK_PFN (OldIrql);

Copy:

    Thread = PsGetCurrentThread ();
    MmSavePageFaultReadAhead( Thread, &SavedState );
    MmSetPageFaultReadAhead( Thread, 0 );
    status = STATUS_SUCCESS;

    //
    // Copy the user buffer into the cache under an exception handler.
    //

    try {

        Buffer = (PVOID)((PCHAR)Address + Offset);
        RtlCopyBytes (Buffer, UserBuffer, CountInBytes);

        if (TransitionState) {

            //
            // Only zero the memory outside the range if a page was taken
            // from the free list.
            //

            if (Offset != 0) {
                RtlZeroMemory (Address, Offset);
            }

            if (DontZero == FALSE) {
                EndFill = PAGE_SIZE - (Offset + CountInBytes);

                if (EndFill != 0) {
                    Buffer = (PVOID)((PCHAR)Buffer + CountInBytes);
                    RtlZeroMemory (Buffer, EndFill);
                }
            }
        }
    } except (MiMapCacheExceptionFilter (&status, GetExceptionInformation())) {

        if (status == STATUS_MULTIPLE_FAULT_VIOLATION) {
            ASSERT (TransitionState == TRUE);
        }

        //
        // Zero out the page if it came from the free list.
        //

        if (TransitionState) {
            RtlZeroMemory (Address, PAGE_SIZE);
        }
    }

    MmResetPageFaultReadAhead(Thread, SavedState);

    if (AddToWorkingSet) {

        LOCK_PFN (OldIrql);

        if (ApcsExplicitlyBlocked == TRUE) {
            KeLeaveCriticalRegion();
        }

        ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
        ASSERT (Pfn1->PteAddress == ProtoPte);

        if (TransitionState) {

            //
            // This is a newly allocated page.
            //

            ASSERT (ShareCountUpped == FALSE);
            ASSERT (Pfn1->u2.ShareCount <= 1);
            ASSERT (Pfn1->u1.Event == &Event->Event);

            MiMakeSystemAddressValidPfn (ProtoPte);
            MI_SET_GLOBAL_STATE (TempPte, 0);
            MI_WRITE_VALID_PTE (ProtoPte, TempPte);
            Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
            ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
            ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);

            ASSERT (Event->Completed == FALSE);
            Event->Completed = TRUE;

            ASSERT (Pfn1->u2.ShareCount == 0);
            MI_REMOVE_LOCKED_PAGE_CHARGE(Pfn1, 41);
            Pfn1->u3.e1.PageLocation = ActiveAndValid;

            ASSERT (Pfn1->u3.e1.ReadInProgress == 1);
            Pfn1->u3.e1.ReadInProgress = 0;

            //
            // Increment the share count since the page is
            // being put into a working set.
            //

            Pfn1->u2.ShareCount += 1;

            if (Event->WaitCount != 1) {
                Event->IoStatus.Status = STATUS_SUCCESS;
                Event->IoStatus.Information = 0;
                KeSetEvent (&Event->Event, 0, FALSE);
            }

            MiFreeInPageSupportBlock (Event);
            if (DontZero != FALSE) {
                MI_ADD_LOCKED_PAGE_CHARGE(Pfn1, 40);
                Pfn1->u3.e2.ReferenceCount += 1;
                status = STATUS_CACHE_PAGE_LOCKED;
            }

            ASSERT (Thread->NestedFaultCount <= 3);
            ASSERT (Thread->NestedFaultCount != 0);
    
            Thread->NestedFaultCount -= 1;

            if ((Thread->ApcNeeded == 1) && (Thread->NestedFaultCount == 0)) {
                ApcNeeded = TRUE;
                Thread->ApcNeeded = 0;
            }

        } else {

            //
            // This is either a frame that was originally on the transition list
            // or was already valid when this routine began execution.  Either
            // way, the share count (and therefore the systemwide locked pages
            // count) has been dealt with.
            //

            ASSERT (ShareCountUpped == TRUE);

            if (Pfn1->u1.Event == NULL) {
                Pfn1->u1.Event = (PVOID)PsGetCurrentThread();
            }
        }

        UNLOCK_PFN (OldIrql);

        LOCK_SYSTEM_WS (OldIrql);

        WorkingSetIndex = MiLocateAndReserveWsle (&MmSystemCacheWs);

        MiUpdateWsle (&WorkingSetIndex,
                      MiGetVirtualAddressMappedByPte (PointerPte),
                      MmSystemCacheWorkingSetList,
                      Pfn1);

        MmSystemCacheWsle[WorkingSetIndex].u1.e1.SameProtectAsProto = 1;

        MI_SET_PTE_IN_WORKING_SET (PointerPte, WorkingSetIndex);
    
        UNLOCK_SYSTEM_WS (OldIrql);

        if (ApcNeeded == TRUE) {
            ASSERT (OldIrql < APC_LEVEL);
            ASSERT (Thread->NestedFaultCount == 0);
            ASSERT (Thread->ApcNeeded == 0);
            KeRaiseIrql (APC_LEVEL, &OldIrql);
            IoRetryIrpCompletions ();
            KeLowerIrql (OldIrql);
        }
    }
    else {
        ASSERT (ApcsExplicitlyBlocked == FALSE);
    }

    return status;
}

LONG
MiMapCacheExceptionFilter (
    IN PNTSTATUS Status,
    IN PEXCEPTION_POINTERS ExceptionPointer
    )

/*++

Routine Description:

    This routine is a filter for exceptions during copying data
    from the user buffer to the system cache.  It stores the
    status code from the exception record into the status argument.
    In the case of an in page i/o error it returns the actual
    error code and in the case of an access violation it returns
    STATUS_INVALID_USER_BUFFER.

Arguments:

    Status - Returns the status from the exception record.

    ExceptionCode - Supplies the exception code to being checked.

Return Value:

    ULONG - returns EXCEPTION_EXECUTE_HANDLER

--*/

{
    NTSTATUS local;

    local = ExceptionPointer->ExceptionRecord->ExceptionCode;

    //
    // If the exception is STATUS_IN_PAGE_ERROR, get the I/O error code
    // from the exception record.
    //

    if (local == STATUS_IN_PAGE_ERROR) {
        if (ExceptionPointer->ExceptionRecord->NumberParameters >= 3) {
            local = (NTSTATUS)ExceptionPointer->ExceptionRecord->ExceptionInformation[2];
        }
    }

    if (local == STATUS_ACCESS_VIOLATION) {
        local = STATUS_INVALID_USER_BUFFER;
    }

    *Status = local;
    return EXCEPTION_EXECUTE_HANDLER;
}


VOID
MmUnlockCachedPage (
    IN PVOID AddressInCache
    )

/*++

Routine Description:

    This routine unlocks a previous locked cached page.

Arguments:

    AddressInCache - Supplies the address where the page was locked
                     in the system cache.  This must be the same
                     address that MmCopyToCachedPage was called with.

Return Value:

    None.

--*/

{
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    PointerPte = MiGetPteAddress (AddressInCache);

    ASSERT (PointerPte->u.Hard.Valid == 1);
    Pfn1 = MI_PFN_ELEMENT (PointerPte->u.Hard.PageFrameNumber);

    LOCK_PFN (OldIrql);

    if (Pfn1->u3.e2.ReferenceCount <= 1) {
        KeBugCheckEx (MEMORY_MANAGEMENT,
                      0x777,
                      (ULONG_PTR)PointerPte->u.Hard.PageFrameNumber,
                      Pfn1->u3.e2.ReferenceCount,
                      (ULONG_PTR)AddressInCache);
        return;
    }

    MI_REMOVE_LOCKED_PAGE_CHARGE(Pfn1, 25);
    Pfn1->u3.e2.ReferenceCount -= 1;

    UNLOCK_PFN (OldIrql);
    return;
}
