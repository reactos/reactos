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
 * FILE:             drivers/filesystem/mountmgr/point.c
 * PURPOSE:          Mount Manager - Mount points
 * PROGRAMMER:       Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#include "mntmgr.h"

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
NTSTATUS
MountMgrCreatePointWorker(IN PDEVICE_EXTENSION DeviceExtension,
                          IN PUNICODE_STRING SymbolicLinkName,
                          IN PUNICODE_STRING DeviceName)
{
    NTSTATUS Status;
    PLIST_ENTRY DeviceEntry;
    PMOUNTDEV_UNIQUE_ID UniqueId;
    PSYMLINK_INFORMATION SymlinkInformation;
    UNICODE_STRING SymLink, TargetDeviceName;
    PDEVICE_INFORMATION DeviceInformation = NULL, DeviceInfo;

    /* Get device name */
    Status = QueryDeviceInformation(SymbolicLinkName,
                                    &TargetDeviceName,
                                    NULL, NULL, NULL,
                                    NULL, NULL, NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* First of all, try to find device */
    for (DeviceEntry = DeviceExtension->DeviceListHead.Flink;
         DeviceEntry != &(DeviceExtension->DeviceListHead);
         DeviceEntry = DeviceEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(DeviceEntry, DEVICE_INFORMATION, DeviceListEntry);

        if (RtlCompareUnicodeString(&TargetDeviceName, &(DeviceInformation->DeviceName), TRUE) == 0)
        {
            break;
        }
    }

    /* Copy symbolic link name and null terminate it */
    SymLink.Buffer = AllocatePool(SymbolicLinkName->Length + sizeof(UNICODE_NULL));
    if (!SymLink.Buffer)
    {
        FreePool(TargetDeviceName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(SymLink.Buffer, SymbolicLinkName->Buffer, SymbolicLinkName->Length);
    SymLink.Buffer[SymbolicLinkName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    SymLink.Length = SymbolicLinkName->Length;
    SymLink.MaximumLength = SymbolicLinkName->Length + sizeof(UNICODE_NULL);

    /* If we didn't find device */
    if (DeviceEntry == &(DeviceExtension->DeviceListHead))
    {
        /* Then, try with unique ID */
        Status = QueryDeviceInformation(SymbolicLinkName,
                                        NULL, &UniqueId,
                                        NULL, NULL, NULL,
                                        NULL, NULL);
        if (!NT_SUCCESS(Status))
        {
            FreePool(TargetDeviceName.Buffer);
            FreePool(SymLink.Buffer);
            return Status;
        }

        /* Create a link to the device */
        Status = GlobalCreateSymbolicLink(&SymLink, &TargetDeviceName);
        if (!NT_SUCCESS(Status))
        {
            FreePool(UniqueId);
            FreePool(TargetDeviceName.Buffer);
            FreePool(SymLink.Buffer);
            return Status;
        }

        /* If caller provided driver letter, delete it */
        if (IsDriveLetter(&SymLink))
        {
            DeleteRegistryDriveLetter(UniqueId);
        }

        /* Device will be identified with its unique ID */
        Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       DatabasePath,
                                       SymLink.Buffer,
                                       REG_BINARY,
                                       UniqueId->UniqueId,
                                       UniqueId->UniqueIdLength);

        FreePool(UniqueId);
        FreePool(TargetDeviceName.Buffer);
        FreePool(SymLink.Buffer);
        return Status;
    }

    /* If call provided a driver letter whereas device already has one
     * fail, this is not doable
     */
    if (IsDriveLetter(&SymLink) && HasDriveLetter(DeviceInformation))
    {
        FreePool(TargetDeviceName.Buffer);
        FreePool(SymLink.Buffer);
        return STATUS_INVALID_PARAMETER;
    }

    /* Now, create a link */
    Status = GlobalCreateSymbolicLink(&SymLink, &TargetDeviceName);
    FreePool(TargetDeviceName.Buffer);
    if (!NT_SUCCESS(Status))
    {
        FreePool(SymLink.Buffer);
        return Status;
    }

    /* Associate Unique ID <-> symbolic name */
    UniqueId = DeviceInformation->UniqueId;
    Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   DatabasePath,
                                   SymLink.Buffer,
                                   REG_BINARY,
                                   UniqueId->UniqueId,
                                   UniqueId->UniqueIdLength);
    if (!NT_SUCCESS(Status))
    {
        GlobalDeleteSymbolicLink(&SymLink);
        FreePool(SymLink.Buffer);
        return Status;
    }

    /* Now, prepare to save the link with the device */
    SymlinkInformation = AllocatePool(sizeof(SYMLINK_INFORMATION));
    if (!SymlinkInformation)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        GlobalDeleteSymbolicLink(&SymLink);
        FreePool(SymLink.Buffer);
        return Status;
    }

    SymlinkInformation->Name.Length = SymLink.Length;
    SymlinkInformation->Name.MaximumLength = SymLink.Length + sizeof(UNICODE_NULL);
    SymlinkInformation->Name.Buffer = AllocatePool(SymlinkInformation->Name.MaximumLength);
    if (!SymlinkInformation->Name.Buffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        FreePool(SymlinkInformation);
        GlobalDeleteSymbolicLink(&SymLink);
        FreePool(SymLink.Buffer);
        return Status;
    }

    /* Save the link and mark it online */
    RtlCopyMemory(SymlinkInformation->Name.Buffer, SymLink.Buffer, SymlinkInformation->Name.Length);
    SymlinkInformation->Name.Buffer[SymlinkInformation->Name.Length / sizeof(WCHAR)] = UNICODE_NULL;
    SymlinkInformation->Online = TRUE;
    InsertTailList(&DeviceInformation->SymbolicLinksListHead, &SymlinkInformation->SymbolicLinksListEntry);
    SendLinkCreated(&(SymlinkInformation->Name));

    /* If we have a drive letter */
    if (IsDriveLetter(&SymLink))
    {
        /* Then, delete the no drive letter entry */
        DeleteNoDriveLetterEntry(UniqueId);

        /* And post online notification if asked */
        if (!DeviceInformation->SkipNotifications)
        {
            PostOnlineNotification(DeviceExtension, &DeviceInformation->SymbolicName);
        }
    }

    /* If that's a volume with automatic drive letter, it's now time to resync databases */
    if (MOUNTMGR_IS_VOLUME_NAME(&SymLink) && DeviceExtension->AutomaticDriveLetter)
    {
        for (DeviceEntry = DeviceExtension->DeviceListHead.Flink;
             DeviceEntry != &(DeviceExtension->DeviceListHead);
             DeviceEntry = DeviceEntry->Flink)
        {
            DeviceInfo = CONTAINING_RECORD(DeviceEntry, DEVICE_INFORMATION, DeviceListEntry);

            /* If there's one, ofc! */
            if (!DeviceInfo->NoDatabase)
            {
                ReconcileThisDatabaseWithMaster(DeviceExtension, DeviceInfo);
            }
        }
    }

    /* Notify & quit */
    FreePool(SymLink.Buffer);
    MountMgrNotify(DeviceExtension);

    if (!DeviceInformation->Volume)
    {
        MountMgrNotifyNameChange(DeviceExtension, DeviceName, FALSE);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
QueryPointsFromMemory(IN PDEVICE_EXTENSION DeviceExtension,
                      IN PIRP Irp,
                      IN PMOUNTDEV_UNIQUE_ID UniqueId OPTIONAL,
                      IN PUNICODE_STRING SymbolicName OPTIONAL)
{
    NTSTATUS Status;
    PIO_STACK_LOCATION Stack;
    UNICODE_STRING DeviceName;
    ULONG TotalSize, TotalSymLinks;
    PMOUNTMGR_MOUNT_POINTS MountPoints;
    PDEVICE_INFORMATION DeviceInformation;
    PLIST_ENTRY DeviceEntry, SymlinksEntry;
    PSYMLINK_INFORMATION SymlinkInformation;

    /* If we got a symbolic link, query device */
    if (SymbolicName)
    {
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

    /* Browse all the links to count number of links & size used */
    TotalSize = 0;
    TotalSymLinks = 0;
    for (DeviceEntry = DeviceExtension->DeviceListHead.Flink;
         DeviceEntry != &(DeviceExtension->DeviceListHead);
         DeviceEntry = DeviceEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(DeviceEntry, DEVICE_INFORMATION, DeviceListEntry);

        /* If we were given an unique ID, it has to match */
        if (UniqueId)
        {
            if (UniqueId->UniqueIdLength != DeviceInformation->UniqueId->UniqueIdLength)
            {
                continue;
            }

            if (RtlCompareMemory(UniqueId->UniqueId,
                                 DeviceInformation->UniqueId->UniqueId,
                                 UniqueId->UniqueIdLength) != UniqueId->UniqueIdLength)
            {
                continue;
            }
        }
        /* Or, if we had a symlink, it has to match */
        else if (SymbolicName)
        {
            if (!RtlEqualUnicodeString(&DeviceName, &(DeviceInformation->DeviceName), TRUE))
            {
                continue;
            }
        }

        /* Once here, it matched, save device name & unique ID size */
        TotalSize += DeviceInformation->DeviceName.Length + DeviceInformation->UniqueId->UniqueIdLength;

        /* And count number of symlinks (and their size) */
        for (SymlinksEntry = DeviceInformation->SymbolicLinksListHead.Flink;
             SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead);
             SymlinksEntry = SymlinksEntry->Flink)
        {
            SymlinkInformation = CONTAINING_RECORD(SymlinksEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

            TotalSize += SymlinkInformation->Name.Length;
            TotalSymLinks++;
        }

        /* We had a specific item to find
         * if we reach that point, we found it, no need to continue
         */
        if (UniqueId || SymbolicName)
        {
            break;
        }
    }

    /* If we were looking for specific item, ensure we found it */
    if (UniqueId || SymbolicName)
    {
        if (DeviceEntry == &(DeviceExtension->DeviceListHead))
        {
            if (DeviceName.Buffer)
            {
                FreePool(DeviceName.Buffer);
            }

            return STATUS_INVALID_PARAMETER;
        }
    }

    /* Now, ensure output buffer can hold everything */
    Stack = IoGetNextIrpStackLocation(Irp);
    MountPoints = (PMOUNTMGR_MOUNT_POINTS)Irp->AssociatedIrp.SystemBuffer;

    /* Ensure we set output to let user reallocate! */
    MountPoints->Size = sizeof(MOUNTMGR_MOUNT_POINTS) + TotalSize;
    MountPoints->NumberOfMountPoints = TotalSymLinks;

    if (MountPoints->Size > Stack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Now, start putting mount points */
    TotalSymLinks = 0;
    TotalSize = 0;
    for (DeviceEntry = DeviceExtension->DeviceListHead.Flink;
         DeviceEntry != &(DeviceExtension->DeviceListHead);
         DeviceEntry = DeviceEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(DeviceEntry, DEVICE_INFORMATION, DeviceListEntry);

        /* Find back correct mount point */
        if (UniqueId)
        {
            if (UniqueId->UniqueIdLength != DeviceInformation->UniqueId->UniqueIdLength)
            {
                continue;
            }

            if (RtlCompareMemory(UniqueId->UniqueId,
                                 DeviceInformation->UniqueId->UniqueId,
                                 UniqueId->UniqueIdLength) != UniqueId->UniqueIdLength)
            {
                continue;
            }
        }
        else if (SymbolicName)
        {
            if (!RtlEqualUnicodeString(&DeviceName, &(DeviceInformation->DeviceName), TRUE))
            {
                continue;
            }
        }

        /* Now we've got it, but all the data */
        for (SymlinksEntry = DeviceInformation->SymbolicLinksListHead.Flink;
             SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead);
             SymlinksEntry = SymlinksEntry->Flink)
        {
            SymlinkInformation = CONTAINING_RECORD(SymlinksEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);


            MountPoints->MountPoints[TotalSymLinks].SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINTS) +
                                                                             TotalSize;
            MountPoints->MountPoints[TotalSymLinks].SymbolicLinkNameLength = SymlinkInformation->Name.Length;
            MountPoints->MountPoints[TotalSymLinks].UniqueIdOffset = sizeof(MOUNTMGR_MOUNT_POINTS) +
                                                                     SymlinkInformation->Name.Length +
                                                                     TotalSize;
            MountPoints->MountPoints[TotalSymLinks].UniqueIdLength = DeviceInformation->UniqueId->UniqueIdLength;
            MountPoints->MountPoints[TotalSymLinks].DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINTS) +
                                                                       SymlinkInformation->Name.Length +
                                                                       DeviceInformation->UniqueId->UniqueIdLength +
                                                                       TotalSize;
            MountPoints->MountPoints[TotalSymLinks].DeviceNameLength = DeviceInformation->DeviceName.Length;

            RtlCopyMemory((PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[TotalSymLinks].SymbolicLinkNameOffset),
                          SymlinkInformation->Name.Buffer, SymlinkInformation->Name.Length);
            RtlCopyMemory((PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[TotalSymLinks].UniqueIdOffset),
                          DeviceInformation->UniqueId->UniqueId, DeviceInformation->UniqueId->UniqueIdLength);
            RtlCopyMemory((PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[TotalSymLinks].DeviceNameOffset),
                  DeviceInformation->DeviceName.Buffer, DeviceInformation->DeviceName.Length);

            /* Update counters */
            TotalSymLinks++;
            TotalSize += SymlinkInformation->Name.Length + DeviceInformation->UniqueId->UniqueIdLength +
                         DeviceInformation->DeviceName.Length;
        }

        if (UniqueId || SymbolicName)
        {
            break;
        }
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
QueryPointsFromSymbolicLinkName(IN PDEVICE_EXTENSION DeviceExtension,
                                IN PUNICODE_STRING SymbolicName,
                                IN PIRP Irp)
{
    NTSTATUS Status;
    ULONG TotalLength;
    PIO_STACK_LOCATION Stack;
    UNICODE_STRING DeviceName;
    PMOUNTMGR_MOUNT_POINTS MountPoints;
    PDEVICE_INFORMATION DeviceInformation = NULL;
    PLIST_ENTRY DeviceEntry, SymlinksEntry;
    PSYMLINK_INFORMATION SymlinkInformation;

    /* Find device */
    Status = QueryDeviceInformation(SymbolicName, &DeviceName,
                                    NULL, NULL, NULL,
                                    NULL, NULL, NULL);
    if (NT_SUCCESS(Status))
    {
        /* Look for the device information */
        for (DeviceEntry = DeviceExtension->DeviceListHead.Flink;
             DeviceEntry != &(DeviceExtension->DeviceListHead);
             DeviceEntry = DeviceEntry->Flink)
        {
            DeviceInformation = CONTAINING_RECORD(DeviceEntry, DEVICE_INFORMATION, DeviceListEntry);

            if (RtlEqualUnicodeString(&DeviceName, &(DeviceInformation->DeviceName), TRUE) == 0)
            {
                break;
            }
        }

        FreePool(DeviceName.Buffer);

        if (DeviceEntry == &(DeviceExtension->DeviceListHead))
        {
            return STATUS_INVALID_PARAMETER;
        }

        /* Check for the link */
        for (SymlinksEntry = DeviceInformation->SymbolicLinksListHead.Flink;
             SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead);
             SymlinksEntry = DeviceEntry->Flink)
        {
            SymlinkInformation = CONTAINING_RECORD(SymlinksEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

            if (RtlEqualUnicodeString(SymbolicName, &SymlinkInformation->Name, TRUE) == 0)
            {
                break;
            }
        }

        if (SymlinksEntry == &(DeviceInformation->SymbolicLinksListHead))
        {
            return STATUS_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Browse all the devices to try to find the one
         * that has the given link...
         */
        for (DeviceEntry = DeviceExtension->DeviceListHead.Flink;
             DeviceEntry != &(DeviceExtension->DeviceListHead);
             DeviceEntry = DeviceEntry->Flink)
        {
            DeviceInformation = CONTAINING_RECORD(DeviceEntry, DEVICE_INFORMATION, DeviceListEntry);

            for (SymlinksEntry = DeviceInformation->SymbolicLinksListHead.Flink;
                 SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead);
                 SymlinksEntry = SymlinksEntry->Flink)
            {
                SymlinkInformation = CONTAINING_RECORD(SymlinksEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

                if (RtlEqualUnicodeString(SymbolicName, &SymlinkInformation->Name, TRUE) == 0)
                {
                    break;
                }
            }

            if (SymlinksEntry != &(DeviceInformation->SymbolicLinksListHead))
            {
                break;
            }
        }

        /* Even that way we didn't find, give up! */
        if (DeviceEntry == &(DeviceExtension->DeviceListHead))
        {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    /* Get output buffer */
    Stack = IoGetNextIrpStackLocation(Irp);
    MountPoints = (PMOUNTMGR_MOUNT_POINTS)Irp->AssociatedIrp.SystemBuffer;

    /* Compute output length */
    TotalLength = DeviceInformation->UniqueId->UniqueIdLength +
                  SymlinkInformation->Name.Length + DeviceInformation->DeviceName.Length;

    /* Give length to allow reallocation */
    MountPoints->Size = sizeof(MOUNTMGR_MOUNT_POINTS) + TotalLength;
    MountPoints->NumberOfMountPoints = 1;

    if (MountPoints->Size > Stack->Parameters.DeviceIoControl.OutputBufferLength)
    {
        return STATUS_BUFFER_OVERFLOW;
    }

    /* Write out data */
    MountPoints->MountPoints[0].SymbolicLinkNameOffset = sizeof(MOUNTMGR_MOUNT_POINTS);
    MountPoints->MountPoints[0].SymbolicLinkNameLength = SymlinkInformation->Name.Length;
    /* If link is online write it's unique ID, otherwise, forget about it */
    if (SymlinkInformation->Online)
    {
        MountPoints->MountPoints[0].UniqueIdOffset = sizeof(MOUNTMGR_MOUNT_POINTS) +
                                                     SymlinkInformation->Name.Length;
        MountPoints->MountPoints[0].UniqueIdLength = DeviceInformation->UniqueId->UniqueIdLength;
    }
    else
    {
        MountPoints->MountPoints[0].UniqueIdOffset = 0;
        MountPoints->MountPoints[0].UniqueIdLength = 0;
    }

    MountPoints->MountPoints[0].DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINTS) +
                                                   SymlinkInformation->Name.Length +
                                                   DeviceInformation->UniqueId->UniqueIdLength;
    MountPoints->MountPoints[0].DeviceNameLength = DeviceInformation->DeviceName.Length;

    RtlCopyMemory((PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[0].SymbolicLinkNameOffset),
                  SymlinkInformation->Name.Buffer, SymlinkInformation->Name.Length);

    if (SymlinkInformation->Online)
    {
        RtlCopyMemory((PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[0].UniqueIdOffset),
                      DeviceInformation->UniqueId->UniqueId, DeviceInformation->UniqueId->UniqueIdLength);
    }

    RtlCopyMemory((PWSTR)((ULONG_PTR)MountPoints + MountPoints->MountPoints[0].DeviceNameOffset),
                  DeviceInformation->DeviceName.Buffer, DeviceInformation->DeviceName.Length);

    return STATUS_SUCCESS;
}
