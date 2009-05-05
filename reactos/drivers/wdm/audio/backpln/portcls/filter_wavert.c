/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_wavert.c
 * PURPOSE:         portcls wave RT filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct
{
    IPortFilterWaveRTVtbl *lpVtbl;

    LONG ref;

    IPortWaveRT* Port;
    IPortPinWaveRT ** Pins;
    SUBDEVICE_DESCRIPTOR * Descriptor;

}IPortFilterWaveRTImpl;

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveRT_fnQueryInterface(
    IPortFilterWaveRT* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl*)iface;

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
IPortFilterWaveRT_fnAddRef(
    IPortFilterWaveRT* iface)
{
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterWaveRT_fnRelease(
    IPortFilterWaveRT* iface)
{
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl*)iface;

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
IPortFilterWaveRT_fnNewIrpTarget(
    IN IPortFilterWaveRT* iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortPinWaveRT * Pin;
    PKSPIN_CONNECT ConnectDetails;
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl *)iface;

    ASSERT(This->Port);
    ASSERT(This->Descriptor);
    ASSERT(This->Pins);

    DPRINT("IPortFilterWaveRT_fnNewIrpTarget entered\n");

    /* let's verify the connection request */
    Status = PcValidateConnectRequest(Irp, &This->Descriptor->Factory, &ConnectDetails);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (This->Pins[ConnectDetails->PinId] && This->Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount)
    {
        /* release existing instance */
        ASSERT(0);
        This->Pins[ConnectDetails->PinId]->lpVtbl->Close(This->Pins[ConnectDetails->PinId], DeviceObject, NULL);
    }

    /* now create the pin */
    Status = NewPortPinWaveRT(&Pin);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* initialize the pin */
    Status = Pin->lpVtbl->Init(Pin, This->Port, iface, ConnectDetails, &This->Descriptor->Factory.KsPinDescriptor[ConnectDetails->PinId], GetDeviceObjectFromPortWaveRT(This->Port));
    if (!NT_SUCCESS(Status))
    {
        Pin->lpVtbl->Release(Pin);
        return Status;
    }

    /* release existing pin */
    if (This->Pins[ConnectDetails->PinId])
    {
        This->Pins[ConnectDetails->PinId]->lpVtbl->Release(This->Pins[ConnectDetails->PinId]);
    }
    /* store pin */
    This->Pins[ConnectDetails->PinId] = Pin;

    /* store result */
    *OutTarget = (IIrpTarget*)Pin;

    /* increment current instance count */
    This->Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount++;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveRT_fnDeviceIoControl(
    IN IPortFilterWaveRT* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    ISubdevice *SubDevice = NULL;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl *)iface;

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY);
    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&SubDevice);
    ASSERT(Status == STATUS_SUCCESS);
    ASSERT(SubDevice != NULL);

    Status = SubDevice->lpVtbl->GetDescriptor(SubDevice, &Descriptor);
    ASSERT(Status == STATUS_SUCCESS);
    ASSERT(Descriptor != NULL);

    SubDevice->lpVtbl->Release(SubDevice);

    return PcPropertyHandler(Irp, Descriptor);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveRT_fnRead(
    IN IPortFilterWaveRT* iface,
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
IPortFilterWaveRT_fnWrite(
    IN IPortFilterWaveRT* iface,
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
IPortFilterWaveRT_fnFlush(
    IN IPortFilterWaveRT* iface,
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
IPortFilterWaveRT_fnClose(
    IN IPortFilterWaveRT* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    ULONG Index;
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl *)iface;

    for(Index = 0; Index < This->Descriptor->Factory.PinDescriptorCount; Index++)
    {
        if (This->Pins[Index])
        {
            This->Pins[Index]->lpVtbl->Close(This->Pins[Index], DeviceObject, NULL);
        }
    }


    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveRT_fnQuerySecurity(
    IN IPortFilterWaveRT* iface,
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
IPortFilterWaveRT_fnSetSecurity(
    IN IPortFilterWaveRT* iface,
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
IPortFilterWaveRT_fnFastDeviceIoControl(
    IN IPortFilterWaveRT* iface,
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
    NTSTATUS Status;
    ISubdevice * SubDevice = NULL;
    PSUBDEVICE_DESCRIPTOR Descriptor = NULL;
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl *)iface;

    Property = (PKSPROPERTY)InputBuffer;

    if (InputBufferLength < sizeof(KSPROPERTY))
        return FALSE;


    /* get private interface */
    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&SubDevice);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* get descriptor */
    Status = SubDevice->lpVtbl->GetDescriptor(SubDevice, &Descriptor);
    if (!NT_SUCCESS(Status))
    {
        SubDevice->lpVtbl->Release(SubDevice);
        return FALSE;
    }

    for(Index = 0; Index < Descriptor->FilterPropertySet.FreeKsPropertySetOffset; Index++)
    {
        if (IsEqualGUIDAligned(&Property->Set, Descriptor->FilterPropertySet.Properties[Index].Set))
        {
            FastPropertyHandler(FileObject, (PKSPROPERTY)InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, StatusBlock,
                                1,
                                &Descriptor->FilterPropertySet.Properties[Index],
                                Descriptor, SubDevice);
        }
    }
    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IPortFilterWaveRT_fnFastRead(
    IN IPortFilterWaveRT* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
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
IPortFilterWaveRT_fnFastWrite(
    IN IPortFilterWaveRT* iface,
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED
    return FALSE;
}

/*
 * @implemented
 */
static
NTSTATUS
NTAPI
IPortFilterWaveRT_fnInit(
    IN IPortFilterWaveRT* iface,
    IN IPortWaveRT* Port)
{
    ISubdevice * ISubDevice;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;
    IPortFilterWaveRTImpl * This = (IPortFilterWaveRTImpl*)iface;

    This->Port = Port;

    /* get our private interface */
    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* get the subdevice descriptor */
    Status = ISubDevice->lpVtbl->GetDescriptor(ISubDevice, &Descriptor);

    /* release subdevice interface */
    ISubDevice->lpVtbl->Release(ISubDevice);

    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    /* save descriptor */
    This->Descriptor = Descriptor;

    /* allocate pin array */
    This->Pins = AllocateItem(NonPagedPool, Descriptor->Factory.PinDescriptorCount * sizeof(IPortPinWaveRT*), TAG_PORTCLASS);

    if (!This->Pins)
        return STATUS_UNSUCCESSFUL;

    /* increment reference count */
    Port->lpVtbl->AddRef(Port);

    return STATUS_SUCCESS;
}

static IPortFilterWaveRTVtbl vt_IPortFilterWaveRT =
{
    IPortFilterWaveRT_fnQueryInterface,
    IPortFilterWaveRT_fnAddRef,
    IPortFilterWaveRT_fnRelease,
    IPortFilterWaveRT_fnNewIrpTarget,
    IPortFilterWaveRT_fnDeviceIoControl,
    IPortFilterWaveRT_fnRead,
    IPortFilterWaveRT_fnWrite,
    IPortFilterWaveRT_fnFlush,
    IPortFilterWaveRT_fnClose,
    IPortFilterWaveRT_fnQuerySecurity,
    IPortFilterWaveRT_fnSetSecurity,
    IPortFilterWaveRT_fnFastDeviceIoControl,
    IPortFilterWaveRT_fnFastRead,
    IPortFilterWaveRT_fnFastWrite,
    IPortFilterWaveRT_fnInit
};

NTSTATUS 
NewPortFilterWaveRT(
    OUT IPortFilterWaveRT ** OutFilter)
{
    IPortFilterWaveRTImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortFilterWaveRTImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortFilterWaveRT */
    This->ref = 1;
    This->lpVtbl = &vt_IPortFilterWaveRT;

    /* return result */
    *OutFilter = (IPortFilterWaveRT*)&This->lpVtbl;

    return STATUS_SUCCESS;
}
