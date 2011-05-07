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
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    //NTSTATUS Status;
    DPRINT1("USBHUB_PdoStartDevice %x\n", DeviceObject);
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // This should be a PDO
    //
    ASSERT(ChildDeviceExtension->Common.IsFDO == FALSE);

    //
    // FIXME: Fow now assume success
    //

    UNIMPLEMENTED
    return STATUS_SUCCESS;
}

NTSTATUS
USBHUB_PdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    OUT ULONG_PTR* Information)
{
    PHUB_CHILDDEVICE_EXTENSION ChildDeviceExtension;
    ULONG IdType, StringLength = 0;
    PWCHAR SourceString = NULL, ReturnString = NULL;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;

    IdType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryId.IdType;
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    switch (IdType)
    {
        case BusQueryDeviceID:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryDeviceID\n");
            SourceString = ChildDeviceExtension->DeviceId;
            Status = STATUS_SUCCESS;
            break;
        }
        case BusQueryHardwareIDs:
        {
            ULONG Index = 0, LastIndex;
            PWCHAR Ptr;

            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryHardwareIDs\n");

            StringLength = wcslen(ChildDeviceExtension->DeviceId);
            StringLength += wcslen(L"&Rev_XXXX") + 1;
            StringLength += wcslen(ChildDeviceExtension->DeviceId) + 1;
            StringLength = StringLength * sizeof(WCHAR);

            ReturnString = ExAllocatePool(PagedPool, StringLength);
            Ptr = ReturnString;
            LastIndex = Index;
            Index += swprintf(&Ptr[Index],
                              L"%s&Rev_%04lx", ChildDeviceExtension->DeviceId,
                              ChildDeviceExtension->DeviceDesc.bcdDevice)  + 1;
            Ptr[Index] = UNICODE_NULL;
            DPRINT1("%S\n", &Ptr[LastIndex]);
            LastIndex = Index;
            Index += swprintf(&Ptr[Index], L"%s", ChildDeviceExtension->DeviceId)  + 1;
            Ptr[Index] = UNICODE_NULL;
            DPRINT1("%S\n", &Ptr[LastIndex]);
            Status = STATUS_SUCCESS;
            break;
        }
        case BusQueryCompatibleIDs:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryCompatibleIDs\n");
            //SourceString = ChildDeviceExtension->CompatibleIds;
            //return STATUS_NOT_SUPPORTED;
            break;
        }
        case BusQueryInstanceID:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / BusQueryInstanceID\n");
            SourceString = ChildDeviceExtension->InstanceId;
            Status = STATUS_SUCCESS;
            break;
        }
        default:
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_ID / unknown query id type 0x%lx\n", IdType);
            return STATUS_NOT_SUPPORTED;
    }

    if (SourceString)
    {
        StringLength = (wcslen(SourceString) + 1) * sizeof(WCHAR);
        DPRINT1("StringLen %d\n", StringLength);
        ReturnString = ExAllocatePool(PagedPool, StringLength);
        RtlCopyMemory(ReturnString, SourceString, StringLength);
        DPRINT1("%S\n", ReturnString);
    }

    *Information = (ULONG_PTR)ReturnString;

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
    PWCHAR SourceString = NULL, ReturnString = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    LCID LocaleId;
    ULONG StrLen;

    DeviceTextType = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.DeviceTextType;
    LocaleId = IoGetCurrentIrpStackLocation(Irp)->Parameters.QueryDeviceText.LocaleId;
    ChildDeviceExtension = (PHUB_CHILDDEVICE_EXTENSION)DeviceObject->DeviceExtension;

    //
    // FIXME: LocaleId
    //

    switch (DeviceTextType)
    {
        case DeviceTextDescription:
        case DeviceTextLocationInformation:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / DeviceTextDescription\n");
            SourceString = ChildDeviceExtension->TextDescription;
            DPRINT1("%S\n", SourceString);
            break;
        }
        default:
        {
            DPRINT1("IRP_MJ_PNP / IRP_MN_QUERY_DEVICE_TEXT / unknown device text type 0x%lx\n", DeviceTextType);
            Status = STATUS_NOT_SUPPORTED;
            break;
        }
    }

    if (SourceString)
    {
        StrLen = (wcslen(SourceString) + 1) * sizeof(WCHAR);
        ReturnString = ExAllocatePool(PagedPool, StrLen);
        RtlCopyMemory(ReturnString, SourceString, StrLen);
        DPRINT1("%S\n", ReturnString);
        *Information = (ULONG_PTR)ReturnString;
    }

    return Status;
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
            DeviceCapabilities->EjectSupported = TRUE;
            DeviceCapabilities->Removable = TRUE;
            DeviceCapabilities->DockDevice = FALSE;
            DeviceCapabilities->UniqueID = TRUE;
            DeviceCapabilities->SilentInstall = TRUE;
            DeviceCapabilities->RawDeviceOK = FALSE;
            DeviceCapabilities->SurpriseRemovalOK = TRUE;
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
        case IRP_MN_QUERY_BUS_INFORMATION:
        {
            PPNP_BUS_INFORMATION BusInfo;
            BusInfo = (PPNP_BUS_INFORMATION)ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
            RtlCopyMemory(&BusInfo->BusTypeGuid,
                          &GUID_BUS_TYPE_USB,
                          sizeof(BusInfo->BusTypeGuid));
            BusInfo->LegacyBusType = PNPBus;
            // FIXME
            BusInfo->BusNumber = 0;
            Information = (ULONG_PTR)BusInfo;
            Status = STATUS_SUCCESS;
            break;
        }
        case IRP_MN_REMOVE_DEVICE:
        {
            //
            // FIXME
            //
            Status = STATUS_SUCCESS;
        }
        default:
        {
            DPRINT1("PDO IRP_MJ_PNP / unknown minor function 0x%lx\n", MinorFunction);
            Information = Irp->IoStatus.Information;
            Status = Irp->IoStatus.Status;
        }
    }

    Irp->IoStatus.Information = Information;
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}

