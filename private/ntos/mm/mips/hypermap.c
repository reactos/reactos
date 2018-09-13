/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   hypermap.c

Abstract:

    This module contains the routines which map physical pages into
    reserved PTEs within hyper space.

    This module is machine dependent.  This version is targetted
    for MIPS Rxxxx and uses KSEG0 to map the pages at their physical
    addresses.

Author:

    Lou Perazzoli (loup) 5-Apr-1989

Revision History:

--*/

#include "mi.h"


PVOID
MiMapPageInHyperSpace (
    IN ULONG PageFrameIndex,
    IN PKIRQL OldIrql
    )

/*++

Routine Description:

    This routine returns the physical address of the page.

    ************************************
    *                                  *
    * Returns with a spin lock held!!! *
    *                                  *
    ************************************

Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Returns the address where the requested page was mapped.

    RETURNS WITH THE HYPERSPACE SPIN LOCK HELD!!!!

    The routine MiUnmapHyperSpaceMap MUST be called to release the lock!!!!

Environment:

    Kernel mode.

--*/

{
    PMMPFN Pfn1;
    ULONG i;
    PMMPTE PointerPte;
    PMMPTE NextPte;
    MMPTE TempPte;
    ULONG LastEntry;

#if DBG
    if (PageFrameIndex == 0) {
        DbgPrint("attempt to map physical page 0 in hyper space\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    //
    // Pages must be aligned on their natural boundaries.
    //

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    i = Pfn1->u3.e1.PageColor;
    if ((i == (PageFrameIndex & MM_COLOR_MASK)) &&
        (PageFrameIndex < MM_PAGES_IN_KSEG0)) {

        //
        // Virtual and physical alignment match, return the KSEG0 address
        // for this page.
        //

        LOCK_HYPERSPACE (OldIrql);
        return (PVOID)(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
    }

    //
    // Find the proper location in hyper space and map the page there.
    //

    LOCK_HYPERSPACE (OldIrql);
    PointerPte = MmFirstReservedMappingPte + i;
    if (PointerPte->u.Hard.Valid == 1 ) {

        //
        // All the pages in reserved for mapping have been used,
        // flush the TB and reinitialize the pages.
        //

        RtlZeroMemory ((PVOID)MmFirstReservedMappingPte,
                       ( NUMBER_OF_MAPPING_PTES + 1) * sizeof (MMPTE));
        KeFlushEntireTb (TRUE, FALSE);

        LastEntry = NUMBER_OF_MAPPING_PTES - MM_COLOR_MASK;
        NextPte = MmFirstReservedMappingPte;
        while (NextPte <= (MmFirstReservedMappingPte + MM_COLOR_MASK)) {
            NextPte->u.Hard.PageFrameNumber = LastEntry;
            NextPte += 1;
        }
    }

    //
    // Locate next entry in list and reset the next entry in the
    // list.  The list is organized thusly:
    //
    // The first N elements corresponding to the alignment mask + 1
    // contain in their page frame number fields the value of the
    // last free mapping PTE with this alignment.  However, if
    // the valid bit is set, this PTE has been used and the TB
    // must be flushed and the list reinitialized.
    //

    //
    // Get the offset to the first free PTE.
    //

    i = PointerPte->u.Hard.PageFrameNumber;

    //
    // Change the offset for the next time through.
    //

    PointerPte->u.Hard.PageFrameNumber = i - (MM_COLOR_MASK + 1);

    //
    // Point to the free entry and make it valid.
    //

    PointerPte += i;

    ASSERT (PointerPte->u.Hard.Valid == 0);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;

    ASSERT ((((ULONG)PointerPte >> PTE_SHIFT) & MM_COLOR_MASK) ==
         (((ULONG)Pfn1->u3.e1.PageColor)));

    return MiGetVirtualAddressMappedByPte (PointerPte);
}

PVOID
MiMapImageHeaderInHyperSpace (
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    The physical address of the specified page is returned.

Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Returns the virtual address where the specified physical page was
    mapped.

Environment:

    Kernel mode.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    KIRQL OldIrql;

#if DBG
    if (PageFrameIndex == 0) {
        DbgPrint("attempt to map physical page 0 in hyper space\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    //
    // Avoid address aliasing problem on r4000.
    //

    PointerPte = MiGetPteAddress (IMAGE_MAPPING_PTE);

    LOCK_PFN (OldIrql);

    while (PointerPte->u.Long != 0) {

        //
        // If there is no event specified, set one up.
        //

        if (MmWorkingSetList->WaitingForImageMapping == (PKEVENT)NULL) {

            //
            // Set the global event into the field and wait for it.
            //

            MmWorkingSetList->WaitingForImageMapping = &MmImageMappingPteEvent;
        }

        //
        // Release the PFN lock and wait on the event in an
        // atomic operation.
        //

        KeEnterCriticalRegion();
        UNLOCK_PFN_AND_THEN_WAIT(OldIrql);

        KeWaitForSingleObject(MmWorkingSetList->WaitingForImageMapping,
                              Executive,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);
        KeLeaveCriticalRegion();

        LOCK_PFN (OldIrql);
    }

    ASSERT (PointerPte->u.Long == 0);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    *PointerPte = TempPte;

    UNLOCK_PFN (OldIrql);

    return (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);
}

VOID
MiUnmapImageHeaderInHyperSpace (
    VOID
    )

/*++

Routine Description:

    This procedure unmaps the PTE reserved for mapping the image
    header, flushes the TB, and, if the WaitingForImageMapping field
    is not NULL, sets the specified event.

    On the MIPS serries, no action is required as the physical address
    of the page is used.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    KIRQL OldIrql;
    PKEVENT Event;

    PointerPte = MiGetPteAddress (IMAGE_MAPPING_PTE);

    TempPte.u.Long = 0;

    LOCK_PFN (OldIrql);

    //
    // Capture the current state of the event field and clear it out.
    //

    Event = MmWorkingSetList->WaitingForImageMapping;

    MmWorkingSetList->WaitingForImageMapping = (PKEVENT)NULL;

    ASSERT (PointerPte->u.Long != 0);

    KeFlushSingleTb (IMAGE_MAPPING_PTE,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPte,
                     TempPte.u.Hard);

    UNLOCK_PFN (OldIrql);

    if (Event != (PKEVENT)NULL) {

        //
        // If there was an event specified, set the event.
        //

        KePulseEvent (Event, 0, FALSE);
    }

    return;
}
