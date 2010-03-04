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
#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

/* We store 8 bits of location with a page association */
#define ENTRIES_PER_ELEMENT 256
								   
typedef struct _SECTION_PAGE_TABLE 
{
    LARGE_INTEGER FileOffset;
	PMM_SECTION_SEGMENT Segment;
    ULONG Refcount;
    ULONG PageEntries[ENTRIES_PER_ELEMENT];
} SECTION_PAGE_TABLE, *PSECTION_PAGE_TABLE;

static
PVOID
NTAPI
MiSectionPageTableAllocate(PRTL_GENERIC_TABLE Table, CLONG Bytes)
{
    PVOID Result;
    Result = ExAllocatePoolWithTag(NonPagedPool, Bytes, 'MmPt');
    DPRINT("MiSectionPageTableAllocate(%d) => %p\n", Bytes, Result);
    return Result;
}

static
VOID
NTAPI
MiSectionPageTableFree(PRTL_GENERIC_TABLE Table, PVOID Data)
{
    DPRINT("MiSectionPageTableFree(%p)\n", Data);
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

    DPRINT
        ("Compare: %08x%08x vs %08x%08x => %s\n",
         A->u.HighPart, A->u.LowPart,
         B->u.HighPart, B->u.LowPart,
         Result == GenericLessThan ? "GenericLessThan" :
         Result == GenericGreaterThan ? "GenericGreaterThan" : 
            "GenericEqual");

    return Result;
}

static
PSECTION_PAGE_TABLE
NTAPI
MiSectionPageTableGet
(PRTL_GENERIC_TABLE Table, 
 PLARGE_INTEGER FileOffset)
{
    LARGE_INTEGER SearchFileOffset;
	PSECTION_PAGE_TABLE PageTable;
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
PSECTION_PAGE_TABLE
NTAPI
MiSectionPageTableGetOrAllocate
(PRTL_GENERIC_TABLE Table, 
 PLARGE_INTEGER FileOffset)
{
    LARGE_INTEGER SearchFileOffset;
    PSECTION_PAGE_TABLE PageTableSlice = 
        MiSectionPageTableGet(Table, FileOffset);
    if (!PageTableSlice)
    {
		SECTION_PAGE_TABLE SectionZeroPageTable = { };
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
MiSetPageEntrySectionSegment
(PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER Offset,
 ULONG Entry)
{
    ULONG PageIndex;
    PSECTION_PAGE_TABLE PageTable;
    PageTable = 
        MiSectionPageTableGetOrAllocate(&Segment->PageTable, Offset);
    if (!PageTable) return STATUS_NO_MEMORY;
	PageTable->Segment = Segment;
    PageIndex = 
        (Offset->QuadPart - PageTable->FileOffset.QuadPart) / PAGE_SIZE;
    PageTable->PageEntries[PageIndex] = Entry;
	if (Entry && !IS_SWAP_FROM_SSE(Entry))
		MmSetSectionAssociation(PFN_FROM_SSE(Entry), Segment, Offset);
    DPRINT1
        ("MiSetPageEntrySectionSegment(%p,%08x%08x,%x)\n",
         &Segment, Offset->u.HighPart, Offset->u.LowPart, Entry);
    return STATUS_SUCCESS;
}

ULONG
NTAPI
MiGetPageEntrySectionSegment
(PMM_SECTION_SEGMENT Segment,
 PLARGE_INTEGER Offset)
{
    LARGE_INTEGER FileOffset;
    ULONG PageIndex, Result;
    PSECTION_PAGE_TABLE PageTable;

    FileOffset.QuadPart = 
        ROUND_DOWN(Offset->QuadPart, ENTRIES_PER_ELEMENT * PAGE_SIZE);
    PageTable = MiSectionPageTableGet(&Segment->PageTable, &FileOffset);
    if (!PageTable) return 0;
    PageIndex = 
        (Offset->QuadPart - PageTable->FileOffset.QuadPart) / PAGE_SIZE;
    Result = PageTable->PageEntries[PageIndex];
    DPRINT1
        ("MiGetPageEntrySectionSegment(%p,%08x%08x) => %x\n",
         &Segment->PageTable, 
         FileOffset.u.HighPart, 
         FileOffset.u.LowPart,
         Result);
    return Result;
}

VOID
NTAPI
MiFreePageTablesSectionSegment
(PMM_SECTION_SEGMENT Segment, FREE_SECTION_PAGE_FUN FreePage)
{
    PSECTION_PAGE_TABLE Element;
    DPRINT("MiFreePageTablesSectionSegment(%p)\n", &Segment->PageTable);
    while (RtlNumberGenericTableElements(&Segment->PageTable) &&
           (Element = RtlGetElementGenericTable(&Segment->PageTable, 0)))
    {
        DPRINT
            ("Delete table for %x -> %08x%08x\n", 
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
					DPRINT1("Freeing page %x:%x @ %x\n", Segment, Entry, Offset.LowPart);
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
MmGetSectionAssociation(PFN_TYPE Page, PLARGE_INTEGER Offset)
{
    KIRQL oldIrql;
    PMMPFN Pfn;
    PMM_SECTION_SEGMENT Segment = NULL;
    PSECTION_PAGE_TABLE PageTable;

    oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    Pfn = MiGetPfnEntry(Page);
    if (Pfn)
    {
		DPRINT("Pfn %x\n", Pfn);
		DPRINT("Page Location %x\n", Pfn->u3.e1.PageLocation);
		PageTable = (PVOID)((ULONG_PTR)Pfn->u2.SegmentPart & ~3);
		DPRINT("PageTable %x\n", PageTable);
		if (PageTable)
		{
			Segment = PageTable->Segment;
			DPRINT("ShortFlags %x Raw Seg %x\n", Pfn->u3.e2.ShortFlags, Pfn->u2.SegmentPart);
			Offset->QuadPart = PageTable->FileOffset.QuadPart +
				(((((ULONG_PTR)Pfn->u2.SegmentPart & 3) << 6) |
				  ((Pfn->u3.e2.ShortFlags >> 10) & 0x30) | 
				  (Pfn->u3.e2.ShortFlags & 0xf)) << PAGE_SHIFT);
			DPRINT("Final offset %x\n", Offset->LowPart);
		}
    }
    KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);

    return(Segment);
}

NTSTATUS
NTAPI
MmSetSectionAssociation(PFN_TYPE Page, PMM_SECTION_SEGMENT Segment, PLARGE_INTEGER Offset)
{
    PMMPFN Pfn;
    KIRQL oldIrql;
    USHORT SmallSize;
    PSECTION_PAGE_TABLE PageTable;

	DPRINT1("MmSetSectionAssociation %x %x %x\n", Page, Segment, Offset->LowPart);
    oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    Pfn = MiGetPfnEntry(Page);
	DPRINT("Pfn %x\n", Pfn);
	if (!Pfn)
	{
		DPRINT1("Page %x fake for %x:%x\n", Page, Segment, Offset->LowPart);
		ASSERT(FALSE);
	}
    PageTable = 
        MiSectionPageTableGetOrAllocate(&Segment->PageTable, Offset);
    if (!PageTable) return STATUS_NO_MEMORY;
	PageTable->Segment = Segment;
	DPRINT("Page Table %x\n", PageTable);
    SmallSize = (Offset->QuadPart >> PAGE_SHIFT) & 0xff;
	DPRINT("SmallSize %x\n", SmallSize);
    Pfn->u2.SegmentPart = 
		(PVOID)((ULONG_PTR)PageTable + ((SmallSize >> 6) & 3));
    Pfn->u3.e2.ShortFlags = (Pfn->u3.e2.ShortFlags & 0x3ff0) |
        ((SmallSize & 0x30) << 10) |
        (SmallSize & 0xf);
	DPRINT("Pfn->u2.SegmentPart %x\n", Pfn->u2.SegmentPart);
	DPRINT("Short Flags %x\n", Pfn->u3.e2.ShortFlags);
    KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
	DPRINT("MmSetSectionAssociation done\n");
	return STATUS_SUCCESS;
}

VOID
NTAPI
MmDeleteSectionAssociation(PFN_TYPE Page)
{
    PMMPFN Pfn;
    KIRQL oldIrql;
	DPRINT1("MmDeleteSectionAssociation(%x)\n", Page);
    oldIrql = KeAcquireQueuedSpinLock(LockQueuePfnLock);
    Pfn = MiGetPfnEntry(Page);
	Pfn->u2.SegmentPart = NULL;
    KeReleaseQueuedSpinLock(LockQueuePfnLock, oldIrql);
}
