/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_wavepci.cpp
 * PURPOSE:         WavePci IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

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

    VOID NTAPI SetState( IN KSSTATE State);
    VOID NTAPI CloseStream();
protected:

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
    ULONG m_PreCompleted;
    ULONG m_PostCompleted;
    ULONG m_StopCount;

    ULONG m_Delay;

    BOOL m_bUsePrefetch;
    ULONG m_PrefetchOffset;

    KSALLOCATOR_FRAMING m_AllocatorFraming;

    LONG m_Ref;

    NTSTATUS NTAPI HandleKsProperty(IN PVOID InputBuffer, IN ULONG InputBufferLength, IN PVOID OutputBuffer, IN ULONG OutputBufferLength, IN PIO_STATUS_BLOCK IoStatusBlock);
    VOID NTAPI SetStreamState( IN KSSTATE State);
};

typedef struct
{
    CPortPinWavePci *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

//==================================================================================================================================
NTSTATUS
NTAPI
CPortPinWavePci::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    DPRINT("CPortPinWavePci::QueryInterface entered\n");

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
    UNIMPLEMENTED
    PC_ASSERT_IRQL(DISPATCH_LEVEL);
    return STATUS_SUCCESS;
}


VOID
CPortPinWavePci::SetState(KSSTATE State)
{
    ULONG MinimumDataThreshold;
    ULONG MaximumDataThreshold;

    // Has the audio stream resumed?
    if (m_IrpQueue->NumMappings() && State == KSSTATE_STOP)
        return;

    // Set the state
    if (NT_SUCCESS(m_Stream->SetState(State)))
    {
        // Save new internal state
        m_State = State;

        if (m_State == KSSTATE_STOP)
        {
            // reset start stream
            m_IrpQueue->CancelBuffers(); //FIX function name
            //This->ServiceGroup->lpVtbl->CancelDelayedService(This->ServiceGroup);
            // increase stop counter
            m_StopCount++;
            // get current data threshold
            MinimumDataThreshold = m_IrpQueue->GetMinimumDataThreshold();
            // get maximum data threshold
            MaximumDataThreshold = ((PKSDATAFORMAT_WAVEFORMATEX)m_Format)->WaveFormatEx.nAvgBytesPerSec;
            // increase minimum data threshold by 10 frames
            MinimumDataThreshold += m_AllocatorFraming.FrameSize * 10;

            // assure it has not exceeded
            MinimumDataThreshold = min(MinimumDataThreshold, MaximumDataThreshold);
            // store minimum data threshold
            m_IrpQueue->SetMinimumDataThreshold(MinimumDataThreshold);

            DPRINT1("Stopping PreCompleted %u PostCompleted %u StopCount %u MinimumDataThreshold %u\n", m_PreCompleted, m_PostCompleted, m_StopCount, MinimumDataThreshold);
        }
        if (m_State == KSSTATE_RUN)
        {
            // start the notification timer
            //m_ServiceGroup->RequestDelayedService(m_ServiceGroup, m_Delay);
        }
    }


}

VOID
NTAPI
PinWavePciSetStreamWorkerRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    CPortPinWavePci * This;
    PSETSTREAM_CONTEXT Ctx = (PSETSTREAM_CONTEXT)Context;
    KSSTATE State;

    This = Ctx->Pin;
    State = Ctx->State;

    IoFreeWorkItem(Ctx->WorkItem);
    FreeItem(Ctx, TAG_PORTCLASS);

    This->SetState(State);
}

VOID
NTAPI
CPortPinWavePci::SetStreamState(
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
    DeviceObject = GetDeviceObjectFromPortWavePci(m_Port);

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
    IoQueueWorkItem(WorkItem, PinWavePciSetStreamWorkerRoutine, DelayedWorkQueue, (PVOID)Context);
}


VOID
NTAPI
CPortPinWavePci::RequestService()
{
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    if (m_IrpQueue->HasLastMappingFailed())
    {
        if (m_IrpQueue->NumMappings() == 0)
        {
            DPRINT("Stopping stream...\n");
            SetStreamState(KSSTATE_STOP);
            return;
        }
    }

    m_Stream->Service();
    //TODO
    //generate events
}

//==================================================================================================================================

NTSTATUS
NTAPI
CPortPinWavePci::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    UNIMPLEMENTED

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinWavePci::HandleKsProperty(
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN PIO_STATUS_BLOCK IoStatusBlock)
{
    PKSPROPERTY Property;
    NTSTATUS Status;
    UNICODE_STRING GuidString;

    DPRINT("IPortPinWavePci_HandleKsProperty entered\n");

    if (InputBufferLength < sizeof(KSPROPERTY))
    {
        IoStatusBlock->Information = 0;
        IoStatusBlock->Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Property = (PKSPROPERTY)InputBuffer;

    if (IsEqualGUIDAligned(Property->Set, KSPROPSETID_Connection))
    {
        if (Property->Id == KSPROPERTY_CONNECTION_STATE)
        {
            PKSSTATE State = (PKSSTATE)OutputBuffer;

            PC_ASSERT_IRQL(DISPATCH_LEVEL);
            if (OutputBufferLength < sizeof(KSSTATE))
            {
                IoStatusBlock->Information = sizeof(KSSTATE);
                IoStatusBlock->Status = STATUS_BUFFER_TOO_SMALL;
                return STATUS_BUFFER_TOO_SMALL;
            }

            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                Status = STATUS_UNSUCCESSFUL;
                IoStatusBlock->Information = 0;

                if (m_Stream)
                {
                    Status = m_Stream->SetState(*State);

                    DPRINT1("Setting state %u %x\n", *State, Status);
                    if (NT_SUCCESS(Status))
                    {
                        m_State = *State;
                    }
                }
                IoStatusBlock->Status = Status;
                return Status;
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                *State = m_State;
                IoStatusBlock->Information = sizeof(KSSTATE);
                IoStatusBlock->Status = STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
        }
        else if (Property->Id == KSPROPERTY_CONNECTION_DATAFORMAT)
        {
            PKSDATAFORMAT DataFormat = (PKSDATAFORMAT)OutputBuffer;
            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                PKSDATAFORMAT NewDataFormat;
                if (!RtlCompareMemory(DataFormat, m_Format, DataFormat->FormatSize))
                {
                    IoStatusBlock->Information = DataFormat->FormatSize;
                    IoStatusBlock->Status = STATUS_SUCCESS;
                    return STATUS_SUCCESS;
                }

                NewDataFormat = (PKSDATAFORMAT)AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
                if (!NewDataFormat)
                {
                    IoStatusBlock->Information = 0;
                    IoStatusBlock->Status = STATUS_NO_MEMORY;
                    return STATUS_NO_MEMORY;
                }
                RtlMoveMemory(NewDataFormat, DataFormat, DataFormat->FormatSize);

                if (m_Stream)
                {
#if 0
                    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
                    ASSERT(NewDataFormat->FormatSize == sizeof(KSDATAFORMAT_WAVEFORMATEX));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.MajorFormat, &KSDATAFORMAT_TYPE_AUDIO));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));

                    ASSERT(This->State == KSSTATE_STOP);
#endif
                    DPRINT1("NewDataFormat: Channels %u Bits %u Samples %u\n", ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nChannels,
                                                                                 ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.wBitsPerSample,
                                                                                 ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nSamplesPerSec);

                    Status = m_Stream->SetFormat(NewDataFormat);
                    if (NT_SUCCESS(Status))
                    {
                        if (m_Format)
                            ExFreePoolWithTag(m_Format, TAG_PORTCLASS);

                        m_IrpQueue->UpdateFormat((PKSDATAFORMAT)NewDataFormat);
                        m_Format = NewDataFormat;
                        IoStatusBlock->Information = DataFormat->FormatSize;
                        IoStatusBlock->Status = STATUS_SUCCESS;
                        return STATUS_SUCCESS;
                    }
                }
                DPRINT1("Failed to set format\n");
                IoStatusBlock->Information = 0;
                IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
                return STATUS_UNSUCCESSFUL;
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                if (!m_Format)
                {
                    DPRINT1("No format\n");
                    IoStatusBlock->Information = 0;
                    IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
                    return STATUS_UNSUCCESSFUL;
                }
                if (m_Format->FormatSize > OutputBufferLength)
                {
                    IoStatusBlock->Information = m_Format->FormatSize;
                    IoStatusBlock->Status = STATUS_BUFFER_TOO_SMALL;
                    return STATUS_BUFFER_TOO_SMALL;
                }

                RtlMoveMemory(DataFormat, m_Format, m_Format->FormatSize);
                IoStatusBlock->Information = DataFormat->FormatSize;
                IoStatusBlock->Status = STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
        }
        else if (Property->Id == KSPROPERTY_CONNECTION_ALLOCATORFRAMING)
        {
            PKSALLOCATOR_FRAMING Framing = (PKSALLOCATOR_FRAMING)OutputBuffer;

            PC_ASSERT_IRQL(DISPATCH_LEVEL);
            // Validate input buffer
            if (OutputBufferLength < sizeof(KSALLOCATOR_FRAMING))
            {
                IoStatusBlock->Information = sizeof(KSALLOCATOR_FRAMING);
                IoStatusBlock->Status = STATUS_BUFFER_TOO_SMALL;
                return STATUS_BUFFER_TOO_SMALL;
            }
            // copy frame allocator struct
            RtlMoveMemory(Framing, &m_AllocatorFraming, sizeof(KSALLOCATOR_FRAMING));

            IoStatusBlock->Information = sizeof(KSALLOCATOR_FRAMING);
            IoStatusBlock->Status = STATUS_SUCCESS;
            return STATUS_SUCCESS;
        }
    }

    RtlStringFromGUID(Property->Set, &GuidString);
    DPRINT1("Unhandeled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    RtlFreeUnicodeString(&GuidString);

    IoStatusBlock->Status = STATUS_NOT_IMPLEMENTED;
    IoStatusBlock->Information = 0;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
CPortPinWavePci::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
       Status = HandleKsProperty(IoStack->Parameters.DeviceIoControl.Type3InputBuffer, IoStack->Parameters.DeviceIoControl.InputBufferLength, Irp->UserBuffer, IoStack->Parameters.DeviceIoControl.OutputBufferLength, &Irp->IoStatus);
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return Status;
    }


    UNIMPLEMENTED

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

VOID
NTAPI
CPortPinWavePci::CloseStream()
{
    PMINIPORTWAVEPCISTREAM Stream;
    ISubdevice *ISubDevice;
    NTSTATUS Status;
    PSUBDEVICE_DESCRIPTOR Descriptor;

    if (m_Stream)
    {
        if (m_State != KSSTATE_STOP)
        {
            m_Stream->SetState(KSSTATE_STOP);
        }
    }

    if (m_ServiceGroup)
    {
        m_ServiceGroup->RemoveMember(PSERVICESINK(this));
    }

    Status = m_Port->QueryInterface(IID_ISubdevice, (PVOID*)&ISubDevice);
    if (NT_SUCCESS(Status))
    {
        Status = ISubDevice->GetDescriptor(&Descriptor);
        if (NT_SUCCESS(Status))
        {
            Descriptor->Factory.Instances[m_ConnectDetails->PinId].CurrentPinInstanceCount--;
        }
        ISubDevice->Release();
    }

    if (m_Format)
    {
        ExFreePool(m_Format);
        m_Format = NULL;
    }

    if (m_Stream)
    {
        Stream = m_Stream;
        m_Stream = 0;
        DPRINT1("Closing stream at Irql %u\n", KeGetCurrentIrql());
        Stream->Release();
    }
}

VOID
NTAPI
PinWavePciCloseStreamRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID Context)
{
    CPortPinWavePci * This;
    PCLOSESTREAM_CONTEXT Ctx = (PCLOSESTREAM_CONTEXT)Context;

    This = (CPortPinWavePci*)Ctx->Pin;

    This->CloseStream();

    // complete the irp
    Ctx->Irp->IoStatus.Information = 0;
    Ctx->Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Ctx->Irp, IO_NO_INCREMENT);

    // free the work item
    IoFreeWorkItem(Ctx->WorkItem);

    // free work item ctx
    FreeItem(Ctx, TAG_PORTCLASS);
}

NTSTATUS
NTAPI
CPortPinWavePci::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCLOSESTREAM_CONTEXT Ctx;

    if (m_Stream)
    {
        Ctx = (PCLOSESTREAM_CONTEXT)AllocateItem(NonPagedPool, sizeof(CLOSESTREAM_CONTEXT), TAG_PORTCLASS);
        if (!Ctx)
        {
            DPRINT1("Failed to allocate stream context\n");
            goto cleanup;
        }

        Ctx->WorkItem = IoAllocateWorkItem(DeviceObject);
        if (!Ctx->WorkItem)
        {
            DPRINT1("Failed to allocate work item\n");
            goto cleanup;
        }

        Ctx->Irp = Irp;
        Ctx->Pin = (PVOID)this;

        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_PENDING;

        // defer work item
        IoQueueWorkItem(Ctx->WorkItem, PinWavePciCloseStreamRoutine, DelayedWorkQueue, (PVOID)Ctx);
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
    BOOL Capture;

    Port->AddRef();
    Filter->AddRef();

    m_Port = Port;
    m_Filter = Filter;
    m_KsPinDescriptor = KsPinDescriptor;
    m_ConnectDetails = ConnectDetails;
    m_Miniport = GetWavePciMiniport(Port);
    m_DeviceObject = DeviceObject;

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("IPortPinWavePci_fnInit entered\n");

    m_Format = (PKSDATAFORMAT)ExAllocatePoolWithTag(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!m_Format)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlMoveMemory(m_Format, DataFormat, DataFormat->FormatSize);

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
                                   PPORTWAVEPCISTREAM(this),
                                   ConnectDetails->PinId,
                                   Capture,
                                   m_Format,
                                   &m_DmaChannel,
                                   &m_ServiceGroup);

    DPRINT("IPortPinWavePci_fnInit Status %x\n", Status);

    if (!NT_SUCCESS(Status))
        return Status;

    if (m_ServiceGroup)
    {
        Status = m_ServiceGroup->AddMember(PSERVICESINK(this));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to add pin to service group\n");
            return Status;
        }
        m_ServiceGroup->SupportDelayedService();
    }

    // delay of 10 milisec
    m_Delay = Int32x32To64(10, -10000);

    Status = m_Stream->GetAllocatorFraming(&m_AllocatorFraming);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetAllocatorFraming failed with %x\n", Status);
        return Status;
    }

    DPRINT("OptionFlags %x RequirementsFlag %x PoolType %x Frames %lu FrameSize %lu FileAlignment %lu\n",
           m_AllocatorFraming.OptionsFlags, m_AllocatorFraming.RequirementsFlags, m_AllocatorFraming.PoolType, m_AllocatorFraming.Frames, m_AllocatorFraming.FrameSize, m_AllocatorFraming.FileAlignment);

    Status = NewIrpQueue(&m_IrpQueue);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = m_IrpQueue->Init(ConnectDetails, m_Format, DeviceObject, m_AllocatorFraming.FrameSize, m_AllocatorFraming.FileAlignment, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IrpQueue_Init failed with %x\n", Status);
        return Status;
    }

    m_State = KSSTATE_STOP;
    m_Capture = Capture;

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

