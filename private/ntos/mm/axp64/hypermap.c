/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

   hypermap.c

Abstract:

    This module contains the routines which map physical pages into
    reserved PTEs within hyper space for AXP64 systems.

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
    IN PFN_NUMBER PageFrameIndex,
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

    //
    // On AXP64 systems all pages can be mapped physically using the 43-bit
    // super page address range.
    //

    ASSERT(PageFrameIndex != 0);

    LOCK_HYPERSPACE(OldIrql);

    return KSEG_ADDRESS(PageFrameIndex);
}

PVOID
MiMapImageHeaderInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
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

    LOCK_PFN(OldIrql);

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

    PointerPte = MiGetPteAddress(IMAGE_MAPPING_PTE);

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
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified page frame into the 43-bit super
    page address range.

Arguments:

    PageFrameIndex - Supplies the page frame to map.

Return Value:

    The 43-bit super address of the respective page is returned as the
    function value.

--*/

{

    //
    // On AXP64 systems all pages can be mapped physically using the 43-bit
    // super page address range.
    //

    ASSERT(PageFrameIndex != 0);

    MM_PFN_LOCK_ASSERT();

    return KSEG_ADDRESS(PageFrameIndex);
}
