/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_dmus.c
 * PURPOSE:         DMus IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

#include "private.h"

typedef struct
{
    IPortPinDMusVtbl *lpVtbl;
    IServiceSinkVtbl *lpVtblServiceSink;
    IMasterClockVtbl *lpVtblMasterClock;
    IAllocatorMXFVtbl *lpVtblAllocatorMXF;
    IMXFVtbl *lpVtblMXF;

    LONG ref;
    IPortDMus * Port;
    IPortFilterDMus * Filter;
    KSPIN_DESCRIPTOR * KsPinDescriptor;
    PMINIPORTDMUS Miniport;

    PSERVICEGROUP ServiceGroup;

    PMXF Mxf;
    ULONGLONG SchedulePreFetch;
    NPAGED_LOOKASIDE_LIST LookAsideEvent;
    NPAGED_LOOKASIDE_LIST LookAsideBuffer;

    PMINIPORTMIDI MidiMiniport;
    PMINIPORTMIDISTREAM MidiStream;


    KSSTATE State;
    PKSDATAFORMAT Format;
    KSPIN_CONNECT * ConnectDetails;

    BOOL Capture;
    PDEVICE_OBJECT DeviceObject;
    IIrpQueue * IrpQueue;

    ULONG TotalPackets;
    ULONG PreCompleted;
    ULONG PostCompleted;

    ULONG LastTag;

}IPortPinDMusImpl;

typedef struct
{
    DMUS_KERNEL_EVENT Event;
    PVOID Tag;
}DMUS_KERNEL_EVENT_WITH_TAG, *PDMUS_KERNEL_EVENT_WITH_TAG;

typedef struct
{
    IPortPinDMusImpl *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

//==================================================================================================================================


static
NTSTATUS
NTAPI
IMasterClock_fnQueryInterface(
    IMasterClock* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblMasterClock);

    DPRINT("IServiceSink_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtblServiceSink;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IMasterClock_fnAddRef(
    IMasterClock* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblMasterClock);
    DPRINT("IServiceSink_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IMasterClock_fnRelease(
    IMasterClock* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblMasterClock);

    InterlockedDecrement(&This->ref);

    DPRINT("IServiceSink_fnRelease entered %u\n", This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

static
NTSTATUS
NTAPI
IMasterClock_fnGetTime(
    IMasterClock* iface,
    OUT REFERENCE_TIME  *prtTime)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}

static IMasterClockVtbl vt_IMasterClock = 
{
    IMasterClock_fnQueryInterface,
    IMasterClock_fnAddRef,
    IMasterClock_fnRelease,
    IMasterClock_fnGetTime
};

//==================================================================================================================================


static
NTSTATUS
NTAPI
IAllocatorMXF_fnQueryInterface(
    IAllocatorMXF* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblAllocatorMXF);

    DPRINT("IServiceSink_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IAllocatorMXF) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtblServiceSink;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IAllocatorMXF_fnAddRef(
    IAllocatorMXF* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblAllocatorMXF);
    DPRINT("IServiceSink_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IAllocatorMXF_fnRelease(
    IAllocatorMXF* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblAllocatorMXF);

    InterlockedDecrement(&This->ref);

    DPRINT("IServiceSink_fnRelease entered %u\n", This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

static
NTSTATUS
NTAPI
IAllocatorMXF_fnGetMessage(
    IAllocatorMXF* iface,
    OUT PDMUS_KERNEL_EVENT * ppDMKEvt)
{
    PVOID Buffer;
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblAllocatorMXF);

    Buffer = ExAllocateFromNPagedLookasideList(&This->LookAsideEvent);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    *ppDMKEvt = Buffer;
    RtlZeroMemory(Buffer, sizeof(DMUS_KERNEL_EVENT));
    return STATUS_SUCCESS;
}

static
USHORT
NTAPI
IAllocatorMXF_fnGetBufferSize(
    IAllocatorMXF* iface)
{
    return PAGE_SIZE;
}

static
NTSTATUS
NTAPI
IAllocatorMXF_fnGetBuffer(
    IAllocatorMXF* iface,
    OUT PBYTE * ppBuffer)
{
    PVOID Buffer;
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblAllocatorMXF);

    Buffer = ExAllocateFromNPagedLookasideList(&This->LookAsideBuffer);
    if (!Buffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    *ppBuffer = Buffer;
    RtlZeroMemory(Buffer, PAGE_SIZE);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IAllocatorMXF_fnPutBuffer(
    IAllocatorMXF* iface,
    IN PBYTE pBuffer)
{
    PDMUS_KERNEL_EVENT_WITH_TAG Event = (PDMUS_KERNEL_EVENT_WITH_TAG)pBuffer;
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblAllocatorMXF);

    This->IrpQueue->lpVtbl->ReleaseMappingWithTag(This->IrpQueue, Event->Tag);

    ExFreeToNPagedLookasideList(&This->LookAsideBuffer, pBuffer);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IAllocatorMXF_fnSetState(
    IAllocatorMXF* iface,
    IN KSSTATE State)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
NTAPI
IAllocatorMXF_fnPutMessage(
    IAllocatorMXF* iface,
    IN PDMUS_KERNEL_EVENT pDMKEvt)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblAllocatorMXF);

    ExFreeToNPagedLookasideList(&This->LookAsideEvent, pDMKEvt);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IAllocatorMXF_fnConnectOutput(
    IAllocatorMXF* iface,
    IN PMXF sinkMXF)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
NTAPI
IAllocatorMXF_fnDisconnectOutput(
    IAllocatorMXF* iface,
    IN PMXF sinkMXF)
{
    UNIMPLEMENTED
    return STATUS_NOT_IMPLEMENTED;
}

static IAllocatorMXFVtbl vt_IAllocatorMXF =
{
    IAllocatorMXF_fnQueryInterface,
    IAllocatorMXF_fnAddRef,
    IAllocatorMXF_fnRelease,
    IAllocatorMXF_fnSetState,
    IAllocatorMXF_fnPutMessage,
    IAllocatorMXF_fnConnectOutput,
    IAllocatorMXF_fnDisconnectOutput,
    IAllocatorMXF_fnGetMessage,
    IAllocatorMXF_fnGetBufferSize,
    IAllocatorMXF_fnGetBuffer,
    IAllocatorMXF_fnPutBuffer
};

//==================================================================================================================================

static
NTSTATUS
NTAPI
IServiceSink_fnQueryInterface(
    IServiceSink* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblServiceSink);

    DPRINT("IServiceSink_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IServiceSink) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtblServiceSink;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IServiceSink_fnAddRef(
    IServiceSink* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblServiceSink);
    DPRINT("IServiceSink_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IServiceSink_fnRelease(
    IServiceSink* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblServiceSink);

    InterlockedDecrement(&This->ref);

    DPRINT("IServiceSink_fnRelease entered %u\n", This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}


static
VOID
NTAPI
SetStreamWorkerRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    IPortPinDMusImpl * This;
    KSSTATE State;
    NTSTATUS Status;
    PSETSTREAM_CONTEXT Ctx = (PSETSTREAM_CONTEXT)Context;

    This = Ctx->Pin;
    State = Ctx->State;

    IoFreeWorkItem(Ctx->WorkItem);
    FreeItem(Ctx, TAG_PORTCLASS);

    /* Has the audio stream resumed? */
    if (This->IrpQueue->lpVtbl->NumMappings(This->IrpQueue) && State == KSSTATE_STOP)
        return;

    /* Set the state */
    if (This->MidiStream)
    {
        Status = This->MidiStream->lpVtbl->SetState(This->MidiStream, State);
    }
    else
    {
        Status = This->Mxf->lpVtbl->SetState(This->Mxf, State);
    }

    if (NT_SUCCESS(Status))
    {
        /* Set internal state to requested state */
        This->State = State;

        if (This->State == KSSTATE_STOP)
        {
            /* reset start stream */
            This->IrpQueue->lpVtbl->CancelBuffers(This->IrpQueue); //FIX function name
            DPRINT1("Stopping PreCompleted %u PostCompleted %u\n", This->PreCompleted, This->PostCompleted);
        }
    }
}
static
VOID
NTAPI
SetStreamState(
   IN IPortPinDMusImpl * This,
   IN KSSTATE State)
{
    PIO_WORKITEM WorkItem;
    PSETSTREAM_CONTEXT Context;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    /* Has the audio stream resumed? */
    if (This->IrpQueue->lpVtbl->NumMappings(This->IrpQueue) && State == KSSTATE_STOP)
        return;

    /* Has the audio state already been set? */
    if (This->State == State)
        return;

    /* allocate set state context */
    Context = AllocateItem(NonPagedPool, sizeof(SETSTREAM_CONTEXT), TAG_PORTCLASS);

    if (!Context)
        return;

    /* allocate work item */
    WorkItem = IoAllocateWorkItem(This->DeviceObject);

    if (!WorkItem)
    {
        ExFreePool(Context);
        return;
    }

    Context->Pin = (PVOID)This;
    Context->WorkItem = WorkItem;
    Context->State = State;

    /* queue the work item */
    IoQueueWorkItem(WorkItem, SetStreamWorkerRoutine, DelayedWorkQueue, (PVOID)Context);
}

VOID
TransferMidiData(
    IPortPinDMusImpl * This)
{
    NTSTATUS Status;
    PUCHAR Buffer;
    ULONG BufferSize;
    ULONG BytesWritten;

    do
    {
        Status = This->IrpQueue->lpVtbl->GetMapping(This->IrpQueue, &Buffer, &BufferSize);
        if (!NT_SUCCESS(Status))
        {
            SetStreamState(This, KSSTATE_STOP);
            return;
        }

        if (This->Capture)
        {
            Status = This->MidiStream->lpVtbl->Read(This->MidiStream, Buffer, BufferSize, &BytesWritten);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("Read failed with %x\n", Status);
                return;
            }
        }
        else
        {
            Status = This->MidiStream->lpVtbl->Write(This->MidiStream, Buffer, BufferSize, &BytesWritten);
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

        This->IrpQueue->lpVtbl->UpdateMapping(This->IrpQueue, BytesWritten);

    }while(TRUE);

}

VOID
TransferMidiDataToDMus(
    IPortPinDMusImpl * This)
{
    NTSTATUS Status;
    PHYSICAL_ADDRESS  PhysicalAddress;
    ULONG BufferSize, Flags;
    PVOID Buffer;
    PDMUS_KERNEL_EVENT_WITH_TAG Event, LastEvent = NULL, Root = NULL;

    do
    {
        This->LastTag++;
        Status = This->IrpQueue->lpVtbl->GetMappingWithTag(This->IrpQueue, UlongToPtr(This->LastTag), &PhysicalAddress, &Buffer, &BufferSize, &Flags);
        if (!NT_SUCCESS(Status))
        {
            break;
        }

        Status = IAllocatorMXF_fnGetMessage((IAllocatorMXF*)&This->lpVtblAllocatorMXF, (PDMUS_KERNEL_EVENT*)&Event);
        if (!NT_SUCCESS(Status))
            break;

        //FIXME
        //set up struct
        //Event->Event.usFlags = DMUS_KEF_EVENT_COMPLETE;
        Event->Event.cbStruct = sizeof(DMUS_KERNEL_EVENT);
        Event->Event.cbEvent = BufferSize;
        Event->Event.uData.pbData = Buffer;


        if (!Root)
            Root = Event;
        else
            LastEvent->Event.pNextEvt = (struct _DMUS_KERNEL_EVENT *)Event;

        LastEvent = Event;
        LastEvent->Event.pNextEvt = NULL;
        LastEvent->Tag = UlongToPtr(This->LastTag);

    }while(TRUE);

    if (!Root)
    {
        SetStreamState(This, KSSTATE_STOP);
        return;
    }

    Status = This->Mxf->lpVtbl->PutMessage(This->Mxf, (PDMUS_KERNEL_EVENT)Root);
    DPRINT("Status %x\n", Status);
}


static
VOID
NTAPI
IServiceSink_fnRequestService(
    IServiceSink* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)CONTAINING_RECORD(iface, IPortPinDMusImpl, lpVtblServiceSink);

    ASSERT_IRQL(DISPATCH_LEVEL);

    if (This->MidiStream)
    {
        TransferMidiData(This);
    }
    else if (This->Mxf)
    {
        TransferMidiDataToDMus(This);
    }
}

static IServiceSinkVtbl vt_IServiceSink = 
{
    IServiceSink_fnQueryInterface,
    IServiceSink_fnAddRef,
    IServiceSink_fnRelease,
    IServiceSink_fnRequestService
};

//==================================================================================================================================
/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnQueryInterface(
    IPortPinDMus* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IIrpTarget) || 
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortPinDMus_fnAddRef(
    IPortPinDMus* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortPinDMus_fnRelease(
    IPortPinDMus* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    return This->ref;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnNewIrpTarget(
    IN IPortPinDMus* iface,
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

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnDeviceIoControl(
    IN IPortPinDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    UNIMPLEMENTED

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnRead(
    IN IPortPinDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnWrite(
    IN IPortPinDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnFlush(
    IN IPortPinDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

static
VOID
NTAPI
CloseStreamRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID Context)
{
    PMINIPORTMIDISTREAM Stream = NULL;
    NTSTATUS Status;
    ISubdevice *ISubDevice;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    IPortPinDMusImpl * This;
    PCLOSESTREAM_CONTEXT Ctx = (PCLOSESTREAM_CONTEXT)Context;

    This = (IPortPinDMusImpl*)Ctx->Pin;

    if (This->MidiStream)
    {
        if (This->State != KSSTATE_STOP)
        {
            This->MidiStream->lpVtbl->SetState(This->MidiStream, KSSTATE_STOP);
            KeStallExecutionProcessor(10);
        }
        Stream = This->MidiStream;
        This->MidiStream = NULL;
    }

    if (This->ServiceGroup)
    {
        This->ServiceGroup->lpVtbl->RemoveMember(This->ServiceGroup, (PSERVICESINK)&This->lpVtblServiceSink);
        This->ServiceGroup->lpVtbl->Release(This->ServiceGroup);
    }

    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&ISubDevice);
    if (NT_SUCCESS(Status))
    {
        Status = ISubDevice->lpVtbl->GetDescriptor(ISubDevice, &Descriptor);
        if (NT_SUCCESS(Status))
        {
            ISubDevice->lpVtbl->Release(ISubDevice);
            Descriptor->Factory.Instances[This->ConnectDetails->PinId].CurrentPinInstanceCount--;
        }
    }

    if (This->Format)
    {
        ExFreePool(This->Format);
        This->Format = NULL;
    }

    /* complete the irp */
    Ctx->Irp->IoStatus.Information = 0;
    Ctx->Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Ctx->Irp, IO_NO_INCREMENT);

    /* free the work item */
    IoFreeWorkItem(Ctx->WorkItem);

    /* free work item ctx */
    FreeItem(Ctx, TAG_PORTCLASS);

    /* destroy DMus pin */
    This->Filter->lpVtbl->FreePin(This->Filter, (PPORTPINDMUS)This);

    if (Stream)
    {
        DPRINT1("Closing stream at Irql %u\n", KeGetCurrentIrql());
        Stream->lpVtbl->Release(Stream);
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnClose(
    IN IPortPinDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCLOSESTREAM_CONTEXT Ctx;
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    if (This->MidiStream || This->Mxf)
    {
        Ctx = AllocateItem(NonPagedPool, sizeof(CLOSESTREAM_CONTEXT), TAG_PORTCLASS);
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
        Ctx->Pin = (PVOID)This;

        IoMarkIrpPending(Irp);
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_PENDING;

        /* defer work item */
        IoQueueWorkItem(Ctx->WorkItem, CloseStreamRoutine, DelayedWorkQueue, (PVOID)Ctx);
        /* Return result */
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnQuerySecurity(
    IN IPortPinDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnSetSecurity(
    IN IPortPinDMus* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortPinDMus_fnFastDeviceIoControl(
    IN IPortPinDMus* iface,
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
    UNIMPLEMENTED
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortPinDMus_fnFastRead(
    IN IPortPinDMus* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PCONTEXT_WRITE Packet;
    PIRP Irp;
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    DPRINT("IPortPinDMus_fnFastRead entered\n");

    Packet = (PCONTEXT_WRITE)Buffer;

    Irp = Packet->Irp;
    StatusBlock->Status = STATUS_PENDING;

    Status = This->IrpQueue->lpVtbl->AddMapping(This->IrpQueue, Buffer, Length, Irp);

    if (!NT_SUCCESS(Status))
        return FALSE;

    if (This->IrpQueue->lpVtbl->MinimumDataAvailable(This->IrpQueue) == TRUE && This->State != KSSTATE_RUN)
    {
        /* some should initiate a state request but didnt do it */
        DPRINT1("Starting stream with %lu mappings\n", This->IrpQueue->lpVtbl->NumMappings(This->IrpQueue));
        SetStreamState(This, KSSTATE_RUN);
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortPinDMus_fnFastWrite(
    IN IPortPinDMus* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PCONTEXT_WRITE Packet;
    PIRP Irp;
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    InterlockedIncrement((PLONG)&This->TotalPackets);

    DPRINT("IPortPinDMus_fnFastWrite entered Total %u Pre %u Post %u\n", This->TotalPackets, This->PreCompleted, This->PostCompleted);

    Packet = (PCONTEXT_WRITE)Buffer;


    if (This->IrpQueue->lpVtbl->MinimumDataAvailable(This->IrpQueue))
    {
        Irp = Packet->Irp;
        StatusBlock->Status = STATUS_PENDING;
        InterlockedIncrement((PLONG)&This->PostCompleted);
    }
    else
    {
        Irp = NULL;
        Packet->Irp->IoStatus.Status = STATUS_SUCCESS;
        Packet->Irp->IoStatus.Information = Packet->Header.FrameExtent;
        IoCompleteRequest(Packet->Irp, IO_SOUND_INCREMENT);
        StatusBlock->Status = STATUS_SUCCESS;
        InterlockedIncrement((PLONG)&This->PreCompleted);
    }

    Status = This->IrpQueue->lpVtbl->AddMapping(This->IrpQueue, Buffer, Length, Irp);

    if (!NT_SUCCESS(Status))
        return FALSE;

    if (This->IrpQueue->lpVtbl->MinimumDataAvailable(This->IrpQueue) == TRUE && This->State != KSSTATE_RUN)
    {
        SetStreamState(This, KSSTATE_RUN);
        /* some should initiate a state request but didnt do it */
        DPRINT1("Starting stream with %lu mappings Status %x\n", This->IrpQueue->lpVtbl->NumMappings(This->IrpQueue), Status);
    }

    return TRUE;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortPinDMus_fnInit(
    IN IPortPinDMus* iface,
    IN PPORTDMUS Port,
    IN PPORTFILTERDMUS Filter,
    IN KSPIN_CONNECT * ConnectDetails,
    IN KSPIN_DESCRIPTOR * KsPinDescriptor,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PKSDATAFORMAT DataFormat;
    BOOL Capture;

    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    Port->lpVtbl->AddRef(Port);
    Filter->lpVtbl->AddRef(Filter);

    This->Port = Port;
    This->Filter = Filter;
    This->KsPinDescriptor = KsPinDescriptor;
    This->ConnectDetails = ConnectDetails;
    This->DeviceObject = DeviceObject;
    GetDMusMiniport(Port, &This->Miniport, &This->MidiMiniport);

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("IPortPinDMus_fnInit entered\n");

    This->Format = ExAllocatePoolWithTag(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!This->Format)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlMoveMemory(This->Format, DataFormat, DataFormat->FormatSize);

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

    Status = NewIrpQueue(&This->IrpQueue);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to allocate IrpQueue with %x\n", Status);
        return Status;
    }

    if (This->MidiMiniport)
    {
        Status = This->MidiMiniport->lpVtbl->NewStream(This->MidiMiniport,
                                                       &This->MidiStream,
                                                       NULL,
                                                       NonPagedPool,
                                                       ConnectDetails->PinId,
                                                       Capture,
                                                       This->Format,
                                                       &This->ServiceGroup);

        DPRINT("IPortPinDMus_fnInit Status %x\n", Status);

        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        Status = This->Miniport->lpVtbl->NewStream(This->Miniport,
                                                   &This->Mxf,
                                                   NULL,
                                                   NonPagedPool,
                                                   ConnectDetails->PinId,
                                                   Capture, //FIXME
                                                   This->Format,
                                                   &This->ServiceGroup,
                                                   (PAllocatorMXF)&This->lpVtblAllocatorMXF,
                                                   (PMASTERCLOCK)&This->lpVtblMasterClock,
                                                   &This->SchedulePreFetch);

        DPRINT("IPortPinDMus_fnInit Status %x\n", Status);

        if (!NT_SUCCESS(Status))
            return Status;

        if (Capture == DMUS_STREAM_MIDI_CAPTURE)
        {
            Status = This->Mxf->lpVtbl->ConnectOutput(This->Mxf, (PMXF)&This->lpVtblMXF);
            if (!NT_SUCCESS(Status))
            {
                DPRINT("IMXF_ConnectOutput failed with Status %x\n", Status);
                return Status;
            }
        }

        ExInitializeNPagedLookasideList(&This->LookAsideEvent, NULL, NULL, 0, sizeof(DMUS_KERNEL_EVENT_WITH_TAG), TAG_PORTCLASS, 0);
        ExInitializeNPagedLookasideList(&This->LookAsideBuffer, NULL, NULL, 0, PAGE_SIZE, TAG_PORTCLASS, 0);
    }

    if (This->ServiceGroup)
    {
        Status = This->ServiceGroup->lpVtbl->AddMember(This->ServiceGroup, 
                                                       (PSERVICESINK)&This->lpVtblServiceSink);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to add pin to service group\n");
            return Status;
        }
        This->ServiceGroup->lpVtbl->SupportDelayedService(This->ServiceGroup);
    }

    Status = This->IrpQueue->lpVtbl->Init(This->IrpQueue, ConnectDetails, This->Format, DeviceObject, 0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IrpQueue_Init failed with %x\n", Status);
        return Status;
    }

    This->State = KSSTATE_STOP;
    This->Capture = Capture;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
IPortPinDMus_fnNotify(
    IN IPortPinDMus* iface)
{
    IPortPinDMusImpl * This = (IPortPinDMusImpl*)iface;

    This->ServiceGroup->lpVtbl->RequestService(This->ServiceGroup);
}

static IPortPinDMusVtbl vt_IPortPinDMus =
{
    IPortPinDMus_fnQueryInterface,
    IPortPinDMus_fnAddRef,
    IPortPinDMus_fnRelease,
    IPortPinDMus_fnNewIrpTarget,
    IPortPinDMus_fnDeviceIoControl,
    IPortPinDMus_fnRead,
    IPortPinDMus_fnWrite,
    IPortPinDMus_fnFlush,
    IPortPinDMus_fnClose,
    IPortPinDMus_fnQuerySecurity,
    IPortPinDMus_fnSetSecurity,
    IPortPinDMus_fnFastDeviceIoControl,
    IPortPinDMus_fnFastRead,
    IPortPinDMus_fnFastWrite,
    IPortPinDMus_fnInit,
    IPortPinDMus_fnNotify
};




NTSTATUS NewPortPinDMus(
    OUT IPortPinDMus ** OutPin)
{
    IPortPinDMusImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortPinDMusImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortPinDMus */
    This->ref = 1;
    This->lpVtbl = &vt_IPortPinDMus;
    This->lpVtblServiceSink = &vt_IServiceSink;
    This->lpVtblMasterClock = &vt_IMasterClock;
    This->lpVtblAllocatorMXF = &vt_IAllocatorMXF;


    /* store result */
    *OutPin = (IPortPinDMus*)&This->lpVtbl;

    return STATUS_SUCCESS;
}
