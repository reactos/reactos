/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Miniport information callbacks
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "e100.h"

#include <debug.h>

/* GLOBASLS *******************************************************************/

static const NDIS_OID E100SupportedOidList[] =
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
    // OID_TCP_TASK_OFFLOAD,

    /* Power management */
    // OID_PNP_CAPABILITIES,
    // OID_PNP_SET_POWER,
    // OID_PNP_QUERY_POWER,
    // OID_PNP_ADD_WAKE_UP_PATTERN,
    // OID_PNP_REMOVE_WAKE_UP_PATTERN,
    // OID_PNP_ENABLE_WAKE_UP
};

/* FUNCTIONS ******************************************************************/

static
VOID
EeQueryStatisticCounter(
    _In_ PE100_ADAPTER Adapter,
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
            *Counter = Adapter->Statistics.TransmitMoreCollisions;
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
            *Counter = Adapter->Statistics.TransmitHeartbeatErrors;
            break;
        case OID_802_3_XMIT_TIMES_CRS_LOST:
            *Counter = Adapter->Statistics.TransmitLostCarrierSense;
            break;
        case OID_802_3_XMIT_LATE_COLLISIONS:
            *Counter = Adapter->Statistics.TransmitLateCollisions;
            break;

        default:
            ASSERT(FALSE);
            UNREACHABLE;
            break;
    }
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
    PE100_ADAPTER Adapter = MiniportAdapterContext;
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
            InfoPtr = (PVOID)&E100SupportedOidList;
            InfoLength = sizeof(E100SupportedOidList);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            GenericInfo.Ulong = Adapter->PacketFilter;
            break;

        case OID_802_3_MULTICAST_LIST:
            InfoPtr = Adapter->MulticastList;
            InfoLength = Adapter->MulticastListSize;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            GenericInfo.Ulong = E100_MULTICAST_LIST_SIZE;
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        {
            GenericInfo.Medium = NdisMedium802_3;
            InfoLength = sizeof(NDIS_MEDIUM);
            break;
        }

        case OID_GEN_HARDWARE_STATUS:
        {
            /* The card has been physically ejected */
            if (CSR_READ_8(Adapter, FXP_CSR_SCB_STATACK) == 0xFF)
                GenericInfo.Status = NdisHardwareStatusNotReady;
            else
                GenericInfo.Status = NdisHardwareStatusReady;

            InfoLength = sizeof(NDIS_HARDWARE_STATUS);
            break;
        }

        case OID_GEN_MAXIMUM_FRAME_SIZE:
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        case OID_GEN_CURRENT_LOOKAHEAD:
            GenericInfo.Ulong = E100_MAXIMUM_FRAME_SIZE - E100_ETHERNET_HEADER_SIZE;
            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            GenericInfo.Ulong = E100_MAXIMUM_FRAME_SIZE;
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
            GenericInfo.Ulong = E100_TRANSMIT_BLOCK_SIZE;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            GenericInfo.Ulong = E100_TRANSMIT_BLOCK_SIZE * Adapter->TcbCount;
            break;

        case OID_GEN_RECEIVE_BLOCK_SIZE:
            GenericInfo.Ulong = E100_RECEIVE_BLOCK_SIZE;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            GenericInfo.Ulong = E100_RECEIVE_BLOCK_SIZE * Adapter->RfdCount;
            break;

        case OID_GEN_LINK_SPEED:
            if (Adapter->CurrentMedia & E100_MEDIA_100T)
                GenericInfo.Ulong = 100 * 10000;
            else
                GenericInfo.Ulong = 10 * 10000;
            break;

        case OID_GEN_VENDOR_ID:
            GenericInfo.Ulong = 0;
            GenericInfo.Ulong |= (Adapter->PermanentMacAddress[0] << 16);
            GenericInfo.Ulong |= (Adapter->PermanentMacAddress[1] << 8);
            GenericInfo.Ulong |= (Adapter->PermanentMacAddress[2] & 0xFF);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
        {
            static const CHAR VendorDesc[] = "i8255x compatible Ethernet Controller";
            InfoPtr = (PVOID)&VendorDesc;
            InfoLength = sizeof(VendorDesc);
            break;
        }

        case OID_GEN_VENDOR_DRIVER_VERSION:
            /* 1.0.0 */
            GenericInfo.Ulong = 0x100;
            break;

        case OID_GEN_DRIVER_VERSION:
        {
            InfoLength = sizeof(USHORT);
            GenericInfo.Ushort = (NDIS_MINIPORT_MAJOR_VERSION << 8) | NDIS_MINIPORT_MINOR_VERSION;
            break;
        }

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            GenericInfo.Ulong = Adapter->TcbCount;
            break;

        case OID_GEN_MAC_OPTIONS:
            GenericInfo.Ulong = NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                                NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                                NDIS_MAC_OPTION_NO_LOOPBACK;

            if (Adapter->Flags & E100_FLAG_PACKET_PRIORITY)
                GenericInfo.Ulong |= NDIS_MAC_OPTION_8021P_PRIORITY;
            if (Adapter->Flags & E100_FLAG_VLAN_TAGGING)
                GenericInfo.Ulong |= NDIS_MAC_OPTION_8021Q_VLAN;
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            if (Adapter->CurrentMedia == E100_MEDIA_NONE)
                GenericInfo.Ulong = NdisMediaStateDisconnected;
            else
                GenericInfo.Ulong = NdisMediaStateConnected;
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            InfoPtr = Adapter->PermanentMacAddress;
            InfoLength = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            InfoPtr = Adapter->CurrentMacAddress;
            InfoLength = ETH_LENGTH_OF_ADDRESS;
            break;

        case OID_GEN_PHYSICAL_MEDIUM:
            GenericInfo.Ulong = NdisPhysicalMedium802_3;
            break;

        case OID_GEN_XMIT_OK:
        case OID_GEN_RCV_OK:
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
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
            EeQueryStatisticCounter(Adapter, Oid, &GenericInfo.Ulong64);

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
            NdisAcquireSpinLock(&Adapter->SendLock);
            GenericInfo.Ulong = Adapter->TcbCount - Adapter->TxPending;
            NdisReleaseSpinLock(&Adapter->SendLock);
            break;

        case OID_GEN_VLAN_ID:
        {
            if (!(Adapter->Flags & E100_FLAG_VLAN_TAGGING))
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            GenericInfo.Ulong = Adapter->VlanId;
            break;
        }

        // TODO
        // case OID_TCP_TASK_OFFLOAD:

        case OID_PNP_CAPABILITIES:
        {
            PNDIS_PNP_CAPABILITIES Capabilities;

            InfoLength = sizeof(NDIS_PNP_CAPABILITIES);

            if (InformationBufferLength < InfoLength)
            {
                *BytesWritten = 0;
                *BytesNeeded = InfoLength;
                return NDIS_STATUS_BUFFER_TOO_SHORT;
            }

            *BytesWritten = InfoLength;
            *BytesNeeded = 0;

            Capabilities = InformationBuffer;
            Capabilities->WakeUpCapabilities.MinMagicPacketWakeUp = NdisDeviceStateD3;
            Capabilities->WakeUpCapabilities.MinPatternWakeUp = NdisDeviceStateD3;
            Capabilities->WakeUpCapabilities.MinLinkChangeWakeUp = NdisDeviceStateD3;

            /* All hardware is PM-aware */
            return NDIS_STATUS_SUCCESS; // TODO: Check the 82557
        }

        case OID_PNP_QUERY_POWER:
            return NDIS_STATUS_SUCCESS;

        case OID_PNP_ENABLE_WAKE_UP:
            GenericInfo.Ulong = Adapter->WakeUpFlags & (NDIS_PNP_WAKE_UP_MAGIC_PACKET |
                                                        NDIS_PNP_WAKE_UP_PATTERN_MATCH |
                                                        NDIS_PNP_WAKE_UP_LINK_CHANGE);
            break;

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
    PE100_ADAPTER Adapter = MiniportAdapterContext;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG GenericUlong;

    *BytesRead = 0;
    *BytesNeeded = 0;

    switch (Oid)
    {
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

            if (GenericUlong & ~E100_PACKET_FILTERS)
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            Status = EeApplyPacketFilter(Adapter, GenericUlong);
            break;
        }

        case OID_802_3_MULTICAST_LIST:
        {
            ULONG Size;

            if (InformationBufferLength % ETH_LENGTH_OF_ADDRESS)
            {
                *BytesNeeded = (InformationBufferLength / ETH_LENGTH_OF_ADDRESS) *
                               ETH_LENGTH_OF_ADDRESS;
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            Size = E100_MULTICAST_LIST_SIZE * ETH_LENGTH_OF_ADDRESS;
            if (InformationBufferLength > Size)
            {
                *BytesNeeded = Size;
                Status = NDIS_STATUS_MULTICAST_FULL;
                break;
            }

            *BytesRead = InformationBufferLength;
            NdisMoveMemory(Adapter->MulticastList, InformationBuffer, InformationBufferLength);

            Adapter->MulticastListSize = InformationBufferLength;

            Status = EeUpdateMulticastList(Adapter);
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

            if (!(Adapter->Flags & E100_FLAG_VLAN_TAGGING))
            {
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            *BytesRead = sizeof(ULONG);
            NdisMoveMemory(&GenericUlong, InformationBuffer, sizeof(ULONG));

            if (GenericUlong > E100_MAXIMUM_VLAN_ID)
            {
                Status = NDIS_STATUS_FAILURE;
                break;
            }

            Adapter->VlanId = GenericUlong;
            break;
        }

        // TODO
        // case OID_PNP_ENABLE_WAKE_UP:
        // case OID_PNP_ADD_WAKE_UP_PATTERN:
        // case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        // case OID_PNP_SET_POWER:

        default:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    return Status;
}
