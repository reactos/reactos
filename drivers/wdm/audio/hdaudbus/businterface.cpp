/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            drivers/wdm/audio/hdaudbus/hdaudbus.cpp
* PURPOSE:         HDA Driver Entry
* PROGRAMMER:      Johannes Anderwald
*/
#include "hdaudbus.h"


VOID
NTAPI
HDA_InterfaceReference(
PVOID BusContext)
{
    DPRINT1("HDA_InterfaceReference\n");
}

VOID
NTAPI
HDA_InterfaceDereference(
PVOID BusContext)
{
    DPRINT1("HDA_InterfaceDereference\n");
}

NTSTATUS
NTAPI
HDA_TransferCodecVerbs(
    IN PVOID _context,
    IN ULONG Count,
    IN OUT PHDAUDIO_CODEC_TRANSFER CodecTransfer,
    IN PHDAUDIO_TRANSFER_COMPLETE_CALLBACK Callback,
    IN PVOID Context)
{
    ULONG Verbs[MAX_CODEC_RESPONSES], Responses[MAX_CODEC_RESPONSES];
    ULONG Index;
    PHDA_PDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT1("HDA_TransferCodecVerbs Coun %lu CodecTransfer %p Callback %p Context %p\n", Count, CodecTransfer, Callback, Context);

    /* get device extension */
    DeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)_context;
    ASSERT(DeviceExtension->IsFDO == FALSE);

    /* FIXME handle callback*/
    ASSERT(Callback == NULL);

    for (Index = 0; Index < Count; Index++)
    {
        Verbs[Index] = CodecTransfer[Index].Output.Command;
    }

    HDA_SendVerbs(DeviceExtension->FDO, DeviceExtension->Codec, Verbs, Responses, Count);

    for (Index = 0; Index < DeviceExtension->Codec->ResponseCount; Index++)
    {
        CodecTransfer[Index].Input.Response = DeviceExtension->Codec->Responses[Index];
    }
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI 
HDA_AllocateCaptureDmaEngine(
    IN PVOID _context,
    IN UCHAR CodecAddress,
    IN PHDAUDIO_STREAM_FORMAT StreamFormat,
    OUT PHANDLE Handle,
    OUT PHDAUDIO_CONVERTER_FORMAT ConverterFormat)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_AllocateRenderDmaEngine(
    IN PVOID _context,
    IN PHDAUDIO_STREAM_FORMAT StreamFormat,
    IN BOOLEAN Stripe,
    OUT PHANDLE Handle,
    OUT PHDAUDIO_CONVERTER_FORMAT ConverterFormat)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_ChangeBandwidthAllocation(
    IN PVOID _context,
    IN HANDLE Handle,
    IN PHDAUDIO_STREAM_FORMAT StreamFormat,
    OUT PHDAUDIO_CONVERTER_FORMAT ConverterFormat)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_AllocateDmaBuffer(
    IN PVOID _context,
    IN HANDLE Handle,
    IN SIZE_T RequestedBufferSize,
    OUT PMDL *BufferMdl,
    OUT PSIZE_T AllocatedBufferSize,
    OUT PUCHAR StreamId,
    OUT PULONG FifoSize)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_FreeDmaBuffer(
    IN PVOID _context,
    IN HANDLE Handle)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_FreeDmaEngine(
    IN PVOID _context,
    IN HANDLE Handle)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_SetDmaEngineState(
    IN PVOID _context,
    IN HDAUDIO_STREAM_STATE StreamState,
    IN ULONG NumberOfHandles,
    IN PHANDLE Handles)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
HDA_GetWallClockRegister(
    IN PVOID _context,
    OUT PULONG *Wallclock)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
}

NTSTATUS
NTAPI
HDA_GetLinkPositionRegister(
    IN PVOID _context,
    IN HANDLE Handle,
    OUT PULONG *Position)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_RegisterEventCallback(
    IN PVOID _context,
    IN PHDAUDIO_UNSOLICITED_RESPONSE_CALLBACK Routine,
    IN PVOID Context,
    OUT PUCHAR Tag)
{
    UNIMPLEMENTED;
	*Tag = 1;
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
HDA_UnregisterEventCallback(
IN PVOID _context,
IN UCHAR Tag)
{
    UNIMPLEMENTED;
    ASSERT(FALSE);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
HDA_GetDeviceInformation(
    IN PVOID _context,
    OUT PHDAUDIO_DEVICE_INFORMATION DeviceInformation)
{
    PHDA_PDO_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)_context;

    DPRINT1("HDA_GetDeviceInformation\n");

    DeviceInformation->Size = sizeof(HDAUDIO_DEVICE_INFORMATION);
    DeviceInformation->CodecsDetected = 1;  // FIXME
    DeviceInformation->DeviceVersion = DeviceExtension->Codec->Major << 8 | DeviceExtension->Codec->Minor;
    DeviceInformation->DriverVersion = DeviceExtension->Codec->Major << 8 | DeviceExtension->Codec->Minor;
    DeviceInformation->IsStripingSupported = FALSE;

    return STATUS_SUCCESS;
}

VOID
NTAPI
HDA_GetResourceInformation(
    IN PVOID _context,
    OUT PUCHAR CodecAddress,
    OUT PUCHAR FunctionGroupStartNode)
{
    PHDA_PDO_DEVICE_EXTENSION DeviceExtension;

    DeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)_context;

    DPRINT1("HDA_GetResourceInformation Addr %x NodeId %x\n", DeviceExtension->Codec->Addr, DeviceExtension->AudioGroup->NodeId);

    *CodecAddress = DeviceExtension->Codec->Addr;
    *FunctionGroupStartNode = DeviceExtension->AudioGroup->NodeId;
}

NTSTATUS
NTAPI
HDA_AllocateDmaBufferWithNotification(
    IN PVOID _context,
	IN HANDLE Handle,
	IN ULONG NotificationCount,
	IN SIZE_T RequestedBufferSize,
	OUT PMDL *BufferMdl,
	OUT PSIZE_T AllocatedBufferSize,
	OUT PSIZE_T OffsetFromFirstPage,
	OUT PUCHAR StreamId,
	OUT PULONG FifoSize)
{
	UNIMPLEMENTED;
	ASSERT(FALSE);
	return STATUS_NOT_IMPLEMENTED;

}
NTSTATUS
NTAPI
HDA_FreeDmaBufferWithNotification(
    IN PVOID _context,
	IN HANDLE Handle,
	IN PMDL BufferMdl,
	IN SIZE_T BufferSize)
{
	UNIMPLEMENTED;
	ASSERT(FALSE);
	return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
HDA_RegisterNotificationEvent(
    PVOID _context,
    HANDLE Handle,
    IN PKEVENT NotificationEvent)
{
	UNIMPLEMENTED;
	ASSERT(FALSE);
	return STATUS_NOT_IMPLEMENTED;

}

NTSTATUS
NTAPI
HDA_UnregisterNotificationEvent(
    IN PVOID _context,
    IN HANDLE Handle,
    IN PKEVENT NotificationEvent)
{
	UNIMPLEMENTED;
	ASSERT(FALSE);
	return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
HDA_PDOHandleQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PHDAUDIO_BUS_INTERFACE_V2 InterfaceHDA;
    PHDA_PDO_DEVICE_EXTENSION DeviceExtension;

    /* get device extension */
    DeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == FALSE);

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, GUID_HDAUDIO_BUS_INTERFACE))
    {
        InterfaceHDA = (PHDAUDIO_BUS_INTERFACE_V2)IoStack->Parameters.QueryInterface.Interface;
        InterfaceHDA->Version = IoStack->Parameters.QueryInterface.Version;
        InterfaceHDA->Size = sizeof(HDAUDIO_BUS_INTERFACE);
        InterfaceHDA->Context = DeviceExtension;
        InterfaceHDA->InterfaceReference = HDA_InterfaceReference;
        InterfaceHDA->InterfaceDereference = HDA_InterfaceDereference;

        InterfaceHDA->TransferCodecVerbs = HDA_TransferCodecVerbs;
        InterfaceHDA->AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
        InterfaceHDA->AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
        InterfaceHDA->ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
        InterfaceHDA->AllocateDmaBuffer = HDA_AllocateDmaBuffer;
        InterfaceHDA->FreeDmaBuffer = HDA_FreeDmaBuffer;
        InterfaceHDA->FreeDmaEngine = HDA_FreeDmaEngine;
        InterfaceHDA->SetDmaEngineState = HDA_SetDmaEngineState;
        InterfaceHDA->GetWallClockRegister = HDA_GetWallClockRegister;
        InterfaceHDA->GetLinkPositionRegister = HDA_GetLinkPositionRegister;
        InterfaceHDA->RegisterEventCallback = HDA_RegisterEventCallback;
        InterfaceHDA->UnregisterEventCallback = HDA_UnregisterEventCallback;
        InterfaceHDA->GetDeviceInformation = HDA_GetDeviceInformation;
        InterfaceHDA->GetResourceInformation = HDA_GetResourceInformation;
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(*IoStack->Parameters.QueryInterface.InterfaceType, GUID_HDAUDIO_BUS_INTERFACE_V2))
    {
        InterfaceHDA = (PHDAUDIO_BUS_INTERFACE_V2)IoStack->Parameters.QueryInterface.Interface;
        InterfaceHDA->Version = IoStack->Parameters.QueryInterface.Version;
        InterfaceHDA->Size = sizeof(HDAUDIO_BUS_INTERFACE_V2);
        InterfaceHDA->Context = DeviceExtension;
        InterfaceHDA->InterfaceReference = HDA_InterfaceReference;
        InterfaceHDA->InterfaceDereference = HDA_InterfaceDereference;

        InterfaceHDA->TransferCodecVerbs = HDA_TransferCodecVerbs;
        InterfaceHDA->AllocateCaptureDmaEngine = HDA_AllocateCaptureDmaEngine;
        InterfaceHDA->AllocateRenderDmaEngine = HDA_AllocateRenderDmaEngine;
        InterfaceHDA->ChangeBandwidthAllocation = HDA_ChangeBandwidthAllocation;
        InterfaceHDA->AllocateDmaBuffer = HDA_AllocateDmaBuffer;
        InterfaceHDA->FreeDmaBuffer = HDA_FreeDmaBuffer;
        InterfaceHDA->FreeDmaEngine = HDA_FreeDmaEngine;
        InterfaceHDA->SetDmaEngineState = HDA_SetDmaEngineState;
        InterfaceHDA->GetWallClockRegister = HDA_GetWallClockRegister;
        InterfaceHDA->GetLinkPositionRegister = HDA_GetLinkPositionRegister;
        InterfaceHDA->RegisterEventCallback = HDA_RegisterEventCallback;
        InterfaceHDA->UnregisterEventCallback = HDA_UnregisterEventCallback;
        InterfaceHDA->GetDeviceInformation = HDA_GetDeviceInformation;
        InterfaceHDA->GetResourceInformation = HDA_GetResourceInformation;

        InterfaceHDA->AllocateDmaBufferWithNotification = HDA_AllocateDmaBufferWithNotification;
        InterfaceHDA->FreeDmaBufferWithNotification = HDA_FreeDmaBufferWithNotification;
        InterfaceHDA->RegisterNotificationEvent = HDA_RegisterNotificationEvent;
        InterfaceHDA->UnregisterNotificationEvent = HDA_UnregisterNotificationEvent;
    }

    // FIXME
    // implement support for GUID_HDAUDIO_BUS_INTERFACE_BDL
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}
