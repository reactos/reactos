/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/filter_wavecyclic.c
 * PURPOSE:         portcls wave cyclic filter
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#define NDEBUG
#include <debug.h>

class CPortFilterWaveCyclic : public CUnknownImpl<IPortFilterWaveCyclic>
{
public:
    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IPortFilterWaveCyclic;
    CPortFilterWaveCyclic(IUnknown *OuterUnknown){}
    virtual ~CPortFilterWaveCyclic(){}

protected:
    IPortWaveCyclic* m_Port;
    IPortPinWaveCyclic ** m_Pins;
    SUBDEVICE_DESCRIPTOR * m_Descriptor;
    ISubdevice * m_SubDevice;
};

NTSTATUS
NTAPI
CPortFilterWaveCyclic::QueryInterface(
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
        PPORT(*Output)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
CPortFilterWaveCyclic::NewIrpTarget(
    OUT struct IIrpTarget **OutTarget,
    IN PCWSTR Name,
    IN PUNKNOWN Unknown,
    IN POOL_TYPE PoolType,
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN KSOBJECT_CREATE *CreateObject)
{
    NTSTATUS Status;
    IPortPinWaveCyclic * Pin;
    PKSPIN_CONNECT ConnectDetails;

#if 0
    ASSERT(m_Port);
    ASSERT(m_Descriptor);
    ASSERT(m_Pins);
#endif

    DPRINT("CPortFilterWaveCyclic::NewIrpTarget entered\n");

    // let's verify the connection request
    Status = PcValidateConnectRequest(Irp, &m_Descriptor->Factory, &ConnectDetails);
    if (!NT_SUCCESS(Status))
    {
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    if (m_Pins[ConnectDetails->PinId] &&
        (m_Descriptor->Factory.Instances[ConnectDetails->PinId].CurrentPinInstanceCount == m_Descriptor->Factory.Instances[ConnectDetails->PinId].MaxFilterInstanceCount))
    {
        // release existing instance
        ASSERT(FALSE);
        return STATUS_UNSUCCESSFUL;
    }

    // now create the pin
    Status = NewPortPinWaveCyclic(&Pin);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // initialize the pin
    Status = Pin->Init(m_Port, this, ConnectDetails, &m_Descriptor->Factory.KsPinDescriptor[ConnectDetails->PinId]);
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
CPortFilterWaveCyclic::DeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack;
    NTSTATUS Status;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    if (IoStack->Parameters.DeviceIoControl.IoControlCode != IOCTL_KS_PROPERTY)
    {
        DPRINT("Unhandled function %lx Length %x\n", IoStack->Parameters.DeviceIoControl.IoControlCode, IoStack->Parameters.DeviceIoControl.InputBufferLength);

        Irp->IoStatus.Status = STATUS_NOT_FOUND;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return STATUS_NOT_FOUND;
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
CPortFilterWaveCyclic::Read(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWaveCyclic::Write(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWaveCyclic::Flush(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWaveCyclic::Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    //ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;

#if 0
    if (m_ref == 1)
    {
        for(Index = 0; Index < m_Descriptor->Factory.PinDescriptorCount; Index++)
        {
            // all pins should have been closed by now
            ASSERT(m_Pins[Index] == NULL);
        }

        // release reference to port
        m_SubDevice->Release(m_SubDevice);

        // time to shutdown the audio system
        Status = m_SubDevice->ReleaseChildren(m_SubDevice);
    }
#endif

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortFilterWaveCyclic::QuerySecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

NTSTATUS
NTAPI
CPortFilterWaveCyclic::SetSecurity(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    return KsDispatchInvalidDeviceRequest(DeviceObject, Irp);
}

BOOLEAN
NTAPI
CPortFilterWaveCyclic::FastDeviceIoControl(
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
    return KsDispatchFastIoDeviceControlFailure(FileObject, Wait, InputBuffer, InputBufferLength, OutputBuffer, OutputBufferLength, IoControlCode, StatusBlock, DeviceObject);
}

BOOLEAN
NTAPI
CPortFilterWaveCyclic::FastRead(
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
CPortFilterWaveCyclic::FastWrite(
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
CPortFilterWaveCyclic::Init(
    IN IPortWaveCyclic* Port)
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

    // allocate pin array
    m_Pins = (IPortPinWaveCyclic**)AllocateItem(NonPagedPool, Descriptor->Factory.PinDescriptorCount * sizeof(IPortPinWaveCyclic*), TAG_PORTCLASS);

    if (!m_Pins)
        return STATUS_UNSUCCESSFUL;

    // store port driver
    m_Port = Port;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CPortFilterWaveCyclic::FreePin(
    IN IPortPinWaveCyclic * Pin)
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
    ASSERT(FALSE);
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NewPortFilterWaveCyclic(
    OUT IPortFilterWaveCyclic ** OutFilter)
{
    CPortFilterWaveCyclic * This;

    This = new(NonPagedPool, TAG_PORTCLASS)CPortFilterWaveCyclic(NULL);

    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->AddRef();

    // return result
    *OutFilter = (IPortFilterWaveCyclic*)This;

    return STATUS_SUCCESS;
}
