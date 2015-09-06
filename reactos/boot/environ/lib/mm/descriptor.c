/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/mm/descriptor.c
 * PURPOSE:         Boot Library Memory Manager Descriptor Manager
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

BL_MEMORY_DESCRIPTOR MmStaticMemoryDescriptors[512];
ULONG MmGlobalMemoryDescriptorCount;
PBL_MEMORY_DESCRIPTOR MmGlobalMemoryDescriptors;
BOOLEAN MmGlobalMemoryDescriptorsUsed;
PBL_MEMORY_DESCRIPTOR MmDynamicMemoryDescriptors;
ULONG MmDynamicMemoryDescriptorCount;

/* FUNCTIONS *****************************************************************/

VOID
MmMdpSwitchToDynamicDescriptors (
    _In_ ULONG Count
    )
{
    EarlyPrint(L"NOT SUPPORTED!!!\n");
    while (1);
}

NTSTATUS
MmMdFreeDescriptor (
    _In_ PBL_MEMORY_DESCRIPTOR MemoryDescriptor
    )
{
    NTSTATUS Status;

    /* Check if this is a valid static descriptor */
    if (((MmDynamicMemoryDescriptors) &&
        (MemoryDescriptor >= MmDynamicMemoryDescriptors) &&
        (MemoryDescriptor < (MmDynamicMemoryDescriptors + MmDynamicMemoryDescriptorCount))) ||
        ((MemoryDescriptor >= MmStaticMemoryDescriptors) && (MemoryDescriptor < &MmStaticMemoryDescriptors[511])))
    {
        /* It's a global/static descriptor, so just zero it */
        RtlZeroMemory(MemoryDescriptor, sizeof(BL_MEMORY_DESCRIPTOR));
        Status = STATUS_SUCCESS;
    }
    else
    {
        /* It's a dynamic descriptor, so free it */
        EarlyPrint(L"Dynamic descriptors not yet supported\n");
        Status = STATUS_NOT_IMPLEMENTED;
    }

    /* Done */
    return Status;
}

VOID
MmMdpSaveCurrentListPointer (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList,
    _In_ PLIST_ENTRY Current
    )
{
    /* Make sure that this is not a global descriptor and not the first one */
    if (((Current < &MmGlobalMemoryDescriptors->ListEntry) ||
        (Current >= &MmGlobalMemoryDescriptors[MmGlobalMemoryDescriptorCount].ListEntry)) &&
        (Current != MdList->First))
    {
        /* Save this as the current pointer */
        MdList->This = Current;
    }
}

VOID
MmMdRemoveDescriptorFromList (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList,
    _In_ PBL_MEMORY_DESCRIPTOR Entry
    )
{
    /* Remove the entry */
    RemoveEntryList(&Entry->ListEntry);

    /*  Check if this was the current link */
    if (MdList->This == &Entry->ListEntry)
    {
        /* Remove the current link and set the next one */
        MdList->This = NULL;
        MmMdpSaveCurrentListPointer(MdList, Entry->ListEntry.Blink);
    }
}

VOID
MmMdFreeList(
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList
    )
{
    PLIST_ENTRY FirstEntry, NextEntry;
    PBL_MEMORY_DESCRIPTOR Entry;

    /* Go over every descriptor from the top */
    FirstEntry = MdList->First;
    NextEntry = FirstEntry->Flink;
    while (NextEntry != FirstEntry)
    {
        /* Remove and free each one */
        Entry = CONTAINING_RECORD(NextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);
        NextEntry = NextEntry->Flink;
        MmMdRemoveDescriptorFromList(MdList, Entry);
        MmMdFreeDescriptor(Entry);
    }
}

VOID
MmMdInitialize (
    _In_ ULONG Phase,
    _In_ PBL_LIBRARY_PARAMETERS LibraryParameters
    )
{
    /* Are we in phase 1? */
    if (Phase != 0)
    {
        /* Switch to dynamic descriptors if we have too many */
        if (LibraryParameters->DescriptorCount > RTL_NUMBER_OF(MmStaticMemoryDescriptors))
        {
            MmMdpSwitchToDynamicDescriptors(LibraryParameters->DescriptorCount);
        }
    }
    else
    {
        /* In phase 0, start with a pool of 512 static descriptors */
        MmGlobalMemoryDescriptorCount = RTL_NUMBER_OF(MmStaticMemoryDescriptors);
        MmGlobalMemoryDescriptors = MmStaticMemoryDescriptors;
        RtlZeroMemory(MmStaticMemoryDescriptors, sizeof(MmStaticMemoryDescriptors));
        MmGlobalMemoryDescriptorsUsed = FALSE;
    }
}
