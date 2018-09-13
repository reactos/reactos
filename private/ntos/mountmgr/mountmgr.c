/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    mountmgr.c

Abstract:

    This driver manages the kernel mode mount table that handles the level
    of indirection between the persistent dos device name for an object and
    the non-persistent nt device name for an object.

Author:

    Norbert Kusters      20-May-1997

Environment:

    kernel mode only

Notes:

Revision History:

--*/

#define _NTDDK_
#define _NTSRV_

#include <ntos.h>
#include <zwapi.h>
#include <ntdddisk.h>
#include <ntddvol.h>
#include <initguid.h>
#include <wdmguid.h>
#include <mountmgr.h>
#include <mountdev.h>
#include <mntmgr.h>
#include <stdio.h>
#include <ioevent.h>

#define MAX_VOLUME_PATH 100

#define IOCTL_MOUNTMGR_QUERY_POINTS_ADMIN           CTL_CODE(MOUNTMGRCONTROLTYPE, 2, METHOD_BUFFERED, FILE_READ_ACCESS)

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

NTSTATUS
UniqueIdChangeNotifyCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           WorkItem
    );

NTSTATUS
MountMgrChangeNotify(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    );

VOID
MountMgrNotify(
    IN  PDEVICE_EXTENSION   Extension
    );

VOID
ReconcileThisDatabaseWithMaster(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    );

NTSTATUS
QueueWorkItem(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    );

NTSTATUS
MountMgrMountedDeviceRemoval(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     NotificationName
    );

typedef struct _RECONCILE_WORK_ITEM_INFO {
    PDEVICE_EXTENSION           Extension;
    PMOUNTED_DEVICE_INFORMATION DeviceInfo;
} RECONCILE_WORK_ITEM_INFO, *PRECONCILE_WORK_ITEM_INFO;

typedef struct _RECONCILE_WORK_ITEM {
    WORK_QUEUE_ITEM             WorkItem;
    RECONCILE_WORK_ITEM_INFO    WorkItemInfo;
} RECONCILE_WORK_ITEM, *PRECONCILE_WORK_ITEM;

#ifdef POOL_TAGGING
#undef ExAllocatePool
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,' tnM')
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif

#ifdef ALLOC_PRAGMA
#pragma code_seg("PAGE")
#endif


NTSTATUS
QueryDeviceInformation(
    IN  PUNICODE_STRING         NotificationName,
    OUT PUNICODE_STRING         DeviceName,
    OUT PMOUNTDEV_UNIQUE_ID*    UniqueId,
    OUT PBOOLEAN                IsRemovable,
    OUT PBOOLEAN                IsRecognized
    )

/*++

Routine Description:

    This routine queries device information.

Arguments:

    NotificationName    - Supplies the notification name.

    DeviceName          - Returns the device name.

    UniqueId            - Returns the unique id.

    IsRemovable         - Returns whether or not the device is removable.

    IsRecognized        - Returns whether or not this is a recognized partition
                            type.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject;
    BOOLEAN                 isRemovable;
    PARTITION_INFORMATION   partInfo;
    KEVENT                  event;
    PIRP                    irp;
    IO_STATUS_BLOCK         ioStatus;
    ULONG                   outputSize;
    PMOUNTDEV_NAME          output;
    PIO_STACK_LOCATION      irpSp;

    status = IoGetDeviceObjectPointer(NotificationName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    if (fileObject->FileName.Length) {
        ObDereferenceObject(fileObject);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    if (fileObject->DeviceObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        isRemovable = TRUE;
    } else {
        isRemovable = FALSE;
    }

    if (IsRemovable) {
        *IsRemovable = isRemovable;
    }

    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    if (IsRecognized) {

        if (isRemovable) {
            *IsRecognized = TRUE;
        } else {

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                                deviceObject, NULL, 0,
                                                &partInfo, sizeof(partInfo),
                                                FALSE, &event, &ioStatus);
            if (!irp) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }

            if (!NT_SUCCESS(status)) {
                status = STATUS_SUCCESS;
                *IsRecognized = TRUE;
            } else if (IsRecognizedPartition(partInfo.PartitionType)) {
                *IsRecognized = TRUE;
            } else {
                *IsRecognized = FALSE;
            }
        }
    }

    if (DeviceName) {

        outputSize = sizeof(MOUNTDEV_NAME);
        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
              IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, deviceObject, NULL, 0, output,
              outputSize, FALSE, &event, &ioStatus);
        if (!irp) {
            ExFreePool(output);
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->FileObject = fileObject;

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) {

            outputSize = sizeof(MOUNTDEV_NAME) + output->NameLength;
            ExFreePool(output);
            output = ExAllocatePool(PagedPool, outputSize);
            if (!output) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                  IOCTL_MOUNTDEV_QUERY_DEVICE_NAME, deviceObject, NULL, 0, output,
                  outputSize, FALSE, &event, &ioStatus);
            if (!irp) {
                ExFreePool(output);
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->FileObject = fileObject;

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
        }

        if (NT_SUCCESS(status)) {

            DeviceName->Length = output->NameLength;
            DeviceName->MaximumLength = output->NameLength + sizeof(WCHAR);
            DeviceName->Buffer = ExAllocatePool(PagedPool,
                                                DeviceName->MaximumLength);
            if (DeviceName->Buffer) {

                RtlCopyMemory(DeviceName->Buffer, output->Name,
                              output->NameLength);
                DeviceName->Buffer[DeviceName->Length/sizeof(WCHAR)] = 0;

            } else {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
        }

        ExFreePool(output);
    }

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return status;
    }

    if (UniqueId) {

        outputSize = sizeof(MOUNTDEV_UNIQUE_ID);
        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
              IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, deviceObject, NULL, 0, output,
              outputSize, FALSE, &event, &ioStatus);
        if (!irp) {
            ExFreePool(output);
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->FileObject = fileObject;

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }

        if (status == STATUS_BUFFER_OVERFLOW) {

            outputSize = sizeof(MOUNTDEV_UNIQUE_ID) +
                         ((PMOUNTDEV_UNIQUE_ID) output)->UniqueIdLength;
            ExFreePool(output);
            output = ExAllocatePool(PagedPool, outputSize);
            if (!output) {
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            KeInitializeEvent(&event, NotificationEvent, FALSE);
            irp = IoBuildDeviceIoControlRequest(
                  IOCTL_MOUNTDEV_QUERY_UNIQUE_ID, deviceObject, NULL, 0, output,
                  outputSize, FALSE, &event, &ioStatus);
            if (!irp) {
                ExFreePool(output);
                ObDereferenceObject(deviceObject);
                ObDereferenceObject(fileObject);
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            irpSp = IoGetNextIrpStackLocation(irp);
            irpSp->FileObject = fileObject;

            status = IoCallDriver(deviceObject, irp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
        }

        if (NT_SUCCESS(status)) {
            *UniqueId = (PMOUNTDEV_UNIQUE_ID) output;
        } else {
            ExFreePool(output);
        }
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);

    return status;
}

NTSTATUS
FindDeviceInfo(
    IN  PDEVICE_EXTENSION               Extension,
    IN  PUNICODE_STRING                 DeviceName,
    IN  BOOLEAN                         IsCanonicalName,
    OUT PMOUNTED_DEVICE_INFORMATION*    DeviceInfo
    )

/*++

Routine Description:

    This routine finds the device information for the given device.

Arguments:

    Extension           - Supplies the device extension.

    DeviceName          - Supplies the name of the device.

    CanonicalizeName    - Supplies whether or not the name given is canonical.

    DeviceInfo          - Returns the device information.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING              targetName;
    NTSTATUS                    status;
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    if (IsCanonicalName) {
        targetName = *DeviceName;
    } else {
        status = QueryDeviceInformation(DeviceName, &targetName, NULL, NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (RtlEqualUnicodeString(&targetName, &deviceInfo->DeviceName,
                                  TRUE)) {
            break;
        }
    }

    if (!IsCanonicalName) {
        ExFreePool(targetName.Buffer);
    }

    if (l == &Extension->MountedDeviceList) {
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    *DeviceInfo = deviceInfo;

    return STATUS_SUCCESS;
}

NTSTATUS
QuerySuggestedLinkName(
    IN  PUNICODE_STRING NotificationName,
    OUT PUNICODE_STRING SuggestedLinkName,
    OUT PBOOLEAN        UseOnlyIfThereAreNoOtherLinks
    )

/*++

Routine Description:

    This routine queries the mounted device for a suggested link name.

Arguments:

    NotificationName                - Supplies the notification name.

    SuggestedLinkName               - Returns the suggested link name.

    UseOnlyIfThereAreNoOtherLinks   - Returns whether or not to use this name
                                        if there are other links to the device.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        status;
    PFILE_OBJECT                    fileObject;
    PDEVICE_OBJECT                  deviceObject;
    ULONG                           outputSize;
    PMOUNTDEV_SUGGESTED_LINK_NAME   output;
    KEVENT                          event;
    PIRP                            irp;
    IO_STATUS_BLOCK                 ioStatus;
    PIO_STACK_LOCATION              irpSp;

    status = IoGetDeviceObjectPointer(NotificationName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME);
    output = ExAllocatePool(PagedPool, outputSize);
    if (!output) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME, deviceObject, NULL, 0,
          output, outputSize, FALSE, &event, &ioStatus);
    if (!irp) {
        ExFreePool(output);
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (status == STATUS_BUFFER_OVERFLOW) {

        outputSize = sizeof(MOUNTDEV_SUGGESTED_LINK_NAME) + output->NameLength;
        ExFreePool(output);
        output = ExAllocatePool(PagedPool, outputSize);
        if (!output) {
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        KeInitializeEvent(&event, NotificationEvent, FALSE);
        irp = IoBuildDeviceIoControlRequest(
              IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME, deviceObject, NULL, 0,
              output, outputSize, FALSE, &event, &ioStatus);
        if (!irp) {
            ExFreePool(output);
            ObDereferenceObject(deviceObject);
            ObDereferenceObject(fileObject);
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        irpSp = IoGetNextIrpStackLocation(irp);
        irpSp->FileObject = fileObject;

        status = IoCallDriver(deviceObject, irp);
        if (status == STATUS_PENDING) {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            status = ioStatus.Status;
        }
    }

    if (NT_SUCCESS(status)) {

        SuggestedLinkName->Length = output->NameLength;
        SuggestedLinkName->MaximumLength = output->NameLength + sizeof(WCHAR);
        SuggestedLinkName->Buffer = ExAllocatePool(PagedPool,
                                                   SuggestedLinkName->MaximumLength);
        if (SuggestedLinkName->Buffer) {

            RtlCopyMemory(SuggestedLinkName->Buffer, output->Name,
                          output->NameLength);
            SuggestedLinkName->Buffer[output->NameLength/sizeof(WCHAR)] = 0;

        } else {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        *UseOnlyIfThereAreNoOtherLinks = output->UseOnlyIfThereAreNoOtherLinks;
    }

    ExFreePool(output);
    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);

    return status;
}

NTSTATUS
SymbolicLinkNamesFromUniqueIdCount(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine counts all of the occurences of the unique id in the
    registry key.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Supplies the num names count.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;

    if (ValueName[0] == '#' ||
        ValueType != REG_BINARY ||
        uniqueId->UniqueIdLength != ValueLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {


        return STATUS_SUCCESS;
    }

    (*((PULONG) EntryContext))++;

    return STATUS_SUCCESS;
}

NTSTATUS
SymbolicLinkNamesFromUniqueIdQuery(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine counts all of the occurences of the unique id in the
    registry key.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Supplies the dos names array.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;
    UNICODE_STRING      string;
    PUNICODE_STRING     p;

    if (ValueName[0] == '#' ||
        ValueType != REG_BINARY ||
        uniqueId->UniqueIdLength != ValueLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&string, ValueName);
    string.Buffer = ExAllocatePool(PagedPool, string.MaximumLength);
    if (!string.Buffer) {
        return STATUS_SUCCESS;
    }
    RtlCopyMemory(string.Buffer, ValueName, string.Length);
    string.Buffer[string.Length/sizeof(WCHAR)] = 0;

    p = (PUNICODE_STRING) EntryContext;
    while (p->Length != 0) {
        p++;
    }

    *p = string;

    return STATUS_SUCCESS;
}

BOOLEAN
IsDriveLetter(
    IN  PUNICODE_STRING SymbolicLinkName
    )

{
    UNICODE_STRING  dosDevices;

    if (SymbolicLinkName->Length == 28 &&
        ((SymbolicLinkName->Buffer[12] >= 'A' &&
          SymbolicLinkName->Buffer[12] <= 'Z') ||
         SymbolicLinkName->Buffer[12] == 0xFF) &&
        SymbolicLinkName->Buffer[13] == ':') {

        RtlInitUnicodeString(&dosDevices, L"\\DosDevices\\");

        SymbolicLinkName->Length = 24;
        if (RtlEqualUnicodeString(SymbolicLinkName, &dosDevices, TRUE)) {
            SymbolicLinkName->Length = 28;
            return TRUE;
        }
        SymbolicLinkName->Length = 28;
    }

    return FALSE;
}

NTSTATUS
QuerySymbolicLinkNamesFromStorage(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo,
    IN  PUNICODE_STRING             SuggestedName,
    IN  BOOLEAN                     UseOnlyIfThereAreNoOtherLinks,
    OUT PUNICODE_STRING*            SymbolicLinkNames,
    OUT PULONG                      NumNames
    )

/*++

Routine Description:

    This routine queries the symbolic link names from storage for
    the given notification name.

Arguments:

    Extension           - Supplies the device extension.

    DeviceInfo          - Supplies the device information.

    SymbolicLinkNames   - Returns the symbolic link names.

    NumNames            - Returns the number of symbolic link names.

Return Value:

    NTSTATUS

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    BOOLEAN                     extraLink;
    NTSTATUS                    status;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].EntryContext = NumNames;

    *NumNames = 0;
    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    MOUNTED_DEVICES_KEY, queryTable,
                                    DeviceInfo->UniqueId, NULL);

    if (!NT_SUCCESS(status)) {
        *NumNames = 0;
    }

    if (SuggestedName && !IsDriveLetter(SuggestedName)) {
        if (UseOnlyIfThereAreNoOtherLinks) {
            if (*NumNames == 0) {
                extraLink = TRUE;
            } else {
                extraLink = FALSE;
            }
        } else {
            extraLink = TRUE;
        }
    } else {
        extraLink = FALSE;
    }

    if (extraLink) {

        RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                              SuggestedName->Buffer, REG_BINARY,
                              DeviceInfo->UniqueId->UniqueId,
                              DeviceInfo->UniqueId->UniqueIdLength);

        RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
        queryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdCount;
        queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
        queryTable[0].EntryContext = NumNames;

        *NumNames = 0;
        status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                        MOUNTED_DEVICES_KEY, queryTable,
                                        DeviceInfo->UniqueId, NULL);

        if (!NT_SUCCESS(status) || *NumNames == 0) {
            return STATUS_NOT_FOUND;
        }

    } else if (!*NumNames) {
        return STATUS_NOT_FOUND;
    }

    *SymbolicLinkNames = ExAllocatePool(PagedPool,
                                        *NumNames*sizeof(UNICODE_STRING));
    if (!*SymbolicLinkNames) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(*SymbolicLinkNames, *NumNames*sizeof(UNICODE_STRING));

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = SymbolicLinkNamesFromUniqueIdQuery;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].EntryContext = *SymbolicLinkNames;

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    MOUNTED_DEVICES_KEY, queryTable,
                                    DeviceInfo->UniqueId, NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
ChangeUniqueIdRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine replaces all old unique ids with new unique ids.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the old unique id.

    EntryContext    - Supplies the new unique id.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID oldId = Context;
    PMOUNTDEV_UNIQUE_ID newId = EntryContext;

    if (ValueType != REG_BINARY || oldId->UniqueIdLength != ValueLength ||
        RtlCompareMemory(oldId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                          ValueName, ValueType, newId->UniqueId,
                          newId->UniqueIdLength);

    return STATUS_SUCCESS;
}

HANDLE
OpenRemoteDatabase(
    IN  PUNICODE_STRING RemoteDatabaseVolumeName,
    IN  BOOLEAN         Create
    )

/*++

Routine Description:

    This routine opens the remote database on the given volume.

Arguments:

    RemoteDatabaseVolumeName    - Supplies the remote database volume name.

    Create                      - Supplies whether or not to create.

Return Value:

    A handle to the remote database or NULL.

--*/

{
    UNICODE_STRING      suffix;
    UNICODE_STRING      fileName;
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              h;
    IO_STATUS_BLOCK     ioStatus;

    RtlInitUnicodeString(&suffix, L"\\:$MountMgrRemoteDatabase");

    fileName.Length = RemoteDatabaseVolumeName->Length +
                      suffix.Length;
    fileName.MaximumLength = fileName.Length + sizeof(WCHAR);
    fileName.Buffer = ExAllocatePool(PagedPool, fileName.MaximumLength);
    if (!fileName.Buffer) {
        return NULL;
    }

    RtlCopyMemory(fileName.Buffer, RemoteDatabaseVolumeName->Buffer,
                  RemoteDatabaseVolumeName->Length);
    RtlCopyMemory((PCHAR) fileName.Buffer + RemoteDatabaseVolumeName->Length,
                  suffix.Buffer, suffix.Length);
    fileName.Buffer[fileName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &fileName, OBJ_CASE_INSENSITIVE, 0, 0);

    status = ZwCreateFile(&h, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &oa,
                          &ioStatus, NULL, FILE_ATTRIBUTE_NORMAL |
                          FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, 0,
                          Create ? FILE_OPEN_IF : FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_ALERT | FILE_NON_DIRECTORY_FILE,
                          NULL, 0);

    ExFreePool(fileName.Buffer);

    if (!NT_SUCCESS(status)) {
        return NULL;
    }

    return h;
}

ULONG
GetRemoteDatabaseSize(
    IN  HANDLE  RemoteDatabaseHandle
    )

/*++

Routine Description:

    This routine returns the length of the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

Return Value:

    The length of the remote database or 0.

--*/

{
    NTSTATUS                    status;
    IO_STATUS_BLOCK             ioStatus;
    FILE_STANDARD_INFORMATION   info;

    status = ZwQueryInformationFile(RemoteDatabaseHandle, &ioStatus, &info,
                                    sizeof(info), FileStandardInformation);
    if (!NT_SUCCESS(status)) {
        return 0;
    }

    return info.EndOfFile.LowPart;
}

VOID
CloseRemoteDatabase(
    IN  HANDLE  RemoteDatabaseHandle
    )

/*++

Routine Description:

    This routine closes the given remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

Return Value:

    None.

--*/

{
    ULONG                           fileLength;
    FILE_DISPOSITION_INFORMATION    disp;
    IO_STATUS_BLOCK                 ioStatus;

    fileLength = GetRemoteDatabaseSize(RemoteDatabaseHandle);
    if (!fileLength) {
        disp.DeleteFile = TRUE;
        ZwSetInformationFile(RemoteDatabaseHandle, &ioStatus, &disp,
                             sizeof(disp), FileDispositionInformation);
    }

    ZwClose(RemoteDatabaseHandle);
}

NTSTATUS
TruncateRemoteDatabase(
    IN  HANDLE  RemoteDatabaseHandle,
    IN  ULONG   FileOffset
    )

/*++

Routine Description:

    This routine truncates the remote database at the given file offset.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

Return Value:

    NTSTATUS

--*/

{
    FILE_END_OF_FILE_INFORMATION    endOfFileInfo;
    FILE_ALLOCATION_INFORMATION     allocationInfo;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;

    endOfFileInfo.EndOfFile.QuadPart = FileOffset;
    allocationInfo.AllocationSize.QuadPart = FileOffset;

    status = ZwSetInformationFile(RemoteDatabaseHandle, &ioStatus,
                                  &endOfFileInfo, sizeof(endOfFileInfo),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = ZwSetInformationFile(RemoteDatabaseHandle, &ioStatus,
                                  &allocationInfo, sizeof(allocationInfo),
                                  FileAllocationInformation);

    return status;
}

PMOUNTMGR_FILE_ENTRY
GetRemoteDatabaseEntry(
    IN  HANDLE  RemoteDatabaseHandle,
    IN  ULONG   FileOffset
    )

/*++

Routine Description:

    This routine gets the next database entry.  This routine fixes
    corruption as it finds it.  The memory returned from this routine
    must be freed with ExFreePool.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

Return Value:

    A pointer to the next remote database entry.

--*/

{
    LARGE_INTEGER           offset;
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;
    ULONG                   size;
    PMOUNTMGR_FILE_ENTRY    entry;
    ULONG                   len1, len2, len;

    offset.QuadPart = FileOffset;
    status = ZwReadFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                        &size, sizeof(size), &offset, NULL);
    if (!NT_SUCCESS(status)) {
        return NULL;
    }
    if (!size) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        return NULL;
    }

    entry = ExAllocatePool(PagedPool, size);
    if (!entry) {
        return NULL;
    }

    status = ZwReadFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                        entry, size, &offset, NULL);
    if (!NT_SUCCESS(status)) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    if (ioStatus.Information < size) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    if (size < sizeof(MOUNTMGR_FILE_ENTRY)) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    len1 = entry->VolumeNameOffset + entry->VolumeNameLength;
    len2 = entry->UniqueIdOffset + entry->UniqueIdLength;
    len = len1 > len2 ? len1 : len2;

    if (len > size) {
        TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
        ExFreePool(entry);
        return NULL;
    }

    return entry;
}

NTSTATUS
WriteRemoteDatabaseEntry(
    IN  HANDLE                  RemoteDatabaseHandle,
    IN  ULONG                   FileOffset,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine write the given database entry at the given file offset
    to the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

    DatabaseEntry           - Supplies the database entry.

Return Value:

    NTSTATUS

--*/

{
    LARGE_INTEGER   offset;
    NTSTATUS        status;
    IO_STATUS_BLOCK ioStatus;

    offset.QuadPart = FileOffset;
    status = ZwWriteFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                         DatabaseEntry, DatabaseEntry->EntryLength,
                         &offset, NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (ioStatus.Information < DatabaseEntry->EntryLength) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return status;
}

NTSTATUS
DeleteRemoteDatabaseEntry(
    IN  HANDLE  RemoteDatabaseHandle,
    IN  ULONG   FileOffset
    )

/*++

Routine Description:

    This routine deletes the database entry at the given file offset
    in the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    FileOffset              - Supplies the file offset.

Return Value:

    NTSTATUS

--*/

{
    ULONG                   fileSize;
    PMOUNTMGR_FILE_ENTRY    entry;
    LARGE_INTEGER           offset;
    ULONG                   size;
    PVOID                   buffer;
    NTSTATUS                status;
    IO_STATUS_BLOCK         ioStatus;

    fileSize = GetRemoteDatabaseSize(RemoteDatabaseHandle);
    if (!fileSize) {
        return STATUS_INVALID_PARAMETER;
    }

    entry = GetRemoteDatabaseEntry(RemoteDatabaseHandle, FileOffset);
    if (!entry) {
        return STATUS_INVALID_PARAMETER;
    }

    if (FileOffset + entry->EntryLength >= fileSize) {
        ExFreePool(entry);
        return TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
    }

    size = fileSize - FileOffset - entry->EntryLength;
    buffer = ExAllocatePool(PagedPool, size);
    if (!buffer) {
        ExFreePool(entry);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    offset.QuadPart = FileOffset + entry->EntryLength;
    ExFreePool(entry);

    status = ZwReadFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                        buffer, size, &offset, NULL);
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return status;
    }

    if (ioStatus.Information < size) {
        ExFreePool(buffer);
        return STATUS_INVALID_PARAMETER;
    }

    status = TruncateRemoteDatabase(RemoteDatabaseHandle, FileOffset);
    if (!NT_SUCCESS(status)) {
        ExFreePool(buffer);
        return status;
    }

    offset.QuadPart = FileOffset;
    status = ZwWriteFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                         buffer, size, &offset, NULL);

    ExFreePool(buffer);

    return status;
}

NTSTATUS
AddRemoteDatabaseEntry(
    IN  HANDLE                  RemoteDatabaseHandle,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine adds a new database entry to the remote database.

Arguments:

    RemoteDatabaseHandle    - Supplies a handle to the remote database.

    DatabaseEntry           - Supplies the database entry.

Return Value:

    NTSTATUS

--*/

{
    ULONG           fileSize;
    LARGE_INTEGER   offset;
    NTSTATUS        status;
    IO_STATUS_BLOCK ioStatus;

    fileSize = GetRemoteDatabaseSize(RemoteDatabaseHandle);
    offset.QuadPart = fileSize;
    status = ZwWriteFile(RemoteDatabaseHandle, NULL, NULL, NULL, &ioStatus,
                         DatabaseEntry, DatabaseEntry->EntryLength, &offset,
                         NULL);

    return status;
}

VOID
ChangeRemoteDatabaseUniqueId(
    IN  PUNICODE_STRING     RemoteDatabaseVolumeName,
    IN  PMOUNTDEV_UNIQUE_ID OldUniqueId,
    IN  PMOUNTDEV_UNIQUE_ID NewUniqueId
    )

/*++

Routine Description:

    This routine changes the unique id in the remote database.

Arguments:

    RemoteDatabaseVolumeName    - Supplies the remote database volume name.

    OldUniqueId                 - Supplies the old unique id.

    NewUniqueId                 - Supplies the new unique id.

Return Value:

    None.

--*/

{
    HANDLE                  h;
    ULONG                   offset, newSize;
    PMOUNTMGR_FILE_ENTRY    databaseEntry, newDatabaseEntry;
    NTSTATUS                status;

    h = OpenRemoteDatabase(RemoteDatabaseVolumeName, FALSE);
    if (!h) {
        return;
    }

    offset = 0;
    for (;;) {

        databaseEntry = GetRemoteDatabaseEntry(h, offset);
        if (!databaseEntry) {
            break;
        }

        if (databaseEntry->UniqueIdLength != OldUniqueId->UniqueIdLength ||
            RtlCompareMemory(OldUniqueId->UniqueId,
                             (PCHAR) databaseEntry +
                             databaseEntry->UniqueIdOffset,
                             databaseEntry->UniqueIdLength) !=
                             databaseEntry->UniqueIdLength) {

            offset += databaseEntry->EntryLength;
            ExFreePool(databaseEntry);
            continue;
        }

        newSize = databaseEntry->EntryLength + NewUniqueId->UniqueIdLength -
                  OldUniqueId->UniqueIdLength;

        newDatabaseEntry = ExAllocatePool(PagedPool, newSize);
        if (!newDatabaseEntry) {
            offset += databaseEntry->EntryLength;
            ExFreePool(databaseEntry);
            continue;
        }

        newDatabaseEntry->EntryLength = newSize;
        newDatabaseEntry->RefCount = databaseEntry->RefCount;
        newDatabaseEntry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
        newDatabaseEntry->VolumeNameLength = databaseEntry->VolumeNameLength;
        newDatabaseEntry->UniqueIdOffset = newDatabaseEntry->VolumeNameOffset +
                                           newDatabaseEntry->VolumeNameLength;
        newDatabaseEntry->UniqueIdLength = NewUniqueId->UniqueIdLength;

        RtlCopyMemory((PCHAR) newDatabaseEntry +
                      newDatabaseEntry->VolumeNameOffset,
                      (PCHAR) databaseEntry + databaseEntry->VolumeNameOffset,
                      newDatabaseEntry->VolumeNameLength);
        RtlCopyMemory((PCHAR) newDatabaseEntry +
                      newDatabaseEntry->UniqueIdOffset,
                      NewUniqueId->UniqueId, newDatabaseEntry->UniqueIdLength);

        status = DeleteRemoteDatabaseEntry(h, offset);
        if (!NT_SUCCESS(status)) {
            ExFreePool(databaseEntry);
            ExFreePool(newDatabaseEntry);
            break;
        }

        status = AddRemoteDatabaseEntry(h, newDatabaseEntry);
        if (!NT_SUCCESS(status)) {
            ExFreePool(databaseEntry);
            ExFreePool(newDatabaseEntry);
            break;
        }

        ExFreePool(newDatabaseEntry);
        ExFreePool(databaseEntry);
    }

    CloseRemoteDatabase(h);
}

NTSTATUS
WaitForRemoteDatabaseSemaphore(
    IN  PDEVICE_EXTENSION   Extension
    )

{
    LARGE_INTEGER   timeout;
    NTSTATUS        status;

    timeout.QuadPart = -10*1000*1000*10;
    status = KeWaitForSingleObject(&Extension->RemoteDatabaseSemaphore,
                                   Executive, KernelMode, FALSE, &timeout);
    if (status == STATUS_TIMEOUT) {
        status = STATUS_IO_TIMEOUT;
    }

    return status;
}

VOID
ReleaseRemoteDatabaseSemaphore(
    IN  PDEVICE_EXTENSION   Extension
    )

{
    KeReleaseSemaphore(&Extension->RemoteDatabaseSemaphore, IO_NO_INCREMENT,
                       1, FALSE);
}

VOID
MountMgrUniqueIdChangeRoutine(
    IN  PVOID               Context,
    IN  PMOUNTDEV_UNIQUE_ID OldUniqueId,
    IN  PMOUNTDEV_UNIQUE_ID NewUniqueId
    )

/*++

Routine Description:

    This routine is called from a mounted device to notify of a unique
    id change.

Arguments:

    MountedDevice                   - Supplies the mounted device.

    MountMgrUniqueIdChangeRoutine   - Supplies the id change routine.

    Context                         - Supplies the context for this routine.

Return Value:

    None.

--*/

{
    NTSTATUS                    status;
    PDEVICE_EXTENSION           extension = Context;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PREPLICATED_UNIQUE_ID       replUniqueId;
    PVOID                       p;
    BOOLEAN                     changedIds;

    status = WaitForRemoteDatabaseSemaphore(extension);

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = ChangeUniqueIdRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].EntryContext = NewUniqueId;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, OldUniqueId, NULL);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);
        if (OldUniqueId->UniqueIdLength !=
            deviceInfo->UniqueId->UniqueIdLength) {

            continue;
        }

        if (RtlCompareMemory(OldUniqueId->UniqueId,
                             deviceInfo->UniqueId->UniqueId,
                             OldUniqueId->UniqueIdLength) !=
                             OldUniqueId->UniqueIdLength) {

            continue;
        }

        break;
    }

    if (l == &extension->MountedDeviceList) {
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        if (NT_SUCCESS(status)) {
            ReleaseRemoteDatabaseSemaphore(extension);
        }
        return;
    }

    if (!NT_SUCCESS(status)) {
        ReconcileThisDatabaseWithMaster(extension, deviceInfo);
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        return;
    }

    p = ExAllocatePool(PagedPool, NewUniqueId->UniqueIdLength +
                       sizeof(MOUNTDEV_UNIQUE_ID));
    if (!p) {
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(extension);
        return;
    }
    ExFreePool(deviceInfo->UniqueId);
    deviceInfo->UniqueId = p;

    deviceInfo->UniqueId->UniqueIdLength = NewUniqueId->UniqueIdLength;
    RtlCopyMemory(deviceInfo->UniqueId->UniqueId,
                  NewUniqueId->UniqueId, NewUniqueId->UniqueIdLength);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        changedIds = FALSE;
        for (ll = deviceInfo->ReplicatedUniqueIds.Flink;
             ll != &deviceInfo->ReplicatedUniqueIds; ll = ll->Flink) {

            replUniqueId = CONTAINING_RECORD(ll, REPLICATED_UNIQUE_ID,
                                             ListEntry);

            if (replUniqueId->UniqueId->UniqueIdLength !=
                OldUniqueId->UniqueIdLength) {

                continue;
            }

            if (RtlCompareMemory(replUniqueId->UniqueId->UniqueId,
                                 OldUniqueId->UniqueId,
                                 OldUniqueId->UniqueIdLength) !=
                                 OldUniqueId->UniqueIdLength) {

                continue;
            }

            p = ExAllocatePool(PagedPool, NewUniqueId->UniqueIdLength +
                               sizeof(MOUNTDEV_UNIQUE_ID));
            if (!p) {
                continue;
            }

            changedIds = TRUE;

            ExFreePool(replUniqueId->UniqueId);
            replUniqueId->UniqueId = p;

            replUniqueId->UniqueId->UniqueIdLength =
                    NewUniqueId->UniqueIdLength;
            RtlCopyMemory(replUniqueId->UniqueId->UniqueId,
                          NewUniqueId->UniqueId, NewUniqueId->UniqueIdLength);
        }

        if (changedIds) {
            ChangeRemoteDatabaseUniqueId(&deviceInfo->DeviceName, OldUniqueId,
                                         NewUniqueId);
        }
    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
    ReleaseRemoteDatabaseSemaphore(extension);
}

VOID
SendLinkCreated(
    IN  PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine alerts the mounted device that one of its links has
    been created

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name being deleted.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    ULONG               inputSize;
    PMOUNTDEV_NAME      input;
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(SymbolicLinkName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    inputSize = sizeof(USHORT) + SymbolicLinkName->Length;
    input = ExAllocatePool(PagedPool, inputSize);
    if (!input) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }

    input->NameLength = SymbolicLinkName->Length;
    RtlCopyMemory(input->Name, SymbolicLinkName->Buffer,
                  SymbolicLinkName->Length);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_MOUNTDEV_LINK_CREATED, deviceObject, input, inputSize, NULL,
          0, FALSE, &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);
}

NTSTATUS
CreateNewVolumeName(
    OUT PUNICODE_STRING VolumeName
    )

/*++

Routine Description:

    This routine creates a new name of the form \??\Volume{GUID}.

Arguments:

    VolumeName  - Returns the volume name.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS        status;
    UUID            uuid;
    UNICODE_STRING  guidString, prefix;

    status = ExUuidCreate(&uuid);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlStringFromGUID(&uuid, &guidString);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    VolumeName->MaximumLength = 98;
    VolumeName->Buffer = ExAllocatePool(PagedPool, VolumeName->MaximumLength);
    if (!VolumeName->Buffer) {
        ExFreePool(guidString.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitUnicodeString(&prefix, L"\\??\\Volume");
    RtlCopyUnicodeString(VolumeName, &prefix);
    RtlAppendUnicodeStringToString(VolumeName, &guidString);
    VolumeName->Buffer[VolumeName->Length/sizeof(WCHAR)] = 0;

    ExFreePool(guidString.Buffer);

    return STATUS_SUCCESS;
}

VOID
CreateNoDriveLetterEntry(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine creates a "no drive letter" entry for the given device.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    UUID                uuid;
    UNICODE_STRING      guidString;
    PWSTR               valueName;

    status = ExUuidCreate(&uuid);
    if (!NT_SUCCESS(status)) {
        return;
    }

    status = RtlStringFromGUID(&uuid, &guidString);
    if (!NT_SUCCESS(status)) {
        return;
    }

    valueName = ExAllocatePool(PagedPool, guidString.Length + 2*sizeof(WCHAR));
    if (!valueName) {
        ExFreePool(guidString.Buffer);
        return;
    }

    valueName[0] = '#';
    RtlCopyMemory(&valueName[1], guidString.Buffer, guidString.Length);
    valueName[1 + guidString.Length/sizeof(WCHAR)] = 0;
    ExFreePool(guidString.Buffer);

    RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                          valueName, REG_BINARY, UniqueId->UniqueId,
                          UniqueId->UniqueIdLength);

    ExFreePool(valueName);
}

NTSTATUS
CreateNewDriveLetterName(
    OUT PUNICODE_STRING     DriveLetterName,
    IN  PUNICODE_STRING     TargetName,
    IN  UCHAR               SuggestedDriveLetter,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine creates a new name of the form \DosDevices\D:.

Arguments:

    DriveLetterName         - Returns the drive letter name.

    TargetName              - Supplies the target object.

    SuggestedDriveLetter    - Supplies the suggested drive letter.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    UNICODE_STRING          prefix, floppyPrefix, cdromPrefix;
    UCHAR                   driveLetter;

    DriveLetterName->MaximumLength = 30;
    DriveLetterName->Buffer = ExAllocatePool(PagedPool,
                                             DriveLetterName->MaximumLength);
    if (!DriveLetterName->Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlInitUnicodeString(&prefix, L"\\DosDevices\\");
    RtlCopyUnicodeString(DriveLetterName, &prefix);

    DriveLetterName->Length = 28;
    DriveLetterName->Buffer[14] = 0;
    DriveLetterName->Buffer[13] = ':';

    if (SuggestedDriveLetter == 0xFF) {
        CreateNoDriveLetterEntry(UniqueId);
        ExFreePool(DriveLetterName->Buffer);
        return STATUS_UNSUCCESSFUL;
    } else if (SuggestedDriveLetter) {
        DriveLetterName->Buffer[12] = SuggestedDriveLetter;
        status = IoCreateSymbolicLink(DriveLetterName, TargetName);
        if (NT_SUCCESS(status)) {
            return status;
        }
    }

    RtlInitUnicodeString(&floppyPrefix, L"\\Device\\Floppy");
    RtlInitUnicodeString(&cdromPrefix, L"\\Device\\CdRom");
    if (RtlPrefixUnicodeString(&floppyPrefix, TargetName, TRUE)) {
        driveLetter = 'A';
    } else if (RtlPrefixUnicodeString(&cdromPrefix, TargetName, TRUE)) {
        driveLetter = 'D';
    } else {
        driveLetter = 'C';
    }

    for (; driveLetter <= 'Z'; driveLetter++) {
        DriveLetterName->Buffer[12] = driveLetter;

        status = IoCreateSymbolicLink(DriveLetterName, TargetName);
        if (NT_SUCCESS(status)) {
            return status;
        }
    }

    ExFreePool(DriveLetterName->Buffer);

    return status;
}

NTSTATUS
CheckForNoDriveLetterEntry(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine checks for the presence of the "no drive letter" entry.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Returns whether or not there is a "no drive letter" entry.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;

    if (ValueName[0] != '#' || ValueType != REG_BINARY ||
        ValueLength != uniqueId->UniqueIdLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    *((PBOOLEAN) EntryContext) = TRUE;

    return STATUS_SUCCESS;
}

BOOLEAN
HasNoDriveLetterEntry(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine determines whether or not the given device has an
    entry indicating that it should not receice a drive letter.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    FALSE   - The device does not have a "no drive letter" entry.

    TRUE    - The device has a "no drive letter" entry.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    BOOLEAN                     hasNoDriveLetterEntry;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = CheckForNoDriveLetterEntry;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].EntryContext = &hasNoDriveLetterEntry;

    hasNoDriveLetterEntry = FALSE;
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);

    return hasNoDriveLetterEntry;
}

typedef struct _CHANGE_NOTIFY_WORK_ITEM {
    WORK_QUEUE_ITEM     WorkItem;
    PDEVICE_EXTENSION   Extension;
    CCHAR               StackSize;
    PIRP                Irp;
    UNICODE_STRING      DeviceName;
    PVOID               SystemBuffer;
    ULONG               OutputSize;
} CHANGE_NOTIFY_WORK_ITEM, *PCHANGE_NOTIFY_WORK_ITEM;

VOID
IssueUniqueIdChangeNotifyWorker(
    IN  PCHANGE_NOTIFY_WORK_ITEM    WorkItem,
    IN  PMOUNTDEV_UNIQUE_ID         UniqueId
    )

/*++

Routine Description:

    This routine issues a change notify request to the given mounted device.

Arguments:

    WorkItem    - Supplies the work item.

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    PIRP                irp;
    ULONG               inputSize;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(&WorkItem->DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        IoFreeIrp(WorkItem->Irp);
        ExFreePool(WorkItem->DeviceName.Buffer);
        ExFreePool(WorkItem->SystemBuffer);
        ExFreePool(WorkItem);
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    ObDereferenceObject(fileObject);

    irp = WorkItem->Irp;
    IoInitializeIrp(irp, IoSizeOfIrp(WorkItem->StackSize),
                    WorkItem->StackSize);

    irp->AssociatedIrp.SystemBuffer = WorkItem->SystemBuffer;
    irp->Tail.Overlay.Thread = PsGetCurrentThread();

    inputSize = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) +
                UniqueId->UniqueIdLength;

    RtlCopyMemory(irp->AssociatedIrp.SystemBuffer, UniqueId, inputSize);

    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->Parameters.DeviceIoControl.InputBufferLength = inputSize;
    irpSp->Parameters.DeviceIoControl.OutputBufferLength = WorkItem->OutputSize;
    irpSp->Parameters.DeviceIoControl.IoControlCode =
            IOCTL_MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY;
    irpSp->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
    irpSp->MajorFunction = IRP_MJ_DEVICE_CONTROL;
    irpSp->DeviceObject = deviceObject;

    IoSetCompletionRoutine(irp, UniqueIdChangeNotifyCompletion,
                           WorkItem, TRUE, TRUE, TRUE);

    IoCallDriver(deviceObject, irp);

    ObDereferenceObject(deviceObject);
}

VOID
UniqueIdChangeNotifyWorker(
    IN  PVOID   WorkItem
    )

/*++

Routine Description:

    This routine updates the unique id in the database with the new version.

Arguments:

    WorkItem    - Supplies the work item.

Return Value:

    None.

--*/

{
    PCHANGE_NOTIFY_WORK_ITEM                    workItem = WorkItem;
    PMOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT    output;
    PMOUNTDEV_UNIQUE_ID                         oldUniqueId, newUniqueId;

    output = workItem->Irp->AssociatedIrp.SystemBuffer;

    oldUniqueId = ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                                 output->OldUniqueIdLength);
    if (!oldUniqueId) {
        ExFreePool(output);
        IoFreeIrp(workItem->Irp);
        ExFreePool(workItem->DeviceName.Buffer);
        ExFreePool(workItem);
        return;
    }

    oldUniqueId->UniqueIdLength = output->OldUniqueIdLength;
    RtlCopyMemory(oldUniqueId->UniqueId, (PCHAR) output +
                  output->OldUniqueIdOffset, oldUniqueId->UniqueIdLength);

    newUniqueId = ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                                 output->NewUniqueIdLength);
    if (!newUniqueId) {
        ExFreePool(oldUniqueId);
        ExFreePool(output);
        IoFreeIrp(workItem->Irp);
        ExFreePool(workItem->DeviceName.Buffer);
        ExFreePool(workItem);
        return;
    }

    newUniqueId->UniqueIdLength = output->NewUniqueIdLength;
    RtlCopyMemory(newUniqueId->UniqueId, (PCHAR) output +
                  output->NewUniqueIdOffset, newUniqueId->UniqueIdLength);

    MountMgrUniqueIdChangeRoutine(workItem->Extension, oldUniqueId,
                                  newUniqueId);

    IssueUniqueIdChangeNotifyWorker(workItem, newUniqueId);

    ExFreePool(newUniqueId);
    ExFreePool(oldUniqueId);
}

VOID
IssueUniqueIdChangeNotify(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     DeviceName,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine issues a change notify request to the given mounted device.

Arguments:

    Extension   - Supplies the device extension.

    DeviceName  - Supplies a name for the device.

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    NTSTATUS                    status;
    PFILE_OBJECT                fileObject;
    PDEVICE_OBJECT              deviceObject;
    PCHANGE_NOTIFY_WORK_ITEM    workItem;
    ULONG                       outputSize;
    PVOID                       output;
    PIRP                        irp;
    PIO_STACK_LOCATION          irpSp;

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);
    ObDereferenceObject(fileObject);

    workItem = ExAllocatePool(NonPagedPool, sizeof(CHANGE_NOTIFY_WORK_ITEM));
    if (!workItem) {
        ObDereferenceObject(deviceObject);
        return;
    }

    workItem->Extension = Extension;
    workItem->StackSize = deviceObject->StackSize;
    workItem->Irp = IoAllocateIrp(deviceObject->StackSize, FALSE);
    ObDereferenceObject(deviceObject);
    if (!workItem->Irp) {
        ExFreePool(workItem);
        return;
    }

    outputSize = sizeof(MOUNTDEV_UNIQUE_ID_CHANGE_NOTIFY_OUTPUT) + 1024;
    output = ExAllocatePool(NonPagedPool, outputSize);
    if (!output) {
        IoFreeIrp(workItem->Irp);
        ExFreePool(workItem);
        return;
    }

    workItem->DeviceName.Length = DeviceName->Length;
    workItem->DeviceName.MaximumLength = workItem->DeviceName.Length +
                                         sizeof(WCHAR);
    workItem->DeviceName.Buffer = ExAllocatePool(NonPagedPool,
                                  workItem->DeviceName.MaximumLength);
    if (!workItem->DeviceName.Buffer) {
        ExFreePool(output);
        IoFreeIrp(workItem->Irp);
        ExFreePool(workItem);
        return;
    }

    RtlCopyMemory(workItem->DeviceName.Buffer, DeviceName->Buffer,
                  DeviceName->Length);
    workItem->DeviceName.Buffer[DeviceName->Length/sizeof(WCHAR)] = 0;

    workItem->SystemBuffer = output;
    workItem->OutputSize = outputSize;

    IssueUniqueIdChangeNotifyWorker(workItem, UniqueId);
}

BOOLEAN
QueryVolumeName(
    IN      HANDLE          Handle,
    IN      PLONGLONG       FileReference,
    IN OUT  PUNICODE_STRING VolumeName
    )

/*++

Routine Description:

    This routine returns the volume name contained in the reparse point
    at FileReference.

Arguments:

    Handle          - Supplies a handle to the volume containing the file
                      reference.

    FileReference   - Supplies the file reference.

    VolumeName      - Returns the volume name.

Return Value:

    FALSE   - Failure.

    TRUE    - Success.

--*/

{
    UNICODE_STRING          fileId;
    OBJECT_ATTRIBUTES       oa;
    NTSTATUS                status;
    HANDLE                  h;
    PREPARSE_DATA_BUFFER    reparse;
    IO_STATUS_BLOCK         ioStatus;

    fileId.Length = sizeof(LONGLONG);
    fileId.MaximumLength = fileId.Length;
    fileId.Buffer = (PWSTR) FileReference;

    InitializeObjectAttributes(&oa, &fileId, 0, Handle, NULL);

    status = ZwOpenFile(&h, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    reparse = ExAllocatePool(PagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!reparse) {
        ZwClose(h);
        return FALSE;
    }

    status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                             FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                             MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    ZwClose(h);
    if (!NT_SUCCESS(status)) {
        ExFreePool(reparse);
        return FALSE;
    }

    if (reparse->MountPointReparseBuffer.SubstituteNameLength + sizeof(WCHAR) >
        VolumeName->MaximumLength) {

        ExFreePool(reparse);
        return FALSE;
    }

    VolumeName->Length = reparse->MountPointReparseBuffer.SubstituteNameLength;
    RtlCopyMemory(VolumeName->Buffer,
                  (PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                  reparse->MountPointReparseBuffer.SubstituteNameOffset,
                  VolumeName->Length);

    ExFreePool(reparse);

    if (VolumeName->Buffer[VolumeName->Length/sizeof(WCHAR) - 1] != '\\') {
        return FALSE;
    }

    VolumeName->Length -= sizeof(WCHAR);
    VolumeName->Buffer[VolumeName->Length/sizeof(WCHAR)] = 0;

    if (!MOUNTMGR_IS_VOLUME_NAME(VolumeName)) {
        return FALSE;
    }

    return TRUE;
}

NTSTATUS
QueryUniqueIdQueryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine queries the unique id for the given value.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Returns the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId;

    if (ValueLength >= 0x10000) {
        return STATUS_SUCCESS;
    }

    uniqueId = ExAllocatePool(PagedPool, sizeof(MOUNTDEV_UNIQUE_ID) +
                              ValueLength);
    if (!uniqueId) {
        return STATUS_SUCCESS;
    }

    uniqueId->UniqueIdLength = (USHORT) ValueLength;
    RtlCopyMemory(uniqueId->UniqueId, ValueData, ValueLength);

    *((PMOUNTDEV_UNIQUE_ID*) Context) = uniqueId;

    return STATUS_SUCCESS;
}

NTSTATUS
QueryUniqueIdFromMaster(
    IN  PUNICODE_STRING         VolumeName,
    OUT PMOUNTDEV_UNIQUE_ID*    UniqueId
    )

/*++

Routine Description:

    This routine queries the unique id from the master database.

Arguments:

    VolumeName  - Supplies the volume name.

    UniqueId    - Returns the unique id.

Return Value:

    NTSTATUS

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = QueryUniqueIdQueryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = VolumeName->Buffer;

    *UniqueId = NULL;
    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);

    if (!(*UniqueId)) {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeleteDriveLetterRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine deletes the "no drive letter" entry.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;
    UNICODE_STRING      string;

    if (ValueType != REG_BINARY ||
        ValueLength != uniqueId->UniqueIdLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&string, ValueName);
    if (IsDriveLetter(&string)) {
        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               ValueName);
    }

    return STATUS_SUCCESS;
}

VOID
DeleteRegistryDriveLetter(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine checks the current database to see if the given unique
    id already has a drive letter.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    FALSE   - The given unique id does not already have a drive letter.

    TRUE    - The given unique id already has a drive letter.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = DeleteDriveLetterRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);
}

BOOLEAN
HasDriveLetter(
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    )

/*++

Routine Description:

    This routine computes whether or not the given device has a drive letter.

Arguments:

    DeviceInfo  - Supplies the device information.

Return Value:

    FALSE   - This device does not have a drive letter.

    TRUE    - This device does have a drive letter.

--*/

{
    PLIST_ENTRY                 l;
    PSYMBOLIC_LINK_NAME_ENTRY   symEntry;

    for (l = DeviceInfo->SymbolicLinkNames.Flink;
         l != &DeviceInfo->SymbolicLinkNames; l = l->Flink) {

        symEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY, ListEntry);
        if (symEntry->IsInDatabase &&
            IsDriveLetter(&symEntry->SymbolicLinkName)) {

            return TRUE;
        }
    }

    return FALSE;
}

NTSTATUS
DeleteNoDriveLetterEntryRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine deletes the "no drive letter" entry.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;

    if (ValueName[0] != '#' || ValueType != REG_BINARY ||
        ValueLength != uniqueId->UniqueIdLength ||
        RtlCompareMemory(uniqueId->UniqueId, ValueData, ValueLength) !=
        ValueLength) {

        return STATUS_SUCCESS;
    }

    RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           ValueName);

    return STATUS_SUCCESS;
}

VOID
DeleteNoDriveLetterEntry(
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine deletes the "no drive letter" entry for the given device.

Arguments:

    UniqueId    - Supplies the unique id.

Return Value:

    None.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = DeleteNoDriveLetterEntryRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);
}

VOID
MountMgrNotifyNameChange(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     DeviceName,
    IN  BOOLEAN             CheckForPdo
    )

/*++

Routine Description:

    This routine performs a target notification on 'DeviceName' to alert
    of a name change on the device.

Arguments:

    Extension   - Supplies the device extension.

    DeviceName  - Supplies the device name.

    CheckForPdo - Supplies whether or not there needs to be a check for PDO
                    status.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                         l;
    PMOUNTED_DEVICE_INFORMATION         deviceInfo;
    NTSTATUS                            status;
    PFILE_OBJECT                        fileObject;
    PDEVICE_OBJECT                      deviceObject;
    KEVENT                              event;
    PIRP                                irp;
    IO_STATUS_BLOCK                     ioStatus;
    PIO_STACK_LOCATION                  irpSp;
    PDEVICE_RELATIONS                   deviceRelations;
    TARGET_DEVICE_CUSTOM_NOTIFICATION   notification;

    if (CheckForPdo) {
        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            if (!RtlCompareUnicodeString(DeviceName, &deviceInfo->DeviceName,
                                         TRUE)) {

                break;
            }
        }

        if (l == &Extension->MountedDeviceList || deviceInfo->NotAPdo) {
            return;
        }
    }

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(0, deviceObject, NULL, 0, NULL,
                                        0, FALSE, &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }

    irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    irp->IoStatus.Information = 0;
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->MajorFunction = IRP_MJ_PNP;
    irpSp->MinorFunction = IRP_MN_QUERY_DEVICE_RELATIONS;
    irpSp->Parameters.QueryDeviceRelations.Type = TargetDeviceRelation;
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);

    if (!NT_SUCCESS(status)) {
        return;
    }

    deviceRelations = (PDEVICE_RELATIONS) ioStatus.Information;
    if (deviceRelations->Count < 1) {
        ExFreePool(deviceRelations);
        return;
    }

    deviceObject = deviceRelations->Objects[0];
    ExFreePool(deviceRelations);

    notification.Version = 1;
    notification.Size = (USHORT)
                        FIELD_OFFSET(TARGET_DEVICE_CUSTOM_NOTIFICATION,
                                     CustomDataBuffer);
    RtlCopyMemory(&notification.Event, &GUID_IO_VOLUME_NAME_CHANGE,
                  sizeof(GUID_IO_VOLUME_NAME_CHANGE));
    notification.FileObject = NULL;
    notification.NameBufferOffset = -1;

    IoReportTargetDeviceChangeAsynchronous(deviceObject, &notification, NULL,
                                           NULL);

    ObDereferenceObject(deviceObject);
}

NTSTATUS
MountMgrCreatePointWorker(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     SymbolicLinkName,
    IN  PUNICODE_STRING     DeviceName
    )

/*++

Routine Description:

    This routine creates a mount point.

Arguments:

    Extension           - Supplies the device extension.

    SymbolicLinkName    - Supplies the symbolic link name.

    DeviceName          - Supplies the device name.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING                  symbolicLinkName, deviceName;
    NTSTATUS                        status;
    UNICODE_STRING                  targetName;
    PMOUNTDEV_UNIQUE_ID             uniqueId;
    PWSTR                           symName;
    PLIST_ENTRY                     l;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY       symlinkEntry;

    symbolicLinkName = *SymbolicLinkName;
    deviceName = *DeviceName;

    status = QueryDeviceInformation(&deviceName, &targetName, NULL, NULL,
                                    NULL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (!RtlCompareUnicodeString(&targetName, &deviceInfo->DeviceName,
                                     TRUE)) {

            break;
        }
    }

    symName = ExAllocatePool(PagedPool, symbolicLinkName.Length +
                                        sizeof(WCHAR));
    if (!symName) {
        ExFreePool(targetName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(symName, symbolicLinkName.Buffer,
                  symbolicLinkName.Length);
    symName[symbolicLinkName.Length/sizeof(WCHAR)] = 0;

    symbolicLinkName.Buffer = symName;
    symbolicLinkName.MaximumLength += sizeof(WCHAR);

    if (l == &Extension->MountedDeviceList) {

        status = QueryDeviceInformation(&deviceName, NULL, &uniqueId, NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            ExFreePool(symName);
            ExFreePool(targetName.Buffer);
            return status;
        }

        status = IoCreateSymbolicLink(&symbolicLinkName, &targetName);
        if (!NT_SUCCESS(status)) {
            ExFreePool(uniqueId);
            ExFreePool(symName);
            ExFreePool(targetName.Buffer);
            return status;
        }

        if (IsDriveLetter(&symbolicLinkName)) {
            DeleteRegistryDriveLetter(uniqueId);
        }

        status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                       MOUNTED_DEVICES_KEY,
                                       symName, REG_BINARY, uniqueId->UniqueId,
                                       uniqueId->UniqueIdLength);

        ExFreePool(uniqueId);
        ExFreePool(symName);
        ExFreePool(targetName.Buffer);

        return status;
    }

    if (IsDriveLetter(&symbolicLinkName) && HasDriveLetter(deviceInfo)) {
        ExFreePool(symName);
        ExFreePool(targetName.Buffer);
        return STATUS_INVALID_PARAMETER;
    }

    status = IoCreateSymbolicLink(&symbolicLinkName, &targetName);
    ExFreePool(targetName.Buffer);
    if (!NT_SUCCESS(status)) {
        ExFreePool(symName);
        return status;
    }

    uniqueId = deviceInfo->UniqueId;
    status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                                   MOUNTED_DEVICES_KEY,
                                   symName, REG_BINARY, uniqueId->UniqueId,
                                   uniqueId->UniqueIdLength);

    if (!NT_SUCCESS(status)) {
        IoDeleteSymbolicLink(&symbolicLinkName);
        ExFreePool(symName);
        return status;
    }

    symlinkEntry = ExAllocatePool(PagedPool, sizeof(SYMBOLIC_LINK_NAME_ENTRY));
    if (!symlinkEntry) {
        IoDeleteSymbolicLink(&symbolicLinkName);
        ExFreePool(symName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    symlinkEntry->SymbolicLinkName.Length = symbolicLinkName.Length;
    symlinkEntry->SymbolicLinkName.MaximumLength =
            symlinkEntry->SymbolicLinkName.Length + sizeof(WCHAR);
    symlinkEntry->SymbolicLinkName.Buffer =
            ExAllocatePool(PagedPool,
                           symlinkEntry->SymbolicLinkName.MaximumLength);
    if (!symlinkEntry->SymbolicLinkName.Buffer) {
        ExFreePool(symlinkEntry);
        IoDeleteSymbolicLink(&symbolicLinkName);
        ExFreePool(symName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(symlinkEntry->SymbolicLinkName.Buffer,
                  symbolicLinkName.Buffer, symbolicLinkName.Length);
    symlinkEntry->SymbolicLinkName.Buffer[
            symlinkEntry->SymbolicLinkName.Length/sizeof(WCHAR)] = 0;
    symlinkEntry->IsInDatabase = TRUE;

    InsertTailList(&deviceInfo->SymbolicLinkNames, &symlinkEntry->ListEntry);

    SendLinkCreated(&symlinkEntry->SymbolicLinkName);

    if (IsDriveLetter(&symbolicLinkName)) {
        DeleteNoDriveLetterEntry(uniqueId);
    }

    ExFreePool(symName);

    MountMgrNotify(Extension);

    if (!deviceInfo->NotAPdo) {
        MountMgrNotifyNameChange(Extension, DeviceName, FALSE);
    }

    return status;
}

NTSTATUS
WriteUniqueIdToMaster(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine writes the unique id to the master database.

Arguments:

    Extension       - Supplies the device extension.

    DatabaseEntry   - Supplies the database entry.

    DeviceName      - Supplies the device name.

Return Value:

    NTSTATUS

--*/

{
    PWSTR                       name;
    NTSTATUS                    status;
    UNICODE_STRING              symName;
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    name = ExAllocatePool(PagedPool, DatabaseEntry->VolumeNameLength +
                          sizeof(WCHAR));
    if (!name) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(name, (PCHAR) DatabaseEntry +
                  DatabaseEntry->VolumeNameOffset,
                  DatabaseEntry->VolumeNameLength);
    name[DatabaseEntry->VolumeNameLength/sizeof(WCHAR)] = 0;

    status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                                   name, REG_BINARY, (PCHAR) DatabaseEntry +
                                   DatabaseEntry->UniqueIdOffset,
                                   DatabaseEntry->UniqueIdLength);

    ExFreePool(name);

    symName.Length = symName.MaximumLength = DatabaseEntry->VolumeNameLength;
    symName.Buffer = (PWSTR) ((PCHAR) DatabaseEntry +
                              DatabaseEntry->VolumeNameOffset);

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (DatabaseEntry->UniqueIdLength ==
            deviceInfo->UniqueId->UniqueIdLength &&
            RtlCompareMemory((PCHAR) DatabaseEntry +
                             DatabaseEntry->UniqueIdOffset,
                             deviceInfo->UniqueId->UniqueId,
                             DatabaseEntry->UniqueIdLength) ==
                             DatabaseEntry->UniqueIdLength) {

            break;
        }
    }

    if (l != &Extension->MountedDeviceList) {
        MountMgrCreatePointWorker(Extension, &symName,
                                  &deviceInfo->DeviceName);
    }

    return status;
}

VOID
UpdateReplicatedUniqueIds(
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo,
    IN  PMOUNTMGR_FILE_ENTRY        DatabaseEntry
    )

/*++

Routine Description:

    This routine updates the list of replicated unique ids in the device info.

Arguments:

    DeviceInfo      - Supplies the device information.

    DatabaseEntry   - Supplies the database entry.

Return Value:

    None.

--*/

{
    PLIST_ENTRY             l;
    PREPLICATED_UNIQUE_ID   replUniqueId;

    for (l = DeviceInfo->ReplicatedUniqueIds.Flink;
         l != &DeviceInfo->ReplicatedUniqueIds; l = l->Flink) {

        replUniqueId = CONTAINING_RECORD(l, REPLICATED_UNIQUE_ID, ListEntry);

        if (replUniqueId->UniqueId->UniqueIdLength ==
            DatabaseEntry->UniqueIdLength &&
            RtlCompareMemory(replUniqueId->UniqueId->UniqueId,
                             (PCHAR) DatabaseEntry +
                             DatabaseEntry->UniqueIdOffset,
                             replUniqueId->UniqueId->UniqueIdLength) ==
                             replUniqueId->UniqueId->UniqueIdLength) {

            break;
        }
    }

    if (l != &DeviceInfo->ReplicatedUniqueIds) {
        return;
    }

    replUniqueId = ExAllocatePool(PagedPool, sizeof(REPLICATED_UNIQUE_ID));
    if (!replUniqueId) {
        return;
    }

    replUniqueId->UniqueId = ExAllocatePool(PagedPool,
                                            sizeof(MOUNTDEV_UNIQUE_ID) +
                                            DatabaseEntry->UniqueIdLength);
    if (!replUniqueId->UniqueId) {
        ExFreePool(replUniqueId);
        return;
    }

    replUniqueId->UniqueId->UniqueIdLength = DatabaseEntry->UniqueIdLength;
    RtlCopyMemory(replUniqueId->UniqueId->UniqueId, (PCHAR) DatabaseEntry +
                  DatabaseEntry->UniqueIdOffset,
                  replUniqueId->UniqueId->UniqueIdLength);

    InsertTailList(&DeviceInfo->ReplicatedUniqueIds, &replUniqueId->ListEntry);
}

BOOLEAN
IsUniqueIdPresent(
    IN  PDEVICE_EXTENSION       Extension,
    IN  PMOUNTMGR_FILE_ENTRY    DatabaseEntry
    )

/*++

Routine Description:

    This routine checks to see if the given unique id exists in the system.

Arguments:

    Extension       - Supplies the device extension.

    DatabaseEntry   - Supplies the database entry.

Return Value:

    FALSE   - The unique id is not in the system.

    TRUE    - The unique id is in the system.

--*/

{
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (DatabaseEntry->UniqueIdLength ==
            deviceInfo->UniqueId->UniqueIdLength &&
            RtlCompareMemory((PCHAR) DatabaseEntry +
                             DatabaseEntry->UniqueIdOffset,
                             deviceInfo->UniqueId->UniqueId,
                             DatabaseEntry->UniqueIdLength) ==
                             DatabaseEntry->UniqueIdLength) {

            return TRUE;
        }
    }

    return FALSE;
}

VOID
ReconcileThisDatabaseWithMasterWorker(
    IN  PVOID   WorkItem
    )

/*++

Routine Description:

    This routine reconciles the remote database with the master database.

Arguments:

    WorkItem    - Supplies the device information.

Return Value:

    None.

--*/

{
    PRECONCILE_WORK_ITEM_INFO       workItem = WorkItem;
    PDEVICE_EXTENSION               Extension;
    PMOUNTED_DEVICE_INFORMATION     DeviceInfo;
    PLIST_ENTRY                     l;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;
    HANDLE                          remoteDatabaseHandle, indexHandle, junctionHandle;
    UNICODE_STRING                  suffix, indexName;
    OBJECT_ATTRIBUTES               oa;
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;
    FILE_REPARSE_POINT_INFORMATION  reparseInfo, previousReparseInfo;
    ULONG                           offset;
    PMOUNTMGR_FILE_ENTRY            entry;
    WCHAR                           volumeNameBuffer[MAX_VOLUME_PATH];
    UNICODE_STRING                  volumeName, otherVolumeName;
    BOOLEAN                         restartScan;
    PMOUNTDEV_UNIQUE_ID             uniqueId;
    ULONG                           entryLength;

    Extension = workItem->Extension;
    DeviceInfo = workItem->DeviceInfo;

    status = WaitForRemoteDatabaseSemaphore(Extension);
    if (!NT_SUCCESS(status)) {
        ASSERT(FALSE);
        return;
    }

    KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (deviceInfo == DeviceInfo) {
            break;
        }
    }

    if (l == &Extension->MountedDeviceList) {
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    if (DeviceInfo->IsRemovable) {
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    remoteDatabaseHandle = OpenRemoteDatabase(&DeviceInfo->DeviceName, FALSE);

    RtlInitUnicodeString(&suffix, L"\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION");
    indexName.Length = DeviceInfo->DeviceName.Length +
                       suffix.Length;
    indexName.MaximumLength = indexName.Length + sizeof(WCHAR);
    indexName.Buffer = ExAllocatePool(PagedPool, indexName.MaximumLength);
    if (!indexName.Buffer) {
        if (remoteDatabaseHandle) {
            CloseRemoteDatabase(remoteDatabaseHandle);
        }
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    RtlCopyMemory(indexName.Buffer, DeviceInfo->DeviceName.Buffer,
                  DeviceInfo->DeviceName.Length);
    RtlCopyMemory((PCHAR) indexName.Buffer + DeviceInfo->DeviceName.Length,
                  suffix.Buffer, suffix.Length);
    indexName.Buffer[indexName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &indexName, OBJ_CASE_INSENSITIVE, 0, 0);

    status = ZwOpenFile(&indexHandle, FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_ALERT);
    ExFreePool(indexName.Buffer);
    if (!NT_SUCCESS(status)) {
        if (remoteDatabaseHandle) {
            TruncateRemoteDatabase(remoteDatabaseHandle, 0);
            CloseRemoteDatabase(remoteDatabaseHandle);
        }
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    status = ZwQueryDirectoryFile(indexHandle, NULL, NULL, NULL, &ioStatus,
                                  &reparseInfo, sizeof(reparseInfo),
                                  FileReparsePointInformation, TRUE, NULL,
                                  FALSE);
    if (!NT_SUCCESS(status)) {
        ZwClose(indexHandle);
        if (remoteDatabaseHandle) {
            TruncateRemoteDatabase(remoteDatabaseHandle, 0);
            CloseRemoteDatabase(remoteDatabaseHandle);
        }
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        ReleaseRemoteDatabaseSemaphore(Extension);
        return;
    }

    if (!remoteDatabaseHandle) {
        remoteDatabaseHandle = OpenRemoteDatabase(&DeviceInfo->DeviceName,
                                                  TRUE);
        if (!remoteDatabaseHandle) {
            ZwClose(indexHandle);
            KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            ReleaseRemoteDatabaseSemaphore(Extension);
            return;
        }
    }

    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    offset = 0;
    for (;;) {

        entry = GetRemoteDatabaseEntry(remoteDatabaseHandle, offset);
        if (!entry) {
            break;
        }

        entry->RefCount = 0;
        status = WriteRemoteDatabaseEntry(remoteDatabaseHandle, offset, entry);
        if (!NT_SUCCESS(status)) {
            ExFreePool(entry);
            ZwClose(indexHandle);
            CloseRemoteDatabase(remoteDatabaseHandle);
            ReleaseRemoteDatabaseSemaphore(Extension);
            return;
        }

        offset += entry->EntryLength;
        ExFreePool(entry);
    }

    volumeName.MaximumLength = MAX_VOLUME_PATH*sizeof(WCHAR);
    volumeName.Length = 0;
    volumeName.Buffer = volumeNameBuffer;

    restartScan = TRUE;
    for (;;) {

        previousReparseInfo = reparseInfo;

        status = ZwQueryDirectoryFile(indexHandle, NULL, NULL, NULL, &ioStatus,
                                      &reparseInfo, sizeof(reparseInfo),
                                      FileReparsePointInformation, TRUE, NULL,
                                      restartScan);
        if (restartScan) {
            restartScan = FALSE;
        } else {
            if (previousReparseInfo.FileReference ==
                reparseInfo.FileReference &&
                previousReparseInfo.Tag == reparseInfo.Tag) {

                break;
            }
        }

        if (!NT_SUCCESS(status)) {
            break;
        }

        if (reparseInfo.Tag != IO_REPARSE_TAG_MOUNT_POINT) {
            continue;
        }

        if (!QueryVolumeName(indexHandle, &reparseInfo.FileReference,
                             &volumeName)) {

            continue;
        }

        offset = 0;
        for (;;) {

            entry = GetRemoteDatabaseEntry(remoteDatabaseHandle, offset);
            if (!entry) {
                break;
            }

            otherVolumeName.Length = otherVolumeName.MaximumLength =
                    entry->VolumeNameLength;
            otherVolumeName.Buffer = (PWSTR) ((PCHAR) entry +
                    entry->VolumeNameOffset);

            if (RtlEqualUnicodeString(&otherVolumeName, &volumeName, TRUE)) {
                break;
            }

            offset += entry->EntryLength;
            ExFreePool(entry);
        }

        if (!entry) {

            KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);
            status = QueryUniqueIdFromMaster(&volumeName, &uniqueId);
            KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

            if (!NT_SUCCESS(status)) {
                continue;
            }

            entryLength = sizeof(MOUNTMGR_FILE_ENTRY) +
                          volumeName.Length + uniqueId->UniqueIdLength;
            entry = ExAllocatePool(PagedPool, entryLength);
            if (!entry) {
                ExFreePool(uniqueId);
                continue;
            }

            entry->EntryLength = entryLength;
            entry->RefCount = 1;
            entry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
            entry->VolumeNameLength = volumeName.Length;
            entry->UniqueIdOffset = entry->VolumeNameOffset +
                                    entry->VolumeNameLength;
            entry->UniqueIdLength = uniqueId->UniqueIdLength;

            RtlCopyMemory((PCHAR) entry + entry->VolumeNameOffset,
                          volumeName.Buffer, entry->VolumeNameLength);
            RtlCopyMemory((PCHAR) entry + entry->UniqueIdOffset,
                          uniqueId->UniqueId, entry->UniqueIdLength);

            status = AddRemoteDatabaseEntry(remoteDatabaseHandle, entry);

            ExFreePool(entry);
            ExFreePool(uniqueId);

            if (!NT_SUCCESS(status)) {
                ZwClose(indexHandle);
                CloseRemoteDatabase(remoteDatabaseHandle);
                ReleaseRemoteDatabaseSemaphore(Extension);
                return;
            }

            continue;
        }

        if (entry->RefCount) {

            entry->RefCount++;
            status = WriteRemoteDatabaseEntry(remoteDatabaseHandle, offset,
                                              entry);

            if (!NT_SUCCESS(status)) {
                ExFreePool(entry);
                ZwClose(indexHandle);
                CloseRemoteDatabase(remoteDatabaseHandle);
                ReleaseRemoteDatabaseSemaphore(Extension);
                return;
            }

        } else {

            KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);

            status = QueryUniqueIdFromMaster(&volumeName, &uniqueId);

            if (NT_SUCCESS(status)) {

                if (uniqueId->UniqueIdLength == entry->UniqueIdLength &&
                    RtlCompareMemory(uniqueId->UniqueId,
                                     (PCHAR) entry + entry->UniqueIdOffset,
                                     entry->UniqueIdLength) ==
                                     entry->UniqueIdLength) {

                    entry->RefCount++;
                    status = WriteRemoteDatabaseEntry(remoteDatabaseHandle,
                                                      offset, entry);

                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                } else if (IsUniqueIdPresent(Extension, entry)) {

                    status = WriteUniqueIdToMaster(Extension, entry);
                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                    entry->RefCount++;
                    status = WriteRemoteDatabaseEntry(remoteDatabaseHandle,
                                                      offset, entry);

                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                } else {

                    status = DeleteRemoteDatabaseEntry(remoteDatabaseHandle,
                                                       offset);
                    if (!NT_SUCCESS(status)) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ExFreePool(entry);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                    ExFreePool(entry);

                    entryLength = sizeof(MOUNTMGR_FILE_ENTRY) +
                                  volumeName.Length + uniqueId->UniqueIdLength;
                    entry = ExAllocatePool(PagedPool, entryLength);
                    if (!entry) {
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }

                    entry->EntryLength = entryLength;
                    entry->RefCount = 1;
                    entry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
                    entry->VolumeNameLength = volumeName.Length;
                    entry->UniqueIdOffset = entry->VolumeNameOffset +
                                            entry->VolumeNameLength;
                    entry->UniqueIdLength = uniqueId->UniqueIdLength;

                    RtlCopyMemory((PCHAR) entry + entry->VolumeNameOffset,
                                  volumeName.Buffer, entry->VolumeNameLength);
                    RtlCopyMemory((PCHAR) entry + entry->UniqueIdOffset,
                                  uniqueId->UniqueId, entry->UniqueIdLength);

                    status = AddRemoteDatabaseEntry(remoteDatabaseHandle,
                                                    entry);
                    if (!NT_SUCCESS(status)) {
                        ExFreePool(entry);
                        ExFreePool(uniqueId);
                        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                           1, FALSE);
                        ZwClose(indexHandle);
                        CloseRemoteDatabase(remoteDatabaseHandle);
                        ReleaseRemoteDatabaseSemaphore(Extension);
                        return;
                    }
                }

                ExFreePool(uniqueId);

            } else {
                status = WriteUniqueIdToMaster(Extension, entry);
                if (!NT_SUCCESS(status)) {
                    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                       1, FALSE);
                    ExFreePool(entry);
                    ZwClose(indexHandle);
                    CloseRemoteDatabase(remoteDatabaseHandle);
                    ReleaseRemoteDatabaseSemaphore(Extension);
                    return;
                }

                entry->RefCount++;
                status = WriteRemoteDatabaseEntry(remoteDatabaseHandle, offset,
                                                  entry);

                if (!NT_SUCCESS(status)) {
                    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT,
                                       1, FALSE);
                    ExFreePool(entry);
                    ZwClose(indexHandle);
                    CloseRemoteDatabase(remoteDatabaseHandle);
                    ReleaseRemoteDatabaseSemaphore(Extension);
                    return;
                }
            }

            KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        }

        ExFreePool(entry);
    }

    ZwClose(indexHandle);

    KeWaitForSingleObject(&Extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (deviceInfo == DeviceInfo) {
            break;
        }
    }

    if (l == &Extension->MountedDeviceList) {
        deviceInfo = NULL;
    }

    offset = 0;
    for (;;) {

        entry = GetRemoteDatabaseEntry(remoteDatabaseHandle, offset);
        if (!entry) {
            break;
        }

        if (!entry->RefCount) {
            status = DeleteRemoteDatabaseEntry(remoteDatabaseHandle, offset);
            if (!NT_SUCCESS(status)) {
                ExFreePool(entry);
                KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1,
                                   FALSE);
                CloseRemoteDatabase(remoteDatabaseHandle);
                ReleaseRemoteDatabaseSemaphore(Extension);
                return;
            }

            ExFreePool(entry);
            continue;
        }

        if (deviceInfo) {
            UpdateReplicatedUniqueIds(deviceInfo, entry);
        }

        offset += entry->EntryLength;
        ExFreePool(entry);
    }

    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    CloseRemoteDatabase(remoteDatabaseHandle);
    ReleaseRemoteDatabaseSemaphore(Extension);
}

VOID
ReconcileThisDatabaseWithMaster(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    )

/*++

Routine Description:

    This routine reconciles the remote database with the master database.

Arguments:

    DeviceInfo  - Supplies the device information.

Return Value:

    None.

--*/

{
    PRECONCILE_WORK_ITEM    workItem;

    if (DeviceInfo->IsRemovable) {
        return;
    }

    workItem = ExAllocatePool(NonPagedPoolMustSucceed,
                              sizeof(RECONCILE_WORK_ITEM));
    if (!workItem) {
        return;
    }

    ExInitializeWorkItem(&workItem->WorkItem,
                         ReconcileThisDatabaseWithMasterWorker,
                         &workItem->WorkItemInfo);
    workItem->WorkItemInfo.Extension = Extension;
    workItem->WorkItemInfo.DeviceInfo = DeviceInfo;

    QueueWorkItem(Extension, &workItem->WorkItem);
}

NTSTATUS
DeleteFromLocalDatabaseRoutine(
    IN  PWSTR   ValueName,
    IN  ULONG   ValueType,
    IN  PVOID   ValueData,
    IN  ULONG   ValueLength,
    IN  PVOID   Context,
    IN  PVOID   EntryContext
    )

/*++

Routine Description:

    This routine queries the unique id for the given value.

Arguments:

    ValueName       - Supplies the name of the registry value.

    ValueType       - Supplies the type of the registry value.

    ValueData       - Supplies the data of the registry value.

    ValueLength     - Supplies the length of the registry value.

    Context         - Supplies the unique id.

    EntryContext    - Not used.

Return Value:

    NTSTATUS

--*/

{
    PMOUNTDEV_UNIQUE_ID uniqueId = Context;

    if (uniqueId->UniqueIdLength == ValueLength &&
        RtlCompareMemory(uniqueId->UniqueId,
                         ValueData, ValueLength) == ValueLength) {

        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               ValueName);
    }

    return STATUS_SUCCESS;
}

VOID
DeleteFromLocalDatabase(
    IN  PUNICODE_STRING     SymbolicLinkName,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine makes sure that the given symbolic link names exists in the
    local database and that its unique id is equal to the one given.  If these
    two conditions are true then this local database entry is deleted.

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name.

    UniqueId            - Supplies the unique id.

Return Value:

    None.

--*/

{
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].QueryRoutine = DeleteFromLocalDatabaseRoutine;
    queryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED;
    queryTable[0].Name = SymbolicLinkName->Buffer;

    RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                           queryTable, UniqueId, NULL);
}

PSAVED_LINKS_INFORMATION
RemoveSavedLinks(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PMOUNTDEV_UNIQUE_ID UniqueId
    )

/*++

Routine Description:

    This routine finds and removed the given unique id from the saved links
    list.

Arguments:

    Extension   - Supplies the device extension.

    UniqueId    - Supplies the unique id.

Return Value:

    The removed saved links list or NULL.

--*/

{
    PLIST_ENTRY                 l;
    PSAVED_LINKS_INFORMATION    savedLinks;

    for (l = Extension->SavedLinksList.Flink;
         l != &Extension->SavedLinksList; l = l->Flink) {

        savedLinks = CONTAINING_RECORD(l, SAVED_LINKS_INFORMATION, ListEntry);
        if (savedLinks->UniqueId->UniqueIdLength != UniqueId->UniqueIdLength) {
            continue;
        }

        if (RtlCompareMemory(savedLinks->UniqueId->UniqueId,
                             UniqueId->UniqueId, UniqueId->UniqueIdLength) ==
            UniqueId->UniqueIdLength) {

            break;
        }
    }

    if (l == &Extension->SavedLinksList) {
        return NULL;
    }

    RemoveEntryList(l);

    return savedLinks;
}

BOOLEAN
RedirectSavedLink(
    IN  PSAVED_LINKS_INFORMATION    SavedLinks,
    IN  PUNICODE_STRING             SymbolicLinkName,
    IN  PUNICODE_STRING             DeviceName
    )

/*++

Routine Description:

    This routine attempts to redirect the given link to the given device name
    if this link is in the saved links list.  When this is done, the
    symbolic link entry is removed from the saved links list.

Arguments:


Return Value:

    FALSE   - The link was not successfully redirected.

    TRUE    - The link was successfully redirected.

--*/

{
    PLIST_ENTRY                 l;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;

    for (l = SavedLinks->SymbolicLinkNames.Flink;
         l != &SavedLinks->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);

        if (RtlEqualUnicodeString(SymbolicLinkName,
                                  &symlinkEntry->SymbolicLinkName, TRUE)) {

            break;
        }
    }

    if (l == &SavedLinks->SymbolicLinkNames) {
        return FALSE;
    }

    // NOTE There is a small window here where the drive letter could be
    // taken away.  This is the best we can do without more support from OB.

    IoDeleteSymbolicLink(SymbolicLinkName);
    IoCreateSymbolicLink(SymbolicLinkName, DeviceName);

    ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
    ExFreePool(symlinkEntry);
    RemoveEntryList(l);

    return TRUE;
}

BOOLEAN
IsOffline(
    IN  PUNICODE_STRING     SymbolicLinkName
    )

/*++

Routine Description:

    This routine checks to see if the given name has been marked to be
    an offline volume.

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name.

Return Value:

    FALSE   - This volume is not marked for offline.

    TRUE    - This volume is marked for offline.

--*/

{
    ULONG                       zero, offline;
    RTL_QUERY_REGISTRY_TABLE    queryTable[2];
    NTSTATUS                    status;

    zero = 0;

    RtlZeroMemory(queryTable, 2*sizeof(RTL_QUERY_REGISTRY_TABLE));
    queryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    queryTable[0].Name = SymbolicLinkName->Buffer;
    queryTable[0].EntryContext = &offline;
    queryTable[0].DefaultType = REG_DWORD;
    queryTable[0].DefaultData = &zero;
    queryTable[0].DefaultLength = sizeof(ULONG);

    status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
                                    MOUNTED_DEVICES_OFFLINE_KEY, queryTable,
                                    NULL, NULL);
    if (!NT_SUCCESS(status)) {
        offline = 0;
    }

    return offline ? TRUE : FALSE;
}

VOID
SendOnlineNotification(
    IN  PUNICODE_STRING     NotificationName
    )

/*++

Routine Description:

    This routine sends an ONLINE notification to the given device.

Arguments:

    NotificationName    - Supplies the notification name.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(NotificationName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_VOLUME_ONLINE, deviceObject,
                                        NULL, 0, NULL, 0, FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);
}

NTSTATUS
MountMgrTargetDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   DeviceInfo
    )

/*++

Routine Description:

    This routine processes target device notifications.

Arguments:

    NotificationStructure    - Supplies the notification structure.

    DeviceInfo               - Supplies the device information.

Return Value:

    None.

--*/

{
    PTARGET_DEVICE_REMOVAL_NOTIFICATION     notification = NotificationStructure;
    PMOUNTED_DEVICE_INFORMATION             deviceInfo = DeviceInfo;
    PDEVICE_EXTENSION                       extension = deviceInfo->Extension;

    if (!IsEqualGUID(&notification->Event,
                     &GUID_TARGET_DEVICE_REMOVE_COMPLETE)) {

        return STATUS_SUCCESS;
    }

    MountMgrMountedDeviceRemoval(extension, &deviceInfo->NotificationName);

    return STATUS_SUCCESS;
}

VOID
RegisterForTargetDeviceNotification(
    IN  PDEVICE_EXTENSION           Extension,
    IN  PMOUNTED_DEVICE_INFORMATION DeviceInfo
    )

/*++

Routine Description:

    This routine registers for target device notification so that the
    symbolic link to a device interface can be removed in a timely manner.

Arguments:

    Extension   - Supplies the device extension.

    DeviceInfo  - Supplies the device information.

Return Value:

    None.

--*/

{
    NTSTATUS                                status;
    PFILE_OBJECT                            fileObject;
    PDEVICE_OBJECT                          deviceObject;

    status = IoGetDeviceObjectPointer(&DeviceInfo->DeviceName,
                                      FILE_READ_ATTRIBUTES, &fileObject,
                                      &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }

    status = IoRegisterPlugPlayNotification(
                EventCategoryTargetDeviceChange, 0, fileObject,
                Extension->DriverObject, MountMgrTargetDeviceNotification,
                DeviceInfo, &DeviceInfo->TargetDeviceNotificationEntry);

    if (!NT_SUCCESS(status)) {
        DeviceInfo->TargetDeviceNotificationEntry = NULL;
    }

    ObDereferenceObject(fileObject);
}

NTSTATUS
MountMgrMountedDeviceArrival(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     NotificationName,
    IN  BOOLEAN             NotAPdo
    )

{
    PDEVICE_EXTENSION                       extension = Extension;
    PMOUNTED_DEVICE_INFORMATION             deviceInfo, d;
    NTSTATUS                                status;
    UNICODE_STRING                          targetName, otherTargetName;
    PMOUNTDEV_UNIQUE_ID                     uniqueId;
    BOOLEAN                                 isRecognized;
    UNICODE_STRING                          suggestedName;
    BOOLEAN                                 useOnlyIfThereAreNoOtherLinks;
    PUNICODE_STRING                         symbolicLinkNames;
    ULONG                                   numNames, i;
    BOOLEAN                                 hasDriveLetter, offline;
    BOOLEAN                                 hasVolumeName, isLinkPreset;
    PSYMBOLIC_LINK_NAME_ENTRY               symlinkEntry;
    UNICODE_STRING                          volumeName;
    UNICODE_STRING                          driveLetterName;
    PSAVED_LINKS_INFORMATION                savedLinks;
    PLIST_ENTRY                             l;

    deviceInfo = ExAllocatePool(PagedPool,
                                sizeof(MOUNTED_DEVICE_INFORMATION));
    if (!deviceInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(deviceInfo, sizeof(MOUNTED_DEVICE_INFORMATION));

    InitializeListHead(&deviceInfo->SymbolicLinkNames);
    InitializeListHead(&deviceInfo->ReplicatedUniqueIds);

    deviceInfo->NotificationName.Length =
            NotificationName->Length;
    deviceInfo->NotificationName.MaximumLength =
            deviceInfo->NotificationName.Length + sizeof(WCHAR);
    deviceInfo->NotificationName.Buffer =
            ExAllocatePool(PagedPool,
                           deviceInfo->NotificationName.MaximumLength);
    if (!deviceInfo->NotificationName.Buffer) {
        ExFreePool(deviceInfo);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(deviceInfo->NotificationName.Buffer,
                  NotificationName->Buffer,
                  deviceInfo->NotificationName.Length);
    deviceInfo->NotificationName.Buffer[
            deviceInfo->NotificationName.Length/sizeof(WCHAR)] = 0;
    deviceInfo->NotAPdo = NotAPdo;
    deviceInfo->Extension = extension;

    status = QueryDeviceInformation(NotificationName,
                                    &targetName, &uniqueId,
                                    &deviceInfo->IsRemovable, &isRecognized);
    if (!NT_SUCCESS(status)) {

        KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                              FALSE, NULL);

        for (l = extension->DeadMountedDeviceList.Flink;
             l != &extension->DeadMountedDeviceList; l = l->Flink) {

            d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION, ListEntry);
            if (RtlEqualUnicodeString(&deviceInfo->NotificationName,
                                      &d->NotificationName, TRUE)) {

                break;
            }
        }

        if (l == &extension->DeadMountedDeviceList) {
            InsertTailList(&extension->DeadMountedDeviceList,
                           &deviceInfo->ListEntry);
        } else {
            ExFreePool(deviceInfo->NotificationName.Buffer);
            ExFreePool(deviceInfo);
        }

        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

        return status;
    }

    deviceInfo->UniqueId = uniqueId;
    deviceInfo->DeviceName = targetName;
    deviceInfo->KeepLinksWhenOffline = FALSE;

    status = QuerySuggestedLinkName(&deviceInfo->NotificationName,
                                    &suggestedName,
                                    &useOnlyIfThereAreNoOtherLinks);
    if (!NT_SUCCESS(status)) {
        suggestedName.Buffer = NULL;
    }

    if (suggestedName.Buffer && IsDriveLetter(&suggestedName)) {
        deviceInfo->SuggestedDriveLetter = (UCHAR)
                                           suggestedName.Buffer[12];
    } else {
        deviceInfo->SuggestedDriveLetter = 0;
    }

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        d = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);
        if (!RtlCompareUnicodeString(&d->DeviceName, &targetName, TRUE)) {
            break;
        }
    }

    if (l != &extension->MountedDeviceList) {
        if (suggestedName.Buffer) {
            ExFreePool(suggestedName.Buffer);
        }
        ExFreePool(uniqueId);
        ExFreePool(targetName.Buffer);
        ExFreePool(deviceInfo->NotificationName.Buffer);
        ExFreePool(deviceInfo);
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        return STATUS_SUCCESS;
    }

    status = QuerySymbolicLinkNamesFromStorage(extension,
             deviceInfo, suggestedName.Buffer ? &suggestedName : NULL,
             useOnlyIfThereAreNoOtherLinks, &symbolicLinkNames, &numNames);

    if (suggestedName.Buffer) {
        ExFreePool(suggestedName.Buffer);
    }

    if (!NT_SUCCESS(status)) {
        symbolicLinkNames = NULL;
        numNames = 0;
        status = STATUS_SUCCESS;
    }

    savedLinks = RemoveSavedLinks(extension, uniqueId);

    hasDriveLetter = FALSE;
    offline = FALSE;
    hasVolumeName = FALSE;
    for (i = 0; i < numNames; i++) {

        if (MOUNTMGR_IS_VOLUME_NAME(&symbolicLinkNames[i])) {
            hasVolumeName = TRUE;
        } else if (IsDriveLetter(&symbolicLinkNames[i])) {
            if (hasDriveLetter) {
                DeleteFromLocalDatabase(&symbolicLinkNames[i], uniqueId);
                continue;
            }
            hasDriveLetter = TRUE;
        }

        status = IoCreateSymbolicLink(&symbolicLinkNames[i], &targetName);
        if (!NT_SUCCESS(status)) {
            isLinkPreset = TRUE;
            if (!savedLinks ||
                !RedirectSavedLink(savedLinks, &symbolicLinkNames[i],
                                   &targetName)) {

                status = QueryDeviceInformation(&symbolicLinkNames[i],
                                                &otherTargetName, NULL, NULL,
                                                NULL);
                if (!NT_SUCCESS(status)) {
                    isLinkPreset = FALSE;
                }

                if (isLinkPreset &&
                    !RtlEqualUnicodeString(&targetName, &otherTargetName,
                                           TRUE)) {

                    isLinkPreset = FALSE;
                }

                if (NT_SUCCESS(status)) {
                    ExFreePool(otherTargetName.Buffer);
                }
            }

            if (!isLinkPreset) {
                if (IsDriveLetter(&symbolicLinkNames[i])) {
                    hasDriveLetter = FALSE;
                    DeleteFromLocalDatabase(&symbolicLinkNames[i], uniqueId);
                }

                ExFreePool(symbolicLinkNames[i].Buffer);
                continue;
            }
        }

        if (IsOffline(&symbolicLinkNames[i])) {
            offline = TRUE;
        }

        symlinkEntry = ExAllocatePool(PagedPool,
                                      sizeof(SYMBOLIC_LINK_NAME_ENTRY));
        if (!symlinkEntry) {
            IoDeleteSymbolicLink(&symbolicLinkNames[i]);
            ExFreePool(symbolicLinkNames[i].Buffer);
            continue;
        }

        symlinkEntry->SymbolicLinkName = symbolicLinkNames[i];
        symlinkEntry->IsInDatabase = TRUE;

        InsertTailList(&deviceInfo->SymbolicLinkNames,
                       &symlinkEntry->ListEntry);
    }

    for (l = deviceInfo->SymbolicLinkNames.Flink;
         l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);
        SendLinkCreated(&symlinkEntry->SymbolicLinkName);
    }

    if (savedLinks) {
        while (!IsListEmpty(&savedLinks->SymbolicLinkNames)) {
            l = RemoveHeadList(&deviceInfo->SymbolicLinkNames);
            symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);
            IoDeleteSymbolicLink(&symlinkEntry->SymbolicLinkName);
            ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
            ExFreePool(symlinkEntry);
        }
        ExFreePool(savedLinks->UniqueId);
        ExFreePool(savedLinks);
    }

    if (!hasVolumeName) {
        status = CreateNewVolumeName(&volumeName);
        if (NT_SUCCESS(status)) {
            RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                    MOUNTED_DEVICES_KEY, volumeName.Buffer, REG_BINARY,
                    uniqueId->UniqueId, uniqueId->UniqueIdLength);

            IoCreateSymbolicLink(&volumeName, &targetName);

            symlinkEntry = ExAllocatePool(PagedPool,
                                          sizeof(SYMBOLIC_LINK_NAME_ENTRY));
            if (symlinkEntry) {
                symlinkEntry->SymbolicLinkName = volumeName;
                symlinkEntry->IsInDatabase = TRUE;
                InsertTailList(&deviceInfo->SymbolicLinkNames,
                               &symlinkEntry->ListEntry);
                SendLinkCreated(&volumeName);
            } else {
                ExFreePool(volumeName.Buffer);
            }
        }
    }

    if (hasDriveLetter) {
        deviceInfo->SuggestedDriveLetter = 0;
    }

    if (!hasDriveLetter && extension->AutomaticDriveLetterAssignment &&
        (isRecognized || deviceInfo->SuggestedDriveLetter) &&
        !HasNoDriveLetterEntry(uniqueId)) {

        status = CreateNewDriveLetterName(&driveLetterName, &targetName,
                                          deviceInfo->SuggestedDriveLetter,
                                          uniqueId);
        if (NT_SUCCESS(status)) {
            RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
                    MOUNTED_DEVICES_KEY, driveLetterName.Buffer,
                    REG_BINARY, uniqueId->UniqueId,
                    uniqueId->UniqueIdLength);

            symlinkEntry = ExAllocatePool(PagedPool,
                                          sizeof(SYMBOLIC_LINK_NAME_ENTRY));
            if (symlinkEntry) {
                symlinkEntry->SymbolicLinkName = driveLetterName;
                symlinkEntry->IsInDatabase = TRUE;
                InsertTailList(&deviceInfo->SymbolicLinkNames,
                               &symlinkEntry->ListEntry);
                SendLinkCreated(&driveLetterName);
            } else {
                ExFreePool(driveLetterName.Buffer);
            }
        }
    }

    if (!NotAPdo) {
        RegisterForTargetDeviceNotification(extension, deviceInfo);
    }

    InsertTailList(&extension->MountedDeviceList, &deviceInfo->ListEntry);
    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    if (!offline) {
        SendOnlineNotification(NotificationName);
    }

    if (symbolicLinkNames) {
        ExFreePool(symbolicLinkNames);
    }

    IssueUniqueIdChangeNotify(extension, NotificationName,
                              uniqueId);

    if (extension->AutomaticDriveLetterAssignment) {
        KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                              FALSE, NULL);
        ReconcileThisDatabaseWithMaster(extension, deviceInfo);
        KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrMountedDeviceRemoval(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     NotificationName
    )

{
    PDEVICE_EXTENSION                       extension = Extension;
    PMOUNTED_DEVICE_INFORMATION             deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY               symlinkEntry;
    PLIST_ENTRY                             l, ll;
    PREPLICATED_UNIQUE_ID                   replUniqueId;
    PSAVED_LINKS_INFORMATION                savedLinks;

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    for (l = extension->MountedDeviceList.Flink;
         l != &extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);
        if (!RtlCompareUnicodeString(&deviceInfo->NotificationName,
                                     NotificationName, TRUE)) {
            break;
        }
    }

    if (l != &extension->MountedDeviceList) {

        if (deviceInfo->KeepLinksWhenOffline) {
            savedLinks = ExAllocatePool(PagedPool,
                                        sizeof(SAVED_LINKS_INFORMATION));
            if (!savedLinks) {
                deviceInfo->KeepLinksWhenOffline = FALSE;
            }
        }

        if (deviceInfo->KeepLinksWhenOffline) {

            InsertTailList(&extension->SavedLinksList,
                           &savedLinks->ListEntry);
            InitializeListHead(&savedLinks->SymbolicLinkNames);
            savedLinks->UniqueId = deviceInfo->UniqueId;

            while (!IsListEmpty(&deviceInfo->SymbolicLinkNames)) {

                ll = RemoveHeadList(&deviceInfo->SymbolicLinkNames);
                symlinkEntry = CONTAINING_RECORD(ll,
                                                 SYMBOLIC_LINK_NAME_ENTRY,
                                                 ListEntry);

                if (symlinkEntry->IsInDatabase) {
                    InsertTailList(&savedLinks->SymbolicLinkNames, ll);
                } else {
                    IoDeleteSymbolicLink(&symlinkEntry->SymbolicLinkName);
                    ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
                    ExFreePool(symlinkEntry);
                }
            }
        } else {

            while (!IsListEmpty(&deviceInfo->SymbolicLinkNames)) {

                ll = RemoveHeadList(&deviceInfo->SymbolicLinkNames);
                symlinkEntry = CONTAINING_RECORD(ll,
                                                 SYMBOLIC_LINK_NAME_ENTRY,
                                                 ListEntry);

                IoDeleteSymbolicLink(&symlinkEntry->SymbolicLinkName);
                ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
                ExFreePool(symlinkEntry);
            }
        }

        while (!IsListEmpty(&deviceInfo->ReplicatedUniqueIds)) {

            ll = RemoveHeadList(&deviceInfo->ReplicatedUniqueIds);
            replUniqueId = CONTAINING_RECORD(ll, REPLICATED_UNIQUE_ID,
                                             ListEntry);

            ExFreePool(replUniqueId->UniqueId);
            ExFreePool(replUniqueId);
        }

        RemoveEntryList(l);
        ExFreePool(deviceInfo->NotificationName.Buffer);

        if (!deviceInfo->KeepLinksWhenOffline) {
            ExFreePool(deviceInfo->UniqueId);
        }

        ExFreePool(deviceInfo->DeviceName.Buffer);

        if (deviceInfo->TargetDeviceNotificationEntry) {
            IoUnregisterPlugPlayNotification(
                    deviceInfo->TargetDeviceNotificationEntry);
        }

        ExFreePool(deviceInfo);

    } else {

        for (l = extension->DeadMountedDeviceList.Flink;
             l != &extension->DeadMountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);
            if (!RtlCompareUnicodeString(&deviceInfo->NotificationName,
                                         NotificationName, TRUE)) {
                break;
            }
        }

        if (l != &extension->DeadMountedDeviceList) {
            RemoveEntryList(l);
            ExFreePool(deviceInfo->NotificationName.Buffer);
            ExFreePool(deviceInfo);
        }
    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrMountedDeviceNotification(
    IN  PVOID   NotificationStructure,
    IN  PVOID   Extension
    )

/*++

Routine Description:

    This routine is called whenever a volume comes or goes.

Arguments:

    NotificationStructure   - Supplies the notification structure.

    Extension               - Supplies the device extension.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_INTERFACE_CHANGE_NOTIFICATION   notification = NotificationStructure;
    PDEVICE_EXTENSION                       extension = Extension;
    BOOLEAN                                 oldHardErrorMode;
    NTSTATUS                                status;

    oldHardErrorMode = PsGetCurrentThread()->HardErrorsAreDisabled;
    PsGetCurrentThread()->HardErrorsAreDisabled = TRUE;

    if (IsEqualGUID(&notification->Event, &GUID_DEVICE_INTERFACE_ARRIVAL)) {

        status = MountMgrMountedDeviceArrival(extension,
                                              notification->SymbolicLinkName,
                                              FALSE);

    } else if (IsEqualGUID(&notification->Event,
                           &GUID_DEVICE_INTERFACE_REMOVAL)) {

        status = MountMgrMountedDeviceRemoval(extension,
                                              notification->SymbolicLinkName);

    } else {
        status = STATUS_INVALID_PARAMETER;
    }

    PsGetCurrentThread()->HardErrorsAreDisabled = oldHardErrorMode;

    return status;
}

NTSTATUS
MountMgrCreateClose(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a create or close requests.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS            status;

    if (irpSp->MajorFunction == IRP_MJ_CREATE) {
        if (irpSp->Parameters.Create.Options&FILE_DIRECTORY_FILE) {
            status = STATUS_NOT_A_DIRECTORY;
        } else {
            status = STATUS_SUCCESS;
        }
    } else {
        status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS
MountMgrCreatePoint(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine creates a mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_CREATE_POINT_INPUT    input = Irp->AssociatedIrp.SystemBuffer;
    ULONG                           len1, len2, len;
    UNICODE_STRING                  symbolicLinkName, deviceName;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_CREATE_POINT_INPUT)) {

        return STATUS_INVALID_PARAMETER;
    }

    len1 = input->DeviceNameOffset + input->DeviceNameLength;
    len2 = input->SymbolicLinkNameOffset + input->SymbolicLinkNameLength;
    len = len1 > len2 ? len1 : len2;

    if (len > irpSp->Parameters.DeviceIoControl.InputBufferLength) {
        return STATUS_INVALID_PARAMETER;
    }

    symbolicLinkName.Length = symbolicLinkName.MaximumLength =
            input->SymbolicLinkNameLength;
    symbolicLinkName.Buffer = (PWSTR) ((PCHAR) input +
                                       input->SymbolicLinkNameOffset);
    deviceName.Length = deviceName.MaximumLength = input->DeviceNameLength;
    deviceName.Buffer = (PWSTR) ((PCHAR) input + input->DeviceNameOffset);

    return MountMgrCreatePointWorker(Extension, &symbolicLinkName, &deviceName);
}

NTSTATUS
QueryPointsFromSymbolicLinkName(
    IN      PDEVICE_EXTENSION   Extension,
    IN      PUNICODE_STRING     SymbolicLinkName,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine queries the mount point information from the
    symbolic link name.

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name.

    Irp                 - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              deviceName;
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY   symEntry;
    ULONG                       len;
    PIO_STACK_LOCATION          irpSp;
    PMOUNTMGR_MOUNT_POINTS      output;

    status = QueryDeviceInformation(SymbolicLinkName, &deviceName, NULL, NULL,
                                    NULL);
    if (NT_SUCCESS(status)) {

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            if (!RtlCompareUnicodeString(&deviceName, &deviceInfo->DeviceName,
                                         TRUE)) {

                break;
            }
        }

        ExFreePool(deviceName.Buffer);

        if (l == &Extension->MountedDeviceList) {
            return STATUS_INVALID_PARAMETER;
        }

        for (l = deviceInfo->SymbolicLinkNames.Flink;
             l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

            symEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY, ListEntry);
            if (RtlEqualUnicodeString(SymbolicLinkName,
                                      &symEntry->SymbolicLinkName, TRUE)) {

                break;
            }
        }

        if (l == &deviceInfo->SymbolicLinkNames) {
            return STATUS_INVALID_PARAMETER;
        }
    } else {

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            for (ll = deviceInfo->SymbolicLinkNames.Flink;
                 ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

                symEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);
                if (RtlEqualUnicodeString(SymbolicLinkName,
                                          &symEntry->SymbolicLinkName, TRUE)) {

                    break;
                }
            }

            if (ll != &deviceInfo->SymbolicLinkNames) {
                break;
            }
        }

        if (l == &Extension->MountedDeviceList) {
            return STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    len = sizeof(MOUNTMGR_MOUNT_POINTS) + symEntry->SymbolicLinkName.Length +
          deviceInfo->DeviceName.Length + deviceInfo->UniqueId->UniqueIdLength;

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    output = Irp->AssociatedIrp.SystemBuffer;
    output->Size = len;
    output->NumberOfMountPoints = 1;
    Irp->IoStatus.Information = len;

    if (len > irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
        Irp->IoStatus.Information = sizeof(MOUNTMGR_MOUNT_POINTS);
        return STATUS_BUFFER_OVERFLOW;
    }

    output->MountPoints[0].SymbolicLinkNameOffset =
            sizeof(MOUNTMGR_MOUNT_POINTS);
    output->MountPoints[0].SymbolicLinkNameLength =
            symEntry->SymbolicLinkName.Length;

    if (symEntry->IsInDatabase) {
        output->MountPoints[0].UniqueIdOffset =
                output->MountPoints[0].SymbolicLinkNameOffset +
                output->MountPoints[0].SymbolicLinkNameLength;
        output->MountPoints[0].UniqueIdLength =
                deviceInfo->UniqueId->UniqueIdLength;
    } else {
        output->MountPoints[0].UniqueIdOffset = 0;
        output->MountPoints[0].UniqueIdLength = 0;
    }

    output->MountPoints[0].DeviceNameOffset =
            output->MountPoints[0].SymbolicLinkNameOffset +
            output->MountPoints[0].SymbolicLinkNameLength +
            output->MountPoints[0].UniqueIdLength;
    output->MountPoints[0].DeviceNameLength = deviceInfo->DeviceName.Length;

    RtlCopyMemory((PCHAR) output +
                  output->MountPoints[0].SymbolicLinkNameOffset,
                  symEntry->SymbolicLinkName.Buffer,
                  output->MountPoints[0].SymbolicLinkNameLength);

    if (symEntry->IsInDatabase) {
        RtlCopyMemory((PCHAR) output + output->MountPoints[0].UniqueIdOffset,
                      deviceInfo->UniqueId->UniqueId,
                      output->MountPoints[0].UniqueIdLength);
    }

    RtlCopyMemory((PCHAR) output + output->MountPoints[0].DeviceNameOffset,
                  deviceInfo->DeviceName.Buffer,
                  output->MountPoints[0].DeviceNameLength);

    return STATUS_SUCCESS;
}

NTSTATUS
QueryPointsFromMemory(
    IN      PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp,
    IN      PMOUNTDEV_UNIQUE_ID UniqueId,
    IN      PUNICODE_STRING     DeviceName
    )

/*++

Routine Description:

    This routine queries the points for the given unique id or device name.

Arguments:

    Extension           - Supplies the device extension.

    Irp                 - Supplies the I/O request packet.

    UniqueId            - Supplies the unique id.

    DeviceName          - Supplies the device name.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status;
    UNICODE_STRING              targetName;
    ULONG                       numPoints, size;
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;
    PIO_STACK_LOCATION          irpSp;
    PMOUNTMGR_MOUNT_POINTS      output;
    ULONG                       offset, uOffset, dOffset;
    USHORT                      uLen, dLen;

    if (DeviceName) {
        status = QueryDeviceInformation(DeviceName, &targetName, NULL, NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    numPoints = 0;
    size = 0;
    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (UniqueId) {

            if (UniqueId->UniqueIdLength ==
                deviceInfo->UniqueId->UniqueIdLength) {

                if (RtlCompareMemory(UniqueId->UniqueId,
                                     deviceInfo->UniqueId->UniqueId,
                                     UniqueId->UniqueIdLength) !=
                    UniqueId->UniqueIdLength) {

                    continue;
                }

            } else {
                continue;
            }

        } else if (DeviceName) {

            if (!RtlEqualUnicodeString(&targetName, &deviceInfo->DeviceName,
                                       TRUE)) {

                continue;
            }
        }

        size += deviceInfo->UniqueId->UniqueIdLength;
        size += deviceInfo->DeviceName.Length;

        for (ll = deviceInfo->SymbolicLinkNames.Flink;
             ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

            symlinkEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);

            numPoints++;
            size += symlinkEntry->SymbolicLinkName.Length;
        }

        if (UniqueId || DeviceName) {
            break;
        }
    }

    if (UniqueId || DeviceName) {
        if (l == &Extension->MountedDeviceList) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    irpSp = IoGetCurrentIrpStackLocation(Irp);
    output = Irp->AssociatedIrp.SystemBuffer;
    output->Size = FIELD_OFFSET(MOUNTMGR_MOUNT_POINTS, MountPoints) +
                   numPoints*sizeof(MOUNTMGR_MOUNT_POINT) + size;
    output->NumberOfMountPoints = numPoints;
    Irp->IoStatus.Information = output->Size;

    if (output->Size > irpSp->Parameters.DeviceIoControl.OutputBufferLength) {
        Irp->IoStatus.Information = sizeof(MOUNTMGR_MOUNT_POINTS);
        if (DeviceName) {
            ExFreePool(targetName.Buffer);
        }
        return STATUS_BUFFER_OVERFLOW;
    }

    numPoints = 0;
    offset = output->Size - size;
    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (UniqueId) {

            if (UniqueId->UniqueIdLength ==
                deviceInfo->UniqueId->UniqueIdLength) {

                if (RtlCompareMemory(UniqueId->UniqueId,
                                     deviceInfo->UniqueId->UniqueId,
                                     UniqueId->UniqueIdLength) !=
                    UniqueId->UniqueIdLength) {

                    continue;
                }

            } else {
                continue;
            }

        } else if (DeviceName) {

            if (!RtlEqualUnicodeString(&targetName, &deviceInfo->DeviceName,
                                       TRUE)) {

                continue;
            }
        }

        uOffset = offset;
        uLen = deviceInfo->UniqueId->UniqueIdLength;
        dOffset = uOffset + uLen;
        dLen = deviceInfo->DeviceName.Length;
        offset += uLen + dLen;

        RtlCopyMemory((PCHAR) output + uOffset, deviceInfo->UniqueId->UniqueId,
                      uLen);
        RtlCopyMemory((PCHAR) output + dOffset, deviceInfo->DeviceName.Buffer,
                      dLen);

        for (ll = deviceInfo->SymbolicLinkNames.Flink;
             ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

            symlinkEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);

            output->MountPoints[numPoints].SymbolicLinkNameOffset = offset;
            output->MountPoints[numPoints].SymbolicLinkNameLength =
                    symlinkEntry->SymbolicLinkName.Length;

            if (symlinkEntry->IsInDatabase) {
                output->MountPoints[numPoints].UniqueIdOffset = uOffset;
                output->MountPoints[numPoints].UniqueIdLength = uLen;
            } else {
                output->MountPoints[numPoints].UniqueIdOffset = 0;
                output->MountPoints[numPoints].UniqueIdLength = 0;
            }

            output->MountPoints[numPoints].DeviceNameOffset = dOffset;
            output->MountPoints[numPoints].DeviceNameLength = dLen;

            RtlCopyMemory((PCHAR) output + offset,
                          symlinkEntry->SymbolicLinkName.Buffer,
                          symlinkEntry->SymbolicLinkName.Length);

            offset += symlinkEntry->SymbolicLinkName.Length;
            numPoints++;
        }

        if (UniqueId || DeviceName) {
            break;
        }
    }

    if (DeviceName) {
        ExFreePool(targetName.Buffer);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrQueryPoints(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine queries a range of mount points.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_MOUNT_POINT   input;
    ULONG                   len1, len2, len3, len;
    UNICODE_STRING          name;
    NTSTATUS                status;
    PMOUNTDEV_UNIQUE_ID     id;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_MOUNT_POINT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;
    if (!input->SymbolicLinkNameLength) {
        input->SymbolicLinkNameOffset = 0;
    }
    if (!input->UniqueIdLength) {
        input->UniqueIdOffset = 0;
    }
    if (!input->DeviceNameLength) {
        input->DeviceNameOffset = 0;
    }
    len1 = input->SymbolicLinkNameOffset + input->SymbolicLinkNameLength;
    len2 = input->UniqueIdOffset + input->UniqueIdLength;
    len3 = input->DeviceNameOffset + input->DeviceNameLength;
    len = len1 > len2 ? len1 : len2;
    len = len > len3 ? len : len3;
    if (len > irpSp->Parameters.DeviceIoControl.InputBufferLength) {
        return STATUS_INVALID_PARAMETER;
    }
    if (irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTMGR_MOUNT_POINTS)) {

        return STATUS_INVALID_PARAMETER;
    }

    if (input->SymbolicLinkNameLength) {

        if (input->SymbolicLinkNameLength > 0xF000) {
            return STATUS_INVALID_PARAMETER;
        }

        name.Length = input->SymbolicLinkNameLength;
        name.MaximumLength = name.Length + sizeof(WCHAR);
        name.Buffer = ExAllocatePool(PagedPool, name.MaximumLength);
        if (!name.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(name.Buffer,
                      (PCHAR) input + input->SymbolicLinkNameOffset,
                      name.Length);
        name.Buffer[name.Length/sizeof(WCHAR)] = 0;

        status = QueryPointsFromSymbolicLinkName(Extension, &name, Irp);

        ExFreePool(name.Buffer);

    } else if (input->UniqueIdLength) {

        id = ExAllocatePool(PagedPool, input->UniqueIdLength + sizeof(USHORT));
        if (!id) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        id->UniqueIdLength = input->UniqueIdLength;
        RtlCopyMemory(id->UniqueId, (PCHAR) input + input->UniqueIdOffset,
                      input->UniqueIdLength);

        status = QueryPointsFromMemory(Extension, Irp, id, NULL);

        ExFreePool(id);

    } else if (input->DeviceNameLength) {

        if (input->DeviceNameLength > 0xF000) {
            return STATUS_INVALID_PARAMETER;
        }

        name.Length = input->DeviceNameLength;
        name.MaximumLength = name.Length + sizeof(WCHAR);
        name.Buffer = ExAllocatePool(PagedPool, name.MaximumLength);
        if (!name.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        RtlCopyMemory(name.Buffer, (PCHAR) input + input->DeviceNameOffset,
                      name.Length);
        name.Buffer[name.Length/sizeof(WCHAR)] = 0;

        status = QueryPointsFromMemory(Extension, Irp, NULL, &name);

        ExFreePool(name.Buffer);

    } else {
        status = QueryPointsFromMemory(Extension, Irp, NULL, NULL);
    }

    return status;
}

VOID
SendLinkDeleted(
    IN  PUNICODE_STRING DeviceName,
    IN  PUNICODE_STRING SymbolicLinkName
    )

/*++

Routine Description:

    This routine alerts the mounted device that one of its links is
    being deleted.

Arguments:

    DeviceName  - Supplies the device name.

    SymbolicLinkName    - Supplies the symbolic link name being deleted.

Return Value:

    None.

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    ULONG               inputSize;
    PMOUNTDEV_NAME      input;
    KEVENT              event;
    PIRP                irp;
    IO_STATUS_BLOCK     ioStatus;
    PIO_STACK_LOCATION  irpSp;

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return;
    }
    deviceObject = IoGetAttachedDeviceReference(fileObject->DeviceObject);

    inputSize = sizeof(USHORT) + SymbolicLinkName->Length;
    input = ExAllocatePool(PagedPool, inputSize);
    if (!input) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }

    input->NameLength = SymbolicLinkName->Length;
    RtlCopyMemory(input->Name, SymbolicLinkName->Buffer,
                  SymbolicLinkName->Length);

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(
          IOCTL_MOUNTDEV_LINK_DELETED, deviceObject, input, inputSize, NULL, 0,
          FALSE, &event, &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return;
    }
    irpSp = IoGetNextIrpStackLocation(irp);
    irpSp->FileObject = fileObject;

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);
    ObDereferenceObject(fileObject);
}

VOID
DeleteSymbolicLinkNameFromMemory(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PUNICODE_STRING     SymbolicLinkName,
    IN  BOOLEAN             DbOnly
    )

/*++

Routine Description:

    This routine deletes the given symbolic link name from memory.

Arguments:

    Extension           - Supplies the device extension.

    SymbolicLinkName    - Supplies the symbolic link name.

    DbOnly              - Supplies whether or not this is DBONLY.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                 l, ll;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    PSYMBOLIC_LINK_NAME_ENTRY   symlinkEntry;

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        for (ll = deviceInfo->SymbolicLinkNames.Flink;
             ll != &deviceInfo->SymbolicLinkNames; ll = ll->Flink) {

            symlinkEntry = CONTAINING_RECORD(ll, SYMBOLIC_LINK_NAME_ENTRY,
                                             ListEntry);

            if (!RtlCompareUnicodeString(SymbolicLinkName,
                                         &symlinkEntry->SymbolicLinkName,
                                         TRUE)) {

                if (DbOnly) {
                    symlinkEntry->IsInDatabase = FALSE;
                } else {

                    SendLinkDeleted(&deviceInfo->NotificationName,
                                    SymbolicLinkName);

                    RemoveEntryList(ll);
                    ExFreePool(symlinkEntry->SymbolicLinkName.Buffer);
                    ExFreePool(symlinkEntry);
                }
                return;
            }
        }
    }
}

NTSTATUS
MountMgrDeletePoints(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine creates a mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION      irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_MOUNT_POINT   point;
    BOOLEAN                 singlePoint;
    NTSTATUS                status;
    PMOUNTMGR_MOUNT_POINTS  points;
    ULONG                   i;
    UNICODE_STRING          symbolicLinkName;
    PMOUNTDEV_UNIQUE_ID     uniqueId;
    UNICODE_STRING          deviceName;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_MOUNT_POINT)) {

        return STATUS_INVALID_PARAMETER;
    }

    point = Irp->AssociatedIrp.SystemBuffer;
    if (point->SymbolicLinkNameOffset && point->SymbolicLinkNameLength) {
        singlePoint = TRUE;
    } else {
        singlePoint = FALSE;
    }

    status = MountMgrQueryPoints(Extension, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    points = Irp->AssociatedIrp.SystemBuffer;
    for (i = 0; i < points->NumberOfMountPoints; i++) {

        symbolicLinkName.Length = points->MountPoints[i].SymbolicLinkNameLength;
        symbolicLinkName.MaximumLength = symbolicLinkName.Length + sizeof(WCHAR);
        symbolicLinkName.Buffer = ExAllocatePool(PagedPool,
                                                 symbolicLinkName.MaximumLength);
        if (!symbolicLinkName.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(symbolicLinkName.Buffer,
                      (PCHAR) points +
                      points->MountPoints[i].SymbolicLinkNameOffset,
                      symbolicLinkName.Length);

        symbolicLinkName.Buffer[symbolicLinkName.Length/sizeof(WCHAR)] = 0;

        if (singlePoint && IsDriveLetter(&symbolicLinkName)) {
            uniqueId = ExAllocatePool(PagedPool,
                                      points->MountPoints[i].UniqueIdLength +
                                      sizeof(MOUNTDEV_UNIQUE_ID));
            if (uniqueId) {
                uniqueId->UniqueIdLength =
                        points->MountPoints[i].UniqueIdLength;
                RtlCopyMemory(uniqueId->UniqueId, (PCHAR) points +
                              points->MountPoints[i].UniqueIdOffset,
                              uniqueId->UniqueIdLength);

                CreateNoDriveLetterEntry(uniqueId);

                ExFreePool(uniqueId);
            }
        }

        if (i == 0 && !singlePoint) {
            uniqueId = ExAllocatePool(PagedPool,
                                      points->MountPoints[i].UniqueIdLength +
                                      sizeof(MOUNTDEV_UNIQUE_ID));
            if (uniqueId) {
                uniqueId->UniqueIdLength =
                        points->MountPoints[i].UniqueIdLength;
                RtlCopyMemory(uniqueId->UniqueId, (PCHAR) points +
                              points->MountPoints[i].UniqueIdOffset,
                              uniqueId->UniqueIdLength);

                DeleteNoDriveLetterEntry(uniqueId);

                ExFreePool(uniqueId);
            }
        }

        IoDeleteSymbolicLink(&symbolicLinkName);
        DeleteSymbolicLinkNameFromMemory(Extension, &symbolicLinkName, FALSE);

        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               symbolicLinkName.Buffer);

        ExFreePool(symbolicLinkName.Buffer);

        deviceName.Length = points->MountPoints[i].DeviceNameLength;
        deviceName.MaximumLength = deviceName.Length;
        deviceName.Buffer = (PWCHAR) ((PCHAR) points +
                                      points->MountPoints[i].DeviceNameOffset);

        MountMgrNotifyNameChange(Extension, &deviceName, TRUE);
    }

    MountMgrNotify(Extension);

    return status;
}

NTSTATUS
MountMgrDeletePointsDbOnly(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine deletes mount points from the database.  It does not
    delete the symbolic links or the in memory representation.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    PMOUNTMGR_MOUNT_POINTS  points;
    ULONG                   i;
    UNICODE_STRING          symbolicLinkName;
    PMOUNTDEV_UNIQUE_ID     uniqueId;

    status = MountMgrQueryPoints(Extension, Irp);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    points = Irp->AssociatedIrp.SystemBuffer;
    for (i = 0; i < points->NumberOfMountPoints; i++) {

        symbolicLinkName.Length = points->MountPoints[i].SymbolicLinkNameLength;
        symbolicLinkName.MaximumLength = symbolicLinkName.Length + sizeof(WCHAR);
        symbolicLinkName.Buffer = ExAllocatePool(PagedPool,
                                                 symbolicLinkName.MaximumLength);
        if (!symbolicLinkName.Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(symbolicLinkName.Buffer,
                      (PCHAR) points +
                      points->MountPoints[i].SymbolicLinkNameOffset,
                      symbolicLinkName.Length);

        symbolicLinkName.Buffer[symbolicLinkName.Length/sizeof(WCHAR)] = 0;

        if (points->NumberOfMountPoints == 1 &&
            IsDriveLetter(&symbolicLinkName)) {

            uniqueId = ExAllocatePool(PagedPool,
                                      points->MountPoints[i].UniqueIdLength +
                                      sizeof(MOUNTDEV_UNIQUE_ID));
            if (uniqueId) {
                uniqueId->UniqueIdLength =
                        points->MountPoints[i].UniqueIdLength;
                RtlCopyMemory(uniqueId->UniqueId, (PCHAR) points +
                              points->MountPoints[i].UniqueIdOffset,
                              uniqueId->UniqueIdLength);

                CreateNoDriveLetterEntry(uniqueId);

                ExFreePool(uniqueId);
            }
        }

        DeleteSymbolicLinkNameFromMemory(Extension, &symbolicLinkName, TRUE);

        RtlDeleteRegistryValue(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY,
                               symbolicLinkName.Buffer);

        ExFreePool(symbolicLinkName.Buffer);
    }

    return status;
}

VOID
ProcessSuggestedDriveLetters(
    IN OUT  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine processes the saved suggested drive letters.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    UNICODE_STRING              symbolicLinkName;
    WCHAR                       symNameBuffer[30];

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (deviceInfo->SuggestedDriveLetter == 0xFF) {

            if (!HasDriveLetter(deviceInfo) &&
                !HasNoDriveLetterEntry(deviceInfo->UniqueId)) {

                CreateNoDriveLetterEntry(deviceInfo->UniqueId);
            }

            deviceInfo->SuggestedDriveLetter = 0;

        } else if (deviceInfo->SuggestedDriveLetter &&
                   !HasNoDriveLetterEntry(deviceInfo->UniqueId)) {

            symbolicLinkName.Length = symbolicLinkName.MaximumLength = 28;
            symbolicLinkName.Buffer = symNameBuffer;
            RtlCopyMemory(symbolicLinkName.Buffer, L"\\DosDevices\\", 24);
            symbolicLinkName.Buffer[12] = deviceInfo->SuggestedDriveLetter;
            symbolicLinkName.Buffer[13] = ':';

            MountMgrCreatePointWorker(Extension, &symbolicLinkName,
                                      &deviceInfo->DeviceName);
        }
    }
}

BOOLEAN
IsFtVolume(
    IN  PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine checks to see if the given volume is an FT volume.

Arguments:

    DeviceName  - Supplies the device name.

Return Value:

    FALSE   - This is not an FT volume.

    TRUE    - This is an FT volume.

--*/

{
    NTSTATUS                status;
    PFILE_OBJECT            fileObject;
    PDEVICE_OBJECT          deviceObject, checkObject;
    KEVENT                  event;
    PIRP                    irp;
    PARTITION_INFORMATION   partInfo;
    IO_STATUS_BLOCK         ioStatus;

    status = IoGetDeviceObjectPointer(DeviceName,
                                      FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }
    checkObject = fileObject->DeviceObject;
    deviceObject = IoGetAttachedDeviceReference(checkObject);

    if (checkObject->Characteristics&FILE_REMOVABLE_MEDIA) {
        ObDereferenceObject(deviceObject);
        ObDereferenceObject(fileObject);
        return FALSE;
    }

    ObDereferenceObject(fileObject);

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_GET_PARTITION_INFO,
                                        deviceObject, NULL, 0, &partInfo,
                                        sizeof(partInfo), FALSE, &event,
                                        &ioStatus);
    if (!irp) {
        ObDereferenceObject(deviceObject);
        return FALSE;
    }

    status = IoCallDriver(deviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    ObDereferenceObject(deviceObject);

    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    if (partInfo.PartitionType&PARTITION_NTFT) {
        return TRUE;
    }

    return FALSE;
}

NTSTATUS
MountMgrNextDriveLetter(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine gives the next available drive letter to the given device
    unless the device already has a drive letter or the device has a flag
    specifying that it should not receive a drive letter.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION                  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_DRIVE_LETTER_TARGET       input;
    UNICODE_STRING                      deviceName, targetName;
    NTSTATUS                            status;
    BOOLEAN                             isRecognized;
    PLIST_ENTRY                         l;
    PMOUNTED_DEVICE_INFORMATION         deviceInfo;
    PMOUNTMGR_DRIVE_LETTER_INFORMATION  output;
    PSYMBOLIC_LINK_NAME_ENTRY           symlinkEntry;
    UNICODE_STRING                      symbolicLinkName, floppyPrefix, cdromPrefix;
    WCHAR                               symNameBuffer[30];
    UCHAR                               startDriveLetterName;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_DRIVE_LETTER_TARGET) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;
    if (input->DeviceNameLength +
        (ULONG) FIELD_OFFSET(MOUNTMGR_DRIVE_LETTER_TARGET, DeviceName) >
        irpSp->Parameters.DeviceIoControl.InputBufferLength) {

        return STATUS_INVALID_PARAMETER;
    }

    if (!Extension->SuggestedDriveLettersProcessed) {
        ProcessSuggestedDriveLetters(Extension);
        Extension->SuggestedDriveLettersProcessed = TRUE;
    }

    deviceName.MaximumLength = deviceName.Length = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    status = QueryDeviceInformation(&deviceName, &targetName, NULL, NULL,
                                    &isRecognized);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        if (!RtlCompareUnicodeString(&targetName, &deviceInfo->DeviceName,
                                     TRUE)) {

            break;
        }
    }

    if (l == &Extension->MountedDeviceList) {
        ExFreePool(targetName.Buffer);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    output = Irp->AssociatedIrp.SystemBuffer;
    output->DriveLetterWasAssigned = TRUE;

    for (l = deviceInfo->SymbolicLinkNames.Flink;
         l != &deviceInfo->SymbolicLinkNames; l = l->Flink) {

        symlinkEntry = CONTAINING_RECORD(l, SYMBOLIC_LINK_NAME_ENTRY,
                                         ListEntry);

        if (IsDriveLetter(&symlinkEntry->SymbolicLinkName)) {
            output->DriveLetterWasAssigned = FALSE;
            output->CurrentDriveLetter =
                    (UCHAR) symlinkEntry->SymbolicLinkName.Buffer[12];
            break;
        }
    }

    if (l == &deviceInfo->SymbolicLinkNames &&
        (!isRecognized || HasNoDriveLetterEntry(deviceInfo->UniqueId))) {

        output->DriveLetterWasAssigned = FALSE;
        output->CurrentDriveLetter = 0;
        Irp->IoStatus.Information = sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION);
        ExFreePool(targetName.Buffer);
        return STATUS_SUCCESS;
    }

    RtlInitUnicodeString(&floppyPrefix, L"\\Device\\Floppy");
    RtlInitUnicodeString(&cdromPrefix, L"\\Device\\CdRom");
    if (RtlPrefixUnicodeString(&floppyPrefix, &targetName, TRUE)) {
        startDriveLetterName = 'A';
    } else if (RtlPrefixUnicodeString(&cdromPrefix, &targetName, TRUE)) {
        startDriveLetterName = 'D';
    } else {
        startDriveLetterName = 'C';
    }

    if (IsNEC_98) {
        UNICODE_STRING  StartDriveLetterFrom;
        UNICODE_STRING  defaultStartDriveLetter;

        //
        //  Determin whether how to drive assign is NEC98 regacy (HD start is A:) or
        //  AT compatible (HD start C:).
        //

        RtlInitUnicodeString(&StartDriveLetterFrom, NULL);
        RtlInitUnicodeString(&defaultStartDriveLetter, NULL);

        {
            RTL_QUERY_REGISTRY_TABLE SetupTypeTable[]=
                {
                  {NULL,
                   RTL_QUERY_REGISTRY_DIRECT,
                   L"DriveLetter",
                   &StartDriveLetterFrom,
                   REG_SZ,
                   &defaultStartDriveLetter,
                   0
                  },
                  {NULL,0,NULL,NULL,REG_NONE,NULL,0}
                };

            status = RtlQueryRegistryValues(
                            RTL_REGISTRY_ABSOLUTE,
                            L"\\Registry\\MACHINE\\SYSTEM\\Setup",
                            SetupTypeTable,
                            NULL,
                            NULL);

            if (!( NT_SUCCESS( status ) &&
                ( (StartDriveLetterFrom.Buffer[0] == L'C') || (StartDriveLetterFrom.Buffer[0] == L'c')))){

                startDriveLetterName = 'A';
            }

        }
        RtlFreeUnicodeString(&StartDriveLetterFrom);

    }

    if (output->DriveLetterWasAssigned) {

        ASSERT(deviceInfo->SuggestedDriveLetter != 0xFF);

        if ((IsNEC_98? HasNoDriveLetterEntry(deviceInfo->UniqueId):
                       !deviceInfo->SuggestedDriveLetter) &&
            IsFtVolume(&deviceInfo->DeviceName)) {

            output->DriveLetterWasAssigned = FALSE;
            output->CurrentDriveLetter = 0;
            Irp->IoStatus.Information = sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION);
            ExFreePool(targetName.Buffer);
            return STATUS_SUCCESS;
        }

        symbolicLinkName.Length = symbolicLinkName.MaximumLength = 28;
        symbolicLinkName.Buffer = symNameBuffer;
        RtlCopyMemory(symbolicLinkName.Buffer, L"\\DosDevices\\", 24);
        symbolicLinkName.Buffer[13] = ':';

        if (deviceInfo->SuggestedDriveLetter) {
            output->CurrentDriveLetter = deviceInfo->SuggestedDriveLetter;
            symbolicLinkName.Buffer[12] = output->CurrentDriveLetter;
            status = MountMgrCreatePointWorker(Extension, &symbolicLinkName,
                                               &targetName);
            if (NT_SUCCESS(status)) {
                Irp->IoStatus.Information =
                        sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION);
                ExFreePool(targetName.Buffer);
                return STATUS_SUCCESS;
            }
        }

        for (output->CurrentDriveLetter = startDriveLetterName;
             output->CurrentDriveLetter <= 'Z';
             output->CurrentDriveLetter++) {

            symbolicLinkName.Buffer[12] = output->CurrentDriveLetter;
            status = MountMgrCreatePointWorker(Extension, &symbolicLinkName,
                                               &targetName);
            if (NT_SUCCESS(status)) {
                break;
            }
        }

        if (output->CurrentDriveLetter > 'Z') {
            output->CurrentDriveLetter = 0;
            output->DriveLetterWasAssigned = FALSE;
        }
    }

    Irp->IoStatus.Information = sizeof(MOUNTMGR_DRIVE_LETTER_INFORMATION);

    ExFreePool(targetName.Buffer);

    return STATUS_SUCCESS;
}

NTSTATUS
MountMgrVolumeMountPointCreated(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp,
    IN      NTSTATUS            ResultOfWaitForDatabase
    )

/*++

Routine Description:

    This routine alerts that mount manager that a volume mount point has
    been created so that the mount manager can replicate the database entry
    for the given mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_VOLUME_MOUNT_POINT    input;
    ULONG                           len1, len2, len;
    UNICODE_STRING                  remoteDatabaseVolumeName;
    HANDLE                          h;
    UNICODE_STRING                  targetVolumeName, otherTargetVolumeName;
    ULONG                           offset;
    BOOLEAN                         entryFound;
    PMOUNTMGR_FILE_ENTRY            databaseEntry;
    PMOUNTDEV_UNIQUE_ID             uniqueId;
    NTSTATUS                        status;
    ULONG                           size;
    UNICODE_STRING                  remoteDatabaseTargetName;
    PLIST_ENTRY                     l;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;
    PREPLICATED_UNIQUE_ID           replUniqueId;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_VOLUME_MOUNT_POINT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;

    len1 = input->SourceVolumeNameOffset + input->SourceVolumeNameLength;
    len2 = input->TargetVolumeNameOffset + input->TargetVolumeNameLength;
    len = len1 > len2 ? len1 : len2;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < len) {
        return STATUS_INVALID_PARAMETER;
    }

    remoteDatabaseVolumeName.Length =
            remoteDatabaseVolumeName.MaximumLength =
            input->SourceVolumeNameLength;
    remoteDatabaseVolumeName.Buffer = (PWSTR) ((PCHAR) input +
                                      input->SourceVolumeNameOffset);

    if (!NT_SUCCESS(ResultOfWaitForDatabase)) {
        status = FindDeviceInfo(Extension, &remoteDatabaseVolumeName,
                                FALSE, &deviceInfo);
        if (NT_SUCCESS(status)) {
            ReconcileThisDatabaseWithMaster(Extension, deviceInfo);
        }
        return status;
    }

    h = OpenRemoteDatabase(&remoteDatabaseVolumeName, TRUE);
    if (!h) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    targetVolumeName.Length = targetVolumeName.MaximumLength =
            input->TargetVolumeNameLength;
    targetVolumeName.Buffer = (PWSTR) ((PCHAR) input +
                              input->TargetVolumeNameOffset);

    offset = 0;
    entryFound = FALSE;
    for (;;) {

        databaseEntry = GetRemoteDatabaseEntry(h, offset);
        if (!databaseEntry) {
            break;
        }

        otherTargetVolumeName.Length = otherTargetVolumeName.MaximumLength =
                databaseEntry->VolumeNameLength;
        otherTargetVolumeName.Buffer = (PWSTR) ((PCHAR) databaseEntry +
                                       databaseEntry->VolumeNameOffset);

        if (RtlEqualUnicodeString(&targetVolumeName, &otherTargetVolumeName,
                                  TRUE)) {

            entryFound = TRUE;
            break;
        }

        offset += databaseEntry->EntryLength;
        ExFreePool(databaseEntry);
    }

    if (entryFound) {

        databaseEntry->RefCount++;
        status = WriteRemoteDatabaseEntry(h, offset, databaseEntry);
        ExFreePool(databaseEntry);

    } else {

        status = QueryDeviceInformation(&targetVolumeName, NULL, &uniqueId,
                                        NULL, NULL);
        if (!NT_SUCCESS(status)) {
            CloseRemoteDatabase(h);
            return status;
        }

        size = sizeof(MOUNTMGR_FILE_ENTRY) + targetVolumeName.Length +
               uniqueId->UniqueIdLength;

        databaseEntry = ExAllocatePool(PagedPool, size);
        if (!databaseEntry) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        databaseEntry->EntryLength = size;
        databaseEntry->RefCount = 1;
        databaseEntry->VolumeNameOffset = sizeof(MOUNTMGR_FILE_ENTRY);
        databaseEntry->VolumeNameLength = targetVolumeName.Length;
        databaseEntry->UniqueIdOffset = databaseEntry->VolumeNameOffset +
                                        databaseEntry->VolumeNameLength;
        databaseEntry->UniqueIdLength = uniqueId->UniqueIdLength;

        RtlCopyMemory((PCHAR) databaseEntry + databaseEntry->VolumeNameOffset,
                      targetVolumeName.Buffer,
                      databaseEntry->VolumeNameLength);
        RtlCopyMemory((PCHAR) databaseEntry + databaseEntry->UniqueIdOffset,
                      uniqueId->UniqueId, databaseEntry->UniqueIdLength);

        status = AddRemoteDatabaseEntry(h, databaseEntry);

        ExFreePool(databaseEntry);

        if (!NT_SUCCESS(status)) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return status;
        }

        status = QueryDeviceInformation(&remoteDatabaseVolumeName,
                                        &remoteDatabaseTargetName, NULL, NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return status;
        }

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            if (RtlEqualUnicodeString(&remoteDatabaseTargetName,
                                      &deviceInfo->DeviceName, TRUE)) {

                break;
            }
        }

        ExFreePool(remoteDatabaseTargetName.Buffer);

        if (l == &Extension->MountedDeviceList) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return STATUS_UNSUCCESSFUL;
        }

        replUniqueId = ExAllocatePool(PagedPool, sizeof(REPLICATED_UNIQUE_ID));
        if (!replUniqueId) {
            ExFreePool(uniqueId);
            CloseRemoteDatabase(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        replUniqueId->UniqueId = uniqueId;

        InsertTailList(&deviceInfo->ReplicatedUniqueIds,
                       &replUniqueId->ListEntry);
    }

    CloseRemoteDatabase(h);

    return status;
}

NTSTATUS
MountMgrVolumeMountPointDeleted(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp,
    IN      NTSTATUS            ResultOfWaitForDatabase
    )

/*++

Routine Description:

    This routine alerts that mount manager that a volume mount point has
    been created so that the mount manager can replicate the database entry
    for the given mount point.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_VOLUME_MOUNT_POINT    input;
    ULONG                           len1, len2, len;
    UNICODE_STRING                  remoteDatabaseVolumeName;
    HANDLE                          h;
    UNICODE_STRING                  targetVolumeName, otherTargetVolumeName;
    ULONG                           offset;
    BOOLEAN                         entryFound;
    PMOUNTMGR_FILE_ENTRY            databaseEntry;
    NTSTATUS                        status;
    UNICODE_STRING                  remoteDatabaseTargetName;
    PLIST_ENTRY                     l;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;
    PREPLICATED_UNIQUE_ID           replUniqueId;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_VOLUME_MOUNT_POINT)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;

    len1 = input->SourceVolumeNameOffset + input->SourceVolumeNameLength;
    len2 = input->TargetVolumeNameOffset + input->TargetVolumeNameLength;
    len = len1 > len2 ? len1 : len2;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < len) {
        return STATUS_INVALID_PARAMETER;
    }

    remoteDatabaseVolumeName.Length =
            remoteDatabaseVolumeName.MaximumLength =
            input->SourceVolumeNameLength;
    remoteDatabaseVolumeName.Buffer = (PWSTR) ((PCHAR) input +
                                      input->SourceVolumeNameOffset);

    if (!NT_SUCCESS(ResultOfWaitForDatabase)) {
        status = FindDeviceInfo(Extension, &remoteDatabaseVolumeName,
                                FALSE, &deviceInfo);
        if (NT_SUCCESS(status)) {
            ReconcileThisDatabaseWithMaster(Extension, deviceInfo);
        }
        return status;
    }

    h = OpenRemoteDatabase(&remoteDatabaseVolumeName, TRUE);
    if (!h) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    targetVolumeName.Length = targetVolumeName.MaximumLength =
            input->TargetVolumeNameLength;
    targetVolumeName.Buffer = (PWSTR) ((PCHAR) input +
                              input->TargetVolumeNameOffset);

    offset = 0;
    entryFound = FALSE;
    for (;;) {

        databaseEntry = GetRemoteDatabaseEntry(h, offset);
        if (!databaseEntry) {
            break;
        }

        otherTargetVolumeName.Length = otherTargetVolumeName.MaximumLength =
                databaseEntry->VolumeNameLength;
        otherTargetVolumeName.Buffer = (PWSTR) ((PCHAR) databaseEntry +
                                       databaseEntry->VolumeNameOffset);

        if (RtlEqualUnicodeString(&targetVolumeName, &otherTargetVolumeName,
                                  TRUE)) {

            entryFound = TRUE;
            break;
        }

        offset += databaseEntry->EntryLength;
        ExFreePool(databaseEntry);
    }

    if (!entryFound) {
        CloseRemoteDatabase(h);
        return STATUS_INVALID_PARAMETER;
    }

    databaseEntry->RefCount--;
    if (databaseEntry->RefCount) {
        status = WriteRemoteDatabaseEntry(h, offset, databaseEntry);
    } else {
        status = DeleteRemoteDatabaseEntry(h, offset);
        if (!NT_SUCCESS(status)) {
            ExFreePool(databaseEntry);
            CloseRemoteDatabase(h);
            return status;
        }

        status = QueryDeviceInformation(&remoteDatabaseVolumeName,
                                        &remoteDatabaseTargetName, NULL, NULL,
                                        NULL);
        if (!NT_SUCCESS(status)) {
            ExFreePool(databaseEntry);
            CloseRemoteDatabase(h);
            return status;
        }

        for (l = Extension->MountedDeviceList.Flink;
             l != &Extension->MountedDeviceList; l = l->Flink) {

            deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                           ListEntry);

            if (RtlEqualUnicodeString(&remoteDatabaseTargetName,
                                      &deviceInfo->DeviceName, TRUE)) {

                break;
            }
        }

        ExFreePool(remoteDatabaseTargetName.Buffer);

        if (l == &Extension->MountedDeviceList) {
            ExFreePool(databaseEntry);
            CloseRemoteDatabase(h);
            return STATUS_UNSUCCESSFUL;
        }

        for (l = deviceInfo->ReplicatedUniqueIds.Flink;
             l != &deviceInfo->ReplicatedUniqueIds; l = l->Flink) {

            replUniqueId = CONTAINING_RECORD(l, REPLICATED_UNIQUE_ID,
                                             ListEntry);

            if (replUniqueId->UniqueId->UniqueIdLength ==
                databaseEntry->UniqueIdLength &&
                RtlCompareMemory(replUniqueId->UniqueId->UniqueId,
                                 (PCHAR) databaseEntry +
                                 databaseEntry->UniqueIdOffset,
                                 databaseEntry->UniqueIdLength) ==
                                 databaseEntry->UniqueIdLength) {

                break;
            }
        }

        if (l == &deviceInfo->ReplicatedUniqueIds) {
            ExFreePool(databaseEntry);
            CloseRemoteDatabase(h);
            return STATUS_UNSUCCESSFUL;
        }

        RemoveEntryList(l);
        ExFreePool(replUniqueId->UniqueId);
        ExFreePool(replUniqueId);
    }

    ExFreePool(databaseEntry);
    CloseRemoteDatabase(h);

    return status;
}

NTSTATUS
MountMgrKeepLinksWhenOffline(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine sets up the internal data structure to remember to keep
    the symbolic links for the given device even when the device goes offline.
    Then when the device becomes on-line again, it is guaranteed that these
    links will be available and not taken by some other device.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_TARGET_NAME       input = Irp->AssociatedIrp.SystemBuffer;
    ULONG                       size;
    UNICODE_STRING              deviceName;
    NTSTATUS                    status;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_TARGET_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    size = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) +
           input->DeviceNameLength;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < size) {
        return STATUS_INVALID_PARAMETER;
    }

    deviceName.Length = deviceName.MaximumLength = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    status = FindDeviceInfo(Extension, &deviceName, FALSE, &deviceInfo);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceInfo->KeepLinksWhenOffline = TRUE;

    return STATUS_SUCCESS;
}

VOID
ReconcileAllDatabasesWithMaster(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine goes through all of the devices known to the MOUNTMGR and
    reconciles their database with the master database.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    OBJECT_ATTRIBUTES           oa;
    NTSTATUS                    status;
    HANDLE                      handle;
    IO_STATUS_BLOCK             ioStatus;

    for (l = Extension->MountedDeviceList.Flink;
         l != &Extension->MountedDeviceList; l = l->Flink) {

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        // Force an open of the volume so that the file system driver
        // is loaded before AUTOCHK ties up the boot volume.

        if (deviceInfo->IsRemovable) {
            continue;
        }

        InitializeObjectAttributes(&oa, &deviceInfo->DeviceName,
                                   OBJ_CASE_INSENSITIVE, 0, 0);

        status = ZwOpenFile(&handle, FILE_GENERIC_READ, &oa, &ioStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_SYNCHRONOUS_IO_ALERT);
        if (NT_SUCCESS(status)) {
            ZwClose(handle);
        }

        ReconcileThisDatabaseWithMaster(Extension, deviceInfo);
    }
}

NTSTATUS
MountMgrCheckUnprocessedVolumes(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine sets up the internal data structure to remember to keep
    the symbolic links for the given device even when the device goes offline.
    Then when the device becomes on-line again, it is guaranteed that these
    links will be available and not taken by some other device.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                    status = STATUS_SUCCESS;
    LIST_ENTRY                  q;
    PLIST_ENTRY                 l;
    PMOUNTED_DEVICE_INFORMATION deviceInfo;
    NTSTATUS                    status2;

    if (IsListEmpty(&Extension->DeadMountedDeviceList)) {
        KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
        return status;
    }

    q = Extension->DeadMountedDeviceList;
    InitializeListHead(&Extension->DeadMountedDeviceList);

    KeReleaseSemaphore(&Extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    q.Blink->Flink = &q;
    q.Flink->Blink = &q;

    while (!IsListEmpty(&q)) {

        l = RemoveHeadList(&q);

        deviceInfo = CONTAINING_RECORD(l, MOUNTED_DEVICE_INFORMATION,
                                       ListEntry);

        status2 = MountMgrMountedDeviceArrival(Extension,
                                               &deviceInfo->NotificationName,
                                               deviceInfo->NotAPdo);

        ExFreePool(deviceInfo->NotificationName.Buffer);
        ExFreePool(deviceInfo);

        if (NT_SUCCESS(status)) {
            status = status2;
        }
    }

    return status;
}

NTSTATUS
MountMgrVolumeArrivalNotification(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine performs the same actions as though PNP had notified
    the mount manager of a new volume arrival.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION          irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_TARGET_NAME       input = Irp->AssociatedIrp.SystemBuffer;
    ULONG                       size;
    UNICODE_STRING              deviceName;
    BOOLEAN                     oldHardErrorMode;
    NTSTATUS                    status;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_TARGET_NAME)) {

        return STATUS_INVALID_PARAMETER;
    }

    size = FIELD_OFFSET(MOUNTMGR_TARGET_NAME, DeviceName) +
           input->DeviceNameLength;
    if (irpSp->Parameters.DeviceIoControl.InputBufferLength < size) {
        return STATUS_INVALID_PARAMETER;
    }

    deviceName.Length = deviceName.MaximumLength = input->DeviceNameLength;
    deviceName.Buffer = input->DeviceName;

    oldHardErrorMode = PsGetCurrentThread()->HardErrorsAreDisabled;
    PsGetCurrentThread()->HardErrorsAreDisabled = TRUE;

    status = MountMgrMountedDeviceArrival(Extension, &deviceName, TRUE);

    PsGetCurrentThread()->HardErrorsAreDisabled = oldHardErrorMode;

    return status;
}

NTSTATUS
MountMgrDeviceControl(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine is the dispatch for a device io control request.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_EXTENSION               extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS                        status, status2;
    PMOUNTED_DEVICE_INFORMATION     deviceInfo;

    Irp->IoStatus.Information = 0;

    KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode, FALSE,
                          NULL);

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_MOUNTMGR_CREATE_POINT:
            status = MountMgrCreatePoint(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_QUERY_POINTS_ADMIN:
        case IOCTL_MOUNTMGR_QUERY_POINTS:
            status = MountMgrQueryPoints(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS:
            status = MountMgrDeletePoints(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_DELETE_POINTS_DBONLY:
            status = MountMgrDeletePointsDbOnly(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_NEXT_DRIVE_LETTER:
            status = MountMgrNextDriveLetter(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_AUTO_DL_ASSIGNMENTS:
            extension->AutomaticDriveLetterAssignment = TRUE;
            ReconcileAllDatabasesWithMaster(extension);
            status = STATUS_SUCCESS;
            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_CREATED:
            KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            status2 = WaitForRemoteDatabaseSemaphore(extension);
            KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);
            status = MountMgrVolumeMountPointCreated(extension, Irp, status2);
            if (NT_SUCCESS(status2)) {
                ReleaseRemoteDatabaseSemaphore(extension);
            }
            break;

        case IOCTL_MOUNTMGR_VOLUME_MOUNT_POINT_DELETED:
            KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            status2 = WaitForRemoteDatabaseSemaphore(extension);
            KeWaitForSingleObject(&extension->Mutex, Executive, KernelMode,
                                  FALSE, NULL);
            status = MountMgrVolumeMountPointDeleted(extension, Irp, status2);
            if (NT_SUCCESS(status2)) {
                ReleaseRemoteDatabaseSemaphore(extension);
            }
            break;

        case IOCTL_MOUNTMGR_CHANGE_NOTIFY:
            status = MountMgrChangeNotify(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_KEEP_LINKS_WHEN_OFFLINE:
            status = MountMgrKeepLinksWhenOffline(extension, Irp);
            break;

        case IOCTL_MOUNTMGR_CHECK_UNPROCESSED_VOLUMES:
            status = MountMgrCheckUnprocessedVolumes(extension, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        case IOCTL_MOUNTMGR_VOLUME_ARRIVAL_NOTIFICATION:
            KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);
            status = MountMgrVolumeArrivalNotification(extension, Irp);
            Irp->IoStatus.Status = status;
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            return status;

        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;

    }

    KeReleaseSemaphore(&extension->Mutex, IO_NO_INCREMENT, 1, FALSE);

    if (status != STATUS_PENDING) {
        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return status;
}


#ifdef ALLOC_PRAGMA
#pragma code_seg()
#endif

VOID
WorkerThread(
    IN  PVOID   Extension
    )

/*++

Routine Description:

    This is a worker thread to process work queue items.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    PDEVICE_EXTENSION   extension = Extension;
    UNICODE_STRING      volumeSafeEventName;
    OBJECT_ATTRIBUTES   oa;
    KEVENT              event;
    LARGE_INTEGER       timeout;
    ULONG               i;
    NTSTATUS            status;
    HANDLE              volumeSafeEvent;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PWORK_QUEUE_ITEM    queueItem;

    RtlInitUnicodeString(&volumeSafeEventName,
                         L"\\Device\\VolumesSafeForWriteAccess");
    InitializeObjectAttributes(&oa, &volumeSafeEventName,
                               OBJ_CASE_INSENSITIVE, NULL, NULL);
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    timeout.QuadPart = -10*1000*1000;   // 1 second

    for (i = 0; i < 1000; i++) {
        status = ZwOpenEvent(&volumeSafeEvent, EVENT_ALL_ACCESS, &oa);
        if (NT_SUCCESS(status)) {
            break;
        }
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, &timeout);
    }

    if (i < 1000) {
        ZwWaitForSingleObject(volumeSafeEvent, FALSE, NULL);
        ZwClose(volumeSafeEvent);
    }

    for (;;) {

        KeWaitForSingleObject(&extension->WorkerSemaphore,
                              Executive, KernelMode, FALSE, NULL);

        KeAcquireSpinLock(&extension->WorkerSpinLock, &irql);
        ASSERT(!IsListEmpty(&extension->WorkerQueue));
        l = RemoveHeadList(&extension->WorkerQueue);
        KeReleaseSpinLock(&extension->WorkerSpinLock, irql);

        queueItem = CONTAINING_RECORD(l, WORK_QUEUE_ITEM, List);
        queueItem->WorkerRoutine(queueItem->Parameter);

        ExFreePool(queueItem);

        if (InterlockedDecrement(&extension->WorkerRefCount) < 0) {
            break;
        }
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS
QueueWorkItem(
    IN  PDEVICE_EXTENSION   Extension,
    IN  PWORK_QUEUE_ITEM    WorkItem
    )

/*++

Routine Description:

    This routine queues the given work item to the worker thread and if
    necessary starts the worker thread.

Arguments:

    Extension   - Supplies the device extension.

    WorkItem    - Supplies the work item to be queued.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              handle;
    KIRQL               irql;

    if (!InterlockedIncrement(&Extension->WorkerRefCount)) {

        InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

        status = PsCreateSystemThread(&handle, 0, &oa, 0, NULL, WorkerThread,
                                      Extension);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        ZwClose(handle);
    }

    KeAcquireSpinLock(&Extension->WorkerSpinLock, &irql);
    InsertTailList(&Extension->WorkerQueue, &WorkItem->List);
    KeReleaseSpinLock(&Extension->WorkerSpinLock, irql);

    KeReleaseSemaphore(&Extension->WorkerSemaphore, 0, 1, FALSE);

    return STATUS_SUCCESS;
}

VOID
MountMgrNotify(
    IN  PDEVICE_EXTENSION   Extension
    )

/*++

Routine Description:

    This routine completes all of the change notify irps in the queue.

Arguments:

    Extension   - Supplies the device extension.

Return Value:

    None.

--*/

{
    LIST_ENTRY                      q;
    KIRQL                           irql;
    PLIST_ENTRY                     p;
    PIRP                            irp;
    PMOUNTMGR_CHANGE_NOTIFY_INFO    output;

    Extension->EpicNumber++;

    InitializeListHead(&q);
    IoAcquireCancelSpinLock(&irql);
    while (!IsListEmpty(&Extension->ChangeNotifyIrps)) {
        p = RemoveHeadList(&Extension->ChangeNotifyIrps);
        irp = CONTAINING_RECORD(p, IRP, Tail.Overlay.ListEntry);
        IoSetCancelRoutine(irp, NULL);
        InsertTailList(&q, p);
    }
    IoReleaseCancelSpinLock(irql);

    while (!IsListEmpty(&q)) {
        p = RemoveHeadList(&q);
        irp = CONTAINING_RECORD(p, IRP, Tail.Overlay.ListEntry);
        output = irp->AssociatedIrp.SystemBuffer;
        output->EpicNumber = Extension->EpicNumber;
        irp->IoStatus.Information = sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO);
        IoCompleteRequest(irp, IO_NO_INCREMENT);
    }
}

VOID
MountMgrCancel(
    IN OUT  PDEVICE_OBJECT  DeviceObject,
    IN OUT  PIRP            Irp
    )

/*++

Routine Description:

    This routine is called on when the given IRP is cancelled.  It
    will dequeue this IRP off the work queue and complete the
    request as CANCELLED.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the IRP.

Return Value:

    None.

--*/

{
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    IoReleaseCancelSpinLock(Irp->CancelIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

NTSTATUS
MountMgrChangeNotify(
    IN OUT  PDEVICE_EXTENSION   Extension,
    IN OUT  PIRP                Irp
    )

/*++

Routine Description:

    This routine returns when the current Epic number is different than
    the one given.

Arguments:

    Extension   - Supplies the device extension.

    Irp         - Supplies the I/O request packet.

Return Value:

    NTSTATUS

--*/

{
    PIO_STACK_LOCATION              irpSp = IoGetCurrentIrpStackLocation(Irp);
    PMOUNTMGR_CHANGE_NOTIFY_INFO    input;
    KIRQL                           irql;

    if (irpSp->Parameters.DeviceIoControl.InputBufferLength <
        sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO) ||
        irpSp->Parameters.DeviceIoControl.OutputBufferLength <
        sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO)) {

        return STATUS_INVALID_PARAMETER;
    }

    input = Irp->AssociatedIrp.SystemBuffer;
    if (input->EpicNumber != Extension->EpicNumber) {
        input->EpicNumber = Extension->EpicNumber;
        Irp->IoStatus.Information = sizeof(MOUNTMGR_CHANGE_NOTIFY_INFO);
        return STATUS_SUCCESS;
    }

    IoAcquireCancelSpinLock(&irql);
    if (Irp->Cancel) {
        IoReleaseCancelSpinLock(irql);
        return STATUS_CANCELLED;
    }

    InsertTailList(&Extension->ChangeNotifyIrps, &Irp->Tail.Overlay.ListEntry);
    IoMarkIrpPending(Irp);
    IoSetCancelRoutine(Irp, MountMgrCancel);
    IoReleaseCancelSpinLock(irql);

    return STATUS_PENDING;
}

NTSTATUS
UniqueIdChangeNotifyCompletion(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp,
    IN  PVOID           WorkItem
    )

/*++

Routine Description:

    Completion routine for a change notify.

Arguments:

    DeviceObject    - Not used.

    Irp             - Supplies the IRP.

    Extension       - Supplies the work item.


Return Value:

    STATUS_MORE_PROCESSING_REQUIRED

--*/

{
    PCHANGE_NOTIFY_WORK_ITEM    workItem = WorkItem;

    if (!NT_SUCCESS(Irp->IoStatus.Status)) {
        IoFreeIrp(workItem->Irp);
        ExFreePool(workItem->DeviceName.Buffer);
        ExFreePool(workItem->SystemBuffer);
        ExFreePool(workItem);
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    ExInitializeWorkItem(&workItem->WorkItem, UniqueIdChangeNotifyWorker,
                         workItem);

    ExQueueWorkItem(&workItem->WorkItem, DelayedWorkQueue);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
MountMgrCleanup(
    IN  PDEVICE_OBJECT  DeviceObject,
    IN  PIRP            Irp
    )

/*++

Routine Description:

    This routine cancels all of the IRPs currently queued on
    the given device.

Arguments:

    DeviceObject    - Supplies the device object.

    Irp             - Supplies the cleanup IRP.

Return Value:

    STATUS_SUCCESS  - Success.

--*/

{
    PDEVICE_EXTENSION   Extension = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION  irpSp = IoGetCurrentIrpStackLocation(Irp);
    PFILE_OBJECT        file = irpSp->FileObject;
    KIRQL               irql;
    PLIST_ENTRY         l;
    PIRP                irp;

    IoAcquireCancelSpinLock(&irql);

    for (;;) {

        for (l = Extension->ChangeNotifyIrps.Flink;
             l != &Extension->ChangeNotifyIrps; l = l->Flink) {

            irp = CONTAINING_RECORD(l, IRP, Tail.Overlay.ListEntry);
            if (IoGetCurrentIrpStackLocation(irp)->FileObject == file) {
                break;
            }
        }

        if (l == &Extension->ChangeNotifyIrps) {
            break;
        }

        irp->Cancel = TRUE;
        irp->CancelIrql = irql;
        irp->CancelRoutine = NULL;
        MountMgrCancel(DeviceObject, irp);

        IoAcquireCancelSpinLock(&irql);
    }

    IoReleaseCancelSpinLock(irql);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine is the entry point for the driver.

Arguments:

    DriverObject    - Supplies the driver object.

    RegistryPath    - Supplies the registry path for this driver.

Return Value:

    NTSTATUS

--*/

{
    UNICODE_STRING      deviceName, symbolicLinkName;
    NTSTATUS            status;
    PDEVICE_OBJECT      deviceObject;
    PDEVICE_EXTENSION   extension;

    RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, MOUNTED_DEVICES_KEY);

    RtlInitUnicodeString(&deviceName, MOUNTMGR_DEVICE_NAME);
    status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION),
                            &deviceName, FILE_DEVICE_NETWORK, 0, FALSE,
                            &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    extension = deviceObject->DeviceExtension;
    RtlZeroMemory(extension, sizeof(DEVICE_EXTENSION));
    extension->DeviceObject = deviceObject;
    extension->DriverObject = DriverObject;
    InitializeListHead(&extension->MountedDeviceList);
    InitializeListHead(&extension->DeadMountedDeviceList);
    KeInitializeSemaphore(&extension->Mutex, 1, 1);
    KeInitializeSemaphore(&extension->RemoteDatabaseSemaphore, 1, 1);
    InitializeListHead(&extension->ChangeNotifyIrps);
    extension->EpicNumber = 1;
    InitializeListHead(&extension->SavedLinksList);
    InitializeListHead(&extension->WorkerQueue);
    KeInitializeSemaphore(&extension->WorkerSemaphore, 0, MAXLONG);
    extension->WorkerRefCount = -1;
    KeInitializeSpinLock(&extension->WorkerSpinLock);

    RtlInitUnicodeString(&symbolicLinkName, L"\\DosDevices\\MountPointManager");
    IoCreateSymbolicLink(&symbolicLinkName, &deviceName);

    ObReferenceObject(DriverObject);

    status = IoRegisterPlugPlayNotification(
             EventCategoryDeviceInterfaceChange,
             PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
             (PVOID) &MOUNTDEV_MOUNTED_DEVICE_GUID, DriverObject,
             MountMgrMountedDeviceNotification, extension,
             &extension->NotificationEntry);

    if (!NT_SUCCESS(status)) {
        ObDereferenceObject(DriverObject);
        return status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = MountMgrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = MountMgrCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MountMgrDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = MountMgrCleanup;

    return STATUS_SUCCESS;
}
