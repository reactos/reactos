/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_topology.c
 * PURPOSE:         portcls topology filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortFilterTopologyVtbl *lpVtbl;

    LONG ref;

    IPortTopology* Port;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    ISubdevice *SubDevice;

}IPortFilterTopologyImpl;

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterTopology_fnQueryInterface(
    IPortFilterTopology* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortFilterTopologyImpl * This = (IPortFilterTopologyImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IIrpTarget) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(refiid, &IID_IPort))
    {
        *Output = This->Port;
        This->Port->lpVtbl->AddRef(This->Port);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterTopology_fnAddRef(
    IPortFilterTopology* iface)
{
    IPortFilterTopologyImpl * This = (IPortFilterTopologyImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterTopology_fnRelease(
    IPortFilterTopology* iface)
{
    IPortFilterTopologyImpl * This = (IPortFilterTopologyImpl*)iface;

    InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    return This->ref;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterTopology_fnNewIrpTarget(
    IN IPortFilterTopology* iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    DPRINT("IPortFilterTopology_fnNewIrpTarget entered\n");

    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterTopology_fnDeviceIoControl(
    IN IPortFilterTopology* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    IPortFilterTopologyImpl * This = (IPortFilterTopologyImpl *)iface;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY);

    return PcPropertyHandler(Irp, This->Descriptor);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterTopology_fnRead(
    IN IPortFilterTopology* iface,
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
IPortFilterTopology_fnWrite(
    IN IPortFilterTopology* iface,
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
IPortFilterTopology_fnFlush(
    IN IPortFilterTopology* iface,
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
IPortFilterTopology_fnClose(
    IN IPortFilterTopology* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    IPortFilterTopologyImpl * This = (IPortFilterTopologyImpl *)iface;

    /* FIXME handle DirectSound */

    if (This->ref == 1)
    {
        /* release reference to port */
        This->SubDevice->lpVtbl->Release(This->SubDevice);

        /* time to shutdown the audio system */
        Status = This->SubDevice->lpVtbl->ReleaseChildren(This->SubDevice);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterTopology_fnQuerySecurity(
    IN IPortFilterTopology* iface,
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
IPortFilterTopology_fnSetSecurity(
    IN IPortFilterTopology* iface,
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
IPortFilterTopology_fnFastDeviceIoControl(
    IN IPortFilterTopology* iface,
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
    ULONG Index;
    PKSPROPERTY Property;
    IPortFilterTopologyImpl * This = (IPortFilterTopologyImpl*)iface;

    Property = (PKSPROPERTY)InputBuffer;

    if (InputBufferLength < sizeof(KSPROPERTY))
        return FALSE;

    for(Index = 0; Index < This->Descriptor->FilterPropertySet.FreeKsPropertySetOffset; Index++)
    {
        if (IsEqualGUIDAligned(&Property->Set, This->Descriptor->FilterPropertySet.Properties[Index].Set))
        {
            FastPropertyHandler(FileObject, (PKSPROPERTY)InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, StatusBlock,
                                1,
                                &This->Descriptor->FilterPropertySet.Properties[Index],
                                This->Descriptor, This->SubDevice);
        }
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterTopology_fnFastRead(
    IN IPortFilterTopology* iface,
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

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterTopology_fnFastWrite(
    IN IPortFilterTopology* iface,
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

/*
 * @implemented
 */
static
NTSTATUS
NTAPI
IPortFilterTopology_fnInit(
    IN IPortFilterTopology* iface,
    IN IPortTopology* Port)
{
    ISubdevice * ISubDevice;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;
    IPortFilterTopologyImpl * This = (IPortFilterTopologyImpl*)iface;

    /* get our private interface */
    Status = Port->lpVtbl->QueryInterface(Port, &IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* get the subdevice descriptor */
    Status = ISubDevice->lpVtbl->GetDescriptor(ISubDevice, &Descriptor);

    /* store subdevice interface */
    This->SubDevice = ISubDevice;

    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* save descriptor */
    This->Descriptor = Descriptor;

    /* store port object */
    This->Port = Port;

    return STATUS_SUCCESS;
}

static IPortFilterTopologyVtbl vt_IPortFilterTopology =
{
    IPortFilterTopology_fnQueryInterface,
    IPortFilterTopology_fnAddRef,
    IPortFilterTopology_fnRelease,
    IPortFilterTopology_fnNewIrpTarget,
    IPortFilterTopology_fnDeviceIoControl,
    IPortFilterTopology_fnRead,
    IPortFilterTopology_fnWrite,
    IPortFilterTopology_fnFlush,
    IPortFilterTopology_fnClose,
    IPortFilterTopology_fnQuerySecurity,
    IPortFilterTopology_fnSetSecurity,
    IPortFilterTopology_fnFastDeviceIoControl,
    IPortFilterTopology_fnFastRead,
    IPortFilterTopology_fnFastWrite,
    IPortFilterTopology_fnInit
};

NTSTATUS 
NewPortFilterTopology(
    OUT IPortFilterTopology ** OutFilter)
{
    IPortFilterTopologyImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortFilterTopologyImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortFilterTopology */
    This->ref = 1;
    This->lpVtbl = &vt_IPortFilterTopology;

    /* return result */
    *OutFilter = (IPortFilterTopology*)&This->lpVtbl;

    return STATUS_SUCCESS;
}
