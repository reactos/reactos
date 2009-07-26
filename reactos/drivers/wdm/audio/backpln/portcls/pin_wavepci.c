/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/pin_WavePci.c
 * PURPOSE:         WavePci IRP Audio Pin
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortPinWavePciVtbl *lpVtbl;
    IServiceSinkVtbl *lpVtblServiceSink;
    IPortWavePciStreamVtbl *lpVtblPortWavePciStream;

    LONG ref;
    IPortWavePci * Port;
    IPortFilterWavePci * Filter;
    KSPIN_DESCRIPTOR * KsPinDescriptor;
    PMINIPORTWAVEPCI Miniport;
    PSERVICEGROUP ServiceGroup;
    PDMACHANNEL DmaChannel;
    PMINIPORTWAVEPCISTREAM Stream;
    KSSTATE State;
    PKSDATAFORMAT Format;
    KSPIN_CONNECT * ConnectDetails;

    BOOL Capture;
    PDEVICE_OBJECT DeviceObject;
    IIrpQueue * IrpQueue;

    ULONG TotalPackets;
    ULONG PreCompleted;
    ULONG PostCompleted;
    ULONG StopCount;

    ULONG Delay;

    BOOL bUsePrefetch;
    ULONG PrefetchOffset;

    KSALLOCATOR_FRAMING AllocatorFraming;

}IPortPinWavePciImpl;

typedef struct
{
    IPortPinWavePciImpl *Pin;
    PIO_WORKITEM WorkItem;
    KSSTATE State;
}SETSTREAM_CONTEXT, *PSETSTREAM_CONTEXT;

//==================================================================================================================================
static
NTSTATUS
NTAPI
IPortWavePciStream_fnQueryInterface(
    IPortWavePciStream* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblPortWavePciStream);

    DPRINT("IPortWavePciStream_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortWavePciStream) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IPortWavePciStream_fnAddRef(
    IPortWavePciStream* iface)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblPortWavePciStream);
    DPRINT("IPortWavePciStream_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortWavePciStream_fnRelease(
    IPortWavePciStream* iface)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblPortWavePciStream);

    InterlockedDecrement(&This->ref);

    DPRINT("IPortWavePciStream_fnRelease entered %u\n", This->ref);

    /* Return new reference count */
    return This->ref;
}

static
NTSTATUS
NTAPI
IPortWavePciStream_fnGetMapping(
    IN IPortWavePciStream *iface,
    IN PVOID Tag,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress,
    OUT PVOID  *VirtualAddress,
    OUT PULONG  ByteCount,
    OUT PULONG  Flags)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblPortWavePciStream);

    ASSERT_IRQL(DISPATCH_LEVEL);
    return This->IrpQueue->lpVtbl->GetMappingWithTag(This->IrpQueue, Tag, PhysicalAddress, VirtualAddress, ByteCount, Flags);
}

static
NTSTATUS
NTAPI
IPortWavePciStream_fnReleaseMapping(
    IN IPortWavePciStream *iface,
    IN PVOID  Tag)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblPortWavePciStream);

    ASSERT_IRQL(DISPATCH_LEVEL);
    return This->IrpQueue->lpVtbl->ReleaseMappingWithTag(This->IrpQueue, Tag);
}

static
NTSTATUS
NTAPI
IPortWavePciStream_fnTerminatePacket(
    IN IPortWavePciStream *iface)
{
    UNIMPLEMENTED
    ASSERT_IRQL(DISPATCH_LEVEL);
    return STATUS_SUCCESS;
}


static IPortWavePciStreamVtbl vt_PortWavePciStream =
{
    IPortWavePciStream_fnQueryInterface,
    IPortWavePciStream_fnAddRef,
    IPortWavePciStream_fnRelease,
    IPortWavePciStream_fnGetMapping,
    IPortWavePciStream_fnReleaseMapping,
    IPortWavePciStream_fnTerminatePacket
};


static
NTSTATUS
NTAPI
IServiceSink_fnQueryInterface(
    IServiceSink* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblServiceSink);

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
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblServiceSink);
    DPRINT("IServiceSink_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IServiceSink_fnRelease(
    IServiceSink* iface)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblServiceSink);

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
    IPortPinWavePciImpl * This;
    PSETSTREAM_CONTEXT Ctx = (PSETSTREAM_CONTEXT)Context;
    KSSTATE State;
    ULONG MinimumDataThreshold;
    ULONG MaximumDataThreshold;

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
            //This->ServiceGroup->lpVtbl->CancelDelayedService(This->ServiceGroup);
            /* increase stop counter */
            This->StopCount++;
            /* get current data threshold */
            MinimumDataThreshold = This->IrpQueue->lpVtbl->GetMinimumDataThreshold(This->IrpQueue);
            /* get maximum data threshold */
            MaximumDataThreshold = ((PKSDATAFORMAT_WAVEFORMATEX)This->Format)->WaveFormatEx.nAvgBytesPerSec;
            /* increase minimum data threshold by 10 frames */
            MinimumDataThreshold += This->AllocatorFraming.FrameSize * 10;

            /* assure it has not exceeded */
            MinimumDataThreshold = min(MinimumDataThreshold, MaximumDataThreshold);
            /* store minimum data threshold */
            This->IrpQueue->lpVtbl->SetMinimumDataThreshold(This->IrpQueue, MinimumDataThreshold);

            DPRINT1("Stopping PreCompleted %u PostCompleted %u StopCount %u MinimumDataThreshold %u\n", This->PreCompleted, This->PostCompleted, This->StopCount, MinimumDataThreshold);
        }
        if (This->State == KSSTATE_RUN)
        {
            /* start the notification timer */
            //This->ServiceGroup->lpVtbl->RequestDelayedService(This->ServiceGroup, This->Delay);
        }
    }
}
static
VOID
NTAPI
SetStreamState(
   IN IPortPinWavePciImpl * This,
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
    DeviceObject = GetDeviceObjectFromPortWavePci(This->Port);

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
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)CONTAINING_RECORD(iface, IPortPinWavePciImpl, lpVtblServiceSink);

    ASSERT_IRQL(DISPATCH_LEVEL);

    if (This->IrpQueue->lpVtbl->HasLastMappingFailed(This->IrpQueue))
    {
        if (This->IrpQueue->lpVtbl->NumMappings(This->IrpQueue) == 0)
        {
            DPRINT("Stopping stream...\n");
            SetStreamState(This, KSSTATE_STOP);
            return;
        }
    }

    This->Stream->lpVtbl->Service(This->Stream);
    //TODO
    //generate events
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
IPortPinWavePci_fnQueryInterface(
    IPortPinWavePci* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

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
IPortPinWavePci_fnAddRef(
    IPortPinWavePci* iface)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortPinWavePci_fnRelease(
    IPortPinWavePci* iface)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

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
IPortPinWavePci_fnNewIrpTarget(
    IN IPortPinWavePci* iface,
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
IPortPinWavePci_HandleKsProperty(
    IN IPortPinWavePci * iface,
    IN PVOID InputBuffer,
    IN ULONG InputBufferLength,
    IN PVOID OutputBuffer,
    IN ULONG OutputBufferLength,
    IN PIO_STATUS_BLOCK IoStatusBlock)
{
    PKSPROPERTY Property;
    NTSTATUS Status;
    UNICODE_STRING GuidString;

    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;


    DPRINT("IPortPinWavePci_HandleKsProperty entered\n");

    if (InputBufferLength < sizeof(KSPROPERTY))
    {
        IoStatusBlock->Information = 0;
        IoStatusBlock->Status = STATUS_INVALID_PARAMETER;
        return STATUS_INVALID_PARAMETER;
    }

    Property = (PKSPROPERTY)InputBuffer;

    if (IsEqualGUIDAligned(&Property->Set, &KSPROPSETID_Connection))
    {
        if (Property->Id == KSPROPERTY_CONNECTION_STATE)
        {
            PKSSTATE State = (PKSSTATE)OutputBuffer;

            ASSERT_IRQL(DISPATCH_LEVEL);
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

                if (This->Stream)
                {
                    Status = This->Stream->lpVtbl->SetState(This->Stream, *State);

                    DPRINT1("Setting state %u %x\n", *State, Status);
                    if (NT_SUCCESS(Status))
                    {
                        This->State = *State;
                    }
                }
                IoStatusBlock->Status = Status;
                return Status;
            }
            else if (Property->Flags & KSPROPERTY_TYPE_GET)
            {
                *State = This->State;
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
                if (!RtlCompareMemory(DataFormat, This->Format, DataFormat->FormatSize))
                {
                    IoStatusBlock->Information = DataFormat->FormatSize;
                    IoStatusBlock->Status = STATUS_SUCCESS;
                    return STATUS_SUCCESS;
                }

                NewDataFormat = AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
                if (!NewDataFormat)
                {
                    IoStatusBlock->Information = 0;
                    IoStatusBlock->Status = STATUS_NO_MEMORY;
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
                if (!This->Format)
                {
                    DPRINT1("No format\n");
                    IoStatusBlock->Information = 0;
                    IoStatusBlock->Status = STATUS_UNSUCCESSFUL;
                    return STATUS_UNSUCCESSFUL;
                }
                if (This->Format->FormatSize > OutputBufferLength)
                {
                    IoStatusBlock->Information = This->Format->FormatSize;
                    IoStatusBlock->Status = STATUS_BUFFER_TOO_SMALL;
                    return STATUS_BUFFER_TOO_SMALL;
                }

                RtlMoveMemory(DataFormat, This->Format, This->Format->FormatSize);
                IoStatusBlock->Information = DataFormat->FormatSize;
                IoStatusBlock->Status = STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
        }
        else if (Property->Id == KSPROPERTY_CONNECTION_ALLOCATORFRAMING)
        {
            PKSALLOCATOR_FRAMING Framing = (PKSALLOCATOR_FRAMING)OutputBuffer;

            ASSERT_IRQL(DISPATCH_LEVEL);
            /* Validate input buffer */
            if (OutputBufferLength < sizeof(KSALLOCATOR_FRAMING))
            {
                IoStatusBlock->Information = sizeof(KSALLOCATOR_FRAMING);
                IoStatusBlock->Status = STATUS_BUFFER_TOO_SMALL;
                return STATUS_BUFFER_TOO_SMALL;
            }
            /* copy frame allocator struct */
            RtlMoveMemory(Framing, &This->AllocatorFraming, sizeof(KSALLOCATOR_FRAMING));

            IoStatusBlock->Information = sizeof(KSALLOCATOR_FRAMING);
            IoStatusBlock->Status = STATUS_SUCCESS;
            return STATUS_SUCCESS;
        }
    }

    RtlStringFromGUID(&Property->Set, &GuidString);
    DPRINT1("Unhandeled property Set |%S| Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    RtlFreeUnicodeString(&GuidString);

    IoStatusBlock->Status = STATUS_NOT_IMPLEMENTED;
    IoStatusBlock->Information = 0;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortPinWavePci_fnDeviceIoControl(
    IN IPortPinWavePci* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY)
    {
       Status = IPortPinWavePci_HandleKsProperty(iface, IoStack->Parameters.DeviceIoControl.Type3InputBuffer, IoStack->Parameters.DeviceIoControl.InputBufferLength, Irp->UserBuffer, IoStack->Parameters.DeviceIoControl.OutputBufferLength, &Irp->IoStatus);
       IoCompleteRequest(Irp, IO_NO_INCREMENT);
       return Status;
    }


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
IPortPinWavePci_fnRead(
    IN IPortPinWavePci* iface,
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
IPortPinWavePci_fnWrite(
    IN IPortPinWavePci* iface,
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
IPortPinWavePci_fnFlush(
    IN IPortPinWavePci* iface,
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
    PMINIPORTWAVEPCISTREAM Stream;
    NTSTATUS Status;
    ISubdevice *ISubDevice;
    PSUBDEVICE_DESCRIPTOR Descriptor;
    IPortPinWavePciImpl * This;
    PCLOSESTREAM_CONTEXT Ctx = (PCLOSESTREAM_CONTEXT)Context;

    This = (IPortPinWavePciImpl*)Ctx->Pin;

    if (This->Stream)
    {
        if (This->State != KSSTATE_STOP)
        {
            This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_STOP);
        }
    }

    if (This->ServiceGroup)
    {
        This->ServiceGroup->lpVtbl->RemoveMember(This->ServiceGroup, (PSERVICESINK)&This->lpVtblServiceSink);
    }

    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&ISubDevice);
    if (NT_SUCCESS(Status))
    {
        Status = ISubDevice->lpVtbl->GetDescriptor(ISubDevice, &Descriptor);
        if (NT_SUCCESS(Status))
        {
            Descriptor->Factory.Instances[This->ConnectDetails->PinId].CurrentPinInstanceCount--;
        }
        ISubDevice->lpVtbl->Release(ISubDevice);
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

    if (This->Stream)
    {
        Stream = This->Stream;
        This->Stream = NULL;
        DPRINT1("Closing stream at Irql %u\n", KeGetCurrentIrql());
        Stream->lpVtbl->Release(Stream);
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortPinWavePci_fnClose(
    IN IPortPinWavePci* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PCLOSESTREAM_CONTEXT Ctx;
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

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
IPortPinWavePci_fnQuerySecurity(
    IN IPortPinWavePci* iface,
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
IPortPinWavePci_fnSetSecurity(
    IN IPortPinWavePci* iface,
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
IPortPinWavePci_fnFastDeviceIoControl(
    IN IPortPinWavePci* iface,
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
    NTSTATUS Status;

    if (IoControlCode == IOCTL_KS_PROPERTY)
    {
       Status = IPortPinWavePci_HandleKsProperty(iface, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, StatusBlock);
       if (NT_SUCCESS(Status))
       {
           return TRUE;
       }
    }

    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortPinWavePci_fnFastRead(
    IN IPortPinWavePci* iface,
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
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

    DPRINT("IPortPinWavePci_fnFastRead entered\n");

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
IPortPinWavePci_fnFastWrite(
    IN IPortPinWavePci* iface,
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
    ULONG MinimumDataThreshold;
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

    DPRINT("IPortPinWavePci_fnFastWrite entered Total %u Pre %u Post %u\n", This->TotalPackets, This->PreCompleted, This->PostCompleted);

    InterlockedIncrement((PLONG)&This->TotalPackets);

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

    if (This->IrpQueue->lpVtbl->HasLastMappingFailed(This->IrpQueue))
    {
        /* get minimum data threshold */
        MinimumDataThreshold = This->IrpQueue->lpVtbl->GetMinimumDataThreshold(This->IrpQueue);

        if (MinimumDataThreshold < This->IrpQueue->lpVtbl->NumData(This->IrpQueue))
        {
            /* notify port driver that new mapping is available */
            DPRINT("Notifying of new mapping\n");
            This->Stream->lpVtbl->MappingAvailable(This->Stream);
        }
    }

    return TRUE;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortPinWavePci_fnInit(
    IN IPortPinWavePci* iface,
    IN PPORTWAVEPCI Port,
    IN PPORTFILTERWAVEPCI Filter,
    IN KSPIN_CONNECT * ConnectDetails,
    IN KSPIN_DESCRIPTOR * KsPinDescriptor,
    IN PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PKSDATAFORMAT DataFormat;
    BOOL Capture;

    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

    Port->lpVtbl->AddRef(Port);
    Filter->lpVtbl->AddRef(Filter);

    This->Port = Port;
    This->Filter = Filter;
    This->KsPinDescriptor = KsPinDescriptor;
    This->ConnectDetails = ConnectDetails;
    This->Miniport = GetWavePciMiniport(Port);
    This->DeviceObject = DeviceObject;

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("IPortPinWavePci_fnInit entered\n");

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

    Status = This->Miniport->lpVtbl->NewStream(This->Miniport,
                                               &This->Stream,
                                               NULL,
                                               NonPagedPool,
                                               (PPORTWAVEPCISTREAM)&This->lpVtblPortWavePciStream,
                                               ConnectDetails->PinId,
                                               Capture,
                                               This->Format,
                                               &This->DmaChannel,
                                               &This->ServiceGroup);

    DPRINT("IPortPinWavePci_fnInit Status %x\n", Status);

    if (!NT_SUCCESS(Status))
        return Status;

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

    /* delay of 10 milisec */
    This->Delay = Int32x32To64(10, -10000);

    Status = This->Stream->lpVtbl->GetAllocatorFraming(This->Stream, &This->AllocatorFraming);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetAllocatorFraming failed with %x\n", Status);
        return Status;
    }

    DPRINT("OptionFlags %x RequirementsFlag %x PoolType %x Frames %lu FrameSize %lu FileAlignment %lu\n",
           This->AllocatorFraming.OptionsFlags, This->AllocatorFraming.RequirementsFlags, This->AllocatorFraming.PoolType, This->AllocatorFraming.Frames, This->AllocatorFraming.FrameSize, This->AllocatorFraming.FileAlignment);

    Status = NewIrpQueue(&This->IrpQueue);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = This->IrpQueue->lpVtbl->Init(This->IrpQueue, ConnectDetails, This->Format, DeviceObject, This->AllocatorFraming.FrameSize, This->AllocatorFraming.FileAlignment);
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
PVOID
NTAPI
IPortPinWavePci_fnGetIrpStream(
    IN IPortPinWavePci* iface)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

    return (PVOID)This->IrpQueue;
}


/*
 * @implemented
 */
PMINIPORT
NTAPI
IPortPinWavePci_fnGetMiniport(
    IN IPortPinWavePci* iface)
{
    IPortPinWavePciImpl * This = (IPortPinWavePciImpl*)iface;

    return (PMINIPORT)This->Miniport;
}

static IPortPinWavePciVtbl vt_IPortPinWavePci =
{
    IPortPinWavePci_fnQueryInterface,
    IPortPinWavePci_fnAddRef,
    IPortPinWavePci_fnRelease,
    IPortPinWavePci_fnNewIrpTarget,
    IPortPinWavePci_fnDeviceIoControl,
    IPortPinWavePci_fnRead,
    IPortPinWavePci_fnWrite,
    IPortPinWavePci_fnFlush,
    IPortPinWavePci_fnClose,
    IPortPinWavePci_fnQuerySecurity,
    IPortPinWavePci_fnSetSecurity,
    IPortPinWavePci_fnFastDeviceIoControl,
    IPortPinWavePci_fnFastRead,
    IPortPinWavePci_fnFastWrite,
    IPortPinWavePci_fnInit,
    IPortPinWavePci_fnGetIrpStream,
    IPortPinWavePci_fnGetMiniport
};




NTSTATUS NewPortPinWavePci(
    OUT IPortPinWavePci ** OutPin)
{
    IPortPinWavePciImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortPinWavePciImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortPinWavePci */
    This->ref = 1;
    This->lpVtbl = &vt_IPortPinWavePci;
    This->lpVtblServiceSink = &vt_IServiceSink;
    This->lpVtblPortWavePciStream = &vt_PortWavePciStream;


    /* store result */
    *OutPin = (IPortPinWavePci*)&This->lpVtbl;

    return STATUS_SUCCESS;
}
