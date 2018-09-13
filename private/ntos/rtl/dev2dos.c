/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dev2dos.c

Abstract:

    This module implements the device object to DOS name routine.

Author:

    Norbert Kusters (norbertk)  21-Oct-1998

Environment:

    Kernel Mode.

Revision History:

--*/

#include <ntrtlp.h>
#include <mountdev.h>

#ifdef POOL_TAGGING
#undef ExAllocatePool
#undef ExAllocatePoolWithQuota
#define ExAllocatePool(a,b) ExAllocatePoolWithTag(a,b,' d2D')
#define ExAllocatePoolWithQuota(a,b) ExAllocatePoolWithQuotaTag(a,b,' d2D')
#endif

NTSTATUS
QuerySymbolicLink(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING LinkTarget
    );

NTSTATUS
QueryDeviceNameForPath(
    IN  PUNICODE_STRING Path,
    OUT PUNICODE_STRING DeviceName
    );

NTSTATUS
OpenDeviceReparseIndex(
    IN  PUNICODE_STRING DeviceName,
    OUT PHANDLE         Handle
    );

BOOLEAN
IsVolumeName(
    IN  PUNICODE_STRING Name
    );

NTSTATUS
GetNextReparseVolumePath(
    IN  HANDLE          Handle,
    OUT PUNICODE_STRING Path
    );

NTSTATUS
FindPathForDevice(
    IN      PUNICODE_STRING StartingPath,
    IN      PUNICODE_STRING DeviceName,
    IN OUT  PLIST_ENTRY     DevicesInPath,
    OUT     PUNICODE_STRING FinalPath
    );

NTSTATUS
RtlVolumeDeviceToDosName(
    IN  PVOID           VolumeDeviceObject,
    OUT PUNICODE_STRING DosName
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,QuerySymbolicLink)
#pragma alloc_text(PAGE,QueryDeviceNameForPath)
#pragma alloc_text(PAGE,OpenDeviceReparseIndex)
#pragma alloc_text(PAGE,IsVolumeName)
#pragma alloc_text(PAGE,GetNextReparseVolumePath)
#pragma alloc_text(PAGE,FindPathForDevice)
#pragma alloc_text(PAGE,RtlVolumeDeviceToDosName)
#endif

typedef struct _DEVICE_NAME_ENTRY {
    LIST_ENTRY      ListEntry;
    UNICODE_STRING  DeviceName;
} DEVICE_NAME_ENTRY, *PDEVICE_NAME_ENTRY;

NTSTATUS
QuerySymbolicLink(
    IN  PUNICODE_STRING SymbolicLinkName,
    OUT PUNICODE_STRING LinkTarget
    )

/*++

Routine Description:

    This routine returns the target of the symbolic link name.

Arguments:

    SymbolicLinkName    - Supplies the symbolic link name.

    LinkTarget          - Returns the link target.

Return Value:

    NTSTATUS

--*/

{
    OBJECT_ATTRIBUTES   oa;
    NTSTATUS            status;
    HANDLE              h;

    InitializeObjectAttributes(&oa, SymbolicLinkName, OBJ_CASE_INSENSITIVE,
                               0, 0);

    status = ZwOpenSymbolicLinkObject(&h, GENERIC_READ, &oa);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    LinkTarget->MaximumLength = 200*sizeof(WCHAR);
    LinkTarget->Length = 0;
    LinkTarget->Buffer = ExAllocatePool(PagedPool, LinkTarget->MaximumLength);
    if (!LinkTarget->Buffer) {
        ZwClose(h);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwQuerySymbolicLinkObject(h, LinkTarget, NULL);
    ZwClose(h);

    if (!NT_SUCCESS(status)) {
        ExFreePool(LinkTarget->Buffer);
    }

    return status;
}

NTSTATUS
QueryDeviceNameForPath(
    IN  PUNICODE_STRING Path,
    OUT PUNICODE_STRING DeviceName
    )

/*++

Routine Description:

    This routine returns the device name for the given path.  It first checks
    to see if the path is a symbolic link and then checks to see if it is a
    volume reparse point.

Arguments:

    Path        - Supplies the path.

    DeviceName  - Returns the device name.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                status;
    OBJECT_ATTRIBUTES       oa;
    HANDLE                  h;
    IO_STATUS_BLOCK         ioStatus;
    PREPARSE_DATA_BUFFER    reparse;
    UNICODE_STRING          volumeName;

    status = QuerySymbolicLink(Path, DeviceName);
    if (NT_SUCCESS(status)) {
        return status;
    }

    InitializeObjectAttributes(&oa, Path, OBJ_CASE_INSENSITIVE, 0, 0);
    status = ZwOpenFile(&h, SYNCHRONIZE | FILE_GENERIC_READ, &oa, &ioStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    reparse = ExAllocatePool(PagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    if (!reparse) {
        ZwClose(h);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                             FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                             MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
    ZwClose(h);
    if (!NT_SUCCESS(status)) {
        ExFreePool(reparse);
        return status;
    }

    volumeName.Length = reparse->MountPointReparseBuffer.SubstituteNameLength -
                        sizeof(WCHAR);
    volumeName.MaximumLength = volumeName.Length + sizeof(WCHAR);
    volumeName.Buffer = (PWCHAR)
                        ((PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                        reparse->MountPointReparseBuffer.SubstituteNameOffset);
    volumeName.Buffer[volumeName.Length/sizeof(WCHAR)] = 0;

    status = QuerySymbolicLink(&volumeName, DeviceName);
    ExFreePool(reparse);

    return status;
}

NTSTATUS
OpenDeviceReparseIndex(
    IN  PUNICODE_STRING DeviceName,
    OUT PHANDLE         Handle
    )

/*++

Routine Description:

    This routine opens the reparse index on the given device.

Arguments:

    DeviceName  - Supplies the device name.

    Handle      - Returns the handle.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS            status;
    PFILE_OBJECT        fileObject;
    PDEVICE_OBJECT      deviceObject;
    UNICODE_STRING      reparseSuffix, reparseName;
    OBJECT_ATTRIBUTES   oa;
    IO_STATUS_BLOCK     ioStatus;

    status = IoGetDeviceObjectPointer(DeviceName, FILE_READ_ATTRIBUTES,
                                      &fileObject, &deviceObject);
    if (!NT_SUCCESS(status)) {
        return status;
    }
    deviceObject = fileObject->DeviceObject;

    if (!deviceObject->Vpb || !(deviceObject->Vpb->Flags&VPB_MOUNTED)) {
        ObDereferenceObject(fileObject);
        return STATUS_UNSUCCESSFUL;
    }

    ObDereferenceObject(fileObject);

    RtlInitUnicodeString(&reparseSuffix,
                         L"\\$Extend\\$Reparse:$R:$INDEX_ALLOCATION");
    reparseName.Length = DeviceName->Length + reparseSuffix.Length;
    reparseName.MaximumLength = reparseName.Length + sizeof(WCHAR);
    reparseName.Buffer = ExAllocatePool(PagedPool, reparseName.MaximumLength);
    if (!reparseName.Buffer) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory(reparseName.Buffer, DeviceName->Buffer, DeviceName->Length);
    RtlCopyMemory((PCHAR) reparseName.Buffer + DeviceName->Length,
                  reparseSuffix.Buffer, reparseSuffix.Length);
    reparseName.Buffer[reparseName.Length/sizeof(WCHAR)] = 0;

    InitializeObjectAttributes(&oa, &reparseName, OBJ_CASE_INSENSITIVE, 0, 0);
    status = ZwOpenFile(Handle, SYNCHRONIZE | FILE_LIST_DIRECTORY, &oa,
                        &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_ALERT |
                        FILE_OPEN_FOR_BACKUP_INTENT);

    ExFreePool(reparseName.Buffer);

    return status;
}

BOOLEAN
IsVolumeName(
    IN  PUNICODE_STRING Name
    )

{
    if (Name->Length == 96 &&
        Name->Buffer[0] == '\\' &&
        Name->Buffer[1] == '?' &&
        Name->Buffer[2] == '?' &&
        Name->Buffer[3] == '\\' &&
        Name->Buffer[4] == 'V' &&
        Name->Buffer[5] == 'o' &&
        Name->Buffer[6] == 'l' &&
        Name->Buffer[7] == 'u' &&
        Name->Buffer[8] == 'm' &&
        Name->Buffer[9] == 'e' &&
        Name->Buffer[10] == '{' &&
        Name->Buffer[19] == '-' &&
        Name->Buffer[24] == '-' &&
        Name->Buffer[29] == '-' &&
        Name->Buffer[34] == '-' &&
        Name->Buffer[47] == '}') {

        return TRUE;
    }

    return FALSE;
}

NTSTATUS
GetNextReparseVolumePath(
    IN  HANDLE          Handle,
    OUT PUNICODE_STRING Path
    )

/*++

Routine Description:

    This routine queries the reparse index for the next volume mount point.

Arguments:

    Handle  - Supplies the handle.

    Path    - Returns the path.


Return Value:

    NTSTATUS

--*/

{
    NTSTATUS                        status;
    IO_STATUS_BLOCK                 ioStatus;
    FILE_REPARSE_POINT_INFORMATION  reparseInfo;
    UNICODE_STRING                  fileId;
    OBJECT_ATTRIBUTES               oa;
    HANDLE                          h;
    PREPARSE_DATA_BUFFER            reparse;
    UNICODE_STRING                  volumeName;
    ULONG                           nameInfoSize;
    PFILE_NAME_INFORMATION          nameInfo;

    for (;;) {

        status = ZwQueryDirectoryFile(Handle, NULL, NULL, NULL, &ioStatus,
                                      &reparseInfo, sizeof(reparseInfo),
                                      FileReparsePointInformation, TRUE, NULL,
                                      FALSE);
        if (!NT_SUCCESS(status)) {
            return status;
        }

        if (reparseInfo.Tag != IO_REPARSE_TAG_MOUNT_POINT) {
            continue;
        }

        fileId.Length = sizeof(reparseInfo.FileReference);
        fileId.MaximumLength = fileId.Length;
        fileId.Buffer = (PWSTR) &reparseInfo.FileReference;

        InitializeObjectAttributes(&oa, &fileId, 0, Handle, NULL);

        status = ZwOpenFile(&h, SYNCHRONIZE | FILE_GENERIC_READ, &oa,
                            &ioStatus, FILE_SHARE_READ | FILE_SHARE_WRITE,
                            FILE_OPEN_BY_FILE_ID | FILE_OPEN_REPARSE_POINT |
                            FILE_SYNCHRONOUS_IO_ALERT);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        reparse = ExAllocatePool(PagedPool, MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        if (!reparse) {
            ZwClose(h);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        status = ZwFsControlFile(h, NULL, NULL, NULL, &ioStatus,
                                 FSCTL_GET_REPARSE_POINT, NULL, 0, reparse,
                                 MAXIMUM_REPARSE_DATA_BUFFER_SIZE);
        if (!NT_SUCCESS(status)) {
            ExFreePool(reparse);
            ZwClose(h);
            continue;
        }

        volumeName.Length = reparse->MountPointReparseBuffer.SubstituteNameLength -
                            sizeof(WCHAR);
        volumeName.MaximumLength = volumeName.Length + sizeof(WCHAR);
        volumeName.Buffer = (PWCHAR)
                            ((PCHAR) reparse->MountPointReparseBuffer.PathBuffer +
                            reparse->MountPointReparseBuffer.SubstituteNameOffset);
        volumeName.Buffer[volumeName.Length/sizeof(WCHAR)] = 0;

        if (!IsVolumeName(&volumeName)) {
            ExFreePool(reparse);
            ZwClose(h);
            continue;
        }

        ExFreePool(reparse);

        nameInfoSize = 1024;
        nameInfo = ExAllocatePool(PagedPool, nameInfoSize);
        if (!nameInfo) {
            ZwClose(h);
            continue;
        }

        status = ZwQueryInformationFile(h, &ioStatus, nameInfo, nameInfoSize,
                                        FileNameInformation);
        ZwClose(h);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        Path->Length = (USHORT) nameInfo->FileNameLength;
        Path->MaximumLength = Path->Length + sizeof(WCHAR);
        Path->Buffer = ExAllocatePool(PagedPool, Path->MaximumLength);
        if (!Path->Buffer) {
            ExFreePool(nameInfo);
            continue;
        }

        RtlCopyMemory(Path->Buffer, nameInfo->FileName, Path->Length);
        Path->Buffer[Path->Length/sizeof(WCHAR)] = 0;

        ExFreePool(nameInfo);
        break;
    }

    return status;
}

NTSTATUS
FindPathForDevice(
    IN      PUNICODE_STRING StartingPath,
    IN      PUNICODE_STRING DeviceName,
    IN OUT  PLIST_ENTRY     DevicesInPath,
    OUT     PUNICODE_STRING FinalPath
    )

/*++

Routine Description:

    This routine finds a path (if any) to the given device that begins
    with the given starting path.  The final path returned includes the
    starting path.

Arguments:

    StartingPath    - Supplies the path to begin the search on.

    DeviceName      - Supplies the device name whose path we are searching for.

    DevicesInPath   - Supplies a list of the devices in all proper prefixes
                            of the path.

    FinalPath       - Returns the final path to the given device.

Return Value:

    NTSTATUS

--*/

{
    NTSTATUS            status;
    UNICODE_STRING      startingDeviceName;
    PLIST_ENTRY         l;
    PDEVICE_NAME_ENTRY  entry;
    HANDLE              h;
    UNICODE_STRING      path, newStart;

    status = QueryDeviceNameForPath(StartingPath, &startingDeviceName);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (RtlEqualUnicodeString(DeviceName, &startingDeviceName, TRUE)) {
        ExFreePool(startingDeviceName.Buffer);
        FinalPath->Length = StartingPath->Length;
        FinalPath->MaximumLength = FinalPath->Length + sizeof(WCHAR);
        FinalPath->Buffer = ExAllocatePool(PagedPool,
                                           FinalPath->MaximumLength);
        if (!FinalPath->Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        RtlCopyMemory(FinalPath->Buffer, StartingPath->Buffer,
                      FinalPath->Length);
        FinalPath->Buffer[FinalPath->Length/sizeof(WCHAR)] = 0;

        return STATUS_SUCCESS;
    }

    for (l = DevicesInPath->Flink; l != DevicesInPath; l = l->Flink) {
        entry = CONTAINING_RECORD(l, DEVICE_NAME_ENTRY, ListEntry);
        if (RtlEqualUnicodeString(&entry->DeviceName, &startingDeviceName,
                                  TRUE)) {

            ExFreePool(startingDeviceName.Buffer);
            return STATUS_UNSUCCESSFUL;
        }
    }

    status = OpenDeviceReparseIndex(&startingDeviceName, &h);
    if (!NT_SUCCESS(status)) {
        ExFreePool(startingDeviceName.Buffer);
        return status;
    }

    entry = ExAllocatePool(PagedPool, sizeof(DEVICE_NAME_ENTRY));
    if (!entry) {
        ZwClose(h);
        ExFreePool(startingDeviceName.Buffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    entry->DeviceName = startingDeviceName;
    InsertTailList(DevicesInPath, &entry->ListEntry);

    for (;;) {

        status = GetNextReparseVolumePath(h, &path);
        if (!NT_SUCCESS(status)) {
            break;
        }
        newStart.Length = StartingPath->Length + path.Length;
        newStart.MaximumLength = newStart.Length + sizeof(WCHAR);
        newStart.Buffer = ExAllocatePool(PagedPool, newStart.MaximumLength);
        if (!newStart.Buffer) {
            ExFreePool(path.Buffer);
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        RtlCopyMemory(newStart.Buffer, StartingPath->Buffer,
                      StartingPath->Length);
        RtlCopyMemory((PCHAR) newStart.Buffer + StartingPath->Length,
                      path.Buffer, path.Length);
        newStart.Buffer[newStart.Length/sizeof(WCHAR)] = 0;
        ExFreePool(path.Buffer);

        status = FindPathForDevice(&newStart, DeviceName, DevicesInPath,
                                   FinalPath);
        ExFreePool(newStart.Buffer);
        if (NT_SUCCESS(status)) {
            break;
        }
    }

    RemoveEntryList(&entry->ListEntry);
    ExFreePool(entry);
    ZwClose(h);
    ExFreePool(startingDeviceName.Buffer);

    return status;
}

NTSTATUS
RtlVolumeDeviceToDosName(
    IN  PVOID           VolumeDeviceObject,
    OUT PUNICODE_STRING DosName
    )

/*++

Routine Description:

    This routine returns a valid DOS path for the given device object.
    This caller of this routine must call ExFreePool on DosName->Buffer
    when it is no longer needed.

Arguments:

    VolumeDeviceObject  - Supplies the volume device object.

    DosName             - Returns the DOS name for the volume

Return Value:

    NTSTATUS

--*/

{
    PDEVICE_OBJECT  volumeDeviceObject = VolumeDeviceObject;
    PMOUNTDEV_NAME  name;
    CHAR            output[512];
    KEVENT          event;
    PIRP            irp;
    IO_STATUS_BLOCK ioStatus;
    NTSTATUS        status;
    UNICODE_STRING  deviceName;
    WCHAR           buffer[30];
    UNICODE_STRING  driveLetterName;
    WCHAR           c;
    UNICODE_STRING  linkTarget;
    LIST_ENTRY      devicesInPath;

    name = (PMOUNTDEV_NAME) output;
    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                        volumeDeviceObject, NULL, 0, name, 512,
                                        FALSE, &event, &ioStatus);
    if (!irp) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoCallDriver(volumeDeviceObject, irp);
    if (status == STATUS_PENDING) {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        status = ioStatus.Status;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    deviceName.MaximumLength = deviceName.Length = name->NameLength;
    deviceName.Buffer = name->Name;

    swprintf(buffer, L"\\??\\C:");
    RtlInitUnicodeString(&driveLetterName, buffer);

    for (c = 'A'; c <= 'Z'; c++) {
        driveLetterName.Buffer[4] = c;

        status = QuerySymbolicLink(&driveLetterName, &linkTarget);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        if (RtlEqualUnicodeString(&linkTarget, &deviceName, TRUE)) {
            ExFreePool(linkTarget.Buffer);
            break;
        }

        ExFreePool(linkTarget.Buffer);
    }

    if (c <= 'Z') {
        DosName->Buffer = ExAllocatePool(PagedPool, 3*sizeof(WCHAR));
        if (!DosName->Buffer) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        DosName->MaximumLength = 6;
        DosName->Length = 4;
        DosName->Buffer[0] = c;
        DosName->Buffer[1] = ':';
        DosName->Buffer[2] = 0;
        return STATUS_SUCCESS;
    }

    for (c = 'A'; c <= 'Z'; c++) {
        driveLetterName.Buffer[4] = c;
        InitializeListHead(&devicesInPath);
        status = FindPathForDevice(&driveLetterName, &deviceName,
                                   &devicesInPath, DosName);

        if (NT_SUCCESS(status)) {
            DosName->Length -= 4*sizeof(WCHAR);
            RtlMoveMemory(DosName->Buffer, &DosName->Buffer[4],
                          DosName->Length);
            DosName->Buffer[DosName->Length/sizeof(WCHAR)] = 0;
            return status;
        }
    }

    return status;
}
