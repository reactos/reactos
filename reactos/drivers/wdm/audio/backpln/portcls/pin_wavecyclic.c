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

    LIST_ENTRY QueueHead;
    KSPIN_LOCK QueueLock;

    PVOID CommonBuffer;
    ULONG CommonBufferSize;
    ULONG CommonBufferOffset;

    PIRP ActiveIrp;
    PUCHAR ActiveIrpBuffer;
    ULONG ActiveIrpBufferSize;
    ULONG ActiveIrpOffset;
    ULONG DelayedRequestInProgress;


}IPortPinWaveCyclicImpl;

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
NTAPI
IServiceSink_fnRequestService(
    IServiceSink* iface)
{
    ULONG Position;
    ULONG BufferLength, IrpLength, BytesToCopy;
    NTSTATUS Status;

    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortPinWaveCyclicImpl, lpVtblServiceSink);

    if (This->ActiveIrp && This->ActiveIrpOffset >= This->ActiveIrpBufferSize)
    {
        This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_PAUSE);
        if (KeGetCurrentIrql() > DISPATCH_LEVEL)
        {
            This->DelayedRequestInProgress = TRUE;
            This->ServiceGroup->lpVtbl->RequestDelayedService(This->ServiceGroup, -10000000L);
            return;
        }
        else
        {
            if (This->ActiveIrp)
            {
                This->ActiveIrp->IoStatus.Status = STATUS_SUCCESS;
                This->ActiveIrp->IoStatus.Information = This->ActiveIrpBufferSize;
                IoCompleteRequest(This->ActiveIrp, IO_SOUND_INCREMENT);
                This->ActiveIrp = NULL;
            }

            Status = IPortWaveCyclic_fnProcessNewIrp(This);
            if (Status == STATUS_SUCCESS)
                This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_RUN);
            else
                This->ServiceGroup->lpVtbl->RequestDelayedService(This->ServiceGroup, -10000000L);
            return;
        }
    }

    if (!This->ActiveIrp)
    {
        if (KeGetCurrentIrql() > DISPATCH_LEVEL)
        {
            This->ServiceGroup->lpVtbl->RequestDelayedService(This->ServiceGroup, -10000000L);
            return;
        }

        Status = IPortWaveCyclic_fnProcessNewIrp(This);
        if (Status == STATUS_SUCCESS)
            This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_RUN);
        else
            This->ServiceGroup->lpVtbl->RequestDelayedService(This->ServiceGroup, -10000000L);
        return;
    }

    Status = This->Stream->lpVtbl->GetPosition(This->Stream, &Position);
    DPRINT("Position %u BufferSize %u ActiveIrpOffset %u\n", Position, This->CommonBufferSize, This->ActiveIrpOffset);

    if (Position < This->CommonBufferOffset)
    {
        BufferLength = This->CommonBufferSize - This->CommonBufferOffset;
        IrpLength = This->ActiveIrpBufferSize - This->ActiveIrpOffset;

        BytesToCopy = min(BufferLength, IrpLength);

        DPRINT("Copying %u Remaining %u\n", BytesToCopy, IrpLength);

        if (BytesToCopy)
        {
            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel, (PUCHAR)This->CommonBuffer + This->CommonBufferOffset, (PUCHAR)This->ActiveIrpBuffer + This->ActiveIrpOffset, BytesToCopy);
            This->ActiveIrpOffset += BytesToCopy;
            This->CommonBufferOffset = 0;
        }
        else
        {
            This->Stream->lpVtbl->SetState(This->Stream, KSSTATE_PAUSE);
            return;
        }
        BufferLength = Position;
        IrpLength = This->ActiveIrpBufferSize - This->ActiveIrpOffset;
        BytesToCopy = min(BufferLength, IrpLength);

        DPRINT("Copying %u Remaining %u\n", BytesToCopy, IrpLength);

        if (BytesToCopy)
        {
            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel, (PUCHAR)(PUCHAR)This->CommonBuffer, (PUCHAR)This->ActiveIrpBuffer + This->ActiveIrpOffset, BytesToCopy);
            This->ActiveIrpOffset += Position;
            This->CommonBufferOffset = Position;
        }

    }
    else if (Position >= This->CommonBufferOffset)
    {
        BufferLength = Position - This->CommonBufferOffset;
        IrpLength = This->ActiveIrpBufferSize - This->ActiveIrpOffset;

        BytesToCopy = min(BufferLength, IrpLength);
        DPRINT("Copying %u Remaining %u\n", BytesToCopy, IrpLength);

        if (!BytesToCopy)
        {
            This->ServiceGroup->lpVtbl->RequestDelayedService(This->ServiceGroup, -10000000L);
            return;
        }

            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel,
                                         (PUCHAR)This->CommonBuffer + This->CommonBufferOffset,
                                         (PUCHAR)This->ActiveIrpBuffer + This->ActiveIrpOffset,
                                         BytesToCopy);

        This->ActiveIrpOffset += BytesToCopy;
        This->CommonBufferOffset = Position;
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
        //IsEqualGUIDAligned(refiid, &IID_IPortPinWaveCyclic) ||
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
    DPRINT1("IPortPinWaveCyclic_fnNewIrpTarget\n");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
IPortWaveCyclic_fnProcessNewIrp(
    IPortPinWaveCyclicImpl * This)
{
    PIO_STACK_LOCATION IoStack;
    PKSSTREAM_HEADER Header;

    This->ActiveIrp = KsRemoveIrpFromCancelableQueue(&This->QueueHead, &This->QueueLock, KsListEntryTail, KsAcquireAndRemove);
    if (!This->ActiveIrp)
    {
        This->ServiceGroup->lpVtbl->RequestDelayedService(This->ServiceGroup, -10000000L);
        return STATUS_PENDING;
    }

    DPRINT("ActiveIrp %p\n", This->ActiveIrp);

    IoStack = IoGetCurrentIrpStackLocation(This->ActiveIrp);

    if (IoStack->Parameters.DeviceIoControl.InputBufferLength != sizeof(KSSTREAM_HEADER))
    {
        DPRINT1("Irp has unexpected header\n");
        IoCompleteRequest(This->ActiveIrp, IO_NO_INCREMENT);
        This->ActiveIrp = NULL;
        return STATUS_INVALID_PARAMETER;
    }

    Header = (PKSSTREAM_HEADER)IoStack->Parameters.DeviceIoControl.Type3InputBuffer;
    DPRINT("Header %p Size %u\n", Header, Header->Size);


    This->ActiveIrpOffset = 0;
    This->ActiveIrpBuffer = Header->Data;
    This->ActiveIrpBufferSize = Header->DataUsed;

    return STATUS_SUCCESS;
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

    DPRINT1("IPortPinWave_HandleKsProperty entered\n");

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
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                if (This->Stream)
                {
                    This->CommonBufferOffset = 0;
                    This->CommonBufferSize = This->DmaChannel->lpVtbl->AllocatedBufferSize(This->DmaChannel);
                    This->CommonBuffer = This->DmaChannel->lpVtbl->SystemAddress(This->DmaChannel);

                    This->ActiveIrp = KsRemoveIrpFromCancelableQueue(&This->QueueHead, &This->QueueLock, KsListEntryHead, KsAcquireAndRemove);
                    if (This->ActiveIrp)
                    {
                        Status = IPortWaveCyclic_fnProcessNewIrp(This);
                        DPRINT1("IPortWaveCyclic_fnProcessNewIrp result %x\n", Status);
                        if (NT_SUCCESS(Status))
                        {
                            //FIXME
                            // verify offset - resume stream
                            This->DmaChannel->lpVtbl->CopyTo(This->DmaChannel, This->CommonBuffer, This->ActiveIrpBuffer, This->CommonBufferSize);
                            This->ActiveIrpOffset += This->CommonBufferSize;
                        }
                    }


                    Status = This->Stream->lpVtbl->SetState(This->Stream, *State);

                    DPRINT1("Setting state %x\n", Status);
                    if (NT_SUCCESS(Status))
                    {
                        This->State = *State;
                        Irp->IoStatus.Information = sizeof(KSSTATE);
                        Irp->IoStatus.Status = Status;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return Status;
                    }
                    Irp->IoStatus.Status = Status;
                }
                return Irp->IoStatus.Status;
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
                PKSDATAFORMAT NewDataFormat = AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
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
                    Status = This->Stream->lpVtbl->SetFormat(This->Stream, DataFormat);
                    if (NT_SUCCESS(Status))
                    {
                        if (This->Format)
                            ExFreePoolWithTag(This->Format, TAG_PORTCLASS);
                        This->Format = NewDataFormat;
                        Irp->IoStatus.Information = DataFormat->FormatSize;
                        Irp->IoStatus.Status = STATUS_SUCCESS;
                        IoCompleteRequest(Irp, IO_NO_INCREMENT);
                        return STATUS_SUCCESS;
                    }
                }
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
    PIO_STACK_LOCATION IoStack;
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT1("IPortPinWaveCyclic_HandleKsStream entered\n");

    if (!This->Stream)
    {
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    KsAddIrpToCancelableQueue(&This->QueueHead, &This->QueueLock, Irp, KsListEntryTail, NULL);
    IoMarkIrpPending(Irp);
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_PENDING;

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

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortPinWaveCyclic_fnClose(
    IN IPortPinWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    DPRINT1("IPortPinWaveCyclic_fnClose\n");

    //FIXME

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
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
NTSTATUS
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

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
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
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
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
    return STATUS_SUCCESS;
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
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)iface;

    Port->lpVtbl->AddRef(Port);
    Filter->lpVtbl->AddRef(Filter);

    This->Port = Port;
    This->Filter = Filter;
    This->KsPinDescriptor = KsPinDescriptor;
    This->Miniport = GetWaveCyclicMiniport(Port);
    KeInitializeSpinLock(&This->QueueLock);
    InitializeListHead(&This->QueueHead);

    DataFormat = (PKSDATAFORMAT)(ConnectDetails + 1);

    DPRINT("IPortPinWaveCyclic_fnInit entered\n");

    This->Format = ExAllocatePoolWithTag(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
    if (!This->Format)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlMoveMemory(This->Format, DataFormat, DataFormat->FormatSize);

    Status = This->Miniport->lpVtbl->NewStream(This->Miniport,
                                               &This->Stream,
                                               NULL,
                                               NonPagedPool,
                                               FALSE, //FIXME
                                               ConnectDetails->PinId,
                                               This->Format,
                                               &This->DmaChannel,
                                               &This->ServiceGroup);

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

    This->State = KSSTATE_STOP;

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
 * @unimplemented
 */
PVOID
NTAPI
IPortPinWaveCyclic_fnGetIrpStream(
    IN IPortPinWaveCyclic* iface)
{
    UNIMPLEMENTED;
    return NULL;
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
