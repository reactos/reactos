/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel Streaming
 * FILE:            drivers/ksfilter/ks/queue.c
 * PURPOSE:         KS Queue Functions
 * PROGRAMMER:      Johannes Anderwald
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

typedef struct
{
    IKsQueueVtbl * lpVtbl;
    LONG ref;

    LIST_ENTRY          PointerList;
    LIST_ENTRY          FrameList;
    KSPIN_LOCK          FrameListLock;

    PKSPIN              Pin;            // PKSPIN                       
    KSPSTREAM_POINTER * m_Leading;      // leading edge stream pointer
    KSPSTREAM_POINTER * m_Trailing;     // trailing edge stream pointer

}IKsQueueImpl;


NTSTATUS
NTAPI
IKsQueue_fnQueryInterface(
	IKsQueue * iface,
	IN REFIID refiid,
	OUT PVOID* Output)
{
	IKsQueueImpl* This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);

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
IKsQueue_fnAddRef(
	IKsQueue * iface)
{
	IKsQueueImpl* This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);
	return InterlockedIncrement(&This->ref);
}

ULONG
NTAPI
IKsQueue_fnRelease(
	IKsQueue * iface)
{
	IKsQueueImpl* This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);

	InterlockedDecrement(&This->ref);

	if (This->ref == 0)
	{
		FreeItem(This);
		return 0;
	}
	/* Return new reference count */
	return This->ref;
}



NTSTATUS
NTAPI
IKsQueue_fnTransferKsIrp(
    IN IKsQueue * iface,
    IN PIRP Irp,
    OUT IKsTransport ** Transport)
{
    KSPFRAME_HEADER * FrameHeader;
    KSPIRP_FRAMING * IrpFraming;
    PIO_STACK_LOCATION IoStack;
    PKSSTREAM_HEADER Header;
    ULONG NumHeaders;
    KIRQL OldLevel;
    IKsQueueImpl* This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);

    if (Irp->RequestorMode == UserMode)
        Header = (PKSSTREAM_HEADER)Irp->AssociatedIrp.SystemBuffer;
    else
        Header = (PKSSTREAM_HEADER)Irp->UserBuffer;

    IoStack = IoGetCurrentIrpStackLocation(Irp);

    /* calculate num headers */
    NumHeaders = IoStack->Parameters.DeviceIoControl.OutputBufferLength / Header->Size;

    /* assume headers of same length */
    ASSERT(IoStack->Parameters.DeviceIoControl.OutputBufferLength % Header->Size == 0);

    /* FIXME support multiple stream headers */
    ASSERT(NumHeaders == 1);

    IrpFraming = AllocateItem(NonPagedPool, sizeof(KSPIRP_FRAMING));
    if (!IrpFraming)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    FrameHeader = AllocateItem(NonPagedPool, sizeof(KSPFRAME_HEADER));
    if (!FrameHeader)
    {
        FreeItem(IrpFraming);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize frame header */
    FrameHeader->IrpFraming = IrpFraming;
    FrameHeader->OriginalIrp = Irp;
    FrameHeader->Irp = Irp;
    FrameHeader->Mdl = Irp->MdlAddress;
    FrameHeader->StreamHeader = Header;
    FrameHeader->Queue = This;

    /* initialize irp framing*/
    IrpFraming->QueuedFrameHeaderCount = 1;
    IrpFraming->FrameHeaders = FrameHeader;
    IrpFraming->RefCount = 0;

    KeAcquireSpinLock(&This->FrameListLock, &OldLevel);
    InsertTailList(&This->FrameList, &FrameHeader->ListEntry);
    KeReleaseSpinLock(&This->FrameListLock, OldLevel);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsQueue_fnDiscardKsIrp(
    IN IKsQueue * iface,
    IN PIRP Irp,
    OUT IKsTransport ** Transport)
{
	UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsQueue_fnConnect(
	IN IKsQueue * iface,
	IN IKsTransport *T1,
	OUT IKsTransport **OutTransport1,
	OUT IKsTransport **OutTransport2,
	IN KSPIN_DATAFLOW DataFlow)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsQueue_fnSetDeviceState(
	IN IKsQueue * iface,
	IN KSSTATE ToState,
	IN KSSTATE FromState,
	IN PCHAR Message)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsQueue_fnSetResetState(
	IKsQueue * iface,
	IN KSRESET Reset,
	IN PCHAR Message)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsQueue_fnGetTransportConfig(
	IN IKsQueue * iface,
	IN KSPTRANSPORTCONFIG * TransportConfig,
	OUT IKsTransport **OutTransport,
	OUT IKsTransport **OutTransport2)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
IKsQueue_fnSetTransportConfig(
	IN IKsQueue * iface,
	IN KSPTRANSPORTCONFIG *a2,
	IN PCHAR Message,
	OUT IKsTransport **OutTransport,
	OUT IKsTransport **OutTransport2)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
IKsQueue_fnResetTransportConfig(
	IN IKsQueue * iface,
	IN IKsTransport ** NextTransport,
	IN IKsTransport ** PrevTransport)
{
	UNIMPLEMENTED;
}

NTSTATUS
NTAPI
IKsQueue_fnCloneStreamPointer(
    IN IKsQueue * iface,
    OUT KSPSTREAM_POINTER ** CloneStreamPointer,
    IN PFNKSSTREAMPOINTER CancelCallback,
    IN ULONG ContextSize,
    IN KSPSTREAM_POINTER* StreamPointer,
    IN ULONG StreamPointerType)
{
    KSPSTREAM_POINTER * NewStreamPointer;
    KIRQL OldLevel;
    IKsQueueImpl* This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);

    /* sanity check*/
    ASSERT(StreamPointer);
    ASSERT(CloneStreamPointer);

    NewStreamPointer = AllocateItem(NonPagedPool, sizeof(KSPSTREAM_POINTER) + ContextSize);
    if (!NewStreamPointer)
    {
        /* no memory */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeAcquireSpinLock(&This->FrameListLock, &OldLevel);

    /* init new stream pointer */
    RtlCopyMemory(NewStreamPointer, StreamPointer, sizeof(KSPSTREAM_POINTER));
    if (ContextSize)
    {
        NewStreamPointer->StreamPointer.Context = (NewStreamPointer + 1);
        RtlZeroMemory(NewStreamPointer->StreamPointer.Context, ContextSize);
    }

    NewStreamPointer->Type = StreamPointerType;
    NewStreamPointer->CancelCallback = CancelCallback;
    if (NewStreamPointer->FrameHeader)
    {
        NewStreamPointer->FrameHeader->RefCount++;
        NewStreamPointer->FrameHeader->IrpFraming->RefCount++;
    }

    InsertTailList(&This->PointerList, &NewStreamPointer->ListEntry);

    /* release lock */
    KeReleaseSpinLock(&This->FrameListLock, OldLevel);

    *CloneStreamPointer = NewStreamPointer;
    return STATUS_SUCCESS;
}

VOID
NTAPI
IKsQueue_fnDeleteStreamPointer(
	IN IKsQueue * iface,
	IN KSPSTREAM_POINTER * StreamPointer)
{
	UNIMPLEMENTED;
}

KSPSTREAM_POINTER*
NTAPI
IKsQueue_fnLockStreamPointer(
	IN IKsQueue * iface,
	IN KSPSTREAM_POINTER * StreamPointer)
{
	UNIMPLEMENTED;
	return NULL;
}

VOID
NTAPI
IKsQueue_fnUnlockStreamPointer(
    IN IKsQueue * iface,
    IN KSPSTREAM_POINTER * StreamPointer,
    IN enum KSPSTREAM_POINTER_MOTION Motion)
{
	UNIMPLEMENTED;
}

VOID
NTAPI
IKsQueue_fnAdvanceUnlockedStreamPointer(
	IN IKsQueue * iface,
	IN KSPSTREAM_POINTER * StreamPointer)
{
	UNIMPLEMENTED;
}

KSPSTREAM_POINTER *
NTAPI
IKsQueue_fnGetLeadingStreamPointer(
	IN IKsQueue * iface,
	IN KSSTREAM_POINTER_STATE State)
{
    IKsQueueImpl * This;

    This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);

    if (State != KSSTREAM_POINTER_STATE_LOCKED)
    {
        return This->m_Leading;
    }
    else
    {
        if (iface->lpVtbl->LockStreamPointer(iface, This->m_Leading) != NULL)
        {
            return This->m_Leading;
        }
    }
    return NULL;
}

KSPSTREAM_POINTER *
NTAPI
IKsQueue_fnGetTrailingStreamPointer(
	IN IKsQueue * iface,
	IN KSSTREAM_POINTER_STATE State)
{
	IKsQueueImpl * This;

	This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);

	if (State != KSSTREAM_POINTER_STATE_LOCKED)
	{
		return This->m_Trailing;
	}
	else
	{
		if (iface->lpVtbl->LockStreamPointer(iface, This->m_Trailing) != NULL)
		{
			return This->m_Trailing;
		}
	}
	return NULL;
}

VOID
NTAPI
IKsQueue_fnScheduleTimeout(
	IN IKsQueue * iface,
	IN KSPSTREAM_POINTER * StreamPointer,
	IN PFNKSSTREAMPOINTER CancelRoutine,
	IN ULONGLONG TimeOut)
{
	UNIMPLEMENTED;
}

VOID
NTAPI
IKsQueue_fnCancelTimeout(
	IN IKsQueue * iface,
	IN KSPSTREAM_POINTER * StreamPointer)
{
	UNIMPLEMENTED;
}

KSPSTREAM_POINTER*
NTAPI
IKsQueue_fnGetFirstClone(
    IN IKsQueue * iface)
{
	UNIMPLEMENTED;
    return NULL;
}

KSPSTREAM_POINTER*
NTAPI
IKsQueue_fnGetNextClone(
	IN IKsQueue * iface,
	IN KSPSTREAM_POINTER * StreamPointer)
{
	UNIMPLEMENTED;
	return NULL;
}

VOID
NTAPI
IKsQueue_fnGetAvailableByteCount(
	IN IKsQueue * iface,
	IN PULONG InputDataBytes,
	IN PULONG OutputBufferBytes)
{
	UNIMPLEMENTED;
}

VOID
NTAPI
IKsQueue_fnUpdateByteAvailability(
	IN IKsQueue * iface,
	IN KSPSTREAM_POINTER * StreamPointer,
	IN ULONG InUsed,
	IN ULONG OutUsed)
{
	UNIMPLEMENTED;
}

NTSTATUS
NTAPI
IKsQueue_fnSetStreamPointerStatusCode(
    IN IKsQueue * iface,
    IN KSPSTREAM_POINTER * StreamPointer,
    IN NTSTATUS StatusCode)
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;
}

VOID
NTAPI
IKsQueue_fnRegisterFrameDismissalCallback(
    IN IKsQueue * iface,
    IN KSPSTREAM_POINTER * StreamPointer)
{
	UNIMPLEMENTED;
}

UCHAR
NTAPI
IKsQueue_fnGeneratesMappings(
    IKsQueue * iface)
{
	UNIMPLEMENTED;
    return 0;
}

VOID
NTAPI
IKsQueue_fnCopyFrame(
    IN IKsQueue * iface,
    IN KSPSTREAM_POINTER * Src,
    IN KSPSTREAM_POINTER * Target)
{
	UNIMPLEMENTED;
}

IKsQueueVtbl vt_IKsQueue = 
{
    IKsQueue_fnQueryInterface,                           
	IKsQueue_fnAddRef,                                   
	IKsQueue_fnRelease,
	IKsQueue_fnTransferKsIrp,
	IKsQueue_fnDiscardKsIrp,
	IKsQueue_fnConnect,
	IKsQueue_fnSetDeviceState,
	IKsQueue_fnSetResetState,
	IKsQueue_fnGetTransportConfig,
	IKsQueue_fnSetTransportConfig,
	IKsQueue_fnResetTransportConfig,
	IKsQueue_fnCloneStreamPointer,
	IKsQueue_fnDeleteStreamPointer,
	IKsQueue_fnLockStreamPointer,                        
	IKsQueue_fnUnlockStreamPointer,                      
	IKsQueue_fnAdvanceUnlockedStreamPointer,
	IKsQueue_fnGetLeadingStreamPointer,
	IKsQueue_fnGetTrailingStreamPointer,
	IKsQueue_fnScheduleTimeout,
	IKsQueue_fnCancelTimeout,
	IKsQueue_fnGetFirstClone,
	IKsQueue_fnGetNextClone,
	IKsQueue_fnGetAvailableByteCount,
	IKsQueue_fnUpdateByteAvailability,
	IKsQueue_fnSetStreamPointerStatusCode,
	IKsQueue_fnRegisterFrameDismissalCallback,
	IKsQueue_fnGeneratesMappings,
	IKsQueue_fnCopyFrame
};

NTSTATUS
NTAPI
IKsQueue_fnCreateStreamPointer(
    IN IKsQueue * iface,
    OUT KSPSTREAM_POINTER **OutStreamPointer)
{
    IKsQueueImpl * This;
    KSPSTREAM_POINTER * StreamPtr;

    This = CONTAINING_RECORD(iface, IKsQueueImpl, lpVtbl);

    StreamPtr = AllocateItem(NonPagedPool, sizeof(KSPSTREAM_POINTER));
    if (!StreamPtr)
    {
        // no memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // init stream pointer
    StreamPtr->State = KSPSTREAM_POINTER_STATE_UNLOCKED;
    StreamPtr->Type = KSPSTREAM_POINTER_TYPE_NORMAL;
    StreamPtr->StreamPointer.Pin = This->Pin;
    if (This->Pin->DataFlow == KSPIN_DATAFLOW_IN)
    {
        StreamPtr->StreamPointer.Offset = &StreamPtr->StreamPointer.OffsetOut;
    }
    else
    {
        StreamPtr->StreamPointer.Offset = &StreamPtr->StreamPointer.OffsetIn;
    }
    *OutStreamPointer = StreamPtr;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
IKsQueue_fnInit(
    OUT IKsQueue **OutQueue,
    IN PKSPIN Pin,
    IN ULONG PinDescriptorFlags)
{
    IKsQueueImpl * This;
    NTSTATUS Status;
    IKsQueue * Queue;

    This = AllocateItem(NonPagedPool, sizeof(IKsQueueImpl));
    if (!This)
    {
        // no memory
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* init queue */
    This->lpVtbl = &vt_IKsQueue;
    This->ref = 1;
    This->Pin = Pin;
    InitializeListHead(&This->FrameList);
    InitializeListHead(&This->PointerList);
    KeInitializeSpinLock(&This->FrameListLock);

    Queue = (IKsQueue *)&This->lpVtbl;
    Status = IKsQueue_fnCreateStreamPointer(Queue, &This->m_Leading);
    if (NT_SUCCESS(Status))
    {
        if (PinDescriptorFlags & KSPIN_FLAG_DISTINCT_TRAILING_EDGE)
        {
            Status = IKsQueue_fnCreateStreamPointer(Queue, &This->m_Trailing);
        }
    }

    if (NT_SUCCESS(Status))
    {
        *OutQueue = (IKsQueue *)This->lpVtbl;
    }
    return Status;
}


/*
	@implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerSetStatusCode(
    IN PKSSTREAM_POINTER StreamPointer,
    IN NTSTATUS Status)
{
    IKsQueue * Queue;
    KSPSTREAM_POINTER * StreamPtr;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->CKsQueue);

    Queue = StreamPtr->CKsQueue;
    return Queue->lpVtbl->SetStreamPointerStatusCode(Queue, StreamPtr, Status);
}

/*
	@implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerLock(
    IN PKSSTREAM_POINTER StreamPointer)
{
    IKsQueue * Queue;
    KSPSTREAM_POINTER * StreamPtr;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->State != KSPSTREAM_POINTER_STATE_LOCKED);
    ASSERT(StreamPtr->CKsQueue);

    if (StreamPtr->State)
    {
        // invalid state
        return STATUS_DEVICE_NOT_READY;
    }

    Queue = StreamPtr->CKsQueue;
    StreamPtr = Queue->lpVtbl->LockStreamPointer(Queue, StreamPtr);
    if (StreamPtr)
    {
        return STATUS_SUCCESS;
    }
    return STATUS_DEVICE_NOT_READY;
}

/*
	@implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN BOOLEAN Eject)
{
    IKsQueue * Queue;
    KSPSTREAM_POINTER * StreamPtr;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->State != KSPSTREAM_POINTER_STATE_UNLOCKED);
    ASSERT(StreamPtr->CKsQueue);

    if (StreamPtr->State == KSPSTREAM_POINTER_STATE_LOCKED)
    {
        Queue = StreamPtr->CKsQueue;
        Queue->lpVtbl->UnlockStreamPointer(Queue, StreamPtr, Eject != 0);
    }

}

/*
	@implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerAdvanceOffsetsAndUnlock(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject)
{
    IKsQueue * Queue;
    KSPSTREAM_POINTER * StreamPtr;
    BOOLEAN NeedsEject;
    BOOLEAN StreamPointerInAdjusted = FALSE;
    BOOLEAN StreamPointerOutAdjusted = FALSE;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->State != KSPSTREAM_POINTER_STATE_UNLOCKED);
    ASSERT(StreamPtr->CKsQueue);

    if (StreamPtr->State != KSPSTREAM_POINTER_STATE_LOCKED)
    {
        // invalid state
        return;
    }

    if (StreamPointer->OffsetIn.Data)
    {
        if (StreamPointer->OffsetIn.Count == 0 || InUsed)
        {
            ASSERT(InUsed <= StreamPointer->OffsetIn.Remaining);

            /* adjust stream pointer */
            StreamPointer->OffsetIn.Data += InUsed;

            NeedsEject = StreamPointer->OffsetIn.Remaining == InUsed;

            StreamPointer->OffsetIn.Remaining -= InUsed;

            if (NeedsEject)
            {
                Eject = TRUE;
            }
            StreamPointerInAdjusted = TRUE;
        }
    }

    if (StreamPointer->OffsetOut.Data)
    {
        if (StreamPointer->OffsetOut.Count == 0 || OutUsed)
        {
            ASSERT(OutUsed <= StreamPointer->OffsetOut.Remaining);

            /* adjust stream pointer */
            StreamPointer->OffsetOut.Data += OutUsed;

            NeedsEject = StreamPointer->OffsetOut.Remaining == OutUsed;

            StreamPointer->OffsetOut.Remaining -= OutUsed;

            if (NeedsEject)
            {
                Eject = TRUE;
            }
            StreamPointerOutAdjusted = TRUE;
        }
    }

    Queue = StreamPtr->CKsQueue;
    Queue->lpVtbl->UpdateByteAvailability(Queue,
        StreamPtr,
        StreamPointerInAdjusted ? InUsed : 0,
        StreamPointerOutAdjusted ? OutUsed : 0);

    Queue->lpVtbl->UnlockStreamPointer(Queue, StreamPtr, Eject ? KSPSTREAM_POINTER_MOTION_ADVANCE : KSPSTREAM_POINTER_MOTION_NONE);
}

/*
	@implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerDelete(
    IN PKSSTREAM_POINTER StreamPointer)
{
    KSPSTREAM_POINTER * Ptr;
    IKsQueue * iface;

    ASSERT(StreamPointer);

    Ptr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);
    ASSERT(Ptr);
    ASSERT(Ptr->CKsQueue);
    iface = (IKsQueue*)Ptr->CKsQueue;
    iface->lpVtbl->DeleteStreamPointer(iface, Ptr);
}

/*
	@implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerClone(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER CancelCallback OPTIONAL,
    IN ULONG ContextSize,
    OUT PKSSTREAM_POINTER* CloneStreamPointer)
{
    KSPSTREAM_POINTER * StreamPtr;
    KSPSTREAM_POINTER * Result;
    IKsQueue * Queue;
    NTSTATUS Status;

    ASSERT(StreamPointer);
    ASSERT(CloneStreamPointer);

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);
    ASSERT(StreamPtr->CKsQueue);

    if (StreamPtr->State > 0 && StreamPtr->State != KSPSTREAM_POINTER_STATE_LOCKED)
    {
        // invalid state
        return STATUS_DEVICE_NOT_READY;
    }

    Queue = StreamPtr->CKsQueue;
    Status = Queue->lpVtbl->CloneStreamPointer(Queue, &Result, CancelCallback, ContextSize, StreamPtr, 0);
    if (NT_SUCCESS(Status))
    {
        *CloneStreamPointer = &Result->StreamPointer;
    }
    return Status;
}

/*
	@implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerAdvanceOffsets(
    IN PKSSTREAM_POINTER StreamPointer,
    IN ULONG InUsed,
    IN ULONG OutUsed,
    IN BOOLEAN Eject)
{
    IKsQueue * Queue;
    KSPSTREAM_POINTER * StreamPtr;
    KSPSTREAM_POINTER * Result;
    BOOLEAN NeedsEject;
    BOOLEAN StreamPointerInAdjusted = FALSE;
    BOOLEAN StreamPointerOutAdjusted = FALSE;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->State != KSPSTREAM_POINTER_STATE_UNLOCKED);
    ASSERT(StreamPtr->CKsQueue);

    if (StreamPtr->State != KSPSTREAM_POINTER_STATE_LOCKED)
    {
        // invalid state
        return STATUS_DEVICE_NOT_READY;
    }

    if (StreamPointer->OffsetIn.Data)
    {
        if (StreamPointer->OffsetIn.Count == 0 || InUsed)
        {
            ASSERT(InUsed <= StreamPointer->OffsetIn.Remaining);

            /* adjust stream pointer */
            StreamPointer->OffsetIn.Data += InUsed;

            NeedsEject = StreamPointer->OffsetIn.Remaining == InUsed;

            StreamPointer->OffsetIn.Remaining -= InUsed;

            if (NeedsEject)
            {
                Eject = TRUE;
            }
            StreamPointerInAdjusted = TRUE;
        }
    }

    if (StreamPointer->OffsetOut.Data)
    {
        if (StreamPointer->OffsetOut.Count == 0 || OutUsed)
        {
            ASSERT(OutUsed <= StreamPointer->OffsetOut.Remaining);

            /* adjust stream pointer */
            StreamPointer->OffsetOut.Data += OutUsed;

            NeedsEject = StreamPointer->OffsetOut.Remaining == OutUsed;

            StreamPointer->OffsetOut.Remaining -= OutUsed;

            if (NeedsEject)
            {
                Eject = TRUE;
            }
            StreamPointerOutAdjusted = TRUE;
        }
    }

    Queue = StreamPtr->CKsQueue;
    Queue->lpVtbl->UpdateByteAvailability(Queue,
        StreamPtr,
        StreamPointerInAdjusted ? InUsed : 0,
        StreamPointerOutAdjusted ? OutUsed : 0);

    if (!Eject)
    {
        return STATUS_SUCCESS;
    }

    Queue->lpVtbl->UnlockStreamPointer(Queue, StreamPtr, KSPSTREAM_POINTER_MOTION_ADVANCE);

    Result = Queue->lpVtbl->LockStreamPointer(Queue, StreamPtr);
    if (Result)
    {
        return STATUS_SUCCESS;
    }
    else
    {
        return STATUS_DEVICE_NOT_READY;
    }
}

/*
	@implemented
*/
KSDDKAPI
NTSTATUS
NTAPI
KsStreamPointerAdvance(
    IN PKSSTREAM_POINTER StreamPointer)
{
    IKsQueue * Queue;
    KSPSTREAM_POINTER * StreamPtr;
    KSPSTREAM_POINTER * Result;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->State != KSPSTREAM_POINTER_STATE_UNLOCKED);
    ASSERT(StreamPtr->CKsQueue);

    if (StreamPtr->State == KSPSTREAM_POINTER_STATE_LOCKED)
    {
        Queue = StreamPtr->CKsQueue;
        Queue->lpVtbl->UnlockStreamPointer(Queue, StreamPtr, KSPSTREAM_POINTER_MOTION_ADVANCE);
        Result = Queue->lpVtbl->LockStreamPointer(Queue, StreamPtr);
        if (Result != NULL)
        {
            return STATUS_SUCCESS;
        }
        // failed to lock stream pointer
        return STATUS_DEVICE_NOT_READY;

    }
    else if (StreamPtr->State)
    {
        return STATUS_DEVICE_NOT_READY;
    }
    else
    {
        Queue = StreamPtr->CKsQueue;
        Queue->lpVtbl->AdvanceUnlockedStreamPointer(Queue, StreamPtr);
        return STATUS_SUCCESS;
    }
}

/*
	@implemented
*/
KSDDKAPI
PMDL
NTAPI
KsStreamPointerGetMdl(
	IN PKSSTREAM_POINTER StreamPointer)
{
    KSPSTREAM_POINTER * StreamPtr;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->FrameHeader);

    if (StreamPtr->State != KSPSTREAM_POINTER_STATE_LOCKED)
    {
        // stream pointer is not locked
        return NULL;
    }

    return StreamPtr->FrameHeader->Mdl;
}

/*
	@unimplemented
*/
KSDDKAPI
PIRP
NTAPI
KsStreamPointerGetIrp(
    IN PKSSTREAM_POINTER StreamPointer,
    OUT PBOOLEAN FirstFrameInIrp OPTIONAL,
    OUT PBOOLEAN LastFrameInIrp OPTIONAL)
{
    KSPSTREAM_POINTER * StreamPtr;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->FrameHeader);

    if (StreamPtr->State != KSPSTREAM_POINTER_STATE_LOCKED)
    {
        // stream pointer is not locked
        return NULL;
    }

    UNIMPLEMENTED;
    return StreamPtr->FrameHeader->Irp;
}

/*
	@implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerScheduleTimeout(
    IN PKSSTREAM_POINTER StreamPointer,
    IN PFNKSSTREAMPOINTER Callback,
    IN ULONGLONG Interval)
{
    KSPSTREAM_POINTER * StreamPtr;
    IKsQueue * Queue;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->CKsQueue);
    ASSERT(Callback);

    Queue = StreamPtr->CKsQueue;
    Queue->lpVtbl->ScheduleTimeout(Queue, StreamPtr, Callback, Interval);
}

/*
	@implemented
*/
KSDDKAPI
VOID
NTAPI
KsStreamPointerCancelTimeout(
	IN PKSSTREAM_POINTER StreamPointer)
{
    KSPSTREAM_POINTER * StreamPtr;
    IKsQueue * Queue;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->CKsQueue);
    Queue = StreamPtr->CKsQueue;

    Queue->lpVtbl->CancelTimeout(Queue, StreamPtr);
}

/*
	@implemented
*/
KSDDKAPI
PKSSTREAM_POINTER
NTAPI
KsStreamPointerGetNextClone(
	IN PKSSTREAM_POINTER StreamPointer)
{
    KSPSTREAM_POINTER * StreamPtr;
    IKsQueue * Queue;

    StreamPtr = CONTAINING_RECORD(StreamPointer, KSPSTREAM_POINTER, StreamPointer);

    /* sanity checks */
    ASSERT(StreamPointer);
    ASSERT(StreamPtr);
    ASSERT(StreamPtr->CKsQueue);
    Queue = StreamPtr->CKsQueue;

    StreamPtr = Queue->lpVtbl->GetNextClone(Queue, StreamPtr);
    if (StreamPtr != NULL)
    {
        return &StreamPtr->StreamPointer;
    }
    return NULL;
}