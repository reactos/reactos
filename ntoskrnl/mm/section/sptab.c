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

#define ENTRIES_PER_ELEMENT 244

PVOID NTAPI RtlGetElementGenericTable(PRTL_GENERIC_TABLE Table, PVOID Data);
ULONG NTAPI RtlNumberGenericTableElements(PRTL_GENERIC_TABLE Table);

typedef struct _SECTION_PAGE_TABLE 
{
    LARGE_INTEGER FileOffset;
    ULONG Refcount;
    ULONG PageEntries[ENTRIES_PER_ELEMENT];
} SECTION_PAGE_TABLE, *PSECTION_PAGE_TABLE;

SECTION_PAGE_TABLE MiSectionZeroPageTable = { };

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
    SearchFileOffset.QuadPart = ROUND_DOWN(FileOffset->QuadPart, ENTRIES_PER_ELEMENT * PAGE_SIZE);
    return RtlLookupElementGenericTable(Table, &SearchFileOffset);
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
        SearchFileOffset.QuadPart = ROUND_DOWN(FileOffset->QuadPart, ENTRIES_PER_ELEMENT * PAGE_SIZE);
        MiSectionZeroPageTable.FileOffset = SearchFileOffset;
        MiSectionZeroPageTable.Refcount = 1;
        PageTableSlice = RtlInsertElementGenericTable
            (Table, &MiSectionZeroPageTable, sizeof(MiSectionZeroPageTable), NULL);
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
    PageIndex = 
        (Offset->QuadPart - PageTable->FileOffset.QuadPart) / PAGE_SIZE;
    PageTable->PageEntries[PageIndex] = Entry;
    DPRINT
        ("MiSetPageEntrySectionSegment(%p,%08x%08x,%x)\n",
         &Segment->PageTable, Offset->u.HighPart, Offset->u.LowPart, Entry);
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
    DPRINT
        ("MiGetPageEntrySectionSegment(%p,%08x%08x) => %x\n",
         &Segment->PageTable, 
         FileOffset.u.HighPart, 
         FileOffset.u.LowPart,
         Result);
    return Result;
}

VOID
NTAPI
MiFreePageTablesSectionSegment(PMM_SECTION_SEGMENT Segment)
{
    PSECTION_PAGE_TABLE Element;
    DPRINT("MiFreePageTablesSectionSegment(%p)\n", &Segment->PageTable);
    while (RtlNumberGenericTableElements(&Segment->PageTable) &&
           (Element = RtlGetElementGenericTable(&Segment->PageTable, 0)))
    {
        DPRINT
            ("Delete table for %08x%08x\n", 
             Element->FileOffset.u.HighPart, 
             Element->FileOffset.u.LowPart);
        RtlDeleteElementGenericTable(&Segment->PageTable, Element);
    }
}
