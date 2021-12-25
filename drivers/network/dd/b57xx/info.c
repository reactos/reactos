/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Miniport information callbacks
 * COPYRIGHT:   Copyright 2021 Scott Maday <coldasdryice1@gmail.com>
 */

#include "nic.h"

#include "debug.h"

static NDIS_OID SupportedOidList[] =
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
    OID_802_3_MULTICAST_LIST,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_MAC_OPTIONS,
    OID_GEN_MEDIA_CONNECT_STATUS,
    OID_802_3_PERMANENT_ADDRESS,
    OID_802_3_CURRENT_ADDRESS,
    OID_802_3_MAXIMUM_LIST_SIZE,

    /* Statistics */
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
};

NDIS_STATUS
NTAPI
MiniportQueryInformation(IN NDIS_HANDLE MiniportAdapterContext,
                         IN NDIS_OID Oid,
                         IN PVOID InformationBuffer,
                         IN ULONG InformationBufferLength,
                         OUT PULONG BytesWritten,
                         OUT PULONG BytesNeeded)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    union _GENERIC_INFORMATION
    {
        USHORT UShort;
        ULONG ULong;
        ULONG64 ULong64;
        NDIS_MEDIUM Medium;
        NDIS_PNP_CAPABILITIES PmCapabilities;
    } GenericInfo;
    PVOID CopySource = &GenericInfo;
    ULONG CopyLength = sizeof(ULONG);

    switch (Oid)
    {
        case OID_GEN_SUPPORTED_LIST:
            CopySource = (PVOID)&SupportedOidList;
            CopyLength = sizeof(SupportedOidList);
            break;

        case OID_GEN_CURRENT_PACKET_FILTER:
            GenericInfo.ULong = Adapter->PacketFilter;
            break;

        case OID_GEN_HARDWARE_STATUS:
            UNIMPLEMENTED_DBGBREAK();
            GenericInfo.ULong = (ULONG)NdisHardwareStatusReady; // FIXME
            break;

        case OID_GEN_MEDIA_SUPPORTED:
        case OID_GEN_MEDIA_IN_USE:
        {
            GenericInfo.Medium = NdisMedium802_3;
            CopyLength = sizeof(NDIS_MEDIUM);
            break;
        }

        case OID_GEN_RECEIVE_BLOCK_SIZE:
        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        case OID_GEN_CURRENT_LOOKAHEAD:
        case OID_GEN_MAXIMUM_LOOKAHEAD:
        case OID_GEN_MAXIMUM_FRAME_SIZE:
            GenericInfo.ULong = MAXIMUM_FRAME_SIZE - sizeof(ETH_HEADER);
            break;

        case OID_802_3_MULTICAST_LIST:
            CopySource = Adapter->MulticastList;
            CopyLength = Adapter->MulticastListSize * IEEE_802_ADDR_LENGTH;
            break;

        case OID_802_3_MAXIMUM_LIST_SIZE:
            GenericInfo.ULong = MAXIMUM_MULTICAST_ADDRESSES;
            break;

        case OID_GEN_LINK_SPEED:
            GenericInfo.ULong = Adapter->LinkSpeedMbps * 10000;
            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            GenericInfo.ULong = MAXIMUM_FRAME_SIZE;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            GenericInfo.ULong = RECEIVE_BUFFER_SIZE;
            break;

        case OID_GEN_VENDOR_ID:
            /* The 3 bytes of the MAC address is the vendor ID */
            GenericInfo.ULong = ((ULONG)Adapter->PermanentMacAddress[0] << 16) |
                                ((ULONG)Adapter->PermanentMacAddress[1] << 8) |
                                ((ULONG)Adapter->PermanentMacAddress[2] & 0xFF);
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
        {
            static UCHAR VendorDesc[] = "ReactOS Team";
            CopySource = VendorDesc;
            CopyLength = sizeof(VendorDesc);
            break;
        }

        case OID_GEN_VENDOR_DRIVER_VERSION:
            GenericInfo.ULong = DRIVER_VERSION;
            break;

        case OID_GEN_DRIVER_VERSION:
        {
            CopyLength = sizeof(USHORT);
            GenericInfo.UShort = (NDIS_MINIPORT_MAJOR_VERSION << 8) + NDIS_MINIPORT_MINOR_VERSION;
            break;
        }

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
            GenericInfo.ULong = MAXIMUM_FRAME_SIZE;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            GenericInfo.ULong = 1;
            break;

        case OID_GEN_MAC_OPTIONS:
            GenericInfo.ULong = NDIS_MAC_OPTION_RECEIVE_SERIALIZED |
                                NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA |
                                NDIS_MAC_OPTION_TRANSFERS_NOT_PEND |
                                NDIS_MAC_OPTION_NO_LOOPBACK;
            break;

        case OID_GEN_MEDIA_CONNECT_STATUS:
            GenericInfo.ULong = Adapter->MediaState;
            break;

        case OID_802_3_CURRENT_ADDRESS:
            CopySource = Adapter->MulticastList[0].MacAddress;
            CopyLength = IEEE_802_ADDR_LENGTH;
            break;

        case OID_802_3_PERMANENT_ADDRESS:
            CopySource = Adapter->PermanentMacAddress;
            CopyLength = IEEE_802_ADDR_LENGTH;
            break;

        case OID_GEN_XMIT_OK:
        case OID_GEN_RCV_OK:
        case OID_GEN_XMIT_ERROR:
        case OID_GEN_RCV_ERROR:
        case OID_GEN_RCV_NO_BUFFER:
        {
            //GenericInfo.ULong64 = NICQueryStatisticCounter(Adapter, Oid); TODO

            *BytesNeeded = sizeof(ULONG64);
            if (InformationBufferLength >= sizeof(ULONG64))
            {
                *BytesWritten = sizeof(ULONG64);
                NdisMoveMemory(InformationBuffer, CopySource, sizeof(ULONG64));
            }
            else if (InformationBufferLength >= sizeof(ULONG))
            {
                *BytesWritten = sizeof(ULONG);
                NdisMoveMemory(InformationBuffer, CopySource, sizeof(ULONG));
            }
            else
            {
                *BytesWritten = 0;
                return NDIS_STATUS_BUFFER_TOO_SHORT;
            }
            return NDIS_STATUS_SUCCESS;
        }

        default:
            NDIS_MinDbgPrint("Unknown OID 0x%x\n", Oid);
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (CopyLength > InformationBufferLength)
        {
            *BytesNeeded = CopyLength;
            *BytesWritten = 0;
            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
        else
        {
            NdisMoveMemory(InformationBuffer, CopySource, CopyLength);
            *BytesWritten = CopyLength;
            *BytesNeeded = CopyLength;
        }
    }
    else
    {
        *BytesWritten = 0;
        *BytesNeeded = 0;
    }

    NDIS_MinDbgPrint("Query OID 0x%x: %s(0x%x) (%d, %d)\n",
                     Oid,
                     Status == NDIS_STATUS_SUCCESS ? "Completed" : "Failed",
                     Status,
                     *BytesWritten,
                     *BytesNeeded);

    return Status;
}

NDIS_STATUS
NTAPI
MiniportSetInformation(IN NDIS_HANDLE MiniportAdapterContext,
                       IN NDIS_OID Oid,
                       IN PVOID InformationBuffer,
                       IN ULONG InformationBufferLength,
                       OUT PULONG BytesRead,
                       OUT PULONG BytesNeeded)
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    ULONG GenericULong;

    switch (Oid)
    {
        case OID_GEN_CURRENT_PACKET_FILTER:
            if (InformationBufferLength < sizeof(ULONG))
            {
                *BytesRead = 0;
                *BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            NdisMoveMemory(&GenericULong, InformationBuffer, sizeof(ULONG));

            if (GenericULong &
                ~(NDIS_PACKET_TYPE_DIRECTED |
                NDIS_PACKET_TYPE_MULTICAST |
                NDIS_PACKET_TYPE_ALL_MULTICAST |
                NDIS_PACKET_TYPE_BROADCAST |
                NDIS_PACKET_TYPE_PROMISCUOUS |
                NDIS_PACKET_TYPE_MAC_FRAME))
            {
                *BytesRead = sizeof(ULONG);
                *BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }

            if (Adapter->PacketFilter == GenericULong)
            {
                break;
            }

            Adapter->PacketFilter = GenericULong;

            Status = NICApplyPacketFilter(Adapter);
            if (Status != NDIS_STATUS_SUCCESS)
            {
                NDIS_MinDbgPrint("Failed to apply new packet filter (0x%x)\n", Status);
                break;
            }

            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            if (InformationBufferLength < sizeof(ULONG))
            {
                *BytesRead = 0;
                *BytesNeeded = sizeof(ULONG);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            NdisMoveMemory(&GenericULong, InformationBuffer, sizeof(ULONG));

            if (GenericULong > MAXIMUM_FRAME_SIZE - sizeof(ETH_HEADER))
            {
                Status = NDIS_STATUS_INVALID_DATA;
            }

            break;

        case OID_802_3_MULTICAST_LIST:
            if (InformationBufferLength % IEEE_802_ADDR_LENGTH)
            {
                *BytesRead = 0;
                *BytesNeeded = InformationBufferLength +
                               (InformationBufferLength % IEEE_802_ADDR_LENGTH);
                Status = NDIS_STATUS_INVALID_LENGTH;
                break;
            }

            if (InformationBufferLength > sizeof(Adapter->MulticastList))
            {
                *BytesNeeded = sizeof(Adapter->MulticastList);
                *BytesRead = 0;
                Status = NDIS_STATUS_MULTICAST_FULL;
                break;
            }

            NdisMoveMemory(Adapter->MulticastList, InformationBuffer, InformationBufferLength);

            Adapter->MulticastListSize = InformationBufferLength / IEEE_802_ADDR_LENGTH;

            NICUpdateMulticastList(Adapter);
            break;

        default:
            NDIS_MinDbgPrint("Unknown OID 0x%x\n", Oid);
            Status = NDIS_STATUS_NOT_SUPPORTED;
            *BytesRead = 0;
            *BytesNeeded = 0;
            break;
    }

    if (Status == NDIS_STATUS_SUCCESS)
    {
        *BytesRead = InformationBufferLength;
        *BytesNeeded = 0;
    }
    
    NDIS_MinDbgPrint("Set Info on OID 0x%x: %s(0x%x) (%d, %d)\n",
                     Oid,
                     Status == NDIS_STATUS_SUCCESS ? "Completed" : "Failed",
                     Status,
                     *BytesRead,
                     *BytesNeeded);

    return Status;
}
