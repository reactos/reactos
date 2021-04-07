/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/hypermap.c
 * PURPOSE:         ARM Memory Manager Hyperspace Mapping Functionality
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define MODULE_INVOLVED_IN_ARM3
#include <mm/ARM3/miarm.h>

/* GLOBALS ********************************************************************/

PMMPTE MmFirstReservedMappingPte, MmLastReservedMappingPte;
PMMPTE MiFirstReservedZeroingPte;
MMPTE HyperTemplatePte;

/* PRIVATE FUNCTIONS **********************************************************/

_Acquires_lock_(Process->HyperSpaceLock)
_When_(OldIrql == 0, _IRQL_requires_(DISPATCH_LEVEL))
_When_(OldIrql != 0, _IRQL_requires_(PASSIVE_LEVEL))
_When_(OldIrql != 0, _At_(*OldIrql, _IRQL_saves_))
_When_(OldIrql != 0, _IRQL_raises_(DISPATCH_LEVEL))
PVOID
NTAPI
MiMapPageInHyperSpace(_In_ PEPROCESS Process,
                      _In_ PFN_NUMBER Page,
                      _Out_opt_ PKIRQL OldIrql)
{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER Offset;

    ASSERT(((OldIrql != NULL) && (KeGetCurrentIrql() == PASSIVE_LEVEL))
        || ((OldIrql == NULL) && (KeGetCurrentIrql() == DISPATCH_LEVEL)));

    //
    // Never accept page 0 or non-physical pages
    //
    ASSERT(Page != 0);
    ASSERT(MiGetPfnEntry(Page) != NULL);

    //
    // Build the PTE
    //
    TempPte = ValidKernelPteLocal;
    TempPte.u.Hard.PageFrameNumber = Page;

    //
    // Pick the first hyperspace PTE
    //
    PointerPte = MmFirstReservedMappingPte;

    //
    // Acquire the hyperlock
    //
    ASSERT(Process == PsGetCurrentProcess());
    if (OldIrql != NULL)
        KeAcquireSpinLock(&Process->HyperSpaceLock, OldIrql);
    else
        KeAcquireSpinLockAtDpcLevel(&Process->HyperSpaceLock);

    //
    // Now get the first free PTE
    //
    Offset = PFN_FROM_PTE(PointerPte);
    if (!Offset)
    {
        //
        // Reset the PTEs
        //
        Offset = MI_HYPERSPACE_PTES;
        KeFlushProcessTb();
    }

    //
    // Prepare the next PTE
    //
    PointerPte->u.Hard.PageFrameNumber = Offset - 1;

    //
    // Write the current PTE
    //
    PointerPte += Offset;
    MI_WRITE_VALID_PTE(PointerPte, TempPte);

    //
    // Return the address
    //
    return MiPteToAddress(PointerPte);
}

_Requires_lock_held_(Process->HyperSpaceLock)
_Releases_lock_(Process->HyperSpaceLock)
_IRQL_requires_(DISPATCH_LEVEL)
_When_(OldIrql != MM_NOIRQL, _At_(OldIrql, _IRQL_restores_))
VOID
NTAPI
MiUnmapPageInHyperSpace(_In_ PEPROCESS Process,
                        _In_ PVOID Address,
                        _In_ KIRQL OldIrql)
{
    ASSERT(Process == PsGetCurrentProcess());

    //
    // Blow away the mapping
    //
    MiAddressToPte(Address)->u.Long = 0;

    //
    // Release the hyperlock
    //
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    if (OldIrql == MM_NOIRQL)
        KeReleaseSpinLockFromDpcLevel(&Process->HyperSpaceLock);
    else
        KeReleaseSpinLock(&Process->HyperSpaceLock, OldIrql);
}

PVOID
NTAPI
MiMapPagesInZeroSpace(IN PMMPFN Pfn1,
                      IN PFN_NUMBER NumberOfPages)
{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER Offset, PageFrameIndex;

    //
    // Sanity checks
    //
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT(NumberOfPages != 0);
    ASSERT(NumberOfPages <= MI_ZERO_PTES);

    //
    // Pick the first zeroing PTE
    //
    PointerPte = MiFirstReservedZeroingPte;

    //
    // Now get the first free PTE
    //
    Offset = PFN_FROM_PTE(PointerPte);
    if (NumberOfPages > Offset)
    {
        //
        // Reset the PTEs
        //
        Offset = MI_ZERO_PTES;
        PointerPte->u.Hard.PageFrameNumber = Offset;
        KeFlushProcessTb();
    }

    //
    // Prepare the next PTE
    //
    PointerPte->u.Hard.PageFrameNumber = Offset - NumberOfPages;

    /* Choose the correct PTE to use, and which template */
    PointerPte += (Offset + 1);
    TempPte = ValidKernelPte;

    /* Make sure the list isn't empty and loop it */
    ASSERT(Pfn1 != (PVOID)LIST_HEAD);
    while (Pfn1 != (PVOID)LIST_HEAD)
    {
        /* Get the page index for this PFN */
        PageFrameIndex = MiGetPfnEntryIndex(Pfn1);

        //
        // Write the PFN
        //
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;

        //
        // Set the correct PTE to write to, and set its new value
        //
        PointerPte--;
        MI_WRITE_VALID_PTE(PointerPte, TempPte);

        /* Move to the next PFN */
        Pfn1 = (PMMPFN)Pfn1->u1.Flink;
    }

    //
    // Return the address
    //
    return MiPteToAddress(PointerPte);
}

VOID
NTAPI
MiUnmapPagesInZeroSpace(IN PVOID VirtualAddress,
                        IN PFN_NUMBER NumberOfPages)
{
    PMMPTE PointerPte;

    //
    // Sanity checks
    //
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    ASSERT (NumberOfPages != 0);
    ASSERT(NumberOfPages <= MI_ZERO_PTES);

    //
    // Get the first PTE for the mapped zero VA
    //
    PointerPte = MiAddressToPte(VirtualAddress);

    //
    // Blow away the mapped zero PTEs
    //
    RtlZeroMemory(PointerPte, NumberOfPages * sizeof(MMPTE));
}

