/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

   hypermap.c

Abstract:

    This module contains the routines which map physical pages into
    reserved PTEs within hyper space.

    This module is machine dependent.  This version is targetted
    for ALPHA uses KSEG0 (32bit super-page) to map the pages at their physical
    addresses.

Author:

    Lou Perazzoli (loup) 5-Apr-1989
    Joe Notarangelo 23-Apr-1992   ALPHA version from MIPS version

Revision History:

    Chao Chen 21-Aug-1995 Fixed accessing pages above 1 gig problem through
                          hyperspace.

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

    kernel mode.

--*/

{
    PMMPTE PointerPte;
    MMPTE TempPte;
    ULONG offset;

#if DBG
    if (PageFrameIndex == 0) {
        DbgPrint("attempt to map physical page 0 in hyper space\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    //
    // If the page is below 1GB physical, then it can be mapped via
    // KSEG0.
    //

    LOCK_HYPERSPACE (OldIrql);

    if (PageFrameIndex < MM_PAGES_IN_KSEG0) {
        return (PVOID)(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
    }

    PointerPte = MmFirstReservedMappingPte;
    if (PointerPte->u.Hard.Valid == 1) {

        //
        // All the reserved PTEs have been used, make
        // them all invalid.
        //

        MI_MAKING_MULTIPLE_PTES_INVALID (FALSE);

        RtlZeroMemory (MmFirstReservedMappingPte,
                       (NUMBER_OF_MAPPING_PTES + 1) * sizeof(MMPTE));

        //
        // Use the page frame number field of the first PTE as an
        // offset into the available mapping PTEs.
        //

        PointerPte->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;

        //
        // Flush entire TB only on this processor.
        //

        KeFlushEntireTb (TRUE, FALSE);
    }

    //
    // Get offset to first free PTE.
    //

    offset = PointerPte->u.Hard.PageFrameNumber;

    //
    // Change offset for next time through.
    //

    PointerPte->u.Hard.PageFrameNumber = offset - 1;

    //
    // Point to free entry and make it valid.
    //

    PointerPte += offset;
    ASSERT (PointerPte->u.Hard.Valid == 0);


    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;

    //
    // Return the VA that map the page.
    //

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

    On ALPHA, no action is required as the super-page address of the page
    was used.

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

    KeFlushSingleTb (IMAGE_MAPPING_PTE, TRUE, FALSE,
                     (PHARDWARE_PTE)PointerPte, TempPte.u.Hard);

    UNLOCK_PFN (OldIrql);

    if (Event != (PKEVENT)NULL) {

        //
        // If there was an event specified, set the event.
        //

        KePulseEvent (Event, 0, FALSE);
    }

    return;
}


PVOID
MiMapPageToZeroInHyperSpace (
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified physical page into hyper space
    and returns the virtual address which maps the page.

    NOTE: it maps it into the same location reserved for fork operations!!
    This is only to be used by the zeroing page thread.


Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Returns the virtual address where the specified physical page was
    mapped.

Environment:

    Must be holding the PFN lock.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PVOID MappedAddress;

#if DBG
    if (PageFrameIndex == 0) {
        DbgPrint("attempt to map physical page 0 in hyper space\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    //
    // If the page is below 1GB physical then it can be mapped via
    // KSEG0.
    //

    if (PageFrameIndex < MM_PAGES_IN_KSEG0) {
        return (PVOID)(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
    }

    MM_PFN_LOCK_ASSERT();

    PointerPte = MiGetPteAddress (ZEROING_PAGE_PTE);
    MappedAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    TempPte.u.Long = 0;

    KeFlushSingleTb (MappedAddress,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPte, TempPte.u.Hard);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    *PointerPte = TempPte;

    return MappedAddress;
}
