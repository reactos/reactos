/*
 * ReactOS Realtek 8139 Driver
 *
 * Copyright (C) 2013 Cameron Gutman
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "nic.h"

#define NDEBUG
#include <debug.h>

NDIS_STATUS
NTAPI
NICPowerOn (
    IN PRTL_ADAPTER Adapter
    )
{
    //
    // Send 0x00 to the CONFIG_1 register (0x52) to set the LWAKE + LWPTN to active high.
    // This should essentially *power on* the device.
    // -- OSDev Wiki
    //
    NdisRawWritePortUchar(Adapter->IoBase + R_CFG1, 0x00);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICSoftReset (
    IN PRTL_ADAPTER Adapter
    )
{
    UCHAR commandReg;
    UINT resetAttempts;

    //
    // Sending 0x10 to the Command register (0x37) will send the RTL8139 into a software reset.
    // Once that byte is sent, the RST bit must be checked to make sure that the chip has finished the reset.
    // If the RST bit is high (1), then the reset is still in operation.
    // -- OSDev Wiki
    NdisRawWritePortUchar(Adapter->IoBase + R_CMD, B_CMD_RST);

    for (resetAttempts = 0; resetAttempts < MAX_RESET_ATTEMPTS; resetAttempts++)
    {
        NdisRawReadPortUchar(Adapter->IoBase + R_CMD, &commandReg);

        if (!(commandReg & B_CMD_RST))
        {
            return NDIS_STATUS_SUCCESS;
        }

        NdisMSleep(100);
    }

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
NTAPI
NICRegisterReceiveBuffer (
    IN PRTL_ADAPTER Adapter
    )
{
    ASSERT(NdisGetPhysicalAddressHigh(Adapter->ReceiveBufferPa) == 0);

    NdisRawWritePortUlong(Adapter->IoBase + R_RXSA, Adapter->ReceiveBufferPa.LowPart);

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICRemoveReceiveBuffer (
    IN PRTL_ADAPTER Adapter
    )
{
    NdisRawWritePortUlong(Adapter->IoBase + R_RXSA, 0);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICEnableTxRx (
    IN PRTL_ADAPTER Adapter
    )
{
    NdisRawWritePortUchar(Adapter->IoBase + R_CMD, B_CMD_TXE | B_CMD_RXE);

    //
    // TX and RX must be enabled before setting these
    //
    NdisRawWritePortUlong(Adapter->IoBase + R_RC, RC_VAL);
    NdisRawWritePortUlong(Adapter->IoBase + R_TC, TC_VAL);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICGetPermanentMacAddress (
    IN PRTL_ADAPTER Adapter,
    OUT PUCHAR MacAddress
    )
{
    UINT i;

    for (i = 0; i < IEEE_802_ADDR_LENGTH; i++)
    {
        NdisRawReadPortUchar(Adapter->IoBase + R_MAC + i, &MacAddress[i]);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICApplyInterruptMask (
    IN PRTL_ADAPTER Adapter
    )
{
    NdisRawWritePortUshort(Adapter->IoBase + R_IM, Adapter->InterruptMask);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICDisableInterrupts (
    IN PRTL_ADAPTER Adapter
    )
{
    NdisRawWritePortUshort(Adapter->IoBase + R_IM, 0);
    return NDIS_STATUS_SUCCESS;
}

USHORT
NTAPI
NICInterruptRecognized (
    IN PRTL_ADAPTER Adapter,
    OUT PBOOLEAN InterruptRecognized
    )
{
    USHORT interruptStatus;

    NdisRawReadPortUshort(Adapter->IoBase + R_IS, &interruptStatus);

    *InterruptRecognized = (interruptStatus & Adapter->InterruptMask) != 0;

    return (interruptStatus & Adapter->InterruptMask);
}

VOID
NTAPI
NICAcknowledgeInterrupts (
    IN PRTL_ADAPTER Adapter
    )
{
    NdisRawWritePortUshort(Adapter->IoBase + R_IS, Adapter->InterruptPending);
}

VOID
NTAPI
NICUpdateLinkStatus (
    IN PRTL_ADAPTER Adapter
    )
{
    UCHAR mediaState;

    NdisRawReadPortUchar(Adapter->IoBase + R_MS, &mediaState);
    Adapter->MediaState = (mediaState & R_MS_LINKDWN) ? NdisMediaStateDisconnected :
                                                        NdisMediaStateConnected;
    Adapter->LinkSpeedMbps = (mediaState & R_MS_SPEED_10) ? 10 : 100;
}

NDIS_STATUS
NTAPI
NICApplyPacketFilter (
    IN PRTL_ADAPTER Adapter
    )
{
    ULONG filterMask;

    filterMask = RC_VAL;

    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_DIRECTED)
    {
        filterMask |= B_RC_APM;
    }

    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_MULTICAST)
    {
        filterMask |= B_RC_AM;
    }

    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_BROADCAST)
    {
        filterMask |= B_RC_AB;
    }

    if (Adapter->PacketFilter & NDIS_PACKET_TYPE_PROMISCUOUS)
    {
        filterMask |= B_RC_AAP;
    }

    NdisRawWritePortUlong(Adapter->IoBase + R_RC, filterMask);

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
NICTransmitPacket (
    IN PRTL_ADAPTER Adapter,
    IN UCHAR TxDesc,
    IN ULONG PhysicalAddress,
    IN ULONG Length
    )
{
    NdisRawWritePortUlong(Adapter->IoBase + R_TXSAD0 + (TxDesc * sizeof(ULONG)), PhysicalAddress);
    NdisRawWritePortUlong(Adapter->IoBase + R_TXSTS0 + (TxDesc * sizeof(ULONG)), Length);
    return NDIS_STATUS_SUCCESS;
}
