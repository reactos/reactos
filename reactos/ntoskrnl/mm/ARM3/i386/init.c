/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/i386/init.c
 * PURPOSE:         ARM Memory Manager Initialization for x86
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::INIT:X86"
#define MODULE_INVOLVED_IN_ARM3
#include "../../ARM3/miarm.h"

/* GLOBALS ********************************************************************/

//
// Before we have a PFN database, memory comes straight from our physical memory
// blocks, which is nice because it's guaranteed contiguous and also because once
// we take a page from here, the system doesn't see it anymore.
// However, once the fun is over, those pages must be re-integrated back into
// PFN society life, and that requires us keeping a copy of the original layout
// so that we can parse it later.
//
PMEMORY_ALLOCATION_DESCRIPTOR MxFreeDescriptor;
MEMORY_ALLOCATION_DESCRIPTOR MxOldFreeDescriptor;

/* Template PTE and PDE for a kernel page */
MMPTE ValidKernelPde = {.u.Hard.Valid = 1, .u.Hard.Write = 1, .u.Hard.Dirty = 1, .u.Hard.Accessed = 1};
MMPTE ValidKernelPte = {.u.Hard.Valid = 1, .u.Hard.Write = 1, .u.Hard.Dirty = 1, .u.Hard.Accessed = 1};

/* Make the code cleaner with some definitions for size multiples */
#define _1KB (1024)
#define _1MB (1000 * _1KB)

/* Architecture specific size of a PDE directory, and size of a page table */
#define PDE_SIZE (4096 * sizeof(MMPDE))
#define PT_SIZE  (1024 * sizeof(MMPTE))

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiComputeNonPagedPoolVa(IN ULONG FreePages)
{
    IN PFN_NUMBER PoolPages;
    
    /* Check if this is a machine with less than 256MB of RAM, and no overide */
    if ((MmNumberOfPhysicalPages <= MI_MIN_PAGES_FOR_NONPAGED_POOL_TUNING) &&
        !(MmSizeOfNonPagedPoolInBytes))
    {
        /* Force the non paged pool to be 2MB so we can reduce RAM usage */
        MmSizeOfNonPagedPoolInBytes = 2 * _1MB;
    }
    
    /* Hyperspace ends here */
    MmHyperSpaceEnd = (PVOID)((ULONG_PTR)MmSystemCacheWorkingSetList - 1);
    
    /* Check if the user gave a ridicuously large nonpaged pool RAM size */
    if ((MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT) > (FreePages * 7 / 8))
    {
        /* More than 7/8ths of RAM was dedicated to nonpaged pool, ignore! */
        MmSizeOfNonPagedPoolInBytes = 0;
    }
    
    /* Check if no registry setting was set, or if the setting was too low */
    if (MmSizeOfNonPagedPoolInBytes < MmMinimumNonPagedPoolSize)
    {
        /* Start with the minimum (256 KB) and add 32 KB for each MB above 4 */
        MmSizeOfNonPagedPoolInBytes = MmMinimumNonPagedPoolSize;
        MmSizeOfNonPagedPoolInBytes += (FreePages - 1024) / 256 * MmMinAdditionNonPagedPoolPerMb;
    }
    
    /* Check if the registy setting or our dynamic calculation was too high */
    if (MmSizeOfNonPagedPoolInBytes > MI_MAX_INIT_NONPAGED_POOL_SIZE)
    {
        /* Set it to the maximum */
        MmSizeOfNonPagedPoolInBytes = MI_MAX_INIT_NONPAGED_POOL_SIZE;
    }
    
    /* Check if a percentage cap was set through the registry */
    if (MmMaximumNonPagedPoolPercent) UNIMPLEMENTED;
    
    /* Page-align the nonpaged pool size */
    MmSizeOfNonPagedPoolInBytes &= ~(PAGE_SIZE - 1);
    
    /* Now, check if there was a registry size for the maximum size */
    if (!MmMaximumNonPagedPoolInBytes)
    {
        /* Start with the default (1MB) */
        MmMaximumNonPagedPoolInBytes = MmDefaultMaximumNonPagedPool;
        
        /* Add space for PFN database */
        MmMaximumNonPagedPoolInBytes += (ULONG)
            PAGE_ALIGN((MmHighestPhysicalPage +  1) * sizeof(MMPFN));
            
        /* Check if the machine has more than 512MB of free RAM */
        if (FreePages >= 0x1F000)
        {
            /* Add 200KB for each MB above 4 */
            MmMaximumNonPagedPoolInBytes += (FreePages - 1024) / 256 *
                                            (MmMaxAdditionNonPagedPoolPerMb / 2);
            if (MmMaximumNonPagedPoolInBytes < MI_MAX_NONPAGED_POOL_SIZE)
            {
                /* Make it at least 128MB since this machine has a lot of RAM */
                MmMaximumNonPagedPoolInBytes = MI_MAX_NONPAGED_POOL_SIZE;
            }
        }
        else
        {
            /* Add 400KB for each MB above 4 */
            MmMaximumNonPagedPoolInBytes += (FreePages - 1024) / 256 *
                                            MmMaxAdditionNonPagedPoolPerMb;
        }
    }
    
    /* Make sure there's at least 16 pages + the PFN available for expansion */
    PoolPages = MmSizeOfNonPagedPoolInBytes + (PAGE_SIZE * 16) +
                ((ULONG)PAGE_ALIGN(MmHighestPhysicalPage + 1) * sizeof(MMPFN));
    if (MmMaximumNonPagedPoolInBytes < PoolPages)
    {
        /* The maximum should be at least high enough to cover all the above */
        MmMaximumNonPagedPoolInBytes = PoolPages;
    }
    
    /* Systems with 2GB of kernel address space get double the size */
    PoolPages = MI_MAX_NONPAGED_POOL_SIZE * 2;
    
    /* On the other hand, make sure that PFN + nonpaged pool doesn't get too big */
    if (MmMaximumNonPagedPoolInBytes > PoolPages)
    {
        /* Trim it down to the maximum architectural limit (256MB) */
        MmMaximumNonPagedPoolInBytes = PoolPages;
    }
    
    /* Check if this is a system with > 128MB of non paged pool */
    if (MmMaximumNonPagedPoolInBytes > MI_MAX_NONPAGED_POOL_SIZE)
    {
        /* Check if the initial size is less than the extra 128MB boost */
        if (MmSizeOfNonPagedPoolInBytes < (MmMaximumNonPagedPoolInBytes -
                                           MI_MAX_NONPAGED_POOL_SIZE))
        {
            /* FIXME: Should check if the initial pool can be expanded */
            
            /* Assume no expansion possible, check ift he maximum is too large */
            if (MmMaximumNonPagedPoolInBytes > (MmSizeOfNonPagedPoolInBytes +
                                                MI_MAX_NONPAGED_POOL_SIZE))
            {
                /* Set it to the initial value plus the boost */
                MmMaximumNonPagedPoolInBytes = MmSizeOfNonPagedPoolInBytes +
                                               MI_MAX_NONPAGED_POOL_SIZE;
            }    
        }
    }
}

VOID
NTAPI
MiComputeColorInformation(VOID)
{
    ULONG L2Associativity;
    
    /* Check if no setting was provided already */
    if (!MmSecondaryColors)
    {
        /* Get L2 cache information */
        L2Associativity = KeGetPcr()->SecondLevelCacheAssociativity;
        
        /* The number of colors is the number of cache bytes by set/way */
        MmSecondaryColors = KeGetPcr()->SecondLevelCacheSize;
        if (L2Associativity) MmSecondaryColors /= L2Associativity;
    }
    
    /* Now convert cache bytes into pages */
    MmSecondaryColors >>= PAGE_SHIFT;
    if (!MmSecondaryColors)
    {
        /* If there was no cache data from the KPCR, use the default colors */
        MmSecondaryColors = MI_SECONDARY_COLORS;
    }
    else
    {
        /* Otherwise, make sure there aren't too many colors */
        if (MmSecondaryColors > MI_MAX_SECONDARY_COLORS)
        {
            /* Set the maximum */
            MmSecondaryColors = MI_MAX_SECONDARY_COLORS;
        }
        
        /* Make sure there aren't too little colors */
        if (MmSecondaryColors < MI_MIN_SECONDARY_COLORS)
        {
            /* Set the default */
            MmSecondaryColors = MI_SECONDARY_COLORS;
        }
        
        /* Finally make sure the colors are a power of two */
        if (MmSecondaryColors & (MmSecondaryColors - 1))
        {
            /* Set the default */
            MmSecondaryColors = MI_SECONDARY_COLORS;
        }
    }
    
    /* Compute the mask and store it */
    MmSecondaryColorMask = MmSecondaryColors - 1;
    KeGetCurrentPrcb()->SecondaryColorMask = MmSecondaryColorMask;    
}

NTSTATUS
NTAPI
MiInitMachineDependent(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG FreePages = 0;
    PFN_NUMBER PageFrameIndex;
    PMMPTE StartPde, EndPde, PointerPte, LastPte;
    MMPTE TempPde, TempPte;
    PVOID NonPagedPoolExpansionVa;
    ULONG OldCount;
    PFN_NUMBER FreePage, FreePageCount, PagesLeft, BasePage, PageCount;

    /* Check for kernel stack size that's too big */
    if (MmLargeStackSize > (KERNEL_LARGE_STACK_SIZE / _1KB))
    {
        /* Sanitize to default value */
        MmLargeStackSize = KERNEL_LARGE_STACK_SIZE;
    }
    else
    {
        /* Take the registry setting, and convert it into bytes */
        MmLargeStackSize *= _1KB;
        
        /* Now align it to a page boundary */
        MmLargeStackSize = PAGE_ROUND_UP(MmLargeStackSize);
        
        /* Sanity checks */
        ASSERT(MmLargeStackSize <= KERNEL_LARGE_STACK_SIZE);
        ASSERT((MmLargeStackSize & (PAGE_SIZE - 1)) == 0);
        
        /* Make sure it's not too low */
        if (MmLargeStackSize < KERNEL_STACK_SIZE) MmLargeStackSize = KERNEL_STACK_SIZE;
    }
    
    /* Check for global bit */
    if (KeFeatureBits & KF_GLOBAL_PAGE)
    {
        /* Set it on the template PTE and PDE */
        ValidKernelPte.u.Hard.Global = TRUE;
        ValidKernelPde.u.Hard.Global = TRUE;
    }
    
    /* Now templates are ready */
    TempPte = ValidKernelPte;
    TempPde = ValidKernelPde;
     
    //
    // Set CR3 for the system process
    //
    PointerPte = MiAddressToPde(PTE_BASE);
    PageFrameIndex = PFN_FROM_PTE(PointerPte) << PAGE_SHIFT;
    PsGetCurrentProcess()->Pcb.DirectoryTableBase[0] = PageFrameIndex;
    
    //
    // Blow away user-mode
    //
    StartPde = MiAddressToPde(0);
    EndPde = MiAddressToPde(KSEG0_BASE);
    RtlZeroMemory(StartPde, (EndPde - StartPde) * sizeof(MMPTE));
    
    //
    // Loop the memory descriptors
    //
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        //
        // Get the memory block
        //
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        
        //
        // Skip invisible memory
        //
        if ((MdBlock->MemoryType != LoaderFirmwarePermanent) &&
            (MdBlock->MemoryType != LoaderSpecialMemory) &&
            (MdBlock->MemoryType != LoaderHALCachedMemory) &&
            (MdBlock->MemoryType != LoaderBBTMemory))
        {
            //
            // Check if BURNMEM was used
            //
            if (MdBlock->MemoryType != LoaderBad)
            {
                //
                // Count this in the total of pages
                //
                MmNumberOfPhysicalPages += MdBlock->PageCount;
            }
            
            //
            // Check if this is the new lowest page
            //
            if (MdBlock->BasePage < MmLowestPhysicalPage)
            {
                //
                // Update the lowest page
                //
                MmLowestPhysicalPage = MdBlock->BasePage;
            }
            
            //
            // Check if this is the new highest page
            //
            PageFrameIndex = MdBlock->BasePage + MdBlock->PageCount;
            if (PageFrameIndex > MmHighestPhysicalPage)
            {
                //
                // Update the highest page
                //
                MmHighestPhysicalPage = PageFrameIndex - 1;
            }
            
            //
            // Check if this is free memory
            //
            if ((MdBlock->MemoryType == LoaderFree) ||
                (MdBlock->MemoryType == LoaderLoadedProgram) ||
                (MdBlock->MemoryType == LoaderFirmwareTemporary) ||
                (MdBlock->MemoryType == LoaderOsloaderStack))
            {
                //
                // Check if this is the largest memory descriptor
                //
                if (MdBlock->PageCount > FreePages)
                {
                    //
                    // For now, it is
                    //
                    MxFreeDescriptor = MdBlock;
                }
                
                //
                // More free pages
                //
                FreePages += MdBlock->PageCount;
            }
        }
        
        //
        // Keep going
        //
        NextEntry = MdBlock->ListEntry.Flink;
    }
    
    //
    // Save original values of the free descriptor, since it'll be
    // altered by early allocations
    //
    MxOldFreeDescriptor = *MxFreeDescriptor;
    
    /* Compute non paged pool limits and size */
    MiComputeNonPagedPoolVa(FreePages);
    
    /* Compute color information (L2 cache-separated paging lists) */
    MiComputeColorInformation();
    
    //
    // Calculate the number of bytes for the PFN database
    // and then convert to pages
    //
    MxPfnAllocation = (MmHighestPhysicalPage + 1) * sizeof(MMPFN);
    MxPfnAllocation >>= PAGE_SHIFT;
    
    //
    // We have to add one to the count here, because in the process of
    // shifting down to the page size, we actually ended up getting the
    // lower aligned size (so say, 0x5FFFF bytes is now 0x5F pages).
    // Later on, we'll shift this number back into bytes, which would cause
    // us to end up with only 0x5F000 bytes -- when we actually want to have
    // 0x60000 bytes.
    //
    MxPfnAllocation++;
    
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
    // Check if we are in a situation where the size of the paged pool
    // is so large that it overflows into nonpaged pool
    //
    if (MmSizeOfPagedPoolInBytes >
        ((ULONG_PTR)MmNonPagedSystemStart - (ULONG_PTR)MmPagedPoolStart))
    {
        //
        // We need some recalculations here
        //
        DPRINT1("Paged pool is too big!\n");
    }
    
    //
    // Normally, the PFN database should start after the loader images.
    // This is already the case in ReactOS, but for now we want to co-exist
    // with the old memory manager, so we'll create a "Shadow PFN Database"
    // instead, and arbitrarly start it at 0xB0000000.
    //
    MmPfnDatabase = (PVOID)0xB0000000;
    ASSERT(((ULONG_PTR)MmPfnDatabase & ((4 * 1024 * 1024) - 1)) == 0);
            
    //
    // Non paged pool comes after the PFN database
    //
    MmNonPagedPoolStart = (PVOID)((ULONG_PTR)MmPfnDatabase +
                                  (MxPfnAllocation << PAGE_SHIFT));

    //
    // Now we actually need to get these many physical pages. Nonpaged pool
    // is actually also physically contiguous (but not the expansion)
    //
    PageFrameIndex = MxGetNextPage(MxPfnAllocation +
                                   (MmSizeOfNonPagedPoolInBytes >> PAGE_SHIFT));
    ASSERT(PageFrameIndex != 0);
    DPRINT("PFN DB PA PFN begins at: %lx\n", PageFrameIndex);
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
        TempPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
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
    StartPde = MiAddressToPde(MmPfnDatabase);
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
        TempPde.u.Hard.PageFrameNumber = MxGetNextPage(1);
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
    // Sanity check: make sure we have properly defined the system PTE space
    //
    ASSERT(MiAddressToPte(MmNonPagedSystemStart) <
           MiAddressToPte(MmNonPagedPoolExpansionStart));
    
    //
    // Now go ahead and initialize the ARM³ nonpaged pool
    //
    MiInitializeArmPool();

    //
    // Get current page data, since we won't be using MxGetNextPage as it
    // would corrupt our state
    //
    FreePage = MxFreeDescriptor->BasePage;
    FreePageCount = MxFreeDescriptor->PageCount;
    PagesLeft = 0;
    
    //
    // Loop the memory descriptors
    //
    NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &KeLoaderBlock->MemoryDescriptorListHead)
    {
        //
        // Get the descriptor
        //
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        if ((MdBlock->MemoryType == LoaderFirmwarePermanent) ||
            (MdBlock->MemoryType == LoaderBBTMemory) ||
            (MdBlock->MemoryType == LoaderSpecialMemory))
        {
            //
            // These pages are not part of the PFN database
            //
            NextEntry = MdBlock->ListEntry.Flink;
            continue;
        }
        
        //
        // Next, check if this is our special free descriptor we've found
        //
        if (MdBlock == MxFreeDescriptor)
        {
            //
            // Use the real numbers instead
            //
            BasePage = MxOldFreeDescriptor.BasePage;
            PageCount = MxOldFreeDescriptor.PageCount;
        }
        else
        {
            //
            // Use the descriptor's numbers
            //
            BasePage = MdBlock->BasePage;
            PageCount = MdBlock->PageCount;
        }
        
        //
        // Get the PTEs for this range
        //
        PointerPte = MiAddressToPte(&MmPfnDatabase[BasePage]);
        LastPte = MiAddressToPte(((ULONG_PTR)&MmPfnDatabase[BasePage + PageCount]) - 1);
        DPRINT("MD Type: %lx Base: %lx Count: %lx\n", MdBlock->MemoryType, BasePage, PageCount);
        
        //
        // Loop them
        //
        while (PointerPte <= LastPte)
        {
            //
            // We'll only touch PTEs that aren't already valid
            //
            if (PointerPte->u.Hard.Valid == 0)
            {
                //
                // Use the next free page
                //
                TempPte.u.Hard.PageFrameNumber = FreePage;
                ASSERT(FreePageCount != 0);
                
                //
                // Consume free pages
                //
                FreePage++;
                FreePageCount--;
                if (!FreePageCount)
                {
                    //
                    // Out of memory
                    //
                    KeBugCheckEx(INSTALL_MORE_MEMORY,
                                 MmNumberOfPhysicalPages,
                                 FreePageCount,
                                 MxOldFreeDescriptor.PageCount,
                                 1);
                }
                
                //
                // Write out this PTE
                //
                PagesLeft++;
                ASSERT(PointerPte->u.Hard.Valid == 0);
                ASSERT(TempPte.u.Hard.Valid == 1);
                *PointerPte = TempPte;
                
                //
                // Zero this page
                //
                RtlZeroMemory(MiPteToAddress(PointerPte), PAGE_SIZE);
            }
            
            //
            // Next!
            //
            PointerPte++;
        }
        
        //
        // Do the next address range
        //
        NextEntry = MdBlock->ListEntry.Flink;
    }
    
    //
    // Now update the free descriptors to consume the pages we used up during
    // the PFN allocation loop
    //
    MxFreeDescriptor->BasePage = FreePage;
    MxFreeDescriptor->PageCount = FreePageCount;

    /* Call back into shitMM to setup the PFN database */
    MmInitializePageList();
        
    //
    // Reset the descriptor back so we can create the correct memory blocks
    //
    *MxFreeDescriptor = MxOldFreeDescriptor;
    
    //
    // Initialize the nonpaged pool
    //
    InitializePool(NonPagedPool, 0);
    
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
    
    return STATUS_SUCCESS;
}

/* EOF */
