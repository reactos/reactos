/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/multimedia/portcls/irpstream.c
 * PURPOSE:         IRP Stream handling
 * PROGRAMMER:      Johannes Anderwald
 *
 * HISTORY:
 *                  27 Jan 07   Created
 */

#include "private.h"

typedef struct _IRP_MAPPING_
{
    KSSTREAM_HEADER *Header;
    PIRP Irp;
    struct _IRP_MAPPING_ * Next;
}IRP_MAPPING, *PIRP_MAPPING;

typedef struct
{
    IIrpQueueVtbl *lpVtbl;

    LONG ref;

    ULONG CurrentOffset;
    LONG NumMappings;
    ULONG NumDataAvailable;
    KSPIN_CONNECT *ConnectDetails;
    PKSDATAFORMAT_WAVEFORMATEX DataFormat;

    KDPC Dpc;
    PIRP_MAPPING FirstMap;
    PIRP_MAPPING LastMap;

    ULONG DpcActive;
    PIRP_MAPPING FreeMapHead;
    PIRP_MAPPING FreeMapTail;
    ULONG FreeDataSize;
    LONG FreeCount;

}IIrpQueueImpl;

VOID
NTAPI
DpcRoutine(
    IN struct _KDPC  *Dpc,
    IN PVOID  DeferredContext,
    IN PVOID  SystemArgument1,
    IN PVOID  SystemArgument2)
{
    PIRP_MAPPING CurMapping, NextMapping = NULL;
    ULONG Count;
    IIrpQueueImpl * This = (IIrpQueueImpl*)DeferredContext;

    CurMapping = (PIRP_MAPPING)SystemArgument1;
    ASSERT(CurMapping);

    Count = 0;
    while(CurMapping)
    {
        NextMapping = CurMapping->Next;

        This->FreeDataSize -= CurMapping->Header->DataUsed;

        if (CurMapping->Irp)
        {
            CurMapping->Irp->IoStatus.Information = CurMapping->Header->DataUsed;
            CurMapping->Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest(CurMapping->Irp, IO_SOUND_INCREMENT);
        }

        ExFreePool(CurMapping->Header->Data);
        ExFreePool(CurMapping->Header);

        ExFreePool(CurMapping);

        CurMapping = NextMapping;
        InterlockedDecrement(&This->FreeCount);

        Count++;
    }
    This->DpcActive = FALSE;
    DPRINT1("Freed %u Buffers / IRP Available Mappings %u\n", Count, This->NumMappings);
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
    IN PDEVICE_OBJECT DeviceObject)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    This->ConnectDetails = ConnectDetails;
    This->DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;

    KeInitializeDpc(&This->Dpc, DpcRoutine, (PVOID)This);

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

    Mapping = ExAllocatePool(NonPagedPool, sizeof(IRP_MAPPING));
    if (!Mapping)
        return STATUS_UNSUCCESSFUL;

    Mapping->Header = (KSSTREAM_HEADER*)Buffer;
    Mapping->Irp = Irp;
    Mapping->Next = NULL;

    //DPRINT1("FirstMap %p LastMap %p NumMappings %u\n", This->FirstMap, This->LastMap, This->NumMappings);

    if (!This->FirstMap)
        This->FirstMap = Mapping;
    else
        This->LastMap->Next = Mapping;

    This->LastMap = Mapping;

    InterlockedIncrement(&This->NumMappings);

    if (Irp)
    {
        Irp->IoStatus.Status = STATUS_PENDING;
        Irp->IoStatus.Information = 0;
        IoMarkIrpPending(Irp);
    }

    This->NumDataAvailable += Mapping->Header->DataUsed;

    DPRINT1("IIrpQueue_fnAddMapping NumMappings %u SizeOfMapping %lu NumDataAvailable %lu Irp %p\n", This->NumMappings, Mapping->Header->DataUsed, This->NumDataAvailable, Irp);
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

    if (This->FirstMap->Header->DataUsed <=This->CurrentOffset)
    {
        This->CurrentOffset = 0;
        Mapping = This->FirstMap;
        This->FirstMap = This->FirstMap->Next;

        if (!This->FirstMap)
            This->LastMap = NULL;

        This->FreeCount++;

        if (!This->FreeMapHead)
           This->FreeMapHead = Mapping;
        else
           This->FreeMapTail->Next = Mapping;

        This->FreeMapTail = Mapping;
        Mapping->Next = NULL;
        This->FreeDataSize += Mapping->Header->DataUsed;
        InterlockedDecrement(&This->NumMappings);
        This->NumDataAvailable -= Mapping->Header->DataUsed;


        if (This->FreeCount > 5 && This->DpcActive == FALSE)
        {
            Mapping = This->FreeMapHead;
            This->FreeMapHead = NULL;
            This->FreeMapTail = NULL;
            This->DpcActive = TRUE;
            KeInsertQueueDpc(&This->Dpc, (PVOID)Mapping, NULL);
        }
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

    if (This->DataFormat->WaveFormatEx.nAvgBytesPerSec < This->NumDataAvailable)
        Result = TRUE;
    else
        Result = FALSE;

    return Result;
}

BOOL
NTAPI
IIrpQueue_fnCancelBuffers(
    IN IIrpQueue *iface)
{
    PIRP_MAPPING Mapping;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    if (This->DpcActive)
        return FALSE;

    ASSERT(This->FirstMap == NULL);
    ASSERT(This->LastMap == NULL);

    if (This->FreeMapHead == NULL)
    {
        ASSERT(This->FreeMapTail == NULL);
        This->FreeMapTail = NULL;
        return TRUE;
    }

    Mapping = This->FreeMapHead;
    This->FreeMapHead = NULL;
    This->FreeMapTail = NULL;
    This->DpcActive = TRUE;
    KeInsertQueueDpc(&This->Dpc, (PVOID)Mapping, NULL);
    return FALSE;
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
    IIrpQueue_fnCancelBuffers
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
