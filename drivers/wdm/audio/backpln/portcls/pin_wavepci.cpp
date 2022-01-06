/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_wavepci.cpp
 * PURPOSE:         WavePci IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CPortPinWavePci : public IPortPinWavePci,
                        public IServiceSink,
                        public IPortWavePciStream
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
    IMP_IPortPinWavePci;
    IMP_IServiceSink;
    IMP_IPortWavePciStream;
    CPortPinWavePci(IUnknown *OuterUnknown) {}
    virtual ~CPortPinWavePci(){}
protected:

    friend NTSTATUS NTAPI PinWavePciState(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWavePciDataFormat(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWavePciAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
    friend NTSTATUS NTAPI PinWavePciAllocatorFraming(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);

    IPortWavePci * m_Port;
    IPortFilterWavePci * m_Filter;
    KSPIN_DESCRIPTOR * m_KsPinDescriptor;
    PMINIPORTWAVEPCI m_Miniport;
    PSERVICEGROUP m_ServiceGroup;
    PDMACHANNEL m_DmaChannel;
    PMINIPORTWAVEPCISTREAM m_Stream;
    KSSTATE m_State;
    PKSDATAFORMAT m_Format;
    KSPIN_CONNECT * m_ConnectDetails;

    BOOL m_Capture;
    PDEVICE_OBJECT m_DeviceObject;
    IIrpQueue * m_IrpQueue;

    ULONG m_TotalPackets;
    KSAUDIO_POSITION m_Position;
    ULONG m_StopCount;

    BOOL m_bUsePrefetch;
    ULONG m_PrefetchOffset;
    SUBDEVICE_DESCRIPTOR m_Descriptor;

    KSALLOCATOR_FRAMING m_AllocatorFraming;

    LONG m_Ref;

    NTSTATUS NTAPI HandleKsProperty(IN PIRP Irp);
    NTSTATUS NTAPI HandleKsStream(IN PIRP Irp);
};

typedef struct
{
    CPortPinWavePci *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

NTSTATUS NTAPI PinWavePciState(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWavePciDataFormat(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWavePciAudioPosition(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);
NTSTATUS NTAPI PinWavePciAllocatorFraming(IN PIRP Irp, IN PKSIDENTIFIER Request, IN OUT PVOID Data);

DEFINE_KSPROPERTY_CONNECTIONSET(PinWavePciConnectionSet, PinWavePciState, PinWavePciDataFormat, PinWavePciAllocatorFraming);
DEFINE_KSPROPERTY_AUDIOSET(PinWavePciAudioSet, PinWavePciAudioPosition);

KSPROPERTY_SET PinWavePciPropertySet[] =
{
    {
        &KSPROPSETID_Connection,
        sizeof(PinWavePciConnectionSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PinWavePciConnectionSet,
        0,
        NULL
    },
    {
        &KSPROPSETID_Audio,
        sizeof(PinWavePciAudioSet) / sizeof(KSPROPERTY_ITEM),
        (const KSPROPERTY_ITEM*)&PinWavePciAudioSet,
        0,
        NULL
    }
};


NTSTATUS
NTAPI
PinWavePciAllocatorFraming(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    CPortPinWavePci *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWavePci*)Descriptor->PortPin;


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
PinWavePciAudioPosition(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    CPortPinWavePci *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWavePci*)Descriptor->PortPin;

    //sanity check
    PC_ASSERT(Pin->m_Stream);

    if (Request->Flags & KSPROPERTY_TYPE_GET)
    {
        // FIXME non multithreading-safe
        // copy audio position
        RtlMoveMemory(Data, &Pin->m_Position, sizeof(KSAUDIO_POSITION));

        DPRINT("Play %lu Record %lu\n", Pin->m_Position.PlayOffset, Pin->m_Position.WriteOffset);
        Irp->IoStatus.Information = sizeof(KSAUDIO_POSITION);
        return STATUS_SUCCESS;
    }

    // not supported
    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
NTAPI
PinWavePciState(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    CPortPinWavePci *Pin;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    PVOID FirstTag, LastTag;
    ULONG MappingsRevoked;
    PKSSTATE State = (PKSSTATE)Data;

    // get sub device descriptor
    Descriptor = (PSUBDEVICE_DESCRIPTOR)KSPROPERTY_ITEM_IRP_STORAGE(Irp);

    // sanity check
    PC_ASSERT(Descriptor);
    PC_ASSERT(Descriptor->PortPin);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    // cast to pin impl
    Pin = (CPortPinWavePci*)Descriptor->PortPin;

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
                Pin->m_IrpQueue->CancelBuffers();
                while(Pin->m_IrpQueue->GetAcquiredTagRange(&FirstTag, &LastTag))
                {
                    Status = Pin->m_Stream->RevokeMappings(FirstTag, LastTag, &MappingsRevoked);
                    DPRINT("RevokeMappings Status %lx MappingsRevoked: %lu\n", Status, MappingsRevoked);
                    KeStallExecutionProcessor(10);
                }
                Pin->m_Position.PlayOffset = 0;
                Pin->m_Position.WriteOffset = 0;
            }
            else if (Pin->m_State == KSSTATE_STOP)
            {
                Pin->m_IrpQueue->CancelBuffers();
                while(Pin->m_IrpQueue->GetAcquiredTagRange(&FirstTag, &LastTag))
                {
                    Status = Pin->m_Stream->RevokeMappings(FirstTag, LastTag, &MappingsRevoked);
                    DPRINT("RevokeMappings Status %lx MappingsRevoked: %lu\n", Status, MappingsRevoked);
                    KeStallExecutionProcessor(10);
                }
                Pin->m_Position.PlayOffset = 0;
                Pin->m_Position.WriteOffset = 0;
            }
            // store result
            Irp->IoStatus.Information = sizeof(KSSTATE);

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
PinWavePciDataFormat(
    IN PIRP Irp,
    IN PKSIDENTIFIER Request,
    IN OUT PVOID Data)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    CPortPinWavePci *Pin;
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
    Pin = (CPortPinWavePci*)Descriptor->PortPin;

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


//==================================================================================================================================
NTSTATUS
NTAPI
CPortPinWavePci::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    //DPRINT("CPortPinWavePci::QueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, IID_IIrpTarget) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN((IIrpTarget*)this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    if (IsEqualGUIDAligned(refiid, IID_IServiceSink))
    {
        *Output = PVOID(PSERVICESINK(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }


    if (IsEqualGUIDAligned(refiid, IID_IPortWavePciStream))
    {
        *Output = PVOID(PPORTWAVEPCISTREAM(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWavePci::GetMapping(
    IN PVOID Tag,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress,
    OUT PVOID  *VirtualAddress,
    OUT PULONG  ByteCount,
    OUT PULONG  Flags)
{

    PC_ASSERT_IRQL(DISPATCH_LEVEL);
    return m_IrpQueue->GetMappingWithTag(Tag, PhysicalAddress, VirtualAddress, ByteCount, Flags);
}

NTSTATUS
NTAPI
CPortPinWavePci::ReleaseMapping(
    IN PVOID  Tag)
{

    PC_ASSERT_IRQL(DISPATCH_LEVEL);
    return m_IrpQueue->ReleaseMappingWithTag(Tag);
}

NTSTATUS
NTAPI
CPortPinWavePci::TerminatePacket()
{
    UNIMPLEMENTED;
    PC_ASSERT_IRQL(DISPATCH_LEVEL);
    return STATUS_SUCCESS;
}


VOID
NTAPI
CPortPinWavePci::RequestService()
{
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    if (m_State == KSSTATE_RUN)
    {
        m_Stream->Service();
        //TODO
        //generate events
    }
}

//==================================================================================================================================

NTSTATUS
NTAPI
CPortPinWavePci::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    UNIMPLEMENTED;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWavePci::HandleKsProperty(
    IN PIRP Irp)
{
    //PKSPROPERTY Property;
    NTSTATUS Status;
    //UNICODE_STRING GuidString;
    PIO_STACK_LOCATION IoStack;

    //DPRINT("IPortPinWave_HandleKsProperty entered\n");

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        //DPRINT("Unhandled function %lx Length %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode, IoStack->Parameters.DeviceIoControl.InputBufferLength);

        Irp->IoStatus.Status = STATUS_SUCCESS;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    Status = PcHandlePropertyWithTable(Irp,  m_Descriptor.FilterPropertySetCount, m_Descriptor.FilterPropertySet, &m_Descriptor);

    if (Status == STATUS_NOT_FOUND)
    {
        //Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
#if 0
        RtlStringFromGUID(Property->Set, &GuidString);
        //DPRINT("Unhandled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
        RtlFreeUnicodeString(&GuidString);
#endif
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
CPortPinWavePci::HandleKsStream(
    IN PIRP Irp)
{
    NTSTATUS Status;
    ULONG Data = 0;
    InterlockedIncrement((PLONG)&m_TotalPackets);

    DPRINT("IPortPinWaveCyclic_HandleKsStream entered Total %u State %x MinData %u\n", m_TotalPackets, m_State, m_IrpQueue->NumData());

    Status = m_IrpQueue->AddMapping(Irp, &Data);

    if (NT_SUCCESS(Status))
    {
        if (m_Capture)
            m_Position.WriteOffset += Data;
        else
            m_Position.PlayOffset += Data;

        if (m_State == KSSTATE_RUN)
        {
            // notify stream of new mapping
            m_Stream->MappingAvailable();
        }

        return STATUS_PENDING;
    }

    return Status;
}


NTSTATUS
NTAPI
CPortPinWavePci::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
        return HandleKsProperty(Irp);
    }
    else if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_WRITE_STREAM || IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_READ_STREAM)
    {
       return HandleKsStream(Irp);
    }

    UNIMPLEMENTED;

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWavePci::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWavePci::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWavePci::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWavePci::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status;

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

        // do not release service group, it is released by the miniport object
        m_ServiceGroup = NULL;
    }

    if (m_Stream)
    {
        if (m_State != KSSTATE_STOP)
        {
            // stop stream
            Status = m_Stream->SetState(KSSTATE_STOP);
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
        m_Filter->FreePin((PPORTPINWAVEPCI)this);

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

    // successfully complete irp
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortPinWavePci::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinWavePci::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortPinWavePci::FastDeviceIoControl(
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
    return FALSE;
}

BOOLEAN
NTAPI
CPortPinWavePci::FastRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}

BOOLEAN
NTAPI
CPortPinWavePci::FastWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    return FALSE;
}


NTSTATUS
NTAPI
CPortPinWavePci::Init(
    IN PPORTWAVEPCI Port,
    IN PPORTFILTERWAVEPCI Filter,
    IN KSPIN_CONNECT * ConnectDetails,
    IN KSPIN_DESCRIPTOR * KsPinDescriptor,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PKSDATAFORMAT DataFormat;
    BOOLEAN Capture;
    ISubdevice * Subdevice = NULL;
    PSUBDEVICE_DESCRIPTOR SubDeviceDescriptor = NULL;

    // check if it is a source / sink pin
    if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_IN)
    {
        // sink pin
        Capture = FALSE;
    }
    else if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        // source pin
        Capture = TRUE;
    }
    else
    {
        DPRINT("Unexpected Communication %u DataFlow %u\n", KsPinDescriptor->Communication, KsPinDescriptor->DataFlow);
        DbgBreakPoint();
        while(TRUE);
    }

    // add port / filter reference
    Port->AddRef();
    Filter->AddRef();

    // initialize pin
    m_Port = Port;
    m_Filter = Filter;
    m_KsPinDescriptor = KsPinDescriptor;
    m_ConnectDetails = ConnectDetails;
    m_Miniport = GetWavePciMiniport(Port);
    m_DeviceObject = DeviceObject;
    m_State = KSSTATE_STOP;
    m_Capture = Capture;

    DPRINT("IPortPinWavePci_fnInit entered\n");

    // get dataformat
    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    // allocate data format
    m_Format = (PKSDATAFORMAT)AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!m_Format)
    {
        // release references
        m_Port->Release();
        m_Filter->Release();

        // no dangling pointers
        Port = NULL;
        Filter = NULL;

        // failed to allocate data format
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // copy data format
    RtlMoveMemory(m_Format, DataFormat, DataFormat->FormatSize);

    // allocate new stream
    Status = m_Miniport->NewStream(&m_Stream,
                                   NULL,
                                   NonPagedPool,
                                   PPORTWAVEPCISTREAM(this),
                                   ConnectDetails->PinId,
                                   Capture,
                                   m_Format,
                                   &m_DmaChannel,
                                   &m_ServiceGroup);

    DPRINT("IPortPinWavePci_fnInit Status %x\n", Status);

    if (!NT_SUCCESS(Status))
    {
        // free references
        Port->Release();
        Filter->Release();

        // free data format
        FreeItem(m_Format, TAG_PORTCLASS);

        // no dangling pointers
        m_Port = NULL;
        m_Filter = NULL;
        m_Format = NULL;

        // failed to allocate stream
        return Status;
    }

    // get allocator requirements for pin
    Status = m_Stream->GetAllocatorFraming(&m_AllocatorFraming);
    if (NT_SUCCESS(Status))
    {
        DPRINT("OptionFlags %x RequirementsFlag %x PoolType %x Frames %lu FrameSize %lu FileAlignment %lu\n",
               m_AllocatorFraming.OptionsFlags, m_AllocatorFraming.RequirementsFlags, m_AllocatorFraming.PoolType, m_AllocatorFraming.Frames, m_AllocatorFraming.FrameSize, m_AllocatorFraming.FileAlignment);
    }

    // allocate new irp queue
    Status = NewIrpQueue(&m_IrpQueue);
    if (!NT_SUCCESS(Status))
    {
        // free references
        Port->Release();
        Filter->Release();
        m_Stream->Release();

        // free data format
        FreeItem(m_Format, TAG_PORTCLASS);

        // no dangling pointers
        m_Port = NULL;
        m_Filter = NULL;
        m_Format = NULL;
        m_Stream = NULL;

        // failed to allocate irp queue
        return Status;
    }

    // initialize irp queue
    Status = m_IrpQueue->Init(ConnectDetails, KsPinDescriptor, m_AllocatorFraming.FrameSize, m_AllocatorFraming.FileAlignment, TRUE);
    if (!NT_SUCCESS(Status))
    {
        // this should never happen
        ASSERT(0);
    }

    // get subdevice interface
    Status = Port->QueryInterface(IID_ISubdevice, (PVOID*)&Subdevice);

    if (!NT_SUCCESS(Status))
    {
        // this function should never fail
        ASSERT(0);
    }

    // get subdevice descriptor
    Status = Subdevice->GetDescriptor(&SubDeviceDescriptor);
    if (!NT_SUCCESS(Status))
    {
        // this function should never fail
        ASSERT(0);
    }

    // release subdevice
    Subdevice->Release();

    /* set up subdevice descriptor */
    RtlZeroMemory(&m_Descriptor, sizeof(SUBDEVICE_DESCRIPTOR));
    m_Descriptor.FilterPropertySet = PinWavePciPropertySet;
    m_Descriptor.FilterPropertySetCount = sizeof(PinWavePciPropertySet) / sizeof(KSPROPERTY_SET);
    m_Descriptor.UnknownStream = (PUNKNOWN)m_Stream;
    m_Descriptor.DeviceDescriptor = SubDeviceDescriptor->DeviceDescriptor;
    m_Descriptor.UnknownMiniport = SubDeviceDescriptor->UnknownMiniport;
    m_Descriptor.PortPin = (PVOID)this;

    if (m_ServiceGroup)
    {
        Status = m_ServiceGroup->AddMember(PSERVICESINK(this));
        if (!NT_SUCCESS(Status))
        {
            // free references
            m_Stream->Release();
            Port->Release();
            Filter->Release();

            // free data format
            FreeItem(m_Format, TAG_PORTCLASS);

            // no dangling pointers
            m_Stream = NULL;
            m_Port = NULL;
            m_Filter = NULL;
            m_Format = NULL;

            // failed to add to service group
            return Status;
        }
    }


    return STATUS_SUCCESS;
}

PVOID
NTAPI
CPortPinWavePci::GetIrpStream()
{
    return (PVOID)m_IrpQueue;
}


PMINIPORT
NTAPI
CPortPinWavePci::GetMiniport()
{
    return (PMINIPORT)m_Miniport;
}


NTSTATUS
NewPortPinWavePci(
    OUT IPortPinWavePci ** OutPin)
{
    CPortPinWavePci * This;

    This = new(NonPagedPool, TAG_PORTCLASS) CPortPinWavePci(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // store result
    *OutPin = (IPortPinWavePci*)This;

    return STATUS_SUCCESS;
}

