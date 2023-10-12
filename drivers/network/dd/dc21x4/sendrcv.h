/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Send and receive definitions
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#define DC_LIST_ENTRY_FROM_PACKET(Packet) \
    ((PLIST_ENTRY)(&(Packet)->MiniportReservedEx[0]))

#define DC_PACKET_FROM_LIST_ENTRY(ListEntry) \
    (CONTAINING_RECORD(ListEntry, NDIS_PACKET, MiniportReservedEx))

#define DC_RCB_FROM_PACKET(Packet) \
    ((PDC_RCB*)&(Packet)->MiniportReservedEx[0])

#define DC_RBD_FROM_PACKET(Packet) \
    ((PDC_RBD*)&(Packet)->MiniportReservedEx[sizeof(PVOID)])

typedef struct _DC_COALESCE_BUFFER
{
    /* Must be the first entry */
    SINGLE_LIST_ENTRY ListEntry;

    PVOID VirtualAddress;
    ULONG PhysicalAddress;
} DC_COALESCE_BUFFER, *PDC_COALESCE_BUFFER;

typedef struct _DC_TCB
{
    PDC_TBD Tbd;
    PNDIS_PACKET Packet;
    PDC_COALESCE_BUFFER Buffer;
    ULONG SlotsUsed;
} DC_TCB, *PDC_TCB;

typedef struct _DC_RCB
{
    /* Must be the first entry */
    SINGLE_LIST_ENTRY ListEntry;

    ULONG PhysicalAddress;
    ULONG Flags;
#define DC_RCB_FLAG_RECLAIM          0x80000000

    PNDIS_PACKET Packet;
    PNDIS_BUFFER NdisBuffer;
    PVOID VirtualAddress;
    PVOID VirtualAddressOriginal;
    NDIS_PHYSICAL_ADDRESS PhysicalAddressOriginal;
    SINGLE_LIST_ENTRY AllocListEntry;
} DC_RCB, *PDC_RCB;

FORCEINLINE
VOID
DC_RELEASE_TCB(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_TCB Tcb)
{
    if (Tcb->Buffer)
    {
        PushEntryList(&Adapter->SendBufferList, &Tcb->Buffer->ListEntry);
    }

    ++Adapter->TcbSlots;

    Adapter->TbdSlots += Tcb->SlotsUsed;
}

FORCEINLINE
PDC_TCB
DC_NEXT_TCB(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_TCB Tcb)
{
    if (Tcb++ == Adapter->TailTcb)
        return Adapter->HeadTcb;
    else
        return Tcb;
}

FORCEINLINE
PDC_TBD
DC_NEXT_TBD(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_TBD Tbd)
{
    if (Tbd++ == Adapter->TailTbd)
        return Adapter->HeadTbd;
    else
        return Tbd;
}

FORCEINLINE
PDC_RBD
DC_NEXT_RBD(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_RBD Rbd)
{
    if (Rbd++ == Adapter->TailRbd)
        return Adapter->HeadRbd;
    else
        return Rbd;
}

FORCEINLINE
PDC_RCB*
DC_GET_RCB_SLOT(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PDC_RBD Rbd)
{
    return Adapter->RcbArray + (((ULONG_PTR)(Rbd - Adapter->HeadRbd)));
}
