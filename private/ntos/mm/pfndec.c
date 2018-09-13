/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   pfndec.c

Abstract:

    This module contains the routines to decrement the share count and
    the reference counts within the Page Frame Database.

Author:

    Lou Perazzoli (loup) 5-Apr-1989
    Landy Wang (landyw) 2-Jun-1997

Revision History:

--*/

#include "mi.h"

ULONG MmFrontOfList;


VOID
FASTCALL
MiDecrementShareCount (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the share count within the PFN element
    for the specified physical page.  If the share count becomes
    zero the corresponding PTE is converted to the transition state
    and the reference count is decremented and the ValidPte count
    of the PTEframe is decremented.

Arguments:

    PageFrameIndex - Supplies the physical page number of which to decrement
                     the share count.

Return Value:

    None.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PMMPFN Pfn1;
    KIRQL OldIrql;

    ASSERT ((PageFrameIndex <= MmHighestPhysicalPage) &&
            (PageFrameIndex > 0));

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);

    if (Pfn1->u3.e1.PageLocation != ActiveAndValid &&
        Pfn1->u3.e1.PageLocation != StandbyPageList) {
            KeBugCheckEx (PFN_LIST_CORRUPT,
                      0x99,
                      PageFrameIndex,
                      Pfn1->u3.e1.PageLocation,
                      0);
    }

#if PFN_CONSISTENCY
    if (Pfn1->u3.e1.PageTablePage == 1) {

        // Need to run the page and see if the number of transition & valid
        // pages + 1 matches up to the share count.
        //
        // For the page being freed (share == 1), need to snap the caller here

    }
#endif

    Pfn1->u2.ShareCount -= 1;

    PERFINFO_DECREFCNT(Pfn1, PERFINFO_LOG_WSCHANGE, PERFINFO_LOG_TYPE_DECSHARCNT)

    ASSERT (Pfn1->u2.ShareCount < 0xF000000);

    if (Pfn1->u2.ShareCount == 0) {

        PERFINFO_DECREFCNT(Pfn1, PERFINFO_LOG_EMPTYQ, PERFINFO_LOG_TYPE_ZEROSHARECOUNT)

        //
        // The share count is now zero, decrement the reference count
        // for the PFN element and turn the referenced PTE into
        // the transition state if it refers to a prototype PTE.
        // PTEs which are not prototype PTEs do not need to be placed
        // into transition as they are placed in transition when
        // they are removed from the working set (working set free routine).
        //

        //
        // If the PTE referenced by this PFN element is actually
        // a prototype PTE, it must be mapped into hyperspace and
        // then operated on.
        //

        if (Pfn1->u3.e1.PrototypePte == 1) {

            OldIrql = 99;
            if (MmIsAddressValid (Pfn1->PteAddress)) {
                PointerPte = Pfn1->PteAddress;
            } else {

                //
                // The address is not valid in this process, map it into
                // hyperspace so it can be operated upon.
                //

                PointerPte = (PMMPTE)MiMapPageInHyperSpace(Pfn1->PteFrame,
                                                           &OldIrql);
                PointerPte = (PMMPTE)((PCHAR)PointerPte +
                                        MiGetByteOffset(Pfn1->PteAddress));
            }

            TempPte = *PointerPte;
            MI_MAKE_VALID_PTE_TRANSITION (TempPte,
                                          Pfn1->OriginalPte.u.Soft.Protection);
            MI_WRITE_INVALID_PTE (PointerPte, TempPte);

            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql);
            }

            //
            // There is no need to flush the translation buffer at this
            // time as we only invalidated a prototype PTE.
            //

        }

        //
        // Change the page location to inactive (from active and valid).
        //

        Pfn1->u3.e1.PageLocation = TransitionPage;

        //
        // Decrement the reference count as the share count is now zero.
        //

        MiDecrementReferenceCount (PageFrameIndex);
    }

    return;
}

VOID
FASTCALL
MiDecrementReferenceCount (
    IN PFN_NUMBER PageFrameIndex
    )

/*++

Routine Description:

    This routine decrements the reference count for the specified page.
    If the reference count becomes zero, the page is placed on the
    appropriate list (free, modified, standby or bad).  If the page
    is placed on the free or standby list, the number of available
    pages is incremented and if it transitions from zero to one, the
    available page event is set.


Arguments:

    PageFrameIndex - Supplies the physical page number of which to
                     decrement the reference count.

Return Value:

    none.

Environment:

    Must be holding the PFN database mutex with APCs disabled.

--*/

{
    PMMPFN Pfn1;

    MM_PFN_LOCK_ASSERT();

    ASSERT (PageFrameIndex <= MmHighestPhysicalPage);

    Pfn1 = MI_PFN_ELEMENT (PageFrameIndex);
    ASSERT (Pfn1->u3.e2.ReferenceCount != 0);
    Pfn1->u3.e2.ReferenceCount -= 1;


    if (Pfn1->u3.e2.ReferenceCount != 0) {

        //
        // The reference count is not zero, return.
        //

        return;
    }

    //
    // The reference count is now zero, put the page on some
    // list.
    //


    if (Pfn1->u2.ShareCount != 0) {

        KeBugCheckEx (PFN_LIST_CORRUPT,
                      7,
                      PageFrameIndex,
                      Pfn1->u2.ShareCount,
                      0);
        return;
    }

    ASSERT (Pfn1->u3.e1.PageLocation != ActiveAndValid);

    if (MI_IS_PFN_DELETED (Pfn1)) {

        //
        // There is no referenced PTE for this page, delete the page file
        // space (if any), and place the page on the free list.
        //

        MiReleasePageFileSpace (Pfn1->OriginalPte);

        if (Pfn1->u3.e1.RemovalRequested == 1) {
            MiInsertPageInList (MmPageLocationList[BadPageList],
                                PageFrameIndex);
        }
        else {
            MiInsertPageInList (MmPageLocationList[FreePageList],
                                PageFrameIndex);
        }

        return;
    }

    //
    // Place the page on the modified or standby list depending
    // on the state of the modify bit in the PFN element.
    //

    if (Pfn1->u3.e1.Modified == 1) {
        MiInsertPageInList (MmPageLocationList[ModifiedPageList], PageFrameIndex);
    } else {

        if (Pfn1->u3.e1.RemovalRequested == 1) {

            //
            // The page may still be marked as on the modified list if the
            // current thread is the modified writer completing the write.
            // Mark it as standby so restoration of the transition PTE
            // doesn't flag this as illegal.
            //

            Pfn1->u3.e1.PageLocation = StandbyPageList;

            MiRestoreTransitionPte (PageFrameIndex);
            MiInsertPageInList (MmPageLocationList[BadPageList],
                                PageFrameIndex);
            return;
        }

        if (!MmFrontOfList) {
            MiInsertPageInList (MmPageLocationList[StandbyPageList],
                                PageFrameIndex);
        } else {
            MiInsertStandbyListAtFront (PageFrameIndex);
        }
    }

    return;
}
