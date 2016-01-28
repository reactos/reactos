/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/region.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     David Welch
 */

/* INCLUDE *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

static VOID
InsertAfterEntry(PLIST_ENTRY Previous,
                 PLIST_ENTRY Entry)
/*
 * FUNCTION: Insert a list entry after another entry in the list
 */
{
    Previous->Flink->Blink = Entry;

    Entry->Flink = Previous->Flink;
    Entry->Blink = Previous;

    Previous->Flink = Entry;
}

static PMM_REGION
MmSplitRegion(PMM_REGION InitialRegion, PVOID InitialBaseAddress,
              PVOID StartAddress, SIZE_T Length, ULONG NewType,
              ULONG NewProtect, PMMSUPPORT AddressSpace,
              PMM_ALTER_REGION_FUNC AlterFunc)
{
    PMM_REGION NewRegion1;
    PMM_REGION NewRegion2;
    SIZE_T InternalLength;

    /* Allocate this in front otherwise the failure case is too difficult. */
    NewRegion2 = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_REGION),
                                       TAG_MM_REGION);
    if (NewRegion2 == NULL)
    {
        return(NULL);
    }

    /* Create the new region. */
    NewRegion1 = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_REGION),
                                       TAG_MM_REGION);
    if (NewRegion1 == NULL)
    {
        ExFreePoolWithTag(NewRegion2, TAG_MM_REGION);
        return(NULL);
    }
    NewRegion1->Type = NewType;
    NewRegion1->Protect = NewProtect;
    InternalLength = ((char*)InitialBaseAddress + InitialRegion->Length) - (char*)StartAddress;
    InternalLength = min(InternalLength, Length);
    NewRegion1->Length = InternalLength;
    InsertAfterEntry(&InitialRegion->RegionListEntry,
                     &NewRegion1->RegionListEntry);

    /*
     * Call our helper function to do the changes on the addresses contained
     * in the initial region.
     */
    AlterFunc(AddressSpace, StartAddress, InternalLength, InitialRegion->Type,
              InitialRegion->Protect, NewType, NewProtect);

    /*
     * If necessary create a new region for the portion of the initial region
     * beyond the range of addresses to alter.
     */
    if (((char*)InitialBaseAddress + InitialRegion->Length) > ((char*)StartAddress + Length))
    {
        NewRegion2->Type = InitialRegion->Type;
        NewRegion2->Protect = InitialRegion->Protect;
        NewRegion2->Length = ((char*)InitialBaseAddress + InitialRegion->Length) -
                             ((char*)StartAddress + Length);
        InsertAfterEntry(&NewRegion1->RegionListEntry,
                         &NewRegion2->RegionListEntry);
    }
    else
    {
        ExFreePoolWithTag(NewRegion2, TAG_MM_REGION);
    }

    /* Either remove or shrink the initial region. */
    if (InitialBaseAddress == StartAddress)
    {
        RemoveEntryList(&InitialRegion->RegionListEntry);
        ExFreePoolWithTag(InitialRegion, TAG_MM_REGION);
    }
    else
    {
        InitialRegion->Length = (char*)StartAddress - (char*)InitialBaseAddress;
    }

    return(NewRegion1);
}

NTSTATUS
NTAPI
MmAlterRegion(PMMSUPPORT AddressSpace, PVOID BaseAddress,
              PLIST_ENTRY RegionListHead, PVOID StartAddress, SIZE_T Length,
              ULONG NewType, ULONG NewProtect, PMM_ALTER_REGION_FUNC AlterFunc)
{
    PMM_REGION InitialRegion;
    PVOID InitialBaseAddress = NULL;
    PMM_REGION NewRegion;
    PLIST_ENTRY CurrentEntry;
    PMM_REGION CurrentRegion = NULL;
    PVOID CurrentBaseAddress;
    SIZE_T RemainingLength;

    /*
     * Find the first region containing part of the range of addresses to
     * be altered.
     */
    InitialRegion = MmFindRegion(BaseAddress, RegionListHead, StartAddress,
                                 &InitialBaseAddress);
    /*
     * If necessary then split the region into the affected and unaffected parts.
     */
    if (InitialRegion->Type != NewType || InitialRegion->Protect != NewProtect)
    {
        NewRegion = MmSplitRegion(InitialRegion, InitialBaseAddress,
                                  StartAddress, Length, NewType, NewProtect,
                                  AddressSpace, AlterFunc);
        if (NewRegion == NULL)
        {
            return(STATUS_NO_MEMORY);
        }
        if(NewRegion->Length < Length)
            RemainingLength = Length - NewRegion->Length;
        else
            RemainingLength = 0;
    }
    else
    {
        NewRegion = InitialRegion;
        if(((ULONG_PTR)InitialBaseAddress + NewRegion->Length) <
                ((ULONG_PTR)StartAddress + Length))
            RemainingLength = ((ULONG_PTR)StartAddress + Length) - ((ULONG_PTR)InitialBaseAddress + NewRegion->Length);
        else
            RemainingLength = 0;
    }

    /*
     * Free any complete regions that are containing in the range of addresses
     * and call the helper function to actually do the changes.
     */
    CurrentEntry = NewRegion->RegionListEntry.Flink;
    CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION,
                                      RegionListEntry);
    CurrentBaseAddress = (char*)StartAddress + NewRegion->Length;
    while (RemainingLength > 0 && CurrentRegion->Length <= RemainingLength &&
            CurrentEntry != RegionListHead)
    {
        if (CurrentRegion->Type != NewType ||
                CurrentRegion->Protect != NewProtect)
        {
            AlterFunc(AddressSpace, CurrentBaseAddress, CurrentRegion->Length,
                      CurrentRegion->Type, CurrentRegion->Protect,
                      NewType, NewProtect);
        }

        CurrentBaseAddress = (PVOID)((ULONG_PTR)CurrentBaseAddress + CurrentRegion->Length);
        NewRegion->Length += CurrentRegion->Length;
        RemainingLength -= CurrentRegion->Length;
        CurrentEntry = CurrentEntry->Flink;
        RemoveEntryList(&CurrentRegion->RegionListEntry);
        ExFreePoolWithTag(CurrentRegion, TAG_MM_REGION);
        CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION,
                                          RegionListEntry);
    }

    /*
     * Split any final region.
     */
    if (RemainingLength > 0 && CurrentEntry != RegionListHead)
    {
        CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION,
                                          RegionListEntry);
        if (CurrentRegion->Type != NewType ||
                CurrentRegion->Protect != NewProtect)
        {
            AlterFunc(AddressSpace, CurrentBaseAddress, RemainingLength,
                      CurrentRegion->Type, CurrentRegion->Protect,
                      NewType, NewProtect);
        }
        NewRegion->Length += RemainingLength;
        CurrentRegion->Length -= RemainingLength;
    }

    /*
     * If the region after the new region has the same type then merge them.
     */
    if (NewRegion->RegionListEntry.Flink != RegionListHead)
    {
        CurrentEntry = NewRegion->RegionListEntry.Flink;
        CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION,
                                          RegionListEntry);
        if (CurrentRegion->Type == NewRegion->Type &&
                CurrentRegion->Protect == NewRegion->Protect)
        {
            NewRegion->Length += CurrentRegion->Length;
            RemoveEntryList(&CurrentRegion->RegionListEntry);
            ExFreePoolWithTag(CurrentRegion, TAG_MM_REGION);
        }
    }

    /*
     * If the region before the new region has the same type then merge them.
     */
    if (NewRegion->RegionListEntry.Blink != RegionListHead)
    {
        CurrentEntry = NewRegion->RegionListEntry.Blink;
        CurrentRegion = CONTAINING_RECORD(CurrentEntry, MM_REGION,
                                          RegionListEntry);
        if (CurrentRegion->Type == NewRegion->Type &&
                CurrentRegion->Protect == NewRegion->Protect)
        {
            NewRegion->Length += CurrentRegion->Length;
            RemoveEntryList(&CurrentRegion->RegionListEntry);
            ExFreePoolWithTag(CurrentRegion, TAG_MM_REGION);
        }
    }

    return(STATUS_SUCCESS);
}

VOID
NTAPI
MmInitializeRegion(PLIST_ENTRY RegionListHead, SIZE_T Length, ULONG Type,
                   ULONG Protect)
{
    PMM_REGION Region;

    Region = ExAllocatePoolWithTag(NonPagedPool, sizeof(MM_REGION),
                                   TAG_MM_REGION);
    if (!Region) return;

    Region->Type = Type;
    Region->Protect = Protect;
    Region->Length = Length;
    InitializeListHead(RegionListHead);
    InsertHeadList(RegionListHead, &Region->RegionListEntry);
}

PMM_REGION
NTAPI
MmFindRegion(PVOID BaseAddress, PLIST_ENTRY RegionListHead, PVOID Address,
             PVOID* RegionBaseAddress)
{
    PLIST_ENTRY current_entry;
    PMM_REGION current;
    PVOID StartAddress = BaseAddress;

    current_entry = RegionListHead->Flink;
    while (current_entry != RegionListHead)
    {
        current = CONTAINING_RECORD(current_entry, MM_REGION, RegionListEntry);

        if (StartAddress <= Address &&
                ((char*)StartAddress + current->Length) > (char*)Address)
        {
            if (RegionBaseAddress != NULL)
            {
                *RegionBaseAddress = StartAddress;
            }
            return(current);
        }

        current_entry = current_entry->Flink;

        StartAddress = (PVOID)((ULONG_PTR)StartAddress + current->Length);

    }
    return(NULL);
}
