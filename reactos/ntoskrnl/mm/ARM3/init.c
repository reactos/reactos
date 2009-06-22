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
// Right now, we call this the "ARM Pool" and it begins at 0xA0000000 since we
// don't want to interefere with the ReactOS memory manager PFN database (yet).
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

/* PRIVATE FUNCTIONS **********************************************************/

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
    BoundaryAddressMultiple.QuadPart = Low.QuadPart = 0;
    High.QuadPart = -1;
    
    if (Phase == 0)
    {
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
        // Now calculate the nonpaged pool expansion VA region
        //
        MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmNonPagedPoolEnd -
                                      MmMaximumNonPagedPoolInBytes +
                                      MmSizeOfNonPagedPoolInBytes);
        MmNonPagedPoolStart = (PVOID)PAGE_ALIGN(MmNonPagedPoolStart);
        NonPagedPoolExpansionVa = MmNonPagedPoolStart;
        DPRINT1("NP Pool has been tuned to: %d bytes and %d bytes\n",
                MmSizeOfNonPagedPoolInBytes, MmMaximumNonPagedPoolInBytes);
        DPRINT1("NP Expansion VA begins at: %p and ends at: %p\n",
                MmNonPagedPoolStart, MmNonPagedPoolEnd);
        
        //
        // Now calculate the nonpaged system VA region
        // This includes nonpaged pool expansion (above) and the system PTEs
        // Since there are no system PTEs yet, this is (for now) the same
        //
        MmNonPagedSystemStart = MmNonPagedPoolStart;
        DPRINT1("NP System VA (later will be System PTEs) start at: %p\n",
                MmNonPagedSystemStart);
        
        //
        // Non paged pool should come after the PFN database, but since we are
        // co-existing with the ReactOS NP pool, our "ARM Pool" will instead
        // start at this arbitrarly chosen base address.
        // When ARM pool becomes non paged pool, this needs to be changed.
        //
        MmNonPagedPoolStart = (PVOID)0xA0000000;
        DPRINT1("NP VA begins at: %p and ends at: %p\n",
                MmNonPagedPoolStart,
                (ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes);

        //
        // Now we actually need to get these many physical pages. Nonpaged pool
        // is actually also physically contiguous (but not the expansion)
        //
        PageFrameIndex = MmGetContinuousPages(MmSizeOfNonPagedPoolInBytes,
                                              Low,
                                              High,
                                              BoundaryAddressMultiple);
        ASSERT(PageFrameIndex != 0);
        DPRINT1("NP PA PFN begins at: %lx\n", PageFrameIndex);

        //
        // Now we need some pages to create the page tables for the NP system VA
        // which would normally include system PTEs and expansion NP
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
        StartPde = MiAddressToPde(MmNonPagedPoolStart);
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
        // Now rememeber where the expansion starts
        //
        MmNonPagedPoolExpansionStart = NonPagedPoolExpansionVa;

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
        // And we need one more for the system NP (expansion NP only for now)
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
        // Now go ahead and initialize the ARM pool
        //
        MiInitializeArmPool();
    }
    
    //
    // Always return success for now
    //
    return STATUS_SUCCESS;
}

/* EOF */
