/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/routines.c
 * PURPOSE:     Common routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

static UINT RandomNumber = 0x12345678;


UINT Random(
    VOID)
/*
 * FUNCTION: Returns a pseudo random number
 * RETURNS:
 *     Pseudo random number
 */
{
    RandomNumber ^= 0x78563412;

    return RandomNumber;
}

#ifdef DBG
static VOID DisplayIPHeader(
    PCHAR Header,
    UINT Length)
{
    /* FIXME: IPv4 only */
    PIPv4_HEADER IPHeader = (PIPv4_HEADER)Header;

    DbgPrint("IPv4 header:\n");
    DbgPrint("VerIHL: 0x%x (version 0x%x, length %d 32-bit words)\n",
      IPHeader->VerIHL, (IPHeader->VerIHL & 0xF0) >> 4, IPHeader->VerIHL & 0x0F);
    DbgPrint("  Tos: %d\n", IPHeader->Tos);
    DbgPrint("  TotalLength: %d\n", WN2H(IPHeader->TotalLength));
    DbgPrint("  Id: %d\n", WN2H(IPHeader->Id));
    DbgPrint("  FlagsFragOfs: 0x%x (offset 0x%x)\n", WN2H(IPHeader->FlagsFragOfs), WN2H(IPHeader->FlagsFragOfs) & IPv4_FRAGOFS_MASK);
    if ((WN2H(IPHeader->FlagsFragOfs) & IPv4_DF_MASK) > 0) DbgPrint("    IPv4_DF - Don't fragment\n");
    if ((WN2H(IPHeader->FlagsFragOfs) & IPv4_MF_MASK) > 0) DbgPrint("    IPv4_MF - More fragments\n");
    DbgPrint("  Ttl: %d\n", IPHeader->Ttl);
    DbgPrint("  Protocol: %d\n", IPHeader->Protocol);
    DbgPrint("  Checksum: 0x%x\n", WN2H(IPHeader->Checksum));
    DbgPrint("  SrcAddr: %d.%d.%d.%d\n",
      ((IPHeader->SrcAddr >> 0) & 0xFF), ((IPHeader->SrcAddr >> 8) & 0xFF),
      ((IPHeader->SrcAddr >> 16) & 0xFF), ((IPHeader->SrcAddr >> 24) & 0xFF));
    DbgPrint("  DstAddr: %d.%d.%d.%d\n",
      ((IPHeader->DstAddr >> 0) & 0xFF), ((IPHeader->DstAddr >> 8) & 0xFF),
      ((IPHeader->DstAddr >> 16) & 0xFF), ((IPHeader->DstAddr >> 24) & 0xFF));
}

static VOID DisplayTCPHeader(
    PCHAR Header,
    UINT Length)
{
    /* FIXME: IPv4 only */
    PIPv4_HEADER IPHeader = (PIPv4_HEADER)Header;
    PTCPv4_HEADER TCPHeader;

    if (IPHeader->Protocol != IPPROTO_TCP) {
        DbgPrint("This is not a TCP datagram. Protocol is %d\n", IPHeader->Protocol);
        return;
    }

    TCPHeader = (PTCPv4_HEADER)((PCHAR)IPHeader + (IPHeader->VerIHL & 0x0F) * 4);

    DbgPrint("TCP header:\n");
    DbgPrint("  SourcePort: %d\n", WN2H(TCPHeader->SourcePort));
    DbgPrint("  DestinationPort: %d\n", WN2H(TCPHeader->DestinationPort));
    DbgPrint("  SequenceNumber: 0x%x\n", DN2H(TCPHeader->SequenceNumber));
    DbgPrint("  AckNumber: 0x%x\n", DN2H(TCPHeader->AckNumber));
    DbgPrint("  DataOffset: 0x%x (0x%x) 32-bit words\n", TCPHeader->DataOffset, TCPHeader->DataOffset >> 4);
    DbgPrint("  Flags: 0x%x (0x%x)\n", TCPHeader->Flags, TCPHeader->Flags & 0x3F);
    if ((TCPHeader->Flags & TCP_URG) > 0) DbgPrint("    TCP_URG - Urgent Pointer field significant\n");
    if ((TCPHeader->Flags & TCP_ACK) > 0) DbgPrint("    TCP_ACK - Acknowledgement field significant\n");
    if ((TCPHeader->Flags & TCP_PSH) > 0) DbgPrint("    TCP_PSH - Push Function\n");
    if ((TCPHeader->Flags & TCP_RST) > 0) DbgPrint("    TCP_RST - Reset the connection\n");
    if ((TCPHeader->Flags & TCP_SYN) > 0) DbgPrint("    TCP_SYN - Synchronize sequence numbers\n");
    if ((TCPHeader->Flags & TCP_FIN) > 0) DbgPrint("    TCP_FIN - No more data from sender\n");
    DbgPrint("  Window: 0x%x\n", WN2H(TCPHeader->Window));
    DbgPrint("  Checksum: 0x%x\n", WN2H(TCPHeader->Checksum));
    DbgPrint("  Urgent: 0x%x\n", WN2H(TCPHeader->Urgent));
}


VOID DisplayTCPPacket(
    PIP_PACKET IPPacket)
{
    UINT Length;
    PCHAR Buffer;

    if ((DebugTraceLevel & (DEBUG_PBUFFER | DEBUG_TCP)) != (DEBUG_PBUFFER | DEBUG_TCP)) {
        return;
    }

    if (!IPPacket) {
        TI_DbgPrint(MIN_TRACE, ("Cannot display null packet.\n"));
        return;
    }

    DisplayIPPacket(IPPacket);

	  TI_DbgPrint(MIN_TRACE, ("IPPacket is at (0x%X).\n", IPPacket));
    TI_DbgPrint(MIN_TRACE, ("Header buffer is at (0x%X).\n", IPPacket->Header));
    TI_DbgPrint(MIN_TRACE, ("Header size is (%d).\n", IPPacket->HeaderSize));
    TI_DbgPrint(MIN_TRACE, ("TotalSize (%d).\n", IPPacket->TotalSize));
    TI_DbgPrint(MIN_TRACE, ("ContigSize (%d).\n", IPPacket->ContigSize));
    TI_DbgPrint(MIN_TRACE, ("NdisPacket (0x%X).\n", IPPacket->NdisPacket));

    if (IPPacket->NdisPacket) {
        NdisQueryPacket(IPPacket->NdisPacket, NULL, NULL, NULL, &Length);
        Length -= MaxLLHeaderSize;
        Buffer = exAllocatePool(NonPagedPool, Length);
        Length = CopyPacketToBuffer(Buffer, IPPacket->NdisPacket, MaxLLHeaderSize, Length);
        DisplayTCPHeader(Buffer, Length);
        exFreePool(Buffer);
    } else {
        Buffer = IPPacket->Header;
        Length = IPPacket->ContigSize;
        DisplayTCPHeader(Buffer, Length);
    }
}
#endif

VOID DisplayIPPacket(
    PIP_PACKET IPPacket)
{
#ifdef DBG
    PCHAR p;
    UINT Length;
    PNDIS_BUFFER Buffer;
    PNDIS_BUFFER NextBuffer;
    PCHAR CharBuffer;

    if ((DebugTraceLevel & (DEBUG_PBUFFER | DEBUG_IP)) != (DEBUG_PBUFFER | DEBUG_IP)) {
        return;
    }

    if (!IPPacket) {
        TI_DbgPrint(MIN_TRACE, ("Cannot display null packet.\n"));
        return;
    }

	  TI_DbgPrint(MIN_TRACE, ("IPPacket is at (0x%X).\n", IPPacket));
    TI_DbgPrint(MIN_TRACE, ("Header buffer is at (0x%X).\n", IPPacket->Header));
    TI_DbgPrint(MIN_TRACE, ("Header size is (%d).\n", IPPacket->HeaderSize));
    TI_DbgPrint(MIN_TRACE, ("TotalSize (%d).\n", IPPacket->TotalSize));
    TI_DbgPrint(MIN_TRACE, ("ContigSize (%d).\n", IPPacket->ContigSize));
    TI_DbgPrint(MIN_TRACE, ("NdisPacket (0x%X).\n", IPPacket->NdisPacket));

    if (IPPacket->NdisPacket) {
        NdisQueryPacket(IPPacket->NdisPacket, NULL, NULL, &Buffer, NULL);
        for (; Buffer != NULL; Buffer = NextBuffer) {
            NdisGetNextBuffer(Buffer, &NextBuffer);
            NdisQueryBuffer(Buffer, (PVOID)&p, &Length);
	    //OskitDumpBuffer( p, Length );
        }
    } else {
        p      = IPPacket->Header;
        Length = IPPacket->ContigSize;
	//OskitDumpBuffer( p, Length );
    }

    CharBuffer = IPPacket->Header;
    Length = IPPacket->ContigSize;
    DisplayIPHeader(CharBuffer, Length);
#endif
}

