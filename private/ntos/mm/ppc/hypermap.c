/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1993  IBM Corporation

Module Name:

   hypermap.c

Abstract:

    This module contains the routines which map physical pages into
    reserved PTEs within hyper space.

    This module is machine dependent.  This version is targetted
    for PowerPC.

Author:

    Lou Perazzoli (loup) 5-Apr-1989

    Modified for PowerPC by Mark Mergen (mergen@watson.ibm.com) 11-Oct-1993

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

    This routine maps the specified physical page into hyperspace
    and returns the virtual address that maps the page.

    ************************************
    *                                  *
    * Returns with a spin lock held!!! *
    *                                  *
    ************************************

Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Virtual address in hyperspace that maps the page.

    RETURNS WITH THE HYPERSPACE SPIN LOCK HELD!!!!

    The routine MiUnmapPageInHyperSpace MUST be called to release the lock!!!!

Environment:

    Kernel mode.

--*/

{
    ULONG i;
    PMMPTE PointerPte;
    MMPTE TempPte;

#if DBG
    if (PageFrameIndex == 0) {
        DbgPrint("attempt to map physical page 0 in hyper space\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    //
    // Find the proper location in hyper space and map the page there.
    //

    LOCK_HYPERSPACE(OldIrql);
    PointerPte = MmFirstReservedMappingPte;
    if (PointerPte->u.Hard.Valid == 1) {

        //
        // All the pages in reserved for mapping have been used,
        // flush the TB and reinitialize the pages.
        //

        RtlZeroMemory ((PVOID)MmFirstReservedMappingPte,
                       (NUMBER_OF_MAPPING_PTES + 1) * sizeof(MMPTE));
        PointerPte->u.Hard.PageFrameNumber = NUMBER_OF_MAPPING_PTES;
        KeFlushEntireTb (TRUE, FALSE);

    }

    //
    // Get the offset to the first free PTE.
    //

    i = PointerPte->u.Hard.PageFrameNumber;

    //
    // Change the offset for the next time through.
    //

    PointerPte->u.Hard.PageFrameNumber = i - 1;

    //
    // Point to the free entry and make it valid.
    //

    PointerPte += i;

    ASSERT (PointerPte->u.Hard.Valid == 0);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    *PointerPte = TempPte;

    return MiGetVirtualAddressMappedByPte (PointerPte);
}

PVOID
MiMapImageHeaderInHyperSpace (
    IN ULONG PageFrameIndex
    )

/*++

Routine Description:

    This routine maps the specified physical page into hyperspace
    at the address reserved explicitly for image page header mapping
    and returns the virtual address that maps the page.  No other
    hyperspace maps will affect this map.  If another thread attempts
    to map an image at the same time, it will be forced to wait until
    this header is unmapped.

Arguments:

    PageFrameIndex - Supplies the physical page number to map.

Return Value:

    Virtual address in hyperspace that maps the page.

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

#if DBG
    if (PageFrameIndex == 0) {
        DbgPrint("attempt to map physical page 0 in hyper space\n");
        KeBugCheck (MEMORY_MANAGEMENT);
    }
#endif //DBG

    PointerPte = MiGetPteAddress (ZEROING_PAGE_PTE);

    TempPte.u.Long = 0;

    KeFlushSingleTb (ZEROING_PAGE_PTE,
                     TRUE,
                     FALSE,
                     (PHARDWARE_PTE)PointerPte,
                     TempPte.u.Hard);

    TempPte = ValidPtePte;
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

    *PointerPte = TempPte;

    return ZEROING_PAGE_PTE;
}
