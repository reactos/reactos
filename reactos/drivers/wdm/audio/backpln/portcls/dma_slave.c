#include "private.h"


typedef struct
{
    IDmaChannelSlaveVtbl *lpVtbl;

    LONG ref;


    PDEVICE_OBJECT pDeviceObject;
    PDMA_ADAPTER pAdapter;

    BOOL DmaStarted;

    ULONG MapSize;
    PVOID MapRegisterBase;

    ULONG LastTransferCount;

    ULONG MaximumBufferSize;
    ULONG MaxMapRegisters;
    ULONG AllocatedBufferSize;
    ULONG BufferSize;

    PHYSICAL_ADDRESS Address;
    PVOID Buffer;
    PMDL Mdl;
    BOOLEAN WriteToDevice;

}IDmaChannelSlaveImpl;

const GUID IID_IDmaChannel;


//---------------------------------------------------------------
// IUnknown methods
//


NTSTATUS
NTAPI
IDmaChannelSlave_fnQueryInterface(
    IDmaChannelSlave * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, &IID_IDmaChannel) ||
        IsEqualGUIDAligned(refiid, &IID_IDmaChannelSlave))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

ULONG
STDMETHODCALLTYPE
IDmaChannelSlave_fnAddRef(
    IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_AddRef: This %p\n", This);

    return _InterlockedIncrement(&This->ref);
}

ULONG
STDMETHODCALLTYPE
IDmaChannelSlave_fnRelease(
    IDmaChannelSlave* iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    _InterlockedDecrement(&This->ref);

    DPRINT("IDmaChannelSlave_Release: This %p new ref %u\n", This, This->ref);

    if (This->ref == 0)
    {
        This->pAdapter->DmaOperations->PutDmaAdapter(This->pAdapter);
        ExFreePoolWithTag(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

//---------------------------------------------------------------
// IDmaChannel methods
//


NTSTATUS
NTAPI
IDmaChannelSlave_fnAllocateBuffer(
    IN IDmaChannelSlave * iface,
    IN ULONG BufferSize,
    IN PPHYSICAL_ADDRESS  PhysicalAddressConstraint  OPTIONAL)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    /* Did the caller already allocate a buffer ?*/
    if (This->Buffer)
    {
        DPRINT1("IDmaChannelSlave_AllocateBuffer free common buffer first \n");
        return STATUS_UNSUCCESSFUL;
    }

    //FIXME
    // retry with different size on failure

    This->Buffer = This->pAdapter->DmaOperations->AllocateCommonBuffer(This->pAdapter, BufferSize, &This->Address, TRUE);
    if (!This->Buffer)
    {
        DPRINT1("IDmaChannelSlave_AllocateBuffer fAllocateCommonBuffer failed \n");
        return STATUS_UNSUCCESSFUL;
    }

    This->BufferSize = BufferSize;
    This->AllocatedBufferSize = BufferSize;
    return STATUS_SUCCESS;
}

ULONG
NTAPI
IDmaChannelSlave_fnAllocatedBufferSize(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_AllocatedBufferSize: This %p BufferSize %u\n", This, This->BufferSize);
    return This->AllocatedBufferSize;
}

VOID
NTAPI
IDmaChannelSlave_fnCopyFrom(
    IN IDmaChannelSlave * iface,
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_CopyFrom: This %p Destination %p Source %p ByteCount %u\n", This, Destination, Source, ByteCount);

    iface->lpVtbl->CopyTo(iface, Destination, Source, ByteCount);
}

VOID
NTAPI
IDmaChannelSlave_fnCopyTo(
    IN IDmaChannelSlave * iface,
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_CopyTo: This %p Destination %p Source %p ByteCount %u\n", This, Destination, Source, ByteCount);
    RtlCopyMemory(Destination, Source, ByteCount);
}
  
VOID
NTAPI
IDmaChannelSlave_fnFreeBuffer(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_FreeBuffer: This %p\n", This);

    if (!This->Buffer)
    {
        DPRINT1("IDmaChannelSlave_FreeBuffer allocate common buffer first \n");
        return;
    }

    This->pAdapter->DmaOperations->FreeCommonBuffer(This->pAdapter, This->AllocatedBufferSize, This->Address, This->Buffer, TRUE);
    This->Buffer = NULL;
    This->AllocatedBufferSize = 0;
    This->Address.QuadPart = 0LL;

    if (This->Mdl)
    {
        IoFreeMdl(This->Mdl);
        This->Mdl = NULL;
    }
}

PADAPTER_OBJECT
NTAPI
IDmaChannelSlave_fnGetAdapterObject(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_GetAdapterObject: This %p\n", This);
    return (PADAPTER_OBJECT)This->pAdapter;
}

ULONG
NTAPI
IDmaChannelSlave_fnMaximumBufferSize(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_MaximumBufferSize: This %p\n", This);
    return This->MaximumBufferSize;
}

PHYSICAL_ADDRESS
NTAPI
IDmaChannelSlave_fnPhysicalAdress(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_PhysicalAdress: This %p\n", This);
    return This->Address;
}

VOID
NTAPI
IDmaChannelSlave_fnSetBufferSize(
    IN IDmaChannelSlave * iface,
    IN ULONG BufferSize)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_SetBufferSize: This %p\n", This);
    This->BufferSize = BufferSize;

}

ULONG
NTAPI
IDmaChannelSlave_fnBufferSize(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    return This->BufferSize;
}


PVOID
NTAPI
IDmaChannelSlave_fnSystemAddress(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_SystemAddress: This %p\n", This);
    return This->Buffer;
}

ULONG
NTAPI
IDmaChannelSlave_fnTransferCount(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_TransferCount: This %p\n", This);
    return This->LastTransferCount;
}

ULONG
NTAPI
IDmaChannelSlave_fnReadCounter(
    IN IDmaChannelSlave * iface)
{
    ULONG Counter;
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    Counter = This->pAdapter->DmaOperations->ReadDmaCounter(This->pAdapter);

    if (!This->DmaStarted || Counter >= This->LastTransferCount) //FIXME
        Counter = 0;

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
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)Context;

    Length = This->MapSize;
    This->MapRegisterBase = MapRegisterBase;

    This->pAdapter->DmaOperations->MapTransfer(This->pAdapter,
                                               This->Mdl,
                                               MapRegisterBase,
                                               (PVOID)((ULONG_PTR)This->Mdl->StartVa + This->Mdl->ByteOffset),
                                               &Length,
                                               This->WriteToDevice);

    if (Length == This->BufferSize)
    {
        This->DmaStarted = TRUE;
    }

   return KeepObject;
}

NTSTATUS
NTAPI
IDmaChannelSlave_fnStart(
    IN IDmaChannelSlave * iface,
    ULONG  MapSize,
    BOOLEAN WriteToDevice)
{
    NTSTATUS Status;
    ULONG MapRegisters;
    KIRQL OldIrql;
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_Start: This %p\n", This);

    if (This->DmaStarted)
        return STATUS_UNSUCCESSFUL;

    if (!This->Mdl)
    {
        This->Mdl = IoAllocateMdl(&This->Buffer, This->MaximumBufferSize, FALSE, FALSE, NULL);
        if (!This->Mdl)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
        MmBuildMdlForNonPagedPool(This->Mdl);
    }

    This->MapSize = MapSize;
    This->WriteToDevice = WriteToDevice;
    This->LastTransferCount = MapSize;

    //FIXME
    // synchronize access
    //
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    MapRegisters = ADDRESS_AND_SIZE_TO_SPAN_PAGES(This->Buffer, MapSize);
    Status = This->pAdapter->DmaOperations->AllocateAdapterChannel(This->pAdapter, This->pDeviceObject, MapRegisters, AdapterControl, (PVOID)This);
    KeLowerIrql(OldIrql);

    if(!NT_SUCCESS(Status))
        This->LastTransferCount = 0;

    return Status;
}

NTSTATUS
NTAPI
IDmaChannelSlave_fnStop(
    IN IDmaChannelSlave * iface)
{
    KIRQL OldIrql;
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_fnStop: This %p\n", This);

    if (!This->DmaStarted)
        return STATUS_SUCCESS;

    This->pAdapter->DmaOperations->FlushAdapterBuffers(This->pAdapter, 
                                                       This->Mdl,
                                                       This->MapRegisterBase,
                                                       (PVOID)((ULONG_PTR)This->Mdl->StartVa + This->Mdl->ByteOffset),
                                                       This->MapSize,
                                                       This->WriteToDevice);

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    This->pAdapter->DmaOperations->FreeAdapterChannel(This->pAdapter);

    KeLowerIrql(OldIrql);

    This->DmaStarted = FALSE;

    return 0;
}

NTSTATUS
NTAPI
IDmaChannelSlave_fnWaitForTC(
    IN IDmaChannelSlave * iface,
    ULONG  Timeout)
{
    ULONG RetryCount;
    ULONG BytesRemaining;
    ULONG PrevBytesRemaining;

    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    BytesRemaining = This->pAdapter->DmaOperations->ReadDmaCounter(This->pAdapter);
    if (!BytesRemaining)
    {
        return STATUS_SUCCESS;
    }

    RetryCount = Timeout / 10;
    PrevBytesRemaining = 0xFFFFFFFF;
    do
    {
        BytesRemaining = This->pAdapter->DmaOperations->ReadDmaCounter(This->pAdapter);

        if (!BytesRemaining)
            break;

        if (PrevBytesRemaining == BytesRemaining)
            break;

        KeStallExecutionProcessor(10);
        PrevBytesRemaining = BytesRemaining;

    }while(RetryCount-- >= 1);

    //FIXME
    // return error code on timeout
    //

    return STATUS_SUCCESS;

}

IDmaChannelSlaveVtbl vt_IDmaChannelSlaveVtbl =
{
    /* IUnknown methods */
    IDmaChannelSlave_fnQueryInterface,
    IDmaChannelSlave_fnAddRef,
    IDmaChannelSlave_fnRelease,
    /* IDmaChannel methods */
    IDmaChannelSlave_fnAllocateBuffer,
    IDmaChannelSlave_fnFreeBuffer,
    IDmaChannelSlave_fnTransferCount,
    IDmaChannelSlave_fnMaximumBufferSize,
    IDmaChannelSlave_fnAllocatedBufferSize,
    IDmaChannelSlave_fnBufferSize,
    IDmaChannelSlave_fnSetBufferSize,
    IDmaChannelSlave_fnSystemAddress,
    IDmaChannelSlave_fnPhysicalAdress,
    IDmaChannelSlave_fnGetAdapterObject,
    IDmaChannelSlave_fnCopyFrom,
    IDmaChannelSlave_fnCopyTo,
    /* IDmaChannelSlave methods */
    IDmaChannelSlave_fnStart,
    IDmaChannelSlave_fnStop,
    IDmaChannelSlave_fnReadCounter,
    IDmaChannelSlave_fnWaitForTC
};


/*
 * @unimplemented
 */
NTSTATUS NTAPI
PcNewDmaChannel(
    OUT PDMACHANNEL* OutDmaChannel,
    IN  PUNKNOWN OuterUnknown OPTIONAL,
    IN  POOL_TYPE PoolType,
    IN  PDEVICE_DESCRIPTION DeviceDescription,
    IN  PDEVICE_OBJECT DeviceObject)
{
    NTSTATUS Status;
    PDMA_ADAPTER Adapter;
    ULONG MapRegisters;
    INTERFACE_TYPE BusType;
    ULONG ResultLength;

    IDmaChannelSlaveImpl * This;

    This = ExAllocatePoolWithTag(PoolType, sizeof(IDmaChannelSlaveImpl), TAG_PORTCLASS);
    if (!This)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    Status = IoGetDeviceProperty(DeviceObject, DevicePropertyLegacyBusType, sizeof(BusType), (PVOID)&BusType, &ResultLength);
    if (NT_SUCCESS(Status))
    {
        DeviceDescription->InterfaceType = BusType;
    }

    Adapter = IoGetDmaAdapter(DeviceObject, DeviceDescription, &MapRegisters);
    if (!Adapter)
    {
        ExFreePoolWithTag(This, TAG_PORTCLASS);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    RtlZeroMemory(This, sizeof(IDmaChannelSlaveImpl));

    This->ref = 1;
    This->lpVtbl = &vt_IDmaChannelSlaveVtbl;
    This->pAdapter = Adapter;
    This->pDeviceObject = DeviceObject;
    This->MaximumBufferSize = DeviceDescription->MaximumLength;
    This->MaxMapRegisters = MapRegisters;

    *OutDmaChannel = (PVOID)(&This->lpVtbl);

    return STATUS_SUCCESS;

}
