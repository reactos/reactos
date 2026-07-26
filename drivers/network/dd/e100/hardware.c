/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Hardware specific functions
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * HW access code was taken from the FreeBSD fxp driver.
 * Copyright (c) 1995, David Greenman
 * Copyright (c) 2001 Jonathan Lemon <jlemon@freebsd.org>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"
#include "rcvbundl.h"

#include <debug.h>

/* GLOBASLS *******************************************************************/

E100_PAGED_DATA static const ULONG E100MicrocodeD101a[] = D101_A_RCVBUNDLE_UCODE;
E100_PAGED_DATA static const ULONG E100MicrocodeD101b0[] = D101_B0_RCVBUNDLE_UCODE;
E100_PAGED_DATA static const ULONG E100MicrocodeD101ma[] = D101M_B_RCVBUNDLE_UCODE;
E100_PAGED_DATA static const ULONG E100MicrocodeD101s[] = D101S_RCVBUNDLE_UCODE;
E100_PAGED_DATA static const ULONG E100MicrocodeD102[] = D102_B_RCVBUNDLE_UCODE;
E100_PAGED_DATA static const ULONG E100MicrocodeD102c[] = D102_C_RCVBUNDLE_UCODE;
E100_PAGED_DATA static const ULONG E100MicrocodeD102e[] = D102_E_RCVBUNDLE_UCODE;

E100_PAGED_DATA static const E100_UCODE_INFO E100MicrocodeTable[] =
{
#define UCODE(x)    x, RTL_NUMBER_OF(x)
    {
        UCODE(E100MicrocodeD101a), FXP_REV_82558_A4,
        D101_CPUSAVER_DWORD, 0, 0
    },
    {
        UCODE(E100MicrocodeD101b0), FXP_REV_82558_B0,
        D101_CPUSAVER_DWORD, 0, 0
    },
    {
        UCODE(E100MicrocodeD101ma), FXP_REV_82559_A0,
        D101M_CPUSAVER_DWORD, D101M_CPUSAVER_BUNDLE_MAX_DWORD, D101M_CPUSAVER_MIN_SIZE_DWORD
    },
    {
        UCODE(E100MicrocodeD101s), FXP_REV_82559S_A,
        D101S_CPUSAVER_DWORD, D101S_CPUSAVER_BUNDLE_MAX_DWORD, D101S_CPUSAVER_MIN_SIZE_DWORD
    },
    {
        UCODE(E100MicrocodeD102), FXP_REV_82550,
        D102_B_CPUSAVER_DWORD, D102_B_CPUSAVER_BUNDLE_MAX_DWORD, D102_B_CPUSAVER_MIN_SIZE_DWORD
    },
    {
        UCODE(E100MicrocodeD102c), FXP_REV_82550_C,
        D102_C_CPUSAVER_DWORD, D102_C_CPUSAVER_BUNDLE_MAX_DWORD, 0
    },
    {
        UCODE(E100MicrocodeD102e), FXP_REV_82551_F,
        D102_E_CPUSAVER_DWORD, D102_E_CPUSAVER_BUNDLE_MAX_DWORD, D102_E_CPUSAVER_MIN_SIZE_DWORD
    },
    {
        UCODE(E100MicrocodeD102e), FXP_REV_82551_10,
        D102_E_CPUSAVER_DWORD, D102_E_CPUSAVER_BUNDLE_MAX_DWORD, D102_E_CPUSAVER_MIN_SIZE_DWORD
    },
    { NULL, 0, 0, 0, 0 }
#undef UCODE
};

/* FUNCTIONS ******************************************************************/

VOID
EeSoftReset(
    _In_ PE100_ADAPTER Adapter)
{
    /* Do selective reset first to prevent PCI bus lock-up */
    CSR_WRITE_32(Adapter, FXP_CSR_PORT, FXP_PORT_SELECTIVE_RESET);
    NdisStallExecution(10);

    CSR_WRITE_32(Adapter, FXP_CSR_PORT, FXP_PORT_SOFTWARE_RESET);
    NdisStallExecution(10);

    /* Software reset will unmask interrupts so disable them after the reset */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_INTRCNTL, FXP_SCB_INTR_DISABLE);
}

static
PULONG
EeGetDumpCompletionStatus(
    _In_ PE100_ADAPTER Adapter,
    _In_ PFXP_COUNTERS Counters)
{
    if (Adapter->RevisionID >= FXP_REV_82559_A0)
        return &Counters->CompletionStatus559;
    else if (Adapter->RevisionID >= FXP_REV_82558_A4)
        return &Counters->CompletionStatus558;
    else
        return &Counters->CompletionStatus557;
}

static
VOID
EeDumpStatisticsCounters(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_STATISTICS Statistics = &Adapter->Statistics;
    PFXP_COUNTERS Counters = &Adapter->ControlBlock->Counters;
    PULONG CompletionStatus;

    CompletionStatus = EeGetDumpCompletionStatus(Adapter, Counters);
    if (*CompletionStatus != htole32(FXP_STATS_DR_COMPLETE))
        return;

    *CompletionStatus = 0;

    TRACE("Dump command completed successfully\n");

    Statistics->TransmitOk += letoh32(Counters->TxOk);
    Statistics->TransmitErrors += letoh32(Counters->TxMaxCol) +
                                  letoh32(Counters->TxLateCol) +
                                  letoh32(Counters->TxUnderruns) +
                                  letoh32(Counters->TxLossCarrier);
    Statistics->TransmitOneRetry += letoh32(Counters->TxSingleCol);
    Statistics->TransmitMoreCollisions += letoh32(Counters->TxMiltipleCol);
    Statistics->TransmitDeferred += letoh32(Counters->TxDeffered);
    Statistics->TransmitExcessiveCollisions += letoh32(Counters->TxMaxCol);
    Statistics->TransmitUnderrunErrors += letoh32(Counters->TxUnderruns);
    Statistics->TransmitLostCarrierSense += letoh32(Counters->TxLossCarrier);
    Statistics->TransmitLateCollisions += letoh32(Counters->TxLateCol);

    Statistics->ReceiveOk += letoh32(Counters->RxOk);
    Statistics->ReceiveErrors += letoh32(Counters->RxCrcErr) +
                                 letoh32(Counters->RxAlignErr) +
                                 letoh32(Counters->RxResourceErr) +
                                 letoh32(Counters->RxOverrunErr) +
                                 letoh32(Counters->RxShortFramesErr);
    Statistics->ReceiveOverrunErrors += letoh32(Counters->RxOverrunErr);
    Statistics->ReceiveNoBuffers += letoh32(Counters->RxResourceErr);
    Statistics->ReceiveCrcErrors += letoh32(Counters->RxCrcErr);
    Statistics->ReceiveAlignmentErrors += letoh32(Counters->RxAlignErr);

    /*
     * Errata: The receive unit may lock up upon certain types of garbage
     * in the synchronization bits prior to the packet header.
     */
    if (Adapter->Flags & E100_FLAG_RX_LOCKUP_BUG)
    {
        if (Counters->RxOk != 0)
        {
            /* We have received packets */
            Adapter->ReceiverUnitIdleTicks = 0;
        }
        else
        {
            if (++Adapter->ReceiverUnitIdleTicks > 7)
            {
                Adapter->ReceiverUnitIdleTicks = 0;

                /* Reprogram the multicast filter */
                INFO("We have not been received any packets, assume the receive unit has hung\n");
                EeUpdateMulticastList(Adapter);
            }
        }
    }

    NdisAcquireSpinLock(&Adapter->SendLock);

    /* Update the FIFO threshold level to minimize Tx FIFO underrun */
    if (Counters->TxUnderruns != 0)
    {
        if (Adapter->TransmitThreshold < 192)
        {
            Adapter->TransmitThreshold += 64;
            INFO("New transmit threshold %u\n", Adapter->TransmitThreshold);
        }
    }

    /* Start another statistics dump if possible */
    if (CSR_READ_8(Adapter, FXP_CSR_SCB_COMMAND) == 0)
    {
        TRACE("Dump statistics counters\n");
        CSR_WRITE_8(Adapter, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_DUMPRESET);
    }

    NdisReleaseSpinLock(&Adapter->SendLock);
}

BOOLEAN
EeScbWaitForCommandClear(
    _In_ PE100_ADAPTER Adapter)
{
    UCHAR Command;
    ULONG i;

    /* Wait up to 100 ms for the SCB to accept the previous command */
    for (i = (100 * 1000) / 2; i > 0; i--)
    {
        Command = CSR_READ_8(Adapter, FXP_CSR_SCB_COMMAND);

        if (Command == 0)
            return TRUE;

        NdisStallExecution(2);
    }

    ERR("SCB Timeout %02X %02X %02X\n",
        CSR_READ_8(Adapter, FXP_CSR_SCB_COMMAND),
        CSR_READ_8(Adapter, FXP_CSR_SCB_STATACK),
        CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS));
    Adapter->HardError = TRUE;
    return FALSE;
}

static
BOOLEAN
EeCommandUnitWaitForNotActive(
    _In_ PE100_ADAPTER Adapter)
{
    UCHAR CuStatus;
    ULONG i;

    /* Wait up to 500 ms until the command unit is not active */
    for (i = (500 * 1000) / 10; i > 0; i--)
    {
        CuStatus = FXP_SCB_CUS(CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS));

        if (CuStatus != FXP_SCB_CUS_ACTIVE)
            return TRUE;

        NdisStallExecution(10);
    }

    ERR("CU Timeout %02X %02X %02X\n",
        CSR_READ_8(Adapter, FXP_CSR_SCB_COMMAND),
        CSR_READ_8(Adapter, FXP_CSR_SCB_STATACK),
        CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS));
    Adapter->HardError = TRUE;
    return FALSE;
}

static
BOOLEAN
EeCommandUnitWaitForCompiletion(
    _In_ PE100_ADAPTER Adapter,
    _In_ PUSHORT CbStatus)
{
    USHORT Status;
    ULONG i;

    /* Wait up to 500 ms for command completion */
    for (i = (500 * 1000) / 2; i > 0; i--)
    {
        NdisStallExecution(2);

        KeMemoryBarrierWithoutFence();
        Status = htole16(*CbStatus);
        if (Status & FXP_CB_STATUS_C)
            break;
    }
    if (i == 0)
    {
        ERR("CU command timeout %02X %02X %02X %04X\n",
            CSR_READ_8(Adapter, FXP_CSR_SCB_COMMAND),
            CSR_READ_8(Adapter, FXP_CSR_SCB_STATACK),
            CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS),
            Status);
        Adapter->HardError = TRUE;
        return FALSE;
    }

    if (Status & FXP_CB_STATUS_OK)
        return TRUE;

    ERR("CU command failed %02X %02X %02X %04X\n",
        CSR_READ_8(Adapter, FXP_CSR_SCB_COMMAND),
        CSR_READ_8(Adapter, FXP_CSR_SCB_STATACK),
        CSR_READ_8(Adapter, FXP_CSR_SCB_RUSCUS),
        Status);
    Adapter->HardError = TRUE;
    return FALSE;
}

static
BOOLEAN
EeCommandUnitSendCommand(
    _In_ PE100_ADAPTER Adapter,
    _In_ USHORT Command,
    _In_ PFXP_CB_HEADER Header)
{
    ULONG ControlBlockOffset;

    TRACE("Send %X command\n", Command);

    Header->Status = 0;
    Header->Command = htole16(Command | FXP_CB_COMMAND_EL); // CU idle on completion
    Header->LinkAddress = 0xFFFFFFFF;

    EeScbWaitForCommandClear(Adapter);

    if (Adapter->CommandUnitActive)
    {
        EeCommandUnitWaitForNotActive(Adapter);
        Adapter->CommandUnitActive = FALSE;
    }

    ControlBlockOffset = (ULONG)((ULONG_PTR)Header - (ULONG_PTR)Adapter->ControlBlock);

    CSR_WRITE_32(Adapter, FXP_CSR_SCB_GENERAL, Adapter->ControlBlockPa + ControlBlockOffset);
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_START);

    return EeCommandUnitWaitForCompiletion(Adapter, &Header->Status);
}

static
CODE_SEG("PAGE")
BOOLEAN
EeCommandUnitExecuteCommand(
    _In_ PE100_ADAPTER Adapter,
    _In_ UCHAR Command,
    _In_ ULONG PhysicalAddress)
{
    PAGED_CODE();

    if (!EeScbWaitForCommandClear(Adapter))
        return FALSE;

    CSR_WRITE_32(Adapter, FXP_CSR_SCB_GENERAL, PhysicalAddress);
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_COMMAND, Command);
    return TRUE;
}

static
CODE_SEG("PAGE")
BOOLEAN
EeDownloadMicrocode(
    _In_ PE100_ADAPTER Adapter)
{
    const E100_UCODE_INFO* Info;
    PFXP_CB_LOAD_MICROCODE MicrocodeSetup;
    ULONG i;

    PAGED_CODE();

    if (Adapter->Flags & E100_FLAG_NO_UCODE)
        return TRUE;

    for (Info = E100MicrocodeTable; Info->Microcode; ++Info)
    {
        if (Adapter->RevisionID == Info->RevisionID)
            break;
    }
    if (!Info->Microcode)
        return TRUE;

    INFO("Download microcode: INT_DELAY %lu, BUNDLE_MAX %lu, MIN_SIZE_MASK %lX\n",
         Adapter->MicrocodeInterruptDelay,
         Adapter->MicrocodeMaxFramesPerIntr,
         Adapter->MicrocodeMinSizeMask);

    MicrocodeSetup = &Adapter->ControlBlock->MicrocodeSetup;
    for (i = 0; i < Info->Length; ++i)
    {
        MicrocodeSetup->Data[i] = htole32(Info->Microcode[i]);
    }
    for (; i < RTL_NUMBER_OF(MicrocodeSetup->Data); ++i)
    {
        MicrocodeSetup->Data[i] = 0;
    }

    /* Apply user-specified settings */
    if (Info->IntDelayOffset != 0)
    {
        MicrocodeSetup->Data[Info->IntDelayOffset] &= htole32(0xFFFF0000);
        MicrocodeSetup->Data[Info->IntDelayOffset] |= htole32(Adapter->MicrocodeInterruptDelay);
    }
    if (Info->BundleMaxOffset != 0)
    {
        MicrocodeSetup->Data[Info->BundleMaxOffset] &= htole32(0xFFFF0000);
        MicrocodeSetup->Data[Info->BundleMaxOffset] |= htole32(Adapter->MicrocodeMaxFramesPerIntr);
    }
    if (Info->MinSizeMaskOffset != 0)
    {
        MicrocodeSetup->Data[Info->MinSizeMaskOffset] &= htole32(0xFFFF0000);
        MicrocodeSetup->Data[Info->MinSizeMaskOffset] |= htole32(Adapter->MicrocodeMinSizeMask);
    }

    return EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_UCODE, &MicrocodeSetup->Header);
}

static
CODE_SEG("PAGE")
BOOLEAN
EeSetupMacAddress(
    _In_ PE100_ADAPTER Adapter)
{
    PFXP_CB_INDIVIDUAL_ADDRESS_SETUP IaSetup;

    PAGED_CODE();

    IaSetup = &Adapter->ControlBlock->IaSetup;
    NdisMoveMemory(IaSetup->MacAddress, Adapter->CurrentMacAddress, ETH_LENGTH_OF_ADDRESS);

    return EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_IAS, &IaSetup->Header);
}

static
CODE_SEG("PAGE")
BOOLEAN
EeStartReceiveUnit(
    _In_ PE100_ADAPTER Adapter)
{
    PE100_RX_CONTEXT RxContext;

    PAGED_CODE();

    RxContext = CONTAINING_RECORD(Adapter->RxContextList.Flink,
                                  E100_RX_CONTEXT,
                                  ListEntry);

    return EeCommandUnitExecuteCommand(Adapter, FXP_SCB_COMMAND_RU_START, RxContext->RfdPhys);
}

static
CODE_SEG("PAGE")
BOOLEAN
EeSetupStatisticCounters(
    _In_ PE100_ADAPTER Adapter)
{
    PULONG CompletionStatus;
    ULONG i, Address;

    PAGED_CODE();

    Address = Adapter->ControlBlockPa + FIELD_OFFSET(E100_CONTROL_BLOCK, Counters);
    if (!EeCommandUnitExecuteCommand(Adapter, FXP_SCB_COMMAND_CU_DUMP_ADR, Address))
        return FALSE;

    CompletionStatus = EeGetDumpCompletionStatus(Adapter, &Adapter->ControlBlock->Counters);
    *CompletionStatus = 0;
    EE_WRITE_BARRIER();

    /* Clear statistics counters since they are corrupted during power down state */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_DUMPRESET);
    for (i = (100 * 1000) / 2; i > 0; i--)
    {
        NdisStallExecution(2);

        KeMemoryBarrierWithoutFence();
        if (*CompletionStatus == htole32(FXP_STATS_DR_COMPLETE))
            break;
    }
    if (i == 0)
    {
        ERR("Unable to reset statistic counters\n");
        return FALSE;
    }

    /* Discard results of the previous dump */
    EE_WRITE_BARRIER();
    *CompletionStatus = 0;

    /* Start another statistics dump */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_COMMAND, FXP_SCB_COMMAND_CU_DUMPRESET);

    return TRUE;
}

NDIS_STATUS
EeUpdateMulticastList(
    _In_ PE100_ADAPTER Adapter)
{
    PFXP_CB_MULTICAST_SETUP MulticastSetup;
    ULONG ListSize;

    NdisAcquireSpinLock(&Adapter->SendLock);

    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
    {
        ListSize = Adapter->MulticastListSize;
    }
    else
    {
        /*
         * Writing a zero count will reset the hash table
         * and disable the multicast filtering mechanism.
         */
        ListSize = 0;
    }

    MulticastSetup = &Adapter->ControlBlock->MulticastSetup;
    MulticastSetup->Count = htole16(ListSize);
    NdisMoveMemory(MulticastSetup->MacAddress, Adapter->MulticastList, ListSize);

    EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_MCAS, &MulticastSetup->Header);

    NdisReleaseSpinLock(&Adapter->SendLock);

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
EeApplyPacketFilter(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG PacketFilter)
{
    PFXP_CB_CONFIGURE Config;
    ULONG OldPacketFilter;

    INFO("Packet filter value 0x%lx\n", PacketFilter);

    OldPacketFilter = Adapter->PacketFilter;

    if (PacketFilter == OldPacketFilter)
        return NDIS_STATUS_SUCCESS;

    Adapter->PacketFilter = PacketFilter;

    if ((OldPacketFilter ^ PacketFilter) & NDIS_PACKET_TYPE_MULTICAST)
    {
        EeUpdateMulticastList(Adapter);
    }

    NdisAcquireSpinLock(&Adapter->SendLock);

    Config = &Adapter->ControlBlock->Configuration;
    Config->PromiscuousMode = !!(PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS);
    Config->BroadcastDisable = !(PacketFilter & NDIS_PACKET_TYPE_BROADCAST);
    Config->MulticastAll = !!((PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST) ||
                              (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS));

    Config->SaveBadFrames = !!((Adapter->Flags & E100_FLAG_SAVE_BAD_PACKETS) ||
                               (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS));
    Config->DiscardShortReceive = !(PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS);
    Config->StrippingEnable = !(PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS);
    Config->VlanTagStrippingEnable = !!((Adapter->Flags & E100_FLAG_VLAN_TAGGING) &&
                                        !(PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS));

    EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_CONFIG, &Config->Header);

    NdisReleaseSpinLock(&Adapter->SendLock);

    return NDIS_STATUS_SUCCESS;
}

static
VOID
EeSetupFlowControl(
    _Out_ PFXP_CB_CONFIGURE Config,
    _In_ ULONG FlowControl)
{
    switch (FlowControl)
    {
        case E100_MEDIA_PAUSE_TX:
            Config->FcDelayLsb = 0x00;
            Config->FcDelayMsb = 0x40;
            Config->TxFullDuplexFlowControlDisable = 0;
            Config->RxFullDuplexRestopFlowControl = 0;
            Config->RxFullDuplexRestartFlowControl = 0;
            Config->RejectFc = 1;
            break;

        case E100_MEDIA_PAUSE_RX:
            Config->FcDelayLsb = 0x1F;
            Config->FcDelayMsb = 0x01;
            Config->TxFullDuplexFlowControlDisable = 1;
            Config->RxFullDuplexRestopFlowControl = 1;
            Config->RxFullDuplexRestartFlowControl = 1;
            Config->RejectFc = 1;
            break;

        case E100_MEDIA_PAUSE_TX | E100_MEDIA_PAUSE_RX:
            Config->FcDelayLsb = 0x1F;
            Config->FcDelayMsb = 0x01;
            Config->TxFullDuplexFlowControlDisable = 0;
            Config->RxFullDuplexRestopFlowControl = 1;
            Config->RxFullDuplexRestartFlowControl = 1;
            Config->RejectFc = 1;
            break;

        default:
            Config->FcDelayLsb = 0x00;
            Config->FcDelayMsb = 0x40;
            Config->TxFullDuplexFlowControlDisable = 1;
            Config->RxFullDuplexRestopFlowControl = 0;
            Config->RxFullDuplexRestartFlowControl = 0;
            Config->RejectFc = 0;
            break;
    }
}

static
VOID
EeSetSpeedAndDuplex(
    _In_ PE100_ADAPTER Adapter)
{
    PFXP_CB_CONFIGURE Config = &Adapter->ControlBlock->Configuration;

    EeSetupFlowControl(Config, Adapter->CurrentMedia & (E100_MEDIA_PAUSE_TX | E100_MEDIA_PAUSE_RX));

    EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_CONFIG, &Config->Header);
}

BOOLEAN
NTAPI
MiniportCheckForHang(
    _In_ NDIS_HANDLE MiniportAdapterContext)
{
    PE100_ADAPTER Adapter = MiniportAdapterContext;
    UCHAR OldMedia, NewMedia;

    if (!Adapter->AdapterActive)
        return FALSE;

    if (Adapter->HardError)
    {
        ERR("Hard error detected %08lX\n", CSR_READ_32(Adapter, FXP_CSR_SCB_RUSCUS));
        return TRUE;
    }

    /*
     * The hardware does not allow to access statistics counters
     * without doing a fairly expensive DMA to get all of the statistics it maintains,
     * so we do this operation here every 2 seconds in the background by a timer tick.
     */
    EeDumpStatisticsCounters(Adapter);

    NewMedia = EePhyGetSpeedAndDuplex(Adapter);
    OldMedia = Adapter->CurrentMedia;
    if (OldMedia != NewMedia)
    {
        INFO_VERB("Configuring MAC from %u %s-duplex to %u %s-duplex\n",
                  (OldMedia & E100_MEDIA_100T) ? 100 : 10,
                  (OldMedia & E100_MEDIA_FD) ? "full" : "half",
                  (NewMedia & E100_MEDIA_100T) ? 100 : 10,
                  (NewMedia & E100_MEDIA_FD) ? "full" : "half");

        NdisAcquireSpinLock(&Adapter->SendLock);

        Adapter->CurrentMedia = NewMedia;
        EeSetSpeedAndDuplex(Adapter);

        NdisReleaseSpinLock(&Adapter->SendLock);

        if (OldMedia == E100_MEDIA_NONE || NewMedia == E100_MEDIA_NONE)
        {
            INFO_VERB("Link %sconnected, media is 0x%02X\n",
                      (NewMedia != E100_MEDIA_NONE) ? "" : "dis",
                      NewMedia);

            NdisMIndicateStatus(Adapter->AdapterHandle,
                                (NewMedia != E100_MEDIA_NONE) ?
                                NDIS_STATUS_MEDIA_CONNECT :
                                NDIS_STATUS_MEDIA_DISCONNECT,
                                NULL,
                                0);
            NdisMIndicateStatusComplete(Adapter->AdapterHandle);
        }
    }

    return FALSE;
}

static
CODE_SEG("PAGE")
VOID
EeConfigureAdapter(
    _In_ E100_ADAPTER* __restrict Adapter,
    _In_ FXP_CB_CONFIGURE* __restrict Config)
{
    PAGED_CODE();

    NdisZeroMemory(Config, sizeof(*Config));
    /* 0 */
    Config->ByteCount = Adapter->Flags & E100_FLAG_EXT_RFA ? 32 : 22;
    /* 1 */
    Config->RxFifoLimit = 8; // 32 bytes
    Config->TxFifoLimit = 0;
    /* 3 */
    Config->MwiEnable = !!(Adapter->Flags & E100_FLAG_WMI_ENANLE);
    /* 6 */
    Config->DirectDmaDisable = 1;
    Config->TnoInterruptOrTcoStat = 0;
    Config->CiInterrupt = 1; // Interrupt on CU idle
    Config->ExtTxCbDisable = !(Adapter->Flags & E100_FLAG_EXT_TXCB);
    Config->ExtStatsDisable = 1; // No extended counters
    Config->PassOverrunRx = 0; // Do not pass overrun frames to host
    Config->SaveBadFrames = !!(Adapter->Flags & E100_FLAG_SAVE_BAD_PACKETS);
    /* 7 */
    Config->DiscardShortReceive = 1; // Discard short packets
    Config->UnderunRetries = 1; // One re-transmission on DMA underrun
    Config->DynamicTbd = !!(Adapter->Flags & E100_FLAG_EXT_RFA);
    Config->ExtRfa = !!(Adapter->Flags & E100_FLAG_EXT_RFA);
    /* 8 */
    Config->MiiMode = !(Adapter->PhyType == E100_PHY_TYPE_503);
    /* 9 */
    Config->TcpUdpChecksum = 0; // TODO
    /* 10 */
    Config->Byte10_1 = 1;
    Config->Byte10_2 = 1;
    Config->Nsai = 1; // No SA insertion
    Config->PreambleLength = 2; // 7 bytes
    /* 12 */
    Config->InterframeSpacing = 96 / 16; // 96 bits
    /* 14 */
    Config->IpAddressHigh = 0xF2;
    /* 15 */
    Config->PromiscuousMode = 0;
    Config->BroadcastDisable = 0;
    Config->Byte15_3 = 1;
    Config->Byte15_6 = 1;
    Config->CrsCdt = !!(Adapter->PhyType == E100_PHY_TYPE_503);
    /* 18 */
    Config->StrippingEnable = 1; // Truncate RX packet to byte count
    Config->PaddingEnable = 1; // Pad short TX packets
    Config->LongRxOk = !!(Adapter->Flags & E100_FLAG_LONG_PKT);
    Config->Byte18_7 = 1;
    /* 19 */
    Config->MagicPacketWakeDisable = !(Adapter->Flags & E100_FLAG_HAS_WOL);
    Config->ForceFullDuplex = 0;
    Config->AutomaticFullDuplex = 1; // Enable FDX# pin
    /* 20 */
    Config->Byte20_0 = 1;
    Config->Byte20_1 = 1;
    Config->Byte20_2 = 1;
    Config->Byte20_3 = 1;
    Config->Byte20_4 = 1;
    Config->PriorityFcLocation = 1; // Priority field in byte 31
    /* 21 */
    Config->Byte21_0 = 1;
    Config->Byte21_1 = 1;
    Config->MulticastAll = 0;
    /* 22 */
    Config->GamlaRx = !!(Adapter->Flags & E100_FLAG_EXT_RFA);
    Config->VlanTagStrippingEnable = !!(Adapter->Flags & E100_FLAG_VLAN_TAGGING);

    if (Adapter->DefaultMedia & E100_MEDIA_AUTO)
    {
        Config->ForceFullDuplex = 0;
        Config->AutomaticFullDuplex = 1;
    }
    else
    {
        if (Adapter->DefaultMedia & E100_MEDIA_FD)
        {
            Config->ForceFullDuplex = 1;
            Config->AutomaticFullDuplex = 1;
        }
        else
        {
            Config->ForceFullDuplex = 0;
            Config->AutomaticFullDuplex = !(Adapter->RevisionID == FXP_REV_82557);
        }
    }

    if (Adapter->Flags & E100_FLAG_HAS_FLOW_CONTROL)
    {
        Config->PriorityFcThreshold = 3;

        EeSetupFlowControl(Config, 0);
    }
    else
    {
        /* No hardware flow control */
        Config->PriorityFcThreshold = 7;
        Config->FcDelayLsb = 0;
        Config->FcDelayMsb = 0x40;
        Config->TxFullDuplexFlowControlDisable = 0;
        Config->RxFullDuplexRestopFlowControl = 0;
        Config->RxFullDuplexRestartFlowControl = 0;
        Config->RejectFc = 0;
    }

    /* Enable 82558 and 82559 extended statistics functionality */
    if (Adapter->RevisionID >= FXP_REV_82558_A4)
    {
        if (Adapter->RevisionID >= FXP_REV_82559_A0)
        {
            /* Extend configuration table size to 32 to include TCO configuration */
            Config->ByteCount = 32;
            Config->ExtStatsDisable = 1;

            /* Enable TCO statistics */
            Config->TnoInterruptOrTcoStat = 1;
            Config->GamlaRx = 1;
        }
        else
        {
            Config->ExtStatsDisable = 0;
        }
    }

    // TODO: Byte 9 wake on link 0x20
}

CODE_SEG("PAGE")
VOID
EeStartAdapter(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    Adapter->AdapterActive = TRUE;

    /* Enable interrupts */
    CSR_WRITE_8(Adapter, FXP_CSR_SCB_INTRCNTL, 0);
}

static
CODE_SEG("PAGE")
VOID
EeInitTxRing(
    _In_ PE100_ADAPTER Adapter)
{
    PAGED_CODE();

    Adapter->TxLast = &Adapter->TxContext[Adapter->TcbCount - 1];
    Adapter->TxFirst = Adapter->TxLast->Next;
    Adapter->TxPending = 0;

    InitializeListHead(&Adapter->SendQueueList);
}

CODE_SEG("PAGE")
NDIS_STATUS
EeSetupAdapter(
    _In_ PE100_ADAPTER Adapter)
{
    PFXP_CB_CONFIGURE Config;
    ULONG i;

    PAGED_CODE();

    // TODO: Move this to reset?
    Adapter->HardError = FALSE;

    Adapter->ReceiverUnitIdleTicks = 0;
    Adapter->CommandUnitActive = FALSE;
    Adapter->CurrentMedia = E100_MEDIA_NONE;

    // TODO: Clear the WOL events here?

    EeInitTxRing(Adapter);

    EePhyInit(Adapter);

    if (!EeCommandUnitExecuteCommand(Adapter, FXP_SCB_COMMAND_CU_BASE, 0))
        return NDIS_STATUS_HARD_ERRORS;

    if (!EeCommandUnitExecuteCommand(Adapter, FXP_SCB_COMMAND_RU_BASE, 0))
        return NDIS_STATUS_HARD_ERRORS;

    if (!EeDownloadMicrocode(Adapter))
        return NDIS_STATUS_HARD_ERRORS;

    if (Adapter->DefaultMedia & E100_MEDIA_PAUSE_RX)
    {
        /* Set pause RX FIFO threshold to 1KB */
        CSR_WRITE_8(Adapter, FXP_CSR_FC_THRESH, 1);
    }

    Config = &Adapter->ControlBlock->Configuration;
    EeConfigureAdapter(Adapter, Config);
    for (i = 0; i < Config->ByteCount; ++i)
    {
        INFO("Config %2lu: 0x%02X\n", i, ((PUCHAR)Config)[i + sizeof(Config->Header)]);
    }
    if (!EeCommandUnitSendCommand(Adapter, FXP_CB_COMMAND_CONFIG, &Config->Header))
        return NDIS_STATUS_HARD_ERRORS;

    if (!EeSetupMacAddress(Adapter))
        return NDIS_STATUS_HARD_ERRORS;

    if (!EeSetupStatisticCounters(Adapter))
        return NDIS_STATUS_HARD_ERRORS;

    if (!EeStartReceiveUnit(Adapter))
        return NDIS_STATUS_HARD_ERRORS;

    // TODO: Send multicast to restore filter on power up?

    return NDIS_STATUS_SUCCESS;
}
