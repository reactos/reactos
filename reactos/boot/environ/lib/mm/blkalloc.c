/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/blkalloc.c
 * PURPOSE:         Boot Library Memory Manager Block Allocator
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PVOID MmBlockAllocatorTable;
ULONG MmBlockAllocatorTableEntries;
BOOLEAN MmBlockAllocatorInitialized;

typedef struct _BL_BLOCK_DESCRIPTOR
{
    LIST_ENTRY NextEntry;
    UCHAR Unknown[50 - sizeof(LIST_ENTRY)];
} BL_BLOCK_DESCRIPTOR, *PBL_BLOCK_DESCRIPTOR;

/* FUNCTIONS *****************************************************************/

NTSTATUS
MmBaInitialize (
    VOID
    )
{
    NTSTATUS Status;
    ULONG Size;

    /* Allocate 8 table entries */
    MmBlockAllocatorTableEntries = 8;
    Size = sizeof(BL_BLOCK_DESCRIPTOR) * MmBlockAllocatorTableEntries;
    MmBlockAllocatorTable = BlMmAllocateHeap(Size);
    if (MmBlockAllocatorTable)
    {
        /* Zero them out -- we're all done */
        Status = STATUS_SUCCESS;
        RtlZeroMemory(MmBlockAllocatorTable, Size);
        MmBlockAllocatorInitialized = 1;
    }
    else
    {
        /* Bail out since we're out of memory */
        Status = STATUS_NO_MEMORY;
        MmBlockAllocatorInitialized = 0;
    }

    /* Return initialization status */
    return Status;
}
