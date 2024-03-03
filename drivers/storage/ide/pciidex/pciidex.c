/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Main file
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "pciidex.h"

#define NDEBUG
#include <debug.h>

ULONG PciIdeControllerNumber = 0;

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
        Status = IoCallDriver(FdoExtension->Ldo, Irp);
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

static
CODE_SEG("PAGE")
NTSTATUS
PciIdeXQueryInterface(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ const GUID* Guid,
    _Out_ PVOID Interface,
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
                                       FdoExtension->Ldo,
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
    Stack->Parameters.QueryInterface.Version = 1;
    Stack->Parameters.QueryInterface.Size = Size;
    Stack->Parameters.QueryInterface.Interface = Interface;
    Stack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Status = IoCallDriver(FdoExtension->Ldo, Irp);
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
PciIdeXGetConfigurationInfo(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    UCHAR Buffer[RTL_SIZEOF_THROUGH_FIELD(PCI_COMMON_HEADER, BaseClass)];
    PPCI_COMMON_HEADER PciConfig = (PPCI_COMMON_HEADER)Buffer;
    ULONG BytesRead;

    PAGED_CODE();

    BytesRead = (*FdoExtension->BusInterface.GetBusData)(FdoExtension->BusInterface.Context,
                                                         PCI_WHICHSPACE_CONFIG,
                                                         Buffer,
                                                         0,
                                                         sizeof(Buffer));
    if (BytesRead != sizeof(Buffer))
        return STATUS_IO_DEVICE_ERROR;

    FdoExtension->VendorId = PciConfig->VendorID;
    FdoExtension->DeviceId = PciConfig->DeviceID;

    if (PciConfig->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR)
    {
        if (PciConfig->ProgIf & PCIIDE_PROGIF_DMA_CAPABLE)
        {
            FdoExtension->Flags |= FDO_DMA_CAPABLE;
        }

        if (PciConfig->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR)
        {
            /* Both IDE channels in native mode */
            if ((PciConfig->ProgIf & PCIIDE_PROGIF_PRIMARY_CHANNEL_NATIVE_MODE) &&
                (PciConfig->ProgIf & PCIIDE_PROGIF_SECONDARY_CHANNEL_NATIVE_MODE))
            {
                FdoExtension->Flags |= FDO_IN_NATIVE_MODE;
            }
        }
        else if (PciConfig->SubClass == PCI_SUBCLASS_MSC_RAID_CTLR)
        {
            FdoExtension->Flags |= FDO_IN_NATIVE_MODE;
        }
    }

    DPRINT("Controller %04x:%04x, Interface byte 0x%02x, Native mode %d\n",
           FdoExtension->VendorId,
           FdoExtension->DeviceId,
           PciConfig->ProgIf,
           !!(FdoExtension->Flags & FDO_IN_NATIVE_MODE));

    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NTSTATUS
NTAPI
PciIdeXAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
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
                                L"\\Device\\Ide\\PciIde%u",
                                PciIdeControllerNumber);
    ASSERT(NT_SUCCESS(Status));
    RtlInitUnicodeString(&DeviceName, DeviceNameBuffer);

    DPRINT("%s(%p, %p) '%wZ'\n", __FUNCTION__, DriverObject, PhysicalDeviceObject, &DeviceName);

    DriverExtension = IoGetDriverObjectExtension(DriverObject, DriverObject);
    ASSERT(DriverExtension);

    Status = RtlULongAdd(DriverExtension->MiniControllerExtensionSize,
                         sizeof(FDO_DEVICE_EXTENSION),
                         &DeviceExtensionSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Invalid miniport extension size %lx\n",
                DriverExtension->MiniControllerExtensionSize);
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
        DPRINT1("Failed to create FDO 0x%lx\n", Status);
        return Status;
    }

    FdoExtension = Fdo->DeviceExtension;

    RtlZeroMemory(FdoExtension, sizeof(FDO_DEVICE_EXTENSION));
    FdoExtension->Common.IsFDO = TRUE;
    FdoExtension->Common.Self = Fdo;
    FdoExtension->DriverObject = DriverObject;
    FdoExtension->ControllerNumber = PciIdeControllerNumber++;

    KeInitializeSpinLock(&FdoExtension->BusDataLock);
    ExInitializeFastMutex(&FdoExtension->DeviceSyncMutex);

    Status = IoAttachDeviceToDeviceStackSafe(Fdo, PhysicalDeviceObject, &FdoExtension->Ldo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to attach FDO 0x%lx\n", Status);
        goto Failure;
    }

    /* DMA buffers alignment */
    Fdo->AlignmentRequirement = max(FdoExtension->Ldo->AlignmentRequirement, FILE_WORD_ALIGNMENT);

    Status = PciIdeXQueryInterface(FdoExtension,
                                   &GUID_BUS_INTERFACE_STANDARD,
                                   &FdoExtension->BusInterface,
                                   sizeof(BUS_INTERFACE_STANDARD));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("No bus interface 0x%lx\n", Status);
        goto Failure;
    }

    Status = PciIdeXGetConfigurationInfo(FdoExtension);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Unable to retrieve the configuration info %lx\n", Status);
        goto Failure;
    }

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

Failure:
    if (FdoExtension->Ldo)
        IoDetachDevice(FdoExtension->Ldo);

    IoDeleteDevice(Fdo);

    return Status;
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

    DPRINT("PciIdeXInitialize(%p '%wZ' %p 0x%lx)\n",
           DriverObject, RegistryPath, HwGetControllerProperties, ExtensionSize);

    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             DriverObject,
                                             sizeof(PCIIDEX_DRIVER_EXTENSION),
                                             (PVOID*)&DriverExtension);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlZeroMemory(DriverExtension, sizeof(PCIIDEX_DRIVER_EXTENSION));
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
