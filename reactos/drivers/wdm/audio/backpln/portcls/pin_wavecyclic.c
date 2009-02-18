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

}IPortPinWaveCyclicImpl;

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
    IPortPinWaveCyclicImpl * This = (IPortPinWaveCyclicImpl*)CONTAINING_RECORD(iface, IPortPinWaveCyclicImpl, lpVtblServiceSink);

    DPRINT("IServiceSink_fnRequestService entered %p\n", This);
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
                return STATUS_BUFFER_TOO_SMALL;
            }

            if (Property->Id & KSPROPERTY_TYPE_SET)
            {
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                if (This->Stream)
                {
                    Status = This->Stream->lpVtbl->SetState(This->Stream, *State);

                    DPRINT1("Setting state %x\n", Status);
                    if (NT_SUCCESS(Status))
                    {
                        This->State = *State;
                        Irp->IoStatus.Information = sizeof(KSSTATE);
                        Irp->IoStatus.Status = Status;
                        return Status;
                    }
                    Irp->IoStatus.Status = Status;
                }
                return Irp->IoStatus.Status;
            }
            else if (Property->Id & KSPROPERTY_TYPE_GET)
            {
                *State = This->State;
                Irp->IoStatus.Information = sizeof(KSSTATE);
                Irp->IoStatus.Status = STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
        }
        else if (Property->Id == KSPROPERTY_CONNECTION_DATAFORMAT)
        {
            PKSDATAFORMAT DataFormat = (PKSDATAFORMAT)Irp->UserBuffer;
            if (Property->Id & KSPROPERTY_TYPE_SET)
            {
                PKSDATAFORMAT NewDataFormat = AllocateItem(NonPagedPool, DataFormat->FormatSize, TAG_PORTCLASS);
                if (!NewDataFormat)
                {
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_NO_MEMORY;
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
                        return STATUS_SUCCESS;
                    }
                }
                Irp->IoStatus.Information = 0;
                Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                return STATUS_UNSUCCESSFUL;
            }
            else if (Property->Id & KSPROPERTY_TYPE_GET)
            {
                PKSDATAFORMAT DataFormat = (PKSDATAFORMAT)Irp->UserBuffer;
                if (!This->Format)
                {
                    DPRINT1("No format\n");
                    Irp->IoStatus.Information = 0;
                    Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
                    return STATUS_UNSUCCESSFUL;
                }
                if (This->Format->FormatSize > IoStack->Parameters.DeviceIoControl.OutputBufferLength)
                {
                    Irp->IoStatus.Information = This->Format->FormatSize;
                    Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
                    return STATUS_BUFFER_TOO_SMALL;
                }

                RtlMoveMemory(DataFormat, This->Format, This->Format->FormatSize);
                Irp->IoStatus.Information = DataFormat->FormatSize;
                Irp->IoStatus.Status = STATUS_SUCCESS;
                return STATUS_SUCCESS;
            }
        }
    }
    RtlStringFromGUID(&Property->Set, &GuidString);

    DPRINT1("Unhandeled property Set %S Id %u Flags %x\n", GuidString.Buffer, Property->Id, Property->Flags);
    DbgBreakPoint();
    RtlFreeUnicodeString(&GuidString);

    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;
    return STATUS_NOT_IMPLEMENTED;
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

    UNIMPLEMENTED

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    DPRINT1("IPortPinWaveCyclic_fnDeviceIoControl %x %x\n",IoStack->Parameters.DeviceIoControl.IoControlCode,  IOCTL_KS_PROPERTY);
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
        /// FIXME
        /// handle reset state
    }
    else
    {
        return KsDefaultDeviceIoCompletion(DeviceObject, Irp);
    }

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
 * @unimplemented
 */
ULONG
NTAPI
IPortPinWaveCyclic_fnGetDeviceBufferSize(
    IN IPortPinWaveCyclic* iface)
{
    UNIMPLEMENTED;
    return 0;
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
    UNIMPLEMENTED;
    return NULL;
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
