/*
 * PROJECT:         ReactOS Universal Serial Bus Hub Driver
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            drivers/usb/usbhub/fdo.c
 * PURPOSE:         Handle PDO
 * PROGRAMMERS:
 *                  Hervé Poussineau (hpoussin@reactos.org)
 *                  Michael Martin (michael.martin@reactos.org)
 *                  Johannes Anderwald (johannes.anderwald@reactos.org)
 */

#define NDEBUG
#include "usbhub.h"

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)
  
NTSTATUS
USBHUB_PdoHandleInternalDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    NTSTATUS Status;

    DPRINT1("UsbhubInternalDeviceControlPdo(%x) called\n", DeviceObject);

    Stack = IoGetCurrentIrpStackLocation(Irp);
    Status = Irp->IoStatus.Status;

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO:
        {
            PHUB_DEVICE_EXTENSION DeviceExtension;

            DPRINT1("IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO\n");
            if (Irp->AssociatedIrp.SystemBuffer == NULL
                || Stack->Parameters.DeviceIoControl.OutputBufferLength != sizeof(PVOID))
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                PVOID* pHubPointer;
                DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

                pHubPointer = (PVOID*)Irp->AssociatedIrp.SystemBuffer;
                // FIXME
                *pHubPointer = NULL;
                Information = sizeof(PVOID);
                Status = STATUS_SUCCESS;
            }
            break;
        }
        default:
        {
            DPRINT1("Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

NTSTATUS
USBHUB_PdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHUB_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    DPRINT1("USBHUB_PdoStartDevice %x\n", DeviceObject);
    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    UNIMPLEMENTED
    return Status;
}

NTSTATUS
USBHUB_PdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT ULONG_PTR* Information)
{
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    ULONG IdType;
    PWCHAR SourceString = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    switch (IdType)
    {
        case BusQueryDeviceID:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
            SourceString = ChildDeviceExtension->DeviceId;
            break;
        }
        case BusQueryHardwareIDs:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
            SourceString = ChildDeviceExtension->HardwareIds;
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
        case BusQueryCompatibleIDs:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
            SourceString = ChildDeviceExtension->CompatibleIds;
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
        case BusQueryInstanceID:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
            SourceString = ChildDeviceExtension->InstanceId;
            break;
        }
        default:
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
            return STATUS_NOT_SUPPORTED;
    }

    *Information = (ULONG_PTR)SourceString;
    return Status;
}

NTSTATUS
USBHUB_PdoQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT ULONG_PTR* Information)
{
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    DEVICE_TEXT_TYPE DeviceTextType;
    LCID LocaleId;

    DeviceTextType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.DeviceTextType;
    LocaleId = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.LocaleId;
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        case DeviceTextLocationInformation:
        {
            if (DeviceTextType == DeviceTextDescription)
            {
                *Information = (ULONG_PTR)ChildDeviceExtension->TextDescription;
                DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");
            }
            else
                DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextLocationInformation\n");
            return STATUS_SUCCESS;
        }
        default:
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown device text type 0x%lx\n", DeviceTextType);
            return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS
USBHUB_PdoHandlePnp(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG MinorFunction;
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    NTSTATUS Status;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    MinorFunction = Stack->MinorFunction;

    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
            Status = USBHUB_PdoStartDevice(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES:
        {
            PDEVICE_CAPABILITIES DeviceCapabilities;
            ULONG i;
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");

            DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;
            // FIXME: capabilities can change with connected device
            DeviceCapabilities->LockSupported = TRUE;
            DeviceCapabilities->EjectSupported = FALSE;
            DeviceCapabilities->Removable = FALSE;
            DeviceCapabilities->DockDevice = FALSE;
            DeviceCapabilities->UniqueID = FALSE;
            DeviceCapabilities->SilentInstall = TRUE;
            DeviceCapabilities->RawDeviceOK = FALSE;
            DeviceCapabilities->SurpriseRemovalOK = FALSE;
            DeviceCapabilities->HardwareDisabled = FALSE;
            //DeviceCapabilities->NoDisplayInUI = FALSE;
            DeviceCapabilities->DeviceState[0] = PowerDeviceD0;
            for (i = 0; i < PowerSystemMaximum; i++)
                DeviceCapabilities->DeviceState[i] = PowerDeviceD3;
            //DeviceCapabilities->DeviceWake = PowerDeviceUndefined;
            DeviceCapabilities->D1Latency = 0;
            DeviceCapabilities->D2Latency = 0;
            DeviceCapabilities->D3Latency = 0;
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_RESOURCES:
        {
            PCM_RESOURCE_LIST ResourceList;

            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
            ResourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST));
            if (!ResourceList)
            {
                DPRINT1("ExAllocatePool() failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                ResourceList->Count = 0;
                Information = (ULONG_PTR)ResourceList;
                Status = STATUS_SUCCESS;
            }
            break;
        }
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        {
            PIO_RESOURCE_REQUIREMENTS_LIST ResourceList;

            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            ResourceList = ExAllocatePool(PagedPool, sizeof(IO_RESOURCE_REQUIREMENTS_LIST));
            if (!ResourceList)
            {
                DPRINT1("ExAllocatePool() failed\n");
                Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {
                RtlZeroMemory(ResourceList, sizeof(IO_RESOURCE_REQUIREMENTS_LIST));
                ResourceList->ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
                ResourceList->AlternativeLists = 1;
                ResourceList->List->Version = 1;
                ResourceList->List->Revision = 1;
                ResourceList->List->Count = 0;
                Information = (ULONG_PTR)ResourceList;
                Status = STATUS_SUCCESS;
            }
            break;
        }
        case IRP_MN_QUERY_DEVICE_TEXT:
        {
            Status = USBHUB_PdoQueryDeviceText(DeviceObject, Irp, &Information);
            break;
        }
        case IRP_MN_QUERY_ID:
        {
            Status = USBHUB_PdoQueryId(DeviceObject, Irp, &Information);
            break;
        }
        default:
        {
            DPRINT1("ERROR PDO IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

