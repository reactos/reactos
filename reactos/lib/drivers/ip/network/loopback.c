/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        lib/drivers/ip/network/loopback.c
 * PURPOSE:     Loopback adapter
 * PROGRAMMERS: Cameron Gutman (cgutman@reactos.org)
 */

#include "precomp.h"

PIP_INTERFACE Loopback = NULL;

VOID LoopTransmit(
  PVOID Context,
  PNDIS_PACKET NdisPacket,
  UINT Offset,
  PVOID LinkAddress,
  USHORT Type)
/*
 * FUNCTION: Transmits a packet
 * ARGUMENTS:
 *   Context     = Pointer to context information (NULL)
 *   NdisPacket  = Pointer to NDIS packet to send
 *   Offset      = Offset in packet where packet data starts
 *   LinkAddress = Pointer to link address
 *   Type        = LAN protocol type (unused)
 */
{
    IP_PACKET IPPacket;
    PNDIS_PACKET XmitPacket;
    PNDIS_BUFFER NdisBuffer;
    NDIS_STATUS NdisStatus;
    PCHAR PacketBuffer;
    UINT PacketLength;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    GetDataPtr( NdisPacket, Offset, &PacketBuffer, &PacketLength );

    NdisStatus = AllocatePacketWithBuffer
        ( &XmitPacket, PacketBuffer, PacketLength );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        (PC(NdisPacket)->DLComplete)( PC(NdisPacket)->Context, NdisPacket, NdisStatus );
        return;
    }

    NdisGetFirstBufferFromPacket(XmitPacket,
                                 &NdisBuffer,
                                 &IPPacket.Header,
                                 &IPPacket.ContigSize,
                                 &IPPacket.TotalSize);

    IPPacket.NdisPacket = XmitPacket;
    IPPacket.Position = 0;

    TI_DbgPrint(MID_TRACE,
		 ("ContigSize: %d, TotalSize: %d\n",
		  IPPacket.ContigSize, IPPacket.TotalSize));
		
    IPReceive(Loopback, &IPPacket);

    FreeNdisPacket(XmitPacket);

    (PC(NdisPacket)->DLComplete)( PC(NdisPacket)->Context, NdisPacket, NDIS_STATUS_SUCCESS );
}

NDIS_STATUS LoopRegisterAdapter(
  PNDIS_STRING AdapterName,
  PLAN_ADAPTER *Adapter)
/*
 * FUNCTION: Registers loopback adapter with the network layer
 * ARGUMENTS:
 *   AdapterName = Unused
 *   Adapter     = Unused
 * RETURNS:
 *   Status of operation
 */
{
  LLIP_BIND_INFO BindInfo;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  /* Bind the adapter to network (IP) layer */
  BindInfo.Context = NULL;
  BindInfo.HeaderSize = 0;
  BindInfo.MinFrameSize = 0;
  BindInfo.MTU = 16384;
  BindInfo.Address = NULL;
  BindInfo.AddressLength = 0;
  BindInfo.Transmit = LoopTransmit;

  Loopback = IPCreateInterface(&BindInfo);

  Loopback->Name.Buffer = L"Loopback";
  Loopback->Name.MaximumLength = Loopback->Name.Length =
      wcslen(Loopback->Name.Buffer) * sizeof(WCHAR);

  AddrInitIPv4(&Loopback->Unicast, LOOPBACK_ADDRESS_IPv4);
  AddrInitIPv4(&Loopback->Netmask, LOOPBACK_ADDRMASK_IPv4);
  AddrInitIPv4(&Loopback->Broadcast, LOOPBACK_BCASTADDR_IPv4);

  IPRegisterInterface(Loopback);
  IPAddInterfaceRoute(Loopback);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS LoopUnregisterAdapter(
  PLAN_ADAPTER Adapter)
/*
 * FUNCTION: Unregisters loopback adapter with the network layer
 * ARGUMENTS:
 *   Adapter = Unused
 * RETURNS:
 *   Status of operation
 * NOTES:
 *   Does not care wether we have registered loopback adapter
 */
{
  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  if (Loopback != NULL)
    {
      IPUnregisterInterface(Loopback);
      IPDestroyInterface(Loopback);
      Loopback = NULL;
    }

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return NDIS_STATUS_SUCCESS;
}
