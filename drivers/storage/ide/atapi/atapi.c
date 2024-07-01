/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver entrypoint and utility functions
 * COPYRIGHT:   Copyright 2024 Dmitry Borisov <di.sean@protonmail.com>
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
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PCWSTR KeyName,
    _Out_ PULONG KeyValue,
    _In_ ULONG DefaultValue)
{
    UCHAR Buffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(*KeyValue)];
    HANDLE HwKeyHandle, TargetKeyHandle;
    UNICODE_STRING ValueName, TargetKeyName;
    NTSTATUS Status;
    WCHAR TargetKeyBuffer[sizeof("Target99")];
    ULONG ResultLength;
    PKEY_VALUE_PARTIAL_INFORMATION PartialInfo = (PVOID)Buffer;

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(ChanExt->Ldo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &HwKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to open device hardware key, status 0x%lx\n", Status);
        return;
    }

    /* Open or create the 'TargetX' key */
    Status = RtlStringCbPrintfW(TargetKeyBuffer,
                                sizeof(TargetKeyBuffer),
                                L"Target%u",
                                DevExt->AtaScsiAddress.TargetId);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&TargetKeyName, TargetKeyBuffer);
    Status = AtaOpenRegistryKey(&TargetKeyHandle, HwKeyHandle, &TargetKeyName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to create '%wZ' key, status 0x%lx\n", &TargetKeyName, Status);
        goto Cleanup;
    }

    RtlInitUnicodeString(&ValueName, KeyName);
    Status = ZwQueryValueKey(TargetKeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             PartialInfo,
                             sizeof(Buffer),
                             &ResultLength);
    ZwClose(TargetKeyHandle);
    if (!NT_SUCCESS(Status) ||
        (PartialInfo->Type != REG_DWORD) ||
        (PartialInfo->DataLength != sizeof(*KeyValue)))
    {
        TRACE("Failed to read '%wZ' key, status 0x%lx\n", &ValueName, Status);

        *KeyValue = DefaultValue;
        goto Cleanup;
    }

    *KeyValue = *(PULONG)&PartialInfo->Data;

Cleanup:
    ZwClose(HwKeyHandle);
}

CODE_SEG("PAGE")
VOID
AtaSetRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue)
{
    HANDLE HwKeyHandle, TargetKeyHandle;
    UNICODE_STRING ValueName, TargetKeyName;
    NTSTATUS Status;
    WCHAR TargetKeyBuffer[sizeof("Target99")];

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(ChanExt->Ldo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &HwKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to open device hardware key, status 0x%lx\n", Status);
        return;
    }

    /* Open or create the 'TargetX' key */
    Status = RtlStringCbPrintfW(TargetKeyBuffer,
                                sizeof(TargetKeyBuffer),
                                L"Target%u",
                                DevExt->AtaScsiAddress.TargetId);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&TargetKeyName, TargetKeyBuffer);
    Status = AtaOpenRegistryKey(&TargetKeyHandle, HwKeyHandle, &TargetKeyName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to create '%wZ' key, status 0x%lx\n", &TargetKeyName, Status);
        goto Cleanup;
    }

    RtlInitUnicodeString(&ValueName, KeyName);
    Status = ZwSetValueKey(TargetKeyHandle,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &KeyValue,
                           sizeof(KeyValue));
    ZwClose(TargetKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to set '%wZ' key, status 0x%lx\n", &ValueName, Status);
    }

Cleanup:
    ZwClose(HwKeyHandle);
}

CODE_SEG("PAGE")
VOID
AtaWait(
    _In_ ULONG MicroSeconds)
{
    KTIMER Timer;
    LARGE_INTEGER DueTime;

    PAGED_CODE();

    DueTime.QuadPart = UInt32x32To64(MicroSeconds, -100);

    KeInitializeTimer(&Timer);
    KeSetTimer(&Timer, DueTime, NULL);
    KeWaitForSingleObject(&Timer, Executive, KernelMode, FALSE, NULL);
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
    _In_ PATAPORT_DEVICE_EXTENSION DevExt,
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
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _Inout_ PIRP Irp)
{
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(ChanExt->Ldo, Irp);
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

CODE_SEG("PAGE")
NTSTATUS
AtaFdoQueryInterface(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Size)
{
    KEVENT Event;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    PAGED_CODE();

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       ChanExt->Ldo,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatusBlock);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IoStack->Parameters.QueryInterface.InterfaceType = Guid;
    IoStack->Parameters.QueryInterface.Size = Size;
    IoStack->Parameters.QueryInterface.Version = 1;
    IoStack->Parameters.QueryInterface.Interface = Interface;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(ChanExt->Ldo, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

static
CODE_SEG("PAGE")
BOOLEAN
AtaFdoDetectAhciController(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_HEADER, BaseClass)];
    PPCI_COMMON_HEADER PciConfig = (PPCI_COMMON_HEADER)Buffer; // Partial PCI header
    ULONG BytesRead;
    NTSTATUS Status;

    Status = AtaFdoQueryInterface(ChanExt,
                                  &GUID_BUS_INTERFACE_STANDARD,
                                  &ChanExt->BusInterface,
                                  sizeof(BUS_INTERFACE_STANDARD));
    if (!NT_SUCCESS(Status))
    {
        TRACE("No bus interface 0x%lx\n", Status);
        return FALSE;
    }

    BytesRead = (*ChanExt->BusInterface.GetBusData)(ChanExt->BusInterface.Context,
                                                         PCI_WHICHSPACE_CONFIG,
                                                         Buffer,
                                                         0,
                                                         sizeof(Buffer));
    if (BytesRead != sizeof(Buffer))
    {
        ERR("Unable to retrieve the configuration info\n");
        return FALSE;
    }

    ChanExt->DeviceID = PciConfig->DeviceID;
    ChanExt->VendorID = PciConfig->VendorID;

    return (PciConfig->SubClass == PCI_SUBCLASS_MSC_AHCI_CTLR);
}

/* static */
CODE_SEG("PAGE")
VOID
AtaFdoQueryLocation(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt)
{
    NTSTATUS Status;
    ULONG PropertyValue, Length;

    PAGED_CODE();

    /*
     * Determine the IDE channel number that will be used as PathId value.
     * This property is available only for PCI IDE channels.
     */
    Status = IoGetDeviceProperty(ChanExt->Ldo,
                                 DevicePropertyAddress,
                                 sizeof(PropertyValue),
                                 &PropertyValue,
                                 &Length);
    if (NT_SUCCESS(Status))
        ChanExt->PathId = (UCHAR)PropertyValue;
    else
        ChanExt->PathId = (UCHAR)-1;
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
    PATAPORT_CHANNEL_EXTENSION ChanExt;
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
                            sizeof(*ChanExt),
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

    ChanExt = Fdo->DeviceExtension;

    if (FdoExtension)
    {
        *FdoExtension = ChanExt;
    }

    RtlZeroMemory(ChanExt, sizeof(*ChanExt));
    ChanExt->Common.Flags = DO_IS_FDO;
    ChanExt->Common.Self = Fdo;

    ChanExt->Ldo = IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);
    if (!ChanExt->Ldo)
    {
        ERR("Failed to attach the FDO\n");
        Status = STATUS_DEVICE_REMOVED;
        goto Failure;
    }

    ChanExt->ChannelNumber = IdeChannelNumber++;

    /* DMA buffers alignment */
    Fdo->AlignmentRequirement =
        max(ChanExt->Ldo->AlignmentRequirement, FILE_WORD_ALIGNMENT);

    IoInitializeRemoveLock(&ChanExt->Common.RemoveLock, ATAPORT_TAG, 0, 0);
    ExInitializeFastMutex(&ChanExt->DeviceSyncMutex);
    InitializeListHead(&ChanExt->RequestList);

    if (!AtaFdoDetectAhciController(ChanExt))
    {
        ERR("IDE (PATA) devices are not supported yet\n");
        return STATUS_NOT_SUPPORTED;
    }

    ChanExt->Common.Flags |= DO_IS_AHCI;


    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

Failure:
    if (ChanExt->Ldo)
        IoDetachDevice(ChanExt->Ldo);

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
    PAGED_CODE();

    return AtaAddChannel(DriverObject, PhysicalDeviceObject, NULL);
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
                                    ATAPORT_TAG);
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
