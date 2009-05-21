/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/port_wavepcistream.c
 * PURPOSE:         Wave PCI Stream object
 * PROGRAMMER:      Johannes Anderwald
 */

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
        This->Queue->lpVtbl->Release(This->Queue);
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

    ASSERT_IRQL(DISPATCH_LEVEL);
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

    ASSERT_IRQL(DISPATCH_LEVEL);
    return This->Queue->lpVtbl->ReleaseMappingWithTag(This->Queue, Tag);
}

static
NTSTATUS
NTAPI
IPortWavePciStream_fnTerminatePacket(
    IN IPortWavePciStream *iface)
{
    UNIMPLEMENTED
    ASSERT_IRQL(DISPATCH_LEVEL);
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
    OUT PPORTWAVEPCISTREAM *Stream)
{
    IIrpQueue * Queue;
    IPortWavePciStreamImpl * This;
    NTSTATUS Status;

    Status = NewIrpQueue(&Queue);
    if (!NT_SUCCESS(Status))
        return Status;

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

IIrpQueue*
NTAPI
IPortWavePciStream_GetIrpQueue(
    IN IPortWavePciStream *iface)
{
    IPortWavePciStreamImpl * This = (IPortWavePciStreamImpl*)iface;
    return This->Queue;
}

