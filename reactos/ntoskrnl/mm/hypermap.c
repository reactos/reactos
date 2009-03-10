/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/hypermap.c
 * PURPOSE:         Hyperspace Mapping Functionality
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PMMPTE MmFirstReservedMappingPte;
PMMPTE MmLastReservedMappingPte;
MMPTE HyperTemplatePte;
PEPROCESS HyperProcess;
KIRQL HyperIrql;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiInitHyperSpace(VOID)
{
    PMMPTE PointerPte;
    
    //
    // Get the hyperspace PTE and zero out the page table
    //
    PointerPte = MiAddressToPte(HYPER_SPACE);
    RtlZeroMemory(PointerPte, PAGE_SIZE);
    
    //
    // Setup mapping PTEs
    //
    MmFirstReservedMappingPte = MiAddressToPte(MI_MAPPING_RANGE_START);
    MmLastReservedMappingPte =  MiAddressToPte(MI_MAPPING_RANGE_END);
    MmFirstReservedMappingPte->u.Hard.PageFrameNumber = MI_HYPERSPACE_PTES;
}

PVOID
NTAPI
MiMapPageInHyperSpace(IN PEPROCESS Process,
                      IN PFN_NUMBER Page,
                      IN PKIRQL OldIrql)
{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER Offset;
    PVOID Address; 
    
    //
    // Never accept page 0
    //
    ASSERT(Page != 0);
    
    //
    // Build the PTE
    //
    TempPte = HyperTemplatePte;
    TempPte.u.Hard.PageFrameNumber = Page;
    
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
    Address = (PVOID)((ULONG_PTR)PointerPte << 10);
    return Address;
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
MiMapPagesToZeroInHyperSpace(IN PFN_NUMBER Page)
{
    MMPTE TempPte;
    PMMPTE PointerPte;
    PFN_NUMBER Offset;
    PVOID Address; 
    
    //
    // Never accept page 0
    //
    ASSERT(Page != 0);
    
    //
    // Build the PTE
    //
    TempPte = HyperTemplatePte;
    TempPte.u.Hard.PageFrameNumber = Page;
    
    //
    // Pick the first hyperspace PTE
    //
    PointerPte = MmFirstReservedMappingPte;

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
    Address = (PVOID)((ULONG_PTR)PointerPte << 10);
    return Address;
}

VOID
NTAPI
MiUnmapPagesInZeroSpace(IN PVOID Address)
{
    //
    // Blow away the mapping
    //
    MiAddressToPte(Address)->u.Long = 0;
}
