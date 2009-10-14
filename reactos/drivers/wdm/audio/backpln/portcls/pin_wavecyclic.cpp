/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_wavecyclic.cpp
 * PURPOSE:         WaveCyclic IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

class CPortPinWaveCyclic : public IPortPinWaveCyclic,
                           public IServiceSink
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IPortPinWaveCyclic;
    IMP_IServiceSink;
    CPortPinWaveCyclic(IUnknown *OuterUnknown){}
    virtual ~CPortPinWaveCyclic(){}

    VOID SetState(KSSTATE State);

protected:

    VOID UpdateCommonBuffer(ULONG Position, ULONG MaxTransferCount);
    VOID UpdateCommonBufferOverlap(ULONG Position, ULONG MaxTransferCount);
    VOID NTAPI SetStreamState(IN KSSTATE State);
    NTSTATUS NTAPI HandleKsStream(IN PIRP Irp);
    NTSTATUS NTAPI HandleKsProperty(IN PIRP Irp);


    friend VOID NTAPI CloseStreamRoutineWaveCyclic(IN PDEVICE_OBJECT  DeviceObject, IN PVOID Context);
    friend VOID NTAPI SetStreamWorkerRoutineWaveCyclic(IN PDEVICE_OBJECT  DeviceObject, IN PVOID  Context);
    friend NTSTATUS NTAPI PinWaveCyclicState(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveCyclicDataFormat(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveCyclicAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWaveCyclicAllocatorFraming(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);

    IPortWaveCyclic * m_Port;
    IPortFilterWaveCyclic * m_Filter;
    KSPIN_DESCRIPTOR * m_KsPinDescriptor;
    PMINIPORTWAVECYCLIC m_Miniport;
    PSERVICEGROUP m_ServiceGroup;
    PDMACHANNEL m_DmaChannel;
    PMINIPORTWAVECYCLICSTREAM m_Stream;
    KSSTATE m_State;
    PKSDATAFORMAT m_Format;
    KSPIN_CONNECT * m_ConnectDetails;

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
    SUBDEVICE_DESCRIPTOR m_Descriptor;

    ULONG m_Delay;

    LONG m_Ref;
};


typedef struct
{
    CPortPinWaveCyclic *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

NTSTATUS NTAPI PinWaveCyclicState(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveCyclicDataFormat(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveCyclicAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWaveCyclicAllocatorFraming(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);

DEFINE_KSPROPERTY_CONNECTIONSET(PinWaveCyclicConnectionSet, PinWaveCyclicState, PinWaveCyclicDataFormat, PinWaveCyclicAllocatorFraming);
DEFINE_KSPROPERTY_AUDIOSET(PinWaveCyclicAudioSet, PinWaveCyclicAudioPosition);

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
PinWaveCyclicAllocatorFraming(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    CPortPinWaveCyclic *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;

    // get sub device descriptor 
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

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
        RtlMoveMemory(Data, &Pin->m_Position, sizeof(KSAUDIO_POSITION));

        DPRINT1("Play %lu Record %lu\n", Pin->m_Position.PlayOffset, Pin->m_Position.WriteOffset);
        Irp->IoStatus.Information = sizeof(KSAUDIO_POSITION);
        return STATUS_SUCCESS;
    }

    // not supported
    return STATUS_NOT_SUPPORTED;
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
        }
        // store result
        Irp->IoStatus.Information = sizeof(KSSTATE);
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
        PC_ASSERT(Pin->m_State == KSSTATE_STOP);
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

            // update irp queue with new format
            Pin->m_IrpQueue->UpdateFormat((PKSDATAFORMAT)NewDataFormat);

            // store new format
            Pin->m_Format = NewDataFormat;
            Irp->IoStatus.Information = NewDataFormat->FormatSize;

#if 0
            PC_ASSERT(NewDataFormat->FormatSize == sizeof(KSDATAFORMAT_WAVEFORMATEX));
            PC_ASSERT(IsEqualGUIDAligned(((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.MajorFormat, KSDATAFORMAT_TYPE_AUDIO));
            PC_ASSERT(IsEqualGUIDAligned(((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.SubFormat, KSDATAFORMAT_SUBTYPE_PCM));
            PC_ASSERT(IsEqualGUIDAligned(((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.Specifier, KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));


            DPRINT1("NewDataFormat: Channels %u Bits %u Samples %u\n", ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nChannels,
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
CPortPinWaveCyclic::UpdateCommonBuffer(
    ULONG Position,
    ULONG MaxTransferCount)
{
    ULONG BufferLength;
    ULONG BytesToCopy;
    ULONG BufferSize;
    PUCHAR Buffer;
    NTSTATUS Status;

    BufferLength = Position - m_CommonBufferOffset;
    BufferLength = min(BufferLength, MaxTransferCount);

    while(BufferLength)
    {
        Status = m_IrpQueue->GetMapping(&Buffer, &BufferSize);
        if (!NT_SUCCESS(Status))
            return;

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

        BufferLength = Position - m_CommonBufferOffset;
        m_Position.PlayOffset += BytesToCopy;
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
            return;

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

        BufferLength = m_CommonBufferSize - m_CommonBufferOffset;
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
SetStreamWorkerRoutineWaveCyclic(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    CPortPinWaveCyclic * This;
    PSETSTREAM_CONTEXT Ctx = (PSETSTREAM_CONTEXT)Context;
    KSSTATE State;
    ULONG MinimumDataThreshold;
    ULONG MaximumDataThreshold;

    This = Ctx->Pin;
    State = Ctx->State;

    IoFreeWorkItem(Ctx->WorkItem);
    FreeItem(Ctx, TAG_PORTCLASS);

    // Has the audio stream resumed?
    if (This->m_IrpQueue->NumMappings() && State == KSSTATE_STOP)
        return;

    // Has the audio state already been set?
    if (This->m_State == State)
        return;

    // Set the state
    if (NT_SUCCESS(This->m_Stream->SetState(State)))
    {
        // Set internal state
        This->m_State = State;

        if (This->m_State == KSSTATE_STOP)
        {
            // reset start stream
            This->m_IrpQueue->CancelBuffers(); //FIX function name

            // increase stop counter
            This->m_StopCount++;
            // get current data threshold
            MinimumDataThreshold = This->m_IrpQueue->GetMinimumDataThreshold();
            // get maximum data threshold
            MaximumDataThreshold = ((PKSDATAFORMAT_WAVEFORMATEX)This->m_Format)->WaveFormatEx.nAvgBytesPerSec;
            // increase minimum data threshold by a third sec
            MinimumDataThreshold += This->m_FrameSize * 10;

            // assure it has not exceeded
            MinimumDataThreshold = min(MinimumDataThreshold, MaximumDataThreshold);
            // store minimum data threshold
            This->m_IrpQueue->SetMinimumDataThreshold(MinimumDataThreshold);

            DPRINT1("Stopping TotalPackets %u StopCount %u\n", This->m_TotalPackets, This->m_StopCount);
        }
        if (This->m_State == KSSTATE_RUN)
        {
            DPRINT1("State RUN %x MinAvailable %u CommonBufferSize %u Offset %u\n", State, This->m_IrpQueue->MinimumDataAvailable(), This->m_CommonBufferSize, This->m_CommonBufferOffset);
        }
    }
}

VOID
NTAPI
CPortPinWaveCyclic::SetStreamState(
   IN KSSTATE State)
{
    PDEVICE_OBJECT DeviceObject;
    PIO_WORKITEM WorkItem;
    PSETSTREAM_CONTEXT Context;

    PC_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    // Has the audio stream resumed?
    if (m_IrpQueue->NumMappings() && State == KSSTATE_STOP)
        return;

    // Has the audio state already been set?
    if (m_State == State)
        return;

    // Get device object
    DeviceObject = GetDeviceObject(m_Port);

    // allocate set state context
    Context = (PSETSTREAM_CONTEXT)AllocateItem(NonPagedPool, sizeof(SETSTREAM_CONTEXT), TAG_PORTCLASS);

    if (!Context)
        return;

    // allocate work item
    WorkItem = IoAllocateWorkItem(DeviceObject);

    if (!WorkItem)
    {
        ExFreePool(Context);
        return;
    }

    Context->Pin = this;
    Context->WorkItem = WorkItem;
    Context->State = State;

    // queue the work item
    IoQueueWorkItem(WorkItem, SetStreamWorkerRoutineWaveCyclic, DelayedWorkQueue, (PVOID)Context);
}

VOID
NTAPI
CPortPinWaveCyclic::RequestService()
{
    ULONG Position;
    NTSTATUS Status;
    PUCHAR Buffer;
    ULONG BufferSize;

    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    Status = m_IrpQueue->GetMapping(&Buffer, &BufferSize);
    if (!NT_SUCCESS(Status))
    {
        //SetStreamState(This, KSSTATE_STOP);
        return;
    }

    Status = m_Stream->GetPosition(&Position);
    DPRINT1("Position %u Buffer %p BufferSize %u ActiveIrpOffset %u Capture %u\n", Position, Buffer, m_CommonBufferSize, BufferSize, m_Capture);

    if (Position < m_CommonBufferOffset)
    {
        UpdateCommonBufferOverlap(Position, m_FrameSize);
    }
    else if (Position >= m_CommonBufferOffset)
    {
        UpdateCommonBuffer(Position, m_FrameSize);
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
    UNIMPLEMENTED
    DbgBreakPoint();
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::HandleKsProperty(
    IN PIRP Irp)
{
    PKSPROPERTY Property;
    NTSTATUS Status;
    UNICODE_STRING GuidString;
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("IPortPinWave_HandleKsProperty entered\n");

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        DPRINT1("Unhandled function %lx Length %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode, IoStack->Parameters.DeviceIoControl.InputBufferLength);
        
        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    Status = PcHandlePropertyWithTable(Irp,  m_Descriptor.FilterPropertySetCount, m_Descriptor.FilterPropertySet, &m_Descriptor);

    if (Status == STATUS_NOT_FOUND)
    {
        Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

        RtlStringFromGUID(Property->Set, &GuidString);
        DPRINT1("Unhandeled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
        RtlFreeUnicodeString(&GuidString);
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
CPortPinWaveCyclic::HandleKsStream(
    IN PIRP Irp)
{
    NTSTATUS Status;
    InterlockedIncrement((PLONG)&m_TotalPackets);

    DPRINT("IPortPinWaveCyclic_HandleKsStream entered Total %u State %x MinData %u\n", m_TotalPackets, m_State, m_IrpQueue->NumData());

    Status = m_IrpQueue->AddMapping(NULL, 0, Irp);

    if (NT_SUCCESS(Status))
    {

        PKSSTREAM_HEADER Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
        PC_ASSERT(Header);

        if (m_Capture)
            m_Position.WriteOffset += Header->FrameExtent;
        else
            m_Position.WriteOffset += Header->DataUsed;

    }


    return STATUS_PENDING;
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);


    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
        return HandleKsProperty(Irp);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_ENABLE_EVENT)
    {
        /// FIXME
        /// handle enable event
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_DISABLE_EVENT)
    {
        /// FIXME
        /// handle disable event
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_RESET_STATE)
    {
        /// FIXME
        /// handle reset state
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_WRITE_STREAM || IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_READ_STREAM)
    {
       return HandleKsStream(Irp);
    }
    else
    {
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

    UNIMPLEMENTED

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
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

VOID
NTAPI
CloseStreamRoutineWaveCyclic(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID Context)
{
    PMINIPORTWAVECYCLICSTREAM Stream;
    CPortPinWaveCyclic * This;
    NTSTATUS Status;
    PCLOSESTREAM_CONTEXT Ctx = (PCLOSESTREAM_CONTEXT)Context;

    This = (CPortPinWaveCyclic*)Ctx->Pin;

    if (This->m_State != KSSTATE_STOP)
    {
        // stop stream in case it hasn't been
        Status = This->m_Stream->SetState(KSSTATE_STOP);
        if (!NT_SUCCESS(Status))
            DPRINT1("Warning: failed to stop stream with %x\n", Status);

        This->m_State = KSSTATE_STOP;
    }

    if (This->m_Format)
    {
        // free format
        ExFreePool(This->m_Format);
        This->m_Format = NULL;
    }

    if (This->m_IrpQueue)
    {
        This->m_IrpQueue->Release();
    }

    // complete the irp
    Ctx->Irp->IoStatus.Information = 0;
    Ctx->Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Ctx->Irp, IO_NO_INCREMENT);

    // free the work item
    IoFreeWorkItem(Ctx->WorkItem);

    // free work item ctx
    FreeItem(Ctx, TAG_PORTCLASS);

    // release reference to port driver
    This->m_Port->Release();

    // release reference to filter instance
    This->m_Filter->Release();

    if (This->m_Stream)
    {
        Stream = This->m_Stream;
        This->m_Stream = NULL;
        This->m_Filter->FreePin(This);
        DPRINT1("Closing stream at Irql %u\n", KeGetCurrentIrql());
        Stream->Release();
    }
}

NTSTATUS
NTAPI
CPortPinWaveCyclic::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCLOSESTREAM_CONTEXT Ctx;

    DPRINT1("CPortPinWaveCyclic::Close entered\n");

    if (m_Stream)
    {
        // allocate a close context
        Ctx = (PCLOSESTREAM_CONTEXT)AllocateItem(NonPagedPool, sizeof(CLOSESTREAM_CONTEXT), TAG_PORTCLASS);
        if (!Ctx)
        {
            DPRINT1("Failed to allocate stream context\n");
            goto cleanup;
        }
        // allocate work context
        Ctx->WorkItem = IoAllocateWorkItem(DeviceObject);
        if (!Ctx->WorkItem)
        {
            DPRINT1("Failed to allocate work item\n");
            goto cleanup;
        }
        // setup the close context
        Ctx->Irp = Irp;
        Ctx->Pin = this;

        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_PENDING;

        // remove member from service group
        m_ServiceGroup->RemoveMember(PSERVICESINK(this));

        // defer work item
        IoQueueWorkItem(Ctx->WorkItem, CloseStreamRoutineWaveCyclic, DelayedWorkQueue, (PVOID)Ctx);
        // Return result
        return STATUS_PENDING;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

cleanup:

    if (Ctx)
        FreeItem(Ctx, TAG_PORTCLASS);

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_UNSUCCESSFUL;

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
    PDEVICE_OBJECT DeviceObject;
    BOOLEAN Capture;
    PVOID SilenceBuffer;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor = NULL;
    //IDrmAudioStream * DrmAudio = NULL;

    m_KsPinDescriptor = KsPinDescriptor;
    m_ConnectDetails = ConnectDetails;
    m_Miniport = GetWaveCyclicMiniport(Port);

    DeviceObject = GetDeviceObject(Port);

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
        DPRINT1("Unexpected Communication %u DataFlow %u\n", KsPinDescriptor->Communication, KsPinDescriptor->DataFlow);
        KeBugCheck(0);
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
        DPRINT1("Got IID_IDrmAudioStream interface %p\n", DrmAudio);

        DrmRights.CopyProtect = FALSE;
        DrmRights.Reserved = 0;
        DrmRights.DigitalOutputDisable = FALSE;

        Status = DrmAudio->SetContentId(1, &DrmRights);
        DPRINT("Status %x\n", Status);
    }
#endif

    DPRINT("CPortPinWaveCyclic::Init Status %x\n", Status);

    if (!NT_SUCCESS(Status))
        return Status;

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

    /* set up subdevice descriptor */
    RtlZeroMemory(&m_Descriptor, sizeof(SUBDEVICE_DESCRIPTOR));
    m_Descriptor.FilterPropertySet = PinWaveCyclicPropertySet;
    m_Descriptor.FilterPropertySetCount = sizeof(PinWaveCyclicPropertySet) / sizeof(KSPROPERTY_SET);
    m_Descriptor.UnknownStream = (PUNKNOWN)m_Stream;
    m_Descriptor.DeviceDescriptor = SubDeviceDescriptor->DeviceDescriptor;
    m_Descriptor.UnknownMiniport = SubDeviceDescriptor->UnknownMiniport;
    m_Descriptor.PortPin = (PVOID)this;

    // release subdevice descriptor
    Subdevice->Release();

    // add ourselves to service group
    Status = m_ServiceGroup->AddMember(PSERVICESINK(this));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to add pin to service group\n");
        return Status;
    }

    m_ServiceGroup->SupportDelayedService();
    m_Stream->SetState(KSSTATE_STOP);
    m_State = KSSTATE_STOP;
    m_CommonBufferOffset = 0;
    m_CommonBufferSize = m_DmaChannel->AllocatedBufferSize();
    m_CommonBuffer = m_DmaChannel->SystemAddress();
    m_Capture = Capture;
    // delay of 10 milisec
    m_Delay = Int32x32To64(10, -10000);

    Status = m_Stream->SetNotificationFreq(10, &m_FrameSize);

    SilenceBuffer = AllocateItem(NonPagedPool, m_FrameSize, TAG_PORTCLASS);
    if (!SilenceBuffer)
        return STATUS_INSUFFICIENT_RESOURCES;


    /* set up allocator framing */
    m_AllocatorFraming.RequirementsFlags = KSALLOCATOR_REQUIREMENTF_SYSTEM_MEMORY | KSALLOCATOR_REQUIREMENTF_PREFERENCES_ONLY;
    m_AllocatorFraming.PoolType = NonPagedPool;
    m_AllocatorFraming.Frames = 8;
    m_AllocatorFraming.FileAlignment = FILE_64_BYTE_ALIGNMENT;
    m_AllocatorFraming.Reserved = 0;
    m_AllocatorFraming.FrameSize = m_FrameSize;

    m_Stream->Silence(SilenceBuffer, m_FrameSize);

    Status = m_IrpQueue->Init(ConnectDetails, DataFormat, DeviceObject, m_FrameSize, 0, SilenceBuffer);
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

    DPRINT1("Setting state to acquire %x\n", m_Stream->SetState(KSSTATE_ACQUIRE));
    DPRINT1("Setting state to pause %x\n", m_Stream->SetState(KSSTATE_PAUSE));
    m_State = KSSTATE_PAUSE;


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

