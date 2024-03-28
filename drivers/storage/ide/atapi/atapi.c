/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver entrypoint and utility functions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "atapi.h"

/* GLOBALS ********************************************************************/

UNICODE_STRING AtapDriverRegistryPath;
BOOLEAN AtapInPEMode;

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
NTSTATUS
AtaOpenRegistryKey(
    _In_ PHANDLE KeyHandle,
    _In_ HANDLE RootKey,
    _In_ PUNICODE_STRING KeyName,
    _In_ BOOLEAN Create)
{
    NTSTATUS Status;
    ULONG Disposition;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    InitializeObjectAttributes(&ObjectAttributes,
                               KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RootKey,
                               NULL);

    if (Create)
    {
        Status = ZwCreateKey(KeyHandle,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &Disposition);
    }
    else
    {
        Status = ZwOpenKey(KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
    }

    return Status;
}

CODE_SEG("PAGE")
VOID
AtaGetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PCWSTR KeyName,
    _Out_ PULONG KeyValue,
    _In_ ULONG DefaultValue)
{
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(*KeyValue)];
    HANDLE KeyHandle;
    UNICODE_STRING ValueName;
    NTSTATUS Status;
    ULONG ResultLength;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)Buffer;

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(ChannelExtension->Ldo,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_ALL_ACCESS,
                                     &KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to open device software key, status 0x%lx\n", Status);

        *KeyValue = DefaultValue;
        return;
    }

    RtlInitUnicodeString(&ValueName, KeyName);
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(Buffer),
                             &ResultLength);
    if (!NT_SUCCESS(Status) ||
        (PartialInfo->Type != REG_DWORD) ||
        (PartialInfo->DataLength != sizeof(*KeyValue)))
    {
        TRACE("Failed to read '%wZ' key, status 0x%lx\n", &ValueName, Status);

        *KeyValue = DefaultValue;
        return;
    }

    *KeyValue = *(PULONG)&PartialInfo->Data;
}

CODE_SEG("PAGE")
VOID
AtaSetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue)
{
    HANDLE KeyHandle;
    UNICODE_STRING ValueName;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(ChannelExtension->Ldo,
                                     PLUGPLAY_REGKEY_DRIVER,
                                     KEY_ALL_ACCESS,
                                     &KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to open device software key, status 0x%lx\n", Status);
        return;
    }

    RtlInitUnicodeString(&ValueName, KeyName);
    Status = ZwSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &KeyValue,
                           sizeof(KeyValue));
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to set '%wZ' key, status 0x%lx\n", &ValueName, Status);
    }
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaDispatchCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
NTAPI
AtaUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    RtlFreeUnicodeString(&AtapDriverRegistryPath);
}

static
NTSTATUS
AtaPdoPower(
    _In_ PATAPORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;

    Status = Irp->IoStatus.Status;
    PoStartNextPowerIrp(Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static
NTSTATUS
AtaFdoPower(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension,
    _Inout_ PIRP Irp)
{
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(ChannelExtension->Ldo, Irp);
}

NTSTATUS
NTAPI
AtaDispatchPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    if (IS_FDO(DeviceObject->DeviceExtension))
        return AtaFdoPower(DeviceObject->DeviceExtension, Irp);
    else
        return AtaPdoPower(DeviceObject->DeviceExtension, Irp);
}

static
CODE_SEG("PAGE")
VOID
AtaFdoQueryLocation(
    _In_ PATAPORT_CHANNEL_EXTENSION ChannelExtension)
{
    NTSTATUS Status;
    ULONG PropertyValue, Length;

    PAGED_CODE();

    /*
     * Determine the IDE channel number that will be used as PathId value.
     * This property is available only for PCI IDE channels.
     */
    Status = IoGetDeviceProperty(ChannelExtension->Ldo,
                                 DevicePropertyAddress,
                                 sizeof(PropertyValue),
                                 &PropertyValue,
                                 &Length);
    if (NT_SUCCESS(Status))
        ChannelExtension->PathId = (UCHAR)PropertyValue;
    else
        ChannelExtension->PathId = (UCHAR)-1;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaAddChannel(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PATAPORT_CHANNEL_EXTENSION *FdoExtension)
{
    NTSTATUS Status;
    PDEVICE_OBJECT Fdo;
    UNICODE_STRING DeviceName;
    PATAPORT_CHANNEL_EXTENSION ChannelExtension;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\IdePort999")];
    static ULONG IdeChannelNumber = 0;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                L"\\Device\\Ide\\IdePort%lu",
                                IdeChannelNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    INFO("%s(%p, %p) '%wZ'\n", __FUNCTION__, DriverObject, PhysicalDeviceObject, &DeviceName);

    Status = IoCreateDevice(DriverObject,
                            sizeof(*ChannelExtension),
                            &DeviceName,
                            FILE_DEVICE_CONTROLLER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create the FDO with status 0x%lx\n", Status);
        goto Failure;
    }

    ChannelExtension = Fdo->DeviceExtension;
    *FdoExtension = ChannelExtension;

    RtlZeroMemory(ChannelExtension, sizeof(*ChannelExtension));
    ChannelExtension->Common.Signature = ATAPORT_FDO_SIGNATURE;
    ChannelExtension->Common.Self = Fdo;

    ChannelExtension->Ldo = IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);
    if (!ChannelExtension->Ldo)
    {
        ERR("Failed to attach the FDO\n");
        Status = STATUS_DEVICE_REMOVED;
        goto Failure;
    }

    ChannelExtension->ChannelNumber = IdeChannelNumber++;

    /* DMA buffers alignment */
    Fdo->AlignmentRequirement =
        max(ChannelExtension->Ldo->AlignmentRequirement, FILE_WORD_ALIGNMENT);

    IoInitializeRemoveLock(&ChannelExtension->RemoveLock, IDEPORT_TAG, 0, 0);
    KeInitializeSpinLock(&ChannelExtension->QueueLock);
    KeInitializeSpinLock(&ChannelExtension->ChannelLock);
    ExInitializeFastMutex(&ChannelExtension->DeviceSyncMutex);
    KeInitializeTimer(&ChannelExtension->PollingTimer);
    KeInitializeTimer(&ChannelExtension->RetryTimer);
    KeInitializeDpc(&ChannelExtension->PollingTimerDpc, AtaPollingTimerDpc, ChannelExtension);
    KeInitializeDpc(&ChannelExtension->Dpc, AtaDpc, ChannelExtension);
    InitializeListHead(&ChannelExtension->RequestList);

    AtaFdoQueryLocation(ChannelExtension);

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

Failure:
    if (ChannelExtension->Ldo)
        IoDetachDevice(ChannelExtension->Ldo);

    IoDeleteDevice(Fdo);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PVOID Dummy;

    PAGED_CODE();

    return AtaAddChannel(DriverObject, PhysicalDeviceObject, (PVOID)&Dummy);
}

static
CODE_SEG("INIT")
BOOLEAN
AtaInPEMode(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    NTSTATUS Status;
    UNICODE_STRING KeyName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\MiniNT");

    PAGED_CODE();

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&KeyHandle, KEY_READ, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        ZwClose(KeyHandle);
        return TRUE;
    }

    return FALSE;
}

static
CODE_SEG("INIT")
VOID
AtaCreateIdeDirectory(VOID)
{
    HANDLE Handle;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING DirectoryName = RTL_CONSTANT_STRING(L"\\Device\\Ide");

    PAGED_CODE();

    InitializeObjectAttributes(&ObjectAttributes,
                               &DirectoryName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateDirectoryObject(&Handle, DIRECTORY_ALL_ACCESS, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* We don't need a handle for a permanent object */
        ZwClose(Handle);
    }
    /*
     * Ignore directory creation failures (don't report them as a driver initialization error)
     * as the directory may have already been created by another driver.
     * We will handle fatal errors later via IoCreateDevice() call.
     */
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{

    /* Make a copy of the registry path */
    AtapDriverRegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    AtapDriverRegistryPath.Buffer =
        ExAllocatePoolUninitialized(NonPagedPool,
                                    AtapDriverRegistryPath.MaximumLength,
                                    IDEPORT_TAG);
    if (!AtapDriverRegistryPath.Buffer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyUnicodeString(&AtapDriverRegistryPath, RegistryPath);
    AtapDriverRegistryPath.Buffer[RegistryPath->Length / sizeof(WCHAR)] = UNICODE_NULL;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = AtaDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = AtaDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = AtaDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_SCSI] = AtaDispatchScsi;
    DriverObject->MajorFunction[IRP_MJ_POWER] = AtaDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = AtaDispatchWmi;
    DriverObject->MajorFunction[IRP_MJ_PNP] = AtaDispatchPnp;
    DriverObject->DriverExtension->AddDevice = AtaAddDevice;
    DriverObject->DriverUnload = AtaUnload;

    /* Create a directory to hold the driver's device objects */
    AtaCreateIdeDirectory();

    AtapInPEMode = AtaInPEMode();

#if defined(ATA_DETECT_LEGACY_DEVICES)
    AtaDetectLegacyDevices(DriverObject);
#endif

    return STATUS_SUCCESS;
}
