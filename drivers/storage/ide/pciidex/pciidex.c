/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "pciidex.h"

ULONG PciIdeControllerNumber = 0;

BOOLEAN
PciFindDevice(
    _In_ __inner_callback PATA_PCI_MATCH_FN MatchFunction,
    _In_ PVOID Context)
{
    ULONG BusNumber, DeviceNumber, FunctionNumber;

    for (BusNumber = 0; BusNumber < 0xFF; ++BusNumber)
    {
        for (DeviceNumber = 0; DeviceNumber < PCI_MAX_DEVICES; ++DeviceNumber)
        {
            for (FunctionNumber = 0; FunctionNumber < PCI_MAX_FUNCTION; ++FunctionNumber)
            {
                UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_HEADER, RevisionID)];
                PPCI_COMMON_HEADER PciConfig = (PPCI_COMMON_HEADER)Buffer; // Partial PCI header
                PCI_SLOT_NUMBER PciSlot;
                ULONG BytesRead;

                PciSlot.u.AsULONG = 0;
                PciSlot.u.bits.DeviceNumber = DeviceNumber;
                PciSlot.u.bits.FunctionNumber = FunctionNumber;

                BytesRead = HalGetBusDataByOffset(PCIConfiguration,
                                                  BusNumber,
                                                  PciSlot.u.AsULONG,
                                                  &Buffer,
                                                  0,
                                                  sizeof(Buffer));
                if (BytesRead != sizeof(Buffer) ||
                    PciConfig->VendorID == PCI_INVALID_VENDORID ||
                    PciConfig->VendorID == 0)
                {
                    if (FunctionNumber == 0)
                    {
                        /* This slot has no single- or a multi-function device */
                        break;
                    }
                    else
                    {
                        /* Continue scanning the functions */
                        continue;
                    }
                }

                if (MatchFunction(Context, BusNumber, PciSlot, PciConfig))
                    return TRUE;

                if (!PCI_MULTIFUNCTION_DEVICE(PciConfig))
                {
                    /* The device is a single function device */
                    break;
                }
            }
        }
    }

    return FALSE;
}

VOID
PciRead(
    _In_ PATA_CONTROLLER Controller,
    _Out_writes_bytes_all_(BufferLength) PVOID Buffer,
    _In_ ULONG ConfigDataOffset,
    _In_ ULONG BufferLength)
{
    (*Controller->BusInterface.GetBusData)(Controller->BusInterface.Context,
                                           PCI_WHICHSPACE_CONFIG,
                                           Buffer,
                                           ConfigDataOffset,
                                           BufferLength);
}

VOID
PciWrite(
    _In_ PATA_CONTROLLER Controller,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_ ULONG ConfigDataOffset,
    _In_ ULONG BufferLength)
{
    (*Controller->BusInterface.SetBusData)(Controller->BusInterface.Context,
                                           PCI_WHICHSPACE_CONFIG,
                                           Buffer,
                                           ConfigDataOffset,
                                           BufferLength);
}

VOID
AtaSleep(VOID)
{
    KTIMER Timer;
    LARGE_INTEGER DueTime;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    DueTime.QuadPart = PORT_TIMER_TICK_MS * -10000LL;

    KeInitializeTimer(&Timer);
    KeSetTimer(&Timer, DueTime, 0);
    KeWaitForSingleObject(&Timer, Executive, KernelMode, FALSE, 0);
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
PciIdeXDispatchWmi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;

    PAGED_CODE();

    if (IS_FDO(DeviceObject->DeviceExtension))
    {
        PFDO_DEVICE_EXTENSION FdoExtension = DeviceObject->DeviceExtension;

        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(FdoExtension->Common.LowerDeviceObject, Irp);
    }
    else
    {
        Status = Irp->IoStatus.Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

CODE_SEG("PAGE")
VOID
NTAPI
PciIdeXUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    NOTHING;
}

CODE_SEG("PAGE")
NTSTATUS
PciIdeXPnpRepeatRequest(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExt,
    _In_ PIRP Irp,
    _In_opt_ PDEVICE_CAPABILITIES DeviceCapabilities)
{
    PCOMMON_DEVICE_EXTENSION FdoExt = CommonExt->FdoExt;
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
                           PciIdeXPdoCompletionRoutine,
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
PciIdeXPnpQueryDeviceUsageNotification(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExt,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;
    volatile LONG* Counter;

    PAGED_CODE();

    if (IS_FDO(CommonExt))
    {
        Status = PciIdeXPnpRepeatRequest(CommonExt, Irp, NULL);
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
PciIdeXPnpQueryPnpDeviceState(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExt,
    _In_ PIRP Irp)
{
    PAGED_CODE();

    if (CommonExt->PageFiles || CommonExt->HibernateFiles || CommonExt->DumpFiles)
        Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;

    if (IS_FDO(CommonExt))
        Irp->IoStatus.Status = STATUS_SUCCESS;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PciIdeXPdoCompletionRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_reads_opt_(_Inexpressible_("varies")) PVOID Context)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned)
        KeSetEvent(Context, IO_NO_INCREMENT, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXPnpQueryInterface(
    _In_ PCOMMON_DEVICE_EXTENSION CommonExt,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
    _In_ ULONG Version,
    _In_ ULONG Size)
{
    KEVENT Event;
    IO_STATUS_BLOCK IoStatus;
    PIRP Irp;
    PIO_STACK_LOCATION Stack;
    NTSTATUS Status;

    PAGED_CODE();

    KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                                       CommonExt->LowerDeviceObject,
                                       NULL,
                                       0,
                                       NULL,
                                       &Event,
                                       &IoStatus);
    if (!Irp)
        return STATUS_INSUFFICIENT_RESOURCES;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    Stack = IoGetNextIrpStackLocation(Irp);
    Stack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    Stack->Parameters.QueryInterface.InterfaceType = Guid;
    Stack->Parameters.QueryInterface.Version = Version;
    Stack->Parameters.QueryInterface.Size = Size;
    Stack->Parameters.QueryInterface.Interface = Interface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(CommonExt->LowerDeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
        KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
        Status = IoStatus.Status;
    }

    return Status;
}

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXAddDeviceEx(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject,
    _Out_ PVOID *ControllerContext)
{
    PPCIIDEX_DRIVER_EXTENSION DriverExtension;
    PFDO_DEVICE_EXTENSION FdoExtension;
    ULONG DeviceExtensionSize;
    PDEVICE_OBJECT Fdo;
    UNICODE_STRING DeviceName;
    WCHAR DeviceNameBuffer[sizeof("\\Device\\Ide\\PciIde999")];
    NTSTATUS Status;

    PAGED_CODE();

    Status = RtlStringCbPrintfW(DeviceNameBuffer,
                                sizeof(DeviceNameBuffer),
                                L"\\Device\\Ide\\PciIde%lu",
                                PciIdeControllerNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    INFO("%s(%p, %p) '%wZ'\n", __FUNCTION__, DriverObject, PhysicalDeviceObject, &DeviceName);

    DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
    ASSERT(DriverExtension);

    Status = RtlULongAdd(DriverExtension->MiniControllerExtensionSize,
                         sizeof(*FdoExtension),
                         &DeviceExtensionSize);
    if (!NT_SUCCESS(Status))
    {
        ERR("Invalid miniport extension size %lx\n", DriverExtension->MiniControllerExtensionSize);
        return Status;
    }

    Status = IoCreateDevice(DriverObject,
                            DeviceExtensionSize,
                            &DeviceName,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create FDO 0x%lx\n", Status);
        return Status;
    }

    FdoExtension = Fdo->DeviceExtension;

    RtlZeroMemory(FdoExtension, sizeof(*FdoExtension));
    FdoExtension->Common.Flags = DO_IS_FDO;
    FdoExtension->Common.Self = Fdo;
    IoInitializeRemoveLock(&FdoExtension->Common.RemoveLock, TAG_PCIIDEX, 0, 0);

    FdoExtension->DriverObject = DriverObject;
    FdoExtension->Pdo = PhysicalDeviceObject;
    FdoExtension->ControllerNumber = PciIdeControllerNumber++;

    ExInitializeFastMutex(&FdoExtension->PdoListSyncMutex);
    InitializeListHead(&FdoExtension->PdoListHead);

    KeInitializeSpinLock(&FdoExtension->Controller.Lock);

    Status = IoAttachDeviceToDeviceStackSafe(Fdo,
                                             PhysicalDeviceObject,
                                             &FdoExtension->Common.LowerDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to attach FDO 0x%lx\n", Status);
        goto Failure;
    }

    /* DMA buffers alignment */
    Fdo->AlignmentRequirement = FdoExtension->Common.LowerDeviceObject->AlignmentRequirement;
    Fdo->AlignmentRequirement = max(Fdo->AlignmentRequirement, ATA_MIN_BUFFER_ALIGNMENT);

    if (ControllerContext)
    {
        FdoExtension->Controller.Flags |= CTRL_FLAG_NON_PNP;
        *ControllerContext = FdoExtension;
    }
    else
    {
        Status = PciIdeXPnpQueryInterface(&FdoExtension->Common,
                                          &GUID_BUS_INTERFACE_STANDARD,
                                          &FdoExtension->Controller.BusInterface,
                                          PCI_BUS_INTERFACE_STANDARD_VERSION,
                                          sizeof(FdoExtension->Controller.BusInterface));
        if (!NT_SUCCESS(Status))
        {
            ERR("No PCI bus interface 0x%lx\n", Status);
            goto Failure;
        }
    }

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;

Failure:
    if (FdoExtension->Common.LowerDeviceObject)
        IoDetachDevice(FdoExtension->Common.LowerDeviceObject);

    IoDeleteDevice(Fdo);

    return Status;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
PciIdeXAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PAGED_CODE();

    return PciIdeXAddDeviceEx(DriverObject, PhysicalDeviceObject, NULL);
}

static
CODE_SEG("PAGE")
VOID
PciIdeXCreateIdeDirectory(VOID)
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

CODE_SEG("PAGE")
NTSTATUS
NTAPI
PciIdeXInitialize(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
    _In_ PCONTROLLER_PROPERTIES HwGetControllerProperties,
    _In_ ULONG ExtensionSize)
{
    PPCIIDEX_DRIVER_EXTENSION DriverExtension;
    NTSTATUS Status;

    PAGED_CODE();

    INFO("PciIdeXInitialize(%p '%wZ' %p 0x%lx)\n",
         DriverObject, RegistryPath, HwGetControllerProperties, ExtensionSize);

    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             DriverObject,
                                             sizeof(*DriverExtension),
                                             (PVOID*)&DriverExtension);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlZeroMemory(DriverExtension, sizeof(*DriverExtension));
    DriverExtension->MiniControllerExtensionSize = ExtensionSize;
    DriverExtension->HwGetControllerProperties = HwGetControllerProperties;

    DriverObject->MajorFunction[IRP_MJ_PNP] = PciIdeXDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PciIdeXDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = PciIdeXDispatchWmi;
    DriverObject->DriverExtension->AddDevice = PciIdeXAddDevice;
    DriverObject->DriverUnload = PciIdeXUnload;

    /* Create a directory to hold the driver's device objects */
    PciIdeXCreateIdeDirectory();

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE") /* This function is too small to be placed into INIT section */
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);

    PAGED_CODE();

    return STATUS_SUCCESS;
}
