/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_wavecyclic.cpp
 * PURPOSE:         WaveCyclic IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CPortPinWaveCyclic : public CUnknownImpl<IPortPinWaveCyclic, IServiceSink>
{
public:
    inline
    PVOID
    operator new(
        size_t Size,
        POOL_TYPE PoolType,
        ULONG Tag)
    {
        return ExAllocatePoolZero(PoolType, Size, Tag);
    }

    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortPinWaveCyclic;
    IMP_IServiceSink;
    CPortPinWaveCyclic(IUnknown *OuterUnknown) :
        m_Port(nullptr),
        m_Filter(nullptr),
        m_KsPinDescriptor(nullptr),
        m_Miniport(nullptr),
        m_ServiceGroup(nullptr),
        m_DmaChannel(nullptr),
        m_Stream(nullptr),
        m_State(KSSTATE_STOP),
        m_Format(nullptr),
        m_ConnectDetails(nullptr),
        m_CommonBuffer(nullptr),
        m_CommonBufferSize(0),
        m_CommonBufferOffset(0),
        m_IrpQueue(nullptr),
        m_FrameSize(0),
        m_Capture(FALSE),
        m_TotalPackets(0),
        m_StopCount(0),
        m_Position({0}),
        m_AllocatorFraming({{0}}),
        m_Descriptor(nullptr),
        m_EventListLock(0),
        m_EventList({nullptr}),
        m_ResetState(KSRESET_END),
        m_Delay(0)
    {
    }
    virtual ~CPortPinWaveCyclic(){}

protected:

    VOID UpdateCommonBuffer(ULONG Position, ULONG MaxTransferCount);
    VOID UpdateCommonBufferOverlap(ULONG Position, ULONG MaxTransferCount);
    VOID GeneratePositionEvents(IN ULONGLONG OldOffset, IN ULONGLONG NewOffset);

    friend NTSTATUS NTAPI PinWaveCyclicState(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveCyclicDataFormat(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveCyclicAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveCyclicAllocatorFraming(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveCyclicAddEndOfStreamEvent(IN PIRP Irp, IN PKSEVENTDATA EventData, IN PKSEVENT_ENTRY EventEntry);
    friend NTSTATUS NTAPI PinWaveCyclicAddLoopedStreamEvent(IN PIRP Irp, IN PKSEVENTDATA  EventData, IN PKSEVENT_ENTRY EventEntry);
    friend VOID CALLBACK PinSetStateWorkerRoutine(IN PDEVICE_OBJECT  DeviceObject, IN PVOID  Context);

    IPortWaveCyclic * m_Port;
    IPortFilterWaveCyclic * m_Filter;
    KSPIN_DESCRIPTOR * m_KsPinDescriptor;
    PMINIPORTWAVECYCLIC m_Miniport;
    PSERVICEGROUP m_ServiceGroup;
    PDMACHANNEL m_DmaChannel;
    PMINIPORTWAVECYCLICSTREAM m_Stream;
    KSSTATE m_State;
    PKSDATAFORMAT m_Format;
    PKSPIN_CONNECT m_ConnectDetails;

    PVOID m_CommonBuffer;
    ULONG m_CommonBufferSize;
    ULONG m_CommonBufferOffset;

    IIrpQueue * m_IrpQueue;

    ULONG m_FrameSize;
    BOOL m_Capture;

    ULONG m_TotalPackets;
    ULONG m_StopCount;
    KSAUDIO_POSITION m_Position;
    KSALLOCATOR_FRAMING m_AllocatorFraming;
    PSUBDEVICE_DESCRIPTOR m_Descriptor;

    KSPIN_LOCK m_EventListLock;
    LIST_ENTRY m_EventList;

    KSRESET m_ResetState;

    ULONG m_Delay;
};

typedef struct
{
    ULONG bLoopedStreaming;
    ULONGLONG Position;
}LOOPEDSTREAMING_EVENT_CONTEXT, *PLOOPEDSTREAMING_EVENT_CONTEXT;

typedef struct
{
    ULONG bLoopedStreaming;
}ENDOFSTREAM_EVENT_CONTEXT, *PENDOFSTREAM_EVENT_CONTEXT;

NTSTATUS NTAPI PinWaveCyclicState(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveCyclicDataFormat(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveCyclicAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveCyclicAllocatorFraming(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveCyclicAddEndOfStreamEvent(IN PIRP Irp, IN PKSEVENTDATA  EventData, IN PKSEVENT_ENTRY  EventEntry);
NTSTATUS NTAPI PinWaveCyclicAddLoopedStreamEvent(IN PIRP Irp, IN PKSEVENTDATA  EventData, IN PKSEVENT_ENTRY EventEntry);
NTSTATUS NTAPI PinWaveCyclicDRMHandler(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);

DEFINE_KSPROPERTY_CONNECTIONSET(PinWaveCyclicConnectionSet, PinWaveCyclicState, PinWaveCyclicDataFormat, PinWaveCyclicAllocatorFraming);
DEFINE_KSPROPERTY_AUDIOSET(PinWaveCyclicAudioSet, PinWaveCyclicAudioPosition);
DEFINE_KSPROPERTY_DRMSET(PinWaveCyclicDRMSet, PinWaveCyclicDRMHandler);

KSEVENT_ITEM PinWaveCyclicConnectionEventSet =
{
    KSEVENT_CONNECTION_ENDOFSTREAM,
    sizeof(KSEVENTDATA),
    sizeof(ENDOFSTREAM_EVENT_CONTEXT),
    PinWaveCyclicAddEndOfStreamEvent,
    0,
    0
};

KSEVENT_ITEM PinWaveCyclicStreamingEventSet =
{
    KSEVENT_LOOPEDSTREAMING_POSITION,
    sizeof(LOOPEDSTREAMING_POSITION_EVENT_DATA),
    sizeof(LOOPEDSTREAMING_EVENT_CONTEXT),
    PinWaveCyclicAddLoopedStreamEvent,
    0,
    0
};

KSPROPERTY_SET PinWaveCyclicPropertySet[] =
{
    {
        &KSPROPSETID_Connection,
        sizeof(PinWaveCyclicConnectionSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PinWaveCyclicConnectionSet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Audio,
        sizeof(PinWaveCyclicAudioSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PinWaveCyclicAudioSet,
        0,
        NULL
    },
    {
        &KSPROPSETID_DrmAudioStream,
        sizeof(PinWaveCyclicDRMSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PinWaveCyclicDRMSet,
        0,
        NULL
    }
};

KSEVENT_SET PinWaveCyclicEventSet[] =
{
    {
        &KSEVENTSETID_LoopedStreaming,
        sizeof(PinWaveCyclicStreamingEventSet) / sizeof(KSEVENT_ITEM),
        (const KSEVENT_ITEM*)&PinWaveCyclicStreamingEventSet
    },
    {
        &KSEVENTSETID_Connection,
        sizeof(PinWaveCyclicConnectionEventSet) / sizeof(KSEVENT_ITEM),
        (const KSEVENT_ITEM*)&PinWaveCyclicConnectionEventSet
    }
};

//==================================================================================================================================

NTSTATUS
NTAPI
CPortPinWaveCyclic::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    DPRINT("IServiceSink_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, IID_IIrpTarget) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN((IIrpTarget*)this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (IsEqualGUIDAligned(refiid, IID_IServiceSink))
    {
        *Output = PVOID(PUNKNOWN(PSERVICESINK(this)));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
PinWaveCyclicDRMHandler(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    DPRINT1("PinWaveCyclicDRMHandler\n");
    ASSERT(0);
    return STATUS_INVALID_PARAMETER;
}

NTSTATUS
NTAPI
PinWaveCyclicAddEndOfStreamEvent(
    IN PIRP Irp,
    IN PKSEVENTDATA  EventData,
    IN PKSEVENT_ENTRY EventEntry)
{
    PENDOFSTREAM_EVENT_CONTEXT Entry;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinWaveCyclic *Pin;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveCyclic*)Descriptor->PortPin;

     // get extra size
    Entry = (PENDOFSTREAM_EVENT_CONTEXT)(EventEntry + 1);

    // not a looped event
    Entry->bLoopedStreaming = FALSE;

    // insert item
    (void)ExInterlockedInsertTailList(&Pin->m_EventList, &EventEntry->ListEntry, &Pin->m_EventListLock);

    // done
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PinWaveCyclicAddLoopedStreamEvent(
    IN PIRP Irp,
    IN PKSEVENTDATA  EventData,
    IN PKSEVENT_ENTRY EventEntry)
{
    PLOOPEDSTREAMING_POSITION_EVENT_DATA Data;
    PLOOPEDSTREAMING_EVENT_CONTEXT Entry;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinWaveCyclic *Pin;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSEVENT_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveCyclic*)Descriptor->PortPin;

    // cast to looped event
    Data = (PLOOPEDSTREAMING_POSITION_EVENT_DATA)EventData;

    // get extra size
    Entry = (PLOOPEDSTREAMING_EVENT_CONTEXT)(EventEntry + 1);

    Entry->bLoopedStreaming = TRUE;
    Entry->Position = Data->Position;

    DPRINT1("Added event\n");

    // insert item
    (void)ExInterlockedInsertTailList(&Pin->m_EventList, &EventEntry->ListEntry, &Pin->m_EventListLock);

    // done
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PinWaveCyclicAllocatorFraming(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    CPortPinWaveCyclic *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSEVENT_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveCyclic*)Descriptor->PortPin;

    if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        // copy pin framing
        RtlMoveMemory(Data, &Pin->m_AllocatorFraming, sizeof(KSALLOCATOR_FRAMING));

        Irp->IoStatus.Information = sizeof(KSALLOCATOR_FRAMING);
        return STATUS_SUCCESS;
    }

    // not supported
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PinWaveCyclicAudioPosition(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    CPortPinWaveCyclic *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    PKSAUDIO_POSITION Position;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveCyclic*)Descriptor->PortPin;

    //sanity check
    PC_ASSERT(Pin->m_Stream);

    if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        // FIXME non multithreading-safe
        // copy audio position

        Position = (PKSAUDIO_POSITION)Data;

        if (Pin->m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_STREAMING)
        {
            RtlMoveMemory(Data, &Pin->m_Position, sizeof(KSAUDIO_POSITION));
            DPRINT("Play %lu Record %lu\n", Pin->m_Position.PlayOffset, Pin->m_Position.WriteOffset);
        }
        else if (Pin->m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
        {
            Position->PlayOffset = Pin->m_Position.PlayOffset;
            Position->WriteOffset = (ULONGLONG)Pin->m_IrpQueue->GetCurrentIrpOffset();
            DPRINT("Play %lu Write %lu\n", Position->PlayOffset, Position->WriteOffset);
        }

        Irp->IoStatus.Information = sizeof(KSAUDIO_POSITION);
        return STATUS_SUCCESS;
    }

    // not supported
    return STATUS_NOT_SUPPORTED;
}

typedef struct
{
    CPortPinWaveCyclic *Pin;
    KSSTATE NewState;
    PIO_WORKITEM WorkItem;
    PIRP Irp;

}SETPIN_CONTEXT, *PSETPIN_CONTEXT;

VOID
CALLBACK
PinSetStateWorkerRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    PSETPIN_CONTEXT PinWorkContext = (PSETPIN_CONTEXT)Context;
    NTSTATUS Status;

    // try set stream
    Status = PinWorkContext->Pin->m_Stream->SetState(PinWorkContext->NewState);

    DPRINT1("Setting state %u %x\n", PinWorkContext->NewState, Status);
    if (NT_SUCCESS(Status))
    {
        // store new state
        PinWorkContext->Pin->m_State = PinWorkContext->NewState;

        if (PinWorkContext->Pin->m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING && PinWorkContext->Pin->m_State == KSSTATE_STOP)
        {
            /* FIXME complete pending irps with successful state */
            PinWorkContext->Pin->m_IrpQueue->CancelBuffers();
        }
        //HACK
        //PinWorkContext->Pin->m_IrpQueue->CancelBuffers();
    }

    // store result
    PinWorkContext->Irp->IoStatus.Information = sizeof(KSSTATE);
    PinWorkContext->Irp->IoStatus.Status = Status;

    // complete irp
    IoCompleteRequest(PinWorkContext->Irp, IO_NO_INCREMENT);

    // free work item
    IoFreeWorkItem(PinWorkContext->WorkItem);

    // free work context
    FreeItem(PinWorkContext, TAG_PORTCLASS);

}

NTSTATUS
NTAPI
PinWaveCyclicState(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    CPortPinWaveCyclic *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    PKSSTATE State = (PKSSTATE)Data;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWaveCyclic*)Descriptor->PortPin;

    //sanity check
    PC_ASSERT(Pin->m_Stream);

    if (Request->Flags & KSPROPERTY_TYPE_SET)
    {
        // try set stream
        Status = Pin->m_Stream->SetState(*State);

        DPRINT("Setting state %u %x\n", *State, Status);
        if (NT_SUCCESS(Status))
        {
            // store new state
            Pin->m_State = *State;

            if (Pin->m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING && Pin->m_State == KSSTATE_STOP)
            {
                // FIXME
                // complete with successful state
                Pin->m_Stream->Silence(Pin->m_CommonBuffer, Pin->m_CommonBufferSize);
                Pin->m_IrpQueue->CancelBuffers();
                Pin->m_Position.PlayOffset = 0;
                Pin->m_Position.WriteOffset = 0;
            }
            else if (Pin->m_State == KSSTATE_STOP)
            {
                Pin->m_Stream->Silence(Pin->m_CommonBuffer, Pin->m_CommonBufferSize);
                Pin->m_IrpQueue->CancelBuffers();
                Pin->m_Position.PlayOffset = 0;
                Pin->m_Position.WriteOffset = 0;
            }
            // store result
            Irp->IoStatus.Information = sizeof(KSSTATE);
        }
        return Status;
    }
    else if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        // get current stream state
        *State = Pin->m_State;
        // store result
        Irp->IoStatus.Information = sizeof(KSSTATE);

        return STATUS_SUCCESS;
    }

    // unsupported request
    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
PinWaveCyclicDataFormat(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    CPortPinWaveCyclic *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    PIO_STACK_LOCATION IoStack;

    // get current irp stack location
    IoStack = IoGetCurrentIrpStackLocation(Irp);

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);

    // cast to pin impl
    Pin = (CPortPinWaveCyclic*)Descriptor->PortPin;

    //sanity check
    PC_ASSERT(Pin->m_Stream);
    PC_ASSERT(Pin->m_Format);

    if (Request->Flags & KSPROPERTY_TYPE_SET)
    {
        // try to change data format
        PKSDATAFORMAT NewDataFormat, DataFormat = (PKSDATAFORMAT)Irp->UserBuffer;
        ULONG Size = min(Pin->m_Format->FormatSize, DataFormat->FormatSize);

        if (RtlCompareMemory(DataFormat, Pin->m_Format, Size) == Size)
        {
            // format is identical
            Irp->IoStatus.Information = DataFormat->FormatSize;
            return STATUS_SUCCESS;
        }

        // new change request
        PC_ASSERT(Pin->m_State != KSSTATE_RUN);
        // FIXME queue a work item when Irql != PASSIVE_LEVEL
        PC_ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

        // allocate new data format
        NewDataFormat = (PKSDATAFORMAT)AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
        if (!NewDataFormat)
        {
            // not enough memory
            return STATUS_NO_MEMORY;
        }

        // copy new data format
        RtlMoveMemory(NewDataFormat, DataFormat, DataFormat->FormatSize);

        // set new format
        Status = Pin->m_Stream->SetFormat(NewDataFormat);
        if (NT_SUCCESS(Status))
        {
            // free old format
            FreeItem(Pin->m_Format, TAG_PORTCLASS);

            // store new format
            Pin->m_Format = NewDataFormat;
            Irp->IoStatus.Information = NewDataFormat->FormatSize;

#if 0
            PC_ASSERT(NewDataFormat->FormatSize == sizeof(KSDATAFORMAT_WAVEFORMATEX));
            PC_ASSERT(IsEqualGUIDAligned(((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.MajorFormat, KSDATAFORMAT_TYPE_AUDIO));
            PC_ASSERT(IsEqualGUIDAligned(((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.SubFormat, KSDATAFORMAT_SUBTYPE_PCM));
            PC_ASSERT(IsEqualGUIDAligned(((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));

            DPRINT("NewDataFormat: Channels %u Bits %u Samples %u\n", ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nChannels,
                                                                       ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.wBitsPerSample,
                                                                       ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nSamplesPerSec);
#endif

        }
        else
        {
            // failed to set format
            FreeItem(NewDataFormat, TAG_PORTCLASS);
        }

        // done
        return Status;
    }
    else if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        // get current data format
        PC_ASSERT(Pin->m_Format);

        if (Pin->m_Format->FormatSize > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
        {
            // buffer too small
            Irp->IoStatus.Information = Pin->m_Format->FormatSize;
            return STATUS_MORE_ENTRIES;
        }
        // copy data format
        RtlMoveMemory(Data, Pin->m_Format, Pin->m_Format->FormatSize);
        // store result size
        Irp->IoStatus.Information = Pin->m_Format->FormatSize;

        // done
        return STATUS_SUCCESS;
    }

    // unsupported request
    return STATUS_NOT_SUPPORTED;
}

VOID
CPortPinWaveCyclic::GeneratePositionEvents(
    IN ULONGLONG OldOffset,
    IN ULONGLONG NewOffset)
{
    PLIST_ENTRY Entry;
    PKSEVENT_ENTRY EventEntry;
    PLOOPEDSTREAMING_EVENT_CONTEXT Context;

    // acquire event lock
    KeAcquireSpinLockAtDpcLevel(&m_EventListLock);

    // point to first entry
    Entry = m_EventList.Flink;

    while(Entry != &m_EventList)
    {
        // get event entry
        EventEntry = (PKSEVENT_ENTRY)CONTAINING_RECORD(Entry, KSEVENT_ENTRY, ListEntry);

        // get event entry context
        Context = (PLOOPEDSTREAMING_EVENT_CONTEXT)(EventEntry + 1);

        if (Context->bLoopedStreaming != FALSE)
        {
            if (NewOffset > OldOffset)
            {
                /* buffer progress no overlap */
                if (OldOffset < Context->Position && Context->Position <= NewOffset)
                {
                    /* when someone eventually fixes sprintf... */
                    DPRINT("Generating event at OldOffset %I64u\n", OldOffset);
                    DPRINT("Context->Position %I64u\n", Context->Position);
                    DPRINT("NewOffset %I64u\n", NewOffset);
                    /* generate event */
                    KsGenerateEvent(EventEntry);
                }
            }
            else
            {
                /* buffer wrap-arround */
                if (OldOffset < Context->Position || NewOffset > Context->Position)
                {
                    /* when someone eventually fixes sprintf... */
                    DPRINT("Generating event at OldOffset %I64u\n", OldOffset);
                    DPRINT("Context->Position %I64u\n", Context->Position);
                    DPRINT("NewOffset %I64u\n", NewOffset);
                    /* generate event */
                    KsGenerateEvent(EventEntry);
                }
            }
        }

        // move to next entry
        Entry = Entry->Flink;
    }

    // release lock
    KeReleaseSpinLockFromDpcLevel(&m_EventListLock);
}

VOID
CPortPinWaveCyclic::UpdateCommonBuffer(
    ULONG Position,
    ULONG MaxTransferCount)
{
    ULONG BufferLength;
    ULONG BytesToCopy;
    ULONG BufferSize;
    ULONG Gap;
    PUCHAR Buffer;
    NTSTATUS Status;

    BufferLength = Position - m_CommonBufferOffset;
    BufferLength = min(BufferLength, MaxTransferCount);

    while(BufferLength)
    {
        Status = m_IrpQueue->GetMapping(&Buffer, &BufferSize);
        if (!NT_SUCCESS(Status))
        {
            Gap = Position - m_CommonBufferOffset;
            if (Gap > BufferLength)
            {
                // insert silence samples
                DPRINT("Inserting Silence Buffer Offset %lu GapLength %lu\n", m_CommonBufferOffset, BufferLength);
                m_Stream->Silence((PUCHAR)m_CommonBuffer + m_CommonBufferOffset, BufferLength);

                m_CommonBufferOffset += BufferLength;
            }
            break;
        }

        BytesToCopy = min(BufferLength, BufferSize);

        if (m_Capture)
        {
            m_DmaChannel->CopyFrom(Buffer, (PUCHAR)m_CommonBuffer + m_CommonBufferOffset, BytesToCopy);
        }
        else
        {
            m_DmaChannel->CopyTo((PUCHAR)m_CommonBuffer + m_CommonBufferOffset, Buffer, BytesToCopy);
        }

        m_IrpQueue->UpdateMapping(BytesToCopy);
        m_CommonBufferOffset += BytesToCopy;

        BufferLength -= BytesToCopy;
        m_Position.PlayOffset += BytesToCopy;

        if (m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
        {
            if (m_Position.WriteOffset)
            {
                // normalize position
                m_Position.PlayOffset = m_Position.PlayOffset % m_Position.WriteOffset;
            }
        }
    }
}

VOID
CPortPinWaveCyclic::UpdateCommonBufferOverlap(
    ULONG Position,
    ULONG MaxTransferCount)
{
    ULONG BufferLength, Length, Gap;
    ULONG BytesToCopy;
    ULONG BufferSize;
    PUCHAR Buffer;
    NTSTATUS Status;

    BufferLength = Gap = m_CommonBufferSize - m_CommonBufferOffset;
    BufferLength = Length = min(BufferLength, MaxTransferCount);
    while(BufferLength)
    {
        Status = m_IrpQueue->GetMapping(&Buffer, &BufferSize);
        if (!NT_SUCCESS(Status))
        {
            Gap = m_CommonBufferSize - m_CommonBufferOffset + Position;
            if (Gap > BufferLength)
            {
                // insert silence samples
                //DPRINT("Overlap Inserting Silence Buffer Size %lu Offset %lu Gap %lu Position %lu\n", m_CommonBufferSize, m_CommonBufferOffset, Gap, Position);
                m_Stream->Silence((PUCHAR)m_CommonBuffer + m_CommonBufferOffset, BufferLength);

                m_CommonBufferOffset += BufferLength;
            }
            break;
        }

        BytesToCopy = min(BufferLength, BufferSize);

        if (m_Capture)
        {
            m_DmaChannel->CopyFrom(Buffer,
                                             (PUCHAR)m_CommonBuffer + m_CommonBufferOffset,
                                             BytesToCopy);
        }
        else
        {
            m_DmaChannel->CopyTo((PUCHAR)m_CommonBuffer + m_CommonBufferOffset,
                                             Buffer,
                                             BytesToCopy);
        }

        m_IrpQueue->UpdateMapping(BytesToCopy);
        m_CommonBufferOffset += BytesToCopy;
        m_Position.PlayOffset += BytesToCopy;

        BufferLength -=BytesToCopy;

        if (m_ConnectDetails->Interface.Id == KSINTERFACE_STANDARD_LOOPED_STREAMING)
        {
            if (m_Position.WriteOffset)
            {
                // normalize position
                m_Position.PlayOffset = m_Position.PlayOffset % m_Position.WriteOffset;
            }
        }
    }

    if (Gap == Length)
    {
        m_CommonBufferOffset = 0;

        MaxTransferCount -= Length;

        if (MaxTransferCount)
        {
            UpdateCommonBuffer(Position, MaxTransferCount);
        }
    }
}

VOID
NTAPI
CPortPinWaveCyclic::RequestService()
{
    ULONG Position;
    ULONGLONG OldOffset, NewOffset;

    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    if (m_State == KSSTATE_RUN && m_ResetState == KSRESET_END)
    {
        m_Stream->GetPosition(&Position);

        OldOffset = m_Position.PlayOffset;

        if (Position < m_CommonBufferOffset)
        {
            UpdateCommonBufferOverlap(Position, m_FrameSize);
        }
        else if (Position >= m_CommonBufferOffset)
        {
            UpdateCommonBuffer(Position, m_FrameSize);
        }

        NewOffset = m_Position.PlayOffset;

        GeneratePositionEvents(OldOffset, NewOffset);
    }
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    UNIMPLEMENTED;
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status = STATUS_NOT_SUPPORTED;
    ULONG Data = 0;
    KSRESET ResetValue;

    /* get current irp stack location */
    IoStack = IoGetCurrentIrpStackLocation(Irp);
    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
        /* handle property with subdevice descriptor */
        Status = PcHandlePropertyWithTable(Irp,  m_Descriptor->FilterPropertySetCount, m_Descriptor->FilterPropertySet, m_Descriptor);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_ENABLE_EVENT)
    {
        Status = PcHandleEnableEventWithTable(Irp, m_Descriptor);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_DISABLE_EVENT)
    {
        Status = PcHandleDisableEventWithTable(Irp, m_Descriptor);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_RESET_STATE)
    {
        Status = KsAcquireResetValue(Irp, &ResetValue);
        DPRINT("Status %x Value %u\n", Status, ResetValue);
        /* check for success */
        if (NT_SUCCESS(Status))
        {
            //determine state of reset request
            if (ResetValue == KSRESET_BEGIN)
            {
                // start reset process
                // incoming read/write requests will be rejected
                m_ResetState = KSRESET_BEGIN;

                // cancel existing buffers
                m_IrpQueue->CancelBuffers();
            }
            else if (ResetValue == KSRESET_END)
            {
                // end of reset process
                m_ResetState = KSRESET_END;
            }
        }
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_WRITE_STREAM || IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_READ_STREAM)
    {
        /* increment total number of packets */
        InterlockedIncrement((PLONG)&m_TotalPackets);

         DPRINT("New Packet Total %u State %x MinData %u\n", m_TotalPackets, m_State, m_IrpQueue->NumData());

         /* is the device not currently reset */
         if (m_ResetState == KSRESET_END)
         {
             /* add the mapping */
             Status = m_IrpQueue->AddMapping(Irp, &Data);

             /* check for success */
             if (NT_SUCCESS(Status))
             {
                m_Position.WriteOffset += Data;
                Status = STATUS_PENDING;
             }
         }
         else
         {
             /* reset request is currently in progress */
             Status = STATUS_DEVICE_NOT_READY;
             DPRINT1("NotReady\n");
         }
    }
    else
    {
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    return Status;
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT("CPortPinWaveCyclic::Close entered\n");

    PC_ASSERT_IRQL(PASSIVE_LEVEL);

    if (m_Format)
    {
        // free format
        FreeItem(m_Format, TAG_PORTCLASS);

        // format is freed
        m_Format = NULL;
    }

    if (m_IrpQueue)
    {
        // cancel remaining irps
        m_IrpQueue->CancelBuffers();

        // release irp queue
        m_IrpQueue->Release();

        // queue is freed
        m_IrpQueue = NULL;
    }

    if (m_ServiceGroup)
    {
        // remove member from service group
        m_ServiceGroup->RemoveMember(PSERVICESINK(this));

        // release service group
        m_ServiceGroup->Release();

        // freed
        m_ServiceGroup = NULL;
    }

    if (m_DmaChannel)
    {
        // release reference
        m_DmaChannel->Release();

        // freed
        m_DmaChannel = NULL;
    }

    if (m_Stream)
    {
        if (m_State != KSSTATE_STOP)
        {
            // stop stream
            NTSTATUS Status = m_Stream->SetState(KSSTATE_STOP);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("Warning: failed to stop stream with %x\n", Status);
                PC_ASSERT(0);
            }
        }
        // set state to stop
        m_State = KSSTATE_STOP;

        DPRINT("Closing stream at Irql %u\n", KeGetCurrentIrql());

        // release stream
        m_Stream->Release();

        // stream is now freed
        m_Stream = NULL;
    }

    if (m_Filter)
    {
        // disconnect pin from filter
        m_Filter->FreePin((PPORTPINWAVECYCLIC)this);

        // release filter reference
        m_Filter->Release();

        // pin is done with filter
        m_Filter = NULL;
    }

    if (m_Port)
    {
        // release reference to port driver
        m_Port->Release();

        // work is done for port
        m_Port = NULL;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    delete this;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortPinWaveCyclic::FastDeviceIoControl(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return KsDispatchFastIoDeviceControlFailure(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, StatusBlock, DeviceObject);
}

BOOLEAN
NTAPI
CPortPinWaveCyclic::FastRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return KsDispatchFastReadFailure(FileObject, FileOffset, Length, Wait, LockKey, Buffer, StatusBlock, DeviceObject);
}

BOOLEAN
NTAPI
CPortPinWaveCyclic::FastWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return KsDispatchFastReadFailure(FileObject, FileOffset, Length, Wait, LockKey, Buffer, StatusBlock, DeviceObject);
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::Init(
    IN PPORTWAVECYCLIC Port,
    IN PPORTFILTERWAVECYCLIC Filter,
    IN KSPIN_CONNECT * ConnectDetails,
    IN KSPIN_DESCRIPTOR * KsPinDescriptor)
{
    NTSTATUS Status;
    PKSDATAFORMAT DataFormat;
    //PDEVICE_OBJECT DeviceObject;
    BOOLEAN Capture;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor = NULL;
    //IDrmAudioStream * DrmAudio = NULL;

    m_KsPinDescriptor = KsPinDescriptor;
    m_ConnectDetails = ConnectDetails;
    m_Miniport = GetWaveCyclicMiniport(Port);

    //DeviceObject = GetDeviceObject(Port);

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("CPortPinWaveCyclic::Init entered Size %u\n", DataFormat->FormatSize);

    Status = NewIrpQueue(&m_IrpQueue);
    if (!NT_SUCCESS(Status))
        return Status;

    if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_IN)
    {
        Capture = FALSE;
    }
    else if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        Capture = TRUE;
    }
    else
    {
        DPRINT("Unexpected Communication %u DataFlow %u\n", KsPinDescriptor->Communication, KsPinDescriptor->DataFlow);
        DbgBreakPoint();
        while(TRUE);
    }

    Status = m_Miniport->NewStream(&m_Stream,
                                   NULL,
                                   NonPagedPool,
                                   ConnectDetails->PinId,
                                   Capture,
                                   DataFormat,
                                   &m_DmaChannel,
                                   &m_ServiceGroup);
#if 0
    Status = m_Stream->QueryInterface(IID_IDrmAudioStream, (PVOID*)&DrmAudio);
    if (NT_SUCCESS(Status))
    {
        DRMRIGHTS DrmRights;
        DPRINT("Got IID_IDrmAudioStream interface %p\n", DrmAudio);

        DrmRights.CopyProtect = FALSE;
        DrmRights.Reserved = 0;
        DrmRights.DigitalOutputDisable = FALSE;

        Status = DrmAudio->SetContentId(1, &DrmRights);
        DPRINT("Status %x\n", Status);
    }
#endif

    DPRINT1("CPortPinWaveCyclic::Init Status %x PinId %u Capture %u\n", Status, ConnectDetails->PinId, Capture);

    if (!NT_SUCCESS(Status))
    {
        if (m_DmaChannel)
        {
            m_DmaChannel->Release();
            m_DmaChannel = NULL;
        }
        if (m_ServiceGroup)
        {
            m_ServiceGroup->Release();
            m_ServiceGroup = NULL;
        }
        return Status;
    }

    ISubdevice * Subdevice = NULL;
    // get subdevice interface
    Status = Port->QueryInterface(IID_ISubdevice, (PVOID*)&Subdevice);

    if (!NT_SUCCESS(Status))
        return Status;

    Status = Subdevice->GetDescriptor(&SubDeviceDescriptor);
    if (!NT_SUCCESS(Status))
    {
        // failed to get descriptor
        Subdevice->Release();
        return Status;
    }

    /* initialize event management */
    InitializeListHead(&m_EventList);
    KeInitializeSpinLock(&m_EventListLock);

    Status = PcCreateSubdeviceDescriptor(&m_Descriptor,
                                         SubDeviceDescriptor->InterfaceCount,
                                         SubDeviceDescriptor->Interfaces,
                                         0, /* FIXME KSINTERFACE_STANDARD with KSINTERFACE_STANDARD_STREAMING / KSINTERFACE_STANDARD_LOOPED_STREAMING */
                                         NULL,
                                         sizeof(PinWaveCyclicPropertySet) / sizeof(KSPROPERTY_SET),
                                         PinWaveCyclicPropertySet,
                                         0,
                                         0,
                                         0,
                                         NULL,
                                         sizeof(PinWaveCyclicEventSet) / sizeof(KSEVENT_SET),
                                         PinWaveCyclicEventSet,
                                         SubDeviceDescriptor->DeviceDescriptor);

    m_Descriptor->UnknownStream = (PUNKNOWN)m_Stream;
    m_Descriptor->UnknownMiniport = SubDeviceDescriptor->UnknownMiniport;
    m_Descriptor->PortPin = (PVOID)this;
    m_Descriptor->EventList = &m_EventList;
    m_Descriptor->EventListLock = &m_EventListLock;

    // initialize reset state
    m_ResetState = KSRESET_END;

    // release subdevice descriptor
    Subdevice->Release();

    // add ourselves to service group
    Status = m_ServiceGroup->AddMember(PSERVICESINK(this));
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to add pin to service group\n");
        return Status;
    }

    m_Stream->SetState(KSSTATE_STOP);
    m_State = KSSTATE_STOP;
    m_CommonBufferOffset = 0;
    m_CommonBufferSize = m_DmaChannel->BufferSize();
    m_CommonBuffer = m_DmaChannel->SystemAddress();
    m_Capture = Capture;
    // delay of 10 millisec
    m_Delay = Int32x32To64(10, -10000);

    // sanity checks
    PC_ASSERT(m_CommonBufferSize);
    PC_ASSERT(m_CommonBuffer);

    Status = m_Stream->SetNotificationFreq(10, &m_FrameSize);
    PC_ASSERT(NT_SUCCESS(Status));
    PC_ASSERT(m_FrameSize);

    DPRINT1("Bits %u Samples %u Channels %u Tag %u FrameSize %u CommonBufferSize %lu, CommonBuffer %p\n", ((PKSDATAFORMAT_WAVEFORMATEX)(DataFormat))->WaveFormatEx.wBitsPerSample, ((PKSDATAFORMAT_WAVEFORMATEX)(DataFormat))->WaveFormatEx.nSamplesPerSec, ((PKSDATAFORMAT_WAVEFORMATEX)(DataFormat))->WaveFormatEx.nChannels, ((PKSDATAFORMAT_WAVEFORMATEX)(DataFormat))->WaveFormatEx.wFormatTag, m_FrameSize, m_CommonBufferSize, m_DmaChannel->SystemAddress());

    /* set up allocator framing */
    m_AllocatorFraming.RequirementsFlags = KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
    m_AllocatorFraming.PoolType = NonPagedPool;
    m_AllocatorFraming.Frames = 8;
    m_AllocatorFraming.FileAlignment = FILE_64_BYTE_ALIGNMENT;
    m_AllocatorFraming.Reserved = 0;
    m_AllocatorFraming.FrameSize = m_FrameSize;

    m_Stream->Silence(m_CommonBuffer, m_CommonBufferSize);

    Status = m_IrpQueue->Init(ConnectDetails, KsPinDescriptor, m_FrameSize, 0, FALSE);
    if (!NT_SUCCESS(Status))
    {
       m_IrpQueue->Release();
       return Status;
    }

    m_Format = (PKSDATAFORMAT)AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!m_Format)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlMoveMemory(m_Format, DataFormat, DataFormat->FormatSize);

    Port->AddRef();
    Filter->AddRef();

    m_Port = Port;
    m_Filter = Filter;

    return STATUS_SUCCESS;
}

ULONG
NTAPI
CPortPinWaveCyclic::GetCompletedPosition()
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
NTAPI
CPortPinWaveCyclic::GetCycleCount()
{
    UNIMPLEMENTED;
    return 0;
}

ULONG
NTAPI
CPortPinWaveCyclic::GetDeviceBufferSize()
{
    return m_CommonBufferSize;
}

PVOID
NTAPI
CPortPinWaveCyclic::GetIrpStream()
{
    return (PVOID)m_IrpQueue;
}

PMINIPORT
NTAPI
CPortPinWaveCyclic::GetMiniport()
{
    return (PMINIPORT)m_Miniport;
}

NTSTATUS
NewPortPinWaveCyclic(
    OUT IPortPinWaveCyclic ** OutPin)
{
    CPortPinWaveCyclic * This;

    This = new(NonPagedPool, TAG_PORTCLASS)CPortPinWaveCyclic(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // store result
    *OutPin = (IPortPinWaveCyclic*)This;

    return STATUS_SUCCESS;
}
