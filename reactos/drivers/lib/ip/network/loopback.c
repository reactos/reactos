/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        datalink/loopback.c
 * PURPOSE:     Loopback adapter
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
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

  ASSERT_KM_POINTER(NdisPacket);
  ASSERT_KM_POINTER(PC(NdisPacket));
  ASSERT_KM_POINTER(PC(NdisPacket)->DLComplete);

  TI_DbgPrint(MAX_TRACE, ("Called (NdisPacket = %x)\n", NdisPacket));

  IPPacket.NdisPacket = NdisPacket;
  IPPacket.HeaderSize = 0;
  GetDataPtr( NdisPacket, 0, (PCHAR *)&IPPacket.Header, &IPPacket.TotalSize );
  IPPacket.Header += Offset;
  IPPacket.ContigSize = IPPacket.TotalSize;
  IPPacket.Position = Offset;

  TI_DbgPrint(MAX_TRACE, 
	      ("Doing receive (complete: %x, context %x, packet %x)\n",
	       PC(NdisPacket)->DLComplete, Context, NdisPacket));
  IPReceive(Context, &IPPacket);
  TI_DbgPrint(MAX_TRACE, 
	      ("Finished receive (complete: %x, context %x, packet %x)\n",
	       PC(NdisPacket)->DLComplete, Context, NdisPacket));
  PC(NdisPacket)->DLComplete(PC(NdisPacket)->Context, NdisPacket, NDIS_STATUS_SUCCESS);
  TI_DbgPrint(MAX_TRACE, ("Done\n"));
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
  NDIS_STATUS Status;
  LLIP_BIND_INFO BindInfo;

  Status = NDIS_STATUS_SUCCESS;

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
  
  AddrInitIPv4(&Loopback->Unicast, LOOPBACK_ADDRESS_IPv4);
  AddrInitIPv4(&Loopback->Netmask, LOOPBACK_ADDRMASK_IPv4);
  
  IPRegisterInterface(Loopback);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return Status;
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
