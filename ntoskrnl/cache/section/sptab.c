/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Section object page tables
 *
 * PROGRAMMERS:     arty
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#define NDEBUG
#include <debug.h>

#define DPRINTC DPRINT

/* TYPES *********************************************************************/

extern KSPIN_LOCK MiSectionPageTableLock;

static
PVOID
NTAPI
MiSectionPageTableAllocate(PRTL_GENERIC_TABLE Table, CLONG Bytes)
{
    PVOID Result;
    Result = ExAllocatePoolWithTag(NonPagedPool, Bytes, 'MmPt');
    //DPRINT("MiSectionPageTableAllocate(%d) => %p\n", Bytes, Result);
    return Result;
}

static
VOID
NTAPI
MiSectionPageTableFree(PRTL_GENERIC_TABLE Table, PVOID Data)
{
    //DPRINT("MiSectionPageTableFree(%p)\n", Data);
    ExFreePoolWithTag(Data, 'MmPt');
}

static
RTL_GENERIC_COMPARE_RESULTS
NTAPI
MiSectionPageTableCompare(PRTL_GENERIC_TABLE Table, PVOID PtrA, PVOID PtrB)
{
    PLARGE_INTEGER A = PtrA, B = PtrB;
    BOOLEAN Result = (A->QuadPart < B->QuadPart) ? GenericLessThan : 
        (A->QuadPart == B->QuadPart) ? GenericEqual : GenericGreaterThan;

#if 0
    DPRINT
        ("Compare: %08x%08x vs %08x%08x => %s\n",
         A->u.HighPart, A->u.LowPart,
         B->u.HighPart, B->u.LowPart,
         Result == GenericLessThan ? "GenericLessThan" :
         Result == GenericGreaterThan ? "GenericGreaterThan" : 
            "GenericEqual");
#endif

    return Result;
}

static
PCACHE_SECTION_PAGE_TABLE
NTAPI
MiSectionPageTableGet
(PRTL_GENERIC_TABLE Table, 
 PLARGE_INTEGER FileOffset)
{
    LARGE_INTEGER SearchFileOffset;
	PCACHE_SECTION_PAGE_TABLE PageTable;
    SearchFileOffset.QuadPart = ROUND_DOWN(FileOffset->QuadPart, ENTRIES_PER_ELEMENT * PAGE_SIZE);
    PageTable = RtlLookupElementGenericTable(Table, &SearchFileOffset);
	DPRINT
		("MiSectionPageTableGet(%08x,%08x%08x)\n",
		 Table,
		 FileOffset->HighPart,
		 FileOffset->LowPart);
	return PageTable;
}

static
PCACHE_SECTION_PAGE_TABLE
NTAPI
MiSectionPageTableGetOrAllocate
(PRTL_GENERIC_TABLE Table, 
 PLARGE_INTEGER FileOffset)
{
    LARGE_INTEGER SearchFileOffset;
    CACHE_SECTION_PAGE_TABLE SectionZeroPageTable;
    PCACHE_SECTION_PAGE_TABLE PageTableSlice = 
        MiSectionPageTableGet(Table, FileOffset);
    // Please zero memory when taking away zero initialization.
    RtlZeroMemory(&SectionZeroPageTable, sizeof(CACHE_SECTION_PAGE_TABLE));
    if (!PageTableSlice)
    {
        SearchFileOffset.QuadPart = ROUND_DOWN(FileOffset->QuadPart, ENTRIES_PER_ELEMENT * PAGE_SIZE);
        SectionZeroPageTable.FileOffset = SearchFileOffset;
        SectionZeroPageTable.Refcount = 1;
        PageTableSlice = RtlInsertElementGenericTable
            (Table, &SectionZeroPageTable, sizeof(SectionZeroPageTable), NULL);
        DPRINT
            ("Allocate page table %x (%08x%08x)\n", 
             PageTableSlice,
             PageTableSlice->FileOffset.u.HighPart,
             PageTableSlice->FileOffset.u.LowPart);
        if (!PageTableSlice) return NULL;
    }
    return PageTableSlice;
}

VOID
NTAPI
MiInitializeSectionPageTable(PMM_SECTION_SEGMENT Segment)
{
    RtlInitializeGenericTable
        (&Segment->PageTable,
         MiSectionPageTableCompare,
         MiSectionPageTableAllocate,
         MiSectionPageTableFree,
         NULL);
    DPRINT("MiInitializeSectionPageTable(%p)\n", &Segment->PageTable);
}

NTSTATUS
NTAPI
_MmSetPageEntrySectionSegment
(PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER Offset,
 ULONG Entry,
 const char *file,
 int line)
{
    ULONG PageIndex, OldEntry;
    PCACHE_SECTION_PAGE_TABLE PageTable;
	ASSERT(Segment->Locked);
	if (Entry && !IS_SWAP_FROM_SSE(Entry))
		MmGetRmapListHeadPage(PFN_FROM_SSE(Entry));
    PageTable = 
        MiSectionPageTableGetOrAllocate(&Segment->PageTable, Offset);
    if (!PageTable) return STATUS_NO_MEMORY;
	ASSERT(MiSectionPageTableGet(&Segment->PageTable, Offset));
	PageTable->Segment = Segment;
    PageIndex = 
        (Offset->QuadPart - PageTable->FileOffset.QuadPart) / PAGE_SIZE;
	OldEntry = PageTable->PageEntries[PageIndex];
	DPRINT("MiSetPageEntrySectionSegment(%p,%08x%08x,%x=>%x)\n", 
			Segment, Offset->u.HighPart, Offset->u.LowPart, OldEntry, Entry);
	if (PFN_FROM_SSE(Entry) == PFN_FROM_SSE(OldEntry)) {
		// Nothing
	} else if (Entry && !IS_SWAP_FROM_SSE(Entry)) {
		ASSERT(!OldEntry || IS_SWAP_FROM_SSE(OldEntry));
		MmSetSectionAssociation(PFN_FROM_SSE(Entry), Segment, Offset);
	} else if (OldEntry && !IS_SWAP_FROM_SSE(OldEntry)) {
		ASSERT(!Entry || IS_SWAP_FROM_SSE(Entry));
		MmDeleteSectionAssociation(PFN_FROM_SSE(OldEntry));
	} else if (IS_SWAP_FROM_SSE(Entry)) {
		ASSERT(!IS_SWAP_FROM_SSE(OldEntry));
		if (OldEntry)
			MmDeleteSectionAssociation(PFN_FROM_SSE(OldEntry));
	} else if (IS_SWAP_FROM_SSE(OldEntry)) {
		ASSERT(!IS_SWAP_FROM_SSE(Entry));
		if (Entry)
			MmSetSectionAssociation(PFN_FROM_SSE(OldEntry), Segment, Offset);
	} else {
		// We should not be replacing a page like this
		ASSERT(FALSE);
	}
    PageTable->PageEntries[PageIndex] = Entry;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
_MmGetPageEntrySectionSegment
(PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER Offset,
 const char *file,
 int line)
{
    LARGE_INTEGER FileOffset;
    ULONG PageIndex, Result;
    PCACHE_SECTION_PAGE_TABLE PageTable;

    ASSERT(Segment->Locked);
    FileOffset.QuadPart = 
        ROUND_DOWN(Offset->QuadPart, ENTRIES_PER_ELEMENT * PAGE_SIZE);
    PageTable = MiSectionPageTableGet(&Segment->PageTable, &FileOffset);
    if (!PageTable) return 0;
    PageIndex = 
        (Offset->QuadPart - PageTable->FileOffset.QuadPart) / PAGE_SIZE;
    Result = PageTable->PageEntries[PageIndex];
#if 0
    DPRINTC
        ("MiGetPageEntrySectionSegment(%p,%08x%08x) => %x %s:%d\n",
         Segment,
         FileOffset.u.HighPart, 
         FileOffset.u.LowPart + PageIndex * PAGE_SIZE,
         Result,
		 file, line);
#endif
    return Result;
}

VOID
NTAPI
MmFreePageTablesSectionSegment
(PMM_SECTION_SEGMENT Segment, FREE_SECTION_PAGE_FUN FreePage)
{
    PCACHE_SECTION_PAGE_TABLE Element;
    DPRINT("MiFreePageTablesSectionSegment(%p)\n", &Segment->PageTable);
    while ((Element = RtlGetElementGenericTable(&Segment->PageTable, 0))) {
        DPRINT
            ("Delete table for <%wZ> %x -> %08x%08x\n", 
			 Segment->FileObject ? &Segment->FileObject->FileName : NULL,
			 Segment,
             Element->FileOffset.u.HighPart, 
             Element->FileOffset.u.LowPart);
		if (FreePage)
		{
			int i;
			for (i = 0; i < ENTRIES_PER_ELEMENT; i++)
			{
				ULONG Entry;
				LARGE_INTEGER Offset;
				Offset.QuadPart = Element->FileOffset.QuadPart + i * PAGE_SIZE;
				Entry = Element->PageEntries[i];
				if (Entry && !IS_SWAP_FROM_SSE(Entry))
				{
					DPRINT("Freeing page %x:%x @ %x\n", Segment, Entry, Offset.LowPart);
					FreePage(Segment, &Offset);
				}
			}
		}
		DPRINT("Remove memory\n");
        RtlDeleteElementGenericTable(&Segment->PageTable, Element);
    }
	DPRINT("Done\n");
}

PMM_SECTION_SEGMENT
NTAPI
MmGetSectionAssociation(PFN_NUMBER Page, PLARGE_INTEGER Offset)
{
	ULONG RawOffset;
    PMM_SECTION_SEGMENT Segment = NULL;
    PCACHE_SECTION_PAGE_TABLE PageTable;
	
	PageTable = (PCACHE_SECTION_PAGE_TABLE)MmGetSegmentRmap(Page, &RawOffset);
	if (PageTable)
	{
		Segment = PageTable->Segment;
		Offset->QuadPart = PageTable->FileOffset.QuadPart + (RawOffset << PAGE_SHIFT);
	}

    return(Segment);
}

NTSTATUS
NTAPI
MmSetSectionAssociation(PFN_NUMBER Page, PMM_SECTION_SEGMENT Segment, PLARGE_INTEGER Offset)
{
    PCACHE_SECTION_PAGE_TABLE PageTable;
	ULONG ActualOffset;
	PageTable = MiSectionPageTableGet(&Segment->PageTable, Offset);
	ASSERT(PageTable);
	ActualOffset = (ULONG)(Offset->QuadPart - PageTable->FileOffset.QuadPart);
	MmInsertRmap(Page, (PEPROCESS)PageTable, (PVOID)(RMAP_SEGMENT_MASK | (ActualOffset >> PAGE_SHIFT)));
	return STATUS_SUCCESS;
}
