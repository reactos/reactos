/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/wdm/audio/backpln/portcls/irpstream.c
 * PURPOSE:         IRP Stream handling
 * PROGRAMMER:      Johannes Anderwald
 */

#include "private.h"


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

    KSPIN_LOCK IrpListLock;
    LIST_ENTRY IrpList;
    LIST_ENTRY FreeIrpList;
    PIRP Irp;
    PVOID SilenceBuffer;

    ULONG OutOfMapping;
    ULONG MaxFrameSize;
    ULONG Alignment;
    ULONG MinimumDataThreshold;

}IIrpQueueImpl;

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
    IN ULONG Alignment,
    IN PVOID SilenceBuffer)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    This->ConnectDetails = ConnectDetails;
    This->DataFormat = (PKSDATAFORMAT_WAVEFORMATEX)DataFormat;
    This->MaxFrameSize = FrameSize;
    This->SilenceBuffer = SilenceBuffer;
    This->Alignment = Alignment;
    This->MinimumDataThreshold = ((PKSDATAFORMAT_WAVEFORMATEX)DataFormat)->WaveFormatEx.nAvgBytesPerSec / 3;

    InitializeListHead(&This->IrpList);
    InitializeListHead(&This->FreeIrpList);
    KeInitializeSpinLock(&This->IrpListLock);

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
    PKSSTREAM_HEADER Header;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    /* FIXME
     * irp should contain the stream header...
     */

    /* get stream header */
    Header = (KSSTREAM_HEADER*)Buffer;

    /* dont exceed max frame size */
    ASSERT(This->MaxFrameSize >= Header->DataUsed);

    /* hack untill stream probing is ready */
    Irp->Tail.Overlay.DriverContext[2] = (PVOID)Header;

    /* increment num mappings */
    InterlockedIncrement(&This->NumMappings);

    /* increment num data available */
    This->NumDataAvailable += Header->DataUsed;

    /* mark irp as pending */
    IoMarkIrpPending(Irp);

    /* add irp to cancelable queue */
    KsAddIrpToCancelableQueue(&This->IrpList, &This->IrpListLock, Irp, KsListEntryTail, NULL);

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IIrpQueue_fnGetMapping(
    IN IIrpQueue *iface,
    OUT PUCHAR * Buffer,
    OUT PULONG BufferSize)
{
    PIRP Irp;
    ULONG Offset;
    PKSSTREAM_HEADER StreamHeader;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    /* check if there is an irp in the partially processed */
    if (This->Irp)
    {
        /* use last irp */
        if (This->Irp->Cancel == FALSE)
        {
            Irp = This->Irp;
            Offset = This->CurrentOffset;
        }
        else
        {
            /* irp has been cancelled */
            This->Irp->IoStatus.Status = STATUS_CANCELLED;
            IoCompleteRequest(This->Irp, IO_NO_INCREMENT);
            This->Irp = Irp = NULL;
        }
    }
    else
    {
        /* get a fresh new irp from the queue */
        This->Irp = Irp = KsRemoveIrpFromCancelableQueue(&This->IrpList, &This->IrpListLock, KsListEntryHead, KsAcquireAndRemoveOnlySingleItem);
        This->CurrentOffset = Offset = 0;
    }

    if (!Irp)
    {
        /* no irp available, use silence buffer */
        *Buffer = This->SilenceBuffer;
        *BufferSize = This->MaxFrameSize;
        /* flag for port wave pci driver */
        This->OutOfMapping = TRUE;
        /* indicate flag to restart fast buffering */
        This->StartStream = FALSE;
        return STATUS_SUCCESS;
    }

    /* HACK get stream header */
    StreamHeader = (PKSSTREAM_HEADER)Irp->Tail.Overlay.DriverContext[2];

    /* sanity check */
    ASSERT(StreamHeader);

    /* store buffersize */
    *BufferSize = StreamHeader->DataUsed - Offset;

    /* store buffer */
    *Buffer = &((PUCHAR)StreamHeader->Data)[Offset];

    /* unset flag that no irps are available */
    This->OutOfMapping = FALSE;

    return STATUS_SUCCESS;
}

VOID
NTAPI
IIrpQueue_fnUpdateMapping(
    IN IIrpQueue *iface,
    IN ULONG BytesWritten)
{
    PKSSTREAM_HEADER StreamHeader;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    if (!This->Irp)
    {
        /* silence buffer was used */
        return;
    }

    /* HACK get stream header */
    StreamHeader = (PKSSTREAM_HEADER)This->Irp->Tail.Overlay.DriverContext[2];

    /* add to current offset */
    This->CurrentOffset += BytesWritten;

    /* decrement available data counter */
    This->NumDataAvailable -= BytesWritten;

    if (This->CurrentOffset >= StreamHeader->DataUsed)
    {
        /* irp has been processed completly */
        This->Irp->IoStatus.Status = STATUS_SUCCESS;

        /* frame extend contains the original request size, DataUsed contains the real buffer size
         * is different when kmixer performs channel conversion, upsampling etc
         */
        This->Irp->IoStatus.Information = StreamHeader->FrameExtent;

        /* free stream data, no tag as wdmaud.drv does it atm */
        ExFreePool(StreamHeader->Data);

        /* free stream header, no tag as wdmaud.drv allocates it atm */
        ExFreePool(StreamHeader);

        /* complete the request */
        IoCompleteRequest(This->Irp, IO_SOUND_INCREMENT);
        /* remove irp as it is complete */
        This->Irp = NULL;
        This->CurrentOffset = 0;
    }
}

ULONG
NTAPI
IIrpQueue_fnNumMappings(
    IN IIrpQueue *iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    /* returns the amount of mappings available */
    return This->NumMappings;
}

ULONG
NTAPI
IIrpQueue_fnNumData(
    IN IIrpQueue *iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;
    /* returns the amount of audio stream data available */
    return This->NumDataAvailable;
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

    if (This->MinimumDataThreshold < This->NumDataAvailable)
    {
        This->StartStream = TRUE;
        Result = TRUE;
    }
    else
    {
        Result = FALSE;
    }
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
    This->MinimumDataThreshold = This->DataFormat->WaveFormatEx.nAvgBytesPerSec / 3;
    This->StartStream = FALSE;
    This->NumDataAvailable = 0;
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
    PKSSTREAM_HEADER StreamHeader;
    PIRP Irp;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    *Flags = 0;
    ASSERT(Tag != NULL);

    /* get an irp from the queue */
    Irp = KsRemoveIrpFromCancelableQueue(&This->IrpList, &This->IrpListLock, KsListEntryHead, KsAcquireAndRemoveOnlySingleItem);

    /* check if there is an irp */
    if (!Irp)
    {
        /* no irp available */
        This->OutOfMapping = TRUE;
        This->StartStream = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    /* HACK get stream header */
    StreamHeader = (PKSSTREAM_HEADER)Irp->Tail.Overlay.DriverContext[2];

    /* store mapping in the free list */
    ExInterlockedInsertTailList(&This->FreeIrpList, &Irp->Tail.Overlay.ListEntry, &This->IrpListLock);

    /* return mapping */
    *PhysicalAddress = MmGetPhysicalAddress(StreamHeader->Data);
    *VirtualAddress = StreamHeader->Data;
    *ByteCount = StreamHeader->DataUsed;

    /* decrement mapping count */
    InterlockedDecrement(&This->NumMappings);
    /* decrement num data available */
    This->NumDataAvailable -= StreamHeader->DataUsed;

    /* store tag in irp */
    Irp->Tail.Overlay.DriverContext[3] = Tag;

    /* done */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IIrpQueue_fnReleaseMappingWithTag(
    IN IIrpQueue *iface,
    IN PVOID Tag)
{
    PIRP Irp;
    PLIST_ENTRY CurEntry;
    PKSSTREAM_HEADER StreamHeader;
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    DPRINT("IIrpQueue_fnReleaseMappingWithTag Tag %p\n", Tag);

    /* remove irp from used list */
    CurEntry = ExInterlockedRemoveHeadList(&This->FreeIrpList, &This->IrpListLock);
    /* sanity check */
    ASSERT(CurEntry);

    /* get irp from list entry */
    Irp = (PIRP)CONTAINING_RECORD(CurEntry, IRP, Tail.Overlay.ListEntry);

    /* HACK get stream header */
    StreamHeader = (PKSSTREAM_HEADER)Irp->Tail.Overlay.DriverContext[2];

    /* driver must release items in the same order */
    ASSERT(Irp->Tail.Overlay.DriverContext[3] == Tag);

    /* irp has been processed completly */
    Irp->IoStatus.Status = STATUS_SUCCESS;

    /* frame extend contains the original request size, DataUsed contains the real buffer size
     * is different when kmixer performs channel conversion, upsampling etc
     */
    Irp->IoStatus.Information = StreamHeader->FrameExtent;

    /* free stream data, no tag as wdmaud.drv does it atm */
    ExFreePool(StreamHeader->Data);

    /* free stream header, no tag as wdmaud.drv allocates it atm */
    ExFreePool(StreamHeader);

    /* complete the request */
    IoCompleteRequest(Irp, IO_SOUND_INCREMENT);

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

}

VOID
NTAPI
IIrpQueue_fnSetMinimumDataThreshold(
    IN IIrpQueue *iface,
    ULONG MinimumDataThreshold)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    This->MinimumDataThreshold = MinimumDataThreshold;
}

ULONG
NTAPI
IIrpQueue_fnGetMinimumDataThreshold(
    IN IIrpQueue *iface)
{
    IIrpQueueImpl * This = (IIrpQueueImpl*)iface;

    return This->MinimumDataThreshold;
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
    IIrpQueue_fnNumData,
    IIrpQueue_fnMinimumDataAvailable,
    IIrpQueue_fnCancelBuffers,
    IIrpQueue_fnUpdateFormat,
    IIrpQueue_fnGetMappingWithTag,
    IIrpQueue_fnReleaseMappingWithTag,
    IIrpQueue_fnHasLastMappingFailed,
    IIrpQueue_fnPrintQueueStatus,
    IIrpQueue_fnSetMinimumDataThreshold,
    IIrpQueue_fnGetMinimumDataThreshold
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

