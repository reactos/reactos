/*
 * PROJECT:     ReactOS ISA PnP Bus driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Stub driver
 * COPYRIGHT:   Copyright 2021 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * This driver does nothing, and only used if a platform has no ISA PnP support.
 * We need to keep FDO because ACPI driver depends on it. The ACPI bus filter
 * attaches legacy ISA/LPC devices to this FDO.
 */

/* INCLUDES *******************************************************************/

#include "isapnp.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ******************************************************************/

_Dispatch_type_(IRP_MJ_CREATE)
_Dispatch_type_(IRP_MJ_CLOSE)
static CODE_SEG("PAGE") DRIVER_DISPATCH_PAGED IsaStubCreateClose;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaStubCreateClose(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    PAGED_CODE();

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

_Dispatch_type_(IRP_MJ_DEVICE_CONTROL)
_Dispatch_type_(IRP_MJ_SYSTEM_CONTROL)
static CODE_SEG("PAGE") DRIVER_DISPATCH_PAGED IsaStubForward;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaStubForward(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PISAPNP_FDO_EXTENSION FdoExt = DeviceObject->DeviceExtension;

    PAGED_CODE();

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FdoExt->Ldo, Irp);
}

_Dispatch_type_(IRP_MJ_PNP)
static CODE_SEG("PAGE") DRIVER_DISPATCH_PAGED IsaStubPnp;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaStubPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PISAPNP_FDO_EXTENSION FdoExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("%s(%p, %p) Minor - %X\n",
           __FUNCTION__,
           FdoExt,
           Irp,
           IrpSp->MinorFunction);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            if (IoForwardIrpSynchronously(FdoExt->Ldo, Irp))
                Status = Irp->IoStatus.Status;
            else
                Status = STATUS_UNSUCCESSFUL;

            goto CompleteIrp;
        }

        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            PDEVICE_RELATIONS DeviceRelations;

            if (IrpSp->Parameters.QueryDeviceRelations.Type != BusRelations)
                break;

            DeviceRelations = ExAllocatePoolWithTag(PagedPool,
                                                    FIELD_OFFSET(DEVICE_RELATIONS, Objects),
                                                    TAG_ISAPNP);
            if (!DeviceRelations)
            {
                Status = STATUS_NO_MEMORY;
                goto CompleteIrp;
            }

            DeviceRelations->Count = 0;

            Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_REMOVE_DEVICE:
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoSkipCurrentIrpStackLocation(Irp);
            Status = IoCallDriver(FdoExt->Ldo, Irp);

            IoDetachDevice(FdoExt->Ldo);
            IoDeleteDevice(FdoExt->Common.Self);

            return Status;
        }

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
        {
            Irp->IoStatus.Information |= PNP_DEVICE_NOT_DISABLEABLE;
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        case IRP_MN_QUERY_INTERFACE:
        {
            Status = IsaFdoQueryInterface(FdoExt, IrpSp);
            if (Status == STATUS_NOT_SUPPORTED)
            {
                break;
            }
            else if (!NT_SUCCESS(Status))
            {
                goto CompleteIrp;
            }

            Irp->IoStatus.Status = Status;
            break;
        }

        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
        {
            Irp->IoStatus.Status = STATUS_SUCCESS;
            break;
        }

        default:
            break;
    }

    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(FdoExt->Ldo, Irp);

CompleteIrp:
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

_Dispatch_type_(IRP_MJ_POWER)
static DRIVER_DISPATCH_RAISED IsaStubPower;

static
NTSTATUS
NTAPI
IsaStubPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    PISAPNP_FDO_EXTENSION FdoExt = DeviceObject->DeviceExtension;

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(FdoExt->Ldo, Irp);
}

static CODE_SEG("PAGE") DRIVER_ADD_DEVICE IsaStubAddDevice;

static
CODE_SEG("PAGE")
NTSTATUS
NTAPI
IsaStubAddDevice(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PDEVICE_OBJECT PhysicalDeviceObject)
{
    PDEVICE_OBJECT Fdo;
    PISAPNP_FDO_EXTENSION FdoExt;
    NTSTATUS Status;

    PAGED_CODE();

    DPRINT("%s(%p, %p)\n", __FUNCTION__, DriverObject, PhysicalDeviceObject);

    Status = IoCreateDevice(DriverObject,
                            sizeof(ISAPNP_FDO_EXTENSION),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            FALSE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create FDO (0x%08lx)\n", Status);
        return Status;
    }

    FdoExt = Fdo->DeviceExtension;

    RtlZeroMemory(FdoExt, sizeof(ISAPNP_FDO_EXTENSION));
    FdoExt->Common.Self = Fdo;
    FdoExt->Common.Signature = IsaPnpBus;
    FdoExt->DriverObject = DriverObject;
    FdoExt->Pdo = PhysicalDeviceObject;
    FdoExt->Ldo = IoAttachDeviceToDeviceStack(Fdo, PhysicalDeviceObject);
    if (!FdoExt->Ldo)
    {
        IoDeleteDevice(Fdo);
        return STATUS_DEVICE_REMOVED;
    }

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    return STATUS_SUCCESS;
}

static CODE_SEG("PAGE") DRIVER_UNLOAD IsaStubUnload;

static
CODE_SEG("PAGE")
VOID
NTAPI
IsaStubUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);

    PAGED_CODE();

    NOTHING;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    DPRINT("%s(%p, %wZ)\n", __FUNCTION__, DriverObject, RegistryPath);

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IsaStubCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IsaStubCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IsaStubForward;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = IsaStubForward;
    DriverObject->MajorFunction[IRP_MJ_PNP] = IsaStubPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER] = IsaStubPower;
    DriverObject->DriverExtension->AddDevice = IsaStubAddDevice;
    DriverObject->DriverUnload = IsaStubUnload;

    return STATUS_SUCCESS;
}
