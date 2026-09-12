/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Synthetic mountdev used to exercise MountMgr persisted-link recovery
 * COPYRIGHT:   Copyright 2026 Alejandro Sánchez <alesangreat@gmail.com>
 */

#include <kmt_test.h>
#include <mountdev.h>
#include <mountmgr.h>

#include "MountMgrVolume.h"

#define NDEBUG
#include <debug.h>

static PDEVICE_OBJECT FakeDeviceObject;
static UNICODE_STRING FakeDeviceName;
static MOUNTMGR_VOLUME_TEST_INFO TestInfo;

static const UNICODE_STRING DevicePrefix = RTL_CONSTANT_STRING(L"\\Device\\MountMgrTest-");
static const UNICODE_STRING VolumePrefix = RTL_CONSTANT_STRING(L"\\??\\Volume");
static const UNICODE_STRING SentinelPrefix = RTL_CONSTANT_STRING(L"#MountMgrTest-");

static KMT_IRP_HANDLER MountDevIrpHandler;
static KMT_MESSAGE_HANDLER QueryInfoHandler;

static
NTSTATUS
BuildTestName(
    _Out_writes_bytes_(BufferSize) PWCHAR Buffer,
    _In_ SIZE_T BufferSize,
    _In_ PCUNICODE_STRING Prefix,
    _In_ PCUNICODE_STRING Suffix,
    _Out_opt_ PUNICODE_STRING Result)
{
    SIZE_T Length = Prefix->Length + Suffix->Length;

    if (Length + sizeof(UNICODE_NULL) > BufferSize)
        return STATUS_BUFFER_TOO_SMALL;

    RtlCopyMemory(Buffer, Prefix->Buffer, Prefix->Length);
    RtlCopyMemory((PUCHAR)Buffer + Prefix->Length, Suffix->Buffer, Suffix->Length);
    Buffer[Length / sizeof(WCHAR)] = UNICODE_NULL;

    if (Result)
    {
        Result->Buffer = Buffer;
        Result->Length = (USHORT)Length;
        Result->MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
CompleteNameQuery(
    _Inout_ PIRP Irp,
    _In_ ULONG OutputLength)
{
    ULONG RequiredLength;
    PMOUNTDEV_NAME Name = Irp->AssociatedIrp.SystemBuffer;

    if (OutputLength < FIELD_OFFSET(MOUNTDEV_NAME, Name))
        return STATUS_BUFFER_TOO_SMALL;

    Name->NameLength = FakeDeviceName.Length;
    Irp->IoStatus.Information = FIELD_OFFSET(MOUNTDEV_NAME, Name);

    RequiredLength = FIELD_OFFSET(MOUNTDEV_NAME, Name) + FakeDeviceName.Length;
    if (OutputLength < RequiredLength)
        return STATUS_BUFFER_OVERFLOW;

    RtlCopyMemory(Name->Name, FakeDeviceName.Buffer, FakeDeviceName.Length);
    Irp->IoStatus.Information = RequiredLength;
    return STATUS_SUCCESS;
}

static
NTSTATUS
CompleteUniqueIdQuery(
    _Inout_ PIRP Irp,
    _In_ ULONG OutputLength)
{
    ULONG RequiredLength;
    PMOUNTDEV_UNIQUE_ID UniqueId = Irp->AssociatedIrp.SystemBuffer;

    if (OutputLength < FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId))
        return STATUS_BUFFER_TOO_SMALL;

    UniqueId->UniqueIdLength = sizeof(TestInfo.UniqueId);
    Irp->IoStatus.Information = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId);

    RequiredLength = FIELD_OFFSET(MOUNTDEV_UNIQUE_ID, UniqueId) + sizeof(TestInfo.UniqueId);
    if (OutputLength < RequiredLength)
        return STATUS_BUFFER_OVERFLOW;

    RtlCopyMemory(UniqueId->UniqueId, &TestInfo.UniqueId, sizeof(TestInfo.UniqueId));
    Irp->IoStatus.Information = RequiredLength;
    return STATUS_SUCCESS;
}

static
NTSTATUS
MountDevIrpHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStackLocation)
{
    NTSTATUS Status;
    ULONG IoControlCode;
    ULONG OutputLength;

    ok_eq_pointer(DeviceObject, FakeDeviceObject);

    IoControlCode = IoStackLocation->Parameters.DeviceIoControl.IoControlCode;
    OutputLength = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;
    Irp->IoStatus.Information = 0;

    switch (IoControlCode)
    {
        case IOCTL_MOUNTDEV_QUERY_DEVICE_NAME:
            Status = CompleteNameQuery(Irp, OutputLength);
            break;

        case IOCTL_MOUNTDEV_QUERY_UNIQUE_ID:
            Status = CompleteUniqueIdQuery(Irp, OutputLength);
            break;

        case IOCTL_MOUNTDEV_QUERY_STABLE_GUID:
        case IOCTL_MOUNTDEV_QUERY_SUGGESTED_LINK_NAME:
            Status = STATUS_NOT_SUPPORTED;
            break;

        case IOCTL_MOUNTDEV_LINK_CREATED:
        case IOCTL_MOUNTDEV_LINK_DELETED:
            Status = STATUS_SUCCESS;
            break;

        default:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static
NTSTATUS
QueryInfoHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG ControlCode,
    _Inout_updates_bytes_opt_(*OutLength) PVOID Buffer,
    _In_ SIZE_T InLength,
    _Inout_ PSIZE_T OutLength)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (ControlCode != MOUNTMGR_VOLUME_QUERY_INFO || InLength != 0)
        return STATUS_INVALID_PARAMETER;

    if (!Buffer || *OutLength < sizeof(TestInfo))
    {
        *OutLength = sizeof(TestInfo);
        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlCopyMemory(Buffer, &TestInfo, sizeof(TestInfo));
    *OutLength = sizeof(TestInfo);
    return STATUS_SUCCESS;
}

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    NTSTATUS Status;
    UNICODE_STRING GuidString;

    UNREFERENCED_PARAMETER(RegistryPath);
    UNREFERENCED_PARAMETER(Flags);

    *DeviceName = L"MountMgrVolume";
    RtlZeroMemory(&TestInfo, sizeof(TestInfo));

    Status = ExUuidCreate(&TestInfo.UniqueId);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = RtlStringFromGUID(&TestInfo.UniqueId, &GuidString);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = BuildTestName(TestInfo.DeviceName,
                           sizeof(TestInfo.DeviceName),
                           &DevicePrefix,
                           &GuidString,
                           &FakeDeviceName);
    if (NT_SUCCESS(Status))
    {
        Status = BuildTestName(TestInfo.VolumeName,
                               sizeof(TestInfo.VolumeName),
                               &VolumePrefix,
                               &GuidString,
                               NULL);
    }
    if (NT_SUCCESS(Status))
    {
        Status = BuildTestName(TestInfo.SentinelName,
                               sizeof(TestInfo.SentinelName),
                               &SentinelPrefix,
                               &GuidString,
                               NULL);
    }

    RtlFreeUnicodeString(&GuidString);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = IoCreateDevice(DriverObject,
                            0,
                            &FakeDeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &FakeDeviceObject);
    if (!NT_SUCCESS(Status))
        return Status;

    FakeDeviceObject->Flags |= DO_BUFFERED_IO;
    FakeDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    Status = KmtRegisterIrpHandler(IRP_MJ_DEVICE_CONTROL,
                                   FakeDeviceObject,
                                   MountDevIrpHandler);
    if (!NT_SUCCESS(Status))
        goto Failure;

    Status = KmtRegisterMessageHandler(MOUNTMGR_VOLUME_QUERY_INFO,
                                       NULL,
                                       QueryInfoHandler);
    if (!NT_SUCCESS(Status))
    {
        KmtUnregisterIrpHandler(IRP_MJ_DEVICE_CONTROL,
                                FakeDeviceObject,
                                MountDevIrpHandler);
        goto Failure;
    }

    return STATUS_SUCCESS;

Failure:
    IoDeleteDevice(FakeDeviceObject);
    FakeDeviceObject = NULL;
    return Status;
}

VOID
TestUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    KmtUnregisterMessageHandler(MOUNTMGR_VOLUME_QUERY_INFO,
                                NULL,
                                QueryInfoHandler);

    if (FakeDeviceObject)
    {
        KmtUnregisterIrpHandler(IRP_MJ_DEVICE_CONTROL,
                                FakeDeviceObject,
                                MountDevIrpHandler);
        IoDeleteDevice(FakeDeviceObject);
        FakeDeviceObject = NULL;
    }
}
