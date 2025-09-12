/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/windows/wlmemory.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>
#include "winldr.h"

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

static const PCSTR MemTypeDesc[] = {
    "ExceptionBlock    ", // ?
    "SystemBlock       ", // ?
    "Free              ",
    "Bad               ", // used
    "LoadedProgram     ", // == Free
    "FirmwareTemporary ", // == Free
    "FirmwarePermanent ", // == Bad
    "OsloaderHeap      ", // used
    "OsloaderStack     ", // == Free
    "SystemCode        ",
    "HalCode           ",
    "BootDriver        ", // not used
    "ConsoleInDriver   ", // ?
    "ConsoleOutDriver  ", // ?
    "StartupDpcStack   ", // ?
    "StartupKernelStack", // ?
    "StartupPanicStack ", // ?
    "StartupPcrPage    ", // ?
    "StartupPdrPage    ", // ?
    "RegistryData      ", // used
    "MemoryData        ", // not used
    "NlsData           ", // used
    "SpecialMemory     ", // == Bad
    "BBTMemory         " // == Bad
    };

static VOID
WinLdrInsertDescriptor(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor);

/* GLOBALS ***************************************************************/

MEMORY_ALLOCATION_DESCRIPTOR *Mad;
ULONG MadCount = 0;
/* 200 MADs fit into 1 page, that should really be enough! */
#define MAX_MAD_COUNT 200

/* FUNCTIONS **************************************************************/

VOID
MempAddMemoryBlock(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   PFN_NUMBER BasePage,
                   PFN_NUMBER PageCount,
                   ULONG Type)
{
    TRACE("MempAddMemoryBlock(BasePage=0x%lx, PageCount=0x%lx, Type=%ld)\n",
          BasePage, PageCount, Type);

    /* Check for memory block after 4GB - we don't support it yet
       Note: Even last page before 4GB limit is not supported */
    if (BasePage >= MM_MAX_PAGE)
    {
        /* Just skip this, without even adding to MAD list */
        return;
    }

    /* Check if last page is after 4GB limit and shorten this block if needed */
    if (BasePage + PageCount > MM_MAX_PAGE)
    {
        /* Shorten this block */
        PageCount = MM_MAX_PAGE - BasePage;
    }

    /* Check if we have slots left */
    if (MadCount >= MAX_MAD_COUNT)
    {
        ERR("Error: no MAD slots left!\n");
        return;
    }

    /* Set Base page, page count and type */
    Mad[MadCount].BasePage = BasePage;
    Mad[MadCount].PageCount = PageCount;
    Mad[MadCount].MemoryType = Type;

    /* Add descriptor */
    WinLdrInsertDescriptor(LoaderBlock, &Mad[MadCount]);
    MadCount++;
}

VOID
MempSetupPagingForRegion(
    PFN_NUMBER BasePage,
    PFN_NUMBER PageCount,
    ULONG Type)
{
    BOOLEAN Status = TRUE;

    TRACE("MempSetupPagingForRegion(BasePage=0x%lx, PageCount=0x%lx, Type=%ld)\n",
          BasePage, PageCount, Type);

    /* Make sure we don't map too high */
    if (BasePage + PageCount > MmGetLoaderPagesSpanned()) return;

    switch (Type)
    {
        /* Pages used by the loader */
        case LoaderLoadedProgram:
        case LoaderOsloaderStack:
        case LoaderFirmwareTemporary:
            /* Map these pages into user mode */
            Status = MempSetupPaging(BasePage, PageCount, FALSE);
            break;

        /* Pages used by the kernel */
        case LoaderExceptionBlock:
        case LoaderSystemBlock:
        case LoaderFirmwarePermanent:
        case LoaderSystemCode:
        case LoaderHalCode:
        case LoaderBootDriver:
        case LoaderConsoleInDriver:
        case LoaderConsoleOutDriver:
        case LoaderStartupDpcStack:
        case LoaderStartupKernelStack:
        case LoaderStartupPanicStack:
        case LoaderStartupPcrPage:
        case LoaderStartupPdrPage:
        case LoaderRegistryData:
        case LoaderMemoryData:
        case LoaderNlsData:
        case LoaderXIPRom:
        case LoaderOsloaderHeap: // FIXME
            /* Map these pages into kernel mode */
            Status = MempSetupPaging(BasePage, PageCount, TRUE);
            break;

        /* Pages not in use */
        case LoaderFree:
        case LoaderBad:
            break;

        /* Invisible to kernel */
        case LoaderSpecialMemory:
        case LoaderHALCachedMemory:
        case LoaderBBTMemory:
            break;

        // FIXME: not known (not used anyway)
        case LoaderReserve:
        case LoaderLargePageFiller:
        case LoaderErrorLogMemory:
            break;
    }

    if (!Status)
    {
        ERR("Error during MempSetupPaging\n");
    }
}

#ifdef _M_ARM
#define PKTSS PVOID
#endif

BOOLEAN
WinLdrSetupMemoryLayout(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PFN_NUMBER i, PagesCount, MemoryMapSizeInPages, NoEntries;
    PFN_NUMBER LastPageIndex, MemoryMapStartPage;
    PPAGE_LOOKUP_TABLE_ITEM MemoryMap;
    ULONG LastPageType;
    //PKTSS Tss;
    BOOLEAN Status;

    /* Cleanup heap */
    FrLdrHeapCleanupAll();

    //
    // Creating a suitable memory map for Windows can be tricky, so let's
    // give a few advices:
    // 1) One must not map the whole available memory pages to PDE!
    //    Map only what's needed - 16Mb, 24Mb, 32Mb max I think,
    //    thus occupying 4, 6 or 8 PDE entries for identical mapping,
    //    the same quantity for KSEG0_BASE mapping, one more entry for
    //    hyperspace and one more entry for HAL physical pages mapping.
    // 2) Memory descriptors must map *the whole* physical memory
    //    showing any memory above 16/24/32 as FirmwareTemporary
    //
    // 3) Overall memory blocks count must not exceed 30 (?? why?)
    //

    //
    // During MmInitMachineDependent, the kernel zeroes PDE at the following address
    // 0xC0300000 - 0xC03007FC
    //
    // Then it finds the best place for non-paged pool:
    // StartPde C0300F70, EndPde C0300FF8, NumberOfPages C13, NextPhysPage 3AD
    //

    // Allocate memory for memory allocation descriptors
    Mad = MmAllocateMemoryWithType(sizeof(MEMORY_ALLOCATION_DESCRIPTOR) * MAX_MAD_COUNT,
                                   LoaderMemoryData);

    // Setup an entry for each descriptor
    MemoryMap = MmGetMemoryMap(&NoEntries);
    if (MemoryMap == NULL)
    {
        UiMessageBox("Can not retrieve the current memory map.");
        return FALSE;
    }

    // Calculate parameters of the memory map
    MemoryMapStartPage = (ULONG_PTR)MemoryMap >> MM_PAGE_SHIFT;
    MemoryMapSizeInPages = (NoEntries * sizeof(PAGE_LOOKUP_TABLE_ITEM) + MM_PAGE_SIZE - 1) / MM_PAGE_SIZE;

    TRACE("Got memory map with %d entries\n", NoEntries);

    // Always map first page of memory
    Status = MempSetupPaging(0, 1, FALSE);
    if (!Status)
    {
        ERR("Error during MempSetupPaging of first page\n");
        return FALSE;
    }

    /* Before creating the map, we need to map pages to kernel mode */
    LastPageIndex = 1;
    LastPageType = MemoryMap[1].PageAllocated;
    for (i = 2; i < NoEntries; i++)
    {
        if ((MemoryMap[i].PageAllocated != LastPageType) ||
            (i == NoEntries - 1))
        {
            MempSetupPagingForRegion(LastPageIndex, i - LastPageIndex, LastPageType);
            LastPageIndex = i;
            LastPageType = MemoryMap[i].PageAllocated;
        }
    }

    // Construct a good memory map from what we've got,
    // but mark entries which the memory allocation bitmap takes
    // as free entries (this is done in order to have the ability
    // to place mem alloc bitmap outside lower 16Mb zone)
    PagesCount = 1;
    LastPageIndex = 0;
    LastPageType = MemoryMap[0].PageAllocated;
    for (i = 1; i < NoEntries; i++)
    {
        // Check if its memory map itself
        if (i >= MemoryMapStartPage &&
            i < (MemoryMapStartPage+MemoryMapSizeInPages))
        {
            // Exclude it if current page belongs to the memory map
            MemoryMap[i].PageAllocated = LoaderFree;
        }

        // Process entry
        if (MemoryMap[i].PageAllocated == LastPageType &&
            (i != NoEntries-1) )
        {
            PagesCount++;
        }
        else
        {
            // Add the resulting region
            MempAddMemoryBlock(LoaderBlock, LastPageIndex, PagesCount, LastPageType);

            // Reset our counter vars
            LastPageIndex = i;
            LastPageType = MemoryMap[i].PageAllocated;
            PagesCount = 1;
        }
    }

    // TEMP, DEBUG!
    // adding special reserved memory zones for vmware workstation
#if 0
    {
        Mad[MadCount].BasePage = 0xfec00;
        Mad[MadCount].PageCount = 0x10;
        Mad[MadCount].MemoryType = LoaderSpecialMemory;
        WinLdrInsertDescriptor(LoaderBlock, &Mad[MadCount]);
        MadCount++;

        Mad[MadCount].BasePage = 0xfee00;
        Mad[MadCount].PageCount = 0x1;
        Mad[MadCount].MemoryType = LoaderSpecialMemory;
        WinLdrInsertDescriptor(LoaderBlock, &Mad[MadCount]);
        MadCount++;

        Mad[MadCount].BasePage = 0xfffe0;
        Mad[MadCount].PageCount = 0x20;
        Mad[MadCount].MemoryType = LoaderSpecialMemory;
        WinLdrInsertDescriptor(LoaderBlock, &Mad[MadCount]);
        MadCount++;
    }
#endif
    PFREELDR_MEMORY_DESCRIPTOR BiosMemoryMap;
    ULONG BiosMemoryMapEntryCount;

    BiosMemoryMapEntryCount = MmGetBiosMemoryMap(&BiosMemoryMap);

    /* Now we need to add high descriptors from the bios memory map */
    for (i = 0; i < BiosMemoryMapEntryCount; i++)
    {
        /* Check if its higher than the lookup table */
        if (BiosMemoryMap->BasePage > MmGetHighestPhysicalPage())
        {
            /* Copy this descriptor */
            MempAddMemoryBlock(LoaderBlock,
                               BiosMemoryMap->BasePage,
                               BiosMemoryMap->PageCount,
                               BiosMemoryMap->MemoryType);
        }
    }

    TRACE("MadCount: %d\n", MadCount);

    WinLdrpDumpMemoryDescriptors(LoaderBlock); //FIXME: Delete!

    // Map our loader image, so we can continue running
    /*Status = MempSetupPaging(OsLoaderBase >> MM_PAGE_SHIFT, OsLoaderSize >> MM_PAGE_SHIFT);
    if (!Status)
    {
        UiMessageBox("Error during MempSetupPaging.");
        return;
    }*/

    // Fill the memory descriptor list and
    //PrepareMemoryDescriptorList();
    TRACE("Memory Descriptor List prepared, printing PDE\n");
    List_PaToVa(&LoaderBlock->MemoryDescriptorListHead);

#if DBG
    MempDump();
#endif

    return TRUE;
}

// Two special things this func does: it sorts descriptors,
// and it merges free ones
static VOID
WinLdrInsertDescriptor(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor)
{
    PLIST_ENTRY ListHead = &LoaderBlock->MemoryDescriptorListHead;
    PLIST_ENTRY PreviousEntry, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR PreviousDescriptor = NULL, NextDescriptor = NULL;

    TRACE("BP=0x%X PC=0x%X %s\n", NewDescriptor->BasePage,
        NewDescriptor->PageCount, MemTypeDesc[NewDescriptor->MemoryType]);

    /* Find a place where to insert the new descriptor to */
    PreviousEntry = ListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        NextDescriptor = CONTAINING_RECORD(NextEntry,
            MEMORY_ALLOCATION_DESCRIPTOR,
            ListEntry);
        if (NewDescriptor->BasePage < NextDescriptor->BasePage)
            break;

        PreviousEntry = NextEntry;
        PreviousDescriptor = NextDescriptor;
        NextEntry = NextEntry->Flink;
    }

    /* Don't forget about merging free areas */
    if (NewDescriptor->MemoryType != LoaderFree)
    {
        /* Just insert, nothing to merge */
        InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);
    }
    else
    {
        /* Previous block also free? */
        if ((PreviousEntry != ListHead) && (PreviousDescriptor->MemoryType == LoaderFree) &&
            ((PreviousDescriptor->BasePage + PreviousDescriptor->PageCount) ==
            NewDescriptor->BasePage))
        {
            /* Just enlarge previous descriptor's PageCount */
            PreviousDescriptor->PageCount += NewDescriptor->PageCount;
            NewDescriptor = PreviousDescriptor;
        }
        else
        {
            /* Nope, just insert */
            InsertHeadList(PreviousEntry, &NewDescriptor->ListEntry);
        }

        /* Next block is free ?*/
        if ((NextEntry != ListHead) &&
            (NextDescriptor->MemoryType == LoaderFree) &&
            ((NewDescriptor->BasePage + NewDescriptor->PageCount) == NextDescriptor->BasePage))
        {
            /* Enlarge next descriptor's PageCount */
            NewDescriptor->PageCount += NextDescriptor->PageCount;
            RemoveEntryList(&NextDescriptor->ListEntry);
        }
    }

    return;
}
