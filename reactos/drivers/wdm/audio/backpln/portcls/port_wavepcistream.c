#include "private.h"

typedef struct
{
    IPortWavePciStreamVtbl * lpVtbl;
    IIrpQueue *Queue;
    LONG ref;


}IPortWavePciStreamImpl;

static
NTSTATUS
NTAPI
IPortWavePciStream_fnQueryInterface(
    IPortWavePciStream* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IPortWavePciStreamImpl * This = (IPortWavePciStreamImpl*)iface;

    DPRINT("IPortWavePciStream_fnQueryInterface entered\n");

    if (IsEqualGUIDAligned(refiid, &IID_IPortWavePciStream) ||
        IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }

    return STATUS_UNSUCCESSFUL;
}

static
ULONG
NTAPI
IPortWavePciStream_fnAddRef(
    IPortWavePciStream* iface)
{
    IPortWavePciStreamImpl * This = (IPortWavePciStreamImpl*)iface;
    DPRINT("IPortWavePciStream_fnAddRef entered\n");

    return InterlockedIncrement(&This->ref);
}

static
ULONG
NTAPI
IPortWavePciStream_fnRelease(
    IPortWavePciStream* iface)
{
    IPortWavePciStreamImpl * This = (IPortWavePciStreamImpl*)iface;

    InterlockedDecrement(&This->ref);

    DPRINT("IPortWavePciStream_fnRelease entered %u\n", This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}

static
NTSTATUS
NTAPI
IPortWavePciStream_fnGetMapping(
    IN IPortWavePciStream *iface,
    IN PVOID Tag,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress,
    OUT PVOID  *VirtualAddress,
    OUT PULONG  ByteCount,
    OUT PULONG  Flags)
{
    IPortWavePciStreamImpl * This = (IPortWavePciStreamImpl*)iface;

    return This->Queue->lpVtbl->GetMappingWithTag(This->Queue, Tag, PhysicalAddress, VirtualAddress, ByteCount, Flags);
}

static
NTSTATUS
NTAPI
IPortWavePciStream_fnReleaseMapping(
    IN IPortWavePciStream *iface,
    IN PVOID  Tag)
{
    IPortWavePciStreamImpl * This = (IPortWavePciStreamImpl*)iface;
    This->Queue->lpVtbl->ReleaseMappingWithTag(This->Queue, Tag);
    return STATUS_SUCCESS;
}

static
NTSTATUS
NTAPI
IPortWavePciStream_fnTerminatePacket(
    IN IPortWavePciStream *iface)
{
    UNIMPLEMENTED
    return STATUS_SUCCESS;
}


static IPortWavePciStreamVtbl vt_PortWavePciStream =
{
    IPortWavePciStream_fnQueryInterface,
    IPortWavePciStream_fnAddRef,
    IPortWavePciStream_fnRelease,
    IPortWavePciStream_fnGetMapping,
    IPortWavePciStream_fnReleaseMapping,
    IPortWavePciStream_fnTerminatePacket
};

NTSTATUS
NTAPI
NewIPortWavePciStream(
    OUT PPORTWAVEPCISTREAM *Stream,
    IN KSPIN_CONNECT *ConnectDetails,
    IN PKSDATAFORMAT DataFormat,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG FrameSize)
{
    IIrpQueue * Queue;
    IPortWavePciStreamImpl * This;
    NTSTATUS Status;

    Status = NewIrpQueue(&Queue);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = Queue->lpVtbl->Init(Queue, ConnectDetails, DataFormat, DeviceObject, FrameSize);
    if (!NT_SUCCESS(Status))
    {
        Queue->lpVtbl->Release(Queue);
        return Status;
    }

    This = AllocateItem(NonPagedPool, sizeof(IPortWavePciStreamImpl), TAG_PORTCLASS);
    if (!This)
    {
        Queue->lpVtbl->Release(Queue);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    This->lpVtbl = &vt_PortWavePciStream;
    This->ref = 1;
    This->Queue = Queue;

    *Stream = (PPORTWAVEPCISTREAM)&This->lpVtbl;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IPortWavePciStream_AddMapping(
    IN IPortWavePciStream *iface,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN PIRP Irp)
{
    IPortWavePciStreamImpl * This = (IPortWavePciStreamImpl*)iface;
    return This->Queue->lpVtbl->AddMapping(This->Queue, Buffer, BufferSize, Irp);
}

