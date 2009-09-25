/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/mixer.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

const GUID KSNODETYPE_DAC = {0x507AE360L, 0xC554, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};
const GUID KSNODETYPE_ADC = {0x4D837FE0L, 0xC555, 0x11D0, {0x8A, 0x2B, 0x00, 0xA0, 0xC9, 0x25, 0x5A, 0xC1}};

ULONG
GetSysAudioDeviceCount(
    IN  PDEVICE_OBJECT DeviceObject)
{
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    KSPROPERTY Pin;
    ULONG Count, BytesReturned;
    NTSTATUS Status;

    /* setup the query request */
    Pin.Set = KSPROPSETID_Sysaudio;
    Pin.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Pin.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* query sysaudio for the device count */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
        return 0;

    return Count;
}

NTSTATUS
GetSysAudioDevicePnpName(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  ULONG DeviceIndex,
    OUT LPWSTR * Device)
{
    ULONG BytesReturned;
    KSP_PIN Pin;
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;

   /* first check if the device index is within bounds */
   if (DeviceIndex >= GetSysAudioDeviceCount(DeviceObject))
       return STATUS_INVALID_PARAMETER;

    /* setup the query request */
    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_INTERFACE_NAME;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.PinId = DeviceIndex;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* query sysaudio for the device path */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY) + sizeof(ULONG), NULL, 0, &BytesReturned);

    /* check if the request failed */
    if (Status != STATUS_BUFFER_TOO_SMALL || BytesReturned == 0)
        return STATUS_UNSUCCESSFUL;

    /* allocate buffer for the device */
    *Device = ExAllocatePool(NonPagedPool, BytesReturned);
    if (!Device)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* query sysaudio again for the device path */
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY) + sizeof(ULONG), (PVOID)*Device, BytesReturned, &BytesReturned);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(*Device);
        return Status;
    }

    return Status;
}

NTSTATUS
OpenSysAudioDeviceByIndex(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  ULONG DeviceIndex,
    IN  PHANDLE DeviceHandle,
    IN  PFILE_OBJECT * FileObject)
{
    LPWSTR Device = NULL;
    NTSTATUS Status;
    HANDLE hDevice;

    Status = GetSysAudioDevicePnpName(DeviceObject, DeviceIndex, &Device);
    if (!NT_SUCCESS(Status))
        return Status;

    /* now open the device */
    Status = WdmAudOpenSysAudioDevice(Device, &hDevice);

    /* free device buffer */
    ExFreePool(Device);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    *DeviceHandle = hDevice;

    if (FileObject)
    {
        Status = ObReferenceObjectByHandle(hDevice, FILE_READ_DATA | FILE_WRITE_DATA, IoFileObjectType, KernelMode, (PVOID*)FileObject, NULL);

        if (!NT_SUCCESS(Status))
        {
            ZwClose(hDevice);
        }
    }

    return Status;
}

NTSTATUS
GetFilterNodeTypes(
    PFILE_OBJECT FileObject,
    PKSMULTIPLE_ITEM * Item)
{
    NTSTATUS Status;
    ULONG BytesReturned;
    PKSMULTIPLE_ITEM MultipleItem;
    KSPROPERTY Property;

    /* setup query request */
    Property.Id = KSPROPERTY_TOPOLOGY_NODES;
    Property.Flags = KSPROPERTY_TYPE_GET;
    Property.Set = KSPROPSETID_Topology;

    /* query for required size */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), NULL, 0, &BytesReturned);

    /* check for success */
    if (Status != STATUS_MORE_ENTRIES)
        return Status;

    /* allocate buffer */
    MultipleItem = (PKSMULTIPLE_ITEM)ExAllocatePool(NonPagedPool, BytesReturned);
    if (!MultipleItem)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* query for required size */
    Status = KsSynchronousIoControlDevice(FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Property, sizeof(KSPROPERTY), (PVOID)MultipleItem, BytesReturned, &BytesReturned);

    if (!NT_SUCCESS(Status))
    {
        /* failed */
        ExFreePool(MultipleItem);
        return Status;
    }

    *Item = MultipleItem;
    return Status;
}

ULONG
CountNodeType(
    PKSMULTIPLE_ITEM MultipleItem,
    LPGUID NodeType)
{
    ULONG Count;
    ULONG Index;
    LPGUID Guid;

    Count = 0;
    Guid = (LPGUID)(MultipleItem+1);

    /* iterate through node type array */
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (IsEqualGUIDAligned(NodeType, Guid))
        {
            /* found matching guid */
            Count++;
        }
        Guid++;
    }
    return Count;
}

ULONG
GetNodeTypeIndex(
    PKSMULTIPLE_ITEM MultipleItem,
    LPGUID NodeType)
{
    ULONG Index;
    LPGUID Guid;

    Guid = (LPGUID)(MultipleItem+1);

    /* iterate through node type array */
    for(Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (IsEqualGUIDAligned(NodeType, Guid))
        {
            /* found matching guid */
            return Index;
        }
        Guid++;
    }
    return (ULONG)-1;
}

ULONG
GetNumOfMixerDevices(
    IN  PDEVICE_OBJECT DeviceObject)
{
    ULONG DeviceCount, Index, Count;
    NTSTATUS Status;
    HANDLE hDevice;
    PFILE_OBJECT FileObject;
    PKSMULTIPLE_ITEM MultipleItem;

    /* get number of devices */
    DeviceCount = GetSysAudioDeviceCount(DeviceObject);

    if (!DeviceCount)
        return 0;

    Index = 0;
    Count = 0;
    do
    {
        /* open the virtual audio device */
        Status = OpenSysAudioDeviceByIndex(DeviceObject, Index, &hDevice, &FileObject);

        if (NT_SUCCESS(Status))
        {
            /* retrieve all available node types */
            Status = GetFilterNodeTypes(FileObject, &MultipleItem);
            if (NT_SUCCESS(Status))
            {
                if (CountNodeType(MultipleItem, (LPGUID)&KSNODETYPE_DAC))
                {
                    /* increment (output) mixer count */
                    Count++;
                }

                if (CountNodeType(MultipleItem, (LPGUID)&KSNODETYPE_ADC))
                {
                    /* increment (input) mixer count */
                    Count++;
                }
                ExFreePool(MultipleItem);
            }
            ObDereferenceObject(FileObject);
            ZwClose(hDevice);
        }

        Index++;
    }while(Index < DeviceCount);

    return Count;
}

ULONG
IsOutputMixer(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG DeviceIndex)
{
    ULONG DeviceCount, Index, Count;
    NTSTATUS Status;
    HANDLE hDevice;
    PFILE_OBJECT FileObject;
    PKSMULTIPLE_ITEM MultipleItem;

    /* get number of devices */
    DeviceCount = GetSysAudioDeviceCount(DeviceObject);

    if (!DeviceCount)
        return 0;

    Index = 0;
    Count = 0;
    do
    {
        /* open the virtual audio device */
        Status = OpenSysAudioDeviceByIndex(DeviceObject, Index, &hDevice, &FileObject);

        if (NT_SUCCESS(Status))
        {
            /* retrieve all available node types */
            Status = GetFilterNodeTypes(FileObject, &MultipleItem);
            if (NT_SUCCESS(Status))
            {
                if (CountNodeType(MultipleItem, (LPGUID)&KSNODETYPE_DAC))
                {
                    /* increment (output) mixer count */
                    if (DeviceIndex == Count)
                    {
                        ExFreePool(MultipleItem);
                        ObDereferenceObject(FileObject);
                        ZwClose(hDevice);
                        return TRUE;
                    }

                    Count++;
                }

                if (CountNodeType(MultipleItem, (LPGUID)&KSNODETYPE_ADC))
                {
                    /* increment (input) mixer count */
                    if (DeviceIndex == Count)
                    {
                        ExFreePool(MultipleItem);
                        ObDereferenceObject(FileObject);
                        ZwClose(hDevice);
                        return FALSE;
                    }
                    Count++;
                }
                ExFreePool(MultipleItem);
            }
            ObDereferenceObject(FileObject);
            ZwClose(hDevice);
        }

        Index++;
    }while(Index < DeviceCount);

    ASSERT(0);
    return FALSE;
}





NTSTATUS
WdmAudMixerCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    NTSTATUS Status;
    LPWSTR Device;
    WCHAR Buffer[100];

    Status = GetSysAudioDevicePnpName(DeviceObject, DeviceInfo->DeviceIndex,&Device);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to get device name %x\n", Status);
        return Status;
    }

    DeviceInfo->u.MixCaps.cDestinations = 1; //FIXME

    Status = FindProductName(Device, sizeof(Buffer) / sizeof(WCHAR), Buffer);

    /* check for success */
    if (!NT_SUCCESS(Status))
    {
        DeviceInfo->u.MixCaps.szPname[0] = L'\0';
    }
    else
    {
        if (IsOutputMixer(DeviceObject, DeviceInfo->DeviceIndex))
        {
            wcscat(Buffer, L" output");
        }
        else
        {
            wcscat(Buffer, L" Input");
        }
        RtlMoveMemory(DeviceInfo->u.MixCaps.szPname, Buffer, min(MAXPNAMELEN, wcslen(Buffer)+1) * sizeof(WCHAR));
        DeviceInfo->u.MixCaps.szPname[MAXPNAMELEN-1] = L'\0';
    }

    return Status;
}


NTSTATUS
WdmAudControlOpenMixer(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    ULONG Index;
    PWDMAUD_HANDLE Handels;

    DPRINT("WdmAudControlOpenMixer\n");

    if (DeviceInfo->DeviceIndex >= GetNumOfMixerDevices(DeviceObject))
    {
        /* mixer index doesnt exist */
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, 0);
    }

    for(Index = 0; Index < ClientInfo->NumPins; Index++)
    {
        if (ClientInfo->hPins[Index].Handle == (HANDLE)DeviceInfo->DeviceIndex && ClientInfo->hPins[Index].Type == MIXER_DEVICE_TYPE)
        {
            /* re-use pseudo handle */
            DeviceInfo->hDevice = (HANDLE)DeviceInfo->DeviceIndex;
            return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
        }
    }

    Handels = ExAllocatePool(NonPagedPool, sizeof(WDMAUD_HANDLE) * (ClientInfo->NumPins+1));

    if (Handels)
    {
        if (ClientInfo->NumPins)
        {
            RtlMoveMemory(Handels, ClientInfo->hPins, sizeof(WDMAUD_HANDLE) * ClientInfo->NumPins);
            ExFreePool(ClientInfo->hPins);
        }

        ClientInfo->hPins = Handels;
        ClientInfo->hPins[ClientInfo->NumPins].Handle = (HANDLE)DeviceInfo->DeviceIndex;
        ClientInfo->hPins[ClientInfo->NumPins].Type = MIXER_DEVICE_TYPE;
        ClientInfo->NumPins++;
    }
    else
    {
        return SetIrpIoStatus(Irp, STATUS_UNSUCCESSFUL, sizeof(WDMAUD_DEVICE_INFO));
    }
    DeviceInfo->hDevice = (HANDLE)DeviceInfo->DeviceIndex;

    return SetIrpIoStatus(Irp, STATUS_SUCCESS, sizeof(WDMAUD_DEVICE_INFO));
}

NTSTATUS
NTAPI
WdmAudGetLineInfo(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

NTSTATUS
NTAPI
WdmAudGetLineControls(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

NTSTATUS
NTAPI
WdmAudSetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

NTSTATUS
NTAPI
WdmAudGetControlDetails(
    IN  PDEVICE_OBJECT DeviceObject,
    IN  PIRP Irp,
    IN  PWDMAUD_DEVICE_INFO DeviceInfo,
    IN  PWDMAUD_CLIENT ClientInfo)
{
    UNIMPLEMENTED;
    //DbgBreakPoint();
    return SetIrpIoStatus(Irp, STATUS_NOT_IMPLEMENTED, 0);

}

