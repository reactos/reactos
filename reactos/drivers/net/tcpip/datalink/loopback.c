/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        datalink/loopback.c
 * PURPOSE:     Loopback adapter
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <loopback.h>
#include <ip.h>
#include <address.h>
#include <receive.h>
#include <transmit.h>
#include <routines.h>


WORK_QUEUE_ITEM LoopWorkItem;
PIP_INTERFACE Loopback = NULL;
/* Indicates wether the loopback interface is currently transmitting */
BOOLEAN LoopBusy = FALSE;
/* Loopback transmit queue */
PNDIS_PACKET LoopQueueHead = (PNDIS_PACKET)NULL;
PNDIS_PACKET LoopQueueTail = (PNDIS_PACKET)NULL;
/* Spin lock for protecting loopback transmit queue */
KSPIN_LOCK LoopQueueLock;


VOID STDCALL RealTransmit(
  PVOID Context)
/*
 * FUNCTION: Transmits one or more packet(s) in loopback queue to ourselves
 * ARGUMENTS:
 *   Context = Pointer to context information (loopback interface)
 */
{
  PNDIS_PACKET NdisPacket;
  PNDIS_BUFFER NdisBuffer;
  IP_PACKET IPPacket;
  KIRQL OldIrql;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
  KeAcquireSpinLockAtDpcLevel(&LoopQueueLock);

  while (TRUE)
    {
      /* Get the next packet from the queue (if any) */
      NdisPacket = LoopQueueHead;
      if (!NdisPacket)
        break;

      TI_DbgPrint(MAX_TRACE, ("NdisPacket (0x%X)\n", NdisPacket));

      LoopQueueHead = *(PNDIS_PACKET*)NdisPacket->u.s3.MacReserved;
      KeReleaseSpinLockFromDpcLevel(&LoopQueueLock);
      IPPacket.NdisPacket = NdisPacket;

      NdisGetFirstBufferFromPacket(NdisPacket,
        &NdisBuffer,
        &IPPacket.Header,
        &IPPacket.ContigSize,
        &IPPacket.TotalSize);
      IPReceive(Context, &IPPacket);
      AdjustPacket(NdisPacket, 0, PC(NdisPacket)->DLOffset);
      PC(NdisPacket)->DLComplete(Context, NdisPacket, NDIS_STATUS_SUCCESS);
      /* Lower IRQL for a moment to prevent starvation */
      KeLowerIrql(OldIrql);
      KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
      KeAcquireSpinLockAtDpcLevel(&LoopQueueLock);
    }

  LoopBusy = FALSE;
  KeReleaseSpinLockFromDpcLevel(&LoopQueueLock);
  KeLowerIrql(OldIrql);
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
 *   Type        = LAN protocol type (unused)
 */
{
  PNDIS_PACKET *pNdisPacket;
  KIRQL OldIrql;

  TI_DbgPrint(MAX_TRACE, ("Called.\n"));

  /* NDIS send routines don't have an offset argument so we
     must offset the data in upper layers and adjust the
     packet here. We save the offset in the packet context
     area so it can be undone before we release the packet */
  AdjustPacket(NdisPacket, Offset, 0);
  PC(NdisPacket)->DLOffset = Offset;

  pNdisPacket = (PNDIS_PACKET*)NdisPacket->u.s3.MacReserved;
  *pNdisPacket = NULL;

  KeAcquireSpinLock(&LoopQueueLock, &OldIrql);

  /* Add packet to transmit queue */
  if (LoopQueueHead != NULL)
    {
      /* Transmit queue is not empty */
      pNdisPacket = (PNDIS_PACKET*)LoopQueueTail->u.s3.MacReserved;
      *pNdisPacket = NdisPacket;
    }
  else
    {
      /* Transmit queue is empty */
      LoopQueueHead = NdisPacket;
    }

  LoopQueueTail = NdisPacket;

  /* If RealTransmit is not running (or scheduled to run) then schedule it to run now */
  if (!LoopBusy)
    {
      LoopBusy = TRUE;  /* The loopback interface is now busy */
      ExQueueWorkItem(&LoopWorkItem, CriticalWorkQueue);
    }

  KeReleaseSpinLock(&LoopQueueLock, OldIrql);
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
          /* Reference the interface for the NTE. The reference for
             the address is just passed on to the NTE */
          ReferenceObject(Loopback);

          IPRegisterInterface(Loopback);

          ExInitializeWorkItem(&LoopWorkItem, RealTransmit, Loopback);

          KeInitializeSpinLock(&LoopQueueLock);
          LoopBusy = FALSE;
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
