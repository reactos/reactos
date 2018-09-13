/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   wrtfault.c

Abstract:

    This module contains the copy on write routine for memory management.

Author:

    Lou Perazzoli (loup) 10-Apr-1989

Revision History:

--*/

#include "mi.h"

NTSTATUS
FASTCALL
MiCopyOnWrite (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte
    )

/*++

Routine Description:

    This routine performs a copy on write operation for the specified
    virtual address.

Arguments:

    FaultingAddress - Supplies the virtual address which caused the
                      fault.

    PointerPte - Supplies the pointer to the PTE which caused the
                 page fault.


Return Value:

    Returns the status of the fault handling operation.  Can be one of:
        - Success.
        - In-page Error.

Environment:

    Kernel mode, APC's disabled, Working set mutex held.

--*/

{
    MMPTE TempPte;
    PFN_NUMBER PageFrameIndex;
    PFN_NUMBER NewPageIndex;
    PULONG CopyTo;
    PULONG CopyFrom;
    KIRQL OldIrql;
    PMMPFN Pfn1;
//    PMMPTE PointerPde;
    PEPROCESS CurrentProcess;
    PMMCLONE_BLOCK CloneBlock;
    PMMCLONE_DESCRIPTOR CloneDescriptor;
    PVOID VirtualAddress;
    WSLE_NUMBER WorkingSetIndex;
    LOGICAL FakeCopyOnWrite;

    FakeCopyOnWrite = FALSE;

    //
    // This is called from MmAccessFault, the PointerPte is valid
    // and the working set mutex ensures it cannot change state.
    //

#if DBG
    if (MmDebug & MM_DBG_WRITEFAULT) {
        DbgPrint("**copy on write Fault va %lx proc %lx thread %lx\n",
            (ULONG_PTR)FaultingAddress,
            (ULONG_PTR)PsGetCurrentProcess(), (ULONG_PTR)PsGetCurrentThread());
    }

    if (MmDebug & MM_DBG_PTE_UPDATE) {
        MiFormatPte(PointerPte);
    }
#endif //DBG

    ASSERT (PsGetCurrentProcess()->ForkInProgress == NULL);

    //
    // Capture the PTE contents to TempPte.
    //

    TempPte = *PointerPte;

    //
    // Check to see if this is a prototype PTE with copy on write
    // enabled.
    //

    if (TempPte.u.Hard.CopyOnWrite == 0) {

        //
        // This is a fork page which is being made private in order
        // to change the protection of the page.
        // Do not make the page writable.
        //

        FakeCopyOnWrite = TRUE;
    }

    PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE (&TempPte);
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    CurrentProcess = PsGetCurrentProcess();

    //
    // Acquire the PFN mutex.
    //

    VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
    WorkingSetIndex = MiLocateWsle (VirtualAddress, MmWorkingSetList,
                        Pfn1->u1.WsIndex);

    LOCK_PFN (OldIrql);

    //
    // The page must be copied into a new page.
    //

    //
    // If a fork operation is in progress and the faulting thread
    // is not the thread performing the fork operation, block until
    // the fork is completed.
    //

    if ((CurrentProcess->ForkInProgress != NULL) &&
        (CurrentProcess->ForkInProgress != PsGetCurrentThread())) {
        MiWaitForForkToComplete (CurrentProcess);
        UNLOCK_PFN (OldIrql);
        return STATUS_SUCCESS;
    }

    if (MiEnsureAvailablePageOrWait(CurrentProcess, NULL)) {

        //
        // A wait operation was performed to obtain an available
        // page and the working set mutex and pfn mutexes have
        // been released and various things may have changed for
        // the worse.  Rather than examine all the conditions again,
        // return and if things are still proper, the fault we
        // be taken again.
        //

        UNLOCK_PFN (OldIrql);
        return STATUS_SUCCESS;
    }

    //
    // Increment the number of private pages.
    //

    CurrentProcess->NumberOfPrivatePages += 1;

    MmInfoCounters.CopyOnWriteCount += 1;

    //
    // A page is being copied and made private, the global state of
    // the shared page needs to be updated at this point on certain
    // hardware.  This is done by ORing the dirty bit into the modify bit in
    // the PFN element.
    //

    MI_CAPTURE_DIRTY_BIT_TO_PFN (PointerPte, Pfn1);

    //
    // This must be a prototype PTE.  Perform the copy on write.
    //

#if DBG
    if (Pfn1->u3.e1.PrototypePte == 0) {
        DbgPrint("writefault - PTE indicates cow but not protopte\n");
        MiFormatPte(PointerPte);
        MiFormatPfn(Pfn1);
    }
#endif

    CloneBlock = (PMMCLONE_BLOCK)Pfn1->PteAddress;

    //
    // If the share count for the physical page is one, the reference
    // count is one, and the modified flag is clear the current page
    // can be stolen to satisfy the copy on write.
    //

#if 0
// COMMENTED OUT ****************************************************
// COMMENTED OUT ****************************************************
// COMMENTED OUT ****************************************************
    if ((Pfn1->u2.ShareCount == 1) && (Pfn1->u3.e2.ReferenceCount == 1)
            && (Pfn1->u3.e1.Modified == 0)) {

        //
        // Make this page a private page and return the prototype
        // PTE into its original contents.  The PFN database for
        // this page now points to this PTE.
        //

        //
        // Note that a page fault could occur referencing the prototype
        // PTE, so we map it into hyperspace to prevent a fault.
        //

        MiRestorePrototypePte (Pfn1);

        Pfn1->PteAddress = PointerPte;

        //
        // Get the protection for the page.
        //

        VirtualAddress = MiGetVirtualAddressMappedByPte (PointerPte);
        WorkingSetIndex = MiLocateWsle (VirtualAddress, MmWorkingSetList,
                            Pfn1->u1.WsIndex);

        ASSERT (WorkingSetIndex != WSLE_NULL_INDEX) {

        Pfn1->OriginalPte.u.Long = 0;
        Pfn1->OriginalPte.u.Soft.Protection =
                MI_MAKE_PROTECT_NOT_WRITE_COPY (
                                MmWsle[WorkingSetIndex].u1.e1.Protection);

        PointerPde = MiGetPteAddress(PointerPte);
        Pfn1->u3.e1.PrototypePte = 0;
        Pfn1->PteFrame = MI_GET_PAGE_FRAME_FROM_PTE (PointerPde);

        if (!FakeCopyOnWrite) {

            //
            // If the page was Copy On Write and stolen or if the page was not
            // copy on write, update the PTE setting both the dirty bit and the
            // accessed bit. Note, that as this PTE is in the TB, the TB must
            // be flushed.
            //

            MI_SET_PTE_DIRTY (TempPte);
            TempPte.u.Hard.Write = 1;
            MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
            TempPte.u.Hard.CopyOnWrite = 0;
            MI_WRITE_VALID_PTE_NEW_PROTECTION (PointerPte, TempPte);

            //
            // This is a copy on write operation, set the modify bit
            // in the PFN database and deallocate any page file space.
            //

            Pfn1->u3.e1.Modified = 1;

            if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                (Pfn1->u3.e1.WriteInProgress == 0)) {

                //
                // This page is in page file format, deallocate the page
                // file space.
                //

                MiReleasePageFileSpace (Pfn1->OriginalPte);

                //
                // Change original PTE to indicate no page file space is
                // reserved, otherwise the space will be deallocated when
                // the PTE is deleted.
                //

                Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
            }
        }

        //
        // The TB entry must be flushed as the valid PTE with the dirty
        // bit clear has been fetched into the TB. If it isn't flushed,
        // another fault is generated as the dirty bit is not set in
        // the cached TB entry.
        //


        KeFillEntryTb ((PHARDWARE_PTE)PointerPte, FaultingAddress, TRUE);

        CloneDescriptor = MiLocateCloneAddress ((PVOID)CloneBlock);

        if (CloneDescriptor != (PMMCLONE_DESCRIPTOR)NULL) {

            //
            // Decrement the reference count for the clone block,
            // note that this could release and reacquire
            // the mutexes.
            //

            MiDecrementCloneBlockReference ( CloneDescriptor,
                                             CloneBlock,
                                             CurrentProcess );
        }

    ] else [

// ABOVE COMMENTED OUT ****************************************************
// ABOVE COMMENTED OUT ****************************************************
#endif

    //
    // Get a new page with the same color as this page.
    //

    NewPageIndex = MiRemoveAnyPage (
                    MI_GET_SECONDARY_COLOR (PageFrameIndex,
                                            Pfn1));
    MiInitializeCopyOnWritePfn (NewPageIndex, PointerPte, WorkingSetIndex, NULL);

    UNLOCK_PFN (OldIrql);

    CopyTo = (PULONG)MiMapPageInHyperSpace (NewPageIndex, &OldIrql);

#if defined(_MIALT4K_)

    //
    // Should avoid accessing the user space. Accessing the user space may potentially 
    // cause a page fault on the alternate table.   
    //

    CopyFrom = KSEG_ADDRESS(PointerPte->u.Hard.PageFrameNumber);

#else
    CopyFrom = (PULONG)MiGetVirtualAddressMappedByPte (PointerPte);
#endif

    RtlCopyMemory ( CopyTo, CopyFrom, PAGE_SIZE);

    PERFINFO_PRIVATE_COPY_ON_WRITE(CopyFrom, PAGE_SIZE);

    MiUnmapPageInHyperSpace (OldIrql);

    if (!FakeCopyOnWrite) {

        //
        // If the page was really a copy on write page, make it
        // accessed, dirty and writable.  Also, clear the copy-on-write
        // bit in the PTE.
        //

        MI_SET_PTE_DIRTY (TempPte);
        TempPte.u.Hard.Write = 1;
        MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
        TempPte.u.Hard.CopyOnWrite = 0;
        TempPte.u.Hard.PageFrameNumber = NewPageIndex;

    } else {

        //
        // The page was not really a copy on write, just change
        // the frame field of the PTE.
        //

        TempPte.u.Hard.PageFrameNumber = NewPageIndex;
    }

    //
    // If the modify bit is set in the PFN database for the
    // page, the data cache must be flushed.  This is due to the
    // fact that this process may have been cloned and the cache
    // still contains stale data destined for the page we are
    // going to remove.
    //

    ASSERT (TempPte.u.Hard.Valid == 1);

    LOCK_PFN (OldIrql);

    //
    // Flush the TB entry for this page.
    //

    KeFlushSingleTb (FaultingAddress,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPte,
                     TempPte.u.Flush);

    //
    // Decrement the share count for the page which was copied
    // as this pte no longer refers to it.
    //

    MiDecrementShareCount (PageFrameIndex);

    CloneDescriptor = MiLocateCloneAddress ((PVOID)CloneBlock);

    if (CloneDescriptor != (PMMCLONE_DESCRIPTOR)NULL) {

        //
        // Decrement the reference count for the clone block,
        // note that this could release and reacquire
        // the mutexes.
        //

        MiDecrementCloneBlockReference ( CloneDescriptor,
                                         CloneBlock,
                                         CurrentProcess );
    }

    UNLOCK_PFN (OldIrql);
    return STATUS_SUCCESS;
}
