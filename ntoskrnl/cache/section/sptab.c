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
 * FILE:            ntoskrnl/cache/section/sptab.c
 * PURPOSE:         Section object page tables
 *
 * PROGRAMMERS:     arty
 */

/*

This file implements the section page table.  It relies on rtl generic table
functionality to provide access to 256-page chunks.  Calls to
MiSetPageEntrySectionSegment and MiGetPageEntrySectionSegment must be
synchronized by holding the segment lock.

Each page table entry is a ULONG as in x86.

Bit 1 is used as a swap entry indication as in the main page table.
Bit 2 is used as a dirty indication.  A dirty page will eventually be written
back to the file.
Bits 3-11 are used as a map count in the legacy mm code, Note that zero is
illegal, as the legacy code does not take advantage of segment rmaps.
Therefore, every segment page is mapped in at least one address space, and
MmUnsharePageEntry is quite complicated.  In addition, the page may also be
owned by the legacy cache manager, giving an implied additional reference.
Upper bits are a PFN_NUMBER.

These functions, in addition to maintaining the segment page table also
automatically maintain the segment rmap by calling MmSetSectionAssociation
and MmDeleteSectionAssociation.  Segment rmaps are discussed in rmap.c.  The
upshot is that it is impossible to have a page properly registered in a segment
page table and not also found in a segment rmap that can be found from the
paging machinery.

*/

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#define NDEBUG
#include <debug.h>

#define DPRINTC DPRINT

/* TYPES *********************************************************************/

extern KSPIN_LOCK MiSectionPageTableLock;

_Function_class_(RTL_GENERIC_ALLOCATE_ROUTINE)
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

_Function_class_(RTL_GENERIC_FREE_ROUTINE)
static
VOID
NTAPI
MiSectionPageTableFree(PRTL_GENERIC_TABLE Table, PVOID Data)
{
    //DPRINT("MiSectionPageTableFree(%p)\n", Data);
    ExFreePoolWithTag(Data, 'MmPt');
}

_Function_class_(RTL_GENERIC_COMPARE_ROUTINE)
static
RTL_GENERIC_COMPARE_RESULTS
NTAPI
MiSectionPageTableCompare(PRTL_GENERIC_TABLE Table,
                          PVOID PtrA,
                          PVOID PtrB)
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
MiSectionPageTableGet(PRTL_GENERIC_TABLE Table,
                      PLARGE_INTEGER FileOffset)
{
    LARGE_INTEGER SearchFileOffset;
    PCACHE_SECTION_PAGE_TABLE PageTable;
    SearchFileOffset.QuadPart = ROUND_DOWN(FileOffset->QuadPart,
                                           ENTRIES_PER_ELEMENT * PAGE_SIZE);
    PageTable = RtlLookupElementGenericTable(Table, &SearchFileOffset);

    DPRINT("MiSectionPageTableGet(%p,%I64x)\n",
           Table,
           FileOffset->QuadPart);

    return PageTable;
}

static
PCACHE_SECTION_PAGE_TABLE
NTAPI
MiSectionPageTableGetOrAllocate(PRTL_GENERIC_TABLE Table,
                                PLARGE_INTEGER FileOffset)
{
    LARGE_INTEGER SearchFileOffset;
    CACHE_SECTION_PAGE_TABLE SectionZeroPageTable;
    PCACHE_SECTION_PAGE_TABLE PageTableSlice = MiSectionPageTableGet(Table,
                                                                     FileOffset);
    /* Please zero memory when taking away zero initialization. */
    RtlZeroMemory(&SectionZeroPageTable, sizeof(CACHE_SECTION_PAGE_TABLE));
    if (!PageTableSlice)
    {
        SearchFileOffset.QuadPart = ROUND_DOWN(FileOffset->QuadPart,
                                               ENTRIES_PER_ELEMENT * PAGE_SIZE);
        SectionZeroPageTable.FileOffset = SearchFileOffset;
        SectionZeroPageTable.Refcount = 1;
        PageTableSlice = RtlInsertElementGenericTable(Table,
                                                      &SectionZeroPageTable,
                                                      sizeof(SectionZeroPageTable),
                                                      NULL);
        if (!PageTableSlice) return NULL;
        DPRINT("Allocate page table %p (%I64x)\n",
               PageTableSlice,
               PageTableSlice->FileOffset.QuadPart);
    }
    return PageTableSlice;
}

VOID
NTAPI
MiInitializeSectionPageTable(PMM_SECTION_SEGMENT Segment)
{
    RtlInitializeGenericTable(&Segment->PageTable,
                              MiSectionPageTableCompare,
                              MiSectionPageTableAllocate,
                              MiSectionPageTableFree,
                              NULL);

    DPRINT("MiInitializeSectionPageTable(%p)\n", &Segment->PageTable);
}

NTSTATUS
NTAPI
_MmSetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                              PLARGE_INTEGER Offset,
                              ULONG_PTR Entry,
                              const char *file,
                              int line)
{
    ULONG_PTR PageIndex, OldEntry;
    PCACHE_SECTION_PAGE_TABLE PageTable;

    ASSERT(Segment->Locked);
    ASSERT(!IS_SWAP_FROM_SSE(Entry) || !IS_DIRTY_SSE(Entry));

    if (Entry && !IS_SWAP_FROM_SSE(Entry))
        MmGetRmapListHeadPage(PFN_FROM_SSE(Entry));

    PageTable = MiSectionPageTableGetOrAllocate(&Segment->PageTable, Offset);

    if (!PageTable) return STATUS_NO_MEMORY;

    ASSERT(MiSectionPageTableGet(&Segment->PageTable, Offset));

    PageTable->Segment = Segment;
    PageIndex = (ULONG_PTR)((Offset->QuadPart - PageTable->FileOffset.QuadPart) / PAGE_SIZE);
    OldEntry = PageTable->PageEntries[PageIndex];

    DPRINT("MiSetPageEntrySectionSegment(%p,%08x%08x,%x=>%x)\n",
            Segment,
            Offset->u.HighPart,
            Offset->u.LowPart,
            OldEntry,
            Entry);

    if (PFN_FROM_SSE(Entry) == PFN_FROM_SSE(OldEntry)) {
        /* Nothing */
    } else if (Entry && !IS_SWAP_FROM_SSE(Entry)) {
        ASSERT(!OldEntry || IS_SWAP_FROM_SSE(OldEntry));
        MmSetSectionAssociation(PFN_FROM_SSE(Entry), Segment, Offset);
    } else if (OldEntry && !IS_SWAP_FROM_SSE(OldEntry)) {
        ASSERT(!Entry || IS_SWAP_FROM_SSE(Entry));
        MmDeleteSectionAssociation(PFN_FROM_SSE(OldEntry));
    } else if (IS_SWAP_FROM_SSE(Entry)) {
        ASSERT(!IS_SWAP_FROM_SSE(OldEntry) ||
               SWAPENTRY_FROM_SSE(OldEntry) == MM_WAIT_ENTRY);
        if (OldEntry && SWAPENTRY_FROM_SSE(OldEntry) != MM_WAIT_ENTRY)
            MmDeleteSectionAssociation(PFN_FROM_SSE(OldEntry));
    } else if (IS_SWAP_FROM_SSE(OldEntry)) {
        ASSERT(!IS_SWAP_FROM_SSE(Entry));
        if (Entry)
            MmSetSectionAssociation(PFN_FROM_SSE(OldEntry), Segment, Offset);
    } else {
        /* We should not be replacing a page like this */
        ASSERT(FALSE);
    }
    PageTable->PageEntries[PageIndex] = Entry;
    return STATUS_SUCCESS;
}

ULONG_PTR
NTAPI
_MmGetPageEntrySectionSegment(PMM_SECTION_SEGMENT Segment,
                              PLARGE_INTEGER Offset,
                              const char *file,
                              int line)
{
    LARGE_INTEGER FileOffset;
    ULONG_PTR PageIndex, Result;
    PCACHE_SECTION_PAGE_TABLE PageTable;

    ASSERT(Segment->Locked);
    FileOffset.QuadPart = ROUND_DOWN(Offset->QuadPart,
                                     ENTRIES_PER_ELEMENT * PAGE_SIZE);
    PageTable = MiSectionPageTableGet(&Segment->PageTable, &FileOffset);
    if (!PageTable) return 0;
    PageIndex = (ULONG_PTR)((Offset->QuadPart - PageTable->FileOffset.QuadPart) / PAGE_SIZE);
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

/*

Destroy the rtl generic table that serves as the section's page table.  Call
the FreePage function for each non-zero entry in the section page table as
we go.  Note that the page table is still techinally valid until after all
pages are destroyed, as we don't finally destroy the table until we've free
each slice.  There is no order guarantee for deletion of individual elements
although it's in-order as written now.

*/

VOID
NTAPI
MmFreePageTablesSectionSegment(PMM_SECTION_SEGMENT Segment,
                               FREE_SECTION_PAGE_FUN FreePage)
{
    PCACHE_SECTION_PAGE_TABLE Element;
    DPRINT("MiFreePageTablesSectionSegment(%p)\n", &Segment->PageTable);
    while ((Element = RtlGetElementGenericTable(&Segment->PageTable, 0))) {
        DPRINT("Delete table for <%wZ> %p -> %I64x\n",
               Segment->FileObject ? &Segment->FileObject->FileName : NULL,
               Segment,
               Element->FileOffset.QuadPart);
        if (FreePage)
        {
            ULONG i;
            for (i = 0; i < ENTRIES_PER_ELEMENT; i++)
            {
                ULONG_PTR Entry;
                LARGE_INTEGER Offset;
                Offset.QuadPart = Element->FileOffset.QuadPart + i * PAGE_SIZE;
                Entry = Element->PageEntries[i];
                if (Entry && !IS_SWAP_FROM_SSE(Entry))
                {
                    DPRINT("Freeing page %p:%Ix @ %I64x\n",
                           Segment,
                           Entry,
                           Offset.QuadPart);

                    FreePage(Segment, &Offset);
                }
            }
        }
        DPRINT("Remove memory\n");
        RtlDeleteElementGenericTable(&Segment->PageTable, Element);
    }
    DPRINT("Done\n");
}

/*

Retrieves the MM_SECTION_SEGMENT and fills in the LARGE_INTEGER Offset given
by the caller that corresponds to the page specified.  This uses
MmGetSegmentRmap to find the rmap belonging to the segment itself, and uses
the result as a pointer to a 256-entry page table structure.  The rmap also
includes 8 bits of offset information indication one of 256 page entries that
the rmap corresponds to.  This information together gives us an exact offset
into the file, as well as the MM_SECTION_SEGMENT pointer stored in the page
table slice.

NULL is returned is there is no segment rmap for the page.

*/

PMM_SECTION_SEGMENT
NTAPI
MmGetSectionAssociation(PFN_NUMBER Page,
                        PLARGE_INTEGER Offset)
{
    ULONG RawOffset;
    PMM_SECTION_SEGMENT Segment = NULL;
    PCACHE_SECTION_PAGE_TABLE PageTable;

    PageTable = (PCACHE_SECTION_PAGE_TABLE)MmGetSegmentRmap(Page,
                                                            &RawOffset);
    if (PageTable)
    {
        Segment = PageTable->Segment;
        Offset->QuadPart = PageTable->FileOffset.QuadPart +
                           ((ULONG64)RawOffset << PAGE_SHIFT);
    }

    return Segment;
}

NTSTATUS
NTAPI
MmSetSectionAssociation(PFN_NUMBER Page,
                        PMM_SECTION_SEGMENT Segment,
                        PLARGE_INTEGER Offset)
{
    PCACHE_SECTION_PAGE_TABLE PageTable;
    ULONG ActualOffset;

    PageTable = MiSectionPageTableGet(&Segment->PageTable, Offset);
    ASSERT(PageTable);

    ActualOffset = (ULONG)(Offset->QuadPart - PageTable->FileOffset.QuadPart);
    MmInsertRmap(Page,
                 (PEPROCESS)PageTable,
                 (PVOID)(RMAP_SEGMENT_MASK | (ActualOffset >> PAGE_SHIFT)));

    return STATUS_SUCCESS;
}
