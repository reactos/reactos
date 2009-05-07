/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/irpstream.c
 * PURPOSE:         IRP Stream handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"

typedef struct _IRP_MAPPING_
{
    LIST_ENTRY Entry;
    KSSTREAM_HEADER *Header;
    PIRP Irp;

    ULONG References;
    ULONG NumTags;
    PVOID * Tag;
}IRP_MAPPING, *PIRP_MAPPING;

typedef struct
{
    IIrpQueueVtbl *lpVtbl;

    LONG ref;

    ULONG CurrentOffset;
    LONG NumMappings;
    ULONG NumDataAvailable;
    BOOL StartStream;
    KSPIN_CONNECT *ConnectDetails;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;

    KSPIN_LOCK Lock;
    LIST_ENTRY ListHead;

    ULONG OutOfMapping;
    ULONG MaxFrameSize;

}IIrpQueueImpl;

VOID
NTAPI
FreeMappingRoutine(
    PIRP_MAPPING CurMapping)
{
    ASSERT(CurMapping);

    if (CurMapping->Irp)
    {
        CurMapping->Irp->IoStatus.Information = CurMapping->Header->FrameExtent;
        CurMapping->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(CurMapping->Irp, IO_SOUND_INCREMENT);
    }

    if (CurMapping->Tag)
    {
        FreeItem(CurMapping->Tag, TAG_PORTCLASS);
    }

    ExFreePool(CurMapping->Header->Data);
    ExFreePool(CurMapping->Header);

    ExFreePool(CurMapping);
}

NTSTATUS
NTAPI
IIrpQueue_fnQueryInterface(
    IIrpQueue* iface,
    IN  REFIID refiid,
    OUT PVOID* Output)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    if (IsEqualGUIDAligned(refiid, &IID_IUnknown))
    {
        *Output = &This->lpVtbl;
        _InterlockedIncrement(&This->ref);
        return STATUS_SUCCESS;
    }
    return STATUS_UNSUCCESSFUL;
}

ULONG
NTAPI
IIrpQueue_fnAddRef(
    IIrpQueue* iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    return _InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IIrpQueue_fnRelease(
    IIrpQueue* iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    _InterlockedDecrement(&This->ref);

    if (This->ref == 0)
    {
        FreeItem(This, TAG_PORTCLASS);
        return 0;
    }
    /* Return new reference count */
    return This->ref;
}


NTSTATUS
NTAPI
IIrpQueue_fnInit(
    IN IIrpQueue *iface,
    IN KSPIN_CONNECT *ConnectDetails,
    IN PKSDATAFORMAT DataFormat,
    IN PDEVICE_OBJECT DeviceObject,
    IN ULONG FrameSize)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    This->ConnectDetails = ConnectDetails;
    This->DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
    This->MaxFrameSize = FrameSize;

    InitializeListHead(&This->ListHead);
    KeInitializeSpinLock(&This->Lock);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IIrpQueue_fnAddMapping(
    IN IIrpQueue *iface,
    IN PUCHAR Buffer,
    IN ULONG BufferSize,
    IN PIRP Irp)
{
    PIRP_MAPPING Mapping;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    Mapping = AllocateItem(NonPagedPool, sizeof(IRP_MAPPING), TAG_PORTCLASS);
    if (!Mapping)
        return STATUS_UNSUCCESSFUL;

    Mapping->Header = (KSSTREAM_HEADER*)Buffer;
    Mapping->Irp = Irp;

    if (This->MaxFrameSize)
    {
        if (This->MaxFrameSize > Mapping->Header->DataUsed)
        {
            /* small mapping */
            Mapping->NumTags = 1;
        }
        else
        {
            ULONG Rest = Mapping->Header->DataUsed % This->MaxFrameSize;

            Mapping->NumTags = Mapping->Header->DataUsed / This->MaxFrameSize;
            if (Rest)
            {
                Mapping->NumTags++;
            }
        }
    }
    else
    {
        /* no framesize restriction */
        Mapping->NumTags = 1;
    }

    Mapping->Tag = AllocateItem(NonPagedPool, sizeof(PVOID) * Mapping->NumTags, TAG_PORTCLASS);
    if (!Mapping->Tag)
    {
        FreeItem(Mapping, TAG_PORTCLASS);
        return STATUS_UNSUCCESSFUL;
    }
    ASSERT(Mapping->NumTags < 32);
    Mapping->References = (1 << Mapping->NumTags) - 1;

    This->NumDataAvailable += Mapping->Header->DataUsed;

    DPRINT("IIrpQueue_fnAddMapping NumMappings %u SizeOfMapping %lu NumDataAvailable %lu Mapping %p NumTags %u References %x FrameSize %u\n", This->NumMappings, Mapping->Header->DataUsed, This->NumDataAvailable, Mapping, Mapping->NumTags, Mapping->References, This->MaxFrameSize);

    KeAcquireSpinLockAtDpcLevel(&This->Lock);
    InsertTailList(&This->ListHead, &Mapping->Entry);
    KeReleaseSpinLockFromDpcLevel(&This->Lock);

    (void)InterlockedIncrement((volatile long*)&This->NumMappings);

    if (Irp)
    {
        Irp->IoStatus.Status = STATUS_PENDING;
        Irp->IoStatus.Information = 0;
        IoMarkIrpPending(Irp);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IIrpQueue_fnGetMapping(
    IN IIrpQueue *iface,
    OUT PUCHAR * Buffer,
    OUT PULONG BufferSize)
{

    PIRP_MAPPING CurMapping;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;
    PLIST_ENTRY CurEntry;

    KeAcquireSpinLockAtDpcLevel(&This->Lock);


    CurEntry = This->ListHead.Flink;
    CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
    if (CurEntry == &This->ListHead)
    {
        KeReleaseSpinLockFromDpcLevel(&This->Lock);
        This->OutOfMapping = TRUE;
        return STATUS_UNSUCCESSFUL;
    }

    *Buffer = (PUCHAR)CurMapping->Header->Data + This->CurrentOffset;
    *BufferSize = CurMapping->Header->DataUsed - This->CurrentOffset;
    This->OutOfMapping = FALSE;

    KeReleaseSpinLockFromDpcLevel(&This->Lock);
    return STATUS_SUCCESS;
}



VOID
NTAPI
IIrpQueue_fnUpdateMapping(
    IN IIrpQueue *iface,
    IN ULONG BytesWritten)
{
    PLIST_ENTRY CurEntry;
    PIRP_MAPPING CurMapping;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    This->CurrentOffset += BytesWritten;
    This->NumDataAvailable -= BytesWritten;

    CurEntry = This->ListHead.Flink;
    CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);

    if (CurMapping->Header->DataUsed <= This->CurrentOffset)
    {
        This->CurrentOffset = 0;

        KeAcquireSpinLockAtDpcLevel(&This->Lock);
        RemoveHeadList(&This->ListHead);
        KeReleaseSpinLockFromDpcLevel(&This->Lock);

        InterlockedDecrement(&This->NumMappings);
        FreeMappingRoutine(CurMapping);
    }

}

ULONG
NTAPI
IIrpQueue_fnNumMappings(
    IN IIrpQueue *iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    return This->NumMappings;
}

ULONG
NTAPI
IIrpQueue_fnMinMappings(
    IN IIrpQueue *iface)
{
    return 25;
}


BOOL
NTAPI
IIrpQueue_fnMinimumDataAvailable(
    IN IIrpQueue *iface)
{
    BOOL Result;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    if (This->StartStream)
        return TRUE;

    if (This->DataFormat->WaveFormatEx.nAvgBytesPerSec < This->NumDataAvailable)
    {
        This->StartStream = TRUE;
        Result = TRUE;
    }
    else
        Result = FALSE;

    return Result;
}

BOOL
NTAPI
IIrpQueue_fnCancelBuffers(
    IN IIrpQueue *iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    This->StartStream = FALSE;
    return TRUE;
}

VOID
NTAPI
IIrpQueue_fnUpdateFormat(
    IN IIrpQueue *iface,
    PKSDATAFORMAT DataFormat)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;
    This->DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
    This->StartStream = FALSE;

}

VOID
GetMapping(
    IIrpQueueImpl * This,
    IN ULONG Index,
    IN PVOID Tag,
    IN PIRP_MAPPING CurMapping,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress,
    OUT PVOID  *VirtualAddress,
    OUT PULONG  ByteCount,
    OUT PULONG  Flags)
{
    ULONG Offset;

    /* calculate the offset */
    if (Index)
        Offset = Index * This->MaxFrameSize;
    else
        Offset = 0;

    ASSERT(CurMapping->Header->DataUsed > Offset);

    *VirtualAddress = (PUCHAR)CurMapping->Header->Data + Offset;
    *PhysicalAddress = MmGetPhysicalAddress(*VirtualAddress);
    /* FIXME alignment */
    *ByteCount = min(CurMapping->Header->DataUsed - Offset, This->MaxFrameSize);

    /* reset out of mapping indicator */
    This->OutOfMapping = FALSE;

    /* decrement available byte counter */
    This->NumDataAvailable -= *ByteCount;

    /* store the tag */
    CurMapping->Tag[Index] = Tag;

    if (Index + 1 == CurMapping->NumTags)
    {
        /* indicate end of packet */
        *Flags = 1;
    }

    DPRINT("IIrpQueue_fnGetMappingWithTag Tag %p Mapping %p Index %u NumTags %u\n", Tag, CurMapping, Index, CurMapping->NumTags);
}


NTSTATUS
NTAPI
IIrpQueue_fnGetMappingWithTag(
    IN IIrpQueue *iface,
    IN PVOID Tag,
    OUT PPHYSICAL_ADDRESS  PhysicalAddress,
    OUT PVOID  *VirtualAddress,
    OUT PULONG  ByteCount,
    OUT PULONG  Flags)
{
    PIRP_MAPPING CurMapping;
    PLIST_ENTRY CurEntry;
    ULONG Index;
    ULONG Value;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    *Flags = 0;
    ASSERT(Tag != NULL);

    KeAcquireSpinLockAtDpcLevel(&This->Lock);

    CurEntry = This->ListHead.Flink;
    if (CurEntry == &This->ListHead)
    {
        KeReleaseSpinLockFromDpcLevel(&This->Lock);
        This->OutOfMapping = TRUE;
        This->StartStream = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    while (CurEntry != &This->ListHead)
    {
        CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
        for(Index = 0; Index < CurMapping->NumTags; Index++)
        {
            Value = CurMapping->References & ( 1 << Index);
            if (CurMapping->Tag[Index] == NULL && Value)
            {
                /* found a free mapping within audio irp */
                GetMapping(This, Index, Tag, CurMapping, PhysicalAddress, VirtualAddress, ByteCount, Flags);
                KeReleaseSpinLockFromDpcLevel(&This->Lock);
                return STATUS_SUCCESS;
            }
        }
        CurEntry = CurEntry->Flink;
    }
    KeReleaseSpinLockFromDpcLevel(&This->Lock);
    This->OutOfMapping = TRUE;
    This->StartStream = FALSE;
    DPRINT("No Mapping available\n");
    return STATUS_UNSUCCESSFUL;
}

NTSTATUS
NTAPI
IIrpQueue_fnReleaseMappingWithTag(
    IN IIrpQueue *iface,
    IN PVOID Tag)
{
    PIRP_MAPPING CurMapping = NULL;
    PLIST_ENTRY CurEntry;
    ULONG Index = 0;
    ULONG Found;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    KeAcquireSpinLockAtDpcLevel(&This->Lock);

    CurEntry = This->ListHead.Flink;
    if (CurEntry == &This->ListHead)
    {
        KeReleaseSpinLockFromDpcLevel(&This->Lock);
        return STATUS_UNSUCCESSFUL;
    }

    Found = FALSE;
    while (CurEntry != &This->ListHead)
    {
        CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
        for(Index = 0; Index < CurMapping->NumTags; Index++)
        {
            if (CurMapping->Tag[Index] == Tag)
            {
                Found = TRUE;
                CurMapping->Tag[Index] = NULL;
                break;
            }
        }
        if (Found)
            break;

        CurEntry = CurEntry->Flink;
    }

    if (!Found)
    {
        DPRINT1("Tag %p not found\n", Tag);
        ASSERT(Found);
    }
    DPRINT("References %x\n", CurMapping->References);
    CurMapping->References &= ~(1 << Index);

    if (CurMapping->References)
    {
        /* released mapping is not the last mapping of the irp */
        DPRINT1("IIrpQueue_fnReleaseMappingWithTag Tag %p Index %u NumTags %u Refs %x\n", Tag, Index, CurMapping->NumTags, CurMapping->References);
        KeReleaseSpinLockFromDpcLevel(&This->Lock);
        return STATUS_SUCCESS;
    }

    RemoveEntryList(&CurMapping->Entry);

    /* last mapping of the irp, free irp */
    DPRINT("Freeing mapping %p\n", CurMapping);
    InterlockedDecrement(&This->NumMappings);
    FreeMappingRoutine(CurMapping);
    KeReleaseSpinLockFromDpcLevel(&This->Lock);

    return STATUS_SUCCESS;
}

BOOL
NTAPI
IIrpQueue_fnHasLastMappingFailed(
    IN IIrpQueue *iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;
    return This->OutOfMapping;
}

static IIrpQueueVtbl vt_IIrpQueue =
{
    IIrpQueue_fnQueryInterface,
    IIrpQueue_fnAddRef,
    IIrpQueue_fnRelease,
    IIrpQueue_fnInit,
    IIrpQueue_fnAddMapping,
    IIrpQueue_fnGetMapping,
    IIrpQueue_fnUpdateMapping,
    IIrpQueue_fnNumMappings,
    IIrpQueue_fnMinMappings,
    IIrpQueue_fnMinimumDataAvailable,
    IIrpQueue_fnCancelBuffers,
    IIrpQueue_fnUpdateFormat,
    IIrpQueue_fnGetMappingWithTag,
    IIrpQueue_fnReleaseMappingWithTag,
    IIrpQueue_fnHasLastMappingFailed

};


NTSTATUS
NTAPI
NewIrpQueue(
    IN IIrpQueue **Queue)
{
    IIrpQueueImpl *This = AllocateItem(NonPagedPool, sizeof(IIrpQueueImpl), TAG_PORTCLASS);
    if (!This)
        return STATUS_INSUFFICIENT_RESOURCES;

    This->ref = 1;
    This->lpVtbl = &vt_IIrpQueue;

    *Queue = (IIrpQueue*)This;
    return STATUS_SUCCESS;

}

NTSTATUS
NewIrpStreamPhysical(
    OUT IIrpStreamPhysical ** OutIIrpStreamPhysical,
    IN IUnknown *OuterUnknown)
{
    return STATUS_UNSUCCESSFUL;
}


/*
 * @implemented
 */

NTSTATUS
NTAPI
PcNewIrpStreamPhysical(
    OUT IIrpStreamPhysical ** OutIrpStreamPhysical,
    IN IUnknown * OuterUnknown,
    IN BOOLEAN Wait,
    IN KSPIN_CONNECT *ConnectDetails,
    IN PDEVICE_OBJECT DeviceObject,
    IN PDMA_ADAPTER DmaAdapter)
{
    NTSTATUS Status;
    IIrpStreamPhysical * Irp;

    Status = NewIrpStreamPhysical(&Irp, OuterUnknown);
    if (!NT_SUCCESS(Status))
        return Status;


    Status = Irp->lpVtbl->Init(Irp, Wait, ConnectDetails, DeviceObject, DmaAdapter);
    if (!NT_SUCCESS(Status))
    {
        Irp->lpVtbl->Release(Irp);
        return Status;
    }

    *OutIrpStreamPhysical = Irp;
    return Status;
}
