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
    EfiPrintf(L"NOT SUPPORTED!!!\r\n");
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
        EfiPrintf(L"Dynamic descriptors not yet supported\r\n");
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
        EfiPrintf(L"Out of descriptors!\r\n");
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
        EfiPrintf(L"Overlap detected -- this is unexpected on x86/x64 platforms\r\n");
    }

    /* Check for forward overlap */
    if ((NextEntry != MdList->First) && (NextDescriptor->BasePage < EndPage))
    {
        EfiPrintf(L"Overlap detected -- this is unexpected on x86/x64 platforms\r\n");
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
        EfiPrintf(L"Previous descriptor coalescible!\r\n");
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
        EfiPrintf(L"Next descriptor coalescible!\r\n");
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
        if (MemoryDescriptor->Flags & BlMemoryCoalesced)
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
    if (MemoryDescriptor->Flags & BlMemoryUpdate)
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
    BOOLEAN HaveNewList, UseVirtualPage;
    NTSTATUS Status;
    PLIST_ENTRY ListHead, NextEntry;
    PBL_MEMORY_DESCRIPTOR Descriptor;
    BL_MEMORY_DESCRIPTOR NewDescriptor;
    ULONGLONG RegionSize;
    ULONGLONG FoundBasePage, FoundEndPage, FoundPageCount, EndPage;

    /* Set initial status */
    Status = STATUS_SUCCESS;

    /* Check if removed descriptors should go into a new list */
    if (NewMdList != NULL)
    {
        /* Initialize it */
        MmMdInitializeListHead(NewMdList);
        NewMdList->Type = MdList->Type;

        /* Remember for later */
        HaveNewList = TRUE;
    }
    else
    {
        /* For later */
        HaveNewList = FALSE;
    }

    /* Is the region being removed physical? */
    UseVirtualPage = FALSE;
    if (!(Flags & BL_MM_REMOVE_VIRTUAL_REGION_FLAG))
    {
        /* Is this a list of virtual descriptors? */
        if (MdList->Type == BlMdVirtual)
        {
            /* Request is nonsensical, fail */
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }
    else
    {
        /* Is this a list of physical descriptors? */
        if (MdList->Type == BlMdPhysical)
        {
            /* We'll have to use the virtual page instead */
            UseVirtualPage = TRUE;
        }
    }

    /* Loop the list*/
    ListHead = MdList->First;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the descriptor */
        Descriptor = CONTAINING_RECORD(NextEntry, BL_MEMORY_DESCRIPTOR, ListEntry);

        /* Extract range details */
        FoundBasePage = UseVirtualPage ? Descriptor->VirtualPage : Descriptor->BasePage;
        FoundPageCount = Descriptor->PageCount;
        FoundEndPage = FoundBasePage + FoundPageCount;
        EndPage = PageCount + BasePage;
        //EarlyPrint(L"Looking for Region 0x%08I64X-0x%08I64X in 0x%08I64X-0x%08I64X\r\n", BasePage, EndPage, FoundBasePage, FoundEndPage);

        /* Make a copy of the original descriptor */
        RtlCopyMemory(&NewDescriptor, NextEntry, sizeof(NewDescriptor));

        /* Check if the region to be removed starts after the found region starts */
        if ((BasePage > FoundBasePage) || (FoundBasePage >= EndPage))
        {
            /* Check if the region ends after the found region */
            if ((BasePage >= FoundEndPage) || (FoundEndPage > EndPage))
            {
                /* Check if the found region starts after the region or ends before the region */
                if ((FoundBasePage >= BasePage) || (EndPage >= FoundEndPage))
                {
                    /* This descriptor doesn't cover any part of the range */
                    //EarlyPrint(L"No part of this descriptor contains the region\r\n");
                }
                else
                {
                    /* This descriptor covers the head of the allocation */
                    //EarlyPrint(L"Descriptor covers the head of the region\r\n");
                }
            }
            else
            {
                /* This descriptor contains the entire allocation */
                //EarlyPrint(L"Descriptor contains the entire region\r\n");
            }

            /* Keep going */
            NextEntry = NextEntry->Flink;
        }
        else
        {
            /*
             * This descriptor contains the end of the allocation. It may:
             *
             * - Contain the full allocation (i.e.: the start is aligned)
             * - Contain parts of the end of the allocation (i.e.: the end is beyond)
             * - Contain the entire tail end of the allocation (i..e:the end is within)
             *
             * So first, figure out if we cover the entire end or not
             */
            if (EndPage > FoundEndPage)
            {
                /* The allocation goes past the end of this descriptor */
                EndPage = FoundEndPage;
            }

            /* This is how many pages we will eat away from the descriptor */
            RegionSize = EndPage - FoundBasePage;

            /* Update the descriptor to account for the consumed pages */
            Descriptor->BasePage += RegionSize;
            Descriptor->PageCount -= RegionSize;
            if (Descriptor->VirtualPage)
            {
                Descriptor->VirtualPage += RegionSize;
            }

            /* Go to the next entry */
            NextEntry = NextEntry->Flink;

            /* Check if the descriptor is now empty */
            if (!Descriptor->PageCount)
            {
                /* Remove it */
                //EarlyPrint(L"Entire descriptor consumed\r\n");
                MmMdRemoveDescriptorFromList(MdList, Descriptor);
                MmMdFreeDescriptor(Descriptor);

                /* Check if we're supposed to insert it into a new list */
                if (HaveNewList)
                {
                    EfiPrintf(L"Not yet implemented\r\n");
                    Status = STATUS_NOT_IMPLEMENTED;
                    goto Quickie;
                }
            }
        }
    }

Quickie:
    /* Check for failure cleanup */
    if (!NT_SUCCESS(Status))
    {
        /* Did we have to build a new list? */
        if (HaveNewList)
        {
            /* Free and re-initialize it */
            MmMdFreeList(NewMdList);
            MmMdInitializeListHead(NewMdList);
            NewMdList->Type = MdList->Type;
        }
    }

    return Status;
}

BOOLEAN
MmMdFindSatisfyingRegion (
    _In_ PBL_MEMORY_DESCRIPTOR Descriptor,
    _Out_ PBL_MEMORY_DESCRIPTOR NewDescriptor,
    _In_ ULONGLONG Pages,
    _In_ PBL_ADDRESS_RANGE BaseRange,
    _In_ PBL_ADDRESS_RANGE VirtualRange,
    _In_ BOOLEAN TopDown,
    _In_ BL_MEMORY_TYPE MemoryType,
    _In_ ULONG Flags,
    _In_ ULONG Alignment
    )
{
    ULONGLONG BaseMin, BaseMax;
    ULONGLONG VirtualPage, BasePage;

    /* Extract the minimum and maximum range */
    BaseMin = BaseRange->Minimum;
    BaseMax = BaseRange->Maximum;

    /* Don't go below where the descriptor starts */
    if (BaseMin < Descriptor->BasePage)
    {
        BaseMin = Descriptor->BasePage;
    }

    /* Don't go beyond where the descriptor ends */
    if (BaseMax > (Descriptor->BasePage + Descriptor->PageCount - 1))
    {
        BaseMax = (Descriptor->BasePage + Descriptor->PageCount - 1);
    }

    /* Check for start overflow */
    if (BaseMin > BaseMax)
    {
        EfiPrintf(L"Descriptor overflow\r\n");
        return FALSE;
    }

    /* Align the base as required */
    if (Alignment != 1)
    {
        BaseMin = ALIGN_UP_BY(BaseMin, Alignment);
    }

    /* Check for range overflow */
    if (((BaseMin + Pages - 1) < BaseMin) || ((BaseMin + Pages - 1) > BaseMax))
    {
        return FALSE;
    }

    /* Check if this was a top-down request */
    if (TopDown)
    {
        /* Then get the highest page possible */
        BasePage = BaseMax - Pages + 1;
        if (Alignment != 1)
        {
            /* Align it as needed */
            BasePage = ALIGN_DOWN_BY(BasePage, Alignment);
        }
    }
    else
    {
        /* Otherwise, get the lowest page possible */
        BasePage = BaseMin;
    }

    /* If a virtual address range was passed in, this must be a virtual descriptor */
    if (((VirtualRange->Minimum) || (VirtualRange->Maximum)) &&
        !(Descriptor->VirtualPage))
    {
        return FALSE;
    }

    /* Any mapped page already? */
    if (Descriptor->VirtualPage)
    {
        EfiPrintf(L"Virtual memory not yet supported\r\n");
        return FALSE;
    }
    else
    {
        /* Nothing to worry about */
        VirtualPage = 0;
    }

    /* Bail out if the memory type attributes don't match */
    if ((((Flags & 0xFF) & (Descriptor->Flags & 0xFF)) != (Flags & 0xFF)) ||
        (((Flags & 0xFF00) & (Descriptor->Flags & 0xFF00)) != (Flags & 0xFF00)))
    {
        EfiPrintf(L"Incorrect memory attributes\r\n");
        return FALSE;
    }

    /* Bail out if the allocation flags don't match */
    if (((Flags ^ Descriptor->Flags) & 0x190000))
    {
        EfiPrintf(L"Incorrect memory allocation flags\r\n");
        return FALSE;
    }

    /* Bail out if the type doesn't match */
    if (Descriptor->Type != MemoryType)
    {
        //EarlyPrint(L"Incorrect descriptor type\r\n");
        return FALSE;
    }

    /* We have a matching region, fill out the descriptor for it */
    NewDescriptor->BasePage = BasePage;
    NewDescriptor->PageCount = Pages;
    NewDescriptor->Type = Descriptor->Type;
    NewDescriptor->VirtualPage = VirtualPage;
    NewDescriptor->Flags = Descriptor->Flags;
    //EarlyPrint(L"Found a matching descriptor: %08I64X with %08I64X pages\r\n", BasePage, Pages);
    return TRUE;
}

VOID
MmMdFreeGlobalDescriptors (
    VOID
    )
{
    PBL_MEMORY_DESCRIPTOR Descriptor, OldDescriptor;
    ULONG Index = 0;
    PLIST_ENTRY OldFlink, OldBlink;

    /* Make sure we're not int middle of a call using a descriptor */
    if (MmDescriptorCallTreeCount != 1)
    {
        return;
    }

    /* Loop every current global descriptor */
    while (Index < MmGlobalMemoryDescriptorsUsed)
    {
        /* Does it have any valid pageS? */
        OldDescriptor = &MmGlobalMemoryDescriptors[Index];
        if (OldDescriptor->PageCount)
        {
            /* Allocate a copy of it */
            Descriptor = BlMmAllocateHeap(sizeof(*Descriptor));
            if (!Descriptor)
            {
                return;
            }

            /* Save the links */
            OldFlink = OldDescriptor->ListEntry.Blink;
            OldBlink = OldDescriptor->ListEntry.Flink;

            /* Make the copy */
            *Descriptor = *OldDescriptor;

            /* Fix the links */
            OldBlink->Flink = &Descriptor->ListEntry;
            OldFlink->Blink = &Descriptor->ListEntry;

            /* Zero the descriptor */
            RtlZeroMemory(OldDescriptor, sizeof(*OldDescriptor));
        }

        /* Keep going */
        Index++;
    }

    /* All global descriptors freed */
    MmGlobalMemoryDescriptorsUsed = 0;
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
