/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later.html)
 * PURPOSE:     Sending packets
 * COPYRIGHT:   Copyright 2018 Mark Jansen <mark.jansen@reactos.org>
 *              Copyright 2019 Victor Perevertkin <victor.perevertkin@reactos.org>
 */

#include "nic.h"

#include <debug.h>

static
NDIS_STATUS
NICTransmitPacket(
    _In_ PE1000_ADAPTER Adapter,
    _In_ PHYSICAL_ADDRESS PhysicalAddress,
    _In_ ULONG Length)
{
    volatile PE1000_TRANSMIT_DESCRIPTOR TransmitDescriptor;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    TransmitDescriptor = Adapter->TransmitDescriptors + Adapter->CurrentTxDesc;
    TransmitDescriptor->Address = PhysicalAddress.QuadPart;
    TransmitDescriptor->Length = Length;
    TransmitDescriptor->ChecksumOffset = 0;
    TransmitDescriptor->Command = E1000_TDESC_CMD_RS | E1000_TDESC_CMD_IFCS |
                                  E1000_TDESC_CMD_EOP | E1000_TDESC_CMD_IDE;
    TransmitDescriptor->Status = 0;
    TransmitDescriptor->ChecksumStartField = 0;
    TransmitDescriptor->Special = 0;

    Adapter->CurrentTxDesc = (Adapter->CurrentTxDesc + 1) % NUM_TRANSMIT_DESCRIPTORS;

    E1000WriteUlong(Adapter, E1000_REG_TDT, Adapter->CurrentTxDesc);

    if (Adapter->CurrentTxDesc == Adapter->LastTxDesc)
    {
        NDIS_DbgPrint(MID_TRACE, ("All TX descriptors are full now\n"));
        Adapter->TxFull = TRUE;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
NTAPI
MiniportSend(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet,
    _In_ UINT Flags)
{
    PE1000_ADAPTER Adapter = (PE1000_ADAPTER)MiniportAdapterContext;
    PSCATTER_GATHER_LIST sgList;
    ULONG TransmitLength;
    PHYSICAL_ADDRESS TransmitBuffer;
    NDIS_STATUS Status;

    sgList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);

    ASSERT(sgList != NULL);
    ASSERT(sgList->NumberOfElements == 1);
    ASSERT((sgList->Elements[0].Address.LowPart & 3) == 0);
    ASSERT(sgList->Elements[0].Length <= MAXIMUM_FRAME_SIZE);

    if (Adapter->TxFull)
    {
        NDIS_DbgPrint(MIN_TRACE, ("All TX descriptors are full\n"));
        return NDIS_STATUS_RESOURCES;
    }

    TransmitLength = sgList->Elements[0].Length;
    TransmitBuffer = sgList->Elements[0].Address;
    Adapter->TransmitPackets[Adapter->CurrentTxDesc] = Packet;

    Status = NICTransmitPacket(Adapter, TransmitBuffer, TransmitLength);
    if (Status != NDIS_STATUS_SUCCESS)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Transmit packet failed\n"));
        return Status;
    }

    return NDIS_STATUS_PENDING;
}
