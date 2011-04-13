/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/hwiface.c
 * PURPOSE:     EHCI Interface routines: Queue Heads and Queue Element
                Transfer Descriptors.
 * TODO:        Periodic Frame List, Isochronous Transaction Descriptors
                and Split-transaction ITD.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#include "hwiface.h"
#include "physmem.h"
#define NDEBUG
#include <debug.h>

/* Queue Element Transfer Descriptors */

PQUEUE_TRANSFER_DESCRIPTOR
CreateDescriptor(
    PEHCI_HOST_CONTROLLER hcd, 
    UCHAR PIDCode, 
    ULONG TotalBytesToTransfer)
{
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;
    ULONG PhysicalAddress;
    UCHAR i;
    NTSTATUS Status;

    Status = DmaMemAllocator_Allocate(hcd->DmaMemAllocator, sizeof(QUEUE_TRANSFER_DESCRIPTOR), (PVOID*)&Descriptor, &PhysicalAddress);
    if (!NT_SUCCESS(Status))
       return NULL;

    RtlZeroMemory(Descriptor, sizeof(QUEUE_TRANSFER_DESCRIPTOR));
    Descriptor->NextPointer = TERMINATE_POINTER;
    Descriptor->AlternateNextPointer = TERMINATE_POINTER;
    Descriptor->Token.Bits.DataToggle = TRUE;
    Descriptor->Token.Bits.InterruptOnComplete = TRUE;
    Descriptor->Token.Bits.ErrorCounter = 0x03;
    Descriptor->Token.Bits.Active = TRUE;
    Descriptor->Token.Bits.PIDCode = PIDCode;
    Descriptor->Token.Bits.TotalBytesToTransfer = TotalBytesToTransfer;
    Descriptor->PhysicalAddr = PhysicalAddress;

    return Descriptor;
}

VOID
FreeDescriptor(
    PEHCI_HOST_CONTROLLER hcd, 
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor)
{
    DmaMemAllocator_Free(hcd->DmaMemAllocator, Descriptor, sizeof(QUEUE_TRANSFER_DESCRIPTOR));
}

/* Queue Head */

VOID
DumpQueueHeadList(PEHCI_HOST_CONTROLLER hcd)
{
    KIRQL OldIrql;
    PQUEUE_HEAD QueueHead, FirstQueueHead;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);

    QueueHead = (PQUEUE_HEAD)hcd->CommonBufferVA;
    FirstQueueHead = QueueHead;
    DPRINT1("Dumping QueueHead List!!!!!!!!!!!!!\n");
    while (1)
    {
        DPRINT1("QueueHead Address %x\n", QueueHead);
        DPRINT1("QueueHead->PreviousQueueHead = %x\n", QueueHead->PreviousQueueHead);
        DPRINT1("QueueHead->NextQueueHead = %x\n", QueueHead->NextQueueHead);
        DPRINT1(" ---> PhysicalAddress %x\n", (ULONG)MmGetPhysicalAddress(QueueHead).LowPart);
        DPRINT1("QueueHead->HorizontalLinkPointer %x\n", QueueHead->HorizontalLinkPointer);
        QueueHead = QueueHead->NextQueueHead;
        DPRINT1("Next QueueHead %x\n", QueueHead);
        if (QueueHead == FirstQueueHead) break;
    }
    DPRINT1("-----------------------------------\n");
    KeReleaseSpinLock(&hcd->Lock, OldIrql);
}

PQUEUE_HEAD
CreateQueueHead(PEHCI_HOST_CONTROLLER hcd)
{
    PQUEUE_HEAD CurrentQH;
    NTSTATUS Status;
    PHYSICAL_ADDRESS PhysicalAddress;

    Status = DmaMemAllocator_Allocate(hcd->DmaMemAllocator, sizeof(QUEUE_HEAD), (PVOID*)&CurrentQH, &PhysicalAddress);
    if (!NT_SUCCESS(Status))
       return NULL;

    RtlZeroMemory(CurrentQH, sizeof(QUEUE_HEAD));

    ASSERT(CurrentQH);
    CurrentQH->PhysicalAddr = PhysicalAddress.LowPart;
    CurrentQH->HorizontalLinkPointer = TERMINATE_POINTER;
    CurrentQH->CurrentLinkPointer = TERMINATE_POINTER;
    CurrentQH->AlternateNextPointer = TERMINATE_POINTER;
    CurrentQH->NextPointer = TERMINATE_POINTER;

    /* 1 for non high speed, 0 for high speed device */
    CurrentQH->EndPointCharacteristics.ControlEndPointFlag = 0;
    CurrentQH->EndPointCharacteristics.HeadOfReclamation = FALSE;
    CurrentQH->EndPointCharacteristics.MaximumPacketLength = 64;

    /* Set NakCountReload to max value possible */
    CurrentQH->EndPointCharacteristics.NakCountReload = 0xF;

    /* Get the Initial Data Toggle from the QEDT */
    CurrentQH->EndPointCharacteristics.QEDTDataToggleControl = TRUE;

    /* High Speed Device */
    CurrentQH->EndPointCharacteristics.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;

    CurrentQH->EndPointCapabilities.NumberOfTransactionPerFrame = 0x03;

    CurrentQH->Token.DWord = 0;
    CurrentQH->NextQueueHead = NULL;
    CurrentQH->PreviousQueueHead = NULL;
    CurrentQH->Token.Bits.InterruptOnComplete = TRUE;


    return CurrentQH;
}

VOID
LinkQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead)
{
    KIRQL OldIrql;
    PQUEUE_HEAD CurrentHead  = (PQUEUE_HEAD)hcd->AsyncListQueue;
    PQUEUE_HEAD PreviousHead = CurrentHead->PreviousQueueHead;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);

    QueueHead->HorizontalLinkPointer = (CurrentHead->HorizontalLinkPointer | QH_TYPE_QH) & ~TERMINATE_POINTER;
    QueueHead->NextQueueHead = CurrentHead;
    QueueHead->PreviousQueueHead = PreviousHead;

    CurrentHead->PreviousQueueHead = QueueHead;
    if (PreviousHead)
        PreviousHead->NextQueueHead = QueueHead;

    CurrentHead->HorizontalLinkPointer = QueueHead->PhysicalAddr | QH_TYPE_QH;

    KeReleaseSpinLock(&hcd->Lock, OldIrql);
}

VOID
UnlinkQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead)
{
    KIRQL OldIrql;
    PQUEUE_HEAD PreviousHead = QueueHead->PreviousQueueHead;
    PQUEUE_HEAD NextHead = QueueHead->NextQueueHead;
    KeAcquireSpinLock(&hcd->Lock, &OldIrql);

    if (PreviousHead)
    {
        PreviousHead->NextQueueHead = NextHead;
        PreviousHead->HorizontalLinkPointer = QueueHead->HorizontalLinkPointer;
    }
    if (NextHead)
        NextHead->PreviousQueueHead = PreviousHead;

    KeReleaseSpinLock(&hcd->Lock, OldIrql);
}

VOID
DeleteQueueHead(
    PEHCI_HOST_CONTROLLER hcd,
    PQUEUE_HEAD QueueHead)
{
    DmaMemAllocator_Free(hcd->DmaMemAllocator, QueueHead, sizeof(QUEUE_HEAD));
}

