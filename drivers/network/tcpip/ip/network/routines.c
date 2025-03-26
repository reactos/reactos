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

#if DBG
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

#endif

VOID DisplayIPPacket(
    PIP_PACKET IPPacket)
{
#if DBG
    UINT Length;
    PCHAR CharBuffer;

    /* DbgQueryDebugFilterState returns (NTSTATUS)TRUE, (NTSTATUS)FALSE or a failure status code */
    if ((DbgQueryDebugFilterState(DPFLTR_TCPIP_ID, DEBUG_PBUFFER | DPFLTR_MASK) != (NTSTATUS)TRUE) ||
        (DbgQueryDebugFilterState(DPFLTR_TCPIP_ID, DEBUG_IP | DPFLTR_MASK) != (NTSTATUS)TRUE)) {
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
    TI_DbgPrint(MIN_TRACE, ("NdisPacket (0x%X).\n", IPPacket->NdisPacket));

    CharBuffer = IPPacket->Header;
    Length = IPPacket->HeaderSize;
    DisplayIPHeader(CharBuffer, Length);
#endif
}

