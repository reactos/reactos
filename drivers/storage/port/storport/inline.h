/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport driver - inlined helper utilities
 * COPYRIGHT:   Copyright 2024 Wu Haotian (rigoligo03@gmail.com)
 */

#pragma once

#include "precomp.h"
#include "storport.h"

/**
 * @brief Allocates QUEUED_REQUEST_REFERENCE for an SRB that is going to be processed. If the
 * allocation fails, the function will fail the request inside it, and return NULL. Otherwise
 * initializes request reference, sets SRB => ReqRef link, and returns the structure allocated.
 * 
 * @param Srb 
 * @param Irp 
 * @return PQUEUED_REQUEST_REFERENCE RequestReference structure allocated, or NULL on failure. 
 */
FORCEINLINE
PQUEUED_REQUEST_REFERENCE
StorpSrbAllocateRequestReference(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PIRP Irp,
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    PQUEUED_REQUEST_REFERENCE RequestReference;

    RequestReference =
        ExAllocateFromNPagedLookasideList(&PdoExtension->FdoExtension->RequestReferenceLookaside);

    /* Fail request if allocation fails */
    if (RequestReference == NULL)
    {
        Srb->SrbStatus = SRB_STATUS_INTERNAL_ERROR;
        Srb->InternalStatus = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return NULL;
    }

    /* Does initialization. Lookaside entries are recycled, so they need zeroing */
    RtlZeroMemory(RequestReference, sizeof(*RequestReference));

    Srb->OriginalRequest = (PVOID)RequestReference;
    RequestReference->Irp = Irp;
    RequestReference->Srb = Srb;
    RequestReference->TimeoutCounter = Srb->TimeOutValue;

    RequestReference->PdoExtension = PdoExtension;

    return RequestReference;
}

/**
 * @brief Frees an SRB's Request Reference and restore Srb->OriginalRequest field to IRP.
 * This function does not check if Srb->OriginalRequest is actually an request reference, and you
 * must determine it by other means.
 * 
 * @param Srb 
 * @return FORCEINLINE 
 */
FORCEINLINE
VOID
StorpSrbFreeRequestReference(
    _In_ PSCSI_REQUEST_BLOCK Srb)
{
    PQUEUED_REQUEST_REFERENCE RequestReference;
    PNPAGED_LOOKASIDE_LIST Lookaside;

    RequestReference = (PQUEUED_REQUEST_REFERENCE)Srb->OriginalRequest;
    Srb->OriginalRequest = RequestReference->Irp;
    Lookaside = &RequestReference->PdoExtension->FdoExtension->RequestReferenceLookaside;

    ExFreeToNPagedLookasideList(Lookaside, RequestReference);
}


/**
 * @brief Completes a request with specified SRB status and IRP status.
 * 
 */
FORCEINLINE
VOID
StorpCompleteRequest(
    _In_ PIRP Irp,
    _In_ UCHAR SrbStatus,
    _In_ NTSTATUS IrpStatus)
{
    IoGetCurrentIrpStackLocation(Irp)->Parameters.Scsi.Srb->SrbStatus = SrbStatus;
    Irp->IoStatus.Status = IrpStatus;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/**
 * @brief For use in Storport API exposed to miniports; gets FDO extension with HwDeviceExtension.
 * 
 * @param HwDeviceExtension HwDeviceExtension parameter of Storport API.
 * @return PFDO_DEVICE_EXTENSION FDO Device extension.
 */
FORCEINLINE
PFDO_DEVICE_EXTENSION
StorpGetMiniportFdo(
    _In_ PVOID HwDeviceExtension
)
{
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;

    NT_ASSERT(HwDeviceExtension);

    MiniportExtension = CONTAINING_RECORD(HwDeviceExtension,
                                          MINIPORT_DEVICE_EXTENSION,
                                          HwDeviceExtension);

    return MiniportExtension->Miniport->DeviceExtension;
}

/*
  Convenient counted linked list manipulation. In ReactOS Storport there are linked lists whose
  item count is kept alongside the list. This function provides a cleaner implementation to
  reduce complexity in functional code.
*/

/**
 * @brief InsertHeadList and increment element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param ListHead List head of the list.
 * @param Entry List entry to be inserted.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return ULONG Number of items in the list after insertion.
 */
FORCEINLINE
ULONG
StorpInterlockedInsertHeadListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY Entry,
    _In_ PULONG Counter
)
{
    KLOCK_QUEUE_HANDLE LockHandle;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);
    InsertHeadList(ListHead, Entry);
    ++(*Counter);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return (*Counter);
}

/**
 * @brief InsertTailList and increment element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param ListHead List head of the list.
 * @param Entry List entry to be inserted.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return ULONG Number of items in the list after insertion.
 */
FORCEINLINE
ULONG
StorpInterlockedInsertTailListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PLIST_ENTRY Entry,
    _In_ PULONG Counter
)
{
    KLOCK_QUEUE_HANDLE LockHandle;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);
    InsertTailList(ListHead, Entry);
    ++(*Counter);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return (*Counter);
}

/**
 * @brief RemoveEntryList and decrement element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param Entry List entry to be removed.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return BOOLEAN Whether the list is empty after the removal.
 */
FORCEINLINE
BOOLEAN
StorpInterlockedRemoveEntryListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY Entry,
    _In_ PULONG Counter)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    BOOLEAN Empty;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);
    Empty = RemoveEntryList(Entry);
    --(*Counter);

    NT_ASSERT(Empty == (*Counter == 0));

    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return Empty;
}

/**
 * @brief RemoveHeadList and decrement element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param ListHead List head to take an entry from.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return PLIST_ENTRY The element taken from the list.
 */
FORCEINLINE
PLIST_ENTRY
StorpInterlockedRemoveHeadListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PULONG Counter)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY Entry;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);

    NT_ASSERT(IsListEmpty(ListHead) == (*Counter == 0));

    Entry = RemoveHeadList(ListHead);
    --(*Counter);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return Entry;
}

/**
 * @brief RemoveTailList and decrement element counter with a corresponding spin lock held.
 * 
 * @param SpinLock Spin lock for interlocked exclusive access.
 * @param ListHead List head to take an entry from.
 * @param Counter Pointer to the list element counter ULONG value.
 * @return PLIST_ENTRY The element taken from the list.
 */
FORCEINLINE
PLIST_ENTRY
StorpInterlockedRemoveTailListCounted(
    _In_ PKSPIN_LOCK SpinLock,
    _In_ PLIST_ENTRY ListHead,
    _In_ PULONG Counter)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY Entry;

    KeAcquireInStackQueuedSpinLock(SpinLock, &LockHandle);

    NT_ASSERT(IsListEmpty(ListHead) == (*Counter == 0));

    Entry = RemoveTailList(ListHead);
    --(*Counter);
    KeReleaseInStackQueuedSpinLock(&LockHandle);

    return Entry;
}



#ifdef DBG
/**
 * @brief Check data consistency in flow control data structures.
 * Will acquire PDO list lock. Must be executed with FDO flow control lock held.
 * 
 * @param FdoExtension FDO device extension.
 */
FORCEINLINE
VOID
StorpCheckFlowControl(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    KLOCK_QUEUE_HANDLE LockHandle;
    PPDO_DEVICE_EXTENSION PdoExtension;
    PFDO_IO_FLOW_CONTROL FdoFlowControl;
    PPDO_IO_FLOW_CONTROL PdoFlowControl;
    PLIST_ENTRY PdoEntry;
    ULONG PdoOutstandingSum = 0;
    ULONG PdoStrongOrderedSum = 0;

    FdoFlowControl = &FdoExtension->FlowControl;

    KeAcquireInStackQueuedSpinLock(&FdoExtension->PdoListLock, &LockHandle);
    PdoEntry = FdoExtension->PdoListHead.Flink;

    while (PdoEntry != &FdoExtension->PdoListHead)
    {
        /* Obtain PDO extension and flow control */
        PdoExtension = CONTAINING_RECORD(PdoEntry, PDO_DEVICE_EXTENSION, PdoListEntry);
        PdoFlowControl = &PdoExtension->FlowControl;

        /* Accumulate important values and compare to FDO record */
        PdoOutstandingSum += PdoFlowControl->OutstandingRequestCount;
        PdoStrongOrderedSum += PdoFlowControl->StrongOrderedCount;

        /* Next element */
        PdoEntry = PdoEntry->Flink;
    }

    UNREFERENCED_PARAMETER(PdoStrongOrderedSum); /* Don't know what to do with it */

    /* Compare */
    NT_ASSERT(PdoOutstandingSum == FdoFlowControl->OutstandingRequestCount);

    KeReleaseInStackQueuedSpinLock(&LockHandle);
}
#else
#define StorpCheckFlowControl(...)
#endif


// FORCEINLINE
// VOID
// StorpDumpRequest(
//     _In_ PQUEUED_REQUEST_REFERENCE RequestReference)
// {
//     PPDO_DEVICE_EXTENSION PdoExtension = RequestReference->PdoExtension;
//     CHAR HexBuf[50] = {0};

//     RequestReference->SpecialRequestId = InterlockedIncrement(&PdoExtension->SpecialRequestCounter);

//     DPRINT1("\n\n");
//     DPRINT1("Storport SPECIAL REQUEST %d Before Issued\n", RequestReference->SpecialRequestId);
//     DPRINT1("Bus:Target:Lun (%d:%d:%d) CDB %d\n",
//             PdoExtension->Bus,
//             PdoExtension->Target,
//             PdoExtension->Lun,
//             RequestReference->Srb->CdbLength);
//     for (int i = 0; i < RequestReference->Srb->CdbLength; i++)
//     {
//         CHAR Tmp[3];
//         sprintf(Tmp, "%02X", RequestReference->Srb->Cdb[i]);
//         strcat(HexBuf, Tmp);
//         if (i == 7 && RequestReference->Srb->CdbLength != 7)
//             strcat(HexBuf, "-");
//         else
//             strcat(HexBuf, " ");
//     }
//     DPRINT1("CDB: %s\n\n", HexBuf);
// }


// FORCEINLINE
// VOID
// StorpDumpRequestResponse(
//     _In_ PQUEUED_REQUEST_REFERENCE RequestReference)
// {
//     PPDO_DEVICE_EXTENSION PdoExtension = RequestReference->PdoExtension;
//     CHAR HexBuf[50] = {0};
//     ULONG BytesLeft = RequestReference->Srb->DataTransferLength;
//     ULONG Offset = 0;
    
//     DPRINT1("\n\n");
//     DPRINT1("Storport SPECIAL REQUEST %d Completion\n\n", RequestReference->SpecialRequestId);
//     DPRINT1("Bus:Target:Lun (%d:%d:%d) CDB %d (%02X) ReturnLength %d\n",
//             PdoExtension->Bus,
//             PdoExtension->Target,
//             PdoExtension->Lun,
//             RequestReference->Srb->CdbLength,
//             RequestReference->Srb->Cdb[0],
//             BytesLeft);

//     DPRINT1("Returned Data:\n");
//     while (BytesLeft)
//     {
//         /* Print in 16 byte groups */
//         ULONG BytesToPrint = (BytesLeft > 16) ? 16 : BytesLeft;
//         HexBuf[0] = '\0';
//         for (int i = 0; i < BytesToPrint; i++)
//         {
//             CHAR Tmp[3];
//             sprintf(Tmp, "%02X", ((PUCHAR)RequestReference->Srb->DataBuffer)[Offset + i]);
//             strcat(HexBuf, Tmp);
//             if (i == 7 && RequestReference->Srb->CdbLength != 7)
//                 strcat(HexBuf, "-");
//             else
//                 strcat(HexBuf, " ");
//         }
//         DPRINT1("%s\n", HexBuf);
//         BytesLeft -= BytesToPrint;
//         Offset += BytesToPrint;
//     }
//     DPRINT1("\n\n");
// }

