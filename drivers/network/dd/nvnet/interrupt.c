/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Interrupt handling
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "nvnet.h"

#define NDEBUG
#include "debug.h"

/* FUNCTIONS ******************************************************************/

ULONG
ProcessTransmitDescriptorsLegacy(
    _In_ PNVNET_ADAPTER Adapter,
    _Inout_ PLIST_ENTRY SendReadyList)
{
    PNVNET_TCB Tcb = Adapter->Send.LastTcb;
    ULONG TcbProcessed = 0;

    while (Tcb != Adapter->Send.CurrentTcb)
    {
        NVNET_TBD Tbd = Tcb->Tbd;
        ULONG Flags = Tbd.x32->FlagsLength;

        if (Flags & NV_TX_VALID)
            break;

        NDIS_DbgPrint(MIN_TRACE, ("Packet transmitted (flags %lx)\n",
                                  Flags & FLAG_MASK_V1));

        if (Flags & NV_TX_ERROR)
        {
            ++Adapter->Statistics.TransmitErrors;

            if (Flags & NV_TX_UNDERFLOW)
                ++Adapter->Statistics.TransmitUnderrunErrors;
            if (Flags & NV_TX_LATECOLLISION)
                ++Adapter->Statistics.TransmitLateCollisions;
            if (Flags & NV_TX_CARRIERLOST)
                ++Adapter->Statistics.TransmitLostCarrierSense;
            if (Flags & NV_TX_RETRYERROR)
            {
                ++Adapter->Statistics.TransmitExcessiveCollisions;

                if (!(Flags & NV_TX_RETRYCOUNT_MASK))
                {
                    NvNetBackoffReseed(Adapter);
                }
            }
        }
        else
        {
            ++Adapter->Statistics.TransmitOk;

            if (Flags & NV_TX_DEFERRED)
            {
                ++Adapter->Statistics.TransmitDeferred;
            }
            if (!(Flags & NV_TX_RETRYCOUNT_MASK))
                ++Adapter->Statistics.TransmitZeroRetry;
            else if ((Flags & NV_TX_RETRYCOUNT_MASK) == NV_TX_ONE_RETRY)
                ++Adapter->Statistics.TransmitOneRetry;
        }

        InsertTailList(SendReadyList, PACKET_ENTRY(Tcb->Packet));

        NV_RELEASE_TCB(Adapter, Tcb);

        ++TcbProcessed;

        Tcb = NV_NEXT_TCB(Adapter, Tcb);
    }

    Adapter->Send.LastTcb = Tcb;

    return TcbProcessed;
}

ULONG
ProcessTransmitDescriptors32(
    _In_ PNVNET_ADAPTER Adapter,
    _Inout_ PLIST_ENTRY SendReadyList)
{
    PNVNET_TCB Tcb = Adapter->Send.LastTcb;
    ULONG TcbProcessed = 0;

    while (Tcb != Adapter->Send.CurrentTcb)
    {
        NVNET_TBD Tbd = Tcb->Tbd;
        ULONG Flags = Tbd.x32->FlagsLength;

        if (Flags & NV_TX_VALID)
            break;

        NDIS_DbgPrint(MIN_TRACE, ("Packet transmitted (flags %lx)\n",
                                  Flags & FLAG_MASK_V2));

        if (Flags & NV_TX2_ERROR)
        {
            if ((Flags & NV_TX2_RETRYERROR) && !(Flags & NV_TX2_RETRYCOUNT_MASK))
            {
                if (Adapter->Features & DEV_HAS_GEAR_MODE)
                    NvNetBackoffReseedEx(Adapter);
                else
                    NvNetBackoffReseed(Adapter);
            }
        }

        InsertTailList(SendReadyList, PACKET_ENTRY(Tcb->Packet));

        NV_RELEASE_TCB(Adapter, Tcb);

        ++TcbProcessed;

        Tcb = NV_NEXT_TCB(Adapter, Tcb);
    }

    Adapter->Send.LastTcb = Tcb;

    return TcbProcessed;
}

ULONG
ProcessTransmitDescriptors64(
    _In_ PNVNET_ADAPTER Adapter,
    _Inout_ PLIST_ENTRY SendReadyList)
{
    PNVNET_TCB Tcb = Adapter->Send.LastTcb;
    ULONG TcbProcessed = 0;

    while (Tcb != Adapter->Send.CurrentTcb)
    {
        NVNET_TBD Tbd = Tcb->Tbd;
        ULONG Flags = Tbd.x64->FlagsLength;

        if (Flags & NV_TX_VALID)
            break;

        if (Adapter->Flags & NV_SEND_ERRATA_PRESENT)
        {
            PNVNET_TCB DeferredTcb;

            --Adapter->Send.PacketsCount;

            DeferredTcb = Adapter->Send.DeferredTcb;

            if (DeferredTcb)
            {
                DeferredTcb->DeferredTbd.x64->FlagsLength |= NV_TX2_VALID;

                ++Adapter->Send.PacketsCount;

                Adapter->Send.DeferredTcb = NV_NEXT_TCB(Adapter, DeferredTcb);
                if (Adapter->Send.DeferredTcb == Adapter->Send.CurrentTcb)
                {
                    Adapter->Send.DeferredTcb = NULL;
                }

                NV_WRITE(Adapter, NvRegTxRxControl, Adapter->TxRxControl | NVREG_TXRXCTL_KICK);
            }
        }

        NDIS_DbgPrint(MIN_TRACE, ("Packet transmitted (flags %lx)\n",
                                  Flags & FLAG_MASK_V2));

        if (Flags & NV_TX2_ERROR)
        {
            if ((Flags & NV_TX2_RETRYERROR) && !(Flags & NV_TX2_RETRYCOUNT_MASK))
            {
                if (Adapter->Features & DEV_HAS_GEAR_MODE)
                    NvNetBackoffReseedEx(Adapter);
                else
                    NvNetBackoffReseed(Adapter);
            }
        }

        InsertTailList(SendReadyList, PACKET_ENTRY(Tcb->Packet));

        NV_RELEASE_TCB(Adapter, Tcb);

        ++TcbProcessed;

        Tcb = NV_NEXT_TCB(Adapter, Tcb);
    }

    Adapter->Send.LastTcb = Tcb;

    return TcbProcessed;
}

static
BOOLEAN
HandleLengthError(
    _In_ PVOID EthHeader,
    _Inout_ PUSHORT Length)
{
    PUCHAR Buffer = EthHeader;
    ULONG i;

    DBG_UNREFERENCED_LOCAL_VARIABLE(Buffer);

    /* TODO */
    NDIS_DbgPrint(MAX_TRACE, ("() Length error detected (%u): \n", *Length));
    for (i = 0; i < *Length; ++i)
    {
        NDIS_DbgPrint(MAX_TRACE, ("%02x ", Buffer[i]));
    }
    NDIS_DbgPrint(MAX_TRACE, ("\n\n*** Please report it to the team! ***\n\n"));

    return FALSE;
}

/* TODO: This need to be rewritten. I leave it as-is for now */
static
ULONG
ProcessReceiveDescriptors(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG TotalRxProcessed)
{
    ULONG i, RxProcessed = 0;
    BOOLEAN IndicateComplete = FALSE;

    for (i = 0; i < NVNET_RECEIVE_DESCRIPTORS; ++i)
    {
        ULONG Flags;
        USHORT Length;
        PUCHAR EthHeader;
        NV_RBD NvRbd;

        if (Adapter->Features & DEV_HAS_HIGH_DMA)
        {
            NvRbd.x64 = &Adapter->Receive.NvRbd.x64[Adapter->CurrentRx];
            Flags = NvRbd.x64->FlagsLength;
        }
        else
        {
            NvRbd.x32 = &Adapter->Receive.NvRbd.x32[Adapter->CurrentRx];
            Flags = NvRbd.x32->FlagsLength;
        }

        if (Flags & NV_RX_AVAIL)
            break;

        if (TotalRxProcessed + RxProcessed >= NVNET_RECEIVE_PROCESSING_LIMIT)
            break;

        if (!Adapter->PacketFilter)
            goto NextDescriptor;

        if (Adapter->Features & (DEV_HAS_HIGH_DMA | DEV_HAS_LARGEDESC))
        {
            if (!(Flags & NV_RX2_DESCRIPTORVALID))
                goto NextDescriptor;

            Length = Flags & LEN_MASK_V2;
            EthHeader = &Adapter->ReceiveBuffer[Adapter->CurrentRx * NVNET_RECEIVE_BUFFER_SIZE];

            if (Flags & NV_RX2_ERROR)
            {
                if ((Flags & NV_RX2_ERROR_MASK) == NV_RX2_ERROR4)
                {
                    if (!HandleLengthError(EthHeader, &Length))
                        goto NextDescriptor;
                }
                else if ((Flags & NV_RX2_ERROR_MASK) == NV_RX2_FRAMINGERR)
                {
                    if (Flags & NV_RX2_SUBTRACT1)
                        --Length;
                }
                else
                {
                    goto NextDescriptor;
                }
            }

            NDIS_DbgPrint(MIN_TRACE, ("Packet %d received (length %d, flags %lx)\n",
                                      Adapter->CurrentRx, Length, Flags & FLAG_MASK_V2));
        }
        else
        {
            if (!(Flags & NV_RX_DESCRIPTORVALID))
                goto NextDescriptor;

            Length = Flags & LEN_MASK_V1;
            EthHeader = &Adapter->ReceiveBuffer[Adapter->CurrentRx * NVNET_RECEIVE_BUFFER_SIZE];

            if (Flags & NV_RX_ERROR)
            {
                if ((Flags & NV_RX_ERROR_MASK) == NV_RX_ERROR4)
                {
                    if (!HandleLengthError(EthHeader, &Length))
                        goto NextDescriptor;
                }
                else if ((Flags & NV_RX_ERROR_MASK) == NV_RX_FRAMINGERR)
                {
                    if (Flags & NV_RX_SUBTRACT1)
                        --Length;
                }
                else
                {
                    ++Adapter->Statistics.ReceiveErrors;

                    if (Flags & NV_RX_MISSEDFRAME)
                        ++Adapter->Statistics.ReceiveNoBuffers;
                    if (Flags & NV_RX_FRAMINGERR)
                        ++Adapter->Statistics.ReceiveAlignmentErrors;
                    if (Flags & NV_RX_OVERFLOW)
                        ++Adapter->Statistics.ReceiveOverrunErrors;
                    if (Flags & NV_RX_CRCERR)
                        ++Adapter->Statistics.ReceiveCrcErrors;

                    goto NextDescriptor;
                }
            }
            ++Adapter->Statistics.ReceiveOk;

            NDIS_DbgPrint(MIN_TRACE, ("Packet %d received (length %d, flags %lx)\n",
                                      Adapter->CurrentRx, Length, Flags & FLAG_MASK_V1));
        }

        NdisMEthIndicateReceive(Adapter->AdapterHandle,
                                NULL,
                                (PCHAR)EthHeader,
                                sizeof(ETH_HEADER),
                                EthHeader + sizeof(ETH_HEADER),
                                Length - sizeof(ETH_HEADER),
                                Length - sizeof(ETH_HEADER));
        IndicateComplete = TRUE;

NextDescriptor:
        /* Invalidate the buffer length and release the descriptor */
        if (Adapter->Features & DEV_HAS_HIGH_DMA)
            NvRbd.x64->FlagsLength = NV_RX2_AVAIL | NVNET_RECEIVE_BUFFER_SIZE;
        else
            NvRbd.x32->FlagsLength = NV_RX_AVAIL | NVNET_RECEIVE_BUFFER_SIZE;

        Adapter->CurrentRx = (Adapter->CurrentRx + 1) % NVNET_RECEIVE_DESCRIPTORS;
        ++RxProcessed;
    }

    if (IndicateComplete)
    {
        NdisMEthIndicateReceiveComplete(Adapter->AdapterHandle);
    }

    return RxProcessed;
}

static
inline
VOID
ChangeInterruptMode(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG Workload)
{
    if (Workload > NVNET_IM_THRESHOLD)
    {
        Adapter->InterruptIdleCount = 0;

        /* High activity, polling based strategy */
        Adapter->InterruptMask = NVREG_IRQMASK_CPU;
    }
    else
    {
        if (Adapter->InterruptIdleCount < NVNET_IM_MAX_IDLE)
        {
            ++Adapter->InterruptIdleCount;
        }
        else
        {
            /* Low activity, 1 interrupt per packet */
            Adapter->InterruptMask = NVREG_IRQMASK_THROUGHPUT;
        }
    }
}

static
VOID
HandleLinkStateChange(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG MiiStatus;
    BOOLEAN Connected, Report = FALSE;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    NdisDprAcquireSpinLock(&Adapter->Lock);

    MiiStatus = NV_READ(Adapter, NvRegMIIStatus);

    /* Clear the link change interrupt */
    NV_WRITE(Adapter, NvRegMIIStatus, NVREG_MIISTAT_LINKCHANGE);

    if (MiiStatus & NVREG_MIISTAT_LINKCHANGE)
    {
        Connected = NvNetUpdateLinkSpeed(Adapter);
        if (Adapter->Connected != Connected)
        {
            Adapter->Connected = Connected;
            Report = TRUE;

            if (Connected)
            {
                /* Link up */
                NvNetToggleClockPowerGating(Adapter, FALSE);
                NdisDprAcquireSpinLock(&Adapter->Receive.Lock);
                NvNetStartReceiver(Adapter);
            }
            else
            {
                /* Link down */
                NvNetToggleClockPowerGating(Adapter, TRUE);
                NdisDprAcquireSpinLock(&Adapter->Receive.Lock);
                NvNetStopReceiver(Adapter);
            }

            NdisDprReleaseSpinLock(&Adapter->Receive.Lock);
        }
    }

    NdisDprReleaseSpinLock(&Adapter->Lock);

    if (Report)
    {
        NdisMIndicateStatus(Adapter->AdapterHandle,
                            Connected ? NDIS_STATUS_MEDIA_CONNECT : NDIS_STATUS_MEDIA_DISCONNECT,
                            NULL,
                            0);
        NdisMIndicateStatusComplete(Adapter->AdapterHandle);
    }
}

static
VOID
HandleRecoverableError(
    _In_ PNVNET_ADAPTER Adapter)
{
    /* TODO */
    NDIS_DbgPrint(MAX_TRACE, ("() Recoverable error detected\n"));
}

VOID
NTAPI
MiniportHandleInterrupt(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;
    ULONG InterruptStatus = Adapter->InterruptStatus;
    ULONG RxProcessed, TotalTxProcessed = 0, TotalRxProcessed = 0;
    LIST_ENTRY SendReadyList;

    NDIS_DbgPrint(MIN_TRACE, ("() Events 0x%lx\n", InterruptStatus));

    if (!(Adapter->Flags & NV_ACTIVE))
        return;

    InitializeListHead(&SendReadyList);

    /* Process the rings and measure network activity */
    while (TotalRxProcessed < NVNET_RECEIVE_PROCESSING_LIMIT)
    {
        NdisDprAcquireSpinLock(&Adapter->Send.Lock);

        TotalTxProcessed += Adapter->ProcessTransmit(Adapter, &SendReadyList);

        NdisDprReleaseSpinLock(&Adapter->Send.Lock);

        while (!IsListEmpty(&SendReadyList))
        {
            PLIST_ENTRY Entry = RemoveHeadList(&SendReadyList);

            NdisMSendComplete(Adapter->AdapterHandle,
                              CONTAINING_RECORD(Entry, NDIS_PACKET, MiniportReserved),
                              NDIS_STATUS_SUCCESS);
        }

        RxProcessed = ProcessReceiveDescriptors(Adapter, TotalRxProcessed);
        if (!RxProcessed)
            break;

        TotalRxProcessed += RxProcessed;
    }

    NDIS_DbgPrint(MIN_TRACE, ("Total TX: %d, RX: %d\n", TotalTxProcessed, TotalRxProcessed));

    /* Moderate the interrupts */
    if (Adapter->OptimizationMode == NV_OPTIMIZATION_MODE_DYNAMIC)
    {
        ChangeInterruptMode(Adapter, TotalTxProcessed + TotalRxProcessed);
    }

    if (InterruptStatus & NVREG_IRQ_RX_NOBUF)
    {
        ++Adapter->Statistics.ReceiveIrqNoBuffers;
    }

    if (InterruptStatus & NVREG_IRQ_LINK)
    {
        HandleLinkStateChange(Adapter);
    }

    if (InterruptStatus & NVREG_IRQ_RECOVER_ERROR)
    {
        HandleRecoverableError(Adapter);
    }

    /* Enable interrupts on the NIC */
    NvNetApplyInterruptMask(Adapter);
}

VOID
NTAPI
MiniportISR(
    _Out_ PBOOLEAN InterruptRecognized,
    _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;
    ULONG InterruptStatus;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    InterruptStatus = NV_READ(Adapter, NvRegIrqStatus);

    /* Clear any interrupt events */
    NV_WRITE(Adapter, NvRegIrqStatus, InterruptStatus);

    if (InterruptStatus & Adapter->InterruptMask)
    {
        /* Disable further interrupts */
        NvNetDisableInterrupts(Adapter);

        Adapter->InterruptStatus = InterruptStatus;

        *InterruptRecognized = TRUE;
        *QueueMiniportHandleInterrupt = TRUE;
    }
    else
    {
        /* This interrupt is not ours */
        *InterruptRecognized = FALSE;
        *QueueMiniportHandleInterrupt = FALSE;
    }
}
