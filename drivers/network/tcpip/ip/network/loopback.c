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

VOID LoopPassiveWorker(
  PVOID Context)
{
  PIP_PACKET IPPacket = Context;

  /* IPReceive() takes care of the NDIS packet */
  IPReceive(Loopback, IPPacket);

  ExFreePool(IPPacket);
}

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
 *   Type        = LAN protocol type
 */
{
    PCHAR PacketBuffer;
    UINT PacketLength;
    PNDIS_PACKET XmitPacket;
    NDIS_STATUS NdisStatus;
    PIP_PACKET IPPacket;

    ASSERT_KM_POINTER(NdisPacket);
    ASSERT_KM_POINTER(PC(NdisPacket));
    ASSERT_KM_POINTER(PC(NdisPacket)->DLComplete);

    if (Type != LAN_PROTO_IPv4)
    {
        TI_DbgPrint(MAX_TRACE, ("Received unsupported protocol %u\n", Type));
        PC(NdisPacket)->DLComplete(PC(NdisPacket)->Context, NdisPacket, NDIS_STATUS_NOT_SUPPORTED);
        return;
    }

    TI_DbgPrint(MAX_TRACE, ("Called (NdisPacket = %x)\n", NdisPacket));

    GetDataPtr( NdisPacket, 0, &PacketBuffer, &PacketLength );

    NdisStatus = AllocatePacketWithBuffer
        ( &XmitPacket, PacketBuffer, PacketLength );

    if( NT_SUCCESS(NdisStatus) ) {
        IPPacket = ExAllocatePool(NonPagedPool, sizeof(IP_PACKET));
        if (IPPacket)
        {
            IPInitializePacket(IPPacket, 0);

            IPPacket->NdisPacket = XmitPacket;

            GetDataPtr(IPPacket->NdisPacket,
                       0,
                       (PCHAR*)&IPPacket->Header,
                       &IPPacket->TotalSize);

            IPPacket->MappedHeader = TRUE;

            if (!ChewCreate(LoopPassiveWorker, IPPacket))
            {
                IPPacket->Free(IPPacket);
                ExFreePool(IPPacket);
                NdisStatus = NDIS_STATUS_RESOURCES;
            }
        }
        else
            NdisStatus = NDIS_STATUS_RESOURCES;
    }

    (PC(NdisPacket)->DLComplete)
        ( PC(NdisPacket)->Context, NdisPacket, NdisStatus );
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
  BindInfo.Address = NULL;
  BindInfo.AddressLength = 0;
  BindInfo.Transmit = LoopTransmit;

  Loopback = IPCreateInterface(&BindInfo);
  if (!Loopback) return NDIS_STATUS_RESOURCES;

  Loopback->MTU = 16384;

  Loopback->Name.Buffer = L"Loopback";
  Loopback->Name.MaximumLength = Loopback->Name.Length =
      (USHORT)wcslen(Loopback->Name.Buffer) * sizeof(WCHAR);

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
