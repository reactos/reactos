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

  TI_DbgPrint(MAX_TRACE, ("Called (NdisPacket = %x)\n", NdisPacket));

  IPPacket.NdisPacket = NdisPacket;
  IPPacket.HeaderSize = 0;
  GetDataPtr( NdisPacket, 0, (PCHAR *)&IPPacket.Header, &IPPacket.TotalSize );

  TI_DbgPrint(MAX_TRACE, 
	      ("Doing receive (complete: %x, context %x, packet %x)\n",
	       PC(NdisPacket)->DLComplete, Context, NdisPacket));
  IPReceive(Context, &IPPacket);
  TI_DbgPrint(MAX_TRACE, 
	      ("Finished receive (complete: %x, context %x, packet %x)\n",
	       PC(NdisPacket)->DLComplete, Context, NdisPacket));
  PC(NdisPacket)->DLComplete(Context, NdisPacket, NDIS_STATUS_SUCCESS);
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
  PIP_ADDRESS Address;
  NDIS_STATUS Status;

  Status = NDIS_STATUS_SUCCESS;

  TI_DbgPrint(MID_TRACE, ("Called.\n"));

  Address = AddrBuildIPv4(LOOPBACK_ADDRESS_IPv4);
  if (Address != NULL)
    {
      LLIP_BIND_INFO BindInfo;

      /* Bind the adapter to network (IP) layer */
      BindInfo.Context = NULL;
      BindInfo.HeaderSize = 0;
      BindInfo.MinFrameSize = 0;
      BindInfo.MTU = 16384;
      BindInfo.Address = NULL;
      BindInfo.AddressLength = 0;
      BindInfo.Transmit = LoopTransmit;

      Loopback = IPCreateInterface(&BindInfo);

      if ((Loopback != NULL) && (IPCreateNTE(Loopback, Address, 8)))
        {
          IPRegisterInterface(Loopback);
        }
      else
        {
          Status = NDIS_STATUS_RESOURCES;
        }
    }
  else
    {
      Status = NDIS_STATUS_RESOURCES;
    }

  if (!NT_SUCCESS(Status))
    {
      LoopUnregisterAdapter(NULL);
    }

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
