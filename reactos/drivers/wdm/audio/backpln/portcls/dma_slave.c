/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/dma_Init.c
 * PURPOSE:         portcls dma support object
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"


typedef struct
{
    IDmaChannelInitVtbl *lpVtbl;

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

}IDmaChannelInitImpl;

//---------------------------------------------------------------
// IUnknown methods
//

extern GUID IID_IDmaChannelSlave;

NTSTATUS
NTAPI
IDmaChannelInit_fnQueryInterface(
    IDmaChannelInit * iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown) ||
        IsEqualGUIDAligned(refiid, &IID_IDmaChannel) ||
        IsEqualGUIDAligned(refiid, &IID_IDmaChannelSlave))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    DPRINT1("No interface!!!\n");
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IDmaChannelInit_fnAddRef(
    IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_AddRef: This %p\n", This);

    return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IDmaChannelInit_fnRelease(
    IDmaChannelInit* iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    InterlockedDecrement(&This->ref);

    DPRINT("IDmaChannelInit_Release: This %p new ref %u\n", This, This->ref);

    if (This->ref == 0)
    {
        This->pAdapter->DmaOperations->PutDmaAdapter(This->pAdapter);
        FreeItem(This, TAG_PORTCLASS);
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
IDmaChannelInit_fnAllocateBuffer(
    IN IDmaChannelInit * iface,
    IN ULONG BufferSize,
    IN PPHYSICAL_ADDRESS  PhysicalAddressConstraint OPTIONAL)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    /* Did the caller already allocate a buffer ?*/
    if (This->Buffer)
    {
        DPRINT1("IDmaChannelInit_AllocateBuffer free common buffer first \n");
        return STATUS_UNSUCCESSFUL;
    }

    This->Buffer = This->pAdapter->DmaOperations->AllocateCommonBuffer(This->pAdapter, BufferSize, &This->Address, FALSE);
    if (!This->Buffer)
    {
        DPRINT1("IDmaChannelInit_AllocateBuffer fAllocateCommonBuffer failed \n");
        return STATUS_UNSUCCESSFUL;
    }

    This->BufferSize = BufferSize;
    This->AllocatedBufferSize = BufferSize;
    DPRINT1("IDmaChannelInit_fnAllocateBuffer Success Buffer %p BufferSize %u Address %x\n", This->Buffer, BufferSize, This->Address);

    return STATUS_SUCCESS;
}

ULONG
NTAPI
IDmaChannelInit_fnAllocatedBufferSize(
    IN IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_AllocatedBufferSize: This %p BufferSize %u\n", This, This->BufferSize);
    return This->AllocatedBufferSize;
}

VOID
NTAPI
IDmaChannelInit_fnCopyFrom(
    IN IDmaChannelInit * iface,
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_CopyFrom: This %p Destination %p Source %p ByteCount %u\n", This, Destination, Source, ByteCount);

    iface->lpVtbl->CopyTo(iface, Destination, Source, ByteCount);
}

VOID
NTAPI
IDmaChannelInit_fnCopyTo(
    IN IDmaChannelInit * iface,
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_CopyTo: This %p Destination %p Source %p ByteCount %u\n", This, Destination, Source, ByteCount);
    RtlCopyMemory(Destination, Source, ByteCount);
}
  
VOID
NTAPI
IDmaChannelInit_fnFreeBuffer(
    IN IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_FreeBuffer: This %p\n", This);

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (!This->Buffer)
    {
        DPRINT1("IDmaChannelInit_FreeBuffer allocate common buffer first \n");
        return;
    }

    This->pAdapter->DmaOperations->FreeCommonBuffer(This->pAdapter, This->AllocatedBufferSize, This->Address, This->Buffer, FALSE);
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
IDmaChannelInit_fnGetAdapterObject(
    IN IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_GetAdapterObject: This %p\n", This);
    return (PADAPTER_OBJECT)This->pAdapter;
}

ULONG
NTAPI
IDmaChannelInit_fnMaximumBufferSize(
    IN IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_MaximumBufferSize: This %p\n", This);
    return This->MaximumBufferSize;
}

PHYSICAL_ADDRESS
NTAPI
IDmaChannelInit_fnPhysicalAdress(
    IN IDmaChannelInit * iface,
    PPHYSICAL_ADDRESS Address)
{
    PHYSICAL_ADDRESS Result;
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;
    DPRINT("IDmaChannelInit_PhysicalAdress: This %p Virtuell %p Physical High %x Low %x%\n", This, This->Buffer, This->Address.HighPart, This->Address.LowPart);

    Address->QuadPart = This->Address.QuadPart;
    Result.QuadPart = (PtrToUlong(Address));
    return Result;
}

VOID
NTAPI
IDmaChannelInit_fnSetBufferSize(
    IN IDmaChannelInit * iface,
    IN ULONG BufferSize)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_SetBufferSize: This %p\n", This);
    This->BufferSize = BufferSize;

}

ULONG
NTAPI
IDmaChannelInit_fnBufferSize(
    IN IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    return This->BufferSize;
}


PVOID
NTAPI
IDmaChannelInit_fnSystemAddress(
    IN IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_SystemAddress: This %p\n", This);
    return This->Buffer;
}

ULONG
NTAPI
IDmaChannelInit_fnTransferCount(
    IN IDmaChannelInit * iface)
{
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_TransferCount: This %p\n", This);
    return This->LastTransferCount;
}

ULONG
NTAPI
IDmaChannelInit_fnReadCounter(
    IN IDmaChannelInit * iface)
{
    ULONG Counter;
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    ASSERT_IRQL(DISPATCH_LEVEL);

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
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)Context;

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
IDmaChannelInit_fnStart(
    IN IDmaChannelInit * iface,
    ULONG  MapSize,
    BOOLEAN WriteToDevice)
{
    NTSTATUS Status;
    ULONG MapRegisters;
    KIRQL OldIrql;
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_Start: This %p\n", This);

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

    if (This->DmaStarted)
        return STATUS_UNSUCCESSFUL;

    if (!This->Mdl)
    {
        This->Mdl = IoAllocateMdl(This->Buffer, This->MaximumBufferSize, FALSE, FALSE, NULL);
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
IDmaChannelInit_fnStop(
    IN IDmaChannelInit * iface)
{
    KIRQL OldIrql;
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    DPRINT("IDmaChannelInit_fnStop: This %p\n", This);
    ASSERT_IRQL(DISPATCH_LEVEL);

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

    IoFreeMdl(This->Mdl);
    This->Mdl = NULL;

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IDmaChannelInit_fnWaitForTC(
    IN IDmaChannelInit * iface,
    ULONG  Timeout)
{
    ULONG RetryCount;
    ULONG BytesRemaining;
    ULONG PrevBytesRemaining;

    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    ASSERT_IRQL_EQUAL(PASSIVE_LEVEL);

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

NTSTATUS
NTAPI
IDmaChannelInit_fnInit(
    IN IDmaChannelInit * iface,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    IN PDEVICE_OBJECT DeviceObject)
{
    INTERFACE_TYPE BusType;
    NTSTATUS Status;
    PDMA_ADAPTER Adapter;
    PPCLASS_DEVICE_EXTENSION DeviceExt;
    ULONG MapRegisters;
    ULONG ResultLength;
    IDmaChannelInitImpl * This = (IDmaChannelInitImpl*)iface;

    /* Get bus type */
    Status = IoGetDeviceProperty(DeviceObject, DevicePropertyLegacyBusType, sizeof(BusType), (PVOID)&BusType, &ResultLength);
    if (NT_SUCCESS(Status))
    {
        DeviceDescription->InterfaceType = BusType;
    }
    /* Fetch device extension */
    DeviceExt = (PPCLASS_DEVICE_EXTENSION) DeviceObject->DeviceExtension;
    /* Acquire dma adapter */
    Adapter = IoGetDmaAdapter(DeviceExt->PhysicalDeviceObject, DeviceDescription, &MapRegisters);
    if (!Adapter)
    {
        FreeItem(This, TAG_PORTCLASS);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    /* initialize object */
    This->ref = 1;
    This->pAdapter = Adapter;
    This->pDeviceObject = DeviceObject;
    This->MaximumBufferSize = DeviceDescription->MaximumLength;
    This->MaxMapRegisters = MapRegisters;

    return STATUS_SUCCESS;
}


IDmaChannelInitVtbl vt_IDmaChannelInitVtbl =
{
    /* IUnknown methods */
    IDmaChannelInit_fnQueryInterface,
    IDmaChannelInit_fnAddRef,
    IDmaChannelInit_fnRelease,
    /* IDmaChannel methods */
    IDmaChannelInit_fnAllocateBuffer,
    IDmaChannelInit_fnFreeBuffer,
    IDmaChannelInit_fnTransferCount,
    IDmaChannelInit_fnMaximumBufferSize,
    IDmaChannelInit_fnAllocatedBufferSize,
    IDmaChannelInit_fnBufferSize,
    IDmaChannelInit_fnSetBufferSize,
    IDmaChannelInit_fnSystemAddress,
    IDmaChannelInit_fnPhysicalAdress,
    IDmaChannelInit_fnGetAdapterObject,
    IDmaChannelInit_fnCopyTo,
    IDmaChannelInit_fnCopyFrom,
    /* IDmaChannelInit methods */
    IDmaChannelInit_fnStart,
    IDmaChannelInit_fnStop,
    IDmaChannelInit_fnReadCounter,
    IDmaChannelInit_fnWaitForTC,
    IDmaChannelInit_fnInit
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
    IDmaChannelInitImpl * This;

    DPRINT("OutDmaChannel %p OuterUnknown %p PoolType %p DeviceDescription %p DeviceObject %p\n",
            OutDmaChannel, OuterUnknown, PoolType, DeviceDescription, DeviceObject);

    This = AllocateItem(PoolType, sizeof(IDmaChannelInitImpl), TAG_PORTCLASS);
    if (!This)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize object */
    Status = IDmaChannelInit_fnInit((IDmaChannelInit*)This, DeviceDescription, DeviceObject);
    if (NT_SUCCESS(Status))
    {
        /* store result */
        This->lpVtbl = &vt_IDmaChannelInitVtbl;
        *OutDmaChannel = (PVOID)(&This->lpVtbl);
    }

    return Status;

}
