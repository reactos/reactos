#include "private.h"


typedef struct
{
    IDmaChannelSlaveVtbl *lpVtbl;

    LONG ref;
    ULONG BufferSize;
    PHYSICAL_ADDRESS Address;

}IDmaChannelImpl;


/*
    Basic IUnknown methods
*/

	NTSTATUS
	STDMETHODCALLTYPE
	IDmaChannel_fnQueryInterface(
		IDmaChannel* iface,
		IN  REFIID refiid,
		OUT PVOID* Output)
	{
		/* TODO */
		return STATUS_UNSUCCESSFUL;
	}

ULONG
STDMETHODCALLTYPE
IDmaChannel_fnAddRef(
    IDmaChannel* iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_AddRef: This %p\n", This);

    return _InterlockedIncrement(&This->ref);
}

ULONG
STDMETHODCALLTYPE
IDmaChannel_fnRelease(
    IDmaChannel* iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    _InterlockedDecrement(&This->ref);

    DPRINT("IDmaChannel_Release: This %p new ref %u\n", This, This->ref);

    if (This->ref == 0)
    {
        ExFreePoolWithTag(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}



NTSTATUS
NTAPI
IDmaChannel_fnAllocateBuffer(
    IN IDmaChannel * iface,
    IN ULONG BufferSize,
    IN PPHYSICAL_ADDRESS  PhysicalAddressConstraint  OPTIONAL)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_AllocateBuffer: This %p BufferSize %u\n", This, BufferSize);

    /* Did the caller already allocate a buffer ?*/
    if (This->BufferSize) return STATUS_UNSUCCESSFUL;

    /* FIXME */
    //This->BufferSize = BufferSize;

    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IDmaChannel_fnAllocatedBufferSize(
    IN IDmaChannel * iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_AllocatedBufferSize: This %p BufferSize %u\n", This, This->BufferSize);
    return This->BufferSize;
}

VOID
NTAPI
IDmaChannel_fnCopyFrom(
    IN IDmaChannel * iface,
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_CopyFrom: This %p Destination %p Source %p ByteCount %u\n", This, Destination, Source, ByteCount);
}

VOID
NTAPI
IDmaChannel_fnCopyTo(
    IN IDmaChannel * iface,
    IN PVOID  Destination,
    IN PVOID  Source,
    IN ULONG  ByteCount
    )
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_CopyTo: This %p Destination %p Source %p ByteCount %u\n", This, Destination, Source, ByteCount);
}

VOID
NTAPI
IDmaChannel_fnFreeBuffer(
    IN IDmaChannel * iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_FreeBuffer: This %p\n", This);
}

PADAPTER_OBJECT
NTAPI
IDmaChannel_fnGetAdapterObject(
    IN IDmaChannel * iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_GetAdapterObject: This %p\n", This);
    return NULL;
}

ULONG
NTAPI
IDmaChannel_fnMaximumBufferSize(
    IN IDmaChannel * iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_MaximumBufferSize: This %p\n", This);
    return 0;
}

PHYSICAL_ADDRESS
NTAPI
IDmaChannel_fnPhysicalAdress(
    IN IDmaChannel * iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_PhysicalAdress: This %p\n", This);
    return This->Address;
}

VOID
NTAPI
IDmaChannel_fnSetBufferSize(
    IN IDmaChannel * iface,
    IN ULONG BufferSize)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_SetBufferSize: This %p\n", This);

}

PVOID
NTAPI
IDmaChannel_fnSystemAddress(
    IN IDmaChannel * iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_SystemAddress: This %p\n", This);
    return NULL;
}

ULONG
NTAPI
IDmaChannel_fnTransferCount(
    IN IDmaChannel * iface)
{
    IDmaChannelImpl * This = (IDmaChannelImpl*)iface;

    DPRINT("IDmaChannel_TransferCount: This %p\n", This);
    return 0;
}
