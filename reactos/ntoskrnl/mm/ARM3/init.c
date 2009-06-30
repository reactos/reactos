/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/init.c
 * PURPOSE:         ARM Memory Manager Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::INIT"
#define MODULE_INVOLVED_IN_ARM3
#include "../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

//
// These are all registry-configurable, but by default, the memory manager will
// figure out the most appropriate values.
//
ULONG MmMaximumNonPagedPoolPercent;
ULONG MmSizeOfNonPagedPoolInBytes;
ULONG MmMaximumNonPagedPoolInBytes;

//
// These numbers describe the discrete equation components of the nonpaged
// pool sizing algorithm.
//
// They are described on http://support.microsoft.com/default.aspx/kb/126402/ja
// along with the algorithm that uses them, which is implemented later below.
//
ULONG MmMinimumNonPagedPoolSize = 256 * 1024;
ULONG MmMinAdditionNonPagedPoolPerMb = 32 * 1024;
ULONG MmDefaultMaximumNonPagedPool = 1024 * 1024; 
ULONG MmMaxAdditionNonPagedPoolPerMb = 400 * 1024;

//
// The memory layout (and especially variable names) of the NT kernel mode
// components can be a bit hard to twig, especially when it comes to the non
// paged area.
//
// There are really two components to the non-paged pool:
//
// - The initial nonpaged pool, sized dynamically up to a maximum.
// - The expansion nonpaged pool, sized dynamically up to a maximum.
//
// The initial nonpaged pool is physically continuous for performance, and
// immediately follows the PFN database, typically sharing the same PDE. It is
// a very small resource (32MB on a 1GB system), and capped at 128MB.
//
// Right now, we call this the "ARM Pool" and it begins somewhere after the ARM
// PFN database (which starts at 0xB0000000).
//
// The expansion nonpaged pool, on the other hand, can grow much bigger (400MB 
// for a 1GB system). On ARM³ however, it is currently capped at 128MB.
//
// The address where the initial nonpaged pool starts is aptly named
// MmNonPagedPoolStart, and it describes a range of MmSizeOfNonPagedPoolInBytes
// bytes.
//
// Expansion nonpaged pool starts at an address described by the variable called
// MmNonPagedPoolExpansionStart, and it goes on for MmMaximumNonPagedPoolInBytes
// minus MmSizeOfNonPagedPoolInBytes bytes, always reaching MmNonPagedPoolEnd
// (because of the way it's calculated) at 0xFFBE0000.
//
// Initial nonpaged pool is allocated and mapped early-on during boot, but what
// about the expansion nonpaged pool? It is instead composed of special pages
// which belong to what are called System PTEs. These PTEs are the matter of a
// later discussion, but they are also considered part of the "nonpaged" OS, due
// to the fact that they are never paged out -- once an address is described by
// a System PTE, it is always valid, until the System PTE is torn down.
//
// System PTEs are actually composed of two "spaces", the system space proper,
// and the nonpaged pool expansion space. The latter, as we've already seen, 
// begins at MmNonPagedPoolExpansionStart. Based on the number of System PTEs
// that the system will support, the remaining address space below this address
// is used to hold the system space PTEs. This address, in turn, is held in the
// variable named MmNonPagedSystemStart, which itself is never allowed to go
// below 0xEB000000 (thus creating an upper bound on the number of System PTEs).
//
// This means that 330MB are reserved for total nonpaged system VA, on top of
// whatever the initial nonpaged pool allocation is.
//
// The following URLs, valid as of April 23rd, 2008, support this evidence:
//
// http://www.cs.miami.edu/~burt/journal/NT/memory.html
// http://www.ditii.com/2007/09/28/windows-memory-management-x86-virtual-address-space/
//
PVOID MmNonPagedSystemStart;
PVOID MmNonPagedPoolStart;
PVOID MmNonPagedPoolExpansionStart;
PVOID MmNonPagedPoolEnd = (PVOID)0xFFBE0000;

//
// Windows NT seems to choose between 7000, 11000 and 50000
// On systems with more than 32MB, this number is then doubled, and further
// aligned up to a PDE boundary (4MB).
//
ULONG MmNumberOfSystemPtes;

//
// This is how many pages the PFN database will take up
// In Windows, this includes the Quark Color Table, but not in ARM³
//
ULONG MxPfnAllocation;

//
// The ARM³ PFN Database
//
PMMPFN MmArmPfnDatabase;

//
// This structure describes the different pieces of RAM-backed address space
//
PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;

/* PRIVATE FUNCTIONS **********************************************************/

//
// In Bavaria, this is probably a hate crime
//
VOID
FASTCALL
MiSyncARM3WithROS(IN PVOID AddressStart,
                  IN PVOID AddressEnd)
{
    //
    // Puerile piece of junk-grade carbonized horseshit puss sold to the lowest bidder
    //
    ULONG Pde = ADDR_TO_PDE_OFFSET(AddressStart);
    while (Pde < ADDR_TO_PDE_OFFSET(AddressEnd))
    {
        //
        // This both odious and heinous
        //
        extern ULONG MmGlobalKernelPageDirectory[1024];
        MmGlobalKernelPageDirectory[Pde] = ((PULONG)PAGEDIRECTORY_MAP)[Pde];
        Pde++;
    }
}

PPHYSICAL_MEMORY_DESCRIPTOR
NTAPI
MmInitializeMemoryLimits(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN PBOOLEAN IncludeType)
{
    PLIST_ENTRY NextEntry;
    ULONG Run = 0, InitialRuns = 0;
    PFN_NUMBER NextPage = -1, PageCount = 0;
    PPHYSICAL_MEMORY_DESCRIPTOR Buffer, NewBuffer;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    
    //
    // Scan the memory descriptors
    //
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        //
        // For each one, increase the memory allocation estimate
        //
        InitialRuns++;
        NextEntry = NextEntry->Flink;
    }
    
    //
    // Allocate the maximum we'll ever need
    //
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
                                   sizeof(PHYSICAL_MEMORY_RUN) *
                                   (InitialRuns - 1),
                                   'lMmM');
    if (!Buffer) return NULL;

    //
    // For now that's how many runs we have
    //
    Buffer->NumberOfRuns = InitialRuns;
    
    //
    // Now loop through the descriptors again
    //
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        //
        // Grab each one, and check if it's one we should include
        //
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        if ((MdBlock->MemoryType < LoaderMaximum) &&
            (IncludeType[MdBlock->MemoryType]))
        {
            //
            // Add this to our running total
            //
            PageCount += MdBlock->PageCount;
            
            //
            // Check if the next page is described by the next descriptor
            //            
            if (MdBlock->BasePage == NextPage)
            {
                //
                // Combine it into the same physical run
                //
                ASSERT(MdBlock->PageCount != 0);
                Buffer->Run[Run - 1].PageCount += MdBlock->PageCount;
                NextPage += MdBlock->PageCount;
            }
            else
            {
                //
                // Otherwise just duplicate the descriptor's contents
                //
                Buffer->Run[Run].BasePage = MdBlock->BasePage;
                Buffer->Run[Run].PageCount = MdBlock->PageCount;
                NextPage = Buffer->Run[Run].BasePage + Buffer->Run[Run].PageCount;
                
                //
                // And in this case, increase the number of runs
                //
                Run++;
            }
        }
        
        //
        // Try the next descriptor
        //
        NextEntry = MdBlock->ListEntry.Flink;
    }
    
    //
    // We should not have been able to go past our initial estimate
    //
    ASSERT(Run <= Buffer->NumberOfRuns);

    //
    // Our guess was probably exaggerated...
    //
    if (InitialRuns > Run)
    {
        //
        // Allocate a more accurately sized buffer
        //
        NewBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                          sizeof(PHYSICAL_MEMORY_DESCRIPTOR) +
                                          sizeof(PHYSICAL_MEMORY_RUN) *
                                          (Run - 1),
                                          'lMmM');
        if (NewBuffer)
        {
            //
            // Copy the old buffer into the new, then free it
            //
            RtlCopyMemory(NewBuffer->Run,
                          Buffer->Run,
                          sizeof(PHYSICAL_MEMORY_RUN) * Run);
            ExFreePool(Buffer);
            
            //
            // Now use the new buffer
            //
            Buffer = NewBuffer;
        }
    }
    
    //
    // Write the final numbers, and return it
    //
    Buffer->NumberOfRuns = Run;
    Buffer->NumberOfPages = PageCount;
    return Buffer;
}

NTSTATUS
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMEMORY_AREA MArea;
    PHYSICAL_ADDRESS BoundaryAddressMultiple, Low, High;
    PFN_NUMBER PageFrameIndex;
    PMMPTE StartPde, EndPde, PointerPte, LastPte;
    MMPTE TempPde = HyperTemplatePte, TempPte = HyperTemplatePte;
    PVOID NonPagedPoolExpansionVa, BaseAddress;
    NTSTATUS Status;
    ULONG OldCount;
    BOOLEAN IncludeType[LoaderMaximum];
    ULONG i;
    BoundaryAddressMultiple.QuadPart = Low.QuadPart = 0;
    High.QuadPart = -1;
    
    if (Phase == 0)
    {
        //
        // Set CR3 for the system process
        //
        PointerPte = MiAddressToPde(PAGETABLE_MAP);
        PageFrameIndex = PFN_FROM_PTE(PointerPte) << PAGE_SHIFT;
        PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PageFrameIndex;
        
        //
        // Blow away user-mode
        //
        StartPde = MiAddressToPde(0);
        EndPde = MiAddressToPde(KSEG0_BASE);
        RtlZeroMemory(StartPde, (EndPde - StartPde) * sizeof(MMPTE));
        
        //
        // Check if this is a machine with less than 19MB of RAM
        //
        if (MmNumberOfPhysicalPages < MI_MIN_PAGES_FOR_SYSPTE_TUNING)
        {
            //
            // Use the very minimum of system PTEs
            //
            MmNumberOfSystemPtes = 7000;
        }
        else
        {
            //
            // Use the default, but check if we have more than 32MB of RAM
            //
            MmNumberOfSystemPtes = 11000;
            if (MmNumberOfPhysicalPages > MI_MIN_PAGES_FOR_SYSPTE_BOOST)
            {
                //
                // Double the amount of system PTEs
                //
                MmNumberOfSystemPtes <<= 1;
            }
        }
        
        DPRINT("System PTE count has been tuned to %d (%d bytes)\n",
               MmNumberOfSystemPtes, MmNumberOfSystemPtes * PAGE_SIZE);
        
        //
        // Check if this is a machine with less than 256MB of RAM, and no overide
        //
        if ((MmNumberOfPhysicalPages <= MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING) &&
            !(MmSizeOfNonPagedPoolInBytes))
        {
            //
            // Force the non paged pool to be 2MB so we can reduce RAM usage
            //
            MmSizeOfNonPagedPoolInBytes = 2 * 1024 * 1024;
        }
        
        //
        // Check if the user gave a ridicuously large nonpaged pool RAM size
        //
        if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) >
            (MmNumberOfPhysicalPages * 7 / 8))
        {
            //
            // More than 7/8ths of RAM was dedicated to nonpaged pool, ignore!
            //
            MmSizeOfNonPagedPoolInBytes = 0;
        }
        
        //
        // Check if no registry setting was set, or if the setting was too low
        //
        if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize)
        {
            //
            // Start with the minimum (256 KB) and add 32 KB for each MB above 4
            //
            MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;
            MmSizeOfNonPagedPoolInBytes += (MmNumberOfPhysicalPages - 1024) /
                                           256 * MmMinAdditionNonPagedPoolPerMb;
        }
        
        //
        // Check if the registy setting or our dynamic calculation was too high
        //
        if (MmSizeOfNonPagedPoolInBytes > MI_MAX_INIT_NONPAGED_POOL_SIZE)
        {
            //
            // Set it to the maximum
            //
            MmSizeOfNonPagedPoolInBytes = MI_MAX_INIT_NONPAGED_POOL_SIZE;
        }
        
        //
        // Check if a percentage cap was set through the registry
        //
        if (MmMaximumNonPagedPoolPercent)
        {
            //
            // Don't feel like supporting this right now
            //
            UNIMPLEMENTED;
        }
        
        //
        // Page-align the nonpaged pool size
        //
        MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);
        
        //
        // Now, check if there was a registry size for the maximum size
        //
        if (!MmMaximumNonPagedPoolInBytes)
        {
            //
            // Start with the default (1MB) and add 400 KB for each MB above 4
            //
            MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;
            MmMaximumNonPagedPoolInBytes += (MmNumberOfPhysicalPages - 1024) /
                                             256 * MmMaxAdditionNonPagedPoolPerMb;
        }
        
        //
        // Don't let the maximum go too high
        //
        if (MmMaximumNonPagedPoolInBytes > MI_MAX_NONPAGED_POOL_SIZE)
        {
            //
            // Set it to the upper limit
            //
            MmMaximumNonPagedPoolInBytes = MI_MAX_NONPAGED_POOL_SIZE;
        }
        
        //
        // Calculate the number of bytes, and then convert to pages
        //
        MxPfnAllocation = (MmHighestPhysicalPage + 1) * sizeof(MMPFN);
        MxPfnAllocation >>= PAGE_SHIFT;
        
        //
        // Now calculate the nonpaged pool expansion VA region
        //
        MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmNonPagedPoolEnd -
                                      MmMaximumNonPagedPoolInBytes +
                                      MmSizeOfNonPagedPoolInBytes);
        MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);
        NonPagedPoolExpansionVa = MmNonPagedPoolStart;
        DPRINT("NP Pool has been tuned to: %d bytes and %d bytes\n",
               MmSizeOfNonPagedPoolInBytes, MmMaximumNonPagedPoolInBytes);
        
        //
        // Now calculate the nonpaged system VA region, which includes the
        // nonpaged pool expansion (above) and the system PTEs. Note that it is
        // then aligned to a PDE boundary (4MB).
        //
        MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedPoolStart -
                                        (MmNumberOfSystemPtes + 1) * PAGE_SIZE);
        MmNonPagedSystemStart = (PVOID)((ULONG_PTR)MmNonPagedSystemStart &
                                        ~((4 * 1024 * 1024) - 1));
        
        //
        // Don't let it go below the minimum
        //
        if (MmNonPagedSystemStart < (PVOID)0xEB000000)
        {
            //
            // This is a hard-coded limit in the Windows NT address space
            //
            MmNonPagedSystemStart = (PVOID)0xEB000000;
            
            //
            // Reduce the amount of system PTEs to reach this point
            //
            MmNumberOfSystemPtes = ((ULONG_PTR)MmNonPagedPoolStart -
                                    (ULONG_PTR)MmNonPagedSystemStart) >>
                                    PAGE_SHIFT;
            MmNumberOfSystemPtes--;
            ASSERT(MmNumberOfSystemPtes > 1000);
        }
        
        //
        // Normally, the PFN database should start after the loader images.
        // This is already the case in ReactOS, but for now we want to co-exist
        // with the old memory manager, so we'll create a "Shadow PFN Database"
        // instead, and arbitrarly start it at 0xB0000000.
        //
        MmArmPfnDatabase = (PVOID)0xB0000000;
        ASSERT(((ULONG_PTR)MmArmPfnDatabase & ((4 * 1024 * 1024) - 1)) == 0);
                
        //
        // Non paged pool comes after the PFN database
        //
        MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmArmPfnDatabase +
                                      (MxPfnAllocation << PAGE_SHIFT));

        //
        // Now we actually need to get these many physical pages. Nonpaged pool
        // is actually also physically contiguous (but not the expansion)
        //
        PageFrameIndex = MmGetContinuousPages(MmSizeOfNonPagedPoolInBytes +
                                              (MxPfnAllocation << PAGE_SHIFT),
                                              Low,
                                              High,
                                              BoundaryAddressMultiple,
                                              FALSE);
        ASSERT(PageFrameIndex != 0);
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmArmPfnDatabase,
                (ULONG_PTR)MmArmPfnDatabase + (MxPfnAllocation << PAGE_SHIFT),
                "Shadow PFN Database");
        DPRINT("PFN DB PA PFN begins at: %lx\n", PageFrameIndex);
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmNonPagedPoolStart,
                 (ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes,
                "ARM Non Paged Pool");
        DPRINT("NP PA PFN begins at: %lx\n", PageFrameIndex + MxPfnAllocation);

        //
        // Now we need some pages to create the page tables for the NP system VA
        // which includes system PTEs and expansion NP
        //
        StartPde = MiAddressToPde(MmNonPagedSystemStart);
        EndPde = MiAddressToPde((PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
        while (StartPde <= EndPde)
        {
            //
            // Sanity check
            //
            ASSERT(StartPde->u.Hard.Valid == 0);
            
            //
            // Get a page
            //
            TempPde.u.Hard.PageFrameNumber = MmAllocPage(MC_SYSTEM, 0);
            ASSERT(TempPde.u.Hard.Valid == 1);
            *StartPde = TempPde;
            
            //
            // Zero out the page table
            //
            PointerPte = MiPteToAddress(StartPde);
            RtlZeroMemory(PointerPte, PAGE_SIZE);
            
            //
            // Next
            //
            StartPde++;
        }

        //
        // Now we need pages for the page tables which will map initial NP
        //
        StartPde = MiAddressToPde(MmArmPfnDatabase);
        EndPde = MiAddressToPde((PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                        MmSizeOfNonPagedPoolInBytes - 1));
        while (StartPde <= EndPde)
        {
            //
            // Sanity check
            //
            ASSERT(StartPde->u.Hard.Valid == 0);
            
            //
            // Get a page
            //
            TempPde.u.Hard.PageFrameNumber = MmAllocPage(MC_SYSTEM, 0);
            ASSERT(TempPde.u.Hard.Valid == 1);
            *StartPde = TempPde;
            
            //
            // Zero out the page table
            //
            PointerPte = MiPteToAddress(StartPde);
            RtlZeroMemory(PointerPte, PAGE_SIZE);
            
            //
            // Next
            //
            StartPde++;
        }

        //
        // Now remember where the expansion starts
        //
        MmNonPagedPoolExpansionStart = NonPagedPoolExpansionVa;
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmNonPagedSystemStart, MmNonPagedPoolExpansionStart,
                "System PTE Space");
        DPRINT1("          0x%p - 0x%p\t%s\n",
                MmNonPagedPoolExpansionStart, MmNonPagedPoolEnd,
                "Non Paged Pool Expansion PTE Space");

        //
        // Last step is to actually map the nonpaged pool
        //
        PointerPte = MiAddressToPte(MmNonPagedPoolStart);
        LastPte = MiAddressToPte((PVOID)((ULONG_PTR)MmNonPagedPoolStart +
                                         MmSizeOfNonPagedPoolInBytes - 1));
        while (PointerPte <= LastPte)
        {
            //
            // Use one of our contigous pages
            //
            TempPte.u.Hard.PageFrameNumber = PageFrameIndex++;
            ASSERT(PointerPte->u.Hard.Valid == 0);
            ASSERT(TempPte.u.Hard.Valid == 1);
            *PointerPte++ = TempPte;
        }
        
        //
        // ReactOS requires a memory area to keep the initial NP area off-bounds
        //
        BaseAddress = MmNonPagedPoolStart;
        Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                    MEMORY_AREA_SYSTEM | MEMORY_AREA_STATIC,
                                    &BaseAddress,
                                    MmSizeOfNonPagedPoolInBytes,
                                    PAGE_READWRITE,
                                    &MArea,
                                    TRUE,
                                    0,
                                    BoundaryAddressMultiple);
        ASSERT(Status == STATUS_SUCCESS);
        
        //
        // And we need one more for the system NP
        //
        BaseAddress = MmNonPagedSystemStart;
        Status = MmCreateMemoryArea(MmGetKernelAddressSpace(),
                                    MEMORY_AREA_SYSTEM | MEMORY_AREA_STATIC,
                                    &BaseAddress,
                                    (ULONG_PTR)MmNonPagedPoolEnd -
                                    (ULONG_PTR)MmNonPagedSystemStart,
                                    PAGE_READWRITE,
                                    &MArea,
                                    TRUE,
                                    0,
                                    BoundaryAddressMultiple);
        ASSERT(Status == STATUS_SUCCESS);
        
        //
        // Sanity check: make sure we have properly defined the system PTE space
        //
        ASSERT(MiAddressToPte(MmNonPagedSystemStart) <
               MiAddressToPte(MmNonPagedPoolExpansionStart));
        
        //
        // Now go ahead and initialize the ARM pool
        //
        MiInitializeArmPool();
        
        //
        // We PDE-aligned the nonpaged system start VA, so haul some extra PTEs!
        //
        PointerPte = MiAddressToPte(MmNonPagedSystemStart);
        OldCount = MmNumberOfSystemPtes;
        MmNumberOfSystemPtes = MiAddressToPte(MmNonPagedPoolExpansionStart) -
                               PointerPte;
        MmNumberOfSystemPtes--;
        DPRINT("Final System PTE count: %d (%d bytes)\n",
               MmNumberOfSystemPtes, MmNumberOfSystemPtes * PAGE_SIZE);
        
        //
        // Create the system PTE space
        //
        MiInitializeSystemPtes(PointerPte, MmNumberOfSystemPtes, SystemPteSpace);
        
        //
        // Get the PDE For hyperspace
        //
        StartPde = MiAddressToPde(HYPER_SPACE);
        
        //
        // Allocate a page for it and create it
        //
        PageFrameIndex = MmAllocPage(MC_SYSTEM, 0);
        TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
        TempPde.u.Hard.Global = FALSE; // Hyperspace is local!
        ASSERT(StartPde->u.Hard.Valid == 0);
        ASSERT(TempPde.u.Hard.Valid == 1);
        *StartPde = TempPde;
        
        //
        // Zero out the page table now
        //
        PointerPte = MiAddressToPte(HYPER_SPACE);
        RtlZeroMemory(PointerPte, PAGE_SIZE);
        
        //
        // Setup the mapping PTEs
        //
        MmFirstReservedMappingPte = MiAddressToPte(MI_MAPPING_RANGE_START);
        MmLastReservedMappingPte = MiAddressToPte(MI_MAPPING_RANGE_END);
        MmFirstReservedMappingPte->u.Hard.PageFrameNumber = MI_HYPERSPACE_PTES;

        //
        // Reserve system PTEs for zeroing PTEs and clear them
        //
        MiFirstReservedZeroingPte = MiReserveSystemPtes(MI_ZERO_PTES,
                                                        SystemPteSpace);
        RtlZeroMemory(MiFirstReservedZeroingPte, MI_ZERO_PTES * sizeof(MMPTE));
        
        //
        // Set the counter to maximum to boot with
        //
        MiFirstReservedZeroingPte->u.Hard.PageFrameNumber = MI_ZERO_PTES - 1;
        
        //
        // Sync us up with ReactOS Mm
        //
        MiSyncARM3WithROS(MmNonPagedSystemStart, (PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
        MiSyncARM3WithROS(MmNonPagedPoolStart, (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes - 1));
        MiSyncARM3WithROS((PVOID)HYPER_SPACE, (PVOID)(HYPER_SPACE + PAGE_SIZE - 1));
    }
    else
    {
        //
        // Instantiate memory that we don't consider RAM/usable
        // We use the same exclusions that Windows does, in order to try to be
        // compatible with WinLDR-style booting
        //
        for (i = 0; i < LoaderMaximum; i++) IncludeType[i] = TRUE;
        IncludeType[LoaderBad] = FALSE;
        IncludeType[LoaderFirmwarePermanent] = FALSE;
        IncludeType[LoaderSpecialMemory] = FALSE;
        IncludeType[LoaderBBTMemory] = FALSE;
        
        //
        // Build the physical memory block
        //
        MmPhysicalMemoryBlock = MmInitializeMemoryLimits(LoaderBlock,
                                                         IncludeType);
        for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i++)
        {
            //
            // Dump it for debugging
            //
            PPHYSICAL_MEMORY_RUN Run;
            Run = &MmPhysicalMemoryBlock->Run[i];
            DPRINT("PHYSICAL RAM [0x%08p to 0x%08p]\n",
                   Run->BasePage << PAGE_SHIFT,
                   (Run->BasePage + Run->PageCount) << PAGE_SHIFT);
        }
    }
    
    //
    // Always return success for now
    //
    return STATUS_SUCCESS;
}

/* EOF */
