/*
 *  FreeLoader
 *  Copyright (C) 2006-2008     Aleksey Bragin  <aleksey@reactos.org>
 *  Copyright (C) 2006-2009     Hervé Poussineau  <hpoussin@reactos.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <freeldr.h>
#include <debug.h>

DBG_DEFAULT_CHANNEL(MEMORY);

#if DBG
typedef struct
{
    MEMORY_TYPE Type;
    PCSTR TypeString;
} FREELDR_MEMORY_TYPE, *PFREELDR_MEMORY_TYPE;

FREELDR_MEMORY_TYPE MemoryTypeArray[] =
{
    { MemoryMaximum, "Unknown memory" },
    { MemoryExceptionBlock, "Exception block" },
    { MemorySystemBlock, "System block" },
    { MemoryFree, "Free memory" },
    { MemoryBad, "Bad memory" },
    { MemoryLoadedProgram, "Loaded program" },
    { MemoryFirmwareTemporary, "Firmware temporary" },
    { MemoryFirmwarePermanent, "Firmware permanent" },
    { MemoryFreeContiguous, "Free contiguous memory" },
    { MemorySpecialMemory, "Special memory" },
};
ULONG MemoryTypeCount = sizeof(MemoryTypeArray) / sizeof(MemoryTypeArray[0]);
#endif

PVOID	PageLookupTableAddress = NULL;
ULONG		TotalPagesInLookupTable = 0;
ULONG		FreePagesInLookupTable = 0;
ULONG		LastFreePageHint = 0;
ULONG MmLowestPhysicalPage = 0xFFFFFFFF;
ULONG MmHighestPhysicalPage = 0;

extern ULONG_PTR	MmHeapPointer;
extern ULONG_PTR	MmHeapStart;

typedef struct
{
    MEMORY_DESCRIPTOR m;
    ULONG Index;
    BOOLEAN GeneratedDescriptor;
} MEMORY_DESCRIPTOR_INT;
static const MEMORY_DESCRIPTOR_INT MemoryDescriptors[] =
{
#if defined (__i386__) || defined (_M_AMD64)
    { { MemoryFirmwarePermanent, 0x00, 1 }, 0, }, // realmode int vectors
    { { MemoryFirmwareTemporary, 0x01, 7 }, 1, }, // freeldr stack + cmdline
    { { MemoryLoadedProgram, 0x08, 0x70 }, 2, }, // freeldr image (roughly max. 0x64 pages)
    { { MemorySpecialMemory, 0x78, 8 }, 3, }, // prot mode stack. BIOSCALLBUFFER
    { { MemoryFirmwareTemporary, 0x80, 0x10 }, 4, }, // File system read buffer. FILESYSBUFFER
    { { MemoryFirmwareTemporary, 0x90, 0x10 }, 5, }, // Disk read buffer for int 13h. DISKREADBUFFER
    { { MemoryFirmwarePermanent, 0xA0, 0x60 }, 6, }, // ROM / Video
    { { MemorySpecialMemory, 0xFFF, 1 }, 7, }, // unusable memory
#elif __arm__ // This needs to be done per-platform specific way

#endif
};

static
VOID MmFixupSystemMemoryMap(PBIOS_MEMORY_MAP BiosMemoryMap, ULONG* MapCount)
{
	int		Index;
	int		Index2;
	ULONGLONG BaseAddressOffset;

	// Loop through each entry in the array
	for (Index=0; Index<*MapCount; Index++)
	{
		// Correct all the addresses to be aligned on page boundaries
		BaseAddressOffset = ROUND_UP(BiosMemoryMap[Index].BaseAddress, MM_PAGE_SIZE) - BiosMemoryMap[Index].BaseAddress;
		BiosMemoryMap[Index].BaseAddress += BaseAddressOffset;
		if (BiosMemoryMap[Index].Length < BaseAddressOffset)
		{
			BiosMemoryMap[Index].Length = 0;
		}
		else
		{
			BiosMemoryMap[Index].Length -= BaseAddressOffset;
		}
		BiosMemoryMap[Index].Length = ROUND_DOWN(BiosMemoryMap[Index].Length, MM_PAGE_SIZE);

		// If the entry type isn't usable then remove
		// it from the memory map (this will help reduce
		// the size of our lookup table)
		// If the length is less than a full page then
		// get rid of it also.
		if (BiosMemoryMap[Index].Type != BiosMemoryUsable ||
			BiosMemoryMap[Index].Length < MM_PAGE_SIZE)
		{
			// Slide every entry after this down one
			for (Index2=Index; Index2<(*MapCount - 1); Index2++)
			{
				BiosMemoryMap[Index2] = BiosMemoryMap[Index2 + 1];
			}
			(*MapCount)--;
			Index--;
		}
	}
}

const MEMORY_DESCRIPTOR*
ArcGetMemoryDescriptor(const MEMORY_DESCRIPTOR* Current)
{
    MEMORY_DESCRIPTOR_INT* CurrentDescriptor;
    BIOS_MEMORY_MAP BiosMemoryMap[32];
    static ULONG BiosMemoryMapEntryCount;
    static MEMORY_DESCRIPTOR_INT BiosMemoryDescriptors[32];
    static BOOLEAN MemoryMapInitialized = FALSE;
    ULONG i, j;

    //
    // Check if it is the first time we're called
    //
    if (!MemoryMapInitialized)
    {
        //
        // Get the machine generated memory map
        //
        RtlZeroMemory(BiosMemoryMap, sizeof(BIOS_MEMORY_MAP) * 32);
        BiosMemoryMapEntryCount = MachVtbl.GetMemoryMap(BiosMemoryMap,
                                                        sizeof(BiosMemoryMap) /
                                                        sizeof(BIOS_MEMORY_MAP));

        //
        // Fix entries that are not page aligned
        //
        MmFixupSystemMemoryMap(BiosMemoryMap, &BiosMemoryMapEntryCount);

        //
        // Copy the entries to our structure
        //
        for (i = 0, j = 0; i < BiosMemoryMapEntryCount; i++)
        {
            //
            // Is it suitable memory?
            //
            if (BiosMemoryMap[i].Type != BiosMemoryUsable)
            {
                //
                // No. Process next descriptor
                //
                continue;
            }

            //
            // Copy this memory descriptor
            //
            BiosMemoryDescriptors[j].m.MemoryType = MemoryFree;
            BiosMemoryDescriptors[j].m.BasePage = (ULONG)(BiosMemoryMap[i].BaseAddress / MM_PAGE_SIZE);
            BiosMemoryDescriptors[j].m.PageCount = (ULONG)(BiosMemoryMap[i].Length / MM_PAGE_SIZE);
            BiosMemoryDescriptors[j].Index = j;
            BiosMemoryDescriptors[j].GeneratedDescriptor = TRUE;
            j++;
        }

        //
        // Remember how much descriptors we found
        //
        BiosMemoryMapEntryCount = j;

        //
        // Mark memory map as already retrieved and initialized
        //
        MemoryMapInitialized = TRUE;
    }

    CurrentDescriptor = CONTAINING_RECORD(Current, MEMORY_DESCRIPTOR_INT, m);

    if (Current == NULL)
    {
        //
        // First descriptor requested
        //
        if (BiosMemoryMapEntryCount > 0)
        {
            //
            // Return first generated memory descriptor
            //
            return &BiosMemoryDescriptors[0].m;
        }
        else if (sizeof(MemoryDescriptors) > 0)
        {
            //
            // Return first fixed memory descriptor
            //
            return &MemoryDescriptors[0].m;
        }
        else
        {
            //
            // Strange case, we have no memory descriptor
            //
            return NULL;
        }
    }
    else if (CurrentDescriptor->GeneratedDescriptor)
    {
        //
        // Current entry is a generated descriptor
        //
        if (CurrentDescriptor->Index + 1 < BiosMemoryMapEntryCount)
        {
            //
            // Return next generated descriptor
            //
            return &BiosMemoryDescriptors[CurrentDescriptor->Index + 1].m;
        }
        else if (sizeof(MemoryDescriptors) > 0)
        {
            //
            // Return first fixed memory descriptor
            //
            return &MemoryDescriptors[0].m;
        }
        else
        {
            //
            // No fixed memory descriptor; end of memory map
            //
            return NULL;
        }
    }
    else
    {
        //
        // Current entry is a fixed descriptor
        //
        if (CurrentDescriptor->Index + 1 < sizeof(MemoryDescriptors) / sizeof(MemoryDescriptors[0]))
        {
            //
            // Return next fixed descriptor
            //
            return &MemoryDescriptors[CurrentDescriptor->Index + 1].m;
        }
        else
        {
            //
            // No more fixed memory descriptor; end of memory map
            //
            return NULL;
        }
    }
}


BOOLEAN MmInitializeMemoryManager(VOID)
{
#if DBG
	const MEMORY_DESCRIPTOR* MemoryDescriptor = NULL;
#endif

	TRACE("Initializing Memory Manager.\n");

#if DBG
	// Dump the system memory map
	TRACE("System Memory Map (Base Address, Length, Type):\n");
	while ((MemoryDescriptor = ArcGetMemoryDescriptor(MemoryDescriptor)) != NULL)
	{
		TRACE("%x\t %x\t %s\n",
			MemoryDescriptor->BasePage * MM_PAGE_SIZE,
			MemoryDescriptor->PageCount * MM_PAGE_SIZE,
			MmGetSystemMemoryMapTypeString(MemoryDescriptor->MemoryType));
	}
#endif

	// Find address for the page lookup table
	TotalPagesInLookupTable = MmGetAddressablePageCountIncludingHoles();
	PageLookupTableAddress = MmFindLocationForPageLookupTable(TotalPagesInLookupTable);
	LastFreePageHint = MmHighestPhysicalPage;

	if (PageLookupTableAddress == 0)
	{
		// If we get here then we probably couldn't
		// find a contiguous chunk of memory big
		// enough to hold the page lookup table
		printf("Error initializing memory manager!\n");
		return FALSE;
	}

	// Initialize the page lookup table
	MmInitPageLookupTable(PageLookupTableAddress, TotalPagesInLookupTable);
	MmUpdateLastFreePageHint(PageLookupTableAddress, TotalPagesInLookupTable);

	FreePagesInLookupTable = MmCountFreePagesInLookupTable(PageLookupTableAddress, TotalPagesInLookupTable);

	MmInitializeHeap(PageLookupTableAddress);

	TRACE("Memory Manager initialized. %d pages available.\n", FreePagesInLookupTable);
	return TRUE;
}

#if DBG
PCSTR MmGetSystemMemoryMapTypeString(MEMORY_TYPE Type)
{
	ULONG		Index;

	for (Index=1; Index<MemoryTypeCount; Index++)
	{
		if (MemoryTypeArray[Index].Type == Type)
		{
			return MemoryTypeArray[Index].TypeString;
		}
	}

	return MemoryTypeArray[0].TypeString;
}
#endif

ULONG MmGetPageNumberFromAddress(PVOID Address)
{
	return ((ULONG_PTR)Address) / MM_PAGE_SIZE;
}

ULONG MmGetAddressablePageCountIncludingHoles(VOID)
{
    const MEMORY_DESCRIPTOR* MemoryDescriptor = NULL;
    ULONG PageCount;

    //
    // Go through the whole memory map to get max address
    //
    while ((MemoryDescriptor = ArcGetMemoryDescriptor(MemoryDescriptor)) != NULL)
    {
        //
        // Check if we got a higher end page address
        //
        if (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount > MmHighestPhysicalPage)
        {
            //
            // Yes, remember it if this is real memory
            //
            if (MemoryDescriptor->MemoryType == MemoryFree) MmHighestPhysicalPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount;
        }

        //
        // Check if we got a higher (usable) start page address
        //
        if (MemoryDescriptor->BasePage < MmLowestPhysicalPage)
        {
            //
            // Yes, remember it if this is real memory
            //
            if (MemoryDescriptor->MemoryType == MemoryFree) MmLowestPhysicalPage = MemoryDescriptor->BasePage;
        }
    }

    TRACE("lo/hi %lx %lxn", MmLowestPhysicalPage, MmHighestPhysicalPage);
    PageCount = MmHighestPhysicalPage - MmLowestPhysicalPage;
    TRACE("MmGetAddressablePageCountIncludingHoles() returning 0x%x\n", PageCount);
    return PageCount;
}

PVOID MmFindLocationForPageLookupTable(ULONG TotalPageCount)
{
    const MEMORY_DESCRIPTOR* MemoryDescriptor = NULL;
    ULONG PageLookupTableSize;
    ULONG PageLookupTablePages;
    ULONG PageLookupTableStartPage = 0;
    PVOID PageLookupTableMemAddress = NULL;

    //
    // Calculate how much pages we need to keep the page lookup table
    //
    PageLookupTableSize = TotalPageCount * sizeof(PAGE_LOOKUP_TABLE_ITEM);
    PageLookupTablePages = PageLookupTableSize / MM_PAGE_SIZE;

    //
    // Search the highest memory block big enough to contain lookup table
    //
    while ((MemoryDescriptor = ArcGetMemoryDescriptor(MemoryDescriptor)) != NULL)
    {
        //
        // Is it suitable memory?
        //
        if (MemoryDescriptor->MemoryType != MemoryFree)
        {
            //
            // No. Process next descriptor
            //
            continue;
        }

        //
        // Is the block big enough?
        //
        if (MemoryDescriptor->PageCount < PageLookupTablePages)
        {
            //
            // No. Process next descriptor
            //
            continue;
        }

        //
        // Is it at a higher address than previous suitable address?
        //
        if (MemoryDescriptor->BasePage < PageLookupTableStartPage)
        {
            //
            // No. Process next descriptor
            //
            continue;
        }

        //
        // Can we use this address?
        //
        if (MemoryDescriptor->BasePage >= MM_MAX_PAGE)
        {
            //
            // No. Process next descriptor
            //
            continue;
        }

        //
        // Memory block is more suitable than the previous one
        //
        PageLookupTableStartPage = MemoryDescriptor->BasePage;
        PageLookupTableMemAddress = (PVOID)((ULONG_PTR)
            (MemoryDescriptor->BasePage + MemoryDescriptor->PageCount) * MM_PAGE_SIZE
            - PageLookupTableSize);
    }

    TRACE("MmFindLocationForPageLookupTable() returning 0x%x\n", PageLookupTableMemAddress);

    return PageLookupTableMemAddress;
}

VOID MmInitPageLookupTable(PVOID PageLookupTable, ULONG TotalPageCount)
{
    const MEMORY_DESCRIPTOR* MemoryDescriptor = NULL;
    TYPE_OF_MEMORY MemoryMapPageAllocated;
    ULONG PageLookupTableStartPage;
    ULONG PageLookupTablePageCount;

    TRACE("MmInitPageLookupTable()\n");

    //
    // Mark every page as allocated initially
    // We will go through and mark pages again according to the memory map
    // But this will mark any holes not described in the map as allocated
    //
    MmMarkPagesInLookupTable(PageLookupTable, MmLowestPhysicalPage, TotalPageCount, LoaderFirmwarePermanent);

    //
    // Parse the whole memory map
    //
    while ((MemoryDescriptor = ArcGetMemoryDescriptor(MemoryDescriptor)) != NULL)
    {
        //
        // Convert ARC memory type to loader memory type
        //
        switch (MemoryDescriptor->MemoryType)
        {
            case MemoryFree:
            {
                //
                // Allocatable memory
                //
                MemoryMapPageAllocated = LoaderFree;
                break;
            }
            case MemoryFirmwarePermanent:
            {
                //
                // Firmware permanent memory
                //
                MemoryMapPageAllocated = LoaderFirmwarePermanent;
                break;
            }
            case MemoryFirmwareTemporary:
            {
                //
                // Firmware temporary memory
                //
                MemoryMapPageAllocated = LoaderFirmwareTemporary;
                break;
            }
            case MemoryLoadedProgram:
            {
                //
                // Bootloader code
                //
                MemoryMapPageAllocated = LoaderLoadedProgram;
                break;
            }
            case MemorySpecialMemory:
            {
                //
                // OS Loader Stack
                //
                MemoryMapPageAllocated = LoaderOsloaderStack;
                break;
            }
            default:
            {
                //
                // Put something sensible here, which won't be overwritten
                //
                MemoryMapPageAllocated = LoaderSpecialMemory;
                break;
            }
        }

        //
        // Mark used pages in the lookup table
        //
        TRACE("Marking pages as type %d: StartPage: %d PageCount: %d\n", MemoryMapPageAllocated, MemoryDescriptor->BasePage, MemoryDescriptor->PageCount);
        MmMarkPagesInLookupTable(PageLookupTable, MemoryDescriptor->BasePage, MemoryDescriptor->PageCount, MemoryMapPageAllocated);
    }

    //
    // Mark the pages that the lookup table occupies as reserved
    //
    PageLookupTableStartPage = MmGetPageNumberFromAddress(PageLookupTable);
    PageLookupTablePageCount = MmGetPageNumberFromAddress((PVOID)((ULONG_PTR)PageLookupTable + ROUND_UP(TotalPageCount * sizeof(PAGE_LOOKUP_TABLE_ITEM), MM_PAGE_SIZE))) - PageLookupTableStartPage;
    TRACE("Marking the page lookup table pages as reserved StartPage: %d PageCount: %d\n", PageLookupTableStartPage, PageLookupTablePageCount);
    MmMarkPagesInLookupTable(PageLookupTable, PageLookupTableStartPage, PageLookupTablePageCount, LoaderFirmwareTemporary);
}

VOID MmMarkPagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount, TYPE_OF_MEMORY PageAllocated)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;

    StartPage -= MmLowestPhysicalPage;
	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
#if 0
		if ((Index <= (StartPage + 16)) || (Index >= (StartPage+PageCount-16)))
		{
			TRACE("Index = %d StartPage = %d PageCount = %d\n", Index, StartPage, PageCount);
		}
#endif
		RealPageLookupTable[Index].PageAllocated = PageAllocated;
		RealPageLookupTable[Index].PageAllocationLength = (PageAllocated != LoaderFree) ? 1 : 0;
	}
	TRACE("MmMarkPagesInLookupTable() Done\n");
}

VOID MmAllocatePagesInLookupTable(PVOID PageLookupTable, ULONG StartPage, ULONG PageCount, TYPE_OF_MEMORY MemoryType)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;

    StartPage -= MmLowestPhysicalPage;
	for (Index=StartPage; Index<(StartPage+PageCount); Index++)
	{
		RealPageLookupTable[Index].PageAllocated = MemoryType;
		RealPageLookupTable[Index].PageAllocationLength = (Index == StartPage) ? PageCount : 0;
	}
}

ULONG MmCountFreePagesInLookupTable(PVOID PageLookupTable, ULONG TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;
	ULONG							FreePageCount;

	FreePageCount = 0;
	for (Index=0; Index<TotalPageCount; Index++)
	{
		if (RealPageLookupTable[Index].PageAllocated == LoaderFree)
		{
			FreePageCount++;
		}
	}

	return FreePageCount;
}

ULONG MmFindAvailablePages(PVOID PageLookupTable, ULONG TotalPageCount, ULONG PagesNeeded, BOOLEAN FromEnd)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							AvailablePagesSoFar;
	ULONG							Index;

	if (LastFreePageHint > TotalPageCount)
	{
		LastFreePageHint = TotalPageCount;
	}

	AvailablePagesSoFar = 0;
	if (FromEnd)
	{
		/* Allocate "high" (from end) pages */
		for (Index=LastFreePageHint-1; Index>0; Index--)
		{
			if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
			{
				AvailablePagesSoFar = 0;
				continue;
			}
			else
			{
				AvailablePagesSoFar++;
			}

			if (AvailablePagesSoFar >= PagesNeeded)
			{
				return Index + MmLowestPhysicalPage;
			}
		}
	}
	else
	{
		TRACE("Alloc low memory, LastFreePageHint %d, TPC %d\n", LastFreePageHint, TotalPageCount);
		/* Allocate "low" pages */
		for (Index=1; Index < LastFreePageHint; Index++)
		{
			if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
			{
				AvailablePagesSoFar = 0;
				continue;
			}
			else
			{
				AvailablePagesSoFar++;
			}

			if (AvailablePagesSoFar >= PagesNeeded)
			{
				return Index - AvailablePagesSoFar + 1 + MmLowestPhysicalPage;
			}
		}
	}

	return 0;
}

ULONG MmFindAvailablePagesBeforePage(PVOID PageLookupTable, ULONG TotalPageCount, ULONG PagesNeeded, ULONG LastPage)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							AvailablePagesSoFar;
	ULONG							Index;

	if (LastPage > TotalPageCount)
	{
		return MmFindAvailablePages(PageLookupTable, TotalPageCount, PagesNeeded, TRUE);
	}

	AvailablePagesSoFar = 0;
	for (Index=LastPage-1; Index>0; Index--)
	{
		if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
		{
			AvailablePagesSoFar = 0;
			continue;
		}
		else
		{
			AvailablePagesSoFar++;
		}

		if (AvailablePagesSoFar >= PagesNeeded)
		{
			return Index + MmLowestPhysicalPage;
		}
	}

	return 0;
}

VOID MmUpdateLastFreePageHint(PVOID PageLookupTable, ULONG TotalPageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							Index;

	for (Index=TotalPageCount-1; Index>0; Index--)
	{
		if (RealPageLookupTable[Index].PageAllocated == LoaderFree)
		{
			LastFreePageHint = Index + 1 + MmLowestPhysicalPage;
			break;
		}
	}
}

BOOLEAN MmAreMemoryPagesAvailable(PVOID PageLookupTable, ULONG TotalPageCount, PVOID PageAddress, ULONG PageCount)
{
	PPAGE_LOOKUP_TABLE_ITEM		RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTable;
	ULONG							StartPage;
	ULONG							Index;

	StartPage = MmGetPageNumberFromAddress(PageAddress);
    StartPage -= MmLowestPhysicalPage;

	// Make sure they aren't trying to go past the
	// end of availabe memory
	if ((StartPage + PageCount) > TotalPageCount)
	{
		return FALSE;
	}

	for (Index=StartPage; Index<(StartPage + PageCount); Index++)
	{
		// If this page is allocated then there obviously isn't
		// memory availabe so return FALSE
		if (RealPageLookupTable[Index].PageAllocated != LoaderFree)
		{
			return FALSE;
		}
	}

	return TRUE;
}
