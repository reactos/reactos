/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     IPI code for x64
 * COPYRIGHT:   Copyright 2023 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ***/

KSPIN_LOCK KiIpiSpinLock;

/* FUNCTIONS *****************************************************************/

static PKIPI_BROADCAST_WORKER KiIpiBroadcastWorkerTable[] =
{
    NULL, // IPI_APC
    NULL, // IPI_DPC
    NULL, // IPI_FREEZE
};

VOID
FASTCALL
KiIpiInterruptHandler(
    _In_ PKTRAP_FRAME TrapFrame)
{
    PKPRCB Prcb = KeGetCurrentPrcb();
    PKPRCB SenderPrcb;
    ULONG SenderIndex;
    PKREQUEST_PACKET RequestPacket;
    PKIPI_WORKER WorkerRoutine;
    //__debugbreak();
    /* Process all request packets */
    while (Prcb->SenderSummary != 0)
    {
        /* Get the sender index */
        NT_VERIFY(BitScanForwardAffinity(&SenderIndex, Prcb->SenderSummary) != 0);
        ASSERT(SenderIndex < KeNumberProcessors);

        /* Get the request packet */
        RequestPacket = &Prcb->RequestMailbox[SenderIndex].RequestPacket;
        WorkerRoutine = RequestPacket->WorkerRoutine;

        /* Call the worker routine */
        WorkerRoutine(NULL,
                      RequestPacket->CurrentPacket[0],
                      RequestPacket->CurrentPacket[1],
                      RequestPacket->CurrentPacket[2]);

        /* Clear the request summary bit */
        //InterlockedAnd64(&Prcb->RequestMailbox[SenderIndex].RequestSummary, 0);

        /* Clear the sender summary bit */
        InterlockedBitTestAndReset64(&Prcb->SenderSummary, SenderIndex);

        /* Clear the sender's target set */
        SenderPrcb = KiProcessorBlock[SenderIndex];
        InterlockedBitTestAndReset64(&SenderPrcb->TargetSet, Prcb->Number);
    }
}

static
VOID
NTAPI
KiIpiGenericCallWorker(
    _In_ PKIPI_CONTEXT PacketContext,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3)
{
    PKIPI_BROADCAST_WORKER BroadcastWorker = Parameter1;
    ULONG_PTR Argument = (ULONG_PTR)Parameter2;
    PULONG Count = (PULONG)Parameter3;

    /* Acknowledge receival by decrementing the count */
    InterlockedDecrementUL(Count);

    /* Call the broadcast function */
    BroadcastWorker(Argument);
}

VOID
KiRequestIpi(
    _In_ KAFFINITY TargetSet)
{
    KIRQL OldIrql;

    /* Raise to sync level */
    KeRaiseIrql(SYNCH_LEVEL, &OldIrql);

    /* Acquire the spinlock */
    KeAcquireSpinLockAtDpcLevel(&KiIpiSpinLock);

    /* Request the IPI */
    HalRequestIpi(TargetSet);

    /* Lower to IRQL */
    KeReleaseSpinLockFromDpcLevel(&KiIpiSpinLock);
    KeLowerIrql(OldIrql);
}

static
VOID
KiIpiSendRequestPacket(
    _In_ KAFFINITY TargetSet,
    _In_ PKREQUEST_PACKET RequestPacket)
{
    PKPRCB CurrentPrcb = KeGetCurrentPrcb();
    KAFFINITY RemainingSet, SetMember;
    PKPRCB TargetPrcb;
    KIRQL OldIrql;
    ULONG ProcessorIndex;
    ULONG SenderIndex;

    /* Sanitize the target set */
    TargetSet &= KeActiveProcessors;

    /* Remove the current processor from the target set */
    CurrentPrcb->TargetSet = TargetSet & ~CurrentPrcb->SetMember;

    SenderIndex = CurrentPrcb->Number;

    /* Raise to sync level */
    KeRaiseIrql(SYNCH_LEVEL, &OldIrql);

    /* Acquire the IPI spinlock */
    KeAcquireSpinLockAtDpcLevel(&KiIpiSpinLock);

    /* Raise to IPI level, so we don't get interrupted */

    /* Loop while we have more processors */
    RemainingSet = CurrentPrcb->TargetSet;
    while (RemainingSet != 0)
    {
        NT_VERIFY(BitScanForwardAffinity(&ProcessorIndex, RemainingSet) != 0);
        ASSERT(ProcessorIndex < KeNumberProcessors);
        SetMember = AFFINITY_MASK(ProcessorIndex);
        RemainingSet &= ~SetMember;

        /* Get the target PRCB */
        TargetPrcb = KiProcessorBlock[ProcessorIndex];

        // TODO: Don't use the current processor's index, but find a free one,
        // so we can support more than 64 CPUs.

        /* Wait for the mailbox slot to be available */
        while (TargetPrcb->SenderSummary & CurrentPrcb->SetMember)
        {
            KeMemoryBarrier();
        }

        /* Set up the request packet in the request mailbox */
        TargetPrcb->RequestMailbox[SenderIndex].RequestPacket = *RequestPacket;
        TargetPrcb->RequestMailbox[SenderIndex].RequestSummary = 1;

        /* Set the sender summary bit */
        InterlockedOr64(&TargetPrcb->SenderSummary, CurrentPrcb->SetMember);
    }

    /* Request an IPI with hal for all processors, except ourselves */
    HalRequestIpi(TargetSet & ~CurrentPrcb->SetMember);

    /* Run on the current processor, if requested */
    if (TargetSet & CurrentPrcb->SetMember)
    {
        PKIPI_WORKER WorkerRoutine = RequestPacket->WorkerRoutine;

        WorkerRoutine(NULL,
                      RequestPacket->CurrentPacket[0],
                      RequestPacket->CurrentPacket[1],
                      RequestPacket->CurrentPacket[2]);
    }

    /* Wait for acknowledgement */
    while (CurrentPrcb->TargetSet != 0)
    {
        KeMemoryBarrier();
    }

    /* Lower to IRQL */
    KeReleaseSpinLockFromDpcLevel(&KiIpiSpinLock);
    KeLowerIrql(OldIrql);
}

VOID
NTAPI
KiIpiSendRequest(
    _In_ KAFFINITY TargetSet,
    _In_ PKIPI_WORKER WorkerRoutine,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2,
    _In_ PVOID Parameter3)
{
    KREQUEST_PACKET RequestPacket;

    RequestPacket.WorkerRoutine = WorkerRoutine;
    RequestPacket.CurrentPacket[0] = Parameter1;
    RequestPacket.CurrentPacket[1] = Parameter2;
    RequestPacket.CurrentPacket[2] = Parameter3;
    KiIpiSendRequestPacket(TargetSet, &RequestPacket);
}

VOID
FASTCALL
KiIpiSend(
    _In_ KAFFINITY TargetSet,
    _In_ ULONG IpiRequest)
{
    /* Check if we can send the IPI directly */
    if (IpiRequest == IPI_APC)
    {
        HalSendSoftwareInterrupt(TargetSet, APC_LEVEL);
    }
    else if (IpiRequest == IPI_DPC)
    {
        HalSendSoftwareInterrupt(TargetSet, DISPATCH_LEVEL);
    }
    else if (IpiRequest == IPI_FREEZE)
    {
        /* On x64 the freeze IPI is an NMI */
        HalSendNMI(TargetSet);
    }
    else
    {
        __debugbreak();
    }
}

ULONG_PTR
NTAPI
KeIpiGenericCall(
    _In_ PKIPI_BROADCAST_WORKER BroadcastFunction,
    _In_ ULONG_PTR Argument)
{
    KREQUEST_PACKET RequestPacket;
    KAFFINITY TargetSet = KeActiveProcessors & ~KeGetCurrentPrcb()->SetMember;

    __debugbreak();
    RequestPacket.WorkerRoutine = KiIpiGenericCallWorker;
    RequestPacket.CurrentPacket[0] = BroadcastFunction;
    RequestPacket.CurrentPacket[1] = (PVOID)Argument;
    RequestPacket.CurrentPacket[2] = 0;
    KiIpiSendRequestPacket(TargetSet, &RequestPacket);

    return 0;
}
