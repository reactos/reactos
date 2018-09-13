/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   checkpfn.c

Abstract:

    This module contains routines for sanity checking the PFN database.

Author:

    Lou Perazzoli (loup) 25-Apr-1989

Revision History:

--*/

#include "mi.h"

#if DBG

PRTL_BITMAP CheckPfnBitMap;


VOID
MiCheckPfn (
            )

/*++

Routine Description:

    This routine checks each physical page in the PFN database to ensure
    it is in the proper state.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APCs disabled.

--*/

{
    PMMPFN Pfn1;
    PFN_NUMBER Link, Previous;
    ULONG i;
    PMMPTE PointerPte;
    KIRQL PreviousIrql;
    KIRQL OldIrql;
    USHORT ValidCheck[4];
    USHORT ValidPage[4];
    PMMPFN PfnX;

    ValidCheck[0] = ValidCheck[1] = ValidCheck[2] = ValidCheck[3] = 0;
    ValidPage[0] = ValidPage[1] = ValidPage[2] = ValidPage[3] = 0;

    if (CheckPfnBitMap == NULL) {
        MiCreateBitMap ( &CheckPfnBitMap, MmNumberOfPhysicalPages, NonPagedPool);
    }
    RtlClearAllBits (CheckPfnBitMap);

    //
    // Walk free list.
    //

    KeRaiseIrql (APC_LEVEL, &PreviousIrql);
    LOCK_PFN (OldIrql);

    Previous = MM_EMPTY_LIST;
    Link = MmFreePageListHead.Flink;
    for (i=0; i < MmFreePageListHead.Total; i++) {
        if (Link == MM_EMPTY_LIST) {
            DbgPrint("free list total count wrong\n");
            UNLOCK_PFN (OldIrql);
            KeLowerIrql (PreviousIrql);
            return;
        }
        RtlSetBits (CheckPfnBitMap, (ULONG)Link, 1L);
        Pfn1 = MI_PFN_ELEMENT(Link);
        if (Pfn1->u3.e2.ReferenceCount != 0) {
            DbgPrint("non zero reference count on free list\n");
            MiFormatPfn(Pfn1);

        }
        if (Pfn1->u3.e1.PageLocation != FreePageList) {
            DbgPrint("page location not freelist\n");
            MiFormatPfn(Pfn1);
        }
        if (Pfn1->u2.Blink != Previous) {
            DbgPrint("bad blink on free list\n");
            MiFormatPfn(Pfn1);
        }
        Previous = Link;
        Link = Pfn1->u1.Flink;

    }
    if (Link != MM_EMPTY_LIST) {
            DbgPrint("free list total count wrong\n");
            Pfn1 = MI_PFN_ELEMENT(Link);
            MiFormatPfn(Pfn1);
    }

    //
    // Walk zeroed list.
    //

    Previous = MM_EMPTY_LIST;
    Link = MmZeroedPageListHead.Flink;
    for (i=0; i < MmZeroedPageListHead.Total; i++) {
        if (Link == MM_EMPTY_LIST) {
            DbgPrint("zero list total count wrong\n");
            UNLOCK_PFN (OldIrql);
            KeLowerIrql (PreviousIrql);
            return;
        }
        RtlSetBits (CheckPfnBitMap, (ULONG)Link, 1L);
        Pfn1 = MI_PFN_ELEMENT(Link);
        if (Pfn1->u3.e2.ReferenceCount != 0) {
            DbgPrint("non zero reference count on zero list\n");
            MiFormatPfn(Pfn1);

        }
        if (Pfn1->u3.e1.PageLocation != ZeroedPageList) {
            DbgPrint("page location not zerolist\n");
            MiFormatPfn(Pfn1);
        }
        if (Pfn1->u2.Blink != Previous) {
            DbgPrint("bad blink on zero list\n");
            MiFormatPfn(Pfn1);
        }
        Previous = Link;
        Link = Pfn1->u1.Flink;

    }
    if (Link != MM_EMPTY_LIST) {
            DbgPrint("zero list total count wrong\n");
            Pfn1 = MI_PFN_ELEMENT(Link);
            MiFormatPfn(Pfn1);
    }

    //
    // Walk Bad list.
    //
    Previous = MM_EMPTY_LIST;
    Link = MmBadPageListHead.Flink;
    for (i=0; i < MmBadPageListHead.Total; i++) {
        if (Link == MM_EMPTY_LIST) {
            DbgPrint("Bad list total count wrong\n");
            UNLOCK_PFN (OldIrql);
            KeLowerIrql (PreviousIrql);
            return;
        }
        RtlSetBits (CheckPfnBitMap, (ULONG)Link, 1L);
        Pfn1 = MI_PFN_ELEMENT(Link);
        if (Pfn1->u3.e2.ReferenceCount != 0) {
            DbgPrint("non zero reference count on Bad list\n");
            MiFormatPfn(Pfn1);

        }
        if (Pfn1->u3.e1.PageLocation != BadPageList) {
            DbgPrint("page location not Badlist\n");
            MiFormatPfn(Pfn1);
        }
        if (Pfn1->u2.Blink != Previous) {
            DbgPrint("bad blink on Bad list\n");
            MiFormatPfn(Pfn1);
        }
        Previous = Link;
        Link = Pfn1->u1.Flink;

    }
    if (Link != MM_EMPTY_LIST) {
            DbgPrint("Bad list total count wrong\n");
            Pfn1 = MI_PFN_ELEMENT(Link);
            MiFormatPfn(Pfn1);
    }

    //
    // Walk Standby list.
    //

    Previous = MM_EMPTY_LIST;
    Link = MmStandbyPageListHead.Flink;
    for (i=0; i < MmStandbyPageListHead.Total; i++) {
        if (Link == MM_EMPTY_LIST) {
            DbgPrint("Standby list total count wrong\n");
            UNLOCK_PFN (OldIrql);
            KeLowerIrql (PreviousIrql);
            return;
        }
        RtlSetBits (CheckPfnBitMap, (ULONG)Link, 1L);
        Pfn1 = MI_PFN_ELEMENT(Link);
        if (Pfn1->u3.e2.ReferenceCount != 0) {
            DbgPrint("non zero reference count on Standby list\n");
            MiFormatPfn(Pfn1);

        }
        if (Pfn1->u3.e1.PageLocation != StandbyPageList) {
            DbgPrint("page location not Standbylist\n");
            MiFormatPfn(Pfn1);
        }
        if (Pfn1->u2.Blink != Previous) {
            DbgPrint("bad blink on Standby list\n");
            MiFormatPfn(Pfn1);
        }

        //
        // Check to see if referenced PTE is okay.
        //
        if (MI_IS_PFN_DELETED (Pfn1)) {
            DbgPrint("Invalid pteaddress in standby list\n");
            MiFormatPfn(Pfn1);

        } else {

            OldIrql = 99;
            if ((Pfn1->u3.e1.PrototypePte == 1) &&
                            (MmIsAddressValid (Pfn1->PteAddress))) {
                PointerPte = Pfn1->PteAddress;
            } else {
                PointerPte = MiMapPageInHyperSpace(Pfn1->PteFrame,
                                                   &OldIrql);
                PointerPte = (PMMPTE)((ULONG_PTR)PointerPte +
                                    MiGetByteOffset(Pfn1->PteAddress));
            }
            if (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerPte) != Link) {
                DbgPrint("Invalid PFN - PTE address is wrong in standby list\n");
                MiFormatPfn(Pfn1);
                MiFormatPte(PointerPte);
            }
            if (PointerPte->u.Soft.Transition == 0) {
                DbgPrint("Pte not in transition for page on standby list\n");
                MiFormatPfn(Pfn1);
                MiFormatPte(PointerPte);
            }
            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql);
                OldIrql = 99;
            }

        }

        Previous = Link;
        Link = Pfn1->u1.Flink;

    }
    if (Link != MM_EMPTY_LIST) {
            DbgPrint("Standby list total count wrong\n");
            Pfn1 = MI_PFN_ELEMENT(Link);
            MiFormatPfn(Pfn1);
    }

    //
    // Walk Modified list.
    //

    Previous = MM_EMPTY_LIST;
    Link = MmModifiedPageListHead.Flink;
    for (i=0; i < MmModifiedPageListHead.Total; i++) {
        if (Link == MM_EMPTY_LIST) {
            DbgPrint("Modified list total count wrong\n");
            UNLOCK_PFN (OldIrql);
            KeLowerIrql (PreviousIrql);
            return;
        }
        RtlSetBits (CheckPfnBitMap, (ULONG)Link, 1L);
        Pfn1 = MI_PFN_ELEMENT(Link);
        if (Pfn1->u3.e2.ReferenceCount != 0) {
            DbgPrint("non zero reference count on Modified list\n");
            MiFormatPfn(Pfn1);

        }
        if (Pfn1->u3.e1.PageLocation != ModifiedPageList) {
            DbgPrint("page location not Modifiedlist\n");
            MiFormatPfn(Pfn1);
        }
        if (Pfn1->u2.Blink != Previous) {
            DbgPrint("bad blink on Modified list\n");
            MiFormatPfn(Pfn1);
        }
        //
        // Check to see if referenced PTE is okay.
        //
        if (MI_IS_PFN_DELETED (Pfn1)) {
            DbgPrint("Invalid pteaddress in modified list\n");
            MiFormatPfn(Pfn1);

        } else {

            if ((Pfn1->u3.e1.PrototypePte == 1) &&
                            (MmIsAddressValid (Pfn1->PteAddress))) {
                PointerPte = Pfn1->PteAddress;
            } else {
                PointerPte = MiMapPageInHyperSpace(Pfn1->PteFrame, &OldIrql);
                PointerPte = (PMMPTE)((ULONG_PTR)PointerPte +
                                    MiGetByteOffset(Pfn1->PteAddress));
            }

            if (MI_GET_PAGE_FRAME_FROM_TRANSITION_PTE (PointerPte) != Link) {
                DbgPrint("Invalid PFN - PTE address is wrong in modified list\n");
                MiFormatPfn(Pfn1);
                MiFormatPte(PointerPte);
            }
            if (PointerPte->u.Soft.Transition == 0) {
                DbgPrint("Pte not in transition for page on modified list\n");
                MiFormatPfn(Pfn1);
                MiFormatPte(PointerPte);
            }

            if (OldIrql != 99) {
                MiUnmapPageInHyperSpace (OldIrql);
                OldIrql = 99;
            }
        }

        Previous = Link;
        Link = Pfn1->u1.Flink;

    }
    if (Link != MM_EMPTY_LIST) {
            DbgPrint("Modified list total count wrong\n");
            Pfn1 = MI_PFN_ELEMENT(Link);
            MiFormatPfn(Pfn1);
    }
    //
    // All non active pages have been scanned.  Locate the
    // active pages and make sure they are consistent.
    //

    //
    // set bit zero as page zero is reserved for now
    //

    RtlSetBits (CheckPfnBitMap, 0L, 1L);

    Link = RtlFindClearBitsAndSet (CheckPfnBitMap, 1L, 0);
    while (Link != 0xFFFFFFFF) {
        Pfn1 = MI_PFN_ELEMENT (Link);

        //
        // Make sure the PTE address is okay
        //

        if ((Pfn1->PteAddress >= (PMMPTE)HYPER_SPACE)
                && (Pfn1->u3.e1.PrototypePte == 0)) {
            DbgPrint("pfn with illegal pte address\n");
            MiFormatPfn(Pfn1);
            break;
        }

        if (Pfn1->PteAddress < (PMMPTE)PTE_BASE) {
            DbgPrint("pfn with illegal pte address\n");
            MiFormatPfn(Pfn1);
            break;
        }

#if defined(_IA64_)

        //
        // ignore PTEs mapped to IA64 kernel BAT.
        //

        if (MI_IS_PHYSICAL_ADDRESS(MiGetVirtualAddressMappedByPte(Pfn1->PteAddress))) {

            goto NoCheck;
        }
#endif // _IA64_

#ifdef _ALPHA_

        //
        // ignore ptes mapped to ALPHA's 32-bit superpage.
        //

        if ((Pfn1->PteAddress > (PMMPTE)(ULONG_PTR)0xc0100000) &&
            (Pfn1->PteAddress < (PMMPTE)(ULONG_PTR)0xc0180000)) {

            goto NoCheck;
        }
#endif //ALPHA

        //
        // Check to make sure the referenced PTE is for this page.
        //

        if ((Pfn1->u3.e1.PrototypePte == 1) &&
                            (MmIsAddressValid (Pfn1->PteAddress))) {
            PointerPte = Pfn1->PteAddress;
        } else {
            PointerPte = MiMapPageInHyperSpace(Pfn1->PteFrame, &OldIrql);
            PointerPte = (PMMPTE)((ULONG_PTR)PointerPte +
                                    MiGetByteOffset(Pfn1->PteAddress));
        }

        if (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) != Link) {
            DbgPrint("Invalid PFN - PTE address is wrong in active list\n");
            MiFormatPfn(Pfn1);
            MiFormatPte(PointerPte);
        }
        if (PointerPte->u.Hard.Valid == 0) {
            //
            // if the page is a page table page it could be out of
            // the working set yet a transition page is keeping it
            // around in memory (ups the share count).
            //

            if ((Pfn1->PteAddress < (PMMPTE)PDE_BASE) ||
                (Pfn1->PteAddress > (PMMPTE)PDE_TOP)) {

                DbgPrint("Pte not valid for page on active list\n");
                MiFormatPfn(Pfn1);
                MiFormatPte(PointerPte);
            }
        }

        if (Pfn1->u3.e2.ReferenceCount != 1) {
            DbgPrint("refcount not 1\n");
            MiFormatPfn(Pfn1);
        }


        //
        // Check to make sure the PTE count for the frame is okay.
        //

        if (Pfn1->u3.e1.PrototypePte == 1) {
            PfnX = MI_PFN_ELEMENT(Pfn1->PteFrame);
            for (i = 0; i < 4; i++) {
                if (ValidPage[i] == 0) {
                    ValidPage[i] = (USHORT)Pfn1->PteFrame;
                }
                if (ValidPage[i] == (USHORT)Pfn1->PteFrame) {
                    ValidCheck[i] += 1;
                    break;
                }
            }
        }
        if (OldIrql != 99) {
            MiUnmapPageInHyperSpace (OldIrql);
            OldIrql = 99;
        }

#if defined(_ALPHA_) || defined(_IA64_)
NoCheck:
#endif
        Link = RtlFindClearBitsAndSet (CheckPfnBitMap, 1L, 0);

    }

    for (i = 0; i < 4; i++) {
        if (ValidPage[i] == 0) {
            break;
        }
        PfnX = MI_PFN_ELEMENT(ValidPage[i]);
    }

    UNLOCK_PFN (OldIrql);
    KeLowerIrql (PreviousIrql);
    return;

}

VOID
MiDumpPfn ( )

{
    ULONG i;
    PMMPFN Pfn1;

    Pfn1 = MI_PFN_ELEMENT (MmLowestPhysicalPage);

    for (i=0; i < MmNumberOfPhysicalPages; i++) {
        MiFormatPfn (Pfn1);
        Pfn1++;
    }
    return;
}

VOID
MiFormatPfn (
    IN PMMPFN PointerPfn
    )

{

    MMPFN Pfn;
    ULONG i;

    Pfn = *PointerPfn;
    i = (ULONG)(PointerPfn - MmPfnDatabase);

    DbgPrint("***PFN %lx  flink %lx  blink %lx  ptecount-refcnt %lx\n",
        i,
        Pfn.u1.Flink,
        Pfn.u2.Blink,
        Pfn.u3.e2.ReferenceCount);

    DbgPrint("   pteaddr %p  originalPTE %p  flags %lx \n",
        Pfn.PteAddress,
        Pfn.OriginalPte,
        Pfn.u3.e2.ShortFlags);

    return;

}
#endif //DBG
