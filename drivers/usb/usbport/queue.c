/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort queue implementation
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBPORT_CORE
#define NDEBUG_USBPORT_QUEUE
#define NDEBUG_USBPORT_URB
#include "usbdebug.h"

VOID
NTAPI
USBPORT_InsertIdleIrp(IN PIO_CSQ Csq,
                      IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_InsertIdleIrp: Irp - %p\n", Irp);

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     IdleIoCsq);

    InsertTailList(&FdoExtension->IdleIrpList,
                   &Irp->Tail.Overlay.ListEntry);
}

VOID
NTAPI
USBPORT_RemoveIdleIrp(IN PIO_CSQ Csq,
                      IN PIRP Irp)
{
    DPRINT_QUEUE("USBPORT_RemoveIdleIrp: Irp - %p\n", Irp);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

PIRP
NTAPI
USBPORT_PeekNextIdleIrp(IN PIO_CSQ Csq,
                        IN PIRP Irp,
                        IN PVOID PeekContext)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY ListHead;
    PIRP NextIrp = NULL;

    DPRINT_QUEUE("USBPORT_PeekNextIdleIrp: Irp - %p, PeekContext - %p\n",
                 Irp,
                 PeekContext);

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     IdleIoCsq);

    ListHead = &FdoExtension->IdleIrpList;

    if (Irp)
    {
        NextEntry = Irp->Tail.Overlay.ListEntry.Flink;
    }
    else
    {
        NextEntry = ListHead->Flink;
    }

    while (NextEntry != ListHead)
    {
        NextIrp = CONTAINING_RECORD(NextEntry,
                                    IRP,
                                    Tail.Overlay.ListEntry);

        if (!PeekContext)
            break;

        NextEntry = NextEntry->Flink;
    }

    return NextIrp;
}

VOID
NTAPI
USBPORT_AcquireIdleLock(IN PIO_CSQ Csq,
                        IN PKIRQL Irql)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_AcquireIdleLock: ... \n");

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     IdleIoCsq);

    KeAcquireSpinLock(&FdoExtension->IdleIoCsqSpinLock, Irql);
}

VOID
NTAPI
USBPORT_ReleaseIdleLock(IN PIO_CSQ Csq,
                        IN KIRQL Irql)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_ReleaseIdleLock: ... \n");

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     IdleIoCsq);

    KeReleaseSpinLock(&FdoExtension->IdleIoCsqSpinLock, Irql);
}

VOID
NTAPI
USBPORT_CompleteCanceledIdleIrp(IN PIO_CSQ Csq,
                                IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_CompleteCanceledIdleIrp: ... \n");

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     IdleIoCsq);

    InterlockedDecrement(&FdoExtension->IdleLockCounter);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
NTAPI
USBPORT_InsertBadRequest(IN PIO_CSQ Csq,
                         IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_InsertBadRequest: Irp - %p\n", Irp);

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     BadRequestIoCsq);

    InsertTailList(&FdoExtension->BadRequestList,
                   &Irp->Tail.Overlay.ListEntry);
}

VOID
NTAPI
USBPORT_RemoveBadRequest(IN PIO_CSQ Csq,
                         IN PIRP Irp)
{
    DPRINT_QUEUE("USBPORT_RemoveBadRequest: Irp - %p\n", Irp);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

PIRP
NTAPI
USBPORT_PeekNextBadRequest(IN PIO_CSQ Csq,
                           IN PIRP Irp,
                           IN PVOID PeekContext)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PLIST_ENTRY NextEntry;
    PLIST_ENTRY ListHead;
    PIRP NextIrp = NULL;

    DPRINT_QUEUE("USBPORT_PeekNextBadRequest: Irp - %p, PeekContext - %p\n",
                 Irp,
                 PeekContext);

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     BadRequestIoCsq);

    ListHead = &FdoExtension->BadRequestList;

    if (Irp)
    {
        NextEntry = Irp->Tail.Overlay.ListEntry.Flink;
    }
    else
    {
        NextEntry = ListHead->Flink;
    }

    while (NextEntry != ListHead)
    {
        NextIrp = CONTAINING_RECORD(NextEntry,
                                    IRP,
                                    Tail.Overlay.ListEntry);

        if (!PeekContext)
            break;

        NextEntry = NextEntry->Flink;
    }

    return NextIrp;
}

VOID
NTAPI
USBPORT_AcquireBadRequestLock(IN PIO_CSQ Csq,
                              IN PKIRQL Irql)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_AcquireBadRequestLock: ... \n");

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     BadRequestIoCsq);

    KeAcquireSpinLock(&FdoExtension->BadRequestIoCsqSpinLock, Irql);
}

VOID
NTAPI
USBPORT_ReleaseBadRequestLock(IN PIO_CSQ Csq,
                              IN KIRQL Irql)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_ReleaseBadRequestLock: ... \n");

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     BadRequestIoCsq);

    KeReleaseSpinLock(&FdoExtension->BadRequestIoCsqSpinLock, Irql);
}

VOID
NTAPI
USBPORT_CompleteCanceledBadRequest(IN PIO_CSQ Csq,
                                   IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_QUEUE("USBPORT_CompleteCanceledBadRequest: Irp - %p\n", Irp);

    FdoExtension = CONTAINING_RECORD(Csq,
                                     USBPORT_DEVICE_EXTENSION,
                                     BadRequestIoCsq);

    InterlockedDecrement(&FdoExtension->BadRequestLockCounter);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
NTAPI
USBPORT_InsertIrpInTable(IN PUSBPORT_IRP_TABLE IrpTable,
                         IN PIRP Irp)
{
    ULONG ix;

    DPRINT_CORE("USBPORT_InsertIrpInTable: IrpTable - %p, Irp - %p\n",
                IrpTable,
                Irp);

    ASSERT(IrpTable != NULL);

    while (TRUE)
    {
        for (ix = 0; ix < 0x200; ix++)
        {
            if (IrpTable->irp[ix] == NULL)
            {
                IrpTable->irp[ix] = Irp;

                if (ix > 0)
                {
                    DPRINT_CORE("USBPORT_InsertIrpInTable: ix - %x\n", ix);
                }

                return;
            }
        }

        if (ix != 0x200)
        {
            KeBugCheckEx(BUGCODE_USB_DRIVER, 1, 0, 0, 0);
        }

        IrpTable->LinkNextTable = ExAllocatePoolWithTag(NonPagedPool,
                                                        sizeof(USBPORT_IRP_TABLE),
                                                        USB_PORT_TAG);

        if (IrpTable->LinkNextTable == NULL)
        {
            KeBugCheckEx(BUGCODE_USB_DRIVER, 1, 0, 0, 0);
        }

        RtlZeroMemory(IrpTable->LinkNextTable, sizeof(USBPORT_IRP_TABLE));

        IrpTable = IrpTable->LinkNextTable;
    }
}

PIRP
NTAPI
USBPORT_RemoveIrpFromTable(IN PUSBPORT_IRP_TABLE IrpTable,
                           IN PIRP Irp)
{
    ULONG ix;

    DPRINT_CORE("USBPORT_RemoveIrpFromTable: IrpTable - %p, Irp - %p\n",
                IrpTable,
                Irp);

    ASSERT(IrpTable != NULL);

    while (TRUE)
    {
        for (ix = 0; ix < 0x200; ix++)
        {
            if (IrpTable->irp[ix] == Irp)
            {
                IrpTable->irp[ix] = NULL;

                if (ix > 0)
                {
                    DPRINT_CORE("USBPORT_RemoveIrpFromTable: ix - %x\n", ix);
                }

                return Irp;
            }
        }

        if (IrpTable->LinkNextTable == NULL)
            break;

        IrpTable = IrpTable->LinkNextTable;
        continue;
    }

    DPRINT1("USBPORT_RemoveIrpFromTable: return NULL. ix - %x\n", ix);
    return NULL;
}

PIRP
NTAPI
USBPORT_RemoveActiveTransferIrp(IN PDEVICE_OBJECT FdoDevice,
                                IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_CORE("USBPORT_RemoveActiveTransferIrp: Irp - %p\n", Irp);
    FdoExtension = FdoDevice->DeviceExtension;
    return USBPORT_RemoveIrpFromTable(FdoExtension->ActiveIrpTable, Irp);
}

PIRP
NTAPI
USBPORT_RemovePendingTransferIrp(IN PDEVICE_OBJECT FdoDevice,
                                 IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_CORE("USBPORT_RemovePendingTransferIrp: Irp - %p\n", Irp);
    FdoExtension = FdoDevice->DeviceExtension;
    return USBPORT_RemoveIrpFromTable(FdoExtension->PendingIrpTable, Irp);
}

VOID
NTAPI
USBPORT_FindUrbInIrpTable(IN PUSBPORT_IRP_TABLE IrpTable,
                          IN PURB Urb,
                          IN PIRP Irp)
{
    ULONG ix;
    PIRP irp;
    PURB urbIn;

    DPRINT_CORE("USBPORT_FindUrbInIrpTable: IrpTable - %p, Urb - %p, Irp - %p\n",
                IrpTable,
                Urb,
                Irp);

    ASSERT(IrpTable != NULL);

    do
    {
        for (ix = 0; ix < 0x200; ix++)
        {
            irp = IrpTable->irp[ix];

            if (irp)
            {
                urbIn = URB_FROM_IRP(irp);

                if (urbIn == Urb)
                {
                    if (irp == Irp)
                    {
                        KeBugCheckEx(BUGCODE_USB_DRIVER,
                                     4,
                                     (ULONG_PTR)irp,
                                     (ULONG_PTR)urbIn,
                                     0);
                    }

                    KeBugCheckEx(BUGCODE_USB_DRIVER,
                                 2,
                                 (ULONG_PTR)irp,
                                 (ULONG_PTR)Irp,
                                 (ULONG_PTR)urbIn);
                }
            }
        }

        IrpTable = IrpTable->LinkNextTable;
    }
    while (IrpTable);
}

PIRP
NTAPI
USBPORT_FindIrpInTable(IN PUSBPORT_IRP_TABLE IrpTable,
                       IN PIRP Irp)
{
    ULONG ix;
    PIRP irp;

    DPRINT_CORE("USBPORT_FindIrpInTable: IrpTable - %p, Irp - %p\n",
                IrpTable,
                Irp);

    ASSERT(IrpTable != NULL);

    do
    {
        for (ix = 0; ix < 0x200; ix++)
        {
            irp = IrpTable->irp[ix];

            if (irp && irp == Irp)
            {
                return irp;
            }
        }

        IrpTable = IrpTable->LinkNextTable;
    }
    while (IrpTable->LinkNextTable);

    DPRINT_CORE("USBPORT_FindIrpInTable: Not found!!!\n");
    return NULL;
}

PIRP
NTAPI
USBPORT_FindActiveTransferIrp(IN PDEVICE_OBJECT FdoDevice,
                              IN PIRP Irp)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_CORE("USBPORT_FindActiveTransferIrp: Irp - %p\n", Irp);
    FdoExtension = FdoDevice->DeviceExtension;
    return USBPORT_FindIrpInTable(FdoExtension->ActiveIrpTable, Irp);
}

VOID
NTAPI
USBPORT_CancelPendingTransferIrp(IN PDEVICE_OBJECT DeviceObject,
                                 IN PIRP Irp)
{
    PURB Urb;
    PUSBPORT_TRANSFER Transfer;
    PUSBPORT_ENDPOINT Endpoint;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    KIRQL OldIrql;
    PIRP irp;

    DPRINT_CORE("USBPORT_CancelPendingTransferIrp: DeviceObject - %p, Irp - %p\n",
                DeviceObject,
                Irp);

    Urb = URB_FROM_IRP(Irp);
    Transfer = Urb->UrbControlTransfer.hca.Reserved8[0];
    Endpoint = Transfer->Endpoint;

    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = DeviceObject->DeviceExtension;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&FdoExtension->FlushPendingTransferSpinLock, &OldIrql);

    irp = USBPORT_RemovePendingTransferIrp(FdoDevice, Irp);

    if (!irp)
    {
        KeReleaseSpinLock(&FdoExtension->FlushPendingTransferSpinLock,
                          OldIrql);
        return;
    }

    KeAcquireSpinLockAtDpcLevel(&Endpoint->EndpointSpinLock);

    RemoveEntryList(&Transfer->TransferLink);

    Transfer->TransferLink.Flink = NULL;
    Transfer->TransferLink.Blink = NULL;

    KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
    KeReleaseSpinLock(&FdoExtension->FlushPendingTransferSpinLock, OldIrql);

    USBPORT_CompleteTransfer(Transfer->Urb, USBD_STATUS_CANCELED);
}

VOID
NTAPI
USBPORT_CancelActiveTransferIrp(IN PDEVICE_OBJECT DeviceObject,
                                IN PIRP Irp)
{
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PURB Urb;
    PUSBPORT_TRANSFER Transfer;
    PUSBPORT_ENDPOINT Endpoint;
    PIRP irp;
    PUSBPORT_TRANSFER SplitTransfer;
    PLIST_ENTRY Entry;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_CancelActiveTransferIrp: Irp - %p\n", Irp);

    PdoExtension = DeviceObject->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    KeAcquireSpinLock(&FdoExtension->FlushTransferSpinLock, &OldIrql);

    irp = USBPORT_FindActiveTransferIrp(FdoDevice, Irp);

    if (!irp)
    {
        KeReleaseSpinLock(&FdoExtension->FlushTransferSpinLock, OldIrql);
        return;
    }

    Urb = URB_FROM_IRP(irp);
    Transfer = Urb->UrbControlTransfer.hca.Reserved8[0];
    Endpoint = Transfer->Endpoint;

    DPRINT_CORE("USBPORT_CancelActiveTransferIrp: irp - %p, Urb - %p, Transfer - %p\n",
                irp,
                Urb,
                Transfer);

    KeAcquireSpinLockAtDpcLevel(&Endpoint->EndpointSpinLock);

    Transfer->Flags |= TRANSFER_FLAG_CANCELED;

    if (Transfer->Flags & TRANSFER_FLAG_PARENT)
    {
        KeAcquireSpinLockAtDpcLevel(&Transfer->TransferSpinLock);

        Entry = Transfer->SplitTransfersList.Flink;

        while (Entry && Entry != &Transfer->SplitTransfersList)
        {
            SplitTransfer = CONTAINING_RECORD(Entry,
                                              USBPORT_TRANSFER,
                                              SplitLink);

            SplitTransfer->Flags |= TRANSFER_FLAG_CANCELED;

            Entry = Entry->Flink;
        }

        KeReleaseSpinLockFromDpcLevel(&Transfer->TransferSpinLock);
    }

    KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
    KeReleaseSpinLock(&FdoExtension->FlushTransferSpinLock, OldIrql);

    USBPORT_InvalidateEndpointHandler(FdoDevice,
                                      Endpoint,
                                      INVALIDATE_ENDPOINT_WORKER_THREAD);
    return;
}

VOID
NTAPI
USBPORT_FlushAbortList(IN PUSBPORT_ENDPOINT Endpoint)
{
    PLIST_ENTRY Entry;
    PUSBPORT_TRANSFER Transfer;
    PLIST_ENTRY AbortList;
    LIST_ENTRY List;
    NTSTATUS Status;
    PIRP Irp;
    PURB Urb;
    PUSBPORT_DEVICE_HANDLE DeviceHandle = NULL;

    DPRINT_CORE("USBPORT_FlushAbortList: Endpoint - %p\n", Endpoint);

    InitializeListHead(&List);

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

    if (IsListEmpty(&Endpoint->AbortList))
    {
        KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                          Endpoint->EndpointOldIrql);
        return;
    }

    Entry = Endpoint->PendingTransferList.Flink;

    while (Entry && Entry != &Endpoint->PendingTransferList)
    {
        Transfer = CONTAINING_RECORD(Entry,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        if (Transfer->Flags & TRANSFER_FLAG_ABORTED)
        {
            DPRINT_CORE("USBPORT_FlushAbortList: Aborted PendingTransfer  - %p\n",
                        Transfer);

            KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                              Endpoint->EndpointOldIrql);
            return;
        }

        Entry = Transfer->TransferLink.Flink;
    }

    Entry = Endpoint->TransferList.Flink;

    while (Entry && Entry != &Endpoint->TransferList)
    {
        Transfer = CONTAINING_RECORD(Entry,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        if (Transfer->Flags & TRANSFER_FLAG_ABORTED)
        {
            DPRINT_CORE("USBPORT_FlushAbortList: Aborted ActiveTransfer - %p\n",
                        Transfer);

            KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                              Endpoint->EndpointOldIrql);
            return;
        }

        Entry = Transfer->TransferLink.Flink;
    }

    AbortList = &Endpoint->AbortList;

    while (!IsListEmpty(AbortList))
    {
        //DbgBreakPoint();

        Irp = CONTAINING_RECORD(AbortList->Flink,
                                IRP,
                                Tail.Overlay.ListEntry);

        RemoveHeadList(AbortList);
        InsertTailList(&List, &Irp->Tail.Overlay.ListEntry);
    }

    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    while (!IsListEmpty(&List))
    {
        //DbgBreakPoint();

        Irp = CONTAINING_RECORD(List.Flink,
                                IRP,
                                Tail.Overlay.ListEntry);

        RemoveHeadList(&List);

        Urb = URB_FROM_IRP(Irp);

        DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;
        InterlockedDecrement(&DeviceHandle->DeviceHandleLock);

        Status = USBPORT_USBDStatusToNtStatus(Urb, USBD_STATUS_SUCCESS);

        DPRINT_CORE("USBPORT_FlushAbortList: complete Irp - %p\n", Irp);

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

VOID
NTAPI
USBPORT_FlushCancelList(IN PUSBPORT_ENDPOINT Endpoint)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PUSBPORT_TRANSFER Transfer;
    PIRP Irp;
    KIRQL OldIrql;
    KIRQL PrevIrql;

    DPRINT_CORE("USBPORT_FlushCancelList: ... \n");

    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->FlushTransferSpinLock, &OldIrql);
    KeAcquireSpinLockAtDpcLevel(&Endpoint->EndpointSpinLock);

    while (!IsListEmpty(&Endpoint->CancelList))
    {
        Transfer = CONTAINING_RECORD(Endpoint->CancelList.Flink,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        RemoveHeadList(&Endpoint->CancelList);

        Irp = Transfer->Irp;

        if (Irp)
        {
            DPRINT("USBPORT_FlushCancelList: Irp - %p\n", Irp);

            IoAcquireCancelSpinLock(&PrevIrql);
            IoSetCancelRoutine(Irp, NULL);
            IoReleaseCancelSpinLock(PrevIrql);

            USBPORT_RemoveActiveTransferIrp(FdoDevice, Irp);
        }

        KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
        KeReleaseSpinLock(&FdoExtension->FlushTransferSpinLock, OldIrql);

        if (Endpoint->Flags & ENDPOINT_FLAG_NUKE)
        {
            USBPORT_CompleteTransfer(Transfer->Urb, USBD_STATUS_DEVICE_GONE);
        }
        else
        {
            if (Transfer->Flags & TRANSFER_FLAG_DEVICE_GONE)
            {
                USBPORT_CompleteTransfer(Transfer->Urb,
                                         USBD_STATUS_DEVICE_GONE);
            }
            else
            {
                USBPORT_CompleteTransfer(Transfer->Urb,
                                         USBD_STATUS_CANCELED);
            }
        }

        KeAcquireSpinLock(&FdoExtension->FlushTransferSpinLock, &OldIrql);
        KeAcquireSpinLockAtDpcLevel(&Endpoint->EndpointSpinLock);
    }

    KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
    KeReleaseSpinLock(&FdoExtension->FlushTransferSpinLock, OldIrql);

    USBPORT_FlushAbortList(Endpoint);
}

VOID
NTAPI
USBPORT_FlushPendingTransfers(IN PUSBPORT_ENDPOINT Endpoint)
{
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    BOOLEAN IsMapTransfer;
    BOOLEAN IsEnd = FALSE;
    PLIST_ENTRY List;
    PUSBPORT_TRANSFER Transfer;
    PURB Urb;
    PIRP Irp;
    KIRQL OldIrql;
    BOOLEAN Result;

    DPRINT_CORE("USBPORT_FlushPendingTransfers: Endpoint - %p\n", Endpoint);

    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    if (InterlockedCompareExchange(&Endpoint->FlushPendingLock, 1, 0))
    {
        DPRINT_CORE("USBPORT_FlushPendingTransfers: Endpoint Locked \n");
        return;
    }

    while (TRUE)
    {
        IsMapTransfer = 0;

        KeAcquireSpinLock(&FdoExtension->FlushPendingTransferSpinLock,
                          &OldIrql);

        KeAcquireSpinLockAtDpcLevel(&Endpoint->EndpointSpinLock);

        if (FdoExtension->Flags & USBPORT_FLAG_HC_SUSPEND)
        {
            IsEnd = TRUE;

            KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
            KeReleaseSpinLock(&FdoExtension->FlushPendingTransferSpinLock,
                              OldIrql);
            goto Next;
        }

        if (!(Endpoint->Flags & ENDPOINT_FLAG_ROOTHUB_EP0))
        {
            if (!IsListEmpty(&Endpoint->TransferList))
            {
                List = Endpoint->TransferList.Flink;

                while (List && List != &Endpoint->TransferList)
                {
                    Transfer = CONTAINING_RECORD(List,
                                                 USBPORT_TRANSFER,
                                                 TransferLink);

                    if (!(Transfer->Flags & TRANSFER_FLAG_SUBMITED))
                    {
                        KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
                        KeReleaseSpinLock(&FdoExtension->FlushPendingTransferSpinLock,
                                          OldIrql);

                        IsEnd = TRUE;
                        goto Worker;
                    }

                    List = Transfer->TransferLink.Flink;
                }
            }
        }

        List = Endpoint->PendingTransferList.Flink;

        if (List == NULL || IsListEmpty(&Endpoint->PendingTransferList))
        {
            KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
            KeReleaseSpinLock(&FdoExtension->FlushPendingTransferSpinLock,
                              OldIrql);

            IsEnd = TRUE;
            goto Worker;
        }

        Transfer = CONTAINING_RECORD(List,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        if (Transfer->Irp)
        {
            DPRINT_CORE("USBPORT_FlushPendingTransfers: Transfer->Irp->CancelRoutine - %p\n",
                        Transfer->Irp->CancelRoutine);
        }

        if (Transfer->Irp &&
            (IoSetCancelRoutine(Transfer->Irp, NULL) == NULL))
        {
            DPRINT_CORE("USBPORT_FlushPendingTransfers: Transfer->Irp - %p\n",
                        Transfer->Irp);

            Transfer = NULL;
            IsEnd = TRUE;
        }

        if (!Transfer)
        {
            KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
            KeReleaseSpinLock(&FdoExtension->FlushPendingTransferSpinLock,
                              OldIrql);

            if (IsMapTransfer)
            {
                USBPORT_FlushMapTransfers(FdoDevice);
                goto Next;
            }

            goto Worker;
        }

        Irp = Transfer->Irp;
        Urb = Transfer->Urb;

        RemoveEntryList(&Transfer->TransferLink);
        Transfer->TransferLink.Flink = NULL;
        Transfer->TransferLink.Blink = NULL;

        if (Irp)
        {
            Irp = USBPORT_RemovePendingTransferIrp(FdoDevice, Irp);
        }

        KeReleaseSpinLockFromDpcLevel(&Endpoint->EndpointSpinLock);
        KeReleaseSpinLock(&FdoExtension->FlushPendingTransferSpinLock,
                          OldIrql);

        KeAcquireSpinLock(&FdoExtension->FlushTransferSpinLock, &OldIrql);

        if (Irp)
        {
            IoSetCancelRoutine(Irp, USBPORT_CancelActiveTransferIrp);

            if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
            {
                DPRINT_CORE("USBPORT_FlushPendingTransfers: irp - %p\n", Irp);

                KeReleaseSpinLock(&FdoExtension->FlushTransferSpinLock,
                                  OldIrql);

                USBPORT_CompleteTransfer(Transfer->Urb, USBD_STATUS_CANCELED);
                goto Worker;
            }

            USBPORT_FindUrbInIrpTable(FdoExtension->ActiveIrpTable, Urb, Irp);
            USBPORT_InsertIrpInTable(FdoExtension->ActiveIrpTable, Irp);
        }

        IsMapTransfer = USBPORT_QueueActiveUrbToEndpoint(Endpoint, Urb);

        KeReleaseSpinLock(&FdoExtension->FlushTransferSpinLock, OldIrql);

        if (IsMapTransfer)
        {
            USBPORT_FlushMapTransfers(FdoDevice);
            goto Next;
        }

Worker:
        KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
        Result = USBPORT_EndpointWorker(Endpoint, FALSE);
        KeLowerIrql(OldIrql);

        if (Result)
            USBPORT_InvalidateEndpointHandler(FdoDevice,
                                              Endpoint,
                                              INVALIDATE_ENDPOINT_WORKER_THREAD);

Next:
        if (IsEnd)
        {
            InterlockedDecrement(&Endpoint->FlushPendingLock);
            DPRINT_CORE("USBPORT_FlushPendingTransfers: Endpoint Unlocked. Exit\n");
            return;
        }
    }
}

VOID
NTAPI
USBPORT_QueuePendingUrbToEndpoint(IN PUSBPORT_ENDPOINT Endpoint,
                                  IN PURB Urb)
{
    PUSBPORT_TRANSFER Transfer;

    DPRINT_CORE("USBPORT_QueuePendingUrbToEndpoint: Endpoint - %p, Urb - %p\n",
                Endpoint,
                Urb);

    Transfer = Urb->UrbControlTransfer.hca.Reserved8[0];
    //FIXME USBPORT_ResetEndpointIdle();
    InsertTailList(&Endpoint->PendingTransferList, &Transfer->TransferLink);
    Urb->UrbHeader.Status = USBD_STATUS_PENDING;
}

BOOLEAN
NTAPI
USBPORT_QueueActiveUrbToEndpoint(IN PUSBPORT_ENDPOINT Endpoint,
                                 IN PURB Urb)
{
    PUSBPORT_TRANSFER Transfer;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_QueueActiveUrbToEndpoint: Endpoint - %p, Urb - %p\n",
                Endpoint,
                Urb);

    Transfer = Urb->UrbControlTransfer.hca.Reserved8[0];
    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock,
                      &Endpoint->EndpointOldIrql);

    if ((Endpoint->Flags & ENDPOINT_FLAG_NUKE) ||
        (Transfer->Flags & TRANSFER_FLAG_ABORTED))
    {
        InsertTailList(&Endpoint->CancelList, &Transfer->TransferLink);

        KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                          Endpoint->EndpointOldIrql);

        //DPRINT_CORE("USBPORT_QueueActiveUrbToEndpoint: return FALSE\n");
        return FALSE;
    }

    if (Transfer->TransferParameters.TransferBufferLength == 0 ||
        !(Endpoint->Flags & ENDPOINT_FLAG_DMA_TYPE))
    {
        InsertTailList(&Endpoint->TransferList, &Transfer->TransferLink);

        KeReleaseSpinLock(&Endpoint->EndpointSpinLock,
                          Endpoint->EndpointOldIrql);

        //DPRINT_CORE("USBPORT_QueueActiveUrbToEndpoint: return FALSE\n");
        return FALSE;
    }

    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    KeAcquireSpinLock(&FdoExtension->MapTransferSpinLock, &OldIrql);

    InsertTailList(&FdoExtension->MapTransferList, &Transfer->TransferLink);

    DeviceHandle = Transfer->Urb->UrbHeader.UsbdDeviceHandle;
    InterlockedIncrement(&DeviceHandle->DeviceHandleLock);

    KeReleaseSpinLock(&FdoExtension->MapTransferSpinLock, OldIrql);

    //DPRINT_CORE("USBPORT_QueueActiveUrbToEndpoint: return TRUE\n");
    return TRUE;
}

VOID
NTAPI
USBPORT_QueuePendingTransferIrp(IN PIRP Irp)
{
    PURB Urb;
    PUSBPORT_TRANSFER Transfer;
    PUSBPORT_ENDPOINT Endpoint;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;

    DPRINT_CORE("USBPORT_QueuePendingTransferIrp: Irp - %p\n", Irp);

    Urb = URB_FROM_IRP(Irp);

    Transfer = Urb->UrbControlTransfer.hca.Reserved8[0];
    Endpoint = Transfer->Endpoint;

    FdoDevice = Endpoint->FdoDevice;
    FdoExtension = FdoDevice->DeviceExtension;

    Irp->IoStatus.Status = STATUS_PENDING;
    IoMarkIrpPending(Irp);

    IoSetCancelRoutine(Irp, USBPORT_CancelPendingTransferIrp);

    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
    {
        USBPORT_CompleteTransfer(Urb, USBD_STATUS_CANCELED);
    }
    else
    {
        USBPORT_InsertIrpInTable(FdoExtension->PendingIrpTable, Irp);
        USBPORT_QueuePendingUrbToEndpoint(Endpoint, Urb);
    }
}

VOID
NTAPI
USBPORT_QueueTransferUrb(IN PURB Urb)
{
    PUSBPORT_TRANSFER Transfer;
    PUSBPORT_ENDPOINT Endpoint;
    PIRP Irp;
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    PUSBPORT_TRANSFER_PARAMETERS Parameters;

    DPRINT_CORE("USBPORT_QueueTransferUrb: Urb - %p\n", Urb);

    if (Urb->UrbControlTransfer.TransferFlags & USBD_DEFAULT_PIPE_TRANSFER)
        Urb->UrbHeader.Function = URB_FUNCTION_CONTROL_TRANSFER;

    Transfer = Urb->UrbControlTransfer.hca.Reserved8[0];
    Parameters = &Transfer->TransferParameters;

    Endpoint = Transfer->Endpoint;
    Endpoint->Flags &= ~ENDPOINT_FLAG_QUEUENE_EMPTY;

    Parameters->TransferBufferLength = Urb->UrbControlTransfer.TransferBufferLength;
    Parameters->TransferFlags = Urb->UrbControlTransfer.TransferFlags;

    Transfer->TransferBufferMDL = Urb->UrbControlTransfer.TransferBufferMDL;

    if (Urb->UrbControlTransfer.TransferFlags & USBD_TRANSFER_DIRECTION_IN)
    {
        Transfer->Direction = USBPORT_DMA_DIRECTION_FROM_DEVICE;
    }
    else
    {
        Transfer->Direction = USBPORT_DMA_DIRECTION_TO_DEVICE;
    }

    if (Endpoint->EndpointProperties.TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
    {
        RtlCopyMemory(&Parameters->SetupPacket,
                      Urb->UrbControlTransfer.SetupPacket,
                      sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    }

    DPRINT_URB("... URB TransferBufferLength - %x\n",
           Urb->UrbControlTransfer.TransferBufferLength);

    Urb->UrbControlTransfer.TransferBufferLength = 0;

    Irp = Transfer->Irp;

    if (Irp)
    {
        USBPORT_QueuePendingTransferIrp(Irp);
    }
    else
    {
        USBPORT_QueuePendingUrbToEndpoint(Endpoint, Urb);
    }

    DeviceHandle = Urb->UrbHeader.UsbdDeviceHandle;
    InterlockedDecrement(&DeviceHandle->DeviceHandleLock);

    USBPORT_FlushPendingTransfers(Endpoint);

    DPRINT_URB("... URB TransferBufferLength - %x\n",
           Urb->UrbControlTransfer.TransferBufferLength);

    if (Urb->UrbControlTransfer.TransferBufferLength)
    {
        PULONG Buffer;
        ULONG BufferLength;
        ULONG_PTR BufferEnd;
        ULONG ix;

        Buffer = Urb->UrbControlTransfer.TransferBuffer;
        BufferLength = Urb->UrbControlTransfer.TransferBufferLength;
        BufferEnd = (ULONG_PTR)Buffer + BufferLength;

        DPRINT_URB("URB TransferBuffer - %p\n", Buffer);

        for (ix = 0; (ULONG_PTR)(Buffer + ix) < BufferEnd; ix++)
        {
            DPRINT_URB("Buffer[%02X] - %p\n", ix, Buffer[ix]);
        }
    }
}

VOID
NTAPI
USBPORT_FlushAllEndpoints(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PLIST_ENTRY Entry;
    PUSBPORT_ENDPOINT Endpoint;
    LIST_ENTRY List;
    KIRQL OldIrql;

    DPRINT_CORE("USBPORT_FlushAllEndpoints: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);

    InitializeListHead(&List);

    Entry = FdoExtension->EndpointList.Flink;

    while (Entry && Entry != &FdoExtension->EndpointList)
    {
        Endpoint = CONTAINING_RECORD(Entry,
                                     USBPORT_ENDPOINT,
                                     EndpointLink);

        if (USBPORT_GetEndpointState(Endpoint) != USBPORT_ENDPOINT_CLOSED)
        {
            InsertTailList(&List, &Endpoint->FlushLink);
        }

        Entry = Endpoint->EndpointLink.Flink;
    }

    KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);

    while (!IsListEmpty(&List))
    {
        Endpoint = CONTAINING_RECORD(List.Flink,
                                     USBPORT_ENDPOINT,
                                     FlushLink);

        RemoveHeadList(&List);

        Endpoint->FlushLink.Flink = NULL;
        Endpoint->FlushLink.Blink = NULL;

        if (!IsListEmpty(&Endpoint->PendingTransferList))
        {
            USBPORT_FlushPendingTransfers(Endpoint);
        }
    }

    DPRINT_CORE("USBPORT_FlushAllEndpoints: exit\n");
}

ULONG
NTAPI
USBPORT_KillEndpointActiveTransfers(IN PDEVICE_OBJECT FdoDevice,
                                    IN PUSBPORT_ENDPOINT Endpoint)
{
    PLIST_ENTRY ActiveList;
    PUSBPORT_TRANSFER Transfer;
    ULONG KilledTransfers = 0;

    DPRINT_CORE("USBPORT_KillEndpointActiveTransfers \n");

    ActiveList = Endpoint->TransferList.Flink;

    while (ActiveList && ActiveList != &Endpoint->TransferList)
    {
        ++KilledTransfers;

        Transfer = CONTAINING_RECORD(ActiveList,
                                     USBPORT_TRANSFER,
                                     TransferLink);

        Transfer->Flags |= TRANSFER_FLAG_ABORTED;

        ActiveList = Transfer->TransferLink.Flink;
    }

    USBPORT_FlushPendingTransfers(Endpoint);
    USBPORT_FlushCancelList(Endpoint);

    return KilledTransfers;
}

VOID
NTAPI
USBPORT_FlushController(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION  FdoExtension;
    PLIST_ENTRY Entry;
    PUSBPORT_ENDPOINT Endpoint;
    ULONG KilledTransfers;
    PLIST_ENTRY EndpointList;
    KIRQL OldIrql;
    LIST_ENTRY FlushList;

    DPRINT_CORE("USBPORT_FlushController \n");

    FdoExtension = FdoDevice->DeviceExtension;

    EndpointList = &FdoExtension->EndpointList;

    while (TRUE)
    {
        KeAcquireSpinLock(&FdoExtension->EndpointListSpinLock, &OldIrql);

        InitializeListHead(&FlushList);

        Entry = EndpointList->Flink;

        if (!IsListEmpty(EndpointList))
        {
            while (Entry && Entry != EndpointList)
            {
                Endpoint = CONTAINING_RECORD(Entry,
                                             USBPORT_ENDPOINT,
                                             EndpointLink);

                if (Endpoint->StateLast != USBPORT_ENDPOINT_REMOVE &&
                    Endpoint->StateLast != USBPORT_ENDPOINT_CLOSED)
                {
                    InterlockedIncrement(&Endpoint->LockCounter);
                    InsertTailList(&FlushList, &Endpoint->FlushControllerLink);
                }

                Entry = Endpoint->EndpointLink.Flink;
            }
        }

        KeReleaseSpinLock(&FdoExtension->EndpointListSpinLock, OldIrql);

        KilledTransfers = 0;
        while (!IsListEmpty(&FlushList))
        {
            Endpoint = CONTAINING_RECORD(FlushList.Flink,
                                         USBPORT_ENDPOINT,
                                         FlushControllerLink);

            RemoveHeadList(&FlushList);

            KilledTransfers += USBPORT_KillEndpointActiveTransfers(FdoDevice,
                                                                   Endpoint);

            InterlockedDecrement(&Endpoint->LockCounter);
        }

        if (!KilledTransfers)
            break;

        USBPORT_Wait(FdoDevice, 100);
    }
}

VOID
NTAPI
USBPORT_BadRequestFlush(IN PDEVICE_OBJECT FdoDevice)
{
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PIRP Irp;

    DPRINT_QUEUE("USBPORT_BadRequestFlush: ... \n");

    FdoExtension = FdoDevice->DeviceExtension;

    while (TRUE)
    {
        Irp = IoCsqRemoveNextIrp(&FdoExtension->BadRequestIoCsq, 0);

        if (!Irp)
            break;

        DPRINT1("USBPORT_BadRequestFlush: Irp - %p\n", Irp);

        Irp->IoStatus.Status = STATUS_DEVICE_NOT_CONNECTED;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
}

VOID
NTAPI
USBPORT_AbortEndpoint(IN PDEVICE_OBJECT FdoDevice,
                      IN PUSBPORT_ENDPOINT Endpoint,
                      IN PIRP Irp)
{
    PLIST_ENTRY PendingList;
    PUSBPORT_TRANSFER PendingTransfer;
    PLIST_ENTRY ActiveList;
    PUSBPORT_TRANSFER ActiveTransfer;

    DPRINT_CORE("USBPORT_AbortEndpoint: Irp - %p\n", Irp);

    KeAcquireSpinLock(&Endpoint->EndpointSpinLock, &Endpoint->EndpointOldIrql);

    if (Irp)
    {
        InsertTailList(&Endpoint->AbortList, &Irp->Tail.Overlay.ListEntry);
    }

    PendingList = Endpoint->PendingTransferList.Flink;

    while (PendingList && PendingList != &Endpoint->PendingTransferList)
    {
        PendingTransfer = CONTAINING_RECORD(PendingList,
                                            USBPORT_TRANSFER,
                                            TransferLink);

        DPRINT_CORE("USBPORT_AbortEndpoint: Abort PendingTransfer - %p\n",
                    PendingTransfer);

        PendingTransfer->Flags |= TRANSFER_FLAG_ABORTED;

        PendingList = PendingTransfer->TransferLink.Flink;
    }

    ActiveList = Endpoint->TransferList.Flink;

    while (ActiveList && ActiveList != &Endpoint->TransferList)
    {
        ActiveTransfer = CONTAINING_RECORD(ActiveList,
                                           USBPORT_TRANSFER,
                                           TransferLink);

        DPRINT_CORE("USBPORT_AbortEndpoint: Abort ActiveTransfer - %p\n",
                    ActiveTransfer);

        ActiveTransfer->Flags |= TRANSFER_FLAG_ABORTED;

        if (Endpoint->Flags & ENDPOINT_FLAG_ABORTING)
        {
            ActiveTransfer->Flags |= TRANSFER_FLAG_DEVICE_GONE;
        }

        ActiveList = ActiveTransfer->TransferLink.Flink;
    }

    KeReleaseSpinLock(&Endpoint->EndpointSpinLock, Endpoint->EndpointOldIrql);

    USBPORT_InvalidateEndpointHandler(FdoDevice,
                                      Endpoint,
                                      INVALIDATE_ENDPOINT_INT_NEXT_SOF);

    USBPORT_FlushPendingTransfers(Endpoint);
    USBPORT_FlushCancelList(Endpoint);
}
