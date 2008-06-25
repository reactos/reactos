/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/mm/arm/stubs.c
 * PURPOSE:         ARM Memory Manager
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#undef UNIMPLEMENTED
#define UNIMPLEMENTED \
{ \
    DPRINT1("[ARM Mm Bringup]: %s is unimplemented!\n", __FUNCTION__); \
    while (TRUE); \
}

//
// Take 0x80812345 and extract:
// PTE_BASE[0x808][0x12]
//
#define MiGetPteAddress(x)         \
    (PMMPTE)(PTE_BASE + (((ULONG)(x) >> 20) << 12) + ((((ULONG)(x) >> 12) & 0xFF) << 2))

#define MiGetPdeAddress(x)         \
    (PMMPTE)(PDE_BASE + (((ULONG)(x) >> 20) << 2))

#define MiGetPdeOffset(x) (((ULONG)(x)) >> 22)

#define PTE_BASE    0xC0000000
#define PDE_BASE    0xC1000000
#define HYPER_SPACE ((PVOID)0xC1100000)

ULONG MmGlobalKernelPageDirectory[1024];
MMPTE MiArmTemplatePte, MiArmTemplatePde;

VOID
KiFlushSingleTb(IN BOOLEAN Invalid,
                IN PVOID Virtual);

/* FUNCTIONS ******************************************************************/

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
    // TODO
    //
    UNIMPLEMENTED;
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
    MmInitializeAddressSpace(Process, (PMADDRESS_SPACE)&Process->VadRoot);
    
    //
    // The process now has an address space
    //
    Process->HasAddressSpace = TRUE;
    return STATUS_SUCCESS;
}

PULONG
MmGetPageDirectory(VOID)
{
    //
    // Return the TTB
    //
    return (PULONG)KeArmTranslationTableRegisterGet().AsUlong;
}


BOOLEAN
NTAPI
MmCreateProcessAddressSpace(IN ULONG MinWs,
                            IN PEPROCESS Process,
                            IN PLARGE_INTEGER DirectoryTableBase)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
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
    DPRINT1("MmDeletePageTable(%p, %p)\n", Process, Address);
    ASSERT(Address < MmSystemRangeStart);
    
    //
    // Check if this is for a different process
    //
    if ((Process) && (Process != PsGetCurrentProcess()))
    {
        //
        // TODO
        //
        UNIMPLEMENTED;
        return;
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
    KiFlushSingleTb(TRUE, Address);
}

PFN_TYPE
NTAPI
MmGetPfnForProcess(IN PEPROCESS Process,
                   IN PVOID Address)
{
    PMMPTE Pte;
    
    //
    // Check if this is for a different process
    //
    if ((Process) && (Process != PsGetCurrentProcess()))
    {
        //
        // TODO
        //
        UNIMPLEMENTED;
        return 0;
    }
    
    //
    // Get the PDE
    //
    Pte = MiGetPdeAddress(Address);
    if (Pte->u.Hard.L1.Fault.Type != FaultPte)
    {
        //
        // Get the PTE
        //
        Pte = MiGetPteAddress(Address);
    }
    
    //
    // If PTE is invalid, return 0
    //
    if (Pte->u.Hard.L2.Fault.Type == FaultPte) return 0;
    
    //
    // Return PFN
    //
    return Pte->u.Hard.L2.Small.BaseAddress;
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
}

VOID
NTAPI
MmRawDeleteVirtualMapping(IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
}

VOID
NTAPI
MmDeleteVirtualMapping(IN PEPROCESS Process,
                       IN PVOID Address,
                       IN BOOLEAN FreePage,
                       OUT PBOOLEAN WasDirty,
                       OUT PPFN_TYPE Page)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
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
}

BOOLEAN
NTAPI
MmIsPagePresent(IN PEPROCESS Process,
                IN PVOID Address)
{
    PMMPTE Pte;
    
    //
    // Check if this is for a different process
    //
    if ((Process) && (Process != PsGetCurrentProcess()))
    {
        //
        // TODO
        //
        UNIMPLEMENTED;
        return 0;
    }

    //
    // Get the PDE
    //
    Pte = MiGetPdeAddress(Address);
    if (Pte->u.Hard.L1.Fault.Type != FaultPte)
    {
        //
        // Get the PTE
        //
        Pte = MiGetPteAddress(Address);
    }
    
    //
    // Return whether or not it's valid
    //
    return (Pte->u.Hard.L1.Fault.Type != FaultPte);
}

BOOLEAN
NTAPI
MmIsPageSwapEntry(IN PEPROCESS Process,
                  IN PVOID Address)
{
    PMMPTE Pte;
    
    //
    // Check if this is for a different process
    //
    if ((Process) && (Process != PsGetCurrentProcess()))
    {
        //
        // TODO
        //
        UNIMPLEMENTED;
        return 0;
    }
    
    //
    // Get the PDE
    //
    Pte = MiGetPdeAddress(Address);
    if (Pte->u.Hard.L1.Fault.Type != FaultPte)
    {
        //
        // Get the PTE
        //
        Pte = MiGetPteAddress(Address);
    }
    
    //
    // Return whether or not it's valid
    //
    return ((Pte->u.Hard.L2.Fault.Type == FaultPte) && (Pte->u.Hard.AsUlong));
}

NTSTATUS
NTAPI
MmCreateVirtualMappingForKernel(IN PVOID Address,
                                IN ULONG Protection,
                                IN PPFN_NUMBER Pages,
                                IN ULONG PageCount)
{
    PMMPTE PointerPte, LastPte, PointerPde, LastPde;
    MMPTE TempPte, TempPde;
    NTSTATUS Status;
    PFN_NUMBER Pfn;
    DPRINT1("[KMAP]: %p %d\n", Address, PageCount);
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
MmCreatePageFileMapping(IN PEPROCESS Process,
                        IN PVOID Address,
                        IN SWAPENTRY SwapEntry)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
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
        // Call the kernel version
        //
        return MmCreateVirtualMappingForKernel(Address,
                                               Protection,
                                               Pages,
                                               PageCount);
    }
    
    //
    // TODO
    //
    UNIMPLEMENTED;
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

ULONG
NTAPI
MmGetPageProtect(IN PEPROCESS Process,
                 IN PVOID Address)
{
    //
    // TODO
    //
    UNIMPLEMENTED;
    return 0;
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

/*
 * @implemented
 */
PHYSICAL_ADDRESS
NTAPI
MmGetPhysicalAddress(IN PVOID Address)
{
    PHYSICAL_ADDRESS PhysicalAddress = {{0}};
    PMMPTE PointerPte;

    //
    // Early boot PCR check
    //
    if (Address == PCR)
    {
        //
        // ARM Hack while we still use a section PTE
        //
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
    PointerPte = MiGetPteAddress(Address);
    if (PointerPte->u.Hard.L1.Fault.Type == FaultPte)
    {
        //
        // Invalid address
        //
        DPRINT1("Address invalid: %p\n", Address);
        return PhysicalAddress;
    }

    //
    // Return the information
    //
    ASSERT(PointerPte->u.Hard.L2.Small.Type == SmallPte);
    PhysicalAddress.QuadPart = PointerPte->u.Hard.L2.Small.BaseAddress;
    PhysicalAddress.QuadPart <<= PAGE_SHIFT;
    PhysicalAddress.LowPart += BYTE_OFFSET(Address);
    return PhysicalAddress;
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
    FirstPte = PointerPte = MiGetPteAddress(HYPER_SPACE);
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
    Address = HYPER_SPACE + ((PointerPte - FirstPte) * PAGE_SIZE);
    KiFlushSingleTb(FALSE, Address);
    DPRINT1("[HMAP]: %p %lx\n", Address, Page);
    return Address;
}

PFN_TYPE
NTAPI
MmDeleteHyperspaceMapping(IN PVOID Address)
{
    PFN_TYPE Pfn;
    PMMPTE PointerPte;
    DPRINT1("[HUNMAP]: %p\n", Address);
    
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
    KiFlushSingleTb(TRUE, Address);
    return Pfn;
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

ULONG
NTAPI
MiGetUserPageDirectoryCount(VOID)
{
    //
    // Return the index
    //
    return MiGetPdeOffset(MmSystemRangeStart);
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
