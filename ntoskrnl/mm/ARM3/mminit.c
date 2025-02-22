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

#define MODULE_INVOLVED_IN_ARM3
#include "miarm.h"
#undef MmSystemRangeStart

/* GLOBALS ********************************************************************/

//
// These are all registry-configurable, but by default, the memory manager will
// figure out the most appropriate values.
//
ULONG MmMaximumNonPagedPoolPercent;
SIZE_T MmSizeOfNonPagedPoolInBytes;
SIZE_T MmMaximumNonPagedPoolInBytes;

/* Some of the same values, in pages */
PFN_NUMBER MmMaximumNonPagedPoolInPages;

//
// These numbers describe the discrete equation components of the nonpaged
// pool sizing algorithm.
//
// They are described on http://support.microsoft.com/default.aspx/kb/126402/ja (DEAD_LINK)
// along with the algorithm that uses them, which is implemented later below.
//
SIZE_T MmMinimumNonPagedPoolSize = 256 * 1024;
ULONG MmMinAdditionNonPagedPoolPerMb = 32 * 1024;
SIZE_T MmDefaultMaximumNonPagedPool = 1024 * 1024;
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
// https://web.archive.org/web/20130412053421/http://www.ditii.com/2007/09/28/windows-memory-management-x86-virtual-address-space/
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
SIZE_T MmSizeOfPagedPoolInBytes = MI_MIN_INIT_PAGED_POOLSIZE;
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
PVOID MiSessionSpaceWs;
PVOID MiSessionViewStart;   // 0xBE000000
PVOID MiSessionPoolEnd;     // 0xBE000000
PVOID MiSessionPoolStart;   // 0xBD000000
PVOID MmSessionBase;        // 0xBD000000
SIZE_T MmSessionSize;
SIZE_T MmSessionViewSize;
SIZE_T MmSessionPoolSize;
SIZE_T MmSessionImageSize;

/*
 * These are the PTE addresses of the boundaries carved out above
 */
PMMPTE MiSessionImagePteStart;
PMMPTE MiSessionImagePteEnd;
PMMPTE MiSessionBasePte;
PMMPTE MiSessionLastPte;

//
// The system view space, on the other hand, is where sections that are memory
// mapped into "system space" end up.
//
// By default, it is a 16MB region, but we hack it to be 32MB for ReactOS
//
PVOID MiSystemViewStart;
SIZE_T MmSystemViewSize;

#if (_MI_PAGING_LEVELS == 2)
//
// A copy of the system page directory (the page directory associated with the
// System process) is kept (double-mapped) by the manager in order to lazily
// map paged pool PDEs into external processes when they fault on a paged pool
// address.
//
PFN_NUMBER MmSystemPageDirectory[PPE_PER_PAGE];
PMMPDE MmSystemPagePtes;
#endif

//
// The system cache starts right after hyperspace. The first few pages are for
// keeping track of the system working set list.
//
// This should be 0xC0C00000 -- the cache itself starts at 0xC1000000
//
PMMWSL MmSystemCacheWorkingSetList = (PVOID)MI_SYSTEM_CACHE_WS_START;

//
// Windows NT seems to choose between 7000, 11000 and 50000
// On systems with more than 32MB, this number is then doubled, and further
// aligned up to a PDE boundary (4MB).
//
PFN_COUNT MmNumberOfSystemPtes;

//
// This is how many pages the PFN database will take up
// In Windows, this includes the Quark Color Table, but not in ARM³
//
PFN_NUMBER MxPfnAllocation;

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
PFN_NUMBER MmHighestPhysicalPage, MmLowestPhysicalPage = -1;
PFN_COUNT MmNumberOfPhysicalPages;

//
// The total number of pages mapped by the boot loader, which include the kernel
// HAL, boot drivers, registry, NLS files and other loader data structures is
// kept track of here. This depends on "LoaderPagesSpanned" being correct when
// coming from the loader.
//
// This number is later aligned up to a PDE boundary.
//
SIZE_T MmBootImageSize;

//
// These three variables keep track of the core separation of address space that
// exists between kernel mode and user mode.
//
ULONG_PTR MmUserProbeAddress;
PVOID MmHighestUserAddress;
PVOID MmSystemRangeStart;

/* And these store the respective highest PTE/PDE address */
PMMPTE MiHighestUserPte;
PMMPDE MiHighestUserPde;
#if (_MI_PAGING_LEVELS >= 3)
PMMPTE MiHighestUserPpe;
#if (_MI_PAGING_LEVELS >= 4)
PMMPTE MiHighestUserPxe;
#endif
#endif

/* These variables define the system cache address space */
PVOID MmSystemCacheStart = (PVOID)MI_SYSTEM_CACHE_START;
PVOID MmSystemCacheEnd;
ULONG_PTR MmSizeOfSystemCacheInPages;
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

/* An event used in Phase 0 before the rest of the system is ready to go */
KEVENT MiTempEvent;

/* All the events used for memory threshold notifications */
PKEVENT MiLowMemoryEvent;
PKEVENT MiHighMemoryEvent;
PKEVENT MiLowPagedPoolEvent;
PKEVENT MiHighPagedPoolEvent;
PKEVENT MiLowNonPagedPoolEvent;
PKEVENT MiHighNonPagedPoolEvent;

/* The actual thresholds themselves, in page numbers */
PFN_NUMBER MmLowMemoryThreshold;
PFN_NUMBER MmHighMemoryThreshold;
PFN_NUMBER MiLowPagedPoolThreshold;
PFN_NUMBER MiHighPagedPoolThreshold;
PFN_NUMBER MiLowNonPagedPoolThreshold;
PFN_NUMBER MiHighNonPagedPoolThreshold;

/*
 * This number determines how many free pages must exist, at minimum, until we
 * start trimming working sets and flushing modified pages to obtain more free
 * pages.
 *
 * This number changes if the system detects that this is a server product
 */
PFN_NUMBER MmMinimumFreePages = 26;

/*
 * This number indicates how many pages we consider to be a low limit of having
 * "plenty" of free memory.
 *
 * It is doubled on systems that have more than 63MB of memory
 */
PFN_NUMBER MmPlentyFreePages = 400;

/* These values store the type of system this is (small, med, large) and if server */
ULONG MmProductType;
MM_SYSTEMSIZE MmSystemSize;

/*
 * These values store the cache working set minimums and maximums, in pages
 *
 * The minimum value is boosted on systems with more than 24MB of RAM, and cut
 * down to only 32 pages on embedded (<24MB RAM) systems.
 *
 * An extra boost of 2MB is given on systems with more than 33MB of RAM.
 */
PFN_NUMBER MmSystemCacheWsMinimum = 288;
PFN_NUMBER MmSystemCacheWsMaximum = 350;

/* FIXME: Move to cache/working set code later */
BOOLEAN MmLargeSystemCache;

/*
 * This value determines in how many fragments/chunks the subsection prototype
 * PTEs should be allocated when mapping a section object. It is configurable in
 * the registry through the MapAllocationFragment parameter.
 *
 * The default is 64KB on systems with more than 1GB of RAM, 32KB on systems with
 * more than 256MB of RAM, and 16KB on systems with less than 256MB of RAM.
 *
 * The maximum it can be set to is 2MB, and the minimum is 4KB.
 */
SIZE_T MmAllocationFragment;

/*
 * These two values track how much virtual memory can be committed, and when
 * expansion should happen.
 */
 // FIXME: They should be moved elsewhere since it's not an "init" setting?
SIZE_T MmTotalCommitLimit;
SIZE_T MmTotalCommitLimitMaximum;

/*
 * These values tune certain user parameters. They have default values set here,
 * as well as in the code, and can be overwritten by registry settings.
 */
SIZE_T MmHeapSegmentReserve = 1 * _1MB;
SIZE_T MmHeapSegmentCommit = 2 * PAGE_SIZE;
SIZE_T MmHeapDeCommitTotalFreeThreshold = 64 * _1KB;
SIZE_T MmHeapDeCommitFreeBlockThreshold = PAGE_SIZE;
SIZE_T MmMinimumStackCommitInBytes = 0;

/* Internal setting used for debugging memory descriptors */
BOOLEAN MiDbgEnableMdDump =
#ifdef _ARM_
TRUE;
#else
FALSE;
#endif

/* Number of memory descriptors in the loader block */
ULONG MiNumberDescriptors = 0;

/* Number of free pages in the loader block */
PFN_NUMBER MiNumberOfFreePages = 0;

/* Timeout value for critical sections (2.5 minutes) */
ULONG MmCritsectTimeoutSeconds = 150; // NT value: 720 * 60 * 60; (30 days)
LARGE_INTEGER MmCriticalSectionTimeout;

//
// Throttling limits for Cc (in pages)
// Above top, we don't throttle
// Above bottom, we throttle depending on the amount of modified pages
// Otherwise, we throttle!
//
ULONG MmThrottleTop;
ULONG MmThrottleBottom;

/* PRIVATE FUNCTIONS **********************************************************/

VOID
NTAPI
MiScanMemoryDescriptors(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Descriptor;
    PFN_NUMBER PageFrameIndex, FreePages = 0;

    /* Loop the memory descriptors */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Flink)
    {
        /* Get the descriptor */
        Descriptor = CONTAINING_RECORD(ListEntry,
                                       MEMORY_ALLOCATION_DESCRIPTOR,
                                       ListEntry);
        DPRINT("MD Type: %lx Base: %lx Count: %lx\n",
            Descriptor->MemoryType, Descriptor->BasePage, Descriptor->PageCount);

        /* Count this descriptor */
        MiNumberDescriptors++;

        /* If this is invisible memory, skip this descriptor */
        if (MiIsMemoryTypeInvisible(Descriptor->MemoryType))
            continue;

        /* Check if this isn't bad memory */
        if (Descriptor->MemoryType != LoaderBad)
        {
            /* Count it in the physical pages */
            MmNumberOfPhysicalPages += (PFN_COUNT)Descriptor->PageCount;
        }

        /* Check if this is the new lowest page */
        if (Descriptor->BasePage < MmLowestPhysicalPage)
        {
            /* Update the lowest page */
            MmLowestPhysicalPage = Descriptor->BasePage;
        }

        /* Check if this is the new highest page */
        PageFrameIndex = Descriptor->BasePage + Descriptor->PageCount;
        if (PageFrameIndex > MmHighestPhysicalPage)
        {
            /* Update the highest page */
            MmHighestPhysicalPage = PageFrameIndex - 1;
        }

        /* Check if this is free memory */
        if (MiIsMemoryTypeFree(Descriptor->MemoryType))
        {
            /* Count it in the free pages */
            MiNumberOfFreePages += Descriptor->PageCount;

            /* Check if this is the largest memory descriptor */
            if (Descriptor->PageCount > FreePages)
            {
                /* Remember it */
                MxFreeDescriptor = Descriptor;
                FreePages = Descriptor->PageCount;
            }
        }
    }

    /* Save original values of the free descriptor, since it'll be
     * altered by early allocations */
    MxOldFreeDescriptor = *MxFreeDescriptor;
}

CODE_SEG("INIT")
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

CODE_SEG("INIT")
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

CODE_SEG("INIT")
VOID
NTAPI
MiInitializeColorTables(VOID)
{
    ULONG i;
    PMMPTE PointerPte, LastPte;
    MMPTE TempPte = ValidKernelPte;

    /* The color table starts after the ARM3 PFN database */
    MmFreePagesByColor[0] = (PMMCOLOR_TABLES)&MmPfnDatabase[MmHighestPhysicalPage + 1];

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
            MI_WRITE_VALID_PTE(PointerPte, TempPte);

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
        MmFreePagesByColor[ZeroedPageList][i].Flink = LIST_HEAD;
        MmFreePagesByColor[ZeroedPageList][i].Blink = (PVOID)LIST_HEAD;
        MmFreePagesByColor[ZeroedPageList][i].Count = 0;
        MmFreePagesByColor[FreePageList][i].Flink = LIST_HEAD;
        MmFreePagesByColor[FreePageList][i].Blink = (PVOID)LIST_HEAD;
        MmFreePagesByColor[FreePageList][i].Count = 0;
    }
}

#ifndef _M_AMD64
CODE_SEG("INIT")
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

CODE_SEG("INIT")
VOID
NTAPI
MiMapPfnDatabase(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PFN_NUMBER FreePage, FreePageCount, PagesLeft, BasePage, PageCount;
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
        PointerPte = MiAddressToPte(&MmPfnDatabase[BasePage]);
        LastPte = MiAddressToPte(((ULONG_PTR)&MmPfnDatabase[BasePage + PageCount]) - 1);
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
                MI_WRITE_VALID_PTE(PointerPte, TempPte);

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

CODE_SEG("INIT")
VOID
NTAPI
MiBuildPfnDatabaseFromPages(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PMMPDE PointerPde;
    PMMPTE PointerPte;
    ULONG i, Count, j;
    PFN_NUMBER PageFrameIndex, StartupPdIndex, PtePageIndex;
    PMMPFN Pfn1, Pfn2;
    ULONG_PTR BaseAddress = 0;

    /* PFN of the startup page directory */
    StartupPdIndex = PFN_FROM_PTE(MiAddressToPde(PDE_BASE));

    /* Start with the first PDE and scan them all */
    PointerPde = MiAddressToPde(NULL);
    Count = PPE_PER_PAGE * PDE_PER_PAGE;
    for (i = 0; i < Count; i++)
    {
        /* Check for valid PDE */
        if (PointerPde->u.Hard.Valid == 1)
        {
            /* Get the PFN from it */
            PageFrameIndex = PFN_FROM_PTE(PointerPde);

            /* Do we want a PFN entry for this page? */
            if (MiIsRegularMemory(LoaderBlock, PageFrameIndex))
            {
                /* Yes we do, set it up */
                Pfn1 = MiGetPfnEntry(PageFrameIndex);
                Pfn1->u4.PteFrame = StartupPdIndex;
                Pfn1->PteAddress = (PMMPTE)PointerPde;
                Pfn1->u2.ShareCount++;
                Pfn1->u3.e2.ReferenceCount = 1;
                Pfn1->u3.e1.PageLocation = ActiveAndValid;
                Pfn1->u3.e1.CacheAttribute = MiNonCached;
#if MI_TRACE_PFNS
                Pfn1->PfnUsage = MI_USAGE_INIT_MEMORY;
                MI_SET_PFN_PROCESS_NAME(Pfn1, "Initial PDE");
#endif
            }
            else
            {
                /* No PFN entry */
                Pfn1 = NULL;
            }

            /* Now get the PTE and scan the pages */
            PointerPte = MiAddressToPte(BaseAddress);
            for (j = 0; j < PTE_PER_PAGE; j++)
            {
                /* Check for a valid PTE */
                if (PointerPte->u.Hard.Valid == 1)
                {
                    /* Increase the shared count of the PFN entry for the PDE */
                    ASSERT(Pfn1 != NULL);
                    Pfn1->u2.ShareCount++;

                    /* Now check if the PTE is valid memory too */
                    PtePageIndex = PFN_FROM_PTE(PointerPte);
                    if (MiIsRegularMemory(LoaderBlock, PtePageIndex))
                    {
                        /*
                         * Only add pages above the end of system code or pages
                         * that are part of nonpaged pool
                         */
                        if ((BaseAddress >= 0xA0000000) ||
                            ((BaseAddress >= (ULONG_PTR)MmNonPagedPoolStart) &&
                             (BaseAddress < (ULONG_PTR)MmNonPagedPoolStart +
                                            MmSizeOfNonPagedPoolInBytes)))
                        {
                            /* Get the PFN entry and make sure it too is valid */
                            Pfn2 = MiGetPfnEntry(PtePageIndex);
                            if ((MmIsAddressValid(Pfn2)) &&
                                (MmIsAddressValid(Pfn2 + 1)))
                            {
                                /* Setup the PFN entry */
                                Pfn2->u4.PteFrame = PageFrameIndex;
                                Pfn2->PteAddress = PointerPte;
                                Pfn2->u2.ShareCount++;
                                Pfn2->u3.e2.ReferenceCount = 1;
                                Pfn2->u3.e1.PageLocation = ActiveAndValid;
                                Pfn2->u3.e1.CacheAttribute = MiNonCached;
#if MI_TRACE_PFNS
                                Pfn2->PfnUsage = MI_USAGE_INIT_MEMORY;
                                MI_SET_PFN_PROCESS_NAME(Pfn2, "Initial PTE");
#endif
                            }
                        }
                    }
                }

                /* Next PTE */
                PointerPte++;
                BaseAddress += PAGE_SIZE;
            }
        }
        else
        {
            /* Next PDE mapped address */
            BaseAddress += PDE_MAPPED_VA;
        }

        /* Next PTE */
        PointerPde++;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiBuildPfnDatabaseZeroPage(VOID)
{
    PMMPFN Pfn1;
    PMMPDE PointerPde;

    /* Grab the lowest page and check if it has no real references */
    Pfn1 = MiGetPfnEntry(MmLowestPhysicalPage);
    if (!(MmLowestPhysicalPage) && !(Pfn1->u3.e2.ReferenceCount))
    {
        /* Make it a bogus page to catch errors */
        PointerPde = MiAddressToPde(0xFFFFFFFF);
        Pfn1->u4.PteFrame = PFN_FROM_PTE(PointerPde);
        Pfn1->PteAddress = (PMMPTE)PointerPde;
        Pfn1->u2.ShareCount++;
        Pfn1->u3.e2.ReferenceCount = 0xFFF0;
        Pfn1->u3.e1.PageLocation = ActiveAndValid;
        Pfn1->u3.e1.CacheAttribute = MiNonCached;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiBuildPfnDatabaseFromLoaderBlock(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextEntry;
    PFN_NUMBER PageCount = 0;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    PFN_NUMBER PageFrameIndex;
    PMMPFN Pfn1;
    PMMPTE PointerPte;
    PMMPDE PointerPde;
    KIRQL OldIrql;

    /* Now loop through the descriptors */
    NextEntry = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextEntry != &LoaderBlock->MemoryDescriptorListHead)
    {
        /* Get the current descriptor */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Read its data */
        PageCount = MdBlock->PageCount;
        PageFrameIndex = MdBlock->BasePage;

        /* Don't allow memory above what the PFN database is mapping */
        if (PageFrameIndex > MmHighestPhysicalPage)
        {
            /* Since they are ordered, everything past here will be larger */
            break;
        }

        /* On the other hand, the end page might be higher up... */
        if ((PageFrameIndex + PageCount) > (MmHighestPhysicalPage + 1))
        {
            /* In which case we'll trim the descriptor to go as high as we can */
            PageCount = MmHighestPhysicalPage + 1 - PageFrameIndex;
            MdBlock->PageCount = PageCount;

            /* But if there's nothing left to trim, we got too high, so quit */
            if (!PageCount) break;
        }

        /* Now check the descriptor type */
        switch (MdBlock->MemoryType)
        {
            /* Check for bad RAM */
            case LoaderBad:

                DPRINT1("You either have specified /BURNMEMORY or damaged RAM modules.\n");
                break;

            /* Check for free RAM */
            case LoaderFree:
            case LoaderLoadedProgram:
            case LoaderFirmwareTemporary:
            case LoaderOsloaderStack:

                /* Get the last page of this descriptor. Note we loop backwards */
                PageFrameIndex += PageCount - 1;
                Pfn1 = MiGetPfnEntry(PageFrameIndex);

                /* Lock the PFN Database */
                OldIrql = MiAcquirePfnLock();
                while (PageCount--)
                {
                    /* If the page really has no references, mark it as free */
                    if (!Pfn1->u3.e2.ReferenceCount)
                    {
                        /* Add it to the free list */
                        Pfn1->u3.e1.CacheAttribute = MiNonCached;
                        MiInsertPageInFreeList(PageFrameIndex);
                    }

                    /* Go to the next page */
                    Pfn1--;
                    PageFrameIndex--;
                }

                /* Release PFN database */
                MiReleasePfnLock(OldIrql);

                /* Done with this block */
                break;

            /* Check for pages that are invisible to us */
            case LoaderFirmwarePermanent:
            case LoaderSpecialMemory:
            case LoaderBBTMemory:

                /* And skip them */
                break;

            default:

                /* Map these pages with the KSEG0 mapping that adds 0x80000000 */
                PointerPte = MiAddressToPte(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
                Pfn1 = MiGetPfnEntry(PageFrameIndex);
                while (PageCount--)
                {
                    /* Check if the page is really unused */
                    PointerPde = MiAddressToPde(KSEG0_BASE + (PageFrameIndex << PAGE_SHIFT));
                    if (!Pfn1->u3.e2.ReferenceCount)
                    {
                        /* Mark it as being in-use */
                        Pfn1->u4.PteFrame = PFN_FROM_PTE(PointerPde);
                        Pfn1->PteAddress = PointerPte;
                        Pfn1->u2.ShareCount++;
                        Pfn1->u3.e2.ReferenceCount = 1;
                        Pfn1->u3.e1.PageLocation = ActiveAndValid;
                        Pfn1->u3.e1.CacheAttribute = MiNonCached;
#if MI_TRACE_PFNS
                        Pfn1->PfnUsage = MI_USAGE_BOOT_DRIVER;
#endif

                        /* Check for RAM disk page */
                        if (MdBlock->MemoryType == LoaderXIPRom)
                        {
                            /* Make it a pseudo-I/O ROM mapping */
                            Pfn1->u1.Flink = 0;
                            Pfn1->u2.ShareCount = 0;
                            Pfn1->u3.e2.ReferenceCount = 0;
                            Pfn1->u3.e1.PageLocation = 0;
                            Pfn1->u3.e1.Rom = 1;
                            Pfn1->u4.InPageError = 0;
                            Pfn1->u3.e1.PrototypePte = 1;
                        }
                    }

                    /* Advance page structures */
                    Pfn1++;
                    PageFrameIndex++;
                    PointerPte++;
                }
                break;
        }

        /* Next descriptor entry */
        NextEntry = MdBlock->ListEntry.Flink;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiBuildPfnDatabaseSelf(VOID)
{
    PMMPTE PointerPte, LastPte;
    PMMPFN Pfn1;

    /* Loop the PFN database page */
    PointerPte = MiAddressToPte(MiGetPfnEntry(MmLowestPhysicalPage));
    LastPte = MiAddressToPte(MiGetPfnEntry(MmHighestPhysicalPage));
    while (PointerPte <= LastPte)
    {
        /* Make sure the page is valid */
        if (PointerPte->u.Hard.Valid == 1)
        {
            /* Get the PFN entry and just mark it referenced */
            Pfn1 = MiGetPfnEntry(PointerPte->u.Hard.PageFrameNumber);
            Pfn1->u2.ShareCount = 1;
            Pfn1->u3.e2.ReferenceCount = 1;
#if MI_TRACE_PFNS
            Pfn1->PfnUsage = MI_USAGE_PFN_DATABASE;
#endif
        }

        /* Next */
        PointerPte++;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiInitializePfnDatabase(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Scan memory and start setting up PFN entries */
    MiBuildPfnDatabaseFromPages(LoaderBlock);

    /* Add the zero page */
    MiBuildPfnDatabaseZeroPage();

    /* Scan the loader block and build the rest of the PFN database */
    MiBuildPfnDatabaseFromLoaderBlock(LoaderBlock);

    /* Finally add the pages for the PFN database itself */
    MiBuildPfnDatabaseSelf();
}
#endif /* !_M_AMD64 */

CODE_SEG("INIT")
VOID
NTAPI
MmFreeLoaderBlock(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY NextMd;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG_PTR i;
    PFN_NUMBER BasePage, LoaderPages;
    PMMPFN Pfn1;
    KIRQL OldIrql;
    PPHYSICAL_MEMORY_RUN Buffer, Entry;

    /* Loop the descriptors in order to count them */
    i = 0;
    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead)
    {
        MdBlock = CONTAINING_RECORD(NextMd,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        i++;
        NextMd = MdBlock->ListEntry.Flink;
    }

    /* Allocate a structure to hold the physical runs */
    Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   i * sizeof(PHYSICAL_MEMORY_RUN),
                                   'lMmM');
    ASSERT(Buffer != NULL);
    Entry = Buffer;

    /* Loop the descriptors again */
    NextMd = LoaderBlock->MemoryDescriptorListHead.Flink;
    while (NextMd != &LoaderBlock->MemoryDescriptorListHead)
    {
        /* Check what kind this was */
        MdBlock = CONTAINING_RECORD(NextMd,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);
        switch (MdBlock->MemoryType)
        {
            /* Registry, NLS, and heap data */
            case LoaderRegistryData:
            case LoaderOsloaderHeap:
            case LoaderNlsData:
                /* Are all a candidate for deletion */
                Entry->BasePage = MdBlock->BasePage;
                Entry->PageCount = MdBlock->PageCount;
                Entry++;

            /* We keep the rest */
            default:
                break;
        }

        /* Move to the next descriptor */
        NextMd = MdBlock->ListEntry.Flink;
    }

    /* Acquire the PFN lock */
    OldIrql = MiAcquirePfnLock();

    /* Loop the runs */
    LoaderPages = 0;
    while (--Entry >= Buffer)
    {
        /* See how many pages are in this run */
        i = Entry->PageCount;
        BasePage = Entry->BasePage;

        /* Loop each page */
        Pfn1 = MiGetPfnEntry(BasePage);
        while (i--)
        {
            /* Check if it has references or is in any kind of list */
            if (!(Pfn1->u3.e2.ReferenceCount) && (!Pfn1->u1.Flink))
            {
                /* Set the new PTE address and put this page into the free list */
                Pfn1->PteAddress = (PMMPTE)(BasePage << PAGE_SHIFT);
                MiInsertPageInFreeList(BasePage);
                LoaderPages++;
            }
            else if (BasePage)
            {
                /* It has a reference, so simply drop it */
                ASSERT(MI_IS_PHYSICAL_ADDRESS(MiPteToAddress(Pfn1->PteAddress)) == FALSE);

                /* Drop a dereference on this page, which should delete it */
                Pfn1->PteAddress->u.Long = 0;
                MI_SET_PFN_DELETED(Pfn1);
                MiDecrementShareCount(Pfn1, BasePage);
                LoaderPages++;
            }

            /* Move to the next page */
            Pfn1++;
            BasePage++;
        }
    }

    /* Release the PFN lock and flush the TLB */
    DPRINT("Loader pages freed: %lx\n", LoaderPages);
    MiReleasePfnLock(OldIrql);
    KeFlushCurrentTb();

    /* Free our run structure */
    ExFreePoolWithTag(Buffer, 'lMmM');
}

CODE_SEG("INIT")
VOID
NTAPI
MiAdjustWorkingSetManagerParameters(IN BOOLEAN Client)
{
    /* This function needs to do more work, for now, we tune page minimums */

    /* Check for a system with around 64MB RAM or more */
    if (MmNumberOfPhysicalPages >= (63 * _1MB) / PAGE_SIZE)
    {
        /* Double the minimum amount of pages we consider for a "plenty free" scenario */
        MmPlentyFreePages *= 2;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
MiNotifyMemoryEvents(VOID)
{
    /* Are we in a low-memory situation? */
    if (MmAvailablePages < MmLowMemoryThreshold)
    {
        /* Clear high, set low  */
        if (KeReadStateEvent(MiHighMemoryEvent)) KeClearEvent(MiHighMemoryEvent);
        if (!KeReadStateEvent(MiLowMemoryEvent)) KeSetEvent(MiLowMemoryEvent, 0, FALSE);
    }
    else if (MmAvailablePages < MmHighMemoryThreshold)
    {
        /* We are in between, clear both */
        if (KeReadStateEvent(MiHighMemoryEvent)) KeClearEvent(MiHighMemoryEvent);
        if (KeReadStateEvent(MiLowMemoryEvent)) KeClearEvent(MiLowMemoryEvent);
    }
    else
    {
        /* Clear low, set high  */
        if (KeReadStateEvent(MiLowMemoryEvent)) KeClearEvent(MiLowMemoryEvent);
        if (!KeReadStateEvent(MiHighMemoryEvent)) KeSetEvent(MiHighMemoryEvent, 0, FALSE);
    }
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
MiCreateMemoryEvent(IN PUNICODE_STRING Name,
                    OUT PKEVENT *Event)
{
    PACL Dacl;
    HANDLE EventHandle;
    ULONG DaclLength;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_DESCRIPTOR SecurityDescriptor;

    /* Create the SD */
    Status = RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                         SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status)) return Status;

    /* One ACL with 3 ACEs, containing each one SID */
    DaclLength = sizeof(ACL) +
                 3 * sizeof(ACCESS_ALLOWED_ACE) +
                 RtlLengthSid(SeLocalSystemSid) +
                 RtlLengthSid(SeAliasAdminsSid) +
                 RtlLengthSid(SeWorldSid);

    /* Allocate space for the DACL */
    Dacl = ExAllocatePoolWithTag(PagedPool, DaclLength, TAG_DACL);
    if (!Dacl) return STATUS_INSUFFICIENT_RESOURCES;

    /* Setup the ACL inside it */
    Status = RtlCreateAcl(Dacl, DaclLength, ACL_REVISION);
    if (!NT_SUCCESS(Status)) goto CleanUp;

    /* Add query rights for everyone */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    SYNCHRONIZE | EVENT_QUERY_STATE | READ_CONTROL,
                                    SeWorldSid);
    if (!NT_SUCCESS(Status)) goto CleanUp;

    /* Full rights for the admin */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    EVENT_ALL_ACCESS,
                                    SeAliasAdminsSid);
    if (!NT_SUCCESS(Status)) goto CleanUp;

    /* As well as full rights for the system */
    Status = RtlAddAccessAllowedAce(Dacl,
                                    ACL_REVISION,
                                    EVENT_ALL_ACCESS,
                                    SeLocalSystemSid);
    if (!NT_SUCCESS(Status)) goto CleanUp;

    /* Set this DACL inside the SD */
    Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor,
                                          TRUE,
                                          Dacl,
                                          FALSE);
    if (!NT_SUCCESS(Status)) goto CleanUp;

    /* Setup the event attributes, making sure it's a permanent one */
    InitializeObjectAttributes(&ObjectAttributes,
                               Name,
                               OBJ_KERNEL_HANDLE | OBJ_PERMANENT,
                               NULL,
                               &SecurityDescriptor);

    /* Create the event */
    Status = ZwCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           &ObjectAttributes,
                           NotificationEvent,
                           FALSE);
CleanUp:
    /* Free the DACL */
    ExFreePoolWithTag(Dacl, TAG_DACL);

    /* Check if this is the success path */
    if (NT_SUCCESS(Status))
    {
        /* Add a reference to the object, then close the handle we had */
        Status = ObReferenceObjectByHandle(EventHandle,
                                           EVENT_MODIFY_STATE,
                                           ExEventObjectType,
                                           KernelMode,
                                           (PVOID*)Event,
                                           NULL);
        ZwClose (EventHandle);
    }

    /* Return status */
    return Status;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
MiInitializeMemoryEvents(VOID)
{
    UNICODE_STRING LowString = RTL_CONSTANT_STRING(L"\\KernelObjects\\LowMemoryCondition");
    UNICODE_STRING HighString = RTL_CONSTANT_STRING(L"\\KernelObjects\\HighMemoryCondition");
    UNICODE_STRING LowPagedPoolString = RTL_CONSTANT_STRING(L"\\KernelObjects\\LowPagedPoolCondition");
    UNICODE_STRING HighPagedPoolString = RTL_CONSTANT_STRING(L"\\KernelObjects\\HighPagedPoolCondition");
    UNICODE_STRING LowNonPagedPoolString = RTL_CONSTANT_STRING(L"\\KernelObjects\\LowNonPagedPoolCondition");
    UNICODE_STRING HighNonPagedPoolString = RTL_CONSTANT_STRING(L"\\KernelObjects\\HighNonPagedPoolCondition");
    NTSTATUS Status;

    /* Check if we have a registry setting */
    if (MmLowMemoryThreshold)
    {
        /* Convert it to pages */
        MmLowMemoryThreshold *= (_1MB / PAGE_SIZE);
    }
    else
    {
        /* The low memory threshold is hit when we don't consider that we have "plenty" of free pages anymore */
        MmLowMemoryThreshold = MmPlentyFreePages;

        /* More than one GB of memory? */
        if (MmNumberOfPhysicalPages > 0x40000)
        {
            /* Start at 32MB, and add another 16MB for each GB */
            MmLowMemoryThreshold = (32 * _1MB) / PAGE_SIZE;
            MmLowMemoryThreshold += ((MmNumberOfPhysicalPages - 0x40000) >> 7);
        }
        else if (MmNumberOfPhysicalPages > 0x8000)
        {
            /* For systems with > 128MB RAM, add another 4MB for each 128MB */
            MmLowMemoryThreshold += ((MmNumberOfPhysicalPages - 0x8000) >> 5);
        }

        /* Don't let the minimum threshold go past 64MB */
        MmLowMemoryThreshold = min(MmLowMemoryThreshold, (64 * _1MB) / PAGE_SIZE);
    }

    /* Check if we have a registry setting */
    if (MmHighMemoryThreshold)
    {
        /* Convert it into pages */
        MmHighMemoryThreshold *= (_1MB / PAGE_SIZE);
    }
    else
    {
        /* Otherwise, the default is three times the low memory threshold */
        MmHighMemoryThreshold = 3 * MmLowMemoryThreshold;
        ASSERT(MmHighMemoryThreshold > MmLowMemoryThreshold);
    }

    /* Make sure high threshold is actually higher than the low */
    MmHighMemoryThreshold = max(MmHighMemoryThreshold, MmLowMemoryThreshold);

    /* Create the memory events for all the thresholds */
    Status = MiCreateMemoryEvent(&LowString, &MiLowMemoryEvent);
    if (!NT_SUCCESS(Status)) return FALSE;
    Status = MiCreateMemoryEvent(&HighString, &MiHighMemoryEvent);
    if (!NT_SUCCESS(Status)) return FALSE;
    Status = MiCreateMemoryEvent(&LowPagedPoolString, &MiLowPagedPoolEvent);
    if (!NT_SUCCESS(Status)) return FALSE;
    Status = MiCreateMemoryEvent(&HighPagedPoolString, &MiHighPagedPoolEvent);
    if (!NT_SUCCESS(Status)) return FALSE;
    Status = MiCreateMemoryEvent(&LowNonPagedPoolString, &MiLowNonPagedPoolEvent);
    if (!NT_SUCCESS(Status)) return FALSE;
    Status = MiCreateMemoryEvent(&HighNonPagedPoolString, &MiHighNonPagedPoolEvent);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Now setup the pool events */
    MiInitializePoolEvents();

    /* Set the initial event state */
    MiNotifyMemoryEvents();
    return TRUE;
}

CODE_SEG("INIT")
VOID
NTAPI
MiAddHalIoMappings(VOID)
{
    PVOID BaseAddress;
    PMMPDE PointerPde, LastPde;
    PMMPTE PointerPte;
    ULONG j;
    PFN_NUMBER PageFrameIndex;

    /* HAL Heap address -- should be on a PDE boundary */
    BaseAddress = (PVOID)MM_HAL_VA_START;
    ASSERT(MiAddressToPteOffset(BaseAddress) == 0);

    /* Check how many PDEs the heap has */
    PointerPde = MiAddressToPde(BaseAddress);
    LastPde = MiAddressToPde((PVOID)MM_HAL_VA_END);

    while (PointerPde <= LastPde)
    {
        /* Does the HAL own this mapping? */
        if ((PointerPde->u.Hard.Valid == 1) &&
            (MI_IS_PAGE_LARGE(PointerPde) == FALSE))
        {
            /* Get the PTE for it and scan each page */
            PointerPte = MiAddressToPte(BaseAddress);
            for (j = 0; j < PTE_PER_PAGE; j++)
            {
                /* Does the HAL own this page? */
                if (PointerPte->u.Hard.Valid == 1)
                {
                    /* Is the HAL using it for device or I/O mapped memory? */
                    PageFrameIndex = PFN_FROM_PTE(PointerPte);
                    if (!MiGetPfnEntry(PageFrameIndex))
                    {
                        /* FIXME: For PAT, we need to track I/O cache attributes for coherency */
                        DPRINT1("HAL I/O Mapping at %p is unsafe\n", BaseAddress);
                    }
                }

                /* Move to the next page */
                BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + PAGE_SIZE);
                PointerPte++;
            }
        }
        else
        {
            /* Move to the next address */
            BaseAddress = (PVOID)((ULONG_PTR)BaseAddress + PDE_MAPPED_VA);
        }

        /* Move to the next PDE */
        PointerPde++;
    }
}

VOID
NTAPI
MmDumpArmPfnDatabase(IN BOOLEAN StatusOnly)
{
    ULONG i;
    PMMPFN Pfn1;
    PCHAR Consumer = "Unknown";
    KIRQL OldIrql;
    ULONG ActivePages = 0, FreePages = 0, OtherPages = 0;
#if MI_TRACE_PFNS
    ULONG UsageBucket[MI_USAGE_FREE_PAGE + 1] = {0};
    PCHAR MI_USAGE_TEXT[MI_USAGE_FREE_PAGE + 1] =
    {
        "Not set",
        "Paged Pool",
        "Nonpaged Pool",
        "Nonpaged Pool Ex",
        "Kernel Stack",
        "Kernel Stack Ex",
        "System PTE",
        "VAD",
        "PEB/TEB",
        "Section",
        "Page Table",
        "Page Directory",
        "Old Page Table",
        "Driver Page",
        "Contiguous Alloc",
        "MDL",
        "Demand Zero",
        "Zero Loop",
        "Cache",
        "PFN Database",
        "Boot Driver",
        "Initial Memory",
        "Free Page"
    };
#endif
    //
    // Loop the PFN database
    //
    KeRaiseIrql(HIGH_LEVEL, &OldIrql);
    for (i = 0; i <= MmHighestPhysicalPage; i++)
    {
        Pfn1 = MiGetPfnEntry(i);
        if (!Pfn1) continue;
#if MI_TRACE_PFNS
        ASSERT(Pfn1->PfnUsage <= MI_USAGE_FREE_PAGE);
#endif
        //
        // Get the page location
        //
        switch (Pfn1->u3.e1.PageLocation)
        {
            case ActiveAndValid:

                Consumer = "Active and Valid";
                ActivePages++;
                break;

            case ZeroedPageList:

                Consumer = "Zero Page List";
                FreePages++;
                break;//continue;

            case FreePageList:

                Consumer = "Free Page List";
                FreePages++;
                break;//continue;

            default:

                Consumer = "Other (ASSERT!)";
                OtherPages++;
                break;
        }

#if MI_TRACE_PFNS
        /* Add into bucket */
        UsageBucket[Pfn1->PfnUsage]++;
#endif

        //
        // Pretty-print the page
        //
        if (!StatusOnly)
        DbgPrint("0x%08p:\t%20s\t(%04d.%04d)\t[%16s - %16s]\n",
                 i << PAGE_SHIFT,
                 Consumer,
                 Pfn1->u3.e2.ReferenceCount,
                 Pfn1->u2.ShareCount == LIST_HEAD ? 0xFFFF : Pfn1->u2.ShareCount,
#if MI_TRACE_PFNS
                 MI_USAGE_TEXT[Pfn1->PfnUsage],
                 Pfn1->ProcessName);
#else
                 "Page tracking",
                 "is disabled");
#endif
    }

    DbgPrint("Active:               %5d pages\t[%6d KB]\n", ActivePages,  (ActivePages    << PAGE_SHIFT) / 1024);
    DbgPrint("Free:                 %5d pages\t[%6d KB]\n", FreePages,    (FreePages      << PAGE_SHIFT) / 1024);
    DbgPrint("Other:                %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    DbgPrint("-----------------------------------------\n");
#if MI_TRACE_PFNS
    OtherPages = UsageBucket[MI_USAGE_BOOT_DRIVER];
    DbgPrint("Boot Images:          %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_DRIVER_PAGE];
    DbgPrint("System Drivers:       %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_PFN_DATABASE];
    DbgPrint("PFN Database:         %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_PAGE_TABLE] + UsageBucket[MI_USAGE_PAGE_DIRECTORY] + UsageBucket[MI_USAGE_LEGACY_PAGE_DIRECTORY];
    DbgPrint("Page Tables:          %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_SYSTEM_PTE];
    DbgPrint("System PTEs:          %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_VAD];
    DbgPrint("VADs:                 %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_CONTINOUS_ALLOCATION];
    DbgPrint("Continuous Allocs:    %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_MDL];
    DbgPrint("MDLs:                 %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_NONPAGED_POOL] + UsageBucket[MI_USAGE_NONPAGED_POOL_EXPANSION];
    DbgPrint("NonPaged Pool:        %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_PAGED_POOL];
    DbgPrint("Paged Pool:           %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_DEMAND_ZERO];
    DbgPrint("Demand Zero:          %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_ZERO_LOOP];
    DbgPrint("Zero Loop:            %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_PEB_TEB];
    DbgPrint("PEB/TEB:              %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_KERNEL_STACK] + UsageBucket[MI_USAGE_KERNEL_STACK_EXPANSION];
    DbgPrint("Kernel Stack:         %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_INIT_MEMORY];
    DbgPrint("Init Memory:          %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_SECTION];
    DbgPrint("Sections:             %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_CACHE];
    DbgPrint("Cache:                %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
    OtherPages = UsageBucket[MI_USAGE_FREE_PAGE];
    DbgPrint("Free:                 %5d pages\t[%6d KB]\n", OtherPages,   (OtherPages     << PAGE_SHIFT) / 1024);
#endif
    KeLowerIrql(OldIrql);
}

CODE_SEG("INIT")
PPHYSICAL_MEMORY_DESCRIPTOR
NTAPI
MmInitializeMemoryLimits(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
                         IN PBOOLEAN IncludeType)
{
    PLIST_ENTRY NextEntry;
    ULONG Run = 0, InitialRuns;
    PFN_NUMBER NextPage = -1, PageCount = 0;
    PPHYSICAL_MEMORY_DESCRIPTOR Buffer, NewBuffer;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;

    //
    // Start with the maximum we might need
    //
    InitialRuns = MiNumberDescriptors;

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
            ExFreePoolWithTag(Buffer, 'lMmM');

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

CODE_SEG("INIT")
VOID
NTAPI
MiBuildPagedPool(VOID)
{
    PMMPTE PointerPte;
    PMMPDE PointerPde;
    MMPDE TempPde = ValidKernelPde;
    PFN_NUMBER PageFrameIndex;
    KIRQL OldIrql;
    SIZE_T Size, NumberOfPages, NumberOfPdes;
    ULONG BitMapSize;
#if (_MI_PAGING_LEVELS >= 3)
    MMPPE TempPpe = ValidKernelPpe;
    PMMPPE PointerPpe;
#elif (_MI_PAGING_LEVELS == 2)
    MMPTE TempPte = ValidKernelPte;

    //
    // Get the page frame number for the system page directory
    //
    PointerPte = MiAddressToPte(PDE_BASE);
    ASSERT(PPE_PER_PAGE == 1);
    MmSystemPageDirectory[0] = PFN_FROM_PTE(PointerPte);

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
    ASSERT(PPE_PER_PAGE == 1);
    TempPte.u.Hard.PageFrameNumber = MmSystemPageDirectory[0];
    MI_WRITE_VALID_PTE(PointerPte, TempPte);
#endif

#ifdef _M_IX86
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
#endif // _M_IX86

    //
    // Get the size in pages and make sure paged pool is at least 32MB.
    //
    Size = MmSizeOfPagedPoolInBytes;
    if (Size < MI_MIN_INIT_PAGED_POOLSIZE) Size = MI_MIN_INIT_PAGED_POOLSIZE;
    NumberOfPages = BYTES_TO_PAGES(Size);

    //
    // Now check how many PDEs will be required for these many pages.
    //
    NumberOfPdes = (NumberOfPages + (PTE_PER_PAGE - 1)) / PTE_PER_PAGE;

    //
    // Recompute the PDE-aligned size of the paged pool, in bytes and pages.
    //
    MmSizeOfPagedPoolInBytes = NumberOfPdes * PTE_PER_PAGE * PAGE_SIZE;
    MmSizeOfPagedPoolInPages = MmSizeOfPagedPoolInBytes >> PAGE_SHIFT;

#ifdef _M_IX86
    //
    // Let's be really sure this doesn't overflow into nonpaged system VA
    //
    ASSERT((MmSizeOfPagedPoolInBytes + (ULONG_PTR)MmPagedPoolStart) <=
           (ULONG_PTR)MmNonPagedSystemStart);
#endif // _M_IX86

    //
    // This is where paged pool ends
    //
    MmPagedPoolEnd = (PVOID)(((ULONG_PTR)MmPagedPoolStart +
                              MmSizeOfPagedPoolInBytes) - 1);

    //
    // Lock the PFN database
    //
    OldIrql = MiAcquirePfnLock();

#if (_MI_PAGING_LEVELS >= 3)
    /* On these systems, there's no double-mapping, so instead, the PPEs
     * are setup to span the entire paged pool area, so there's no need for the
     * system PD */
    for (PointerPpe = MiAddressToPpe(MmPagedPoolStart);
         PointerPpe <= MiAddressToPpe(MmPagedPoolEnd);
         PointerPpe++)
    {
        /* Check if the PPE is already valid */
        if (!PointerPpe->u.Hard.Valid)
        {
            /* It is not, so map a fresh zeroed page */
            TempPpe.u.Hard.PageFrameNumber = MiRemoveZeroPage(0);
            MI_WRITE_VALID_PPE(PointerPpe, TempPpe);
            MiInitializePfnForOtherProcess(TempPpe.u.Hard.PageFrameNumber,
                                           (PMMPTE)PointerPpe,
                                           PFN_FROM_PTE(MiAddressToPte(PointerPpe)));
        }
    }
#endif

    //
    // So now get the PDE for paged pool and zero it out
    //
    PointerPde = MiAddressToPde(MmPagedPoolStart);
    RtlZeroMemory(PointerPde,
                  (1 + MiAddressToPde(MmPagedPoolEnd) - PointerPde) * sizeof(MMPDE));

    //
    // Next, get the first and last PTE
    //
    PointerPte = MiAddressToPte(MmPagedPoolStart);
    MmPagedPoolInfo.FirstPteForPagedPool = PointerPte;
    MmPagedPoolInfo.LastPteForPagedPool = MiAddressToPte(MmPagedPoolEnd);

    /* Allocate a page and map the first paged pool PDE */
    MI_SET_USAGE(MI_USAGE_PAGED_POOL);
    MI_SET_PROCESS2("Kernel");
    PageFrameIndex = MiRemoveZeroPage(0);
    TempPde.u.Hard.PageFrameNumber = PageFrameIndex;
    MI_WRITE_VALID_PDE(PointerPde, TempPde);
#if (_MI_PAGING_LEVELS >= 3)
    /* Use the PPE of MmPagedPoolStart that was setup above */
//    Bla = PFN_FROM_PTE(PpeAddress(MmPagedPool...));

    /* Initialize the PFN entry for it */
    MiInitializePfnForOtherProcess(PageFrameIndex,
                                   (PMMPTE)PointerPde,
                                   PFN_FROM_PTE(MiAddressToPpe(MmPagedPoolStart)));
#else
    /* Do it this way */
//    Bla = MmSystemPageDirectory[(PointerPde - (PMMPTE)PDE_BASE) / PDE_PER_PAGE]

    /* Initialize the PFN entry for it */
    MiInitializePfnForOtherProcess(PageFrameIndex,
                                   (PMMPTE)PointerPde,
                                   MmSystemPageDirectory[(PointerPde - (PMMPDE)PDE_BASE) / PDE_PER_PAGE]);
#endif

    //
    // Release the PFN database lock
    //
    MiReleasePfnLock(OldIrql);

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
    NumberOfPages = NumberOfPdes * PTE_PER_PAGE;
    ASSERT(NumberOfPages == MmSizeOfPagedPoolInPages);
    BitMapSize = (ULONG)NumberOfPages;
    Size = sizeof(RTL_BITMAP) + (((BitMapSize + 31) / 32) * sizeof(ULONG));

    //
    // Allocate the allocation bitmap, which tells us which regions have not yet
    // been mapped into memory
    //
    MmPagedPoolInfo.PagedPoolAllocationMap = ExAllocatePoolWithTag(NonPagedPool,
                                                                   Size,
                                                                   TAG_MM);
    ASSERT(MmPagedPoolInfo.PagedPoolAllocationMap);

    //
    // Initialize it such that at first, only the first page's worth of PTEs is
    // marked as allocated (incidentially, the first PDE we allocated earlier).
    //
    RtlInitializeBitMap(MmPagedPoolInfo.PagedPoolAllocationMap,
                        (PULONG)(MmPagedPoolInfo.PagedPoolAllocationMap + 1),
                        BitMapSize);
    RtlSetAllBits(MmPagedPoolInfo.PagedPoolAllocationMap);
    RtlClearBits(MmPagedPoolInfo.PagedPoolAllocationMap, 0, PTE_PER_PAGE);

    //
    // We have a second bitmap, which keeps track of where allocations end.
    // Given the allocation bitmap and a base address, we can therefore figure
    // out which page is the last page of that allocation, and thus how big the
    // entire allocation is.
    //
    MmPagedPoolInfo.EndOfPagedPoolBitmap = ExAllocatePoolWithTag(NonPagedPool,
                                                                 Size,
                                                                 TAG_MM);
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

    /* Initialize special pool */
    MiInitializeSpecialPool();

    /* Default low threshold of 30MB or one fifth of paged pool */
    MiLowPagedPoolThreshold = (30 * _1MB) >> PAGE_SHIFT;
    MiLowPagedPoolThreshold = min(MiLowPagedPoolThreshold, Size / 5);

    /* Default high threshold of 60MB or 25% */
    MiHighPagedPoolThreshold = (60 * _1MB) >> PAGE_SHIFT;
    MiHighPagedPoolThreshold = min(MiHighPagedPoolThreshold, (Size * 2) / 5);
    ASSERT(MiLowPagedPoolThreshold < MiHighPagedPoolThreshold);

    /* Setup the global session space */
    MiInitializeSystemSpaceMap(NULL);
}

CODE_SEG("INIT")
VOID
NTAPI
MiDbgDumpMemoryDescriptors(VOID)
{
    PLIST_ENTRY NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR Md;
    PFN_NUMBER TotalPages = 0;
    PCHAR
    MemType[] =
    {
        "ExceptionBlock    ",
        "SystemBlock       ",
        "Free              ",
        "Bad               ",
        "LoadedProgram     ",
        "FirmwareTemporary ",
        "FirmwarePermanent ",
        "OsloaderHeap      ",
        "OsloaderStack     ",
        "SystemCode        ",
        "HalCode           ",
        "BootDriver        ",
        "ConsoleInDriver   ",
        "ConsoleOutDriver  ",
        "StartupDpcStack   ",
        "StartupKernelStack",
        "StartupPanicStack ",
        "StartupPcrPage    ",
        "StartupPdrPage    ",
        "RegistryData      ",
        "MemoryData        ",
        "NlsData           ",
        "SpecialMemory     ",
        "BBTMemory         ",
        "LoaderReserve     ",
        "LoaderXIPRom      "
    };

    DPRINT1("Base\t\tLength\t\tType\n");
    for (NextEntry = KeLoaderBlock->MemoryDescriptorListHead.Flink;
         NextEntry != &KeLoaderBlock->MemoryDescriptorListHead;
         NextEntry = NextEntry->Flink)
    {
        Md = CONTAINING_RECORD(NextEntry, MEMORY_ALLOCATION_DESCRIPTOR, ListEntry);
        DPRINT1("%08lX\t%08lX\t%s\n", Md->BasePage, Md->PageCount, MemType[Md->MemoryType]);
        TotalPages += Md->PageCount;
    }

    DPRINT1("Total: %08lX (%lu MB)\n", (ULONG)TotalPages, (ULONG)(TotalPages * PAGE_SIZE) / 1024 / 1024);
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
MmArmInitSystem(IN ULONG Phase,
                IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG i;
    BOOLEAN IncludeType[LoaderMaximum];
    PVOID Bitmap;
    PPHYSICAL_MEMORY_RUN Run;
    PFN_NUMBER PageCount;
#if DBG
    ULONG j;
    PMMPTE PointerPte, TestPte;
    MMPTE TempPte;
#endif

    /* Dump memory descriptors */
    if (MiDbgEnableMdDump) MiDbgDumpMemoryDescriptors();

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
        /* Count physical pages on the system */
        MiScanMemoryDescriptors(LoaderBlock);

        /* Initialize the phase 0 temporary event */
        KeInitializeEvent(&MiTempEvent, NotificationEvent, FALSE);

        /* Set all the events to use the temporary event for now */
        MiLowMemoryEvent = &MiTempEvent;
        MiHighMemoryEvent = &MiTempEvent;
        MiLowPagedPoolEvent = &MiTempEvent;
        MiHighPagedPoolEvent = &MiTempEvent;
        MiLowNonPagedPoolEvent = &MiTempEvent;
        MiHighNonPagedPoolEvent = &MiTempEvent;

        //
        // Default throttling limits for Cc
        // May be ajusted later on depending on system type
        //
        MmThrottleTop = 450;
        MmThrottleBottom = 127;

        //
        // Define the basic user vs. kernel address space separation
        //
        MmSystemRangeStart = (PVOID)MI_DEFAULT_SYSTEM_RANGE_START;
        MmUserProbeAddress = (ULONG_PTR)MI_USER_PROBE_ADDRESS;
        MmHighestUserAddress = (PVOID)MI_HIGHEST_USER_ADDRESS;

        /* Highest PTE and PDE based on the addresses above */
        MiHighestUserPte = MiAddressToPte(MmHighestUserAddress);
        MiHighestUserPde = MiAddressToPde(MmHighestUserAddress);
#if (_MI_PAGING_LEVELS >= 3)
        MiHighestUserPpe = MiAddressToPpe(MmHighestUserAddress);
#if (_MI_PAGING_LEVELS >= 4)
        MiHighestUserPxe = MiAddressToPxe(MmHighestUserAddress);
#endif
#endif
        //
        // Get the size of the boot loader's image allocations and then round
        // that region up to a PDE size, so that any PDEs we might create for
        // whatever follows are separate from the PDEs that boot loader might've
        // already created (and later, we can blow all that away if we want to).
        //
        MmBootImageSize = KeLoaderBlock->Extension->LoaderPagesSpanned;
        MmBootImageSize *= PAGE_SIZE;
        MmBootImageSize = (MmBootImageSize + PDE_MAPPED_VA - 1) & ~(PDE_MAPPED_VA - 1);
        ASSERT((MmBootImageSize % PDE_MAPPED_VA) == 0);

        /* Initialize session space address layout */
        MiInitializeSessionSpaceLayout();

        /* Set the based section highest address */
        MmHighSectionBase = (PVOID)((ULONG_PTR)MmHighestUserAddress - 0x800000);

        /* Loop all 8 standby lists */
        for (i = 0; i < 8; i++)
        {
            /* Initialize them */
            MmStandbyPageListByPriority[i].Total = 0;
            MmStandbyPageListByPriority[i].ListName = StandbyPageList;
            MmStandbyPageListByPriority[i].Flink = MM_EMPTY_LIST;
            MmStandbyPageListByPriority[i].Blink = MM_EMPTY_LIST;
        }

        /* Initialize the user mode image list */
        InitializeListHead(&MmLoadedUserImageList);

        /* Initalize the Working set list */
        InitializeListHead(&MmWorkingSetExpansionHead);

        /* Initialize critical section timeout value (relative time is negative) */
        MmCriticalSectionTimeout.QuadPart = MmCritsectTimeoutSeconds * (-10000000LL);

        /* Initialize the paged pool mutex and the section commit mutex */
        KeInitializeGuardedMutex(&MmPagedPoolMutex);
        KeInitializeGuardedMutex(&MmSectionCommitMutex);
        KeInitializeGuardedMutex(&MmSectionBasedMutex);

        /* Initialize the Loader Lock */
        KeInitializeMutant(&MmSystemLoadLock, FALSE);

        /* Set up the zero page event */
        KeInitializeEvent(&MmZeroingPageEvent, NotificationEvent, FALSE);

        /* Initialize the dead stack S-LIST */
        InitializeSListHead(&MmDeadStackSListHead);

        //
        // Check if this is a machine with less than 19MB of RAM
        //
        PageCount = MmNumberOfPhysicalPages;
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
            // Use the default
            //
            MmNumberOfSystemPtes = 11000;
            if (PageCount > MI_MIN_PAGES_FOR_SYSPTE_BOOST)
            {
                //
                // Double the amount of system PTEs
                //
                MmNumberOfSystemPtes <<= 1;
            }
            if (PageCount > MI_MIN_PAGES_FOR_SYSPTE_BOOST_BOOST)
            {
                //
                // Double the amount of system PTEs
                //
                MmNumberOfSystemPtes <<= 1;
            }
            if (MmSpecialPoolTag != 0 && MmSpecialPoolTag != -1)
            {
                //
                // Add some extra PTEs for special pool
                //
                MmNumberOfSystemPtes += 0x6000;
            }
        }

        DPRINT("System PTE count has been tuned to %lu (%lu bytes)\n",
               MmNumberOfSystemPtes, MmNumberOfSystemPtes * PAGE_SIZE);

        /* Check if no values are set for the heap limits */
        if (MmHeapSegmentReserve == 0)
        {
            MmHeapSegmentReserve = 2 * _1MB;
        }

        if (MmHeapSegmentCommit == 0)
        {
            MmHeapSegmentCommit = 2 * PAGE_SIZE;
        }

        if (MmHeapDeCommitTotalFreeThreshold == 0)
        {
            MmHeapDeCommitTotalFreeThreshold = 64 * _1KB;
        }

        if (MmHeapDeCommitFreeBlockThreshold == 0)
        {
            MmHeapDeCommitFreeBlockThreshold = PAGE_SIZE;
        }

        /* Initialize the working set lock */
        ExInitializePushLock(&MmSystemCacheWs.WorkingSetMutex);

        /* Set commit limit */
        MmTotalCommitLimit = (2 * _1GB) >> PAGE_SHIFT;
        MmTotalCommitLimitMaximum = MmTotalCommitLimit;

        /* Has the allocation fragment been setup? */
        if (!MmAllocationFragment)
        {
            /* Use the default value */
            MmAllocationFragment = MI_ALLOCATION_FRAGMENT;
            if (PageCount < ((256 * _1MB) / PAGE_SIZE))
            {
                /* On memory systems with less than 256MB, divide by 4 */
                MmAllocationFragment = MI_ALLOCATION_FRAGMENT / 4;
            }
            else if (PageCount < (_1GB / PAGE_SIZE))
            {
                /* On systems with less than 1GB, divide by 2 */
                MmAllocationFragment = MI_ALLOCATION_FRAGMENT / 2;
            }
        }
        else
        {
            /* Convert from 1KB fragments to pages */
            MmAllocationFragment *= _1KB;
            MmAllocationFragment = ROUND_TO_PAGES(MmAllocationFragment);

            /* Don't let it past the maximum */
            MmAllocationFragment = min(MmAllocationFragment,
                                       MI_MAX_ALLOCATION_FRAGMENT);

            /* Don't let it too small either */
            MmAllocationFragment = max(MmAllocationFragment,
                                       MI_MIN_ALLOCATION_FRAGMENT);
        }

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

        /* Compute color information (L2 cache-separated paging lists) */
        MiComputeColorInformation();

        // Calculate the number of bytes for the PFN database
        // then add the color tables and convert to pages
        MxPfnAllocation = (MmHighestPhysicalPage + 1) * sizeof(MMPFN);
        MxPfnAllocation += (MmSecondaryColors * sizeof(MMCOLOR_TABLES) * 2);
        MxPfnAllocation >>= PAGE_SHIFT;

        // We have to add one to the count here, because in the process of
        // shifting down to the page size, we actually ended up getting the
        // lower aligned size (so say, 0x5FFFF bytes is now 0x5F pages).
        // Later on, we'll shift this number back into bytes, which would cause
        // us to end up with only 0x5F000 bytes -- when we actually want to have
        // 0x60000 bytes.
        MxPfnAllocation++;

        /* Initialize the platform-specific parts */
        MiInitMachineDependent(LoaderBlock);

#if DBG
        /* Prototype PTEs are assumed to be in paged pool, so check if the math works */
        PointerPte = (PMMPTE)MmPagedPoolStart;
        MI_MAKE_PROTOTYPE_PTE(&TempPte, PointerPte);
        TestPte = MiProtoPteToPte(&TempPte);
        ASSERT(PointerPte == TestPte);

        /* Try the last nonpaged pool address */
        PointerPte = (PMMPTE)MI_NONPAGED_POOL_END;
        MI_MAKE_PROTOTYPE_PTE(&TempPte, PointerPte);
        TestPte = MiProtoPteToPte(&TempPte);
        ASSERT(PointerPte == TestPte);

        /* Try a bunch of random addresses near the end of the address space */
        PointerPte = (PMMPTE)((ULONG_PTR)MI_HIGHEST_SYSTEM_ADDRESS - 0x37FFF);
        for (j = 0; j < 20; j += 1)
        {
            MI_MAKE_PROTOTYPE_PTE(&TempPte, PointerPte);
            TestPte = MiProtoPteToPte(&TempPte);
            ASSERT(PointerPte == TestPte);
            PointerPte++;
        }

        /* Subsection PTEs are always in nonpaged pool, pick a random address to try */
        PointerPte = (PMMPTE)((ULONG_PTR)MmNonPagedPoolStart + (MmSizeOfNonPagedPoolInBytes / 2));
        MI_MAKE_SUBSECTION_PTE(&TempPte, PointerPte);
        TestPte = MiSubsectionPteToSubsection(&TempPte);
        ASSERT(PointerPte == TestPte);
#endif

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
                                       TAG_MM);
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
                            (ULONG)MmHighestPhysicalPage + 1);
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
                RtlSetBits(&MiPfnBitMap, (ULONG)Run->BasePage, (ULONG)Run->PageCount);
            }
        }

        /* Look for large page cache entries that need caching */
        MiSyncCachedRanges();

        /* Loop for HAL Heap I/O device mappings that need coherency tracking */
        MiAddHalIoMappings();

        /* Set the initial resident page count */
        MmResidentAvailablePages = MmAvailablePages - 32;

        /* Initialize large page structures on PAE/x64, and MmProcessList on x86 */
        MiInitializeLargePageSupport();

        /* Check if the registry says any drivers should be loaded with large pages */
        MiInitializeDriverLargePageList();

        /* Relocate the boot drivers into system PTE space and fixup their PFNs */
        MiReloadBootLoadedDrivers(LoaderBlock);

        /* FIXME: Call out into Driver Verifier for initialization  */

        /* Check how many pages the system has */
        if (MmNumberOfPhysicalPages <= ((13 * _1MB) / PAGE_SIZE))
        {
            /* Set small system */
            MmSystemSize = MmSmallSystem;
            MmMaximumDeadKernelStacks = 0;
        }
        else if (MmNumberOfPhysicalPages <= ((19 * _1MB) / PAGE_SIZE))
        {
            /* Set small system and add 100 pages for the cache */
            MmSystemSize = MmSmallSystem;
            MmSystemCacheWsMinimum += 100;
            MmMaximumDeadKernelStacks = 2;
        }
        else
        {
            /* Set medium system and add 400 pages for the cache */
            MmSystemSize = MmMediumSystem;
            MmSystemCacheWsMinimum += 400;
            MmMaximumDeadKernelStacks = 5;
        }

        /* Check for less than 24MB */
        if (MmNumberOfPhysicalPages < ((24 * _1MB) / PAGE_SIZE))
        {
            /* No more than 32 pages */
            MmSystemCacheWsMinimum = 32;
        }

        /* Check for more than 32MB */
        if (MmNumberOfPhysicalPages >= ((32 * _1MB) / PAGE_SIZE))
        {
            /* Check for product type being "Wi" for WinNT */
            if (MmProductType == '\0i\0W')
            {
                /* Then this is a large system */
                MmSystemSize = MmLargeSystem;
            }
            else
            {
                /* For servers, we need 64MB to consider this as being large */
                if (MmNumberOfPhysicalPages >= ((64 * _1MB) / PAGE_SIZE))
                {
                    /* Set it as large */
                    MmSystemSize = MmLargeSystem;
                }
            }
        }

        /* Check for more than 33 MB */
        if (MmNumberOfPhysicalPages > ((33 * _1MB) / PAGE_SIZE))
        {
            /* Add another 500 pages to the cache */
            MmSystemCacheWsMinimum += 500;
        }

        /* Now setup the shared user data fields */
        ASSERT(SharedUserData->NumberOfPhysicalPages == 0);
        SharedUserData->NumberOfPhysicalPages = MmNumberOfPhysicalPages;
        SharedUserData->LargePageMinimum = 0;

        /* Check for workstation (Wi for WinNT) */
        if (MmProductType == '\0i\0W')
        {
            /* Set Windows NT Workstation product type */
            SharedUserData->NtProductType = NtProductWinNt;
            MmProductType = 0;

            /* For this product, we wait till the last moment to throttle */
            MmThrottleTop = 250;
            MmThrottleBottom = 30;
        }
        else
        {
            /* Check for LanMan server (La for LanmanNT) */
            if (MmProductType == '\0a\0L')
            {
                /* This is a domain controller */
                SharedUserData->NtProductType = NtProductLanManNt;
            }
            else
            {
                /* Otherwise it must be a normal server (Se for ServerNT) */
                SharedUserData->NtProductType = NtProductServer;
            }

            /* Set the product type, and make the system more aggressive with low memory */
            MmProductType = 1;
            MmMinimumFreePages = 81;

            /* We will throttle earlier to preserve memory */
            MmThrottleTop = 450;
            MmThrottleBottom = 80;
        }

        /* Update working set tuning parameters */
        MiAdjustWorkingSetManagerParameters(!MmProductType);

        /* Finetune the page count by removing working set and NP expansion */
        MmResidentAvailablePages -= MiExpansionPoolPagesInitialCharge;
        MmResidentAvailablePages -= MmSystemCacheWsMinimum;
        MmResidentAvailableAtInit = MmResidentAvailablePages;
        if (MmResidentAvailablePages <= 0)
        {
            /* This should not happen */
            DPRINT1("System cache working set too big\n");
            return FALSE;
        }

        /* Define limits for system cache */
#ifdef _M_AMD64
        MmSizeOfSystemCacheInPages = ((MI_SYSTEM_CACHE_END + 1) - MI_SYSTEM_CACHE_START) / PAGE_SIZE;
#else
        MmSizeOfSystemCacheInPages = ((ULONG_PTR)MI_PAGED_POOL_START - (ULONG_PTR)MI_SYSTEM_CACHE_START) / PAGE_SIZE;
#endif
        MmSystemCacheEnd = (PVOID)((ULONG_PTR)MmSystemCacheStart + (MmSizeOfSystemCacheInPages * PAGE_SIZE) - 1);
#ifdef _M_AMD64
        ASSERT(MmSystemCacheEnd == (PVOID)MI_SYSTEM_CACHE_END);
#else
        ASSERT(MmSystemCacheEnd == (PVOID)((ULONG_PTR)MI_PAGED_POOL_START - 1));
#endif

        /* Initialize the system cache */
        //MiInitializeSystemCache(MmSystemCacheWsMinimum, MmAvailablePages);

        /* Update the commit limit */
        MmTotalCommitLimit = MmAvailablePages;
        if (MmTotalCommitLimit > 1024) MmTotalCommitLimit -= 1024;
        MmTotalCommitLimitMaximum = MmTotalCommitLimit;

        /* Size up paged pool and build the shadow system page directory */
        MiBuildPagedPool();

        /* Debugger physical memory support is now ready to be used */
        MmDebugPte = MiAddressToPte(MiDebugMapping);

        /* Initialize the loaded module list */
        MiInitializeLoadedModuleList(LoaderBlock);
    }

    //
    // Always return success for now
    //
    return TRUE;
}

/* EOF */
