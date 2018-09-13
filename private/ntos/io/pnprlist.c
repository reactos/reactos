/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    pnprlist.c

Abstract:

    This module contains routines to manipulate relations list.  Relation lists
    are used by Plug and Play during the processing of device removal and
    ejection.

    These routines are all pageable and can't be called at raised IRQL or with
    a spinlock held.

Author:

    Robert Nelson (robertn) Apr, 1998.

Revision History:

--*/

#include "iop.h"

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,'lrpP')
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IopAddRelationToList)
#pragma alloc_text(PAGE, IopAllocateRelationList)
#pragma alloc_text(PAGE, IopCompressRelationList)
#pragma alloc_text(PAGE, IopEnumerateRelations)
#pragma alloc_text(PAGE, IopFreeRelationList)
#pragma alloc_text(PAGE, IopGetRelationsCount)
#pragma alloc_text(PAGE, IopGetRelationsTaggedCount)
#pragma alloc_text(PAGE, IopIsRelationInList)
#pragma alloc_text(PAGE, IopMergeRelationLists)
#pragma alloc_text(PAGE, IopRemoveIndirectRelationsFromList)
#pragma alloc_text(PAGE, IopRemoveRelationFromList)
#pragma alloc_text(PAGE, IopSetAllRelationsTags)
#pragma alloc_text(PAGE, IopSetRelationsTag)
#endif

#define RELATION_FLAGS              0x00000003

#define RELATION_FLAG_TAGGED        0x00000001
#define RELATION_FLAG_DESCENDANT    0x00000002

NTSTATUS
IopAddRelationToList(
    IN PRELATION_LIST List,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN DirectDescendant,
    IN BOOLEAN Tagged
    )

/*++

Routine Description:

    Adds an element to a relation list.

    If this is the first DeviceObject of a particular level then a new
    RELATION_LIST_ENTRY will be allocated.

    This routine should only be called on an uncompressed relation list,
    otherwise it is likely that STATUS_INVALID_PARAMETER will be returned.

Arguments:

    List                Relation list to which the DeviceObject is added.

    DeviceObject        DeviceObject to be added to List.  It must be a
                        PhysicalDeviceObject (PDO).

    DirectDescendant    Indicates whether DeviceObject is a direct descendant of
                        the original target device of this remove.

    Tagged              Indicates whether DeviceObject should be tagged in List.

Return Value:

    STATUS_SUCCESS

        The DeviceObject was added successfully.

    STATUS_OBJECT_NAME_COLLISION

        The DeviceObject already exists in the relation list.

    STATUS_INSUFFICIENT_RESOURCES

        There isn't enough PagedPool available to allocate a new
        RELATION_LIST_ENTRY.

    STATUS_INVALID_PARAMETER

        The level of the DEVICE_NODE associated with DeviceObject is less than
        FirstLevel or greater than the MaxLevel.

    STATUS_NO_SUCH_DEVICE

        DeviceObject is not a PhysicalDeviceObject (PDO), it doesn't have a
        DEVICE_NODE associated with it.

--*/

{
    PDEVICE_NODE            deviceNode;
    PRELATION_LIST_ENTRY    entry;
    ULONG                   level;
    ULONG                   index;
    ULONG                   flags;

    PAGED_CODE();

    flags = 0;

    if (Tagged) {
        Tagged = 1;
        flags |= RELATION_FLAG_TAGGED;
    }

    if (DirectDescendant) {
        flags |= RELATION_FLAG_DESCENDANT;
    }

    if ((deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode) != NULL) {
        level = deviceNode->Level;

        //
        // Since this routine is called with the DeviceNode Tree locked and
        // List is initially allocated with enough entries to hold the deepest
        // DEVICE_NODE this ASSERT should never fire.  If it does then either
        // the tree is changing or we were given a compressed list.
        //
        ASSERT(List->FirstLevel <= level && level <= List->MaxLevel);

        if (List->FirstLevel <= level && level <= List->MaxLevel) {

            if ((entry = List->Entries[ level - List->FirstLevel ]) == NULL) {

                //
                // This is the first DeviceObject of its level, allocate a new
                // RELATION_LIST_ENTRY.
                //
                entry = ExAllocatePool( PagedPool,
                                        sizeof(RELATION_LIST_ENTRY) +
                                        IopNumberDeviceNodes * sizeof(PDEVICE_OBJECT));

                if (entry == NULL) {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                //
                // We always allocate enough Devices to hold the whole tree as
                // a simplification.  Since each entry is a PDEVICE_OBJECT and
                // there is generally under 50 devices on a machine this means
                // under 1K for each entry.  The excess space will be freed when
                // the list is compressed.
                //
                entry->Count = 0;
                entry->MaxCount = IopNumberDeviceNodes;

                List->Entries[ level - List->FirstLevel ] = entry;
            }

            //
            // There should always be room for a DeviceObject since the Entry is
            // initially dimensioned large enough to hold all the DEVICE_NODES
            // in the system.
            //
            ASSERT(entry->Count < entry->MaxCount);

            if (entry->Count < entry->MaxCount) {
                //
                // Search the list to see if DeviceObject has already been
                // added.
                //
                for (index = 0; index < entry->Count; index++) {
                    if (((ULONG_PTR)entry->Devices[ index ] & ~RELATION_FLAGS) == (ULONG_PTR)DeviceObject) {

                        //
                        // DeviceObject already exists in the list.  However
                        // the Direct Descendant flag may differ.  We will
                        // override it if DirectDescendant is TRUE.  This could
                        // happen if we merged two relation lists.

                        if (DirectDescendant) {
                            entry->Devices[ index ] = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ index ] | RELATION_FLAG_DESCENDANT);
                        }

                        return STATUS_OBJECT_NAME_COLLISION;
                    }
                }
            } else {
                //
                // There isn't room in the Entry for another DEVICE_OBJECT, the
                // list has probably already been compressed.
                //
                return STATUS_INVALID_PARAMETER;
            }

            //
            // Take out a reference on DeviceObject, we will release it when we
            // free the list or remove the DeviceObject from the list.
            //
            ObReferenceObject( DeviceObject );

            entry->Devices[ index ] = (PDEVICE_OBJECT)((ULONG_PTR)DeviceObject | flags);
            entry->Count++;

            List->Count++;
            List->TagCount += Tagged;

            return STATUS_SUCCESS;
        } else {
            //
            // There isn't an Entry available for the level of this
            // DEVICE_OBJECT, the list has probably already been compressed.
            //

            return STATUS_INVALID_PARAMETER;
        }
    } else {
        //
        // DeviceObject is not a PhysicalDeviceObject (PDO).
        //
        return STATUS_NO_SUCH_DEVICE;
    }
}

PRELATION_LIST
IopAllocateRelationList(
    VOID
    )

/*++

Routine Description:

    Allocate a new Relations List.  The list is initially sized large enough to
    hold the deepest DEVICE_NODE encountered since the system started.

Arguments:

    NONE

Return Value:

    Newly allocated list if enough PagedPool is available, otherwise NULL.

--*/

{
    PRELATION_LIST  list;
    ULONG           maxLevel;
    ULONG           listSize;

    PAGED_CODE();

    //
    // Level number of the deepest DEVICE_NODE allocated since the system
    // started.
    //
    maxLevel = IopMaxDeviceNodeLevel;
    listSize = sizeof(RELATION_LIST) + maxLevel * sizeof(PRELATION_LIST_ENTRY);

    if ((list = ExAllocatePool( PagedPool, listSize )) != NULL) {
        RtlZeroMemory(list, listSize);
        // list->FirstLevel = 0;
        // list->Count = 0;
        // list->Tagged = 0;
        list->MaxLevel = maxLevel;
    }

    return list;
}

NTSTATUS
IopCompressRelationList(
    IN OUT PRELATION_LIST *List
    )

/*++

Routine Description:

    Compresses the relation list by reallocating the list and all the entries so
    that they a just large enough to hold their current contents.

    Once a list has been compressed IopAddRelationToList and
    IopMergeRelationLists targetting this list are both likely to fail.

Arguments:

    List    Relation List to compress.

Return Value:

    STATUS_SUCCESS

        The list was compressed.  Although this routine does allocate memory and
        the allocation can fail, the routine itself will never fail.  Since the
        memory we are allocating is always smaller then the memory it is
        replacing we just keep the old memory if the allocation fails.

--*/

{
    PRELATION_LIST          oldList, newList;
    PRELATION_LIST_ENTRY    oldEntry, newEntry;
    ULONG                   lowestLevel;
    ULONG                   highestLevel;
    ULONG                   index;

    PAGED_CODE();

    oldList = *List;

    //
    // Initialize lowestLevel and highestLevel with illegal values chosen so
    // that the first real entry will override them.
    //
    lowestLevel = oldList->MaxLevel;
    highestLevel = oldList->FirstLevel;

    //
    // Loop through the list looking for allocated entries.
    //
    for (index = 0; index <= (oldList->MaxLevel - oldList->FirstLevel); index++) {

        if ((oldEntry = oldList->Entries[ index ]) != NULL) {
            //
            // This entry is allocated, update lowestLevel and highestLevel if
            // necessary.
            //
            if (lowestLevel > index) {
                lowestLevel = index;
            }

            if (highestLevel < index) {
                highestLevel = index;
            }

            if (oldEntry->Count < oldEntry->MaxCount) {

                //
                // This entry is only partially full.  Allocate a new entry
                // which is just the right size to hold the current number of
                // PDEVICE_OBJECTs.
                //
                newEntry = ExAllocatePool( PagedPool,
                                           sizeof(RELATION_LIST_ENTRY) +
                                           (oldEntry->Count - 1) * sizeof(PDEVICE_OBJECT));

                if (newEntry != NULL) {

                    //
                    // Initialize Count and MaxCount to the number of
                    // PDEVICE_OBJECTs in the old entry.
                    //
                    newEntry->Count = oldEntry->Count;
                    newEntry->MaxCount = oldEntry->Count;

                    //
                    // Copy the PDEVICE_OBJECTs from the old entry to the new
                    // one.
                    //
                    RtlCopyMemory( newEntry->Devices,
                                   oldEntry->Devices,
                                   oldEntry->Count * sizeof(PDEVICE_OBJECT));

                    //
                    // Free the old entry and store the new entry in the list.
                    //
                    ExFreePool( oldEntry );

                    oldList->Entries[ index ] = newEntry;
                }
            }
        }
    }

    //
    // Assert that the old list isn't empty.
    //
    ASSERT(lowestLevel <= highestLevel);

    if (lowestLevel > highestLevel) {
        //
        // The list is empty - we shouldn't get asked to compress an empty list
        // but lets do it anyways.
        //
        lowestLevel = 0;
        highestLevel = 0;
    }

    //
    // Check if the old list had unused entries at the beginning or the end of
    // the Entries array.
    //
    if (lowestLevel != oldList->FirstLevel || highestLevel != oldList->MaxLevel) {

        //
        // Allocate a new List with just enough Entries to hold those between
        // FirstLevel and MaxLevel inclusive.
        //
        newList = ExAllocatePool( PagedPool,
                                  sizeof(RELATION_LIST) +
                                  (highestLevel - lowestLevel) * sizeof(PRELATION_LIST_ENTRY));

        if (newList != NULL) {
            //
            // Copy the old list to the new list and return it to the caller.
            //
            newList->Count = oldList->Count;
            newList->TagCount = oldList->TagCount;
            newList->FirstLevel = lowestLevel;
            newList->MaxLevel = highestLevel;

            RtlCopyMemory( newList->Entries,
                           &oldList->Entries[ lowestLevel ],
                           (highestLevel - lowestLevel + 1) * sizeof(PRELATION_LIST_ENTRY));

            ExFreePool( oldList );

            *List = newList;
        }
    }

    return STATUS_SUCCESS;
}

BOOLEAN
IopEnumerateRelations(
    IN PRELATION_LIST List,
    IN OUT PULONG Marker,
    OUT PDEVICE_OBJECT *DeviceObject,
    OUT BOOLEAN *DirectDescendant, OPTIONAL
    OUT BOOLEAN *Tagged, OPTIONAL
    IN BOOLEAN Reverse
    )

/*++

Routine Description:

    Enumerates the relations in a list.

Arguments:

    List                Relation list to be enumerated.

    Marker              Cookie used to maintain current place in the list.  It
                        must be initialized to 0 the first time
                        IopEnumerateRelations is called.

    DeviceObject        Returned Relation.

    DirectDescendant    If specified then it is set if the relation is a direct
                        descendant of the original target device of this remove.

    Tagged              If specified then it is set if the relation is tagged
                        otherwise it is cleared.

    Reverse             Direction of traversal, TRUE means from deepest to
                        closest to the root, FALSE means from the root down.

                        If Reverse changes on a subsequent call then the
                        previously enumerated relation is skipped.  For example,
                        given the sequence A, B, C, D, E.  If
                        IopEnumerateRelations is called thrice with Reverse set
                        to FALSE and then called repeatedly with Reverse set to
                        TRUE until it returns FALSE, the sequence would be: A,
                        B, C, B, A.

                        Once the end has been reached it is not possible to
                        change directions.


Return Value:

    TRUE

        DeviceObject and optionally Tagged have been set to the next relation.

    FALSE

        There are no more relations.

--*/

{
    PRELATION_LIST_ENTRY    entry;
    LONG                    levelIndex;
    ULONG                   entryIndex;

    PAGED_CODE();

    //
    // The basic assumptions of our use of Marker is that there will never be
    // more than 16M DeviceNodes at any one level and that the tree will never
    // be more than 127 deep.
    //
    // The format of Marker is
    //      Bit 31      = Valid (used to distinguish the initial call
    //      Bit 30-24   = Current index into entries
    //      Bit 23-0    = Current index into devices, 0xFFFFFF means last
    //
    if (*Marker == ~0U) {
        //
        // We've reached the end.
        //
        return FALSE;
    }

    if (*Marker == 0) {
        //
        // This is the initial call to IopEnumerateRelations
        //
        if (Reverse) {
            //
            // Initialize levelIndex to the last element of Entries
            //
            levelIndex = List->MaxLevel - List->FirstLevel;
        } else {
            //
            // Initialize levelIndex to the first element of Entries
            //
            levelIndex = 0;
        }
        //
        // Initialize entryIndex to unknown element of Devices.  If we are going
        // in reverse then this will appear to be beyond the last element and
        // we'll adjust it the last one.  If we are going forward then this will
        // appear to be just prior to the first element so when we increment it,
        // it will become zero.
        //
        entryIndex = ~0U;
    } else {
        //
        // Bit 31 is our valid bit, used to distinguish level 0, device 0 from
        // the first time call.
        //
        ASSERT(*Marker & ((ULONG)1 << 31));
        //
        // Current level stored in bits 30-24.
        //
        levelIndex = (*Marker >> 24) & 0x7F;
        //
        // Current device stored in bits 23-0.
        //
        entryIndex = *Marker & 0x00FFFFFF;
    }

    if (Reverse) {
        //
        // We are traversing the list bottom up, from the deepest device towards
        // the root.
        //
        for ( ; levelIndex >= 0; levelIndex--) {

            //
            // Since the Entries array can be sparse find the next allocated
            // Entry.
            //
            if ((entry = List->Entries[ levelIndex ]) != NULL) {

                if (entryIndex > entry->Count) {
                    //
                    // entryIndex (the current one) is greater than Count, this
                    // will be the case where it is 0xFFFFFF, in other words
                    // unspecified.  Adjust it so that it is one past the last
                    // one in this Entry.
                    //
                    entryIndex = entry->Count;
                }

                if (entryIndex > 0) {

                    //
                    // The current entry is beyond the first entry so the next
                    // entry (which is the one we are looking for is immediately
                    // prior, adjust entryIndex.
                    //
                    entryIndex--;

                    //
                    // Get the device object and remove the tag.
                    //
                    *DeviceObject = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ entryIndex ] & ~RELATION_FLAGS);

                    if (Tagged != NULL) {
                        //
                        // The caller is interested in the tag value.
                        //
                        *Tagged = (BOOLEAN)((ULONG_PTR)entry->Devices[ entryIndex ] & RELATION_FLAG_TAGGED);
                    }

                    if (DirectDescendant != NULL) {
                        //
                        // The caller is interested in the DirectDescendant value.
                        //
                        *DirectDescendant = (BOOLEAN)((ULONG_PTR)entry->Devices[ entryIndex ] & RELATION_FLAG_DESCENDANT);
                    }

                    //
                    // Update the marker (info for current device)
                    //
                    *Marker = ((ULONG)1 << 31) | (levelIndex << 24) | (entryIndex & 0x00FFFFFF);

                    return TRUE;
                }
            }

            //
            // The current device object has been deleted or the current
            // device object is the first one in this Entry.
            // We need to continue to search backwards through the other
            // Entries.
            //
            entryIndex = ~0U;
        }
    } else {
        for ( ; levelIndex <= (LONG)(List->MaxLevel - List->FirstLevel); levelIndex++) {

            //
            // Since the Entries array can be sparse find the next allocated
            // Entry.
            //
            if ((entry = List->Entries[ levelIndex ]) != NULL) {

                //
                // entryIndex is the index of the current device or 0xFFFFFFFF
                // if this is the first time we have been called or the current
                // current device is the last one in its Entry.  Increment the
                // index to point to the next device.
                //
                entryIndex++;

                if (entryIndex < entry->Count) {

                    //
                    // The next device is within this entry.
                    //
                    //
                    // Get the device object and remove the tag.
                    //
                    *DeviceObject = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ entryIndex ] & ~RELATION_FLAGS);

                    if (Tagged != NULL) {
                        //
                        // The caller is interested in the tag value.
                        //
                        *Tagged = (BOOLEAN)((ULONG_PTR)entry->Devices[ entryIndex ] & RELATION_FLAG_TAGGED);
                    }

                    if (DirectDescendant != NULL) {
                        //
                        // The caller is interested in the DirectDescendant value.
                        //
                        *DirectDescendant = (BOOLEAN)((ULONG_PTR)entry->Devices[ entryIndex ] & RELATION_FLAG_DESCENDANT);
                    }

                    //
                    // Update the marker (info for current device)
                    //
                    *Marker = ((ULONG)1 << 31) | (levelIndex << 24) | (entryIndex & 0x00FFFFFF);

                    return TRUE;
                }
            }

            //
            // The current device has been removed or we have processed the
            // last device in the current entry.
            // Set entryIndex so that it is just before the first device in
            // the next entry.  Continue the search looking for the next
            // allocated Entry.
            //
            entryIndex = ~0U;
        }
    }

    //
    // We are at the end of the list
    //
    *Marker = ~0U;
    *DeviceObject = NULL;

    if (Tagged != NULL) {
        *Tagged = FALSE;
    }

    if (DirectDescendant != NULL) {
        *DirectDescendant = FALSE;
    }

    return FALSE;
}

VOID
IopFreeRelationList(
    IN PRELATION_LIST List
    )

/*++

Routine Description:

    Free a relation list allocated by IopAllocateRelationList.

Arguments:

    List    The list to be freed.

Return Value:

    NONE.

--*/

{
    PRELATION_LIST_ENTRY    entry;
    ULONG                   levelIndex;
    ULONG                   entryIndex;

    PAGED_CODE();

    //
    // Search the list looking for allocated Entries.
    //
    for (levelIndex = 0; levelIndex <= (List->MaxLevel - List->FirstLevel); levelIndex++) {

        if ((entry = List->Entries[ levelIndex ]) != NULL) {
            //
            // This entry has been allocated.
            //
            for (entryIndex = 0; entryIndex < entry->Count; entryIndex++) {
                //
                // Dereference all the Devices in the entry.
                //
                ObDereferenceObject((PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ entryIndex ] & ~RELATION_FLAGS));
            }
            //
            // Free the Entry.
            //
            ExFreePool( entry );
        }
    }

    //
    // Free the list.  It isn't necessary to dereference the DeviceObject that
    // was the original target that caused the list to be created.  This
    // DeviceObject is also in one of the Entries and its reference is taken
    // and released there.
    //
    ExFreePool( List );
}

ULONG
IopGetRelationsCount(
    PRELATION_LIST List
    )

/*++

Routine Description:

    Returns the total number of relations (Device Objects) in all the entries.

Arguments:

    List    Relation List.

Return Value:

    Count of relations (Device Objects).

--*/

{
    PAGED_CODE();

    return List->Count;
}

ULONG
IopGetRelationsTaggedCount(
    PRELATION_LIST List
    )

/*++

Routine Description:

    Returns the total number of relations (Device Objects) in all the entries
    which are tagged.

Arguments:

    List    Relation List.

Return Value:

    Count of tagged relations (Device Objects).

--*/

{
    PAGED_CODE();

    return List->TagCount;
}

BOOLEAN
IopIsRelationInList(
    PRELATION_LIST List,
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Checks if a relation (Device Object) exists in the specified relation list.

Arguments:

    List            Relation list to check.

    DeviceObject    Relation to be checked.


Return Value:

    TRUE

        Relation exists.

    FALSE

        Relation is not in the list.

--*/

{
    PDEVICE_NODE            deviceNode;
    PRELATION_LIST_ENTRY    entry;
    ULONG                   level;
    ULONG                   index;

    PAGED_CODE();

    if ((deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode) != NULL) {
        //
        // The device object is a PDO.
        //
        level = deviceNode->Level;

        if (List->FirstLevel <= level && level <= List->MaxLevel) {
            //
            // The level is within the range of levels stored in this list.
            //
            if ((entry = List->Entries[ level - List->FirstLevel ]) != NULL) {
                //
                // There is an Entry for this level.
                //
                for (index = 0; index < entry->Count; index++) {
                    //
                    // For each Device in the entry, compare it to the given
                    // DeviceObject
                    if (((ULONG_PTR)entry->Devices[ index ] & ~RELATION_FLAGS) == (ULONG_PTR)DeviceObject) {
                        //
                        // It matches
                        //
                        return TRUE;
                    }
                }
            }
        }
    }

    //
    // It wasn't a PDO
    //      or the level wasn't in the range of levels in this list
    //      or there are no DeviceObjects at the same level in this list
    //      or the DeviceObject isn't in the Entry for its level in this list
    //
    return FALSE;
}

NTSTATUS
IopMergeRelationLists(
    IN OUT PRELATION_LIST TargetList,
    IN PRELATION_LIST SourceList,
    IN BOOLEAN Tagged
    )

/*++

Routine Description:

    Merges two relation lists by copying all the relations from the source list
    to the target list.  Source list remains unchanged.

Arguments:

    TargetList  List to which the relations from Sourcelist are added.

    SourceList  List of relations to be added to TargetList.

    Tagged      TRUE if relations from SourceList should be tagged when added to
                TargetList.  If FALSE then relations added from SourceList are
                untagged.

Return Value:

    STATUS_SUCCESS

        All the relations in SourceList were added to TargetList successfully.

    STATUS_OBJECT_NAME_COLLISION

        One of the relations in SourceList already exists in TargetList.  This
        is a fatal error and TargetList may already have some of the relations
        from SourceList added.  This could be dealt with more gracefully if
        necessary but the current callers of IopMergeRelationLists avoid this
        situation.

    STATUS_INSUFFICIENT_RESOURCES

        There isn't enough PagedPool available to allocate a new
        RELATION_LIST_ENTRY.

    STATUS_INVALID_PARAMETER

        The level of one of the relations in SourceList is less than FirstLevel
        or greater than the MaxLevel.  This is a fatal error and TargetList may
        already have some of the relations from SourceList added.  The only way
        this could happen is if the tree lock isn't held or if TargetList has
        been compressed by IopCompressRelationList.  Both situations would be
        bugs in the caller.

    STATUS_NO_SUCH_DEVICE

        One of the relations in SourceList is not a PhysicalDeviceObject (PDO),
        it doesn't have a DEVICE_NODE associated with it.  This is a fatal error
        and TargetList may already have some of the relations from SourceList
        added.  This should never happen since it was a PDO when it was added to
        SourceList.


--*/

{
    PRELATION_LIST_ENTRY    entry;
    ULONG                   levelIndex;
    ULONG                   entryIndex;
    NTSTATUS                status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    // For each level entry in SourceList
    //
    for (levelIndex = 0;
         levelIndex <= (SourceList->MaxLevel - SourceList->FirstLevel);
         levelIndex++) {

        if ((entry = SourceList->Entries[ levelIndex ]) != NULL) {
            //
            // The Entry has Devices
            //
            for (entryIndex = 0; entryIndex < entry->Count; entryIndex++) {
                //
                // For each Device in the Entry, add it to the target List.
                //
                status = IopAddRelationToList( TargetList,
                                               (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ entryIndex ] & ~RELATION_FLAGS),
                                               FALSE,
                                               Tagged);

                //
                // BUGBUG - Need to handle STATUS_INSUFFICIENT_RESOURCES more
                // gracefully.  Currently the only way this can happen is
                // during Eject and that code isn't enabled yet.
                //
                ASSERT(NT_SUCCESS(status));

                if (!NT_SUCCESS(status)) {
                    //
                    // BUGBUG - We should undo the damage we've done to
                    // TargetList by removing those relations we've added from
                    // SourceList.
                    //
                    break;
                }
            }
        }
    }

    return status;
}

NTSTATUS
IopRemoveIndirectRelationsFromList(
    IN PRELATION_LIST List
    )

/*++

Routine Description:

    Removes all the relations without the DirectDescendant flag from a relation
    list.

Arguments:

    List    List from which to remove the relations.


Return Value:

    STATUS_SUCCESS

        The relations were removed successfully.

--*/

{
    PDEVICE_OBJECT          deviceObject;
    PRELATION_LIST_ENTRY    entry;
    ULONG                   level;
    LONG                    index;

    PAGED_CODE();

    //
    // For each Entry in the list.
    //
    for (level = List->FirstLevel; level <= List->MaxLevel; level++) {

        //
        // If the entry is allocated.
        //
        if ((entry = List->Entries[ level - List->FirstLevel ]) != NULL) {

            //
            // For each Device in the list.
            //
            for (index = entry->Count - 1; index >= 0; index--) {
                if (!((ULONG_PTR)entry->Devices[ index ] & RELATION_FLAG_DESCENDANT)) {

                    deviceObject = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ index ] & ~RELATION_FLAGS);

                    ObDereferenceObject( deviceObject );

                    if ((ULONG_PTR)entry->Devices[ index ] & RELATION_FLAG_TAGGED) {
                        List->TagCount--;
                    }

                    if (index < ((LONG)entry->Count - 1)) {

                        RtlMoveMemory( &entry->Devices[ index ],
                                        &entry->Devices[ index + 1 ],
                                        (entry->Count - index - 1) * sizeof(PDEVICE_OBJECT));
                    }

                    if (--entry->Count == 0) {
                        List->Entries[ level - List->FirstLevel ] = NULL;
                        ExFreePool(entry);
                    }

                    List->Count--;
                }
            }
        }
    }
    return STATUS_SUCCESS;
}

NTSTATUS
IopRemoveRelationFromList(
    PRELATION_LIST List,
    PDEVICE_OBJECT DeviceObject
    )

/*++

Routine Description:

    Removes a relation from a relation list.

Arguments:

    List            List from which to remove the relation.

    DeviceObject    Relation to remove.

Return Value:

    STATUS_SUCCESS

        The relation was removed successfully.

    STATUS_NO_SUCH_DEVICE

        The relation doesn't exist in the list.

--*/

{
    PDEVICE_NODE            deviceNode;
    PRELATION_LIST_ENTRY    entry;
    ULONG                   level;
    LONG                    index;

    PAGED_CODE();

    if ((deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode) != NULL) {
        level = deviceNode->Level;

        ASSERT(List->FirstLevel <= level && level <= List->MaxLevel);

        if (List->FirstLevel <= level && level <= List->MaxLevel) {
            if ((entry = List->Entries[ level - List->FirstLevel ]) != NULL) {
                for (index = entry->Count - 1; index >= 0; index--) {
                    if (((ULONG_PTR)entry->Devices[ index ] & ~RELATION_FLAGS) == (ULONG_PTR)DeviceObject) {

                        ObDereferenceObject( DeviceObject );

                        if (((ULONG_PTR)entry->Devices[ index ] & RELATION_FLAG_TAGGED) != 0) {
                            List->TagCount--;
                        }
                        if (index < ((LONG)entry->Count - 1)) {

                            RtlMoveMemory( &entry->Devices[ index ],
                                           &entry->Devices[ index + 1 ],
                                           (entry->Count - index - 1) * sizeof(PDEVICE_OBJECT));
                        }

                        if (--entry->Count == 0) {
                            List->Entries[ level - List->FirstLevel ] = NULL;
                            ExFreePool(entry);
                        }

                        List->Count--;

                        return STATUS_SUCCESS;
                    }
                }
            }
        }
    }
    return STATUS_NO_SUCH_DEVICE;
}

VOID
IopSetAllRelationsTags(
    PRELATION_LIST List,
    BOOLEAN Tagged
    )

/*++

Routine Description:

    Tags or untags all the relations in a relations list.

Arguments:

    List    Relation list containing relations to be tagged or untagged.

    Tagged  TRUE if the relations should be tagged, FALSE if they are to be
            untagged.

Return Value:

    NONE

--*/

{
    PRELATION_LIST_ENTRY    entry;
    ULONG                   level;
    ULONG                   index;

    PAGED_CODE();

    //
    // For each Entry in the list.
    //
    for (level = List->FirstLevel; level <= List->MaxLevel; level++) {

        //
        // If the entry is allocated.
        //
        if ((entry = List->Entries[ level - List->FirstLevel ]) != NULL) {

            //
            // For each Device in the list.
            //
            for (index = 0; index < entry->Count; index++) {

                //
                // Set or clear the tag based on the argument Tagged.
                //
                if (Tagged) {
                    entry->Devices[ index ] = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ index ] | RELATION_FLAG_TAGGED);
                } else {
                    entry->Devices[ index ] = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ index ] & ~RELATION_FLAG_TAGGED);
                }
            }
        }
    }

    //
    // If we are setting the tags then update the TagCount to the number of
    // relations in the list.  Otherwise reset it to zero.
    //
    List->TagCount = Tagged ? List->Count : 0;
}

NTSTATUS
IopSetRelationsTag(
    IN PRELATION_LIST List,
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN Tagged
    )

/*++

Routine Description:

    Sets or clears a tag on a specified relation in a relations list.  This
    routine is also used by some callers to determine if a relation exists in
    a list and if so to set the tag.

Arguments:

    List            List containing relation to be tagged or untagged.

    DeviceObject    Relation to be tagged or untagged.

    Tagged          TRUE if relation is to be tagged, FALSE if it is to be
                    untagged.

Return Value:

    STATUS_SUCCESS

        The relation was tagged successfully.

    STATUS_NO_SUCH_DEVICE

        The relation doesn't exist in the list.

--*/

{
    PDEVICE_NODE            deviceNode;
    PRELATION_LIST_ENTRY    entry;
    ULONG                   level;
    LONG                    index;

    PAGED_CODE();

    if ((deviceNode = DeviceObject->DeviceObjectExtension->DeviceNode) != NULL) {
        //
        // DeviceObject is a PhysicalDeviceObject (PDO), get its level.
        //
        level = deviceNode->Level;

        if (List->FirstLevel <= level && level <= List->MaxLevel) {
            //
            // The level is within the range of levels in this List.
            //
            if ((entry = List->Entries[ level - List->FirstLevel ]) != NULL) {
                //
                // The Entry for this level is allocated.  Search each device
                // in the Entry looking for a match.
                //
                for (index = entry->Count - 1; index >= 0; index--) {

                    if (((ULONG_PTR)entry->Devices[ index ] & ~RELATION_FLAGS) == (ULONG_PTR)DeviceObject) {

                        //
                        // We found a match
                        //
                        if ((ULONG_PTR)entry->Devices[ index ] & RELATION_FLAG_TAGGED) {
                            //
                            // The relation is already tagged so to simplify the
                            // logic below decrement the TagCount.  We'll
                            // increment it later if the caller still wants it
                            // to be tagged.
                            //
                            List->TagCount--;
                        }

                        if (Tagged) {
                            //
                            // Set the tag and increment the number of tagged
                            // relations.
                            //
                            entry->Devices[ index ] = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ index ] | RELATION_FLAG_TAGGED);
                            List->TagCount++;
                        } else {
                            //
                            // Clear the tag.
                            //
                            entry->Devices[ index ] = (PDEVICE_OBJECT)((ULONG_PTR)entry->Devices[ index ] & ~RELATION_FLAG_TAGGED);
                        }

                        return STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    //
    // It wasn't a PDO
    //      or the level wasn't in the range of levels in this list
    //      or there are no DeviceObjects at the same level in this list
    //      or the DeviceObject isn't in the Entry for its level in this list
    //
    return STATUS_NO_SUCH_DEVICE;
}

