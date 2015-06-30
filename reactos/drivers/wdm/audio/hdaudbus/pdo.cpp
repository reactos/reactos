/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            drivers/wdm/audio/hdaudbus/pdo.cpp
* PURPOSE:         HDA Driver Entry
* PROGRAMMER:      Johannes Anderwald
*/
#include "hdaudbus.h"

NTSTATUS
HDA_PDOQueryBusInformation(
    IN PIRP Irp)
{
    PPNP_BUS_INFORMATION BusInformation;

    /* allocate bus information */
    BusInformation = (PPNP_BUS_INFORMATION)AllocateItem(PagedPool, sizeof(PNP_BUS_INFORMATION));

    if (!BusInformation)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* return info */
    BusInformation->BusNumber = 0;
    BusInformation->LegacyBusType = PCIBus;
    RtlMoveMemory(&BusInformation->BusTypeGuid, &GUID_HDAUDIO_BUS_INTERFACE, sizeof(GUID));

    /* store result */
    Irp->IoStatus.Information = (ULONG_PTR)BusInformation;

    /* done */
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
HDA_PDOQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    WCHAR DeviceName[200];
    PHDA_PDO_DEVICE_EXTENSION DeviceExtension;
    ULONG Length;
    LPWSTR Device;

    /* get device extension */
    DeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == FALSE);

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
    {
        UNIMPLEMENTED;

        // FIXME
        swprintf(DeviceName, L"%08x", 1);
        Length = wcslen(DeviceName) + 20;

        /* allocate result buffer*/
        Device = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
        if (!Device)
            return STATUS_INSUFFICIENT_RESOURCES;

        swprintf(Device, L"%08x", 1);

        DPRINT1("ID: %S\n", Device);
        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Device;
        return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID ||
        IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {

        /* calculate size */
        swprintf(DeviceName, L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X&SUBSYS_%08X", DeviceExtension->AudioGroup->FunctionGroup, DeviceExtension->Codec->VendorId, DeviceExtension->Codec->ProductId, DeviceExtension->Codec->VendorId << 16 | DeviceExtension->Codec->ProductId);
        Length = wcslen(DeviceName) + 20;

        /* allocate result buffer*/
        Device = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
        if (!Device)
            return STATUS_INSUFFICIENT_RESOURCES;

        wcscpy(Device, DeviceName);

        DPRINT1("ID: %S\n", Device);
        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Device;
        return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryCompatibleIDs)
    {
        RtlZeroMemory(DeviceName, sizeof(DeviceName));
        Length = swprintf(DeviceName, L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X&REV_%04X", DeviceExtension->AudioGroup->FunctionGroup, DeviceExtension->Codec->VendorId, DeviceExtension->Codec->ProductId, DeviceExtension->Codec->Major << 12 | DeviceExtension->Codec->Minor << 8 | DeviceExtension->Codec->Revision) + 1;
        Length += swprintf(&DeviceName[Length], L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X", DeviceExtension->AudioGroup->FunctionGroup, DeviceExtension->Codec->VendorId, DeviceExtension->Codec->ProductId) + 1;
        Length += swprintf(&DeviceName[Length], L"HDAUDIO\\FUNC_%02X&VEN_%04X", DeviceExtension->AudioGroup->FunctionGroup, DeviceExtension->Codec->VendorId) + 1;
        Length += swprintf(&DeviceName[Length], L"HDAUDIO\\FUNC_%02X&VEN_%04X", DeviceExtension->AudioGroup->FunctionGroup) + 2;

        /* allocate result buffer*/
        Device = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
        if (!Device)
            return STATUS_INSUFFICIENT_RESOURCES;

        RtlCopyMemory(Device, DeviceName, Length * sizeof(WCHAR));

        DPRINT1("ID: %S\n", Device);
        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Device;
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT1("QueryID Type %x not implemented\n", IoStack->Parameters.QueryId.IdType);
        return Irp->IoStatus.Status;
    }
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
HDA_PDOHandleQueryDeviceText(
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    LPWSTR Buffer;
    static WCHAR DeviceText[] = L"Audio Device on High Definition Audio Bus";

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription)
    {
        DPRINT("HDA_PdoHandleQueryDeviceText DeviceTextDescription\n");

        Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
        if (!Buffer)
        {
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(Buffer, DeviceText);

        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT("HDA_PdoHandleQueryDeviceText DeviceTextLocationInformation\n");

        Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
        if (!Buffer)
        {
            Irp->IoStatus.Information = 0;
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        wcscpy(Buffer, DeviceText);

        /* save result */
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }

}

NTSTATUS
HDA_PDOQueryBusDeviceCapabilities(
    IN PIRP Irp)
{
    PDEVICE_CAPABILITIES Capabilities;
    PIO_STACK_LOCATION IoStack;

    /* get stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* get capabilities */
    Capabilities = IoStack->Parameters.DeviceCapabilities.Capabilities;

    RtlZeroMemory(Capabilities, sizeof(DEVICE_CAPABILITIES));

    /* setup capabilities */
    Capabilities->UniqueID = TRUE;
    Capabilities->SilentInstall = TRUE;
    Capabilities->SurpriseRemovalOK = TRUE;
    Capabilities->Address = 0;
    Capabilities->UINumber = 0;
    Capabilities->SystemWake = PowerSystemWorking; /* FIXME common device extension */
    Capabilities->DeviceWake = PowerDeviceD0;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
HDA_PDOQueryBusDevicePnpState(
    IN PIRP Irp)
{
    /* set device flags */
    Irp->IoStatus.Information = PNP_DEVICE_DONT_DISPLAY_IN_UI | PNP_DEVICE_NOT_DISABLEABLE;

    /* done */
    return STATUS_SUCCESS;
}

