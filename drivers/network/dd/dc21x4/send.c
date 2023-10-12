/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Packet sending
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include "dc21x4.h"

#include <debug.h>

/* FUNCTIONS ******************************************************************/

static
VOID
DcTransmitPacket(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_TCB Tcb,
    _In_ PSCATTER_GATHER_LIST SgList)
{
    PDC_TBD Tbd, NextTbd, FirstTbd, LastTbd;
    ULONG i, TbdStatus;

    TbdStatus = 0;
    Tbd = Adapter->CurrentTbd;

    for (i = 0; i < SgList->NumberOfElements; ++i)
    {
        ULONG Address, Length;

        LastTbd = Tbd;

        /* Not owned by NIC */
        ASSERT(!(((i % 2) == 0) && (Tbd->Status & DC_TBD_STATUS_OWNED)));

        Tbd->Status = TbdStatus;

        /* 32-bit DMA */
        ASSERT(SgList->Elements[i].Address.HighPart == 0);

        Address = SgList->Elements[i].Address.LowPart;
        Length = SgList->Elements[i].Length;

        /* Two data buffers per descriptor */
        if ((i % 2) == 0)
        {
            Tbd->Control &= DC_TBD_CONTROL_END_OF_RING;

            Tbd->Control |= Length;
            Tbd->Address1 = Address;

            NextTbd = DC_NEXT_TBD(Adapter, Tbd);
        }
        else
        {
            Tbd->Control |= Length << DC_TBD_CONTROL_LENGTH_2_SHIFT;
            Tbd->Address2 = Address;

            Tbd = NextTbd;
            TbdStatus = DC_TBD_STATUS_OWNED;
        }
    }

    /* Enable IRQ on last element */
    LastTbd->Control |= DC_TBD_CONTROL_LAST_FRAGMENT | DC_TBD_CONTROL_REQUEST_INTERRUPT;

    Tcb->Tbd = LastTbd;

    FirstTbd = Adapter->CurrentTbd;
    Adapter->CurrentTbd = NextTbd;

    /* Not owned by NIC */
    ASSERT(!(FirstTbd->Status & DC_TBD_STATUS_OWNED));

    FirstTbd->Control |= DC_TBD_CONTROL_FIRST_FRAGMENT;
    DC_WRITE_BARRIER();
    FirstTbd->Status = DC_TBD_STATUS_OWNED;
}

static
BOOLEAN
DcCopyPacket(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PNDIS_PACKET Packet,
    _In_ PDC_COALESCE_BUFFER Buffer)
{
    PNDIS_BUFFER CurrentBuffer;
    PVOID Address;
    UINT CurrentLength, PacketLength;
    PUCHAR Destination;

    NdisGetFirstBufferFromPacketSafe(Packet,
                                     &CurrentBuffer,
                                     &Address,
                                     &CurrentLength,
                                     &PacketLength,
                                     HighPagePriority);
    if (!Address)
        return FALSE;

    Destination = Buffer->VirtualAddress;

    while (TRUE)
    {
        NdisMoveMemory(Destination, Address, CurrentLength);
        Destination += CurrentLength;

        NdisGetNextBuffer(CurrentBuffer, &CurrentBuffer);
        if (!CurrentBuffer)
            break;

        NdisQueryBufferSafe(CurrentBuffer,
                            &Address,
                            &CurrentLength,
                            HighPagePriority);
        if (!Address)
            return FALSE;
    }

    return TRUE;
}

static
NDIS_STATUS
DcSendPacket(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;
    PSCATTER_GATHER_LIST SgList;
    PDC_TCB Tcb;
    ULONG SlotsUsed;

    if (!Adapter->TcbSlots)
        return NDIS_STATUS_RESOURCES;

    SgList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet, ScatterGatherListPacketInfo);

    if (SgList->NumberOfElements > DC_FRAGMENTATION_THRESHOLD)
    {
        PDC_COALESCE_BUFFER CoalesceBuffer;
        UINT PacketLength;

        if (!Adapter->TbdSlots || !Adapter->SendBufferList.Next)
        {
            return NDIS_STATUS_RESOURCES;
        }

        NdisQueryPacketLength(Packet, &PacketLength);

        CoalesceBuffer = (PDC_COALESCE_BUFFER)PopEntryList(&Adapter->SendBufferList);

        if (!DcCopyPacket(Adapter, Packet, CoalesceBuffer))
        {
            PushEntryList(&Adapter->SendBufferList, &CoalesceBuffer->ListEntry);
            return NDIS_STATUS_RESOURCES;
        }

        SgList = &Adapter->LocalSgList;
        SgList->Elements[0].Address.LowPart = CoalesceBuffer->PhysicalAddress;
        SgList->Elements[0].Length = PacketLength;
        SgList->NumberOfElements = 1;
        SlotsUsed = 1;

        Tcb = Adapter->CurrentTcb;
        Tcb->SlotsUsed = 1;
        Tcb->Buffer = CoalesceBuffer;
    }
    else
    {
        /* We use two data buffers per descriptor */
        SlotsUsed = (SgList->NumberOfElements + 1) / 2;

        if (SlotsUsed > Adapter->TbdSlots)
            return NDIS_STATUS_RESOURCES;

        Tcb = Adapter->CurrentTcb;
        Tcb->SlotsUsed = SlotsUsed;
        Tcb->Buffer = NULL;
    }

    --Adapter->TcbSlots;

    Adapter->CurrentTcb = DC_NEXT_TCB(Adapter, Tcb);

    Tcb->Packet = Packet;

    ASSERT(Adapter->TbdSlots >= Tcb->SlotsUsed);
    Adapter->TbdSlots -= SlotsUsed;

    DcTransmitPacket(Adapter, Tcb, SgList);

    DC_WRITE(Adapter, DcCsr1_TxPoll, DC_TX_POLL_DOORBELL);

    return NDIS_STATUS_PENDING;
}

VOID
DcProcessPendingPackets(
    _In_ PDC21X4_ADAPTER Adapter)
{
    PLIST_ENTRY Entry;
    NDIS_STATUS Status;
    PNDIS_PACKET Packet;

    ASSERT(!IsListEmpty(&Adapter->SendQueueList));

    do
    {
        Entry = RemoveHeadList(&Adapter->SendQueueList);

        Packet = DC_PACKET_FROM_LIST_ENTRY(Entry);

        Status = DcSendPacket(Adapter, Packet);
        if (Status == NDIS_STATUS_RESOURCES)
        {
            InsertHeadList(&Adapter->SendQueueList, DC_LIST_ENTRY_FROM_PACKET(Packet));
            break;
        }
    }
    while (!IsListEmpty(&Adapter->SendQueueList));
}

VOID
NTAPI
DcSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PPNDIS_PACKET PacketArray,
    _In_ UINT NumberOfPackets)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;
    NDIS_STATUS Status;
    ULONG i;

    NdisAcquireSpinLock(&Adapter->SendLock);

    if (!(Adapter->Flags & DC_ACTIVE))
    {
        NdisReleaseSpinLock(&Adapter->SendLock);

        for (i = 0; i < NumberOfPackets; ++i)
        {
            NdisMSendComplete(Adapter->AdapterHandle,
                              PacketArray[i],
                              NDIS_STATUS_NOT_ACCEPTED);
        }

        return;
    }

    TRACE("Send packets %u\n", NumberOfPackets);

    for (i = 0; i < NumberOfPackets; ++i)
    {
        PNDIS_PACKET Packet = PacketArray[i];

        Status = DcSendPacket(Adapter, Packet);
        if (Status == NDIS_STATUS_RESOURCES)
        {
            InsertTailList(&Adapter->SendQueueList, DC_LIST_ENTRY_FROM_PACKET(Packet));
        }
    }

    NdisReleaseSpinLock(&Adapter->SendLock);
}

VOID
NTAPI
DcCancelSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PVOID CancelId)
{
    PDC21X4_ADAPTER Adapter = (PDC21X4_ADAPTER)MiniportAdapterContext;
    LIST_ENTRY DoneList;
    PLIST_ENTRY Entry, NextEntry;

    TRACE("Called\n");

    InitializeListHead(&DoneList);

    NdisAcquireSpinLock(&Adapter->SendLock);

    NextEntry = Adapter->SendQueueList.Flink;
    while (NextEntry != &Adapter->SendQueueList)
    {
        PNDIS_PACKET Packet;

        Entry = NextEntry;
        NextEntry = NextEntry->Flink;

        Packet = DC_PACKET_FROM_LIST_ENTRY(Entry);

        if (NDIS_GET_PACKET_CANCEL_ID(Packet) == CancelId)
        {
            RemoveEntryList(DC_LIST_ENTRY_FROM_PACKET(Packet));

            InsertTailList(&DoneList, DC_LIST_ENTRY_FROM_PACKET(Packet));
        }
    }

    NdisReleaseSpinLock(&Adapter->SendLock);

    while (!IsListEmpty(&DoneList))
    {
        Entry = RemoveHeadList(&DoneList);

        NdisMSendComplete(Adapter->AdapterHandle,
                          DC_PACKET_FROM_LIST_ENTRY(Entry),
                          NDIS_STATUS_REQUEST_ABORTED);
    }
}
