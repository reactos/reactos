/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/arm/stubs.c
 * PURPOSE:         ARM Memory Manager
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ULONG MmGlobalKernelPageDirectory[1024];
MMPTE MiArmTemplatePte, MiArmTemplatePde;

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
MiUnmapPageTable(IN PMMPTE PointerPde)
{
    //
    // Check if this address belongs to the kernel
    //
    if (((ULONG_PTR)PointerPde > PDE_BASE) ||
        ((ULONG_PTR)PointerPde < (PDE_BASE + 1024*1024)))
    {
        //
        // Nothing to do
        //
        return TRUE;
    }
    
    //
    // FIXME-USER: Shouldn't get here yet
    //
    ASSERT(FALSE);
    return FALSE;
}

VOID
NTAPI
MiFlushTlb(IN PMMPTE PointerPte,
           IN PVOID Address)
{
    //
    // Make sure the PTE is valid, and unmap the pagetable if user-mode
    //
    if (((PointerPte) && (MiUnmapPageTable(PointerPte))) ||
        (Address >= MmSystemRangeStart))
    {
        //
        // Invalidate this page
        //
        KeArmInvalidateTlbEntry(Address);
    }
}

PMMPTE
NTAPI
MiGetPageTableForProcess(IN PEPROCESS Process,
                         IN PVOID Address,
                         IN BOOLEAN Create)
{
    ULONG PdeOffset;
    PMMPTE PointerPde;
    MMPTE Pte;
    NTSTATUS Status;
    PFN_NUMBER Pfn;
    
    //
    // Check if this is a user-mode, non-kernel or non-current address
    //
    if ((Address < MmSystemRangeStart) &&
        (Process) &&
        (Process != PsGetCurrentProcess()))
    {
        //
        // FIXME-USER: No user-mode memory support
        //
        ASSERT(FALSE);
    }
    
    //
    // Get the PDE
    //
    PointerPde = MiGetPdeAddress(Address);
    if (PointerPde->u.Hard.L1.Fault.Type == FaultPte)
    {
        //
        // Invalid PDE, is this a kernel address?
        //
        if (Address >= MmSystemRangeStart)
        {            
            //
            // Does it exist in the kernel page directory?
            //
            PdeOffset = MiGetPdeOffset(Address);
            if (MmGlobalKernelPageDirectory[PdeOffset] == 0)
            {
                //
                // It doesn't. Is this a create operation? If not, fail
                //
                if (Create == FALSE) return NULL;
                
                //
                // THIS WHOLE PATH IS TODO
                //
                ASSERT(FALSE);
                
                //
                // Allocate a non paged pool page for the PDE
                //
                Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
                if (!NT_SUCCESS(Status)) return NULL;
                
                //
                // Make the entry valid
                //
                Pte.u.Hard.AsUlong = 0xDEADBEEF;
                
                //
                // Save it
                //
                MmGlobalKernelPageDirectory[PdeOffset] = Pte.u.Hard.AsUlong;
            }
            
            //
            // Now set the actual PDE
            //
            PointerPde = (PMMPTE)&MmGlobalKernelPageDirectory[PdeOffset];
        }
        else
        {
            //
            // Is this a create operation? If not, fail
            //
            if (Create == FALSE) return NULL;
            
            //
            // THIS WHOLE PATH IS TODO
            //
            ASSERT(FALSE);
         
            //
            // Allocate a non paged pool page for the PDE
            //
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
            if (!NT_SUCCESS(Status)) return NULL;
            
            //
            // Make the entry valid
            //
            Pte.u.Hard.AsUlong = 0xDEADBEEF;
            
            //
            // Set it
            //
            *PointerPde = Pte;
        }
    }
    
    //
    // Return the PTE
    //
    return MiGetPteAddress(Address);
}

MMPTE
NTAPI
MiGetPageEntryForProcess(IN PEPROCESS Process,
                         IN PVOID Address)
{
    PMMPTE PointerPte;
    MMPTE Pte;
    Pte.u.Hard.AsUlong = 0;
    
    //
    // Get the PTE
    //
    PointerPte = MiGetPageTableForProcess(Process, Address, FALSE);
    if (PointerPte)
    {
        //
        // Capture the PTE value and unmap the page table
        //
        Pte = *PointerPte;
        MiUnmapPageTable(PointerPte);
    }

    //
    // Return the PTE value
    //
    return Pte;
}

VOID
NTAPI
MmDeletePageTable(IN PEPROCESS Process,
                  IN PVOID Address)
{
    PMMPTE PointerPde;
    
    //
    // Not valid for kernel addresses
    //
    DPRINT("MmDeletePageTable(%p, %p)\n", Process, Address);
    ASSERT(Address < MmSystemRangeStart);
    
    //
    // Check if this is for a different process
    //
    if ((Process) && (Process != PsGetCurrentProcess()))
    {
        //
        // FIXME-USER: Need to attach to the process
        //
        ASSERT(FALSE);
    }
    
    //
    // Get the PDE
    //
    PointerPde = MiGetPdeAddress(Address);
    
    //
    // On ARM, we use a section mapping for the original low-memory mapping
    //
    if ((Address) || (PointerPde->u.Hard.L1.Section.Type != SectionPte))
    {
        //
        // Make sure it's valid
        //
        ASSERT(PointerPde->u.Hard.L1.Coarse.Type == CoarsePte);
    }
    
    //
    // Clear the PDE
    //
    PointerPde->u.Hard.AsUlong = 0;
    ASSERT(PointerPde->u.Hard.L1.Fault.Type == FaultPte);
    
    //
    // Invalidate the TLB entry
    //
    MiFlushTlb(PointerPde, MiAddressToPte(Address));
}

BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PLARGE_INTEGER DirectoryTableBase)
{
    //
    // FIXME-USER: Need to create address space
    //
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

VOID
NTAPI
MmUpdatePageDir(IN PEPROCESS Process,
                IN PVOID Address,
                IN ULONG Size)
{
    //
    // Nothing to do
    //
    return;
}

NTSTATUS
NTAPI
Mmi386ReleaseMmInfo(IN PEPROCESS Process)
{
    //
    // FIXME-USER: Need to delete address space
    //
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

NTSTATUS
NTAPI
MmInitializeHandBuiltProcess(IN PEPROCESS Process,
                             IN PLARGE_INTEGER DirectoryTableBase)
{
    //
    // Share the directory base with the idle process
    //
    *DirectoryTableBase = PsGetCurrentProcess()->Pcb.DirectoryTableBase;
    
    //
    // Initialize the Addresss Space
    //
    KeInitializeGuardedMutex(&Process->AddressCreationLock);
    Process->VadRoot.BalancedRoot.u1.Parent = NULL;
    
    //
    // The process now has an address space
    //
    Process->HasAddressSpace = TRUE;
    return STATUS_SUCCESS;
}

PULONG
NTAPI
MmGetPageDirectory(VOID)
{
    //
    // Return the TTB
    //
    return (PULONG)KeArmTranslationTableRegisterGet().AsUlong;
}

VOID
NTAPI
MmDisableVirtualMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        OUT PBOOLEAN WasDirty,
                        OUT PPFN_TYPE Page)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
MmEnableVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    while (TRUE);
}

NTSTATUS
NTAPI
MmCreateVirtualMappingInternal(IN PVOID Address,
                               IN ULONG Protection,
                               IN PPFN_NUMBER Pages,
                               IN ULONG PageCount,
                               IN BOOLEAN MarkAsMapped)
{
    PMMPTE PointerPte, LastPte, PointerPde, LastPde;
    MMPTE TempPte, TempPde;
    NTSTATUS Status;
    PFN_NUMBER Pfn;
    DPRINT("[KMAP]: %p %d\n", Address, PageCount);
    //ASSERT(Address >= MmSystemRangeStart);
    
    //
    // Get our templates
    //
    TempPte = MiArmTemplatePte;
    TempPde = MiArmTemplatePde;
    
    //
    // Check if we have PDEs for this region
    //
    PointerPde = MiGetPdeAddress(Address);
    LastPde = PointerPde + (PageCount / 256);
    while (PointerPde <= LastPde)
    {
        //
        // Check if we need to allocate the PDE
        //
        if (PointerPde->u.Hard.L1.Fault.Type == FaultPte)
        {
            //
            // Request a page
            //
            Status = MmRequestPageMemoryConsumer(MC_NPPOOL, FALSE, &Pfn);
            if (!NT_SUCCESS(Status)) return Status;
            
            //
            // Setup the PFN
            //
            TempPde.u.Hard.L1.Coarse.BaseAddress = (Pfn << PAGE_SHIFT) >> CPT_SHIFT;
            
            //
            // Write the PDE
            //
            ASSERT(PointerPde->u.Hard.L1.Fault.Type == FaultPte);
            ASSERT(TempPde.u.Hard.L1.Coarse.Type == CoarsePte);
            *PointerPde = TempPde;
            
            //
            // Get the PTE for this 1MB region
            //
            PointerPte = MiGetPteAddress(MiGetPteAddress(Address));
            
            //
            // Write the PFN of the PDE
            //
            TempPte.u.Hard.L2.Small.BaseAddress = Pfn;
            
            //
            // Write the PTE
            //
            ASSERT(PointerPte->u.Hard.L2.Fault.Type == FaultPte);
            ASSERT(TempPte.u.Hard.L2.Small.Type == SmallPte);
            *PointerPte = TempPte;
        }
        
        //
        // Next
        //
        PointerPde++;
    }
    
    //
    // Get start and end address and loop each PTE
    //
    PointerPte = MiGetPteAddress(Address);
    LastPte = PointerPte + PageCount - 1;
    while (PointerPte <= LastPte)
    {
        //
        // Mark it as mapped
        //
        if (MarkAsMapped) MmMarkPageMapped(*Pages);
        
        //
        // Set the PFN
        //
        TempPte.u.Hard.L2.Small.BaseAddress = *Pages++;
        
        //
        // Write the PTE
        //
        ASSERT(PointerPte->u.Hard.L2.Fault.Type == FaultPte);
        ASSERT(TempPte.u.Hard.L2.Small.Type == SmallPte);
        *PointerPte = TempPte;
        
        //
        // Next
        //
        PointerPte++;
    }
    
    //
    // All done
    //
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(IN PVOID Address,
                                IN ULONG Protection,
                                IN PPFN_NUMBER Pages,
                                IN ULONG PageCount)
{
    //
    // Call the internal version
    //
    return MmCreateVirtualMappingInternal(Address,
                                          Protection,
                                          Pages,
                                          PageCount,
                                          FALSE);
}

NTSTATUS
NTAPI
MmCreateVirtualMappingUnsafe(IN PEPROCESS Process,
                             IN PVOID Address,
                             IN ULONG Protection,
                             IN PPFN_TYPE Pages,
                             IN ULONG PageCount)
{
    //
    // Are we only handling the kernel?
    //
    if (!(Process) || (Process == PsGetCurrentProcess()))
    {
        //
        // Call the internal version
        //
        return MmCreateVirtualMappingInternal(Address,
                                              Protection,
                                              Pages,
                                              PageCount,
                                              TRUE);
    }
    
    //
    // FIXME-USER: Support user-mode mappings
    //
    ASSERT(FALSE);
    return 0;
}

NTSTATUS
NTAPI
MmCreateVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN ULONG Protection,
                       IN PPFN_TYPE Pages,
                       IN ULONG PageCount)
{
    ULONG i;
    
    //
    // Loop each page
    //
    for (i = 0; i < PageCount; i++)
    {
        //
        // Make sure the page is marked as in use
        //
        ASSERT(MmIsPageInUse(Pages[i]));
    }
    
    //
    // Call the unsafe version
    //
    return MmCreateVirtualMappingUnsafe(Process,
                                        Address,
                                        Protection,
                                        Pages,
                                        PageCount);
}

VOID
NTAPI
MmRawDeleteVirtualMapping(IN PVOID Address)
{
    PMMPTE PointerPte;
    
    //
    // Get the PTE
    //
    PointerPte = MiGetPageTableForProcess(NULL, Address, FALSE);
    if ((PointerPte) && (PointerPte->u.Hard.L2.Fault.Type != FaultPte))
    {
        //
        // Destroy it
        //
        PointerPte->u.Hard.AsUlong = 0;
    
        //
        // Flush the TLB
        //
        MiFlushTlb(PointerPte, Address);
    }
}

VOID
NTAPI
MmDeleteVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN BOOLEAN FreePage,
                       OUT PBOOLEAN WasDirty,
                       OUT PPFN_TYPE Page)
{
    PMMPTE PointerPte;
    MMPTE Pte;
    PFN_NUMBER Pfn = 0;
    
    //
    // Get the PTE
    //
    PointerPte = MiGetPageTableForProcess(NULL, Address, FALSE);
    if (PointerPte)
    {       
        //
        // Save and destroy the PTE
        //
        Pte = *PointerPte;
        PointerPte->u.Hard.AsUlong = 0;
        
        //
        // Flush the TLB
        //
        MiFlushTlb(PointerPte, Address);
        
        //
        // Unmap the PFN
        //
        Pfn = Pte.u.Hard.L2.Small.BaseAddress;
        if (Pfn) MmMarkPageUnmapped(Pfn);
        
        //
        // Release the PFN if it was ours
        //
        if ((FreePage) && (Pfn)) MmReleasePageMemoryConsumer(MC_NPPOOL, Pfn);
    }
    
    //
    // Return if the page was dirty
    //
    if (WasDirty) *WasDirty = FALSE; // LIE!!!
    if (Page) *Page = Pfn;
}

PVOID
NTAPI
MmCreateHyperspaceMapping(IN PFN_TYPE Page)
{
    PMMPTE PointerPte, FirstPte, LastPte;
    MMPTE TempPte;
    PVOID Address;
    
    //
    // Loop hyperspace PTEs (1MB)
    //
    FirstPte = PointerPte = MiGetPteAddress((PVOID)HYPER_SPACE);
    LastPte = PointerPte + 256;
    while (PointerPte <= LastPte)
    {
        //
        // Find a free slot
        //
        if (PointerPte->u.Hard.L2.Fault.Type == FaultPte)
        {
            //
            // Use this entry
            //
            break;
        }
        
        //
        // Try the next one
        //
        PointerPte++;
    }
    
    //
    // Check if we didn't find anything
    //
    if (PointerPte > LastPte) return NULL;
    
    //
    // Create the mapping
    //
    TempPte = MiArmTemplatePte;
    TempPte.u.Hard.L2.Small.BaseAddress = Page;
    ASSERT(PointerPte->u.Hard.L2.Fault.Type == FaultPte);
    ASSERT(TempPte.u.Hard.L2.Small.Type == SmallPte);
    *PointerPte = TempPte;
    
    //
    // Return the address
    //
    Address = (PVOID)(HYPER_SPACE + ((PointerPte - FirstPte) * PAGE_SIZE));
    KeArmInvalidateTlbEntry(Address);
    DPRINT("[HMAP]: %p %lx\n", Address, Page);
    return Address;
}

PFN_TYPE
NTAPI
MmDeleteHyperspaceMapping(IN PVOID Address)
{
    PFN_TYPE Pfn;
    PMMPTE PointerPte;
    DPRINT("[HUNMAP]: %p\n", Address);
    
    //
    // Get the PTE
    //
    PointerPte = MiGetPteAddress(Address);
    ASSERT(PointerPte->u.Hard.L2.Small.Type == SmallPte);
    
    //
    // Save the PFN
    //
    Pfn = PointerPte->u.Hard.L2.Small.BaseAddress;
    
    //
    // Destroy the PTE
    //
    PointerPte->u.Hard.AsUlong = 0;
    ASSERT(PointerPte->u.Hard.L2.Fault.Type == FaultPte);
    
    //
    // Flush the TLB entry and return the PFN
    //
    KeArmInvalidateTlbEntry(Address);
    return Pfn;
}

VOID
NTAPI
MmDeletePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY *SwapEntry)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    while (TRUE);
}

NTSTATUS
NTAPI
MmCreatePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY SwapEntry)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

PFN_TYPE
NTAPI
MmGetPfnForProcess(IN PEPROCESS Process,
                   IN PVOID Address)
{
    MMPTE Pte;
    
    //
    // Get the PTE
    //
    Pte = MiGetPageEntryForProcess(Process, Address);
    if (Pte.u.Hard.L2.Fault.Type == FaultPte) return 0;
    
    //
    // Return PFN
    //
    return Pte.u.Hard.L2.Small.BaseAddress;
}

BOOLEAN
NTAPI
MmIsDirtyPage(IN PEPROCESS Process,
              IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

VOID
NTAPI
MmSetCleanPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
MmSetDirtyPage(IN PEPROCESS Process,
               IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
MmIsPagePresent(IN PEPROCESS Process,
                IN PVOID Address)
{
    //
    // Fault PTEs are 0, which is FALSE (non-present)
    //
    return MiGetPageEntryForProcess(Process, Address).u.Hard.L2.Fault.Type;
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(IN PEPROCESS Process,
                  IN PVOID Address)
{
    MMPTE Pte;
    
    //
    // Get the PTE
    //
    Pte = MiGetPageEntryForProcess(Process, Address);
    
    //
    // Make sure it's valid, but faulting
    //
    return (Pte.u.Hard.L2.Fault.Type == FaultPte) && (Pte.u.Hard.AsUlong);
}

ULONG
NTAPI
MmGetPageProtect(IN PEPROCESS Process,
                 IN PVOID Address)
{
    //
    // We don't enforce any protection on the pages -- they are all RWX
    //
    return PAGE_READWRITE;
}

VOID
NTAPI
MmSetPageProtect(IN PEPROCESS Process,
                 IN PVOID Address,
                 IN ULONG Protection)
{
    //
    // We don't enforce any protection on the pages -- they are all RWX
    //
    return;
}

VOID
NTAPI
MmInitGlobalKernelPageDirectory(VOID)
{
    ULONG i;
    PULONG CurrentPageDirectory = (PULONG)PDE_BASE;
    
    //
    // Good place to setup template PTE/PDEs.
    // We are lazy and pick a known-good PTE
    //
    MiArmTemplatePte = *MiGetPteAddress(0x80000000);
    MiArmTemplatePde = *MiGetPdeAddress(0x80000000);
    
    //
    // Loop the 2GB of address space which belong to the kernel
    //
    for (i = MiGetPdeOffset(MmSystemRangeStart); i < 1024; i++)
    {
        //
        // Check if we have an entry for this already
        //
        if ((i != MiGetPdeOffset(PTE_BASE)) &&
            (i != MiGetPdeOffset(HYPER_SPACE)) &&
            (!MmGlobalKernelPageDirectory[i]) &&
            (CurrentPageDirectory[i]))
        {
            //
            // We don't, link it in our global page directory
            //
            MmGlobalKernelPageDirectory[i] = CurrentPageDirectory[i];
        }
    }
}

VOID
NTAPI
MiInitPageDirectoryMap(VOID)
{
    MEMORY_AREA* MemoryArea = NULL;
    PHYSICAL_ADDRESS BoundaryAddressMultiple;
    PVOID BaseAddress;
    NTSTATUS Status;
    
    //
    // Create memory area for the PTE area
    //
    BoundaryAddressMultiple.QuadPart = 0;
    BaseAddress = (PVOID)PTE_BASE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_SYSTEM,
                                &BaseAddress,
                                0x1000000,
                                PAGE_READWRITE,
                                &MemoryArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(NT_SUCCESS(Status));
    
    //
    // Create memory area for the PDE area
    //
    BaseAddress = (PVOID)PDE_BASE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_SYSTEM,
                                &BaseAddress,
                                0x100000,
                                PAGE_READWRITE,
                                &MemoryArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(NT_SUCCESS(Status));
    
    //
    // And finally, hyperspace
    //
    BaseAddress = (PVOID)HYPER_SPACE;
    Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                MEMORY_AREA_SYSTEM,
                                &BaseAddress,
                                PAGE_SIZE,
                                PAGE_READWRITE,
                                &MemoryArea,
                                TRUE,
                                0,
                                BoundaryAddressMultiple);
    ASSERT(NT_SUCCESS(Status));
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(IN PVOID Address)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    MMPTE Pte;

    //
    // Early boot PCR check
    //
    if (Address == PCR)
    {
        //
        // ARM Hack while we still use a section PTE
        //
        PMMPTE PointerPte;
        PointerPte = MiGetPdeAddress(PCR);
        ASSERT(PointerPte->u.Hard.L1.Section.Type == SectionPte);
        PhysicalAddress.QuadPart = PointerPte->u.Hard.L1.Section.BaseAddress;
        PhysicalAddress.QuadPart <<= CPT_SHIFT;
        PhysicalAddress.LowPart += BYTE_OFFSET(Address);
        return PhysicalAddress;
    }
    
    //
    // Get the PTE
    //
    Pte = MiGetPageEntryForProcess(NULL, Address);
    if ((Pte.u.Hard.AsUlong) && (Pte.u.Hard.L2.Fault.Type != FaultPte))
    {
        //
        // Return the information
        //
        ASSERT(Pte.u.Hard.L2.Small.Type == SmallPte);
        PhysicalAddress.QuadPart = Pte.u.Hard.L2.Small.BaseAddress;
        PhysicalAddress.QuadPart <<= PAGE_SHIFT;
        PhysicalAddress.LowPart += BYTE_OFFSET(Address);
    }
    else
    {
        //
        // Invalid or unmapped
        //
        PhysicalAddress.QuadPart = 0;
    }
    
    //
    // Return the physical address
    //
    return PhysicalAddress;
}
