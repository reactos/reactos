#include "private.h"

typedef struct
{
    IPortFilterWaveCyclicVtbl *lpVtbl;

    LONG ref;

    IPortWaveCyclic* Port;
    IPortPinWaveCyclic * Pin;

}IPortFilterWaveCyclicImpl;

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnQueryInterface(
    IPortFilterWaveCyclic* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IIrpTarget) || 
        //IsEqualGUIDAligned(refiid, &IID_IPortFilterWaveCyclic) ||
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
IPortFilterWaveCyclic_fnAddRef(
    IPortFilterWaveCyclic* iface)
{
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    return InterlockedIncrement(&This->ref);
}

/*
 * @implemented
 */
ULONG
NTAPI
IPortFilterWaveCyclic_fnRelease(
    IPortFilterWaveCyclic* iface)
{
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

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
IPortFilterWaveCyclic_fnNewIrpTarget(
    IN IPortFilterWaveCyclic* iface,
    OUT struct IIrpTarget **OutTarget,
    IN WCHAR * Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    ISubdevice * ISubDevice;
    NTSTATUS Status;
    IPortPinWaveCyclic * Pin;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    PKSPIN_CONNECT ConnectDetails;
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl *)iface;

    ASSERT(This->Port);

    DPRINT("IPortFilterWaveCyclic_fnNewIrpTarget entered\n");

    Status = This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    Status = ISubDevice->lpVtbl->GetDescriptor(ISubDevice, &Descriptor);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    Status = PcValidateConnectRequest(Irp, &Descriptor->Factory, &ConnectDetails);
    if (!NT_SUCCESS(Status))
    {
        ISubDevice->lpVtbl->Release(ISubDevice);
        return STATUS_UNSUCCESSFUL;
    }

    ISubDevice->lpVtbl->Release(ISubDevice);

    Status = NewPortPinWaveCyclic(&Pin);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = Pin->lpVtbl->Init(Pin, This->Port, iface, ConnectDetails, &Descriptor->Factory.KsPinDescriptor[ConnectDetails->PinId]);
    if (!NT_SUCCESS(Status))
    {
        Pin->lpVtbl->Release(Pin);
        return Status;
    }

    /* store pin handle */
    This->Pin = Pin;

    /* store result */
    *OutTarget = (IIrpTarget*)Pin;
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnDeviceIoControl(
    IN IPortFilterWaveCyclic* iface,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    ISubdevice *SubDevice = NULL;
    SUBDEVICE_DESCRIPTOR * Descriptor = NULL;
	NTSTATUS Status;
#if defined(DBG)
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl *)iface;
#endif

    IoStack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(IoStack->Parameters.DeviceIoControl.IoControlCode == IOCTL_KS_PROPERTY);
    ASSERT(This->Port->lpVtbl->QueryInterface(This->Port, &IID_ISubdevice, (PVOID*)&SubDevice) == STATUS_SUCCESS);
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
IPortFilterWaveCyclic_fnRead(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnWrite(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnFlush(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnClose(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnQuerySecurity(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnSetSecurity(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnFastDeviceIoControl(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnFastRead(
    IN IPortFilterWaveCyclic* iface,
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
IPortFilterWaveCyclic_fnFastWrite(
    IN IPortFilterWaveCyclic* iface,
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
static
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnInit(
    IN IPortFilterWaveCyclic* iface,
    IN IPortWaveCyclic* Port)
{
    IPortFilterWaveCyclicImpl * This = (IPortFilterWaveCyclicImpl*)iface;

    This->Port = Port;

    /* increment reference count */
    iface->lpVtbl->AddRef(iface);

    return STATUS_SUCCESS;
}

static IPortFilterWaveCyclicVtbl vt_IPortFilterWaveCyclic =
{
    IPortFilterWaveCyclic_fnQueryInterface,
    IPortFilterWaveCyclic_fnAddRef,
    IPortFilterWaveCyclic_fnRelease,
    IPortFilterWaveCyclic_fnNewIrpTarget,
    IPortFilterWaveCyclic_fnDeviceIoControl,
    IPortFilterWaveCyclic_fnRead,
    IPortFilterWaveCyclic_fnWrite,
    IPortFilterWaveCyclic_fnFlush,
    IPortFilterWaveCyclic_fnClose,
    IPortFilterWaveCyclic_fnQuerySecurity,
    IPortFilterWaveCyclic_fnSetSecurity,
    IPortFilterWaveCyclic_fnFastDeviceIoControl,
    IPortFilterWaveCyclic_fnFastRead,
    IPortFilterWaveCyclic_fnFastWrite,
    IPortFilterWaveCyclic_fnInit
};

NTSTATUS 
NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter)
{
    IPortFilterWaveCyclicImpl * This;

    This = AllocateItem(NonPagedPool, sizeof(IPortFilterWaveCyclicImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortFilterWaveCyclic */
    This->ref = 1;
    This->lpVtbl = &vt_IPortFilterWaveCyclic;

    /* return result */
    *OutFilter = (IPortFilterWaveCyclic*)&This->lpVtbl;

    return STATUS_SUCCESS;
}


