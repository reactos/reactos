/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Source file for private memory functions specific to SMP
 * COPYRIGHT:  Copyright 2021 Justin Miller <justinmiller100@gmail.com>
 */

/* INCLUDES ******************************************************************/

#include <hal.h>
#include <smp.h>
#include "smpp.h"
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* AP spinup stub specific */
extern PVOID APEntry;
extern PVOID APEntryEnd;
extern PVOID APSpinup;
extern PVOID APSpinupEnd;
extern UINT16 APJumpOffset;
extern UINT16 PageTableLocation;

/* Pagetables specific */
#define MM_PAGE_SIZE     4096
#define MM_PAGE_SHIFT    12
#define SELFMAP_ENTRY    0x300

PFN_NUMBER TotalPagesInLookupTable = 0;
PHARDWARE_PTE PDE;
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))

/* FUNCTIONS *****************************************************************/

VOID
HalpInitializeAPStub(PVOID APStubLocation)
{
    PVOID APStubSecondPhaseLoc;
    PVOID APJumppLoc;
    PVOID APPageLoc;

    /* Get the locations used to copy over */
    APJumppLoc = (PUSHORT)((ULONG_PTR)APStubLocation + (ULONG_PTR)&APJumpOffset - (ULONG_PTR)&APEntry); 
    APPageLoc = (PUSHORT)(((ULONG_PTR)APStubLocation) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry) +
        ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
    APStubSecondPhaseLoc = (PVOID)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    APJumpOffset = (UINT16)(((ULONG_PTR)APStubLocation * 4) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry));
    PageTableLocation = (UINT16)(((ULONG_PTR)APStubLocation * 4) + ((ULONG_PTR)&APEntryEnd  - (ULONG_PTR)&APEntry) +
        ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
    /* Copy over the bootstub for specific AP */
    RtlCopyMemory(APStubLocation, &APEntry,  ((ULONG_PTR)&APEntryEnd - (ULONG_PTR)&APEntry));
    RtlCopyMemory(APStubSecondPhaseLoc, &APSpinup,  ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
    RtlCopyMemory(APJumppLoc, &APJumpOffset,  sizeof(APJumpOffset));
    RtlCopyMemory(APPageLoc, &PageTableLocation, sizeof(PageTableLocation));

}

VOID
HalpInitalizeAPPageTable(PVOID APStubLocation)
{
   ULONG NumPageTables, TotalSize;
   PUCHAR Buffer;
   PVOID HalpAfterSpinupLoc;

   NumPageTables = TotalPagesInLookupTable >> 10;

   // Allocate memory block for all these things:
   // PDE, HAL mapping page table, physical mapping, kernel mapping
   TotalSize = (1 + 1 + NumPageTables * 2) * MM_PAGE_SIZE;

   HalpAfterSpinupLoc = (PVOID)((ULONG_PTR)APStubLocation + ((ULONG_PTR)&APEntryEnd - (ULONG_PTR)&APEntry) + 
        ((ULONG_PTR)&APSpinupEnd - (ULONG_PTR)&APSpinup));
   Buffer = (PUCHAR)HalpAfterSpinupLoc;
     // Zero all this memory block
   RtlZeroMemory(Buffer, TotalSize);

   // Set up pointers correctly now
   PDE = (PHARDWARE_PTE)Buffer;

   // Map the page directory at 0xC0000000 (maps itself)
   PDE[SELFMAP_ENTRY].PageFrameNumber = (ULONG)PDE >> MM_PAGE_SHIFT;
   PDE[SELFMAP_ENTRY].Valid = 1;
   PDE[SELFMAP_ENTRY].Write = 1;

   RtlCopyMemory(HalpAfterSpinupLoc, &PDE, TotalSize);
}