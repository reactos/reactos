/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

   checkpte.c

Abstract:

    This module contains routines for sanity checking the page directory.

Author:

    Lou Perazzoli (loup) 25-Apr-1989

Revision History:

--*/

#include "mi.h"

#if DBG
#if 0       // LWFIX: fix this up for Sundown before re-enabling

VOID
CheckValidPte (
    IN PMMPTE PointerPte
    );

VOID
CheckInvalidPte (
    IN PMMPTE PointerPte
    );


VOID
MiCheckPte (
    VOID
    )

/*++

Routine Description:

    This routine checks each page table page in an address space to
    ensure it is in the proper state.

Arguments:

    None.

Return Value:

    None.

Environment:

    Kernel mode, APC's disabled.

--*/

{
    ULONG i,j;
    PMMPTE PointerPte;
    PMMPTE PointerPde;
    PMMPFN Pfn1;
    ULONG ValidCount;
    ULONG TransitionCount;
    KIRQL PreviousIrql;
    KIRQL OldIrql;
    PEPROCESS TargetProcess;
    USHORT UsedPages;
    ULONG PdeValidCount;

    TargetProcess = PsGetCurrentProcess ();

    KeRaiseIrql (APC_LEVEL, &PreviousIrql);

    LOCK_WS (TargetProcess);
    LOCK_PFN (OldIrql);

    PointerPde = MiGetPdeAddress(0);

    UsedPages = 0;
    PdeValidCount = 1;

    for (i = 0; i < PDE_PER_PAGE; i++) {
        if (PointerPde->u.Hard.Valid) {

            if ((i < 512) || (i == 769) || (i== 896) ) {
                PdeValidCount += 1;
            }

            ValidCount = 0;
            TransitionCount = 0;
            CheckValidPte (PointerPde);

            PointerPte = MiGetPteAddress (i<<22);

            for (j=0; j < PTE_PER_PAGE; j++) {

                if ((PointerPte >= MiGetPteAddress(HYPER_SPACE)) &&
                        (PointerPte < MiGetPteAddress(WORKING_SET_LIST))) {
                    goto endloop;
                }

                if (PointerPte->u.Hard.Valid) {
                    ValidCount += 1;
                    CheckValidPte (PointerPte);

                } else {
                    CheckInvalidPte (PointerPte);

                    if ((PointerPte->u.Soft.Transition == 1) &&
                        (PointerPte->u.Soft.Prototype == 0)) {

                        //
                        // Transition PTE, up the transition count.
                        //

                        TransitionCount += 1;

                    }
                }

                if (PointerPte->u.Long != 0) {
                    UsedPages += 1;
                }
endloop:
                PointerPte++;

            }
            if ((i < 512) || (i == 896)) {
                if (MmWorkingSetList->UsedPageTableEntries[i] != UsedPages) {
                   DbgPrint("used pages and page table used not equal %lx %lx %lx\n",
                    i,MmWorkingSetList->UsedPageTableEntries[i], UsedPages);
                }
            }

            //
            // Check the share count for the page table page.
            //
            if ((i < 511) || (i == 896)) {
                Pfn1 = MI_PFN_ELEMENT (PointerPde->u.Hard.PageFrameNumber);
                if (Pfn1->u2.ShareCount != ((ULONG)1+ValidCount+TransitionCount)) {
                    DbgPrint("share count for page table page bad - %lx %lx %lx\n",
                        i,ValidCount, TransitionCount);
                    MiFormatPfn(Pfn1);
                }
            }
        }
        PointerPde++;
        UsedPages = 0;
    }

    PointerPde = (PMMPTE)(ULONG_PTR)0xc0300c00;
    Pfn1 = MI_PFN_ELEMENT(PointerPde->u.Hard.PageFrameNumber);
    UNLOCK_PFN (OldIrql);
    UNLOCK_WS (TargetProcess);
    KeLowerIrql (PreviousIrql);
    return;

}

VOID
CheckValidPte (
    IN PMMPTE PointerPte
    )

{
    PMMPFN Pfn1;
    PMMPTE PointerPde;

    if (MI_GET_PAGE_FRAME_FROM_PTE (PointerPte) > MmNumberOfPhysicalPages) {
        return;
    }


    Pfn1 = MI_PFN_ELEMENT(PointerPte->u.Hard.PageFrameNumber);

    if (PointerPte->u.Hard.PageFrameNumber == 0) {
        DbgPrint("physical page zero mapped\n");
        MiFormatPte(PointerPte);
        MiFormatPfn(Pfn1);
    }

    if (Pfn1->u3.e1.PageLocation != ActiveAndValid) {
        DbgPrint("valid PTE with page frame not active and valid\n");
        MiFormatPfn(Pfn1);
        MiFormatPte(PointerPte);
    }

    if (Pfn1->u3.e1.PrototypePte == 0) {
        //
        // This is not a prototype PTE.
        //
        if (Pfn1->PteAddress != PointerPte) {
            DbgPrint("checkpte - pfn pte address and pte address not equal\n");
            MiFormatPte(PointerPte);
            MiFormatPfn(Pfn1);
            return;
        }

    }

    if (!MmIsAddressValid(Pfn1->PteAddress)) {
        return;
    }

    PointerPde = MiGetPteAddress (Pfn1->PteAddress);
    if (PointerPde->u.Hard.Valid == 1) {

        if (MI_GET_PAGE_FRAME_FROM_PTE (PointerPde) != Pfn1->PteFrame) {
                DbgPrint("checkpte - pteframe not right\n");
                MiFormatPfn(Pfn1);
                MiFormatPte(PointerPte);
                MiFormatPte(PointerPde);
        }
    }
    return;

}

VOID
CheckInvalidPte (
    IN PMMPTE PointerPte
    )


{
    PointerPte;
    return;

}
#endif // 0
#endif //DBG
