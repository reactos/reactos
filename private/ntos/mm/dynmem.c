/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    dynmem.c

Abstract:

    This module contains the routines which implement dynamically adding
    and removing physical memory from the system.

Author:

    Landy Wang (landyw) 05-Feb-1999

Revision History:

--*/

#include "mi.h"

FAST_MUTEX MmDynamicMemoryMutex;

LOGICAL MiTrimRemovalPagesOnly = FALSE;

#if DBG
ULONG MiShowStuckPages;
ULONG MiDynmemData[9];
#endif

#define PFN_REMOVED     ((PMMPTE)(INT_PTR)(int)0x99887766)

PFN_COUNT
MiRemovePhysicalPages (
    IN PFN_NUMBER StartPage,
    IN PFN_NUMBER EndPage
    );


NTSTATUS
MmAddPhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )

/*++

Routine Description:

    This routine adds the specified physical address range to the system.
    This includes initializing PFN database entries and adding it to the
    freelists.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being added.
                     If any bytes were added (ie: STATUS_SUCCESS is being
                     returned), the actual amount is returned here.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    LOGICAL Inserted;
    LOGICAL Updated;
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPTE LastPte;
    PFN_NUMBER NumberOfPages;
    PFN_NUMBER start;
    PFN_NUMBER count;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_COUNT PagesNeeded;
    PPHYSICAL_MEMORY_DESCRIPTOR OldPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_DESCRIPTOR NewPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_RUN NewRun;
    LOGICAL PfnDatabaseIsPhysical;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT (BYTE_OFFSET(NumberOfBytes->LowPart) == 0);
    ASSERT (BYTE_OFFSET(StartAddress->LowPart) == 0);

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase)) {

        //
        // The system must be configured for dynamic memory addition.  This is
        // critical as only then is the database guaranteed to be non-sparse.
        //
    
        if (MmDynamicPfn == FALSE) {
            return STATUS_NOT_SUPPORTED;
        }

        PfnDatabaseIsPhysical = TRUE;
    }
    else {
        PfnDatabaseIsPhysical = FALSE;
    }

    StartPage = (PFN_NUMBER)(StartAddress->QuadPart >> PAGE_SHIFT);
    NumberOfPages = (PFN_NUMBER)(NumberOfBytes->QuadPart >> PAGE_SHIFT);

    EndPage = StartPage + NumberOfPages;

    if (EndPage - 1 > MmHighestPossiblePhysicalPage) {

        //
        // Truncate the request into something that can be mapped by the PFN
        // database.
        //

        EndPage = MmHighestPossiblePhysicalPage + 1;
        NumberOfPages = EndPage - StartPage;
    }

    //
    // The range cannot wrap.
    //

    if (StartPage >= EndPage) {
        return STATUS_INVALID_PARAMETER_1;
    }

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    i = (sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
         (sizeof(PHYSICAL_MEMORY_RUN) * (MmPhysicalMemoryBlock->NumberOfRuns + 1)));

    NewPhysicalMemoryBlock = ExAllocatePoolWithTag (NonPagedPool,
                                                    i,
                                                    '  mM');

    if (NewPhysicalMemoryBlock == NULL) {
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // The range cannot overlap any ranges that are already present.
    //

    start = 0;

    LOCK_PFN (OldIrql);

    do {

        count = MmPhysicalMemoryBlock->Run[start].PageCount;
        Page = MmPhysicalMemoryBlock->Run[start].BasePage;

        if (count != 0) {

            LastPage = Page + count;

            if ((StartPage < Page) && (EndPage > Page)) {
                UNLOCK_PFN (OldIrql);
                ExReleaseFastMutex (&MmDynamicMemoryMutex);
                ExFreePool (NewPhysicalMemoryBlock);
                return STATUS_CONFLICTING_ADDRESSES;
            }

            if ((StartPage >= Page) && (StartPage < LastPage)) {
                UNLOCK_PFN (OldIrql);
                ExReleaseFastMutex (&MmDynamicMemoryMutex);
                ExFreePool (NewPhysicalMemoryBlock);
                return STATUS_CONFLICTING_ADDRESSES;
            }
        }

        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    //
    // Fill any gaps in the (sparse) PFN database needed for these pages,
    // unless the PFN database was physically allocated and completely
    // committed up front.
    //

    PagesNeeded = 0;

    if (PfnDatabaseIsPhysical == FALSE) {
        PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(StartPage));
        LastPte = MiGetPteAddress ((PCHAR)(MI_PFN_ELEMENT(EndPage)) - 1);
    
        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
                PagesNeeded += 1;
            }
            PointerPte += 1;
        }
    
        if (MmAvailablePages < PagesNeeded) {
            UNLOCK_PFN (OldIrql);
            ExReleaseFastMutex (&MmDynamicMemoryMutex);
            ExFreePool (NewPhysicalMemoryBlock);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    
        TempPte = ValidKernelPte;
    
        PointerPte = MiGetPteAddress (MI_PFN_ELEMENT(StartPage));
    
        while (PointerPte <= LastPte) {
            if (PointerPte->u.Hard.Valid == 0) {
    
                PageFrameIndex = MiRemoveZeroPage(MI_GET_PAGE_COLOR_FROM_PTE (PointerPte));
    
                MiInitializePfn (PageFrameIndex, PointerPte, 0);
    
                TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
                *PointerPte = TempPte;
            }
            PointerPte += 1;
        }
        MmResidentAvailablePages -= PagesNeeded;
    }

    //
    // If the new range is adjacent to an existing range, just merge it into
    // the old block.  Otherwise use the new block as a new entry will have to
    // be used.
    //

    NewPhysicalMemoryBlock->NumberOfRuns = MmPhysicalMemoryBlock->NumberOfRuns + 1;
    NewPhysicalMemoryBlock->NumberOfPages = MmPhysicalMemoryBlock->NumberOfPages + NumberOfPages;

    NewRun = &NewPhysicalMemoryBlock->Run[0];
    start = 0;
    Inserted = FALSE;
    Updated = FALSE;

    do {

        Page = MmPhysicalMemoryBlock->Run[start].BasePage;
        count = MmPhysicalMemoryBlock->Run[start].PageCount;

        if (Inserted == FALSE) {

            //
            // Note overlaps into adjacent ranges were already checked above.
            //

            if (StartPage == Page + count) {
                MmPhysicalMemoryBlock->Run[start].PageCount += NumberOfPages;
                OldPhysicalMemoryBlock = NewPhysicalMemoryBlock;
                MmPhysicalMemoryBlock->NumberOfPages += NumberOfPages;

                //
                // Coalesce below and above to avoid leaving zero length gaps
                // as these gaps would prevent callers from removing ranges
                // the span them.
                //

                if (start + 1 < MmPhysicalMemoryBlock->NumberOfRuns) {

                    start += 1;
                    Page = MmPhysicalMemoryBlock->Run[start].BasePage;
                    count = MmPhysicalMemoryBlock->Run[start].PageCount;

                    if (StartPage + NumberOfPages == Page) {
                        MmPhysicalMemoryBlock->Run[start - 1].PageCount +=
                            count;
                        MmPhysicalMemoryBlock->NumberOfRuns -= 1;

                        //
                        // Copy any remaining entries.
                        //
    
                        if (start != MmPhysicalMemoryBlock->NumberOfRuns) {
                            RtlMoveMemory (&MmPhysicalMemoryBlock->Run[start],
                                           &MmPhysicalMemoryBlock->Run[start + 1],
                                           (MmPhysicalMemoryBlock->NumberOfRuns - start) * sizeof (PHYSICAL_MEMORY_RUN));
                        }
                    }
                }
                Updated = TRUE;
                break;
            }

            if (StartPage + NumberOfPages == Page) {
                MmPhysicalMemoryBlock->Run[start].BasePage = StartPage;
                MmPhysicalMemoryBlock->Run[start].PageCount += NumberOfPages;
                OldPhysicalMemoryBlock = NewPhysicalMemoryBlock;
                MmPhysicalMemoryBlock->NumberOfPages += NumberOfPages;
                Updated = TRUE;
                break;
            }

            if (StartPage + NumberOfPages <= Page) {

                if (start + 1 < MmPhysicalMemoryBlock->NumberOfRuns) {

                    if (StartPage + NumberOfPages <= MmPhysicalMemoryBlock->Run[start + 1].BasePage) {
                        //
                        // Don't insert here - the new entry really belongs
                        // (at least) one entry further down.
                        //

                        continue;
                    }
                }

                NewRun->BasePage = StartPage;
                NewRun->PageCount = NumberOfPages;
                NewRun += 1;
                Inserted = TRUE;
                Updated = TRUE;
            }
        }

        *NewRun = MmPhysicalMemoryBlock->Run[start];
        NewRun += 1;

        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    //
    // If the memory block has not been updated, then the new entry must
    // be added at the very end.
    //

    if (Updated == FALSE) {
        ASSERT (Inserted == FALSE);
        NewRun->BasePage = StartPage;
        NewRun->PageCount = NumberOfPages;
        Inserted = TRUE;
    }

    //
    // Repoint the MmPhysicalMemoryBlock at the new chunk, free the old after
    // releasing the PFN lock.
    //

    if (Inserted == TRUE) {
        OldPhysicalMemoryBlock = MmPhysicalMemoryBlock;
        MmPhysicalMemoryBlock = NewPhysicalMemoryBlock;
    }

    //
    // Note that the page directory (page parent entries on Win64) must be
    // filled in at system boot so that already-created processes do not fault
    // when referencing the new PFNs.
    //

    //
    // Walk through the memory descriptors and add pages to the
    // free list in the PFN database.
    //

    PageFrameIndex = StartPage;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    if (EndPage - 1 > MmHighestPhysicalPage) {
        MmHighestPhysicalPage = EndPage - 1;
    }

    while (PageFrameIndex < EndPage) {

        ASSERT (Pfn1->u2.ShareCount == 0);
        ASSERT (Pfn1->u3.e2.ShortFlags == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT64 (Pfn1->UsedPageTableEntries == 0);
        ASSERT (Pfn1->OriginalPte.u.Long == ZeroKernelPte.u.Long);
        ASSERT (Pfn1->PteFrame == 0);
        ASSERT ((Pfn1->PteAddress == PFN_REMOVED) ||
                (Pfn1->PteAddress == (PMMPTE)(UINT_PTR)0));

        //
        // Set the PTE address to the physical page for
        // virtual address alignment checking.
        //

        Pfn1->PteAddress = (PMMPTE)(PageFrameIndex << PTE_SHIFT);

        MiInsertPageInList (MmPageLocationList[FreePageList],
                            PageFrameIndex);

        PageFrameIndex += 1;
        
        Pfn1 += 1;
    }

    MmResidentAvailablePages += NumberOfPages;
    MmNumberOfPhysicalPages += (PFN_COUNT)NumberOfPages;

    UNLOCK_PFN (OldIrql);

    //
    // Increase all commit limits to reflect the additional memory.
    //

    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

    MmTotalCommitLimit += NumberOfPages;
    MmTotalCommitLimitMaximum += NumberOfPages;

    MmTotalCommittedPages += PagesNeeded;

    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    ExFreePool (OldPhysicalMemoryBlock);

    //
    // Indicate number of bytes actually added to our caller.
    //

    NumberOfBytes->QuadPart = (ULONGLONG)NumberOfPages * PAGE_SIZE;

    return STATUS_SUCCESS;
}


NTSTATUS
MmRemovePhysicalMemory (
    IN PPHYSICAL_ADDRESS StartAddress,
    IN OUT PLARGE_INTEGER NumberOfBytes
    )

/*++

Routine Description:

    This routine attempts to remove the specified physical address range
    from the system.

Arguments:

    StartAddress  - Supplies the starting physical address.

    NumberOfBytes  - Supplies a pointer to the number of bytes being removed.

Return Value:

    NTSTATUS.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    ULONG Additional;
    PFN_NUMBER Page;
    PFN_NUMBER LastPage;
    PFN_NUMBER OriginalLastPage;
    PFN_NUMBER start;
    PFN_NUMBER PagesReleased;
    PMMPFN Pfn1;
    PMMPFN StartPfn;
    PMMPFN EndPfn;
    KIRQL OldIrql;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    PFN_COUNT NumberOfPages;
    SPFN_NUMBER MaxPages;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER RemovedPages;
    LOGICAL Inserted;
    NTSTATUS Status;
    PMMPTE PointerPte;
    PMMPTE EndPte;
    PVOID VirtualAddress;
    PPHYSICAL_MEMORY_DESCRIPTOR OldPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_DESCRIPTOR NewPhysicalMemoryBlock;
    PPHYSICAL_MEMORY_RUN NewRun;
    LOGICAL PfnDatabaseIsPhysical;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ASSERT (BYTE_OFFSET(NumberOfBytes->LowPart) == 0);
    ASSERT (BYTE_OFFSET(StartAddress->LowPart) == 0);

    if (MI_IS_PHYSICAL_ADDRESS(MmPfnDatabase)) {

        //
        // The system must be configured for dynamic memory addition.  This is
        // not strictly required to remove the memory, but it's better to check
        // for it now under the assumption that the administrator is probably
        // going to want to add this range of memory back in - better to give
        // the error now and refuse the removal than to refuse the addition
        // later.
        //
    
        if (MmDynamicPfn == FALSE) {
            return STATUS_NOT_SUPPORTED;
        }

        PfnDatabaseIsPhysical = TRUE;
    }
    else {
        PfnDatabaseIsPhysical = FALSE;
    }

    StartPage = (PFN_NUMBER)(StartAddress->QuadPart >> PAGE_SHIFT);
    NumberOfPages = (PFN_COUNT)(NumberOfBytes->QuadPart >> PAGE_SHIFT);

    EndPage = StartPage + NumberOfPages;

    if (EndPage - 1 > MmHighestPossiblePhysicalPage) {

        //
        // Truncate the request into something that can be mapped by the PFN
        // database.
        //

        EndPage = MmHighestPossiblePhysicalPage + 1;
        NumberOfPages = (PFN_COUNT)(EndPage - StartPage);
    }

    //
    // The range cannot wrap.
    //

    if (StartPage >= EndPage) {
        return STATUS_INVALID_PARAMETER_1;
    }

    StartPfn = MI_PFN_ELEMENT (StartPage);
    EndPfn = MI_PFN_ELEMENT (EndPage);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

#if DBG
    MiDynmemData[0] += 1;
#endif

    //
    // Decrease all commit limits to reflect the removed memory.
    //

    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);

    ASSERT (MmTotalCommitLimit <= MmTotalCommitLimitMaximum);

    if ((NumberOfPages + 100 > MmTotalCommitLimit - MmTotalCommittedPages) ||
        (MmTotalCommittedPages > MmTotalCommitLimit)) {

#if DBG
        MiDynmemData[1] += 1;
#endif
        ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    MmTotalCommitLimit -= NumberOfPages;
    MmTotalCommitLimitMaximum -= NumberOfPages;

    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

    //
    // Check for outstanding promises that cannot be broken.
    //

    LOCK_PFN (OldIrql);

    MaxPages = MI_NONPAGABLE_MEMORY_AVAILABLE() - 100;

    if ((SPFN_NUMBER)NumberOfPages > MaxPages) {
#if DBG
        MiDynmemData[2] += 1;
#endif
        UNLOCK_PFN (OldIrql);
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto giveup2;
    }

    MmResidentAvailablePages -= NumberOfPages;
    MmNumberOfPhysicalPages -= NumberOfPages;

    //
    // The range must be contained in a single entry.  It is permissible for
    // it to be part of a single entry, but it must not cross multiple entries.
    //

    Additional = (ULONG)-2;

    start = 0;
    do {

        Page = MmPhysicalMemoryBlock->Run[start].BasePage;
        LastPage = Page + MmPhysicalMemoryBlock->Run[start].PageCount;

        if ((StartPage >= Page) && (EndPage <= LastPage)) {
            if ((StartPage == Page) && (EndPage == LastPage)) {
                Additional = (ULONG)-1;
            }
            else if ((StartPage == Page) || (EndPage == LastPage)) {
                Additional = 0;
            }
            else {
                Additional = 1;
            }
            break;
        }

        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    if (Additional == (ULONG)-2) {
#if DBG
        MiDynmemData[3] += 1;
#endif
        MmResidentAvailablePages += NumberOfPages;
        MmNumberOfPhysicalPages += NumberOfPages;
        UNLOCK_PFN (OldIrql);
        Status = STATUS_CONFLICTING_ADDRESSES;
        goto giveup2;
    }

    for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
        Pfn1->u3.e1.RemovalRequested = 1;
    }

    //
    // The free and zero lists must be pruned now before releasing the PFN
    // lock otherwise if another thread allocates the page from these lists,
    // the allocation will clear the RemovalRequested flag forever.
    //

    RemovedPages = MiRemovePhysicalPages (StartPage, EndPage);

    if (RemovedPages != NumberOfPages) {

#if DBG
retry:
#endif
    
        Pfn1 = StartPfn;
    
        InterlockedIncrement (&MiDelayPageFaults);
    
        for (i = 0; i < 5; i += 1) {
    
            UNLOCK_PFN (OldIrql);
    
            //
            // Attempt to move pages to the standby list.  Note that only the
            // pages with RemovalRequested set are moved.
            //
    
            MiTrimRemovalPagesOnly = TRUE;
    
            MiEmptyAllWorkingSets ();
    
            MiTrimRemovalPagesOnly = FALSE;
    
            MiFlushAllPages ();
    
            KeDelayExecutionThread (KernelMode, FALSE, &MmHalfSecond);
    
            LOCK_PFN (OldIrql);
    
            RemovedPages += MiRemovePhysicalPages (StartPage, EndPage);
    
            if (RemovedPages == NumberOfPages) {
                break;
            }
    
            //
            // RemovedPages doesn't include pages that were freed directly to
            // the bad page list via MiDecrementReferenceCount.  So use the above
            // check purely as an optimization - and walk here when necessary.
            //
    
            for ( ; Pfn1 < EndPfn; Pfn1 += 1) {
                if (Pfn1->u3.e1.PageLocation != BadPageList) {
                    break;
                }
            }
    
            if (Pfn1 == EndPfn) {
                RemovedPages = NumberOfPages;
                break;
            }
        }

        InterlockedDecrement (&MiDelayPageFaults);
    }

    if (RemovedPages != NumberOfPages) {
#if DBG
        MiDynmemData[4] += 1;
        if (MiShowStuckPages != 0) {

            RemovedPages = 0;
            for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
                if (Pfn1->u3.e1.PageLocation != BadPageList) {
                    RemovedPages += 1;
                }
            }

            ASSERT (RemovedPages != 0);

            DbgPrint("MmRemovePhysicalMemory : could not get %d of %d pages\n",
                RemovedPages, NumberOfPages);

            if (MiShowStuckPages & 0x2) {

                ULONG PfnsPrinted;
                ULONG EnoughShown;
                PMMPFN FirstPfn;
                PFN_COUNT PfnCount;

                PfnCount = 0;
                PfnsPrinted = 0;
                EnoughShown = 100;
    
                if (MiShowStuckPages & 0x4) {
                    EnoughShown = (ULONG)-1;
                }
    
                DbgPrint("Stuck PFN list: ");
                for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
                    if (Pfn1->u3.e1.PageLocation != BadPageList) {
                        if (PfnCount == 0) {
                            FirstPfn = Pfn1;
                        }
                        PfnCount += 1;
                    }
                    else {
                        if (PfnCount != 0) {
                            DbgPrint("%x -> %x ; ", FirstPfn - MmPfnDatabase,
                                                    (FirstPfn - MmPfnDatabase) + PfnCount - 1);
                            PfnsPrinted += 1;
                            if (PfnsPrinted == EnoughShown) {
                                break;
                            }
                            PfnCount = 0;
                        }
                    }
                }
                if (PfnCount != 0) {
                    DbgPrint("%x -> %x ; ", FirstPfn - MmPfnDatabase,
                                            (FirstPfn - MmPfnDatabase) + PfnCount - 1);
                }
                DbgPrint("\n");
            }
            if (MiShowStuckPages & 0x8) {
                DbgBreakPoint ();
            }
            if (MiShowStuckPages & 0x10) {
                goto retry;
            }
        }
#endif
        UNLOCK_PFN (OldIrql);
        Status = STATUS_NO_MEMORY;
        goto giveup;
    }

#if DBG
    for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {
        ASSERT (Pfn1->u3.e1.PageLocation == BadPageList);
    }
#endif

    //
    // All the pages in the range have been removed.  Update the physical
    // memory blocks and other associated housekeeping.
    //

    if (Additional == 0) {

        //
        // The range can be split off from an end of an existing chunk so no
        // pool growth or shrinkage is required.
        //

        NewPhysicalMemoryBlock = MmPhysicalMemoryBlock;
        OldPhysicalMemoryBlock = NULL;
    }
    else {

        //
        // The range cannot be split off from an end of an existing chunk so
        // pool growth or shrinkage is required.
        //

        UNLOCK_PFN (OldIrql);

        i = (sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
             (sizeof(PHYSICAL_MEMORY_RUN) * (MmPhysicalMemoryBlock->NumberOfRuns + Additional)));

        NewPhysicalMemoryBlock = ExAllocatePoolWithTag (NonPagedPool,
                                                        i,
                                                        '  mM');

        if (NewPhysicalMemoryBlock == NULL) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
#if DBG
            MiDynmemData[5] += 1;
#endif
            goto giveup;
        }

        OldPhysicalMemoryBlock = MmPhysicalMemoryBlock;
        RtlZeroMemory (NewPhysicalMemoryBlock, i);

        LOCK_PFN (OldIrql);
    }

    //
    // Remove or split the requested range from the existing memory block.
    //

    NewPhysicalMemoryBlock->NumberOfRuns = MmPhysicalMemoryBlock->NumberOfRuns + Additional;
    NewPhysicalMemoryBlock->NumberOfPages = MmPhysicalMemoryBlock->NumberOfPages - NumberOfPages;

    NewRun = &NewPhysicalMemoryBlock->Run[0];
    start = 0;
    Inserted = FALSE;

    do {

        Page = MmPhysicalMemoryBlock->Run[start].BasePage;
        LastPage = Page + MmPhysicalMemoryBlock->Run[start].PageCount;

        if (Inserted == FALSE) {

            if ((StartPage >= Page) && (EndPage <= LastPage)) {

                if ((StartPage == Page) && (EndPage == LastPage)) {
                    ASSERT (Additional == -1);
                    start += 1;
                    continue;
                }
                else if ((StartPage == Page) || (EndPage == LastPage)) {
                    ASSERT (Additional == 0);
                    if (StartPage == Page) {
                        MmPhysicalMemoryBlock->Run[start].BasePage += NumberOfPages;
                    }
                    MmPhysicalMemoryBlock->Run[start].PageCount -= NumberOfPages;
                }
                else {
                    ASSERT (Additional == 1);

                    OriginalLastPage = LastPage;

                    MmPhysicalMemoryBlock->Run[start].PageCount =
                        StartPage - MmPhysicalMemoryBlock->Run[start].BasePage;

                    *NewRun = MmPhysicalMemoryBlock->Run[start];
                    NewRun += 1;

                    NewRun->BasePage = EndPage;
                    NewRun->PageCount = OriginalLastPage - EndPage;
                    NewRun += 1;

                    start += 1;
                    continue;
                }

                Inserted = TRUE;
            }
        }

        *NewRun = MmPhysicalMemoryBlock->Run[start];
        NewRun += 1;
        start += 1;

    } while (start != MmPhysicalMemoryBlock->NumberOfRuns);

    //
    // Repoint the MmPhysicalMemoryBlock at the new chunk.
    // Free the old block after releasing the PFN lock.
    //

    MmPhysicalMemoryBlock = NewPhysicalMemoryBlock;

    if (EndPage - 1 == MmHighestPhysicalPage) {
        MmHighestPhysicalPage = StartPage - 1;
    }

    //
    // Throw away all the removed pages that are currently enqueued.
    //

    for (Pfn1 = StartPfn; Pfn1 < EndPfn; Pfn1 += 1) {

        ASSERT (Pfn1->u3.e1.PageLocation == BadPageList);
        ASSERT (Pfn1->u3.e1.RemovalRequested == 1);

        MiUnlinkPageFromList (Pfn1);

        ASSERT (Pfn1->u1.Flink == 0);
        ASSERT (Pfn1->u2.Blink == 0);
        ASSERT (Pfn1->u3.e2.ReferenceCount == 0);
        ASSERT64 (Pfn1->UsedPageTableEntries == 0);

        Pfn1->PteAddress = PFN_REMOVED;
        Pfn1->u3.e2.ShortFlags = 0;
        Pfn1->OriginalPte.u.Long = ZeroKernelPte.u.Long;
        Pfn1->PteFrame = 0;
    }

    //
    // Now that the removed pages have been discarded, eliminate the PFN
    // entries that mapped them.  Straddling entries left over from an
    // adjacent earlier removal are not collapsed at this point.
    //
    //

    PagesReleased = 0;

    if (PfnDatabaseIsPhysical == FALSE) {

        VirtualAddress = (PVOID)ROUND_TO_PAGES(MI_PFN_ELEMENT(StartPage));
        PointerPte = MiGetPteAddress (VirtualAddress);
        EndPte = MiGetPteAddress (PAGE_ALIGN(MI_PFN_ELEMENT(EndPage)));

        while (PointerPte < EndPte) {
            PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (PointerPte);
            Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
            ASSERT (Pfn1->u2.ShareCount == 1);
            ASSERT (Pfn1->u3.e2.ReferenceCount == 1);
            Pfn1->u2.ShareCount = 0;
            MI_SET_PFN_DELETED (Pfn1);
#if DBG
            Pfn1->u3.e1.PageLocation = StandbyPageList;
#endif //DBG
            MiDecrementReferenceCount (PageFrameIndex);
    
            KeFlushSingleTb (VirtualAddress,
                             TRUE,
                             TRUE,
                             (PHARDWARE_PTE)PointerPte,
                             ZeroKernelPte.u.Flush);
    
            PagesReleased += 1;
            PointerPte += 1;
            VirtualAddress = (PVOID)((PCHAR)VirtualAddress + PAGE_SIZE);
        }

        MmResidentAvailablePages += PagesReleased;
    }

#if DBG
    MiDynmemData[6] += 1;
#endif

    UNLOCK_PFN (OldIrql);

    if (PagesReleased != 0) {
        MiReturnCommitment (PagesReleased);
    }

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    if (OldPhysicalMemoryBlock != NULL) {
        ExFreePool (OldPhysicalMemoryBlock);
    }

    NumberOfBytes->QuadPart = (ULONGLONG)NumberOfPages * PAGE_SIZE;

    return STATUS_SUCCESS;

giveup:

    //
    // All the pages in the range were not obtained.  Back everything out.
    //

    PageFrameIndex = StartPage;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    LOCK_PFN (OldIrql);

    while (PageFrameIndex < EndPage) {

        ASSERT (Pfn1->u3.e1.RemovalRequested == 1);

        Pfn1->u3.e1.RemovalRequested = 0;

        if ((Pfn1->u3.e1.PageLocation == BadPageList) &&
            (Pfn1->u3.e1.ParityError == 0)) {

            MiUnlinkPageFromList (Pfn1);
            MiInsertPageInList (MmPageLocationList[FreePageList],
                                PageFrameIndex);
        }

        Pfn1 += 1;
        PageFrameIndex += 1;
    }

    MmResidentAvailablePages += NumberOfPages;
    MmNumberOfPhysicalPages += NumberOfPages;

    UNLOCK_PFN (OldIrql);

giveup2:

    ExAcquireSpinLock (&MmChargeCommitmentLock, &OldIrql);
    MmTotalCommitLimit += NumberOfPages;
    MmTotalCommitLimitMaximum += NumberOfPages;
    ExReleaseSpinLock (&MmChargeCommitmentLock, OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return Status;
}


PPHYSICAL_MEMORY_RANGE
MmGetPhysicalMemoryRanges (
    VOID
    )

/*++

Routine Description:

    This routine returns the virtual address of a nonpaged pool block which
    contains the physical memory ranges in the system.

    The returned block contains physical address and page count pairs.
    The last entry contains zero for both.

    The caller must understand that this block can change at any point before
    or after this snapshot.

    It is the caller's responsibility to free this block.

Arguments:

    None.

Return Value:

    NULL on failure.

Environment:

    Kernel mode.  PASSIVE level.  No locks held.

--*/

{
    ULONG i;
    KIRQL OldIrql;
    PPHYSICAL_MEMORY_RANGE p;
    PPHYSICAL_MEMORY_RANGE PhysicalMemoryBlock;

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    ExAcquireFastMutex (&MmDynamicMemoryMutex);

    i = sizeof(PHYSICAL_MEMORY_RANGE) * (MmPhysicalMemoryBlock->NumberOfRuns + 1);

    PhysicalMemoryBlock = ExAllocatePoolWithTag (NonPagedPool,
                                                 i,
                                                 'hPmM');

    if (PhysicalMemoryBlock == NULL) {
        ExReleaseFastMutex (&MmDynamicMemoryMutex);
        return NULL;
    }

    p = PhysicalMemoryBlock;

    LOCK_PFN (OldIrql);

    ASSERT (i == (sizeof(PHYSICAL_MEMORY_RANGE) * (MmPhysicalMemoryBlock->NumberOfRuns + 1)));

    for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i += 1) {
        p->BaseAddress.QuadPart = (LONGLONG)MmPhysicalMemoryBlock->Run[i].BasePage * PAGE_SIZE;
        p->NumberOfBytes.QuadPart = (LONGLONG)MmPhysicalMemoryBlock->Run[i].PageCount * PAGE_SIZE;
        p += 1;
    }

    p->BaseAddress.QuadPart = 0;
    p->NumberOfBytes.QuadPart = 0;

    UNLOCK_PFN (OldIrql);

    ExReleaseFastMutex (&MmDynamicMemoryMutex);

    return PhysicalMemoryBlock;
}

PFN_COUNT
MiRemovePhysicalPages (
    IN PFN_NUMBER StartPage,
    IN PFN_NUMBER EndPage
    )

/*++

Routine Description:

    This routine searches the PFN database for free, zeroed or standby pages
    that are marked for removal.

Arguments:

    StartPage - Supplies the low physical frame number to remove.

    EndPage - Supplies the last physical frame number to remove.

Return Value:

    Returns the number of pages removed from the free, zeroed and standby lists.

Environment:

    Kernel mode, PFN lock held.

--*/

{
    PMMPFN Pfn1;
    PMMPFN Pfn2;
    PMMPFN PfnNextColored;
    PMMPFN PfnNextFlink;
    PMMPFN PfnLastColored;
    PFN_NUMBER Page;
    LOGICAL RemovePage;
    ULONG Color;
    PMMCOLOR_TABLES ColorHead;
    PFN_NUMBER MovedPage;
    MMLISTS MemoryList;
    PFN_NUMBER PageNextColored;
    PFN_NUMBER PageNextFlink;
    PFN_NUMBER PageLastColored;
    PFN_COUNT NumberOfPages;
    PMMPFNLIST ListHead;
    LOGICAL RescanNeeded;

    MM_PFN_LOCK_ASSERT();

    NumberOfPages = 0;

rescan:

    //
    // Grab all zeroed (and then free) pages first directly from the
    // colored lists to avoid multiple walks down these singly linked lists.
    // Handle transition pages last.
    //

    for (MemoryList = ZeroedPageList; MemoryList <= FreePageList; MemoryList += 1) {

        ListHead = MmPageLocationList[MemoryList];

        for (Color = 0; Color < MmSecondaryColors; Color += 1) {
            ColorHead = &MmFreePagesByColor[MemoryList][Color];

            MovedPage = MM_EMPTY_LIST;

            while (ColorHead->Flink != MM_EMPTY_LIST) {

                Page = ColorHead->Flink;
    
                Pfn1 = MI_PFN_ELEMENT(Page);

                ASSERT ((MMLISTS)Pfn1->u3.e1.PageLocation == MemoryList);

                // 
                // The Flink and Blink must be nonzero here for the page
                // to be on the listhead.  Only code that scans the
                // MmPhysicalMemoryBlock has to check for the zero case.
                //

                ASSERT (Pfn1->u1.Flink != 0);
                ASSERT (Pfn1->u2.Blink != 0);

                //
                // See if the page is desired by the caller.
                //

                if (Pfn1->u3.e1.RemovalRequested == 1) {

                    ASSERT (Pfn1->u3.e1.ReadInProgress == 0);
    
                    MiUnlinkFreeOrZeroedPage (Page);
    
                    MiInsertPageInList (MmPageLocationList[BadPageList],
                                        Page);

                    NumberOfPages += 1;
                }
                else {

                    //
                    // Unwanted so put the page on the end of list.
                    // If first time, save pfn.
                    //

                    if (MovedPage == MM_EMPTY_LIST) {
                        MovedPage = Page;
                    }
                    else if (Page == MovedPage) {

                        //
                        // No more pages available in this colored chain.
                        //

                        break;
                    }

                    //
                    // If the colored chain has more than one entry then
                    // put this page on the end.
                    //

                    PageNextColored = (PFN_NUMBER)Pfn1->OriginalPte.u.Long;

                    if (PageNextColored == MM_EMPTY_LIST) {

                        //
                        // No more pages available in this colored chain.
                        //

                        break;
                    }

                    ASSERT (Pfn1->u1.Flink != 0);
                    ASSERT (Pfn1->u1.Flink != MM_EMPTY_LIST);
                    ASSERT (Pfn1->PteFrame != MI_MAGIC_AWE_PTEFRAME);

                    PfnNextColored = MI_PFN_ELEMENT(PageNextColored);
                    ASSERT ((MMLISTS)PfnNextColored->u3.e1.PageLocation == MemoryList);
                    ASSERT (PfnNextColored->PteFrame != MI_MAGIC_AWE_PTEFRAME);

                    //
                    // Adjust the free page list so Page
                    // follows PageNextFlink.
                    //

                    PageNextFlink = Pfn1->u1.Flink;
                    PfnNextFlink = MI_PFN_ELEMENT(PageNextFlink);

                    ASSERT ((MMLISTS)PfnNextFlink->u3.e1.PageLocation == MemoryList);
                    ASSERT (PfnNextFlink->PteFrame != MI_MAGIC_AWE_PTEFRAME);

                    PfnLastColored = ColorHead->Blink;
                    ASSERT (PfnLastColored != (PMMPFN)MM_EMPTY_LIST);
                    ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                    ASSERT (PfnLastColored->PteFrame != MI_MAGIC_AWE_PTEFRAME);
                    ASSERT (PfnLastColored->u2.Blink != MM_EMPTY_LIST);

                    ASSERT ((MMLISTS)PfnLastColored->u3.e1.PageLocation == MemoryList);
                    PageLastColored = PfnLastColored - MmPfnDatabase;

                    if (ListHead->Flink == Page) {

                        ASSERT (Pfn1->u2.Blink == MM_EMPTY_LIST);
                        ASSERT (ListHead->Blink != Page);

                        ListHead->Flink = PageNextFlink;

                        PfnNextFlink->u2.Blink = MM_EMPTY_LIST;
                    }
                    else {

                        ASSERT (Pfn1->u2.Blink != MM_EMPTY_LIST);
                        ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->PteFrame != MI_MAGIC_AWE_PTEFRAME);
                        ASSERT ((MMLISTS)(MI_PFN_ELEMENT((MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink)))->u3.e1.PageLocation == MemoryList);

                        MI_PFN_ELEMENT(Pfn1->u2.Blink)->u1.Flink = PageNextFlink;
                        PfnNextFlink->u2.Blink = Pfn1->u2.Blink;
                    }

#if DBG
                    if (PfnLastColored->u1.Flink == MM_EMPTY_LIST) {
                        ASSERT (ListHead->Blink == PageLastColored);
                    }
#endif

                    Pfn1->u1.Flink = PfnLastColored->u1.Flink;
                    Pfn1->u2.Blink = PageLastColored;

                    if (ListHead->Blink == PageLastColored) {
                        ListHead->Blink = Page;
                    }

                    //
                    // Adjust the colored chains.
                    //

                    if (PfnLastColored->u1.Flink != MM_EMPTY_LIST) {
                        ASSERT (MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->PteFrame != MI_MAGIC_AWE_PTEFRAME);
                        ASSERT ((MMLISTS)(MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u3.e1.PageLocation) == MemoryList);
                        MI_PFN_ELEMENT(PfnLastColored->u1.Flink)->u2.Blink = Page;
                    }

                    PfnLastColored->u1.Flink = Page;

                    ColorHead->Flink = PageNextColored;
                    Pfn1->OriginalPte.u.Long = MM_EMPTY_LIST;

                    ASSERT (PfnLastColored->OriginalPte.u.Long == MM_EMPTY_LIST);
                    PfnLastColored->OriginalPte.u.Long = Page;
                    ColorHead->Blink = Pfn1;
                }
            }
        }
    }

    RescanNeeded = FALSE;
    Pfn1 = MI_PFN_ELEMENT (StartPage);

    do {

        if ((Pfn1->u3.e1.PageLocation == StandbyPageList) &&
            (Pfn1->u1.Flink != 0) &&
            (Pfn1->u2.Blink != 0) &&
            (Pfn1->u3.e2.ReferenceCount == 0)) {

            ASSERT (Pfn1->u3.e1.ReadInProgress == 0);

            RemovePage = TRUE;

            if (Pfn1->u3.e1.RemovalRequested == 0) {

                //
                // This page is not directly needed for a hot remove - but if
                // it contains a chunk of prototype PTEs (and this chunk is
                // in a page that needs to be removed), then any pages
                // referenced by transition prototype PTEs must also be removed
                // before the desired page can be removed.
                //
                // The same analogy holds for page parent, directory and table
                // pages.
                //

                Pfn2 = MI_PFN_ELEMENT (Pfn1->PteFrame);
                if (Pfn2->u3.e1.RemovalRequested == 0) {
#if defined (_WIN64)
                    Pfn2 = MI_PFN_ELEMENT (Pfn2->PteFrame);
                    if (Pfn2->u3.e1.RemovalRequested == 0) {
                        RemovePage = FALSE;
                    }
                    else if (Pfn2->u2.ShareCount == 1) {
                        RescanNeeded = TRUE;
                    }
#else
                    RemovePage = FALSE;
#endif
                }
                else if (Pfn2->u2.ShareCount == 1) {
                    RescanNeeded = TRUE;
                }
            }
    
            if (RemovePage == TRUE) {

                //
                // This page is in the desired range - grab it.
                //
    
                MiUnlinkPageFromList (Pfn1);
                MiRestoreTransitionPte (StartPage);
                MiInsertPageInList (MmPageLocationList[BadPageList],
                                    StartPage);
                NumberOfPages += 1;
            }
        }

        StartPage += 1;
        Pfn1 += 1;

    } while (StartPage < EndPage);

    if (RescanNeeded == TRUE) {

        //
        // A page table, directory or parent was freed by removing a transition
        // page from the cache.  Rescan from the top to pick it up.
        //

#if DBG
        MiDynmemData[7] += 1;
#endif

        goto rescan;
    }
#if DBG
    else {
        MiDynmemData[8] += 1;
    }
#endif

    return NumberOfPages;
}
