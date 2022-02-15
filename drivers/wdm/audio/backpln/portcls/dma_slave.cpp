/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/dma_init.c
 * PURPOSE:         portcls dma support object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.hpp"

#ifndef YDEBUG
#define NDEBUG
#endif

#include <debug.h>

class CDmaChannelInit : public CUnknownImpl<IDmaChannelInit>
{
public:
    inline
    PVOID
    operator new(
        size_t Size,
        POOL_TYPE PoolType,
        ULONG Tag)
    {
        return ExAllocatePoolWithTag(PoolType, Size, Tag);
    }

    STDMETHODIMP QueryInterface( REFIID InterfaceId, PVOID* Interface);

    IMP_IDmaChannelInit;
    CDmaChannelInit(IUnknown * OuterUnknown) :
        m_pDeviceObject(nullptr),
        m_pAdapter(nullptr),
        m_DmaStarted(FALSE),
        m_MapSize(0),
        m_MapRegisterBase(nullptr),
        m_LastTransferCount(0),
        m_MaximumBufferSize(0),
        m_MaxMapRegisters(0),
        m_AllocatedBufferSize(0),
        m_BufferSize(0),
        m_Address({0}),
        m_Buffer(nullptr),
        m_Mdl(nullptr),
        m_WriteToDevice(FALSE)
    {
    }
    virtual ~CDmaChannelInit(){}

protected:

    PDEVICE_OBJECT m_pDeviceObject;
    PDMA_ADAPTER m_pAdapter;

    BOOL m_DmaStarted;

    ULONG m_MapSize;
    PVOID m_MapRegisterBase;

    ULONG m_LastTransferCount;

    ULONG m_MaximumBufferSize;
    ULONG m_MaxMapRegisters;
    ULONG m_AllocatedBufferSize;
    ULONG m_BufferSize;

    PHYSICAL_ADDRESS m_Address;
    PVOID m_Buffer;
    PMDL m_Mdl;
    BOOLEAN m_WriteToDevice;

    friend IO_ALLOCATION_ACTION NTAPI AdapterControl(IN PDEVICE_OBJECT  DeviceObject, IN PIRP  Irp, IN PVOID  MapRegisterBase, IN PVOID  Context);
};



//---------------------------------------------------------------
// IUnknown methods
//

extern GUID IID_IDmaChannelSlave;

NTSTATUS
NTAPI
CDmaChannelInit::QueryInterface(
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    if (IsEqualGUIDAligned(refiid, IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, IID_IDmaChannel))
        //IsEqualGUIDAligned(refiid, IID_IDmaChannelSlave)) // HACK
    {
        *Output = PVOID(PUNKNOWN(this));
        PUNKNOWN(*Output)->AddRef();
        return STATUS_SUCCESS;
    }
    DPRINT("No interface!!!\n");
    return STATUS_UNSUCCESSFUL;
}

//---------------------------------------------------------------
// IDmaChannel methods
//

NTSTATUS
NTAPI
CDmaChannelInit::AllocateBuffer(
    IN ULONG BufferSize,
    IN PPHYSICAL_ADDRESS  PhysicalAddressConstraint OPTIONAL)
{
    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    // Did the caller already allocate a buffer ?*/
    if (m_Buffer)
    {
        DPRINT("CDmaChannelInit_AllocateBuffer free common buffer first \n");
        return STATUS_UNSUCCESSFUL;
    }

    m_Buffer = m_pAdapter->DmaOperations->AllocateCommonBuffer(m_pAdapter, BufferSize, &m_Address, FALSE);
    if (!m_Buffer)
    {
        DPRINT("CDmaChannelInit_AllocateBuffer fAllocateCommonBuffer failed \n");
        return STATUS_UNSUCCESSFUL;
    }

    m_BufferSize = BufferSize;
    m_AllocatedBufferSize = BufferSize;
    DPRINT("CDmaChannelInit::AllocateBuffer Success Buffer %p BufferSize %u Address %x\n", m_Buffer, BufferSize, m_Address);

    return STATUS_SUCCESS;
}

ULONG
NTAPI
CDmaChannelInit::AllocatedBufferSize()
{
    DPRINT("CDmaChannelInit_AllocatedBufferSize: this %p BufferSize %u\n", this, m_BufferSize);
    return m_AllocatedBufferSize;
}

VOID
NTAPI
CDmaChannelInit::CopyFrom(
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    DPRINT("CDmaChannelInit_CopyFrom: this %p Destination %p Source %p ByteCount %u\n", this, Destination, Source, ByteCount);

    CopyTo(Destination, Source, ByteCount);
}

VOID
NTAPI
CDmaChannelInit::CopyTo(
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    DPRINT("CDmaChannelInit_CopyTo: this %p Destination %p Source %p ByteCount %u\n", this, Destination, Source, ByteCount);
    RtlCopyMemory(Destination, Source, ByteCount);
}

VOID
NTAPI
CDmaChannelInit::FreeBuffer()
{
    DPRINT("CDmaChannelInit_FreeBuffer: this %p\n", this);

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!m_Buffer)
    {
        DPRINT("CDmaChannelInit_FreeBuffer allocate common buffer first \n");
        return;
    }

    m_pAdapter->DmaOperations->FreeCommonBuffer(m_pAdapter, m_AllocatedBufferSize, m_Address, m_Buffer, FALSE);
    m_Buffer = NULL;
    m_AllocatedBufferSize = 0;
    m_Address.QuadPart = 0LL;

    if (m_Mdl)
    {
        IoFreeMdl(m_Mdl);
        m_Mdl = NULL;
    }
}

PADAPTER_OBJECT
NTAPI
CDmaChannelInit::GetAdapterObject()
{
    DPRINT("CDmaChannelInit_GetAdapterObject: this %p\n", this);
    return (PADAPTER_OBJECT)m_pAdapter;
}

ULONG
NTAPI
CDmaChannelInit::MaximumBufferSize()
{
    DPRINT("CDmaChannelInit_MaximumBufferSize: this %p\n", this);
    return m_MaximumBufferSize;
}

#ifdef _MSC_VER

PHYSICAL_ADDRESS
NTAPI
CDmaChannelInit::PhysicalAddress()
{
    DPRINT("CDmaChannelInit_PhysicalAddress: this %p Virtual %p Physical High %x Low %x%\n", this, m_Buffer, m_Address.HighPart, m_Address.LowPart);

    return m_Address;
}

#else

PHYSICAL_ADDRESS
NTAPI
CDmaChannelInit::PhysicalAddress(
    PPHYSICAL_ADDRESS Address)
{
    DPRINT("CDmaChannelInit_PhysicalAddress: this %p Virtual %p Physical High %x Low %x%\n", this, m_Buffer, m_Address.HighPart, m_Address.LowPart);

    PHYSICAL_ADDRESS Result;

    Address->QuadPart = m_Address.QuadPart;
    Result.QuadPart = (ULONG_PTR)Address;
    return Result;
}


#endif

VOID
NTAPI
CDmaChannelInit::SetBufferSize(
    IN ULONG BufferSize)
{
    DPRINT("CDmaChannelInit_SetBufferSize: this %p\n", this);
    m_BufferSize = BufferSize;

}

ULONG
NTAPI
CDmaChannelInit::BufferSize()
{
    DPRINT("BufferSize %u\n", m_BufferSize);
    PC_ASSERT(m_BufferSize);
    return m_BufferSize;
}


PVOID
NTAPI
CDmaChannelInit::SystemAddress()
{
    DPRINT("CDmaChannelInit_SystemAddress: this %p\n", this);
    return m_Buffer;
}

ULONG
NTAPI
CDmaChannelInit::TransferCount()
{
    DPRINT("CDmaChannelInit_TransferCount: this %p\n", this);
    return m_LastTransferCount;
}

ULONG
NTAPI
CDmaChannelInit::ReadCounter()
{
    ULONG Counter;

    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    Counter = m_pAdapter->DmaOperations->ReadDmaCounter(m_pAdapter);

    if (!m_DmaStarted || Counter >= m_LastTransferCount)
        Counter = 0;

    DPRINT("ReadCounter %u\n", Counter);

    return Counter;
}

IO_ALLOCATION_ACTION
NTAPI
AdapterControl(
    IN PDEVICE_OBJECT  DeviceObject,
    IN PIRP  Irp,
    IN PVOID  MapRegisterBase,
    IN PVOID  Context)
{
    ULONG Length;
    CDmaChannelInit * This = (CDmaChannelInit*)Context;

    Length = This->m_MapSize;
    This->m_MapRegisterBase = MapRegisterBase;

    This->m_pAdapter->DmaOperations->MapTransfer(This->m_pAdapter,
                                                This->m_Mdl,
                                                MapRegisterBase,
                                                (PVOID)((ULONG_PTR)This->m_Mdl->StartVa + This->m_Mdl->ByteOffset),
                                                &Length,
                                                This->m_WriteToDevice);

    if (Length == This->m_BufferSize)
    {
        This->m_DmaStarted = TRUE;
    }

   return KeepObject;
}

NTSTATUS
NTAPI
CDmaChannelInit::Start(
    ULONG  MapSize,
    BOOLEAN WriteToDevice)
{
    NTSTATUS Status;
    ULONG MapRegisters;
    KIRQL OldIrql;

    DPRINT("CDmaChannelInit_Start: this %p\n", this);

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (m_DmaStarted)
        return STATUS_UNSUCCESSFUL;

    if (!m_Mdl)
    {
        m_Mdl = IoAllocateMdl(m_Buffer, m_MaximumBufferSize, FALSE, FALSE, NULL);
        if (!m_Mdl)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        MmBuildMdlForNonPagedPool(m_Mdl);
    }

    m_MapSize = MapSize;
    m_WriteToDevice = WriteToDevice;
    m_LastTransferCount = MapSize;

    //FIXME
    // synchronize access
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    MapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(m_Buffer, MapSize);
    Status = m_pAdapter->DmaOperations->AllocateAdapterChannel(m_pAdapter, m_pDeviceObject, MapRegisters, AdapterControl, (PVOID)this);
    KeLowerIrql(OldIrql);

    if(!NT_SUCCESS(Status))
        m_LastTransferCount = 0;

    return Status;
}

NTSTATUS
NTAPI
CDmaChannelInit::Stop()
{
    KIRQL OldIrql;

    DPRINT("CDmaChannelInit::Stop: this %p\n", this);
    PC_ASSERT_IRQL(DISPATCH_LEVEL);

    if (!m_DmaStarted)
        return STATUS_SUCCESS;

    m_pAdapter->DmaOperations->FlushAdapterBuffers(m_pAdapter,
                                                       m_Mdl,
                                                       m_MapRegisterBase,
                                                       (PVOID)((ULONG_PTR)m_Mdl->StartVa + m_Mdl->ByteOffset),
                                                       m_MapSize,
                                                       m_WriteToDevice);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    m_pAdapter->DmaOperations->FreeAdapterChannel(m_pAdapter);

    KeLowerIrql(OldIrql);

    m_DmaStarted = FALSE;

    IoFreeMdl(m_Mdl);
    m_Mdl = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CDmaChannelInit::WaitForTC(
    ULONG  Timeout)
{
    ULONG RetryCount;
    ULONG BytesRemaining;
    ULONG PrevBytesRemaining;

    PC_ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    BytesRemaining = m_pAdapter->DmaOperations->ReadDmaCounter(m_pAdapter);
    if (!BytesRemaining)
    {
        return STATUS_SUCCESS;
    }

    RetryCount = Timeout / 10;
    PrevBytesRemaining = 0xFFFFFFFF;
    do
    {
        BytesRemaining = m_pAdapter->DmaOperations->ReadDmaCounter(m_pAdapter);

        if (!BytesRemaining)
            break;

        if (PrevBytesRemaining == BytesRemaining)
            break;

        KeStallExecutionProcessor(10);
        PrevBytesRemaining = BytesRemaining;

    }while(RetryCount-- >= 1);

    if (BytesRemaining)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;

}

NTSTATUS
NTAPI
CDmaChannelInit::Init(
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN PDEVICE_OBJECT DeviceObject)
{
    INTERFACE_TYPE BusType;
    NTSTATUS Status;
    PDMA_ADAPTER Adapter;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    ULONG MapRegisters;
    ULONG ResultLength;

    // Get bus type
    Status = IoGetDeviceProperty(DeviceObject, DevicePropertyLegacyBusType, sizeof(BusType), (PVOID)&BusType, &ResultLength);
    if (NT_SUCCESS(Status))
    {
        DeviceDescription->InterfaceType = BusType;
    }
    // Fetch device extension
    DeviceExt = (PPCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    // Acquire dma adapter
    Adapter = IoGetDmaAdapter(DeviceExt->PhysicalDeviceObject, DeviceDescription, &MapRegisters);
    if (!Adapter)
    {
        FreeItem(this, TAG_PORTCLASS);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    // initialize object
    m_pAdapter = Adapter;
    m_pDeviceObject = DeviceObject;
    m_MaximumBufferSize = DeviceDescription->MaximumLength;
    m_MaxMapRegisters = MapRegisters;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
PcNewDmaChannel(
    OUT PDMACHANNEL* OutDmaChannel,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PDEVICE_DESCRIPTION DeviceDescription,
    IN  PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    CDmaChannelInit * This;

    DPRINT("OutDmaChannel %p OuterUnknown %p PoolType %p DeviceDescription %p DeviceObject %p\n",
            OutDmaChannel, OuterUnknown, PoolType, DeviceDescription, DeviceObject);

    This = new(PoolType, TAG_PORTCLASS)CDmaChannelInit(OuterUnknown);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = This->QueryInterface(IID_IDmaChannel, (PVOID*)OutDmaChannel);

    if (!NT_SUCCESS(Status))
    {
        delete This;
        return Status;
    }

    Status = This->Init(DeviceDescription, DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        delete This;
        return Status;
    }

    return Status;
}
