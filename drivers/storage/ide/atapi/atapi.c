/*
 * PROJECT:     ReactOS ATA Port Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver entrypoint and utility functions
 * COPYRIGHT:   Copyright 2024-2025 Dmitry Borisov <di.sean@protonmail.com>
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
    _Out_ PHANDLE KeyHandle,
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
    _In_ UCHAR TargetId,
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

    Status = IoOpenDeviceRegistryKey(ChanExt->Pdo,
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
                                TargetId);
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
    _In_ UCHAR TargetId,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue)
{
    HANDLE HwKeyHandle, TargetKeyHandle;
    UNICODE_STRING ValueName, TargetKeyName;
    NTSTATUS Status;
    WCHAR TargetKeyBuffer[sizeof("Target99")];

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(ChanExt->Pdo,
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
                                TargetId);
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
AtaSetPortRegistryKey(
    _In_ PATAPORT_CHANNEL_EXTENSION ChanExt,
    _In_ PCWSTR KeyName,
    _In_ ULONG KeyValue)
{
    HANDLE HwKeyHandle;
    UNICODE_STRING ValueName;
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoOpenDeviceRegistryKey(ChanExt->Pdo,
                                     PLUGPLAY_REGKEY_DEVICE,
                                     KEY_ALL_ACCESS,
                                     &HwKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Failed to open device hardware key, status 0x%lx\n", Status);
        return;
    }

    RtlInitUnicodeString(&ValueName, KeyName);
    Status = ZwSetValueKey(HwKeyHandle,
                           &ValueName,
                           0,
                           REG_DWORD,
                           &KeyValue,
                           sizeof(KeyValue));
    ZwClose(HwKeyHandle);
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

VOID
NTAPI
AtaStorageNotificationWorker(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_opt_ PVOID Context)
{
    PIO_WORKITEM WorkItem = Context;
    PDEVICE_OBJECT TopDeviceObject;
    PIRP Irp;
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    STORAGE_EVENT_NOTIFICATION EventObject;
    NTSTATUS Status;

    TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    EventObject.Version = STORAGE_EVENT_NOTIFICATION_VERSION_V1;
    EventObject.Size = sizeof(EventObject);
    EventObject.Events = STORAGE_EVENT_ALL;

    Irp = IoBuildDeviceIoControlRequest(IOCTL_STORAGE_EVENT_NOTIFICATION,
                                        TopDeviceObject,
                                        &EventObject,
                                        sizeof(EventObject),
                                        NULL,
                                        0,
                                        FALSE,
                                        &Event,
                                        &IoStatus);
    if (!Irp)
    {
        ERR("IoBuildDeviceIoControlRequest() failed\n");
        goto Exit;
    }

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Suspended, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    TRACE("Notification result %08lx\n", Status);

Exit:
    ObDereferenceObject(TopDeviceObject);
    IoFreeWorkItem(WorkItem);
}

VOID
NTAPI
AtaStorageNotificationlDpc(
    _In_ PKDPC Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    PATAPORT_PORT_DATA PortData = DeferredContext;
    ULONG DeviceBitmap = PtrToUlong(SystemArgument1);
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    ULONG i;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument2);

    ChanExt = CONTAINING_RECORD(PortData, ATAPORT_CHANNEL_EXTENSION, PortData);

    for (i = 0; i < ATA_MAX_DEVICE; ++i)
    {
        PATAPORT_DEVICE_EXTENSION DevExt;
        PIO_WORKITEM WorkItem;

        if (!(DeviceBitmap & (1 << i)))
            continue;

        DevExt = AtaFdoFindDeviceByPath(ChanExt,
                                        AtaMarshallScsiAddress(PortData->PortNumber, i, 0),
                                        AtaStorageNotificationlDpc);
        if (!DevExt)
            continue;

        WorkItem = IoAllocateWorkItem(DevExt->Common.Self);
        if (!WorkItem)
        {
            ERR("Failed to allocate workitem");
        }
        else
        {
            IoQueueWorkItem(WorkItem, AtaStorageNotificationWorker, CriticalWorkQueue, WorkItem);
        }

        IoReleaseRemoveLock(&DevExt->Common.RemoveLock, AtaStorageNotificationlDpc);
    }
}

CODE_SEG("PAGE")
NTSTATUS
AtaPnpQueryInterface(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Version,
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
                                       CommonExt->LowerDeviceObject,
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
    IoStack->Parameters.QueryInterface.Version = Version;
    IoStack->Parameters.QueryInterface.Interface = Interface;
    IoStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(CommonExt->LowerDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatusBlock.Status;
    }

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
AtaPnpRepeatRequest(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PIRP Irp,
    _In_opt_ PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PATAPORT_COMMON_EXTENSION FdoExt = CommonExt->FdoExt;
    PDEVICE_OBJECT TopDeviceObject;
    PIO_STACK_LOCATION IoStack, SubStack;
    PIRP SubIrp;
    KEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();
    ASSERT(!IS_FDO(CommonExt));

    TopDeviceObject = IoGetAttachedDeviceReference(FdoExt->Self);

    SubIrp = IoAllocateIrp(TopDeviceObject->StackSize, FALSE);
    if (!SubIrp)
    {
        ObDereferenceObject(TopDeviceObject);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    SubStack = IoGetNextIrpStackLocation(SubIrp);
    RtlCopyMemory(SubStack, IoStack, sizeof(*SubStack));

    if (DeviceCapabilities)
        SubStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    IoSetCompletionRoutine(SubIrp,
                           AtaPdoCompletionRoutine,
                           &Event,
                           TRUE,
                           TRUE,
                           TRUE);

    SubIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(TopDeviceObject, SubIrp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
    }

    ObDereferenceObject(TopDeviceObject);

    Status = SubIrp->IoStatus.Status;
    IoFreeIrp(SubIrp);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
AtaPnpQueryDeviceUsageNotification(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    if (IS_FDO(CommonExt))
    {
        Status = AtaPnpRepeatRequest(CommonExt, Irp, NULL);
    }
    else
    {
        if (!NT_VERIFY(IoForwardIrpSynchronously(CommonExt->LowerDeviceObject, Irp)))
            return STATUS_UNSUCCESSFUL;
        Status = Irp->IoStatus.Status;
    }
    if (!NT_SUCCESS(Status))
        return Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    switch (IoStack->Parameters.UsageNotification.Type)
    {
        case DeviceUsageTypePaging:
            Counter = &CommonExt->PageFiles;
            break;

        case DeviceUsageTypeHibernation:
            Counter = &CommonExt->HibernateFiles;
            break;

        case DeviceUsageTypeDumpFile:
            Counter = &CommonExt->DumpFiles;
            break;

        default:
            return Status;
    }

    IoAdjustPagingPathCount(Counter, IoStack->Parameters.UsageNotification.InPath);

    if (!IS_FDO(CommonExt))
        IoInvalidateDeviceState(CommonExt->Self);

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
AtaPnpQueryPnpDeviceState(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (CommonExt->PageFiles || CommonExt->HibernateFiles || CommonExt->DumpFiles)
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;

    if (IS_FDO(CommonExt))
        Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
AtaPnpInitializeCommonExtension(
    _In_ PATAPORT_COMMON_EXTENSION CommonExt,
    _In_ PDEVICE_OBJECT SelfDeviceObject,
    _In_ ULONG Flags)
{
    PAGED_CODE();

    CommonExt->Flags = Flags;
    CommonExt->Self = SelfDeviceObject;
    CommonExt->DevicePowerState = PowerDeviceD0;
    CommonExt->SystemPowerState = PowerSystemWorking;

    IoInitializeRemoveLock(&CommonExt->RemoveLock, ATAPORT_TAG, 0, 0);
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
AtaAddChannel(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT Fdo;
    UNICODE_STRING DeviceName;
    PATAPORT_CHANNEL_EXTENSION ChanExt;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\IdePort999")];
    DECLARE_PAGED_WSTRING(FdoFormat, L"\\Device\\Ide\\IdePort%lu");
    static ULONG AtapFdoNumber = 0;

    PAGED_CODE();

    DbgPrint("ATAPI AtaAddChannel\n"); // FIXME

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                FdoFormat,
                                AtapFdoNumber);
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
        return Status;
    }

    ChanExt = Fdo->DeviceExtension;

    RtlZeroMemory(ChanExt, sizeof(*ChanExt));
    AtaPnpInitializeCommonExtension(&ChanExt->Common, Fdo, DO_IS_FDO);

    ChanExt->Common.LowerDeviceObject = IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);
    if (!ChanExt->Common.LowerDeviceObject)
    {
        ERR("Failed to attach the FDO\n");
        Status = STATUS_DEVICE_REMOVED;
        goto Failure;
    }
    ChanExt->DeviceObjectNumber = AtapFdoNumber++;
    ChanExt->Pdo = PhysicalDeviceObject;

    /* DMA buffers alignment */
    Fdo->AlignmentRequirement = ChanExt->Common.LowerDeviceObject->AlignmentRequirement;
    Fdo->AlignmentRequirement = max(Fdo->AlignmentRequirement, ATA_MIN_BUFFER_ALIGNMENT);

    KeInitializeSpinLock(&ChanExt->PdoListLock);

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

Failure:
    if (ChanExt->Common.LowerDeviceObject)
        IoDetachDevice(ChanExt->Common.LowerDeviceObject);

    IoDeleteDevice(Fdo);

    return Status;
}

static
CODE_SEG("INIT")
BOOLEAN
AtaInPEMode(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    NTSTATUS Status;
    DECLARE_PAGED_UNICODE_STRING(
        KeyName, L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\MiniNT");

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
    DECLARE_PAGED_UNICODE_STRING(DirectoryName, L"\\Device\\Ide");

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
    DbgPrint("ATAPI driver entry\n");

    /* FIXME: No crashdump/hibernation support */
    if (!DriverObject)
        return STATUS_NOT_IMPLEMENTED;

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
    DriverObject->DriverExtension->AddDevice = AtaAddChannel;
    DriverObject->DriverUnload = AtaUnload;

    KeInitializeDpc(&AtapCompletionDpc, AtaReqCompletionDpc, NULL);
    InitializeSListHead(&AtapCompletionQueueList);

    /* Create a directory to hold the driver's device objects */
    AtaCreateIdeDirectory();

    AtapInPEMode = AtaInPEMode();

#if defined(ATA_DETECT_LEGACY_DEVICES)
    // TODO: not implemented yet
    //AtaDetectLegacyChannels(DriverObject);
#endif

    return STATUS_SUCCESS;
}
