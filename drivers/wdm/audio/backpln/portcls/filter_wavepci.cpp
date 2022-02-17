/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_wavepci.cpp
 * PURPOSE:         portcls wave pci filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CPortFilterWavePci : public CUnknownImpl<IPortFilterWavePci>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortFilterPci;
    CPortFilterWavePci(IUnknown *OuterUnknown){}
    virtual ~CPortFilterWavePci(){}

protected:
    IPortWavePci* m_Port;
    IPortPinWavePci ** m_Pins;
    SUBDEVICE_DESCRIPTOR * m_Descriptor;
};

NTSTATUS
NTAPI
CPortFilterWavePci::QueryInterface(

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
        *Output = PUNKNOWN(m_Port);
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortFilterWavePci::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortPinWavePci * Pin;
    PKSPIN_CONNECT ConnectDetails;

#if 0
    ASSERT(m_Port);
    ASSERT(m_Descriptor);
    ASSERT(m_Pins);
#endif

    DPRINT("CPortFilterWavePci::NewIrpTarget entered\n");

    // let's verify the connection request
    Status = PcValidateConnectRequest(Irp, &m_Descriptor->Factory, &ConnectDetails);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (m_Pins[ConnectDetails->PinId] &&
        (m_Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount == m_Descriptor->Factory.Instances[ConnectDetails->PinId].MaxFilterInstanceCount))
    {
        // no available instance
        return STATUS_UNSUCCESSFUL;
    }


    // now create the pin
    Status = NewPortPinWavePci(&Pin);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // initialize the pin
    Status = Pin->Init(m_Port, this, ConnectDetails, &m_Descriptor->Factory.KsPinDescriptor[ConnectDetails->PinId], GetDeviceObjectFromPortWavePci(m_Port));
    if (!NT_SUCCESS(Status))
    {
        Pin->Release();
        return Status;
    }

    // store pin
    m_Pins[ConnectDetails->PinId] = Pin;

    // store result
    *OutTarget = (IIrpTarget*)Pin;

    // increment current instance count
    m_Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount++;

    return Status;
}

NTSTATUS
NTAPI
CPortFilterWavePci::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        DPRINT("Unhandled function %lx Length %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode, IoStack->Parameters.DeviceIoControl.InputBufferLength);

        Irp->IoStatus.Status = STATUS_SUCCESS;

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
CPortFilterWavePci::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWavePci::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWavePci::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWavePci::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortFilterWavePci::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWavePci::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortFilterWavePci::FastDeviceIoControl(
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
CPortFilterWavePci::FastRead(
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
CPortFilterWavePci::FastWrite(
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
CPortFilterWavePci::Init(
    IN IPortWavePci* Port)
{
    ISubdevice * ISubDevice;
    SUBDEVICE_DESCRIPTOR * Descriptor;
    NTSTATUS Status;

    m_Port = Port;

    // get our private interface
    Status = m_Port->QueryInterface(IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    // get the subdevice descriptor
    Status = ISubDevice->GetDescriptor(&Descriptor);

    // release subdevice interface
    ISubDevice->Release();

    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    // save descriptor
    m_Descriptor = Descriptor;

    // allocate pin array
    m_Pins = (IPortPinWavePci**)AllocateItem(NonPagedPool, Descriptor->Factory.PinDescriptorCount * sizeof(IPortPinWavePci*), TAG_PORTCLASS);

    if (!m_Pins)
        return STATUS_UNSUCCESSFUL;

    // increment reference count
    Port->AddRef();

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortFilterWavePci::FreePin(
    IN struct IPortPinWavePci* Pin)
{
    ULONG Index;

    for(Index = 0; Index < m_Descriptor->Factory.PinDescriptorCount; Index++)
    {
        if (m_Pins[Index] == Pin)
        {
            m_Descriptor->Factory.Instances[Index].CurrentPinInstanceCount--;
            m_Pins[Index] = NULL;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
NewPortFilterWavePci(
    OUT IPortFilterWavePci ** OutFilter)
{
    CPortFilterWavePci * This;

    This = new(NonPagedPool, TAG_PORTCLASS)CPortFilterWavePci(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutFilter = (IPortFilterWavePci*)This;

    return STATUS_SUCCESS;
}
