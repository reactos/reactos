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
CreateDescriptor(PEHCI_HOST_CONTROLLER hcd, UCHAR PIDCode, ULONG TotalBytesToTransfer)
{
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;
    ULONG PhysicalAddress;
    UCHAR i;
    KIRQL OldIrql;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);

    Descriptor = (PQUEUE_TRANSFER_DESCRIPTOR)AllocateMemory(hcd, sizeof(QUEUE_TRANSFER_DESCRIPTOR), &PhysicalAddress);
    RtlZeroMemory(Descriptor, sizeof(QUEUE_TRANSFER_DESCRIPTOR));
    Descriptor->NextPointer = TERMINATE_POINTER;
    Descriptor->AlternateNextPointer = TERMINATE_POINTER;
    Descriptor->Token.Bits.DataToggle = TRUE;
    Descriptor->Token.Bits.ErrorCounter = 0x03;
    Descriptor->Token.Bits.Active = TRUE;
    Descriptor->Token.Bits.PIDCode = PIDCode;
    Descriptor->Token.Bits.TotalBytesToTransfer = TotalBytesToTransfer;
    Descriptor->PhysicalAddr = PhysicalAddress;
    for (i=0;i<5;i++)
        Descriptor->BufferPointer[i] = 0;

    KeReleaseSpinLock(&hcd->Lock, OldIrql);

    return Descriptor;
}

VOID
FreeDescriptor(PEHCI_HOST_CONTROLLER hcd, PQUEUE_TRANSFER_DESCRIPTOR Descriptor)
{
    ReleaseMemory(hcd, (ULONG)Descriptor);
}

/* Queue Head */

VOID
DumpQueueHeadList(PEHCI_HOST_CONTROLLER hcd)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);

    PQUEUE_HEAD QueueHead = (PQUEUE_HEAD)hcd->CommonBufferVA;
    PQUEUE_HEAD FirstQueueHead = QueueHead;
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
    ULONG PhysicalAddress , i;
    KIRQL OldIrql;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);
    CurrentQH = (PQUEUE_HEAD)AllocateMemory(hcd, sizeof(QUEUE_HEAD), &PhysicalAddress);
    RtlZeroMemory(CurrentQH, sizeof(QUEUE_HEAD));

    ASSERT(CurrentQH);
    CurrentQH->PhysicalAddr = PhysicalAddress;
    CurrentQH->HorizontalLinkPointer = TERMINATE_POINTER;
    CurrentQH->AlternateNextPointer = TERMINATE_POINTER;
    CurrentQH->NextPointer = TERMINATE_POINTER;

    /* 1 for non high speed, 0 for high speed device */
    CurrentQH->EndPointCharacteristics.ControlEndPointFlag = 0;
    CurrentQH->EndPointCharacteristics.HeadOfReclamation = FALSE;
    CurrentQH->EndPointCharacteristics.MaximumPacketLength = 64;

    /* Set NakCountReload to max value possible */
    CurrentQH->EndPointCharacteristics.NakCountReload = 0xF;

    /* Get the Initial Data Toggle from the QEDT */
    CurrentQH->EndPointCharacteristics.QEDTDataToggleControl = FALSE;

    /* High Speed Device */
    CurrentQH->EndPointCharacteristics.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;

    CurrentQH->EndPointCapabilities.NumberOfTransactionPerFrame = 0x03;

    CurrentQH->Token.DWord = 0;
    CurrentQH->NextQueueHead = NULL;
    CurrentQH->PreviousQueueHead = NULL;
    for (i=0; i<5; i++)
        CurrentQH->BufferPointer[i] = 0;

    CurrentQH->Token.Bits.InterruptOnComplete = FALSE;

    KeReleaseSpinLock(&hcd->Lock, OldIrql);
    return CurrentQH;
}

VOID
LinkQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead)
{
    KIRQL OldIrql;
    PQUEUE_HEAD CurrentHead  = (PQUEUE_HEAD)hcd->AsyncListQueue;
    PQUEUE_HEAD PreviousHead;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);
    PreviousHead = CurrentHead->PreviousQueueHead;
    QueueHead->NextQueueHead = CurrentHead;
    QueueHead->PreviousQueueHead = PreviousHead;
    PreviousHead->NextQueueHead = QueueHead;
    CurrentHead->PreviousQueueHead = QueueHead;
    
    QueueHead->HorizontalLinkPointer = (CurrentHead->HorizontalLinkPointer | QH_TYPE_QH) & ~TERMINATE_POINTER;
    PreviousHead->HorizontalLinkPointer = QueueHead->PhysicalAddr| QH_TYPE_QH;

    KeReleaseSpinLock(&hcd->Lock, OldIrql);
}

VOID
UnlinkQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead)
{
    KIRQL OldIrql;
    PQUEUE_HEAD PreviousHead;
    PQUEUE_HEAD NextHead;
    KeAcquireSpinLock(&hcd->Lock, &OldIrql);

    PreviousHead = QueueHead->PreviousQueueHead;
    NextHead = QueueHead->NextQueueHead;

    PreviousHead->NextQueueHead = NextHead;
    NextHead->PreviousQueueHead = PreviousHead;

    PreviousHead->HorizontalLinkPointer = NextHead->HorizontalLinkPointer;
    KeReleaseSpinLock(&hcd->Lock, OldIrql);
}

VOID
LinkQueueHeadToCompletedList(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead)
{
    KIRQL OldIrql;
    PQUEUE_HEAD CurrentHead  = (PQUEUE_HEAD)hcd->CompletedListQueue;
    PQUEUE_HEAD PreviousHead;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);
        
    PreviousHead = CurrentHead->PreviousQueueHead;
    QueueHead->NextQueueHead = CurrentHead;
    QueueHead->PreviousQueueHead = PreviousHead;
    PreviousHead->NextQueueHead = QueueHead;
    CurrentHead->PreviousQueueHead = QueueHead;

    KeReleaseSpinLock(&hcd->Lock, OldIrql);

}

VOID
DeleteQueueHead(PEHCI_HOST_CONTROLLER hcd, PQUEUE_HEAD QueueHead)
{
    ReleaseMemory(hcd, (ULONG)QueueHead);
}

VOID
CleanupAsyncList(PEHCI_HOST_CONTROLLER hcd)
{
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;
    PQUEUE_HEAD QueueHead;
    KIRQL OldIrql;

    KeAcquireSpinLock(&hcd->Lock, &OldIrql);

    QueueHead = hcd->CompletedListQueue;
    QueueHead = QueueHead->NextQueueHead;

    while (QueueHead != hcd->CompletedListQueue)
    {
        Descriptor = QueueHead->FirstTransferDescriptor;
        while (Descriptor)
        {
            if (Descriptor->Token.Bits.PIDCode == PID_CODE_SETUP_TOKEN)
                ReleaseMemory(hcd, Descriptor->BufferPointerVA[0]);
            FreeDescriptor(hcd, Descriptor);
            Descriptor = Descriptor->NextDescriptor;
        }

        if (QueueHead->FreeMdl)
        {
            DPRINT("Freeing Mdl %x, StartVA %x\n", QueueHead->Mdl, QueueHead->Mdl->StartVa);
            IoFreeMdl(QueueHead->Mdl);
        }
        
        QueueHead = QueueHead->NextQueueHead;
    }
    
    hcd->CompletedListQueue->NextQueueHead = hcd->CompletedListQueue;
    hcd->CompletedListQueue->PreviousQueueHead = hcd->CompletedListQueue;
    
    KeReleaseSpinLock(&hcd->Lock, OldIrql);
}

