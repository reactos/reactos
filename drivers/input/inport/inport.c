/*
 * PROJECT:     ReactOS InPort (Bus) Mouse Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Driver entrypoint
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

/* INCLUDES *******************************************************************/

#include "inport.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, InPortCreateClose)
#pragma alloc_text(PAGE, InPortAddDevice)
#pragma alloc_text(PAGE, InPortUnload)
#endif

UNICODE_STRING DriverRegistryPath;

/* FUNCTIONS ******************************************************************/

NTSTATUS
NTAPI
InPortCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PAGED_CODE();

    DPRINT("%s(%p, %p) %X\n", __FUNCTION__, DeviceObject,
           Irp, IoGetCurrentIrpStackLocation(Irp)->MajorFunction);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
InPortAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    NTSTATUS Status;
    PDEVICE_OBJECT Fdo;
    PINPORT_DEVICE_EXTENSION FdoExtension = NULL;
    WCHAR HardwareIdBuffer[32];
    UNICODE_STRING HardwareId;
    ULONG DummyResultLength;
    UNICODE_STRING HardwareId1 = RTL_CONSTANT_STRING(L"*nEC1F00");
    UNICODE_STRING HardwareId2 = RTL_CONSTANT_STRING(L"*PNP0F00");
    UNICODE_STRING HardwareId3 = RTL_CONSTANT_STRING(L"*PNP0F02");
    UNICODE_STRING HardwareId4 = RTL_CONSTANT_STRING(L"*PNP0F0D");
    UNICODE_STRING HardwareId5 = RTL_CONSTANT_STRING(L"*PNP0F11");
    UNICODE_STRING HardwareId6 = RTL_CONSTANT_STRING(L"*PNP0F15");

    PAGED_CODE();

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DriverObject, PhysicalDeviceObject);

    Status = IoCreateDevice(DriverObject,
                            sizeof(INPORT_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_INPORT_PORT,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create FDO 0x%X\n", Status);
        goto Failure;
    }

    FdoExtension = Fdo->DeviceExtension;

    RtlZeroMemory(FdoExtension, sizeof(INPORT_DEVICE_EXTENSION));
    FdoExtension->State = dsStopped;
    FdoExtension->Self = Fdo;
    FdoExtension->Pdo = PhysicalDeviceObject;
    FdoExtension->Ldo = IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);
    if (!FdoExtension->Ldo)
    {
        DPRINT1("Failed to attach FDO\n");
        Status = STATUS_NO_SUCH_DEVICE;
        goto Failure;
    }

    IoInitializeRemoveLock(&FdoExtension->RemoveLock, INPORT_TAG, 0, 0);
    IoInitializeDpcRequest(Fdo, InPortDpcForIsr);

    Status = IoGetDeviceProperty(PhysicalDeviceObject,
                                 DevicePropertyHardwareID,
                                 sizeof(HardwareIdBuffer),
                                 HardwareIdBuffer,
                                 &DummyResultLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query the hardware ID string 0x%X\n", Status);
        goto Failure;
    }
    RtlInitUnicodeString(&HardwareId, HardwareIdBuffer);

    if (RtlEqualUnicodeString(&HardwareId, &HardwareId1, FALSE))
    {
        FdoExtension->MouseType = NecBusMouse;
        FdoExtension->MouseAttributes.SampleRate = 60;
        FdoExtension->MouseAttributes.NumberOfButtons = 2;
    }
    else if (RtlEqualUnicodeString(&HardwareId, &HardwareId3, FALSE) ||
             RtlEqualUnicodeString(&HardwareId, &HardwareId4, FALSE))
    {
        FdoExtension->MouseType = MsInPortMouse;
        FdoExtension->MouseAttributes.SampleRate = 100;
        FdoExtension->MouseAttributes.NumberOfButtons = 3;
    }
    else if (RtlEqualUnicodeString(&HardwareId, &HardwareId2, FALSE) ||
             RtlEqualUnicodeString(&HardwareId, &HardwareId5, FALSE) ||
             RtlEqualUnicodeString(&HardwareId, &HardwareId6, FALSE))
    {
        FdoExtension->MouseType = LogitechBusMouse;
        FdoExtension->MouseAttributes.SampleRate = 100;
        FdoExtension->MouseAttributes.NumberOfButtons = 3;
    }
    else
    {
        DPRINT1("Unrecognized hardware '%wZ'\n", &HardwareId);
        Status = STATUS_DEVICE_REMOVED;
        goto Failure;
    }
    FdoExtension->MouseAttributes.MouseIdentifier = MOUSE_INPORT_HARDWARE;
    /* 1 packet */
    FdoExtension->MouseAttributes.InputDataQueueLength = sizeof(MOUSE_INPUT_DATA);

    Fdo->Flags |= DO_BUFFERED_IO;
    if (FdoExtension->Ldo->Flags & DO_POWER_PAGABLE)
        Fdo->Flags |= DO_POWER_PAGABLE;
    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;

Failure:
    if (FdoExtension)
    {
        if (FdoExtension->Ldo)
            IoDetachDevice(FdoExtension->Ldo);
    }

    if (Fdo)
    {
        IoDeleteDevice(Fdo);
    }

    return Status;
}

VOID
NTAPI
InPortUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    DPRINT("%s(%p)\n", __FUNCTION__, DriverObject);

    RtlFreeUnicodeString(&DriverRegistryPath);
}

NTSTATUS
NTAPI
InPortPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PINPORT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

    DPRINT("%s(%p, %p) %X\n", __FUNCTION__, DeviceObject, Irp,
           IoGetCurrentIrpStackLocation(Irp)->MinorFunction);

    Status = IoAcquireRemoveLock(&DeviceExtension->RemoveLock, Irp);
    if (!NT_SUCCESS(Status))
    {
        Irp->IoStatus.Status = Status;
        PoStartNextPowerIrp(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);

        return Status;
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    Status = PoCallDriver(DeviceExtension->Ldo, Irp);

    IoReleaseRemoveLock(&DeviceExtension->RemoveLock, Irp);

    return Status;
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DPRINT("%s(%p, %wZ)\n", __FUNCTION__, DriverObject, RegistryPath);

    DriverRegistryPath.Buffer = ExAllocatePoolWithTag(NonPagedPool,
                                                      RegistryPath->Length +
                                                      sizeof(UNICODE_NULL),
                                                      INPORT_TAG);
    if (!DriverRegistryPath.Buffer)
    {
        DPRINT1("Failed to allocate the registry string buffer\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    DriverRegistryPath.MaximumLength = RegistryPath->Length + sizeof(UNICODE_NULL);
    RtlCopyUnicodeString(&DriverRegistryPath, RegistryPath);
    DriverRegistryPath.Buffer[RegistryPath->Length / sizeof(WCHAR)] = UNICODE_NULL;

    DriverObject->MajorFunction[IRP_MJ_CREATE] =
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = InPortCreateClose;
    DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] = InPortInternalDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = InPortPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = InPortWmi;
    DriverObject->MajorFunction[IRP_MJ_PNP] = InPortPnp;
    DriverObject->DriverExtension->AddDevice = InPortAddDevice;
    DriverObject->DriverUnload = InPortUnload;

    return STATUS_SUCCESS;
}
