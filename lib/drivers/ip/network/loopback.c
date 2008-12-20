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
typedef struct _LAN_WQ_ITEM {
    LIST_ENTRY ListEntry;
    PNDIS_PACKET Packet;
    PLAN_ADAPTER Adapter;
    UINT BytesTransferred;
} LAN_WQ_ITEM, *PLAN_WQ_ITEM;

/* Work around being called back into afd at Dpc level */
KSPIN_LOCK LoopWorkLock;
LIST_ENTRY LoopWorkList;
WORK_QUEUE_ITEM LoopWorkItem;
BOOLEAN LoopReceiveWorkerBusy = FALSE;

VOID STDCALL LoopReceiveWorker( PVOID Context ) {
    PLIST_ENTRY ListEntry;
    PLAN_WQ_ITEM WorkItem;
    PNDIS_PACKET Packet;
    PLAN_ADAPTER Adapter;
    UINT BytesTransferred;
    PNDIS_BUFFER NdisBuffer;
    IP_PACKET IPPacket;

    TI_DbgPrint(DEBUG_DATALINK, ("Called.\n"));

    while( (ListEntry =
	    ExInterlockedRemoveHeadList( &LoopWorkList, &LoopWorkLock )) ) {
	WorkItem = CONTAINING_RECORD(ListEntry, LAN_WQ_ITEM, ListEntry);

	TI_DbgPrint(DEBUG_DATALINK, ("WorkItem: %x\n", WorkItem));

	Packet = WorkItem->Packet;
	Adapter = WorkItem->Adapter;
	BytesTransferred = WorkItem->BytesTransferred;

	ExFreePool( WorkItem );

        IPPacket.NdisPacket = Packet;

        TI_DbgPrint(DEBUG_DATALINK, ("Packet %x Adapter %x Trans %x\n",
                                     Packet, Adapter, BytesTransferred));

        NdisGetFirstBufferFromPacket(Packet,
                                     &NdisBuffer,
                                     &IPPacket.Header,
                                     &IPPacket.ContigSize,
                                     &IPPacket.TotalSize);

	IPPacket.ContigSize = IPPacket.TotalSize = BytesTransferred;
        /* Determine which upper layer protocol that should receive
           this packet and pass it to the correct receive handler */

	TI_DbgPrint(MID_TRACE,
		    ("ContigSize: %d, TotalSize: %d, BytesTransferred: %d\n",
		     IPPacket.ContigSize, IPPacket.TotalSize,
		     BytesTransferred));

	IPPacket.Position = 0;

        IPReceive(Loopback, &IPPacket);

	FreeNdisPacket( Packet );
    }
    TI_DbgPrint(DEBUG_DATALINK, ("Leaving\n"));
    LoopReceiveWorkerBusy = FALSE;
}

VOID LoopSubmitReceiveWork(
    NDIS_HANDLE BindingContext,
    PNDIS_PACKET Packet,
    NDIS_STATUS Status,
    UINT BytesTransferred) {
    PLAN_WQ_ITEM WQItem;
    PLAN_ADAPTER Adapter = (PLAN_ADAPTER)BindingContext;
    KIRQL OldIrql;

    TcpipAcquireSpinLock( &LoopWorkLock, &OldIrql );

    WQItem = ExAllocatePool( NonPagedPool, sizeof(LAN_WQ_ITEM) );
    if( !WQItem ) {
	TcpipReleaseSpinLock( &LoopWorkLock, OldIrql );
	return;
    }

    WQItem->Packet = Packet;
    WQItem->Adapter = Adapter;
    WQItem->BytesTransferred = BytesTransferred;
    InsertTailList( &LoopWorkList, &WQItem->ListEntry );

    TI_DbgPrint(DEBUG_DATALINK, ("Packet %x Adapter %x BytesTrans %x\n",
                                 Packet, Adapter, BytesTransferred));

    if( !LoopReceiveWorkerBusy ) {
	LoopReceiveWorkerBusy = TRUE;
	ExQueueWorkItem( &LoopWorkItem, CriticalWorkQueue );
	TI_DbgPrint(DEBUG_DATALINK,
		    ("Work item inserted %x %x\n", &LoopWorkItem, WQItem));
    } else {
        TI_DbgPrint(DEBUG_DATALINK,
                    ("LOOP WORKER BUSY %x %x\n", &LoopWorkItem, WQItem));
    }
    TcpipReleaseSpinLock( &LoopWorkLock, OldIrql );
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
    PCHAR PacketBuffer;
    UINT PacketLength;
    PNDIS_PACKET XmitPacket;
    NDIS_STATUS NdisStatus;

    ASSERT_KM_POINTER(NdisPacket);
    ASSERT_KM_POINTER(PC(NdisPacket));
    ASSERT_KM_POINTER(PC(NdisPacket)->DLComplete);

    TI_DbgPrint(MAX_TRACE, ("Called (NdisPacket = %x)\n", NdisPacket));

    GetDataPtr( NdisPacket, MaxLLHeaderSize, &PacketBuffer, &PacketLength );

    NdisStatus = AllocatePacketWithBuffer
        ( &XmitPacket, PacketBuffer, PacketLength );

    if( NT_SUCCESS(NdisStatus) ) {
        LoopSubmitReceiveWork
            ( NULL, XmitPacket, STATUS_SUCCESS, PacketLength );
    }

    (PC(NdisPacket)->DLComplete)
        ( PC(NdisPacket)->Context, NdisPacket, STATUS_SUCCESS );

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

  InitializeListHead( &LoopWorkList );
  ExInitializeWorkItem( &LoopWorkItem, LoopReceiveWorker, NULL );

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
