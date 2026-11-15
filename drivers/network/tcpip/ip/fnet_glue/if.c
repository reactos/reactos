/*
 * PROJECT:     ReactOS TCP/IP protocol driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     FNET glue: one FNET netif per ReactOS interface, packets in and out
 * COPYRIGHT:   Copyright 2026 Muhammad Yahya Bin Ehsan Qureshi <yahyauni54@gmail.com>
 */

#include "precomp.h"
#include "fnet_shim.h"
#include <ipifcons.h>

/* Packets queued to the worker and not yet handed to FNET; beyond this they are dropped */
#define FNET_GLUE_MAX_QUEUED_PACKETS    2048

typedef struct _FNET_GLUE_IF
{
    LONG RefCount;
    PIP_INTERFACE Interface;
    PVOID Netif;            /* Inside FNET only */
    ULONG ScopeId;
    ULONG Address;
    ULONG Mask;
    BOOLEAN Loopback;
} FNET_GLUE_IF, *PFNET_GLUE_IF;

typedef struct _FNET_GLUE_PACKET
{
    FNET_PORT_WORK_ITEM Item;
    PFNET_GLUE_IF GlueIf;
    ULONG Length;
    UCHAR Data[ANYSIZE_ARRAY];
} FNET_GLUE_PACKET, *PFNET_GLUE_PACKET;

static LONG QueuedPackets;
static PFNET_GLUE_IF DefaultIf;

static VOID GlueInputRoutine(PVOID Context);

static
VOID
GlueIfReference(PFNET_GLUE_IF GlueIf)
{
    InterlockedIncrement(&GlueIf->RefCount);
}

static
VOID
GlueIfDereference(PFNET_GLUE_IF GlueIf)
{
    if (InterlockedDecrement(&GlueIf->RefCount) == 0)
    {
        ASSERT(GlueIf->Netif == NULL);
        ExFreePoolWithTag(GlueIf, FNET_GLUE_IF_TAG);
    }
}

/* The e1000 driver reports 1508 for OID_GEN_MAXIMUM_FRAME_SIZE; lwIP never noticed because its MSS was fixed at 1460 */
static
ULONG
GlueInterfaceMtu(PIP_INTERFACE IF, BOOLEAN Loopback)
{
    return Loopback ? IF->MTU : min(IF->MTU, 1500);
}

ULONG
FnetGlueInterfaceMtu(PIP_INTERFACE IF)
{
    PFNET_GLUE_IF GlueIf = IF ? IF->TCPContext : NULL;

    return GlueIf ? GlueInterfaceMtu(IF, GlueIf->Loopback) : (IF ? min(IF->MTU, 1500) : 0);
}

ULONG
FnetGlueInterfaceScopeId(PIP_INTERFACE IF)
{
    PFNET_GLUE_IF GlueIf = IF ? IF->TCPContext : NULL;

    return GlueIf ? GlueIf->ScopeId : 0;
}

/* Inside FNET: FNET built an IPv4 datagram; ReactOS routes, fragments and resolves it */
static
VOID
GlueOutput(PVOID IfContext, PVOID FnetPacket, ULONG TotalLength)
{
    NDIS_STATUS NdisStatus;
    PNEIGHBOR_CACHE_ENTRY NCE = NULL;
    PIP_INTERFACE LocalIf;
    IP_PACKET Packet;
    IP_ADDRESS RemoteAddress, LocalAddress;
    IPv4_HEADER Header;

    UNREFERENCED_PARAMETER(IfContext);

    if (TotalLength < sizeof(IPv4_HEADER))
        return;

    FnetShimCopyPacket(FnetPacket, &Header, sizeof(Header));

    LocalAddress.Type = IP_ADDRESS_V4;
    LocalAddress.Address.IPv4Address = Header.SrcAddr;
    RemoteAddress.Type = IP_ADDRESS_V4;
    RemoteAddress.Address.IPv4Address = Header.DstAddr;

    IPInitializePacket(&Packet, LocalAddress.Type);

    /* To ourselves: queued, then fed back into FNET on this thread before it leaves FNET, as lwIP loops its own addresses internally */
    LocalIf = AddrLocateInterface(&RemoteAddress);
    if (!LocalIf || !AddrIsEqual(&LocalIf->Unicast, &RemoteAddress) || !LocalIf->TCPContext)
    {
        LocalIf = NULL;
        NCE = RouteGetRouteToDestination(&RemoteAddress);
        if (!NCE)
            return;
        if (NCE->Interface == Loopback && Loopback->TCPContext)
            LocalIf = Loopback;
    }

    if (LocalIf)
    {
        PFNET_GLUE_IF LoopIf = LocalIf->TCPContext;
        PFNET_GLUE_PACKET GluePacket;

        if (InterlockedIncrement(&QueuedPackets) > FNET_GLUE_MAX_QUEUED_PACKETS)
        {
            InterlockedDecrement(&QueuedPackets);
            return;
        }
        GluePacket = ExAllocatePoolWithTag(NonPagedPool, FIELD_OFFSET(FNET_GLUE_PACKET, Data[TotalLength]),
                                           FNET_GLUE_PACKET_TAG);
        if (!GluePacket)
        {
            InterlockedDecrement(&QueuedPackets);
            return;
        }
        FnetShimCopyPacket(FnetPacket, GluePacket->Data, TotalLength);
        GluePacket->Length = TotalLength;
        GlueIfReference(LoopIf);
        GluePacket->GlueIf = LoopIf;
        FnetPortQueueLocal(&GluePacket->Item, GlueInputRoutine, GluePacket);
        return;
    }

    NdisStatus = AllocatePacketWithBuffer(&Packet.NdisPacket, NULL, TotalLength);
    if (NdisStatus != NDIS_STATUS_SUCCESS)
        return;

    GetDataPtr(Packet.NdisPacket, 0, (PCHAR*)&Packet.Header, &Packet.TotalSize);
    Packet.MappedHeader = TRUE;
    ASSERT(Packet.TotalSize == TotalLength);

    FnetShimCopyPacket(FnetPacket, Packet.Header, TotalLength);

    Packet.HeaderSize = sizeof(IPv4_HEADER);
    Packet.TotalSize = TotalLength;
    Packet.SrcAddr = LocalAddress;
    Packet.DstAddr = RemoteAddress;

    IPSendDatagram(&Packet, NCE);
}

/* ReactOS reassembly keeps the first fragment's header (MF set, short total length); FNET would drop that */
static
VOID
GlueFixReassembledHeader(PUCHAR Data, ULONG Length)
{
    PIPv4_HEADER Header = (PIPv4_HEADER)Data;
    ULONG HeaderLength = (Header->VerIHL & 0x0F) << 2;
    USHORT Flags = WN2H(Header->FlagsFragOfs);

    /* Frames may carry Ethernet padding past the IP length; FNET trims that, so only reassembled datagrams need fixing */
    if ((Flags & (IPv4_MF_MASK | IPv4_FRAGOFS_MASK)) == 0)
        return;

    Header->TotalLength = WH2N((USHORT)Length);
    Header->FlagsFragOfs = WH2N(Flags & ~(IPv4_MF_MASK | IPv4_FRAGOFS_MASK));
    Header->Checksum = 0;
    Header->Checksum = (USHORT)IPv4Checksum(Header, HeaderLength, 0);
}

static
VOID
GlueInputRoutine(PVOID Context)
{
    PFNET_GLUE_PACKET GluePacket = Context;

    if (GluePacket->GlueIf->Netif)
        FnetShimNetifInput(GluePacket->GlueIf->Netif, GluePacket->Data, GluePacket->Length);

    InterlockedDecrement(&QueuedPackets);
    GlueIfDereference(GluePacket->GlueIf);
    ExFreePoolWithTag(GluePacket, FNET_GLUE_PACKET_TAG);
}

/* IRQL <= DISPATCH_LEVEL, from TCPReceive: copy the datagram and hand it to the worker */
void
LibIPInsertPacket(void *ifarg, const void *const data, const u32_t size)
{
    PFNET_GLUE_IF GlueIf = ifarg;
    PFNET_GLUE_PACKET GluePacket;
    const IPv4_HEADER *Header = data;

    if (!GlueIf || size < sizeof(IPv4_HEADER))
        return;

    /* FNET only ever sees TCP */
    if (Header->Protocol != IPPROTO_TCP)
        return;

    if (InterlockedIncrement(&QueuedPackets) > FNET_GLUE_MAX_QUEUED_PACKETS)
    {
        InterlockedDecrement(&QueuedPackets);
        return;
    }

    GluePacket = ExAllocatePoolWithTag(NonPagedPool, FIELD_OFFSET(FNET_GLUE_PACKET, Data[size]), FNET_GLUE_PACKET_TAG);
    if (!GluePacket)
    {
        InterlockedDecrement(&QueuedPackets);
        return;
    }

    RtlCopyMemory(GluePacket->Data, data, size);
    GluePacket->Length = size;
    GlueFixReassembledHeader(GluePacket->Data, size);

    GlueIfReference(GlueIf);
    GluePacket->GlueIf = GlueIf;
    FnetPortQueueWork(&GluePacket->Item, GlueInputRoutine, GluePacket);
}

static
VOID
GlueCreateRoutine(PVOID Context)
{
    PFNET_GLUE_IF GlueIf = Context;

    GlueIf->Netif = FnetShimNetifCreate(GlueIf, GlueInterfaceMtu(GlueIf->Interface, GlueIf->Loopback), GlueIf->Loopback, GlueOutput);
    if (GlueIf->Netif)
        GlueIf->ScopeId = FnetShimNetifScopeId(GlueIf->Netif);
}

VOID
TCPRegisterInterface(PIP_INTERFACE IF)
{
    PFNET_GLUE_IF GlueIf;

    GlueIf = ExAllocatePoolWithTag(NonPagedPool, sizeof(*GlueIf), FNET_GLUE_IF_TAG);
    if (!GlueIf)
    {
        IF->TCPContext = NULL;
        return;
    }

    RtlZeroMemory(GlueIf, sizeof(*GlueIf));
    GlueIf->RefCount = 1;
    GlueIf->Interface = IF;
    /* The global Loopback pointer is only set after IPCreateInterface returns; the loopback interface is the one without a link layer */
    GlueIf->Loopback = (IF->Context == NULL && IF->HeaderSize == 0);

    FnetPortRunSync(GlueCreateRoutine, GlueIf);
    if (!GlueIf->Netif)
    {
        DbgPrint("TCPIP: no FNET netif for interface %p, TCP will not use it\n", IF);
        GlueIfDereference(GlueIf);
        IF->TCPContext = NULL;
        return;
    }

    IF->TCPContext = GlueIf;
    TCPUpdateInterfaceIPInformation(IF);
}

static
VOID
GlueReleaseRoutine(PVOID Context)
{
    PFNET_GLUE_IF GlueIf = Context;

    if (GlueIf->Netif)
    {
        FnetShimNetifRelease(GlueIf->Netif);
        GlueIf->Netif = NULL;
    }
    if (DefaultIf == GlueIf)
        DefaultIf = NULL;
}

VOID
TCPUnregisterInterface(PIP_INTERFACE IF)
{
    PFNET_GLUE_IF GlueIf = IF->TCPContext;

    if (!GlueIf)
        return;

    /* Connections over this interface cannot continue; abort them before the netif goes */
    FnetGlueAbortAddress(GlueIf->Address);

    /* Packets already queued for this interface keep their own reference and are dropped */
    FnetPortRunSync(GlueReleaseRoutine, GlueIf);
    IF->TCPContext = NULL;
    GlueIfDereference(GlueIf);
}

typedef struct _GLUE_ADDRESS_CALL
{
    PFNET_GLUE_IF GlueIf;
    ULONG Address;
    ULONG Mask;
} GLUE_ADDRESS_CALL;

static
VOID
GlueAddressRoutine(PVOID Context)
{
    GLUE_ADDRESS_CALL *Call = Context;

    if (!Call->GlueIf->Netif)
        return;

    FnetShimNetifSetMtu(Call->GlueIf->Netif, GlueInterfaceMtu(Call->GlueIf->Interface, Call->GlueIf->Loopback));
    FnetShimNetifSetAddress(Call->GlueIf->Netif, Call->Address, Call->Mask);

    /* FNET still uses its own route for MSS on unknown destinations; point it at a real adapter */
    if (!Call->GlueIf->Loopback && Call->Address && !DefaultIf)
    {
        DefaultIf = Call->GlueIf;
        FnetShimNetifSetDefault(Call->GlueIf->Netif);
    }
}

VOID
TCPUpdateInterfaceIPInformation(PIP_INTERFACE IF)
{
    PFNET_GLUE_IF GlueIf = IF->TCPContext;
    GLUE_ADDRESS_CALL Call;

    if (!GlueIf)
        return;

    Call.GlueIf = GlueIf;
    GetInterfaceIPv4Address(IF, ADE_UNICAST, &Call.Address);
    GetInterfaceIPv4Address(IF, ADE_ADDRMASK, &Call.Mask);

    if (Call.Address == GlueIf->Address && Call.Mask == GlueIf->Mask)
        return;

    /* Connections bound to the old address are dead; lwIP aborts them on an address change too */
    if (GlueIf->Address && GlueIf->Address != Call.Address)
        FnetGlueAbortAddress(GlueIf->Address);

    GlueIf->Address = Call.Address;
    GlueIf->Mask = Call.Mask;
    FnetPortRunSync(GlueAddressRoutine, &Call);

    TI_DbgPrint(MIN_TRACE, ("FNET netif %lu: address %lx mask %lx MTU %lu%s\n",
                GlueIf->ScopeId, Call.Address, Call.Mask, GlueInterfaceMtu(IF, GlueIf->Loopback),
                GlueIf->Loopback ? " (loopback)" : ""));
}

VOID
TCPUpdateInterfaceLinkStatus(PIP_INTERFACE IF)
{
    /* lwIP only tracked link state; it does not drop connections, so nothing to do here either */
    UNREFERENCED_PARAMETER(IF);
}
