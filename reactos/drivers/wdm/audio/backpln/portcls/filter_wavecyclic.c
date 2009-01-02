#include "private.h"

typedef struct
{
    IPortFilterWaveCyclicVtbl *lpVtbl;

    LONG ref;

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
        ExFreePoolWithTag(This, TAG_PORTCLASS);
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
    IN PDEVICE_OBJECT * DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{

    return STATUS_UNSUCCESSFUL;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
IPortFilterWaveCyclic_fnDeviceIoControl(
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
    IPortFilterWaveCyclic_fnFastWrite
};


NTSTATUS NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter)
{
    IPortFilterWaveCyclicImpl * This;

    This = ExAllocatePoolWithTag(NonPagedPool, sizeof(IPortFilterWaveCyclicImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* initialize IPortFilterWaveCyclic */
    This->ref = 1;
    This->lpVtbl = &vt_IPortFilterWaveCyclic;

    /* return result */
    *OutFilter = (IPortFilterWaveCyclic*)&This->lpVtbl;

    return STATUS_SUCCESS;
}


