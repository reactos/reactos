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
 * FILE:             drivers/filesystem/mountmgr/symlink.c
 * PURPOSE:          Mount Manager - Symbolic links functions
 * PROGRAMMER:       Pierre Schweitzer (pierre.schweitzer@reactos.org)
 */

#include "mntmgr.h"

#define NDEBUG
#include <debug.h>

UNICODE_STRING DeviceMount = RTL_CONSTANT_STRING(MOUNTMGR_DEVICE_NAME);
UNICODE_STRING DosDevicesMount = RTL_CONSTANT_STRING(L"\\DosDevices\\MountPointManager");
UNICODE_STRING DosDevices = RTL_CONSTANT_STRING(L"\\DosDevices\\");
UNICODE_STRING DeviceFloppy = RTL_CONSTANT_STRING(L"\\Device\\Floppy");
UNICODE_STRING DeviceCdRom = RTL_CONSTANT_STRING(L"\\Device\\CdRom");
UNICODE_STRING DosGlobal = RTL_CONSTANT_STRING(L"\\GLOBAL??\\");
UNICODE_STRING Global = RTL_CONSTANT_STRING(L"\\??\\");
UNICODE_STRING SafeVolumes = RTL_CONSTANT_STRING(L"\\Device\\VolumesSafeForWriteAccess");
UNICODE_STRING Volume = RTL_CONSTANT_STRING(L"\\??\\Volume");
UNICODE_STRING ReparseIndex = RTL_CONSTANT_STRING(L"\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION");

/*
 * @implemented
 */
NTSTATUS
CreateStringWithGlobal(IN PUNICODE_STRING DosName,
                       OUT PUNICODE_STRING GlobalString)
{
    UNICODE_STRING IntGlobal;

    if (RtlPrefixUnicodeString(&DosDevices, DosName, TRUE))
    {
        /* DOS device - use DOS global */
        IntGlobal.Length = DosName->Length - DosDevices.Length + DosGlobal.Length;
        IntGlobal.MaximumLength = IntGlobal.Length + sizeof(WCHAR);
        IntGlobal.Buffer = AllocatePool(IntGlobal.MaximumLength);
        if (!IntGlobal.Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(IntGlobal.Buffer, DosGlobal.Buffer, DosGlobal.Length);
        RtlCopyMemory(IntGlobal.Buffer + (DosGlobal.Length / sizeof(WCHAR)),
                      DosName->Buffer + (DosDevices.Length / sizeof(WCHAR)),
                      DosName->Length - DosDevices.Length);
        IntGlobal.Buffer[IntGlobal.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else if (RtlPrefixUnicodeString(&Global, DosName, TRUE))
    {
        /* Switch to DOS global */
        IntGlobal.Length = DosName->Length - Global.Length + DosGlobal.Length;
        IntGlobal.MaximumLength = IntGlobal.Length + sizeof(WCHAR);
        IntGlobal.Buffer = AllocatePool(IntGlobal.MaximumLength);
        if (!IntGlobal.Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(IntGlobal.Buffer, DosGlobal.Buffer, DosGlobal.Length);
        RtlCopyMemory(IntGlobal.Buffer + (DosGlobal.Length / sizeof(WCHAR)),
                      DosName->Buffer + (Global.Length / sizeof(WCHAR)),
                      DosName->Length - Global.Length);
        IntGlobal.Buffer[IntGlobal.Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        /* Simply duplicate string */
        IntGlobal.Length = DosName->Length;
        IntGlobal.MaximumLength = DosName->MaximumLength;
        IntGlobal.Buffer = AllocatePool(IntGlobal.MaximumLength);
        if (!IntGlobal.Buffer)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(IntGlobal.Buffer, DosName->Buffer, IntGlobal.MaximumLength);
    }

    /* Return string */
    GlobalString->Length = IntGlobal.Length;
    GlobalString->MaximumLength = IntGlobal.MaximumLength;
    GlobalString->Buffer = IntGlobal.Buffer;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
GlobalCreateSymbolicLink(IN PUNICODE_STRING DosName,
                         IN PUNICODE_STRING DeviceName)
{
    NTSTATUS Status;
    UNICODE_STRING GlobalName;

    /* First create the global string */
    Status = CreateStringWithGlobal(DosName, &GlobalName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Then, create the symlink */
    Status = IoCreateSymbolicLink(&GlobalName, DeviceName);

    FreePool(GlobalName.Buffer);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
GlobalDeleteSymbolicLink(IN PUNICODE_STRING DosName)
{
    NTSTATUS Status;
    UNICODE_STRING GlobalName;

    /* Recreate the string (to find the link) */
    Status = CreateStringWithGlobal(DosName, &GlobalName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* And delete the link */
    Status = IoDeleteSymbolicLink(&GlobalName);

    FreePool(GlobalName.Buffer);

    return Status;
}

/*
 * @implemented
 */
VOID
SendLinkCreated(IN PUNICODE_STRING SymbolicName)
{
    PIRP Irp;
    KEVENT Event;
    ULONG NameSize;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    PMOUNTDEV_NAME Name = NULL;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get the device associated with the name */
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Get attached device (will notify it) */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    /* NameSize is the size of the whole MOUNTDEV_NAME struct */
    NameSize = sizeof(USHORT) + SymbolicName->Length;
    Name = AllocatePool(NameSize);
    if (!Name)
    {
        goto Cleanup;
    }

    /* Initialize struct */
    Name->NameLength = SymbolicName->Length;
    RtlCopyMemory(Name->Name, SymbolicName->Buffer, SymbolicName->Length);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    /* Microsoft does it twice... Once with limited access, second with any
     * So, first one here
     */
    Irp = IoBuildDeviceIoControlRequest(CTL_CODE(MOUNTDEVCONTROLTYPE, 4, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
                                        DeviceObject,
                                        Name,
                                        NameSize,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    /* This one can fail, no one matters */
    if (Irp)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->FileObject = FileObject;

        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
    }

    /* Then, second one */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_LINK_CREATED,
                                        DeviceObject,
                                        Name,
                                        NameSize,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        goto Cleanup;
    }

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->FileObject = FileObject;

    /* Really notify */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

Cleanup:
    if (Name)
    {
        FreePool(Name);
    }

    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);

    return;
}

/*
 * @implemented
 */
VOID
SendLinkDeleted(IN PUNICODE_STRING DeviceName,
                IN PUNICODE_STRING SymbolicName)
{
    PIRP Irp;
    KEVENT Event;
    ULONG NameSize;
    NTSTATUS Status;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;
    PMOUNTDEV_NAME Name = NULL;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Get the device associated with the name */
    Status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return;
    }

    /* Get attached device (will notify it) */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    /* NameSize is the size of the whole MOUNTDEV_NAME struct */
    NameSize = sizeof(USHORT) + SymbolicName->Length;
    Name = AllocatePool(NameSize);
    if (!Name)
    {
        goto Cleanup;
    }

    /* Initialize struct */
    Name->NameLength = SymbolicName->Length;
    RtlCopyMemory(Name->Name, SymbolicName->Buffer, SymbolicName->Length);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    /* Cf: SendLinkCreated comment */
    Irp = IoBuildDeviceIoControlRequest(CTL_CODE(MOUNTDEVCONTROLTYPE, 5, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS),
                                        DeviceObject,
                                        Name,
                                        NameSize,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    /* This one can fail, no one matters */
    if (Irp)
    {
        Stack = IoGetNextIrpStackLocation(Irp);
        Stack->FileObject = FileObject;

        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        }
    }

    /* Then, second one */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_LINK_DELETED,
                                        DeviceObject,
                                        Name,
                                        NameSize,
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        goto Cleanup;
    }

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->FileObject = FileObject;

    /* Really notify */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

Cleanup:
    if (Name)
    {
        FreePool(Name);
    }

    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);

    return;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
SymbolicLinkNamesFromUniqueIdCount(IN PWSTR ValueName,
                                   IN ULONG ValueType,
                                   IN PVOID ValueData,
                                   IN ULONG ValueLength,
                                   IN PVOID Context,
                                   IN PVOID EntryContext)
{
    UNICODE_STRING ValueNameString;
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;

    if (ValueName[0] != L'#' || ValueType != REG_BINARY ||
        (UniqueId->UniqueIdLength != ValueLength))
    {
        return STATUS_SUCCESS;
    }

    if (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) != ValueLength)
    {
        return STATUS_SUCCESS;
    }

    /* That one matched, increase count */
    RtlInitUnicodeString(&ValueNameString, ValueName);
    if (ValueNameString.Length)
    {
        (*((PULONG)EntryContext))++;
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
SymbolicLinkNamesFromUniqueIdQuery(IN PWSTR ValueName,
                                   IN ULONG ValueType,
                                   IN PVOID ValueData,
                                   IN ULONG ValueLength,
                                   IN PVOID Context,
                                   IN PVOID EntryContext)
{
    UNICODE_STRING ValueNameString;
    PMOUNTDEV_UNIQUE_ID UniqueId = Context;
    /* Unicode strings table */
    PUNICODE_STRING ReturnString = EntryContext;

    if (ValueName[0] != L'#' || ValueType != REG_BINARY ||
        (UniqueId->UniqueIdLength != ValueLength))
    {
        return STATUS_SUCCESS;
    }

    if (RtlCompareMemory(UniqueId->UniqueId, ValueData, ValueLength) != ValueLength)
    {
        return STATUS_SUCCESS;
    }

    /* Unique ID matches, let's put the symlink */
    RtlInitUnicodeString(&ValueNameString, ValueName);
    if (!ValueNameString.Length)
    {
        return STATUS_SUCCESS;
    }

    /* Allocate string to copy */
    ValueNameString.Buffer = AllocatePool(ValueNameString.MaximumLength);
    if (!ValueNameString.Buffer)
    {
        return STATUS_SUCCESS;
    }

    /* Copy */
    RtlCopyMemory(ValueNameString.Buffer, ValueName, ValueNameString.Length);
    ValueNameString.Buffer[ValueNameString.Length / sizeof(WCHAR)] = UNICODE_NULL;

    while (ReturnString->Length)
    {
        ReturnString++;
    }

    /* And return that string */
    *ReturnString = ValueNameString;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
CreateNewVolumeName(OUT PUNICODE_STRING VolumeName,
                    IN PGUID VolumeGuid OPTIONAL)
{
    GUID Guid;
    NTSTATUS Status;
    UNICODE_STRING GuidString;

    /* If no GUID was provided, then create one */
    if (!VolumeGuid)
    {
        Status = ExUuidCreate(&Guid);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        RtlCopyMemory(&Guid, VolumeGuid, sizeof(GUID));
    }

    /* Convert GUID to string */
    Status = RtlStringFromGUID(&Guid, &GuidString);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Size for volume namespace, literal GUID, and null char */
    VolumeName->MaximumLength = 0x14 + 0x4C + sizeof(UNICODE_NULL);
    VolumeName->Buffer = AllocatePool(0x14 + 0x4C + sizeof(UNICODE_NULL));
    if (!VolumeName->Buffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        RtlCopyUnicodeString(VolumeName, &Volume);
        RtlAppendUnicodeStringToString(VolumeName, &GuidString);
        VolumeName->Buffer[VolumeName->Length / sizeof(WCHAR)] = UNICODE_NULL;
        Status = STATUS_SUCCESS;
    }

    ExFreePoolWithTag(GuidString.Buffer, 0);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
QuerySymbolicLinkNamesFromStorage(IN PDEVICE_EXTENSION DeviceExtension,
                                  IN PDEVICE_INFORMATION DeviceInformation,
                                  IN PUNICODE_STRING SuggestedLinkName,
                                  IN BOOLEAN UseOnlyIfThereAreNoOtherLinks,
                                  OUT PUNICODE_STRING * SymLinks,
                                  OUT PULONG SymLinkCount,
                                  IN BOOLEAN HasGuid,
                                  IN LPGUID Guid)
{
    NTSTATUS Status;
    BOOLEAN WriteNew;
    RTL_QUERY_REGISTRY_TABLE QueryTable[2];

    UNREFERENCED_PARAMETER(DeviceExtension);

    /* First of all, count links */
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
    QueryTable[0].EntryContext = SymLinkCount;
    *SymLinkCount = 0;

    Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    DatabasePath,
                                    QueryTable,
                                    DeviceInformation->UniqueId,
                                    NULL);
    if (!NT_SUCCESS(Status))
    {
        *SymLinkCount = 0;
    }

    /* Check if we have to write a new one first */
    if (SuggestedLinkName && !IsDriveLetter(SuggestedLinkName) &&
        UseOnlyIfThereAreNoOtherLinks && *SymLinkCount == 0)
    {
        WriteNew = TRUE;
    }
    else
    {
        WriteNew = FALSE;
    }

    /* If has GUID, it makes one more link */
    if (HasGuid)
    {
        (*SymLinkCount)++;
    }

    if (WriteNew)
    {
        /* Write link */
        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                              DatabasePath,
                              SuggestedLinkName->Buffer,
                              REG_BINARY,
                              DeviceInformation->UniqueId->UniqueId,
                              DeviceInformation->UniqueId->UniqueIdLength);

        /* And recount all the needed links */
        RtlZeroMemory(QueryTable, sizeof(QueryTable));
        QueryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
        QueryTable[0].EntryContext = SymLinkCount;
        *SymLinkCount = 0;

        Status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        DatabasePath,
                                        QueryTable,
                                        DeviceInformation->UniqueId,
                                        NULL);
        if (!NT_SUCCESS(Status))
        {
            return STATUS_NOT_FOUND;
        }
    }

    /* Not links found? */
    if (!*SymLinkCount)
    {
        return STATUS_NOT_FOUND;
    }

    /* Allocate a buffer big enough to hold symlinks (table of unicode strings) */
    *SymLinks = AllocatePool(*SymLinkCount * sizeof(UNICODE_STRING));
    if (!*SymLinks)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Prepare to query links */
    RtlZeroMemory(*SymLinks, *SymLinkCount * sizeof(UNICODE_STRING));
    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdQuery;

    /* No GUID? Keep it that way */
    if (!HasGuid)
    {
        QueryTable[0].EntryContext = *SymLinks;
    }
    /* Otherwise, first create volume name */
    else
    {
        Status = CreateNewVolumeName(SymLinks[0], Guid);
        if (!NT_SUCCESS(Status))
        {
            FreePool(*SymLinks);
            return Status;
        }

        /* Skip first link (ours) */
        QueryTable[0].EntryContext = *SymLinks + 1;
    }

    /* Now, query */
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                           DatabasePath,
                           QueryTable,
                           DeviceInformation->UniqueId,
                           NULL);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PSAVED_LINK_INFORMATION
RemoveSavedLinks(IN PDEVICE_EXTENSION DeviceExtension,
                 IN PMOUNTDEV_UNIQUE_ID UniqueId)
{
    PLIST_ENTRY NextEntry;
    PSAVED_LINK_INFORMATION SavedLinkInformation;

    /* No saved links? Easy! */
    if (IsListEmpty(&(DeviceExtension->SavedLinksListHead)))
    {
        return NULL;
    }

    /* Now, browse saved links */
    for (NextEntry = DeviceExtension->SavedLinksListHead.Flink;
         NextEntry != &(DeviceExtension->SavedLinksListHead);
         NextEntry = NextEntry->Flink)
    {
        SavedLinkInformation = CONTAINING_RECORD(NextEntry,
                                                 SAVED_LINK_INFORMATION,
                                                 SavedLinksListEntry);

        /* Find the one that matches */
        if (SavedLinkInformation->UniqueId->UniqueIdLength == UniqueId->UniqueIdLength)
        {
            if (RtlCompareMemory(SavedLinkInformation->UniqueId->UniqueId,
                                 UniqueId->UniqueId,
                                 UniqueId->UniqueIdLength) ==
                UniqueId->UniqueIdLength)
            {
                /* Remove it and return it */
                RemoveEntryList(&(SavedLinkInformation->SavedLinksListEntry));
                return SavedLinkInformation;
            }
        }
    }

    /* None found (none removed) */
    return NULL;
}

/*
 * @implemented
 */
NTSTATUS
QuerySuggestedLinkName(IN PUNICODE_STRING SymbolicName,
                       OUT PUNICODE_STRING SuggestedLinkName,
                       OUT PBOOLEAN UseOnlyIfThereAreNoOtherLinks)
{
    PIRP Irp;
    KEVENT Event;
    NTSTATUS Status;
    USHORT NameLength;
    PFILE_OBJECT FileObject;
    PDEVICE_OBJECT DeviceObject;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IoStackLocation;
    PMOUNTDEV_SUGGESTED_LINK_NAME IoCtlSuggested;

    /* First, get device */
    Status = IoGetDeviceObjectPointer(SymbolicName,
                                      FILE_READ_ATTRIBUTES,
                                      &FileObject,
                                      &DeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Then, get attached device */
    DeviceObject = IoGetAttachedDeviceReference(FileObject->DeviceObject);

    /* Then, prepare buffer to query suggested name */
    IoCtlSuggested = AllocatePool(sizeof(MOUNTDEV_SUGGESTED_LINK_NAME));
    if (!IoCtlSuggested)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Dereference;
    }

    /* Prepare request */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);
    Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,
                                        DeviceObject,
                                        NULL,
                                        0,
                                        IoCtlSuggested,
                                        sizeof(MOUNTDEV_SUGGESTED_LINK_NAME),
                                        FALSE,
                                        &Event,
                                        &IoStatusBlock);
    if (!Irp)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Release;
    }

    IoStackLocation = IoGetNextIrpStackLocation(Irp);
    IoStackLocation->FileObject = FileObject;

    /* And ask */
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode,
                              FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    /* Overflow? Normal */
    if (Status == STATUS_BUFFER_OVERFLOW)
    {
        /* Reallocate big enough buffer */
        NameLength = IoCtlSuggested->NameLength + sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
        FreePool(IoCtlSuggested);

        IoCtlSuggested = AllocatePool(NameLength);
        if (!IoCtlSuggested)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Dereference;
        }

        /* And reask */
        KeInitializeEvent(&Event, NotificationEvent, FALSE);
        Irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME,
                                            DeviceObject,
                                            NULL,
                                            0,
                                            IoCtlSuggested,
                                            NameLength,
                                            FALSE,
                                            &Event,
                                            &IoStatusBlock);
        if (!Irp)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Release;
        }

        IoStackLocation = IoGetNextIrpStackLocation(Irp);
        IoStackLocation->FileObject = FileObject;

        Status = IoCallDriver(DeviceObject, Irp);
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event, Executive, KernelMode,
                                  FALSE, NULL);
            Status = IoStatusBlock.Status;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto Release;
    }

    /* Now we have suggested name, copy it */
    SuggestedLinkName->Length = IoCtlSuggested->NameLength;
    SuggestedLinkName->MaximumLength = IoCtlSuggested->NameLength + sizeof(UNICODE_NULL);
    SuggestedLinkName->Buffer = AllocatePool(IoCtlSuggested->NameLength + sizeof(UNICODE_NULL));
    if (!SuggestedLinkName->Buffer)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        RtlCopyMemory(SuggestedLinkName->Buffer, IoCtlSuggested->Name, IoCtlSuggested->NameLength);
        SuggestedLinkName->Buffer[SuggestedLinkName->Length / sizeof(WCHAR)] = UNICODE_NULL;
    }

    /* Also return its priority */
    *UseOnlyIfThereAreNoOtherLinks = IoCtlSuggested->UseOnlyIfThereAreNoOtherLinks;

Release:
    FreePool(IoCtlSuggested);

Dereference:
    ObDereferenceObject(DeviceObject);
    ObDereferenceObject(FileObject);

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
RedirectSavedLink(IN PSAVED_LINK_INFORMATION SavedLinkInformation,
                  IN PUNICODE_STRING DosName,
                  IN PUNICODE_STRING NewLink)
{
    PLIST_ENTRY NextEntry;
    PSYMLINK_INFORMATION SymlinkInformation;

    /* Find the link */
    for (NextEntry = SavedLinkInformation->SymbolicLinksListHead.Flink;
         NextEntry != &(SavedLinkInformation->SymbolicLinksListHead);
         NextEntry = NextEntry->Flink)
    {
        SymlinkInformation = CONTAINING_RECORD(NextEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

        if (!RtlEqualUnicodeString(DosName, &(SymlinkInformation->Name), TRUE))
        {
            /* Delete old link */
            GlobalDeleteSymbolicLink(DosName);
            /* Set its new location */
            GlobalCreateSymbolicLink(DosName, NewLink);

            /* And remove it from the list (not valid any more) */
            RemoveEntryList(&(SymlinkInformation->SymbolicLinksListEntry));
            FreePool(SymlinkInformation->Name.Buffer);
            FreePool(SymlinkInformation);

            return TRUE;
        }
    }

    return FALSE;
}

/*
 * @implemented
 */
VOID
DeleteSymbolicLinkNameFromMemory(IN PDEVICE_EXTENSION DeviceExtension,
                                 IN PUNICODE_STRING SymbolicLink,
                                 IN BOOLEAN MarkOffline)
{
    PLIST_ENTRY DeviceEntry, SymbolEntry;
    PDEVICE_INFORMATION DeviceInformation;
    PSYMLINK_INFORMATION SymlinkInformation;

    /* First of all, ensure we have devices */
    if (IsListEmpty(&(DeviceExtension->DeviceListHead)))
    {
        return;
    }

    /* Then, look for the symbolic name */
    for (DeviceEntry = DeviceExtension->DeviceListHead.Flink;
         DeviceEntry != &(DeviceExtension->DeviceListHead);
         DeviceEntry = DeviceEntry->Flink)
    {
        DeviceInformation = CONTAINING_RECORD(DeviceEntry, DEVICE_INFORMATION, DeviceListEntry);

        for (SymbolEntry = DeviceInformation->SymbolicLinksListHead.Flink;
             SymbolEntry != &(DeviceInformation->SymbolicLinksListHead);
             SymbolEntry = SymbolEntry->Flink)
        {
            SymlinkInformation = CONTAINING_RECORD(SymbolEntry, SYMLINK_INFORMATION, SymbolicLinksListEntry);

            /* One we have found it */
            if (RtlCompareUnicodeString(SymbolicLink, &(SymlinkInformation->Name), TRUE) == 0)
            {
                /* Check if caller just want it to be offline */
                if (MarkOffline)
                {
                    SymlinkInformation->Online = FALSE;
                }
                else
                {
                    /* If not, delete it & notify */
                    SendLinkDeleted(&(DeviceInformation->SymbolicName), SymbolicLink);
                    RemoveEntryList(&(SymlinkInformation->SymbolicLinksListEntry));

                    FreePool(SymlinkInformation->Name.Buffer);
                    FreePool(SymlinkInformation);
                }

                /* No need to go farther */
                return;
            }
        }
    }

    return;
}

/*
 * @implemented
 */
BOOLEAN
IsDriveLetter(PUNICODE_STRING SymbolicName)
{
    WCHAR Letter, Colon;

    /* We must have a precise length */
    if (SymbolicName->Length != DosDevices.Length + 2 * sizeof(WCHAR))
    {
        return FALSE;
    }

    /* Must start with the DosDevices prefix */
    if (!RtlPrefixUnicodeString(&DosDevices, SymbolicName, TRUE))
    {
        return FALSE;
    }

    /* Check if letter is correct */
    Letter = SymbolicName->Buffer[DosDevices.Length / sizeof(WCHAR)];
    if ((Letter < L'A' || Letter > L'Z') && Letter != (WCHAR)-1)
    {
        return FALSE;
    }

    /* And finally it must end with a colon */
    Colon = SymbolicName->Buffer[DosDevices.Length / sizeof(WCHAR) + 1];
    if (Colon != L':')
    {
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
NTSTATUS
MountMgrQuerySymbolicLink(IN PUNICODE_STRING SymbolicName,
                          IN OUT PUNICODE_STRING LinkTarget)
{
    NTSTATUS Status;
    HANDLE LinkHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Open the symbolic link */
    InitializeObjectAttributes(&ObjectAttributes,
                               SymbolicName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwOpenSymbolicLinkObject(&LinkHandle,
                                      GENERIC_READ,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Query its target */
    Status = ZwQuerySymbolicLinkObject(LinkHandle,
                                       LinkTarget,
                                       NULL);

    ZwClose(LinkHandle);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (LinkTarget->Length <= sizeof(WCHAR))
    {
        return Status;
    }

    /* If it's not finished by \, just return */
    if (LinkTarget->Buffer[LinkTarget->Length / sizeof(WCHAR) - 1] != L'\\')
    {
        return Status;
    }

    /* Otherwise, ensure to drop the tailing \ */
    LinkTarget->Length -= sizeof(WCHAR);
    LinkTarget->Buffer[LinkTarget->Length / sizeof(WCHAR)] = UNICODE_NULL;

    return Status;
}
