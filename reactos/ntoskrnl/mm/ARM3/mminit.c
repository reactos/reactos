/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/mm/ARM3/mminit.c
 * PURPOSE:         ARM Memory Manager Initialization
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#line 15 "ARM³::INIT"
#define MODULE_INVOLVED_IN_ARM3
#include "miarm.h"

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
// Right now we call this the "ARM³ Nonpaged Pool" and it begins somewhere after
// the PFN database (which starts at 0xB0000000).
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
PVOID MmNonPagedPoolEnd = MI_NONPAGED_POOL_END;

//
// This is where paged pool starts by default
//
PVOID MmPagedPoolStart = MI_PAGED_POOL_START;
PVOID MmPagedPoolEnd;

//
// And this is its default size
//
ULONG MmSizeOfPagedPoolInBytes = MI_MIN_INIT_PAGED_POOLSIZE;
PFN_NUMBER MmSizeOfPagedPoolInPages = MI_MIN_INIT_PAGED_POOLSIZE / PAGE_SIZE;

//
// Session space starts at 0xBFFFFFFF and grows downwards
// By default, it includes an 8MB image area where we map win32k and video card
// drivers, followed by a 4MB area containing the session's working set. This is
// then followed by a 20MB mapped view area and finally by the session's paged
// pool, by default 16MB.
// 
// On a normal system, this results in session space occupying the region from
// 0xBD000000 to 0xC0000000
//
// See miarm.h for the defines that determine the sizing of this region. On an
// NT system, some of these can be configured through the registry, but we don't
// support that yet.
//
PVOID MiSessionSpaceEnd;    // 0xC0000000
PVOID MiSessionImageEnd;    // 0xC0000000
PVOID MiSessionImageStart;  // 0xBF800000
PVOID MiSessionViewStart;   // 0xBE000000
PVOID MiSessionPoolEnd;     // 0xBE000000
PVOID MiSessionPoolStart;   // 0xBD000000
PVOID MmSessionBase;        // 0xBD000000
ULONG MmSessionSize;
ULONG MmSessionViewSize;
ULONG MmSessionPoolSize;
ULONG MmSessionImageSize;

//
// The system view space, on the other hand, is where sections that are memory
// mapped into "system space" end up.
//
// By default, it is a 16MB region.
//
PVOID MiSystemViewStart;
ULONG MmSystemViewSize;

//
// A copy of the system page directory (the page directory associated with the
// System process) is kept (double-mapped) by the manager in order to lazily
// map paged pool PDEs into external processes when they fault on a paged pool
// address.
//
PFN_NUMBER MmSystemPageDirectory;
PMMPTE MmSystemPagePtes;

//
// The system cache starts right after hyperspace. The first few pages are for
// keeping track of the system working set list.
//
// This should be 0xC0C00000 -- the cache itself starts at 0xC1000000
//
PMMWSL MmSystemCacheWorkingSetList = MI_SYSTEM_CACHE_WS_START;

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
// Unlike the old ReactOS Memory Manager, ARM³ (and Windows) does not keep track
// of pages that are not actually valid physical memory, such as ACPI reserved
// regions, BIOS address ranges, or holes in physical memory address space which
// could indicate device-mapped I/O memory.
//
// In fact, the lack of a PFN entry for a page usually indicates that this is
// I/O space instead.
//
// A bitmap, called the PFN bitmap, keeps track of all page frames by assigning
// a bit to each. If the bit is set, then the page is valid physical RAM.
//
RTL_BITMAP MiPfnBitMap;

//
// This structure describes the different pieces of RAM-backed address space
//
PPHYSICAL_MEMORY_DESCRIPTOR MmPhysicalMemoryBlock;

//
// This is where we keep track of the most basic physical layout markers
//
ULONG MmNumberOfPhysicalPages, MmHighestPhysicalPage, MmLowestPhysicalPage = -1;

//
// The total number of pages mapped by the boot loader, which include the kernel
// HAL, boot drivers, registry, NLS files and other loader data structures is
// kept track of here. This depends on "LoaderPagesSpanned" being correct when
// coming from the loader.
//
// This number is later aligned up to a PDE boundary.
//
ULONG MmBootImageSize;

//
// These three variables keep track of the core separation of address space that
// exists between kernel mode and user mode.
//
ULONG MmUserProbeAddress;
PVOID MmHighestUserAddress;
PVOID MmSystemRangeStart;

PVOID MmSystemCacheStart;
PVOID MmSystemCacheEnd;
MMSUPPORT MmSystemCacheWs;

//
// This is where hyperspace ends (followed by the system cache working set)
//
PVOID MmHyperSpaceEnd;

//
// Page coloring algorithm data
//
ULONG MmSecondaryColors;
ULONG MmSecondaryColorMask;

//
// Actual (registry-configurable) size of a GUI thread's stack
//
ULONG MmLargeStackSize = KERNEL_LARGE_STACK_SIZE;

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

/*
 * For each page's worth bytes of L2 cache in a given set/way line, the zero and
 * free lists are organized in what is called a "color".
 *
 * This array points to the two lists, so it can be thought of as a multi-dimensional
 * array of MmFreePagesByColor[2][MmSecondaryColors]. Since the number is dynamic,
 * we describe the array in pointer form instead.
 *
 * On a final note, the color tables themselves are right after the PFN database.
 */
C_ASSERT(FreePageList == 1);
PMMCOLOR_TABLES MmFreePagesByColor[FreePageList + 1];

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
    while (Pde <= ADDR_TO_PDE_OFFSET(AddressEnd))
    {
        //
        // This both odious and heinous
        //
        extern ULONG MmGlobalKernelPageDirectory[1024];
        MmGlobalKernelPageDirectory[Pde] = ((PULONG)PDE_BASE)[Pde];
        Pde++;
    }
}

PFN_NUMBER
NTAPI
MxGetNextPage(IN PFN_NUMBER PageCount)
{
    PFN_NUMBER Pfn;
 
    /* Make sure we have enough pages */
    if (PageCount > MxFreeDescriptor->PageCount)
    {
        /* Crash the system */
        KeBugCheckEx(INSTALL_MORE_MEMORY,
                     MmNumberOfPhysicalPages,
                     MxFreeDescriptor->PageCount,
                     MxOldFreeDescriptor.PageCount,
                     PageCount);
    }
    
    /* Use our lowest usable free pages */
    Pfn = MxFreeDescriptor->BasePage;
    MxFreeDescriptor->BasePage += PageCount;
    MxFreeDescriptor->PageCount -= PageCount;
    return Pfn;
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

VOID
NTAPI
MiInitializeColorTables(VOID)
{
    ULONG i;
    PMMPTE PointerPte, LastPte;
    MMPTE TempPte = ValidKernelPte;
    
    /* The color table starts after the ARM3 PFN database */
    MmFreePagesByColor[0] = (PMMCOLOR_TABLES)&MmPfnDatabase[1][MmHighestPhysicalPage + 1];
    
    /* Loop the PTEs. We have two color tables for each secondary color */
    PointerPte = MiAddressToPte(&MmFreePagesByColor[0][0]);
    LastPte = MiAddressToPte((ULONG_PTR)MmFreePagesByColor[0] +
                             (2 * MmSecondaryColors * sizeof(MMCOLOR_TABLES))
                             - 1);
    while (PointerPte <= LastPte)
    {
        /* Check for valid PTE */
        if (PointerPte->u.Hard.Valid == 0)
        {
            /* Get a page and map it */
            TempPte.u.Hard.PageFrameNumber = MxGetNextPage(1);
            ASSERT(TempPte.u.Hard.Valid == 1);
            *PointerPte = TempPte;
            
            /* Zero out the page */
            RtlZeroMemory(MiPteToAddress(PointerPte), PAGE_SIZE);
        }
        
        /* Next */
        PointerPte++;
    }
    
    /* Now set the address of the next list, right after this one */
    MmFreePagesByColor[1] = &MmFreePagesByColor[0][MmSecondaryColors];
    
    /* Now loop the lists to set them up */
    for (i = 0; i < MmSecondaryColors; i++)
    {
        /* Set both free and zero lists for each color */
        MmFreePagesByColor[ZeroedPageList][i].Flink = 0xFFFFFFFF;
        MmFreePagesByColor[ZeroedPageList][i].Blink = (PVOID)0xFFFFFFFF;
        MmFreePagesByColor[ZeroedPageList][i].Count = 0;
        MmFreePagesByColor[FreePageList][i].Flink = 0xFFFFFFFF;
        MmFreePagesByColor[FreePageList][i].Blink = (PVOID)0xFFFFFFFF;
        MmFreePagesByColor[FreePageList][i].Count = 0;
    }
}

BOOLEAN
NTAPI
MiIsRegularMemory(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                  IN PFN_NUMBER Pfn)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;

    /* Loop the memory descriptors */
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        /* Get the memory descriptor */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Check if this PFN could be part of the block */
        if (Pfn >= (MdBlock->BasePage))
        {
            /* Check if it really is part of the block */
            if (Pfn < (MdBlock->BasePage + MdBlock->PageCount))
            {
                /* Check if the block is actually memory we don't map */
                if ((MdBlock->MemoryType == LoaderFirmwarePermanent) ||
                    (MdBlock->MemoryType == LoaderBBTMemory) ||
                    (MdBlock->MemoryType == LoaderSpecialMemory))
                {
                    /* We don't need PFN database entries for this memory */
                    break;
                }

                /* This is memory we want to map */
                return TRUE;
            }
        }
        else
        {
            /* Blocks are ordered, so if it's not here, it doesn't exist */
            break;
        }

        /* Get to the next descriptor */
        NextEntry = MdBlock->ListEntry.Flink;
    }

    /* Check if this PFN is actually from our free memory descriptor */
    if ((Pfn >= MxOldFreeDescriptor.BasePage) &&
        (Pfn < MxOldFreeDescriptor.BasePage + MxOldFreeDescriptor.PageCount))
    {
        /* We use these pages for initial mappings, so we do want to count them */
        return TRUE;
    }

    /* Otherwise this isn't memory that we describe or care about */
    return FALSE;
}

VOID
NTAPI
MiMapPfnDatabase(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG FreePage, FreePageCount, PagesLeft, BasePage, PageCount;
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    PMMPTE PointerPte, LastPte;
    MMPTE TempPte = ValidKernelPte;
    
    /* Get current page data, since we won't be using MxGetNextPage as it would corrupt our state */
    FreePage = MxFreeDescriptor->BasePage;
    FreePageCount = MxFreeDescriptor->PageCount;
    PagesLeft = 0;
    
    /* Loop the memory descriptors */
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        /* Get the descriptor */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        if ((MdBlock->MemoryType == LoaderFirmwarePermanent) ||
            (MdBlock->MemoryType == LoaderBBTMemory) ||
            (MdBlock->MemoryType == LoaderSpecialMemory))
        {
            /* These pages are not part of the PFN database */
            NextEntry = MdBlock->ListEntry.Flink;
            continue;
        }
        
        /* Next, check if this is our special free descriptor we've found */
        if (MdBlock == MxFreeDescriptor)
        {
            /* Use the real numbers instead */
            BasePage = MxOldFreeDescriptor.BasePage;
            PageCount = MxOldFreeDescriptor.PageCount;
        }
        else
        {
            /* Use the descriptor's numbers */
            BasePage = MdBlock->BasePage;
            PageCount = MdBlock->PageCount;
        }
        
        /* Get the PTEs for this range */
        PointerPte = MiAddressToPte(&MmPfnDatabase[0][BasePage]);
        LastPte = MiAddressToPte(((ULONG_PTR)&MmPfnDatabase[0][BasePage + PageCount]) - 1);
        DPRINT("MD Type: %lx Base: %lx Count: %lx\n", MdBlock->MemoryType, BasePage, PageCount);
        
        /* Loop them */
        while (PointerPte <= LastPte)
        {
            /* We'll only touch PTEs that aren't already valid */
            if (PointerPte->u.Hard.Valid == 0)
            {
                /* Use the next free page */
                TempPte.u.Hard.PageFrameNumber = FreePage;
                ASSERT(FreePageCount != 0);
                
                /* Consume free pages */
                FreePage++;
                FreePageCount--;
                if (!FreePageCount)
                {
                    /* Out of memory */
                    KeBugCheckEx(INSTALL_MORE_MEMORY,
                                 MmNumberOfPhysicalPages,
                                 FreePageCount,
                                 MxOldFreeDescriptor.PageCount,
                                 1);
                }
                
                /* Write out this PTE */
                PagesLeft++;
                ASSERT(PointerPte->u.Hard.Valid == 0);
                ASSERT(TempPte.u.Hard.Valid == 1);
                *PointerPte = TempPte;
                
                /* Zero this page */
                RtlZeroMemory(MiPteToAddress(PointerPte), PAGE_SIZE);
            }
            
            /* Next! */
            PointerPte++;
        }
        
        /* Do the next address range */
        NextEntry = MdBlock->ListEntry.Flink;
    }
    
    /* Now update the free descriptors to consume the pages we used up during the PFN allocation loop */
    MxFreeDescriptor->BasePage = FreePage;
    MxFreeDescriptor->PageCount = FreePageCount;
}

PFN_NUMBER
NTAPI
MiPagesInLoaderBlock(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                     IN PBOOLEAN IncludeType)
{
    PLIST_ENTRY NextEntry;
    PFN_NUMBER PageCount = 0;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    
    //
    // Now loop through the descriptors
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
        }
        
        //
        // Try the next descriptor
        //
        NextEntry = MdBlock->ListEntry.Flink;
    }
    
    //
    // Return the total
    //
    return PageCount;
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

VOID
NTAPI
MiBuildPagedPool(VOID)
{
    PMMPTE PointerPte, PointerPde;
    MMPTE TempPte = ValidKernelPte;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    ULONG Size, BitMapSize;
    
    //
    // Get the page frame number for the system page directory
    //
    PointerPte = MiAddressToPte(PDE_BASE);
    MmSystemPageDirectory = PFN_FROM_PTE(PointerPte);
    
    //
    // Allocate a system PTE which will hold a copy of the page directory
    //
    PointerPte = MiReserveSystemPtes(1, SystemPteSpace);
    ASSERT(PointerPte);
    MmSystemPagePtes = MiPteToAddress(PointerPte);

    //
    // Make this system PTE point to the system page directory.
    // It is now essentially double-mapped. This will be used later for lazy
    // evaluation of PDEs accross process switches, similarly to how the Global
    // page directory array in the old ReactOS Mm is used (but in a less hacky
    // way).
    //
    TempPte = ValidKernelPte;
    TempPte.u.Hard.PageFrameNumber = MmSystemPageDirectory;
    ASSERT(PointerPte->u.Hard.Valid == 0);
    ASSERT(TempPte.u.Hard.Valid == 1);
    *PointerPte = TempPte;

    //
    // Let's get back to paged pool work: size it up.
    // By default, it should be twice as big as nonpaged pool.
    //
    MmSizeOfPagedPoolInBytes = 2 * MmMaximumNonPagedPoolInBytes;
    if (MmSizeOfPagedPoolInBytes > ((ULONG_PTR)MmNonPagedSystemStart -
                                    (ULONG_PTR)MmPagedPoolStart))
    {
        //
        // On the other hand, we have limited VA space, so make sure that the VA
        // for paged pool doesn't overflow into nonpaged pool VA. Otherwise, set
        // whatever maximum is possible.
        //
        MmSizeOfPagedPoolInBytes = (ULONG_PTR)MmNonPagedSystemStart -
                                   (ULONG_PTR)MmPagedPoolStart;
    }

    //
    // Get the size in pages and make sure paged pool is at least 32MB.
    //
    Size = MmSizeOfPagedPoolInBytes;
    if (Size < MI_MIN_INIT_PAGED_POOLSIZE) Size = MI_MIN_INIT_PAGED_POOLSIZE;
    Size = BYTES_TO_PAGES(Size);

    //
    // Now check how many PTEs will be required for these many pages.
    //
    Size = (Size + (1024 - 1)) / 1024;

    //
    // Recompute the page-aligned size of the paged pool, in bytes and pages.
    //
    MmSizeOfPagedPoolInBytes = Size * PAGE_SIZE * 1024;
    MmSizeOfPagedPoolInPages = MmSizeOfPagedPoolInBytes >> PAGE_SHIFT;

    //
    // Let's be really sure this doesn't overflow into nonpaged system VA
    //
    ASSERT((MmSizeOfPagedPoolInBytes + (ULONG_PTR)MmPagedPoolStart) <= 
           (ULONG_PTR)MmNonPagedSystemStart);

    //
    // This is where paged pool ends
    //
    MmPagedPoolEnd = (PVOID)(((ULONG_PTR)MmPagedPoolStart +
                              MmSizeOfPagedPoolInBytes) - 1);

    //
    // So now get the PDE for paged pool and zero it out
    //
    PointerPde = MiAddressToPde(MmPagedPoolStart);
    RtlZeroMemory(PointerPde,
                  (1 + MiAddressToPde(MmPagedPoolEnd) - PointerPde) * sizeof(MMPTE));

    //
    // Next, get the first and last PTE
    //
    PointerPte = MiAddressToPte(MmPagedPoolStart);
    MmPagedPoolInfo.FirstPteForPagedPool = PointerPte;
    MmPagedPoolInfo.LastPteForPagedPool = MiAddressToPte(MmPagedPoolEnd);

    //
    // Lock the PFN database
    //
    OldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);

    //
    // Allocate a page and map the first paged pool PDE
    //
    PageFrameIndex = MmAllocPage(MC_NPPOOL, 0);
    TempPte.u.Hard.PageFrameNumber = PageFrameIndex;
    ASSERT(PointerPde->u.Hard.Valid == 0);
    ASSERT(TempPte.u.Hard.Valid == 1);
    *PointerPde = TempPte;

    //
    // Release the PFN database lock
    //
    KeReleaseQueuedSpinLock(LockQueuePfnLock, OldIrql);

    //
    // We only have one PDE mapped for now... at fault time, additional PDEs
    // will be allocated to handle paged pool growth. This is where they'll have
    // to start.
    //
    MmPagedPoolInfo.NextPdeForPagedPoolExpansion = PointerPde + 1;

    //
    // We keep track of each page via a bit, so check how big the bitmap will
    // have to be (make sure to align our page count such that it fits nicely
    // into a 4-byte aligned bitmap.
    //
    // We'll also allocate the bitmap header itself part of the same buffer.
    //
    Size = Size * 1024;
    ASSERT(Size == MmSizeOfPagedPoolInPages);
    BitMapSize = Size;
    Size = sizeof(RTL_BITMAP) + (((Size + 31) / 32) * sizeof(ULONG));

    //
    // Allocate the allocation bitmap, which tells us which regions have not yet
    // been mapped into memory
    //
    MmPagedPoolInfo.PagedPoolAllocationMap = ExAllocatePoolWithTag(NonPagedPool,
                                                                   Size,
                                                                   '  mM');
    ASSERT(MmPagedPoolInfo.PagedPoolAllocationMap);

    //
    // Initialize it such that at first, only the first page's worth of PTEs is
    // marked as allocated (incidentially, the first PDE we allocated earlier).
    //
    RtlInitializeBitMap(MmPagedPoolInfo.PagedPoolAllocationMap,
                        (PULONG)(MmPagedPoolInfo.PagedPoolAllocationMap + 1),
                        BitMapSize);
    RtlSetAllBits(MmPagedPoolInfo.PagedPoolAllocationMap);
    RtlClearBits(MmPagedPoolInfo.PagedPoolAllocationMap, 0, 1024);

    //
    // We have a second bitmap, which keeps track of where allocations end.
    // Given the allocation bitmap and a base address, we can therefore figure
    // out which page is the last page of that allocation, and thus how big the
    // entire allocation is.
    //
    MmPagedPoolInfo.EndOfPagedPoolBitmap = ExAllocatePoolWithTag(NonPagedPool,
                                                                 Size,
                                                                 '  mM');
    ASSERT(MmPagedPoolInfo.EndOfPagedPoolBitmap);
    RtlInitializeBitMap(MmPagedPoolInfo.EndOfPagedPoolBitmap,
                        (PULONG)(MmPagedPoolInfo.EndOfPagedPoolBitmap + 1),
                        BitMapSize);

    //
    // Since no allocations have been made yet, there are no bits set as the end
    //
    RtlClearAllBits(MmPagedPoolInfo.EndOfPagedPoolBitmap);

    //
    // Initialize paged pool.
    //
    InitializePool(PagedPool, 0);

    //
    // Initialize the paged pool mutex
    //
    KeInitializeGuardedMutex(&MmPagedPoolMutex);
}

NTSTATUS
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG i;
    BOOLEAN IncludeType[LoaderMaximum];
    PVOID Bitmap;
    PPHYSICAL_MEMORY_RUN Run;
    PFN_NUMBER PageCount;
    
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
    if (Phase == 0)
    {
        //
        // Define the basic user vs. kernel address space separation
        //
        MmSystemRangeStart = (PVOID)KSEG0_BASE;
        MmUserProbeAddress = (ULONG_PTR)MmSystemRangeStart - 0x10000;
        MmHighestUserAddress = (PVOID)(MmUserProbeAddress - 1);
        
        //
        // Get the size of the boot loader's image allocations and then round
        // that region up to a PDE size, so that any PDEs we might create for
        // whatever follows are separate from the PDEs that boot loader might've
        // already created (and later, we can blow all that away if we want to).
        //
        MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned;
        MmBootImageSize *= PAGE_SIZE;
        MmBootImageSize = (MmBootImageSize + (4 * 1024 * 1024) - 1) & ~((4 * 1024 * 1024) - 1);
        ASSERT((MmBootImageSize % (4 * 1024 * 1024)) == 0);
        
        //
        // Set the size of session view, pool, and image
        //
        MmSessionSize = MI_SESSION_SIZE;
        MmSessionViewSize = MI_SESSION_VIEW_SIZE;
        MmSessionPoolSize = MI_SESSION_POOL_SIZE;
        MmSessionImageSize = MI_SESSION_IMAGE_SIZE;
        
        //
        // Set the size of system view
        //
        MmSystemViewSize = MI_SYSTEM_VIEW_SIZE;
        
        //
        // This is where it all ends
        //
        MiSessionImageEnd = (PVOID)PTE_BASE;
        
        //
        // This is where we will load Win32k.sys and the video driver
        //
        MiSessionImageStart = (PVOID)((ULONG_PTR)MiSessionImageEnd -
                                      MmSessionImageSize);
        
        //
        // So the view starts right below the session working set (itself below
        // the image area)
        //
        MiSessionViewStart = (PVOID)((ULONG_PTR)MiSessionImageEnd -
                                     MmSessionImageSize -
                                     MI_SESSION_WORKING_SET_SIZE - 
                                     MmSessionViewSize);
        
        //
        // Session pool follows
        //
        MiSessionPoolEnd = MiSessionViewStart;
        MiSessionPoolStart = (PVOID)((ULONG_PTR)MiSessionPoolEnd -
                                     MmSessionPoolSize);
        
        //
        // And it all begins here
        //
        MmSessionBase = MiSessionPoolStart;
        
        //
        // Sanity check that our math is correct
        //
        ASSERT((ULONG_PTR)MmSessionBase + MmSessionSize == PTE_BASE);
        
        //
        // Session space ends wherever image session space ends
        //
        MiSessionSpaceEnd = MiSessionImageEnd;
        
        //
        // System view space ends at session space, so now that we know where
        // this is, we can compute the base address of system view space itself.
        //
        MiSystemViewStart = (PVOID)((ULONG_PTR)MmSessionBase -
                                    MmSystemViewSize);
                                    
        //
        // Count physical pages on the system
        //
        PageCount = MiPagesInLoaderBlock(LoaderBlock, IncludeType);
        
        //
        // Check if this is a machine with less than 19MB of RAM
        //
        if (PageCount < MI_MIN_PAGES_FOR_SYSPTE_TUNING)
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
            if (PageCount > MI_MIN_PAGES_FOR_SYSPTE_BOOST)
            {
                //
                // Double the amount of system PTEs
                //
                MmNumberOfSystemPtes <<= 1;
            }
        }
        
        DPRINT("System PTE count has been tuned to %d (%d bytes)\n",
               MmNumberOfSystemPtes, MmNumberOfSystemPtes * PAGE_SIZE);
        
        /* Initialize the platform-specific parts */       
        MiInitMachineDependent(LoaderBlock);
        
        //
        // Sync us up with ReactOS Mm
        //
        MiSyncARM3WithROS(MmNonPagedSystemStart, (PVOID)((ULONG_PTR)MmNonPagedPoolEnd - 1));
        MiSyncARM3WithROS(MmPfnDatabase[0], (PVOID)((ULONG_PTR)MmNonPagedPoolStart + MmSizeOfNonPagedPoolInBytes - 1));
        MiSyncARM3WithROS((PVOID)HYPER_SPACE, (PVOID)(HYPER_SPACE + PAGE_SIZE - 1));
      
        //
        // Build the physical memory block
        //
        MmPhysicalMemoryBlock = MmInitializeMemoryLimits(LoaderBlock,
                                                         IncludeType);
         
        //
        // Allocate enough buffer for the PFN bitmap
        // Align it up to a 32-bit boundary
        //
        Bitmap = ExAllocatePoolWithTag(NonPagedPool,
                                       (((MmHighestPhysicalPage + 1) + 31) / 32) * 4,
                                       '  mM');
        if (!Bitmap)
        {
            //
            // This is critical
            //
            KeBugCheckEx(INSTALL_MORE_MEMORY,
                         MmNumberOfPhysicalPages,
                         MmLowestPhysicalPage,
                         MmHighestPhysicalPage,
                         0x101);
        }
        
        //
        // Initialize it and clear all the bits to begin with
        //
        RtlInitializeBitMap(&MiPfnBitMap,
                            Bitmap,
                            MmHighestPhysicalPage + 1);
        RtlClearAllBits(&MiPfnBitMap);
        
        //
        // Loop physical memory runs
        //
        for (i = 0; i < MmPhysicalMemoryBlock->NumberOfRuns; i++)
        {
            //
            // Get the run
            //
            Run = &MmPhysicalMemoryBlock->Run[i];
            DPRINT("PHYSICAL RAM [0x%08p to 0x%08p]\n",
                   Run->BasePage << PAGE_SHIFT,
                   (Run->BasePage + Run->PageCount) << PAGE_SHIFT);

            //
            // Make sure it has pages inside it
            //
            if (Run->PageCount)
            {
                //
                // Set the bits in the PFN bitmap
                //
                RtlSetBits(&MiPfnBitMap, Run->BasePage, Run->PageCount);
            }
        }
        
        //
        // Size up paged pool and build the shadow system page directory
        //
        MiBuildPagedPool();
    }
    
    //
    // Always return success for now
    //
    return STATUS_SUCCESS;
}

/* EOF */
