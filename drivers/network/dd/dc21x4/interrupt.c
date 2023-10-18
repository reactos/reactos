/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Interrupt handling
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
DcAdjustTxFifoThreshold(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG OpMode;

    TRACE("TX underrun\n");

    /* Maximum threshold reached */
    if ((Adapter->OpMode & DC_OPMODE_STORE_AND_FORWARD) ||
        (++Adapter->TransmitUnderruns < DC_TX_UNDERRUN_LIMIT))
    {
        NdisDprAcquireSpinLock(&Adapter->SendLock);

        /* Start the transmit process if it was suspended */
        DC_WRITE(Adapter, DcCsr1_TxPoll, DC_TX_POLL_DOORBELL);

        NdisDprReleaseSpinLock(&Adapter->SendLock);
        return;
    }
    Adapter->TransmitUnderruns = 0;

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    OpMode = Adapter->OpMode;

    /* Update the FIFO threshold level to minimize Tx FIFO underrun */
    if ((OpMode & DC_OPMODE_TX_THRESHOLD_CTRL_MASK) != DC_OPMODE_TX_THRESHOLD_MAX)
    {
        OpMode += DC_OPMODE_TX_THRESHOLD_LEVEL;

        INFO("New OP Mode %08lx\n", OpMode);
    }
    else
    {
        OpMode |= DC_OPMODE_STORE_AND_FORWARD;

        INFO("Store & Forward\n");
    }

    DcStopTxRxProcess(Adapter);

    Adapter->OpMode = OpMode;

    /* Restart the transmit process */
    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);

    NdisDprReleaseSpinLock(&Adapter->ModeLock);
}

static
VOID
DcHandleTxJabberTimeout(
    _In_ PDC21X4_ADAPTER Adapter)
{
    WARN("Transmit jabber timer timed out\n");

    NdisWriteErrorLogEntry(Adapter->AdapterHandle, NDIS_ERROR_CODE_HARDWARE_FAILURE, 1, __LINE__);

    NdisDprAcquireSpinLock(&Adapter->ModeLock);

    /* Start the transmit process if it was stopped */
    DC_WRITE(Adapter, DcCsr6_OpMode, Adapter->OpMode);

    NdisDprReleaseSpinLock(&Adapter->ModeLock);
}

static
VOID
DcHandleTxCompletedFrames(
    _In_ PDC21X4_ADAPTER Adapter,
    _Inout_ PLIST_ENTRY SendReadyList,
    _Out_ PULONG DpcEvents)
{
    PDC_TCB Tcb;
    PDC_TBD Tbd;
    ULONG TbdStatus, Collisions;

    for (Tcb = Adapter->LastTcb;
         Tcb != Adapter->CurrentTcb;
         Tcb = DC_NEXT_TCB(Adapter, Tcb))
    {
        Tbd = Tcb->Tbd;
        TbdStatus = Tbd->Status;

        if (TbdStatus & DC_TBD_STATUS_OWNED)
            break;

        ++Adapter->TcbCompleted;

        /* Complete the packet filter change request asynchronously */
        if (Tbd->Control & DC_TBD_CONTROL_SETUP_FRAME)
        {
            Tbd->Control &= ~DC_TBD_CONTROL_SETUP_FRAME;

            if (Tbd->Control & DC_TBD_CONTROL_REQUEST_INTERRUPT)
            {
                *DpcEvents |= DC_EVENT_SETUP_FRAME_COMPLETED;
            }

            continue;
        }

        /* This is our media test packet, so no need to update the TX statistics */
        if (!Tcb->Packet)
        {
            _InterlockedExchange(&Adapter->MediaTestStatus,
                                 !(TbdStatus & DC_TBD_STATUS_ERROR_SUMMARY));

            ASSERT(Adapter->LoopbackFrameSlots < DC_LOOPBACK_FRAMES);

            ++Adapter->LoopbackFrameSlots;

            continue;
        }

        if (TbdStatus & DC_TBD_STATUS_ERROR_SUMMARY)
        {
            ++Adapter->Statistics.TransmitErrors;

            if (TbdStatus & DC_TBD_STATUS_UNDERFLOW)
                ++Adapter->Statistics.TransmitUnderrunErrors;
            else if (TbdStatus & DC_TBD_STATUS_LATE_COLLISION)
                ++Adapter->Statistics.TransmitLateCollisions;

            if (TbdStatus & DC_TBD_STATUS_RETRY_ERROR)
                ++Adapter->Statistics.TransmitExcessiveCollisions;
            if (TbdStatus & DC_TBD_STATUS_CARRIER_LOST)
                ++Adapter->Statistics.TransmitLostCarrierSense;
        }
        else
        {
            ++Adapter->Statistics.TransmitOk;

            if (TbdStatus & DC_TBD_STATUS_DEFFERED)
                ++Adapter->Statistics.TransmitDeferred;
            if (TbdStatus & DC_TBD_STATUS_HEARTBEAT_FAIL)
                ++Adapter->Statistics.TransmitHeartbeatErrors;

            Collisions = (TbdStatus & DC_TBD_STATUS_COLLISIONS_MASK) >>
                         DC_TBD_STATUS_COLLISIONS_SHIFT;
            if (Collisions == 1)
                ++Adapter->Statistics.TransmitOneRetry;
            else if (Collisions > 1)
                ++Adapter->Statistics.TransmitMoreCollisions;
        }

        InsertTailList(SendReadyList, DC_LIST_ENTRY_FROM_PACKET(Tcb->Packet));

        DC_RELEASE_TCB(Adapter, Tcb);
    }

    Adapter->LastTcb = Tcb;
}

static
VOID
DcHandleTx(
    _In_ PDC21X4_ADAPTER Adapter)
{
    LIST_ENTRY SendReadyList;
    ULONG DpcEvents;

    TRACE("Handle TX\n");

    InitializeListHead(&SendReadyList);
    DpcEvents = 0;

    NdisDprAcquireSpinLock(&Adapter->SendLock);

    DcHandleTxCompletedFrames(Adapter, &SendReadyList, &DpcEvents);

    if (!IsListEmpty(&Adapter->SendQueueList))
    {
        DcProcessPendingPackets(Adapter);
    }

    NdisDprReleaseSpinLock(&Adapter->SendLock);

    while (!IsListEmpty(&SendReadyList))
    {
        PLIST_ENTRY Entry = RemoveHeadList(&SendReadyList);

        TRACE("Complete TX packet %p\n", DC_PACKET_FROM_LIST_ENTRY(Entry));

        NdisMSendComplete(Adapter->AdapterHandle,
                          DC_PACKET_FROM_LIST_ENTRY(Entry),
                          NDIS_STATUS_SUCCESS);
    }

    /* We have to complete the OID request outside of the spinlock */
    if (DpcEvents & DC_EVENT_SETUP_FRAME_COMPLETED)
    {
        TRACE("SP completed\n");

        Adapter->OidPending = FALSE;

        NdisMSetInformationComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS);
    }
}

static
VOID
DcStopRxProcess(
    _In_ PDC21X4_ADAPTER Adapter)
{
    ULONG i, OpMode, Status;

    OpMode = Adapter->OpMode;
    OpMode &= ~DC_OPMODE_RX_ENABLE;
    DC_WRITE(Adapter, DcCsr6_OpMode, OpMode);

    for (i = 0; i < 5000; ++i)
    {
        Status = DC_READ(Adapter, DcCsr5_Status);

        if ((Status & DC_STATUS_RX_STATE_MASK) == DC_STATUS_RX_STATE_STOPPED)
            return;

        NdisStallExecution(10);
    }

    WARN("Failed to stop the RX process 0x%08lx\n", Status);
}

VOID
NTAPI
DcReturnPacket(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;
    PDC_RCB Rcb;

    Rcb = *DC_RCB_FROM_PACKET(Packet);

    NdisAcquireSpinLock(&Adapter->ReceiveLock);

    PushEntryList(&Adapter->FreeRcbList, &Rcb->ListEntry);

    ++Adapter->RcbFree;

    NdisReleaseSpinLock(&Adapter->ReceiveLock);
}

static
VOID
DcIndicateReceivePackets(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PNDIS_PACKET* ReceiveArray,
    _In_ ULONG PacketsToIndicate)
{
    PNDIS_PACKET Packet;
    PDC_RBD Rbd;
    PDC_RCB Rcb;
    ULONG i;

    NdisDprReleaseSpinLock(&Adapter->ReceiveLock);

    NdisMIndicateReceivePacket(Adapter->AdapterHandle,
                               ReceiveArray,
                               PacketsToIndicate);

    NdisDprAcquireSpinLock(&Adapter->ReceiveLock);

    for (i = 0; i < PacketsToIndicate; ++i)
    {
        Packet = ReceiveArray[i];
        Rcb = *DC_RCB_FROM_PACKET(Packet);

        /* Reuse the RCB immediately */
        if (Rcb->Flags & DC_RCB_FLAG_RECLAIM)
        {
            Rbd = *DC_RBD_FROM_PACKET(Packet);

            Rbd->Status = DC_RBD_STATUS_OWNED;
        }
    }
}

static
VOID
DcHandleRxReceivedFrames(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PDC_RBD Rbd, StartRbd, LastRbd;
    ULONG PacketsToIndicate;
    PNDIS_PACKET ReceiveArray[DC_RECEIVE_ARRAY_SIZE];

    Rbd = StartRbd = LastRbd = Adapter->CurrentRbd;
    PacketsToIndicate = 0;

    while (PacketsToIndicate < RTL_NUMBER_OF(ReceiveArray))
    {
        PDC_RCB Rcb;
        PDC_RCB* RcbSlot;
        PNDIS_PACKET Packet;
        ULONG RbdStatus, PacketLength, RxCounters;

        if (Rbd->Status & DC_RBD_STATUS_OWNED)
            break;

        /* Work around the RX overflow bug */
        if ((Adapter->Features & DC_NEED_RX_OVERFLOW_WORKAROUND) && (Rbd == LastRbd))
        {
            /* Find the last received packet, to correctly catch invalid packets */
            do
            {
                LastRbd = DC_NEXT_RBD(Adapter, LastRbd);

                if (LastRbd->Status & DC_RBD_STATUS_OWNED)
                    break;
            }
            while (LastRbd != Rbd);

            RxCounters = DC_READ(Adapter, DcCsr8_RxCounters);

            Adapter->Statistics.ReceiveNoBuffers += RxCounters & DC_COUNTER_RX_NO_BUFFER_MASK;

            /* A receive overflow might indicate a data corruption */
            if (RxCounters & DC_COUNTER_RX_OVERFLOW_MASK)
            {
                ERR("RX overflow, dropping the packets\n");

                Adapter->Statistics.ReceiveOverrunErrors +=
                    (RxCounters & DC_COUNTER_RX_OVERFLOW_MASK) >> DC_COUNTER_RX_OVERFLOW_SHIFT;

                NdisDprAcquireSpinLock(&Adapter->ModeLock);

                /* Stop the receive process */
                DcStopRxProcess(Adapter);

                /* Drop all received packets regardless of what the status indicates */
                while (TRUE)
                {
                    if (Rbd->Status & DC_RBD_STATUS_OWNED)
                        break;

                    ++Adapter->Statistics.ReceiveOverrunErrors;

                    Rbd->Status = DC_RBD_STATUS_OWNED;

                    Rbd = DC_NEXT_RBD(Adapter, Rbd);
                }
                LastRbd = Rbd;

                /* Restart the receive process */
                DC_WRITE(Adapter, DcCsr6_OpMode, Adapter->OpMode);

                NdisDprReleaseSpinLock(&Adapter->ModeLock);

                continue;
            }
        }

        RbdStatus = Rbd->Status;

        /* Ignore oversized packets */
        if (!(RbdStatus & DC_RBD_STATUS_LAST_DESCRIPTOR))
        {
            Rbd->Status = DC_RBD_STATUS_OWNED;
            goto NextRbd;
        }

        /* Check for an invalid packet */
        if (RbdStatus & DC_RBD_STATUS_INVALID)
        {
            ++Adapter->Statistics.ReceiveErrors;

            if (RbdStatus & DC_RBD_STATUS_OVERRUN)
                ++Adapter->Statistics.ReceiveOverrunErrors;

            if (RbdStatus & DC_RBD_STATUS_CRC_ERROR)
            {
                if (RbdStatus & DC_RBD_STATUS_DRIBBLE)
                    ++Adapter->Statistics.ReceiveAlignmentErrors;
                else
                    ++Adapter->Statistics.ReceiveCrcErrors;
            }

            Rbd->Status = DC_RBD_STATUS_OWNED;
            goto NextRbd;
        }

        ++Adapter->Statistics.ReceiveOk;

        PacketLength = (RbdStatus & DC_RBD_STATUS_FRAME_LENGTH_MASK) >>
                       DC_RBD_STATUS_FRAME_LENGTH_SHIFT;

        /* Omit the CRC */
        PacketLength -= 4;

        RcbSlot = DC_GET_RCB_SLOT(Adapter, Rbd);
        Rcb = *RcbSlot;

        TRACE("RX packet (len %u), RCB %p\n", PacketLength, Rcb);

        NdisAdjustBufferLength(Rcb->NdisBuffer, PacketLength);

        /* Receive buffers are in cached memory */
        NdisFlushBuffer(Rcb->NdisBuffer, FALSE);

        if (RbdStatus & DC_RBD_STATUS_MULTICAST)
        {
            if (ETH_IS_BROADCAST(Rcb->VirtualAddress))
                ++Adapter->Statistics.ReceiveBroadcast;
            else
                ++Adapter->Statistics.ReceiveMulticast;
        }
        else
        {
            ++Adapter->Statistics.ReceiveUnicast;
        }

        Packet = Rcb->Packet;

        ReceiveArray[PacketsToIndicate++] = Packet;

        if (Adapter->FreeRcbList.Next)
        {
            Rcb->Flags = 0;
            NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_SUCCESS);

            Rcb = (PDC_RCB)DcPopEntryList(&Adapter->FreeRcbList);
            *RcbSlot = Rcb;

            ASSERT(Adapter->RcbFree > 0);
            --Adapter->RcbFree;

            Rbd->Address1 = Rcb->PhysicalAddress;
            DC_WRITE_BARRIER();
            Rbd->Status = DC_RBD_STATUS_OWNED;
        }
        else
        {
            Rcb->Flags = DC_RCB_FLAG_RECLAIM;
            NDIS_SET_PACKET_STATUS(Packet, NDIS_STATUS_RESOURCES);

            *DC_RBD_FROM_PACKET(Packet) = Rbd;
        }

NextRbd:
        Rbd = DC_NEXT_RBD(Adapter, Rbd);

        /*
         * Check the next descriptor to prevent wrap-around.
         * Since we don't use a fixed-sized ring,
         * the receive ring may be smaller in length than the ReceiveArray[].
         */
        if (Rbd == StartRbd)
            break;
    }

    Adapter->CurrentRbd = Rbd;

    /* Pass the packets up */
    if (PacketsToIndicate)
    {
        DcIndicateReceivePackets(Adapter, ReceiveArray, PacketsToIndicate);
    }
}

static
VOID
DcHandleRx(
    _In_ PDC21X4_ADAPTER Adapter)
{
    NdisDprAcquireSpinLock(&Adapter->ReceiveLock);

    do
    {
        DcHandleRxReceivedFrames(Adapter);
    }
    while (!(Adapter->CurrentRbd->Status & DC_RBD_STATUS_OWNED));

    NdisDprReleaseSpinLock(&Adapter->ReceiveLock);
}

static
VOID
DcHandleSystemError(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG InterruptStatus)
{
    ERR("%s error occured, CSR5 %08lx\n", DcDbgBusError(InterruptStatus), InterruptStatus);

    NdisWriteErrorLogEntry(Adapter->AdapterHandle, NDIS_ERROR_CODE_HARDWARE_FAILURE, 1, __LINE__);

    /* Issue a software reset, which also enables the interrupts */
    if (_InterlockedCompareExchange(&Adapter->ResetLock, 2, 0) == 0)
    {
        NdisScheduleWorkItem(&Adapter->ResetWorkItem);
    }
}

VOID
NTAPI
DcHandleInterrupt(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    ULONG InterruptStatus, IoLimit;
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;

    TRACE("Events %08lx\n", Adapter->InterruptStatus);

    if (!(Adapter->Flags & DC_ACTIVE))
        return;

    IoLimit = DC_INTERRUPT_PROCESSING_LIMIT;
    InterruptStatus = Adapter->InterruptStatus;

    /* Loop until the condition to stop is encountered */
    while (TRUE)
    {
        /* Uncommon interrupts */
        if (InterruptStatus & DC_IRQ_ABNORMAL_SUMMARY)
        {
            /* PCI bus error detected */
            if (InterruptStatus & DC_IRQ_SYSTEM_ERROR)
            {
                DcHandleSystemError(Adapter, InterruptStatus);
                return;
            }

            /* Transmit jabber timeout */
            if (InterruptStatus & DC_IRQ_TX_JABBER_TIMEOUT)
            {
                DcHandleTxJabberTimeout(Adapter);
            }

            /* Link state changed */
            if (InterruptStatus & Adapter->LinkStateChangeMask)
            {
                Adapter->HandleLinkStateChange(Adapter, InterruptStatus);
            }
        }

        /* Handling receive interrupts */
        if (InterruptStatus & (DC_IRQ_RX_OK | DC_IRQ_RX_STOPPED))
        {
            DcHandleRx(Adapter);
        }

        /* Handling transmit interrupts */
        if (InterruptStatus & (DC_IRQ_TX_OK | DC_IRQ_TX_STOPPED))
        {
            DcHandleTx(Adapter);
        }

        /* Transmit underflow error detected */
        if (InterruptStatus & DC_IRQ_TX_UNDERFLOW)
        {
            DcAdjustTxFifoThreshold(Adapter);
        }

        /* Limit in order to avoid doing too much work at DPC level */
        if (!--IoLimit)
            break;

        /* Check if new events have occurred */
        InterruptStatus = DC_READ(Adapter, DcCsr5_Status);
        if (InterruptStatus == 0xFFFFFFFF || !(InterruptStatus & Adapter->InterruptMask))
            break;

        /* Acknowledge the events */
        DC_WRITE(Adapter, DcCsr5_Status, InterruptStatus);
    }

    /* TODO: Add interrupt mitigation (CSR11) */

    /* Reenable interrupts */
    _InterlockedExchange((PLONG)&Adapter->CurrentInterruptMask, Adapter->InterruptMask);
    DC_WRITE(Adapter, DcCsr7_IrqMask, Adapter->InterruptMask);
}

VOID
NTAPI
DcIsr(
    _Out_ PBOOLEAN InterruptRecognized,
    _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;
    ULONG InterruptStatus;

    if (Adapter->CurrentInterruptMask == 0)
        goto NotOurs;

    InterruptStatus = DC_READ(Adapter, DcCsr5_Status);
    if (InterruptStatus == 0xFFFFFFFF || !(InterruptStatus & Adapter->CurrentInterruptMask))
        goto NotOurs;

    /* Disable further interrupts */
    DC_WRITE(Adapter, DcCsr7_IrqMask, 0);

    /* Clear all pending events */
    DC_WRITE(Adapter, DcCsr5_Status, InterruptStatus);

    Adapter->InterruptStatus = InterruptStatus;
    Adapter->CurrentInterruptMask = 0;

    *InterruptRecognized = TRUE;
    *QueueMiniportHandleInterrupt = TRUE;
    return;

NotOurs:
    *InterruptRecognized = FALSE;
    *QueueMiniportHandleInterrupt = FALSE;
}
