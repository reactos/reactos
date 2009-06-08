/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/legacy/wdmaud/mixer.c
 * PURPOSE:         System Audio graph builder
 * PROGRAMMER:      Andrew Greenwood
 *                  Johannes Anderwald
 */
#include "wdmaud.h"

ULONG
IsVirtualDeviceATopologyFilter(
    IN  PDEVICE_OBJECT DeviceObject,
    ULONG VirtualDeviceId)
{
    KSP_PIN Pin;
    ULONG Count, BytesReturned, Index, NumPins;
    NTSTATUS Status;
    KSPIN_COMMUNICATION Communication;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    ULONG MixerPinCount;

    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
        return FALSE;

    if (VirtualDeviceId >= Count)
        return FALSE;

    /* query number of pins */
    Pin.Reserved = VirtualDeviceId; // see sysaudio
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;
    Pin.Property.Set = KSPROPSETID_Pin;
    Pin.Property.Id = KSPROPERTY_PIN_CTYPES;
    Pin.PinId = 0;

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&NumPins, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* enumerate now all pins */
    MixerPinCount = 0;
    for(Index = 0; Index < NumPins; Index++)
    {
        Pin.PinId = Index;
        Pin.Property.Id = KSPROPERTY_PIN_COMMUNICATION;
        Communication = KSPIN_COMMUNICATION_NONE;

        /* get pin communication type */
        Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)&Communication, sizeof(KSPIN_COMMUNICATION), &BytesReturned);
        if (NT_SUCCESS(Status))
        {
            if (Communication == KSPIN_COMMUNICATION_NONE)
                MixerPinCount++;
        }

    }

    if (MixerPinCount == NumPins)
    {
        /* filter has no pins which can be instantiated -> topology filter */
        return TRUE;
    }

    return FALSE;
}

ULONG
GetNumOfMixerPinsFromTopologyFilter(
    IN  PDEVICE_OBJECT DeviceObject,
    ULONG VirtualDeviceId)
{
    KSP_PIN Pin;
    ULONG BytesReturned, Index, NumPins;
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;
    PKSMULTIPLE_ITEM MultipleItem;
    PKSTOPOLOGY_CONNECTION Conn;

    Pin.PinId = 0;
    Pin.Reserved = VirtualDeviceId;
    Pin.Property.Set = KSPROPSETID_Topology;
    Pin.Property.Id = KSPROPERTY_TOPOLOGY_CONNECTIONS;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    BytesReturned = 0;
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)NULL, 0, &BytesReturned);

    if (Status != STATUS_BUFFER_TOO_SMALL)
        return 0;

    MultipleItem = ExAllocatePool(NonPagedPool, BytesReturned);
    if (!MultipleItem)
        return 0;

    RtlZeroMemory(MultipleItem, BytesReturned);

    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSP_PIN), (PVOID)MultipleItem, BytesReturned, &BytesReturned);
    if (!NT_SUCCESS(Status))
    {
        ExFreePool(MultipleItem);
        return 0;
    }

    Conn = (PKSTOPOLOGY_CONNECTION)(MultipleItem + 1);
    NumPins = 0;
    for (Index = 0; Index < MultipleItem->Count; Index++)
    {
        if (Conn[Index].ToNode == PCFILTER_NODE)
        {
            NumPins++;
        }
    }

    ExFreePool(MultipleItem);
    return NumPins;
}

ULONG
GetNumOfMixerDevices(
    IN  PDEVICE_OBJECT DeviceObject)
{
    KSP_PIN Pin;
    ULONG Count, BytesReturned, Index, NumPins;
    NTSTATUS Status;
    PWDMAUD_DEVICE_EXTENSION DeviceExtension;


    Pin.Property.Set = KSPROPSETID_Sysaudio;
    Pin.Property.Id = KSPROPERTY_SYSAUDIO_DEVICE_COUNT;
    Pin.Property.Flags = KSPROPERTY_TYPE_GET;

    DeviceExtension = (PWDMAUD_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    Count = 0;
    Status = KsSynchronousIoControlDevice(DeviceExtension->FileObject, KernelMode, IOCTL_KS_PROPERTY, (PVOID)&Pin, sizeof(KSPROPERTY), (PVOID)&Count, sizeof(ULONG), &BytesReturned);
    if (!NT_SUCCESS(Status) || !Count)
        return STATUS_UNSUCCESSFUL;

    NumPins = 0;
    for(Index = 0; Index < Count; Index++)
    {
        if (IsVirtualDeviceATopologyFilter(DeviceObject, Index))
        {
            NumPins += GetNumOfMixerPinsFromTopologyFilter(DeviceObject, Index);
        }
    }

    return NumPins;
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

