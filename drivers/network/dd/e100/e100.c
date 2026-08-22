/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Miniport driver entry
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

DECLSPEC_NOINLINE_FROM_PAGED
VOID
EeFlushTransmitQueue(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_TX_CONTEXT TxContext;
    LIST_ENTRY DoneList;
    PLIST_ENTRY Entry;

    InitializeListHead(&DoneList);

    NdisAcquireSpinLock(&Adapter->SendLock);

    /* Remove pending transmissions from the transmit ring */
    for (TxContext = Adapter->TxFirst; Adapter->TxPending > 0; TxContext = TxContext->Next)
    {
        PNDIS_PACKET Packet = TxContext->Packet;

        ASSERT(Packet);

        InsertTailList(&DoneList, E100_LIST_ENTRY_FROM_PACKET(Packet));

        E100_RELEASE_TX_CONTEXT(Adapter, TxContext, TxContext->Tcb);
    }

    Adapter->TxFirst = TxContext;

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
                          E100_PACKET_FROM_LIST_ENTRY(Entry),
                          NDIS_STATUS_FAILURE);
    }
}

static
DECLSPEC_NOINLINE_FROM_PAGED
VOID
EeWaitForReceiveIndications(
    _In_ PE100_ADAPTER Adapter)
{
    BOOLEAN RxStopped;

    ASSERT(KeGetCurrentIrql() < DISPATCH_LEVEL);

    while (TRUE)
    {
        NdisAcquireSpinLock(&Adapter->ReceiveLock);

        RxStopped = (Adapter->RxPending == 0);

        NdisReleaseSpinLock(&Adapter->ReceiveLock);

        if (RxStopped)
            break;

        NdisMSleep(10);
    }
}

CODE_SEG("PAGE")
VOID
EeStopAdapter(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    /* Attempt to disable interrupts to complete more quickly */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_INTRCNTL, FXP_SCB_INTR_DISABLE);

    /* Prevent DPCs from executing and stop accepting incoming packets */
    Adapter->AdapterActive = FALSE;

    /* Wait for any DPCs to complete */
    KeFlushQueuedDpcs();
}

CODE_SEG("PAGE")
VOID
NTAPI
EeResetWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PE100_ADAPTER Adapter = Context;
    NDIS_STATUS Status;

    UNREFERENCED_PARAMETER(WorkItem);

    PAGED_CODE();

    EeStopAdapter(Adapter);
    EeSoftReset(Adapter, TRUE);

    if (Adapter->CurrentMedia != E100_MEDIA_NONE)
    {
        Adapter->CurrentMedia = E100_MEDIA_NONE;

        NdisMIndicateStatus(Adapter->AdapterHandle,
                            NDIS_STATUS_MEDIA_DISCONNECT,
                            NULL,
                            0);
        NdisMIndicateStatusComplete(Adapter->AdapterHandle);
    }

    EeFlushTransmitQueue(Adapter);

    /* Check if the device is present */
    if (CSR_READ_8(Adapter, FXP_CSR_SCB_STATACK) == 0xFF)
    {
        ERR("Hardware is gone...\n");

        /* Remove this adapter */
        NdisMRemoveMiniport(Adapter->AdapterHandle);

        Status = NDIS_STATUS_HARD_ERRORS;
        goto Done;
    }

    Status = EeSetupAdapter(Adapter);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        ERR("Failed to setup adapter\n");
        goto Done;
    }

    /* Restore the RX filter */
    EeApplyPacketFilter(Adapter);

    EeStartAdapter(Adapter);

Done:
    _InterlockedExchange(&Adapter->ResetLock, 0);

    NdisMResetComplete(Adapter->AdapterHandle, Status, FALSE);
}

static
NDIS_STATUS
NTAPI
MiniportReset(
    _Out_ PBOOLEAN AddressingReset,
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;

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
MiniportHalt(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;

    PAGED_CODE();

    INFO("Called\n");

    EeStopAdapter(Adapter);
    EeSoftReset(Adapter, FALSE);
    EePhySetPower(Adapter, FALSE);

    EeFlushTransmitQueue(Adapter);
    EeWaitForReceiveIndications(Adapter);

    EeFreeAdapter(Adapter);
}

static
VOID
NTAPI
MiniportShutdown(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;

    INFO("Called\n");

    EeSoftReset(Adapter, FALSE);
    EePhySetPower(Adapter, FALSE);
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

    NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);
    if (!WrapperHandle)
        return NDIS_STATUS_FAILURE;

    Characteristics.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
    Characteristics.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
    Characteristics.CheckForHangHandler = MiniportCheckForHang;
    Characteristics.HaltHandler = MiniportHalt;
    Characteristics.HandleInterruptHandler = MiniportHandleInterrupt;
    Characteristics.InitializeHandler = MiniportInitialize;
    Characteristics.ISRHandler = MiniportIsr;
    Characteristics.QueryInformationHandler = MiniportQueryInformation;
    Characteristics.ResetHandler = MiniportReset;
    Characteristics.SetInformationHandler = MiniportSetInformation;
    Characteristics.ReturnPacketHandler = MiniportReturnPacket;
    Characteristics.SendPacketsHandler = MiniportSendPackets;
    Characteristics.CancelSendPacketsHandler = MiniportCancelSendPackets;
    Characteristics.AdapterShutdownHandler = MiniportShutdown;

    Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(Characteristics));
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(WrapperHandle, NULL);
        return Status;
    }

    return NDIS_STATUS_SUCCESS;
}
