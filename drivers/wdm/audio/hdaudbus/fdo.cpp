/*
* COPYRIGHT:       See COPYING in the top level directory
* PROJECT:         ReactOS Kernel Streaming
* FILE:            drivers/wdm/audio/hdaudbus/fdo.cpp
* PURPOSE:         HDA Driver Entry
* PROGRAMMER:      Johannes Anderwald
*/
#include "hdaudbus.h"

BOOLEAN
NTAPI
HDA_InterruptService(
    IN PKINTERRUPT  Interrupt,
    IN PVOID  ServiceContext)
{
    PDEVICE_OBJECT DeviceObject;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG InterruptStatus;
    UCHAR RirbStatus, CorbStatus;

    /* get device extension */
    DeviceObject = static_cast<PDEVICE_OBJECT>(ServiceContext);
    DeviceExtension = static_cast<PHDA_FDO_DEVICE_EXTENSION>(DeviceObject->DeviceExtension);
    ASSERT(DeviceExtension->IsFDO == TRUE);

    // Check if this interrupt is ours
    InterruptStatus = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_INTR_STATUS));

    DPRINT1("HDA_InterruptService %lx\n", InterruptStatus);
    if ((InterruptStatus & INTR_STATUS_GLOBAL) == 0)
        return FALSE;

    // Controller or stream related?
    if (InterruptStatus & INTR_STATUS_CONTROLLER) {
        RirbStatus = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_STATUS);
        CorbStatus = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_STATUS);

        // Check for incoming responses
        if (RirbStatus) {
            WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_STATUS, RirbStatus);

            if (DeviceExtension->RirbLength == 0)
            {
                /* HACK: spurious interrupt */
                return FALSE;
            }

            if ((RirbStatus & RIRB_STATUS_RESPONSE) != 0) {
                IoRequestDpc(DeviceObject, NULL, NULL);
            }

            if ((RirbStatus & RIRB_STATUS_OVERRUN) != 0)
                DPRINT1("hda: RIRB Overflow\n");
        }

        // Check for sending errors
        if (CorbStatus) {
            WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_STATUS, CorbStatus);

            if ((CorbStatus & CORB_STATUS_MEMORY_ERROR) != 0)
                DPRINT1("hda: CORB Memory Error!\n");
        }
    }
#if 0
    if ((intrStatus & INTR_STATUS_STREAM_MASK) != 0) {
        for (uint32 index = 0; index < HDA_MAX_STREAMS; index++) {
            if ((intrStatus & (1 << index)) != 0) {
                if (controller->streams[index]) {
                    if (stream_handle_interrupt(controller,
                        controller->streams[index], index)) {
                        handled = B_INVOKE_SCHEDULER;
                    }
                }
                else {
                    dprintf("hda: Stream interrupt for unconfigured stream "
                        "%ld!\n", index);
                }
            }
        }
    }
#endif
    return TRUE;
}

VOID
NTAPI
HDA_DpcForIsr(
    _In_ PKDPC Dpc,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp,
    _In_opt_ PVOID Context)
{
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG Response, ResponseFlags, Cad;
    USHORT WritePos;
    PHDA_CODEC_ENTRY Codec;

    /* get device extension */
    DeviceExtension = static_cast<PHDA_FDO_DEVICE_EXTENSION>(DeviceObject->DeviceExtension);
    ASSERT(DeviceExtension->IsFDO == TRUE);

    WritePos = (READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_WRITE_POS)) + 1) % DeviceExtension->RirbLength;

    for (; DeviceExtension->RirbReadPos != WritePos; DeviceExtension->RirbReadPos = (DeviceExtension->RirbReadPos + 1) % DeviceExtension->RirbLength)
    {
        Response = DeviceExtension->RirbBase[DeviceExtension->RirbReadPos].response;
        ResponseFlags = DeviceExtension->RirbBase[DeviceExtension->RirbReadPos].flags;
        Cad = ResponseFlags & RESPONSE_FLAGS_CODEC_MASK;
        DPRINT1("Response %lx ResponseFlags %lx Cad %lx\n", Response, ResponseFlags, Cad);

        /* get codec */
        Codec = DeviceExtension->Codecs[Cad];
        if (Codec == NULL)
        {
            DPRINT1("hda: response for unknown codec %x Response %x ResponseFlags %x\n", Cad, Response, ResponseFlags);
            continue;
        }

        /* check response count */
        if (Codec->ResponseCount >= MAX_CODEC_RESPONSES)
        {
            DPRINT1("too many responses for codec %x Response %x ResponseFlags %x\n", Cad, Response, ResponseFlags);
            continue;
        }

        // FIXME handle unsolicited responses
        ASSERT((ResponseFlags & RESPONSE_FLAGS_UNSOLICITED) == 0);

        /* store response */
        Codec->Responses[Codec->ResponseCount] = Response;
        Codec->ResponseCount++;
        KeReleaseSemaphore(&Codec->ResponseSemaphore, IO_NO_INCREMENT, 1, FALSE);
    }
}


VOID
HDA_SendVerbs(
    IN PDEVICE_OBJECT DeviceObject,
    IN PHDA_CODEC_ENTRY Codec,
    IN PULONG Verbs,
    OUT PULONG Responses,
    IN ULONG Count)
{
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG Sent = 0, ReadPosition, WritePosition, Queued;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO);

    /* reset response count */
    Codec->ResponseCount = 0;

    while (Sent < Count) {
        ReadPosition = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));

        Queued = 0;

        while (Sent < Count) {
            WritePosition = (DeviceExtension->CorbWritePos + 1) % DeviceExtension->CorbLength;

            if (WritePosition == ReadPosition) {
                // There is no space left in the ring buffer; execute the
                // queued commands and wait until
                break;
            }

            DeviceExtension->CorbBase[WritePosition] = Verbs[Sent++];
            DeviceExtension->CorbWritePos = WritePosition;
            Queued++;
        }

        WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_WRITE_POS), DeviceExtension->CorbWritePos);
    }

    while (Queued--)
    {
        LARGE_INTEGER Timeout;
        Timeout.QuadPart = -1000LL * 10000; // 1 sec

        NTSTATUS waitStatus = KeWaitForSingleObject(&Codec->ResponseSemaphore,
                                                    Executive,
                                                    KernelMode,
                                                    FALSE,
                                                    &Timeout);

        if (waitStatus == STATUS_TIMEOUT)
        {
            DPRINT1("HDA_SendVerbs: timeout! Queued: %u\n", Queued);
            break;
        }
    }

    if (Responses != NULL) {
        memcpy(Responses, Codec->Responses, Codec->ResponseCount * sizeof(ULONG));
    }
}

NTSTATUS
HDA_InitCodec(
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG codecAddress)
{
    PHDA_CODEC_ENTRY Entry;
    ULONG verbs[3];
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    CODEC_RESPONSE Response;
    ULONG NodeId, GroupType;
    NTSTATUS Status;
    PHDA_CODEC_AUDIO_GROUP AudioGroup;
    PHDA_PDO_DEVICE_EXTENSION ChildDeviceExtension;

    /* lets allocate the entry */
    Entry = (PHDA_CODEC_ENTRY)AllocateItem(NonPagedPool, sizeof(HDA_CODEC_ENTRY));
    if (!Entry)
    {
        DPRINT1("hda: failed to allocate memory");
        return STATUS_UNSUCCESSFUL;
    }

    /* init codec */
    Entry->Addr = codecAddress;
    KeInitializeSemaphore(&Entry->ResponseSemaphore, 0, MAX_CODEC_RESPONSES);

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* store codec */
    DeviceExtension->Codecs[codecAddress] = Entry;

    verbs[0] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_VENDOR_ID);
    verbs[1] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_REVISION_ID);
    verbs[2] = MAKE_VERB(codecAddress, 0, VID_GET_PARAMETER, PID_SUB_NODE_COUNT);

    /* get basic info */
    HDA_SendVerbs(DeviceObject, Entry, verbs, (PULONG)&Response, 3);

    /* store codec details */
    Entry->Major = Response.major;
    Entry->Minor = Response.minor;
    Entry->ProductId = Response.device;
    Entry->Revision = Response.revision;
    Entry->Stepping = Response.stepping;
    Entry->VendorId = Response.vendor;

    DPRINT1("hda Codec %ld Vendor: %04lx Product: %04lx, Revision: %lu.%lu.%lu.%lu NodeStart %u NodeCount %u \n", codecAddress, Response.vendor,
        Response.device, Response.major, Response.minor, Response.revision, Response.stepping, Response.start, Response.count);

    for (NodeId = Response.start; NodeId < Response.start + Response.count; NodeId++) {

        /* get function type */
        verbs[0] = MAKE_VERB(codecAddress, NodeId, VID_GET_PARAMETER, PID_FUNCTION_GROUP_TYPE);

        HDA_SendVerbs(DeviceObject, Entry, verbs, &GroupType, 1);
        DPRINT1("NodeId %u GroupType %x\n", NodeId, GroupType);

        if ((GroupType & FUNCTION_GROUP_NODETYPE_MASK) == FUNCTION_GROUP_NODETYPE_AUDIO) {
            if (Entry->AudioGroupCount >= HDA_MAX_AUDIO_GROUPS)
            {
                DPRINT1("Too many audio groups in node %u. Skipping.\n", NodeId);
                break;
            }

            AudioGroup = (PHDA_CODEC_AUDIO_GROUP)AllocateItem(NonPagedPool, sizeof(HDA_CODEC_AUDIO_GROUP));
            if (!AudioGroup)
            {
                DPRINT1("hda: insufficient memory\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* init audio group */
            AudioGroup->NodeId = NodeId;
            AudioGroup->FunctionGroup = FUNCTION_GROUP_NODETYPE_AUDIO;

            // Found an Audio Function Group!
            DPRINT1("NodeId %x found an audio function group!\n", NodeId);

            Status = IoCreateDevice(DeviceObject->DriverObject, sizeof(HDA_PDO_DEVICE_EXTENSION), NULL, FILE_DEVICE_SOUND, FILE_AUTOGENERATED_DEVICE_NAME, FALSE, &AudioGroup->ChildPDO);
            if (!NT_SUCCESS(Status))
            {
                FreeItem(AudioGroup);
                DPRINT1("hda failed to create device object %x\n", Status);
                return Status;
            }

            /* init child pdo*/
            ChildDeviceExtension = (PHDA_PDO_DEVICE_EXTENSION)AudioGroup->ChildPDO->DeviceExtension;
            ChildDeviceExtension->IsFDO = FALSE;
            ChildDeviceExtension->ReportedMissing = FALSE;
            ChildDeviceExtension->Codec = Entry;
            ChildDeviceExtension->AudioGroup = AudioGroup;
            ChildDeviceExtension->FDO = DeviceObject;

            /* setup flags */
            AudioGroup->ChildPDO->Flags |= DO_POWER_PAGABLE;
            AudioGroup->ChildPDO->Flags &= ~DO_DEVICE_INITIALIZING;

            /* add audio group*/
            Entry->AudioGroups[Entry->AudioGroupCount] = AudioGroup;
            Entry->AudioGroupCount++;
        }
    }
    return STATUS_SUCCESS;

}

NTSTATUS
NTAPI
HDA_InitCorbRirbPos(
    IN PDEVICE_OBJECT DeviceObject)
{
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    UCHAR corbSize, value, rirbSize;
    PHYSICAL_ADDRESS HighestPhysicalAddress, CorbPhysicalAddress;
    ULONG Index;
    USHORT corbReadPointer, rirbWritePointer, interruptValue, corbControl, rirbControl;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    // Determine and set size of CORB
    corbSize = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE);
    if ((corbSize & CORB_SIZE_CAP_256_ENTRIES) != 0) {
        DeviceExtension->CorbLength = 256;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE) & ~HDAC_CORB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE, value | CORB_SIZE_256_ENTRIES);
    }
    else if (corbSize & CORB_SIZE_CAP_16_ENTRIES) {
        DeviceExtension->CorbLength = 16;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE) & ~HDAC_CORB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE, value | CORB_SIZE_16_ENTRIES);
    }
    else if (corbSize & CORB_SIZE_CAP_2_ENTRIES) {
        DeviceExtension->CorbLength = 2;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE) & ~HDAC_CORB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_SIZE, value | CORB_SIZE_2_ENTRIES);
    }

    // Determine and set size of RIRB
    rirbSize = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE);
    if (rirbSize & RIRB_SIZE_CAP_256_ENTRIES) {
        DeviceExtension->RirbLength = 256;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE) & ~HDAC_RIRB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE, value | RIRB_SIZE_256_ENTRIES);
    }
    else if (rirbSize & RIRB_SIZE_CAP_16_ENTRIES) {
        DeviceExtension->RirbLength = 16;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE) & ~HDAC_RIRB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE, value | RIRB_SIZE_16_ENTRIES);
    }
    else if (rirbSize & RIRB_SIZE_CAP_2_ENTRIES) {
        DeviceExtension->RirbLength = 2;

        value = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE) & ~HDAC_RIRB_SIZE_MASK;
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_SIZE, value | RIRB_SIZE_2_ENTRIES);
    }

    /* init corb */
    HighestPhysicalAddress.QuadPart = 0x00000000FFFFFFFF;
    DeviceExtension->CorbBase = (PULONG)MmAllocateContiguousMemory(PAGE_SIZE * 3, HighestPhysicalAddress);
    ASSERT(DeviceExtension->CorbBase != NULL);

    // FIXME align rirb 128bytes
    ASSERT(DeviceExtension->CorbLength == 256);
    ASSERT(DeviceExtension->RirbLength == 256);

    CorbPhysicalAddress = MmGetPhysicalAddress(DeviceExtension->CorbBase);
    ASSERT(CorbPhysicalAddress.QuadPart != 0LL);

    // Program CORB/RIRB for these locations
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_CORB_BASE_LOWER), CorbPhysicalAddress.LowPart);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_CORB_BASE_UPPER), CorbPhysicalAddress.HighPart);

    DeviceExtension->RirbBase = (PRIRB_RESPONSE)((ULONG_PTR)DeviceExtension->CorbBase + PAGE_SIZE);
    CorbPhysicalAddress.QuadPart += PAGE_SIZE;
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_RIRB_BASE_LOWER), CorbPhysicalAddress.LowPart);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_RIRB_BASE_UPPER), CorbPhysicalAddress.HighPart);

    // Program DMA position update
    DeviceExtension->StreamPositions = (PVOID)((ULONG_PTR)DeviceExtension->RirbBase + PAGE_SIZE);
    CorbPhysicalAddress.QuadPart += PAGE_SIZE;
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_LOWER), CorbPhysicalAddress.LowPart);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_UPPER), CorbPhysicalAddress.HighPart);

    value = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_WRITE_POS)) & ~HDAC_CORB_WRITE_POS_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_WRITE_POS), value);

    // Reset CORB read pointer. Preserve bits marked as RsvdP.
    // After setting the reset bit, we must wait for the hardware
    // to acknowledge it, then manually unset it and wait for that
    // to be acknowledged as well.
    corbReadPointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));

    corbReadPointer |= CORB_READ_POS_RESET;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS), corbReadPointer);

    for (Index = 0; Index < 10; Index++) {
        KeStallExecutionProcessor(100);
        corbReadPointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));
        if ((corbReadPointer & CORB_READ_POS_RESET) != 0)
            break;
    }
    if ((corbReadPointer & CORB_READ_POS_RESET) == 0) {
        DPRINT1("hda: CORB read pointer reset not acknowledged\n");

        // According to HDA spec v1.0a ch3.3.21, software must read the
        // bit as 1 to verify that the reset completed. However, at least
        // some nVidia HDA controllers do not update the bit after reset.
        // Thus don't fail here on nVidia controllers.
        //if (controller->pci_info.vendor_id != PCI_VENDOR_NVIDIA)
        //	return B_BUSY;
    }

    corbReadPointer &= ~CORB_READ_POS_RESET;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS), corbReadPointer);
    for (Index = 0; Index < 10; Index++) {
        KeStallExecutionProcessor(100);
        corbReadPointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_READ_POS));
        if ((corbReadPointer & CORB_READ_POS_RESET) == 0)
            break;
    }
    if ((corbReadPointer & CORB_READ_POS_RESET) != 0) {
        DPRINT1("hda: CORB read pointer reset failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Reset RIRB write pointer
    rirbWritePointer = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_WRITE_POS)) & ~RIRB_WRITE_POS_RESET;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_WRITE_POS), rirbWritePointer | RIRB_WRITE_POS_RESET);

    // Generate interrupt for every response
    interruptValue = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RESPONSE_INTR_COUNT)) & ~HDAC_RESPONSE_INTR_COUNT_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RESPONSE_INTR_COUNT), interruptValue | 1);

    // Setup cached read/write indices
    DeviceExtension->RirbReadPos = 1;
    DeviceExtension->CorbWritePos = 0;

    // Gentlemen, start your engines...
    corbControl = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_CONTROL)) & ~HDAC_CORB_CONTROL_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_CORB_CONTROL), corbControl | CORB_CONTROL_RUN | CORB_CONTROL_MEMORY_ERROR_INTR);

    rirbControl = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_CONTROL)) & ~HDAC_RIRB_CONTROL_MASK;
    WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_RIRB_CONTROL), rirbControl | RIRB_CONTROL_DMA_ENABLE | RIRB_CONTROL_OVERRUN_INTR | RIRB_CONTROL_RESPONSE_INTR);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_ResetController(
    IN PDEVICE_OBJECT DeviceObject)
{
    USHORT ValCapabilities;
    ULONG Index;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG InputStreams, OutputStreams, BiDirStreams, Control;
    UCHAR corbControl, rirbControl;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* read caps */
    ValCapabilities = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_GLOBAL_CAP));

    InputStreams = GLOBAL_CAP_INPUT_STREAMS(ValCapabilities);
    OutputStreams = GLOBAL_CAP_OUTPUT_STREAMS(ValCapabilities);
    BiDirStreams = GLOBAL_CAP_BIDIR_STREAMS(ValCapabilities);

    DPRINT1("NumInputStreams %u\n", InputStreams);
    DPRINT1("NumOutputStreams %u\n", OutputStreams);
    DPRINT1("NumBiDirStreams %u\n", BiDirStreams);

    /* stop all streams */
    for (Index = 0; Index < InputStreams; Index++)
    {
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE + HDAC_INPUT_STREAM_OFFSET(Index), 0);
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_STATUS + HDAC_STREAM_BASE + HDAC_INPUT_STREAM_OFFSET(Index), 0);
    }

    for (Index = 0; Index < OutputStreams; Index++) {
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE + HDAC_OUTPUT_STREAM_OFFSET(InputStreams, Index), 0);
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_STATUS + HDAC_STREAM_BASE + HDAC_OUTPUT_STREAM_OFFSET(InputStreams, Index), 0);
    }

    for (Index = 0; Index < BiDirStreams; Index++) {
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_CONTROL0 + HDAC_STREAM_BASE + HDAC_BIDIR_STREAM_OFFSET(InputStreams, OutputStreams, Index), 0);
        WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_STREAM_STATUS + HDAC_STREAM_BASE + HDAC_BIDIR_STREAM_OFFSET(InputStreams, OutputStreams, Index), 0);
    }

    // stop DMA
    Control = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_CONTROL) & ~HDAC_CORB_CONTROL_MASK;
    WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_CONTROL, Control);

    Control = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_CONTROL) & ~HDAC_RIRB_CONTROL_MASK;
    WRITE_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_CONTROL, Control);

    for (int timeout = 0; timeout < 10; timeout++) {
        KeStallExecutionProcessor(100);

        corbControl = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_CORB_CONTROL);
        rirbControl = READ_REGISTER_UCHAR(DeviceExtension->RegBase + HDAC_RIRB_CONTROL);
        if (corbControl == 0 && rirbControl == 0)
            break;
    }
    if (corbControl != 0 || rirbControl != 0) {
        DPRINT1("hda: unable to stop dma\n");
        return STATUS_UNSUCCESSFUL;
    }

    // reset DMA position buffer
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_LOWER), 0);
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_DMA_POSITION_BASE_UPPER), 0);

    // Set reset bit - it must be asserted for at least 100us
    Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL), Control & ~GLOBAL_CONTROL_RESET);

    for (int timeout = 0; timeout < 10; timeout++) {
        KeStallExecutionProcessor(100);

        Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
        if ((Control & GLOBAL_CONTROL_RESET) == 0)
            break;
    }
    if ((Control & GLOBAL_CONTROL_RESET) != 0)
    {
        DPRINT1("hda: unable to reset controller\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Unset reset bit
    Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL), Control | GLOBAL_CONTROL_RESET);

    for (int timeout = 0; timeout < 10; timeout++) {
        KeStallExecutionProcessor(100);

        Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
        if ((Control & GLOBAL_CONTROL_RESET) != 0)
            break;
    }
    if ((Control & GLOBAL_CONTROL_RESET) == 0) {
        DPRINT1("hda: unable to exit reset\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Wait for codecs to finish their own reset (apparently needs more
    // time than documented in the specs)
    KeStallExecutionProcessor(1000);

    // Enable unsolicited responses
    Control = READ_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL));
    WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_GLOBAL_CONTROL), Control | GLOBAL_CONTROL_UNSOLICITED);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
HDA_FDOStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_SUCCESS;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    PCM_RESOURCE_LIST Resources;
    ULONG Index;
    USHORT Value;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == TRUE);

    /* forward irp to lower device */
    if (!IoForwardIrpSynchronously(DeviceExtension->LowerDevice, Irp))
    {
        ASSERT(FALSE);
        return STATUS_INVALID_DEVICE_REQUEST;
    }
    Status = Irp->IoStatus.Status;
    if (!NT_SUCCESS(Status))
    {
        // failed to start
        DPRINT1("HDA_StartDevice Lower device failed to start %x\n", Status);
        return Status;
    }

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    Resources = IoStack->Parameters.StartDevice.AllocatedResourcesTranslated;
    for (Index = 0; Index < Resources->List[0].PartialResourceList.Count; Index++)
    {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor = &Resources->List[0].PartialResourceList.PartialDescriptors[Index];

        if (Descriptor->Type == CmResourceTypeMemory)
        {
            DeviceExtension->RegLength = Descriptor->u.Memory.Length;
            DeviceExtension->RegBase = (PUCHAR)MmMapIoSpace(Descriptor->u.Memory.Start, Descriptor->u.Memory.Length, MmNonCached);
            if (DeviceExtension->RegBase == NULL)
            {
                DPRINT1("[HDAB] Failed to map registers\n");
                Status = STATUS_UNSUCCESSFUL;
                break;
            }
        }
        else if (Descriptor->Type == CmResourceTypeInterrupt)
        {
            Status = IoConnectInterrupt(&DeviceExtension->Interrupt,
                HDA_InterruptService,
                DeviceObject,
                NULL,
                Descriptor->u.Interrupt.Vector,
                Descriptor->u.Interrupt.Level,
                Descriptor->u.Interrupt.Level,
                (KINTERRUPT_MODE)(Descriptor->Flags & CM_RESOURCE_INTERRUPT_LATCHED),
                (Descriptor->ShareDisposition != CmResourceShareDeviceExclusive),
                Descriptor->u.Interrupt.Affinity,
                FALSE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("[HDAB] Failed to connect interrupt. Status=%lx\n", Status);
                break;
            }

        }
    }

    if (NT_SUCCESS(Status))
    {
        // Get controller into valid state
        Status = HDA_ResetController(DeviceObject);
        if (!NT_SUCCESS(Status)) return Status;

        // Setup CORB/RIRB/DMA POS
        Status = HDA_InitCorbRirbPos(DeviceObject);
        if (!NT_SUCCESS(Status)) return Status;


        // Don't enable codec state change interrupts. We don't handle
        // them, as we want to use the STATE_STATUS register to identify
        // available codecs. We'd have to clear that register in the interrupt
        // handler to 'ack' the codec change.
        Value = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_WAKE_ENABLE)) & ~HDAC_WAKE_ENABLE_MASK;
        WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_WAKE_ENABLE), Value);

        // Enable controller interrupts
        WRITE_REGISTER_ULONG((PULONG)(DeviceExtension->RegBase + HDAC_INTR_CONTROL), INTR_CONTROL_GLOBAL_ENABLE | INTR_CONTROL_CONTROLLER_ENABLE);

        KeStallExecutionProcessor(1000);

        Value = READ_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_STATE_STATUS));
        if (!Value) {
            DPRINT1("hda: bad codec status\n");
            return STATUS_UNSUCCESSFUL;
        }
        WRITE_REGISTER_USHORT((PUSHORT)(DeviceExtension->RegBase + HDAC_STATE_STATUS), Value);

        // Create codecs
        DPRINT1("Codecs %lx\n", Value);
        for (Index = 0; Index < HDA_MAX_CODECS; Index++) {
            if ((Value & (1 << Index)) != 0) {
                HDA_InitCodec(DeviceObject, Index);
            }
        }
    }

    return Status;
}

NTSTATUS
NTAPI
HDA_FDORemoveDevice(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp)
{
    NTSTATUS Status;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    ULONG CodecIndex, AFGIndex;
    PHDA_CODEC_ENTRY CodecEntry;
    PDEVICE_OBJECT ChildPDO;
    PHDA_PDO_DEVICE_EXTENSION ChildDeviceExtension;

    /* get device extension */
    DeviceExtension = static_cast<PHDA_FDO_DEVICE_EXTENSION>(DeviceObject->DeviceExtension);
    ASSERT(DeviceExtension->IsFDO == TRUE);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoSkipCurrentIrpStackLocation(Irp);
    Status = IoCallDriver(DeviceExtension->LowerDevice, Irp);

    IoDetachDevice(DeviceExtension->LowerDevice);

    if (DeviceExtension->RegBase != NULL)
    {
        MmUnmapIoSpace(DeviceExtension->RegBase,
                       DeviceExtension->RegLength);
    }
    if (DeviceExtension->Interrupt != NULL)
    {
        IoDisconnectInterrupt(DeviceExtension->Interrupt);
    }
    if (DeviceExtension->CorbBase != NULL)
    {
        MmFreeContiguousMemory(DeviceExtension->CorbBase);
    }

    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        CodecEntry = DeviceExtension->Codecs[CodecIndex];
        if (CodecEntry == NULL)
        {
            continue;
        }

        ASSERT(CodecEntry->AudioGroupCount <= HDA_MAX_AUDIO_GROUPS);
        for (AFGIndex = 0; AFGIndex < CodecEntry->AudioGroupCount; AFGIndex++)
        {
            ChildPDO = CodecEntry->AudioGroups[AFGIndex]->ChildPDO;
            if (ChildPDO != NULL)
            {
                ChildDeviceExtension = static_cast<PHDA_PDO_DEVICE_EXTENSION>(ChildPDO->DeviceExtension);
                ChildDeviceExtension->Codec = NULL;
                ChildDeviceExtension->AudioGroup = NULL;
                ChildDeviceExtension->FDO = NULL;
                ChildDeviceExtension->ReportedMissing = TRUE;
                HDA_PDORemoveDevice(ChildPDO);
            }
            FreeItem(CodecEntry->AudioGroups[AFGIndex]);
        }
        FreeItem(CodecEntry);
    }

    IoDeleteDevice(DeviceObject);

    return Status;
}

NTSTATUS
NTAPI
HDA_FDOQueryBusRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG DeviceCount, CodecIndex, AFGIndex;
    PHDA_FDO_DEVICE_EXTENSION DeviceExtension;
    PHDA_CODEC_ENTRY Codec;
    PDEVICE_RELATIONS DeviceRelations;

    /* get device extension */
    DeviceExtension = (PHDA_FDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    ASSERT(DeviceExtension->IsFDO == TRUE);

    DeviceCount = 0;
    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        if (DeviceExtension->Codecs[CodecIndex] == NULL)
            continue;

        Codec = DeviceExtension->Codecs[CodecIndex];
        DeviceCount += Codec->AudioGroupCount;
    }

    if (DeviceCount == 0)
        return STATUS_UNSUCCESSFUL;

    DeviceRelations = (PDEVICE_RELATIONS)AllocateItem(NonPagedPool, sizeof(DEVICE_RELATIONS) + (DeviceCount > 1 ? sizeof(PDEVICE_OBJECT) * (DeviceCount - 1) : 0));
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 0;
    for (CodecIndex = 0; CodecIndex < HDA_MAX_CODECS; CodecIndex++)
    {
        if (DeviceExtension->Codecs[CodecIndex] == NULL)
            continue;

        Codec = DeviceExtension->Codecs[CodecIndex];
        ASSERT(Codec->AudioGroupCount <= HDA_MAX_AUDIO_GROUPS);
        for (AFGIndex = 0; AFGIndex < Codec->AudioGroupCount; AFGIndex++)
        {
            DeviceRelations->Objects[DeviceRelations->Count] = Codec->AudioGroups[AFGIndex]->ChildPDO;
            ObReferenceObject(Codec->AudioGroups[AFGIndex]->ChildPDO);
            DeviceRelations->Count++;
        }
    }

    /* FIXME handle existing device relations */
    ASSERT(Irp->IoStatus.Information == 0);

    /* store device relations */
    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    /* done */
    return STATUS_SUCCESS;
}


