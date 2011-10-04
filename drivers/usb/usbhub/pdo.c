/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         USB hub driver
 * FILE:            drivers/usb/cromwell/hub/pdo.c
 * PURPOSE:         IRP_MJ_PNP operations for PDOs
 *
 * PROGRAMMERS:     Copyright 2005-2006 Hervé Poussineau (hpoussin@reactos.org)
 *                  2010 Michael Martin (michael.martin@reactos.org)
 */

#define NDEBUG
#include <stdio.h>
#include "usbhub.h"

#define IO_METHOD_FROM_CTL_CODE(ctlCode) (ctlCode&0x00000003)

NTSTATUS
UsbhubInternalDeviceControlPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION Stack;
    ULONG_PTR Information = 0;
    NTSTATUS Status;

    DPRINT1("Usbhub: UsbhubInternalDeviceControlPdo() called\n");

    Stack = IoGetCurrentIrpStackLocation(Irp);
    Status = Irp->IoStatus.Status;

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO:
        {
            PHUB_DEVICE_EXTENSION DeviceExtension;

            DPRINT1("Usbhub: IOCTL_INTERNAL_USB_GET_PARENT_HUB_INFO\n");
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
                *pHubPointer = DeviceExtension->dev;
                Information = sizeof(PVOID);
                Status = STATUS_SUCCESS;
            }
            break;
        }
        default:
        {
            DPRINT1("Usbhub: Unknown IOCTL code 0x%lx\n", Stack->Parameters.DeviceIoControl.IoControlCode);
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

static NTSTATUS
UsbhubPdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PHUB_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    DeviceExtension = (PHUB_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Register and activate device interface */
/*
    Status = IoRegisterDeviceInterface(
        DeviceObject,
        DeviceExtension->dev->descriptor.bDeviceClass == USB_CLASS_HUB ?
            &GUID_DEVINTERFACE_USB_HUB :
            &GUID_DEVINTERFACE_USB_DEVICE,
        NULL,
        &DeviceExtension->SymbolicLinkName);
*/
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Usbhub: IoRegisterDeviceInterface() failed with status 0x%08lx\n", Status);
        return Status;
    }

    //Status = IoSetDeviceInterfaceState(&DeviceExtension->SymbolicLinkName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Usbhub: IoSetDeviceInterfaceState() failed with status 0x%08lx\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
UsbhubPdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT ULONG_PTR* Information)
{
    PHUB_CHILDDEVICE_EXTENSION DeviceExtension;
    ULONG IdType;
    PWCHAR SourceString = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
    DeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    switch (IdType)
    {
    case BusQueryDeviceID:
        {
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
            SourceString = DeviceExtension->DeviceId;
            break;
        }
        /* FIXME: Implement */
        case BusQueryHardwareIDs:
        {
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");
            SourceString = DeviceExtension->HardwareIds;
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
        /* FIXME: Implement */
        case BusQueryCompatibleIDs:
        {
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
            SourceString = DeviceExtension->CompatibleIds;
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
        case BusQueryInstanceID:
        {
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
            SourceString = DeviceExtension->InstanceId;
            break;
        }
        default:
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
            return STATUS_NOT_SUPPORTED;
    }

    *Information = (ULONG_PTR)SourceString;
    return Status;
}

static NTSTATUS
UsbhubPdoQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT ULONG_PTR* Information)
{
    PHUB_CHILDDEVICE_EXTENSION DeviceExtension;
    DEVICE_TEXT_TYPE DeviceTextType;
    LCID LocaleId;

    DeviceTextType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.DeviceTextType;
    LocaleId = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.LocaleId;
    DeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        case DeviceTextLocationInformation:
        {
            if (DeviceTextType == DeviceTextDescription)
            {
                *Information = (ULONG_PTR)DeviceExtension->TextDescription;
                DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");
            }
            else
                DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextLocationInformation\n");

/*            if (!DeviceExtension->dev->descriptor.iProduct)
                return STATUS_NOT_SUPPORTED;*/

            return STATUS_SUCCESS;
        }
        default:
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown device text type 0x%lx\n", DeviceTextType);
            return STATUS_NOT_SUPPORTED;
    }
}

NTSTATUS NTAPI
UsbhubPnpPdo(
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
        case IRP_MN_START_DEVICE: /* 0x0 */
        {
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_START_DEVICE\n");
            Status = UsbhubPdoStartDevice(DeviceObject, Irp);
            break;
        }
        case IRP_MN_QUERY_CAPABILITIES: /* 0x09 */
        {
            PDEVICE_CAPABILITIES DeviceCapabilities;
            ULONG i;
            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_CAPABILITIES\n");

            DeviceCapabilities = (PDEVICE_CAPABILITIES)Stack->Parameters.DeviceCapabilities.Capabilities;
            /* FIXME: capabilities can change with connected device */
            DeviceCapabilities->LockSupported = TRUE;
            DeviceCapabilities->EjectSupported = FALSE;
            DeviceCapabilities->Removable = FALSE;
            DeviceCapabilities->DockDevice = FALSE;
            DeviceCapabilities->UniqueID = FALSE;
            DeviceCapabilities->SilentInstall = TRUE;
            DeviceCapabilities->RawDeviceOK = FALSE;
            DeviceCapabilities->SurpriseRemovalOK = FALSE;
            DeviceCapabilities->HardwareDisabled = FALSE; /* FIXME */
            //DeviceCapabilities->NoDisplayInUI = FALSE; /* FIXME */
            DeviceCapabilities->DeviceState[0] = PowerDeviceD0; /* FIXME */
            for (i = 0; i < PowerSystemMaximum; i++)
                DeviceCapabilities->DeviceState[i] = PowerDeviceD3; /* FIXME */
            //DeviceCapabilities->DeviceWake = PowerDeviceUndefined; /* FIXME */
            DeviceCapabilities->D1Latency = 0; /* FIXME */
            DeviceCapabilities->D2Latency = 0; /* FIXME */
            DeviceCapabilities->D3Latency = 0; /* FIXME */
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_QUERY_RESOURCES: /* 0x0a */
        {
            PCM_RESOURCE_LIST ResourceList;

            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_RESOURCES\n");
            ResourceList = ExAllocatePool(PagedPool, sizeof(CM_RESOURCE_LIST));
            if (!ResourceList)
            {
                DPRINT1("Usbhub: ExAllocatePool() failed\n");
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
        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS: /* 0x0b */
        {
            PIO_RESOURCE_REQUIREMENTS_LIST ResourceList;

            DPRINT1("Usbhub: IRP_MJ_PNP / IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n");
            ResourceList = ExAllocatePool(PagedPool, sizeof(IO_RESOURCE_REQUIREMENTS_LIST));
            if (!ResourceList)
            {
                DPRINT1("Usbhub: ExAllocatePool() failed\n");
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
        case IRP_MN_QUERY_DEVICE_TEXT: /* 0x0c */
        {
            Status = UsbhubPdoQueryDeviceText(DeviceObject, Irp, &Information);
            break;
        }
        case IRP_MN_QUERY_ID: /* 0x13 */
        {
            Status = UsbhubPdoQueryId(DeviceObject, Irp, &Information);
            break;
        }
        default:
        {
            /* We can't forward request to the lower driver, because
             * we are a Pdo, so we don't have lower driver...
             */
            DPRINT1("Usbhub: IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

