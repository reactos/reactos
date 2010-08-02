/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            freeldr/winldr/wlmemory.c
 * PURPOSE:         Memory related routines
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>

#include <ndk/asm.h>
#include <debug.h>

extern ULONG LoaderPagesSpanned;

PCHAR  MemTypeDesc[]  = {
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

VOID
WinLdrpDumpMemoryDescriptors(PLOADER_PARAMETER_BLOCK LoaderBlock);


VOID
MempAddMemoryBlock(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG BasePage,
                   ULONG PageCount,
                   ULONG Type);
VOID
WinLdrInsertDescriptor(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor);

VOID
WinLdrRemoveDescriptor(IN PMEMORY_ALLOCATION_DESCRIPTOR Descriptor);

VOID
WinLdrSetProcessorContext(PVOID GdtIdt, IN ULONG_PTR Pcr, IN ULONG_PTR Tss);

BOOLEAN
MempAllocatePageTables();

BOOLEAN
MempSetupPaging(IN ULONG StartPage,
				IN ULONG NumberOfPages);

BOOLEAN
WinLdrMapSpecialPages(ULONG PcrBasePage);

VOID
MempUnmapPage(ULONG Page);

VOID
MempDump();

/* GLOBALS ***************************************************************/

MEMORY_ALLOCATION_DESCRIPTOR *Mad;
ULONG MadCount = 0;

/* FUNCTIONS **************************************************************/

VOID
MempDisablePages()
{
	ULONG i;

	//
	// We need to delete kernel mapping from memory areas which are
	// marked as Special or Permanent memory (thus non-accessible)
	//

	for (i=0; i<MadCount; i++)
	{
		ULONG StartPage, EndPage, Page;

		StartPage = Mad[i].BasePage;
		EndPage = Mad[i].BasePage + Mad[i].PageCount;

		if (Mad[i].MemoryType == LoaderFirmwarePermanent ||
			Mad[i].MemoryType == LoaderSpecialMemory ||
			Mad[i].MemoryType == LoaderFree ||
			(Mad[i].MemoryType == LoaderFirmwareTemporary && EndPage <= LoaderPagesSpanned) ||
			Mad[i].MemoryType == LoaderOsloaderStack ||
			Mad[i].MemoryType == LoaderLoadedProgram)
		{
			//
			// But, the first megabyte of memory always stays!
			// And, to tell the truth, we don't care about what's higher
			// than LoaderPagesSpanned
			if (Mad[i].MemoryType == LoaderFirmwarePermanent ||
				Mad[i].MemoryType == LoaderSpecialMemory)
			{
				if (StartPage < 0x100)
					StartPage = 0x100;

				if (EndPage > LoaderPagesSpanned)
					EndPage = LoaderPagesSpanned;
			}

			for (Page = StartPage; Page < EndPage; Page++)
			{
				MempUnmapPage(Page);
			}
		}
	}
}

VOID
MempAddMemoryBlock(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG BasePage,
                   ULONG PageCount,
                   ULONG Type)
{
	BOOLEAN Status;

	//
	// Check for some weird stuff at the top
	//
	if (BasePage + PageCount > 0xF0000)
	{
		//
		// Just skip this, without even adding to MAD list
		//
		return;
	}

	//
	// Set Base page, page count and type
	//
	Mad[MadCount].BasePage = BasePage;
	Mad[MadCount].PageCount = PageCount;
	Mad[MadCount].MemoryType = Type;

	//
	// Check if it's more than the allowed for OS loader
	// if yes - don't map the pages, just add as FirmwareTemporary
	//
	if (BasePage + PageCount > LoaderPagesSpanned)
	{
		if (Mad[MadCount].MemoryType != LoaderSpecialMemory &&
			Mad[MadCount].MemoryType != LoaderFirmwarePermanent &&
			Mad[MadCount].MemoryType != LoaderFree)
		{
			DPRINTM(DPRINT_WINDOWS, "Setting page %x %x to Temporary from %d\n",
				BasePage, PageCount, Mad[MadCount].MemoryType);
			Mad[MadCount].MemoryType = LoaderFirmwareTemporary;
		}

		WinLdrInsertDescriptor(LoaderBlock, &Mad[MadCount]);
		MadCount++;

		return;
	}
	
	//
	// Add descriptor
	//
	WinLdrInsertDescriptor(LoaderBlock, &Mad[MadCount]);
	MadCount++;

	//
	// Map it (don't map low 1Mb because it was already contiguously
	// mapped in WinLdrTurnOnPaging)
	//
	if (BasePage >= 0x100)
	{
		Status = MempSetupPaging(BasePage, PageCount);
		if (!Status)
		{
			DPRINTM(DPRINT_WINDOWS, "Error during MempSetupPaging\n");
			return;
		}
	}
}

#ifdef _M_ARM
#define PKTSS PVOID
#endif

BOOLEAN
WinLdrTurnOnPaging(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                   ULONG PcrBasePage,
                   ULONG TssBasePage,
                   PVOID GdtIdt)
{
	ULONG i, PagesCount, MemoryMapSizeInPages;
	ULONG LastPageIndex, LastPageType, MemoryMapStartPage;
	PPAGE_LOOKUP_TABLE_ITEM MemoryMap;
	ULONG NoEntries;
	PKTSS Tss;
	BOOLEAN Status;

	//
	// Creating a suitable memory map for the Windows can be tricky, so let's
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

	// Before we start mapping pages, create a block of memory, which will contain
	// PDE and PTEs
	if (MempAllocatePageTables() == FALSE)
		return FALSE;

	// Allocate memory for memory allocation descriptors
	Mad = MmHeapAlloc(sizeof(MEMORY_ALLOCATION_DESCRIPTOR) * 1024);

	// Setup an entry for each descriptor
	MemoryMap = MmGetMemoryMap(&NoEntries);
	if (MemoryMap == NULL)
	{
		UiMessageBox("Can not retrieve the current memory map");
		return FALSE;
	}

	// Calculate parameters of the memory map
	MemoryMapStartPage = (ULONG_PTR)MemoryMap >> MM_PAGE_SHIFT;
	MemoryMapSizeInPages = NoEntries * sizeof(PAGE_LOOKUP_TABLE_ITEM);

	DPRINTM(DPRINT_WINDOWS, "Got memory map with %d entries\n", NoEntries);

	// Always contiguously map low 1Mb of memory
	Status = MempSetupPaging(0, 0x100);
	if (!Status)
	{
		DPRINTM(DPRINT_WINDOWS, "Error during MempSetupPaging of low 1Mb\n");
		return FALSE;
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

	DPRINTM(DPRINT_WINDOWS, "MadCount: %d\n", MadCount);

	WinLdrpDumpMemoryDescriptors(LoaderBlock); //FIXME: Delete!

	// Map our loader image, so we can continue running
	/*Status = MempSetupPaging(OsLoaderBase >> MM_PAGE_SHIFT, OsLoaderSize >> MM_PAGE_SHIFT);
	if (!Status)
	{
		UiMessageBox("Error during MempSetupPaging");
		return;
	}*/

	/* Map stuff like PCR, KI_USER_SHARED_DATA and Apic */
	WinLdrMapSpecialPages(PcrBasePage);

	Tss = (PKTSS)(KSEG0_BASE | (TssBasePage << MM_PAGE_SHIFT));

	// Unmap what is not needed from kernel page table
	MempDisablePages();

	// Fill the memory descriptor list and 
	//PrepareMemoryDescriptorList();
	DPRINTM(DPRINT_WINDOWS, "Memory Descriptor List prepared, printing PDE\n");
	List_PaToVa(&LoaderBlock->MemoryDescriptorListHead);

#if DBG
    MempDump();
#endif

	// Set processor context
	WinLdrSetProcessorContext(GdtIdt, KIP0PCRADDRESS, KSEG0_BASE | (TssBasePage << MM_PAGE_SHIFT));

	// Zero KI_USER_SHARED_DATA page
	memset((PVOID)KI_USER_SHARED_DATA, 0, MM_PAGE_SIZE);

	return TRUE;
}

// Two special things this func does: it sorts descriptors,
// and it merges free ones
VOID
WinLdrInsertDescriptor(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                       IN PMEMORY_ALLOCATION_DESCRIPTOR NewDescriptor)
{
	PLIST_ENTRY ListHead = &LoaderBlock->MemoryDescriptorListHead;
	PLIST_ENTRY PreviousEntry, NextEntry;
	PMEMORY_ALLOCATION_DESCRIPTOR PreviousDescriptor = NULL, NextDescriptor = NULL;

	DPRINTM(DPRINT_WINDOWS, "BP=0x%X PC=0x%X %s\n", NewDescriptor->BasePage,
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

