/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_topology.c
 * PURPOSE:         portcls topology filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CPortFilterTopology : public CUnknownImpl<IPortFilterTopology>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortFilterTopology;
    CPortFilterTopology(IUnknown *OuterUnknown){}
    virtual ~CPortFilterTopology(){}

protected:
    IPortTopology * m_Port;
    SUBDEVICE_DESCRIPTOR * m_Descriptor;
    ISubdevice * m_SubDevice;
};

NTSTATUS
NTAPI
CPortFilterTopology::QueryInterface(
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
    else if (IsEqualGUIDAligned(refiid, IID_IPort))
    {
        *Output = PVOID(PUNKNOWN(m_Port));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortFilterTopology::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    DPRINT("CPortFilterTopology::NewIrpTarget entered\n");

    return STATUS_NOT_SUPPORTED;
}

NTSTATUS
NTAPI
CPortFilterTopology::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        DPRINT1("Unhandled function %lx Length %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode, IoStack->Parameters.DeviceIoControl.InputBufferLength);
        Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_SUCCESS;
    }

    Status = PcHandlePropertyWithTable(Irp, m_Descriptor->FilterPropertySetCount, m_Descriptor->FilterPropertySet, m_Descriptor);
    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        DPRINT("Result %x Length %u\n", Status, Irp->IoStatus.Information);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    return Status;

}

NTSTATUS
NTAPI
CPortFilterTopology::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterTopology::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterTopology::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterTopology::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;

    // FIXME handle DirectSound

#if 0
//FIXME
    if (m_ref == 1)
    {
        // release reference to port
        This->SubDevice->lpVtbl->Release(This->SubDevice);

        // time to shutdown the audio system
        Status = This->SubDevice->lpVtbl->ReleaseChildren(This->SubDevice);
    }
#endif

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortFilterTopology::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterTopology::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortFilterTopology::FastDeviceIoControl(
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
CPortFilterTopology::FastRead(
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
CPortFilterTopology::FastWrite(
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
CPortFilterTopology::Init(
    IN IPortTopology* Port)
{
    ISubdevice * ISubDevice;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;

    // get our private interface
    Status = Port->QueryInterface(IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    // get the subdevice descriptor
    Status = ISubDevice->GetDescriptor(&Descriptor);

    // store subdevice interface
    m_SubDevice = ISubDevice;

    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    // save descriptor
    m_Descriptor = Descriptor;

    // store port object
    m_Port = Port;

    return STATUS_SUCCESS;
}

NTSTATUS
NewPortFilterTopology(
    OUT IPortFilterTopology ** OutFilter)
{
    CPortFilterTopology * This;

    This = new(NonPagedPool, TAG_PORTCLASS)CPortFilterTopology(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutFilter = (CPortFilterTopology*)This;

    return STATUS_SUCCESS;
}
