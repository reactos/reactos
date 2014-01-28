/*
 *  ReactOS kernel
 *  Copyright (C) 2011 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/filesystem/mountmgr/uniqueid.c
 * PURPOSE:          Mount Manager - Unique ID
 * PROGRAMMER:       Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#include "mntmgr.h"

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
NTSTATUS
NTAPI
ChangeUniqueIdRoutine(IN PWSTR ValueName,
                      IN ULONG ValueType,
                      IN PVOID ValueData,
                      IN ULONG ValueLength,
                      IN PVOID Context,
                      IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID OldUniqueId = Context;
    PMOUNTDEV_UNIQUE_ID NewUniqueId = EntryContext;

    /* Validate parameters not to corrupt registry */
    if ((ValueType != REG_BINARY) ||
        (OldUniqueId->UniqueIdLength != ValueLength))
    {
        return STATUS_SUCCESS;
    }

    if (RtlCompareMemory(OldUniqueId->UniqueId, ValueData, ValueLength) == ValueLength)
    {
        /* Write new data */
        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                              DatabasePath,
                              ValueName,
                              REG_BINARY,
                              NewUniqueId,
                              NewUniqueId->UniqueIdLength);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
MountMgrUniqueIdChangeRoutine(IN PDEVICE_EXTENSION DeviceExtension,
                              IN PMOUNTDEV_UNIQUE_ID OldUniqueId,
                              IN PMOUNTDEV_UNIQUE_ID NewUniqueId)
{
    NTSTATUS Status;
    BOOLEAN ResyncNeeded;
    PUNIQUE_ID_REPLICATE DuplicateId;
    PDEVICE_INFORMATION DeviceInformation;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];
    PMOUNTDEV_UNIQUE_ID UniqueId, NewDuplicateId;
    PLIST_ENTRY ListHead, NextEntry, ReplicatedHead, NextReplicated;

    /* Synchronise with remote databases */
    Status = WaitForRemoteDatabaseSemaphore(DeviceExtension);
    KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = ChangeUniqueIdRoutine;
    QueryTable[0].EntryContext = NewUniqueId;

    /* Write new data */
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           DatabasePath,
                           QueryTable,
                           OldUniqueId,
                           NULL);

    /* Browse all the devices to find the one that
     * owns the old unique ID
     */
    ListHead = &(DeviceExtension->DeviceListHead);
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry,
                                              DEVICE_INFORMATION,
                                              DeviceListEntry);

        if (DeviceInformation->UniqueId->UniqueIdLength == OldUniqueId->UniqueIdLength &&
            RtlCompareMemory(OldUniqueId->UniqueId,
                             DeviceInformation->UniqueId->UniqueId,
                             OldUniqueId->UniqueIdLength) == OldUniqueId->UniqueIdLength)
        {
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    /* If we didn't find any release everything and quit */
    if (ListHead == NextEntry)
    {
        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT,
                           1, FALSE);

        if (NT_SUCCESS(Status))
        {
            ReleaseRemoteDatabaseSemaphore(DeviceExtension);
        }

        return;
    }

    /* If lock failed, then, just update this database */
    if (!NT_SUCCESS(Status))
    {
        ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInformation);
        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT,
                           1, FALSE);
        return;
    }

    /* Allocate new unique ID */
    UniqueId = AllocatePool(NewUniqueId->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
    if (!UniqueId)
    {
        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT,
                           1, FALSE);
        ReleaseRemoteDatabaseSemaphore(DeviceExtension);
        return;
    }

    /* Release old one */
    FreePool(DeviceInformation->UniqueId);
    /* And set new one */
    DeviceInformation->UniqueId = UniqueId;
    UniqueId->UniqueIdLength = NewUniqueId->UniqueIdLength;
    RtlCopyMemory(UniqueId->UniqueId, NewUniqueId->UniqueId, NewUniqueId->UniqueIdLength);

    /* Now, check if it's required to update replicated unique IDs as well */
    ListHead = &(DeviceExtension->DeviceListHead);
    NextEntry = ListHead->Flink;
    while (ListHead != NextEntry)
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry,
                                              DEVICE_INFORMATION,
                                              DeviceListEntry);
        ResyncNeeded = FALSE;

        ReplicatedHead = &(DeviceInformation->ReplicatedUniqueIdsListHead);
        NextReplicated = ReplicatedHead->Flink;
        while (ReplicatedHead != NextReplicated)
        {
            DuplicateId = CONTAINING_RECORD(NextReplicated,
                                            UNIQUE_ID_REPLICATE,
                                            ReplicatedUniqueIdsListEntry);

            if (DuplicateId->UniqueId->UniqueIdLength == OldUniqueId->UniqueIdLength)
            {
                if (RtlCompareMemory(DuplicateId->UniqueId->UniqueId,
                                     OldUniqueId->UniqueId,
                                     OldUniqueId->UniqueIdLength) == OldUniqueId->UniqueIdLength)
                {
                    /* It was our old unique ID */
                    NewDuplicateId = AllocatePool(NewUniqueId->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
                    if (NewDuplicateId)
                    {
                        /* Update it */
                        ResyncNeeded = TRUE;
                        FreePool(DuplicateId->UniqueId);

                        DuplicateId->UniqueId = NewDuplicateId;
                        DuplicateId->UniqueId->UniqueIdLength = NewUniqueId->UniqueIdLength;
                        RtlCopyMemory(NewDuplicateId->UniqueId, NewUniqueId->UniqueId, NewUniqueId->UniqueIdLength);
                    }
                }
            }

            NextReplicated = NextReplicated->Flink;
        }

        /* If resync is required on this device, do it */
        if (ResyncNeeded)
        {
            ChangeRemoteDatabaseUniqueId(DeviceInformation, OldUniqueId, NewUniqueId);
        }

        NextEntry = NextEntry->Flink;
    }

    KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);
    ReleaseRemoteDatabaseSemaphore(DeviceExtension);

    return;
}

/*
 * @implemented
 */
BOOLEAN
IsUniqueIdPresent(IN PDEVICE_EXTENSION DeviceExtension,
                  IN PDATABASE_ENTRY DatabaseEntry)
{
    PLIST_ENTRY NextEntry;
    PDEVICE_INFORMATION DeviceInformation;

    /* If no device, no unique ID (O'rly?!)
     * ./)/).
     * (°-°)
     * (___) ORLY?
     *  " "
     */
    if (IsListEmpty(&(DeviceExtension->DeviceListHead)))
    {
        return FALSE;
    }

    /* Now we know that we have devices, find the one */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &(DeviceExtension->DeviceListHead);
         NextEntry = NextEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry,
                                              DEVICE_INFORMATION,
                                              DeviceListEntry);

        if (DeviceInformation->UniqueId->UniqueIdLength != DatabaseEntry->UniqueIdLength)
        {
            continue;
        }

        /* It's matching! */
        if (RtlCompareMemory((PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset),
                             DeviceInformation->UniqueId->UniqueId,
                             DatabaseEntry->UniqueIdLength) == DatabaseEntry->UniqueIdLength)
        {
            return TRUE;
        }
    }

    /* No luck... */
    return FALSE;
}

/*
 * @implemented
 */
VOID
CreateNoDriveLetterEntry(IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    UUID Guid;
    PWCHAR String;
    UNICODE_STRING GuidString;

    /* Entry with no drive letter are made that way:
     * Instead of having a path with the letter,
     * you have GUID with the unique ID.
     */
    if (!NT_SUCCESS(ExUuidCreate(&Guid)))
    {
        return;
    }

    /* Convert to string */
    if (!NT_SUCCESS(RtlStringFromGUID(&Guid, &GuidString)))
    {
        return;
    }

    /* No letter entries must start with #, so allocate a proper string */
    String = AllocatePool(GuidString.Length + 2 * sizeof(WCHAR));
    if (!String)
    {
        ExFreePoolWithTag(GuidString.Buffer, 0);
        return;
    }

    /* Write the complete string */
    String[0] = L'#';
    RtlCopyMemory(String + 1, GuidString.Buffer, GuidString.Length);
    String[GuidString.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Don't need that one anymore */
    ExFreePoolWithTag(GuidString.Buffer, 0);

    /* Write the entry */
    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                          DatabasePath,
                          String,
                          REG_BINARY,
                          UniqueId->UniqueId,
                          UniqueId->UniqueIdLength);

    FreePool(String);

    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
CheckForNoDriveLetterEntry(IN PWSTR ValueName,
                           IN ULONG ValueType,
                           IN PVOID ValueData,
                           IN ULONG ValueLength,
                           IN PVOID Context,
                           IN PVOID EntryContext)
{
    PBOOLEAN EntryPresent = EntryContext;
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;

    /* Check if matches no drive letter entry */
    if (ValueName[0] != L'#' || ValueType != REG_BINARY ||
        UniqueId->UniqueIdLength != ValueLength)
    {
        return STATUS_SUCCESS;
    }

    /* Compare unique ID */
    if (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) == ValueLength)
    {
        *EntryPresent = TRUE;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
BOOLEAN
HasNoDriveLetterEntry(IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    BOOLEAN EntryPresent = FALSE;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = CheckForNoDriveLetterEntry;
    QueryTable[0].EntryContext = &EntryPresent;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           DatabasePath,
                           QueryTable,
                           UniqueId,
                           NULL);

    return EntryPresent;
}

/*
 * @implemented
 */
VOID
UpdateReplicatedUniqueIds(IN PDEVICE_INFORMATION DeviceInformation, IN PDATABASE_ENTRY DatabaseEntry)
{
    PLIST_ENTRY NextEntry;
    PUNIQUE_ID_REPLICATE ReplicatedUniqueId, NewEntry;

    /* Browse all the device replicated unique IDs */
    for (NextEntry = DeviceInformation->ReplicatedUniqueIdsListHead.Flink;
         NextEntry != &(DeviceInformation->ReplicatedUniqueIdsListHead);
         NextEntry = NextEntry->Flink)
    {
        ReplicatedUniqueId = CONTAINING_RECORD(NextEntry,
                                               UNIQUE_ID_REPLICATE,
                                               ReplicatedUniqueIdsListEntry);

        if (ReplicatedUniqueId->UniqueId->UniqueIdLength != DatabaseEntry->UniqueIdLength)
        {
            continue;
        }

        /* If we find the UniqueId to update, break */
        if (RtlCompareMemory(ReplicatedUniqueId->UniqueId->UniqueId,
                             (PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset),
                             ReplicatedUniqueId->UniqueId->UniqueIdLength) == ReplicatedUniqueId->UniqueId->UniqueIdLength)
        {
            break;
        }
    }

    /* We found the unique ID, no need to continue */
    if (NextEntry != &(DeviceInformation->ReplicatedUniqueIdsListHead))
    {
        return;
    }

    /* Allocate a new entry for unique ID */
    NewEntry = AllocatePool(sizeof(UNIQUE_ID_REPLICATE));
    if (!NewEntry)
    {
        return;
    }

    /* Allocate the unique ID */
    NewEntry->UniqueId = AllocatePool(DatabaseEntry->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
    if (!NewEntry->UniqueId)
    {
        FreePool(NewEntry);
        return;
    }

    /* Copy */
    NewEntry->UniqueId->UniqueIdLength = DatabaseEntry->UniqueIdLength;
    RtlCopyMemory(NewEntry->UniqueId->UniqueId,
                  (PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset),
                  DatabaseEntry->UniqueIdLength);
    /* And insert into replicated unique IDs list */
    InsertTailList(&DeviceInformation->ReplicatedUniqueIdsListHead, &NewEntry->ReplicatedUniqueIdsListEntry);

    return;
}
