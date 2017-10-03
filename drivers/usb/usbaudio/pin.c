/*
* PROJECT:     ReactOS Universal Audio Class Driver
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        drivers/usb/usbaudio/pin.c
* PURPOSE:     USB Audio device driver.
* PROGRAMMERS:
*              Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "usbaudio.h"

#define PACKET_COUNT 10


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
    ULONG UrbSize;
    ULONG Index;

    /* calculate urb size*/
    UrbSize = GET_ISO_URB_SIZE(PACKET_COUNT);

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
    Urb->UrbIsochronousTransfer.NumberOfPackets = PACKET_COUNT;

    for (Index = 0; Index < PACKET_COUNT; Index++)
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

    /* get pin context */
    PinContext = Pin->Context;

    /* FIXME: determine controls and format urb */
    UsbBuildVendorRequest(Urb,
        URB_FUNCTION_CLASS_ENDPOINT,
        sizeof(struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST),
        USBD_TRANSFER_DIRECTION_OUT,
        0,
        0x01, // SET_CUR
        0x100,
        PinContext->DeviceExtension->InterfaceInfo->Pipes[0].EndpointAddress,
        SampleRateBuffer,
        NULL,
        3,
        NULL);



    /* submit urb */
    Status = SubmitUrbSync(PinContext->LowerDevice, Urb);

    DPRINT1("USBAudioPinSetDataFormat Pin %p Status %x\n", Pin, Status);
    FreeFunction(Urb);
    FreeFunction(SampleRateBuffer);
    return Status;
}

NTSTATUS
USBAudioSelectAudioStreamingInterface(
    IN PKSPIN Pin,
    IN PPIN_CONTEXT PinContext,
    IN PDEVICE_EXTENSION DeviceExtension,
    IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    IN ULONG FormatDescriptorIndex)
{
    PURB Urb;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    NTSTATUS Status;
    ULONG Found, Index;

    PUSB_AUDIO_STREAMING_INTERFACE_DESCRIPTOR StreamingInterfaceDescriptor;
    PUSB_AUDIO_CONTROL_INPUT_TERMINAL_DESCRIPTOR TerminalDescriptor = NULL;

    /* search for terminal descriptor of that irp sink / irp source */
    TerminalDescriptor = UsbAudioGetStreamingTerminalDescriptorByIndex(DeviceExtension->ConfigurationDescriptor, Pin->Id);
    ASSERT(TerminalDescriptor != NULL);

    /* grab interface descriptor */
    InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, ConfigurationDescriptor, -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    if (!InterfaceDescriptor)
    {
        /* no such interface */
        return STATUS_INVALID_PARAMETER;
    }

    Found = FALSE;
    Index = 0;

    /* selects the interface which has an audio streaming interface descriptor associated to the input / output terminal at the given format index */
    while (InterfaceDescriptor != NULL)
    {
        if (InterfaceDescriptor->bInterfaceSubClass == 0x02 /* AUDIO_STREAMING */ && InterfaceDescriptor->bNumEndpoints > 0) 
        {
            StreamingInterfaceDescriptor = (PUSB_AUDIO_STREAMING_INTERFACE_DESCRIPTOR)USBD_ParseDescriptors(ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength, InterfaceDescriptor, USB_AUDIO_CONTROL_TERMINAL_DESCRIPTOR_TYPE);
            if (StreamingInterfaceDescriptor != NULL)
            {
                ASSERT(StreamingInterfaceDescriptor->bDescriptorSubtype == 0x01);
                ASSERT(StreamingInterfaceDescriptor->wFormatTag == WAVE_FORMAT_PCM);
                if (StreamingInterfaceDescriptor->bTerminalLink == TerminalDescriptor->bTerminalID)
                {
                    if (FormatDescriptorIndex == Index)
                    {
                        Found = TRUE;
                        break;
                    }
                    Index++;
                }
            }
        }
        InterfaceDescriptor = USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor, (PVOID)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength), -1, -1, USB_DEVICE_CLASS_AUDIO, -1, -1);
    }

    if (!Found)
    {
        /* no such interface */
        DPRINT1("No Interface found\n");
        return STATUS_INVALID_PARAMETER;
    }

    Urb = AllocFunction(GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints));
    if (!Urb)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

     /* now prepare interface urb */
     UsbBuildSelectInterfaceRequest(Urb, GET_SELECT_INTERFACE_REQUEST_SIZE(InterfaceDescriptor->bNumEndpoints), DeviceExtension->ConfigurationHandle, InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting);

     /* now select the interface */
     Status = SubmitUrbSync(DeviceExtension->LowerDevice, Urb);

     DPRINT1("USBAudioSelectAudioStreamingInterface Status %x UrbStatus %x InterfaceNumber %x AlternateSetting %x\n", Status, Urb->UrbSelectInterface.Hdr.Status, InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting);

     /* did it succeeed */
     if (NT_SUCCESS(Status))
     {
         /* free old interface info */
         if (DeviceExtension->InterfaceInfo)
         {
             /* free old info */
             FreeFunction(DeviceExtension->InterfaceInfo);
         }

         /* alloc interface info */
         DeviceExtension->InterfaceInfo = AllocFunction(Urb->UrbSelectInterface.Interface.Length);
         if (DeviceExtension->InterfaceInfo == NULL)
         {
             /* no memory */
             FreeFunction(Urb);
             return STATUS_INSUFFICIENT_RESOURCES;
         }

         /* copy interface info */
         RtlCopyMemory(DeviceExtension->InterfaceInfo, &Urb->UrbSelectInterface.Interface, Urb->UrbSelectInterface.Interface.Length);
         PinContext->InterfaceDescriptor = InterfaceDescriptor;
     }

     /* free urb */
     FreeFunction(Urb);
     return Status;
}

VOID
NTAPI
CaptureGateOnWorkItem(
    _In_ PVOID Context)
{
    PKSPIN Pin;
    PPIN_CONTEXT PinContext;
    PKSGATE Gate;
    ULONG Count;

    /* get pin */
    Pin = Context;

    /* get pin context */
    PinContext = Pin->Context;

    do
    {
        /* acquire processing mutex */
        KsPinAcquireProcessingMutex(Pin);

        /* get pin control gate */
        Gate = KsPinGetAndGate(Pin);

        /* turn input on */
        KsGateTurnInputOn(Gate);

        /* schedule processing */
        KsPinAttemptProcessing(Pin, TRUE);

        /* release processing mutex */
        KsPinReleaseProcessingMutex(Pin);

        /* decrement worker count */
        Count = KsDecrementCountedWorker(PinContext->CaptureWorker);
    } while (Count);
}

NTSTATUS
RenderInitializeUrbAndIrp(
    IN PKSPIN Pin,
    IN PPIN_CONTEXT PinContext,
    IN OUT PIRP Irp,
    IN PVOID TransferBuffer,
    IN ULONG TransferBufferSize,
    IN ULONG PacketSize)
{
    ULONG Index, PacketCount;
    PURB Urb;
    PIO_STACK_LOCATION IoStack;

    /* initialize irp */
    IoInitializeIrp(Irp, IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize), PinContext->DeviceExtension->LowerDevice->StackSize);

    /* set irp members */
    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    Irp->Flags = 0;
    Irp->UserBuffer = NULL;

    /* init stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->DeviceObject = PinContext->DeviceExtension->LowerDevice;
    IoStack->Parameters.Others.Argument2 = NULL;
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    /* set completion routine */
    IoSetCompletionRoutine(Irp, UsbAudioRenderComplete, Pin, TRUE, TRUE, TRUE);

    /* calculate packet count */
    PacketCount = TransferBufferSize / PacketSize;
    ASSERT(TransferBufferSize % PacketSize == 0);

    /* lets allocate urb */
    Urb = (PURB)AllocFunction(GET_ISO_URB_SIZE(PacketCount));
    if (!Urb)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init urb */
    Urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    Urb->UrbIsochronousTransfer.Hdr.Length = GET_ISO_URB_SIZE(PacketCount);
    Urb->UrbIsochronousTransfer.PipeHandle = PinContext->DeviceExtension->InterfaceInfo->Pipes[0].PipeHandle;
    Urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_OUT | USBD_START_ISO_TRANSFER_ASAP;
    Urb->UrbIsochronousTransfer.TransferBufferLength = TransferBufferSize;
    Urb->UrbIsochronousTransfer.TransferBuffer = TransferBuffer;
    Urb->UrbIsochronousTransfer.NumberOfPackets = PacketCount;
    Urb->UrbIsochronousTransfer.StartFrame = 0;

    for (Index = 0; Index < PacketCount; Index++)
    {
        Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset = Index * PacketSize;
    }

    /* store urb */
    IoStack->Parameters.Others.Argument1 = Urb;
    Irp->Tail.Overlay.DriverContext[0] = Urb;


    /* done */
    return STATUS_SUCCESS;
}

VOID
CaptureInitializeUrbAndIrp(
    IN PKSPIN Pin,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    PURB Urb;
    PUCHAR TransferBuffer;
    ULONG Index;
    PPIN_CONTEXT PinContext;

    /* get pin context */
    PinContext = Pin->Context;

    /* backup urb and transferbuffer */
    Urb = Irp->Tail.Overlay.DriverContext[0];
    TransferBuffer = Urb->UrbIsochronousTransfer.TransferBuffer;

    /* initialize irp */
    IoInitializeIrp(Irp, IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize), PinContext->DeviceExtension->LowerDevice->StackSize);

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;
    Irp->Flags = 0;
    Irp->UserBuffer = NULL;
    Irp->Tail.Overlay.DriverContext[0] = Urb;
    Irp->Tail.Overlay.DriverContext[1] = NULL;

    /* init stack location */
    IoStack = IoGetNextIrpStackLocation(Irp);
    IoStack->DeviceObject = PinContext->DeviceExtension->LowerDevice;
    IoStack->Parameters.Others.Argument1 = Urb;
    IoStack->Parameters.Others.Argument2 = NULL;
    IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
    IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

    IoSetCompletionRoutine(Irp, UsbAudioCaptureComplete, Pin, TRUE, TRUE, TRUE);

    RtlZeroMemory(Urb, GET_ISO_URB_SIZE(PACKET_COUNT));

    /* init urb */
    Urb->UrbIsochronousTransfer.Hdr.Function = URB_FUNCTION_ISOCH_TRANSFER;
    Urb->UrbIsochronousTransfer.Hdr.Length = GET_ISO_URB_SIZE(10);
    Urb->UrbIsochronousTransfer.PipeHandle = PinContext->DeviceExtension->InterfaceInfo->Pipes[0].PipeHandle;
    Urb->UrbIsochronousTransfer.TransferFlags = USBD_TRANSFER_DIRECTION_IN | USBD_START_ISO_TRANSFER_ASAP;
    Urb->UrbIsochronousTransfer.TransferBufferLength = PinContext->DeviceExtension->InterfaceInfo->Pipes[0].MaximumPacketSize * 10;
    Urb->UrbIsochronousTransfer.TransferBuffer = TransferBuffer;
    Urb->UrbIsochronousTransfer.NumberOfPackets = PACKET_COUNT;
    Urb->UrbIsochronousTransfer.StartFrame = 0;

    for (Index = 0; Index < PACKET_COUNT; Index++)
    {
        Urb->UrbIsochronousTransfer.IsoPacket[Index].Offset = Index * PinContext->DeviceExtension->InterfaceInfo->Pipes[0].MaximumPacketSize;
    }
}


VOID
NTAPI
CaptureAvoidPipeStarvationWorker(
    _In_ PVOID Context)
{
    PKSPIN Pin;
    PPIN_CONTEXT PinContext;
    KIRQL OldLevel;
    PLIST_ENTRY CurEntry;
    PIRP Irp;

    /* get pin */
    Pin = Context;

    /* get pin context */
    PinContext = Pin->Context;

    /* acquire spin lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    if (!IsListEmpty(&PinContext->IrpListHead))
    {
        /* sanity check */
        ASSERT(!IsListEmpty(&PinContext->IrpListHead));

        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->IrpListHead);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* reinitialize irp and urb */
        CaptureInitializeUrbAndIrp(Pin, Irp);

        KsDecrementCountedWorker(PinContext->StarvationWorker);

        /* call driver */
        IoCallDriver(PinContext->DeviceExtension->LowerDevice, Irp);
    }
    else
    {
        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        KsDecrementCountedWorker(PinContext->StarvationWorker);
    }
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
    PKSALLOCATOR_FRAMING_EX Framing;
    PKSGATE Gate;


    /* set sample rate */
    Status = UsbAudioSetFormat(Pin);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* get pin context */
    PinContext = Pin->Context;

    /* lets get maximum packet size */
    MaximumPacketSize = GetMaxPacketSizeForInterface(PinContext->DeviceExtension->ConfigurationDescriptor, PinContext->InterfaceDescriptor, Pin->DataFlow);

    /* initialize work item for capture worker */
    ExInitializeWorkItem(&PinContext->CaptureWorkItem, CaptureGateOnWorkItem, (PVOID)Pin);

    /* register worker */
    Status = KsRegisterCountedWorker(CriticalWorkQueue, &PinContext->CaptureWorkItem, &PinContext->CaptureWorker);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        return Status;
    }

    /* initialize work item */
    ExInitializeWorkItem(&PinContext->StarvationWorkItem, CaptureAvoidPipeStarvationWorker, (PVOID)Pin);

    /* register worker */
    Status = KsRegisterCountedWorker(CriticalWorkQueue, &PinContext->StarvationWorkItem, &PinContext->StarvationWorker);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        KsUnregisterWorker(PinContext->CaptureWorker);
    }

    /* lets edit framing struct */
    Framing = (PKSALLOCATOR_FRAMING_EX)Pin->Descriptor->AllocatorFraming;
    Framing->FramingItem[0].PhysicalRange.MinFrameSize =
        Framing->FramingItem[0].PhysicalRange.MaxFrameSize =
        Framing->FramingItem[0].FramingRange.Range.MinFrameSize =
        Framing->FramingItem[0].FramingRange.Range.MaxFrameSize =
    MaximumPacketSize;

    /* calculate buffer size 8 irps * 10 iso packets * max packet size */
    BufferSize = 8 * PACKET_COUNT * MaximumPacketSize;

    /* allocate pin capture buffer */
    PinContext->BufferSize = BufferSize;
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
        Irp = AllocFunction(IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize));
        if (!Irp)
        {
            /* no memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* initialize irp */
        IoInitializeIrp(Irp, IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize), PinContext->DeviceExtension->LowerDevice->StackSize);
        
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Information = 0;
        Irp->Flags = 0;
        Irp->UserBuffer = NULL;
        
        IoStack = IoGetNextIrpStackLocation(Irp);
        IoStack->DeviceObject = PinContext->DeviceExtension->LowerDevice;
        IoStack->Parameters.Others.Argument2 = NULL;
        IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

        IoSetCompletionRoutine(Irp, UsbAudioCaptureComplete, Pin, TRUE, TRUE, TRUE);

        /* insert into irp list */
        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* add to object bag*/
        KsAddItemToObjectBag(Pin->Bag, Irp, ExFreePool);

        /* FIXME select correct pipe handle */
        Status = UsbAudioAllocCaptureUrbIso(PinContext->DeviceExtension->InterfaceInfo->Pipes[0].PipeHandle, 
                                            MaximumPacketSize,
                                            &PinContext->Buffer[MaximumPacketSize * PACKET_COUNT * Index],
                                            MaximumPacketSize * PACKET_COUNT,
                                            &Urb);

        DPRINT1("InitCapturePin Irp %p Urb %p\n", Irp, Urb);

        if (NT_SUCCESS(Status))
        {
            /* get next stack location */
            IoStack = IoGetNextIrpStackLocation(Irp);

            /* store urb */
            IoStack->Parameters.Others.Argument1 = Urb;
            Irp->Tail.Overlay.DriverContext[0] = Urb;
        }
        else
        {
            /* failed */
            return Status;
        }
    }

    /* get process control gate */
    Gate = KsPinGetAndGate(Pin);

    /* turn input off */
    KsGateTurnInputOff(Gate);

    return Status;
}

NTSTATUS
InitStreamPin(
    IN PKSPIN Pin)
{
    ULONG Index;
    PIRP Irp;
    PPIN_CONTEXT PinContext;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormatEx;
    PIO_STACK_LOCATION IoStack;

    DPRINT1("InitStreamPin\n");

    /* get pin context */
    PinContext = Pin->Context;

    /* allocate 1 sec buffer */
    WaveFormatEx = (PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat;
    PinContext->Buffer = AllocFunction(WaveFormatEx->WaveFormatEx.nAvgBytesPerSec);
    if (!PinContext->Buffer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init buffer size*/
    PinContext->BufferSize = WaveFormatEx->WaveFormatEx.nAvgBytesPerSec;
    PinContext->BufferOffset = 0;
    PinContext->BufferLength = 0;

    /* init irps */
    for (Index = 0; Index < 12; Index++)
    {
        /* allocate irp */
        Irp = AllocFunction(IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize));
        if (!Irp)
        {
            /* no memory */
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        DPRINT1("InitStreamPin Irp %p\n", Irp);

        /* initialize irp */
        IoInitializeIrp(Irp, IoSizeOfIrp(PinContext->DeviceExtension->LowerDevice->StackSize), PinContext->DeviceExtension->LowerDevice->StackSize);

        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Information = 0;
        Irp->Flags = 0;
        Irp->UserBuffer = NULL;

        IoStack = IoGetNextIrpStackLocation(Irp);
        IoStack->DeviceObject = PinContext->DeviceExtension->LowerDevice;
        IoStack->Parameters.Others.Argument2 = NULL;
        IoStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
        IoStack->Parameters.DeviceIoControl.IoControlCode = IOCTL_INTERNAL_USB_SUBMIT_URB;

        IoSetCompletionRoutine(Irp, UsbAudioRenderComplete, Pin, TRUE, TRUE, TRUE);

        /* insert into irp list */
        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* add to object bag*/
        KsAddItemToObjectBag(Pin->Bag, Irp, ExFreePool);
    }

    return STATUS_SUCCESS;
}

ULONG
GetDataRangeIndexForFormat(
    IN PKSDATARANGE ConnectionFormat,
    IN PKSDATARANGE * DataRanges,
    IN ULONG DataRangesCount)
{
    ULONG Index;
    PKSDATARANGE CurrentDataRange;
    PKSDATARANGE_AUDIO CurrentAudioDataRange;
    PKSDATAFORMAT_WAVEFORMATEX ConnectionDataFormat;

    if (ConnectionFormat->FormatSize != sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX))
    {
        /* unsupported connection format */
        DPRINT1("GetDataRangeIndexForFormat expected KSDATARANGE_AUDIO\n");
        return MAXULONG;
    }

    /* cast to right type */
    ConnectionDataFormat = (PKSDATAFORMAT_WAVEFORMATEX)ConnectionFormat;

    for (Index = 0; Index < DataRangesCount; Index++)
    {
         /* get current data range */
         CurrentDataRange = DataRanges[Index];

         /* compare guids */
         if (!IsEqualGUIDAligned(&CurrentDataRange->MajorFormat, &ConnectionFormat->MajorFormat) ||
             !IsEqualGUIDAligned(&CurrentDataRange->SubFormat, &ConnectionFormat->SubFormat) ||
             !IsEqualGUIDAligned(&CurrentDataRange->Specifier, &ConnectionFormat->Specifier))
         {
             /* no match */
             continue;
         }

         /* all pin data ranges are KSDATARANGE_AUDIO */
         CurrentAudioDataRange = (PKSDATARANGE_AUDIO)CurrentDataRange;

         /* check if number of channel match */
         if (CurrentAudioDataRange->MaximumChannels != ConnectionDataFormat->WaveFormatEx.nChannels)
         {
             /* number of channels mismatch */
             continue;
         }

         if (CurrentAudioDataRange->MinimumSampleFrequency > ConnectionDataFormat->WaveFormatEx.nSamplesPerSec)
         {
             /* channel frequency too low */
             continue;
         }

         if (CurrentAudioDataRange->MaximumSampleFrequency < ConnectionDataFormat->WaveFormatEx.nSamplesPerSec)
         {
             /* channel frequency too high */
             continue;
         }

         /* FIXME add checks for bitrate / sample size etc */
         return Index;
    }

    /* no datarange found */
    return MAXULONG;
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
    ULONG FormatIndex;

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
    InitializeListHead(&PinContext->DoneIrpListHead);
    KeInitializeSpinLock(&PinContext->IrpListLock);

    /* store pin context*/
    Pin->Context = PinContext;

    /* lets edit allocator framing struct */
    Status = _KsEdit(Pin->Bag, (PVOID*)&Pin->Descriptor, sizeof(KSPIN_DESCRIPTOR_EX), sizeof(KSPIN_DESCRIPTOR_EX), USBAUDIO_TAG);
    if (NT_SUCCESS(Status))
    {
        Status = _KsEdit(Pin->Bag, (PVOID*)&Pin->Descriptor->AllocatorFraming, sizeof(KSALLOCATOR_FRAMING_EX), sizeof(KSALLOCATOR_FRAMING_EX), USBAUDIO_TAG);
        ASSERT(Status == STATUS_SUCCESS);
    }

    /* choose correct dataformat */
    FormatIndex = GetDataRangeIndexForFormat(Pin->ConnectionFormat, Pin->Descriptor->PinDescriptor.DataRanges, Pin->Descriptor->PinDescriptor.DataRangesCount);
    if (FormatIndex == MAXULONG)
    {
        /* no format match */
        return STATUS_NO_MATCH;
    }

    /* select streaming interface */
    Status = USBAudioSelectAudioStreamingInterface(Pin, PinContext, PinContext->DeviceExtension, PinContext->DeviceExtension->ConfigurationDescriptor, FormatIndex);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        DPRINT1("USBAudioSelectAudioStreamingInterface failed with %x\n", Status);
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

    return Status;
}

NTSTATUS
NTAPI
USBAudioPinClose(
    _In_ PKSPIN Pin,
    _In_ PIRP Irp)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
UsbAudioRenderComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PKSPIN Pin;
    PPIN_CONTEXT PinContext;
    KIRQL OldLevel;
    PKSSTREAM_POINTER StreamPointerClone;
    NTSTATUS Status;
    PURB Urb;

    /* get pin context */
    Pin = Context;
    PinContext = Pin->Context;

    /* get status */
    Status = Irp->IoStatus.Status;

    /* get streampointer */
    StreamPointerClone = Irp->Tail.Overlay.DriverContext[1];

    /* get urb */
    Urb = Irp->Tail.Overlay.DriverContext[0];

    /* and free it */
    FreeFunction(Urb);

    /* acquire lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    /* insert entry into ready list */
    InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);

    /* release lock */
    KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

    if (!NT_SUCCESS(Status) && StreamPointerClone)
    {
        /* set status code because it failed */
        KsStreamPointerSetStatusCode(StreamPointerClone, STATUS_DEVICE_DATA_ERROR);
        DPRINT1("UsbAudioRenderComplete failed with %x\n", Status);
    }

    if (StreamPointerClone)
    {
        /* lets delete the stream pointer clone */
        KsStreamPointerDelete(StreamPointerClone);
    }

    /* done */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
UsbAudioCaptureComplete(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context)
{
    PKSPIN Pin;
    PPIN_CONTEXT PinContext;
    KIRQL OldLevel;
    PURB Urb;

    /* get pin context */
    Pin = Context;
    PinContext = Pin->Context;

    /* get urb */
    Urb = Irp->Tail.Overlay.DriverContext[0];

    /* acquire lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    if (!NT_SUCCESS(Urb->UrbIsochronousTransfer.Hdr.Status))
    {
        //DPRINT("UsbAudioCaptureComplete Irp %p Urb %p Status %x Packet Status %x\n", Irp, Urb, Urb->UrbIsochronousTransfer.Hdr.Status, Urb->UrbIsochronousTransfer.IsoPacket[0].Status);

        /* insert entry into ready list */
        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        KsIncrementCountedWorker(PinContext->StarvationWorker);
    }
    else
    {
        /* insert entry into done list */
        InsertTailList(&PinContext->DoneIrpListHead, &Irp->Tail.Overlay.ListEntry);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        KsIncrementCountedWorker(PinContext->CaptureWorker);
    }

    /* done */
    return STATUS_MORE_PROCESSING_REQUIRED;
}

PIRP
PinGetIrpFromReadyList(
    IN PKSPIN Pin)
{
    PPIN_CONTEXT PinContext;
    PLIST_ENTRY CurEntry;
    KIRQL OldLevel;
    PIRP Irp = NULL;

    /* get pin context */
    PinContext = Pin->Context;

    /* acquire spin lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    if (!IsListEmpty(&PinContext->IrpListHead))
    {
        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->IrpListHead);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);
    }

    /* release lock */
    KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

    return Irp;
}

NTSTATUS
PinRenderProcess(
    IN PKSPIN Pin)
{
    PKSSTREAM_POINTER LeadingStreamPointer;
    PKSSTREAM_POINTER CloneStreamPointer;
    NTSTATUS Status;
    PPIN_CONTEXT PinContext;
    ULONG PacketCount, TotalPacketSize, Offset;
    PKSDATAFORMAT_WAVEFORMATEX WaveFormatEx;
    PUCHAR TransferBuffer;
    PIRP Irp = NULL;

    //DPRINT1("PinRenderProcess\n");

    LeadingStreamPointer = KsPinGetLeadingEdgeStreamPointer(Pin, KSSTREAM_POINTER_STATE_LOCKED);
    if (LeadingStreamPointer == NULL)
    {
        return STATUS_SUCCESS;
    }

    if (NULL == LeadingStreamPointer->StreamHeader->Data)
    {
        Status = KsStreamPointerAdvance(LeadingStreamPointer);
        DPRINT1("Advancing Streampointer\n");
    }


    /* get pin context */
    PinContext = Pin->Context;

    /* get irp from ready list */
    Irp = PinGetIrpFromReadyList(Pin);

    if (!Irp)
    {
        /* no irps available */
        DPRINT1("No irps available");
        KsStreamPointerUnlock(LeadingStreamPointer, TRUE);
        return STATUS_SUCCESS;
    }

    /* clone stream pointer */
    Status = KsStreamPointerClone(LeadingStreamPointer, NULL, 0, &CloneStreamPointer);
    if (!NT_SUCCESS(Status))
    {
        /* failed */
        KsStreamPointerUnlock(LeadingStreamPointer, TRUE);
        DPRINT1("Leaking Irp %p\n", Irp);
        return STATUS_SUCCESS;
    }

    /* calculate packet count */
    /* FIXME support various sample rates */
    WaveFormatEx = (PKSDATAFORMAT_WAVEFORMATEX)Pin->ConnectionFormat;
    TotalPacketSize = WaveFormatEx->WaveFormatEx.nAvgBytesPerSec / 1000;

    /* init transfer buffer*/
    TransferBuffer = CloneStreamPointer->StreamHeader->Data;

    Offset = 0;

    /* are there bytes from previous request*/
    if (PinContext->BufferLength)
    {
        ASSERT(PinContext->BufferLength < TotalPacketSize);

        /* calculate offset*/
        Offset = TotalPacketSize - PinContext->BufferLength;

        if (PinContext->BufferOffset + TotalPacketSize >= PinContext->BufferSize)
        {
            RtlMoveMemory(PinContext->Buffer, &PinContext->Buffer[PinContext->BufferOffset - PinContext->BufferLength], PinContext->BufferLength);
            PinContext->BufferOffset = PinContext->BufferLength;
        }

        /* copy audio bytes */
        RtlCopyMemory(&PinContext->Buffer[PinContext->BufferOffset], TransferBuffer, Offset);

        /* init irp*/
        Status = RenderInitializeUrbAndIrp(Pin, PinContext, Irp, &PinContext->Buffer[PinContext->BufferOffset-PinContext->BufferLength], TotalPacketSize, TotalPacketSize);
        if (NT_SUCCESS(Status))
        {
            /* render audio bytes */
            Status = IoCallDriver(PinContext->LowerDevice, Irp);
        }
        else
        {
            ASSERT(FALSE);
        }

        PinContext->BufferLength = 0;
        PinContext->BufferOffset += Offset;

        /* get new irp from ready list */
        Irp = PinGetIrpFromReadyList(Pin);
        ASSERT(Irp);

    }

    /* FIXME correct MaximumPacketSize ? */
    PacketCount = (CloneStreamPointer->OffsetIn.Remaining - Offset) / TotalPacketSize;

    Status = RenderInitializeUrbAndIrp(Pin, PinContext, Irp, &TransferBuffer[Offset], PacketCount * TotalPacketSize, TotalPacketSize);
    if (NT_SUCCESS(Status))
    {
        /* store in irp context */
        Irp->Tail.Overlay.DriverContext[1] = CloneStreamPointer;

        if ((PacketCount * TotalPacketSize) + Offset < CloneStreamPointer->OffsetIn.Remaining)
        {
            /* calculate remaining buffer bytes */
            PinContext->BufferLength = CloneStreamPointer->OffsetIn.Remaining - ((PacketCount * TotalPacketSize) + Offset);

            /* check for overflow */
            if (PinContext->BufferOffset + TotalPacketSize >= PinContext->BufferSize)
            {
                /* reset buffer offset*/
                PinContext->BufferOffset = 0;
            }
            RtlCopyMemory(&PinContext->Buffer[PinContext->BufferOffset], &TransferBuffer[(PacketCount * TotalPacketSize) + Offset], PinContext->BufferLength);
            PinContext->BufferOffset += PinContext->BufferLength;
        }

        /* render audio bytes */
        Status = IoCallDriver(PinContext->LowerDevice, Irp);
    }


    /* unlock stream pointer and finish*/
    KsStreamPointerUnlock(LeadingStreamPointer, TRUE);
    return STATUS_PENDING;
}

NTSTATUS
PinCaptureProcess(
    IN PKSPIN Pin)
{
    PKSSTREAM_POINTER LeadingStreamPointer;
    KIRQL OldLevel;
    PPIN_CONTEXT PinContext;
    PLIST_ENTRY CurEntry;
    PIRP Irp;
    PURB Urb;
    PUCHAR TransferBuffer, OutBuffer;
    ULONG Offset, Length;
    NTSTATUS Status;
    PKSGATE Gate;

    //DPRINT1("PinCaptureProcess\n");
    LeadingStreamPointer = KsPinGetLeadingEdgeStreamPointer(Pin, KSSTREAM_POINTER_STATE_LOCKED);
    if (LeadingStreamPointer == NULL)
    {
        /* get process control gate */
        Gate = KsPinGetAndGate(Pin);

        /* shutdown processing */
        KsGateTurnInputOff(Gate);

        return STATUS_SUCCESS;
    }

    /* get pin context */
    PinContext = Pin->Context;

    /* acquire spin lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    while (!IsListEmpty(&PinContext->DoneIrpListHead))
    {
        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->DoneIrpListHead);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* get urb from irp */
        Urb = (PURB)Irp->Tail.Overlay.DriverContext[0];
        ASSERT(Urb);

        Offset = PtrToUlong(Irp->Tail.Overlay.DriverContext[1]);

        /* get transfer buffer */
        TransferBuffer = Urb->UrbIsochronousTransfer.TransferBuffer;

        /* get target buffer */
        OutBuffer = (PUCHAR)LeadingStreamPointer->StreamHeader->Data;

        /* calculate length */
        Length = min(LeadingStreamPointer->OffsetOut.Count - LeadingStreamPointer->StreamHeader->DataUsed, Urb->UrbIsochronousTransfer.TransferBufferLength - Offset);

        /* FIXME copy each packet extra */
        /* copy audio bytes */
        RtlCopyMemory((PUCHAR)&OutBuffer[LeadingStreamPointer->StreamHeader->DataUsed], &TransferBuffer[Offset], Length);

        //DPRINT1("Irp %p Urb %p OutBuffer %p TransferBuffer %p Offset %lu Remaining %lu TransferBufferLength %lu Length %lu\n", Irp, Urb, OutBuffer, TransferBuffer, Offset, LeadingStreamPointer->OffsetOut.Remaining, Urb->UrbIsochronousTransfer.TransferBufferLength, Length);

        /* adjust streampointer */
        LeadingStreamPointer->StreamHeader->DataUsed += Length;

        if (Length == LeadingStreamPointer->OffsetOut.Remaining)
        {
            KsStreamPointerAdvanceOffsetsAndUnlock(LeadingStreamPointer, 0, Length, TRUE);

            /* acquire spin lock */
            KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

            /* adjust offset */
            Irp->Tail.Overlay.DriverContext[1] = UlongToPtr(Length);

            /* reinsert into processed list */
            InsertHeadList(&PinContext->DoneIrpListHead, &Irp->Tail.Overlay.ListEntry);

            /* release lock */
            KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

            LeadingStreamPointer = KsPinGetLeadingEdgeStreamPointer(Pin, KSSTREAM_POINTER_STATE_LOCKED);
            if (LeadingStreamPointer == NULL)
            {
                /* no more work to be done*/
                return STATUS_PENDING;
            }
            else
            {
                /* resume work on this irp */
                continue;
            }
        }
        else
        {
            Status = KsStreamPointerAdvanceOffsets(LeadingStreamPointer, 0, Length, FALSE);
            NT_ASSERT(NT_SUCCESS(Status));
            ASSERT(Length == Urb->UrbIsochronousTransfer.TransferBufferLength - Offset);
        }


        /* acquire spin lock */
        KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

        InsertTailList(&PinContext->IrpListHead, &Irp->Tail.Overlay.ListEntry);
    }

    while (!IsListEmpty(&PinContext->IrpListHead))
    {
        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->IrpListHead);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* reinitialize irp and urb */
        CaptureInitializeUrbAndIrp(Pin, Irp);

        IoCallDriver(PinContext->DeviceExtension->LowerDevice, Irp);

        /* acquire spin lock */
        KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    }

    /* release lock */
    KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

    if (LeadingStreamPointer != NULL)
        KsStreamPointerUnlock(LeadingStreamPointer, FALSE);

    /* get process control gate */
    Gate = KsPinGetAndGate(Pin);

    /* shutdown processing */
    KsGateTurnInputOff(Gate);

    return STATUS_PENDING;
}


NTSTATUS
NTAPI
USBAudioPinProcess(
    _In_ PKSPIN Pin)
{
    NTSTATUS Status;

    if (Pin->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        Status = PinCaptureProcess(Pin);
    }
    else
    {
        Status = PinRenderProcess(Pin);
    }

    return Status;
}


VOID
NTAPI
USBAudioPinReset(
    _In_ PKSPIN Pin)
{
    UNIMPLEMENTED;
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
        UNIMPLEMENTED;
        return STATUS_SUCCESS;
    }

    return UsbAudioSetFormat(Pin);
}

NTSTATUS
StartCaptureIsocTransfer(
    IN PKSPIN Pin)
{
    PPIN_CONTEXT PinContext;
    PLIST_ENTRY CurEntry;
    PIRP Irp;
    KIRQL OldLevel;

    /* get pin context */
    PinContext = Pin->Context;

    /* acquire spin lock */
    KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    while(!IsListEmpty(&PinContext->IrpListHead))
    {
        /* remove entry from list */
        CurEntry = RemoveHeadList(&PinContext->IrpListHead);

        /* get irp offset */
        Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

        /* release lock */
        KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

        /* reinitialize irp and urb */
        CaptureInitializeUrbAndIrp(Pin, Irp);

        DPRINT("StartCaptureIsocTransfer Irp %p\n", Irp);
        IoCallDriver(PinContext->DeviceExtension->LowerDevice, Irp);

        /* acquire spin lock */
        KeAcquireSpinLock(&PinContext->IrpListLock, &OldLevel);

    }

    /* release lock */
    KeReleaseSpinLock(&PinContext->IrpListLock, OldLevel);

    return STATUS_SUCCESS;
}

NTSTATUS
CapturePinStateChange(
    _In_ PKSPIN Pin,
    _In_ KSSTATE ToState,
    _In_ KSSTATE FromState)
{
    NTSTATUS Status = STATUS_SUCCESS;

    if (FromState != ToState)
    {
        if (ToState)
        {
            if (ToState == KSSTATE_PAUSE)
            {
                if (FromState == KSSTATE_RUN)
                {
                    /* wait until pin processing is finished*/
                }
            }
            else
            {
                if (ToState == KSSTATE_RUN)
                {
                    Status = StartCaptureIsocTransfer(Pin);
                }
            }
        }
    }
    return Status;
}


NTSTATUS
NTAPI
USBAudioPinSetDeviceState(
    _In_ PKSPIN Pin,
    _In_ KSSTATE ToState,
    _In_ KSSTATE FromState)
{
    NTSTATUS Status;

    if (Pin->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        /* handle capture state changes */
        Status = CapturePinStateChange(Pin, ToState, FromState);
    }
    else
    {
        UNIMPLEMENTED;
        Status = STATUS_SUCCESS;
    }

    return Status;
}


NTSTATUS
NTAPI
UsbAudioPinDataIntersect(
    _In_  PVOID        Context,
    _In_  PIRP         Irp,
    _In_  PKSP_PIN     Pin,
    _In_  PKSDATARANGE DataRange,
    _In_  PKSDATARANGE MatchingDataRange,
    _In_  ULONG        DataBufferSize,
    _Out_ PVOID        Data,
    _Out_ PULONG       DataSize)
{
    PKSFILTER Filter;
    PKSPIN_DESCRIPTOR_EX PinDescriptor;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;
    PKSDATARANGE_AUDIO DataRangeAudio;

    /* get filter from irp*/
    Filter = KsGetFilterFromIrp(Irp);
    if (!Filter)
    {
        /* no match*/
        return STATUS_NO_MATCH;
    }

    /* get pin descriptor */
    PinDescriptor = (PKSPIN_DESCRIPTOR_EX)&Filter->Descriptor->PinDescriptors[Pin->PinId];

    *DataSize = sizeof(KSDATAFORMAT_WAVEFORMATEX);
    if (DataBufferSize == 0)
    {
        /* buffer too small */
        return STATUS_BUFFER_OVERFLOW;
    }

    /* sanity checks*/
    ASSERT(PinDescriptor->PinDescriptor.DataRangesCount >= 0);
    ASSERT(PinDescriptor->PinDescriptor.DataRanges[0]->FormatSize == sizeof(KSDATARANGE_AUDIO));

    DataRangeAudio = (PKSDATARANGE_AUDIO)PinDescriptor->PinDescriptor.DataRanges[0];

    DataFormat = Data;
    DataFormat->WaveFormatEx.wFormatTag = WAVE_FORMAT_PCM;
    DataFormat->WaveFormatEx.nChannels = DataRangeAudio->MaximumChannels;
    DataFormat->WaveFormatEx.nSamplesPerSec = DataRangeAudio->MaximumSampleFrequency;
    DataFormat->WaveFormatEx.nAvgBytesPerSec = DataRangeAudio->MaximumSampleFrequency * (DataRangeAudio->MaximumBitsPerSample / 8) * DataRangeAudio->MaximumChannels;
    DataFormat->WaveFormatEx.nBlockAlign = (DataRangeAudio->MaximumBitsPerSample / 8) * DataRangeAudio->MaximumChannels;
    DataFormat->WaveFormatEx.wBitsPerSample = DataRangeAudio->MaximumBitsPerSample;
    DataFormat->WaveFormatEx.cbSize = 0;

    DataFormat->DataFormat.FormatSize = sizeof(KSDATAFORMAT) + sizeof(WAVEFORMATEX);
    DataFormat->DataFormat.Flags = 0;
    DataFormat->DataFormat.Reserved = 0;
    DataFormat->DataFormat.MajorFormat = KSDATAFORMAT_TYPE_AUDIO;
    DataFormat->DataFormat.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    DataFormat->DataFormat.Specifier = KSDATAFORMAT_SPECIFIER_WAVEFORMATEX;
    DataFormat->DataFormat.SampleSize = (DataRangeAudio->MaximumBitsPerSample / 8) * DataRangeAudio->MaximumChannels;

    return STATUS_SUCCESS;
}
