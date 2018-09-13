/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   setmodfy.c

Abstract:

    This module contains the setting modify bit routine for memory management.

    i386 specific.

Author:

    10-Apr-1989

Revision History:

--*/

#include "mi.h"

VOID
MiSetModifyBit (
    IN PMMPFN Pfn
    )

/*++

Routine Description:

    This routine sets the modify bit in the specified PFN element
    and deallocates and allocated page file space.

Arguments:

    Pfn - Supplies the pointer to the PFN element to update.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working set mutex held and PFN lock held.

--*/

{

    //
    // Set the modified field in the PFN database, also, if the physical
    // page is currently in a paging file, free up the page file space
    // as the contents are now worthless.
    //

    Pfn->u3.e1.Modified = 1;

    if (Pfn->OriginalPte.u.Soft.Prototype == 0) {

        //
        // This page is in page file format, deallocate the page file space.
        //

        MiReleasePageFileSpace (Pfn->OriginalPte);

        //
        // Change original PTE to indicate no page file space is reserved,
        // otherwise the space will be deallocated when the PTE is
        // deleted.
        //

        Pfn->OriginalPte.u.Soft.PageFileHigh = 0;
    }


    return;
}

ULONG
FASTCALL
MiDetermineUserGlobalPteMask (
    IN PMMPTE Pte
    )

/*++

Routine Description:

    Builds a mask to OR with the PTE frame field.
    This mask has the valid and access bits set and
    has the global and owner bits set based on the
    address of the PTE.

    *******************  NOTE *********************************************
        THIS ROUTINE DOES NOT CHECK FOR PDEs WHICH NEED TO BE
        SET GLOBAL AS IT ASSUMES PDEs FOR SYSTEM SPACE ARE
        PROPERLY SET AT INITIALIZATION TIME!

Arguments:

    Pte - Supplies a pointer to the PTE in which to fill.

Return Value:

    Mask to OR into the frame to make a valid PTE.

Environment:

    Kernel mode, 386 specific.

--*/


{
    MMPTE Mask;

    Mask.u.Long = 0;
    Mask.u.Hard.Valid = 1;
    Mask.u.Hard.Accessed = 1;

    if (Pte <= MiHighestUserPte) {
        Mask.u.Hard.Owner = 1;
    } else if ((Pte < MiGetPteAddress (PTE_BASE)) ||
        (Pte >= MiGetPteAddress (MM_SYSTEM_CACHE_WORKING_SET))) {
            if (MI_IS_SESSION_PTE (Pte) == FALSE) {
#if defined (_X86PAE_)
              if ((Pte < (PMMPTE)PDE_BASE) || (Pte > (PMMPTE)PDE_TOP))
#endif
                Mask.u.Long |= MmPteGlobal.u.Long;
            }
    } else if ((Pte >= MiGetPdeAddress (NULL)) && (Pte <= MiHighestUserPde)) {
        Mask.u.Hard.Owner = 1;
    }

    //
    // Since the valid, accessed, global and owner bits are always in the
    // low dword of the PTE, returning a ULONG is ok.
    //

    return (ULONG)Mask.u.Long;
}




#if !defined(NT_UP)

VOID
MiSetDirtyBit (
    IN PVOID FaultingAddress,
    IN PMMPTE PointerPte,
    IN ULONG PfnHeld
    )

/*++

Routine Description:

    This routine sets dirty in the specified PTE and the modify bit in the
    corresponding PFN element.  If any page file space is allocated, it
    is deallocated.

Arguments:

    FaultingAddress - Supplies the faulting address.

    PointerPte - Supplies a pointer to the corresponding valid PTE.

    PfnHeld - Supplies TRUE if the PFN lock is already held.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled, Working set mutex held.

--*/

{
    MMPTE TempPte;
    ULONG PageFrameIndex;
    PMMPFN Pfn1;

    //
    // The page is NOT copy on write, update the PTE setting both the
    // dirty bit and the accessed bit. Note, that as this PTE is in
    // the TB, the TB must be flushed.
    //

    TempPte = *PointerPte;
    MI_SET_PTE_DIRTY (TempPte);
    MI_SET_ACCESSED_IN_PTE (&TempPte, 1);
    MI_WRITE_VALID_PTE_NEW_PROTECTION(PointerPte, TempPte);

    //
    // Check state of PFN lock and if not held, don't update PFN database.
    //

    if (PfnHeld) {

        PageFrameIndex = MI_GET_PAGE_FRAME_FROM_PTE(PointerPte);
        Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

        //
        // Set the modified field in the PFN database, also, if the physical
        // page is currently in a paging file, free up the page file space
        // as the contents are now worthless.
        //

        if ((Pfn1->OriginalPte.u.Soft.Prototype == 0) &&
                             (Pfn1->u3.e1.WriteInProgress == 0)) {

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

    //
    // The TB entry must be flushed as the valid PTE with the dirty bit clear
    // has been fetched into the TB. If it isn't flushed, another fault
    // is generated as the dirty bit is not set in the cached TB entry.
    //

    KeFillEntryTb ((PHARDWARE_PTE)PointerPte, FaultingAddress, TRUE);
    return;
}
#endif
