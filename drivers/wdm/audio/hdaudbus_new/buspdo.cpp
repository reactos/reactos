#include "driver.h"
#include "adsp.h"

NTSTATUS
HDA_PDORemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PPDO_DEVICE_DATA DeviceExtension;
    ULONG CodecIndex;

    /* get device extension */
    DeviceExtension = static_cast<PPDO_DEVICE_DATA>(DeviceObject->DeviceExtension);
    ASSERT(DeviceExtension->IsFDO == FALSE);

    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        if (DeviceExtension->FdoContext->codecs[CodecIndex] == NULL)
            continue;

        if (DeviceExtension->FdoContext->codecs[CodecIndex]->ChildPDO == DeviceObject)
        {
            DPRINT1("Removing reference\n");
            DeviceExtension->FdoContext->codecs[CodecIndex]->ChildPDO = NULL;
        }
    }
    IoDeleteDevice(DeviceObject);
    return STATUS_SUCCESS;
}

NTSTATUS
HDA_PDOQueryBusInformation(
    IN PIRP Irp)
{
    PPNP_BUS_INFORMATION BusInformation;

    DPRINT1("HDA_PDOQueryBusInformation\n");

    /* allocate bus information */
    BusInformation = (PPNP_BUS_INFORMATION)AllocateItem(PagedPool, sizeof(PNP_BUS_INFORMATION));

    if (!BusInformation)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* return info */
    BusInformation->BusNumber = 0;
    BusInformation->LegacyBusType = PNPBus;
    RtlCopyMemory(&BusInformation->BusTypeGuid, &GUID_HDAUDIO_BUS_CLASS, sizeof(GUID));

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
    PPDO_DEVICE_DATA DeviceExtension;
    PFDO_CONTEXT FdoDeviceExtension;
    ULONG Length;
    LPWSTR Device;
    NTSTATUS Status;

    /* get device extension */
    DeviceExtension = (PPDO_DEVICE_DATA)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == FALSE);

    /* get FDO device extension */
    FdoDeviceExtension = DeviceExtension->FdoContext;
    ASSERT(FdoDeviceExtension->IsFDO);

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.QueryId.IdType == BusQueryInstanceID)
    {
        Status = RtlStringCbPrintfW(DeviceName,
                                    sizeof(DeviceName),
                                    L"%02x%02x",
                                    DeviceExtension->CodecIds.CodecAddress,
                                    DeviceExtension->CodecIds.FunctionGroupStartNode);
        NT_ASSERT(NT_SUCCESS(Status));
        Length = wcslen(DeviceName) + 1;

        /* allocate result buffer*/
        Device = (LPWSTR)AllocateItem(PagedPool, Length * sizeof(WCHAR));
        if (!Device)
            return STATUS_INSUFFICIENT_RESOURCES;

        Status = RtlStringCbCopyW(Device,
                                  Length * sizeof(WCHAR),
                                  DeviceName);
        NT_ASSERT(NT_SUCCESS(Status));

        DPRINT1("ID: %S\n", Device);
        /* store result */
        Irp->IoStatus.Information = (ULONG_PTR)Device;
        return STATUS_SUCCESS;
    }
    else if (IoStack->Parameters.QueryId.IdType == BusQueryDeviceID ||
        IoStack->Parameters.QueryId.IdType == BusQueryHardwareIDs)
    {

        /* calculate size */
        swprintf(DeviceName, L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X&SUBSYS_%08X",
            DeviceExtension->CodecIds.FuncId,
            DeviceExtension->CodecIds.VenId,
            DeviceExtension->CodecIds.DevId,
            DeviceExtension->CodecIds.SubsysId);

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
        Length = swprintf(DeviceName, L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X&REV_%04X", DeviceExtension->CodecIds.FuncId, DeviceExtension->CodecIds.VenId, DeviceExtension->CodecIds.DevId, DeviceExtension->CodecIds.SubsysId) + 1;
        Length += swprintf(&DeviceName[Length], L"HDAUDIO\\FUNC_%02X&VEN_%04X&DEV_%04X", DeviceExtension->CodecIds.FuncId, DeviceExtension->CodecIds.VenId, DeviceExtension->CodecIds.DevId) + 1;
        Length += swprintf(&DeviceName[Length], L"HDAUDIO\\FUNC_%02X&VEN_%04X", DeviceExtension->CodecIds.FuncId, DeviceExtension->CodecIds.VenId) + 1;
        Length += swprintf(&DeviceName[Length], L"HDAUDIO\\FUNC_%02X", DeviceExtension->CodecIds.FuncId) + 2;

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
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    LPWSTR Buffer;
    PPDO_DEVICE_DATA DeviceExtension;

    static WCHAR DeviceText[] = L"Audio Device on High Definition Audio Bus";
    static WCHAR ModemText[] = L"Modem Device on High Definition Audio Bus";

    /* get device extension */
    DeviceExtension = (PPDO_DEVICE_DATA)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.QueryDeviceText.DeviceTextType == DeviceTextDescription)
    {
        DPRINT("HDA_PdoHandleQueryDeviceText DeviceTextDescription\n");

        if (DeviceExtension->CodecIds.IsDSP)
        {
            Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
            if (!Buffer)
            {
                Irp->IoStatus.Information = 0;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            wcscpy(Buffer, DeviceText);
        }
        else
        {
            Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(ModemText));
            if (!Buffer)
            {
                Irp->IoStatus.Information = 0;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            wcscpy(Buffer, ModemText);

        }
        Irp->IoStatus.Information = (ULONG_PTR)Buffer;
        return STATUS_SUCCESS;
    }
    else
    {
        DPRINT("HDA_PdoHandleQueryDeviceText DeviceTextLocationInformation\n");

        if (DeviceExtension->CodecIds.IsDSP)
        {
            Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(DeviceText));
            if (!Buffer)
            {
                Irp->IoStatus.Information = 0;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            wcscpy(Buffer, DeviceText);
        }
        else
        {
            Buffer = (LPWSTR)AllocateItem(PagedPool, sizeof(ModemText));
            if (!Buffer)
            {
                Irp->IoStatus.Information = 0;
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            wcscpy(Buffer, ModemText);
        }

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
    Capabilities->Size = sizeof(DEVICE_CAPABILITIES);
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

NTSTATUS
HDA_PDOHandleQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PPDO_DEVICE_DATA DeviceExtension;
    UNICODE_STRING GuidString;
    NTSTATUS Status;

    /* get device extension */
    DeviceExtension = (PPDO_DEVICE_DATA)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, GUID_HDAUDIO_BUS_INTERFACE))
    {
        PHDAUDIO_BUS_INTERFACE InterfaceHDA = (PHDAUDIO_BUS_INTERFACE)IoStack->Parameters.QueryInterface.Interface;
        HDA_BusInterface(DeviceExtension, InterfaceHDA);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, GUID_HDAUDIO_BUS_INTERFACE_V2))
    {
        PHDAUDIO_BUS_INTERFACE_V2 InterfaceHDA = (PHDAUDIO_BUS_INTERFACE_V2)IoStack->Parameters.QueryInterface.Interface;
        HDA_BusInterfaceV2(DeviceExtension, InterfaceHDA);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, GUID_HDAUDIO_BUS_INTERFACE_BDL))
    {
        PHDAUDIO_BUS_INTERFACE_BDL InterfaceHDA = (PHDAUDIO_BUS_INTERFACE_BDL)IoStack->Parameters.QueryInterface.Interface;
        HDA_BusInterfaceBDL(DeviceExtension, InterfaceHDA);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, GUID_BUS_INTERFACE_STANDARD))
    {
        PBUS_INTERFACE_STANDARD InterfaceBus = (PBUS_INTERFACE_STANDARD)IoStack->Parameters.QueryInterface.Interface;
        HDA_BusInterfaceStandard(DeviceExtension, InterfaceBus);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, KSMEDIUMSETID_Standard))
    {
        PBUS_INTERFACE_REFERENCE InterfaceBus = (PBUS_INTERFACE_REFERENCE)IoStack->Parameters.QueryInterface.Interface;
        HDA_BusInterfaceReference(DeviceExtension, InterfaceBus);
        return STATUS_SUCCESS;
    }

    Status = RtlStringFromGUID(*IoStack->Parameters.QueryInterface.InterfaceType, &GuidString);
    if (NT_SUCCESS(Status))
    {
        DPRINT1("UNIMPLEMENTED InterfaceType: %wZ\n", &GuidString);
    }
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PDO_CreateDevices(PDEVICE_OBJECT DeviceObject)
{
    PFDO_CONTEXT FdoDeviceExtension;
    ULONG Index;
    PPDO_DEVICE_DATA ChildDeviceExtension;
    PDEVICE_OBJECT ChildPDO;
    NTSTATUS Status;

    FdoDeviceExtension = (PFDO_CONTEXT)DeviceObject->DeviceExtension;
    DPRINT1("PDO_CreateDevices numCodecs %u\n", FdoDeviceExtension->numCodecs);

    for (Index = 0; Index < FdoDeviceExtension->numCodecs; Index++)
    {
        Status = IoCreateDevice(DeviceObject->DriverObject, sizeof(PDO_DEVICE_DATA), NULL, FILE_DEVICE_SOUND, FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &ChildPDO);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("hda failed to create device object %x\n", Status);
            return Status;
        }

        FdoDeviceExtension->codecs[Index] = ChildDeviceExtension = (PPDO_DEVICE_DATA)ChildPDO->DeviceExtension;
        RtlZeroMemory(ChildDeviceExtension, sizeof(PDO_DEVICE_DATA));
        ChildDeviceExtension->FdoContext = FdoDeviceExtension;
        ChildDeviceExtension->ChildPDO = ChildPDO;
        ChildDeviceExtension->IsFDO = FALSE;
        RtlCopyMemory(&ChildDeviceExtension->CodecIds, &FdoDeviceExtension->CodecIds[Index], sizeof(CODEC_IDS));
        /* setup flags */
        ChildPDO->Flags |= DO_POWER_PAGABLE;
        ChildPDO->Flags &= ~DO_DEVICE_INITIALIZING;
    }
    return STATUS_SUCCESS;
}
