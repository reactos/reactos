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
 * FILE:             drivers/filesystem/mountmgr/device.c
 * PURPOSE:          Mount Manager - Device Control
 * PROGRAMMER:       Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#include "mntmgr.h"

#define MAX_DEVICES 0x3E8 /* Matches 1000 devices */

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
NTSTATUS
MountMgrChangeNotify(IN PDEVICE_EXTENSION DeviceExtension,
                     IN PIRP Irp)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    PMOUNTMGR_CHANGE_NOTIFY_INFO ChangeNotify;

    /* Get the I/O buffer */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    ChangeNotify = (PMOUNTMGR_CHANGE_NOTIFY_INFO)Irp->AssociatedIrp.SystemBuffer;

    /* Validate it */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO) ||
        Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If epic number doesn't match, just return now one */
    if (DeviceExtension->EpicNumber != ChangeNotify->EpicNumber)
    {
        ChangeNotify->EpicNumber = DeviceExtension->EpicNumber;
        Irp->IoStatus.Information = sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO);
        return STATUS_SUCCESS;
    }

    /* If IRP is to be canceled, forget about that */
    IoAcquireCancelSpinLock(&OldIrql);
    if (Irp->Cancel)
    {
        Status = STATUS_CANCELLED;
    }
    /* Otherwise queue the IRP to be notified with the next epic number change */
    else
    {
        InsertTailList(&(DeviceExtension->IrpListHead), &(Irp->Tail.Overlay.ListEntry));
        IoMarkIrpPending(Irp);
        IoSetCancelRoutine(Irp, MountMgrCancel);
        Status = STATUS_PENDING;
    }
    IoReleaseCancelSpinLock(OldIrql);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountmgrWriteNoAutoMount(IN PDEVICE_EXTENSION DeviceExtension)
{
    ULONG Value = DeviceExtension->NoAutoMount;

    return RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                 DeviceExtension->RegistryPath.Buffer,
                                 L"NoAutoMount",
                                 REG_DWORD,
                                 &Value,
                                 sizeof(Value));

}

/*
 * @implemented
 */
NTSTATUS
MountMgrSetAutoMount(IN PDEVICE_EXTENSION DeviceExtension,
                     IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PMOUNTMGR_SET_AUTO_MOUNT SetState;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_SET_AUTO_MOUNT))
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    /* Only change if there's a real difference */
    SetState = (PMOUNTMGR_SET_AUTO_MOUNT)Irp->AssociatedIrp.SystemBuffer;
    if (SetState->NewState == !DeviceExtension->NoAutoMount)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    /* Set new state; ! on purpose */
    DeviceExtension->NoAutoMount = !SetState->NewState;
    Irp->IoStatus.Information = 0;
    return MountmgrWriteNoAutoMount(DeviceExtension);
}

/*
 * @implemented
 */
NTSTATUS
MountMgrQueryAutoMount(IN PDEVICE_EXTENSION DeviceExtension,
                       IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    PMOUNTMGR_QUERY_AUTO_MOUNT QueryState;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTMGR_QUERY_AUTO_MOUNT))
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INVALID_PARAMETER;
    }

    QueryState = (PMOUNTMGR_QUERY_AUTO_MOUNT)Irp->AssociatedIrp.SystemBuffer;
    QueryState->CurrentState = !DeviceExtension->NoAutoMount;
    Irp->IoStatus.Information = sizeof(MOUNTMGR_QUERY_AUTO_MOUNT);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
ScrubRegistryRoutine(IN PWSTR ValueName,
                     IN ULONG ValueType,
                     IN PVOID ValueData,
                     IN ULONG ValueLength,
                     IN PVOID Context,
                     IN PVOID EntryContext)
{
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    PDEVICE_INFORMATION DeviceInfo;
    PBOOLEAN Continue = EntryContext;
    PDEVICE_EXTENSION DeviceExtension = Context;

    if (ValueType != REG_BINARY)
    {
        return STATUS_SUCCESS;
    }

    /* Delete values for devices that don't have the matching unique ID */
    if (!IsListEmpty(&(DeviceExtension->DeviceListHead)))
    {
        for (NextEntry = DeviceExtension->DeviceListHead.Flink;
             NextEntry != &(DeviceExtension->DeviceListHead);
             NextEntry = NextEntry->Flink)
        {
            DeviceInfo = CONTAINING_RECORD(NextEntry,
                                           DEVICE_INFORMATION,
                                           DeviceListEntry);

             if (!DeviceInfo->UniqueId || DeviceInfo->UniqueId->UniqueIdLength != ValueLength)
             {
                 continue;
             }

             if (RtlCompareMemory(DeviceInfo->UniqueId->UniqueId, ValueData, ValueLength) == ValueLength)
             {
                 return STATUS_SUCCESS;
             }
        }
    }

    /* Wrong unique ID, scrub it */
    Status = RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                    DatabasePath,
                                    ValueName);
    if (!NT_SUCCESS(Status))
    {
        *Continue = TRUE;
        return STATUS_UNSUCCESSFUL;
    }

    *Continue = FALSE;
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrScrubRegistry(IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    BOOLEAN Continue;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    do
    {
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].QueryRoutine = ScrubRegistryRoutine;
        QueryTable[0].EntryContext = &Continue;
        Continue = FALSE;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        DatabasePath,
                                        QueryTable,
                                        DeviceExtension,
                                        NULL);
    }
    while (Continue);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrCreatePoint(IN PDEVICE_EXTENSION DeviceExtension,
                    IN PIRP Irp)
{
    ULONG MaxLength;
    PIO_STACK_LOCATION Stack;
    PMOUNTMGR_CREATE_POINT_INPUT Point;
    UNICODE_STRING DeviceName, SymbolicName;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_CREATE_POINT_INPUT))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Point = (PMOUNTMGR_CREATE_POINT_INPUT)Irp->AssociatedIrp.SystemBuffer;

    MaxLength = MAX((Point->DeviceNameOffset + Point->DeviceNameLength),
                    (Point->SymbolicLinkNameLength + Point->SymbolicLinkNameOffset));
    if (MaxLength > Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get all the strings and call the worker */
    SymbolicName.Length = Point->SymbolicLinkNameLength;
    SymbolicName.MaximumLength = Point->SymbolicLinkNameLength;
    DeviceName.Length = Point->DeviceNameLength;
    DeviceName.MaximumLength = Point->DeviceNameLength;
    SymbolicName.Buffer = (PVOID)((ULONG_PTR)Point + Point->SymbolicLinkNameOffset);
    DeviceName.Buffer = (PVOID)((ULONG_PTR)Point + Point->DeviceNameOffset);

    return MountMgrCreatePointWorker(DeviceExtension, &SymbolicName, &DeviceName);
}

/*
 * @implemented
 */
NTSTATUS
MountMgrCheckUnprocessedVolumes(IN PDEVICE_EXTENSION DeviceExtension,
                                IN PIRP Irp)
{
    PLIST_ENTRY NextEntry;
    PDEVICE_INFORMATION DeviceInformation;
    NTSTATUS ArrivalStatus, Status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Irp);

    /* No offline volumes, nothing more to do */
    if (IsListEmpty(&(DeviceExtension->OfflineDeviceListHead)))
    {
        KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

    /* Reactivate all the offline volumes */
    while (!IsListEmpty(&(DeviceExtension->OfflineDeviceListHead)))
    {
        NextEntry = RemoveHeadList(&(DeviceExtension->OfflineDeviceListHead));
        DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

        ArrivalStatus = MountMgrMountedDeviceArrival(DeviceExtension,
                                                     &(DeviceInformation->SymbolicName),
                                                     DeviceInformation->ManuallyRegistered);
        /* Then, remove them dead information */
        MountMgrFreeDeadDeviceInfo(DeviceInformation);

        if (NT_SUCCESS(Status))
        {
            Status = ArrivalStatus;
        }
    }

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
IsFtVolume(IN PUNICODE_STRING SymbolicName)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PARTITION_INFORMATION PartitionInfo;
    PDEVICE_OBJECT DeviceObject, FileDeviceObject;

    /* Get device object */
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Get attached device */
    FileDeviceObject = FileObject->DeviceObject;
    DeviceObject = IoGetAttachedDeviceReference(FileDeviceObject);

    /* FT volume can't be removable */
    if (FileDeviceObject->Characteristics & FILE_REMOVABLE_MEDIA)
    {
        ObDereferenceObject(DeviceObject);
        ObDereferenceObject(FileObject);
        return FALSE;
    }

    ObDereferenceObject(FileObject);

    /* Get partition information */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        &PartitionInfo,
                                        sizeof(PartitionInfo),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        ObDereferenceObject(DeviceObject);
        return FALSE;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    ObDereferenceObject(DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return FALSE;
    }

    /* Check if this is a FT volume */
    return IsFTPartition(PartitionInfo.PartitionType);
}

/*
 * @implemented
 */
VOID
ProcessSuggestedDriveLetters(IN PDEVICE_EXTENSION DeviceExtension)
{
    WCHAR NameBuffer[DRIVE_LETTER_LENGTH / sizeof(WCHAR)];
    PLIST_ENTRY NextEntry;
    UNICODE_STRING SymbolicName;
    PDEVICE_INFORMATION DeviceInformation;

    /* No devices? Nothing to do! */
    if (IsListEmpty(&(DeviceExtension->DeviceListHead)))
    {
        return;
    }

    /* For all the devices */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &(DeviceExtension->DeviceListHead);
         NextEntry = NextEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

        /* If no drive letter */
        if (DeviceInformation->SuggestedDriveLetter == (UCHAR)-1)
        {
            /* Ensure it has no entry yet */
            if (!HasDriveLetter(DeviceInformation) &&
                !HasNoDriveLetterEntry(DeviceInformation->UniqueId))
            {
                /* And create one */
                CreateNoDriveLetterEntry(DeviceInformation->UniqueId);
            }

            DeviceInformation->SuggestedDriveLetter = 0;
        }
        /* Suggested letter & no entry */
        else if (DeviceInformation->SuggestedDriveLetter &&
                 !HasNoDriveLetterEntry(DeviceInformation->UniqueId))
        {
            /* Just create a mount point */
            SymbolicName.Buffer = NameBuffer;
            RtlCopyMemory(NameBuffer, DosDevices.Buffer, DosDevices.Length);
            NameBuffer[LETTER_POSITION] = DeviceInformation->SuggestedDriveLetter;
            NameBuffer[COLON_POSITION] = L':';
            SymbolicName.Length =
            SymbolicName.MaximumLength = DRIVE_LETTER_LENGTH;

            MountMgrCreatePointWorker(DeviceExtension, &SymbolicName, &(DeviceInformation->DeviceName));
        }
    }
}

/*
 * @implemented
 */
NTSTATUS
MountMgrNextDriveLetterWorker(IN PDEVICE_EXTENSION DeviceExtension,
                              IN PUNICODE_STRING DeviceName,
                              OUT PMOUNTMGR_DRIVE_LETTER_INFORMATION DriveLetterInfo)
{
    NTSTATUS Status;
    UCHAR DriveLetter;
    PLIST_ENTRY NextEntry;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    BOOLEAN Removable, GptDriveLetter;
    PDEVICE_INFORMATION DeviceInformation;
    WCHAR NameBuffer[DRIVE_LETTER_LENGTH];
    PSYMLINK_INFORMATION SymlinkInformation;
    UNICODE_STRING TargetDeviceName, SymbolicName;

    /* First, process suggested letters */
    if (!DeviceExtension->ProcessedSuggestions)
    {
        ProcessSuggestedDriveLetters(DeviceExtension);
        DeviceExtension->ProcessedSuggestions = TRUE;
    }

    /* Then, get information about the device */
    Status = QueryDeviceInformation(DeviceName, &TargetDeviceName, NULL, &Removable, &GptDriveLetter, NULL, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Ensure we have such device */
    NextEntry = DeviceExtension->DeviceListHead.Flink;
    while (NextEntry != &(DeviceExtension->DeviceListHead))
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

        if (RtlCompareUnicodeString(&(DeviceInformation->DeviceName), &TargetDeviceName, TRUE) == 0)
        {
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    if (NextEntry == &(DeviceExtension->DeviceListHead))
    {
        FreePool(TargetDeviceName.Buffer);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    /* Now, mark we have assigned a letter (assumption) */
    DeviceInformation->LetterAssigned =
    DriveLetterInfo->DriveLetterWasAssigned = TRUE;

    /* Browse all the symlinks to check if there is already a drive letter */
    NextEntry = DeviceInformation->SymbolicLinksListHead.Flink;
    while (NextEntry != &(DeviceInformation->SymbolicLinksListHead))
    {
        SymlinkInformation = CONTAINING_RECORD(NextEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

        /* If this is a drive letter and it is online, forget about new drive letter */
        if (IsDriveLetter(&(SymlinkInformation->Name)) && SymlinkInformation->Online)
        {
            DriveLetterInfo->DriveLetterWasAssigned = FALSE;
            DriveLetterInfo->CurrentDriveLetter = (CHAR)SymlinkInformation->Name.Buffer[LETTER_POSITION];
            break;
        }

        NextEntry = NextEntry->Flink;
    }

    /* If we didn't find a drive letter online
     * ensure this is not a no drive entry
    * by querying GPT attributes & database
     */
    if (NextEntry == &(DeviceInformation->SymbolicLinksListHead))
    {
        if (!GptDriveLetter || HasNoDriveLetterEntry(DeviceInformation->UniqueId))
        {
            DriveLetterInfo->DriveLetterWasAssigned = FALSE;
            DriveLetterInfo->CurrentDriveLetter = 0;

            goto Release;
        }
    }

    /* No, ensure that the device is not automounted nor removable */
    if (!DeviceExtension->NoAutoMount && !Removable)
    {
        if (DriveLetterInfo->DriveLetterWasAssigned)
        {
            DriveLetterInfo->DriveLetterWasAssigned = FALSE;
            DriveLetterInfo->CurrentDriveLetter = 0;

            goto Release;
        }
    }

    if (!DriveLetterInfo->DriveLetterWasAssigned)
    {
        goto Release;
    }

    /* Now everything is fine, start processing */

    if (RtlPrefixUnicodeString(&DeviceFloppy, &TargetDeviceName, TRUE))
    {
        /* If the device is a floppy, start with letter A */
        DriveLetter = 'A';
    }
    else if (RtlPrefixUnicodeString(&DeviceCdRom, &TargetDeviceName, TRUE))
    {
        /* If the device is a CD-ROM, start with letter D */
        DriveLetter = 'D';
    }
    else
    {
        /* Finally, if it's a disk, use C */
        DriveLetter = 'C';
    }

    /* We cannot set NO drive letter */
    ASSERT(DeviceInformation->SuggestedDriveLetter != (UCHAR)-1);

    /* If we don't have suggested letter but it's a FT volume, fail */
    if (!DeviceInformation->SuggestedDriveLetter && IsFtVolume(&(DeviceInformation->DeviceName)))
    {
        DriveLetterInfo->DriveLetterWasAssigned = FALSE;
        DriveLetterInfo->CurrentDriveLetter = 0;

        goto Release;
    }

    /* Prepare buffer */
    RtlCopyMemory(NameBuffer, DosDevices.Buffer, DosDevices.Length);
    NameBuffer[COLON_POSITION] = L':';
    SymbolicName.Buffer = NameBuffer;
    SymbolicName.Length =
    SymbolicName.MaximumLength = DRIVE_LETTER_LENGTH;

    /* It's all prepared, create mount point */
    if (DeviceInformation->SuggestedDriveLetter)
    {
        DriveLetterInfo->CurrentDriveLetter = DeviceInformation->SuggestedDriveLetter;
        NameBuffer[LETTER_POSITION] = DeviceInformation->SuggestedDriveLetter;

        Status = MountMgrCreatePointWorker(DeviceExtension, &SymbolicName, &TargetDeviceName);
        if (NT_SUCCESS(Status))
        {
            goto Release;
        }
    }

    /* It failed with this letter... Try another one! */
    for (DriveLetterInfo->CurrentDriveLetter = DriveLetter;
         DriveLetterInfo->CurrentDriveLetter <= L'Z';
         DriveLetterInfo->CurrentDriveLetter++)
    {
        NameBuffer[LETTER_POSITION] = DriveLetterInfo->CurrentDriveLetter;

        Status = MountMgrCreatePointWorker(DeviceExtension, &SymbolicName, &TargetDeviceName);
        if (NT_SUCCESS(Status))
        {
            break;
        }
    }

    /* We failed setting a letter */
    if (DriveLetterInfo->CurrentDriveLetter > L'Z')
    {
        DriveLetterInfo->DriveLetterWasAssigned = FALSE;
        DriveLetterInfo->CurrentDriveLetter = 0;

        /* Try at least to add a no drive letter entry */
        Status = QueryDeviceInformation(&TargetDeviceName, NULL, &UniqueId, NULL, NULL, NULL, NULL, NULL);
        if (NT_SUCCESS(Status))
        {
            CreateNoDriveLetterEntry(UniqueId);
            FreePool(UniqueId);
        }
    }

Release:
    FreePool(TargetDeviceName.Buffer);

    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
NTSTATUS
MountMgrNextDriveLetter(IN PDEVICE_EXTENSION DeviceExtension,
                        IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    UNICODE_STRING DeviceName;
    PMOUNTMGR_DRIVE_LETTER_TARGET DriveLetterTarget;
    MOUNTMGR_DRIVE_LETTER_INFORMATION DriveLetterInformation;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) ||
        Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION))
    {
        return STATUS_INVALID_PARAMETER;
    }

    DriveLetterTarget = (PMOUNTMGR_DRIVE_LETTER_TARGET)Irp->AssociatedIrp.SystemBuffer;
    if (DriveLetterTarget->DeviceNameLength + sizeof(USHORT) > Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Call the worker */
    DeviceName.Buffer = DriveLetterTarget->DeviceName;
    DeviceName.Length =
    DeviceName.MaximumLength = DriveLetterTarget->DeviceNameLength;

    Status = MountMgrNextDriveLetterWorker(DeviceExtension, &DeviceName,
                                           &DriveLetterInformation);
    if (NT_SUCCESS(Status))
    {
        *(PMOUNTMGR_DRIVE_LETTER_INFORMATION)Irp->AssociatedIrp.SystemBuffer =
            DriveLetterInformation;
        Irp->IoStatus.Information = sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MountMgrQuerySystemVolumeNameQueryRoutine(IN PWSTR ValueName,
                                          IN ULONG ValueType,
                                          IN PVOID ValueData,
                                          IN ULONG ValueLength,
                                          IN PVOID Context,
                                          IN PVOID EntryContext)
{
    UNICODE_STRING ValueString;
    PUNICODE_STRING SystemVolumeName;

    UNREFERENCED_PARAMETER(ValueName);
    UNREFERENCED_PARAMETER(ValueLength);
    UNREFERENCED_PARAMETER(EntryContext);

    if (ValueType != REG_SZ)
    {
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&ValueString, ValueData);
    SystemVolumeName = Context;

    /* Return a string containing system volume name */
    SystemVolumeName->Length = ValueString.Length;
    SystemVolumeName->MaximumLength = ValueString.Length + sizeof(WCHAR);
    SystemVolumeName->Buffer = AllocatePool(SystemVolumeName->MaximumLength);
    if (SystemVolumeName->Buffer)
    {
        RtlCopyMemory(SystemVolumeName->Buffer, ValueData, ValueString.Length);
        SystemVolumeName->Buffer[ValueString.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    return STATUS_SUCCESS;

}

/*
 * @implemented
 */
NTSTATUS
MountMgrQuerySystemVolumeName(OUT PUNICODE_STRING SystemVolumeName)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = MountMgrQuerySystemVolumeNameQueryRoutine;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"SystemPartition";

    SystemVolumeName->Buffer = NULL;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           L"\\Registry\\Machine\\System\\Setup",
                           QueryTable,
                           SystemVolumeName,
                           NULL);

    if (SystemVolumeName->Buffer)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
VOID
MountMgrAssignDriveLetters(IN PDEVICE_EXTENSION DeviceExtension)
{
    NTSTATUS Status;
    PLIST_ENTRY NextEntry;
    UNICODE_STRING SystemVolumeName;
    PDEVICE_INFORMATION DeviceInformation;
    MOUNTMGR_DRIVE_LETTER_INFORMATION DriveLetterInformation;

    /* First, get system volume name */
    Status = MountMgrQuerySystemVolumeName(&SystemVolumeName);

    /* If there are no device, it's all done */
    if (IsListEmpty(&(DeviceExtension->DeviceListHead)))
    {
        if (NT_SUCCESS(Status))
        {
            FreePool(SystemVolumeName.Buffer);
        }

        return;
    }

    /* Now, for all the devices... */
    for (NextEntry = DeviceExtension->DeviceListHead.Flink;
         NextEntry != &(DeviceExtension->DeviceListHead);
         NextEntry = NextEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(NextEntry, DEVICE_INFORMATION, DeviceListEntry);

        /* If the device doesn't have a letter assigned, do it! */
        if (!DeviceInformation->LetterAssigned)
        {
            MountMgrNextDriveLetterWorker(DeviceExtension,
                                          &(DeviceInformation->DeviceName),
                                          &DriveLetterInformation);
        }

        /* If it was the system volume */
        if (NT_SUCCESS(Status) && RtlEqualUnicodeString(&SystemVolumeName, &(DeviceInformation->DeviceName), TRUE))
        {
            /* Keep track of it */
            DeviceExtension->DriveLetterData = AllocatePool(DeviceInformation->UniqueId->UniqueIdLength +
                                                            sizeof(MOUNTDEV_UNIQUE_ID));
            if (DeviceExtension->DriveLetterData)
            {
                RtlCopyMemory(DeviceExtension->DriveLetterData,
                              DeviceInformation->UniqueId,
                              DeviceInformation->UniqueId->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
            }

            /* If it was not automount, ensure it gets mounted */
            if (!DeviceExtension->NoAutoMount)
            {
                DeviceExtension->NoAutoMount = TRUE;

                MountMgrNextDriveLetterWorker(DeviceExtension,
                                              &(DeviceInformation->DeviceName),
                                              &DriveLetterInformation);

                DeviceExtension->NoAutoMount = FALSE;
            }
        }
    }

    if (NT_SUCCESS(Status))
    {
        FreePool(SystemVolumeName.Buffer);
    }
}

/*
 * @implemented
 */
NTSTATUS
MountMgrQueryDosVolumePath(IN PDEVICE_EXTENSION DeviceExtension,
                           IN PIRP Irp)
{
    NTSTATUS Status;
    ULONG DevicesFound;
    PIO_STACK_LOCATION Stack;
    PLIST_ENTRY SymlinksEntry;
    UNICODE_STRING SymbolicName;
    PMOUNTMGR_TARGET_NAME Target;
    PWSTR DeviceString, OldBuffer;
    USHORT DeviceLength, OldLength;
    PDEVICE_INFORMATION DeviceInformation;
    PSYMLINK_INFORMATION SymlinkInformation;
    PASSOCIATED_DEVICE_ENTRY AssociatedDevice;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input size */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_TARGET_NAME))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Ensure we have received UNICODE_STRING */
    Target = (PMOUNTMGR_TARGET_NAME)Irp->AssociatedIrp.SystemBuffer;
    if (Target->DeviceNameLength & 1)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate the entry structure size */
    if (Target->DeviceNameLength + sizeof(UNICODE_NULL) > Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Ensure we can at least return needed size */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Construct string for query */
    SymbolicName.Length = Target->DeviceNameLength;
    SymbolicName.MaximumLength = Target->DeviceNameLength + sizeof(UNICODE_NULL);
    SymbolicName.Buffer = Target->DeviceName;

    /* Find device with our info */
    Status = FindDeviceInfo(DeviceExtension, &SymbolicName, FALSE, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DeviceLength = 0;
    DeviceString = NULL;
    DevicesFound = 0;

    /* Try to find associated device info */
    while (TRUE)
    {
        for (SymlinksEntry = DeviceInformation->SymbolicLinksListHead.Flink;
             SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead);
             SymlinksEntry = SymlinksEntry->Flink)
        {
            SymlinkInformation = CONTAINING_RECORD(SymlinksEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

            /* Try to find with drive letter */
            if (MOUNTMGR_IS_DRIVE_LETTER(&SymlinkInformation->Name) && SymlinkInformation->Online)
            {
                break;
            }
        }

        /* We didn't find, break */
        if (SymlinksEntry == &(DeviceInformation->SymbolicLinksListHead))
        {
            break;
        }

        /* It doesn't have associated device, go to fallback method */
        if (IsListEmpty(&DeviceInformation->AssociatedDevicesHead))
        {
            goto TryWithVolumeName;
        }

        /* Create a string with the information about the device */
        AssociatedDevice = CONTAINING_RECORD(&(DeviceInformation->SymbolicLinksListHead), ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);
        OldLength = DeviceLength;
        OldBuffer = DeviceString;
        DeviceLength += AssociatedDevice->String.Length;
        DeviceString = AllocatePool(DeviceLength);
        if (!DeviceString)
        {
            if (OldBuffer)
            {
                FreePool(OldBuffer);
            }

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Store our info and previous if any */
        RtlCopyMemory(DeviceString, AssociatedDevice->String.Buffer, AssociatedDevice->String.Length);
        if (OldBuffer)
        {
            RtlCopyMemory(&DeviceString[AssociatedDevice->String.Length / sizeof(WCHAR)], OldBuffer, OldLength);
            FreePool(OldBuffer);
        }

        /* Count and continue looking */
        ++DevicesFound;
        DeviceInformation = AssociatedDevice->DeviceInformation;

        /* If too many devices, try another way */
        if (DevicesFound > MAX_DEVICES) /*  1000 */
        {
            goto TryWithVolumeName;
        }
    }

    /* Reallocate our string, so that we can prepend disk letter */
    OldBuffer = DeviceString;
    OldLength = DeviceLength;
    DeviceLength += 2 * sizeof(WCHAR);
    DeviceString = AllocatePool(DeviceLength);
    if (!DeviceString)
    {
        if (OldBuffer)
        {
            FreePool(OldBuffer);
        }

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get the letter */
    DeviceString[0] = SymlinkInformation->Name.Buffer[LETTER_POSITION];
    DeviceString[1] = L':';

    /* And copy the rest */
    if (OldBuffer)
    {
        RtlCopyMemory(&DeviceString[2], OldBuffer, OldLength);
        FreePool(OldBuffer);
    }

TryWithVolumeName:
    /* If we didn't find anything, try differently */
    if (DeviceLength < 2 * sizeof(WCHAR) || DeviceString[1] != L':')
    {
        if (DeviceString)
        {
            FreePool(DeviceString);
            DeviceLength = 0;
        }

        /* Try to find a volume name matching */
        for (SymlinksEntry = DeviceInformation->SymbolicLinksListHead.Flink;
             SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead);
             SymlinksEntry = SymlinksEntry->Flink)
        {
            SymlinkInformation = CONTAINING_RECORD(SymlinksEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

            if (MOUNTMGR_IS_VOLUME_NAME(&SymlinkInformation->Name))
            {
                break;
            }
        }

        /* If found copy */
        if (SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead))
        {
            DeviceLength = SymlinkInformation->Name.Length;
            DeviceString = AllocatePool(DeviceLength);
            if (!DeviceString)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            RtlCopyMemory(DeviceString, SymlinkInformation->Name.Buffer, DeviceLength);
            /* Ensure we are in the right namespace; [1] can be ? */
            DeviceString[1] = L'\\';
        }
    }

    /* If we found something */
    if (DeviceString)
    {
        /* At least, we will return our length */
        ((PMOUNTMGR_VOLUME_PATHS)Irp->AssociatedIrp.SystemBuffer)->MultiSzLength = DeviceLength;
        /* MOUNTMGR_VOLUME_PATHS is a string + a ULONG */
        Irp->IoStatus.Information = DeviceLength + sizeof(ULONG);

        /* If we have enough room for copying the string */
        if (sizeof(ULONG) + DeviceLength <= Stack->Parameters.DeviceIoControl.OutputBufferLength)
        {
            /* Copy it */
            if (DeviceLength)
            {
                RtlCopyMemory(((PMOUNTMGR_VOLUME_PATHS)Irp->AssociatedIrp.SystemBuffer)->MultiSz, DeviceString, DeviceLength);
            }

            /* And double zero at its end - this is needed in case of multiple paths which are separated by a single 0 */
            FreePool(DeviceString);
            ((PMOUNTMGR_VOLUME_PATHS)Irp->AssociatedIrp.SystemBuffer)->MultiSz[DeviceLength / sizeof(WCHAR)] = 0;
            ((PMOUNTMGR_VOLUME_PATHS)Irp->AssociatedIrp.SystemBuffer)->MultiSz[DeviceLength / sizeof(WCHAR) + 1] = 0;

            return STATUS_SUCCESS;
        }
        else
        {
            /* Just return appropriate size and leave */
            FreePool(DeviceString);
            Irp->IoStatus.Information = sizeof(ULONG);
            return STATUS_BUFFER_OVERFLOW;
        }
    }

    /* Fail */
    return STATUS_NOT_FOUND;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrValidateBackPointer(IN PASSOCIATED_DEVICE_ENTRY AssociatedDeviceEntry,
                            IN PDEVICE_INFORMATION DeviceInformation,
                            OUT PBOOLEAN Invalid)
{
    HANDLE Handle;
    NTSTATUS Status;
    PLIST_ENTRY SymlinksEntry;
    IO_STATUS_BLOCK IoStatusBlock;
    PREPARSE_DATA_BUFFER ReparseData;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING FullName, SubstituteName;
    PSYMLINK_INFORMATION SymlinkInformation;

    /* Initialize & allocate a string big enough to contain our complete mount point name */
    FullName.Length = 0;
    FullName.MaximumLength = AssociatedDeviceEntry->String.Length
                             + AssociatedDeviceEntry->DeviceInformation->DeviceName.Length
                             + sizeof(WCHAR)
                             + sizeof(UNICODE_NULL);
    FullName.Buffer = AllocatePool(FullName.MaximumLength);
    if (!FullName.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Create the path */
    RtlAppendUnicodeStringToString(&FullName, &AssociatedDeviceEntry->DeviceInformation->DeviceName);
    FullName.Buffer[FullName.Length / sizeof(WCHAR)] = L'\\';
    RtlAppendUnicodeStringToString(&FullName, &AssociatedDeviceEntry->String);
    FullName.Buffer[FullName.Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Open it to query the reparse point */
    InitializeObjectAttributes(&ObjectAttributes,
                               &FullName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&Handle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes, &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT);
    FreePool(FullName.Buffer);

    if (!NT_SUCCESS(Status))
    {
        *Invalid = TRUE;
        return STATUS_SUCCESS;
    }

    /* Allocate a buffer big enough to read reparse data */
    ReparseData = AllocatePool(MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (ReparseData == NULL)
    {
        ZwClose(Handle);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Query reparse data */
    Status = ZwFsControlFile(Handle,
                             NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_GET_REPARSE_POINT,
                             NULL, 0,
                             ReparseData, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    ZwClose(Handle);

    if (!NT_SUCCESS(Status))
    {
        FreePool(ReparseData);
        *Invalid = TRUE;
        return STATUS_SUCCESS;
    }

    /* Create a string with the substitute name */
    SubstituteName.Length = ReparseData->SymbolicLinkReparseBuffer.SubstituteNameLength;
    SubstituteName.MaximumLength = SubstituteName.Length;
    SubstituteName.Buffer = (PWSTR)((ULONG_PTR)ReparseData->SymbolicLinkReparseBuffer.PathBuffer + ReparseData->SymbolicLinkReparseBuffer.SubstituteNameOffset);

    /* If that's a volume name that matches our associated device, that's a success! */
    if (MOUNTMGR_IS_VOLUME_NAME(&SubstituteName))
    {
        if (SubstituteName.Length == 98 && SubstituteName.Buffer[1] == L'?')
        {
            for (SymlinksEntry = DeviceInformation->SymbolicLinksListHead.Flink;
                 SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead);
                 SymlinksEntry = SymlinksEntry->Flink)
            {
                SymlinkInformation = CONTAINING_RECORD(SymlinksEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

                if (RtlEqualUnicodeString(&SubstituteName, &SymlinkInformation->Name, TRUE))
                {
                    FreePool(ReparseData);
                    return STATUS_SUCCESS;
                }
            }
        }
    }

    FreePool(ReparseData);
    *Invalid = TRUE;
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrQueryVolumePaths(IN PDEVICE_EXTENSION DeviceExtension,
                         IN PDEVICE_INFORMATION DeviceInformation,
                         IN PLIST_ENTRY DeviceInfoList,
                         OUT PMOUNTMGR_VOLUME_PATHS * VolumePaths,
                         OUT PDEVICE_INFORMATION *FailedDevice)
{
    ULONG Written;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PSYMLINK_INFORMATION SymlinkInformation;
    PDEVICE_INFORMATION_ENTRY DeviceInfoEntry;
    PASSOCIATED_DEVICE_ENTRY AssociatedDeviceEntry;
    PMOUNTMGR_VOLUME_PATHS * Paths = NULL, * CurrentPath;
    ULONG OutputPathLength, NumberOfPaths, ReturnedPaths;

    /* We return at least null char */
    OutputPathLength = sizeof(UNICODE_NULL);

    for (Entry = DeviceInformation->SymbolicLinksListHead.Flink;
         Entry != &(DeviceInformation->SymbolicLinksListHead);
         Entry = Entry->Flink)
    {
        SymlinkInformation = CONTAINING_RECORD(Entry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

        /* Try to find the drive letter (ie, DOS device) */
        if (MOUNTMGR_IS_DRIVE_LETTER(&SymlinkInformation->Name) && SymlinkInformation->Online)
        {
            /* We'll return the letter */
            OutputPathLength = 4 * sizeof(WCHAR);
            break;
        }
    }

    /* We didn't find any */
    if (Entry == &(DeviceInformation->SymbolicLinksListHead))
    {
        SymlinkInformation = NULL;
    }

    /* Do we have any device info to return? */
    for (Entry = DeviceInfoList->Flink; Entry != DeviceInfoList; Entry = Entry->Flink)
    {
        DeviceInfoEntry = CONTAINING_RECORD(Entry, DEVICE_INFORMATION_ENTRY, DeviceInformationEntry);

        /* Matching current device */
        if (DeviceInfoEntry->DeviceInformation == DeviceInformation)
        {
            /* Allocate the output buffer */
            *VolumePaths = AllocatePool(sizeof(ULONG) + OutputPathLength);
            if (*VolumePaths == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Set size */
            (*VolumePaths)->MultiSzLength = OutputPathLength;
            /* If we have a drive letter, return it */
            if (SymlinkInformation != NULL)
            {
                (*VolumePaths)->MultiSz[0] = SymlinkInformation->Name.Buffer[LETTER_POSITION];
                (*VolumePaths)->MultiSz[1] = L':';
                (*VolumePaths)->MultiSz[2] = UNICODE_NULL;
                (*VolumePaths)->MultiSz[3] = UNICODE_NULL;
            }
            else
            {
                (*VolumePaths)->MultiSz[0] = UNICODE_NULL;
            }

            return STATUS_SUCCESS;
        }
    }

    /* Allocate a new device entry */
    DeviceInfoEntry = AllocatePool(sizeof(DEVICE_INFORMATION_ENTRY));
    if (DeviceInfoEntry == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Add it to the list */
    DeviceInfoEntry->DeviceInformation = DeviceInformation;
    InsertTailList(DeviceInfoList, &DeviceInfoEntry->DeviceInformationEntry);

    NumberOfPaths = 0;
    /* Count the amount of devices we will have to handle */
    if (!IsListEmpty(&DeviceInformation->AssociatedDevicesHead))
    {
        for (Entry = DeviceInformation->AssociatedDevicesHead.Flink;
                     Entry != &DeviceInformation->AssociatedDevicesHead;
                     Entry = Entry->Flink)
        {
            ++NumberOfPaths;
        }

        ASSERT(NumberOfPaths != 0);
        /* And allocate a big enough buffer */
        Paths = AllocatePool(NumberOfPaths * sizeof(PMOUNTMGR_VOLUME_PATHS));
        if (Paths == NULL)
        {
            RemoveEntryList(&DeviceInfoEntry->DeviceInformationEntry);
            FreePool(DeviceInfoEntry);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    /* Start the hot loop to gather all the paths and be able to compute total output length! */
    ReturnedPaths = 0;
    CurrentPath = Paths;
    for (Entry = DeviceInformation->AssociatedDevicesHead.Flink;
         Entry != &DeviceInformation->AssociatedDevicesHead;
         Entry = Entry->Flink)
    {
        USHORT InnerStrings;
        BOOLEAN Invalid = FALSE;

        AssociatedDeviceEntry = CONTAINING_RECORD(Entry, ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);

        /* Validate the fact its a mount point by query reparse data */
        Status = MountMgrValidateBackPointer(AssociatedDeviceEntry, DeviceInformation, &Invalid);

        /* If we found an invalid device, that's a failure */
        if (Invalid)
        {
            *FailedDevice = AssociatedDeviceEntry->DeviceInformation;
            Status = STATUS_UNSUCCESSFUL;
        }

        /* Check whether we failed, if so, bail out */
        if (!NT_SUCCESS(Status))
        {
            ULONG i;

            for (i = 0; i < ReturnedPaths; ++i)
            {
                FreePool(Paths[i]);
            }

            if (Paths != NULL)
            {
                FreePool(Paths);
            }
            RemoveEntryList(&DeviceInfoEntry->DeviceInformationEntry);
            FreePool(DeviceInfoEntry);
            return Status;
        }

        /* Query associated paths (hello ourselves :-)) */
        Status = MountMgrQueryVolumePaths(DeviceExtension,
                                          AssociatedDeviceEntry->DeviceInformation,
                                          DeviceInfoList,
                                          CurrentPath,
                                          FailedDevice);
        if (!NT_SUCCESS(Status))
        {
            ULONG i;

            for (i = 0; i < ReturnedPaths; ++i)
            {
                FreePool(Paths[i]);
            }

            if (Paths != NULL)
            {
                FreePool(Paths);
            }
            RemoveEntryList(&DeviceInfoEntry->DeviceInformationEntry);
            FreePool(DeviceInfoEntry);
            return Status;
        }

        /* Count the number of strings we have in the multi string buffer */
        InnerStrings = 0;
        if ((*CurrentPath)->MultiSzLength != sizeof(UNICODE_NULL))
        {
            ULONG i;
            PWSTR MultiSz = (*CurrentPath)->MultiSz;

            for (i = 0; i < (*CurrentPath)->MultiSzLength / sizeof(WCHAR); ++i, ++MultiSz)
            {
                if (*MultiSz == UNICODE_NULL)
                {
                    ++InnerStrings;
                }
            }
        }

        /* We returned one more path (ie, one more allocated buffer) */
        ++ReturnedPaths;
        /* Move the next pointer to use in the array */
        ++CurrentPath;
        /* Multiply String.Length by the number of found paths, we always add it after a path */
        OutputPathLength += (*CurrentPath)->MultiSzLength + InnerStrings * AssociatedDeviceEntry->String.Length - sizeof(UNICODE_NULL);
    }

    /* Allocate the output buffer */
    *VolumePaths = AllocatePool(sizeof(ULONG) + OutputPathLength);
    if (*VolumePaths == NULL)
    {
        ULONG i;

        for (i = 0; i < ReturnedPaths; ++i)
        {
            FreePool(Paths[i]);
        }

        if (Paths != NULL)
        {
            FreePool(Paths);
        }
        RemoveEntryList(&DeviceInfoEntry->DeviceInformationEntry);
        FreePool(DeviceInfoEntry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Written = 0;
    /* If we had found a DOS letter, that's the first thing we return */
    (*VolumePaths)->MultiSzLength = OutputPathLength;
    if (SymlinkInformation != NULL)
    {
        (*VolumePaths)->MultiSz[0] = SymlinkInformation->Name.Buffer[LETTER_POSITION];
        (*VolumePaths)->MultiSz[1] = L':';
        (*VolumePaths)->MultiSz[2] = UNICODE_NULL;
        Written = 3;
    }

    /* Now, browse again all our paths to return them */
    CurrentPath = Paths;
    for (Entry = DeviceInformation->AssociatedDevicesHead.Flink;
         Entry != &DeviceInformation->AssociatedDevicesHead;
         Entry = Entry->Flink)
    {
        AssociatedDeviceEntry = CONTAINING_RECORD(Entry, ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);

        /* If we had a path... */
        if ((*CurrentPath)->MultiSzLength != sizeof(UNICODE_NULL))
        {
            ULONG i, Offset;
            PWSTR MultiSz;

            /* This offset is used to "jump" into MultiSz, so, start with the string begin (ie, skip MultiSzLength) */
            Offset = sizeof(ULONG);
            /* Browse every single letter, and skip last UNICODE_NULL */
            for (i = 0; i < (*CurrentPath)->MultiSzLength / sizeof(WCHAR) - 1; ++i)
            {
                /* Get the letter */
                MultiSz = (PWSTR)((ULONG_PTR)(*CurrentPath) + Offset);
                /* If it was part of the path, just return it */
                if (*MultiSz != UNICODE_NULL)
                {
                    (*VolumePaths)->MultiSz[Written] = *MultiSz;
                }
                else
                {
                    /* Otherwise, as planed, return our whole associated device name */
                    RtlCopyMemory(&(*VolumePaths)->MultiSz[Written],
                                  AssociatedDeviceEntry->String.Buffer,
                                  AssociatedDeviceEntry->String.Length);
                    Written += AssociatedDeviceEntry->String.Length / sizeof(WCHAR);
                    /* And don't forget to nullify */
                    (*VolumePaths)->MultiSz[Written] = UNICODE_NULL;
                }

                /* We at least return a letter or a null char */
                ++Written;
                /* Move to the next letter */
                Offset += sizeof(WCHAR);
            }
        }

        FreePool(*CurrentPath);
        ++CurrentPath;
    }

    /* MultiSz: don't forget last null char */
    (*VolumePaths)->MultiSz[Written] = UNICODE_NULL;
    /* Cleanup everything and return success! */
    if (Paths != NULL)
    {
        FreePool(Paths);
    }
    RemoveEntryList(&DeviceInfoEntry->DeviceInformationEntry);
    FreePool(DeviceInfoEntry);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrQueryDosVolumePaths(IN PDEVICE_EXTENSION DeviceExtension,
                            IN PIRP Irp)
{
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    LIST_ENTRY Devices;
    BOOLEAN NeedNotification;
    PIO_STACK_LOCATION Stack;
    UNICODE_STRING SymbolicName;
    ULONG Attempts, OutputLength;
    PMOUNTMGR_TARGET_NAME Target;
    PMOUNTMGR_VOLUME_PATHS Paths, Output;
    RECONCILE_WORK_ITEM_CONTEXT ReconcileContext;
    PDEVICE_INFORMATION DeviceInformation, ListDeviceInfo, FailedDevice;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input size */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_TARGET_NAME))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Ensure we have received UNICODE_STRING */
    Target = (PMOUNTMGR_TARGET_NAME)Irp->AssociatedIrp.SystemBuffer;
    if (Target->DeviceNameLength & 1)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate the entry structure size */
    if (Target->DeviceNameLength + FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) > Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Ensure we can at least return needed size */
    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(ULONG))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Construct string for query */
    SymbolicName.Length = Target->DeviceNameLength;
    SymbolicName.MaximumLength = Target->DeviceNameLength + sizeof(UNICODE_NULL);
    SymbolicName.Buffer = Target->DeviceName;

    /* Find device with our info */
    Status = FindDeviceInfo(DeviceExtension, &SymbolicName, FALSE, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    NeedNotification = FALSE;
    Attempts = 0;
    for (;;)
    {
        FailedDevice = NULL;
        InitializeListHead(&Devices);

        /* Query paths */
        Status = MountMgrQueryVolumePaths(DeviceExtension, DeviceInformation, &Devices, &Paths, &FailedDevice);
        if (NT_SUCCESS(Status))
        {
            break;
        }

        /* If it failed for generic reason (memory, whatever), bail out (ie, FailedDevice not set) */
        if (FailedDevice == NULL)
        {
            return Status;
        }

        /* If PnP, let's notify in case of success */
        if (!DeviceInformation->ManuallyRegistered)
        {
            NeedNotification = TRUE;
        }

        /* Reconcile database */
        ReconcileContext.DeviceExtension = DeviceExtension;
        ReconcileContext.DeviceInformation = FailedDevice;
        KeReleaseSemaphore(&DeviceExtension->DeviceLock, IO_NO_INCREMENT, 1, FALSE);
        ReconcileThisDatabaseWithMasterWorker(&ReconcileContext);
        KeWaitForSingleObject(&DeviceExtension->DeviceLock, Executive, KernelMode, FALSE, NULL);

        /* Look for our device, to check it's online */
        for (Entry = DeviceExtension->DeviceListHead.Flink;
             Entry != &DeviceExtension->DeviceListHead;
             Entry = Entry->Flink)
        {
            ListDeviceInfo = CONTAINING_RECORD(Entry, DEVICE_INFORMATION, DeviceListEntry);
            /* It's online, it's OK! */
            if (ListDeviceInfo == DeviceInformation)
            {
                break;
            }
        }

        /* It's not online, it's not good */
        if (Entry == &DeviceExtension->DeviceListHead)
        {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }

        /* Increase attempts count */
        ++Attempts;
        /* Don't look forever and fail if we get out of attempts */
        if (Attempts >= 1000)
        {
            return Status;
        }
    }

    /* We need to notify? Go ahead */
    if (NeedNotification)
    {
        MountMgrNotifyNameChange(DeviceExtension, &SymbolicName, FALSE);
    }

    /* Get output buffer */
    Output = (PMOUNTMGR_VOLUME_PATHS)Irp->AssociatedIrp.SystemBuffer;

    /* Set required size */
    Output->MultiSzLength = Paths->MultiSzLength;

    /* Compute total length */
    OutputLength = Output->MultiSzLength + sizeof(ULONG);

    /* If it cannot fit, just return need size and quit */
    if (OutputLength > Stack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        Irp->IoStatus.Information = sizeof(ULONG);
        FreePool(Paths);
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Copy data and quit */
    Irp->IoStatus.Information = OutputLength;
    RtlCopyMemory(Output->MultiSz, Paths->MultiSz, Output->MultiSzLength);
    FreePool(Paths);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrKeepLinksWhenOffline(IN PDEVICE_EXTENSION DeviceExtension,
                             IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    UNICODE_STRING SymbolicName;
    PMOUNTMGR_TARGET_NAME Target;
    PDEVICE_INFORMATION DeviceInformation;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_TARGET_NAME))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Target = (PMOUNTMGR_TARGET_NAME)Irp->AssociatedIrp.SystemBuffer;
    if (Target->DeviceNameLength + sizeof(USHORT) > Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    SymbolicName.Length =
    SymbolicName.MaximumLength = Target->DeviceNameLength;
    SymbolicName.Buffer = Target->DeviceName;

    /* Find the associated device */
    Status = FindDeviceInfo(DeviceExtension, &SymbolicName, FALSE, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Mark we want to keep links */
    DeviceInformation->KeepLinks = TRUE;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrVolumeArrivalNotification(IN PDEVICE_EXTENSION DeviceExtension,
                                  IN PIRP Irp)
{
    NTSTATUS Status;
    BOOLEAN OldState;
    PIO_STACK_LOCATION Stack;
    UNICODE_STRING SymbolicName;
    PMOUNTMGR_TARGET_NAME Target;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_TARGET_NAME))
    {
        return STATUS_INVALID_PARAMETER;
    }

    Target = (PMOUNTMGR_TARGET_NAME)Irp->AssociatedIrp.SystemBuffer;
    if (Target->DeviceNameLength + sizeof(USHORT) > Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    SymbolicName.Length =
    SymbolicName.MaximumLength = Target->DeviceNameLength;
    SymbolicName.Buffer = Target->DeviceName;

    /* Disable hard errors */
    OldState = PsGetThreadHardErrorsAreDisabled(PsGetCurrentThread());
    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), TRUE);

    /* Call real worker */
    Status = MountMgrMountedDeviceArrival(DeviceExtension, &SymbolicName, TRUE);

    PsSetThreadHardErrorsAreDisabled(PsGetCurrentThread(), OldState);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrQueryPoints(IN PDEVICE_EXTENSION DeviceExtension,
                    IN PIRP Irp)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PMOUNTMGR_MOUNT_POINT MountPoint;
    UNICODE_STRING SymbolicName, DeviceName;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input... */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_MOUNT_POINT))
    {
        return STATUS_INVALID_PARAMETER;
    }

    MountPoint = (PMOUNTMGR_MOUNT_POINT)Irp->AssociatedIrp.SystemBuffer;
    if (!MountPoint->SymbolicLinkNameLength)
    {
        MountPoint->SymbolicLinkNameOffset = 0;
    }

    if (!MountPoint->UniqueIdLength)
    {
        MountPoint->UniqueIdOffset = 0;
    }

    if (!MountPoint->DeviceNameLength)
    {
        MountPoint->DeviceNameOffset = 0;
    }

    /* Addresses can't be odd */
    if ((MountPoint->SymbolicLinkNameOffset & 1) ||
        (MountPoint->SymbolicLinkNameLength & 1))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((MountPoint->UniqueIdOffset & 1) ||
        (MountPoint->UniqueIdLength & 1))
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((MountPoint->DeviceNameOffset & 1) ||
        (MountPoint->DeviceNameLength & 1))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* We can't go beyond */
    if (((ULONG)MountPoint->SymbolicLinkNameLength + MountPoint->UniqueIdLength +
        MountPoint->DeviceNameLength) > Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (Stack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(MOUNTMGR_MOUNT_POINTS))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If caller provided a Symlink, use it */
    if (MountPoint->SymbolicLinkNameLength != 0)
    {
        if (MountPoint->SymbolicLinkNameLength > MAXSHORT)
        {
            return STATUS_INVALID_PARAMETER;
        }

        SymbolicName.Length = MountPoint->SymbolicLinkNameLength;
        SymbolicName.MaximumLength = MountPoint->SymbolicLinkNameLength + sizeof(WCHAR);
        SymbolicName.Buffer = AllocatePool(SymbolicName.MaximumLength);
        if (!SymbolicName.Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(SymbolicName.Buffer,
                      (PWSTR)((ULONG_PTR)MountPoint + MountPoint->SymbolicLinkNameOffset),
                      SymbolicName.Length);
        SymbolicName.Buffer[SymbolicName.Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Query links using it */
        Status = QueryPointsFromSymbolicLinkName(DeviceExtension, &SymbolicName, Irp);
        FreePool(SymbolicName.Buffer);
    }
    /* If user provided an unique ID */
    else if (MountPoint->UniqueIdLength != 0)
    {
        UniqueId = AllocatePool(MountPoint->UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
        if (!UniqueId)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        UniqueId->UniqueIdLength = MountPoint->UniqueIdLength;
        RtlCopyMemory(UniqueId->UniqueId,
                      (PVOID)((ULONG_PTR)MountPoint + MountPoint->UniqueIdOffset),
                      MountPoint->UniqueIdLength);

         /* Query links using it */
         Status = QueryPointsFromMemory(DeviceExtension, Irp, UniqueId, NULL);
         FreePool(UniqueId);
    }
    /* If caller provided a device name */
    else if (MountPoint->DeviceNameLength != 0)
    {
        if (MountPoint->DeviceNameLength > MAXSHORT)
        {
            return STATUS_INVALID_PARAMETER;
        }

        DeviceName.Length = MountPoint->DeviceNameLength;
        DeviceName.MaximumLength = MountPoint->DeviceNameLength + sizeof(WCHAR);
        DeviceName.Buffer = AllocatePool(DeviceName.MaximumLength);
        if (!DeviceName.Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(DeviceName.Buffer,
                      (PWSTR)((ULONG_PTR)MountPoint + MountPoint->DeviceNameOffset),
                      DeviceName.Length);
        DeviceName.Buffer[DeviceName.Length / sizeof(WCHAR)] = UNICODE_NULL;

         /* Query links using it */
        Status = QueryPointsFromMemory(DeviceExtension, Irp, NULL, &DeviceName);
        FreePool(DeviceName.Buffer);
    }
    else
    {
        /* Otherwise, query all links */
        Status = QueryPointsFromMemory(DeviceExtension, Irp, NULL, NULL);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrDeletePoints(IN PDEVICE_EXTENSION DeviceExtension,
                     IN PIRP Irp)
{
    ULONG Link;
    NTSTATUS Status;
    BOOLEAN CreateNoDrive;
    PIO_STACK_LOCATION Stack;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PMOUNTMGR_MOUNT_POINT MountPoint;
    PMOUNTMGR_MOUNT_POINTS MountPoints;
    UNICODE_STRING SymbolicName, DeviceName;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_MOUNT_POINT))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Query points */
    MountPoint = (PMOUNTMGR_MOUNT_POINT)Irp->AssociatedIrp.SystemBuffer;
    CreateNoDrive = (MountPoint->SymbolicLinkNameOffset && MountPoint->SymbolicLinkNameLength);

    Status = MountMgrQueryPoints(DeviceExtension, Irp);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* For all the points matching the request */
    MountPoints = (PMOUNTMGR_MOUNT_POINTS)Irp->AssociatedIrp.SystemBuffer;
    for (Link = 0; Link < MountPoints->NumberOfMountPoints; Link++)
    {
        SymbolicName.Length = MountPoints->MountPoints[Link].SymbolicLinkNameLength;
        SymbolicName.MaximumLength = SymbolicName.Length + sizeof(WCHAR);
        SymbolicName.Buffer = AllocatePool(SymbolicName.MaximumLength);
        if (!SymbolicName.Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(SymbolicName.Buffer,
                      (PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[Link].SymbolicLinkNameOffset),
                      SymbolicName.Length);
        SymbolicName.Buffer[SymbolicName.Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* Create a no drive entry for the drive letters */
        if (CreateNoDrive && IsDriveLetter(&SymbolicName))
        {
            UniqueId = AllocatePool(MountPoints->MountPoints[Link].UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
            if (UniqueId)
            {
                UniqueId->UniqueIdLength = MountPoints->MountPoints[Link].UniqueIdLength;
                RtlCopyMemory(UniqueId->UniqueId,
                              (PMOUNTDEV_UNIQUE_ID)((ULONG_PTR)MountPoints + MountPoints->MountPoints[Link].UniqueIdOffset),
                              MountPoints->MountPoints[Link].UniqueIdLength);

                CreateNoDriveLetterEntry(UniqueId);
                FreePool(UniqueId);
            }
        }

        /* If there are no link any more, and no need to create a no drive entry */
        if (Link == 0 && !CreateNoDrive)
        {
            /* Then, delete everything */
            UniqueId = AllocatePool(MountPoints->MountPoints[Link].UniqueIdLength);
            if (UniqueId)
            {
                RtlCopyMemory(UniqueId,
                              (PMOUNTDEV_UNIQUE_ID)((ULONG_PTR)MountPoints + MountPoints->MountPoints[Link].UniqueIdOffset),
                              MountPoints->MountPoints[Link].UniqueIdLength);

                DeleteNoDriveLetterEntry(UniqueId);
                FreePool(UniqueId);
            }
        }

        /* Delete all the information about the mount point */
        GlobalDeleteSymbolicLink(&SymbolicName);
        DeleteSymbolicLinkNameFromMemory(DeviceExtension, &SymbolicName, FALSE);
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, DatabasePath, SymbolicName.Buffer);
        FreePool(SymbolicName.Buffer);

        /* Notify the change */
        DeviceName.Length = DeviceName.MaximumLength =
        MountPoints->MountPoints[Link].DeviceNameLength;
        DeviceName.Buffer = (PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[Link].DeviceNameOffset);
        MountMgrNotifyNameChange(DeviceExtension, &DeviceName, TRUE);
    }

    MountMgrNotify(DeviceExtension);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrDeletePointsDbOnly(IN PDEVICE_EXTENSION DeviceExtension,
                           IN PIRP Irp)
{
    ULONG Link;
    NTSTATUS Status;
    UNICODE_STRING SymbolicName;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PMOUNTMGR_MOUNT_POINTS MountPoints;

    /* Query points */
    Status = MountMgrQueryPoints(DeviceExtension, Irp);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    MountPoints = (PMOUNTMGR_MOUNT_POINTS)Irp->AssociatedIrp.SystemBuffer;
    if (MountPoints->NumberOfMountPoints == 0)
    {
        return Status;
    }

    /* For all the mount points */
    for (Link = 0; Link < MountPoints->NumberOfMountPoints; Link++)
    {
        SymbolicName.Length = MountPoints->MountPoints[Link].SymbolicLinkNameLength;
        SymbolicName.MaximumLength = SymbolicName.Length + sizeof(WCHAR);
        SymbolicName.Buffer = AllocatePool(SymbolicName.MaximumLength);
        if (!SymbolicName.Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(SymbolicName.Buffer,
                      (PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[Link].SymbolicLinkNameOffset),
                      SymbolicName.Length);
        SymbolicName.Buffer[SymbolicName.Length / sizeof(WCHAR)] = UNICODE_NULL;

        /* If the only mount point is a drive letter, then create a no letter drive entry */
        if (MountPoints->NumberOfMountPoints == 1 && IsDriveLetter(&SymbolicName))
        {
            UniqueId = AllocatePool(MountPoints->MountPoints[Link].UniqueIdLength + sizeof(MOUNTDEV_UNIQUE_ID));
            if (UniqueId)
            {
                UniqueId->UniqueIdLength = MountPoints->MountPoints[Link].UniqueIdLength;
                RtlCopyMemory(UniqueId->UniqueId,
                              (PMOUNTDEV_UNIQUE_ID)((ULONG_PTR)MountPoints + MountPoints->MountPoints[Link].UniqueIdOffset),
                              MountPoints->MountPoints[Link].UniqueIdLength);

                CreateNoDriveLetterEntry(UniqueId);
                FreePool(UniqueId);
            }
        }

        /* Simply delete mount point from DB */
        DeleteSymbolicLinkNameFromMemory(DeviceExtension, &SymbolicName, TRUE);
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, DatabasePath, SymbolicName.Buffer);
        FreePool(SymbolicName.Buffer);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrVolumeMountPointChanged(IN PDEVICE_EXTENSION DeviceExtension,
                                IN PIRP Irp,
                                IN NTSTATUS LockStatus,
                                OUT PUNICODE_STRING SourceDeviceName,
                                OUT PUNICODE_STRING SourceSymbolicName,
                                OUT PUNICODE_STRING TargetVolumeName)
{
    HANDLE Handle;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    ULONG Length, SavedLength;
    BOOLEAN FOReferenced = FALSE;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PDEVICE_INFORMATION DeviceInformation;
    OBJECT_NAME_INFORMATION ObjectNameInfo;
    FILE_FS_DEVICE_INFORMATION FsDeviceInfo;
    PFILE_NAME_INFORMATION FileNameInfo = NULL;
    PMOUNTMGR_VOLUME_MOUNT_POINT VolumeMountPoint;
    POBJECT_NAME_INFORMATION ObjectNameInfoPtr = NULL;
    UNICODE_STRING SourceVolumeName, TargetDeviceName;

    Stack = IoGetCurrentIrpStackLocation(Irp);

    /* Validate input */
    if (Stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(MOUNTMGR_VOLUME_MOUNT_POINT))
    {
        return STATUS_INVALID_PARAMETER;
    }

    VolumeMountPoint = (PMOUNTMGR_VOLUME_MOUNT_POINT)Irp->AssociatedIrp.SystemBuffer;

    if (((ULONG)VolumeMountPoint->SourceVolumeNameLength + VolumeMountPoint->TargetVolumeNameLength) <
        Stack->Parameters.DeviceIoControl.InputBufferLength)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get source volume name */
    SourceVolumeName.Length =
    SourceVolumeName.MaximumLength = VolumeMountPoint->SourceVolumeNameLength;
    SourceVolumeName.Buffer = (PWSTR)((ULONG_PTR)VolumeMountPoint + VolumeMountPoint->SourceVolumeNameOffset);

    InitializeObjectAttributes(&ObjectAttributes,
                               &SourceVolumeName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    /* Open it */
    Status = ZwOpenFile(&Handle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_REPARSE_POINT);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    TargetDeviceName.Buffer = NULL;

    /* Query its attributes */
    Status = ZwQueryVolumeInformationFile(Handle,
                                          &IoStatusBlock,
                                          &FsDeviceInfo,
                                          sizeof(FsDeviceInfo),
                                          FileFsDeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (FsDeviceInfo.DeviceType != FILE_DEVICE_DISK && FsDeviceInfo.DeviceType != FILE_DEVICE_VIRTUAL_DISK)
    {
        goto Cleanup;
    }

    if (FsDeviceInfo.Characteristics != (FILE_REMOTE_DEVICE | FILE_REMOVABLE_MEDIA))
    {
        goto Cleanup;
    }

    /* Reference it */
    Status = ObReferenceObjectByHandle(Handle, 0, *IoFileObjectType, KernelMode, (PVOID *)&FileObject, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }
    FOReferenced = TRUE;

    /* Get file name */
    FileNameInfo = AllocatePool(sizeof(FILE_NAME_INFORMATION));
    if (!FileNameInfo)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    Status = ZwQueryInformationFile(Handle, &IoStatusBlock, FileNameInfo,
                                    sizeof(FILE_NAME_INFORMATION),
                                    FileNameInformation);
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        /* Now we have real length, use it */
        Length = FileNameInfo->FileNameLength;
        FreePool(FileNameInfo);

        FileNameInfo = AllocatePool(sizeof(FILE_NAME_INFORMATION) + Length);
        if (!FileNameInfo)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        /* Really query file name */
        Status = ZwQueryInformationFile(Handle, &IoStatusBlock, FileNameInfo,
                                        sizeof(FILE_NAME_INFORMATION) + Length,
                                        FileNameInformation);
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Get symbolic name */
    ObjectNameInfoPtr = &ObjectNameInfo;
    SavedLength = sizeof(OBJECT_NAME_INFORMATION);
    Status = ObQueryNameString(FileObject->DeviceObject, ObjectNameInfoPtr, sizeof(OBJECT_NAME_INFORMATION), &Length);
    if (Status == STATUS_INFO_LENGTH_MISMATCH)
    {
        /* Once again, with proper size, it works better */
        ObjectNameInfoPtr = AllocatePool(Length);
        if (!ObjectNameInfoPtr)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }

        SavedLength = Length;
        Status = ObQueryNameString(FileObject->DeviceObject, ObjectNameInfoPtr, SavedLength, &Length);
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Now, query the device name */
    Status = QueryDeviceInformation(&ObjectNameInfoPtr->Name, SourceDeviceName,
                                    NULL, NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* For target volume name, use input */
    TargetVolumeName->Length =
    TargetVolumeName->MaximumLength = VolumeMountPoint->TargetVolumeNameLength;
    TargetVolumeName->Buffer = (PWSTR)((ULONG_PTR)VolumeMountPoint + VolumeMountPoint->TargetVolumeNameOffset);

    /* Query its device name */
    Status = QueryDeviceInformation(TargetVolumeName, &TargetDeviceName,
                                    NULL, NULL, NULL, NULL, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Return symbolic name */
    SourceSymbolicName->Length =
    SourceSymbolicName->MaximumLength = (USHORT)FileNameInfo->FileNameLength;
    SourceSymbolicName->Buffer = (PWSTR)FileNameInfo;
    /* memmove allows memory overlap */
    RtlMoveMemory(SourceSymbolicName->Buffer, FileNameInfo->FileName, SourceSymbolicName->Length);
    FileNameInfo = NULL;

    /* Notify the change */
    MountMgrNotify(DeviceExtension);
    MountMgrNotifyNameChange(DeviceExtension, &TargetDeviceName, TRUE);

    /* If we are locked, sync databases if possible */
    if (NT_SUCCESS(LockStatus))
    {
        Status = FindDeviceInfo(DeviceExtension, SourceDeviceName, FALSE, &DeviceInformation);
        if (NT_SUCCESS(Status))
        {
            ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInformation);
        }
        else
        {
            Status = STATUS_PENDING;
        }
    }

Cleanup:
    if (TargetDeviceName.Buffer)
    {
        FreePool(TargetDeviceName.Buffer);
    }

    if (ObjectNameInfoPtr && ObjectNameInfoPtr != &ObjectNameInfo)
    {
        FreePool(ObjectNameInfoPtr);
    }

    if (FileNameInfo)
    {
        FreePool(FileNameInfo);
    }

    if (FOReferenced)
    {
        ObDereferenceObject(FileObject);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrVolumeMountPointCreated(IN PDEVICE_EXTENSION DeviceExtension,
                                IN PIRP Irp,
                                IN NTSTATUS LockStatus)
{
    LONG Offset;
    BOOLEAN Found;
    NTSTATUS Status;
    HANDLE RemoteDatabase;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PDATABASE_ENTRY DatabaseEntry;
    PASSOCIATED_DEVICE_ENTRY AssociatedEntry;
    PDEVICE_INFORMATION DeviceInformation, TargetDeviceInformation;
    UNICODE_STRING LinkTarget, SourceDeviceName, SourceSymbolicName, TargetVolumeName, VolumeName, DbName;

    /* Initialize string */
    LinkTarget.Length = 0;
    LinkTarget.MaximumLength = 0xC8;
    LinkTarget.Buffer = AllocatePool(LinkTarget.MaximumLength);
    if (LinkTarget.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* If the mount point was created, then, it changed!
     * Also use it to query some information
     */
    Status = MountMgrVolumeMountPointChanged(DeviceExtension, Irp, LockStatus, &SourceDeviceName, &SourceSymbolicName, &TargetVolumeName);
    /* Pending means DB are under synchronization, bail out */
    if (Status == STATUS_PENDING)
    {
        FreePool(LinkTarget.Buffer);
        FreePool(SourceDeviceName.Buffer);
        FreePool(SourceSymbolicName.Buffer);
        return STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(Status))
    {
        FreePool(LinkTarget.Buffer);
        return Status;
    }

    /* Query the device information */
    Status = FindDeviceInfo(DeviceExtension, &SourceDeviceName, FALSE, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        /* If it failed, first try to get volume name */
        Status = QueryVolumeName(0, NULL, &SourceDeviceName, &LinkTarget, &VolumeName);
        if (!NT_SUCCESS(Status))
        {
            /* Then, try to read the symlink */
            Status = MountMgrQuerySymbolicLink(&SourceDeviceName, &LinkTarget);
            if (!NT_SUCCESS(Status))
            {
                FreePool(LinkTarget.Buffer);
                FreePool(SourceDeviceName.Buffer);
                FreePool(SourceSymbolicName.Buffer);
                return Status;
            }
        }
        else
        {
            FreePool(VolumeName.Buffer);
        }

        FreePool(SourceDeviceName.Buffer);

        SourceDeviceName.Length = LinkTarget.Length;
        SourceDeviceName.MaximumLength = LinkTarget.MaximumLength;
        SourceDeviceName.Buffer = LinkTarget.Buffer;

        /* Now that we have the correct source, reattempt to query information */
        Status = FindDeviceInfo(DeviceExtension, &SourceDeviceName, FALSE, &DeviceInformation);
        if (!NT_SUCCESS(Status))
        {
            FreePool(SourceDeviceName.Buffer);
            FreePool(SourceSymbolicName.Buffer);
            return Status;
        }
    }

    FreePool(SourceDeviceName.Buffer);

    /* Get information about target device */
    Status = FindDeviceInfo(DeviceExtension, &TargetVolumeName, FALSE, &TargetDeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        FreePool(SourceSymbolicName.Buffer);
        return Status;
    }

    /* Notify if not disabled */
    if (!TargetDeviceInformation->SkipNotifications)
    {
        PostOnlineNotification(DeviceExtension, &TargetDeviceInformation->SymbolicName);
    }

    /* Open the remote database */
    RemoteDatabase = OpenRemoteDatabase(DeviceInformation, TRUE);
    if (RemoteDatabase == 0)
    {
        FreePool(SourceSymbolicName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Browse all the entries */
    Offset = 0;
    Found = FALSE;
    for (;;)
    {
        DatabaseEntry = GetRemoteDatabaseEntry(RemoteDatabase, Offset);
        if (DatabaseEntry == NULL)
        {
            break;
        }

        /* Try to find ourselves */
        DbName.MaximumLength = DatabaseEntry->SymbolicNameLength;
        DbName.Length = DbName.MaximumLength;
        DbName.Buffer = (PWSTR)((ULONG_PTR)DatabaseEntry + DatabaseEntry->SymbolicNameOffset);
        if (RtlEqualUnicodeString(&TargetVolumeName, &DbName, TRUE))
        {
            /* Reference ourselves and update the entry */
            ++DatabaseEntry->EntryReferences;
            Status = WriteRemoteDatabaseEntry(RemoteDatabase, Offset, DatabaseEntry);
            FreePool(DatabaseEntry);
            Found = TRUE;
            break;
        }

        Offset += DatabaseEntry->EntrySize;
        FreePool(DatabaseEntry);
    }

    /* We couldn't find ourselves, we'll have to add ourselves */
    if (!Found)
    {
        ULONG EntrySize;
        PUNIQUE_ID_REPLICATE UniqueIdReplicate;

        /* Query the device unique ID */
        Status = QueryDeviceInformation(&TargetVolumeName, NULL, &UniqueId, NULL, NULL, NULL, NULL, NULL);
        if (!NT_SUCCESS(Status))
        {
            FreePool(SourceSymbolicName.Buffer);
            CloseRemoteDatabase(RemoteDatabase);
            return Status;
        }

        /* Allocate a database entry */
        EntrySize = UniqueId->UniqueIdLength + TargetVolumeName.Length + sizeof(DATABASE_ENTRY);
        DatabaseEntry = AllocatePool(EntrySize);
        if (DatabaseEntry == NULL)
        {
            FreePool(UniqueId);
            FreePool(SourceSymbolicName.Buffer);
            CloseRemoteDatabase(RemoteDatabase);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Fill it in */
        DatabaseEntry->EntrySize = EntrySize;
        DatabaseEntry->EntryReferences = 1;
        DatabaseEntry->SymbolicNameOffset = sizeof(DATABASE_ENTRY);
        DatabaseEntry->SymbolicNameLength = TargetVolumeName.Length;
        DatabaseEntry->UniqueIdOffset = TargetVolumeName.Length + sizeof(DATABASE_ENTRY);
        DatabaseEntry->UniqueIdLength = UniqueId->UniqueIdLength;
        RtlCopyMemory((PVOID)((ULONG_PTR)DatabaseEntry + sizeof(DATABASE_ENTRY)), TargetVolumeName.Buffer, DatabaseEntry->SymbolicNameLength);
        RtlCopyMemory((PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset), UniqueId->UniqueId, UniqueId->UniqueIdLength);

        /* And write it down */
        Status = AddRemoteDatabaseEntry(RemoteDatabase, DatabaseEntry);
        FreePool(DatabaseEntry);
        if (!NT_SUCCESS(Status))
        {
            FreePool(UniqueId);
            FreePool(SourceSymbolicName.Buffer);
            CloseRemoteDatabase(RemoteDatabase);
            return Status;
        }

        /* And now, allocate an Unique ID item */
        UniqueIdReplicate = AllocatePool(sizeof(UNIQUE_ID_REPLICATE));
        if (UniqueIdReplicate == NULL)
        {
            FreePool(UniqueId);
            FreePool(SourceSymbolicName.Buffer);
            CloseRemoteDatabase(RemoteDatabase);
            return Status;
        }

        /* To associate it with the device */
        UniqueIdReplicate->UniqueId = UniqueId;
        InsertTailList(&DeviceInformation->ReplicatedUniqueIdsListHead, &UniqueIdReplicate->ReplicatedUniqueIdsListEntry);
    }

    /* We're done with the remote database */
    CloseRemoteDatabase(RemoteDatabase);

    /* Check we were find writing the entry */
    if (!NT_SUCCESS(Status))
    {
        FreePool(SourceSymbolicName.Buffer);
        return Status;
    }

    /* This is the end, allocate an associated entry */
    AssociatedEntry = AllocatePool(sizeof(ASSOCIATED_DEVICE_ENTRY));
    if (AssociatedEntry == NULL)
    {
        FreePool(SourceSymbolicName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize its source name string */
    AssociatedEntry->String.Length = SourceSymbolicName.Length;
    AssociatedEntry->String.MaximumLength = AssociatedEntry->String.Length + sizeof(UNICODE_NULL);
    AssociatedEntry->String.Buffer = AllocatePool(AssociatedEntry->String.MaximumLength);
    if (AssociatedEntry->String.Buffer == NULL)
    {
        FreePool(AssociatedEntry);
        FreePool(SourceSymbolicName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Copy data & insert in list */
    RtlCopyMemory(AssociatedEntry->String.Buffer, SourceSymbolicName.Buffer, SourceSymbolicName.Length);
    AssociatedEntry->String.Buffer[SourceSymbolicName.Length / sizeof(WCHAR)] = UNICODE_NULL;
    AssociatedEntry->DeviceInformation = DeviceInformation;
    InsertTailList(&TargetDeviceInformation->AssociatedDevicesHead, &AssociatedEntry->AssociatedDevicesEntry);

    /* We're done! */
    FreePool(SourceSymbolicName.Buffer);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrVolumeMountPointDeleted(IN PDEVICE_EXTENSION DeviceExtension,
                                IN PIRP Irp,
                                IN NTSTATUS LockStatus)
{
    LONG Offset;
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    HANDLE RemoteDatabase;
    PDATABASE_ENTRY DatabaseEntry;
    PUNIQUE_ID_REPLICATE UniqueIdReplicate;
    PASSOCIATED_DEVICE_ENTRY AssociatedEntry;
    PDEVICE_INFORMATION DeviceInformation, TargetDeviceInformation;
    UNICODE_STRING LinkTarget, SourceDeviceName, SourceSymbolicName, TargetVolumeName, VolumeName, DbName;

    /* Initialize string */
    LinkTarget.Length = 0;
    LinkTarget.MaximumLength = 0xC8;
    LinkTarget.Buffer = AllocatePool(LinkTarget.MaximumLength);
    if (LinkTarget.Buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* If the mount point was deleted, then, it changed!
     * Also use it to query some information
     */
    Status = MountMgrVolumeMountPointChanged(DeviceExtension, Irp, LockStatus, &SourceDeviceName, &SourceSymbolicName, &TargetVolumeName);
    /* Pending means DB are under synchronization, bail out */
    if (Status == STATUS_PENDING)
    {
        FreePool(LinkTarget.Buffer);
        FreePool(SourceDeviceName.Buffer);
        FreePool(SourceSymbolicName.Buffer);
        return STATUS_SUCCESS;
    }
    else if (!NT_SUCCESS(Status))
    {
        FreePool(LinkTarget.Buffer);
        return Status;
    }

    /* Query the device information */
    Status = FindDeviceInfo(DeviceExtension, &SourceDeviceName, FALSE, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        /* If it failed, first try to get volume name */
        Status = QueryVolumeName(0, NULL, &SourceDeviceName, &LinkTarget, &VolumeName);
        if (!NT_SUCCESS(Status))
        {
            /* Then, try to read the symlink */
            Status = MountMgrQuerySymbolicLink(&SourceDeviceName, &LinkTarget);
            if (!NT_SUCCESS(Status))
            {
                FreePool(LinkTarget.Buffer);
                FreePool(SourceDeviceName.Buffer);
                FreePool(SourceSymbolicName.Buffer);
                return Status;
            }
        }
        else
        {
            FreePool(VolumeName.Buffer);
        }

        FreePool(SourceDeviceName.Buffer);

        SourceDeviceName.Length = LinkTarget.Length;
        SourceDeviceName.MaximumLength = LinkTarget.MaximumLength;
        SourceDeviceName.Buffer = LinkTarget.Buffer;

        /* Now that we have the correct source, reattempt to query information */
        Status = FindDeviceInfo(DeviceExtension, &SourceDeviceName, FALSE, &DeviceInformation);
        if (!NT_SUCCESS(Status))
        {
            FreePool(SourceDeviceName.Buffer);
            FreePool(SourceSymbolicName.Buffer);
            return Status;
        }
    }

    FreePool(SourceDeviceName.Buffer);

    /* Get information about target device */
    Status = FindDeviceInfo(DeviceExtension, &TargetVolumeName, FALSE, &TargetDeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        FreePool(SourceSymbolicName.Buffer);
        return Status;
    }

    /* Open the remote database */
    RemoteDatabase = OpenRemoteDatabase(DeviceInformation, TRUE);
    if (RemoteDatabase == 0)
    {
        FreePool(SourceSymbolicName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Browse all the entries */
    Offset = 0;
    for (;;)
    {
        DatabaseEntry = GetRemoteDatabaseEntry(RemoteDatabase, Offset);
        if (DatabaseEntry == NULL)
        {
            /* We didn't find ourselves, that's infortunate! */
            FreePool(SourceSymbolicName.Buffer);
            CloseRemoteDatabase(RemoteDatabase);
            return STATUS_INVALID_PARAMETER;
        }

        /* Try to find ourselves */
        DbName.MaximumLength = DatabaseEntry->SymbolicNameLength;
        DbName.Length = DbName.MaximumLength;
        DbName.Buffer = (PWSTR)((ULONG_PTR)DatabaseEntry + DatabaseEntry->SymbolicNameOffset);
        if (RtlEqualUnicodeString(&TargetVolumeName, &DbName, TRUE))
        {
            break;
        }

        Offset += DatabaseEntry->EntrySize;
        FreePool(DatabaseEntry);
    }

    /* Dereference ourselves */
    DatabaseEntry->EntryReferences--;
    if (DatabaseEntry->EntryReferences == 0)
    {
        /* If we're still referenced, just update the entry */
        Status = WriteRemoteDatabaseEntry(RemoteDatabase, Offset, DatabaseEntry);
    }
    else
    {
        /* Otherwise, delete the entry */
        Status = DeleteRemoteDatabaseEntry(RemoteDatabase, Offset);
        if (!NT_SUCCESS(Status))
        {
            FreePool(DatabaseEntry);
            FreePool(SourceSymbolicName.Buffer);
            CloseRemoteDatabase(RemoteDatabase);
            return Status;
        }

        /* Also, delete our unique ID replicated record */
        for (Entry = DeviceInformation->ReplicatedUniqueIdsListHead.Flink;
             Entry != &DeviceInformation->ReplicatedUniqueIdsListHead;
             Entry = Entry->Flink)
        {
            UniqueIdReplicate = CONTAINING_RECORD(Entry, UNIQUE_ID_REPLICATE, ReplicatedUniqueIdsListEntry);

            if (UniqueIdReplicate->UniqueId->UniqueIdLength == DatabaseEntry->UniqueIdLength &&
                RtlCompareMemory(UniqueIdReplicate->UniqueId->UniqueId,
                                 (PVOID)((ULONG_PTR)DatabaseEntry + DatabaseEntry->UniqueIdOffset),
                                 DatabaseEntry->UniqueIdLength) == DatabaseEntry->UniqueIdLength)
            {
                break;
            }
        }

        /* It has to exist! */
        if (Entry == &DeviceInformation->ReplicatedUniqueIdsListHead)
        {
            FreePool(DatabaseEntry);
            FreePool(SourceSymbolicName.Buffer);
            CloseRemoteDatabase(RemoteDatabase);
            return STATUS_UNSUCCESSFUL;
        }

        /* Remove it and free it */
        RemoveEntryList(&UniqueIdReplicate->ReplicatedUniqueIdsListEntry);
        FreePool(UniqueIdReplicate->UniqueId);
        FreePool(UniqueIdReplicate);
    }

    /* We're done with the remote database */
    FreePool(DatabaseEntry);
    CloseRemoteDatabase(RemoteDatabase);

    /* Check write operation succeed */
    if (!NT_SUCCESS(Status))
    {
        FreePool(SourceSymbolicName.Buffer);
        return Status;
    }

    /* Try to find our associated device entry */
    for (Entry = TargetDeviceInformation->AssociatedDevicesHead.Flink;
         Entry != &TargetDeviceInformation->AssociatedDevicesHead;
         Entry = Entry->Flink)
    {
        AssociatedEntry = CONTAINING_RECORD(Entry, ASSOCIATED_DEVICE_ENTRY, AssociatedDevicesEntry);

        /* If found, delete it */
        if (AssociatedEntry->DeviceInformation == DeviceInformation &&
            RtlEqualUnicodeString(&AssociatedEntry->String, &SourceSymbolicName, TRUE))
        {
            RemoveEntryList(&AssociatedEntry->AssociatedDevicesEntry);
            FreePool(AssociatedEntry->String.Buffer);
            FreePool(AssociatedEntry);
            break;
        }
    }

    /* We're done! */
    FreePool(SourceSymbolicName.Buffer);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
MountMgrDeviceControl(IN PDEVICE_OBJECT DeviceObject,
                      IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status, LockStatus;
    PDEVICE_EXTENSION DeviceExtension;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    DeviceExtension = DeviceObject->DeviceExtension;

    KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_MOUNTMGR_CREATE_POINT:
            Status = MountMgrCreatePoint(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS:
            Status = MountMgrDeletePoints(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_QUERY_POINTS:
            Status = MountMgrQueryPoints(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY:
            Status = MountMgrDeletePointsDbOnly(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER:
            Status = MountMgrNextDriveLetter(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS:
            DeviceExtension->AutomaticDriveLetter = TRUE;
            Status = STATUS_SUCCESS;

            MountMgrAssignDriveLetters(DeviceExtension);
            ReconcileAllDatabasesWithMaster(DeviceExtension);
            WaitForOnlinesToComplete(DeviceExtension);
            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED:
            KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

            LockStatus = WaitForRemoteDatabaseSemaphore(DeviceExtension);
            KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);
            Status = MountMgrVolumeMountPointCreated(DeviceExtension, Irp, LockStatus);
            if (NT_SUCCESS(LockStatus))
            {
                ReleaseRemoteDatabaseSemaphore(DeviceExtension);
            }

            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED:
            KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

            LockStatus = WaitForRemoteDatabaseSemaphore(DeviceExtension);
            KeWaitForSingleObject(&(DeviceExtension->DeviceLock), Executive, KernelMode, FALSE, NULL);
            Status = MountMgrVolumeMountPointDeleted(DeviceExtension, Irp, LockStatus);
            if (NT_SUCCESS(LockStatus))
            {
                ReleaseRemoteDatabaseSemaphore(DeviceExtension);
            }

            break;

        case IOCTL_MOUNTMGR_CHANGE_NOTIFY:
            Status = MountMgrChangeNotify(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE:
            Status = MountMgrKeepLinksWhenOffline(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES:
            Status = MountMgrCheckUnprocessedVolumes(DeviceExtension, Irp);
            goto Complete;

        case IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION:
            KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);
            Status = MountMgrVolumeArrivalNotification(DeviceExtension, Irp);
            goto Complete;

        case IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATH:
            Status = MountMgrQueryDosVolumePath(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_QUERY_DOS_VOLUME_PATHS:
            Status = MountMgrQueryDosVolumePaths(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_SCRUB_REGISTRY:
            Status = MountMgrScrubRegistry(DeviceExtension);
            break;

        case IOCTL_MOUNTMGR_QUERY_AUTO_MOUNT:
            Status = MountMgrQueryAutoMount(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_SET_AUTO_MOUNT:
            Status = MountMgrSetAutoMount(DeviceExtension, Irp);
            break;

        case IOCTL_MOUNTMGR_DEFINE_UNIX_DRIVE:
        case IOCTL_MOUNTMGR_QUERY_UNIX_DRIVE:
            DPRINT1("Winism! Rewrite the caller!\n");
        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
    }

    KeReleaseSemaphore(&(DeviceExtension->DeviceLock), IO_NO_INCREMENT, 1, FALSE);

    if (Status != STATUS_PENDING)
    {
        goto Complete;
    }

    return Status;

Complete:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}
