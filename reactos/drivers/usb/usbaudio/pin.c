/*
* PROJECT:     ReactOS Universal Audio Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbaudio/pin.c
* PURPOSE:     USB Audio device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "usbaudio.h"

NTSTATUS
GetMaxPacketSizeForInterface(
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    KSPIN_DATAFLOW DataFlow)
{
    PUSB_COMMON_DESCRIPTOR CommonDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor;

    /* loop descriptors */
    CommonDescriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);
    ASSERT(InterfaceDescriptor->bNumEndpoints > 0);
    while (CommonDescriptor)
    {
        if (CommonDescriptor->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
        {
            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)CommonDescriptor;
            return EndpointDescriptor->wMaxPacketSize;
        }

        if (CommonDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
        {
            /* reached next interface descriptor */
            break;
        }

        if ((ULONG_PTR)CommonDescriptor + CommonDescriptor->bLength >= ((ULONG_PTR)ConfigurationDescriptor + ConfigurationDescriptor->wTotalLength))
            break;

        CommonDescriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)CommonDescriptor + CommonDescriptor->bLength);
    }

    /* default to 100 */
    return 100;
}

NTSTATUS
UsbAudioAllocCaptureUrbIso(
    IN USBD_PIPE_HANDLE PipeHandle,
    IN ULONG MaxPacketSize,
    IN PVOID Buffer,
    IN ULONG BufferLength,
    OUT PURB * OutUrb)
{
    PURB Urb;
    ULONG PacketCount;
    ULONG UrbSize;
    ULONG Index;

    /* calculate packet count */
    PacketCount = BufferLength / MaxPacketSize;

    /* calculate urb size*/
    UrbSize = GET_ISO_URB_SIZE(PacketCount);

    /* allocate urb */
    Urb = AllocFunction(UrbSize);
    if (!Urb)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init urb */
    Urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    Urb->UrbIsochronousTransfer.Hdr.Length = UrbSize;
    Urb->UrbIsochronousTransfer.PipeHandle = PipeHandle;
    Urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_START_ISO_TRANSFER_ASAP;
    Urb->UrbIsochronousTransfer.TransferBufferLength = BufferLength;
    Urb->UrbIsochronousTransfer.TransferBuffer = Buffer;
    Urb->UrbIsochronousTransfer.NumberOfPackets = PacketCount;

    for (Index = 0; Index < PacketCount; Index++)
    {
        Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset = Index * MaxPacketSize;
    }

    *OutUrb = Urb;
    return STATUS_SUCCESS;

}



NTSTATUS
UsbAudioSetFormat(
    IN PKSPIN Pin)
{
    PURB Urb;
    PUCHAR SampleRateBuffer;
    PPIN_CONTEXT PinContext;
    NTSTATUS Status;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormatEx;

    /* allocate sample rate buffer */
    SampleRateBuffer = AllocFunction(sizeof(ULONG));
    if (!SampleRateBuffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (IsEqualGUIDAligned(&Pin->ConnectionFormat->MajorFormat, &KSDATAFORMAT_TYPE_AUDIO) &&
        IsEqualGUIDAligned(&Pin->ConnectionFormat->SubFormat, &KSDATAFORMAT_SUBTYPE_PCM) &&
        IsEqualGUIDAligned(&Pin->ConnectionFormat->Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX))
    {
        WaveFormatEx = (PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat;
        SampleRateBuffer[2] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 16) & 0xFF;
        SampleRateBuffer[1] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 8) & 0xFF;
        SampleRateBuffer[0] = (WaveFormatEx->WaveFormatEx.nSamplesPerSec >> 0) & 0xFF;

        /* TODO: verify connection format */
    }
    else
    {
        /* not supported yet*/
        UNIMPLEMENTED;
        FreeFunction(SampleRateBuffer);
        return STATUS_INVALID_PARAMETER;
    }

    /* allocate urb */
    Urb = AllocFunction(sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST));
    if (!Urb)
    {
        /* no memory */
        FreeFunction(SampleRateBuffer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* format urb */
    UsbBuildVendorRequest(Urb,
        URB_FUNCTION_CLASS_ENDPOINT,
        sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        USBD_TRANSFER_DIRECTION_OUT,
        0,
        0x01,
        0x100,
        0x81, //FIXME bEndpointAddress
        SampleRateBuffer,
        NULL,
        3,
        NULL);

    /* get pin context */
    PinContext = Pin->Context;

    /* submit urb */
    Status = SubmitUrbSync(PinContext->LowerDevice, Urb);

    DPRINT1("USBAudioPinSetDataFormat Pin %p Status %x\n", Pin, Status);
    FreeFunction(Urb);
    FreeFunction(SampleRateBuffer);
    return Status;
}

NTSTATUS
USBAudioSelectAudioStreamingInterface(
    IN PPIN_CONTEXT PinContext,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    PURB Urb;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    NTSTATUS Status;

    /* grab interface descriptor */
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    if (!InterfaceDescriptor)
    {
        /* no such interface */
        return STATUS_INVALID_PARAMETER;
    }

    /* FIXME selects the first interface with audio streaming and non zero num of endpoints */
    while (InterfaceDescriptor != NULL)
    {
        if (InterfaceDescriptor->bInterfaceSubClass == 0x02 /* AUDIO_STREAMING */ && InterfaceDescriptor->bNumEndpoints > 0) 
        {
            break;
        }
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, (PVOID)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength), -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    }

    if (!InterfaceDescriptor)
    {
        /* no such interface */
        return STATUS_INVALID_PARAMETER;
    }

    Urb = AllocFunction(GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints));
    if (!Urb)
    {
        /* no memory */
        return USBD_STATUS_INSUFFICIENT_RESOURCES;
    }

     /* now prepare interface urb */
     UsbBuildSelectInterfaceRequest(Urb, GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints), DeviceExtension->ConfigurationHandle, InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting);

     /* copy interface information */
     RtlCopyMemory(&Urb->UrbSelectInterface.Interface, DeviceExtension->InterfaceInfo, DeviceExtension->InterfaceInfo->Length);

     /* now select the interface */
     Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);

     DPRINT1("USBAudioSelectAudioStreamingInterface Status %x UrbStatus %x\n", Status, Urb->UrbSelectInterface.Hdr.Status);

     /* did it succeeed */
     if (NT_SUCCESS(Status))
     {
         /* update configuration info */
         ASSERT(Urb->UrbSelectInterface.Interface.Length == DeviceExtension->InterfaceInfo->Length);
         RtlCopyMemory(DeviceExtension->InterfaceInfo, &Urb->UrbSelectInterface.Interface, Urb->UrbSelectInterface.Interface.Length);
         PinContext->InterfaceDescriptor = InterfaceDescriptor;
     }

     /* free urb */
     FreeFunction(Urb);
     return Status;
}

NTSTATUS
InitCapturePin(
    IN PKSPIN Pin)
{
    NTSTATUS Status;
    ULONG Index;
    ULONG BufferSize;
    ULONG MaximumPacketSize;
    PIRP Irp;
    PURB Urb;
    PPIN_CONTEXT PinContext;
    PIO_STACK_LOCATION IoStack;

    /* set sample rate */
    Status = UsbAudioSetFormat(Pin);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* get pin context */
    PinContext = Pin->Context;
	DbgBreakPoint();
    MaximumPacketSize = GetMaxPacketSizeForInterface(PinContext->DeviceExtension->ConfigurationDescriptor, PinContext->InterfaceDescriptor, Pin->DataFlow);

    /* calculate buffer size 8 irps * 10 iso packets * max packet size */
    BufferSize = 8 * 10 * MaximumPacketSize;

    /* allocate pin capture buffer */
    PinContext->Buffer = AllocFunction(BufferSize);
    if (!PinContext->Buffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    KsAddItemToObjectBag(Pin->Bag, PinContext->Buffer, ExFreePool);

    /* init irps */
    for (Index = 0; Index < 8; Index++)
    {
        /* allocate irp */
        Irp = IoAllocateIrp(PinContext->DeviceExtension->LowerDevice->StackSize, FALSE);
        if (!Irp)
        {
            /* no memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* insert into irp list */
        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* add to object bag*/
        KsAddItemToObjectBag(Pin->Bag, Irp, IoFreeIrp);

        Status = UsbAudioAllocCaptureUrbIso(PinContext->DeviceExtension->InterfaceInfo->Pipes[0].PipeHandle, 
                                            MaximumPacketSize,
                                            &PinContext->Buffer[MaximumPacketSize * 10 * Index], 
                                            MaximumPacketSize * 10,
                                            &Urb);

        if (NT_SUCCESS(Status))
        {
            /* get next stack location */
            IoStack = IoGetNextIrpStackLocation(Irp);

            /* store urb */
            IoStack->Parameters.Others.Argument1 = Urb;
        }
        else
        {
            /* failed */
            return Status;
        }
    }
    return Status;
}

NTSTATUS
InitStreamPin(
    IN PKSPIN Pin)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
USBAudioPinCreate(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp)
{
    PKSFILTER Filter;
    PFILTER_CONTEXT FilterContext;
    PPIN_CONTEXT PinContext;
    NTSTATUS Status;

    Filter = KsPinGetParentFilter(Pin);
    if (Filter == NULL)
    {
        /* invalid parameter */
        return STATUS_INVALID_PARAMETER;
    }

    /* get filter context */
    FilterContext = Filter->Context;

    /* allocate pin context */
    PinContext = AllocFunction(sizeof(PIN_CONTEXT));
    if (!PinContext)
    {
        /* no memory*/
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init pin context */
    PinContext->DeviceExtension = FilterContext->DeviceExtension;
    PinContext->LowerDevice = FilterContext->LowerDevice;
    InitializeListHead(&PinContext->IrpListHead);

    /* store pin context*/
    Pin->Context = PinContext;

    /* select streaming interface */
    Status = USBAudioSelectAudioStreamingInterface(PinContext, PinContext->DeviceExtension, PinContext->DeviceExtension->ConfigurationDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    if (Pin->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        /* init capture pin */
        Status = InitCapturePin(Pin);
    }
    else
    {
        /* audio streaming pin*/
        Status = InitStreamPin(Pin);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBAudioPinClose(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
USBAudioPinProcess(
    _In_ PKSPIN Pin)
{
    return STATUS_SUCCESS;
}


VOID
NTAPI
USBAudioPinReset(
    _In_ PKSPIN Pin)
{
    UNIMPLEMENTED
}

NTSTATUS
NTAPI
USBAudioPinSetDataFormat(
    _In_ PKSPIN Pin,
    _In_opt_ PKSDATAFORMAT OldFormat,
    _In_opt_ PKSMULTIPLE_ITEM OldAttributeList,
    _In_ const KSDATARANGE* DataRange,
    _In_opt_ const KSATTRIBUTE_LIST* AttributeRange)
{
    if (OldFormat == NULL)
    {
        /* TODO: verify connection format */
        UNIMPLEMENTED
        return STATUS_SUCCESS;
    }

    return UsbAudioSetFormat(Pin);
}

NTSTATUS
NTAPI
USBAudioPinSetDeviceState(
    _In_ PKSPIN Pin,
    _In_ KSSTATE ToState,
    _In_ KSSTATE FromState)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}
