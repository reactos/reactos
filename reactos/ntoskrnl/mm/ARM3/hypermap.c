/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/hypermap.c
 * PURPOSE:         Hyperspace Mapping Functionality
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PMMPTE MmFirstReservedMappingPte, MmLastReservedMappingPte;
PMMPTE MiFirstReservedZeroingPte;
MMPTE HyperTemplatePte;
PEPROCESS HyperProcess;
KIRQL HyperIrql;

/* PRIVATE FUNCTIONS **********************************************************/

PVOID
NTAPI
MiMapPageInHyperSpace(IN PEPROCESS Process,
                      IN PFN_NUMBER Page,
                      IN PKIRQL OldIrql)
{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER Offset;

    //
    // Never accept page 0
    //
    ASSERT(Page != 0);

    //
    // Build the PTE
    //
    TempPte = HyperTemplatePte;
    TempPte.u.Hard.PageFrameNumber = Page;
    TempPte.u.Hard.Global = 0; // Hyperspace is local!

    //
    // Pick the first hyperspace PTE
    //
    PointerPte = MmFirstReservedMappingPte;

    //
    // Acquire the hyperlock
    //
    ASSERT(Process == PsGetCurrentProcess());
    KeAcquireSpinLock(&Process->HyperSpaceLock, OldIrql);

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
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(TempPte.u.Hard.Valid == 1);
    *PointerPte = TempPte;

    //
    // Return the address
    //
    return MiPteToAddress(PointerPte);
}

VOID
NTAPI
MiUnmapPageInHyperSpace(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN KIRQL OldIrql)
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
    KeReleaseSpinLock(&Process->HyperSpaceLock, OldIrql);
}

PVOID
NTAPI
MiMapPagesToZeroInHyperSpace(IN PMMPFN *Pages,
                             IN PFN_NUMBER NumberOfPages)
{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER Offset, PageFrameIndex;
    PMMPFN Page;

    //
    // Sanity checks
    //
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT(NumberOfPages != 0);
    ASSERT(NumberOfPages <= (MI_ZERO_PTES - 1));
    
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
        Offset = MI_ZERO_PTES - 1;
        PointerPte->u.Hard.PageFrameNumber = Offset;
        KeFlushProcessTb();
    }
    
    //
    // Prepare the next PTE
    //
    PointerPte->u.Hard.PageFrameNumber = Offset - NumberOfPages;
   
    //
    // Write the current PTE
    //
    PointerPte += (Offset + 1);
    TempPte = HyperTemplatePte;
    TempPte.u.Hard.Global = FALSE; // Hyperspace is local!
    do
    {
        //
        // Get the first page entry and its PFN
        //
        Page = *Pages++;
        PageFrameIndex = MiGetPfnEntryIndex(Page);
        
        //
        // Write the PFN
        //
        TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
        
        //
        // Set the correct PTE to write to, and set its new value
        //
        PointerPte--;
        ASSERT(PointerPte->u.Hard.Valid == 0);
        ASSERT(TempPte.u.Hard.Valid == 1);
        *PointerPte = TempPte;
    } while (--NumberOfPages);
    
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
    ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
    ASSERT (NumberOfPages != 0);
    ASSERT (NumberOfPages <= (MI_ZERO_PTES - 1));
    
    //
    // Get the first PTE for the mapped zero VA
    //
    PointerPte = MiAddressToPte(VirtualAddress);

    //
    // Blow away the mapped zero PTEs
    //
    RtlZeroMemory(PointerPte, NumberOfPages * sizeof(MMPTE));
}

