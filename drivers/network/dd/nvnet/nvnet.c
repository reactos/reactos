/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport driver entrypoint
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "nvnet.h"

#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

/* FUNCTIONS ******************************************************************/

CODE_SEG("PAGE")
VOID
NvNetFlushTransmitQueue(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NDIS_STATUS CompleteStatus)
{
    PNVNET_TCB Tcb;

    PAGED_CODE();

    for (Tcb = Adapter->Send.LastTcb;
         Tcb != Adapter->Send.CurrentTcb;
         Tcb = NV_NEXT_TCB(Adapter, Tcb))
    {
        NdisMSendComplete(Adapter->AdapterHandle,
                          Tcb->Packet,
                          CompleteStatus);

        NV_RELEASE_TCB(Adapter, Tcb);
    }
}

DECLSPEC_NOINLINE /* Called from pageable code */
VOID
NvNetPauseProcessing(
    _In_ PNVNET_ADAPTER Adapter)
{
    NvNetDisableInterrupts(Adapter);

    NdisAcquireSpinLock(&Adapter->Send.Lock);
    Adapter->Flags &= ~NV_ACTIVE;
    NdisReleaseSpinLock(&Adapter->Send.Lock);
}

CODE_SEG("PAGE")
VOID
NvNetStopAdapter(
    _In_ PNVNET_ADAPTER Adapter)
{
    BOOLEAN Dummy;

    PAGED_CODE();

    NdisMCancelTimer(&Adapter->MediaDetectionTimer, &Dummy);

    NvNetDisableInterrupts(Adapter);

    KeFlushQueuedDpcs();
}

CODE_SEG("PAGE")
VOID
NvNetStartAdapter(
    _In_ PNVNET_ADAPTER Adapter)
{
    PAGED_CODE();

    NdisMSynchronizeWithInterrupt(&Adapter->Interrupt,
                                  NvNetInitPhaseSynchronized,
                                  Adapter);

    /* Setup the link timer right after the NIC is initialized */
    if (Adapter->Features & DEV_NEED_LINKTIMER)
    {
        NdisMSetPeriodicTimer(&Adapter->MediaDetectionTimer,
                              NVNET_MEDIA_DETECTION_INTERVAL);
    }
}

static
CODE_SEG("PAGE")
VOID
NTAPI
NvNetResetWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PNVNET_ADAPTER Adapter = Context;

    PAGED_CODE();

    NvNetStopAdapter(Adapter);

    NvNetIdleTransmitter(Adapter, TRUE);
    NvNetStopTransmitter(Adapter);
    NvNetStopReceiver(Adapter);
    NV_WRITE(Adapter, NvRegTxRxControl,
             Adapter->TxRxControl | NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET);
    NdisStallExecution(NV_TXRX_RESET_DELAY);

    NvNetFlushTransmitQueue(Adapter, NDIS_STATUS_FAILURE);

    NT_VERIFY(NvNetInitNIC(Adapter, FALSE) == NDIS_STATUS_SUCCESS);

    NvNetStartAdapter(Adapter);

    _InterlockedDecrement(&Adapter->ResetLock);

    NdisMResetComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS, TRUE);
}

/* NDIS CALLBACKS *************************************************************/

static
BOOLEAN
NTAPI
MiniportCheckForHang(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;

    NdisDprAcquireSpinLock(&Adapter->Send.Lock);

    if (Adapter->Flags & NV_ACTIVE &&
        Adapter->Send.TcbSlots < NVNET_TRANSMIT_BLOCKS)
    {
        if (++Adapter->Send.StuckCount > NVNET_TRANSMIT_HANG_THRESHOLD)
        {
            NDIS_DbgPrint(MAX_TRACE, ("Transmit timeout!\n"));

#if defined(SARCH_XBOX)
            /* Apply a HACK to make XQEMU happy... */
            NvNetDisableInterrupts(Adapter);
            NvNetApplyInterruptMask(Adapter);
#endif

            Adapter->Send.StuckCount = 0;
        }
    }

    NdisDprReleaseSpinLock(&Adapter->Send.Lock);

    return FALSE;
}

static
CODE_SEG("PAGE")
VOID
NTAPI
MiniportHalt(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;
    BOOLEAN IsActive = !!(Adapter->Flags & NV_ACTIVE);

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    PAGED_CODE();

    if (IsActive)
    {
        NvNetPauseProcessing(Adapter);
    }

    NvNetStopAdapter(Adapter);

    if (IsActive)
    {
        NvNetIdleTransmitter(Adapter, TRUE);
        NvNetStopTransmitter(Adapter);
        NvNetStopReceiver(Adapter);
        NV_WRITE(Adapter, NvRegTxRxControl,
                 Adapter->TxRxControl | NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET);
        NdisStallExecution(NV_TXRX_RESET_DELAY);
    }

    NvNetFlushTransmitQueue(Adapter, NDIS_STATUS_FAILURE);

    NvNetToggleClockPowerGating(Adapter, TRUE);

    NV_WRITE(Adapter, NvRegMacAddrA, Adapter->OriginalMacAddress[0]);
    NV_WRITE(Adapter, NvRegMacAddrB, Adapter->OriginalMacAddress[1]);
    NV_WRITE(Adapter, NvRegTransmitPoll,
             NV_READ(Adapter, NvRegTransmitPoll) & ~NVREG_TRANSMITPOLL_MAC_ADDR_REV);

    SidebandUnitReleaseSemaphore(Adapter);

    NvNetFreeAdapter(Adapter);
}

static
NDIS_STATUS
NTAPI
MiniportReset(
    _Out_ PBOOLEAN AddressingReset,
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (_InterlockedCompareExchange(&Adapter->ResetLock, 1, 0))
    {
        return NDIS_STATUS_RESET_IN_PROGRESS;
    }

    NvNetPauseProcessing(Adapter);

    NdisInitializeWorkItem(&Adapter->ResetWorkItem, NvNetResetWorker, Adapter);
    NdisScheduleWorkItem(&Adapter->ResetWorkItem);

    return NDIS_STATUS_PENDING;
}

static
VOID
NTAPI
MiniportShutdown(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (Adapter->Flags & NV_ACTIVE)
    {
        if (KeGetCurrentIrql() <= DISPATCH_LEVEL)
        {
            NvNetPauseProcessing(Adapter);
        }

        NvNetIdleTransmitter(Adapter, TRUE);
        NvNetStopTransmitter(Adapter);
        NvNetStopReceiver(Adapter);
        NV_WRITE(Adapter, NvRegTxRxControl,
                 Adapter->TxRxControl | NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET);
        NdisStallExecution(NV_TXRX_RESET_DELAY);

        SidebandUnitReleaseSemaphore(Adapter);
    }

    NvNetDisableInterrupts(Adapter);

    NvNetSetPowerState(Adapter, NdisDeviceStateD3, 0);
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    NDIS_HANDLE WrapperHandle;
    NDIS_MINIPORT_CHARACTERISTICS Characteristics = { 0 };
    NDIS_STATUS Status;

    NdisMInitializeWrapper(&WrapperHandle, DriverObject, RegistryPath, NULL);
    if (!WrapperHandle)
    {
        return NDIS_STATUS_FAILURE;
    }

    Characteristics.MajorNdisVersion = NDIS_MINIPORT_MAJOR_VERSION;
    Characteristics.MinorNdisVersion = NDIS_MINIPORT_MINOR_VERSION;
    Characteristics.CheckForHangHandler = MiniportCheckForHang;
    Characteristics.HaltHandler = MiniportHalt;
    Characteristics.HandleInterruptHandler = MiniportHandleInterrupt;
    Characteristics.InitializeHandler = MiniportInitialize;
    Characteristics.ISRHandler = MiniportISR;
    Characteristics.QueryInformationHandler = MiniportQueryInformation;
    Characteristics.ResetHandler = MiniportReset;
    Characteristics.SendHandler = MiniportSend; /* TODO */
    Characteristics.SetInformationHandler = MiniportSetInformation;
    // Characteristics.ReturnPacketHandler = MiniportReturnPacket; /* TODO */
    // Characteristics.SendPacketsHandler = MiniportSendPackets; /* TODO */
    Characteristics.AdapterShutdownHandler = MiniportShutdown;

    Status = NdisMRegisterMiniport(WrapperHandle, &Characteristics, sizeof(Characteristics));
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NdisTerminateWrapper(WrapperHandle, NULL);
        return Status;
    }

    return Status;
}
