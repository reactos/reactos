/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NIC support code
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * HW access code was taken from the Linux forcedeth driver
 * Copyright (C) 2003,4,5 Manfred Spraul
 * Copyright (C) 2004 Andrew de Quincey
 * Copyright (C) 2004 Carl-Daniel Hailfinger
 * Copyright (c) 2004,2005,2006,2007,2008,2009 NVIDIA Corporation
 */

/* INCLUDES *******************************************************************/

#include "nvnet.h"

#define NDEBUG
#include "debug.h"

/* FUNCTIONS ******************************************************************/

static
CODE_SEG("PAGE")
VOID
NvNetClearStatisticsCounters(
    _In_ PNVNET_ADAPTER Adapter)
{
    NVNET_REGISTER Counter, CounterEnd;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (Adapter->Features & DEV_HAS_STATISTICS_V2)
        CounterEnd = NvRegRxDropFrame;
    else
        CounterEnd = NvRegRxBroadcast;

    for (Counter = NvRegTxCnt; Counter <= CounterEnd; Counter += sizeof(ULONG))
    {
        NV_READ(Adapter, Counter);
    }

    if (Adapter->Features & DEV_HAS_STATISTICS_V3)
    {
        NV_READ(Adapter, NvRegTxUnicast);
        NV_READ(Adapter, NvRegTxMulticast);
        NV_READ(Adapter, NvRegTxBroadcast);
    }
}

static
CODE_SEG("PAGE")
VOID
NvNetResetMac(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG Temp[3];

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (!(Adapter->Features & DEV_HAS_POWER_CNTRL))
        return;

    NV_WRITE(Adapter, NvRegTxRxControl,
             Adapter->TxRxControl | NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET);

    /* Save registers since they will be cleared on reset */
    Temp[0] = NV_READ(Adapter, NvRegMacAddrA);
    Temp[1] = NV_READ(Adapter, NvRegMacAddrB);
    Temp[2] = NV_READ(Adapter, NvRegTransmitPoll);

    NV_WRITE(Adapter, NvRegMacReset, NVREG_MAC_RESET_ASSERT);
    NdisStallExecution(NV_MAC_RESET_DELAY);
    NV_WRITE(Adapter, NvRegMacReset, 0);
    NdisStallExecution(NV_MAC_RESET_DELAY);

    /* Restore saved registers */
    NV_WRITE(Adapter, NvRegMacAddrA, Temp[0]);
    NV_WRITE(Adapter, NvRegMacAddrB, Temp[1]);
    NV_WRITE(Adapter, NvRegTransmitPoll, Temp[2]);

    NV_WRITE(Adapter, NvRegTxRxControl,
             Adapter->TxRxControl | NVREG_TXRXCTL_BIT2);
}

VOID
NvNetResetReceiverAndTransmitter(
    _In_ PNVNET_ADAPTER Adapter)
{
    NV_WRITE(Adapter, NvRegTxRxControl,
             Adapter->TxRxControl | NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET);

    NdisStallExecution(NV_TXRX_RESET_DELAY);

    NV_WRITE(Adapter, NvRegTxRxControl,
             Adapter->TxRxControl | NVREG_TXRXCTL_BIT2);
}

VOID
NvNetStartReceiver(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG RxControl;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    RxControl = NV_READ(Adapter, NvRegReceiverControl);
    if ((NV_READ(Adapter, NvRegReceiverControl) & NVREG_RCVCTL_START) &&
        !(Adapter->Flags & NV_MAC_IN_USE))
    {
        /* Already running? Stop it */
        RxControl &= ~NVREG_RCVCTL_START;
        NV_WRITE(Adapter, NvRegReceiverControl, RxControl);
    }
    NV_WRITE(Adapter, NvRegLinkSpeed, Adapter->LinkSpeed | NVREG_LINKSPEED_FORCE);

    RxControl |= NVREG_RCVCTL_START;
    if (Adapter->Flags & NV_MAC_IN_USE)
    {
        RxControl &= ~NVREG_RCVCTL_RX_PATH_EN;
    }
    NV_WRITE(Adapter, NvRegReceiverControl, RxControl);
}

VOID
NvNetStartTransmitter(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG TxControl;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    TxControl = NV_READ(Adapter, NvRegTransmitterControl);
    TxControl |= NVREG_XMITCTL_START;
    if (Adapter->Flags & NV_MAC_IN_USE)
    {
        TxControl &= ~NVREG_XMITCTL_TX_PATH_EN;
    }
    NV_WRITE(Adapter, NvRegTransmitterControl, TxControl);
}

VOID
NvNetStopReceiver(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG RxControl, i;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    RxControl = NV_READ(Adapter, NvRegReceiverControl);
    if (!(Adapter->Flags & NV_MAC_IN_USE))
        RxControl &= ~NVREG_RCVCTL_START;
    else
        RxControl |= NVREG_RCVCTL_RX_PATH_EN;
    NV_WRITE(Adapter, NvRegReceiverControl, RxControl);

    for (i = 0; i < NV_RXSTOP_DELAY1MAX; ++i)
    {
        if (!(NV_READ(Adapter, NvRegReceiverStatus) & NVREG_RCVSTAT_BUSY))
            break;

        NdisStallExecution(NV_RXSTOP_DELAY1);
    }

    NdisStallExecution(NV_RXSTOP_DELAY2);

    if (!(Adapter->Flags & NV_MAC_IN_USE))
    {
        NV_WRITE(Adapter, NvRegLinkSpeed, 0);
    }
}

VOID
NvNetStopTransmitter(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG TxControl, i;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    TxControl = NV_READ(Adapter, NvRegTransmitterControl);
    if (!(Adapter->Flags & NV_MAC_IN_USE))
        TxControl &= ~NVREG_XMITCTL_START;
    else
        TxControl |= NVREG_XMITCTL_TX_PATH_EN;
    NV_WRITE(Adapter, NvRegTransmitterControl, TxControl);

    for (i = 0; i < NV_TXSTOP_DELAY1MAX; ++i)
    {
        if (!(NV_READ(Adapter, NvRegTransmitterStatus) & NVREG_XMITSTAT_BUSY))
            break;

        NdisStallExecution(NV_TXSTOP_DELAY1);
    }

    NdisStallExecution(NV_TXSTOP_DELAY2);

    if (!(Adapter->Flags & NV_MAC_IN_USE))
    {
        NV_WRITE(Adapter, NvRegTransmitPoll,
                 NV_READ(Adapter, NvRegTransmitPoll) & NVREG_TRANSMITPOLL_MAC_ADDR_REV);
    }
}

CODE_SEG("PAGE")
VOID
NvNetIdleTransmitter(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ BOOLEAN ClearPhyControl)
{
    ULONG i;

    PAGED_CODE();

    if (ClearPhyControl)
    {
        NV_WRITE(Adapter, NvRegAdapterControl,
                 NV_READ(Adapter, NvRegAdapterControl) & ~NVREG_ADAPTCTL_RUNNING);
    }
    else
    {
        NV_WRITE(Adapter, NvRegAdapterControl,
                 (Adapter->PhyAddress << NVREG_ADAPTCTL_PHYSHIFT) |
                 NVREG_ADAPTCTL_PHYVALID | NVREG_ADAPTCTL_RUNNING);
    }

    NV_WRITE(Adapter, NvRegTxRxControl, Adapter->TxRxControl | NVREG_TXRXCTL_BIT2);
    for (i = 0; i < NV_TXIDLE_ATTEMPTS; ++i)
    {
        if (NV_READ(Adapter, NvRegTxRxControl) & NVREG_TXRXCTL_IDLE)
            break;

        NdisStallExecution(NV_TXIDLE_DELAY);
    }
}

VOID
NvNetUpdatePauseFrame(
    _Inout_ PNVNET_ADAPTER Adapter,
    _In_ ULONG PauseFlags)
{
    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    Adapter->PauseFlags &= ~(NV_PAUSEFRAME_TX_ENABLE | NV_PAUSEFRAME_RX_ENABLE);

    if (Adapter->PauseFlags & NV_PAUSEFRAME_RX_CAPABLE)
    {
        ULONG PacketFilter = NV_READ(Adapter, NvRegPacketFilterFlags) & ~NVREG_PFF_PAUSE_RX;

        if (PauseFlags & NV_PAUSEFRAME_RX_ENABLE)
        {
            PacketFilter |= NVREG_PFF_PAUSE_RX;
            Adapter->PauseFlags |= NV_PAUSEFRAME_RX_ENABLE;
        }
        NV_WRITE(Adapter, NvRegPacketFilterFlags, PacketFilter);
    }

    if (Adapter->PauseFlags & NV_PAUSEFRAME_TX_CAPABLE)
    {
        ULONG Mics = NV_READ(Adapter, NvRegMisc1) & ~NVREG_MISC1_PAUSE_TX;

        if (PauseFlags & NV_PAUSEFRAME_TX_ENABLE)
        {
            ULONG PauseEnable = NVREG_TX_PAUSEFRAME_ENABLE_V1;

            if (Adapter->Features & DEV_HAS_PAUSEFRAME_TX_V2)
                PauseEnable = NVREG_TX_PAUSEFRAME_ENABLE_V2;
            if (Adapter->Features & DEV_HAS_PAUSEFRAME_TX_V3)
            {
                PauseEnable = NVREG_TX_PAUSEFRAME_ENABLE_V3;
                /* Limit the number of TX pause frames to a default of 8 */
                NV_WRITE(Adapter,
                         NvRegTxPauseFrameLimit,
                         NV_READ(Adapter, NvRegTxPauseFrameLimit) |
                         NVREG_TX_PAUSEFRAMELIMIT_ENABLE);
            }
            NV_WRITE(Adapter, NvRegTxPauseFrame, PauseEnable);
            NV_WRITE(Adapter, NvRegMisc1, Mics | NVREG_MISC1_PAUSE_TX);
            Adapter->PauseFlags |= NV_PAUSEFRAME_TX_ENABLE;
        }
        else
        {
            NV_WRITE(Adapter, NvRegTxPauseFrame, NVREG_TX_PAUSEFRAME_DISABLE);
            NV_WRITE(Adapter, NvRegMisc1, Mics);
        }
    }
}

VOID
NvNetToggleClockPowerGating(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ BOOLEAN Gate)
{
    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    if (!(Adapter->Flags & NV_MAC_IN_USE) && (Adapter->Features & DEV_HAS_POWER_CNTRL))
    {
        ULONG PowerState = NV_READ(Adapter, NvRegPowerState2);

        if (Gate)
            PowerState |= NVREG_POWERSTATE2_GATE_CLOCKS;
        else
            PowerState &= ~NVREG_POWERSTATE2_GATE_CLOCKS;
        NV_WRITE(Adapter, NvRegPowerState2, PowerState);
    }
}

VOID
NTAPI
NvNetMediaDetectionDpc(
    _In_ PVOID SystemSpecific1,
    _In_ PVOID FunctionContext,
    _In_ PVOID SystemSpecific2,
    _In_ PVOID SystemSpecific3)
{
    PNVNET_ADAPTER Adapter = FunctionContext;
    BOOLEAN Connected, Report = FALSE;

    UNREFERENCED_PARAMETER(SystemSpecific1);
    UNREFERENCED_PARAMETER(SystemSpecific2);
    UNREFERENCED_PARAMETER(SystemSpecific3);

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    NdisDprAcquireSpinLock(&Adapter->Lock);

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

BOOLEAN
NTAPI
NvNetInitPhaseSynchronized(
    _In_ PVOID SynchronizeContext)
{
    PNVNET_ADAPTER Adapter = SynchronizeContext;

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    /* Enable interrupts on the NIC */
    NvNetApplyInterruptMask(Adapter);

    /*
     * One manual link speed update: Interrupts are enabled,
     * future link speed changes cause interrupts.
     */
    NV_READ(Adapter, NvRegMIIStatus);
    NV_WRITE(Adapter, NvRegMIIStatus, NVREG_MIISTAT_MASK_ALL);

    /* Set link speed to invalid value, thus force NvNetUpdateLinkSpeed() to init HW */
    Adapter->LinkSpeed = 0xFFFFFFFF;

    Adapter->Connected = NvNetUpdateLinkSpeed(Adapter);

    NvNetStartReceiver(Adapter);
    NvNetStartTransmitter(Adapter);

    Adapter->Flags |= NV_ACTIVE;

    return TRUE;
}

CODE_SEG("PAGE")
NDIS_STATUS
NvNetInitNIC(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ BOOLEAN InitPhy)
{
    ULONG MiiControl, i;
    NDIS_STATUS Status;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    /* Disable WOL */
    NV_WRITE(Adapter, NvRegWakeUpFlags, 0);

    if (InitPhy)
    {
        Status = NvNetPhyInit(Adapter);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            return Status;
        }
    }

    if (Adapter->PauseFlags & NV_PAUSEFRAME_TX_CAPABLE)
    {
        NV_WRITE(Adapter, NvRegTxPauseFrame, NVREG_TX_PAUSEFRAME_DISABLE);
    }

    /* Power up PHY */
    MiiRead(Adapter, Adapter->PhyAddress, MII_CONTROL, &MiiControl);
    MiiControl &= ~MII_CR_POWER_DOWN;
    MiiWrite(Adapter, Adapter->PhyAddress, MII_CONTROL, MiiControl);

    NvNetToggleClockPowerGating(Adapter, FALSE);

    NvNetResetMac(Adapter);

    /* Clear multicast masks and addresses */
    NV_WRITE(Adapter, NvRegMulticastAddrA, 0);
    NV_WRITE(Adapter, NvRegMulticastAddrB, 0);
    NV_WRITE(Adapter, NvRegMulticastMaskA, NVREG_MCASTMASKA_NONE);
    NV_WRITE(Adapter, NvRegMulticastMaskB, NVREG_MCASTMASKB_NONE);

    NV_WRITE(Adapter, NvRegTransmitterControl, 0);
    NV_WRITE(Adapter, NvRegReceiverControl, 0);

    NV_WRITE(Adapter, NvRegAdapterControl, 0);

    NV_WRITE(Adapter, NvRegLinkSpeed, 0);
    NV_WRITE(Adapter, NvRegTransmitPoll,
             NV_READ(Adapter, NvRegTransmitPoll) & NVREG_TRANSMITPOLL_MAC_ADDR_REV);
    NvNetResetReceiverAndTransmitter(Adapter);
    NV_WRITE(Adapter, NvRegUnknownSetupReg6, 0);

    /* Receive descriptor ring buffer */
    NV_WRITE(Adapter, NvRegRxRingPhysAddr,
             NdisGetPhysicalAddressLow(Adapter->RbdPhys));
    if (Adapter->Features & DEV_HAS_HIGH_DMA)
    {
        NV_WRITE(Adapter, NvRegRxRingPhysAddrHigh,
                 NdisGetPhysicalAddressHigh(Adapter->RbdPhys));
    }

    /* Transmit descriptor ring buffer */
    NV_WRITE(Adapter, NvRegTxRingPhysAddr,
             NdisGetPhysicalAddressLow(Adapter->TbdPhys));
    if (Adapter->Features & DEV_HAS_HIGH_DMA)
    {
        NV_WRITE(Adapter, NvRegTxRingPhysAddrHigh,
                 NdisGetPhysicalAddressHigh(Adapter->TbdPhys));
    }

    /* Ring sizes */
    NV_WRITE(Adapter, NvRegRingSizes,
             (NVNET_RECEIVE_DESCRIPTORS - 1) << NVREG_RINGSZ_RXSHIFT |
             (NVNET_TRANSMIT_DESCRIPTORS - 1) << NVREG_RINGSZ_TXSHIFT);

    /* Set default link speed settings */
    NV_WRITE(Adapter, NvRegLinkSpeed, NVREG_LINKSPEED_FORCE | NVREG_LINKSPEED_10);

    if (Adapter->Features & (DEV_HAS_HIGH_DMA | DEV_HAS_LARGEDESC))
        NV_WRITE(Adapter, NvRegTxWatermark, NVREG_TX_WM_DESC2_3_DEFAULT);
    else
        NV_WRITE(Adapter, NvRegTxWatermark, NVREG_TX_WM_DESC1_DEFAULT);

    NV_WRITE(Adapter, NvRegTxRxControl, Adapter->TxRxControl);
    NV_WRITE(Adapter, NvRegVlanControl, Adapter->VlanControl);
    NV_WRITE(Adapter, NvRegTxRxControl, Adapter->TxRxControl | NVREG_TXRXCTL_BIT1);

    for (i = 0; i < NV_SETUP5_DELAYMAX; ++i)
    {
        if (NV_READ(Adapter, NvRegUnknownSetupReg5) & NVREG_UNKSETUP5_BIT31)
            break;

        NdisStallExecution(NV_SETUP5_DELAY);
    }

    NV_WRITE(Adapter, NvRegMIIMask, 0);
    NV_WRITE(Adapter, NvRegIrqStatus, NVREG_IRQSTAT_MASK);
    NV_WRITE(Adapter, NvRegMIIStatus, NVREG_MIISTAT_MASK_ALL);

    NV_WRITE(Adapter, NvRegMisc1, NVREG_MISC1_FORCE | NVREG_MISC1_HD);
    NV_WRITE(Adapter, NvRegTransmitterStatus, NV_READ(Adapter, NvRegTransmitterStatus));
    NV_WRITE(Adapter, NvRegPacketFilterFlags, NVREG_PFF_ALWAYS | NVREG_PFF_MYADDR);
    NV_WRITE(Adapter, NvRegOffloadConfig, (NVNET_MAXIMUM_FRAME_SIZE - sizeof(ETH_HEADER))
                                          + NV_RX_HEADERS);

    NV_WRITE(Adapter, NvRegReceiverStatus, NV_READ(Adapter, NvRegReceiverStatus));

    NvNetBackoffSetSlotTime(Adapter);

    NV_WRITE(Adapter, NvRegTxDeferral, NVREG_TX_DEFERRAL_DEFAULT);
    NV_WRITE(Adapter, NvRegRxDeferral, NVREG_RX_DEFERRAL_DEFAULT);

    if (Adapter->OptimizationMode == NV_OPTIMIZATION_MODE_THROUGHPUT)
        NV_WRITE(Adapter, NvRegPollingInterval, NVREG_POLL_DEFAULT_THROUGHPUT);
    else
        NV_WRITE(Adapter, NvRegPollingInterval, NVREG_POLL_DEFAULT_CPU);
    NV_WRITE(Adapter, NvRegUnknownSetupReg6, NVREG_UNKSETUP6_VAL);

    NV_WRITE(Adapter, NvRegAdapterControl,
             (Adapter->PhyAddress << NVREG_ADAPTCTL_PHYSHIFT) |
             NVREG_ADAPTCTL_PHYVALID | NVREG_ADAPTCTL_RUNNING);
    NV_WRITE(Adapter, NvRegMIISpeed, NVREG_MIISPEED_BIT8 | NVREG_MIIDELAY);
    NV_WRITE(Adapter, NvRegMIIMask, NVREG_MII_LINKCHANGE);

    NdisStallExecution(10);
    NV_WRITE(Adapter, NvRegPowerState,
             NV_READ(Adapter, NvRegPowerState) & ~NVREG_POWERSTATE_VALID);

    if (Adapter->Features & DEV_HAS_STATISTICS_COUNTERS)
    {
        NvNetClearStatisticsCounters(Adapter);
    }

    return NDIS_STATUS_SUCCESS;
}

CODE_SEG("PAGE")
NDIS_STATUS
NvNetGetPermanentMacAddress(
    _Inout_ PNVNET_ADAPTER Adapter,
    _Out_writes_bytes_all_(ETH_LENGTH_OF_ADDRESS) PUCHAR MacAddress)
{
    ULONG Temp[2], TxPoll;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    Temp[0] = NV_READ(Adapter, NvRegMacAddrA);
    Temp[1] = NV_READ(Adapter, NvRegMacAddrB);

    TxPoll = NV_READ(Adapter, NvRegTransmitPoll);

    if (Adapter->Features & DEV_HAS_CORRECT_MACADDR)
    {
        /* MAC address is already in the correct order */
        MacAddress[0] = (Temp[0] >> 0) & 0xFF;
        MacAddress[1] = (Temp[0] >> 8) & 0xFF;
        MacAddress[2] = (Temp[0] >> 16) & 0xFF;
        MacAddress[3] = (Temp[0] >> 24) & 0xFF;
        MacAddress[4] = (Temp[1] >> 0) & 0xFF;
        MacAddress[5] = (Temp[1] >> 8) & 0xFF;
    }
    /* Handle the special flag for the correct MAC address order */
    else if (TxPoll & NVREG_TRANSMITPOLL_MAC_ADDR_REV)
    {
        /* MAC address is already in the correct order */
        MacAddress[0] = (Temp[0] >> 0) & 0xFF;
        MacAddress[1] = (Temp[0] >> 8) & 0xFF;
        MacAddress[2] = (Temp[0] >> 16) & 0xFF;
        MacAddress[3] = (Temp[0] >> 24) & 0xFF;
        MacAddress[4] = (Temp[1] >> 0) & 0xFF;
        MacAddress[5] = (Temp[1] >> 8) & 0xFF;

        /*
        * Set original MAC address back to the reversed version.
        * This flag will be cleared during low power transition.
        * Therefore, we should always put back the reversed address.
        */
        Temp[0] = (MacAddress[5] << 0) | (MacAddress[4] << 8) |
                  (MacAddress[3] << 16) | (MacAddress[2] << 24);
        Temp[1] = (MacAddress[1] << 0) | (MacAddress[0] << 8);
    }
    else
    {
        /* Need to reverse MAC address to the correct order */
        MacAddress[0] = (Temp[1] >> 8) & 0xFF;
        MacAddress[1] = (Temp[1] >> 0) & 0xFF;
        MacAddress[2] = (Temp[0] >> 24) & 0xFF;
        MacAddress[3] = (Temp[0] >> 16) & 0xFF;
        MacAddress[4] = (Temp[0] >> 8) & 0xFF;
        MacAddress[5] = (Temp[0] >> 0) & 0xFF;

        /*
         * Use a flag to signal the driver whether the MAC address was already corrected,
         * so that it is not reversed again on a subsequent initialize.
         */
        NV_WRITE(Adapter, NvRegTransmitPoll, TxPoll | NVREG_TRANSMITPOLL_MAC_ADDR_REV);
    }

    Adapter->OriginalMacAddress[0] = Temp[0];
    Adapter->OriginalMacAddress[1] = Temp[1];

    NDIS_DbgPrint(MIN_TRACE, ("MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                              MacAddress[0],
                              MacAddress[1],
                              MacAddress[2],
                              MacAddress[3],
                              MacAddress[4],
                              MacAddress[5]));

    if (ETH_IS_MULTICAST(MacAddress) || ETH_IS_EMPTY(MacAddress))
        return NDIS_STATUS_INVALID_ADDRESS;

    return NDIS_STATUS_SUCCESS;
}

CODE_SEG("PAGE")
VOID
NvNetSetupMacAddress(
    _In_ PNVNET_ADAPTER Adapter,
    _In_reads_bytes_(ETH_LENGTH_OF_ADDRESS) PUCHAR MacAddress)
{
    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    NV_WRITE(Adapter, NvRegMacAddrA,
             MacAddress[3] << 24 | MacAddress[2] << 16 | MacAddress[1] << 8 | MacAddress[0]);
    NV_WRITE(Adapter, NvRegMacAddrB, MacAddress[5] << 8 | MacAddress[4]);
}

static
VOID
CODE_SEG("PAGE")
NvNetValidateConfiguration(
    _Inout_ PNVNET_ADAPTER Adapter)
{
    PAGED_CODE();

    if (!(Adapter->Features & DEV_HAS_LARGEDESC))
    {
        Adapter->MaximumFrameSize = NVNET_MAXIMUM_FRAME_SIZE;
    }
    if (!(Adapter->Features & DEV_HAS_CHECKSUM))
    {
        Adapter->Flags &= ~(NV_SEND_CHECKSUM | NV_SEND_LARGE_SEND);
    }
    if (!(Adapter->Features & DEV_HAS_VLAN))
    {
        Adapter->Flags &= ~(NV_PACKET_PRIORITY | NV_VLAN_TAGGING);
    }
    if ((Adapter->Features & DEV_NEED_TIMERIRQ) &&
        (Adapter->OptimizationMode == NV_OPTIMIZATION_MODE_DYNAMIC))
    {
        Adapter->OptimizationMode = NV_OPTIMIZATION_MODE_THROUGHPUT;
    }
    if (!(Adapter->Features & DEV_HAS_TX_PAUSEFRAME))
    {
        if (Adapter->FlowControlMode == NV_FLOW_CONTROL_TX)
        {
            Adapter->FlowControlMode = NV_FLOW_CONTROL_AUTO;
        }
        else if (Adapter->FlowControlMode == NV_FLOW_CONTROL_RX_TX)
        {
            Adapter->FlowControlMode = NV_FLOW_CONTROL_RX;
        }
    }
}

CODE_SEG("PAGE")
NDIS_STATUS
NvNetRecognizeHardware(
    _Inout_ PNVNET_ADAPTER Adapter)
{
    ULONG Bytes;
    PCI_COMMON_CONFIG PciConfig;

    PAGED_CODE();

    NDIS_DbgPrint(MIN_TRACE, ("()\n"));

    Bytes = NdisReadPciSlotInformation(Adapter->AdapterHandle,
                                       0,
                                       FIELD_OFFSET(PCI_COMMON_CONFIG, VendorID),
                                       &PciConfig,
                                       PCI_COMMON_HDR_LENGTH);
    if (Bytes != PCI_COMMON_HDR_LENGTH)
        return NDIS_STATUS_ADAPTER_NOT_FOUND;

    if (PciConfig.VendorID != 0x10DE)
        return NDIS_STATUS_ADAPTER_NOT_FOUND;

    Adapter->DeviceId = PciConfig.DeviceID;
    Adapter->RevisionId = PciConfig.RevisionID;

    switch (PciConfig.DeviceID)
    {
        case 0x01C3: /* nForce */
        case 0x0066: /* nForce2 */
        case 0x00D6: /* nForce2 */
            Adapter->Features = DEV_NEED_TIMERIRQ | DEV_NEED_LINKTIMER;
            break;

        case 0x0086: /* nForce3 */
        case 0x008C: /* nForce3 */
        case 0x00E6: /* nForce3 */
        case 0x00DF: /* nForce3 */
            Adapter->Features = DEV_NEED_TIMERIRQ | DEV_NEED_LINKTIMER |
                                DEV_HAS_LARGEDESC | DEV_HAS_CHECKSUM;
            break;

        case 0x0056: /* CK804 */
        case 0x0057: /* CK804 */
        case 0x0037: /* MCP04 */
        case 0x0038: /* MCP04 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_LARGEDESC | DEV_HAS_CHECKSUM |
                                DEV_HAS_HIGH_DMA | DEV_HAS_STATISTICS_V1 | DEV_NEED_TX_LIMIT;
            break;

        case 0x0268: /* MCP51 */
        case 0x0269: /* MCP51 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_HIGH_DMA | DEV_HAS_POWER_CNTRL |
                                DEV_HAS_STATISTICS_V1 | DEV_NEED_LOW_POWER_FIX;
            break;

        case 0x0372: /* MCP55 */
        case 0x0373: /* MCP55 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_LARGEDESC | DEV_HAS_CHECKSUM |
                                DEV_HAS_HIGH_DMA | DEV_HAS_VLAN | DEV_HAS_MSI | DEV_HAS_MSI_X |
                                DEV_HAS_POWER_CNTRL | DEV_HAS_PAUSEFRAME_TX_V1 |
                                DEV_HAS_STATISTICS_V1 | DEV_HAS_STATISTICS_V2 |
                                DEV_HAS_TEST_EXTENDED | DEV_HAS_MGMT_UNIT |
                                DEV_NEED_TX_LIMIT | DEV_NEED_MSI_FIX;
            break;

        case 0x03E5: /* MCP61 */
        case 0x03E6: /* MCP61 */
        case 0x03EE: /* MCP61 */
        case 0x03EF: /* MCP61 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_HIGH_DMA | DEV_HAS_POWER_CNTRL |
                                DEV_HAS_MSI | DEV_HAS_PAUSEFRAME_TX_V1 | DEV_HAS_STATISTICS_V1 |
                                DEV_HAS_STATISTICS_V2 | DEV_HAS_TEST_EXTENDED | DEV_HAS_MGMT_UNIT |
                                DEV_HAS_CORRECT_MACADDR | DEV_NEED_MSI_FIX;
            break;

        case 0x0450: /* MCP65 */
        case 0x0451: /* MCP65 */
        case 0x0452: /* MCP65 */
        case 0x0453: /* MCP65 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_LARGEDESC | DEV_HAS_HIGH_DMA |
                                DEV_HAS_POWER_CNTRL | DEV_HAS_MSI | DEV_HAS_PAUSEFRAME_TX_V1 |
                                DEV_HAS_STATISTICS_V1 | DEV_HAS_STATISTICS_V2 |
                                DEV_HAS_TEST_EXTENDED | DEV_HAS_MGMT_UNIT |
                                DEV_HAS_CORRECT_MACADDR | DEV_NEED_TX_LIMIT |
                                DEV_HAS_GEAR_MODE | DEV_NEED_MSI_FIX;
            break;

        case 0x054C: /* MCP67 */
        case 0x054D: /* MCP67 */
        case 0x054E: /* MCP67 */
        case 0x054F: /* MCP67 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_HIGH_DMA | DEV_HAS_POWER_CNTRL |
                                DEV_HAS_MSI | DEV_HAS_PAUSEFRAME_TX_V1 | DEV_HAS_STATISTICS_V1 |
                                DEV_HAS_STATISTICS_V2 | DEV_HAS_TEST_EXTENDED | DEV_HAS_MGMT_UNIT |
                                DEV_HAS_CORRECT_MACADDR | DEV_HAS_GEAR_MODE | DEV_NEED_MSI_FIX;
            break;

        case 0x07DC: /* MCP73 */
        case 0x07DD: /* MCP73 */
        case 0x07DE: /* MCP73 */
        case 0x07DF: /* MCP73 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_HIGH_DMA | DEV_HAS_POWER_CNTRL |
                                DEV_HAS_MSI | DEV_HAS_PAUSEFRAME_TX_V1 | DEV_HAS_STATISTICS_V1 |
                                DEV_HAS_STATISTICS_V2 | DEV_HAS_TEST_EXTENDED | DEV_HAS_MGMT_UNIT |
                                DEV_HAS_CORRECT_MACADDR | DEV_HAS_COLLISION_FIX |
                                DEV_HAS_GEAR_MODE | DEV_NEED_MSI_FIX;
            break;

        case 0x0760: /* MCP77 */
        case 0x0761: /* MCP77 */
        case 0x0762: /* MCP77 */
        case 0x0763: /* MCP77 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_CHECKSUM | DEV_HAS_HIGH_DMA |
                                DEV_HAS_MSI | DEV_HAS_POWER_CNTRL | DEV_HAS_PAUSEFRAME_TX_V2 |
                                DEV_HAS_STATISTICS_V1 | DEV_HAS_STATISTICS_V2 |
                                DEV_HAS_STATISTICS_V3 | DEV_HAS_TEST_EXTENDED | DEV_HAS_MGMT_UNIT |
                                DEV_HAS_CORRECT_MACADDR | DEV_HAS_COLLISION_FIX |
                                DEV_NEED_TX_LIMIT2 | DEV_HAS_GEAR_MODE |
                                DEV_NEED_PHY_INIT_FIX | DEV_NEED_MSI_FIX;
            break;

        case 0x0AB0: /* MCP79 */
        case 0x0AB1: /* MCP79 */
        case 0x0AB2: /* MCP79 */
        case 0x0AB3: /* MCP79 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_LARGEDESC | DEV_HAS_CHECKSUM |
                                DEV_HAS_HIGH_DMA | DEV_HAS_MSI | DEV_HAS_POWER_CNTRL |
                                DEV_HAS_PAUSEFRAME_TX_V3 | DEV_HAS_STATISTICS_V1 |
                                DEV_HAS_STATISTICS_V2 | DEV_HAS_STATISTICS_V3 |
                                DEV_HAS_TEST_EXTENDED | DEV_HAS_CORRECT_MACADDR |
                                DEV_HAS_COLLISION_FIX | DEV_NEED_TX_LIMIT2 |
                                DEV_HAS_GEAR_MODE | DEV_NEED_PHY_INIT_FIX | DEV_NEED_MSI_FIX;
            break;

        case 0x0D7D: /* MCP89 */
            Adapter->Features = DEV_NEED_LINKTIMER | DEV_HAS_LARGEDESC | DEV_HAS_CHECKSUM |
                                DEV_HAS_HIGH_DMA | DEV_HAS_MSI | DEV_HAS_POWER_CNTRL |
                                DEV_HAS_PAUSEFRAME_TX_V3 | DEV_HAS_STATISTICS_V1 |
                                DEV_HAS_STATISTICS_V2 | DEV_HAS_STATISTICS_V3 |
                                DEV_HAS_TEST_EXTENDED | DEV_HAS_CORRECT_MACADDR |
                                DEV_HAS_COLLISION_FIX | DEV_HAS_GEAR_MODE | DEV_NEED_PHY_INIT_FIX;
            break;

        default:
            return NDIS_STATUS_NOT_RECOGNIZED;
    }

    /* Normalize all .INF parameters */
    NvNetValidateConfiguration(Adapter);

    /* FIXME: Disable some NIC features, we don't support these yet */
#if 1
    Adapter->VlanControl = 0;
    Adapter->Flags &= ~(NV_SEND_CHECKSUM | NV_SEND_LARGE_SEND |
                        NV_PACKET_PRIORITY | NV_VLAN_TAGGING);
#endif

    /* For code paths debugging (32-bit descriptors work on all hardware variants) */
#if 0
    Adapter->Features &= ~(DEV_HAS_HIGH_DMA | DEV_HAS_LARGEDESC);
#endif

    if (Adapter->Features & DEV_HAS_POWER_CNTRL)
        Adapter->WakeFrameBitmap = ~(0xFFFFFFFF << NV_WAKEUPPATTERNS_V2);
    else
        Adapter->WakeFrameBitmap = ~(0xFFFFFFFF << NV_WAKEUPPATTERNS);

    /* 64-bit descriptors */
    if (Adapter->Features & DEV_HAS_HIGH_DMA)
    {
        /* Note: Some devices here also support Jumbo Frames */
        Adapter->TxRxControl = NVREG_TXRXCTL_DESC_3;
    }
    /* 32-bit descriptors */
    else
    {
        if (Adapter->Features & DEV_HAS_LARGEDESC)
        {
            /* Jumbo Frames */
            Adapter->TxRxControl = NVREG_TXRXCTL_DESC_2;
        }
        else
        {
            /* Original packet format */
            Adapter->TxRxControl = NVREG_TXRXCTL_DESC_1;
        }
    }

    /* Flow control */
    Adapter->PauseFlags = NV_PAUSEFRAME_RX_CAPABLE | NV_PAUSEFRAME_RX_REQ | NV_PAUSEFRAME_AUTONEG;
    if (Adapter->Features & DEV_HAS_TX_PAUSEFRAME)
    {
        Adapter->PauseFlags |= NV_PAUSEFRAME_TX_CAPABLE | NV_PAUSEFRAME_TX_REQ;
    }
    if (Adapter->FlowControlMode != NV_FLOW_CONTROL_AUTO)
    {
        Adapter->PauseFlags &= ~(NV_PAUSEFRAME_AUTONEG | NV_PAUSEFRAME_RX_REQ |
                                 NV_PAUSEFRAME_TX_REQ);
        switch (Adapter->FlowControlMode)
        {
            case NV_FLOW_CONTROL_RX:
                Adapter->PauseFlags |= NV_PAUSEFRAME_RX_REQ;
                break;
            case NV_FLOW_CONTROL_TX:
                Adapter->PauseFlags |= NV_PAUSEFRAME_TX_REQ;
                break;
            case NV_FLOW_CONTROL_RX_TX:
                Adapter->PauseFlags |= NV_PAUSEFRAME_RX_REQ | NV_PAUSEFRAME_TX_REQ;
                break;

            default:
                break;
        }
    }

    /* Work around errata in some NICs */
    if (Adapter->Features & (DEV_NEED_TX_LIMIT | DEV_NEED_TX_LIMIT2))
    {
        Adapter->Flags |= NV_SEND_ERRATA_PRESENT;

        if ((Adapter->Features & DEV_NEED_TX_LIMIT2) && Adapter->RevisionId >= 0xA2)
        {
            Adapter->Flags &= ~NV_SEND_ERRATA_PRESENT;
        }
    }
    if (Adapter->Flags & NV_SEND_ERRATA_PRESENT)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Transmit workaround active\n"));
    }

    /* Initialize the interrupt mask */
    if (Adapter->OptimizationMode == NV_OPTIMIZATION_MODE_CPU)
    {
        Adapter->InterruptMask = NVREG_IRQMASK_CPU;
    }
    else
    {
        Adapter->InterruptMask = NVREG_IRQMASK_THROUGHPUT;
    }
    if (Adapter->Features & DEV_NEED_TIMERIRQ)
    {
        Adapter->InterruptMask |= NVREG_IRQ_TIMER;
    }

    if (Adapter->Features & DEV_NEED_LINKTIMER)
    {
        NdisMInitializeTimer(&Adapter->MediaDetectionTimer,
                             Adapter->AdapterHandle,
                             NvNetMediaDetectionDpc,
                             Adapter);
    }

    return NDIS_STATUS_SUCCESS;
}
