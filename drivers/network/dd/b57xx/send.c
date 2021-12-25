/*
 * PROJECT:     ReactOS Broadcom NetXtreme Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Sending packets
 * COPYRIGHT:   Copyright 2021 Scott Maday <coldasdryice1@gmail.com>
 */

#include "nic.h"

#include <debug.h>

NDIS_STATUS
NTAPI
NICTransmitPacket(IN PB57XX_ADAPTER Adapter,
                  IN PHYSICAL_ADDRESS PhysicalAddress,
                  IN ULONG Length)
{
    NDIS_MinDbgPrint("NICTransmitPacket\n");

    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
NTAPI
MiniportSend(IN NDIS_HANDLE MiniportAdapterContext,
             IN PNDIS_PACKET Packet,
             IN UINT Flags)
{
    PB57XX_ADAPTER Adapter = (PB57XX_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS Status;
    PSCATTER_GATHER_LIST SGList;
    ULONG TransmitLength;
    PHYSICAL_ADDRESS TransmitBuffer;

    NDIS_MinDbgPrint("B57XX send\n");
    
    SGList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);

    ASSERT(SGList != NULL);
    ASSERT(SGList->NumberOfElements == 1);
    ASSERT((SGList->Elements[0].Address.LowPart & 3) == 0);
    ASSERT(SGList->Elements[0].Length <= MAXIMUM_FRAME_SIZE);

    /*if (Adapter->TxFull)
    {
        NDIS_MinDbgPrint("All TX descriptors are full\n");
        return NDIS_STATUS_RESOURCES;
    }*/

    TransmitLength = SGList->Elements[0].Length;
    TransmitBuffer = SGList->Elements[0].Address;
    //Adapter->TransmitPackets[Adapter->CurrentTxDesc] = Packet; TODO

    Status = NICTransmitPacket(Adapter, TransmitBuffer, TransmitLength);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_MinDbgPrint("Transmit packet failed\n");
        return Status;
    }

    return NDIS_STATUS_PENDING;
}
