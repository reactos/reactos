#include "private.h"


typedef struct
{
    IDmaChannelSlaveVtbl *lpVtbl;

    LONG ref;

    ULONG BufferSize;
    PHYSICAL_ADDRESS Address;

}IDmaChannelSlaveImpl;


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
    /* TODO */
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

    DPRINT("IDmaChannelSlave_AllocateBuffer: This %p BufferSize %u\n", This, BufferSize);

    /* Did the caller already allocate a buffer ?*/
    if (This->BufferSize) return STATUS_UNSUCCESSFUL;

    /* FIXME */
    //This->BufferSize = BufferSize;

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IDmaChannelSlave_fnAllocatedBufferSize(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_AllocatedBufferSize: This %p BufferSize %u\n", This, This->BufferSize);
    return This->BufferSize;
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
}

VOID
NTAPI
IDmaChannelSlave_fnFreeBuffer(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_FreeBuffer: This %p\n", This);
}

PADAPTER_OBJECT
NTAPI
IDmaChannelSlave_fnGetAdapterObject(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_GetAdapterObject: This %p\n", This);
    return NULL;
}

ULONG
NTAPI
IDmaChannelSlave_fnMaximumBufferSize(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_MaximumBufferSize: This %p\n", This);
    return 0;
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

}

ULONG
NTAPI
IDmaChannelSlave_fnBufferSize(
    IN IDmaChannelSlave * iface)
{
    return 0;
}


PVOID
NTAPI
IDmaChannelSlave_fnSystemAddress(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_SystemAddress: This %p\n", This);
    return NULL;
}

ULONG
NTAPI
IDmaChannelSlave_fnTransferCount(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_TransferCount: This %p\n", This);
    return 0;
}

ULONG
NTAPI
IDmaChannelSlave_fnReadCounter(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_ReadCounter: This %p\n", This);
    return 0;
}

NTSTATUS
NTAPI
IDmaChannelSlave_fnStart(
    IN IDmaChannelSlave * iface,
    ULONG  MapSize,
    BOOLEAN WriteToDevice)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_Start: This %p\n", This);
    return 0;
}

NTSTATUS
NTAPI
IDmaChannelSlave_fnStop(
    IN IDmaChannelSlave * iface)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_fnStop: This %p\n", This);
    return 0;
}

NTSTATUS
NTAPI
IDmaChannelSlave_fnWaitForTC(
    IN IDmaChannelSlave * iface,
    ULONG  Timeout)
{
    IDmaChannelSlaveImpl * This = (IDmaChannelSlaveImpl*)iface;

    DPRINT("IDmaChannelSlave_WaitForTC: This %p\n", This);
    return 0;

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

