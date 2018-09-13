/*++

Copyright (c) 1989  Microsoft Corporation
Copyright (c) 1992  Digital Equipment Corporation

Module Name:

   setdirty.c

Abstract:

    This module contains the setting dirty bit routine for memory management.

    ALPHA specific.

Author:

    Lou Perazzoli (loup) 10-Apr-1990.
    Joe Notarangelo  23-Apr-1992   ALPHA version

Revision History:

--*/

#include "mi.h"

VOID
MiSetDirtyBit (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN ULONG PfnHeld
    )

/*++

Routine Description:

    This routine sets dirty in the specified PTE and the modify bit in the
    correpsonding PFN element.  If any page file space is allocated, it
    is deallocated.

Arguments:

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies a pointer to the corresponding valid PTE.

    PfnHeld - Supplies TRUE if the PFN mutex is already held.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled, Working set mutex held.

--*/

{
    MMPTE TempPte;
    ULONG PageFrameIndex;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    //
    // The TB entry must be flushed as the valid PTE with the dirty bit clear
    // has been fetched into the TB. If it isn't flushed, another fault
    // is generated as the dirty bit is not set in the cached TB entry.
    //

    // KiFlushSingleDataTb( FaultingAddress );
    __dtbis( FaultingAddress );

    //
    // The page is NOT copy on write, update the PTE setting both the
    // dirty bit and the accessed bit. Note, that as this PTE is in
    // the TB, the TB must be flushed.
    //

    PageFrameIndex = PointerPte->u.Hard.PageFrameNumber;
    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    TempPte = *PointerPte;
    TempPte.u.Hard.Dirty = 1;
    MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
    *PointerPte = TempPte;

    //
    // If the PFN database lock is not held, then do not update the
    // PFN database.
    //

    if (PfnHeld) {

        //
        // Set the modified field in the PFN database, also, if the physical
        // page is currently in a paging file, free up the page file space
        // as the contents are now worthless.
        //

        if ( (Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
             (Pfn1->u3.e1.WriteInProgress == 0) ) {

            //
            // This page is in page file format, deallocate the page file space.
            //

            MiReleasePageFileSpace (Pfn1->OriginalPte);

            //
            // Change original PTE to indicate no page file space is reserved,
            // otherwise the space will be deallocated when the PTE is
            // deleted.
            //

            Pfn1->OriginalPte.u.Soft.PageFileHigh = 0;
        }

        Pfn1->u3.e1.Modified = 1;

    }


    return;
}
