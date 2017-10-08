/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_dmus.cpp
 * PURPOSE:         portcls wave pci filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CPortFilterDMus : public IPortFilterDMus
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    STDMETHODIMP_(ULONG) AddRef()
    {
        InterlockedIncrement(&m_Ref);
        return m_Ref;
    }
    STDMETHODIMP_(ULONG) Release()
    {
        InterlockedDecrement(&m_Ref);

        if (!m_Ref)
        {
            delete this;
            return 0;
        }
        return m_Ref;
    }
    IMP_IPortFilterDMus;
    CPortFilterDMus(IUnknown *OuterUnknown){}
    virtual ~CPortFilterDMus(){}

protected:
    IPortDMus* m_Port;
    IPortPinDMus ** m_Pins;
    SUBDEVICE_DESCRIPTOR * m_Descriptor;
    LONG m_Ref;
};

NTSTATUS
NTAPI
CPortFilterDMus::QueryInterface(
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
CPortFilterDMus::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortPinDMus * Pin;
    PKSPIN_CONNECT ConnectDetails;

    PC_ASSERT(m_Port);
    PC_ASSERT(m_Descriptor);
    PC_ASSERT(m_Pins);

    DPRINT("CPortFilterDMus::NewIrpTarget entered\n");

    // let's verify the connection request
    Status = PcValidateConnectRequest(Irp, &m_Descriptor->Factory, &ConnectDetails);
    if (!NT_SUCCESS(Status))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (m_Pins[ConnectDetails->PinId] && m_Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount)
    {
        // release existing instance
        PC_ASSERT(0);
    }

    // now create the pin
    Status = NewPortPinDMus(&Pin);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // initialize the pin
    Status = Pin->Init(m_Port, this, ConnectDetails, &m_Descriptor->Factory.KsPinDescriptor[ConnectDetails->PinId], DeviceObject);
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
CPortFilterDMus::DeviceIoControl(
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
CPortFilterDMus::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterDMus::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterDMus::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterDMus::Close(
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
CPortFilterDMus::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterDMus::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortFilterDMus::FastDeviceIoControl(
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
CPortFilterDMus::FastRead(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOLEAN
NTAPI
CPortFilterDMus::FastWrite(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK StatusBlock,
    IN PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}

NTSTATUS
NTAPI
CPortFilterDMus::Init(
    IN IPortDMus* Port)
{
    ISubdevice * ISubDevice;
    NTSTATUS Status;

    m_Port = Port;

    // get our private interface
    Status = m_Port->QueryInterface(IID_ISubdevice, (PVOID*)&ISubDevice);
    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    // get the subdevice descriptor
    Status = ISubDevice->GetDescriptor(&m_Descriptor);

    // release subdevice interface
    ISubDevice->Release();

    if (!NT_SUCCESS(Status))
        return STATUS_UNSUCCESSFUL;

    // allocate pin array
    m_Pins = (IPortPinDMus**)AllocateItem(NonPagedPool, m_Descriptor->Factory.PinDescriptorCount * sizeof(IPortPinDMus*), TAG_PORTCLASS);

    if (!m_Pins)
        return STATUS_UNSUCCESSFUL;

    // increment reference count
    Port->AddRef();

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
CPortFilterDMus::FreePin(
    IN struct IPortPinDMus* Pin)
{
    ULONG Index;

    for(Index = 0; Index < m_Descriptor->Factory.PinDescriptorCount; Index++)
    {
        if (m_Pins[Index] == Pin)
        {
            m_Pins[Index]->Release();
            m_Pins[Index] = NULL;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_UNSUCCESSFUL;
}

VOID
NTAPI
CPortFilterDMus::NotifyPins()
{
    ULONG Index;

    DPRINT("Notifying %u pins\n", m_Descriptor->Factory.PinDescriptorCount);

    for(Index = 0; Index < m_Descriptor->Factory.PinDescriptorCount; Index++)
    {
        m_Pins[Index]->Notify();
    }
}


NTSTATUS 
NewPortFilterDMus(
    OUT PPORTFILTERDMUS * OutFilter)
{
    CPortFilterDMus * This;

    This = new(NonPagedPool, TAG_PORTCLASS) CPortFilterDMus(NULL);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutFilter = (CPortFilterDMus*)This;

    return STATUS_SUCCESS;
}


