/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_dmus.cpp
 * PURPOSE:         DMus IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

class CPortPinDMus : public IPortPinDMus
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
    IMP_IPortPinDMus;
    IMP_IServiceSink;
    IMP_IMasterClock;
    IMP_IAllocatorMXF;

     CPortPinDMus(IUnknown * OuterUnknown){}
     virtual ~CPortPinDMus(){}

protected:
    VOID TransferMidiDataToDMus();
    VOID TransferMidiData();

    VOID NTAPI SetStreamState(IN KSSTATE State);


    IPortDMus * m_Port;
    IPortFilterDMus * m_Filter;
    KSPIN_DESCRIPTOR * m_KsPinDescriptor;
    PMINIPORTDMUS m_Miniport;

    PSERVICEGROUP m_ServiceGroup;

    PMXF m_Mxf;
    ULONGLONG m_SchedulePreFetch;
    NPAGED_LOOKASIDE_LIST m_LookAsideEvent;
    NPAGED_LOOKASIDE_LIST m_LookAsideBuffer;

    PMINIPORTMIDI m_MidiMiniport;
    PMINIPORTMIDISTREAM m_MidiStream;


    KSSTATE m_State;
    PKSDATAFORMAT m_Format;
    KSPIN_CONNECT * m_ConnectDetails;

    DMUS_STREAM_TYPE m_Capture;
    PDEVICE_OBJECT m_DeviceObject;
    IIrpQueue * m_IrpQueue;

    ULONG m_TotalPackets;
    ULONG m_PreCompleted;
    ULONG m_PostCompleted;

    ULONG m_LastTag;

    LONG m_Ref;

    friend VOID NTAPI SetStreamWorkerRoutineDMus(IN PDEVICE_OBJECT  DeviceObject, IN PVOID  Context);
    friend VOID NTAPI CloseStreamRoutineDMus(IN PDEVICE_OBJECT  DeviceObject, IN PVOID Context);

};

typedef struct
{
    DMUS_KERNEL_EVENT Event;
    PVOID Tag;
}DMUS_KERNEL_EVENT_WITH_TAG, *PDMUS_KERNEL_EVENT_WITH_TAG;

typedef struct
{
    CPortPinDMus *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

//==================================================================================================================================
NTSTATUS
NTAPI
CPortPinDMus::GetTime(OUT REFERENCE_TIME  *prtTime)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}

//==================================================================================================================================
NTSTATUS
NTAPI
CPortPinDMus::GetMessage(
    OUT PDMUS_KERNEL_EVENT * ppDMKEvt)
{
    PVOID Buffer;

    Buffer = ExAllocateFromNPagedLookasideList(&m_LookAsideEvent);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    *ppDMKEvt = (PDMUS_KERNEL_EVENT)Buffer;
    RtlZeroMemory(Buffer, sizeof(DMUS_KERNEL_EVENT));
    return STATUS_SUCCESS;
}

USHORT
NTAPI
CPortPinDMus::GetBufferSize()
{
    return PAGE_SIZE;
}

NTSTATUS
NTAPI
CPortPinDMus::GetBuffer(
    OUT PBYTE * ppBuffer)
{
    PVOID Buffer;

    Buffer = ExAllocateFromNPagedLookasideList(&m_LookAsideBuffer);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    *ppBuffer = (PBYTE)Buffer;
    RtlZeroMemory(Buffer, PAGE_SIZE);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CPortPinDMus::PutBuffer(
    IN PBYTE pBuffer)
{
    PDMUS_KERNEL_EVENT_WITH_TAG Event = (PDMUS_KERNEL_EVENT_WITH_TAG)pBuffer;

    m_IrpQueue->ReleaseMappingWithTag(Event->Tag);

    ExFreeToNPagedLookasideList(&m_LookAsideBuffer, pBuffer);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortPinDMus::SetState(
    IN KSSTATE State)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
CPortPinDMus::PutMessage(
    IN PDMUS_KERNEL_EVENT pDMKEvt)
{
    ExFreeToNPagedLookasideList(&m_LookAsideEvent, pDMKEvt);
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CPortPinDMus::ConnectOutput(
    IN PMXF sinkMXF)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS
NTAPI
CPortPinDMus::DisconnectOutput(
    IN PMXF sinkMXF)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

//==================================================================================================================================

VOID
NTAPI
SetStreamWorkerRoutineDMus(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    CPortPinDMus* This;
    KSSTATE State;
    NTSTATUS Status;
    PSETSTREAM_CONTEXT Ctx = (PSETSTREAM_CONTEXT)Context;

    This = Ctx->Pin;
    State = Ctx->State;

    IoFreeWorkItem(Ctx->WorkItem);
    FreeItem(Ctx, TAG_PORTCLASS);

    // Has the audio stream resumed?
    if (This->m_IrpQueue->NumMappings() && State == KSSTATE_STOP)
        return;

    // Set the state
    if (This->m_MidiStream)
    {
        Status = This->m_MidiStream->SetState(State);
    }
    else
    {
        Status = This->m_Mxf->SetState(State);
    }

    if (NT_SUCCESS(Status))
    {
        // Set internal state to requested state
        This->m_State = State;

        if (This->m_State == KSSTATE_STOP)
        {
            // reset start stream
            This->m_IrpQueue->CancelBuffers(); //FIX function name
            DPRINT1("Stopping PreCompleted %u PostCompleted %u\n", This->m_PreCompleted, This->m_PostCompleted);
        }
    }
}

VOID
NTAPI
CPortPinDMus::SetStreamState(
   IN KSSTATE State)
{
    PIO_WORKITEM WorkItem;
    PSETSTREAM_CONTEXT Context;

    PC_ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    // Has the audio stream resumed?
    if (m_IrpQueue->NumMappings() && State == KSSTATE_STOP)
        return;

    // Has the audio state already been set?
    if (m_State == State)
        return;

    // allocate set state context
    Context = (PSETSTREAM_CONTEXT)AllocateItem(NonPagedPool, sizeof(SETSTREAM_CONTEXT), TAG_PORTCLASS);

    if (!Context)
        return;

    // allocate work item
    WorkItem = IoAllocateWorkItem(m_DeviceObject);

    if (!WorkItem)
    {
        ExFreePool(Context);
        return;
    }

    Context->Pin = this;
    Context->WorkItem = WorkItem;
    Context->State = State;

    // queue the work item
    IoQueueWorkItem(WorkItem, SetStreamWorkerRoutineDMus, DelayedWorkQueue, (PVOID)Context);
}

VOID
CPortPinDMus::TransferMidiData()
{
    NTSTATUS Status;
    PUCHAR Buffer;
    ULONG BufferSize;
    ULONG BytesWritten;

    do
    {
        Status = m_IrpQueue->GetMapping(&Buffer, &BufferSize);
        if (!NT_SUCCESS(Status))
        {
            SetStreamState(KSSTATE_STOP);
            return;
        }

        if (m_Capture)
        {
            Status = m_MidiStream->Read(Buffer, BufferSize, &BytesWritten);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("Read failed with %x\n", Status);
                return;
            }
        }
        else
        {
            Status = m_MidiStream->Write(Buffer, BufferSize, &BytesWritten);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("Write failed with %x\n", Status);
                return;
            }
        }

        if (!BytesWritten)
        {
            DPRINT("Device is busy retry later\n");
            return;
        }

        m_IrpQueue->UpdateMapping(BytesWritten);

    }while(TRUE);

}

VOID
CPortPinDMus::TransferMidiDataToDMus()
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS  PhysicalAddress;
    ULONG BufferSize, Flags;
    PVOID Buffer;
    PDMUS_KERNEL_EVENT_WITH_TAG Event, LastEvent = NULL, Root = NULL;

    do
    {
        m_LastTag++;
        Status = m_IrpQueue->GetMappingWithTag(UlongToPtr(m_LastTag), &PhysicalAddress, &Buffer, &BufferSize, &Flags);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = GetMessage((PDMUS_KERNEL_EVENT*)&Event);
        if (!NT_SUCCESS(Status))
            break;

        //FIXME
        //set up struct
        //Event->Event.usFlags = DMUS_KEF_EVENT_COMPLETE;
        Event->Event.cbStruct = sizeof(DMUS_KERNEL_EVENT);
        Event->Event.cbEvent = BufferSize;
        Event->Event.uData.pbData = (PBYTE)Buffer;


        if (!Root)
            Root = Event;
        else
            LastEvent->Event.pNextEvt = (struct _DMUS_KERNEL_EVENT *)Event;

        LastEvent = Event;
        LastEvent->Event.pNextEvt = NULL;
        LastEvent->Tag = UlongToPtr(m_LastTag);

    }while(TRUE);

    if (!Root)
    {
        SetStreamState(KSSTATE_STOP);
        return;
    }

    Status = m_Mxf->PutMessage((PDMUS_KERNEL_EVENT)Root);
    DPRINT("Status %x\n", Status);
}



VOID
NTAPI
CPortPinDMus::RequestService()
{
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    if (m_MidiStream)
    {
        TransferMidiData();
    }
    else if (m_Mxf)
    {
        TransferMidiDataToDMus();
    }
}

//==================================================================================================================================
NTSTATUS
NTAPI
CPortPinDMus::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{

    if (IsEqualGUIDAligned(refiid, IID_IIrpTarget) ||
        IsEqualGUIDAligned(refiid, IID_IUnknown))
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinDMus::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
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
CPortPinDMus::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortPinDMus::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinDMus::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinDMus::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}


VOID
NTAPI
CloseStreamRoutineDMus(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID Context)
{
    PMINIPORTMIDISTREAM Stream = NULL;
    NTSTATUS Status;
    ISubdevice *ISubDevice;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    CPortPinDMus * This;
    PCLOSESTREAM_CONTEXT Ctx = (PCLOSESTREAM_CONTEXT)Context;

    This = (CPortPinDMus*)Ctx->Pin;

    if (This->m_MidiStream)
    {
        if (This->m_State != KSSTATE_STOP)
        {
            This->m_MidiStream->SetState(KSSTATE_STOP);
        }
        Stream = This->m_MidiStream;
        This->m_MidiStream = NULL;
    }

    if (This->m_ServiceGroup)
    {
        This->m_ServiceGroup->RemoveMember(PSERVICESINK(This));
    }

    Status = This->m_Port->QueryInterface(IID_ISubdevice, (PVOID*)&ISubDevice);
    if (NT_SUCCESS(Status))
    {
        Status = ISubDevice->GetDescriptor(&Descriptor);
        if (NT_SUCCESS(Status))
        {
            Descriptor->Factory.Instances[This->m_ConnectDetails->PinId].CurrentPinInstanceCount--;
            ISubDevice->Release();
        }
    }

    if (This->m_Format)
    {
        ExFreePool(This->m_Format);
        This->m_Format = NULL;
    }

    // complete the irp
    Ctx->Irp->IoStatus.Information = 0;
    Ctx->Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Ctx->Irp, IO_NO_INCREMENT);

    // free the work item
    IoFreeWorkItem(Ctx->WorkItem);

    // free work item ctx
    FreeItem(Ctx, TAG_PORTCLASS);

    // destroy DMus pin
    This->m_Filter->FreePin(PPORTPINDMUS(This));

    if (Stream)
    {
        DPRINT1("Closing stream at Irql %u\n", KeGetCurrentIrql());
        Stream->Release();
    }
}

NTSTATUS
NTAPI
CPortPinDMus::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCLOSESTREAM_CONTEXT Ctx;

    if (m_MidiStream || m_Mxf)
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
        Ctx->Pin = this;

        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_PENDING;

        // defer work item
        IoQueueWorkItem(Ctx->WorkItem, CloseStreamRoutineDMus, DelayedWorkQueue, (PVOID)Ctx);
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
CPortPinDMus::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortPinDMus::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortPinDMus::FastDeviceIoControl(
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
CPortPinDMus::FastRead(
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
CPortPinDMus::FastWrite(
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
CPortPinDMus::Init(
    IN PPORTDMUS Port,
    IN PPORTFILTERDMUS Filter,
    IN KSPIN_CONNECT * ConnectDetails,
    IN KSPIN_DESCRIPTOR * KsPinDescriptor,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PKSDATAFORMAT DataFormat;
    DMUS_STREAM_TYPE Type;

    Port->AddRef();
    Filter->AddRef();

    m_Port = Port;
    m_Filter = Filter;
    m_KsPinDescriptor = KsPinDescriptor;
    m_ConnectDetails = ConnectDetails;
    m_DeviceObject = DeviceObject;

    GetDMusMiniport(Port, &m_Miniport, &m_MidiMiniport);

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("CPortPinDMus::Init entered\n");

    m_Format = (PKSDATAFORMAT)AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!m_Format)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlMoveMemory(m_Format, DataFormat, DataFormat->FormatSize);

    if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_IN)
    {
        Type = DMUS_STREAM_MIDI_RENDER;
    }
    else if (KsPinDescriptor->Communication == KSPIN_COMMUNICATION_SINK && KsPinDescriptor->DataFlow == KSPIN_DATAFLOW_OUT)
    {
        Type = DMUS_STREAM_MIDI_CAPTURE;
    }
    else
    {
        DPRINT1("Unexpected Communication %u DataFlow %u\n", KsPinDescriptor->Communication, KsPinDescriptor->DataFlow);
        KeBugCheck(0);
    }

    Status = NewIrpQueue(&m_IrpQueue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate IrpQueue with %x\n", Status);
        return Status;
    }

    if (m_MidiMiniport)
    {
        Status = m_MidiMiniport->NewStream(&m_MidiStream, NULL, NonPagedPool, ConnectDetails->PinId, Type, m_Format, &m_ServiceGroup);

        DPRINT("CPortPinDMus::Init Status %x\n", Status);

        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        Status = m_Miniport->NewStream(&m_Mxf, NULL, NonPagedPool, ConnectDetails->PinId, Type, m_Format, &m_ServiceGroup, PAllocatorMXF(this), PMASTERCLOCK(this),&m_SchedulePreFetch);

        DPRINT("CPortPinDMus::Init Status %x\n", Status);

        if (!NT_SUCCESS(Status))
            return Status;

        if (Type == DMUS_STREAM_MIDI_CAPTURE)
        {
            Status = m_Mxf->ConnectOutput(PMXF(this));
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IMXF_ConnectOutput failed with Status %x\n", Status);
                return Status;
            }
        }

        ExInitializeNPagedLookasideList(&m_LookAsideEvent, NULL, NULL, 0, sizeof(DMUS_KERNEL_EVENT_WITH_TAG), TAG_PORTCLASS, 0);
        ExInitializeNPagedLookasideList(&m_LookAsideBuffer, NULL, NULL, 0, PAGE_SIZE, TAG_PORTCLASS, 0);
    }

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

    Status = m_IrpQueue->Init(ConnectDetails, m_Format, DeviceObject, 0, 0, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IrpQueue_Init failed with %x\n", Status);
        return Status;
    }

    m_State = KSSTATE_STOP;
    m_Capture = Type;

    return STATUS_SUCCESS;
}

VOID
NTAPI
CPortPinDMus::Notify()
{
    m_ServiceGroup->RequestService();
}

NTSTATUS
NewPortPinDMus(
    OUT IPortPinDMus ** OutPin)
{
    CPortPinDMus * This;

    This = new (NonPagedPool, TAG_PORTCLASS)CPortPinDMus(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // store result
    *OutPin = (IPortPinDMus*)This;

    return STATUS_SUCCESS;
}

