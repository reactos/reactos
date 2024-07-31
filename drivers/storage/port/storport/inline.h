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
PQUEUED_REQUEST_REFERENCE
FORCEINLINE
StorpSrbAllocateRequestReference(
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ PIRP Irp,
    _In_ PPDO_DEVICE_EXTENSION PdoExtension)
{
    PQUEUED_REQUEST_REFERENCE RequestReference;

    RequestReference = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(QUEUED_REQUEST_REFERENCE),
                                             TAG_QUEUED_REQUEST);

    /* Fail request if allocation fails */
    if (RequestReference == NULL)
    {
        // Srb->SrbStatus = 5; // BUSY?
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        Irp->IoStatus.Information = 0;

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return NULL;
    }

    /* Does initialization */
    RtlZeroMemory(RequestReference, sizeof(QUEUED_REQUEST_REFERENCE));

    Srb->OriginalRequest = (PVOID)RequestReference;
    RequestReference->Irp = Irp;
    RequestReference->Srb = Srb;
    RequestReference->TimeoutCounter = Srb->TimeOutValue;

    RequestReference->PdoExtension = PdoExtension;

    return RequestReference;
}

/**
 * @brief Completes a request with specified SRB status and IRP status.
 * 
 */
VOID
FORCEINLINE
StorpCompleteRequest(
    _In_ PIRP Irp,
    _In_ UCHAR SrbStatus,
    _In_ NTSTATUS IrpStatus)
{
    Irp->Tail.Overlay.CurrentStackLocation->Parameters.Scsi.Srb->SrbStatus = SrbStatus;
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
PFDO_DEVICE_EXTENSION
FORCEINLINE
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

/**
 * @brief Assigns a queue tag to a request if it stated it supports one.
 * 
 * @param PdoExtension PDO Device extension
 * @param Srb SRB that represents this request
 * @return BOOLEAN Whether the request was assigned a tag at last.
 */
BOOLEAN
FORCEINLINE
StorpAssignTagToSrb(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PSCSI_REQUEST_BLOCK Srb
)
{
    ULONG NewTag;
    
    /* Bail out if the request is untagged */
    if (!TEST_FLAG(Srb->SrbFlags, SRB_FLAGS_QUEUE_ACTION_ENABLE))
    {
        return FALSE;
    }

    /* FIXME: What if the counter overflows? */
    NewTag = InterlockedIncrement(&PdoExtension->TagCounter);

    /*
     * We use ULONG for tag counter, and here seems to do a stupid truncation by casting to UCHAR;
     * But since Storport has a default maximum 254 requests/LUN limitation, this is actually OK for
     * our use case. If a miniport needs to support more than 254 tags it must use
     * STORAGE_REQUEST_BLOCK which we don't support yet :D
     */
    Srb->QueueTag = (UCHAR)(NewTag % PdoExtension->OutstandingRequestMax);

    return TRUE;
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
ULONG
FORCEINLINE
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
ULONG
FORCEINLINE
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
BOOLEAN
FORCEINLINE
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
PLIST_ENTRY
FORCEINLINE
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
PLIST_ENTRY
FORCEINLINE
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



// see https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-scsi-devices
// and https://docs.microsoft.com/en-us/windows-hardware/drivers/install/identifiers-for-ide-devices

FORCEINLINE
PCSTR
GetDeviceType(
    _In_ PINQUIRYDATA InquiryData)
{
    switch (InquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return "Disk";
        case SEQUENTIAL_ACCESS_DEVICE:
            return "Sequential";
        case PRINTER_DEVICE:
            return "Printer";
        case PROCESSOR_DEVICE:
            return "Processor";
        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "Worm";
        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "CdRom";
        case SCANNER_DEVICE:
            return "Scanner";
        case OPTICAL_DEVICE:
            return "Optical";
        case MEDIUM_CHANGER:
            return "Changer";
        case COMMUNICATION_DEVICE:
            return "Net";
        case ARRAY_CONTROLLER_DEVICE:
            return "Array";
        case SCSI_ENCLOSURE_DEVICE:
            return "Enclosure";
        case REDUCED_BLOCK_DEVICE:
            return "RBC";
        case OPTICAL_CARD_READER_WRITER_DEVICE:
            return "CardReader";
        case BRIDGE_CONTROLLER_DEVICE:
            return "Bridge";
        default:
            return "Other";
    }
}


FORCEINLINE
PCSTR
GetGenericType(
    _In_ PINQUIRYDATA InquiryData)
{
    switch (InquiryData->DeviceType)
    {
        case DIRECT_ACCESS_DEVICE:
            return "GenDisk";
        case PRINTER_DEVICE:
            return "GenPrinter";
        case WRITE_ONCE_READ_MULTIPLE_DEVICE:
            return "GenWorm";
        case READ_ONLY_DIRECT_ACCESS_DEVICE:
            return "GenCdRom";
        case SCANNER_DEVICE:
            return "GenScanner";
        case OPTICAL_DEVICE:
            return "GenOptical";
        case MEDIUM_CHANGER:
            return "ScsiChanger";
        case COMMUNICATION_DEVICE:
            return "ScsiNet";
        case ARRAY_CONTROLLER_DEVICE:
            return "ScsiArray";
        case SCSI_ENCLOSURE_DEVICE:
            return "ScsiEnclosure";
        case REDUCED_BLOCK_DEVICE:
            return "ScsiRBC";
        case OPTICAL_CARD_READER_WRITER_DEVICE:
            return "ScsiCardReader";
        case BRIDGE_CONTROLLER_DEVICE:
            return "ScsiBridge";
        default:
            return "ScsiOther";
    }
}
