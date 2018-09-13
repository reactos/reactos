/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   hypermap.c

Abstract:

    This module contains the routines which map physical pages into
    reserved PTEs within hyper space.

Author:

    Lou Perazzoli (loup) 5-Apr-1989

Revision History:

--*/

#include "mi.h"


PVOID
MiMapPageInHyperSpace (
    IN PFN_NUMBER PageFrameIndex,
    IN PKIRQL OldIrql
    )

/*++

Routine Description:

    This procedure maps the specified physical page into hyper space
    and returns the virtual address which maps the page.

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

    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER offset;

#if DBG
    if (PageFrameIndex == 0) {
        DbgPrint("attempt to map physical page 0 in hyper space\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    LOCK_HYPERSPACE(OldIrql);

#if HYPERMAP 

    if( PageFrameIndex < MmKseg2Frame){
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

    offset = (PFN_NUMBER)PointerPte->u.Hard.PageFrameNumber;

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

#else
 
    return KSEG_ADDRESS(PageFrameIndex);

#endif
}

PVOID
MiMapImageHeaderInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified physical page into the
    PTE within hyper space reserved explicitly for image page
    header mapping.  By reserving an explicit PTE for mapping
    the PTE, page faults can occur while the PTE is mapped within
    hyperspace and no other hyperspace maps will affect this PTE.

    Note that if another thread attempts to map an image at the
    same time, it will be forced into a wait state until the
    header is "unmapped".

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

#if HYPERMAP

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

        UNLOCK_PFN_AND_THEN_WAIT(OldIrql);

        KeWaitForSingleObject(MmWorkingSetList->WaitingForImageMapping,
                              Executive,
                              KernelMode,
                              FALSE,
                              (PLARGE_INTEGER)NULL);

        LOCK_PFN (OldIrql);
    }

    ASSERT (PointerPte->u.Long == 0);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    *PointerPte = TempPte;

    UNLOCK_PFN (OldIrql);

    return (PVOID)MiGetVirtualAddressMappedByPte (PointerPte);

#else

    return KSEG_ADDRESS(PageFrameIndex);

#endif
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

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode.

--*/

{
#if HYPERMAP

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
                     (PHARDWARE_PTE)PointerPte, TempPte.u.Flush);

    UNLOCK_PFN (OldIrql);

    if (Event != (PKEVENT)NULL) {

        //
        // If there was an event specified, set the event.
        //

        KePulseEvent (Event, 0, FALSE);
    }

#endif

    return;
}

PVOID
MiMapPageToZeroInHyperSpace (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This procedure maps the specified physical page into hyper space
    and returns the virtual address which maps the page.

    NOTE: it maps it into the same location reserved for zeroing operations.
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
#endif

    MM_PFN_LOCK_ASSERT();

#if HYPERMAP

    if( PageFrameIndex < MmKseg2Frame){
        return (PVOID)(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
    }

    PointerPte = MiGetPteAddress (ZEROING_PAGE_PTE);

    MappedAddress = MiGetVirtualAddressMappedByPte (PointerPte);

    TempPte.u.Long = 0;

    KeFlushSingleTb (MappedAddress, TRUE, FALSE,
                     (PHARDWARE_PTE)PointerPte, TempPte.u.Flush);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    *PointerPte = TempPte;

    return MappedAddress;

#else

    return KSEG_ADDRESS(PageFrameIndex);

#endif
}
