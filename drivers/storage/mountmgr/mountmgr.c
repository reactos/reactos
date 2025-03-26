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
 * FILE:             drivers/filesystem/mountmgr/mountmgr.c
 * PURPOSE:          Mount Manager
 * PROGRAMMER:       Pierre Schweitzer (pierre.schweitzer@reactos.org)
 *                   Alex Ionescu (alex.ionescu@reactos.org)
 */

#include "mntmgr.h"

#define NDEBUG
#include <debug.h>

/* FIXME */
GUID MountedDevicesGuid = {0x53F5630D, 0xB6BF, 0x11D0, {0x94, 0xF2, 0x00, 0xA0, 0xC9, 0x1E, 0xFB, 0x8B}};

PDEVICE_OBJECT gdeviceObject;
KEVENT UnloadEvent;
LONG Unloading;

static const WCHAR Cunc[] = L"\\??\\C:";
#define Cunc_LETTER_POSITION 4

/**
 * @brief
 * Sends a synchronous IOCTL to the specified device object.
 *
 * @param[in]   IoControlCode
 * The IOCTL to send to the device.
 *
 * @param[in]   DeviceObject
 * Pointer to the device object that will handle the IOCTL.
 *
 * @param[in]   InputBuffer
 * Optional pointer to a buffer containing input data for the IOCTL.
 * When specified, the buffer should be at least of InputBufferLength size.
 *
 * @param[in]   InputBufferLength
 * Size in bytes, of the buffer pointed by InputBuffer.
 *
 * @param[out]  OutputBuffer
 * Optional pointer to a buffer that will receive output data from the IOCTL.
 * When specified, the buffer should be at least of OutputBufferLength size.
 *
 * @param[in]   OutputBufferLength
 * Size in bytes, of the buffer pointed by OutputBuffer.
 *
 * @param[in]   FileObject
 * Optional pointer to a file object that may be necessary for the IOCTL.
 *
 * @return
 * An NTSTATUS code indicating success or failure of this function.
 *
 * @note
 * Must be called at PASSIVE_LEVEL with all APCs enabled.
 **/
_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
MountMgrSendSyncDeviceIoCtl(
    _In_ ULONG IoControlCode,
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_opt_(OutputBufferLength) PVOID OutputBuffer,
    _In_ ULONG OutputBufferLength,
    _In_opt_ PFILE_OBJECT FileObject)
{
    NTSTATUS Status;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatusBlock;
    PIRP Irp;

    /* We must be at passive level as we are using an on-stack event, and
     * APCs must be enabled for allowing the Special Kernel APC queued by
     * the IO Manager to run for completing the IRP */
    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);
    ASSERT(!KeAreAllApcsDisabled());

    /* Initialize the on-stack notification event and build the threaded IRP */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IoControlCode,
                                        DeviceObject,
                                        InputBuffer,
                                        InputBufferLength,
                                        OutputBuffer,
                                        OutputBufferLength,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Set up the FileObject for the IOCTL if required */
    if (FileObject)
        IoGetNextIrpStackLocation(Irp)->FileObject = FileObject;

    /* Finally, call the driver and wait for IRP completion if necessary */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
IsOffline(PUNICODE_STRING SymbolicName)
{
    NTSTATUS Status;
    ULONG IsOffline, Default;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    /* Prepare to look in the registry to see if
     * given volume is offline
     */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = SymbolicName->Buffer;
    QueryTable[0].EntryContext = &IsOffline;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultLength = sizeof(ULONG);
    QueryTable[0].DefaultData = &Default;

    Default = 0;

    /* Query status */
    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    OfflinePath,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        IsOffline = 0;
    }

    return (IsOffline != 0);
}

/*
 * @implemented
 */
BOOLEAN
HasDriveLetter(IN PDEVICE_INFORMATION DeviceInformation)
{
    PLIST_ENTRY NextEntry;
    PSYMLINK_INFORMATION SymlinkInfo;

    /* Browse all the symlinks to check if there is at least a drive letter */
    for (NextEntry = DeviceInformation->SymbolicLinksListHead.Flink;
         NextEntry != &DeviceInformation->SymbolicLinksListHead;
         NextEntry = NextEntry->Flink)
    {
        SymlinkInfo = CONTAINING_RECORD(NextEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

        if (IsDriveLetter(&SymlinkInfo->Name) && SymlinkInfo->Online)
        {
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * @implemented
 */
NTSTATUS
CreateNewDriveLetterName(OUT PUNICODE_STRING DriveLetter,
                         IN PUNICODE_STRING DeviceName,
                         IN UCHAR Letter,
                         IN PMOUNTDEV_UNIQUE_ID UniqueId OPTIONAL)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Allocate a big enough buffer to contain the symbolic link */
    DriveLetter->MaximumLength = DosDevices.Length + 3 * sizeof(WCHAR);
    DriveLetter->Buffer = AllocatePool(DriveLetter->MaximumLength);
    if (!DriveLetter->Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy prefix */
    RtlCopyUnicodeString(DriveLetter, &DosDevices);

    /* Update string to reflect real contents */
    DriveLetter->Length = DosDevices.Length + 2 * sizeof(WCHAR);
    DriveLetter->Buffer[DosDevices.Length / sizeof(WCHAR) + 2] = UNICODE_NULL;
    DriveLetter->Buffer[DosDevices.Length / sizeof(WCHAR) + 1] = L':';

    /* If caller wants a no drive entry */
    if (Letter == (UCHAR)-1)
    {
        /* Then, create a no letter entry */
        CreateNoDriveLetterEntry(UniqueId);
        FreePool(DriveLetter->Buffer);
        return STATUS_UNSUCCESSFUL;
    }
    else if (Letter)
    {
        /* Use the letter given by the caller */
        DriveLetter->Buffer[DosDevices.Length / sizeof(WCHAR)] = (WCHAR)Letter;
        Status = GlobalCreateSymbolicLink(DriveLetter, DeviceName);
        if (NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* If caller didn't provide a letter, let's find one for him */

    if (RtlPrefixUnicodeString(&DeviceFloppy, DeviceName, TRUE))
    {
        /* If the device is a floppy, start with letter A */
        Letter = 'A';
    }
    else if (RtlPrefixUnicodeString(&DeviceCdRom, DeviceName, TRUE))
    {
        /* If the device is a CD-ROM, start with letter D */
        Letter = 'D';
    }
    else
    {
        /* Finally, if it's a disk, use C */
        Letter = 'C';
    }

    /* Try to affect a letter (up to Z, ofc) until it's possible */
    for (; Letter <= 'Z'; Letter++)
    {
        DriveLetter->Buffer[DosDevices.Length / sizeof(WCHAR)] = (WCHAR)Letter;
        Status = GlobalCreateSymbolicLink(DriveLetter, DeviceName);
        if (NT_SUCCESS(Status))
        {
            DPRINT("Assigned drive %c: to %wZ\n", Letter, DeviceName);
            return Status;
        }
    }

    /* We failed to allocate a letter */
    FreePool(DriveLetter->Buffer);
    DPRINT("Failed to create a drive letter for %wZ\n", DeviceName);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
QueryDeviceInformation(
    _In_ PUNICODE_STRING SymbolicName,
    _Out_opt_ PUNICODE_STRING DeviceName,
    _Out_opt_ PMOUNTDEV_UNIQUE_ID* UniqueId,
    _Out_opt_ PBOOLEAN Removable,
    _Out_opt_ PBOOLEAN GptDriveLetter,
    _Out_opt_ PBOOLEAN HasGuid,
    _Inout_opt_ LPGUID StableGuid,
    _Out_opt_ PBOOLEAN IsFT)
{
    NTSTATUS Status;
    USHORT Size;
    BOOLEAN IsRemovable;
    PMOUNTDEV_NAME Name;
    PMOUNTDEV_UNIQUE_ID Id;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    PARTITION_INFORMATION_EX PartitionInfo;
    STORAGE_DEVICE_NUMBER StorageDeviceNumber;
    VOLUME_GET_GPT_ATTRIBUTES_INFORMATION GptAttributes;

    /* Get device associated with the symbolic name */
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* The associate FO can't have a file name */
    if (FileObject->FileName.Length)
    {
        ObDereferenceObject(FileObject);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Check if it's removable & return to the user (if asked to) */
    IsRemovable = (FileObject->DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA);
    if (Removable)
    {
        *Removable = IsRemovable;
    }

    /* Get the attached device */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    /* If we've been asked for a GPT drive letter */
    if (GptDriveLetter)
    {
        /* Consider it has one */
        *GptDriveLetter = TRUE;

        if (!IsRemovable)
        {
            /* Query the GPT attributes */
            Status = MountMgrSendSyncDeviceIoCtl(IOCTL_VOLUME_GET_GPT_ATTRIBUTES,
                                                 DeviceObject,
                                                 NULL,
                                                 0,
                                                 &GptAttributes,
                                                 sizeof(GptAttributes),
                                                 NULL);
            /* Failure isn't major */
            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_SUCCESS;
            }
            /* Check if it has a drive letter */
            else if (GptAttributes.GptAttributes & GPT_BASIC_DATA_ATTRIBUTE_NO_DRIVE_LETTER)
            {
                *GptDriveLetter = FALSE;
            }
        }
    }

    /* If caller wants to know if this is a FT volume */
    if (IsFT)
    {
        /* Suppose it's not */
        *IsFT = FALSE;

        /* FT volume can't be removable */
        if (!IsRemovable)
        {
            /* Query partition information */
            Status = MountMgrSendSyncDeviceIoCtl(IOCTL_DISK_GET_PARTITION_INFO_EX,
                                                 DeviceObject,
                                                 NULL,
                                                 0,
                                                 &PartitionInfo,
                                                 sizeof(PartitionInfo),
                                                 NULL);
            /* Failure isn't major */
            if (!NT_SUCCESS(Status))
            {
                Status = STATUS_SUCCESS;
            }
            /* Check if this is a FT volume */
            else if ((PartitionInfo.PartitionStyle == PARTITION_STYLE_MBR) &&
                     IsFTPartition(PartitionInfo.Mbr.PartitionType))
            {
                *IsFT = TRUE;
            }

            /* It looks like a FT volume. Verify it is really one by checking
             * that it does NOT lie on a specific storage device (i.e. it is
             * not a basic volume). */
            if (*IsFT)
            {
                Status = MountMgrSendSyncDeviceIoCtl(IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                                     DeviceObject,
                                                     NULL,
                                                     0,
                                                     &StorageDeviceNumber,
                                                     sizeof(StorageDeviceNumber),
                                                     NULL);
                if (!NT_SUCCESS(Status))
                    Status = STATUS_SUCCESS;
                else
                    *IsFT = FALSE; // Succeeded, so this cannot be a FT volume.
            }
        }
    }

    /* If caller needs device name */
    if (DeviceName)
    {
        /* Allocate a buffer just to request length */
        Name = AllocatePool(sizeof(MOUNTDEV_NAME));
        if (!Name)
        {
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Query device name */
        Status = MountMgrSendSyncDeviceIoCtl(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                             DeviceObject,
                                             NULL,
                                             0,
                                             Name,
                                             sizeof(MOUNTDEV_NAME),
                                             FileObject);
        /* Retry with appropriate length */
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            Size = Name->NameLength + sizeof(MOUNTDEV_NAME);

            FreePool(Name);

            /* Allocate proper size */
            Name = AllocatePool(Size);
            if (!Name)
            {
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* And query name (for real that time) */
            Status = MountMgrSendSyncDeviceIoCtl(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                                 DeviceObject,
                                                 NULL,
                                                 0,
                                                 Name,
                                                 Size,
                                                 FileObject);
        }

        if (NT_SUCCESS(Status))
        {
            /* Copy back found name to the caller */
            DeviceName->Length = Name->NameLength;
            DeviceName->MaximumLength = Name->NameLength + sizeof(WCHAR);
            DeviceName->Buffer = AllocatePool(DeviceName->MaximumLength);
            if (!DeviceName->Buffer)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                RtlCopyMemory(DeviceName->Buffer, Name->Name, Name->NameLength);
                DeviceName->Buffer[Name->NameLength / sizeof(WCHAR)] = UNICODE_NULL;
            }
        }

        FreePool(Name);
    }

    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return Status;
    }

    /* If caller wants device unique ID */
    if (UniqueId)
    {
        /* Prepare buffer to probe length */
        Id = AllocatePool(sizeof(MOUNTDEV_UNIQUE_ID));
        if (!Id)
        {
            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Query unique ID length */
        Status = MountMgrSendSyncDeviceIoCtl(IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
                                             DeviceObject,
                                             NULL,
                                             0,
                                             Id,
                                             sizeof(MOUNTDEV_UNIQUE_ID),
                                             FileObject);
        /* Retry with appropriate length */
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            Size = Id->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID);

            FreePool(Id);

            /* Allocate the correct buffer */
            Id = AllocatePool(Size);
            if (!Id)
            {
                ObDereferenceObject(DeviceObject);
                ObDereferenceObject(FileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Query unique ID */
            Status = MountMgrSendSyncDeviceIoCtl(IOCTL_MOUNTDEV_QUERY_UNIQUE_ID,
                                                 DeviceObject,
                                                 NULL,
                                                 0,
                                                 Id,
                                                 Size,
                                                 FileObject);
        }

        /* Hands back unique ID */
        if (NT_SUCCESS(Status))
        {
            *UniqueId = Id;
        }
        else
        {
            /* In case of failure, also free the rest */
            FreePool(Id);
            if (DeviceName->Length)
                FreePool(DeviceName->Buffer);

            ObDereferenceObject(DeviceObject);
            ObDereferenceObject(FileObject);
            return Status;
        }
    }

    /* If user wants to know about GUID */
    if (HasGuid)
    {
        /* Query device stable GUID */
        NTSTATUS IntStatus;
        IntStatus = MountMgrSendSyncDeviceIoCtl(IOCTL_MOUNTDEV_QUERY_STABLE_GUID,
                                                DeviceObject,
                                                NULL,
                                                0,
                                                StableGuid,
                                                sizeof(GUID),
                                                FileObject);
        *HasGuid = NT_SUCCESS(IntStatus);
    }

    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
FindDeviceInfo(IN PDEVICE_EXTENSION DeviceExtension,
               IN PUNICODE_STRING SymbolicName,
               IN BOOLEAN DeviceNameGiven,
               OUT PDEVICE_INFORMATION * DeviceInformation)
{
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING DeviceName;
    PDEVICE_INFORMATION DeviceInfo = NULL;

    /* If a device name was given, use it */
    if (DeviceNameGiven)
    {
        DeviceName.Length = SymbolicName->Length;
        DeviceName.Buffer = SymbolicName->Buffer;
    }
    else
    {
        /* Otherwise, query it */
        Status = QueryDeviceInformation(SymbolicName,
                                        &DeviceName,
                                        NULL, NULL,
                                        NULL, NULL,
                                        NULL, NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* Look for device information matching devive */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &(DeviceExtension->DeviceListHead);
         NextEntry = NextEntry->Flink)
    {
        DeviceInfo = CONTAINING_RECORD(NextEntry,
                                       DEVICE_INFORMATION,
                                       DeviceListEntry);

        if (RtlEqualUnicodeString(&DeviceName, &(DeviceInfo->DeviceName), TRUE))
        {
            break;
        }
    }

    /* Release our buffer if required */
    if (!DeviceNameGiven)
    {
        FreePool(DeviceName.Buffer);
    }

    /* Return found information */
    if (NextEntry == &(DeviceExtension->DeviceListHead))
    {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    *DeviceInformation = DeviceInfo;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
MountMgrFreeDeadDeviceInfo(IN PDEVICE_INFORMATION DeviceInformation)
{
    FreePool(DeviceInformation->SymbolicName.Buffer);
    FreePool(DeviceInformation);
}

/*
 * @implemented
 */
VOID
MountMgrFreeMountedDeviceInfo(IN PDEVICE_INFORMATION DeviceInformation)
{
    PLIST_ENTRY NextEntry;
    PSYMLINK_INFORMATION SymLink;
    PUNIQUE_ID_REPLICATE UniqueId;
    PASSOCIATED_DEVICE_ENTRY AssociatedDevice;

    /* Purge symbolic links list */
    while (!IsListEmpty(&(DeviceInformation->SymbolicLinksListHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceInformation->SymbolicLinksListHead));
        SymLink = CONTAINING_RECORD(NextEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

        GlobalDeleteSymbolicLink(&(SymLink->Name));
        FreePool(SymLink->Name.Buffer);
    }

    /* Purge replicated unique IDs list */
    while (!IsListEmpty(&(DeviceInformation->ReplicatedUniqueIdsListHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceInformation->ReplicatedUniqueIdsListHead));
        UniqueId = CONTAINING_RECORD(NextEntry, UNIQUE_ID_REPLICATE, ReplicatedUniqueIdsListEntry);

        FreePool(UniqueId->UniqueId);
        FreePool(UniqueId);
    }

    while (!IsListEmpty(&(DeviceInformation->AssociatedDevicesHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceInformation->AssociatedDevicesHead));
        AssociatedDevice = CONTAINING_RECORD(NextEntry, ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);

        FreePool(AssociatedDevice->String.Buffer);
        FreePool(AssociatedDevice);
    }

    /* Free the rest of the buffers */
    FreePool(DeviceInformation->SymbolicName.Buffer);
    if (DeviceInformation->KeepLinks)
    {
        FreePool(DeviceInformation->UniqueId);
    }
    FreePool(DeviceInformation->DeviceName.Buffer);

    /* Finally, stop waiting for notifications for this device */
    if (DeviceInformation->TargetDeviceNotificationEntry)
    {
        IoUnregisterPlugPlayNotification(DeviceInformation->TargetDeviceNotificationEntry);
    }
}

/*
 * @implemented
 */
VOID
MountMgrFreeSavedLink(IN PSAVED_LINK_INFORMATION SavedLinkInformation)
{
    PLIST_ENTRY NextEntry;
    PSYMLINK_INFORMATION SymlinkInformation;

    /* For all the saved links */
    while (!IsListEmpty(&(SavedLinkInformation->SymbolicLinksListHead)))
    {
        NextEntry = RemoveHeadList(&(SavedLinkInformation->SymbolicLinksListHead));
        SymlinkInformation = CONTAINING_RECORD(NextEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

        /* Remove from system & free */
        GlobalDeleteSymbolicLink(&(SymlinkInformation->Name));
        FreePool(SymlinkInformation->Name.Buffer);
        FreePool(SymlinkInformation);
    }

    /* And free unique ID & entry */
    FreePool(SavedLinkInformation->UniqueId);
    FreePool(SavedLinkInformation);
}


/*
 * @implemented
 */
VOID
NTAPI
MountMgrUnload(IN PDRIVER_OBJECT DriverObject)
{
    PLIST_ENTRY NextEntry;
    PUNIQUE_ID_WORK_ITEM WorkItem;
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_INFORMATION DeviceInformation;
    PSAVED_LINK_INFORMATION SavedLinkInformation;

    UNREFERENCED_PARAMETER(DriverObject);

    /* Don't get notification any longer */
    IoUnregisterShutdownNotification(gdeviceObject);

    /* Free registry buffer */
    DeviceExtension = gdeviceObject->DeviceExtension;
    if (DeviceExtension->RegistryPath.Buffer)
    {
        FreePool(DeviceExtension->RegistryPath.Buffer);
        DeviceExtension->RegistryPath.Buffer = NULL;
    }

    InterlockedExchange(&Unloading, TRUE);

    KeInitializeEvent(&UnloadEvent, NotificationEvent, FALSE);

    /* Wait for workers to finish */
    if (InterlockedIncrement(&DeviceExtension->WorkerReferences) > 0)
    {
        KeReleaseSemaphore(&(DeviceExtension->WorkerSemaphore),
                           IO_NO_INCREMENT, 1, FALSE);

        KeWaitForSingleObject(&UnloadEvent, Executive, KernelMode, FALSE, NULL);
    }
    else
    {
        InterlockedDecrement(&(DeviceExtension->WorkerReferences));
    }

    /* Don't get any notification any longer² */
    IoUnregisterPlugPlayNotification(DeviceExtension->NotificationEntry);

    /* Acquire the driver exclusively */
    KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode,
                          FALSE, NULL);

    /* Clear offline devices list */
    while (!IsListEmpty(&(DeviceExtension->OfflineDeviceListHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceExtension->OfflineDeviceListHead));
        DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);
        MountMgrFreeDeadDeviceInfo(DeviceInformation);
    }

    /* Clear saved links list */
    while (!IsListEmpty(&(DeviceExtension->SavedLinksListHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceExtension->SavedLinksListHead));
        SavedLinkInformation = CONTAINING_RECORD(NextEntry, SAVED_LINK_INFORMATION, SavedLinksListEntry);
        MountMgrFreeSavedLink(SavedLinkInformation);
    }

    /* Clear workers list */
    while (!IsListEmpty(&(DeviceExtension->UniqueIdWorkerItemListHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceExtension->UniqueIdWorkerItemListHead));
        WorkItem = CONTAINING_RECORD(NextEntry, UNIQUE_ID_WORK_ITEM, UniqueIdWorkerItemListEntry);

        KeClearEvent(&UnloadEvent);
        WorkItem->Event = &UnloadEvent;

        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT,
                           1, FALSE);

        IoCancelIrp(WorkItem->Irp);
        KeWaitForSingleObject(&UnloadEvent, Executive, KernelMode, FALSE, NULL);

        IoFreeIrp(WorkItem->Irp);
        FreePool(WorkItem->DeviceName.Buffer);
        FreePool(WorkItem->IrpBuffer);
        FreePool(WorkItem);

        KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode,
                              FALSE, NULL);
    }

    /* If we have drive letter data, release */
    if (DeviceExtension->DriveLetterData)
    {
        FreePool(DeviceExtension->DriveLetterData);
        DeviceExtension->DriveLetterData = NULL;
    }

    /* Release driver & quit */
    KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

    GlobalDeleteSymbolicLink(&DosDevicesMount);
    IoDeleteDevice(gdeviceObject);
}

/**
 * @brief   Retrieves the "NoAutoMount" setting.
 * @return  TRUE if AutoMount is disabled; FALSE if AutoMount is enabled.
 **/
CODE_SEG("INIT")
BOOLEAN
MountmgrReadNoAutoMount(
    _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    ULONG Result, Default = 0;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    /* Retrieve data from registry */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = L"NoAutoMount";
    QueryTable[0].EntryContext = &Result;
    QueryTable[0].DefaultType = REG_DWORD;
    QueryTable[0].DefaultData = &Default;
    QueryTable[0].DefaultLength = sizeof(Default);

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    RegistryPath->Buffer,
                                    QueryTable,
                                    NULL,
                                    NULL);
    if (!NT_SUCCESS(Status))
        Result = Default;

    return (Result != 0);
}

/*
 * @implemented
 */
NTSTATUS
MountMgrMountedDeviceArrival(IN PDEVICE_EXTENSION DeviceExtension,
                             IN PUNICODE_STRING SymbolicName,
                             IN BOOLEAN ManuallyRegistered)
{
    WCHAR Letter;
    GUID StableGuid;
    HANDLE LinkHandle;
    ULONG SymLinkCount, i;
    PLIST_ENTRY NextEntry;
    PUNICODE_STRING SymLinks;
    NTSTATUS Status, IntStatus;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSYMLINK_INFORMATION SymlinkInformation;
    PMOUNTDEV_UNIQUE_ID UniqueId, NewUniqueId;
    PSAVED_LINK_INFORMATION SavedLinkInformation;
    PDEVICE_INFORMATION DeviceInformation, CurrentDevice;
    WCHAR CSymLinkBuffer[RTL_NUMBER_OF(Cunc)], LinkTargetBuffer[MAX_PATH];
    UNICODE_STRING TargetDeviceName, SuggestedLinkName, DeviceName, VolumeName, DriveLetter, LinkTarget, CSymLink;
    BOOLEAN HasGuid, HasGptDriveLetter, IsFT, UseOnlyIfThereAreNoOtherLinks;
    BOOLEAN IsDrvLetter, IsOff, IsVolumeName, SetOnline;

    /* New device = new structure to represent it */
    DeviceInformation = AllocatePool(sizeof(DEVICE_INFORMATION));
    if (!DeviceInformation)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialise device structure */
    RtlZeroMemory(DeviceInformation, sizeof(DEVICE_INFORMATION));
    InitializeListHead(&(DeviceInformation->SymbolicLinksListHead));
    InitializeListHead(&(DeviceInformation->ReplicatedUniqueIdsListHead));
    InitializeListHead(&(DeviceInformation->AssociatedDevicesHead));
    DeviceInformation->SymbolicName.Length = SymbolicName->Length;
    DeviceInformation->SymbolicName.MaximumLength = SymbolicName->Length + sizeof(UNICODE_NULL);
    DeviceInformation->SymbolicName.Buffer = AllocatePool(DeviceInformation->SymbolicName.MaximumLength);
    if (!DeviceInformation->SymbolicName.Buffer)
    {
        FreePool(DeviceInformation);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy symbolic name */
    RtlCopyMemory(DeviceInformation->SymbolicName.Buffer, SymbolicName->Buffer, SymbolicName->Length);
    DeviceInformation->SymbolicName.Buffer[DeviceInformation->SymbolicName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    DeviceInformation->ManuallyRegistered = ManuallyRegistered;
    DeviceInformation->DeviceExtension = DeviceExtension;

    /* Query as much data as possible about device */
    Status = QueryDeviceInformation(SymbolicName,
                                    &TargetDeviceName,
                                    &UniqueId,
                                    &(DeviceInformation->Removable),
                                    &HasGptDriveLetter,
                                    &HasGuid,
                                    &StableGuid,
                                    &IsFT);
    if (!NT_SUCCESS(Status))
    {
        KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);

        for (NextEntry = DeviceExtension->OfflineDeviceListHead.Flink;
             NextEntry != &(DeviceExtension->OfflineDeviceListHead);
             NextEntry = NextEntry->Flink)
        {
            CurrentDevice = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

            if (RtlEqualUnicodeString(&(DeviceInformation->SymbolicName), &(CurrentDevice->SymbolicName), TRUE))
            {
                break;
            }
        }

        if (NextEntry != &(DeviceExtension->OfflineDeviceListHead))
        {
            MountMgrFreeDeadDeviceInfo(DeviceInformation);
        }
        else
        {
            InsertTailList(&(DeviceExtension->OfflineDeviceListHead), &(DeviceInformation->DeviceListEntry));
        }

        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

        return Status;
    }

    /* Save gathered data */
    DeviceInformation->UniqueId = UniqueId;
    DeviceInformation->DeviceName = TargetDeviceName;
    DeviceInformation->KeepLinks = FALSE;

    /* If we found system partition, mark it */
    if (DeviceExtension->DriveLetterData && UniqueId->UniqueIdLength == DeviceExtension->DriveLetterData->UniqueIdLength)
    {
        if (RtlCompareMemory(UniqueId->UniqueId, DeviceExtension->DriveLetterData->UniqueId, UniqueId->UniqueIdLength)
            == UniqueId->UniqueIdLength)
        {
            IoSetSystemPartition(&TargetDeviceName);
        }
    }

    /* Check suggested link name */
    Status = QuerySuggestedLinkName(&(DeviceInformation->SymbolicName),
                                    &SuggestedLinkName,
                                    &UseOnlyIfThereAreNoOtherLinks);
    if (!NT_SUCCESS(Status))
    {
        SuggestedLinkName.Buffer = NULL;
    }

    /* If it's OK, set it and save its letter (if any) */
    if (SuggestedLinkName.Buffer && IsDriveLetter(&SuggestedLinkName))
    {
        DeviceInformation->SuggestedDriveLetter = (UCHAR)SuggestedLinkName.Buffer[LETTER_POSITION];
    }

    /* Acquire driver exclusively */
    KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);

    /* Check if we already have device in to prevent double registration */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &(DeviceExtension->DeviceListHead);
         NextEntry = NextEntry->Flink)
    {
        CurrentDevice = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

        if (RtlEqualUnicodeString(&(CurrentDevice->DeviceName), &TargetDeviceName, TRUE))
        {
            break;
        }
    }

    /* If we found it, clear ours, and return success, all correct */
    if (NextEntry != &(DeviceExtension->DeviceListHead))
    {
        if (SuggestedLinkName.Buffer)
        {
            FreePool(SuggestedLinkName.Buffer);
        }

        FreePool(UniqueId);
        FreePool(TargetDeviceName.Buffer);
        FreePool(DeviceInformation->SymbolicName.Buffer);
        FreePool(DeviceInformation);

        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

        return STATUS_SUCCESS;
    }

    /* Check if there are symlinks associated with our device in registry */
    Status = QuerySymbolicLinkNamesFromStorage(DeviceExtension,
                                               DeviceInformation,
                                               (SuggestedLinkName.Buffer) ? &SuggestedLinkName : NULL,
                                               UseOnlyIfThereAreNoOtherLinks,
                                               &SymLinks,
                                               &SymLinkCount,
                                               HasGuid,
                                               &StableGuid);

    /* If our device is a CD-ROM */
    if (RtlPrefixUnicodeString(&DeviceCdRom, &TargetDeviceName, TRUE))
    {
        LinkTarget.Length = 0;
        LinkTarget.MaximumLength = sizeof(LinkTargetBuffer);
        LinkTarget.Buffer = LinkTargetBuffer;

        RtlCopyMemory(CSymLinkBuffer, Cunc, sizeof(Cunc));
        RtlInitUnicodeString(&CSymLink, CSymLinkBuffer);

        /* Start checking all letters that could have been associated */
        for (Letter = L'D'; Letter <= L'Z'; Letter++)
        {
            CSymLink.Buffer[Cunc_LETTER_POSITION] = Letter;

            InitializeObjectAttributes(&ObjectAttributes,
                                       &CSymLink,
                                       OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);

            /* Try to open the associated symlink */
            Status = ZwOpenSymbolicLinkObject(&LinkHandle, SYMBOLIC_LINK_QUERY, &ObjectAttributes);
            if (!NT_SUCCESS(Status))
            {
                continue;
            }

            /* And query its target */
            Status = ZwQuerySymbolicLinkObject(LinkHandle, &LinkTarget, NULL);
            ZwClose(LinkHandle);

            if (!NT_SUCCESS(Status))
            {
                continue;
            }

            IntStatus = STATUS_UNSUCCESSFUL;
            if (!RtlEqualUnicodeString(&LinkTarget, &DeviceInformation->DeviceName, FALSE))
            {
                continue;
            }

            /* This link is matching our device, whereas it's not supposed to have any
             * symlink associated.
             * Delete it
             */
            if (!SymLinkCount)
            {
                IoDeleteSymbolicLink(&CSymLink);
                continue;
            }

            /* Now, for all the symlinks, check for ours */
            for (i = 0; i < SymLinkCount; i++)
            {
                if (IsDriveLetter(&(SymLinks[i])))
                {
                    /* If it exists, that's correct */
                    if (SymLinks[i].Buffer[LETTER_POSITION] == Letter)
                    {
                        IntStatus = STATUS_SUCCESS;
                    }
                }
            }

            /* Useless link, delete it */
            if (IntStatus == STATUS_UNSUCCESSFUL)
            {
                IoDeleteSymbolicLink(&CSymLink);
            }
        }
    }

    /* Suggested name is no longer required */
    if (SuggestedLinkName.Buffer)
    {
        FreePool(SuggestedLinkName.Buffer);
    }

    /* If if failed, ensure we don't take symlinks into account */
    if (!NT_SUCCESS(Status))
    {
        SymLinks = NULL;
        SymLinkCount = 0;
    }

    /* Now we queried them, remove the symlinks */
    SavedLinkInformation = RemoveSavedLinks(DeviceExtension, UniqueId);

    IsDrvLetter = FALSE;
    IsOff = FALSE;
    IsVolumeName = FALSE;
    /* For all the symlinks */
    for (i = 0; i < SymLinkCount; i++)
    {
        /* Check if our device is a volume */
        if (MOUNTMGR_IS_VOLUME_NAME(&(SymLinks[i])))
        {
            IsVolumeName = TRUE;
        }
        /* If it has a drive letter */
        else if (IsDriveLetter(&(SymLinks[i])))
        {
            if (IsDrvLetter)
            {
                DeleteFromLocalDatabase(&(SymLinks[i]), UniqueId);
                continue;
            }
            else
            {
                IsDrvLetter = TRUE;
            }
        }

        /* And recreate the symlink to our device */
        Status = GlobalCreateSymbolicLink(&(SymLinks[i]), &TargetDeviceName);
        if (!NT_SUCCESS(Status))
        {
            BOOLEAN LinkError = TRUE;

            if ((SavedLinkInformation && !RedirectSavedLink(SavedLinkInformation, &(SymLinks[i]), &TargetDeviceName)) ||
                !SavedLinkInformation)
            {
                Status = QueryDeviceInformation(&(SymLinks[i]), &DeviceName, NULL, NULL, NULL, NULL, NULL, NULL);
                if (NT_SUCCESS(Status))
                {
                    LinkError = RtlEqualUnicodeString(&TargetDeviceName, &DeviceName, TRUE);
                    FreePool(DeviceName.Buffer);
                }

                if (!LinkError)
                {
                    if (IsDriveLetter(&(SymLinks[i])))
                    {
                        IsDrvLetter = FALSE;
                        DeleteFromLocalDatabase(&(SymLinks[i]), UniqueId);
                    }

                    FreePool(SymLinks[i].Buffer);
                    continue;
                }
            }
        }

        /* Check if was offline */
        if (IsOffline(&(SymLinks[i])))
        {
            IsOff = TRUE;
        }

        /* Finally, associate this symlink with the device */
        SymlinkInformation = AllocatePool(sizeof(SYMLINK_INFORMATION));
        if (!SymlinkInformation)
        {
            GlobalDeleteSymbolicLink(&(SymLinks[i]));
            FreePool(SymLinks[i].Buffer);
            continue;
        }

        SymlinkInformation->Name = SymLinks[i];
        SymlinkInformation->Online = TRUE;

        InsertTailList(&(DeviceInformation->SymbolicLinksListHead),
                       &(SymlinkInformation->SymbolicLinksListEntry));
    }

    /* Now, for all the recreated symlinks, notify their recreation */
    for (NextEntry = DeviceInformation->SymbolicLinksListHead.Flink;
         NextEntry != &(DeviceInformation->SymbolicLinksListHead);
         NextEntry = NextEntry->Flink)
    {
        SymlinkInformation = CONTAINING_RECORD(NextEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

        SendLinkCreated(&(SymlinkInformation->Name));
    }

    /* If we had saved links, it's time to free them */
    if (SavedLinkInformation)
    {
        MountMgrFreeSavedLink(SavedLinkInformation);
    }

    /* If our device doesn't have a volume name */
    if (!IsVolumeName)
    {
        /* It's time to create one */
        Status = CreateNewVolumeName(&VolumeName, NULL);
        if (NT_SUCCESS(Status))
        {
            /* Write it to global database */
            RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                  DatabasePath,
                                  VolumeName.Buffer,
                                  REG_BINARY,
                                  UniqueId->UniqueId,
                                  UniqueId->UniqueIdLength);

            /* And create the symlink */
            GlobalCreateSymbolicLink(&VolumeName, &TargetDeviceName);

            SymlinkInformation = AllocatePool(sizeof(SYMLINK_INFORMATION));
            if (!SymlinkInformation)
            {
                FreePool(VolumeName.Buffer);
            }
            /* Finally, associate it with the device and notify creation */
            else
            {
                SymlinkInformation->Name = VolumeName;
                SymlinkInformation->Online = TRUE;
                InsertTailList(&(DeviceInformation->SymbolicLinksListHead),
                               &(SymlinkInformation->SymbolicLinksListEntry));

                SendLinkCreated(&VolumeName);
            }
        }
    }

    /* If we found a drive letter, then, ignore the suggested one */
    if (IsDrvLetter)
    {
        DeviceInformation->SuggestedDriveLetter = 0;
    }
    /* Else, it's time to set up one */
    else if ((!DeviceExtension->NoAutoMount || DeviceInformation->Removable) &&
             DeviceExtension->AutomaticDriveLetter &&
             (HasGptDriveLetter || DeviceInformation->SuggestedDriveLetter) &&
             !HasNoDriveLetterEntry(UniqueId))
    {
        /* Create a new drive letter */
        Status = CreateNewDriveLetterName(&DriveLetter, &TargetDeviceName,
                                          DeviceInformation->SuggestedDriveLetter,
                                          NULL);
        if (!NT_SUCCESS(Status))
        {
            CreateNoDriveLetterEntry(UniqueId);
        }
        else
        {
            /* Save it to global database */
            RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                  DatabasePath,
                                  DriveLetter.Buffer,
                                  REG_BINARY,
                                  UniqueId->UniqueId,
                                  UniqueId->UniqueIdLength);

            /* Associate it with the device and notify creation */
            SymlinkInformation = AllocatePool(sizeof(SYMLINK_INFORMATION));
            if (!SymlinkInformation)
            {
                FreePool(DriveLetter.Buffer);
            }
            else
            {
                SymlinkInformation->Name = DriveLetter;
                SymlinkInformation->Online = TRUE;
                InsertTailList(&(DeviceInformation->SymbolicLinksListHead),
                               &(SymlinkInformation->SymbolicLinksListEntry));

                SendLinkCreated(&DriveLetter);
            }
        }
    }

    /* If that's a PnP device, register for notifications */
    if (!ManuallyRegistered)
    {
        RegisterForTargetDeviceNotification(DeviceExtension, DeviceInformation);
    }

    /* Finally, insert the device into our devices list */
    InsertTailList(&(DeviceExtension->DeviceListHead), &(DeviceInformation->DeviceListEntry));

    /* Copy device unique ID */
    NewUniqueId = AllocatePool(UniqueId->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
    if (NewUniqueId)
    {
        NewUniqueId->UniqueIdLength = UniqueId->UniqueIdLength;
        RtlCopyMemory(NewUniqueId->UniqueId, UniqueId->UniqueId, UniqueId->UniqueIdLength);
    }

    /* Skip online notifications if the device is offline or a FT volume */
    if (IsOff || IsFT)
        DeviceInformation->SkipNotifications = TRUE;

    /* If automount is enabled or the device was already mounted, send now
     * the online notification if needed; otherwise, defer its posting */
    if (!DeviceExtension->NoAutoMount || IsDrvLetter)
        SetOnline = !DeviceInformation->SkipNotifications;
    else
        SetOnline = FALSE;

    /* Finally, release the exclusive lock */
    KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

    /* Set the device online now if necessary */
    if (SetOnline)
        SendOnlineNotification(SymbolicName);

    /* If we had symlinks (from storage), free them */
    if (SymLinks)
    {
        FreePool(SymLinks);
    }

    /* Notify about unique id change */
    if (NewUniqueId)
    {
        IssueUniqueIdChangeNotify(DeviceExtension, SymbolicName, NewUniqueId);
        FreePool(NewUniqueId);
    }

    /* If this drive was set to have a drive letter automatically
     * Now it's back, local databases sync will be required
     */
    if (DeviceExtension->AutomaticDriveLetter)
    {
        KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);

        ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInformation);

        NextEntry = DeviceExtension->DeviceListHead.Flink;
        CurrentDevice = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);
        while (CurrentDevice != DeviceInformation)
        {
            if (!CurrentDevice->NoDatabase)
            {
                ReconcileThisDatabaseWithMaster(DeviceExtension, CurrentDevice);
            }

            NextEntry = NextEntry->Flink;
            CurrentDevice = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);
        }

        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
MountMgrMountedDeviceRemoval(IN PDEVICE_EXTENSION DeviceExtension,
                             IN PUNICODE_STRING DeviceName)
{
    PLIST_ENTRY NextEntry, DeviceEntry;
    PUNIQUE_ID_REPLICATE UniqueIdReplicate;
    PSYMLINK_INFORMATION SymlinkInformation;
    PASSOCIATED_DEVICE_ENTRY AssociatedDevice;
    PSAVED_LINK_INFORMATION SavedLinkInformation = NULL;
    PDEVICE_INFORMATION DeviceInformation, CurrentDevice;

    /* Acquire device exclusively */
    KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);

    /* Look for the leaving device */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &(DeviceExtension->DeviceListHead);
         NextEntry = NextEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

        if (!RtlCompareUnicodeString(&(DeviceInformation->SymbolicName), DeviceName, TRUE))
        {
            break;
        }
    }

    /* If we found it */
    if (NextEntry != &(DeviceExtension->DeviceListHead))
    {
        /* If it's asked to keep links, then, prepare to save them */
        if (DeviceInformation->KeepLinks)
        {
            SavedLinkInformation = AllocatePool(sizeof(SAVED_LINK_INFORMATION));
            if (!SavedLinkInformation)
            {
                DeviceInformation->KeepLinks = FALSE;
            }
        }

        /* If it's possible (and asked), start to save them */
        if (DeviceInformation->KeepLinks)
        {
            InsertTailList(&(DeviceExtension->SavedLinksListHead), &(SavedLinkInformation->SavedLinksListEntry));
            InitializeListHead(&(SavedLinkInformation->SymbolicLinksListHead));
            SavedLinkInformation->UniqueId = DeviceInformation->UniqueId;
        }

        /* For all the symlinks */
        while (!IsListEmpty(&(DeviceInformation->SymbolicLinksListHead)))
        {
            NextEntry = RemoveHeadList(&(DeviceInformation->SymbolicLinksListHead));
            SymlinkInformation = CONTAINING_RECORD(NextEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

            /* If we have to, save the link */
            if (DeviceInformation->KeepLinks)
            {
                InsertTailList(&(SavedLinkInformation->SymbolicLinksListHead), &(SymlinkInformation->SymbolicLinksListEntry));
            }
            /* Otherwise, just release it */
            else
            {
                GlobalDeleteSymbolicLink(&(SymlinkInformation->Name));
                FreePool(SymlinkInformation->Name.Buffer);
                FreePool(SymlinkInformation);
            }
        }

        /* Free all the replicated unique IDs */
        while (!IsListEmpty(&(DeviceInformation->ReplicatedUniqueIdsListHead)))
        {
            NextEntry = RemoveHeadList(&(DeviceInformation->ReplicatedUniqueIdsListHead));
            UniqueIdReplicate = CONTAINING_RECORD(NextEntry, UNIQUE_ID_REPLICATE, ReplicatedUniqueIdsListEntry);


            FreePool(UniqueIdReplicate->UniqueId);
            FreePool(UniqueIdReplicate);
        }

        while (!IsListEmpty(&(DeviceInformation->AssociatedDevicesHead)))
        {
            NextEntry = RemoveHeadList(&(DeviceInformation->AssociatedDevicesHead));
            AssociatedDevice = CONTAINING_RECORD(NextEntry, ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);

            DeviceInformation->NoDatabase = TRUE;
            FreePool(AssociatedDevice->String.Buffer);
            FreePool(AssociatedDevice);
        }

        /* Remove device from the device list */
        RemoveEntryList(&(DeviceInformation->DeviceListEntry));

        /* If there are still devices, check if some were associated with ours */
        if (!IsListEmpty(&(DeviceInformation->DeviceListEntry)))
        {
            for (NextEntry = DeviceExtension->DeviceListHead.Flink;
                 NextEntry != &(DeviceExtension->DeviceListHead);
                 NextEntry = NextEntry->Flink)
            {
                CurrentDevice = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

                /* And then, remove them */
                DeviceEntry = CurrentDevice->AssociatedDevicesHead.Flink;
                while (DeviceEntry != &(CurrentDevice->AssociatedDevicesHead))
                {
                    AssociatedDevice = CONTAINING_RECORD(NextEntry, ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);
                    DeviceEntry = DeviceEntry->Flink;

                    if (AssociatedDevice->DeviceInformation != DeviceInformation)
                    {
                        continue;
                    }

                    RemoveEntryList(&(AssociatedDevice->AssociatedDevicesEntry));
                    FreePool(AssociatedDevice->String.Buffer);
                    FreePool(AssociatedDevice);
                }
            }
        }

        /* Finally, clean up device name, symbolic name */
        FreePool(DeviceInformation->SymbolicName.Buffer);
        if (!DeviceInformation->KeepLinks)
        {
            FreePool(DeviceInformation->UniqueId);
        }
        FreePool(DeviceInformation->DeviceName.Buffer);

        /* Unregister notifications */
        if (DeviceInformation->TargetDeviceNotificationEntry)
        {
            IoUnregisterPlugPlayNotification(DeviceInformation->TargetDeviceNotificationEntry);
        }

        /*  And leave */
        FreePool(DeviceInformation);
    }
    else
    {
        /* We didn't find device, perhaps because it was offline */
        for (NextEntry = DeviceExtension->OfflineDeviceListHead.Flink;
             NextEntry != &(DeviceExtension->OfflineDeviceListHead);
             NextEntry = NextEntry->Flink)
        {
            DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

            /* It was, remove it */
            if (RtlCompareUnicodeString(&(DeviceInformation->SymbolicName), DeviceName, TRUE) == 0)
            {
                RemoveEntryList(&(DeviceInformation->DeviceListEntry));
                MountMgrFreeDeadDeviceInfo(DeviceInformation);
                break;
            }
        }
    }

    /* Release driver */
    KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MountMgrMountedDeviceNotification(IN PVOID NotificationStructure,
                                  IN PVOID Context)
{
    BOOLEAN OldState;
    PDEVICE_EXTENSION DeviceExtension;
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION Notification;

    /* Notification for a device arrived */
    /* Disable hard errors */
    OldState = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), TRUE);

    DeviceExtension = Context;
    Notification = NotificationStructure;

    /* Dispatch according to the event */
    if (IsEqualGUID(&(Notification->Event), &GUID_DEVICE_INTERFACE_ARRIVAL))
    {
        MountMgrMountedDeviceArrival(DeviceExtension, Notification->SymbolicLinkName, FALSE);
    }
    else if (IsEqualGUID(&(Notification->Event), &GUID_DEVICE_INTERFACE_REMOVAL))
    {
        MountMgrMountedDeviceRemoval(DeviceExtension, Notification->SymbolicLinkName);
    }

    /* Reset hard errors */
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), OldState);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MountMgrCreateClose(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DeviceObject);

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Allow driver opening for communication
     * as long as it's not taken for a directory
     */
    if (Stack->MajorFunction == IRP_MJ_CREATE &&
        Stack->Parameters.Create.Options & FILE_DIRECTORY_FILE)
    {
        Status = STATUS_NOT_A_DIRECTORY;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
MountMgrCancel(IN PDEVICE_OBJECT DeviceObject,
               IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    RemoveEntryList(&(Irp->Tail.Overlay.ListEntry));

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MountMgrCleanup(IN PDEVICE_OBJECT DeviceObject,
                IN PIRP Irp)
{
    PIRP ListIrp;
    KIRQL OldIrql;
    PLIST_ENTRY NextEntry;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    PDEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;
    Stack = IoGetCurrentIrpStackLocation(Irp);
    FileObject = Stack->FileObject;

    IoAcquireCancelSpinLock(&OldIrql);

    /* If IRP list if empty, it's OK */
    if (IsListEmpty(&(DeviceExtension->IrpListHead)))
    {
        IoReleaseCancelSpinLock(OldIrql);

        Irp->IoStatus.Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return STATUS_SUCCESS;
    }

    /* Otherwise, cancel all the IRPs */
    NextEntry = DeviceExtension->IrpListHead.Flink;
    do
    {
        ListIrp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);
        if (IoGetCurrentIrpStackLocation(ListIrp)->FileObject == FileObject)
        {
            ListIrp->Cancel = TRUE;
            ListIrp->CancelIrql = OldIrql;
            ListIrp->CancelRoutine = NULL;
            MountMgrCancel(DeviceObject, ListIrp);

            IoAcquireCancelSpinLock(&OldIrql);
        }

        NextEntry = NextEntry->Flink;
    }
    while (NextEntry != &(DeviceExtension->IrpListHead));

    IoReleaseCancelSpinLock(OldIrql);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MountMgrShutdown(IN PDEVICE_OBJECT DeviceObject,
                 IN PIRP Irp)
{
    PDEVICE_EXTENSION DeviceExtension;

    DeviceExtension = DeviceObject->DeviceExtension;

    InterlockedExchange(&Unloading, TRUE);

    KeInitializeEvent(&UnloadEvent, NotificationEvent, FALSE);

    /* Wait for workers */
    if (InterlockedIncrement(&(DeviceExtension->WorkerReferences)) > 0)
    {
        KeReleaseSemaphore(&(DeviceExtension->WorkerSemaphore),
                           IO_NO_INCREMENT,
                           1,
                           FALSE);
        KeWaitForSingleObject(&UnloadEvent, Executive, KernelMode, FALSE, NULL);
    }
    else
    {
        InterlockedDecrement(&(DeviceExtension->WorkerReferences));
    }

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/* FUNCTIONS ****************************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(IN PDRIVER_OBJECT DriverObject,
            IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_EXTENSION DeviceExtension;

    RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, DatabasePath);

    Status = IoCreateDevice(DriverObject,
                            sizeof(DEVICE_EXTENSION),
                            &DeviceMount,
                            FILE_DEVICE_NETWORK,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DriverObject->DriverUnload = MountMgrUnload;

    DeviceExtension = DeviceObject->DeviceExtension;
    RtlZeroMemory(DeviceExtension, sizeof(DEVICE_EXTENSION));
    DeviceExtension->DeviceObject = DeviceObject;
    DeviceExtension->DriverObject = DriverObject;

    InitializeListHead(&(DeviceExtension->DeviceListHead));
    InitializeListHead(&(DeviceExtension->OfflineDeviceListHead));

    KeInitializeSemaphore(&(DeviceExtension->DeviceLock), 1, 1);
    KeInitializeSemaphore(&(DeviceExtension->RemoteDatabaseLock), 1, 1);

    InitializeListHead(&(DeviceExtension->IrpListHead));
    DeviceExtension->EpicNumber = 1;

    InitializeListHead(&(DeviceExtension->SavedLinksListHead));

    InitializeListHead(&(DeviceExtension->WorkerQueueListHead));
    KeInitializeSemaphore(&(DeviceExtension->WorkerSemaphore), 0, MAXLONG);
    DeviceExtension->WorkerReferences = -1;
    KeInitializeSpinLock(&(DeviceExtension->WorkerLock));

    InitializeListHead(&(DeviceExtension->UniqueIdWorkerItemListHead));
    InitializeListHead(&(DeviceExtension->OnlineNotificationListHead));
    DeviceExtension->OnlineNotificationCount = 1;

    DeviceExtension->RegistryPath.Length = RegistryPath->Length;
    DeviceExtension->RegistryPath.MaximumLength = RegistryPath->Length + sizeof(WCHAR);
    DeviceExtension->RegistryPath.Buffer = AllocatePool(DeviceExtension->RegistryPath.MaximumLength);
    if (!DeviceExtension->RegistryPath.Buffer)
    {
        IoDeleteDevice(DeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyUnicodeString(&(DeviceExtension->RegistryPath), RegistryPath);

    DeviceExtension->NoAutoMount = MountmgrReadNoAutoMount(&(DeviceExtension->RegistryPath));

    GlobalCreateSymbolicLink(&DosDevicesMount, &DeviceMount);

    /* Register for device arrival & removal. Ask to be notified for already
     * present devices
     */
    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            &MountedDevicesGuid,
                                            DriverObject,
                                            MountMgrMountedDeviceNotification,
                                            DeviceExtension,
                                            &(DeviceExtension->NotificationEntry));

    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE]         =
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = MountMgrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MountMgrDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = MountMgrCleanup;
    DriverObject->MajorFunction[IRP_MJ_SHUTDOWN]       = MountMgrShutdown;

    gdeviceObject = DeviceObject;

    Status = IoRegisterShutdownNotification(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        IoDeleteDevice(DeviceObject);
    }

    return Status;
}
