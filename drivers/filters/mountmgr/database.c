/*
 *  ReactOS kernel
 *  Copyright (C) 2011-2012 ReactOS Team
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
 * FILE:             drivers/filesystem/mountmgr/mountmgr.c
 * PURPOSE:          Mount Manager - remote/local database handler
 * PROGRAMMER:       Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#include "mntmgr.h"

#define NDEBUG
#include <debug.h>

PWSTR DatabasePath = L"\\Registry\\Machine\\System\\MountedDevices";
PWSTR OfflinePath = L"\\Registry\\Machine\\System\\MountedDevices\\Offline";

UNICODE_STRING RemoteDatabase = RTL_CONSTANT_STRING(L"\\System Volume Information\\MountPointManagerRemoteDatabase");

/*
 * @implemented
 */
LONG
GetRemoteDatabaseSize(IN HANDLE Database)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

    /* Just query the size */
    Status = ZwQueryInformationFile(Database,
                                    &IoStatusBlock,
                                    &StandardInfo,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (NT_SUCCESS(Status))
    {
        return StandardInfo.EndOfFile.LowPart;
    }

    return 0;
}

/*
 * @implemented
 */
NTSTATUS
AddRemoteDatabaseEntry(IN HANDLE Database,
                       IN PDATABASE_ENTRY Entry)
{
    LARGE_INTEGER Size;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get size to append data */
    Size.QuadPart = GetRemoteDatabaseSize(Database);

    return ZwWriteFile(Database, NULL, NULL, NULL,
                       &IoStatusBlock, Entry,
                       Entry->EntrySize, &Size, NULL);
}

/*
 * @implemented
 */
NTSTATUS
CloseRemoteDatabase(IN HANDLE Database)
{
    return ZwClose(Database);
}

/*
 * @implemented
 */
NTSTATUS
TruncateRemoteDatabase(IN HANDLE Database,
                       IN LONG NewSize)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_END_OF_FILE_INFORMATION EndOfFile;
    FILE_ALLOCATION_INFORMATION Allocation;

    EndOfFile.EndOfFile.QuadPart = NewSize;
    Allocation.AllocationSize.QuadPart = NewSize;

    /* First set EOF */
    Status = ZwSetInformationFile(Database,
                                  &IoStatusBlock,
                                  &EndOfFile,
                                  sizeof(FILE_END_OF_FILE_INFORMATION),
                                  FileEndOfFileInformation);
    if (NT_SUCCESS(Status))
    {
        /* And then, properly set allocation information */
        Status = ZwSetInformationFile(Database,
                                      &IoStatusBlock,
                                      &Allocation,
                                      sizeof(FILE_ALLOCATION_INFORMATION),
                                      FileAllocationInformation);
    }

    return Status;
}

/*
 * @implemented
 */
PDATABASE_ENTRY
GetRemoteDatabaseEntry(IN HANDLE Database,
                       IN LONG StartingOffset)
{
    NTSTATUS Status;
    ULONG EntrySize;
    PDATABASE_ENTRY Entry;
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get the entry at the given position */
    ByteOffset.QuadPart = StartingOffset;
    Status = ZwReadFile(Database,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        &EntrySize,
                        sizeof(EntrySize),
                        &ByteOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    /* If entry doesn't exist, truncate database */
    if (!EntrySize)
    {
        TruncateRemoteDatabase(Database, StartingOffset);
        return NULL;
    }

    /* Allocate the entry */
    Entry = AllocatePool(EntrySize);
    if (!Entry)
    {
        return NULL;
    }

    /* Effectively read the entry */
    Status = ZwReadFile(Database,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        Entry,
                        EntrySize,
                        &ByteOffset,
                        NULL);
    /* If it fails or returns inconsistent data, drop it (= truncate) */
    if (!NT_SUCCESS(Status) ||
        (IoStatusBlock.Information != EntrySize) ||
        (EntrySize < sizeof(DATABASE_ENTRY)) )
    {
        TruncateRemoteDatabase(Database, StartingOffset);
        FreePool(Entry);
        return NULL;
    }

    /* Validate entry */
    if (MAX(Entry->SymbolicNameOffset + Entry->SymbolicNameLength,
            Entry->UniqueIdOffset + Entry->UniqueIdLength) > (LONG)EntrySize)
    {
        TruncateRemoteDatabase(Database, StartingOffset);
        FreePool(Entry);
        return NULL;
    }

    return Entry;
}

/*
 * @implemented
 */
NTSTATUS
WriteRemoteDatabaseEntry(IN HANDLE Database,
                         IN LONG Offset,
                         IN PDATABASE_ENTRY Entry)
{
    NTSTATUS Status;
    LARGE_INTEGER ByteOffset;
    IO_STATUS_BLOCK IoStatusBlock;

    ByteOffset.QuadPart = Offset;
    Status = ZwWriteFile(Database,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         Entry,
                         Entry->EntrySize,
                         &ByteOffset,
                         NULL);
    if (NT_SUCCESS(Status))
    {
        if (IoStatusBlock.Information < Entry->EntrySize)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
DeleteRemoteDatabaseEntry(IN HANDLE Database,
                          IN LONG StartingOffset)
{
    ULONG EndSize;
    PVOID TmpBuffer;
    NTSTATUS Status;
    ULONG DatabaseSize;
    PDATABASE_ENTRY Entry;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER EndEntriesOffset;

    /* First, get database size */
    DatabaseSize = GetRemoteDatabaseSize(Database);
    if (!DatabaseSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Then, get the entry to remove */
    Entry = GetRemoteDatabaseEntry(Database, StartingOffset);
    if (!Entry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate parameters: ensure we won't get negative size */
    if (Entry->EntrySize + StartingOffset > DatabaseSize)
    {
        /* If we get invalid parameters, truncate the whole database
         * starting the wrong entry. We can't rely on the rest
         */
        FreePool(Entry);
        return TruncateRemoteDatabase(Database, StartingOffset);
    }

    /* Now, get the size of the remaining entries (those after the one to remove) */
    EndSize = DatabaseSize - Entry->EntrySize - StartingOffset;
    /* Allocate a buffer big enough to hold them */
    TmpBuffer = AllocatePool(EndSize);
    if (!TmpBuffer)
    {
        FreePool(Entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get the offset of the entry right after the one to delete */
    EndEntriesOffset.QuadPart = Entry->EntrySize + StartingOffset;
    /* We don't need the entry any more */
    FreePool(Entry);

    /* Read the ending entries */
    Status = ZwReadFile(Database, NULL, NULL, NULL, &IoStatusBlock,
                        TmpBuffer, EndSize, &EndEntriesOffset, NULL);
    if (!NT_SUCCESS(Status))
    {
        FreePool(TmpBuffer);
        return Status;
    }

    /* Ensure nothing went wrong - we don't want to corrupt the DB */
    if (IoStatusBlock.Information != EndSize)
    {
        FreePool(TmpBuffer);
        return STATUS_INVALID_PARAMETER;
    }

    /* Remove the entry */
    Status = TruncateRemoteDatabase(Database, StartingOffset + EndSize);
    if (!NT_SUCCESS(Status))
    {
        FreePool(TmpBuffer);
        return Status;
    }

    /* Now, shift the ending entries to erase the entry */
    EndEntriesOffset.QuadPart = StartingOffset;
    Status = ZwWriteFile(Database, NULL, NULL, NULL, &IoStatusBlock,
                         TmpBuffer, EndSize, &EndEntriesOffset, NULL);

    FreePool(TmpBuffer);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DeleteFromLocalDatabaseRoutine(IN PWSTR ValueName,
                               IN ULONG ValueType,
                               IN PVOID ValueData,
                               IN ULONG ValueLength,
                               IN PVOID Context,
                               IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;

    UNREFERENCED_PARAMETER(ValueType);
    UNREFERENCED_PARAMETER(EntryContext);

    /* Ensure it matches, and delete */
    if ((UniqueId->UniqueIdLength == ValueLength) &&
        (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) ==
         ValueLength))
    {
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                               DatabasePath,
                               ValueName);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
DeleteFromLocalDatabase(IN PUNICODE_STRING SymbolicLink,
                        IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = DeleteFromLocalDatabaseRoutine;
    QueryTable[0].Name = SymbolicLink->Buffer;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           DatabasePath,
                           QueryTable,
                           UniqueId,
                           NULL);
}

/*
 * @implemented
 */
NTSTATUS
WaitForRemoteDatabaseSemaphore(IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    LARGE_INTEGER Timeout;

    /* Wait for 7 minutes */
    Timeout.QuadPart = 0xFA0A1F00;
    Status = KeWaitForSingleObject(&(DeviceExtension->RemoteDatabaseLock), Executive, KernelMode, FALSE, &Timeout);
    if (Status != STATUS_TIMEOUT)
    {
        return Status;
    }

    return STATUS_IO_TIMEOUT;
}

/*
 * @implemented
 */
VOID
ReleaseRemoteDatabaseSemaphore(IN PDEVICE_EXTENSION DeviceExtension)
{
    KeReleaseSemaphore(&(DeviceExtension->RemoteDatabaseLock), IO_NO_INCREMENT, 1, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
QueryUniqueIdQueryRoutine(IN PWSTR ValueName,
                          IN ULONG ValueType,
                          IN PVOID ValueData,
                          IN ULONG ValueLength,
                          IN PVOID Context,
                          IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID IntUniqueId;
    PMOUNTDEV_UNIQUE_ID * UniqueId;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueType);
    UNREFERENCED_PARAMETER(EntryContext);

    /* Sanity check */
    if (ValueLength >= 0x10000)
    {
        return STATUS_SUCCESS;
    }

    /* Allocate the Unique ID */
    IntUniqueId = AllocatePool(sizeof(UniqueId) + ValueLength);
    if (IntUniqueId)
    {
        /* Copy data & return */
        IntUniqueId->UniqueIdLength = (USHORT)ValueLength;
        RtlCopyMemory(&(IntUniqueId->UniqueId), ValueData, ValueLength);

        UniqueId = Context;
        *UniqueId = IntUniqueId;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
QueryUniqueIdFromMaster(IN PDEVICE_EXTENSION DeviceExtension,
                        IN PUNICODE_STRING SymbolicName,
                        OUT PMOUNTDEV_UNIQUE_ID * UniqueId)
{
    NTSTATUS Status;
    PDEVICE_INFORMATION DeviceInformation;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    /* Query the unique ID */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = QueryUniqueIdQueryRoutine;
    QueryTable[0].Name = SymbolicName->Buffer;

    *UniqueId = NULL;
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           DatabasePath,
                           QueryTable,
                           UniqueId,
                           NULL);
    /* Unique ID found, no need to go farther */
    if (*UniqueId)
    {
        return STATUS_SUCCESS;
    }

    /* Otherwise, find associate device information */
    Status = FindDeviceInfo(DeviceExtension, SymbolicName, FALSE, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *UniqueId = AllocatePool(DeviceInformation->UniqueId->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
    if (!*UniqueId)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return this unique ID (better than nothing) */
    (*UniqueId)->UniqueIdLength = DeviceInformation->UniqueId->UniqueIdLength;
    RtlCopyMemory(&((*UniqueId)->UniqueId), &(DeviceInformation->UniqueId->UniqueId), (*UniqueId)->UniqueIdLength);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
WriteUniqueIdToMaster(IN PDEVICE_EXTENSION DeviceExtension,
                      IN PDATABASE_ENTRY DatabaseEntry)
{
    NTSTATUS Status;
    PWCHAR SymbolicName;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING SymbolicString;
    PDEVICE_INFORMATION DeviceInformation;

    /* Create symbolic name from database entry */
    SymbolicName = AllocatePool(DatabaseEntry->SymbolicNameLength + sizeof(WCHAR));
    if (!SymbolicName)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(SymbolicName,
                  (PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->SymbolicNameOffset),
                  DatabaseEntry->SymbolicNameLength);
    SymbolicName[DatabaseEntry->SymbolicNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    /* Associate the unique ID with the name from remote database */
    Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   DatabasePath,
                                   SymbolicName,
                                   REG_BINARY,
                                   (PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset),
                                   DatabaseEntry->UniqueIdLength);
    FreePool(SymbolicName);

    /* Reget symbolic name */
    SymbolicString.Length = DatabaseEntry->SymbolicNameLength;
    SymbolicString.MaximumLength = DatabaseEntry->SymbolicNameLength;
    SymbolicString.Buffer = (PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->SymbolicNameOffset);

    /* Find the device using this unique ID */
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

        if (RtlCompareMemory((PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset),
                             DeviceInformation->UniqueId->UniqueId,
                             DatabaseEntry->UniqueIdLength) == DatabaseEntry->UniqueIdLength)
        {
            break;
        }
    }

    /* If found, create a mount point */
    if (NextEntry != &(DeviceExtension->DeviceListHead))
    {
        MountMgrCreatePointWorker(DeviceExtension, &SymbolicString, &(DeviceInformation->DeviceName));
    }

    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
ReconcileThisDatabaseWithMasterWorker(IN PVOID Parameter)
{
    ULONG Offset;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PDATABASE_ENTRY DatabaseEntry;
    HANDLE DatabaseHandle, Handle;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDEVICE_INFORMATION ListDeviceInfo;
    PLIST_ENTRY Entry, EntryInfo, NextEntry;
    PASSOCIATED_DEVICE_ENTRY AssociatedDevice;
    BOOLEAN HardwareErrors, Restart, FailedFinding;
    WCHAR FileNameBuffer[0x8], SymbolicNameBuffer[100];
    UNICODE_STRING ReparseFile, FileName, SymbolicName, VolumeName;
    FILE_REPARSE_POINT_INFORMATION ReparsePointInformation, SavedReparsePointInformation;
    PDEVICE_EXTENSION DeviceExtension = ((PRECONCILE_WORK_ITEM_CONTEXT)Parameter)->DeviceExtension;
    PDEVICE_INFORMATION DeviceInformation = ((PRECONCILE_WORK_ITEM_CONTEXT)Parameter)->DeviceInformation;

    /* We're unloading, do nothing */
    if (Unloading)
    {
        return;
    }

    /* Lock remote DB */
    if (!NT_SUCCESS(WaitForRemoteDatabaseSemaphore(DeviceExtension)))
    {
        return;
    }

    /* Recheck for unloading */
    if (Unloading)
    {
        goto ReleaseRDS;
    }

    /* Find the DB to reconcile */
    KeWaitForSingleObject(&DeviceExtension->DeviceLock, Executive, KernelMode, FALSE, NULL);
    for (Entry = DeviceExtension->DeviceListHead.Flink;
         Entry != &DeviceExtension->DeviceListHead;
         Entry = Entry->Flink)
    {
        ListDeviceInfo = CONTAINING_RECORD(Entry, DEVICE_INFORMATION, DeviceListEntry);
        if (ListDeviceInfo == DeviceInformation)
        {
            break;
        }
    }

    /* If not found, or if removable, bail out */
    if (Entry == &DeviceExtension->DeviceListHead || DeviceInformation->Removable)
    {
        KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
        goto ReleaseRDS;
    }

    /* Get our device object */
    Status = IoGetDeviceObjectPointer(&ListDeviceInfo->DeviceName, FILE_READ_ATTRIBUTES, &FileObject, &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
        goto ReleaseRDS;
    }

    /* Mark mounted only if not unloading */
    if (!(DeviceObject->Flags & DO_UNLOAD_PENDING))
    {
        InterlockedExchangeAdd(&ListDeviceInfo->MountState, 1);
    }

    ObDereferenceObject(FileObject);

    /* Force default: no DB, and need for reconcile */
    DeviceInformation->NeedsReconcile = TRUE;
    DeviceInformation->NoDatabase = TRUE;
    FailedFinding = FALSE;

    /* Remove any associated device that refers to the DB to reconcile */
    for (Entry = DeviceExtension->DeviceListHead.Flink;
         Entry != &DeviceExtension->DeviceListHead;
         Entry = Entry->Flink)
    {
        ListDeviceInfo = CONTAINING_RECORD(Entry, DEVICE_INFORMATION, DeviceListEntry);

        EntryInfo = ListDeviceInfo->AssociatedDevicesHead.Flink;
        while (EntryInfo != &ListDeviceInfo->AssociatedDevicesHead)
        {
            AssociatedDevice = CONTAINING_RECORD(EntryInfo, ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);
            NextEntry = EntryInfo->Flink;

            if (AssociatedDevice->DeviceInformation == DeviceInformation)
            {
                RemoveEntryList(&AssociatedDevice->AssociatedDevicesEntry);
                FreePool(AssociatedDevice->String.Buffer);
                FreePool(AssociatedDevice);
            }

            EntryInfo = NextEntry;
        }
    }

    /* Open the remote database */
    DatabaseHandle = OpenRemoteDatabase(DeviceInformation, FALSE);

    /* Prepare a string with reparse point index */
    ReparseFile.Length = DeviceInformation->DeviceName.Length + ReparseIndex.Length;
    ReparseFile.MaximumLength = ReparseFile.Length + sizeof(UNICODE_NULL);
    ReparseFile.Buffer = AllocatePool(ReparseFile.MaximumLength);
    if (ReparseFile.Buffer == NULL)
    {
        if (DatabaseHandle != 0)
        {
            CloseRemoteDatabase(DatabaseHandle);
        }
        KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
        goto ReleaseRDS;
    }

    RtlCopyMemory(ReparseFile.Buffer, DeviceInformation->DeviceName.Buffer,
                  DeviceInformation->DeviceName.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)ReparseFile.Buffer + DeviceInformation->DeviceName.Length),
                  ReparseIndex.Buffer, ReparseIndex.Length);
    ReparseFile.Buffer[ReparseFile.Length / sizeof(WCHAR)] = UNICODE_NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                               &ReparseFile,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open reparse point directory */
    HardwareErrors = IoSetThreadHardErrorMode(FALSE);
    Status = ZwOpenFile(&Handle,
                        FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    IoSetThreadHardErrorMode(HardwareErrors);

    FreePool(ReparseFile.Buffer);

    if (!NT_SUCCESS(Status))
    {
        if (DatabaseHandle != 0)
        {
            TruncateRemoteDatabase(DatabaseHandle, 0);
            CloseRemoteDatabase(DatabaseHandle);
        }
        KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
        goto ReleaseRDS;
    }

    /* Query reparse point information
     * We only pay attention to mout point
     */
    RtlZeroMemory(FileNameBuffer, sizeof(FileNameBuffer));
    FileName.Buffer = FileNameBuffer;
    FileName.Length = sizeof(FileNameBuffer);
    FileName.MaximumLength = sizeof(FileNameBuffer);
    ((PULONG)FileNameBuffer)[0] = IO_REPARSE_TAG_MOUNT_POINT;
    Status = ZwQueryDirectoryFile(Handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  &ReparsePointInformation,
                                  sizeof(FILE_REPARSE_POINT_INFORMATION),
                                  FileReparsePointInformation,
                                  TRUE,
                                  &FileName,
                                  FALSE);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(Handle);
        if (DatabaseHandle != 0)
        {
            TruncateRemoteDatabase(DatabaseHandle, 0);
            CloseRemoteDatabase(DatabaseHandle);
        }
        KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
        goto ReleaseRDS;
    }

    /* If we failed to open the remote DB previously,
     * retry this time allowing migration (and thus, creation if required)
     */
    if (DatabaseHandle == 0)
    {
        DatabaseHandle = OpenRemoteDatabase(DeviceInformation, TRUE);
        if (DatabaseHandle == 0)
        {
            KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
            goto ReleaseRDS;
        }
    }

    KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);

    /* Reset all the references to our DB entries */
    Offset = 0;
    for (;;)
    {
        DatabaseEntry = GetRemoteDatabaseEntry(DatabaseHandle, Offset);
        if (DatabaseEntry == NULL)
        {
            break;
        }

        DatabaseEntry->EntryReferences = 0;
        Status = WriteRemoteDatabaseEntry(DatabaseHandle, Offset, DatabaseEntry);
        if (!NT_SUCCESS(Status))
        {
            FreePool(DatabaseEntry);
            goto CloseReparse;
        }

        Offset += DatabaseEntry->EntrySize;
        FreePool(DatabaseEntry);
    }

    /* Init string for QueryVolumeName call */
    SymbolicName.MaximumLength = sizeof(SymbolicNameBuffer);
    SymbolicName.Length = 0;
    SymbolicName.Buffer = SymbolicNameBuffer;
    Restart = TRUE;

    /* Start looping on reparse points */
    for (;;)
    {
        RtlCopyMemory(&SavedReparsePointInformation, &ReparsePointInformation, sizeof(FILE_REPARSE_POINT_INFORMATION));
        Status = ZwQueryDirectoryFile(Handle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      &ReparsePointInformation,
                                      sizeof(FILE_REPARSE_POINT_INFORMATION),
                                      FileReparsePointInformation,
                                      TRUE,
                                      Restart ? &FileName : NULL,
                                      Restart);
        /* Restart only once */
        if (Restart)
        {
            Restart = FALSE;
        }
        else
        {
            /* If we get the same one, we're done, bail out */
            if (ReparsePointInformation.FileReference == SavedReparsePointInformation.FileReference &&
                ReparsePointInformation.Tag == SavedReparsePointInformation.Tag)
            {
                break;
            }
        }

        /* If querying failed, or if onloading, or if not returning mount points, bail out */
        if (!NT_SUCCESS(Status) || Unloading || ReparsePointInformation.Tag != IO_REPARSE_TAG_MOUNT_POINT)
        {
            break;
        }

        /* Get the volume name associated to the mount point */
        Status = QueryVolumeName(Handle, &ReparsePointInformation, 0, &SymbolicName, &VolumeName);
        if (!NT_SUCCESS(Status))
        {
            continue;
        }

        /* Browse the DB to find the name */
        Offset = 0;
        for (;;)
        {
            UNICODE_STRING DbName;

            DatabaseEntry = GetRemoteDatabaseEntry(DatabaseHandle, Offset);
            if (DatabaseEntry == NULL)
            {
                break;
            }

            DbName.MaximumLength = DatabaseEntry->SymbolicNameLength;
            DbName.Length = DbName.MaximumLength;
            DbName.Buffer = (PWSTR)((ULONG_PTR)DatabaseEntry + DatabaseEntry->SymbolicNameOffset);
            /* Found, we're done! */
            if (RtlEqualUnicodeString(&DbName, &SymbolicName, TRUE))
            {
                break;
            }

            Offset += DatabaseEntry->EntrySize;
            FreePool(DatabaseEntry);
        }

        /* If we found the mount point.... */
        if (DatabaseEntry != NULL)
        {
            /* If it was referenced, reference it once more and update to remote */
            if (DatabaseEntry->EntryReferences)
            {
                ++DatabaseEntry->EntryReferences;
                Status = WriteRemoteDatabaseEntry(DatabaseHandle, Offset, DatabaseEntry);
                if (!NT_SUCCESS(Status))
                {
                    goto FreeDBEntry;
                }

                FreePool(DatabaseEntry);
            }
            else
            {
                /* Query the Unique ID associated to that mount point in case it changed */
                KeWaitForSingleObject(&DeviceExtension->DeviceLock, Executive, KernelMode, FALSE, NULL);
                Status = QueryUniqueIdFromMaster(DeviceExtension, &SymbolicName, &UniqueId);
                if (!NT_SUCCESS(Status))
                {
                    /* If we failed doing so, reuse the old Unique ID and push it to master */
                    Status = WriteUniqueIdToMaster(DeviceExtension, DatabaseEntry);
                    if (!NT_SUCCESS(Status))
                    {
                        goto ReleaseDeviceLock;
                    }

                    /* And then, reference & write the entry */
                    ++DatabaseEntry->EntryReferences;
                    Status = WriteRemoteDatabaseEntry(DatabaseHandle, Offset, DatabaseEntry);
                    if (!NT_SUCCESS(Status))
                    {
                        goto ReleaseDeviceLock;
                    }

                    KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
                    FreePool(DatabaseEntry);
                }
                /* If the Unique ID didn't change */
                else if (UniqueId->UniqueIdLength == DatabaseEntry->UniqueIdLength &&
                         RtlCompareMemory(UniqueId->UniqueId,
                                          (PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset),
                                          UniqueId->UniqueIdLength) == UniqueId->UniqueIdLength)
                {
                    /* Reference the entry, and update to remote */
                    ++DatabaseEntry->EntryReferences;
                    Status = WriteRemoteDatabaseEntry(DatabaseHandle, Offset, DatabaseEntry);
                    if (!NT_SUCCESS(Status))
                    {
                        goto FreeUniqueId;
                    }

                    FreePool(UniqueId);
                    KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
                    FreePool(DatabaseEntry);
                }
                /* Would, by chance, the Unique ID be present elsewhere? */
                else if (IsUniqueIdPresent(DeviceExtension, DatabaseEntry))
                {
                    /* Push the ID to master */
                    Status = WriteUniqueIdToMaster(DeviceExtension, DatabaseEntry);
                    if (!NT_SUCCESS(Status))
                    {
                        goto FreeUniqueId;
                    }

                    /* And then, reference & write the entry */
                    ++DatabaseEntry->EntryReferences;
                    Status = WriteRemoteDatabaseEntry(DatabaseHandle, Offset, DatabaseEntry);
                    if (!NT_SUCCESS(Status))
                    {
                        goto FreeUniqueId;
                    }

                    FreePool(UniqueId);
                    KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
                    FreePool(DatabaseEntry);
                }
                else
                {
                    /* OK, at that point, we're facing a totally unknown unique ID
                     * So, get rid of the old entry, and recreate a new one with
                     * the know unique ID
                     */
                    Status = DeleteRemoteDatabaseEntry(DatabaseHandle, Offset);
                    if (!NT_SUCCESS(Status))
                    {
                        goto FreeUniqueId;
                    }

                    FreePool(DatabaseEntry);
                    /* Allocate a new entry big enough */
                    DatabaseEntry = AllocatePool(UniqueId->UniqueIdLength + SymbolicName.Length + sizeof(DATABASE_ENTRY));
                    if (DatabaseEntry == NULL)
                    {
                       goto FreeUniqueId;
                    }

                    /* Configure it */
                    DatabaseEntry->EntrySize = UniqueId->UniqueIdLength + SymbolicName.Length + sizeof(DATABASE_ENTRY);
                    DatabaseEntry->EntryReferences = 1;
                    DatabaseEntry->SymbolicNameOffset = sizeof(DATABASE_ENTRY);
                    DatabaseEntry->SymbolicNameLength = SymbolicName.Length;
                    DatabaseEntry->UniqueIdOffset = SymbolicName.Length + sizeof(DATABASE_ENTRY);
                    DatabaseEntry->UniqueIdLength = UniqueId->UniqueIdLength;
                    RtlCopyMemory((PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->SymbolicNameOffset), SymbolicName.Buffer, DatabaseEntry->SymbolicNameLength);
                    RtlCopyMemory((PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset), UniqueId->UniqueId, UniqueId->UniqueIdLength);

                    /* And write it remotely */
                    Status = AddRemoteDatabaseEntry(DatabaseHandle, DatabaseEntry);
                    if (!NT_SUCCESS(Status))
                    {
                       FreePool(DatabaseEntry);
                       goto FreeUniqueId;
                    }

                    FreePool(UniqueId);
                    FreePool(DatabaseEntry);
                    KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
                }
            }
        }
        else
        {
            /* We failed finding it remotely
             * So, let's allocate a new remote DB entry
             */
            KeWaitForSingleObject(&DeviceExtension->DeviceLock, Executive, KernelMode, FALSE, NULL);
            /* To be able to do so, we need the device Unique ID, ask master */
            Status = QueryUniqueIdFromMaster(DeviceExtension, &SymbolicName, &UniqueId);
            KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
            if (NT_SUCCESS(Status))
            {
                /* Allocate a new entry big enough */
                DatabaseEntry = AllocatePool(UniqueId->UniqueIdLength + SymbolicName.Length + sizeof(DATABASE_ENTRY));
                if (DatabaseEntry != NULL)
                {
                    /* Configure it */
                    DatabaseEntry->EntrySize = UniqueId->UniqueIdLength + SymbolicName.Length + sizeof(DATABASE_ENTRY);
                    DatabaseEntry->EntryReferences = 1;
                    DatabaseEntry->SymbolicNameOffset = sizeof(DATABASE_ENTRY);
                    DatabaseEntry->SymbolicNameLength = SymbolicName.Length;
                    DatabaseEntry->UniqueIdOffset = SymbolicName.Length + sizeof(DATABASE_ENTRY);
                    DatabaseEntry->UniqueIdLength = UniqueId->UniqueIdLength;
                    RtlCopyMemory((PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->SymbolicNameOffset), SymbolicName.Buffer, DatabaseEntry->SymbolicNameLength);
                    RtlCopyMemory((PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset), UniqueId->UniqueId, UniqueId->UniqueIdLength);

                    /* And write it remotely */
                    Status = AddRemoteDatabaseEntry(DatabaseHandle, DatabaseEntry);
                    FreePool(DatabaseEntry);
                    FreePool(UniqueId);

                    if (!NT_SUCCESS(Status))
                    {
                        goto FreeVolume;
                    }
                }
                else
                {
                    FreePool(UniqueId);
                }
            }
        }

        /* Find info about the device associated associated with the mount point */
        KeWaitForSingleObject(&DeviceExtension->DeviceLock, Executive, KernelMode, FALSE, NULL);
        Status = FindDeviceInfo(DeviceExtension, &SymbolicName, FALSE, &ListDeviceInfo);
        if (!NT_SUCCESS(Status))
        {
            FailedFinding = TRUE;
            FreePool(VolumeName.Buffer);
        }
        else
        {
            /* Associate the device with the currrent DB */
            AssociatedDevice = AllocatePool(sizeof(ASSOCIATED_DEVICE_ENTRY));
            if (AssociatedDevice == NULL)
            {
                FreePool(VolumeName.Buffer);
            }
            else
            {
                AssociatedDevice->DeviceInformation = DeviceInformation;
                AssociatedDevice->String.Length = VolumeName.Length;
                AssociatedDevice->String.MaximumLength = VolumeName.MaximumLength;
                AssociatedDevice->String.Buffer = VolumeName.Buffer;
                InsertTailList(&ListDeviceInfo->AssociatedDevicesHead, &AssociatedDevice->AssociatedDevicesEntry);
            }

            /* If we don't have to skip notifications, notify */
            if (!ListDeviceInfo->SkipNotifications)
            {
                PostOnlineNotification(DeviceExtension, &ListDeviceInfo->SymbolicName);
            }
        }

        KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
    }

    /* We don't need mount points any longer */
    ZwClose(Handle);

    /* Look for the DB again */
    KeWaitForSingleObject(&DeviceExtension->DeviceLock, Executive, KernelMode, FALSE, NULL);
    for (Entry = DeviceExtension->DeviceListHead.Flink;
         Entry != &DeviceExtension->DeviceListHead;
         Entry = Entry->Flink)
    {
        ListDeviceInfo = CONTAINING_RECORD(Entry, DEVICE_INFORMATION, DeviceListEntry);
        if (ListDeviceInfo == DeviceInformation)
        {
            break;
        }
    }

    if (Entry == &DeviceExtension->DeviceListHead)
    {
        ListDeviceInfo = NULL;
    }

    /* Start the pruning loop */
    Offset = 0;
    for (;;)
    {
        /* Get the entry */
        DatabaseEntry = GetRemoteDatabaseEntry(DatabaseHandle, Offset);
        if (DatabaseEntry == NULL)
        {
            break;
        }

        /* It's not referenced anylonger? Prune it */
        if (DatabaseEntry->EntryReferences == 0)
        {
            Status = DeleteRemoteDatabaseEntry(DatabaseHandle, Offset);
            if (!NT_SUCCESS(Status))
            {
                FreePool(DatabaseEntry);
                KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
                goto CloseRDB;
            }
        }
        /* Update the Unique IDs to reflect the changes we might have done previously */
        else
        {
            if (ListDeviceInfo != NULL)
            {
                UpdateReplicatedUniqueIds(ListDeviceInfo, DatabaseEntry);
            }

            Offset += DatabaseEntry->EntrySize;
        }

        FreePool(DatabaseEntry);
    }

    /* We do have a DB now :-) */
    if (ListDeviceInfo != NULL && !FailedFinding)
    {
        DeviceInformation->NoDatabase = FALSE;
    }

    KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);

    goto CloseRDB;

FreeUniqueId:
    FreePool(UniqueId);
ReleaseDeviceLock:
    KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
FreeDBEntry:
    FreePool(DatabaseEntry);
FreeVolume:
    FreePool(VolumeName.Buffer);
CloseReparse:
    ZwClose(Handle);
CloseRDB:
    CloseRemoteDatabase(DatabaseHandle);
ReleaseRDS:
    ReleaseRemoteDatabaseSemaphore(DeviceExtension);
    return;
}

/*
 * @implemented
 */
VOID
NTAPI
WorkerThread(IN PDEVICE_OBJECT DeviceObject,
             IN PVOID Context)
{
    ULONG i;
    KEVENT Event;
    KIRQL OldIrql;
    NTSTATUS Status;
    HANDLE SafeEvent;
    PLIST_ENTRY Entry;
    LARGE_INTEGER Timeout;
    PRECONCILE_WORK_ITEM WorkItem;
    PDEVICE_EXTENSION DeviceExtension;
    OBJECT_ATTRIBUTES ObjectAttributes;

    UNREFERENCED_PARAMETER(DeviceObject);

    InitializeObjectAttributes(&ObjectAttributes,
                               &SafeVolumes,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Timeout.QuadPart = -10000000LL; /* Wait for 1 second */

    /* Wait as long as possible for clearance from autochk
     * We will write remote databases only if it is safe
     * to access volumes.
     * First, given we start before SMSS, wait for the
     * event creation.
     */
    i = 0;
    do
    {
        /* If we started to shutdown, stop waiting forever and jump to last attempt */
        if (Unloading)
        {
            i = 999;
        }
        else
        {
            /* Attempt to open the event */
            Status = ZwOpenEvent(&SafeEvent, EVENT_ALL_ACCESS, &ObjectAttributes);
            if (NT_SUCCESS(Status))
            {
                break;
            }

            /* Wait a bit to give SMSS a chance to create the event */
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, &Timeout);
        }

        ++i;
    }
    while (i < 1000);

    /* We managed to open the event, wait until autochk signals it */
    if (i < 1000)
    {
        do
        {
            Status = ZwWaitForSingleObject(SafeEvent, FALSE, &Timeout);
        }
        while (Status == STATUS_TIMEOUT && !Unloading);

        ZwClose(SafeEvent);
    }

    DeviceExtension = Context;

    InterlockedExchange(&(DeviceExtension->WorkerThreadStatus), 1);

    /* Acquire workers lock */
    KeWaitForSingleObject(&(DeviceExtension->WorkerSemaphore), Executive, KernelMode, FALSE, NULL);

    KeAcquireSpinLock(&(DeviceExtension->WorkerLock), &OldIrql);

    /* Ensure there are workers */
    while (!IsListEmpty(&(DeviceExtension->WorkerQueueListHead)))
    {
        /* Unqueue a worker */
        Entry = RemoveHeadList(&(DeviceExtension->WorkerQueueListHead));
        WorkItem = CONTAINING_RECORD(Entry,
                                     RECONCILE_WORK_ITEM,
                                     WorkerQueueListEntry);

        KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);

        /* Call it */
        WorkItem->WorkerRoutine(WorkItem->Context);

        IoFreeWorkItem(WorkItem->WorkItem);
        FreePool(WorkItem);

        if (InterlockedDecrement(&(DeviceExtension->WorkerReferences)) < 0)
        {
            return;
        }

        KeWaitForSingleObject(&(DeviceExtension->WorkerSemaphore), Executive, KernelMode, FALSE, NULL);
        KeAcquireSpinLock(&(DeviceExtension->WorkerLock), &OldIrql);
    }
    KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);

    InterlockedDecrement(&(DeviceExtension->WorkerReferences));

    /* Reset event */
    KeSetEvent(&UnloadEvent, IO_NO_INCREMENT, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
QueueWorkItem(IN PDEVICE_EXTENSION DeviceExtension,
              IN PRECONCILE_WORK_ITEM WorkItem,
              IN PVOID Context)
{
    KIRQL OldIrql;

    WorkItem->Context = Context;

    /* When called, lock is already acquired */

    /* If noone (-1 as references), start to work */
    if (InterlockedIncrement(&(DeviceExtension->WorkerReferences)) == 0)
    {
        IoQueueWorkItem(WorkItem->WorkItem, WorkerThread, DelayedWorkQueue, DeviceExtension);
    }

    /* Otherwise queue worker for delayed execution */
    KeAcquireSpinLock(&(DeviceExtension->WorkerLock), &OldIrql);
    InsertTailList(&(DeviceExtension->WorkerQueueListHead),
                   &(WorkItem->WorkerQueueListEntry));
    KeReleaseSpinLock(&(DeviceExtension->WorkerLock), OldIrql);

    KeReleaseSemaphore(&(DeviceExtension->WorkerSemaphore), IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
QueryVolumeName(IN HANDLE RootDirectory,
                IN PFILE_REPARSE_POINT_INFORMATION ReparsePointInformation,
                IN PUNICODE_STRING FileName OPTIONAL,
                OUT PUNICODE_STRING SymbolicName,
                OUT PUNICODE_STRING VolumeName)
{
    HANDLE Handle;
    NTSTATUS Status;
    ULONG NeededLength;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILE_NAME_INFORMATION FileNameInfo;
    PREPARSE_DATA_BUFFER ReparseDataBuffer;

    UNREFERENCED_PARAMETER(ReparsePointInformation);

    if (!FileName)
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   OBJ_KERNEL_HANDLE,
                                   RootDirectory,
                                   NULL);
    }
    else
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   FileName,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
    }

    /* Open volume */
    Status = ZwOpenFile(&Handle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        (FileName) ? FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT :
                                     FILE_OPEN_BY_FILE_ID | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Get the reparse point data */
    ReparseDataBuffer = AllocatePool(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!ReparseDataBuffer)
    {
        ZwClose(Handle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ZwFsControlFile(Handle,
                             0,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_GET_REPARSE_POINT,
                             NULL,
                             0,
                             ReparseDataBuffer,
                             MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!NT_SUCCESS(Status))
    {
        FreePool(ReparseDataBuffer);
        ZwClose(Handle);
        return Status;
    }

    /* Check that name can fit in buffer */
    if (ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength + sizeof(UNICODE_NULL) > SymbolicName->MaximumLength)
    {
        FreePool(ReparseDataBuffer);
        ZwClose(Handle);
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Copy symbolic name */
    SymbolicName->Length = ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength;
    RtlCopyMemory(SymbolicName->Buffer,
                  (PWSTR)((ULONG_PTR)ReparseDataBuffer->MountPointReparseBuffer.PathBuffer +
                                     ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameOffset),
                  ReparseDataBuffer->MountPointReparseBuffer.SubstituteNameLength);

    FreePool(ReparseDataBuffer);

    /* Name has to \ terminated */
    if (SymbolicName->Buffer[SymbolicName->Length / sizeof(WCHAR) - 1] != L'\\')
    {
        ZwClose(Handle);
        return STATUS_INVALID_PARAMETER;
    }

    /* So that we can delete it, and match mountmgr requirements */
    SymbolicName->Length -= sizeof(WCHAR);
    SymbolicName->Buffer[SymbolicName->Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Also ensure it's really a volume name... */
    if (!MOUNTMGR_IS_VOLUME_NAME(SymbolicName))
    {
        ZwClose(Handle);
        return STATUS_INVALID_PARAMETER;
    }

    /* Now prepare to really get the name */
    FileNameInfo = AllocatePool(sizeof(FILE_NAME_INFORMATION) + 2 * sizeof(WCHAR));
    if (!FileNameInfo)
    {
        ZwClose(Handle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ZwQueryInformationFile(Handle,
                                    &IoStatusBlock,
                                    FileNameInfo,
                                    sizeof(FILE_NAME_INFORMATION) + 2 * sizeof(WCHAR),
                                    FileNameInformation);
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        /* As expected... Reallocate with proper size */
        NeededLength = FileNameInfo->FileNameLength;
        FreePool(FileNameInfo);

        FileNameInfo = AllocatePool(sizeof(FILE_NAME_INFORMATION) + NeededLength);
        if (!FileNameInfo)
        {
            ZwClose(Handle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* And query name */
        Status = ZwQueryInformationFile(Handle,
                                        &IoStatusBlock,
                                        FileNameInfo,
                                        sizeof(FILE_NAME_INFORMATION) + NeededLength,
                                        FileNameInformation);
    }

    ZwClose(Handle);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Return the volume name */
    VolumeName->Length = (USHORT)FileNameInfo->FileNameLength;
    VolumeName->MaximumLength = (USHORT)FileNameInfo->FileNameLength + sizeof(WCHAR);
    VolumeName->Buffer = AllocatePool(VolumeName->MaximumLength);
    if (!VolumeName->Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(VolumeName->Buffer, FileNameInfo->FileName, FileNameInfo->FileNameLength);
    VolumeName->Buffer[FileNameInfo->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    FreePool(FileNameInfo);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
OnlineMountedVolumes(IN PDEVICE_EXTENSION DeviceExtension,
                     IN PDEVICE_INFORMATION DeviceInformation)
{
    HANDLE Handle;
    NTSTATUS Status;
    BOOLEAN RestartScan;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDEVICE_INFORMATION VolumeDeviceInformation;
    WCHAR FileNameBuffer[0x8], SymbolicNameBuffer[0x64];
    UNICODE_STRING ReparseFile, FileName, SymbolicName, VolumeName;
    FILE_REPARSE_POINT_INFORMATION ReparsePointInformation, SavedReparsePointInformation;

    /* Removable devices don't have remote database on them */
    if (DeviceInformation->Removable)
    {
        return;
    }

    /* Prepare a string with reparse point index */
    ReparseFile.Length = DeviceInformation->DeviceName.Length + ReparseIndex.Length;
    ReparseFile.MaximumLength = ReparseFile.Length + sizeof(UNICODE_NULL);
    ReparseFile.Buffer = AllocatePool(ReparseFile.MaximumLength);
    if (!ReparseFile.Buffer)
    {
        return;
    }

    RtlCopyMemory(ReparseFile.Buffer, DeviceInformation->DeviceName.Buffer,
                  DeviceInformation->DeviceName.Length);
    RtlCopyMemory((PVOID)((ULONG_PTR)ReparseFile.Buffer + DeviceInformation->DeviceName.Length),
                  ReparseFile.Buffer, ReparseFile.Length);
    ReparseFile.Buffer[ReparseFile.Length / sizeof(WCHAR)] = UNICODE_NULL;

    InitializeObjectAttributes(&ObjectAttributes,
                               &ReparseFile,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open reparse point */
    Status = ZwOpenFile(&Handle,
                        FILE_GENERIC_READ,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT | FILE_OPEN_REPARSE_POINT);
    FreePool(ReparseFile.Buffer);
    if (!NT_SUCCESS(Status))
    {
        DeviceInformation->NoDatabase = FALSE;
        return;
    }

    /* Query reparse point information
     * We only pay attention to mout point
     */
    RtlZeroMemory(FileNameBuffer, sizeof(FileNameBuffer));
    FileName.Buffer = FileNameBuffer;
    FileName.Length = sizeof(FileNameBuffer);
    FileName.MaximumLength = sizeof(FileNameBuffer);
    ((PULONG)FileNameBuffer)[0] = IO_REPARSE_TAG_MOUNT_POINT;
    Status = ZwQueryDirectoryFile(Handle,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatusBlock,
                                  &ReparsePointInformation,
                                  sizeof(FILE_REPARSE_POINT_INFORMATION),
                                  FileReparsePointInformation,
                                  TRUE,
                                  &FileName,
                                  FALSE);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(Handle);
        return;
    }

    RestartScan = TRUE;

    /* Query mount points */
    while (TRUE)
    {
        SymbolicName.Length = 0;
        SymbolicName.MaximumLength = sizeof(SymbolicNameBuffer);
        SymbolicName.Buffer = SymbolicNameBuffer;
        RtlCopyMemory(&SavedReparsePointInformation, &ReparsePointInformation, sizeof(FILE_REPARSE_POINT_INFORMATION));

        Status = ZwQueryDirectoryFile(Handle,
                                      NULL,
                                      NULL,
                                      NULL,
                                      &IoStatusBlock,
                                      &ReparsePointInformation,
                                      sizeof(FILE_REPARSE_POINT_INFORMATION),
                                      FileReparsePointInformation,
                                      TRUE,
                                      (RestartScan) ? &FileName : NULL,
                                      RestartScan);
         if (!RestartScan)
         {
             if (ReparsePointInformation.FileReference == SavedReparsePointInformation.FileReference &&
                 ReparsePointInformation.Tag == SavedReparsePointInformation.Tag)
             {
                 break;
             }
         }
         else
         {
             RestartScan = FALSE;
         }

         if (!NT_SUCCESS(Status) || ReparsePointInformation.Tag != IO_REPARSE_TAG_MOUNT_POINT)
         {
             break;
         }

         /* Get the volume name associated to the mount point */
         Status = QueryVolumeName(Handle,
                                  &ReparsePointInformation,
                                  NULL, &SymbolicName,
                                  &VolumeName);
         if (!NT_SUCCESS(Status))
         {
             continue;
         }

         FreePool(VolumeName.Buffer);

         /* Get its information */
         Status = FindDeviceInfo(DeviceExtension, &SymbolicName,
                                 FALSE, &VolumeDeviceInformation);
         if (!NT_SUCCESS(Status))
         {
             DeviceInformation->NoDatabase = TRUE;
             continue;
         }

         /* If notification are enabled, mark it online */
         if (!DeviceInformation->SkipNotifications)
         {
             PostOnlineNotification(DeviceExtension, &VolumeDeviceInformation->SymbolicName);
         }
    }

    ZwClose(Handle);
}

/*
 * @implemented
 */
VOID
ReconcileThisDatabaseWithMaster(IN PDEVICE_EXTENSION DeviceExtension,
                                IN PDEVICE_INFORMATION DeviceInformation)
{
    PRECONCILE_WORK_ITEM WorkItem;

    /* Removable devices don't have remote database */
    if (DeviceInformation->Removable)
    {
        return;
    }

    /* Allocate a work item */
    WorkItem = AllocatePool(sizeof(RECONCILE_WORK_ITEM));
    if (!WorkItem)
    {
        return;
    }

    WorkItem->WorkItem = IoAllocateWorkItem(DeviceExtension->DeviceObject);
    if (!WorkItem->WorkItem)
    {
        FreePool(WorkItem);
        return;
    }

    /* And queue it */
    WorkItem->WorkerRoutine = ReconcileThisDatabaseWithMasterWorker;
    WorkItem->DeviceExtension = DeviceExtension;
    WorkItem->DeviceInformation = DeviceInformation;
    QueueWorkItem(DeviceExtension, WorkItem, &(WorkItem->DeviceExtension));

    /* If there's no automount, and automatic letters
     * all volumes to find those online and notify there presence
     */
    if (DeviceExtension->WorkerThreadStatus == 0 &&
        DeviceExtension->AutomaticDriveLetter == 1 &&
        DeviceExtension->NoAutoMount == FALSE)
    {
        OnlineMountedVolumes(DeviceExtension, DeviceInformation);
    }
}

/*
 * @implemented
 */
VOID
ReconcileAllDatabasesWithMaster(IN PDEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY NextEntry;
    PDEVICE_INFORMATION DeviceInformation;

    /* Browse all the devices */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &(DeviceExtension->DeviceListHead);
         NextEntry = NextEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry,
                                              DEVICE_INFORMATION,
                                              DeviceListEntry);
        /* If it's not removable, then, it might have a database to sync */
        if (!DeviceInformation->Removable)
        {
            ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInformation);
        }
    }
}

/*
 * @implemented
 */
VOID
NTAPI
CreateRemoteDatabaseWorker(IN PDEVICE_OBJECT DeviceObject,
                           IN PVOID Context)
{
    NTSTATUS Status;
    HANDLE Database = 0;
    UNICODE_STRING DatabaseName;
    PMIGRATE_WORK_ITEM WorkItem;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDEVICE_INFORMATION DeviceInformation;

    UNREFERENCED_PARAMETER(DeviceObject);

    /* Extract context */
    WorkItem = Context;
    DeviceInformation = WorkItem->DeviceInformation;

    /* Reconstruct appropriate string */
    DatabaseName.Length = DeviceInformation->DeviceName.Length + RemoteDatabase.Length;
    DatabaseName.MaximumLength = DatabaseName.Length + sizeof(WCHAR);
    DatabaseName.Buffer = AllocatePool(DatabaseName.MaximumLength);
    if (DatabaseName.Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    /* Create the required folder (in which the database will be stored
     * \System Volume Information at root of the volume
     */
    Status = RtlCreateSystemVolumeInformationFolder(&(DeviceInformation->DeviceName));
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Finish initiating strings */
    RtlCopyMemory(DatabaseName.Buffer, DeviceInformation->DeviceName.Buffer, DeviceInformation->DeviceName.Length);
    RtlCopyMemory(DatabaseName.Buffer + (DeviceInformation->DeviceName.Length / sizeof(WCHAR)),
                  RemoteDatabase.Buffer, RemoteDatabase.Length);
    DatabaseName.Buffer[DatabaseName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Create database */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DatabaseName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = IoCreateFile(&Database,
                          SYNCHRONIZE | READ_CONTROL | FILE_WRITE_ATTRIBUTES |
                          FILE_READ_ATTRIBUTES | FILE_WRITE_EA | FILE_READ_EA |
                          FILE_APPEND_DATA | FILE_WRITE_DATA | FILE_READ_DATA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                          0,
                          FILE_CREATE,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_STOP_ON_SYMLINK | IO_NO_PARAMETER_CHECKING);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_STOPPED_ON_SYMLINK)
        {
            DPRINT1("Attempt to exploit CVE-2015-1769. See CORE-10216\n");
        }

        Database = 0;
        goto Cleanup;
    }

Cleanup:
    if (DatabaseName.Buffer)
    {
        FreePool(DatabaseName.Buffer);
    }

    if (NT_SUCCESS(Status))
    {
        DeviceInformation->Migrated = 1;
    }
    else if (Database != 0)
    {
        ZwClose(Database);
    }

    IoFreeWorkItem(WorkItem->WorkItem);

    WorkItem->WorkItem = NULL;
    WorkItem->Status = Status;
    WorkItem->Database = Database;

    KeSetEvent(WorkItem->Event, 0, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
CreateRemoteDatabase(IN PDEVICE_INFORMATION DeviceInformation,
                     IN OUT PHANDLE Database)
{
    KEVENT Event;
    NTSTATUS Status;
    PMIGRATE_WORK_ITEM WorkItem;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Allocate a work item dedicated to migration */
    WorkItem = AllocatePool(sizeof(MIGRATE_WORK_ITEM));
    if (!WorkItem)
    {
        *Database = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(WorkItem, sizeof(MIGRATE_WORK_ITEM));
    WorkItem->Event = &Event;
    WorkItem->DeviceInformation = DeviceInformation;
    WorkItem->WorkItem = IoAllocateWorkItem(DeviceInformation->DeviceExtension->DeviceObject);
    if (!WorkItem->WorkItem)
    {
        FreePool(WorkItem);
        *Database = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* And queue it */
    IoQueueWorkItem(WorkItem->WorkItem,
                    CreateRemoteDatabaseWorker,
                    DelayedWorkQueue,
                    WorkItem);

    KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    Status = WorkItem->Status;

    *Database = (NT_SUCCESS(Status) ? WorkItem->Database : 0);

    FreePool(WorkItem);
    return Status;
}

/*
 * @implemented
 */
HANDLE
OpenRemoteDatabase(IN PDEVICE_INFORMATION DeviceInformation,
                   IN BOOLEAN MigrateDatabase)
{
    HANDLE Database;
    NTSTATUS Status;
    BOOLEAN PreviousMode;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DeviceRemoteDatabase;

    Database = 0;

    /* Get database name */
    DeviceRemoteDatabase.Length = DeviceInformation->DeviceName.Length + RemoteDatabase.Length;
    DeviceRemoteDatabase.MaximumLength = DeviceRemoteDatabase.Length + sizeof(WCHAR);
    DeviceRemoteDatabase.Buffer = AllocatePool(DeviceRemoteDatabase.MaximumLength);
    if (!DeviceRemoteDatabase.Buffer)
    {
        return 0;
    }

    RtlCopyMemory(DeviceRemoteDatabase.Buffer, DeviceInformation->DeviceName.Buffer, DeviceInformation->DeviceName.Length);
    RtlCopyMemory(DeviceRemoteDatabase.Buffer + (DeviceInformation->DeviceName.Length / sizeof(WCHAR)),
                  RemoteDatabase.Buffer, RemoteDatabase.Length);
    DeviceRemoteDatabase.Buffer[DeviceRemoteDatabase.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Open database */
    InitializeObjectAttributes(&ObjectAttributes,
                               &DeviceRemoteDatabase,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Disable hard errors */
    PreviousMode = IoSetThreadHardErrorMode(FALSE);

    Status = IoCreateFile(&Database,
                          SYNCHRONIZE | READ_CONTROL | FILE_WRITE_ATTRIBUTES |
                          FILE_READ_ATTRIBUTES | FILE_WRITE_EA | FILE_READ_EA |
                          FILE_APPEND_DATA | FILE_WRITE_DATA | FILE_READ_DATA,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN,
                          0,
                          (!MigrateDatabase || DeviceInformation->Migrated == 0) ? FILE_OPEN_IF : FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_STOP_ON_SYMLINK | IO_NO_PARAMETER_CHECKING);
    if (Status == STATUS_STOPPED_ON_SYMLINK)
    {
        DPRINT1("Attempt to exploit CVE-2015-1769. See CORE-10216\n");
    }

    /* If base it to be migrated and was opened successfully, go ahead */
    if (MigrateDatabase && NT_SUCCESS(Status))
    {
        CreateRemoteDatabase(DeviceInformation, &Database);
    }

    IoSetThreadHardErrorMode(PreviousMode);
    FreePool(DeviceRemoteDatabase.Buffer);

    return Database;
}

/*
 * @implemented
 */
VOID
ChangeRemoteDatabaseUniqueId(IN PDEVICE_INFORMATION DeviceInformation,
                             IN PMOUNTDEV_UNIQUE_ID OldUniqueId,
                             IN PMOUNTDEV_UNIQUE_ID NewUniqueId)
{
    LONG Offset = 0;
    HANDLE Database;
    PDATABASE_ENTRY Entry, NewEntry;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Open the remote database */
    Database = OpenRemoteDatabase(DeviceInformation, FALSE);
    if (!Database)
    {
        return;
    }

    /* Get all the entries */
    do
    {
        Entry = GetRemoteDatabaseEntry(Database, Offset);
        if (!Entry)
        {
            break;
        }

        /* Not the correct entry, skip it */
        if (Entry->UniqueIdLength != OldUniqueId->UniqueIdLength)
        {
            Offset += Entry->EntrySize;
            FreePool(Entry);
            continue;
        }

        /* Not the correct entry, skip it */
        if (RtlCompareMemory(OldUniqueId->UniqueId,
                             (PVOID)((ULONG_PTR)Entry + Entry->UniqueIdOffset),
                             Entry->UniqueIdLength) != Entry->UniqueIdLength)
        {
            Offset += Entry->EntrySize;
            FreePool(Entry);
            continue;
        }

        /* Here, we have the correct entry */
        NewEntry = AllocatePool(Entry->EntrySize + NewUniqueId->UniqueIdLength - OldUniqueId->UniqueIdLength);
        if (!NewEntry)
        {
            Offset += Entry->EntrySize;
            FreePool(Entry);
            continue;
        }

        /* Recreate the entry from the previous one */
        NewEntry->EntrySize = Entry->EntrySize + NewUniqueId->UniqueIdLength - OldUniqueId->UniqueIdLength;
        NewEntry->EntryReferences = Entry->EntryReferences;
        NewEntry->SymbolicNameOffset = sizeof(DATABASE_ENTRY);
        NewEntry->SymbolicNameLength = Entry->SymbolicNameLength;
        NewEntry->UniqueIdOffset = Entry->SymbolicNameLength + sizeof(DATABASE_ENTRY);
        NewEntry->UniqueIdLength = NewUniqueId->UniqueIdLength;
        RtlCopyMemory((PVOID)((ULONG_PTR)NewEntry + NewEntry->SymbolicNameOffset),
                      (PVOID)((ULONG_PTR)Entry + Entry->SymbolicNameOffset),
                      NewEntry->SymbolicNameLength);
        RtlCopyMemory((PVOID)((ULONG_PTR)NewEntry + NewEntry->UniqueIdOffset),
                      NewUniqueId->UniqueId, NewEntry->UniqueIdLength);

        /* Delete old entry */
        Status = DeleteRemoteDatabaseEntry(Database, Offset);
        if (!NT_SUCCESS(Status))
        {
            FreePool(Entry);
            FreePool(NewEntry);
            break;
        }

        /* And replace with new one */
        Status = AddRemoteDatabaseEntry(Database, NewEntry);
        FreePool(Entry);
        FreePool(NewEntry);
    } while (NT_SUCCESS(Status));

    CloseRemoteDatabase(Database);

    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DeleteDriveLetterRoutine(IN PWSTR ValueName,
                         IN ULONG ValueType,
                         IN PVOID ValueData,
                         IN ULONG ValueLength,
                         IN PVOID Context,
                         IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID UniqueId;
    UNICODE_STRING RegistryEntry;

    UNREFERENCED_PARAMETER(EntryContext);

    if (ValueType != REG_BINARY)
    {
        return STATUS_SUCCESS;
    }

    UniqueId = Context;

    /* First ensure we have the correct data */
    if (UniqueId->UniqueIdLength != ValueLength)
    {
        return STATUS_SUCCESS;
    }

    if (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) != ValueLength)
    {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&RegistryEntry, ValueName);

    /* Then, it's a drive letter, erase it */
    if (IsDriveLetter(&RegistryEntry))
    {
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                               DatabasePath,
                               ValueName);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
DeleteRegistryDriveLetter(IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = DeleteDriveLetterRoutine;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           DatabasePath,
                           QueryTable,
                           UniqueId,
                           NULL);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
DeleteNoDriveLetterEntryRoutine(IN PWSTR ValueName,
                                IN ULONG ValueType,
                                IN PVOID ValueData,
                                IN ULONG ValueLength,
                                IN PVOID Context,
                                IN PVOID EntryContext)
{
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;

    UNREFERENCED_PARAMETER(EntryContext);

    /* Ensure we have correct input */
    if (ValueName[0] != L'#' || ValueType != REG_BINARY ||
        UniqueId->UniqueIdLength != ValueLength)
    {
        return STATUS_SUCCESS;
    }

    /* And then, if unique ID matching, delete entry */
    if (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) != ValueLength)
    {
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                               DatabasePath,
                               ValueName);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
DeleteNoDriveLetterEntry(IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = DeleteNoDriveLetterEntryRoutine;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           DatabasePath,
                           QueryTable,
                           UniqueId,
                           NULL);
}
