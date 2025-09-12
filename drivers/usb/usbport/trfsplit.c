/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort split transfer functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

ULONG
NTAPI
USBPORT_MakeSplitTransfer(IN PDEVICE_OBJECT FdoDevice,
                          IN PUSBPORT_TRANSFER Transfer,
                          IN PUSBPORT_TRANSFER SplitTransfer,
                          IN ULONG MaxTransferSize,
                          IN PULONG SgIdx,
                          IN PULONG SgOffset,
                          IN ULONG TransferRemainLen,
                          IN ULONG TransferOffset)
{
    PUSBPORT_SCATTER_GATHER_LIST SplitSgList;
    PUSBPORT_SCATTER_GATHER_ELEMENT Element0;
    PUSBPORT_SCATTER_GATHER_ELEMENT Element1;
    SIZE_T SgLength;
    SIZE_T SgRemainLen;

    DPRINT("USBPORT_MakeSplitTransfer: ... \n");

    SplitSgList = &SplitTransfer->SgList;
    Element0 = &SplitSgList->SgElement[0];

    SgLength = Transfer->SgList.SgElement[*SgIdx].SgTransferLength - *SgOffset;

    if (SgLength > MaxTransferSize)
    {
        /* SgLength > MaxTransferSize */
        SplitTransfer->SgList.SgElementCount = 1;

        Element0->SgOffset = 0;
        Element0->SgTransferLength = MaxTransferSize;
        Element0->SgPhysicalAddress.LowPart = Transfer->SgList.SgElement[*SgIdx].SgPhysicalAddress.LowPart + *SgOffset;

        SplitTransfer->TransferParameters.IsTransferSplited = TRUE;
        SplitTransfer->TransferParameters.TransferBufferLength = MaxTransferSize;

        SplitTransfer->SgList.CurrentVa = Transfer->SgList.CurrentVa + TransferOffset;
        SplitTransfer->SgList.MappedSystemVa = (PVOID)((ULONG_PTR)Transfer->SgList.MappedSystemVa + TransferOffset);

        SplitTransfer->Flags |= TRANSFER_FLAG_SPLITED;

        *SgOffset += MaxTransferSize;
        TransferRemainLen -= MaxTransferSize;
        return TransferRemainLen;
    }

    /* SgLength <= MaxTransferSize */
    SplitTransfer->SgList.SgElementCount = 1;
    TransferRemainLen -= SgLength;

    Element0->SgOffset = 0;
    Element0->SgTransferLength = SgLength;
    Element0->SgPhysicalAddress.LowPart = Transfer->SgList.SgElement[*SgIdx].SgPhysicalAddress.LowPart + *SgOffset;

    SplitTransfer->TransferParameters.TransferBufferLength = SgLength;
    SplitTransfer->TransferParameters.IsTransferSplited = TRUE;

    SplitTransfer->SgList.CurrentVa = Transfer->SgList.CurrentVa + TransferOffset;
    SplitTransfer->SgList.MappedSystemVa = (PVOID)((ULONG_PTR)Transfer->SgList.MappedSystemVa + TransferOffset);

    SplitTransfer->Flags |= TRANSFER_FLAG_SPLITED;

    *SgOffset += SgLength;

    SgRemainLen = MaxTransferSize - SgLength;

    if (SgRemainLen > TransferRemainLen)
    {
        SgRemainLen = TransferRemainLen;
    }

    if (!SgRemainLen)
    {
        /* SgLength == MaxTransferSize */
        ++*SgIdx;
        *SgOffset = 0;
        return TransferRemainLen;
    }

    /* SgLength < MaxTransferSize */

    DPRINT1("MakeSplitTransfer: SgRemainLen - %x\n", SgRemainLen);
    DPRINT1("MakeSplitTransfer: SgIdx - %x\n", *SgIdx);
    ++*SgIdx;

    *SgOffset = 0;
    SplitTransfer->SgList.SgElementCount++;

    Element1 = &SplitSgList->SgElement[1];

    Element1->SgOffset = SgRemainLen;
    Element1->SgTransferLength = Element0->SgTransferLength;
    Element1->SgPhysicalAddress.LowPart = Transfer->SgList.SgElement[*SgIdx].SgPhysicalAddress.LowPart + *SgOffset;

    SplitTransfer->TransferParameters.TransferBufferLength += SgRemainLen;

    *SgOffset += SgRemainLen;
    TransferRemainLen -= SgRemainLen;

    return TransferRemainLen;
}

VOID
NTAPI
USBPORT_SplitBulkInterruptTransfer(IN PDEVICE_OBJECT FdoDevice,
                                   IN PUSBPORT_ENDPOINT Endpoint,
                                   IN PUSBPORT_TRANSFER Transfer,
                                   IN PLIST_ENTRY List)
{
    PUSBPORT_TRANSFER SplitTransfer;
    LIST_ENTRY tmplist;
    ULONG NeedSplits;
    SIZE_T TransferBufferLength;
    SIZE_T MaxTransferSize;
    SIZE_T TransferOffset = 0;
    SIZE_T RemainLength;
    ULONG ix;
    ULONG SgIdx = 0;
    ULONG SgOffset = 0;

    DPRINT("USBPORT_SplitBulkInterruptTransfer: ... \n");

    MaxTransferSize = Endpoint->EndpointProperties.TotalMaxPacketSize *
                      (Endpoint->EndpointProperties.MaxTransferSize /
                       Endpoint->EndpointProperties.TotalMaxPacketSize);

    if (Endpoint->EndpointProperties.MaxTransferSize > PAGE_SIZE)
    {
        KeBugCheckEx(BUGCODE_USB_DRIVER, 1, 0, 0, 0);
    }

    TransferBufferLength = Transfer->TransferParameters.TransferBufferLength;
    Transfer->Flags |= TRANSFER_FLAG_PARENT;

    NeedSplits = TransferBufferLength / MaxTransferSize + 1;

    InitializeListHead(&tmplist);

    DPRINT("USBPORT_SplitBulkInterruptTransfer: TransferBufferLength - %x, NeedSplits - %x\n",
           TransferBufferLength, NeedSplits);

    if (!NeedSplits)
    {
        DPRINT1("USBPORT_SplitBulkInterruptTransfer: DbgBreakPoint \n");
        DbgBreakPoint();
        goto Exit;
    }

    for (ix = 0; ix < NeedSplits; ++ix)
    {
        SplitTransfer = ExAllocatePoolWithTag(NonPagedPool,
                                              Transfer->FullTransferLength,
                                              USB_PORT_TAG);

        if (!SplitTransfer)
        {
            DPRINT1("USBPORT_SplitBulkInterruptTransfer: DbgBreakPoint \n");
            DbgBreakPoint();
            goto Exit;
        }

        RtlCopyMemory(SplitTransfer, Transfer, Transfer->FullTransferLength);

        SplitTransfer->MiniportTransfer = (PVOID)((ULONG_PTR)SplitTransfer +
                                          SplitTransfer->PortTransferLength);

        InsertTailList(&tmplist, &SplitTransfer->TransferLink);
    }

    if (Transfer->TransferParameters.TransferBufferLength == 0)
    {
        goto Exit;
    }

    RemainLength = Transfer->TransferParameters.TransferBufferLength;

    do
    {
        SplitTransfer = CONTAINING_RECORD(tmplist.Flink,
                                          USBPORT_TRANSFER,
                                          TransferLink);

        RemoveHeadList(&tmplist);

        RemainLength = USBPORT_MakeSplitTransfer(FdoDevice,
                                                 Transfer,
                                                 SplitTransfer,
                                                 MaxTransferSize,
                                                 &SgIdx,
                                                 &SgOffset,
                                                 RemainLength,
                                                 TransferOffset);

        TransferOffset += SplitTransfer->TransferParameters.TransferBufferLength;

        InsertTailList(List, &SplitTransfer->TransferLink);
        InsertTailList(&Transfer->SplitTransfersList,&SplitTransfer->SplitLink);
    }
    while (RemainLength != 0);

Exit:

    while (!IsListEmpty(&tmplist))
    {
        DPRINT("USBPORT_SplitBulkInterruptTransfer: ... \n");

        SplitTransfer = CONTAINING_RECORD(tmplist.Flink,
                                          USBPORT_TRANSFER,
                                          TransferLink);
        RemoveHeadList(&tmplist);

        ExFreePoolWithTag(SplitTransfer, USB_PORT_TAG);
    }

    return;
}

VOID
NTAPI
USBPORT_SplitTransfer(IN PDEVICE_OBJECT FdoDevice,
                      IN PUSBPORT_ENDPOINT Endpoint,
                      IN PUSBPORT_TRANSFER Transfer,
                      IN PLIST_ENTRY List)
{
    ULONG TransferType;

    DPRINT("USBPORT_SplitTransfer ... \n");

    InitializeListHead(List);
    InitializeListHead(&Transfer->SplitTransfersList);

    Transfer->USBDStatus = USBD_STATUS_SUCCESS;

    if (Transfer->TransferParameters.TransferBufferLength >
        Endpoint->EndpointProperties.MaxTransferSize)
    {
        TransferType = Endpoint->EndpointProperties.TransferType;

        if (TransferType == USBPORT_TRANSFER_TYPE_BULK ||
            TransferType == USBPORT_TRANSFER_TYPE_INTERRUPT)
        {
            USBPORT_SplitBulkInterruptTransfer(FdoDevice,
                                               Endpoint,
                                               Transfer,
                                               List);
        }
        else if (TransferType == USBPORT_TRANSFER_TYPE_ISOCHRONOUS ||
                 TransferType == USBPORT_TRANSFER_TYPE_CONTROL)
        {
            KeBugCheckEx(BUGCODE_USB_DRIVER, 1, 0, 0, 0);
        }
        else
        {
            DPRINT1("USBPORT_SplitTransfer: Unknown TransferType - %x\n",
                    TransferType);
        }
    }
    else
    {
        InsertTailList(List, &Transfer->TransferLink);
    }
}

VOID
NTAPI
USBPORT_DoneSplitTransfer(IN PUSBPORT_TRANSFER SplitTransfer)
{
    PUSBPORT_TRANSFER ParentTransfer;
    KIRQL OldIrql;

    DPRINT("USBPORT_DoneSplitTransfer: ... \n");

    ParentTransfer = SplitTransfer->ParentTransfer;
    ParentTransfer->CompletedTransferLen += SplitTransfer->CompletedTransferLen;

    if (SplitTransfer->USBDStatus != USBD_STATUS_SUCCESS)
    {
        DPRINT1("USBPORT_DoneSplitTransfer: SplitTransfer->USBDStatus - %X\n",
                SplitTransfer->USBDStatus);

        ParentTransfer->USBDStatus = SplitTransfer->USBDStatus;
    }

    KeAcquireSpinLock(&ParentTransfer->TransferSpinLock, &OldIrql);

    RemoveEntryList(&SplitTransfer->SplitLink);
    ExFreePoolWithTag(SplitTransfer, USB_PORT_TAG);

    if (IsListEmpty(&ParentTransfer->SplitTransfersList))
    {
        KeReleaseSpinLock(&ParentTransfer->TransferSpinLock, OldIrql);
        USBPORT_DoneTransfer(ParentTransfer);
    }
    else
    {
        KeReleaseSpinLock(&ParentTransfer->TransferSpinLock, OldIrql);
    }
}

VOID
NTAPI
USBPORT_CancelSplitTransfer(IN PUSBPORT_TRANSFER SplitTransfer)
{
    PUSBPORT_TRANSFER ParentTransfer;
    PUSBPORT_ENDPOINT Endpoint;
    KIRQL OldIrql;

    DPRINT("USBPORT_CancelSplitTransfer \n");

    Endpoint = SplitTransfer->Endpoint;
    ParentTransfer = SplitTransfer->ParentTransfer;
    ParentTransfer->CompletedTransferLen += SplitTransfer->CompletedTransferLen;

    KeAcquireSpinLock(&ParentTransfer->TransferSpinLock, &OldIrql);
    RemoveEntryList(&SplitTransfer->SplitLink);
    KeReleaseSpinLock(&ParentTransfer->TransferSpinLock, OldIrql);

    ExFreePool(SplitTransfer);

    if (IsListEmpty(&ParentTransfer->SplitTransfersList))
    {
        InsertTailList(&Endpoint->CancelList, &ParentTransfer->TransferLink);
    }
}
