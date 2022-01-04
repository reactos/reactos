/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD-3-Clause (https://spdx.org/licenses/BSD-3-Clause.html)
 * FILE:            ntoskrnl/mm/i386/procsup.c
 * PURPOSE:         Process handling for i386 architecture
 * PROGRAMMERS:     Jérôme Gardou
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

BOOLEAN
MiArchCreateProcessAddressSpace(
    _In_ PEPROCESS Process,
    _In_ PULONG_PTR DirectoryTableBase)
{
    PFN_NUMBER PdeIndex = DirectoryTableBase[0] >> PAGE_SHIFT;
    PFN_NUMBER HyperIndex = DirectoryTableBase[1] >> PAGE_SHIFT;
    PMMPTE PointerPte;
    MMPTE PdePte, TempPte;
    PMMPTE PteTable;
    ULONG PdeOffset;
    KIRQL OldIrql;

    /* Get a PTE  */
    PointerPte = MiReserveSystemPtes(1, SystemPteSpace);
    if (!PointerPte)
        return FALSE;
    PteTable = MiPteToAddress(PointerPte);

    /* Build a page table for hyper space */
    MI_MAKE_HARDWARE_PTE_KERNEL(&PdePte,
                                PointerPte,
                                MM_READWRITE,
                                HyperIndex);

    /* Set it dirty and map it */
    MI_MAKE_DIRTY_PAGE(&PdePte);
    MI_WRITE_VALID_PTE(PointerPte, PdePte);

    /* Now write the PTE/PDE entry for the working set list index itself */
    TempPte = ValidKernelPteLocal;
    TempPte.u.Hard.PageFrameNumber = Process->WorkingSetPage;
    PdeOffset = MiAddressToPteOffset(MmWorkingSetList);
    PteTable[PdeOffset] = TempPte;

    /* Now we map the page directory */
    MI_MAKE_HARDWARE_PTE_KERNEL(&PdePte,
                                PointerPte,
                                MM_READWRITE,
                                PdeIndex);

    /* Set it dirty and map it */
    MI_MAKE_DIRTY_PAGE(&PdePte);
    *PointerPte = PdePte;
    /* We changed the page! */
    __invlpg(PteTable);

    /* Now get the page directory (which we'll double map, so call it a page table) */
    PteTable = MiPteToAddress(PointerPte);

    /* Copy all the kernel mappings */
    PdeOffset = MiGetPdeOffset(MmSystemRangeStart);
    RtlCopyMemory(&PteTable[PdeOffset],
                  MiAddressToPde(MmSystemRangeStart),
                  PAGE_SIZE - PdeOffset * sizeof(MMPTE));

    /* Now write the PTE/PDE entry for hyperspace itself */
    TempPte = ValidKernelPteLocal;
    TempPte.u.Hard.PageFrameNumber = HyperIndex;
    PdeOffset = MiGetPdeOffset(HYPER_SPACE);
    PteTable[PdeOffset] = TempPte;

    /* Sanity check */
    PdeOffset++;
    ASSERT(MiGetPdeOffset(MmHyperSpaceEnd) >= PdeOffset);

    /* Now do the x86 trick of making the PDE a page table itself */
    PdeOffset = MiGetPdeOffset(PTE_BASE);
    TempPte.u.Hard.PageFrameNumber = PdeIndex;
    PteTable[PdeOffset] = TempPte;

    /* Let go of the system PTE */
    MiReleaseSystemPtes(PointerPte, 1, SystemPteSpace);

    /* Insert us into the Mm process list */
    OldIrql = MiAcquireExpansionLock();
    InsertTailList(&MmProcessList, &Process->MmProcessLinks);
    MiReleaseExpansionLock(OldIrql);

    return TRUE;
}
