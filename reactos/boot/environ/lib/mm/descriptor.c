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

BL_MEMORY_TYPE MmPlatformMemoryTypePrecedence[] =
{
    BlReservedMemory,
    BlUnusableMemory,
    BlDeviceIoMemory,
    BlDevicePortMemory,
    BlPalMemory,
    BlEfiRuntimeMemory,
    BlAcpiNvsMemory,
    BlAcpiReclaimMemory,
    BlEfiBootMemory
};

/* FUNCTIONS *****************************************************************/

/* The order is Conventional > Other > System > Loader > Application  */
BOOLEAN
MmMdpHasPrecedence (
    _In_ BL_MEMORY_TYPE Type1,
    _In_ BL_MEMORY_TYPE Type2
    )
{
    BL_MEMORY_CLASS Class1, Class2;
    ULONG i, j;

    /* Descriptor is free RAM -- it preceeds */
    if (Type1 == BlConventionalMemory)
    {
        return TRUE;
    }

    /* It isn't free RAM, but the comparator is -- it suceeds it */
    if (Type2 == BlConventionalMemory)
    {
        return FALSE;
    }

    /* Descriptor is not system, application, or loader class -- it preceeds */
    Class1 = Type1 >> BL_MEMORY_CLASS_SHIFT;
    if ((Class1 != BlSystemClass) &&
        (Class1 != BlApplicationClass) &&
        (Class1 != BlLoaderClass))
    {
        return TRUE;
    }

    /* It isn't one of those classes, but the comparator it -- it suceeds it */
    Class2 = Type2 >> BL_MEMORY_CLASS_SHIFT;
    if ((Class2 != BlSystemClass) &&
        (Class2 != BlApplicationClass) &&
        (Class2 != BlLoaderClass))
    {
        return FALSE;
    }

    /* Descriptor is system class */
    if (Class1 == BlSystemClass)
    {
        /* And so is the other guy... */
        if (Class2 == BlSystemClass)
        {
            i = 0;
            j = 0;

            /* Scan for the descriptor's system precedence index */
            do
            {
                if (MmPlatformMemoryTypePrecedence[j] == Type1)
                {
                    break;
                }
            } while (++j < RTL_NUMBER_OF(MmPlatformMemoryTypePrecedence));

            /* Use an invalid index if one wasn't found */
            if (j == RTL_NUMBER_OF(MmPlatformMemoryTypePrecedence))
            {
                j = 0xFFFFFFFF;
            }

            /* Now scan for the comparator's system precedence index */
            while (MmPlatformMemoryTypePrecedence[i] != Type2)
            {
                /* Use an invalid index if one wasn't found */
                if (++i >= RTL_NUMBER_OF(MmPlatformMemoryTypePrecedence))
                {
                    i = 0xFFFFFFFF;
                    break;
                }
            }

            /* Does the current have a valid index? */
            if (j != 0xFFFFFFFF)
            {
                /* Yes, what about the comparator? */
                if (i != 0xFFFFFFFF)
                {
                    /* Let the indexes fight! */
                    return i >= j;
                }

                /* Succeed the comparator, its index is unknown */
                return FALSE;
            }
        }

        /* The comparator isn't system, so it preceeds it */
        return TRUE;
    }

    /* Descriptor is not system class, but comparator is -- it suceeds it */
    if (Class2 == BlSystemClass)
    {
        return FALSE;
    }

    /* Descriptor is loader class -- it preceeds */
    if (Class1 == BlLoaderClass)
    {
        return TRUE;
    }

    /* It isn't loader class  -- if the other guy is, suceed it */
    return Class2 != BlLoaderClass;
}

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

PBL_MEMORY_DESCRIPTOR
MmMdInitByteGranularDescriptor (
    _In_ ULONG Flags,
    _In_ BL_MEMORY_TYPE Type,
    _In_ ULONGLONG BasePage,
    _In_ ULONGLONG VirtualPage,
    _In_ ULONGLONG PageCount
    )
{
    PBL_MEMORY_DESCRIPTOR MemoryDescriptor;

    /* If we're out of descriptors, bail out */
    if (MmGlobalMemoryDescriptorsUsed >= MmGlobalMemoryDescriptorCount)
    {
        EarlyPrint(L"Out of descriptors!\n");
        return NULL;
    }

    /* Take one of the available descriptors and fill it out */
    MemoryDescriptor = &MmGlobalMemoryDescriptors[MmGlobalMemoryDescriptorsUsed];
    MemoryDescriptor->BaseAddress = BasePage;
    MemoryDescriptor->VirtualPage = VirtualPage;
    MemoryDescriptor->PageCount = PageCount;
    MemoryDescriptor->Flags = Flags;
    MemoryDescriptor->Type = Type;
    InitializeListHead(&MemoryDescriptor->ListEntry);

    /* Increment the count and return the descriptor */
    MmGlobalMemoryDescriptorsUsed++;
    return MemoryDescriptor;
}

BOOLEAN
MmMdpTruncateDescriptor (
    __in PBL_MEMORY_DESCRIPTOR_LIST MdList,
    __in PBL_MEMORY_DESCRIPTOR MemoryDescriptor,
    __in ULONG Flags
    )
{
    PBL_MEMORY_DESCRIPTOR NextDescriptor, PreviousDescriptor;
    PLIST_ENTRY NextEntry, PreviousEntry;
    ULONGLONG EndPage, PreviousEndPage;// , NextEndPage;

    /* Get the next descriptor */
    NextEntry = MemoryDescriptor->ListEntry.Flink;
    NextDescriptor = CONTAINING_RECORD(NextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

    /* Get the previous descriptor */
    PreviousEntry = MemoryDescriptor->ListEntry.Blink;
    PreviousDescriptor = CONTAINING_RECORD(PreviousEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

    /* Calculate end pages */
    EndPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount;
    //NextEndPage = NextDescriptor->BasePage + NextDescriptor->PageCount;
    PreviousEndPage = PreviousDescriptor->BasePage + PreviousDescriptor->PageCount;

    /* Check for backward overlap */
    if ((PreviousEntry != MdList->First) && (MemoryDescriptor->BasePage < PreviousEndPage))
    {
        EarlyPrint(L"Overlap detected -- this is unexpected on x86/x64 platforms\n");
    }

    /* Check for forward overlap */
    if ((NextEntry != MdList->First) && (NextDescriptor->BasePage < EndPage))
    {
        EarlyPrint(L"Overlap detected -- this is unexpected on x86/x64 platforms\n");
    }

    /* Nothing to do */
    return FALSE;
}

BOOLEAN
MmMdpCoalesceDescriptor (
    __in PBL_MEMORY_DESCRIPTOR_LIST MdList,
    __in PBL_MEMORY_DESCRIPTOR MemoryDescriptor,
    __in ULONG Flags
    )
{
    PBL_MEMORY_DESCRIPTOR NextDescriptor, PreviousDescriptor;
    PLIST_ENTRY NextEntry, PreviousEntry;
    ULONGLONG EndPage, PreviousEndPage, PreviousMappedEndPage, MappedEndPage;

    /* Get the next descriptor */
    NextEntry = MemoryDescriptor->ListEntry.Flink;
    NextDescriptor = CONTAINING_RECORD(NextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

    /* Get the previous descriptor */
    PreviousEntry = MemoryDescriptor->ListEntry.Blink;
    PreviousDescriptor = CONTAINING_RECORD(PreviousEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

    /* Calculate end pages */
    EndPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount;
    MappedEndPage = MemoryDescriptor->BasePage + MemoryDescriptor->PageCount;
    PreviousMappedEndPage = PreviousDescriptor->VirtualPage + PreviousDescriptor->PageCount;
    PreviousEndPage = PreviousDescriptor->BasePage + PreviousDescriptor->PageCount;
    PreviousMappedEndPage = PreviousDescriptor->VirtualPage + PreviousDescriptor->PageCount;

    /* Check if the previous entry touches the current entry, and is compatible */
    if ((PreviousEntry != MdList->First) &&
        (PreviousDescriptor->Type == MemoryDescriptor->Type) &&
        ((PreviousDescriptor->Flags ^ MemoryDescriptor->Flags) & 0x1B19FFFF) &&
        (PreviousEndPage == MemoryDescriptor->BasePage) &&
        ((!(MemoryDescriptor->VirtualPage) && !(PreviousDescriptor->VirtualPage)) ||
          ((MemoryDescriptor->VirtualPage) && (PreviousDescriptor->VirtualPage) &&
           (PreviousMappedEndPage == MemoryDescriptor->VirtualPage))))
    {
        EarlyPrint(L"Previous descriptor coalescible!\n");
    }

    /* CHeck if the current entry touches the next entry, and is compatible */
    if ((NextEntry != MdList->First) &&
        (NextDescriptor->Type == MemoryDescriptor->Type) &&
        ((NextDescriptor->Flags ^ MemoryDescriptor->Flags) & 0x1B19FFFF) &&
        (EndPage == NextDescriptor->BasePage) &&
        ((!(MemoryDescriptor->VirtualPage) && !(PreviousDescriptor->VirtualPage)) ||
            ((MemoryDescriptor->VirtualPage) && (PreviousDescriptor->VirtualPage) &&
                (MappedEndPage == NextDescriptor->VirtualPage))))
    {
        EarlyPrint(L"Next descriptor coalescible!\n");
    }

    /* Nothing to do */
    return FALSE;
}

NTSTATUS
MmMdAddDescriptorToList (
    _In_ PBL_MEMORY_DESCRIPTOR_LIST MdList,
    _In_ PBL_MEMORY_DESCRIPTOR MemoryDescriptor,
    _In_ ULONG Flags
    )
{
    PLIST_ENTRY ThisEntry, FirstEntry;
    PBL_MEMORY_DESCRIPTOR ThisDescriptor;

    /* Arguments must be present */
    if (!(MdList) || !(MemoryDescriptor))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if coalescing is forcefully disabled */
    if (Flags & BL_MM_ADD_DESCRIPTOR_NEVER_COALESCE_FLAG)
    {
        /* Then we won't be coalescing */
        Flags &= BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG;
    }
    else
    {
        /* Coalesce if the descriptor requires it */
        if (MemoryDescriptor->Flags & BL_MM_DESCRIPTOR_REQUIRES_COALESCING_FLAG)
        {
            Flags |= BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG;
        }
    }

    /* Check if truncation is forcefully disabled */
    if (Flags & BL_MM_ADD_DESCRIPTOR_NEVER_TRUNCATE_FLAG)
    {
        Flags &= ~BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG;
    }

    /* Update the current list pointer if the descriptor requires it */
    if (MemoryDescriptor->Flags & BL_MM_DESCRIPTOR_REQUIRES_UPDATING_FLAG)
    {
        Flags |= BL_MM_ADD_DESCRIPTOR_UPDATE_LIST_POINTER_FLAG;
    }

    /* Get the current descriptor */
    ThisEntry = MdList->This;
    ThisDescriptor = CONTAINING_RECORD(ThisEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

    /* Also get the first descriptor */
    FirstEntry = MdList->First;

    /* Check if there's no current pointer, or if it's higher than the new one */
    if (!(ThisEntry) ||
        (MemoryDescriptor->BaseAddress <= ThisDescriptor->BaseAddress))
    {
        /* Start at the first descriptor instead, since current is past us */
        ThisEntry = FirstEntry->Flink;
        ThisDescriptor = CONTAINING_RECORD(ThisEntry, BL_MEMORY_DESCRIPTOR, ListEntry);
    }

    /* Loop until we find the right location to insert */
    while (1)
    {
        /* Have we gotten back to the first entry? */
        if (ThisEntry == FirstEntry)
        {
            /* Then we didn't find a good match, so insert it right here */
            InsertTailList(FirstEntry, &MemoryDescriptor->ListEntry);

            /* Do we have to truncate? */
            if (Flags & BL_MM_ADD_DESCRIPTOR_TRUNCATE_FLAG)
            {
                /* Do it and then exit */
                if (MmMdpTruncateDescriptor(MdList, MemoryDescriptor, Flags))
                {
                    return STATUS_SUCCESS;
                }
            }

            /* Do we have to coalesce? */
            if (Flags & BL_MM_ADD_DESCRIPTOR_COALESCE_FLAG)
            {
                /* Do it and then exit */
                if (MmMdpCoalesceDescriptor(MdList, MemoryDescriptor, Flags))
                {
                    return STATUS_SUCCESS;
                }
            }

            /* Do we have to update the current pointer? */
            if (Flags & BL_MM_ADD_DESCRIPTOR_UPDATE_LIST_POINTER_FLAG)
            {
                /* Do it */
                MmMdpSaveCurrentListPointer(MdList, &MemoryDescriptor->ListEntry);
            }

            /* We're done */
            return STATUS_SUCCESS;
        }

        /* Is the new descriptor below this address, and has precedence over it? */
        if ((MemoryDescriptor->BaseAddress < ThisDescriptor->BaseAddress) &&
            (MmMdpHasPrecedence(MemoryDescriptor->Type, ThisDescriptor->Type)))
        {
            /* Then insert right here */
            InsertTailList(ThisEntry, &MemoryDescriptor->ListEntry);
            return STATUS_SUCCESS;
        }

        /* Try the next descriptor */
        ThisEntry = ThisEntry->Flink;
        ThisDescriptor = CONTAINING_RECORD(ThisEntry, BL_MEMORY_DESCRIPTOR, ListEntry);
    }
}

NTSTATUS
MmMdRemoveRegionFromMdlEx (
    __in PBL_MEMORY_DESCRIPTOR_LIST MdList,
    __in ULONG Flags,
    __in ULONGLONG BasePage,
    __in ULONGLONG PageCount,
    __in PBL_MEMORY_DESCRIPTOR_LIST NewMdList
    )
{
    return STATUS_NOT_IMPLEMENTED;
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
