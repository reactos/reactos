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
    KDPC Dpc;

    ULONG NumTags;
    PVOID * Tag;
    LONG ReferenceCount;

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

    PIRP_MAPPING FirstMap;
    PIRP_MAPPING LastMap;

    KSPIN_LOCK Lock;
    LIST_ENTRY ListHead;

    PVOID LastTag;
    BOOL OutOfMapping;
    ULONG MaxFrameSize;

}IIrpQueueImpl;

VOID
NTAPI
DpcRoutine(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2)
{
    PIRP_MAPPING CurMapping;

    CurMapping = (PIRP_MAPPING)SystemArgument1;
    ASSERT(CurMapping);

    if (CurMapping->Irp)
    {
        CurMapping->Irp->IoStatus.Information = CurMapping->Header->FrameExtent;
        CurMapping->Irp->IoStatus.Status = STATUS_SUCCESS;
        IoCompleteRequest(CurMapping->Irp, IO_SOUND_INCREMENT);
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
    This->LastTag = (PVOID)0x12345678;

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
    KeInitializeDpc(&Mapping->Dpc, DpcRoutine, (PVOID)Mapping);
    KeSetImportanceDpc(&Mapping->Dpc, HighImportance);

    if (This->MaxFrameSize)
    {
        Mapping->NumTags = max((Mapping->Header->DataUsed / This->MaxFrameSize) + 1, 1);
        Mapping->Tag = AllocateItem(NonPagedPool, sizeof(PVOID) * This->NumMappings, TAG_PORTCLASS);
    }

    This->NumDataAvailable += Mapping->Header->DataUsed;

    DPRINT("IIrpQueue_fnAddMapping NumMappings %u SizeOfMapping %lu NumDataAvailable %lu Irp %p\n", This->NumMappings, Mapping->Header->DataUsed, This->NumDataAvailable, Irp);

    /* FIXME use InterlockedCompareExchangePointer */
    if (InterlockedCompareExchange((volatile long *)&This->FirstMap, (LONG)Mapping, (LONG)0) != 0)
        ExInterlockedInsertTailList(&This->ListHead, &Mapping->Entry, &This->Lock);

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
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    if (!This->FirstMap)
        return STATUS_UNSUCCESSFUL;

    *Buffer = (PUCHAR)This->FirstMap->Header->Data + This->CurrentOffset;
    *BufferSize = This->FirstMap->Header->DataUsed - This->CurrentOffset;

    return STATUS_SUCCESS;
}



VOID
NTAPI
IIrpQueue_fnUpdateMapping(
    IN IIrpQueue *iface,
    IN ULONG BytesWritten)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;
    PIRP_MAPPING Mapping;

    This->CurrentOffset += BytesWritten;
    This->NumDataAvailable -= BytesWritten;

    if (This->FirstMap->Header->DataUsed <=This->CurrentOffset)
    {
        This->CurrentOffset = 0;
        Mapping = (PIRP_MAPPING)ExInterlockedRemoveHeadList(&This->ListHead, &This->Lock);

        InterlockedDecrement(&This->NumMappings);

        KeInsertQueueDpc(&This->FirstMap->Dpc, (PVOID)This->FirstMap, NULL);
        (void)InterlockedExchangePointer((PVOID volatile*)&This->FirstMap, (PVOID)Mapping);
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
    KIRQL OldIrql;
    PIRP_MAPPING CurMapping;
    PIRP_MAPPING Result;
    PLIST_ENTRY CurEntry;
    ULONG Index;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    *Flags = 0;
    Result = NULL;

    KeAcquireSpinLock(&This->Lock, &OldIrql);

    CurEntry = This->ListHead.Flink;

    while (CurEntry != &This->ListHead)
    {
        CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
        for(Index = 0; Index < CurMapping->NumTags; Index++)
        {
            if (This->LastTag == (PVOID)0x12345678)
            {
                CurMapping->Tag[Index] = Tag;
                CurMapping->ReferenceCount++;
                Result = CurMapping;
                if (Index + 1 == CurMapping->NumTags - 1)
                {
                    /* indicate end of packet */
                    *Flags = 1;
                }
                break;
            }


            if (CurMapping->Tag[Index] == This->LastTag)
            {
                if (Index + 1 < CurMapping->NumTags)
                {
                    CurMapping->Tag[Index+1] = Tag;
                    CurMapping->ReferenceCount++;
                    Result = CurMapping;

                    if (Index + 1 == CurMapping->NumTags - 1)
                    {
                        /* indicate end of packet */
                        *Flags = 1;
                    }
                    break;
                }

                CurEntry = CurEntry->Flink;
                if (&This->ListHead == CurEntry)
                {
                    This->OutOfMapping = TRUE;
                    break;
                }
                Result = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
                Result->Tag[0] = Tag;
                Result->ReferenceCount++;
                break;
            }
        }
        CurEntry = CurEntry->Flink;
    }

    KeReleaseSpinLock(&This->Lock, OldIrql);
    if (!Result)
        return STATUS_UNSUCCESSFUL;

    Result->Tag = Tag;
    *PhysicalAddress = MmGetPhysicalAddress(Result->Header->Data);
    *VirtualAddress = Result->Header->Data;
    *ByteCount = Result->Header->DataUsed;
    This->LastTag = Tag;
    return STATUS_SUCCESS;
}

VOID
NTAPI
IIrpQueue_fnReleaseMappingWithTag(
    IN IIrpQueue *iface,
    IN PVOID Tag)
{
    KIRQL OldIrql;
    PIRP_MAPPING CurMapping;
    PLIST_ENTRY CurEntry;
    ULONG Index;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    KeAcquireSpinLock(&This->Lock, &OldIrql);
    CurEntry = This->ListHead.Flink;

    while (CurEntry != &This->ListHead)
    {
        CurMapping = CONTAINING_RECORD(CurEntry, IRP_MAPPING, Entry);
        for(Index = 0; Index < CurMapping->NumTags; Index++)
        {
            if (CurMapping->Tag[Index] == Tag)
            {
                CurMapping->ReferenceCount--;
                if (!CurMapping->ReferenceCount)
                {
                    RemoveEntryList(&CurMapping->Entry);
                    if (CurMapping->Irp)
                    {
                        CurMapping->Irp->IoStatus.Information = CurMapping->Header->FrameExtent;
                        CurMapping->Irp->IoStatus.Status = STATUS_SUCCESS;
                        IoCompleteRequest(CurMapping->Irp, IO_SOUND_INCREMENT);
                    }
                    ExFreePool(CurMapping->Header->Data);
                    ExFreePool(CurMapping->Header);
                    ExFreePool(CurMapping->Tag);
                    ExFreePool(CurMapping);
                }
                break;
            }
        }
        CurEntry = CurEntry->Flink;
    }

    KeReleaseSpinLock(&This->Lock, OldIrql);
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
    IIrpQueue_fnReleaseMappingWithTag
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
