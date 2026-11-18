/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/tcpip/ip/network/dhcp6.c
 * PURPOSE:     DHCPv6 Solicit and Request client
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * Solicit, Advertise, Request, and Reply from RFC 8415 Sections 7.3 and
 * 18.2 (https://www.rfc-editor.org/rfc/rfc8415). All-DHCP-Agents multicast
 * FF02::1:2 and UDP ports 546/547 are Sections 7.1 and 7.2. DUID-LL is
 * Section 11.2. Ethernet multicast mapping is RFC 2464 Section 7
 * (https://www.rfc-editor.org/rfc/rfc2464). The server is
 * virtual-machines/veros-ntfs/Send-Ipv6Peer.ps1 -Mode Dhcp6, which offers
 * fec0::10. Accept line is "DHCP6 ADDR fec0::10".
 */

#include "precomp.h"

#include <dhcp6.h>
#include <udp.h>
#include <icmp.h>
#include <checksum.h>

#define DHCP6_OPT_CLIENTID  1
#define DHCP6_OPT_SERVERID  2
#define DHCP6_OPT_IA_NA     3
#define DHCP6_OPT_IAADDR    5
#define DHCP6_OPT_ELAPSED   8

#define DHCP6_SAVED_MAX     96

static VOID
Dhcp6TransmitComplete(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    NDIS_STATUS NdisStatus)
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(NdisStatus);
    FreeNdisPacket(NdisPacket);
}

static PIP_INTERFACE Dhcp6Interface;
static UCHAR Dhcp6Xid[3];
static UCHAR Dhcp6ClientId[16];
static USHORT Dhcp6ClientIdLen;
static UCHAR Dhcp6ServerOpt[DHCP6_SAVED_MAX];
static USHORT Dhcp6ServerOptLen;
static UCHAR Dhcp6IaNaOpt[DHCP6_SAVED_MAX];
static USHORT Dhcp6IaNaOptLen;
static BOOLEAN Dhcp6WantRequest;
static BOOLEAN Dhcp6Done;
static BOOLEAN Dhcp6Started;

static USHORT
Dhcp6Read16(PUCHAR Data)
{
    return (USHORT)(((USHORT)Data[0] << 8) | Data[1]);
}

static VOID
Dhcp6Write16(PUCHAR Data, USHORT Value)
{
    Data[0] = (UCHAR)(Value >> 8);
    Data[1] = (UCHAR)(Value & 0xFF);
}

static BOOLEAN
Dhcp6OptionAt(
    PUCHAR Msg,
    UINT Length,
    UINT Offset,
    USHORT *Code,
    USHORT *OptLen,
    PUCHAR *Data)
{
    if (Offset + 4 > Length)
        return FALSE;

    *Code = Dhcp6Read16(Msg + Offset);
    *OptLen = Dhcp6Read16(Msg + Offset + 2);
    if (Offset + 4 + *OptLen > Length)
        return FALSE;

    *Data = Msg + Offset + 4;
    return TRUE;
}

static BOOLEAN
Dhcp6FindOption(
    PUCHAR Msg,
    UINT Length,
    USHORT Want,
    PUCHAR *Data,
    USHORT *OptLen,
    UINT *HeaderOffset)
{
    UINT Offset = 4;

    while (Offset + 4 <= Length)
    {
        USHORT Code;
        USHORT ThisLen;
        PUCHAR ThisData;

        if (!Dhcp6OptionAt(Msg, Length, Offset, &Code, &ThisLen, &ThisData))
            return FALSE;

        if (Code == Want)
        {
            *Data = ThisData;
            *OptLen = ThisLen;
            if (HeaderOffset)
                *HeaderOffset = Offset;
            return TRUE;
        }

        Offset += 4 + ThisLen;
    }

    return FALSE;
}

static BOOLEAN
Dhcp6IaNaHasFec0_10(PUCHAR IaNa, USHORT IaNaLen)
{
    UINT Offset;

    if (IaNaLen < 12)
        return FALSE;

    Offset = 12;
    while (Offset + 4 <= IaNaLen)
    {
        USHORT Code = Dhcp6Read16(IaNa + Offset);
        USHORT OptLen = Dhcp6Read16(IaNa + Offset + 2);

        if (Offset + 4 + OptLen > IaNaLen)
            return FALSE;

        if (Code == DHCP6_OPT_IAADDR && OptLen >= 16)
        {
            PUCHAR Addr = IaNa + Offset + 4;

            if (Addr[0] == 0xFE && Addr[1] == 0xC0 && Addr[15] == 0x10)
            {
                UINT i;
                BOOLEAN Zeros = TRUE;

                for (i = 2; i < 15; i++)
                {
                    if (Addr[i] != 0)
                        Zeros = FALSE;
                }
                if (Zeros)
                    return TRUE;
            }
        }

        Offset += 4 + OptLen;
    }

    return FALSE;
}

static BOOLEAN
Dhcp6SaveOption(PUCHAR Msg, UINT Length, UINT Offset, PUCHAR Dest, USHORT *DestLen)
{
    USHORT OptLen;

    if (Offset + 4 > Length)
        return FALSE;

    OptLen = Dhcp6Read16(Msg + Offset + 2);
    if (Offset + 4 + OptLen > Length)
        return FALSE;
    if ((UINT)(4 + OptLen) > DHCP6_SAVED_MAX)
        return FALSE;

    RtlCopyMemory(Dest, Msg + Offset, 4 + OptLen);
    *DestLen = (USHORT)(4 + OptLen);
    return TRUE;
}

static VOID
Dhcp6Install(PIP_INTERFACE Interface)
{
    if (Dhcp6Done || !Interface)
        return;

    AddrInitIPv6(&Interface->Dhcp6Address,
                 WH2N(0xFEC0), 0, 0, 0, 0, 0, 0, WH2N(0x0010));
    NBAddNeighbor(Interface, &Interface->Dhcp6Address,
                  Interface->Address, Interface->AddressLength,
                  NUD_PERMANENT, 0);
    Dhcp6Done = TRUE;
    DbgPrint("DHCP6 ADDR fec0::10\n");
}

static BOOLEAN
Dhcp6Transmit(UCHAR MsgType)
{
    UCHAR Message[256];
    UINT MsgLen;
    UCHAR PacketBuf[512];
    PNDIS_PACKET NdisPacket;
    NDIS_STATUS NdisStatus;
    PIPv6_HEADER Header;
    PUDP_HEADER Udp;
    PVOID Data;
    UINT Contig;
    UINT TotalSize;
    UINT UdpLength;
    IP_ADDRESS Src;
    IP_ADDRESS Dst;
    UCHAR LinkAddress[6];
    UCHAR AddrBytes[16];

    if (!Dhcp6Interface || AddrIsUnspecified(&Dhcp6Interface->LinkLocal))
        return FALSE;
    if (Dhcp6Interface->AddressLength != 6 || !Dhcp6Interface->Address)
        return FALSE;

    MsgLen = 0;
    Message[MsgLen++] = MsgType;
    Message[MsgLen++] = Dhcp6Xid[0];
    Message[MsgLen++] = Dhcp6Xid[1];
    Message[MsgLen++] = Dhcp6Xid[2];

    if (MsgLen + 4 + Dhcp6ClientIdLen > sizeof(Message))
        return FALSE;
    Dhcp6Write16(Message + MsgLen, DHCP6_OPT_CLIENTID);
    Dhcp6Write16(Message + MsgLen + 2, Dhcp6ClientIdLen);
    MsgLen += 4;
    RtlCopyMemory(Message + MsgLen, Dhcp6ClientId, Dhcp6ClientIdLen);
    MsgLen += Dhcp6ClientIdLen;

    if (MsgType == DHCP6_MSG_REQUEST)
    {
        if (!Dhcp6ServerOptLen || !Dhcp6IaNaOptLen)
            return FALSE;
        if (MsgLen + Dhcp6ServerOptLen + Dhcp6IaNaOptLen + 6 > sizeof(Message))
            return FALSE;
        RtlCopyMemory(Message + MsgLen, Dhcp6ServerOpt, Dhcp6ServerOptLen);
        MsgLen += Dhcp6ServerOptLen;
        RtlCopyMemory(Message + MsgLen, Dhcp6IaNaOpt, Dhcp6IaNaOptLen);
        MsgLen += Dhcp6IaNaOptLen;
    }
    else
    {
        if (MsgLen + 16 > sizeof(Message))
            return FALSE;
        Dhcp6Write16(Message + MsgLen, DHCP6_OPT_IA_NA);
        Dhcp6Write16(Message + MsgLen + 2, 12);
        MsgLen += 4;
        Message[MsgLen++] = 0;
        Message[MsgLen++] = 0;
        Message[MsgLen++] = 0;
        Message[MsgLen++] = 1;
        RtlZeroMemory(Message + MsgLen, 8);
        MsgLen += 8;
    }

    if (MsgLen + 6 > sizeof(Message))
        return FALSE;
    Dhcp6Write16(Message + MsgLen, DHCP6_OPT_ELAPSED);
    Dhcp6Write16(Message + MsgLen + 2, 2);
    MsgLen += 4;
    Dhcp6Write16(Message + MsgLen, 0);
    MsgLen += 2;

    UdpLength = sizeof(UDP_HEADER) + MsgLen;
    TotalSize = IPv6_HEADER_SIZE + UdpLength;
    if (TotalSize > sizeof(PacketBuf))
        return FALSE;

    Src = Dhcp6Interface->LinkLocal;
    AddrInitIPv6(&Dst, WH2N(0xFF02), 0, 0, 0, 0, WH2N(0x0001), 0, WH2N(0x0002));

    RtlZeroMemory(PacketBuf, TotalSize);
    Header = (PIPv6_HEADER)PacketBuf;
    Header->VTF = DH2N(0x60000000);
    Header->PayloadLength = WH2N((USHORT)UdpLength);
    Header->NextHeader = IPPROTO_UDP;
    Header->HopLimit = 1;
    RtlCopyMemory(Header->SrcAddr, Src.Address.IPv6Address, sizeof(IPv6_RAW_ADDRESS));
    RtlCopyMemory(Header->DstAddr, Dst.Address.IPv6Address, sizeof(IPv6_RAW_ADDRESS));

    Udp = (PUDP_HEADER)(PacketBuf + IPv6_HEADER_SIZE);
    Udp->SourcePort = WH2N(DHCP6_CLIENT_PORT);
    Udp->DestPort = WH2N(DHCP6_SERVER_PORT);
    Udp->Length = WH2N((USHORT)UdpLength);
    Udp->Checksum = 0;
    RtlCopyMemory((PUCHAR)Udp + sizeof(UDP_HEADER), Message, MsgLen);
    Udp->Checksum = IPv6PseudoChecksum(&Src, &Dst, (USHORT)UdpLength,
                                       IPPROTO_UDP, Udp, UdpLength);
    if (Udp->Checksum == 0)
        Udp->Checksum = 0xFFFF;

    NdisStatus = AllocatePacketWithBuffer(&NdisPacket, NULL, TotalSize);
    if (!NT_SUCCESS(NdisStatus))
        return FALSE;

    GetDataPtr(NdisPacket, 0, (PCHAR *)&Data, &Contig);
    if (!Data || Contig < TotalSize)
    {
        FreeNdisPacket(NdisPacket);
        return FALSE;
    }
    RtlCopyMemory(Data, PacketBuf, TotalSize);

    RtlCopyMemory(AddrBytes, Dst.Address.IPv6Address, sizeof(AddrBytes));
    LinkAddress[0] = 0x33;
    LinkAddress[1] = 0x33;
    LinkAddress[2] = AddrBytes[12];
    LinkAddress[3] = AddrBytes[13];
    LinkAddress[4] = AddrBytes[14];
    LinkAddress[5] = AddrBytes[15];

    PC(NdisPacket)->DLComplete = Dhcp6TransmitComplete;
    (*Dhcp6Interface->Transmit)(Dhcp6Interface->Context, NdisPacket, 0,
                                LinkAddress, LAN_PROTO_IPv6);
    DbgPrint("DHCP6 send type %u\n", MsgType);
    return TRUE;
}

VOID DHCP6Receive(PUCHAR Payload, UINT PayloadLength)
{
    PUCHAR Client;
    USHORT ClientLen;
    PUCHAR IaNa;
    USHORT IaNaLen;
    UINT ServerOff;
    UINT IaOff;
    UCHAR MsgType;

    if (!Dhcp6Interface || Dhcp6Done || PayloadLength < 4)
        return;

    if (Payload[1] != Dhcp6Xid[0] ||
        Payload[2] != Dhcp6Xid[1] ||
        Payload[3] != Dhcp6Xid[2])
        return;

    if (!Dhcp6FindOption(Payload, PayloadLength, DHCP6_OPT_CLIENTID,
                         &Client, &ClientLen, NULL))
        return;
    if (ClientLen != Dhcp6ClientIdLen ||
        RtlCompareMemory(Client, Dhcp6ClientId, ClientLen) != ClientLen)
        return;

    if (!Dhcp6FindOption(Payload, PayloadLength, DHCP6_OPT_IA_NA,
                         &IaNa, &IaNaLen, &IaOff))
        return;
    if (!Dhcp6IaNaHasFec0_10(IaNa, IaNaLen))
        return;

    MsgType = Payload[0];
    if (MsgType == DHCP6_MSG_ADVERTISE)
    {
        if (!Dhcp6FindOption(Payload, PayloadLength, DHCP6_OPT_SERVERID,
                             &Client, &ClientLen, &ServerOff))
            return;
        if (!Dhcp6SaveOption(Payload, PayloadLength, ServerOff,
                             Dhcp6ServerOpt, &Dhcp6ServerOptLen))
            return;
        if (!Dhcp6SaveOption(Payload, PayloadLength, IaOff,
                             Dhcp6IaNaOpt, &Dhcp6IaNaOptLen))
            return;
        Dhcp6WantRequest = TRUE;
        Dhcp6Transmit(DHCP6_MSG_REQUEST);
        return;
    }

    if (MsgType == DHCP6_MSG_REPLY)
        Dhcp6Install(Dhcp6Interface);
}

BOOLEAN
DHCP6SourceForDestination(
    PIP_INTERFACE Interface,
    PIP_ADDRESS Destination,
    PIP_ADDRESS Source)
{
    UINT i;

    if (!Interface || !Destination || Destination->Type != IP_ADDRESS_V6)
        return FALSE;
    if (AddrIsUnspecified(&Interface->Dhcp6Address))
        return FALSE;

    for (i = 0; i < 4; i++)
    {
        if (Destination->Address.IPv6Address[i] !=
            Interface->Dhcp6Address.Address.IPv6Address[i])
            return FALSE;
    }

    if (Source)
        *Source = Interface->Dhcp6Address;
    return TRUE;
}

static VOID NTAPI
Dhcp6Thread(PVOID Context)
{
    LARGE_INTEGER Delay;
    UINT Attempt;

    UNREFERENCED_PARAMETER(Context);

    Delay.QuadPart = -3LL * 10000000LL;
    for (Attempt = 0; Attempt < 8 && !Dhcp6Done; Attempt++)
    {
        if (Dhcp6WantRequest)
            Dhcp6Transmit(DHCP6_MSG_REQUEST);
        else
            Dhcp6Transmit(DHCP6_MSG_SOLICIT);
        KeDelayExecutionThread(KernelMode, FALSE, &Delay);
    }
    PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS DHCP6Startup(VOID)
{
    return STATUS_SUCCESS;
}

NTSTATUS DHCP6Shutdown(VOID)
{
    Dhcp6Done = TRUE;
    return STATUS_SUCCESS;
}

NTSTATUS DHCP6StartOnInterface(PIP_INTERFACE Interface)
{
    LARGE_INTEGER Seed;
    HANDLE Thread;
    NTSTATUS Status;

    if (!Interface)
        return STATUS_INVALID_PARAMETER;

    if (AddrIsUnspecified(&Interface->LinkLocal) ||
        !Interface->Address || Interface->AddressLength != 6)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    if (Dhcp6Started)
        return STATUS_SUCCESS;

    Dhcp6Interface = Interface;
    Dhcp6ClientId[0] = 0x00;
    Dhcp6ClientId[1] = 0x03;
    Dhcp6ClientId[2] = 0x00;
    Dhcp6ClientId[3] = 0x01;
    RtlCopyMemory(Dhcp6ClientId + 4, Interface->Address, 6);
    Dhcp6ClientIdLen = 10;

    Seed = KeQueryPerformanceCounter(NULL);
    Dhcp6Xid[0] = (UCHAR)(Seed.LowPart >> 16);
    Dhcp6Xid[1] = (UCHAR)(Seed.LowPart >> 8);
    Dhcp6Xid[2] = (UCHAR)Seed.LowPart;
    if (Dhcp6Xid[0] == 0 && Dhcp6Xid[1] == 0 && Dhcp6Xid[2] == 0)
        Dhcp6Xid[2] = 1;

    Dhcp6Started = TRUE;
    Status = PsCreateSystemThread(&Thread, THREAD_ALL_ACCESS, NULL, NULL, NULL,
                                  Dhcp6Thread, NULL);
    if (!NT_SUCCESS(Status))
    {
        Dhcp6Started = FALSE;
        return Status;
    }
    ZwClose(Thread);
    return STATUS_SUCCESS;
}

VOID DHCP6StopOnInterface(PIP_INTERFACE Interface)
{
    if (Interface == Dhcp6Interface)
        Dhcp6Done = TRUE;
}
