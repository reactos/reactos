/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Serial enumerator driver
 * FILE:            drivers/storage/fdc/fdc/pdo.c
 * PURPOSE:         Floppy class driver PDO functions
 *
 * PROGRAMMERS:     Cameron Gutman (cameron.gutman@reactos.org)
 */

#include <wdm.h>

#define INITGUID
#include <wdmguid.h>

#include "fdc.h"

#define NDEBUG
#include <debug.h>

static NTSTATUS
FdcPdoQueryId(PFDC_PDO_EXTENSION DevExt,
              PIRP Irp,
              PIO_STACK_LOCATION IrpSp)
{
    WCHAR Buffer[100];
    PWCHAR BufferP;
    
    switch (IrpSp->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            BufferP = L"FDC\\GENERIC_FLOPPY_DRIVE";
            break;
        case BusQueryHardwareIDs:
            BufferP = L"FDC\\GENERIC_FLOPPY_DRIVE\0";
            break;
        case BusQueryCompatibleIDs:
            BufferP = L"GenFloppyDisk\0";
            break;
        case BusQueryInstanceID:
            swprintf(Buffer, L"%d", DevExt->FloppyNumber);
            BufferP = Buffer;
            break;
        default:
            return STATUS_UNSUCCESSFUL;
    }
    
    Irp->IoStatus.Information = (ULONG_PTR)BufferP;
    
    return STATUS_SUCCESS;
}

#if 0
static NTSTATUS
FdcPdoQueryBusInformation(PIRP Irp)
{
    PPNP_BUS_INFORMATION BusInformation;
    
    BusInformation = ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
    if (!BusInformation)
    {
        DPRINT1("Failed to allocate PnP bus info struct\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    BusInformation->BusTypeGuid = GUID_BUS_TYPE_INTERNAL;
    BusInformation->LegacyBusType = Internal;
    BusInformation->BusNumber = 0;
    
    Irp->IoStatus.Information = (ULONG_PTR)BusInformation;
    
    return STATUS_SUCCESS;
}
#endif

static NTSTATUS
FdcPdoQueryCapabilities(PFDC_PDO_EXTENSION DevExt,
                        PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_CAPABILITIES DevCaps = IrpSp->Parameters.DeviceCapabilities.Capabilities;
    
    DevCaps->DeviceD1 = 0;
    DevCaps->DeviceD2 = 0;
    DevCaps->LockSupported = 0;
    DevCaps->EjectSupported = 0;
    DevCaps->Removable = 0;
    DevCaps->DockDevice = 0;
    DevCaps->UniqueID = 0;
    DevCaps->SilentInstall = 0;
    DevCaps->RawDeviceOK = 0;
    DevCaps->SurpriseRemovalOK = 0;
    DevCaps->WakeFromD0 = 0;
    DevCaps->WakeFromD1 = 0;
    DevCaps->WakeFromD2 = 0;
    DevCaps->WakeFromD3 = 0;
    DevCaps->HardwareDisabled = 0;
    DevCaps->NoDisplayInUI = 0;
    DevCaps->Address = DevExt->FloppyNumber;
    DevCaps->SystemWake = PowerSystemUnspecified;
    DevCaps->DeviceWake = PowerDeviceUnspecified;
    DevCaps->D1Latency = 0;
    DevCaps->D2Latency = 0;
    DevCaps->D3Latency = 0;
    
    return STATUS_SUCCESS;
}

static NTSTATUS
FdcPdoQueryTargetDeviceRelations(PFDC_PDO_EXTENSION DevExt,
                                 PIRP Irp)
{
    PDEVICE_RELATIONS DeviceRelations;

    DeviceRelations = ExAllocatePool(NonPagedPool,
                                     sizeof(DEVICE_RELATIONS));
    if (!DeviceRelations)
    {
        DPRINT1("Failed to allocate memory for device relations\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    
    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DevExt->Common.DeviceObject;
    
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;
    
    return STATUS_SUCCESS;
}

static VOID
FdcPdoRemoveDevice(PFDC_PDO_EXTENSION DevExt)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&DevExt->FdoDevExt->FloppyDriveListLock, &OldIrql);
    RemoveEntryList(&DevExt->ListEntry);
    KeReleaseSpinLock(&DevExt->FdoDevExt->FloppyDriveListLock, OldIrql);

    IoDeleteDevice(DevExt->Common.DeviceObject);
}

NTSTATUS
FdcPdoPnpDispatch(IN PDEVICE_OBJECT DeviceObject,
                  IN PIRP Irp)
{
    PFDC_PDO_EXTENSION DevExt = DeviceObject->DeviceExtension;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS Status = Irp->IoStatus.Status;
    PWCHAR Buffer;
    
    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_START_DEVICE:
            DPRINT("Starting FDC PDO\n");
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_STOP_DEVICE:
            DPRINT("Stopping FDC PDO\n");
            /* We don't need to do anything here */
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_QUERY_STOP_DEVICE:
            /* We don't care */
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_REMOVE_DEVICE:
            DPRINT("Removing FDC PDO\n");
            
            FdcPdoRemoveDevice(DevExt);
            
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_SURPRISE_REMOVAL:
            /* Nothing special to do here to deal with surprise removal */
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_QUERY_DEVICE_TEXT:
            Buffer = L"Floppy disk drive";
            Irp->IoStatus.Information = (ULONG_PTR)Buffer;
            Status = STATUS_SUCCESS;
            break;
        case IRP_MN_QUERY_ID:
            Status = FdcPdoQueryId(DevExt, Irp, IrpSp);
            break;
        case IRP_MN_QUERY_CAPABILITIES:
            Status = FdcPdoQueryCapabilities(DevExt, IrpSp);
            break;
        case IRP_MN_QUERY_DEVICE_RELATIONS:
            if (IrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
            {
                Status = FdcPdoQueryTargetDeviceRelations(DevExt, Irp);
            }
            break;
#if 0
        case IRP_MN_QUERY_BUS_INFORMATION:
            Status = FdcPdoQueryBusInformation(Irp);
            break;
#endif
        case IRP_MN_QUERY_RESOURCES:
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            /* All resources are owned by the controller's FDO */
            break;
        default:
            break;
    }
    
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Status;
}

NTSTATUS
FdcPdoPowerDispatch(IN PDEVICE_OBJECT DeviceObject,
                    IN PIRP Irp)
{    
    DPRINT1("Power request not handled\n");
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Irp->IoStatus.Status;
}

NTSTATUS
FdcPdoDeviceControlDispatch(IN PDEVICE_OBJECT DeviceObject,
                            IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    /* FIXME: We don't handle any of these yet */
    
    DPRINT1("Device control request not handled\n");
    
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Irp->IoStatus.Status;
}

NTSTATUS
FdcPdoInternalDeviceControlDispatch(IN PDEVICE_OBJECT DeviceObject,
                                    IN PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    /* FIXME: We don't handle any of these yet */
    
    DPRINT1("Internal device control request not handled\n");
    
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return Irp->IoStatus.Status;
}