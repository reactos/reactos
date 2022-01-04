/*
 * COPYRIGHT:       GPL, See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/amd64/procsup.c
 * PURPOSE:         Low level memory managment manipulation
 *
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 *                  ReactOS Portable Systems Group
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
    KIRQL OldIrql;
    PFN_NUMBER TableBasePfn, HyperPfn, HyperPdPfn, HyperPtPfn;
    PMMPTE SystemPte;
    MMPTE TempPte, PdePte;
    ULONG TableIndex;
    PMMPTE PageTablePointer;
    ULONG PageColor;

    /* Non-arch specific code-path allocated those for us */
    TableBasePfn = DirectoryTableBase[0] >> PAGE_SHIFT;
    HyperPfn = DirectoryTableBase[1] >> PAGE_SHIFT;

    /*
     * Lock PFN database. Try getting zero pages.
     * If that doesn't work, we take the slow path
     * outside of the PFN lock.
     */
    OldIrql = MiAcquirePfnLock();
    PageColor = MI_GET_NEXT_PROCESS_COLOR(Process);
    HyperPdPfn = MiRemoveZeroPageSafe(PageColor);
    if(!HyperPdPfn)
    {
        HyperPdPfn = MiRemoveAnyPage(PageColor);
        MiReleasePfnLock(OldIrql);
        MiZeroPhysicalPage(HyperPdPfn);
        OldIrql = MiAcquirePfnLock();
    }
    PageColor = MI_GET_NEXT_PROCESS_COLOR(Process);
    HyperPtPfn = MiRemoveZeroPageSafe(PageColor);
    if(!HyperPtPfn)
    {
        HyperPtPfn = MiRemoveAnyPage(PageColor);
        MiReleasePfnLock(OldIrql);
        MiZeroPhysicalPage(HyperPtPfn);
    }
    else
    {
        MiReleasePfnLock(OldIrql);
    }

    /* Get a PTE to map the page directory */
    SystemPte = MiReserveSystemPtes(1, SystemPteSpace);
    if (!SystemPte)
        return FALSE;

    /* Get its address */
    PageTablePointer = MiPteToAddress(SystemPte);

    /* Build the PTE for the page directory and map it */
    MI_MAKE_HARDWARE_PTE_KERNEL(&PdePte, SystemPte, MM_READWRITE, TableBasePfn);
    MI_WRITE_VALID_PTE(SystemPte, PdePte);

    /* Copy the kernel mappings and zero out the rest */
    TableIndex = PXE_PER_PAGE / 2;
    RtlZeroMemory(PageTablePointer, TableIndex * sizeof(MMPTE));
    RtlCopyMemory(PageTablePointer + TableIndex,
                  MiAddressToPxe(0) + TableIndex,
                  PAGE_SIZE - TableIndex * sizeof(MMPTE));

    /* Sanity check */
    ASSERT(MiAddressToPxi(MmHyperSpaceEnd) >= TableIndex);

    /* Setup a PTE for the page directory mappings */
    TempPte = ValidKernelPte;

    /* Update the self mapping of the PML4 */
    TableIndex = MiAddressToPxi((PVOID)PXE_SELFMAP);
    TempPte.u.Hard.PageFrameNumber = TableBasePfn;
    PageTablePointer[TableIndex] = TempPte;

    /* Write the PML4 entry for hyperspace */
    TableIndex = MiAddressToPxi((PVOID)HYPER_SPACE);
    TempPte.u.Hard.PageFrameNumber = HyperPfn;
    PageTablePointer[TableIndex] = TempPte;

    /* Map the hyperspace PDPT to the system PTE */
    PdePte.u.Hard.PageFrameNumber = HyperPfn;
    *SystemPte = PdePte;
    __invlpg(PageTablePointer);

    /* Write the hyperspace entry for the first PD */
    TempPte.u.Hard.PageFrameNumber = HyperPdPfn;
    PageTablePointer[0] = TempPte;

    /* Map the hyperspace PD to the system PTE */
    PdePte.u.Hard.PageFrameNumber = HyperPdPfn;
    *SystemPte = PdePte;
    __invlpg(PageTablePointer);

    /* Write the hyperspace entry for the first PT */
    TempPte.u.Hard.PageFrameNumber = HyperPtPfn;
    PageTablePointer[0] = TempPte;

    /* Map the hyperspace PT to the system PTE */
    PdePte.u.Hard.PageFrameNumber = HyperPtPfn;
    *SystemPte = PdePte;
    __invlpg(PageTablePointer);

    /* Write the hyperspace PTE for the working set list index */
    TempPte.u.Hard.PageFrameNumber = Process->WorkingSetPage;
    TableIndex = MiAddressToPti(MmWorkingSetList);
    PageTablePointer[TableIndex] = TempPte;

    /* Release the system PTE */
    MiReleaseSystemPtes(SystemPte, 1, SystemPteSpace);

    return TRUE;
}
