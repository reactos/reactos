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

PMEMORY_DESCRIPTOR BiosMemoryMap;
ULONG BiosMemoryMapEntryCount;

extern ULONG_PTR	MmHeapPointer;
extern ULONG_PTR	MmHeapStart;

ULONG
AddMemoryDescriptor(
    IN OUT PMEMORY_DESCRIPTOR List,
    IN ULONG MaxCount,
    IN PFN_NUMBER BasePage,
    IN PFN_NUMBER PageCount,
    IN MEMORY_TYPE MemoryType)
{
    ULONG i, c;
    PFN_NUMBER NextBase;
    TRACE("AddMemoryDescriptor(0x%lx-0x%lx [0x%lx pages])\n",
          BasePage, BasePage + PageCount, PageCount);

    /* Scan through all existing descriptors */
    for (i = 0, c = 0; (c < MaxCount) && (List[c].PageCount != 0); c++)
    {
        /* Count entries completely below the new range */
        if (List[i].BasePage + List[i].PageCount <= BasePage) i++;
    }

    /* Check if the list is full */
    if (c >= MaxCount) return c;

    /* Is there an existing descriptor starting before the new range */
    while ((i < c) && (List[i].BasePage <= BasePage))
    {
        /* The end of the existing one is the minimum for the new range */
        NextBase = List[i].BasePage + List[i].PageCount;

        /* Bail out, if everything is trimmed away */
        if ((BasePage + PageCount) <= NextBase) return c;

        /* Trim the naew range at the lower end */
        PageCount -= (NextBase - BasePage);
        BasePage = NextBase;

        /* Go to the next entry and repeat */
        i++;
    }

    ASSERT(PageCount > 0);

    /* Are there still entries above? */
    if (i < c)
    {
        /* Shift the following entries one up */
        RtlMoveMemory(&List[i+1], &List[i], (c - i) * sizeof(List[0]));

        /* Insert the new range */
        List[i].BasePage = BasePage;
        List[i].PageCount = min(PageCount, List[i+1].BasePage - BasePage);
        List[i].MemoryType = MemoryType;
        c++;

        TRACE("Inserting at i=%ld: (0x%lx:0x%lx)\n",
              i, List[i].BasePage, List[i].PageCount);

        /* Check if the range was trimmed */
        if (PageCount > List[i].PageCount)
        {
            /* Recursively process the trimmed part */
            c = AddMemoryDescriptor(List,
                                    MaxCount,
                                    BasePage + List[i].PageCount,
                                    PageCount - List[i].PageCount,
                                    MemoryType);
        }
    }
    else
    {
        /* We can simply add the range here */
        TRACE("Adding i=%ld: (0x%lx:0x%lx)\n", i, BasePage, PageCount);
        List[i].BasePage = BasePage;
        List[i].PageCount = PageCount;
        List[i].MemoryType = MemoryType;
        c++;
    }

    /* Return the new count */
    return c;
}

const MEMORY_DESCRIPTOR*
ArcGetMemoryDescriptor(const MEMORY_DESCRIPTOR* Current)
{
    if (Current == NULL)
    {
        return BiosMemoryMap;
    }
    else
    {
        Current++;
        if (Current->PageCount == 0) return NULL;
        return Current;
    }
}


BOOLEAN MmInitializeMemoryManager(VOID)
{
#if DBG
	const MEMORY_DESCRIPTOR* MemoryDescriptor = NULL;
#endif

	TRACE("Initializing Memory Manager.\n");

    BiosMemoryMap = MachVtbl.GetMemoryMap(&BiosMemoryMapEntryCount);

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

{
    ULONG Type, Index, PrevIndex = 0;
	PPAGE_LOOKUP_TABLE_ITEM RealPageLookupTable = (PPAGE_LOOKUP_TABLE_ITEM)PageLookupTableAddress;

    Type = RealPageLookupTable[0].PageAllocated;
	for (Index = 1; Index < TotalPagesInLookupTable; Index++)
	{
	    if ((RealPageLookupTable[Index].PageAllocated != Type) ||
            (Index == TotalPagesInLookupTable - 1))
	    {
            TRACE("Range: 0x%lx - 0x%lx Type=%d\n",
                PrevIndex, Index - 1, Type);
	        Type = RealPageLookupTable[Index].PageAllocated;
	        PrevIndex = Index;
	    }
	}
}


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
            MmLowestPhysicalPage = MemoryDescriptor->BasePage;
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
        TRACE("Got range: 0x%lx-0x%lx, type=%s\n",
              MemoryDescriptor->BasePage,
              MemoryDescriptor->BasePage + MemoryDescriptor->PageCount,
              MmGetSystemMemoryMapTypeString(MemoryDescriptor->MemoryType));
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
	TRACE("MmMarkPagesInLookupTable()\n");

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

	if (StartPage < MmLowestPhysicalPage) return FALSE;

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
