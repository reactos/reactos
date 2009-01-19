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

extern ULONG TotalNLSSize;
extern ULONG LoaderPagesSpanned;

// This is needed because headers define wrong one for ReactOS
#undef KIP0PCRADDRESS
#define KIP0PCRADDRESS                      0xffdff000

#define HYPER_SPACE_ENTRY       0x300

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
WinLdrSetProcessorContext(PVOID GdtIdt, IN ULONG Pcr, IN ULONG Tss);

// This is needed only for SetProcessorContext routine
#pragma pack(2)
	typedef struct
	{
		USHORT Limit;
		ULONG Base;
	} GDTIDT;
#pragma pack(4)

// this is needed for new IDT filling
#if 0
extern ULONG_PTR i386DivideByZero;
extern ULONG_PTR i386DebugException;
extern ULONG_PTR i386NMIException;
extern ULONG_PTR i386Breakpoint;
extern ULONG_PTR i386Overflow;
extern ULONG_PTR i386BoundException;
extern ULONG_PTR i386InvalidOpcode;
extern ULONG_PTR i386FPUNotAvailable;
extern ULONG_PTR i386DoubleFault;
extern ULONG_PTR i386CoprocessorSegment;
extern ULONG_PTR i386InvalidTSS;
extern ULONG_PTR i386SegmentNotPresent;
extern ULONG_PTR i386StackException;
extern ULONG_PTR i386GeneralProtectionFault;
extern ULONG_PTR i386PageFault; // exc 14
extern ULONG_PTR i386CoprocessorError; // exc 16
extern ULONG_PTR i386AlignmentCheck; // exc 17
#endif

/* GLOBALS ***************************************************************/

PHARDWARE_PTE PDE;
PHARDWARE_PTE HalPageTable;

PUCHAR PhysicalPageTablesBuffer;
PUCHAR KernelPageTablesBuffer;
ULONG PhysicalPageTables;
ULONG KernelPageTables;

MEMORY_ALLOCATION_DESCRIPTOR *Mad;
ULONG MadCount = 0;


/* FUNCTIONS **************************************************************/

BOOLEAN
MempAllocatePageTables()
{
	ULONG NumPageTables, TotalSize;
	PUCHAR Buffer;
	// It's better to allocate PDE + PTEs contigiuos

	// Max number of entries = MaxPageNum >> 10
	// FIXME: This is a number to describe ALL physical memory
	// and windows doesn't expect ALL memory mapped...
	NumPageTables = (GetSystemMemorySize() >> MM_PAGE_SHIFT) >> 10;

	DPRINTM((DPRINT_WINDOWS, "NumPageTables = %d\n", NumPageTables));

	// Allocate memory block for all these things:
	// PDE, HAL mapping page table, physical mapping, kernel mapping
	TotalSize = (1+1+NumPageTables*2)*MM_PAGE_SIZE;

	// PDE+HAL+KernelPTEs == MemoryData
	Buffer = MmAllocateMemoryWithType(TotalSize, LoaderMemoryData);

	// Physical PTEs = FirmwareTemporary
	PhysicalPageTablesBuffer = (PUCHAR)Buffer + TotalSize - NumPageTables*MM_PAGE_SIZE;
	MmSetMemoryType(PhysicalPageTablesBuffer,
	                NumPageTables*MM_PAGE_SIZE,
	                LoaderFirmwareTemporary);

	// This check is now redundant
	if (Buffer + (TotalSize - NumPageTables*MM_PAGE_SIZE) !=
		PhysicalPageTablesBuffer)
	{
		DPRINTM((DPRINT_WINDOWS, "There was a problem allocating two adjacent blocks of memory!"));
	}

	if (Buffer == NULL || PhysicalPageTablesBuffer == NULL)
	{
		UiMessageBox("Impossible to allocate memory block for page tables!");
		return FALSE;
	}

	// Zero all this memory block
	RtlZeroMemory(Buffer, TotalSize);

	// Set up pointers correctly now
	PDE = (PHARDWARE_PTE)Buffer;

	// Map the page directory at 0xC0000000 (maps itself)
	PDE[HYPER_SPACE_ENTRY].PageFrameNumber = (ULONG)PDE >> MM_PAGE_SHIFT;
	PDE[HYPER_SPACE_ENTRY].Valid = 1;
	PDE[HYPER_SPACE_ENTRY].Write = 1;

	// The last PDE slot is allocated for HAL's memory mapping (Virtual Addresses 0xFFC00000 - 0xFFFFFFFF)
	HalPageTable = (PHARDWARE_PTE)&Buffer[MM_PAGE_SIZE*1];

	// Map it
	PDE[1023].PageFrameNumber = (ULONG)HalPageTable >> MM_PAGE_SHIFT;
	PDE[1023].Valid = 1;
	PDE[1023].Write = 1;

	// Store pointer to the table for easier access
	KernelPageTablesBuffer = &Buffer[MM_PAGE_SIZE*2];

	// Zero counters of page tables used
	PhysicalPageTables = 0;
	KernelPageTables = 0;

	return TRUE;
}

VOID
MempAllocatePTE(ULONG Entry, PHARDWARE_PTE *PhysicalPT, PHARDWARE_PTE *KernelPT)
{
	//Print(L"Creating PDE Entry %X\n", Entry);

	// Identity mapping
	*PhysicalPT = (PHARDWARE_PTE)&PhysicalPageTablesBuffer[PhysicalPageTables*MM_PAGE_SIZE];
	PhysicalPageTables++;

	PDE[Entry].PageFrameNumber = (ULONG)*PhysicalPT >> MM_PAGE_SHIFT;
	PDE[Entry].Valid = 1;
	PDE[Entry].Write = 1;

	if (Entry+(KSEG0_BASE >> 22) > 1023)
	{
		DPRINTM((DPRINT_WINDOWS, "WARNING! Entry: %X > 1023\n", Entry+(KSEG0_BASE >> 22)));
	}

	// Kernel-mode mapping
	*KernelPT = (PHARDWARE_PTE)&KernelPageTablesBuffer[KernelPageTables*MM_PAGE_SIZE];
	KernelPageTables++;

	PDE[Entry+(KSEG0_BASE >> 22)].PageFrameNumber = ((ULONG)*KernelPT >> MM_PAGE_SHIFT);
	PDE[Entry+(KSEG0_BASE >> 22)].Valid = 1;
	PDE[Entry+(KSEG0_BASE >> 22)].Write = 1;
}

BOOLEAN
MempSetupPaging(IN ULONG StartPage,
				IN ULONG NumberOfPages)
{
	PHARDWARE_PTE PhysicalPT;
	PHARDWARE_PTE KernelPT;
	ULONG Entry, Page;

	//Print(L"MempSetupPaging: SP 0x%X, Number: 0x%X\n", StartPage, NumberOfPages);
	
	// HACK
	if (StartPage+NumberOfPages >= 0x80000)
	{
		//
		// We can't map this as it requires more than 1 PDE
		// and in fact it's not possible at all ;)
		//
		//Print(L"skipping...\n");
		return TRUE;
	}

	//
	// Now actually set up the page tables for identity mapping
	//
	for (Page=StartPage; Page < StartPage+NumberOfPages; Page++)
	{
		Entry = Page >> 10;

		if (((PULONG)PDE)[Entry] == 0)
		{
			MempAllocatePTE(Entry, &PhysicalPT, &KernelPT);
		}
		else
		{
			PhysicalPT = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << MM_PAGE_SHIFT);
			KernelPT = (PHARDWARE_PTE)(PDE[Entry+(KSEG0_BASE >> 22)].PageFrameNumber << MM_PAGE_SHIFT);
		}

		if (Page == 0)
		{
			PhysicalPT[Page & 0x3ff].PageFrameNumber = Page;
			PhysicalPT[Page & 0x3ff].Valid = 0;
			PhysicalPT[Page & 0x3ff].Write = 0;

			KernelPT[Page & 0x3ff].PageFrameNumber = Page;
			KernelPT[Page & 0x3ff].Valid = 0;
			KernelPT[Page & 0x3ff].Write = 0;
		}
		else
		{
			PhysicalPT[Page & 0x3ff].PageFrameNumber = Page;
			PhysicalPT[Page & 0x3ff].Valid = 1;
			PhysicalPT[Page & 0x3ff].Write = 1;

			KernelPT[Page & 0x3ff].PageFrameNumber = Page;
			KernelPT[Page & 0x3ff].Valid = 1;
			KernelPT[Page & 0x3ff].Write = 1;
		}
	}

	return TRUE;
}

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
				PHARDWARE_PTE KernelPT;
				ULONG Entry = (Page >> 10) + (KSEG0_BASE >> 22);

				if (PDE[Entry].Valid)
				{
					KernelPT = (PHARDWARE_PTE)(PDE[Entry].PageFrameNumber << MM_PAGE_SHIFT);

					if (KernelPT)
					{
						KernelPT[Page & 0x3ff].PageFrameNumber = 0;
						KernelPT[Page & 0x3ff].Valid = 0;
						KernelPT[Page & 0x3ff].Write = 0;
					}
				}
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
			DPRINTM((DPRINT_WINDOWS, "Setting page %x %x to Temporary from %d\n",
				BasePage, PageCount, Mad[MadCount].MemoryType));
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
	// Map it (don't map low 1Mb because it was already contigiously
	// mapped in WinLdrTurnOnPaging)
	//
	if (BasePage >= 0x100)
	{
		Status = MempSetupPaging(BasePage, PageCount);
		if (!Status)
		{
			DPRINTM((DPRINT_WINDOWS, "Error during MempSetupPaging\n"));
			return;
		}
	}
}

#ifdef _M_IX86

BOOLEAN LocalAPIC = FALSE;
ULONG_PTR APICAddress = 0;

VOID
WinLdrpMapApic()
{
	/* Check if we have a local APIC */
	asm(".intel_syntax noprefix\n");
		asm("mov eax, 1\n");
		asm("cpuid\n");
		asm("shr edx, 9\n");
		asm("and edx, 0x1\n");
		asm("mov _LocalAPIC, edx\n");
	asm(".att_syntax\n");

	/* If there is no APIC, just return */
	if (!LocalAPIC)
		return;

	asm(".intel_syntax noprefix\n");
		asm("mov ecx, 0x1B\n");
		asm("rdmsr\n");
		asm("mov edx, eax\n");
		asm("and edx, 0xFFFFF000\n");
		asm("mov _APICAddress, edx");
	asm(".att_syntax\n");

	DPRINTM((DPRINT_WINDOWS, "Local APIC detected at address 0x%x\n",
		APICAddress));

	/* Map it */
	HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber
		= APICAddress >> MM_PAGE_SHIFT;
	HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
	HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;
	HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].WriteThrough = 1;
	HalPageTable[(APIC_BASE - 0xFFC00000) >> MM_PAGE_SHIFT].CacheDisable = 1;
}
#else
VOID
WinLdrpMapApic()
{
	/* Implement it for another arch */
}
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

	DPRINTM((DPRINT_WINDOWS, "Got memory map with %d entries\n", NoEntries));

	// Always contigiously map low 1Mb of memory
	Status = MempSetupPaging(0, 0x100);
	if (!Status)
	{
		DPRINTM((DPRINT_WINDOWS, "Error during MempSetupPaging of low 1Mb\n"));
		return FALSE;
	}

	// Construct a good memory map from what we've got,
	// but mark entries which the memory allocation bitmap takes
	// as free entries (this is done in order to have the ability
	// to place mem alloc bitmap outside lower 16Mb zone)
	PagesCount = 1;
	LastPageIndex = 0;
	LastPageType = MemoryMap[0].PageAllocated;
	for(i=1;i<NoEntries;i++)
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

	DPRINTM((DPRINT_WINDOWS, "MadCount: %d\n", MadCount));

	WinLdrpDumpMemoryDescriptors(LoaderBlock); //FIXME: Delete!

	// Map our loader image, so we can continue running
	/*Status = MempSetupPaging(OsLoaderBase >> MM_PAGE_SHIFT, OsLoaderSize >> MM_PAGE_SHIFT);
	if (!Status)
	{
		UiMessageBox("Error during MempSetupPaging");
		return;
	}*/

	//VideoDisplayString(L"Hello from VGA, going into the kernel\n");
	DPRINTM((DPRINT_WINDOWS, "HalPageTable: 0x%X\n", HalPageTable));

	// Page Tables have been setup, make special handling for PCR and TSS
	// (which is done in BlSetupFotNt in usual ntldr)
	HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber = PcrBasePage+1;
	HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
	HalPageTable[(KI_USER_SHARED_DATA - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;

	HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].PageFrameNumber = PcrBasePage;
	HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].Valid = 1;
	HalPageTable[(KIP0PCRADDRESS - 0xFFC00000) >> MM_PAGE_SHIFT].Write = 1;

	// Map APIC
	WinLdrpMapApic();

	// Map VGA memory
	//VideoMemoryBase = MmMapIoSpace(0xb8000, 4000, MmNonCached);
	//DPRINTM((DPRINT_WINDOWS, "VideoMemoryBase: 0x%X\n", VideoMemoryBase));

	Tss = (PKTSS)(KSEG0_BASE | (TssBasePage << MM_PAGE_SHIFT));

	// Unmap what is not needed from kernel page table
	MempDisablePages();

	// Fill the memory descriptor list and 
	//PrepareMemoryDescriptorList();
	DPRINTM((DPRINT_WINDOWS, "Memory Descriptor List prepared, printing PDE\n"));
	List_PaToVa(&LoaderBlock->MemoryDescriptorListHead);

#ifdef DBG
	{
		ULONG *PDE_Addr=(ULONG *)PDE;//0xC0300000;
		int j;

		DPRINTM((DPRINT_WINDOWS, "\nPDE\n"));

		for (i=0; i<128; i++)
		{
			DPRINTM((DPRINT_WINDOWS, "0x%04X | ", i*8));

			for (j=0; j<8; j++)
			{
				DPRINTM((DPRINT_WINDOWS, "0x%08X ", PDE_Addr[i*8+j]));
			}

			DPRINTM((DPRINT_WINDOWS, "\n"));
		}
	}
#endif


	// Enable paging
	//BS->ExitBootServices(ImageHandle,MapKey);

	// Disable Interrupts
	_disable();

	// Re-initalize EFLAGS
	Ke386EraseFlags();

	// Set the PDBR
	__writecr3((ULONG_PTR)PDE);

	// Enable paging by modifying CR0
	__writecr0(__readcr0() | CR0_PG);

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

	DPRINTM((DPRINT_WINDOWS, "BP=0x%X PC=0x%X %s\n", NewDescriptor->BasePage,
		NewDescriptor->PageCount, MemTypeDesc[NewDescriptor->MemoryType]));

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

VOID
WinLdrSetProcessorContext(PVOID GdtIdt, IN ULONG Pcr, IN ULONG Tss)
{
	GDTIDT GdtDesc, IdtDesc, OldIdt;
	PKGDTENTRY	pGdt;
	PKIDTENTRY	pIdt;
	ULONG Ldt = 0;
	//ULONG i;

	DPRINTM((DPRINT_WINDOWS, "GDtIdt %p, Pcr %p, Tss 0x%08X\n",
		GdtIdt, Pcr, Tss));

	// Kernel expects the PCR to be zero-filled on startup
	// FIXME: Why zero it here when we can zero it right after allocation?
	RtlZeroMemory((PVOID)Pcr, MM_PAGE_SIZE); //FIXME: Why zero only 1 page when we allocate 2?

	// Get old values of GDT and IDT
	Ke386GetGlobalDescriptorTable(GdtDesc);
	Ke386GetInterruptDescriptorTable(IdtDesc);

	// Save old IDT
	OldIdt.Base = IdtDesc.Base;
	OldIdt.Limit = IdtDesc.Limit;

	// Prepare new IDT+GDT
	GdtDesc.Base  = KSEG0_BASE | (ULONG_PTR)GdtIdt;
	GdtDesc.Limit = NUM_GDT * sizeof(KGDTENTRY) - 1;
	IdtDesc.Base  = (ULONG)((PUCHAR)GdtDesc.Base + GdtDesc.Limit + 1);
	IdtDesc.Limit = NUM_IDT * sizeof(KIDTENTRY) - 1;

	// ========================
	// Fill all descriptors now
	// ========================

	pGdt = (PKGDTENTRY)GdtDesc.Base;
	pIdt = (PKIDTENTRY)IdtDesc.Base;

	//
	// Code selector (0x8)
	// Flat 4Gb
	//
	pGdt[1].LimitLow				= 0xFFFF;
	pGdt[1].BaseLow					= 0;
	pGdt[1].HighWord.Bytes.BaseMid	= 0;
	pGdt[1].HighWord.Bytes.Flags1	= 0x9A;
	pGdt[1].HighWord.Bytes.Flags2	= 0xCF;
	pGdt[1].HighWord.Bytes.BaseHi	= 0;

	//
	// Data selector (0x10)
	// Flat 4Gb
	//
	pGdt[2].LimitLow				= 0xFFFF;
	pGdt[2].BaseLow					= 0;
	pGdt[2].HighWord.Bytes.BaseMid	= 0;
	pGdt[2].HighWord.Bytes.Flags1	= 0x92;
	pGdt[2].HighWord.Bytes.Flags2	= 0xCF;
	pGdt[2].HighWord.Bytes.BaseHi	= 0;

	//
	// Selector (0x18)
	// Flat 2Gb
	//
	pGdt[3].LimitLow				= 0xFFFF;
	pGdt[3].BaseLow					= 0;
	pGdt[3].HighWord.Bytes.BaseMid	= 0;
	pGdt[3].HighWord.Bytes.Flags1	= 0xFA;
	pGdt[3].HighWord.Bytes.Flags2	= 0xCF;
	pGdt[3].HighWord.Bytes.BaseHi	= 0;

	//
	// Selector (0x20)
	// Flat 2Gb
	//
	pGdt[4].LimitLow				= 0xFFFF;
	pGdt[4].BaseLow					= 0;
	pGdt[4].HighWord.Bytes.BaseMid	= 0;
	pGdt[4].HighWord.Bytes.Flags1	= 0xF2;
	pGdt[4].HighWord.Bytes.Flags2	= 0xCF;
	pGdt[4].HighWord.Bytes.BaseHi	= 0;

	//
	// TSS Selector (0x28)
	//
	pGdt[5].LimitLow				= 0x78-1; //FIXME: Check this
	pGdt[5].BaseLow = (USHORT)(Tss & 0xffff);
	pGdt[5].HighWord.Bytes.BaseMid = (UCHAR)((Tss >> 16) & 0xff);
	pGdt[5].HighWord.Bytes.Flags1	= 0x89;
	pGdt[5].HighWord.Bytes.Flags2	= 0x00;
	pGdt[5].HighWord.Bytes.BaseHi  = (UCHAR)((Tss >> 24) & 0xff);

	//
	// PCR Selector (0x30)
	//
	pGdt[6].LimitLow				= 0x01;
	pGdt[6].BaseLow  = (USHORT)(Pcr & 0xffff);
	pGdt[6].HighWord.Bytes.BaseMid = (UCHAR)((Pcr >> 16) & 0xff);
	pGdt[6].HighWord.Bytes.Flags1	= 0x92;
	pGdt[6].HighWord.Bytes.Flags2	= 0xC0;
	pGdt[6].HighWord.Bytes.BaseHi  = (UCHAR)((Pcr >> 24) & 0xff);

	//
	// Selector (0x38)
	//
	pGdt[7].LimitLow				= 0xFFFF;
	pGdt[7].BaseLow					= 0;
	pGdt[7].HighWord.Bytes.BaseMid	= 0;
	pGdt[7].HighWord.Bytes.Flags1	= 0xF3;
	pGdt[7].HighWord.Bytes.Flags2	= 0x40;
	pGdt[7].HighWord.Bytes.BaseHi	= 0;

	//
	// Some BIOS stuff (0x40)
	//
	pGdt[8].LimitLow				= 0xFFFF;
	pGdt[8].BaseLow					= 0x400;
	pGdt[8].HighWord.Bytes.BaseMid	= 0;
	pGdt[8].HighWord.Bytes.Flags1	= 0xF2;
	pGdt[8].HighWord.Bytes.Flags2	= 0x0;
	pGdt[8].HighWord.Bytes.BaseHi	= 0;

	//
	// Selector (0x48)
	//
	pGdt[9].LimitLow				= 0;
	pGdt[9].BaseLow					= 0;
	pGdt[9].HighWord.Bytes.BaseMid	= 0;
	pGdt[9].HighWord.Bytes.Flags1	= 0;
	pGdt[9].HighWord.Bytes.Flags2	= 0;
	pGdt[9].HighWord.Bytes.BaseHi	= 0;

	//
	// Selector (0x50)
	//
	pGdt[10].LimitLow				= 0xFFFF; //FIXME: Not correct!
	pGdt[10].BaseLow				= 0;
	pGdt[10].HighWord.Bytes.BaseMid	= 0x2;
	pGdt[10].HighWord.Bytes.Flags1	= 0x89;
	pGdt[10].HighWord.Bytes.Flags2	= 0;
	pGdt[10].HighWord.Bytes.BaseHi	= 0;

	//
	// Selector (0x58)
	//
	pGdt[11].LimitLow				= 0xFFFF;
	pGdt[11].BaseLow				= 0;
	pGdt[11].HighWord.Bytes.BaseMid	= 0x2;
	pGdt[11].HighWord.Bytes.Flags1	= 0x9A;
	pGdt[11].HighWord.Bytes.Flags2	= 0;
	pGdt[11].HighWord.Bytes.BaseHi	= 0;

	//
	// Selector (0x60)
	//
	pGdt[12].LimitLow				= 0xFFFF;
	pGdt[12].BaseLow				= 0; //FIXME: Maybe not correct, but noone cares
	pGdt[12].HighWord.Bytes.BaseMid	= 0x2;
	pGdt[12].HighWord.Bytes.Flags1	= 0x92;
	pGdt[12].HighWord.Bytes.Flags2	= 0;
	pGdt[12].HighWord.Bytes.BaseHi	= 0;

	//
	// Video buffer Selector (0x68)
	//
	pGdt[13].LimitLow				= 0x3FFF;
	pGdt[13].BaseLow				= 0x8000;
	pGdt[13].HighWord.Bytes.BaseMid	= 0x0B;
	pGdt[13].HighWord.Bytes.Flags1	= 0x92;
	pGdt[13].HighWord.Bytes.Flags2	= 0;
	pGdt[13].HighWord.Bytes.BaseHi	= 0;

	//
	// Points to GDT (0x70)
	//
	pGdt[14].LimitLow				= NUM_GDT*sizeof(KGDTENTRY) - 1;
	pGdt[14].BaseLow				= 0x7000;
	pGdt[14].HighWord.Bytes.BaseMid	= 0xFF;
	pGdt[14].HighWord.Bytes.Flags1	= 0x92;
	pGdt[14].HighWord.Bytes.Flags2	= 0;
	pGdt[14].HighWord.Bytes.BaseHi	= 0xFF;

	//
	// Some unused descriptors should go here
	//

	// Copy the old IDT
	RtlCopyMemory(pIdt, (PVOID)OldIdt.Base, OldIdt.Limit);

	// Mask interrupts
	//asm("cli\n"); // they are already masked before enabling paged mode

	// Load GDT+IDT
	Ke386SetGlobalDescriptorTable(GdtDesc);
	Ke386SetInterruptDescriptorTable(IdtDesc);

	// Jump to proper CS and clear prefetch queue
	asm("ljmp	$0x08, $mb1\n"
		"mb1:\n");

	// Set SS selector
	asm(".intel_syntax noprefix\n");
		asm("mov ax, 0x10\n"); // DataSelector=0x10
		asm("mov ss, ax\n");
	asm(".att_syntax\n");

	// Set DS and ES selectors
	Ke386SetDs(0x10);
	Ke386SetEs(0x10); // this is vital for rep stosd

	// LDT = not used ever, thus set to 0
	Ke386SetLocalDescriptorTable(Ldt);

	// Load TSR
	Ke386SetTr(0x28);

	// Clear GS
	asm(".intel_syntax noprefix\n");
		asm("push 0\n");
		asm("pop gs\n");
	asm(".att_syntax\n");

	// Set FS to PCR
	Ke386SetFs(0x30);

		// Real end of the function, just for information
		/* do not uncomment!
		pop edi;
		pop esi;
		pop ebx;
		mov esp, ebp;
		pop ebp;
		ret
		*/
}
