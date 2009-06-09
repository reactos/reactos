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
    PVOID Buffer;
    ULONG BufferSize;
    ULONG OriginalBufferSize;
    PVOID OriginalBuffer;
    PIRP Irp;

    PVOID Tag;
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
    LIST_ENTRY FreeHead;

    ULONG OutOfMapping;
    ULONG MaxFrameSize;
    ULONG Alignment;

}IIrpQueueImpl;

VOID
NTAPI
FreeMappingRoutine(
    PIRP_MAPPING CurMapping)
{
    ASSERT(CurMapping);

    if (CurMapping->Irp)
    {
        CurMapping->Irp->IoStatus.Information = CurMapping->OriginalBufferSize;
        CurMapping->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(CurMapping->Irp, IO_SOUND_INCREMENT);
    }

    if (CurMapping->OriginalBuffer)
    {
        ExFreePool(CurMapping->OriginalBuffer);
    }

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
    IN ULONG FrameSize,
    IN ULONG Alignment)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    This->ConnectDetails = ConnectDetails;
    This->DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
    This->MaxFrameSize = FrameSize;
    This->Alignment = Alignment;

    InitializeListHead(&This->ListHead);
    InitializeListHead(&This->FreeHead);
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
    PIRP_MAPPING Mapping = NULL;
    KSSTREAM_HEADER * Header = (KSSTREAM_HEADER*)Buffer;
    ULONG Index, NumMappings, Offset;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;


    if (This->MaxFrameSize)
    {
        if (This->MaxFrameSize > Header->DataUsed)
        {
            /* small mapping */
            NumMappings = 1;
        }
        else
        {
            ULONG Rest = Header->DataUsed % This->MaxFrameSize;

            NumMappings = Header->DataUsed / This->MaxFrameSize;
            if (Rest)
            {
                NumMappings++;
            }
        }
    }
    else
    {
        /* no framesize restriction */
        NumMappings = 1;
    }

    for(Index = 0; Index < NumMappings; Index++)
    {
        Mapping = AllocateItem(NonPagedPool, sizeof(IRP_MAPPING), TAG_PORTCLASS);
        if (!Mapping)
        {
            DPRINT("OutOfMemory\n");
            return STATUS_UNSUCCESSFUL;
        }

        if (Index)
            Offset = Index * This->MaxFrameSize;
        else
            Offset = 0;

        Mapping->Buffer = (PVOID)UlongToPtr((PtrToUlong(Header->Data) + Offset + 3) & ~(0x3));

        if (This->MaxFrameSize)
            Mapping->BufferSize = min(Header->DataUsed - Offset, This->MaxFrameSize);
        else
            Mapping->BufferSize = Header->DataUsed;

        Mapping->OriginalBufferSize = Header->FrameExtent;
        Mapping->OriginalBuffer = NULL;
        Mapping->Irp = NULL;
        Mapping->Tag = NULL;

        This->NumDataAvailable += Mapping->BufferSize;

        if (Index == NumMappings - 1)
        {
            /* last mapping should free the irp if provided */
            Mapping->OriginalBuffer = Header->Data;
            Mapping->Irp = Irp;
        }

        ExInterlockedInsertTailList(&This->ListHead, &Mapping->Entry, &This->Lock);
        (void)InterlockedIncrement((volatile long*)&This->NumMappings);

        DPRINT("IIrpQueue_fnAddMapping NumMappings %u SizeOfMapping %lu NumDataAvailable %lu Mapping %p FrameSize %u\n", This->NumMappings, Mapping->BufferSize, This->NumDataAvailable, Mapping, This->MaxFrameSize);
    }

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

    CurEntry = ExInterlockedRemoveHeadList(&This->ListHead, &This->Lock);
    if (!CurEntry)
    {
        This->StartStream = FALSE;
        This->OutOfMapping = TRUE;
        return STATUS_UNSUCCESSFUL;
    }

    CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
    *Buffer = (PUCHAR)CurMapping->Buffer + This->CurrentOffset;
    *BufferSize = CurMapping->BufferSize - This->CurrentOffset;
    ExInterlockedInsertHeadList(&This->ListHead, &CurMapping->Entry, &This->Lock);
    This->OutOfMapping = FALSE;

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

    CurEntry = ExInterlockedRemoveHeadList(&This->ListHead, &This->Lock);
    CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);

    This->CurrentOffset += BytesWritten;
    This->NumDataAvailable -= BytesWritten;

    if (CurMapping->BufferSize <= This->CurrentOffset)
    {
        This->CurrentOffset = 0;
        InterlockedDecrement(&This->NumMappings);
        FreeMappingRoutine(CurMapping);
    }
    else
    {
        ExInterlockedInsertHeadList(&This->ListHead, &CurMapping->Entry, &This->Lock);
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
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    *Flags = 0;
    ASSERT(Tag != NULL);


    CurEntry = ExInterlockedRemoveHeadList(&This->ListHead, &This->Lock);
    if (!CurEntry)
    {
        This->OutOfMapping = TRUE;
        This->StartStream = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);

    *PhysicalAddress = MmGetPhysicalAddress(CurMapping->Buffer);
    *VirtualAddress = CurMapping->Buffer;
    *ByteCount = CurMapping->BufferSize;

    InterlockedDecrement(&This->NumMappings);
    This->NumDataAvailable -= CurMapping->BufferSize;

    if (CurMapping->OriginalBuffer)
    {
        /* last partial buffer */
        *Flags = 1;

        /* store tag */
        CurMapping->Tag = Tag;

        /* insert into list to free later */
        ExInterlockedInsertTailList(&This->FreeHead, &CurMapping->Entry, &This->Lock);
        DPRINT("IIrpQueue_fnGetMappingWithTag Tag %p Mapping %p\n", Tag, CurMapping);
    }
    else
    {
        /* we can free this entry now */
        FreeItem(CurMapping, TAG_PORTCLASS);
        DPRINT("IIrpQueue_fnGetMappingWithTag Tag %p Mapping %p FREED\n", Tag, CurMapping);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IIrpQueue_fnReleaseMappingWithTag(
    IN IIrpQueue *iface,
    IN PVOID Tag)
{
    PIRP_MAPPING CurMapping = NULL;
    PLIST_ENTRY CurEntry;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    DPRINT("IIrpQueue_fnReleaseMappingWithTag Tag %p\n", Tag);

    CurEntry = ExInterlockedRemoveHeadList(&This->FreeHead, &This->Lock);
    if (!CurMapping)
    {
        return STATUS_SUCCESS;
    }

    CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
    if (CurMapping->Tag != Tag)
    {
        /* the released mapping is not the last one */
        ExInterlockedInsertHeadList(&This->FreeHead, &CurMapping->Entry, &This->Lock);
        return STATUS_SUCCESS;
    }

    /* last mapping of the irp, free irp */
    DPRINT("IIrpQueue_fnReleaseMappingWithTag Tag %p Mapping %p FREED\n", Tag, CurMapping);

    FreeMappingRoutine(CurMapping);
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

VOID
NTAPI
IIrpQueue_fnPrintQueueStatus(
    IN IIrpQueue *iface)
{
    PIRP_MAPPING CurMapping = NULL;
    PLIST_ENTRY CurEntry;

    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;
    KeAcquireSpinLockAtDpcLevel(&This->Lock);

    CurEntry = This->ListHead.Flink;
    DPRINT("IIrpQueue_fnPrintQueueStatus  % u ===============\n", This->NumMappings);

    while (CurEntry != &This->ListHead)
    {
        CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
        DPRINT("Mapping %p Size %u Original %p\n", CurMapping, CurMapping->BufferSize, CurMapping->OriginalBuffer);
        CurEntry = CurEntry->Flink;
    }

    KeReleaseSpinLockFromDpcLevel(&This->Lock);
    DPRINT("IIrpQueue_fnPrintQueueStatus ===============\n");
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
    IIrpQueue_fnHasLastMappingFailed,
    IIrpQueue_fnPrintQueueStatus

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
