/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport driver entry
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

ULONG
DcEthernetCrc(
    _In_reads_bytes_(Size) const VOID* Buffer,
    _In_ ULONG Size)
{
    ULONG i, j, Crc;
    const UCHAR* Data = Buffer;

    Crc = 0xFFFFFFFF;
    for (i = 0; i < Size; ++i)
    {
        Crc ^= Data[i];
        for (j = 8; j > 0; j--)
        {
            /* CRC-32 polynomial little-endian */
            Crc = (Crc >> 1) ^ (-(LONG)(Crc & 1) & 0xEDB88320);
        }
    }

    return Crc;
}

static
VOID
DcFlushTransmitQueue(
    _In_ PDC21X4_ADAPTER Adapter)
{
    LIST_ENTRY DoneList;
    PLIST_ENTRY Entry;
    PNDIS_PACKET Packet;
    PDC_TCB Tcb;

    InitializeListHead(&DoneList);

    NdisAcquireSpinLock(&Adapter->SendLock);

    /* Remove pending transmissions from the transmit ring */
    for (Tcb = Adapter->LastTcb;
         Tcb != Adapter->CurrentTcb;
         Tcb = DC_NEXT_TCB(Adapter, Tcb))
    {
        Packet = Tcb->Packet;

        if (!Packet)
            continue;

        InsertTailList(&DoneList, DC_LIST_ENTRY_FROM_PACKET(Packet));

        DC_RELEASE_TCB(Adapter, Tcb);
    }
    Adapter->CurrentTcb = Tcb;

    /* Remove pending transmissions from the internal queue */
    while (!IsListEmpty(&Adapter->SendQueueList))
    {
        Entry = RemoveHeadList(&Adapter->SendQueueList);

        InsertTailList(&DoneList, Entry);
    }

    NdisReleaseSpinLock(&Adapter->SendLock);

    while (!IsListEmpty(&DoneList))
    {
        Entry = RemoveHeadList(&DoneList);

        NdisMSendComplete(Adapter->AdapterHandle,
                          DC_PACKET_FROM_LIST_ENTRY(Entry),
                          NDIS_STATUS_FAILURE);
    }
}

static
VOID
DcStopReceivePath(
    _In_ PDC21X4_ADAPTER Adapter)
{
    BOOLEAN RxStopped;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

#if DBG
    NdisAcquireSpinLock(&Adapter->ReceiveLock);
    if (Adapter->RcbFree != Adapter->RcbCount)
    {
        INFO("RX packets: %u/%u\n", Adapter->RcbFree, Adapter->RcbCount);
    }
    NdisReleaseSpinLock(&Adapter->ReceiveLock);
#endif

    while (TRUE)
    {
        NdisAcquireSpinLock(&Adapter->ReceiveLock);

        RxStopped = (Adapter->RcbFree == Adapter->RcbCount);

        NdisReleaseSpinLock(&Adapter->ReceiveLock);

        if (RxStopped)
            break;

        NdisMSleep(10);
    }
}

DECLSPEC_NOINLINE /* Called from pageable code */
VOID
DcStopAdapter(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN WaitForPackets)
{
    BOOLEAN TimerCancelled;

    /* Attempt to disable interrupts to complete more quickly */
    DC_WRITE(Adapter, DcCsr7_IrqMask, 0);

    /* Prevent DPCs from executing and stop accepting incoming packets */
    NdisAcquireSpinLock(&Adapter->SendLock);
    Adapter->Flags &= ~DC_ACTIVE;
    NdisReleaseSpinLock(&Adapter->SendLock);

    NdisMCancelTimer(&Adapter->MediaMonitorTimer, &TimerCancelled);

    /* Wait for any DPCs to complete */
    KeFlushQueuedDpcs();

    /* Disable interrupts */
    DC_WRITE(Adapter, DcCsr7_IrqMask, 0);

    /* Wait for completion of TX/RX and stop the DMA engine inside the NIC */
    DcStopTxRxProcess(Adapter);
    Adapter->OpMode &= ~(DC_OPMODE_RX_ENABLE | DC_OPMODE_TX_ENABLE);

    DcFlushTransmitQueue(Adapter);

    /* Wait for the packets to be returned to the driver */
    if (WaitForPackets)
    {
        DcStopReceivePath(Adapter);
    }

    /* Make sure there is no pending OID request */
    if (Adapter->OidPending)
    {
        NdisMSetInformationComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS);

        Adapter->OidPending = FALSE;
    }
}

CODE_SEG("PAGE")
VOID
DcStartAdapter(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PAGED_CODE();

    /* Enable interrupts */
    _InterlockedExchange((PLONG)&Adapter->CurrentInterruptMask, Adapter->InterruptMask);
    DC_WRITE(Adapter, DcCsr7_IrqMask, Adapter->InterruptMask);

    Adapter->Flags |= DC_ACTIVE;

    /* Start the RX process */
    Adapter->OpMode |= DC_OPMODE_RX_ENABLE;
    DC_WRITE(Adapter, DcCsr6_OpMode, Adapter->OpMode);

    /* Start the media monitor, wait the selected media to become ready */
    NdisMSetTimer(&Adapter->MediaMonitorTimer, 2400);
}

CODE_SEG("PAGE")
VOID
NTAPI
DcResetWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PDC21X4_ADAPTER Adapter = Context;
    NDIS_STATUS Status;
    ULONG InterruptStatus;
    LONG ResetReason;

    UNREFERENCED_PARAMETER(WorkItem);

    PAGED_CODE();

    Status = NDIS_STATUS_SUCCESS;

    /* Check if the device is present */
    InterruptStatus = DC_READ(Adapter, DcCsr5_Status);
    if (InterruptStatus == 0xFFFFFFFF)
    {
        ERR("Hardware is gone...\n");

        /* Remove this adapter */
        NdisMRemoveMiniport(Adapter->AdapterHandle);

        Status = NDIS_STATUS_HARD_ERRORS;
        goto Done;
    }

    DcStopAdapter(Adapter, FALSE);

    if (Adapter->LinkUp)
    {
        Adapter->LinkUp = FALSE;

        NdisMIndicateStatus(Adapter->AdapterHandle,
                            NDIS_STATUS_MEDIA_DISCONNECT,
                            NULL,
                            0);
        NdisMIndicateStatusComplete(Adapter->AdapterHandle);
    }

    DcSetupAdapter(Adapter);

    DcStartAdapter(Adapter);

Done:
    ResetReason = _InterlockedExchange(&Adapter->ResetLock, 0);

    /* Complete the pending reset request */
    if (ResetReason == 1)
    {
        NdisMResetComplete(Adapter->AdapterHandle, Status, FALSE);
    }
}

VOID
NTAPI
DcTransmitTimeoutRecoveryWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PDC21X4_ADAPTER Adapter = Context;

    UNREFERENCED_PARAMETER(WorkItem);

    NdisAcquireSpinLock(&Adapter->ModeLock);

    DcStopTxRxProcess(Adapter);
    DC_WRITE(Adapter, DcCsr6_OpMode, Adapter->OpMode);

    NdisDprAcquireSpinLock(&Adapter->SendLock);

    DC_WRITE(Adapter, DcCsr1_TxPoll, DC_TX_POLL_DOORBELL);

    NdisDprReleaseSpinLock(&Adapter->SendLock);

    NdisReleaseSpinLock(&Adapter->ModeLock);
}

static
BOOLEAN
NTAPI
DcCheckForHang(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;
    ULONG TcbCompleted;
    BOOLEAN TxHang = FALSE;

    if (!(Adapter->Flags & DC_ACTIVE))
        return FALSE;

    NdisDprAcquireSpinLock(&Adapter->SendLock);

    if (Adapter->TcbSlots != (DC_TRANSMIT_BLOCKS - DC_TCB_RESERVE))
    {
        TcbCompleted = Adapter->TcbCompleted;
        TxHang = (TcbCompleted == Adapter->LastTcbCompleted);
        Adapter->LastTcbCompleted = TcbCompleted;
    }

    NdisDprReleaseSpinLock(&Adapter->SendLock);

    if (TxHang)
    {
        WARN("Transmit timeout, CSR12 %08lx, CSR5 %08lx\n",
             DC_READ(Adapter, DcCsr12_SiaStatus),
             DC_READ(Adapter, DcCsr5_Status));

        NdisScheduleWorkItem(&Adapter->TxRecoveryWorkItem);
    }

    return FALSE;
}

static
NDIS_STATUS
NTAPI
DcReset(
    _Out_ PBOOLEAN AddressingReset,
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;

    WARN("Called\n");

    if (_InterlockedCompareExchange(&Adapter->ResetLock, 1, 0))
    {
        return NDIS_STATUS_RESET_IN_PROGRESS;
    }

    NdisScheduleWorkItem(&Adapter->ResetWorkItem);

    return NDIS_STATUS_PENDING;
}

static
CODE_SEG("PAGE")
VOID
NTAPI
DcHalt(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;

    PAGED_CODE();

    INFO("Called\n");

    DcStopAdapter(Adapter, TRUE);

    DcDisableHw(Adapter);

    DcFreeAdapter(Adapter);
}

static
VOID
NTAPI
DcShutdown(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;

    INFO("Called\n");

    DcDisableHw(Adapter);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NDIS_HANDLE WrapperHandle;
    NDIS_STATUS Status;
    NDIS_MINIPORT_CHARACTERISTICS Characteristics = { 0 };

    INFO("Called\n");

    NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);
    if (!WrapperHandle)
        return NDIS_STATUS_FAILURE;

    Characteristics.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
    Characteristics.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
    Characteristics.CheckForHangHandler = DcCheckForHang;
    Characteristics.HaltHandler = DcHalt;
    Characteristics.HandleInterruptHandler = DcHandleInterrupt;
    Characteristics.InitializeHandler = DcInitialize;
    Characteristics.ISRHandler = DcIsr;
    Characteristics.QueryInformationHandler = DcQueryInformation;
    Characteristics.ResetHandler = DcReset;
    Characteristics.SetInformationHandler = DcSetInformation;
    Characteristics.ReturnPacketHandler = DcReturnPacket;
    Characteristics.SendPacketsHandler = DcSendPackets;
    Characteristics.CancelSendPacketsHandler = DcCancelSendPackets;
    Characteristics.AdapterShutdownHandler = DcShutdown;

    Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(Characteristics));
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(WrapperHandle, NULL);
        return Status;
    }

    InitializeListHead(&SRompAdapterList);

    return NDIS_STATUS_SUCCESS;
}
