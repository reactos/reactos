/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport information callbacks
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "nvnet.h"

#define NDEBUG
#include "debug.h"

/* GLOBALS ********************************************************************/

static const NDIS_OID NvpSupportedOidList[] =
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_CURRENT_PACKET_FILTER,
    OID_GEN_HARDWARE_STATUS,
    OID_GEN_MEDIA_SUPPORTED,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_FRAME_SIZE,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_LINK_SPEED,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_GEN_VLAN_ID,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MULTICAST_LIST,
    OID_802_3_MAXIMUM_LIST_SIZE,

    /* Statistics */
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_DIRECTED_FRAMES_RCV,
    OID_GEN_RCV_CRC_ERROR,
    OID_GEN_TRANSMIT_QUEUE_LENGTH,
    OID_802_3_RCV_ERROR_ALIGNMENT,
    OID_802_3_XMIT_ONE_COLLISION,
    OID_802_3_XMIT_MORE_COLLISIONS,
    OID_802_3_XMIT_DEFERRED,
    OID_802_3_XMIT_MAX_COLLISIONS,
    OID_802_3_RCV_OVERRUN,
    OID_802_3_XMIT_UNDERRUN,
    OID_802_3_XMIT_HEARTBEAT_FAILURE,
    OID_802_3_XMIT_TIMES_CRS_LOST,
    OID_802_3_XMIT_LATE_COLLISIONS,

    /* Offload */
    OID_TCP_TASK_OFFLOAD,

    /* Power management */
    OID_PNP_CAPABILITIES,
    OID_PNP_SET_POWER,
    OID_PNP_QUERY_POWER,
    OID_PNP_ADD_WAKE_UP_PATTERN,
    OID_PNP_REMOVE_WAKE_UP_PATTERN,
    OID_PNP_ENABLE_WAKE_UP
};

/* FUNCTIONS ******************************************************************/

static
ULONG
NvNetGetLinkSpeed(
    _In_ PNVNET_ADAPTER Adapter)
{
    ULONG LinkSpeedMbps;

    switch (Adapter->LinkSpeed)
    {
        case NVREG_LINKSPEED_10:
            LinkSpeedMbps = 10;
            break;
        case NVREG_LINKSPEED_100:
            LinkSpeedMbps = 100;
            break;
        case NVREG_LINKSPEED_1000:
            LinkSpeedMbps = 1000;
            break;

        default:
            UNREACHABLE;
            break;
    }

    return LinkSpeedMbps;
}

static
ULONG
PacketFilterToMask(
    _In_ ULONG PacketFilter)
{
    ULONG FilterMask = NVREG_PFF_ALWAYS | NVREG_PFF_MYADDR | NVREG_PFF_PROMISC;

    if (PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        FilterMask &= ~NVREG_PFF_MYADDR;
    }

    if (PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
    {
        FilterMask &= ~NVREG_PFF_PROMISC;
    }

    return FilterMask;
}

static
DECLSPEC_NOINLINE /* Called from pageable code */
VOID
NvNetApplyPacketFilter(
    _In_ PNVNET_ADAPTER Adapter)
{
    UCHAR Address[ETH_LENGTH_OF_ADDRESS];
    UCHAR Mask[ETH_LENGTH_OF_ADDRESS];
    ULONG FilterMask;
    BOOLEAN RestartReceiver;

    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_ALL_MULTICAST)
    {
        NdisZeroMemory(Address, sizeof(Address));
        NdisZeroMemory(Mask, sizeof(Mask));

        Address[0] |= NVREG_MCASTADDRA_FORCE;
        Mask[0] |= NVREG_MCASTADDRA_FORCE;
    }
    else if (Adapter->PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
    {
        if (Adapter->MulticastListSize > 0)
        {
            ULONG i, j;

            NdisFillMemory(Address, sizeof(Address), 0xFF);
            NdisFillMemory(Mask, sizeof(Mask), 0xFF);

            for (i = 0; i < Adapter->MulticastListSize; ++i)
            {
                PUCHAR MacAddress = Adapter->MulticastList[i].MacAddress;

                for (j = 0; j < ETH_LENGTH_OF_ADDRESS; ++j)
                {
                    Address[j] &= MacAddress[j];
                    Mask[j] &= ~MacAddress[j];
                }
            }

            for (j = 0; j < ETH_LENGTH_OF_ADDRESS; ++j)
            {
                Mask[j] |= Address[j];
            }
        }
        else
        {
            NdisZeroMemory(Address, sizeof(Address));
            NdisZeroMemory(Mask, sizeof(Mask));
        }
    }
    else
    {
        NdisZeroMemory(Address, sizeof(Address));
        NdisFillMemory(Mask, sizeof(Mask), 0xFF);
    }

    FilterMask = NV_READ(Adapter, NvRegPacketFilterFlags) & NVREG_PFF_PAUSE_RX;
    FilterMask |= PacketFilterToMask(Adapter->PacketFilter);

    NdisAcquireSpinLock(&Adapter->Receive.Lock);

    RestartReceiver = !!(NV_READ(Adapter, NvRegReceiverControl) & NVREG_RCVCTL_START);
    if (RestartReceiver)
    {
        NvNetStopReceiver(Adapter);
    }

    NV_WRITE(Adapter, NvRegMulticastAddrA,
             Address[3] << 24 | Address[2] << 16 | Address[1] << 8 | Address[0]);
    NV_WRITE(Adapter, NvRegMulticastAddrB, Address[5] << 8 | Address[4]);
    NV_WRITE(Adapter, NvRegMulticastMaskA,
             Mask[3] << 24 | Mask[2] << 16 | Mask[1] << 8 | Mask[0]);
    NV_WRITE(Adapter, NvRegMulticastMaskB, Mask[5] << 8 | Mask[4]);

    NV_WRITE(Adapter, NvRegPacketFilterFlags, FilterMask);

    if (RestartReceiver)
    {
        NvNetStartReceiver(Adapter);
    }

    NdisReleaseSpinLock(&Adapter->Receive.Lock);
}

static
VOID
NvNetReadStatistics(
    _In_ PNVNET_ADAPTER Adapter)
{
    Adapter->Statistics.HwTxCnt += NV_READ(Adapter, NvRegTxCnt);
    Adapter->Statistics.HwTxZeroReXmt += NV_READ(Adapter, NvRegTxZeroReXmt);
    Adapter->Statistics.HwTxOneReXmt += NV_READ(Adapter, NvRegTxOneReXmt);
    Adapter->Statistics.HwTxManyReXmt += NV_READ(Adapter, NvRegTxManyReXmt);
    Adapter->Statistics.HwTxLateCol += NV_READ(Adapter, NvRegTxLateCol);
    Adapter->Statistics.HwTxUnderflow += NV_READ(Adapter, NvRegTxUnderflow);
    Adapter->Statistics.HwTxLossCarrier += NV_READ(Adapter, NvRegTxLossCarrier);
    Adapter->Statistics.HwTxExcessDef += NV_READ(Adapter, NvRegTxExcessDef);
    Adapter->Statistics.HwTxRetryErr += NV_READ(Adapter, NvRegTxRetryErr);
    Adapter->Statistics.HwRxFrameErr += NV_READ(Adapter, NvRegRxFrameErr);
    Adapter->Statistics.HwRxExtraByte += NV_READ(Adapter, NvRegRxExtraByte);
    Adapter->Statistics.HwRxLateCol += NV_READ(Adapter, NvRegRxLateCol);
    Adapter->Statistics.HwRxRunt += NV_READ(Adapter, NvRegRxRunt);
    Adapter->Statistics.HwRxFrameTooLong += NV_READ(Adapter, NvRegRxFrameTooLong);
    Adapter->Statistics.HwRxOverflow += NV_READ(Adapter, NvRegRxOverflow);
    Adapter->Statistics.HwRxFCSErr += NV_READ(Adapter, NvRegRxFCSErr);
    Adapter->Statistics.HwRxFrameAlignErr += NV_READ(Adapter, NvRegRxFrameAlignErr);
    Adapter->Statistics.HwRxLenErr += NV_READ(Adapter, NvRegRxLenErr);
    Adapter->Statistics.HwRxUnicast += NV_READ(Adapter, NvRegRxUnicast);
    Adapter->Statistics.HwRxMulticast += NV_READ(Adapter, NvRegRxMulticast);
    Adapter->Statistics.HwRxBroadcast += NV_READ(Adapter, NvRegRxBroadcast);

    if (Adapter->Features & DEV_HAS_STATISTICS_V2)
    {
        Adapter->Statistics.HwTxDef += NV_READ(Adapter, NvRegTxDef);
        Adapter->Statistics.HwTxFrame += NV_READ(Adapter, NvRegTxFrame);
        Adapter->Statistics.HwRxCnt += NV_READ(Adapter, NvRegRxCnt);
        Adapter->Statistics.HwTxPause += NV_READ(Adapter, NvRegTxPause);
        Adapter->Statistics.HwRxPause += NV_READ(Adapter, NvRegRxPause);
        Adapter->Statistics.HwRxDropFrame += NV_READ(Adapter, NvRegRxDropFrame);
    }

    if (Adapter->Features & DEV_HAS_STATISTICS_V3)
    {
        Adapter->Statistics.HwTxUnicast += NV_READ(Adapter, NvRegTxUnicast);
        Adapter->Statistics.HwTxMulticast += NV_READ(Adapter, NvRegTxMulticast);
        Adapter->Statistics.HwTxBroadcast += NV_READ(Adapter, NvRegTxBroadcast);
    }
}

static
VOID
NvNetQueryHwCounter(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NDIS_OID Oid,
    _Out_ PULONG64 Counter)
{
    switch (Oid)
    {
        case OID_GEN_XMIT_OK:
            *Counter = (Adapter->Features & DEV_HAS_STATISTICS_V2)
                       ? Adapter->Statistics.HwTxFrame
                       : (Adapter->Statistics.HwTxZeroReXmt +
                          Adapter->Statistics.HwTxOneReXmt +
                          Adapter->Statistics.HwTxManyReXmt);
            break;
        case OID_GEN_RCV_OK:
            *Counter = (Adapter->Statistics.HwRxUnicast +
                        Adapter->Statistics.HwRxMulticast +
                        Adapter->Statistics.HwRxBroadcast);
            break;
        case OID_GEN_XMIT_ERROR:
            *Counter = (Adapter->Statistics.HwTxRetryErr +
                        Adapter->Statistics.HwTxLateCol +
                        Adapter->Statistics.HwTxUnderflow +
                        Adapter->Statistics.HwTxLossCarrier +
                        Adapter->Statistics.HwTxExcessDef);
            break;
        case OID_GEN_RCV_ERROR:
            *Counter = (Adapter->Statistics.HwRxFrameAlignErr +
                        Adapter->Statistics.HwRxLenErr +
                        Adapter->Statistics.HwRxRunt +
                        Adapter->Statistics.HwRxFrameTooLong +
                        Adapter->Statistics.HwRxFCSErr +
                        Adapter->Statistics.HwRxFrameErr +
                        Adapter->Statistics.HwRxExtraByte +
                        Adapter->Statistics.HwRxLateCol);
            break;
        case OID_GEN_RCV_NO_BUFFER:
            *Counter = (Adapter->Statistics.HwRxDropFrame +
                        Adapter->Statistics.HwRxOverflow +
                        Adapter->Statistics.ReceiveIrqNoBuffers);
            break;
        case OID_GEN_DIRECTED_FRAMES_RCV:
            *Counter = Adapter->Statistics.HwRxUnicast;
            break;
        case OID_GEN_RCV_CRC_ERROR:
            *Counter = Adapter->Statistics.HwRxFCSErr;
            break;
        case OID_802_3_RCV_ERROR_ALIGNMENT:
            *Counter = Adapter->Statistics.HwRxFrameErr;
            break;
        case OID_802_3_XMIT_ONE_COLLISION:
            *Counter = Adapter->Statistics.HwTxOneReXmt;
            break;
        case OID_802_3_XMIT_MORE_COLLISIONS:
            *Counter = Adapter->Statistics.HwTxManyReXmt;
            break;
        case OID_802_3_XMIT_DEFERRED:
            *Counter = Adapter->Statistics.HwTxDef;
            break;
        case OID_802_3_XMIT_MAX_COLLISIONS:
            *Counter = Adapter->Statistics.HwTxRetryErr;
            break;
        case OID_802_3_RCV_OVERRUN:
            *Counter = Adapter->Statistics.HwRxOverflow;
            break;
        case OID_802_3_XMIT_UNDERRUN:
            *Counter = Adapter->Statistics.HwTxUnderflow;
            break;
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
            *Counter = Adapter->Statistics.HwTxZeroReXmt;
            break;
        case OID_802_3_XMIT_TIMES_CRS_LOST:
            *Counter = Adapter->Statistics.HwTxLossCarrier;
            break;
        case OID_802_3_XMIT_LATE_COLLISIONS:
            *Counter = Adapter->Statistics.HwTxLateCol;
            break;

        default:
            UNREACHABLE;
            break;
    }
}

static
VOID
NvNetQuerySoftwareCounter(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NDIS_OID Oid,
    _Out_ PULONG64 Counter)
{
    switch (Oid)
    {
        case OID_GEN_XMIT_OK:
            *Counter = Adapter->Statistics.TransmitOk;
            break;
        case OID_GEN_RCV_OK:
            *Counter = Adapter->Statistics.ReceiveOk;
            break;
        case OID_GEN_XMIT_ERROR:
            *Counter = Adapter->Statistics.TransmitErrors;
            break;
        case OID_GEN_RCV_ERROR:
            *Counter = Adapter->Statistics.ReceiveErrors;
            break;
        case OID_GEN_RCV_NO_BUFFER:
            *Counter = Adapter->Statistics.ReceiveNoBuffers;
            break;
        case OID_GEN_DIRECTED_FRAMES_RCV:
            *Counter = 0;
            break;
        case OID_GEN_RCV_CRC_ERROR:
            *Counter = Adapter->Statistics.ReceiveCrcErrors;
            break;
        case OID_802_3_RCV_ERROR_ALIGNMENT:
            *Counter = Adapter->Statistics.ReceiveAlignmentErrors;
            break;
        case OID_802_3_XMIT_ONE_COLLISION:
            *Counter = Adapter->Statistics.TransmitOneRetry;
            break;
        case OID_802_3_XMIT_MORE_COLLISIONS:
            *Counter = (Adapter->Statistics.TransmitOk -
                        Adapter->Statistics.TransmitOneRetry -
                        Adapter->Statistics.TransmitZeroRetry);
            break;
        case OID_802_3_XMIT_DEFERRED:
            *Counter = Adapter->Statistics.TransmitDeferred;
            break;
        case OID_802_3_XMIT_MAX_COLLISIONS:
            *Counter = Adapter->Statistics.TransmitExcessiveCollisions;
            break;
        case OID_802_3_RCV_OVERRUN:
            *Counter = Adapter->Statistics.ReceiveOverrunErrors;
            break;
        case OID_802_3_XMIT_UNDERRUN:
            *Counter = Adapter->Statistics.TransmitUnderrunErrors;
            break;
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
            *Counter = Adapter->Statistics.TransmitZeroRetry;
            break;
        case OID_802_3_XMIT_TIMES_CRS_LOST:
            *Counter = Adapter->Statistics.TransmitLostCarrierSense;
            break;
        case OID_802_3_XMIT_LATE_COLLISIONS:
            *Counter = Adapter->Statistics.TransmitLateCollisions;
            break;

        default:
            UNREACHABLE;
            break;
    }
}

static
NDIS_STATUS
NvNetFillPowerManagementCapabilities(
    _In_ PNVNET_ADAPTER Adapter,
    _Out_ PNDIS_PNP_CAPABILITIES Capabilities)
{
    Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp =
    Capabilities->WakeUpCapabilities.MinPatternWakeUp =
    Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateD3;

    /* All hardware is PM-aware */
    return NDIS_STATUS_SUCCESS;
}

static
ULONG
BuildFrameSignature(
    _In_ PNVNET_WAKE_FRAME WakeFrame)
{
    ULONG i, j, Crc;

    Crc = 0xFFFFFFFF;
    for (i = 0; i < sizeof(WakeFrame->WakeUpPattern); ++i)
    {
        if (WakeFrame->PatternMask.AsUCHAR[i / 8] & (1 << (i % 8)))
        {
            Crc ^= WakeFrame->WakeUpPattern[i];
            for (j = 8; j > 0; --j)
            {
                Crc = (Crc >> 1) ^ (-(LONG)(Crc & 1) & 0xEDB88320);
            }
        }
    }

    return ~Crc;
}

static
VOID
WriteWakeFrame(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNVNET_WAKE_FRAME WakeFrame,
    _In_ ULONG FrameNumber)
{
    ULONG Offset = FrameNumber * 5 * sizeof(ULONG);

    if (FrameNumber >= NV_WAKEUPPATTERNS)
    {
        Offset += NV_PATTERN_V2_OFFSET;
    }

    NV_WRITE(Adapter, NvRegPatternCrc + Offset, BuildFrameSignature(WakeFrame));
    NV_WRITE(Adapter, NvRegPatternMask0 + Offset, WakeFrame->PatternMask.AsULONG[0]);
    NV_WRITE(Adapter, NvRegPatternMask1 + Offset, WakeFrame->PatternMask.AsULONG[1]);
    NV_WRITE(Adapter, NvRegPatternMask2 + Offset, WakeFrame->PatternMask.AsULONG[2]);
    NV_WRITE(Adapter, NvRegPatternMask3 + Offset, WakeFrame->PatternMask.AsULONG[3]);
}

static
ULONG
FrameNumberToWakeUpMask(
    _In_ ULONG FrameNumber)
{
    if (FrameNumber < 5)
        return 0x10000 << FrameNumber;
    else
        return 0;
}

static
ULONG
FrameNumberToPowerMask(
    _In_ ULONG FrameNumber)
{
    switch (FrameNumber)
    {
        case 5:
            return NVREG_POWERSTATE2_WAKEUPPAT_5;
        case 6:
            return NVREG_POWERSTATE2_WAKEUPPAT_6;
        case 7:
            return NVREG_POWERSTATE2_WAKEUPPAT_7;

        default:
            return 0;
    }
}

VOID
NvNetSetPowerState(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NDIS_DEVICE_POWER_STATE NewPowerState,
    _In_ ULONG WakeFlags)
{
    ULONG i, PowerState, PowerState2, WakeUpFlags;

    NV_READ(Adapter, NvRegPowerCap);

    WakeUpFlags = 0;
    PowerState2 = 0;
    if (Adapter->Features & DEV_HAS_POWER_CNTRL)
    {
        PowerState2 = NV_READ(Adapter, NvRegPowerState2);
        PowerState2 &= ~(NVREG_POWERSTATE2_WAKEUPPAT_5 |
                         NVREG_POWERSTATE2_WAKEUPPAT_6 |
                         NVREG_POWERSTATE2_WAKEUPPAT_7);
    }

    if (NewPowerState != NdisDeviceStateD0)
    {
        ULONG FramesEnabled = 0;

        if (WakeFlags & NDIS_PNP_WAKE_UP_PATTERN_MATCH)
            WakeUpFlags |= NVREG_WAKEUPFLAGS_ENABLE_MAGPAT;
        if (WakeFlags & NDIS_PNP_WAKE_UP_LINK_CHANGE)
            WakeUpFlags |= NVREG_WAKEUPFLAGS_ENABLE_LINKCHANGE;
        if (WakeFlags & NDIS_PNP_WAKE_UP_MAGIC_PACKET)
        {
            WakeUpFlags |= NVREG_WAKEUPFLAGS_ENABLE_WAKEUPPAT;

            for (i = 0; i < RTL_NUMBER_OF(Adapter->WakeFrames); ++i)
            {
                PNVNET_WAKE_FRAME WakeFrame = Adapter->WakeFrames[i];

                if (!WakeFrame)
                    continue;

                WriteWakeFrame(Adapter, WakeFrame, i);

                PowerState2 |= FrameNumberToPowerMask(i);
                WakeUpFlags |= FrameNumberToWakeUpMask(i);

                ++FramesEnabled;
            }
        }

        if (WakeUpFlags)
        {
            if (!(Adapter->Flags & NV_MAC_IN_USE))
            {
                PowerState2 &= ~NVREG_POWERSTATE2_GATE_CLOCKS;
                PowerState2 |= NVREG_POWERSTATE2_GATE_CLOCK_3;

                if (!FramesEnabled && (WakeUpFlags & NVREG_WAKEUPFLAGS_ENABLE_LINKCHANGE))
                    PowerState2 |= NVREG_POWERSTATE2_GATE_CLOCK_1;
                if (FramesEnabled < NV_WAKEUPMASKENTRIES)
                    PowerState2 |= NVREG_POWERSTATE2_GATE_CLOCK_2;
            }

            NvNetStartReceiver(Adapter);
        }
        else
        {
            if (!(Adapter->Flags & NV_MAC_IN_USE))
                PowerState2 |= NVREG_POWERSTATE2_GATE_CLOCKS;
        }
    }

    NdisStallExecution(NV_POWER_STALL);

    NV_WRITE(Adapter, NvRegWakeUpFlags, WakeUpFlags);
    if (Adapter->Features & DEV_HAS_POWER_CNTRL)
    {
        NV_WRITE(Adapter, NvRegPowerState2, PowerState2);
    }

    NV_WRITE(Adapter, NvRegPowerState,
             NV_READ(Adapter, NvRegPowerState) | NVREG_POWERSTATE_POWEREDUP);
    for (i = 0; i < NV_POWER_ATTEMPTS; ++i)
    {
        ULONG State = NV_READ(Adapter, NvRegPowerState);

        if (!(State & NVREG_POWERSTATE_POWEREDUP))
            break;

        NV_WRITE(Adapter, NvRegPowerState, State | NVREG_POWERSTATE_POWEREDUP);

        NdisStallExecution(NV_POWER_DELAY);
    }

    PowerState = NewPowerState - 1;
    if (WakeUpFlags)
    {
        PowerState |= NVREG_POWERSTATE_VALID;
    }
    NV_WRITE(Adapter, NvRegPowerState, PowerState);
}

static
CODE_SEG("PAGE")
VOID
NTAPI
NvNetPowerWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context)
{
    PNVNET_ADAPTER Adapter = Context;

    UNREFERENCED_PARAMETER(WorkItem);

    PAGED_CODE();

    if (Adapter->PowerStatePending == NdisDeviceStateD0)
    {
        NvNetSetPowerState(Adapter, NdisDeviceStateD0, 0);

        NT_VERIFY(NvNetInitNIC(Adapter, TRUE) == NDIS_STATUS_SUCCESS);

        NvNetStartAdapter(Adapter);

        NvNetApplyPacketFilter(Adapter);
    }
    else
    {
        NvNetPauseProcessing(Adapter);

        NvNetStopAdapter(Adapter);

        NvNetIdleTransmitter(Adapter, FALSE);
        NvNetStopTransmitter(Adapter);
        NvNetStopReceiver(Adapter);
        NV_WRITE(Adapter, NvRegTxRxControl,
                 Adapter->TxRxControl | NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET);
        NdisStallExecution(NV_TXRX_RESET_DELAY);

        NvNetFlushTransmitQueue(Adapter, NDIS_STATUS_FAILURE);

        NvNetSetPowerState(Adapter, Adapter->PowerStatePending, Adapter->WakeFlags);
    }

    NdisMSetInformationComplete(Adapter->AdapterHandle, NDIS_STATUS_SUCCESS);
}

static
NDIS_STATUS
NvNetSetPower(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NDIS_DEVICE_POWER_STATE NewPowerState)
{
    Adapter->PowerStatePending = NewPowerState;

    NdisInitializeWorkItem(&Adapter->PowerWorkItem, NvNetPowerWorker, Adapter);
    NdisScheduleWorkItem(&Adapter->PowerWorkItem);

    return NDIS_STATUS_PENDING;
}

static
NDIS_STATUS
NvNetAddWakeUpPattern(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN Pattern)
{
    ULONG FrameNumber;
    NDIS_STATUS Status;
    PNVNET_WAKE_FRAME WakeFrame;

    if (!_BitScanForward(&FrameNumber, Adapter->WakeFrameBitmap))
    {
        return NDIS_STATUS_RESOURCES;
    }

    Status = NdisAllocateMemoryWithTag((PVOID*)&WakeFrame, sizeof(*WakeFrame), NVNET_TAG);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Adapter->WakeFrameBitmap &= ~(1 << FrameNumber);

    NdisZeroMemory(WakeFrame, sizeof(*WakeFrame));
    NdisMoveMemory(&WakeFrame->PatternMask,
                   (PUCHAR)Pattern + sizeof(NDIS_PM_PACKET_PATTERN),
                   min(Pattern->MaskSize, 16));
    NdisMoveMemory(&WakeFrame->WakeUpPattern,
                   (PUCHAR)Pattern + Pattern->PatternOffset,
                   min(Pattern->PatternSize, 128));
    Adapter->WakeFrames[FrameNumber] = WakeFrame;

    /* TODO: VLAN frame translation */

    return NDIS_STATUS_SUCCESS;
}

static
BOOLEAN
NvEqualMemory(
    _In_reads_bytes_(Length) PVOID Destination,
    _In_reads_bytes_(Length) PVOID Source,
    _In_ ULONG Length)
{
    ULONG i;
    PUCHAR Src, Dest;

    Src = Source;
    Dest = Destination;
    for (i = 0; i < Length; ++i)
    {
        if (Src[i] != Dest[i])
            return FALSE;
    }

    return TRUE;
}
/* 'memcmp' is unavailable for some reason */
#undef NdisEqualMemory
#define NdisEqualMemory NvEqualMemory

static
NDIS_STATUS
NvNetRemoveWakeUpPattern(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN Pattern)
{
    ULONG i;

    for (i = 0; i < RTL_NUMBER_OF(Adapter->WakeFrames); ++i)
    {
        PNVNET_WAKE_FRAME WakeFrame = Adapter->WakeFrames[i];

        if (!WakeFrame)
            continue;

        if (!NdisEqualMemory(&WakeFrame->PatternMask,
                             (PUCHAR)Pattern + sizeof(NDIS_PM_PACKET_PATTERN),
                             min(Pattern->MaskSize, 16)))
        {
            continue;
        }

        if (!NdisEqualMemory(&WakeFrame->WakeUpPattern,
                             (PUCHAR)Pattern + Pattern->PatternOffset,
                             min(Pattern->PatternSize, 128)))
        {
            continue;
        }

        NdisFreeMemory(WakeFrame, sizeof(*WakeFrame), 0);

        Adapter->WakeFrameBitmap |= (1 << i);
        Adapter->WakeFrames[i] = NULL;

        return NDIS_STATUS_SUCCESS;
    }

    return NDIS_STATUS_INVALID_DATA;
}

static
ULONG
NvNetGetWakeUp(
    _In_ PNVNET_ADAPTER Adapter)
{
    return Adapter->WakeFlags & (NDIS_PNP_WAKE_UP_MAGIC_PACKET |
                                 NDIS_PNP_WAKE_UP_PATTERN_MATCH |
                                 NDIS_PNP_WAKE_UP_LINK_CHANGE);
}

static
VOID
NvNetEnableWakeUp(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG Flags)
{
    Adapter->WakeFlags = Flags;
}

static
NDIS_STATUS
NvNetGetTcpTaskOffload(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_TASK_OFFLOAD_HEADER TaskOffloadHeader,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesWritten,
    _Out_ PULONG BytesNeeded)
{
    ULONG InfoLength;
    PNDIS_TASK_OFFLOAD TaskOffload;

    if (!(Adapter->Flags & (NV_SEND_CHECKSUM | NV_SEND_LARGE_SEND)))
    {
        *BytesWritten = 0;
        *BytesNeeded = 0;
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    InfoLength = sizeof(NDIS_TASK_OFFLOAD_HEADER);
    if (Adapter->Flags & NV_SEND_CHECKSUM)
    {
        InfoLength += FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) +
                      sizeof(NDIS_TASK_TCP_IP_CHECKSUM);
    }
    if (Adapter->Flags & NV_SEND_LARGE_SEND)
    {
        InfoLength += FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) +
                      sizeof(NDIS_TASK_TCP_LARGE_SEND);
    }

    if (InformationBufferLength < InfoLength)
    {
        *BytesWritten = 0;
        *BytesNeeded = InfoLength;
        return NDIS_STATUS_BUFFER_TOO_SHORT;
    }

    if ((TaskOffloadHeader->EncapsulationFormat.Encapsulation != IEEE_802_3_Encapsulation) &&
        (TaskOffloadHeader->EncapsulationFormat.Encapsulation != UNSPECIFIED_Encapsulation ||
         TaskOffloadHeader->EncapsulationFormat.EncapsulationHeaderSize != sizeof(ETH_HEADER)))
    {
        *BytesWritten = 0;
        *BytesNeeded = 0;
        return NDIS_STATUS_NOT_SUPPORTED;
    }
    if (TaskOffloadHeader->Version != NDIS_TASK_OFFLOAD_VERSION)
    {
        *BytesWritten = 0;
        *BytesNeeded = 0;
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    TaskOffloadHeader->OffsetFirstTask = sizeof(NDIS_TASK_OFFLOAD_HEADER);
    TaskOffload = (PNDIS_TASK_OFFLOAD)(TaskOffloadHeader + 1);
    if (Adapter->Flags & NV_SEND_CHECKSUM)
    {
        PNDIS_TASK_TCP_IP_CHECKSUM ChecksumTask;

        TaskOffload->Size = sizeof(NDIS_TASK_OFFLOAD);
        TaskOffload->Version = NDIS_TASK_OFFLOAD_VERSION;
        TaskOffload->Task = TcpIpChecksumNdisTask;
        TaskOffload->TaskBufferLength = sizeof(NDIS_TASK_TCP_IP_CHECKSUM);
        TaskOffload->OffsetNextTask = FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer) +
                                      sizeof(NDIS_TASK_TCP_IP_CHECKSUM);

        ChecksumTask = (PNDIS_TASK_TCP_IP_CHECKSUM)TaskOffload->TaskBuffer;
        NdisZeroMemory(ChecksumTask, sizeof(*ChecksumTask));

        ChecksumTask->V4Transmit.IpOptionsSupported = 1;
        ChecksumTask->V4Transmit.TcpOptionsSupported = 1;
        ChecksumTask->V4Transmit.TcpChecksum = 1;
        ChecksumTask->V4Transmit.UdpChecksum = 1;
        ChecksumTask->V4Transmit.IpChecksum = 1;

        ChecksumTask->V4Receive.IpOptionsSupported = 1;
        ChecksumTask->V4Receive.TcpOptionsSupported = 1;
        ChecksumTask->V4Receive.TcpChecksum = 1;
        ChecksumTask->V4Receive.UdpChecksum = 1;
        ChecksumTask->V4Receive.IpChecksum = 1;

        TaskOffload = (PNDIS_TASK_OFFLOAD)(ChecksumTask + 1);
    }
    if (Adapter->Flags & NV_SEND_LARGE_SEND)
    {
        PNDIS_TASK_TCP_LARGE_SEND LargeSendTask;

        TaskOffload->Size = sizeof(NDIS_TASK_OFFLOAD);
        TaskOffload->Version = NDIS_TASK_OFFLOAD_VERSION;
        TaskOffload->Task = TcpLargeSendNdisTask;
        TaskOffload->TaskBufferLength = sizeof(NDIS_TASK_TCP_LARGE_SEND);
        TaskOffload->OffsetNextTask = 0;

        LargeSendTask = (PNDIS_TASK_TCP_LARGE_SEND)TaskOffload->TaskBuffer;
        LargeSendTask->Version = NDIS_TASK_TCP_LARGE_SEND_V0;
        LargeSendTask->MinSegmentCount = NVNET_MINIMUM_LSO_SEGMENT_COUNT;
        LargeSendTask->MaxOffLoadSize = NVNET_MAXIMUM_LSO_FRAME_SIZE;
        LargeSendTask->IpOptions = TRUE;
        LargeSendTask->TcpOptions = TRUE;
    }
    TaskOffload->OffsetNextTask = 0;

    *BytesWritten = InfoLength;
    *BytesNeeded = 0;

    return NDIS_STATUS_SUCCESS;
}

static
NDIS_STATUS
NvNetSetTcpTaskOffload(
    _Inout_ PNVNET_ADAPTER Adapter,
    _In_ PNDIS_TASK_OFFLOAD_HEADER TaskOffloadHeader,
    _In_ PULONG BytesRead)
{
    ULONG Offset;
    PNDIS_TASK_OFFLOAD TaskOffload;

    if (TaskOffloadHeader->Version != NDIS_TASK_OFFLOAD_VERSION)
    {
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    Adapter->IpHeaderOffset = TaskOffloadHeader->EncapsulationFormat.EncapsulationHeaderSize;

    TaskOffload = (PNDIS_TASK_OFFLOAD)TaskOffloadHeader;
    Offset = TaskOffloadHeader->OffsetFirstTask;

    while (Offset)
    {
        *BytesRead += FIELD_OFFSET(NDIS_TASK_OFFLOAD, TaskBuffer);

        TaskOffload = (PNDIS_TASK_OFFLOAD)((PUCHAR)TaskOffload + Offset);
        switch (TaskOffload->Task)
        {
            case TcpIpChecksumNdisTask:
            {
                PNDIS_TASK_TCP_IP_CHECKSUM Task;

                *BytesRead += sizeof(NDIS_TASK_TCP_IP_CHECKSUM);

                if (!(Adapter->Flags & NV_SEND_CHECKSUM))
                {
                    return NDIS_STATUS_NOT_SUPPORTED;
                }

                Task = (PNDIS_TASK_TCP_IP_CHECKSUM)TaskOffload->TaskBuffer;

                Adapter->Offload.SendTcpChecksum = Task->V4Transmit.TcpChecksum;
                Adapter->Offload.SendUdpChecksum = Task->V4Transmit.UdpChecksum;
                Adapter->Offload.SendIpChecksum = Task->V4Transmit.IpChecksum;

                Adapter->Offload.ReceiveTcpChecksum = Task->V4Receive.TcpChecksum;
                Adapter->Offload.ReceiveUdpChecksum = Task->V4Receive.UdpChecksum;
                Adapter->Offload.ReceiveIpChecksum = Task->V4Receive.IpChecksum;
                break;
            }

            case TcpLargeSendNdisTask:
            {
                PNDIS_TASK_TCP_LARGE_SEND Task;

                if (!(Adapter->Flags & NV_SEND_LARGE_SEND))
                {
                    return NDIS_STATUS_NOT_SUPPORTED;
                }

                if ((TaskOffloadHeader->
                     EncapsulationFormat.Encapsulation != IEEE_802_3_Encapsulation) &&
                    (TaskOffloadHeader->
                     EncapsulationFormat.Encapsulation != UNSPECIFIED_Encapsulation ||
                     TaskOffloadHeader->
                     EncapsulationFormat.EncapsulationHeaderSize != sizeof(ETH_HEADER)))
                {
                    return NDIS_STATUS_NOT_SUPPORTED;
                }

                *BytesRead += sizeof(NDIS_TASK_TCP_LARGE_SEND);

                Task = (PNDIS_TASK_TCP_LARGE_SEND)TaskOffload->TaskBuffer;

                if (Task->MinSegmentCount != NVNET_MINIMUM_LSO_SEGMENT_COUNT)
                    return NDIS_STATUS_NOT_SUPPORTED;

                if (Task->MaxOffLoadSize > NVNET_MAXIMUM_LSO_FRAME_SIZE)
                    return NDIS_STATUS_NOT_SUPPORTED;

                /* Nothing to do */
                break;
            }

            default:
                break;
        }

        Offset = TaskOffload->OffsetNextTask;
    }

    NdisAcquireSpinLock(&Adapter->Send.Lock);

    if (Adapter->Offload.ReceiveTcpChecksum ||
        Adapter->Offload.ReceiveUdpChecksum ||
        Adapter->Offload.ReceiveIpChecksum)
    {
        Adapter->TxRxControl |= NVREG_TXRXCTL_RXCHECK;
    }
    else
    {
        Adapter->TxRxControl &= ~NVREG_TXRXCTL_RXCHECK;
    }
    NV_WRITE(Adapter, NvRegTxRxControl, Adapter->TxRxControl);

    NdisReleaseSpinLock(&Adapter->Send.Lock);

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
MiniportQueryInformation(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesWritten,
    _Out_ PULONG BytesNeeded)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG InfoLength;
    PVOID InfoPtr;
    union _GENERIC_INFORMATION
    {
        USHORT Ushort;
        ULONG Ulong;
        ULONG64 Ulong64;
        NDIS_MEDIUM Medium;
        NDIS_HARDWARE_STATUS Status;
        NDIS_DEVICE_POWER_STATE PowerState;
    } GenericInfo;

    InfoLength = sizeof(ULONG);
    InfoPtr = &GenericInfo;

    switch (Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            InfoPtr = (PVOID)&NvpSupportedOidList;
            InfoLength = sizeof(NvpSupportedOidList);
            break;

        case OID_GEN_HARDWARE_STATUS:
            InfoLength = sizeof(NDIS_HARDWARE_STATUS);
            GenericInfo.Status = NdisHardwareStatusReady;
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        {
            InfoLength = sizeof(NDIS_MEDIUM);
            GenericInfo.Medium = NdisMedium802_3;
            break;
        }

        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        {
            GenericInfo.Ulong = Adapter->MaximumFrameSize - sizeof(ETH_HEADER);
            break;
        }

        case OID_GEN_MAXIMUM_FRAME_SIZE:
        {
            GenericInfo.Ulong = Adapter->MaximumFrameSize;
            break;
        }

        case OID_GEN_LINK_SPEED:
        {
            GenericInfo.Ulong = NvNetGetLinkSpeed(Adapter) * 10000;
            break;
        }

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
        {
            /* TODO: Change this later, once the driver can handle multipacket sends */
            GenericInfo.Ulong = Adapter->MaximumFrameSize;
            break;
        }

        case OID_GEN_RECEIVE_BUFFER_SPACE:
        {
            GenericInfo.Ulong = Adapter->MaximumFrameSize * NVNET_RECEIVE_DESCRIPTORS;
        }

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_RECEIVE_BLOCK_SIZE:
        {
            GenericInfo.Ulong = Adapter->MaximumFrameSize;
            break;
        }

        case OID_GEN_VENDOR_ID:
        {
            GenericInfo.Ulong = 0;
            GenericInfo.Ulong |= (Adapter->PermanentMacAddress[0] << 16);
            GenericInfo.Ulong |= (Adapter->PermanentMacAddress[1] << 8);
            GenericInfo.Ulong |= (Adapter->PermanentMacAddress[2] & 0xFF);
            break;
        }

        case OID_GEN_DRIVER_VERSION:
        {
            InfoLength = sizeof(USHORT);
            GenericInfo.Ushort = (NDIS_MINIPORT_MAJOR_VERSION << 8) | NDIS_MINIPORT_MINOR_VERSION;
            break;
        }

        case OID_GEN_VENDOR_DESCRIPTION:
        {
            static const CHAR VendorDesc[] = "nVidia nForce Ethernet Controller";
            InfoPtr = (PVOID)&VendorDesc;
            InfoLength = sizeof(VendorDesc);
            break;
        }

        case OID_GEN_CURRENT_PACKET_FILTER:
        {
            GenericInfo.Ulong = Adapter->PacketFilter;
            break;
        }

        case OID_GEN_MAC_OPTIONS:
        {
            GenericInfo.Ulong = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                                NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                                NDIS_MAC_OPTION_NO_LOOPBACK;

            if (Adapter->Flags & NV_PACKET_PRIORITY)
                GenericInfo.Ulong |= NDIS_MAC_OPTION_8021P_PRIORITY;
            if (Adapter->Flags & NV_VLAN_TAGGING)
                GenericInfo.Ulong |= NDIS_MAC_OPTION_8021Q_VLAN;
            break;
        }

        case OID_GEN_MEDIA_CONNECT_STATUS:
        {
            GenericInfo.Ulong = Adapter->Connected ? NdisMediaStateConnected
                                                   : NdisMediaStateDisconnected;
            break;
        }

        case OID_GEN_MAXIMUM_SEND_PACKETS:
        {
            /* TODO: Multipacket sends */
            GenericInfo.Ulong = 1;
            break;
        }

        case OID_GEN_VENDOR_DRIVER_VERSION:
        {
            /* 1.0.0 */
            GenericInfo.Ulong = 0x100;
            break;
        }

        case OID_GEN_XMIT_OK:
        case OID_GEN_RCV_OK:
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
        case OID_GEN_DIRECTED_FRAMES_RCV:
        case OID_GEN_RCV_CRC_ERROR:
        case OID_802_3_RCV_ERROR_ALIGNMENT:
        case OID_802_3_XMIT_ONE_COLLISION:
        case OID_802_3_XMIT_MORE_COLLISIONS:
        case OID_802_3_XMIT_DEFERRED:
        case OID_802_3_XMIT_MAX_COLLISIONS:
        case OID_802_3_RCV_OVERRUN:
        case OID_802_3_XMIT_UNDERRUN:
        case OID_802_3_XMIT_HEARTBEAT_FAILURE:
        case OID_802_3_XMIT_TIMES_CRS_LOST:
        case OID_802_3_XMIT_LATE_COLLISIONS:
        {
            if (Adapter->Features & DEV_HAS_STATISTICS_COUNTERS)
            {
                NvNetReadStatistics(Adapter);
                NvNetQueryHwCounter(Adapter, Oid, &GenericInfo.Ulong64);
            }
            else
            {
                NvNetQuerySoftwareCounter(Adapter, Oid, &GenericInfo.Ulong64);
            }

            *BytesNeeded = sizeof(ULONG64);
            if (InformationBufferLength < sizeof(ULONG))
            {
                *BytesWritten = 0;
                return NDIS_STATUS_BUFFER_TOO_SHORT;
            }
            if (InformationBufferLength >= sizeof(ULONG64))
            {
                *BytesWritten = sizeof(ULONG64);
                NdisMoveMemory(InformationBuffer, InfoPtr, sizeof(ULONG64));
            }
            else
            {
                *BytesWritten = sizeof(ULONG);
                NdisMoveMemory(InformationBuffer, InfoPtr, sizeof(ULONG));
            }

            return NDIS_STATUS_SUCCESS;
        }

        case OID_GEN_TRANSMIT_QUEUE_LENGTH:
        {
            GenericInfo.Ulong = NVNET_TRANSMIT_BLOCKS - Adapter->Send.TcbSlots;
            break;
        }

        case OID_802_3_PERMANENT_ADDRESS:
        {
            InfoPtr = Adapter->PermanentMacAddress;
            InfoLength = ETH_LENGTH_OF_ADDRESS;
            break;
        }

        case OID_802_3_CURRENT_ADDRESS:
        {
            InfoPtr = Adapter->CurrentMacAddress;
            InfoLength = ETH_LENGTH_OF_ADDRESS;
            break;
        }

        case OID_802_3_MULTICAST_LIST:
        {
            InfoPtr = Adapter->MulticastList;
            InfoLength = Adapter->MulticastListSize * ETH_LENGTH_OF_ADDRESS;
            break;
        }

        case OID_802_3_MAXIMUM_LIST_SIZE:
        {
            GenericInfo.Ulong = NVNET_MULTICAST_LIST_SIZE;
            break;
        }

        case OID_TCP_TASK_OFFLOAD:
        {
            return NvNetGetTcpTaskOffload(Adapter,
                                          InformationBuffer,
                                          InformationBufferLength,
                                          BytesWritten,
                                          BytesWritten);
        }

        case OID_PNP_ENABLE_WAKE_UP:
        {
            GenericInfo.Ulong = NvNetGetWakeUp(Adapter);
            break;
        }

        case OID_PNP_CAPABILITIES:
        {
            InfoLength = sizeof(NDIS_PNP_CAPABILITIES);

            if (InformationBufferLength < InfoLength)
            {
                *BytesWritten = 0;
                *BytesNeeded = InfoLength;
                return NDIS_STATUS_BUFFER_TOO_SHORT;
            }

            *BytesWritten = InfoLength;
            *BytesNeeded = 0;
            return NvNetFillPowerManagementCapabilities(Adapter, InformationBuffer);
        }

        case OID_PNP_QUERY_POWER:
        {
            return NDIS_STATUS_SUCCESS;
        }

        case OID_GEN_VLAN_ID:
        {
            /* TODO: Implement software VLAN support */
            if (!(Adapter->Flags & NV_VLAN_TAGGING))
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            GenericInfo.Ulong = Adapter->VlanId;
            break;
        }

        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (InfoLength > InformationBufferLength)
        {
            *BytesWritten = 0;
            *BytesNeeded = InfoLength;
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
        else
        {
            NdisMoveMemory(InformationBuffer, InfoPtr, InfoLength);
            *BytesWritten = InfoLength;
            *BytesNeeded = 0;
        }
    }
    else
    {
        *BytesWritten = 0;
        *BytesNeeded = 0;
    }

    return Status;
}

NDIS_STATUS
NTAPI
MiniportSetInformation(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesRead,
    _Out_ PULONG BytesNeeded)
{
    PNVNET_ADAPTER Adapter = (PNVNET_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG GenericUlong;

    *BytesRead = 0;
    *BytesNeeded = 0;

    switch (Oid)
    {
        case OID_802_3_MULTICAST_LIST:
        {
            if (InformationBufferLength % ETH_LENGTH_OF_ADDRESS)
            {
                *BytesNeeded = (InformationBufferLength / ETH_LENGTH_OF_ADDRESS) *
                               ETH_LENGTH_OF_ADDRESS;
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            if (InformationBufferLength > sizeof(Adapter->MulticastList))
            {
                *BytesNeeded = sizeof(Adapter->MulticastList);
                Status = NDIS_STATUS_MULTICAST_FULL;
                break;
            }

            *BytesRead = InformationBufferLength;
            NdisMoveMemory(Adapter->MulticastList, InformationBuffer, InformationBufferLength);

            Adapter->MulticastListSize = InformationBufferLength / ETH_LENGTH_OF_ADDRESS;

            NvNetApplyPacketFilter(Adapter);
            break;
        }

        case OID_GEN_CURRENT_PACKET_FILTER:
        {
            if (InformationBufferLength < sizeof(ULONG))
            {
                *BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            *BytesRead = sizeof(ULONG);
            NdisMoveMemory(&GenericUlong, InformationBuffer, sizeof(ULONG));

            if (GenericUlong & ~NVNET_PACKET_FILTERS)
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            /* Do not check to see if the filter is the same filter */
            Adapter->PacketFilter = GenericUlong;

            NvNetApplyPacketFilter(Adapter);
            break;
        }

        case OID_GEN_CURRENT_LOOKAHEAD:
        {
            if (InformationBufferLength < sizeof(ULONG))
            {
                *BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            /* Nothing to do */
            *BytesRead = sizeof(ULONG);
            break;
        }

        case OID_GEN_VLAN_ID:
        {
            if (InformationBufferLength < sizeof(ULONG))
            {
                *BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            if (!(Adapter->Flags & NV_VLAN_TAGGING))
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            *BytesRead = sizeof(ULONG);
            NdisMoveMemory(&GenericUlong, InformationBuffer, sizeof(ULONG));

            if (GenericUlong > NVNET_MAXIMUM_VLAN_ID)
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            Adapter->VlanId = GenericUlong;
            break;
        }

        case OID_TCP_TASK_OFFLOAD:
        {
            if (InformationBufferLength < sizeof(NDIS_TASK_OFFLOAD_HEADER))
            {
                *BytesNeeded = sizeof(NDIS_TASK_OFFLOAD_HEADER);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            *BytesRead = InformationBufferLength;

            Status = NvNetSetTcpTaskOffload(Adapter, InformationBuffer, BytesRead);
            break;
        }

        case OID_PNP_ADD_WAKE_UP_PATTERN:
        {
            if (InformationBufferLength < sizeof(NDIS_PM_PACKET_PATTERN))
            {
                *BytesNeeded = sizeof(NDIS_PM_PACKET_PATTERN);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            *BytesRead = sizeof(NDIS_PM_PACKET_PATTERN);

            Status = NvNetAddWakeUpPattern(Adapter, InformationBuffer);
            break;
        }

        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        {
            if (InformationBufferLength < sizeof(NDIS_PM_PACKET_PATTERN))
            {
                *BytesNeeded = sizeof(NDIS_PM_PACKET_PATTERN);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            *BytesRead = sizeof(NDIS_PM_PACKET_PATTERN);

            Status = NvNetRemoveWakeUpPattern(Adapter, InformationBuffer);
            break;
        }

        case OID_PNP_ENABLE_WAKE_UP:
        {
            if (InformationBufferLength < sizeof(ULONG))
            {
                *BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            *BytesRead = sizeof(ULONG);
            NdisMoveMemory(&GenericUlong, InformationBuffer, sizeof(ULONG));

            NvNetEnableWakeUp(Adapter, GenericUlong);
            break;
        }

        case OID_PNP_SET_POWER:
        {
            if (InformationBufferLength < sizeof(NDIS_DEVICE_POWER_STATE))
            {
                *BytesNeeded = sizeof(NDIS_DEVICE_POWER_STATE);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            *BytesRead = sizeof(ULONG);
            NdisMoveMemory(&GenericUlong, InformationBuffer, sizeof(ULONG));

            if (GenericUlong < NdisDeviceStateD0 || GenericUlong > NdisDeviceStateD3)
            {
                Status = NDIS_STATUS_INVALID_DATA;
                break;
            }

            Status = NvNetSetPower(Adapter, GenericUlong);
            break;
        }

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    return Status;
}
