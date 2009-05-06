/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_wavecyclic.c
 * PURPOSE:         WaveCyclic IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortPinWaveCyclicVtbl *lpVtbl;
    IServiceSinkVtbl *lpVtblServiceSink;

    LONG ref;
    IPortWaveCyclic * Port;
    IPortFilterWaveCyclic * Filter;
    KSPIN_DESCRIPTOR * KsPinDescriptor;
    PMINIPORTWAVECYCLIC Miniport;
    PSERVICEGROUP ServiceGroup;
    PDMACHANNEL DmaChannel;
    PMINIPORTWAVECYCLICSTREAM Stream;
    KSSTATE State;
    PKSDATAFORMAT Format;
    KSPIN_CONNECT * ConnectDetails;

    PVOID CommonBuffer;
    ULONG CommonBufferSize;
    ULONG CommonBufferOffset;

    IIrpQueue * IrpQueue;

    ULONG FrameSize;
    BOOL Capture;

    ULONG TotalPackets;
    ULONG PreCompleted;
    ULONG PostCompleted;

}IPortPinWaveCyclicImpl;


typedef struct
{
    IPortPinWaveCyclicImpl *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

NTSTATUS
NTAPI
IPortWaveCyclic_fnProcessNewIrp(
    IPortPinWaveCyclicImpl * This);

//==================================================================================================================================

static
NTSTATUS
NTAPI
IServiceSink_fnQueryInterface(
    IServiceSink* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortPinWaveCyclicImpl, lpVtblServiceSink);

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
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortPinWaveCyclicImpl, lpVtblServiceSink);
    DPRINT("IServiceSink_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IServiceSink_fnRelease(
    IServiceSink* iface)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortPinWaveCyclicImpl, lpVtblServiceSink);

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
UpdateCommonBuffer(
    IPortPinWaveCyclicImpl * This,
    ULONG Position)
{
    ULONG BufferLength;
    ULONG BytesToCopy;
    ULONG BufferSize;
    PUCHAR Buffer;
    NTSTATUS Status;

    BufferLength = Position - This->CommonBufferOffset;
    while(BufferLength)
    {
        Status = This->IrpQueue->lpVtbl->GetMapping(This->IrpQueue, &Buffer, &BufferSize);
        if (!NT_SUCCESS(Status))
            return;

        BytesToCopy = min(BufferLength, BufferSize);


        if (This->Capture)
        {
            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel,
                                             Buffer,
                                             (PUCHAR)This->CommonBuffer + This->CommonBufferOffset,
                                             BytesToCopy);
        }
        else
        {
            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel,
                                             (PUCHAR)This->CommonBuffer + This->CommonBufferOffset,
                                             Buffer,
                                             BytesToCopy);
        }

        This->IrpQueue->lpVtbl->UpdateMapping(This->IrpQueue, BytesToCopy);
        This->CommonBufferOffset += BytesToCopy;

        BufferLength = Position - This->CommonBufferOffset;
    }
}

static
VOID
UpdateCommonBufferOverlap(
    IPortPinWaveCyclicImpl * This,
    ULONG Position)
{
    ULONG BufferLength;
    ULONG BytesToCopy;
    ULONG BufferSize;
    PUCHAR Buffer;
    NTSTATUS Status;


    BufferLength = This->CommonBufferSize - This->CommonBufferOffset;
    while(BufferLength)
    {
        Status = This->IrpQueue->lpVtbl->GetMapping(This->IrpQueue, &Buffer, &BufferSize);
        if (!NT_SUCCESS(Status))
            return;

        BytesToCopy = min(BufferLength, BufferSize);

        if (This->Capture)
        {
            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel,
                                             Buffer,
                                             (PUCHAR)This->CommonBuffer + This->CommonBufferOffset,
                                             BytesToCopy);
        }
        else
        {
            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel,
                                             (PUCHAR)This->CommonBuffer + This->CommonBufferOffset,
                                             Buffer,
                                             BytesToCopy);
        }

        This->IrpQueue->lpVtbl->UpdateMapping(This->IrpQueue, BytesToCopy);
        This->CommonBufferOffset += BytesToCopy;

        BufferLength = This->CommonBufferSize - This->CommonBufferOffset;
    }
    This->CommonBufferOffset = 0;
    UpdateCommonBuffer(This, Position);
}

VOID
NTAPI
SetStreamWorkerRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID  Context)
{
    IPortPinWaveCyclicImpl * This;
    PSETSTREAM_CONTEXT Ctx = (PSETSTREAM_CONTEXT)Context;
    KSSTATE State;

    This = Ctx->Pin;
    State = Ctx->State;

    IoFreeWorkItem(Ctx->WorkItem);
    FreeItem(Ctx, TAG_PORTCLASS);

    /* Has the audio stream resumed? */
    if (This->IrpQueue->lpVtbl->NumMappings(This->IrpQueue) && State == KSSTATE_STOP)
        return;

    /* Set the state */
    if (NT_SUCCESS(This->Stream->lpVtbl->SetState(This->Stream, State)))
    {
        /* Set internal state to stop */
        This->State = State;

        if (This->State == KSSTATE_STOP)
        {
            /* reset start stream */
            This->IrpQueue->lpVtbl->CancelBuffers(This->IrpQueue); //FIX function name
            DPRINT1("Stopping PreCompleted %u PostCompleted %u\n", This->PreCompleted, This->PostCompleted);
        }
    }
}

VOID
NTAPI
SetStreamState(
   IN IPortPinWaveCyclicImpl * This,
   IN KSSTATE State)
{
    PDEVICE_OBJECT DeviceObject;
    PIO_WORKITEM WorkItem;
    PSETSTREAM_CONTEXT Context;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    /* Has the audio stream resumed? */
    if (This->IrpQueue->lpVtbl->NumMappings(This->IrpQueue) && State == KSSTATE_STOP)
        return;

    /* Has the audio state already been set? */
    if (This->State == State)
        return;

    /* Get device object */
    DeviceObject = GetDeviceObject(This->Port);

    /* allocate set state context */
    Context = AllocateItem(NonPagedPool, sizeof(SETSTREAM_CONTEXT), TAG_PORTCLASS);

    if (!Context)
        return;

    /* allocate work item */
    WorkItem = IoAllocateWorkItem(DeviceObject);

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

static
VOID
NTAPI
IServiceSink_fnRequestService(
    IServiceSink* iface)
{
    ULONG Position;
    NTSTATUS Status;
    PUCHAR Buffer;
    ULONG BufferSize;
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortPinWaveCyclicImpl, lpVtblServiceSink);

    ASSERT_IRQL(DISPATCH_LEVEL);

    Status = This->IrpQueue->lpVtbl->GetMapping(This->IrpQueue, &Buffer, &BufferSize);
    if (!NT_SUCCESS(Status))
    {
        SetStreamState(This, KSSTATE_STOP);
        return;
    }

    Status = This->Stream->lpVtbl->GetPosition(This->Stream, &Position);
    DPRINT("Position %u Buffer %p BufferSize %u ActiveIrpOffset %u\n", Position, Buffer, This->CommonBufferSize, BufferSize);

    if (Position < This->CommonBufferOffset)
    {
        UpdateCommonBufferOverlap(This, Position);
    }
    else if (Position >= This->CommonBufferOffset)
    {
        UpdateCommonBuffer(This, Position);
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
IPortPinWaveCyclic_fnQueryInterface(
    IPortPinWaveCyclic* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

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
IPortPinWaveCyclic_fnAddRef(
    IPortPinWaveCyclic* iface)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortPinWaveCyclic_fnRelease(
    IPortPinWaveCyclic* iface)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

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
IPortPinWaveCyclic_fnNewIrpTarget(
    IN IPortPinWaveCyclic* iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    UNIMPLEMENTED
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
IPortPinWaveCyclic_HandleKsProperty(
    IN IPortPinWaveCyclic * iface,
    IN PIRP Irp)
{
    PKSPROPERTY Property;
    NTSTATUS Status;
    UNICODE_STRING GuidString;
    PIO_STACK_LOCATION IoStack;
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("IPortPinWave_HandleKsProperty entered\n");

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength < sizeof(KSPROPERTY))
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_INVALID_PARAMETER;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_INVALID_PARAMETER;
    }

    Property = (PKSPROPERTY)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;

    if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Connection))
    {
        if (Property->Id == KSPROPERTY_CONNECTION_STATE)
        {
            PKSSTATE State = (PKSSTATE)Irp->UserBuffer;

            if (IoStack->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KSSTATE))
            {
                Irp->IoStatus.Information = sizeof(KSSTATE);
                Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_BUFFER_TOO_SMALL;
            }

            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                Status = STATUS_UNSUCCESSFUL;
                Irp->IoStatus.Information = 0;

                if (This->Stream)
                {
                    Status = This->Stream->lpVtbl->SetState(This->Stream, *State);

                    DPRINT1("Setting state %u %x\n", *State, Status);
                    if (NT_SUCCESS(Status))
                    {
                        This->State = *State;
                    }
                }
                Irp->IoStatus.Status = Status;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return Status;
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                *State = This->State;
                Irp->IoStatus.Information = sizeof(KSSTATE);
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
        }
        else if (Property->Id == KSPROPERTY_CONNECTION_DATAFORMAT)
        {
            PKSDATAFORMAT DataFormat = (PKSDATAFORMAT)Irp->UserBuffer;
            if (Property->Flags & KSPROPERTY_TYPE_SET)
            {
                PKSDATAFORMAT NewDataFormat;
                if (!RtlCompareMemory(DataFormat, This->Format, DataFormat->FormatSize))
                {
                    Irp->IoStatus.Information = DataFormat->FormatSize;
                    Irp->IoStatus.Status = STATUS_SUCCESS;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_SUCCESS;
                }

                NewDataFormat = AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
                if (!NewDataFormat)
                {
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_NO_MEMORY;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_NO_MEMORY;
                }
                RtlMoveMemory(NewDataFormat, DataFormat, DataFormat->FormatSize);

                if (This->Stream)
                {
                    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
                    ASSERT(NewDataFormat->FormatSize == sizeof(KSDATAFORMAT_WAVEFORMATEX));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.MajorFormat, &KSDATAFORMAT_TYPE_AUDIO));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.SubFormat, &KSDATAFORMAT_SUBTYPE_PCM));
                    ASSERT(IsEqualGUIDAligned(&((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->DataFormat.Specifier, &KSDATAFORMAT_SPECIFIER_WAVEFORMATEX));

                    ASSERT(This->State == KSSTATE_STOP);
                    DPRINT1("NewDataFormat: Channels %u Bits %u Samples %u\n", ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nChannels,
                                                                                 ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.wBitsPerSample,
                                                                                 ((PKSDATAFORMAT_WAVEFORMATEX)NewDataFormat)->WaveFormatEx.nSamplesPerSec);

                    Status = This->Stream->lpVtbl->SetFormat(This->Stream, NewDataFormat);
                    if (NT_SUCCESS(Status))
                    {
                        if (This->Format)
                            ExFreePoolWithTag(This->Format, TAG_PORTCLASS);

                        This->IrpQueue->lpVtbl->UpdateFormat(This->IrpQueue, (PKSDATAFORMAT)NewDataFormat);
                        This->Format = NewDataFormat;
                        Irp->IoStatus.Information = DataFormat->FormatSize;
                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return STATUS_SUCCESS;
                    }
                }
                DPRINT1("Failed to set format\n");
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_UNSUCCESSFUL;
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                if (!This->Format)
                {
                    DPRINT1("No format\n");
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_UNSUCCESSFUL;
                }
                if (This->Format->FormatSize > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
                {
                    Irp->IoStatus.Information = This->Format->FormatSize;
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    IoCompleteRequest(Irp, IO_NO_INCREMENT);
                    return STATUS_BUFFER_TOO_SMALL;
                }

                RtlMoveMemory(DataFormat, This->Format, This->Format->FormatSize);
                Irp->IoStatus.Information = DataFormat->FormatSize;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                IoCompleteRequest(Irp, IO_NO_INCREMENT);
                return STATUS_SUCCESS;
            }
        }

    }
    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT1("Unhandeled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    DbgBreakPoint();
    RtlFreeUnicodeString(&GuidString);

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IPortPinWaveCyclic_HandleKsStream(
    IN IPortPinWaveCyclic * iface,
    IN PIRP Irp)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    DPRINT("IPortPinWaveCyclic_HandleKsStream entered State %u Stream %p\n", This->State, This->Stream);

    return STATUS_PENDING;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortPinWaveCyclic_fnDeviceIoControl(
    IN IPortPinWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;

    IoStack = IoGetCurrentIrpStackLocation(Irp);


    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
       return IPortPinWaveCyclic_HandleKsProperty(iface, Irp);
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
       return IPortPinWaveCyclic_HandleKsStream(iface, Irp);
    }
    else
    {
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

    UNIMPLEMENTED
    DbgBreakPoint();

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
IPortPinWaveCyclic_fnRead(
    IN IPortPinWaveCyclic* iface,
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
IPortPinWaveCyclic_fnWrite(
    IN IPortPinWaveCyclic* iface,
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
IPortPinWaveCyclic_fnFlush(
    IN IPortPinWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

VOID
NTAPI
CloseStreamRoutine(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PVOID Context)
{
    PMINIPORTWAVECYCLICSTREAM Stream;
    NTSTATUS Status;
    ISubdevice *ISubDevice;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    IPortPinWaveCyclicImpl * This;
    PCLOSESTREAM_CONTEXT Ctx = (PCLOSESTREAM_CONTEXT)Context;

    This = (IPortPinWaveCyclicImpl*)Ctx->Pin;

    if (This->Stream)
    {
        if (This->State != KSSTATE_STOP)
        {
            This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_STOP);
            KeStallExecutionProcessor(10);
        }
    }

    This->ServiceGroup->lpVtbl->RemoveMember(This->ServiceGroup, (PSERVICESINK)&This->lpVtblServiceSink);
    This->ServiceGroup->lpVtbl->Release(This->ServiceGroup);
    This->DmaChannel->lpVtbl->Release(This->DmaChannel);

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

    if (This->IrpQueue)
    {
        This->IrpQueue->lpVtbl->Release(This->IrpQueue);
    }

    /* complete the irp */
    Ctx->Irp->IoStatus.Information = 0;
    Ctx->Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Ctx->Irp, IO_NO_INCREMENT);

    /* free the work item */
    IoFreeWorkItem(Ctx->WorkItem);

    /* free work item ctx */
    FreeItem(Ctx, TAG_PORTCLASS);

    if (This->Stream)
    {
        Stream = This->Stream;
        This->Stream = NULL;
        DPRINT1("Closing stream at Irql %u\n", KeGetCurrentIrql());
        Stream->lpVtbl->Release(Stream);
        /* this line is never reached */
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinWaveCyclic_fnClose(
    IN IPortPinWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCLOSESTREAM_CONTEXT Ctx;
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    if (This->Stream)
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
IPortPinWaveCyclic_fnQuerySecurity(
    IN IPortPinWaveCyclic* iface,
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
IPortPinWaveCyclic_fnSetSecurity(
    IN IPortPinWaveCyclic* iface,
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
IPortPinWaveCyclic_fnFastDeviceIoControl(
    IN IPortPinWaveCyclic* iface,
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
IPortPinWaveCyclic_fnFastRead(
    IN IPortPinWaveCyclic* iface,
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
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    DPRINT("IPortPinWaveCyclic_fnFastRead entered\n");

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

        This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_RUN);
        This->State = KSSTATE_RUN;
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortPinWaveCyclic_fnFastWrite(
    IN IPortPinWaveCyclic* iface,
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
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    InterlockedIncrement((PLONG)&This->TotalPackets);

    DPRINT("IPortPinWaveCyclic_fnFastWrite entered Total %u Pre %u Post %u\n", This->TotalPackets, This->PreCompleted, This->PostCompleted);

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
IPortPinWaveCyclic_fnInit(
    IN IPortPinWaveCyclic* iface,
    IN PPORTWAVECYCLIC Port,
    IN PPORTFILTERWAVECYCLIC Filter,
    IN KSPIN_CONNECT * ConnectDetails,
    IN KSPIN_DESCRIPTOR * KsPinDescriptor)
{
    NTSTATUS Status;
    PKSDATAFORMAT DataFormat;
    PDEVICE_OBJECT DeviceObject;
    BOOL Capture;
    //IDrmAudioStream * DrmAudio = NULL;

    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    Port->lpVtbl->AddRef(Port);
    Filter->lpVtbl->AddRef(Filter);

    This->Port = Port;
    This->Filter = Filter;
    This->KsPinDescriptor = KsPinDescriptor;
    This->ConnectDetails = ConnectDetails;
    This->Miniport = GetWaveCyclicMiniport(Port);

    DeviceObject = GetDeviceObject(Port);

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("IPortPinWaveCyclic_fnInit entered\n");

    This->Format = ExAllocatePoolWithTag(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!This->Format)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlMoveMemory(This->Format, DataFormat, DataFormat->FormatSize);

    Status = NewIrpQueue(&This->IrpQueue);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = This->IrpQueue->lpVtbl->Init(This->IrpQueue, ConnectDetails, DataFormat, DeviceObject, 0);
    if (!NT_SUCCESS(Status))
    {
       This->IrpQueue->lpVtbl->Release(This->IrpQueue);
       return Status;
    }

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

    Status = This->Miniport->lpVtbl->NewStream(This->Miniport,
                                               &This->Stream,
                                               NULL,
                                               NonPagedPool,
                                               Capture,
                                               ConnectDetails->PinId,
                                               This->Format,
                                               &This->DmaChannel,
                                               &This->ServiceGroup);
#if 0
    Status = This->Stream->lpVtbl->QueryInterface(This->Stream, &IID_IDrmAudioStream, (PVOID*)&DrmAudio);
    if (NT_SUCCESS(Status))
    {
        DRMRIGHTS DrmRights;
        DPRINT1("Got IID_IDrmAudioStream interface %p\n", DrmAudio);

        DrmRights.CopyProtect = FALSE;
        DrmRights.Reserved = 0;
        DrmRights.DigitalOutputDisable = FALSE;

        Status = DrmAudio->lpVtbl->SetContentId(DrmAudio, 1, &DrmRights);
        DPRINT("Status %x\n", Status);
    }
#endif

    DPRINT("IPortPinWaveCyclic_fnInit Status %x\n", Status);

    if (!NT_SUCCESS(Status))
        return Status;

    Status = This->ServiceGroup->lpVtbl->AddMember(This->ServiceGroup, 
                                                   (PSERVICESINK)&This->lpVtblServiceSink);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to add pin to service group\n");
        return Status;
    }

    This->ServiceGroup->lpVtbl->SupportDelayedService(This->ServiceGroup);
    This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_STOP);
    This->State = KSSTATE_STOP;
    This->CommonBufferOffset = 0;
    This->CommonBufferSize = This->DmaChannel->lpVtbl->AllocatedBufferSize(This->DmaChannel);
    This->CommonBuffer = This->DmaChannel->lpVtbl->SystemAddress(This->DmaChannel);
    This->Capture = Capture;

    Status = This->Stream->lpVtbl->SetNotificationFreq(This->Stream, 10, &This->FrameSize);

    This->Stream->lpVtbl->SetFormat(This->Stream, (PKSDATAFORMAT)This->Format);


    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
ULONG
NTAPI
IPortPinWaveCyclic_fnGetCompletedPosition(
    IN IPortPinWaveCyclic* iface)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
ULONG
NTAPI
IPortPinWaveCyclic_fnGetCycleCount(
    IN IPortPinWaveCyclic* iface)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortPinWaveCyclic_fnGetDeviceBufferSize(
    IN IPortPinWaveCyclic* iface)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    return This->CommonBufferSize;
}

/*
 * @implemented
 */
PVOID
NTAPI
IPortPinWaveCyclic_fnGetIrpStream(
    IN IPortPinWaveCyclic* iface)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    return (PVOID)This->IrpQueue;
}


/*
 * @implemented
 */
PMINIPORT
NTAPI
IPortPinWaveCyclic_fnGetMiniport(
    IN IPortPinWaveCyclic* iface)
{
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    return (PMINIPORT)This->Miniport;
}

static IPortPinWaveCyclicVtbl vt_IPortPinWaveCyclic =
{
    IPortPinWaveCyclic_fnQueryInterface,
    IPortPinWaveCyclic_fnAddRef,
    IPortPinWaveCyclic_fnRelease,
    IPortPinWaveCyclic_fnNewIrpTarget,
    IPortPinWaveCyclic_fnDeviceIoControl,
    IPortPinWaveCyclic_fnRead,
    IPortPinWaveCyclic_fnWrite,
    IPortPinWaveCyclic_fnFlush,
    IPortPinWaveCyclic_fnClose,
    IPortPinWaveCyclic_fnQuerySecurity,
    IPortPinWaveCyclic_fnSetSecurity,
    IPortPinWaveCyclic_fnFastDeviceIoControl,
    IPortPinWaveCyclic_fnFastRead,
    IPortPinWaveCyclic_fnFastWrite,
    IPortPinWaveCyclic_fnInit,
    IPortPinWaveCyclic_fnGetCompletedPosition,
    IPortPinWaveCyclic_fnGetCycleCount,
    IPortPinWaveCyclic_fnGetDeviceBufferSize,
    IPortPinWaveCyclic_fnGetIrpStream,
    IPortPinWaveCyclic_fnGetMiniport
};




NTSTATUS NewPortPinWaveCyclic(
    OUT IPortPinWaveCyclic ** OutPin)
{
    IPortPinWaveCyclicImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortPinWaveCyclicImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortPinWaveCyclic */
    This->ref = 1;
    This->lpVtbl = &vt_IPortPinWaveCyclic;
    This->lpVtblServiceSink = &vt_IServiceSink;


    /* store result */
    *OutPin = (IPortPinWaveCyclic*)&This->lpVtbl;

    return STATUS_SUCCESS;
}
