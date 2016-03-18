/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/miniport.c
 * PURPOSE:     Routines used by NDIS miniport drivers
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   20 Aug 2003 vizzini - DMA support
 *   3  Oct 2003 vizzini - SendPackets support
 */

#include "ndissys.h"

#include <ndisguid.h>

/*
 * Define to 1 to get a debugger breakpoint at the end of NdisInitializeWrapper
 * for each new miniport starting up
 */
#define BREAK_ON_MINIPORT_INIT 0

/*
 * This has to be big enough to hold the results of querying the Route value
 * from the Linkage key.  Please re-code me to determine this dynamically.
 */
#define ROUTE_DATA_SIZE 256

/* Number of media we know */
#define MEDIA_ARRAY_SIZE    15

static NDIS_MEDIUM MediaArray[MEDIA_ARRAY_SIZE] =
{
    NdisMedium802_3,
    NdisMedium802_5,
    NdisMediumFddi,
    NdisMediumWan,
    NdisMediumLocalTalk,
    NdisMediumDix,
    NdisMediumArcnetRaw,
    NdisMediumArcnet878_2,
    NdisMediumAtm,
    NdisMediumWirelessWan,
    NdisMediumIrda,
    NdisMediumBpc,
    NdisMediumCoWan,
    NdisMedium1394,
    NdisMediumMax
};

/* global list and lock of Miniports NDIS has registered */
LIST_ENTRY MiniportListHead;
KSPIN_LOCK MiniportListLock;

/* global list and lock of adapters NDIS has registered */
LIST_ENTRY AdapterListHead;
KSPIN_LOCK AdapterListLock;

VOID
MiniDisplayPacket(
    PNDIS_PACKET Packet)
{
#if DBG
    ULONG i, Length;
    UCHAR Buffer[64];
    if ((DebugTraceLevel & DEBUG_PACKET) > 0) {
        Length = CopyPacketToBuffer(
            Buffer,
            Packet,
            0,
            64);

        DbgPrint("*** PACKET START ***");

        for (i = 0; i < Length; i++) {
            if (i % 12 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", Buffer[i]);
        }

        DbgPrint("*** PACKET STOP ***\n");
    }
#endif /* DBG */
}

VOID
MiniDisplayPacket2(
    PVOID  HeaderBuffer,
    UINT   HeaderBufferSize,
    PVOID  LookaheadBuffer,
    UINT   LookaheadBufferSize)
{
#if DBG
    if ((DebugTraceLevel & DEBUG_PACKET) > 0) {
        ULONG i, Length;
        PUCHAR p;

        DbgPrint("*** RECEIVE PACKET START ***\n");
        DbgPrint("HEADER:");
        p = HeaderBuffer;
        for (i = 0; i < HeaderBufferSize; i++) {
            if (i % 16 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", *p++);
        }

        DbgPrint("\nFRAME:");

        p = LookaheadBuffer;
        Length = (LookaheadBufferSize < 64)? LookaheadBufferSize : 64;
        for (i = 0; i < Length; i++) {
            if (i % 16 == 0)
                DbgPrint("\n%04X ", i);
            DbgPrint("%02X ", *p++);
        }

        DbgPrint("\n*** RECEIVE PACKET STOP ***\n");
    }
#endif /* DBG */
}

PNDIS_MINIPORT_WORK_ITEM
MiniGetFirstWorkItem(
    PLOGICAL_ADAPTER Adapter,
    NDIS_WORK_ITEM_TYPE Type)
{
    PNDIS_MINIPORT_WORK_ITEM CurrentEntry = Adapter->WorkQueueHead;

    while (CurrentEntry)
    {
      if (CurrentEntry->WorkItemType == Type || Type == NdisMaxWorkItems)
          return CurrentEntry;

      CurrentEntry = (PNDIS_MINIPORT_WORK_ITEM)CurrentEntry->Link.Next;
    }

    return NULL;
}

BOOLEAN
MiniIsBusy(
    PLOGICAL_ADAPTER Adapter,
    NDIS_WORK_ITEM_TYPE Type)
{
    BOOLEAN Busy = FALSE;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    if (MiniGetFirstWorkItem(Adapter, Type))
    {
        Busy = TRUE;
    }
    else if (Type == NdisWorkItemRequest && Adapter->NdisMiniportBlock.PendingRequest)
    {
       Busy = TRUE;
    }
    else if (Type == NdisWorkItemSend && Adapter->NdisMiniportBlock.FirstPendingPacket)
    {
       Busy = TRUE;
    }
    else if (Type == NdisWorkItemResetRequested &&
             Adapter->NdisMiniportBlock.ResetStatus == NDIS_STATUS_PENDING)
    {
       Busy = TRUE;
    }

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

    return Busy;
}

VOID
MiniIndicateData(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_HANDLE         MacReceiveContext,
    PVOID               HeaderBuffer,
    UINT                HeaderBufferSize,
    PVOID               LookaheadBuffer,
    UINT                LookaheadBufferSize,
    UINT                PacketSize)
/*
 * FUNCTION: Indicate received data to bound protocols
 * ARGUMENTS:
 *     Adapter             = Pointer to logical adapter
 *     MacReceiveContext   = MAC receive context handle
 *     HeaderBuffer        = Pointer to header buffer
 *     HeaderBufferSize    = Size of header buffer
 *     LookaheadBuffer     = Pointer to lookahead buffer
 *     LookaheadBufferSize = Size of lookahead buffer
 *     PacketSize          = Total size of received packet
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PADAPTER_BINDING AdapterBinding;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called. Adapter (0x%X)  HeaderBuffer (0x%X)  "
      "HeaderBufferSize (0x%X)  LookaheadBuffer (0x%X)  LookaheadBufferSize (0x%X).\n",
      Adapter, HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize));

  MiniDisplayPacket2(HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize);

  NDIS_DbgPrint(MAX_TRACE, ("acquiring miniport block lock\n"));
  KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    {
      CurrentEntry = Adapter->ProtocolListHead.Flink;
      NDIS_DbgPrint(DEBUG_MINIPORT, ("CurrentEntry = %x\n", CurrentEntry));

      if (CurrentEntry == &Adapter->ProtocolListHead)
        {
          NDIS_DbgPrint(MIN_TRACE, ("WARNING: No upper protocol layer.\n"));
        }

      while (CurrentEntry != &Adapter->ProtocolListHead)
        {
          AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);
	  NDIS_DbgPrint(DEBUG_MINIPORT, ("AdapterBinding = %x\n", AdapterBinding));

	  NDIS_DbgPrint
	      (MID_TRACE,
	       ("XXX (%x) %x %x %x %x %x %x %x XXX\n",
		*AdapterBinding->ProtocolBinding->Chars.ReceiveHandler,
		AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
		MacReceiveContext,
		HeaderBuffer,
		HeaderBufferSize,
		LookaheadBuffer,
		LookaheadBufferSize,
		PacketSize));

          /* call the receive handler */
          (*AdapterBinding->ProtocolBinding->Chars.ReceiveHandler)(
              AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
              MacReceiveContext,
              HeaderBuffer,
              HeaderBufferSize,
              LookaheadBuffer,
              LookaheadBufferSize,
              PacketSize);

          CurrentEntry = CurrentEntry->Flink;
        }
    }
  KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}

/*
 * @implemented
 */
VOID
EXPORT
NdisReturnPackets(
    IN  PNDIS_PACKET    *PacketsToReturn,
    IN  UINT            NumberOfPackets)
/*
 * FUNCTION: Releases ownership of one or more packets
 * ARGUMENTS:
 *     PacketsToReturn = Pointer to an array of pointers to packet descriptors
 *     NumberOfPackets = Number of pointers in descriptor pointer array
 */
{
    UINT i;
    PLOGICAL_ADAPTER Adapter;
    KIRQL OldIrql;

    NDIS_DbgPrint(MID_TRACE, ("Returning %d packets\n", NumberOfPackets));

    for (i = 0; i < NumberOfPackets; i++)
    {
        PacketsToReturn[i]->WrapperReserved[0]--;
        if (PacketsToReturn[i]->WrapperReserved[0] == 0)
        {
            Adapter = (PVOID)(ULONG_PTR)PacketsToReturn[i]->Reserved[1];

            NDIS_DbgPrint(MAX_TRACE, ("Freeing packet %d (adapter = 0x%p)\n", i, Adapter));

            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
            Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.ReturnPacketHandler(
                  Adapter->NdisMiniportBlock.MiniportAdapterContext,
                  PacketsToReturn[i]);
            KeLowerIrql(OldIrql);
        }
    }
}

VOID NTAPI
MiniIndicateReceivePacket(
    IN  NDIS_HANDLE    MiniportAdapterHandle,
    IN  PPNDIS_PACKET  PacketArray,
    IN  UINT           NumberOfPackets)
/*
 * FUNCTION: receives miniport packet array indications
 * ARGUMENTS:
 *     MiniportAdapterHandle: Miniport handle for the adapter
 *     PacketArray: pointer to a list of packet pointers to indicate
 *     NumberOfPackets: number of packets to indicate
 *
 */
{
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;
    PLIST_ENTRY CurrentEntry;
    PADAPTER_BINDING AdapterBinding;
    KIRQL OldIrql;
    UINT i;

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    CurrentEntry = Adapter->ProtocolListHead.Flink;

    while (CurrentEntry != &Adapter->ProtocolListHead)
    {
        AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);

        for (i = 0; i < NumberOfPackets; i++)
        {
            /* Store the indicating miniport in the packet */
            PacketArray[i]->Reserved[1] = (ULONG_PTR)Adapter;

            if (AdapterBinding->ProtocolBinding->Chars.ReceivePacketHandler &&
                NDIS_GET_PACKET_STATUS(PacketArray[i]) != NDIS_STATUS_RESOURCES)
            {
                NDIS_DbgPrint(MID_TRACE, ("Indicating packet to protocol's ReceivePacket handler\n"));
                PacketArray[i]->WrapperReserved[0] += (*AdapterBinding->ProtocolBinding->Chars.ReceivePacketHandler)(
                                                       AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
                                                       PacketArray[i]);
                NDIS_DbgPrint(MID_TRACE, ("Protocol is holding %d references to the packet\n", PacketArray[i]->WrapperReserved[0]));
            }
            else
            {
                UINT FirstBufferLength, TotalBufferLength, LookAheadSize, HeaderSize;
                PNDIS_BUFFER NdisBuffer;
                PVOID NdisBufferVA, LookAheadBuffer;

                NdisGetFirstBufferFromPacket(PacketArray[i],
                                             &NdisBuffer,
                                             &NdisBufferVA,
                                             &FirstBufferLength,
                                             &TotalBufferLength);

                HeaderSize = NDIS_GET_PACKET_HEADER_SIZE(PacketArray[i]);

                LookAheadSize = TotalBufferLength - HeaderSize;

                LookAheadBuffer = ExAllocatePool(NonPagedPool, LookAheadSize);
                if (!LookAheadBuffer)
                {
                    NDIS_DbgPrint(MIN_TRACE, ("Failed to allocate lookahead buffer!\n"));
                    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
                    return;
                }

                CopyBufferChainToBuffer(LookAheadBuffer,
                                        NdisBuffer,
                                        HeaderSize,
                                        LookAheadSize);

                NDIS_DbgPrint(MID_TRACE, ("Indicating packet to protocol's legacy Receive handler\n"));
                (*AdapterBinding->ProtocolBinding->Chars.ReceiveHandler)(
                     AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
                     AdapterBinding->NdisOpenBlock.MacHandle,
                     NdisBufferVA,
                     HeaderSize,
                     LookAheadBuffer,
                     LookAheadSize,
                     TotalBufferLength - HeaderSize);

                ExFreePool(LookAheadBuffer);
            }
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    /* Loop the packet array to get everything
     * set up for return the packets to the miniport */
    for (i = 0; i < NumberOfPackets; i++)
    {
        /* First, check the initial packet status */
        if (NDIS_GET_PACKET_STATUS(PacketArray[i]) == NDIS_STATUS_RESOURCES)
        {
            /* The miniport driver gets it back immediately so nothing to do here */
            NDIS_DbgPrint(MID_TRACE, ("Miniport needs the packet back immediately\n"));
            continue;
        }

        /* Different behavior depending on whether it's serialized or not */
        if (Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_DESERIALIZE)
        {
            /* We need to check the reference count */
            if (PacketArray[i]->WrapperReserved[0] == 0)
            {
                /* NOTE: Unlike serialized miniports, this is REQUIRED to be called for each
                 * packet received that can be reused immediately, it is not implied! */
                Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.ReturnPacketHandler(
                      Adapter->NdisMiniportBlock.MiniportAdapterContext,
                      PacketArray[i]);
                NDIS_DbgPrint(MID_TRACE, ("Packet has been returned to miniport (Deserialized)\n"));
            }
            else
            {
                /* Packet will be returned by the protocol's call to NdisReturnPackets */
                NDIS_DbgPrint(MID_TRACE, ("Packet will be returned to miniport later (Deserialized)\n"));
            }
        }
        else
        {
            /* Check the reference count */
            if (PacketArray[i]->WrapperReserved[0] == 0)
            {
                /* NDIS_STATUS_SUCCESS means the miniport can have the packet back immediately */
                NDIS_SET_PACKET_STATUS(PacketArray[i], NDIS_STATUS_SUCCESS);

                NDIS_DbgPrint(MID_TRACE, ("Packet has been returned to miniport (Serialized)\n"));
            }
            else
            {
                /* NDIS_STATUS_PENDING means the miniport needs to wait for MiniportReturnPacket */
                NDIS_SET_PACKET_STATUS(PacketArray[i], NDIS_STATUS_PENDING);

                NDIS_DbgPrint(MID_TRACE, ("Packet will be returned to miniport later (Serialized)\n"));
            }
        }
    }

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}

VOID NTAPI
MiniResetComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status,
    IN  BOOLEAN     AddressingReset)
{
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;
    PLIST_ENTRY CurrentEntry;
    PADAPTER_BINDING AdapterBinding;
    KIRQL OldIrql;

    if (AddressingReset)
        MiniDoAddressingReset(Adapter);

    NdisMIndicateStatus(Adapter, NDIS_STATUS_RESET_END, NULL, 0);
    NdisMIndicateStatusComplete(Adapter);

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    if (Adapter->NdisMiniportBlock.ResetStatus != NDIS_STATUS_PENDING)
    {
        KeBugCheckEx(BUGCODE_ID_DRIVER,
                     (ULONG_PTR)MiniportAdapterHandle,
                     (ULONG_PTR)Status,
                     (ULONG_PTR)AddressingReset,
                     0);
    }

    Adapter->NdisMiniportBlock.ResetStatus = Status;

    CurrentEntry = Adapter->ProtocolListHead.Flink;

    while (CurrentEntry != &Adapter->ProtocolListHead)
    {
        AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);

        (*AdapterBinding->ProtocolBinding->Chars.ResetCompleteHandler)(
               AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
               Status);

        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}

VOID NTAPI
MiniRequestComplete(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_STATUS Status)
{
    PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
    PNDIS_REQUEST Request;
    PNDIS_REQUEST_MAC_BLOCK MacBlock;
    KIRQL OldIrql;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    Request = Adapter->NdisMiniportBlock.PendingRequest;
    KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

    MacBlock = (PNDIS_REQUEST_MAC_BLOCK)Request->MacReserved;

    /* We may or may not be doing this request on behalf of an adapter binding */
    if (MacBlock->Binding != NULL)
    {
        /* We are, so invoke its request complete handler */
        if (MacBlock->Binding->RequestCompleteHandler != NULL)
        {
            (*MacBlock->Binding->RequestCompleteHandler)(
                MacBlock->Binding->ProtocolBindingContext,
                Request,
                Status);
        }
    }
    else
    {
        /* We are doing this internally, so we'll signal this event we've stashed in the MacBlock */
        ASSERT(MacBlock->Unknown1 != NULL);
        ASSERT(MacBlock->Unknown3 == NULL);
        MacBlock->Unknown3 = (PVOID)Status;
        KeSetEvent(MacBlock->Unknown1, IO_NO_INCREMENT, FALSE);
    }

    KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    Adapter->NdisMiniportBlock.PendingRequest = NULL;
    KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    KeLowerIrql(OldIrql);

    MiniWorkItemComplete(Adapter, NdisWorkItemRequest);
}

VOID NTAPI
MiniSendComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status)
/*
 * FUNCTION: Forwards a message to the initiating protocol saying
 *           that a packet was handled
 * ARGUMENTS:
 *     NdisAdapterHandle = Handle input to MiniportInitialize
 *     Packet            = Pointer to NDIS packet that was sent
 *     Status            = Status of send operation
 */
{
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;
    PADAPTER_BINDING AdapterBinding;
    KIRQL OldIrql;
    PSCATTER_GATHER_LIST SGList;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    AdapterBinding = (PADAPTER_BINDING)Packet->Reserved[1];

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    if (Adapter->NdisMiniportBlock.ScatterGatherListSize != 0)
    {
        NDIS_DbgPrint(MAX_TRACE, ("Freeing Scatter/Gather list\n"));

        SGList = NDIS_PER_PACKET_INFO_FROM_PACKET(Packet,
                                                  ScatterGatherListPacketInfo);

        Adapter->NdisMiniportBlock.SystemAdapterObject->
            DmaOperations->PutScatterGatherList(
                           Adapter->NdisMiniportBlock.SystemAdapterObject,
                           SGList,
                           TRUE);

        NDIS_PER_PACKET_INFO_FROM_PACKET(Packet,
                                         ScatterGatherListPacketInfo) = NULL;
    }

    (*AdapterBinding->ProtocolBinding->Chars.SendCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Packet,
        Status);

    KeLowerIrql(OldIrql);

    MiniWorkItemComplete(Adapter, NdisWorkItemSend);
}


VOID NTAPI
MiniSendResourcesAvailable(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    /* Run the work if anything is waiting */
    MiniWorkItemComplete((PLOGICAL_ADAPTER)MiniportAdapterHandle, NdisWorkItemSend);
}


VOID NTAPI
MiniTransferDataComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred)
{
    PADAPTER_BINDING AdapterBinding;
    KIRQL OldIrql;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    AdapterBinding = (PADAPTER_BINDING)Packet->Reserved[1];

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    (*AdapterBinding->ProtocolBinding->Chars.TransferDataCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Packet,
        Status,
        BytesTransferred);
    KeLowerIrql(OldIrql);
}


BOOLEAN
MiniAdapterHasAddress(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_PACKET Packet)
/*
 * FUNCTION: Determines whether a packet has the same destination address as an adapter
 * ARGUMENTS:
 *     Adapter = Pointer to logical adapter object
 *     Packet  = Pointer to NDIS packet
 * RETURNS:
 *     TRUE if the destination address is that of the adapter, FALSE if not
 */
{
    UINT Length;
    PUCHAR Start1;
    PUCHAR Start2;
    PNDIS_BUFFER NdisBuffer;
    UINT BufferLength;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

#if DBG
    if(!Adapter)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Adapter object was null\n"));
        return FALSE;
    }

    if(!Packet)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Packet was null\n"));
        return FALSE;
    }
#endif

    NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

    if (!NdisBuffer)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Packet contains no buffers.\n"));
        return FALSE;
    }

    NdisQueryBuffer(NdisBuffer, (PVOID)&Start2, &BufferLength);

    /* FIXME: Should handle fragmented packets */

    switch (Adapter->NdisMiniportBlock.MediaType)
    {
        case NdisMedium802_3:
            Length = ETH_LENGTH_OF_ADDRESS;
            /* Destination address is the first field */
            break;

        default:
            NDIS_DbgPrint(MIN_TRACE, ("Adapter has unsupported media type (0x%X).\n", Adapter->NdisMiniportBlock.MediaType));
            return FALSE;
    }

    if (BufferLength < Length)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Buffer is too small.\n"));
        return FALSE;
    }

    Start1 = (PUCHAR)&Adapter->Address;
    NDIS_DbgPrint(MAX_TRACE, ("packet address: %x:%x:%x:%x:%x:%x adapter address: %x:%x:%x:%x:%x:%x\n",
                              *((char *)Start1), *(((char *)Start1)+1), *(((char *)Start1)+2), *(((char *)Start1)+3), *(((char *)Start1)+4), *(((char *)Start1)+5),
                              *((char *)Start2), *(((char *)Start2)+1), *(((char *)Start2)+2), *(((char *)Start2)+3), *(((char *)Start2)+4), *(((char *)Start2)+5)));

    return (RtlCompareMemory((PVOID)Start1, (PVOID)Start2, Length) == Length);
}


PLOGICAL_ADAPTER
MiniLocateDevice(
    PNDIS_STRING AdapterName)
/*
 * FUNCTION: Finds an adapter object by name
 * ARGUMENTS:
 *     AdapterName = Pointer to name of adapter
 * RETURNS:
 *     Pointer to logical adapter object, or NULL if none was found.
 *     If found, the adapter is referenced for the caller. The caller
 *     is responsible for dereferencing after use
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PLOGICAL_ADAPTER Adapter = 0;

    ASSERT(AdapterName);

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    if(IsListEmpty(&AdapterListHead))
    {
        NDIS_DbgPrint(MIN_TRACE, ("No registered miniports for protocol to bind to\n"));
        return NULL;
    }

    NDIS_DbgPrint(DEBUG_MINIPORT, ("AdapterName = %wZ\n", AdapterName));

    KeAcquireSpinLock(&AdapterListLock, &OldIrql);
    {
        CurrentEntry = AdapterListHead.Flink;

        while (CurrentEntry != &AdapterListHead)
        {
            Adapter = CONTAINING_RECORD(CurrentEntry, LOGICAL_ADAPTER, ListEntry);

            ASSERT(Adapter);

            NDIS_DbgPrint(DEBUG_MINIPORT, ("Examining adapter 0x%lx\n", Adapter));

            /* We're technically not allowed to call this above PASSIVE_LEVEL, but it doesn't break
             * right now and I'd rather use a working API than reimplement it here */
            if (RtlCompareUnicodeString(AdapterName, &Adapter->NdisMiniportBlock.MiniportName, TRUE) == 0)
            {
                break;
            }

            Adapter = NULL;
            CurrentEntry = CurrentEntry->Flink;
        }
    }
    KeReleaseSpinLock(&AdapterListLock, OldIrql);

    if(Adapter)
    {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Leaving. Adapter found at 0x%x\n", Adapter));
    }
    else
    {
        NDIS_DbgPrint(MIN_TRACE, ("Leaving (adapter not found for %wZ).\n", AdapterName));
    }

    return Adapter;
}

NDIS_STATUS
MiniSetInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PVOID               Buffer,
    PULONG              BytesRead)
{
  NDIS_STATUS NdisStatus;
  PNDIS_REQUEST NdisRequest;
  KEVENT Event;
  PNDIS_REQUEST_MAC_BLOCK MacBlock;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  NdisRequest = ExAllocatePool(NonPagedPool, sizeof(NDIS_REQUEST));
  if (!NdisRequest) {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
      return NDIS_STATUS_RESOURCES;
  }

  RtlZeroMemory(NdisRequest, sizeof(NDIS_REQUEST));

  NdisRequest->RequestType = NdisRequestSetInformation;
  NdisRequest->DATA.SET_INFORMATION.Oid = Oid;
  NdisRequest->DATA.SET_INFORMATION.InformationBuffer = Buffer;
  NdisRequest->DATA.SET_INFORMATION.InformationBufferLength = Size;

  /* We'll need to give the completion routine some way of letting us know
   * when it's finished. We'll stash a pointer to an event in the MacBlock */
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  MacBlock = (PNDIS_REQUEST_MAC_BLOCK)NdisRequest->MacReserved;
  MacBlock->Unknown1 = &Event;

  NdisStatus = MiniDoRequest(Adapter, NdisRequest);

  if (NdisStatus == NDIS_STATUS_PENDING)
  {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      NdisStatus = (NDIS_STATUS)MacBlock->Unknown3;
  }

  *BytesRead = NdisRequest->DATA.SET_INFORMATION.BytesRead;

  ExFreePool(NdisRequest);

  return NdisStatus;
}

NDIS_STATUS
MiniQueryInformation(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_OID            Oid,
    ULONG               Size,
    PVOID               Buffer,
    PULONG              BytesWritten)
/*
 * FUNCTION: Queries a logical adapter for properties
 * ARGUMENTS:
 *     Adapter      = Pointer to the logical adapter object to query
 *     Oid          = Specifies the Object ID to query for
 *     Size         = Size of the passed buffer
 *     Buffer       = Buffer for the output
 *     BytesWritten = Address of buffer to place number of bytes written
 * RETURNS:
 *     Status of operation
 */
{
  NDIS_STATUS NdisStatus;
  PNDIS_REQUEST NdisRequest;
  KEVENT Event;
  PNDIS_REQUEST_MAC_BLOCK MacBlock;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  NdisRequest = ExAllocatePool(NonPagedPool, sizeof(NDIS_REQUEST));
  if (!NdisRequest) {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
      return NDIS_STATUS_RESOURCES;
  }

  RtlZeroMemory(NdisRequest, sizeof(NDIS_REQUEST));

  NdisRequest->RequestType = NdisRequestQueryInformation;
  NdisRequest->DATA.QUERY_INFORMATION.Oid = Oid;
  NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer = Buffer;
  NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength = Size;

  /* We'll need to give the completion routine some way of letting us know
   * when it's finished. We'll stash a pointer to an event in the MacBlock */
  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  MacBlock = (PNDIS_REQUEST_MAC_BLOCK)NdisRequest->MacReserved;
  MacBlock->Unknown1 = &Event;

  NdisStatus = MiniDoRequest(Adapter, NdisRequest);

  if (NdisStatus == NDIS_STATUS_PENDING)
  {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      NdisStatus = (NDIS_STATUS)MacBlock->Unknown3;
  }

  *BytesWritten = NdisRequest->DATA.QUERY_INFORMATION.BytesWritten;

  ExFreePool(NdisRequest);

  return NdisStatus;
}

BOOLEAN
MiniCheckForHang( PLOGICAL_ADAPTER Adapter )
/*
 * FUNCTION: Checks to see if the miniport is hung
 * ARGUMENTS:
 *     Adapter = Pointer to the logical adapter object
 * RETURNS:
 *     TRUE if the miniport is hung
 *     FALSE if the miniport is not hung
 */
{
   BOOLEAN Ret = FALSE;
   KIRQL OldIrql;

   KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
   if (Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.CheckForHangHandler)
       Ret = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.CheckForHangHandler)(
         Adapter->NdisMiniportBlock.MiniportAdapterContext);
   KeLowerIrql(OldIrql);

   return Ret;
}

VOID
MiniDoAddressingReset(PLOGICAL_ADAPTER Adapter)
{
   ULONG BytesRead;

   MiniSetInformation(Adapter,
                      OID_GEN_CURRENT_LOOKAHEAD,
                      sizeof(ULONG),
                      &Adapter->NdisMiniportBlock.CurrentLookahead,
                      &BytesRead);

   /* FIXME: Set more stuff */
}

NDIS_STATUS
MiniReset(
    PLOGICAL_ADAPTER Adapter)
/*
 * FUNCTION: Resets the miniport
 * ARGUMENTS:
 *     Adapter = Pointer to the logical adapter object
 * RETURNS:
 *     Status of the operation
 */
{
   NDIS_STATUS Status;
   KIRQL OldIrql;
   BOOLEAN AddressingReset = TRUE;

   if (MiniIsBusy(Adapter, NdisWorkItemResetRequested)) {
       MiniQueueWorkItem(Adapter, NdisWorkItemResetRequested, NULL, FALSE);
       return NDIS_STATUS_PENDING;
   }

   NdisMIndicateStatus(Adapter, NDIS_STATUS_RESET_START, NULL, 0);
   NdisMIndicateStatusComplete(Adapter);

   KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
   Status = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.ResetHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            &AddressingReset);

   KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
   Adapter->NdisMiniportBlock.ResetStatus = Status;
   KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

   KeLowerIrql(OldIrql);

   if (Status != NDIS_STATUS_PENDING) {
       if (AddressingReset)
           MiniDoAddressingReset(Adapter);

       NdisMIndicateStatus(Adapter, NDIS_STATUS_RESET_END, NULL, 0);
       NdisMIndicateStatusComplete(Adapter);

       MiniWorkItemComplete(Adapter, NdisWorkItemResetRequested);
   }

   return Status;
}

VOID NTAPI
MiniportHangDpc(
        PKDPC Dpc,
        PVOID DeferredContext,
        PVOID SystemArgument1,
        PVOID SystemArgument2)
{
  PLOGICAL_ADAPTER Adapter = DeferredContext;

  if (MiniCheckForHang(Adapter)) {
      NDIS_DbgPrint(MIN_TRACE, ("Miniport detected adapter hang\n"));
      MiniReset(Adapter);
  }
}

VOID
MiniWorkItemComplete(
    PLOGICAL_ADAPTER     Adapter,
    NDIS_WORK_ITEM_TYPE  WorkItemType)
{
    PIO_WORKITEM IoWorkItem;

    /* Check if there's anything queued to run after this work item */
    if (!MiniIsBusy(Adapter, WorkItemType))
        return;

    /* There is, so fire the worker */
    IoWorkItem = IoAllocateWorkItem(Adapter->NdisMiniportBlock.DeviceObject);
    if (IoWorkItem)
        IoQueueWorkItem(IoWorkItem, MiniportWorker, DelayedWorkQueue, IoWorkItem);
}

VOID
FASTCALL
MiniQueueWorkItem(
    PLOGICAL_ADAPTER     Adapter,
    NDIS_WORK_ITEM_TYPE  WorkItemType,
    PVOID                WorkItemContext,
    BOOLEAN              Top)
/*
 * FUNCTION: Queues a work item for execution at a later time
 * ARGUMENTS:
 *     Adapter         = Pointer to the logical adapter object to queue work item on
 *     WorkItemType    = Type of work item to queue
 *     WorkItemContext = Pointer to context information for work item
 * RETURNS:
 *     Status of operation
 */
{
    PNDIS_MINIPORT_WORK_ITEM MiniportWorkItem;
    KIRQL OldIrql;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    ASSERT(Adapter);

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    if (Top)
    {
        if (WorkItemType == NdisWorkItemSend)
        {
            NDIS_DbgPrint(MIN_TRACE, ("Requeuing failed packet (%x).\n", WorkItemContext));
            Adapter->NdisMiniportBlock.FirstPendingPacket = WorkItemContext;
        }
        else
        {
            //This should never happen
            ASSERT(FALSE);
        }
    }
    else
    {
        MiniportWorkItem = ExAllocatePool(NonPagedPool, sizeof(NDIS_MINIPORT_WORK_ITEM));
        if (!MiniportWorkItem)
        {
            KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
            NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return;
        }

        MiniportWorkItem->WorkItemType    = WorkItemType;
        MiniportWorkItem->WorkItemContext = WorkItemContext;

        /* safe due to adapter lock held */
        MiniportWorkItem->Link.Next = NULL;
        if (!Adapter->WorkQueueHead)
        {
            Adapter->WorkQueueHead = MiniportWorkItem;
            Adapter->WorkQueueTail = MiniportWorkItem;
        }
        else
        {
            Adapter->WorkQueueTail->Link.Next = (PSINGLE_LIST_ENTRY)MiniportWorkItem;
            Adapter->WorkQueueTail = MiniportWorkItem;
        }
    }

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}

NDIS_STATUS
FASTCALL
MiniDequeueWorkItem(
    PLOGICAL_ADAPTER    Adapter,
    NDIS_WORK_ITEM_TYPE *WorkItemType,
    PVOID               *WorkItemContext)
/*
 * FUNCTION: Dequeues a work item from the work queue of a logical adapter
 * ARGUMENTS:
 *     Adapter         = Pointer to the logical adapter object to dequeue work item from
 *     AdapterBinding  = Address of buffer for adapter binding for this request
 *     WorkItemType    = Address of buffer for work item type
 *     WorkItemContext = Address of buffer for pointer to context information
 * NOTES:
 *     Adapter lock must be held when called
 * RETURNS:
 *     Status of operation
 */
{
    PNDIS_MINIPORT_WORK_ITEM MiniportWorkItem;
    PNDIS_PACKET Packet;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    MiniportWorkItem = Adapter->WorkQueueHead;

    if ((Packet = Adapter->NdisMiniportBlock.FirstPendingPacket))
    {
        Adapter->NdisMiniportBlock.FirstPendingPacket = NULL;

        *WorkItemType = NdisWorkItemSend;
        *WorkItemContext = Packet;

        return NDIS_STATUS_SUCCESS;
    }
    else if (MiniportWorkItem)
    {
        /* safe due to adapter lock held */
        Adapter->WorkQueueHead = (PNDIS_MINIPORT_WORK_ITEM)MiniportWorkItem->Link.Next;

        if (MiniportWorkItem == Adapter->WorkQueueTail)
            Adapter->WorkQueueTail = NULL;

        *WorkItemType    = MiniportWorkItem->WorkItemType;
        *WorkItemContext = MiniportWorkItem->WorkItemContext;

        ExFreePool(MiniportWorkItem);

        return NDIS_STATUS_SUCCESS;
    }
    else
    {
        NDIS_DbgPrint(MIN_TRACE, ("No work item to dequeue\n"));

        return NDIS_STATUS_FAILURE;
    }
}

NDIS_STATUS
MiniDoRequest(
    PLOGICAL_ADAPTER Adapter,
    PNDIS_REQUEST NdisRequest)
/*
 * FUNCTION: Sends a request to a miniport
 * ARGUMENTS:
 *     AdapterBinding = Pointer to binding used in the request
 *     NdisRequest    = Pointer to NDIS request structure describing request
 * RETURNS:
 *     Status of operation
 */
{
    NDIS_STATUS Status;
    KIRQL OldIrql;
    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);

    KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    Adapter->NdisMiniportBlock.PendingRequest = NdisRequest;
    KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

    if (!Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.CoRequestHandler)
    {
        switch (NdisRequest->RequestType)
        {
        case NdisRequestQueryInformation:
            Status = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.QueryInformationHandler)(
                Adapter->NdisMiniportBlock.MiniportAdapterContext,
                NdisRequest->DATA.QUERY_INFORMATION.Oid,
                NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
                (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded);
            break;

        case NdisRequestSetInformation:
            Status = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SetInformationHandler)(
                Adapter->NdisMiniportBlock.MiniportAdapterContext,
                NdisRequest->DATA.SET_INFORMATION.Oid,
                NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
                NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
                (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
                (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded);
            break;

        default:
            NDIS_DbgPrint(MIN_TRACE, ("Bad request type\n"));
            Status = NDIS_STATUS_FAILURE;
        }
    }
    else
    {
        Status = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.CoRequestHandler)(
            Adapter->NdisMiniportBlock.MiniportAdapterContext,
            NULL, /* FIXME */
            NdisRequest);
    }

    if (Status != NDIS_STATUS_PENDING) {
        KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
        Adapter->NdisMiniportBlock.PendingRequest = NULL;
        KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);
    }

    KeLowerIrql(OldIrql);

    if (Status != NDIS_STATUS_PENDING) {
        MiniWorkItemComplete(Adapter, NdisWorkItemRequest);
    }

    return Status;
}

/*
 * @implemented
 */
#undef NdisMSetInformationComplete
VOID
EXPORT
NdisMSetInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status)
{
  PLOGICAL_ADAPTER Adapter =
	(PLOGICAL_ADAPTER)MiniportAdapterHandle;
  KIRQL OldIrql;
  ASSERT(Adapter);
  KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
  if (Adapter->NdisMiniportBlock.SetCompleteHandler)
     (Adapter->NdisMiniportBlock.SetCompleteHandler)(MiniportAdapterHandle, Status);
  KeLowerIrql(OldIrql);
}

/*
 * @implemented
 */
#undef NdisMQueryInformationComplete
VOID
EXPORT
NdisMQueryInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status)
{
    PLOGICAL_ADAPTER Adapter =
	(PLOGICAL_ADAPTER)MiniportAdapterHandle;
    KIRQL OldIrql;
    ASSERT(Adapter);
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    if( Adapter->NdisMiniportBlock.QueryCompleteHandler )
	(Adapter->NdisMiniportBlock.QueryCompleteHandler)(MiniportAdapterHandle, Status);
    KeLowerIrql(OldIrql);
}

VOID
NTAPI
MiniportWorker(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context)
{
  PLOGICAL_ADAPTER Adapter = DeviceObject->DeviceExtension;
  KIRQL OldIrql, RaiseOldIrql;
  NDIS_STATUS NdisStatus;
  PVOID WorkItemContext;
  NDIS_WORK_ITEM_TYPE WorkItemType;
  BOOLEAN AddressingReset;

  IoFreeWorkItem((PIO_WORKITEM)Context);

  KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

  NdisStatus =
      MiniDequeueWorkItem
      (Adapter, &WorkItemType, &WorkItemContext);

  KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

  if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
      switch (WorkItemType)
        {
          case NdisWorkItemSend:
            /*
             * called by ProSend when protocols want to send packets to the miniport
             */

            if(Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)
              {
                if(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_DESERIALIZE)
                {
                    NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's SendPackets handler\n"));
                    (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)(
                     Adapter->NdisMiniportBlock.MiniportAdapterContext, (PPNDIS_PACKET)&WorkItemContext, 1);
                    NdisStatus = NDIS_STATUS_PENDING;
                }
                else
                {
                    /* SendPackets is called at DISPATCH_LEVEL for all serialized miniports */
                    KeRaiseIrql(DISPATCH_LEVEL, &RaiseOldIrql);
                    {
                      NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's SendPackets handler\n"));
                      (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)(
                       Adapter->NdisMiniportBlock.MiniportAdapterContext, (PPNDIS_PACKET)&WorkItemContext, 1);
                    }
                    KeLowerIrql(RaiseOldIrql);

                    NdisStatus = NDIS_GET_PACKET_STATUS((PNDIS_PACKET)WorkItemContext);
                    if( NdisStatus == NDIS_STATUS_RESOURCES ) {
                        MiniQueueWorkItem(Adapter, WorkItemType, WorkItemContext, TRUE);
                        break;
                    }
                }
              }
            else
              {
                if(Adapter->NdisMiniportBlock.Flags & NDIS_ATTRIBUTE_DESERIALIZE)
                {
                  NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's Send handler\n"));
                  NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendHandler)(
                                Adapter->NdisMiniportBlock.MiniportAdapterContext, (PNDIS_PACKET)WorkItemContext,
                                ((PNDIS_PACKET)WorkItemContext)->Private.Flags);
                  NDIS_DbgPrint(MAX_TRACE, ("back from miniport's send handler\n"));
                }
                else
                {
                  /* Send is called at DISPATCH_LEVEL for all serialized miniports */
                  KeRaiseIrql(DISPATCH_LEVEL, &RaiseOldIrql);
                  NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's Send handler\n"));
                  NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendHandler)(
                                Adapter->NdisMiniportBlock.MiniportAdapterContext, (PNDIS_PACKET)WorkItemContext,
                                ((PNDIS_PACKET)WorkItemContext)->Private.Flags);
                  NDIS_DbgPrint(MAX_TRACE, ("back from miniport's send handler\n"));
                  KeLowerIrql(RaiseOldIrql);
                  if( NdisStatus == NDIS_STATUS_RESOURCES ) {
                      MiniQueueWorkItem(Adapter, WorkItemType, WorkItemContext, TRUE);
                      break;
                  }
                }
              }

	    if( NdisStatus != NDIS_STATUS_PENDING ) {
		MiniSendComplete
		    ( Adapter, (PNDIS_PACKET)WorkItemContext, NdisStatus );
	    }
            break;

          case NdisWorkItemSendLoopback:
            /*
             * called by ProSend when protocols want to send loopback packets
             */
            /* XXX atm ProIndicatePacket sends a packet up via the loopback adapter only */
            NdisStatus = ProIndicatePacket(Adapter, (PNDIS_PACKET)WorkItemContext);

            if( NdisStatus != NDIS_STATUS_PENDING )
                MiniSendComplete((NDIS_HANDLE)Adapter, (PNDIS_PACKET)WorkItemContext, NdisStatus);
            break;

          case NdisWorkItemReturnPackets:
            break;

          case NdisWorkItemResetRequested:
            NdisMIndicateStatus(Adapter, NDIS_STATUS_RESET_START, NULL, 0);
            NdisMIndicateStatusComplete(Adapter);

            KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
            NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.ResetHandler)(
                          Adapter->NdisMiniportBlock.MiniportAdapterContext,
                          &AddressingReset);

            KeAcquireSpinLockAtDpcLevel(&Adapter->NdisMiniportBlock.Lock);
            Adapter->NdisMiniportBlock.ResetStatus = NdisStatus;
            KeReleaseSpinLockFromDpcLevel(&Adapter->NdisMiniportBlock.Lock);

            KeLowerIrql(OldIrql);

            if (NdisStatus != NDIS_STATUS_PENDING)
               MiniResetComplete(Adapter, NdisStatus, AddressingReset);
            break;

          case NdisWorkItemResetInProgress:
            break;

          case NdisWorkItemMiniportCallback:
            break;

          case NdisWorkItemRequest:
            NdisStatus = MiniDoRequest(Adapter, (PNDIS_REQUEST)WorkItemContext);

            if (NdisStatus == NDIS_STATUS_PENDING)
              break;

            Adapter->NdisMiniportBlock.PendingRequest = (PNDIS_REQUEST)WorkItemContext;
            switch (((PNDIS_REQUEST)WorkItemContext)->RequestType)
              {
                case NdisRequestQueryInformation:
                  NdisMQueryInformationComplete((NDIS_HANDLE)Adapter, NdisStatus);
                  break;

                case NdisRequestSetInformation:
                  NdisMSetInformationComplete((NDIS_HANDLE)Adapter, NdisStatus);
                  break;

                default:
                  NDIS_DbgPrint(MIN_TRACE, ("Unknown NDIS request type.\n"));
                  break;
              }
            Adapter->NdisMiniportBlock.PendingRequest = NULL;
            break;

          default:
            NDIS_DbgPrint(MIN_TRACE, ("Unknown NDIS work item type (%d).\n", WorkItemType));
            break;
        }
    }
}


VOID
NTAPI
MiniStatus(
    IN NDIS_HANDLE  MiniportHandle,
    IN NDIS_STATUS  GeneralStatus,
    IN PVOID  StatusBuffer,
    IN UINT  StatusBufferSize)
{
    PLOGICAL_ADAPTER Adapter = MiniportHandle;
    PLIST_ENTRY CurrentEntry;
    PADAPTER_BINDING AdapterBinding;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    CurrentEntry = Adapter->ProtocolListHead.Flink;

    while (CurrentEntry != &Adapter->ProtocolListHead)
    {
       AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);

       (*AdapterBinding->ProtocolBinding->Chars.StatusHandler)(
           AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
           GeneralStatus,
           StatusBuffer,
           StatusBufferSize);

       CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}

VOID
NTAPI
MiniStatusComplete(
    IN NDIS_HANDLE  MiniportAdapterHandle)
{
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;
    PLIST_ENTRY CurrentEntry;
    PADAPTER_BINDING AdapterBinding;
    KIRQL OldIrql;

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    CurrentEntry = Adapter->ProtocolListHead.Flink;

    while (CurrentEntry != &Adapter->ProtocolListHead)
    {
       AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);

       (*AdapterBinding->ProtocolBinding->Chars.StatusCompleteHandler)(
           AdapterBinding->NdisOpenBlock.ProtocolBindingContext);

       CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMCloseLog(
    IN  NDIS_HANDLE LogHandle)
{
    PNDIS_LOG Log = (PNDIS_LOG)LogHandle;
    PNDIS_MINIPORT_BLOCK Miniport = Log->Miniport;
    KIRQL OldIrql;

    NDIS_DbgPrint(MAX_TRACE, ("called: LogHandle 0x%x\n", LogHandle));

    KeAcquireSpinLock(&(Miniport)->Lock, &OldIrql);
    Miniport->Log = NULL;
    KeReleaseSpinLock(&(Miniport)->Lock, OldIrql);

    ExFreePool(Log);
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMCreateLog(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  UINT            Size,
    OUT PNDIS_HANDLE    LogHandle)
{
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;
    PNDIS_LOG Log;
    KIRQL OldIrql;

    NDIS_DbgPrint(MAX_TRACE, ("called: MiniportAdapterHandle 0x%x, Size %ld\n", MiniportAdapterHandle, Size));

    KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);

    if (Adapter->NdisMiniportBlock.Log)
    {
        *LogHandle = NULL;
        return NDIS_STATUS_FAILURE;
    }

    Log = ExAllocatePool(NonPagedPool, Size + sizeof(NDIS_LOG));
    if (!Log)
    {
        *LogHandle = NULL;
        return NDIS_STATUS_RESOURCES;
    }

    Adapter->NdisMiniportBlock.Log = Log;

    KeInitializeSpinLock(&Log->LogLock);

    Log->Miniport = &Adapter->NdisMiniportBlock;
    Log->TotalSize = Size;
    Log->CurrentSize = 0;
    Log->OutPtr = 0;
    Log->InPtr = 0;
    Log->Irp = NULL;

    *LogHandle = Log;

    KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

    return NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE MiniportHandle)
/*
 * FUNCTION: de-registers a shutdown handler
 * ARGUMENTS:  MiniportHandle:  Handle passed into MiniportInitialize
 */
{
  PLOGICAL_ADAPTER  Adapter = (PLOGICAL_ADAPTER)MiniportHandle;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  if(Adapter->BugcheckContext->ShutdownHandler) {
    KeDeregisterBugCheckCallback(Adapter->BugcheckContext->CallbackRecord);
    IoUnregisterShutdownNotification(Adapter->NdisMiniportBlock.DeviceObject);
  }
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMFlushLog(
    IN  NDIS_HANDLE LogHandle)
{
    PNDIS_LOG Log = (PNDIS_LOG) LogHandle;
    KIRQL OldIrql;

    NDIS_DbgPrint(MAX_TRACE, ("called: LogHandle 0x%x\n", LogHandle));

    /* Lock object */
    KeAcquireSpinLock(&Log->LogLock, &OldIrql);

    /* Set buffers size */
    Log->CurrentSize = 0;
    Log->OutPtr = 0;
    Log->InPtr = 0;

    /* Unlock object */
    KeReleaseSpinLock(&Log->LogLock, OldIrql);
}

/*
 * @implemented
 */
#undef NdisMIndicateStatus
VOID
EXPORT
NdisMIndicateStatus(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS GeneralStatus,
    IN  PVOID       StatusBuffer,
    IN  UINT        StatusBufferSize)
{
    MiniStatus(MiniportAdapterHandle, GeneralStatus, StatusBuffer, StatusBufferSize);
}

/*
 * @implemented
 */
#undef NdisMIndicateStatusComplete
VOID
EXPORT
NdisMIndicateStatusComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    MiniStatusComplete(MiniportAdapterHandle);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisInitializeWrapper(
    OUT PNDIS_HANDLE    NdisWrapperHandle,
    IN  PVOID           SystemSpecific1,
    IN  PVOID           SystemSpecific2,
    IN  PVOID           SystemSpecific3)
/*
 * FUNCTION: Notifies the NDIS library that a new miniport is initializing
 * ARGUMENTS:
 *     NdisWrapperHandle = Address of buffer to place NDIS wrapper handle
 *     SystemSpecific1   = Pointer to the driver's driver object
 *     SystemSpecific2   = Pointer to the driver's registry path
 *     SystemSpecific3   = Always NULL
 * NOTES:
 *     - SystemSpecific2 goes invalid so we copy it
 */
{
  PNDIS_M_DRIVER_BLOCK Miniport;
  PUNICODE_STRING RegistryPath;
  WCHAR *RegistryBuffer;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ASSERT(NdisWrapperHandle);

  *NdisWrapperHandle = NULL;

#if BREAK_ON_MINIPORT_INIT
  DbgBreakPoint();
#endif

  Miniport = ExAllocatePool(NonPagedPool, sizeof(NDIS_M_DRIVER_BLOCK));

  if (!Miniport)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return;
    }

  RtlZeroMemory(Miniport, sizeof(NDIS_M_DRIVER_BLOCK));

  KeInitializeSpinLock(&Miniport->Lock);

  Miniport->DriverObject = (PDRIVER_OBJECT)SystemSpecific1;

  /* set the miniport's driver registry path */
  RegistryPath = ExAllocatePool(PagedPool, sizeof(UNICODE_STRING));
  if(!RegistryPath)
    {
      ExFreePool(Miniport);
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return;
    }

  RegistryPath->Length = ((PUNICODE_STRING)SystemSpecific2)->Length;
  RegistryPath->MaximumLength = RegistryPath->Length + sizeof(WCHAR);	/* room for 0-term */

  RegistryBuffer = ExAllocatePool(PagedPool, RegistryPath->MaximumLength);
  if(!RegistryBuffer)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      ExFreePool(Miniport);
      ExFreePool(RegistryPath);
      return;
    }

  RtlCopyMemory(RegistryBuffer, ((PUNICODE_STRING)SystemSpecific2)->Buffer, RegistryPath->Length);
  RegistryBuffer[RegistryPath->Length/sizeof(WCHAR)] = 0;

  RegistryPath->Buffer = RegistryBuffer;
  Miniport->RegistryPath = RegistryPath;

  InitializeListHead(&Miniport->DeviceList);

  /* Put miniport in global miniport list */
  ExInterlockedInsertTailList(&MiniportListHead, &Miniport->ListEntry, &MiniportListLock);

  *NdisWrapperHandle = Miniport;
}

VOID NTAPI NdisIBugcheckCallback(
    IN PVOID   Buffer,
    IN ULONG   Length)
/*
 * FUNCTION:  Internal callback for handling bugchecks - calls adapter's shutdown handler
 * ARGUMENTS:
 *     Buffer:  Pointer to a bugcheck callback context
 *     Length:  Unused
 */
{
  PMINIPORT_BUGCHECK_CONTEXT Context = (PMINIPORT_BUGCHECK_CONTEXT)Buffer;
  ADAPTER_SHUTDOWN_HANDLER sh = (ADAPTER_SHUTDOWN_HANDLER)Context->ShutdownHandler;

   NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  if(sh)
    sh(Context->DriverContext);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE                 MiniportHandle,
    IN  PVOID                       ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler)
/*
 * FUNCTION:  Register a shutdown handler for an adapter
 * ARGUMENTS:
 *     MiniportHandle:  Handle originally passed into MiniportInitialize
 *     ShutdownContext:  Pre-initialized bugcheck context
 *     ShutdownHandler:  Function to call to handle the bugcheck
 * NOTES:
 *     - I'm not sure about ShutdownContext
 */
{
  PLOGICAL_ADAPTER            Adapter = (PLOGICAL_ADAPTER)MiniportHandle;
  PMINIPORT_BUGCHECK_CONTEXT  BugcheckContext;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  if (Adapter->BugcheckContext != NULL)
  {
      NDIS_DbgPrint(MIN_TRACE, ("Attempted to register again for a shutdown callback\n"));
      return;
  }

  BugcheckContext = ExAllocatePool(NonPagedPool, sizeof(MINIPORT_BUGCHECK_CONTEXT));
  if(!BugcheckContext)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return;
    }

  BugcheckContext->ShutdownHandler = ShutdownHandler;
  BugcheckContext->DriverContext = ShutdownContext;

  BugcheckContext->CallbackRecord = ExAllocatePool(NonPagedPool, sizeof(KBUGCHECK_CALLBACK_RECORD));
  if (!BugcheckContext->CallbackRecord) {
      ExFreePool(BugcheckContext);
      return;
  }

  Adapter->BugcheckContext = BugcheckContext;

  KeInitializeCallbackRecord(BugcheckContext->CallbackRecord);

  KeRegisterBugCheckCallback(BugcheckContext->CallbackRecord, NdisIBugcheckCallback,
      BugcheckContext, sizeof(*BugcheckContext), (PUCHAR)"Ndis Miniport");

  IoRegisterShutdownNotification(Adapter->NdisMiniportBlock.DeviceObject);
}

NDIS_STATUS
DoQueries(
    PLOGICAL_ADAPTER Adapter,
    NDIS_OID AddressOID)
/*
 * FUNCTION: Queries miniport for information
 * ARGUMENTS:
 *     Adapter    = Pointer to logical adapter
 *     AddressOID = OID to use to query for current address
 * RETURNS:
 *     Status of operation
 */
{
  ULONG BytesWritten;
  NDIS_STATUS NdisStatus;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  /* Get MAC options for adapter */
  NdisStatus = MiniQueryInformation(Adapter, OID_GEN_MAC_OPTIONS, sizeof(UINT),
                                    &Adapter->NdisMiniportBlock.MacOptions,
                                    &BytesWritten);

  if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("OID_GEN_MAC_OPTIONS failed. NdisStatus (0x%X).\n", NdisStatus));
      return NdisStatus;
    }

  NDIS_DbgPrint(DEBUG_MINIPORT, ("MacOptions (0x%X).\n", Adapter->NdisMiniportBlock.MacOptions));

  /* Get current hardware address of adapter */
  NdisStatus = MiniQueryInformation(Adapter, AddressOID, Adapter->AddressLength,
                                    &Adapter->Address, &BytesWritten);

  if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Address OID (0x%X) failed. NdisStatus (0x%X).\n", AddressOID, NdisStatus));
      return NdisStatus;
    }

#if DBG
    {
      /* 802.3 only */

      PUCHAR A = (PUCHAR)&Adapter->Address.Type.Medium802_3;

      NDIS_DbgPrint(MAX_TRACE, ("Adapter address is (%02X %02X %02X %02X %02X %02X).\n", A[0], A[1], A[2], A[3], A[4], A[5]));
    }
#endif /* DBG */

  /* Get maximum lookahead buffer size of adapter */
  NdisStatus = MiniQueryInformation(Adapter, OID_GEN_MAXIMUM_LOOKAHEAD, sizeof(ULONG),
                                    &Adapter->NdisMiniportBlock.MaximumLookahead, &BytesWritten);

  if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("OID_GEN_MAXIMUM_LOOKAHEAD failed. NdisStatus (0x%X).\n", NdisStatus));
      return NdisStatus;
    }

  NDIS_DbgPrint(DEBUG_MINIPORT, ("MaxLookaheadLength (0x%X).\n", Adapter->NdisMiniportBlock.MaximumLookahead));

  /* Get current lookahead buffer size of adapter */
  NdisStatus = MiniQueryInformation(Adapter, OID_GEN_CURRENT_LOOKAHEAD, sizeof(ULONG),
                                    &Adapter->NdisMiniportBlock.CurrentLookahead, &BytesWritten);

  if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("OID_GEN_CURRENT_LOOKAHEAD failed. NdisStatus (0x%X).\n", NdisStatus));
      return NdisStatus;
    }

  NdisStatus = MiniQueryInformation(Adapter, OID_GEN_MAXIMUM_SEND_PACKETS, sizeof(ULONG),
                                    &Adapter->NdisMiniportBlock.MaxSendPackets, &BytesWritten);

  if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("OID_GEN_MAXIMUM_SEND_PACKETS failed. NdisStatus (0x%X).\n", NdisStatus));

      /* Set it to 1 if it fails because some drivers don't support this (?)*/
      Adapter->NdisMiniportBlock.MaxSendPackets = 1;
    }

  NDIS_DbgPrint(DEBUG_MINIPORT, ("CurLookaheadLength (0x%X).\n", Adapter->NdisMiniportBlock.CurrentLookahead));

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NdisIForwardIrpAndWaitCompletionRoutine(
    PDEVICE_OBJECT Fdo,
    PIRP Irp,
    PVOID Context)
{
  PKEVENT Event = Context;

  if (Irp->PendingReturned)
    KeSetEvent(Event, IO_NO_INCREMENT, FALSE);

  return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS
NTAPI
NdisIForwardIrpAndWait(PLOGICAL_ADAPTER Adapter, PIRP Irp)
{
  KEVENT Event;
  NTSTATUS Status;

  KeInitializeEvent(&Event, NotificationEvent, FALSE);
  IoCopyCurrentIrpStackLocationToNext(Irp);
  IoSetCompletionRoutine(Irp, NdisIForwardIrpAndWaitCompletionRoutine, &Event,
                         TRUE, TRUE, TRUE);
  Status = IoCallDriver(Adapter->NdisMiniportBlock.NextDeviceObject, Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = Irp->IoStatus.Status;
    }
  return Status;
}

NTSTATUS
NTAPI
NdisICreateClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NdisIPnPStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Handle the PnP start device event
 * ARGUMENTS:
 *     DeviceObejct = Functional Device Object
 *     Irp          = IRP_MN_START_DEVICE I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)DeviceObject->DeviceExtension;
  NDIS_WRAPPER_CONTEXT WrapperContext;
  NDIS_STATUS NdisStatus;
  NDIS_STATUS OpenErrorStatus;
  NTSTATUS Status;
  UINT SelectedMediumIndex = 0;
  NDIS_OID AddressOID;
  BOOLEAN Success = FALSE;
  ULONG ResourceCount;
  ULONG ResourceListSize;
  UNICODE_STRING ParamName;
  PNDIS_CONFIGURATION_PARAMETER ConfigParam;
  NDIS_HANDLE ConfigHandle;
  ULONG Size;
  LARGE_INTEGER Timeout;
  UINT MaxMulticastAddresses;
  ULONG BytesWritten;
  PLIST_ENTRY CurrentEntry;
  PPROTOCOL_BINDING ProtocolBinding;

  /*
   * Prepare wrapper context used by HW and configuration routines.
   */

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Start Device %wZ\n", &Adapter->NdisMiniportBlock.MiniportName));

  NDIS_DbgPrint(MAX_TRACE, ("Inserting adapter 0x%x into adapter list\n", Adapter));

  /* Put adapter in global adapter list */
  ExInterlockedInsertTailList(&AdapterListHead, &Adapter->ListEntry, &AdapterListLock);

  Status = IoOpenDeviceRegistryKey(
    Adapter->NdisMiniportBlock.PhysicalDeviceObject, PLUGPLAY_REGKEY_DRIVER,
    KEY_ALL_ACCESS, &WrapperContext.RegistryHandle);
  if (!NT_SUCCESS(Status))
    {
      NDIS_DbgPrint(MIN_TRACE,("failed to open adapter-specific reg key\n"));
      ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
      return Status;
    }

  NDIS_DbgPrint(MAX_TRACE, ("opened device reg key\n"));

  WrapperContext.DeviceObject = Adapter->NdisMiniportBlock.DeviceObject;

  /*
   * Store the adapter resources used by HW routines such as
   * NdisMQueryAdapterResources.
   */

  if (Stack->Parameters.StartDevice.AllocatedResources != NULL)
    {
      ResourceCount = Stack->Parameters.StartDevice.AllocatedResources->List[0].
                      PartialResourceList.Count;
      ResourceListSize =
        FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.
                     PartialDescriptors[ResourceCount]);

      Adapter->NdisMiniportBlock.AllocatedResources =
        ExAllocatePool(PagedPool, ResourceListSize);
      if (Adapter->NdisMiniportBlock.AllocatedResources == NULL)
        {
          NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
	  ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
          return STATUS_INSUFFICIENT_RESOURCES;
        }

      Adapter->NdisMiniportBlock.Resources =
        ExAllocatePool(PagedPool, ResourceListSize);
      if (!Adapter->NdisMiniportBlock.Resources)
      {
          NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
          ExFreePool(Adapter->NdisMiniportBlock.AllocatedResources);
          ExInterlockedRemoveEntryList(&Adapter->ListEntry, &AdapterListLock);
          return STATUS_INSUFFICIENT_RESOURCES;
      }

      RtlCopyMemory(Adapter->NdisMiniportBlock.Resources,
                    Stack->Parameters.StartDevice.AllocatedResources,
                    ResourceListSize);

      RtlCopyMemory(Adapter->NdisMiniportBlock.AllocatedResources,
                    Stack->Parameters.StartDevice.AllocatedResources,
                    ResourceListSize);
    }

  if (Stack->Parameters.StartDevice.AllocatedResourcesTranslated != NULL)
    {
      ResourceCount = Stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].
                      PartialResourceList.Count;
      ResourceListSize =
        FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.
                     PartialDescriptors[ResourceCount]);

      Adapter->NdisMiniportBlock.AllocatedResourcesTranslated =
        ExAllocatePool(PagedPool, ResourceListSize);
      if (Adapter->NdisMiniportBlock.AllocatedResourcesTranslated == NULL)
        {
          NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
	  ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
          return STATUS_INSUFFICIENT_RESOURCES;
        }

      RtlCopyMemory(Adapter->NdisMiniportBlock.AllocatedResourcesTranslated,
                    Stack->Parameters.StartDevice.AllocatedResourcesTranslated,
                    ResourceListSize);
   }

  /*
   * Store the Bus Type, Bus Number and Slot information. It's used by
   * the hardware routines then.
   */

  NdisOpenConfiguration(&NdisStatus, &ConfigHandle, (NDIS_HANDLE)&WrapperContext);
  if (NdisStatus != NDIS_STATUS_SUCCESS)
  {
      NDIS_DbgPrint(MIN_TRACE, ("Failed to open configuration key\n"));
      ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
      return NdisStatus;
  }

  Size = sizeof(ULONG);
  Status = IoGetDeviceProperty(Adapter->NdisMiniportBlock.PhysicalDeviceObject,
                               DevicePropertyLegacyBusType, Size,
                               &Adapter->NdisMiniportBlock.BusType, &Size);
  if (!NT_SUCCESS(Status) || (INTERFACE_TYPE)Adapter->NdisMiniportBlock.BusType == InterfaceTypeUndefined)
    {
      NdisInitUnicodeString(&ParamName, L"BusType");
      NdisReadConfiguration(&NdisStatus, &ConfigParam, ConfigHandle,
                            &ParamName, NdisParameterInteger);
      if (NdisStatus == NDIS_STATUS_SUCCESS)
        Adapter->NdisMiniportBlock.BusType = ConfigParam->ParameterData.IntegerData;
      else
        Adapter->NdisMiniportBlock.BusType = Isa;
    }

  Status = IoGetDeviceProperty(Adapter->NdisMiniportBlock.PhysicalDeviceObject,
                               DevicePropertyBusNumber, Size,
                               &Adapter->NdisMiniportBlock.BusNumber, &Size);
  if (!NT_SUCCESS(Status) || Adapter->NdisMiniportBlock.BusNumber == 0xFFFFFFF0)
    {
      NdisInitUnicodeString(&ParamName, L"BusNumber");
      NdisReadConfiguration(&NdisStatus, &ConfigParam, ConfigHandle,
                            &ParamName, NdisParameterInteger);
      if (NdisStatus == NDIS_STATUS_SUCCESS)
        Adapter->NdisMiniportBlock.BusNumber = ConfigParam->ParameterData.IntegerData;
      else
        Adapter->NdisMiniportBlock.BusNumber = 0;
    }
  WrapperContext.BusNumber = Adapter->NdisMiniportBlock.BusNumber;

  Status = IoGetDeviceProperty(Adapter->NdisMiniportBlock.PhysicalDeviceObject,
                               DevicePropertyAddress, Size,
                               &Adapter->NdisMiniportBlock.SlotNumber, &Size);
  if (!NT_SUCCESS(Status) || Adapter->NdisMiniportBlock.SlotNumber == (NDIS_INTERFACE_TYPE)-1)
    {
      NdisInitUnicodeString(&ParamName, L"SlotNumber");
      NdisReadConfiguration(&NdisStatus, &ConfigParam, ConfigHandle,
                            &ParamName, NdisParameterInteger);
      if (NdisStatus == NDIS_STATUS_SUCCESS)
        Adapter->NdisMiniportBlock.SlotNumber = ConfigParam->ParameterData.IntegerData;
      else
        Adapter->NdisMiniportBlock.SlotNumber = 0;
    }
  else
    {
        /* Convert slotnumber to PCI_SLOT_NUMBER */
        ULONG PciSlotNumber = Adapter->NdisMiniportBlock.SlotNumber;
        PCI_SLOT_NUMBER SlotNumber;

        SlotNumber.u.AsULONG = 0;
        SlotNumber.u.bits.DeviceNumber = (PciSlotNumber >> 16) & 0xFFFF;
        SlotNumber.u.bits.FunctionNumber = PciSlotNumber & 0xFFFF;

        Adapter->NdisMiniportBlock.SlotNumber = SlotNumber.u.AsULONG;
    }
  WrapperContext.SlotNumber = Adapter->NdisMiniportBlock.SlotNumber;

  NdisCloseConfiguration(ConfigHandle);

  /* Set handlers (some NDIS macros require these) */
  Adapter->NdisMiniportBlock.EthRxCompleteHandler = EthFilterDprIndicateReceiveComplete;
  Adapter->NdisMiniportBlock.EthRxIndicateHandler = EthFilterDprIndicateReceive;
  Adapter->NdisMiniportBlock.SendCompleteHandler  = MiniSendComplete;
  Adapter->NdisMiniportBlock.SendResourcesHandler = MiniSendResourcesAvailable;
  Adapter->NdisMiniportBlock.ResetCompleteHandler = MiniResetComplete;
  Adapter->NdisMiniportBlock.TDCompleteHandler    = MiniTransferDataComplete;
  Adapter->NdisMiniportBlock.PacketIndicateHandler= MiniIndicateReceivePacket;
  Adapter->NdisMiniportBlock.StatusHandler        = MiniStatus;
  Adapter->NdisMiniportBlock.StatusCompleteHandler= MiniStatusComplete;
  Adapter->NdisMiniportBlock.SendPacketsHandler   = ProSendPackets;
  Adapter->NdisMiniportBlock.QueryCompleteHandler = MiniRequestComplete;
  Adapter->NdisMiniportBlock.SetCompleteHandler   = MiniRequestComplete;

  /*
   * Call MiniportInitialize.
   */

  NDIS_DbgPrint(MID_TRACE, ("calling MiniportInitialize\n"));
  NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.InitializeHandler)(
    &OpenErrorStatus, &SelectedMediumIndex, &MediaArray[0],
    MEDIA_ARRAY_SIZE, Adapter, (NDIS_HANDLE)&WrapperContext);

  ZwClose(WrapperContext.RegistryHandle);

  if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("MiniportInitialize() failed for an adapter.\n"));
      ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
      if (Adapter->NdisMiniportBlock.Interrupt)
      {
          KeBugCheckEx(BUGCODE_ID_DRIVER,
                       (ULONG_PTR)Adapter,
                       (ULONG_PTR)Adapter->NdisMiniportBlock.Interrupt,
                       (ULONG_PTR)Adapter->NdisMiniportBlock.TimerQueue,
                       1);
      }
      if (Adapter->NdisMiniportBlock.TimerQueue)
      {
          KeBugCheckEx(BUGCODE_ID_DRIVER,
                       (ULONG_PTR)Adapter,
                       (ULONG_PTR)Adapter->NdisMiniportBlock.Interrupt,
                       (ULONG_PTR)Adapter->NdisMiniportBlock.TimerQueue,
                       1);
      }
      return NdisStatus;
    }

  if (SelectedMediumIndex >= MEDIA_ARRAY_SIZE)
    {
      NDIS_DbgPrint(MIN_TRACE, ("MiniportInitialize() selected a bad index\n"));
      ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
      return NDIS_STATUS_UNSUPPORTED_MEDIA;
    }

  Adapter->NdisMiniportBlock.MediaType = MediaArray[SelectedMediumIndex];

  switch (Adapter->NdisMiniportBlock.MediaType)
    {
      case NdisMedium802_3:
        Adapter->MediumHeaderSize = 14;       /* XXX figure out what to do about LLC */
        AddressOID = OID_802_3_CURRENT_ADDRESS;
        Adapter->AddressLength = ETH_LENGTH_OF_ADDRESS;
        NdisStatus = DoQueries(Adapter, AddressOID);
        if (NdisStatus == NDIS_STATUS_SUCCESS)
          {
            NdisStatus = MiniQueryInformation(Adapter, OID_802_3_MAXIMUM_LIST_SIZE, sizeof(UINT),
                                    &MaxMulticastAddresses, &BytesWritten);

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
               ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
               NDIS_DbgPrint(MIN_TRACE, ("MiniQueryInformation failed (%x)\n", NdisStatus));
               return NdisStatus;
            }

            Success = EthCreateFilter(MaxMulticastAddresses,
                                      Adapter->Address.Type.Medium802_3,
                                      &Adapter->NdisMiniportBlock.EthDB);
            if (Success)
              ((PETHI_FILTER)Adapter->NdisMiniportBlock.EthDB)->Miniport = (PNDIS_MINIPORT_BLOCK)Adapter;
            else
              NdisStatus = NDIS_STATUS_RESOURCES;
          }
        break;

      default:
        /* FIXME: Support other types of media */
        NDIS_DbgPrint(MIN_TRACE, ("error: unsupported media\n"));
        ASSERT(FALSE);
	ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
        return STATUS_UNSUCCESSFUL;
    }

  if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("couldn't create filter (%x)\n", NdisStatus));
      return NdisStatus;
    }

  /* Check for a hang every two seconds if it wasn't set in MiniportInitialize */
  if (Adapter->NdisMiniportBlock.CheckForHangSeconds == 0)
      Adapter->NdisMiniportBlock.CheckForHangSeconds = 2;

  Adapter->NdisMiniportBlock.OldPnPDeviceState = Adapter->NdisMiniportBlock.PnPDeviceState;
  Adapter->NdisMiniportBlock.PnPDeviceState = NdisPnPDeviceStarted;

  IoSetDeviceInterfaceState(&Adapter->NdisMiniportBlock.SymbolicLinkName, TRUE);

  Timeout.QuadPart = Int32x32To64(Adapter->NdisMiniportBlock.CheckForHangSeconds, -1000000);
  KeSetTimerEx(&Adapter->NdisMiniportBlock.WakeUpDpcTimer.Timer, Timeout,
               Adapter->NdisMiniportBlock.CheckForHangSeconds * 1000,
               &Adapter->NdisMiniportBlock.WakeUpDpcTimer.Dpc);

  /* Put adapter in adapter list for this miniport */
  ExInterlockedInsertTailList(&Adapter->NdisMiniportBlock.DriverHandle->DeviceList, &Adapter->MiniportListEntry, &Adapter->NdisMiniportBlock.DriverHandle->Lock);

  /* Refresh bindings for all protocols */
  CurrentEntry = ProtocolListHead.Flink;
  while (CurrentEntry != &ProtocolListHead)
  {
      ProtocolBinding = CONTAINING_RECORD(CurrentEntry, PROTOCOL_BINDING, ListEntry);

      ndisBindMiniportsToProtocol(&NdisStatus, ProtocolBinding);

      CurrentEntry = CurrentEntry->Flink;
  }

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NdisIPnPStopDevice(
    IN PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Handle the PnP stop device event
 * ARGUMENTS:
 *     DeviceObejct = Functional Device Object
 *     Irp          = IRP_MN_STOP_DEVICE I/O request packet
 * RETURNS:
 *     Status of operation
 */
{
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)DeviceObject->DeviceExtension;

  /* Remove adapter from adapter list for this miniport */
  ExInterlockedRemoveEntryList(&Adapter->MiniportListEntry, &Adapter->NdisMiniportBlock.DriverHandle->Lock);

  /* Remove adapter from global adapter list */
  ExInterlockedRemoveEntryList(&Adapter->ListEntry, &AdapterListLock);

  KeCancelTimer(&Adapter->NdisMiniportBlock.WakeUpDpcTimer.Timer);

  /* Set this here so MiniportISR will be forced to run for interrupts generated in MiniportHalt */
  Adapter->NdisMiniportBlock.OldPnPDeviceState = Adapter->NdisMiniportBlock.PnPDeviceState;
  Adapter->NdisMiniportBlock.PnPDeviceState = NdisPnPDeviceStopped;

  (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.HaltHandler)(Adapter);

  IoSetDeviceInterfaceState(&Adapter->NdisMiniportBlock.SymbolicLinkName, FALSE);

  if (Adapter->NdisMiniportBlock.AllocatedResources)
    {
      ExFreePool(Adapter->NdisMiniportBlock.AllocatedResources);
      Adapter->NdisMiniportBlock.AllocatedResources = NULL;
    }
  if (Adapter->NdisMiniportBlock.AllocatedResourcesTranslated)
    {
      ExFreePool(Adapter->NdisMiniportBlock.AllocatedResourcesTranslated);
      Adapter->NdisMiniportBlock.AllocatedResourcesTranslated = NULL;
    }

  if (Adapter->NdisMiniportBlock.Resources)
    {
      ExFreePool(Adapter->NdisMiniportBlock.Resources);
      Adapter->NdisMiniportBlock.Resources = NULL;
    }

  if (Adapter->NdisMiniportBlock.EthDB)
    {
      EthDeleteFilter(Adapter->NdisMiniportBlock.EthDB);
      Adapter->NdisMiniportBlock.EthDB = NULL;
    }

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NdisIShutdown(
    IN PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
  PLOGICAL_ADAPTER Adapter = DeviceObject->DeviceExtension;
  PMINIPORT_BUGCHECK_CONTEXT Context = Adapter->BugcheckContext;
  ADAPTER_SHUTDOWN_HANDLER ShutdownHandler = Context->ShutdownHandler;

  ASSERT(ShutdownHandler);

  ShutdownHandler(Context->DriverContext);

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NdisIDeviceIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)DeviceObject->DeviceExtension;
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  NDIS_STATUS Status = STATUS_NOT_SUPPORTED;
  ULONG Written;

  Irp->IoStatus.Information = 0;

  ASSERT(Adapter);

  switch (Stack->Parameters.DeviceIoControl.IoControlCode)
  {
    case IOCTL_NDIS_QUERY_GLOBAL_STATS:
      Status = MiniQueryInformation(Adapter,
                                    *(PNDIS_OID)Irp->AssociatedIrp.SystemBuffer,
                                    Stack->Parameters.DeviceIoControl.OutputBufferLength,
                                    MmGetSystemAddressForMdl(Irp->MdlAddress),
                                    &Written);
      Irp->IoStatus.Information = Written;
      break;

    default:
      ASSERT(FALSE);
      break;
  }

  if (Status != NDIS_STATUS_PENDING)
  {
      Irp->IoStatus.Status = Status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }
  else
      IoMarkIrpPending(Irp);

  return Status;
}

NTSTATUS
NTAPI
NdisIDispatchPnp(
    IN PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
{
  PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
  PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)DeviceObject->DeviceExtension;
  NTSTATUS Status;

  switch (Stack->MinorFunction)
    {
      case IRP_MN_START_DEVICE:
        Status = NdisIForwardIrpAndWait(Adapter, Irp);
        if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
          {
	      Status = NdisIPnPStartDevice(DeviceObject, Irp);
          }
          else
              NDIS_DbgPrint(MIN_TRACE, ("Lower driver failed device start\n"));
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;

      case IRP_MN_STOP_DEVICE:
        Status = NdisIPnPStopDevice(DeviceObject, Irp);
        if (!NT_SUCCESS(Status))
            NDIS_DbgPrint(MIN_TRACE, ("WARNING: Ignoring halt device failure! Passing the IRP down anyway\n"));
        Irp->IoStatus.Status = STATUS_SUCCESS;
        break;

      case IRP_MN_QUERY_REMOVE_DEVICE:
      case IRP_MN_QUERY_STOP_DEVICE:
        Status = NdisIPnPQueryStopDevice(DeviceObject, Irp);
        Irp->IoStatus.Status = Status;
        if (Status != STATUS_SUCCESS)
        {
            IoCompleteRequest(Irp, IO_NO_INCREMENT);
            NDIS_DbgPrint(MIN_TRACE, ("Failing miniport halt request\n"));
            return Status;
        }
        break;

      case IRP_MN_CANCEL_REMOVE_DEVICE:
      case IRP_MN_CANCEL_STOP_DEVICE:
        Status = NdisIForwardIrpAndWait(Adapter, Irp);
        if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
        {
            Status = NdisIPnPCancelStopDevice(DeviceObject, Irp);
        }
        else
        {
            NDIS_DbgPrint(MIN_TRACE, ("Lower driver failed cancel stop/remove request\n"));
        }
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return Status;

      case IRP_MN_QUERY_PNP_DEVICE_STATE:
        Status = NDIS_STATUS_SUCCESS;
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information |= Adapter->NdisMiniportBlock.PnPFlags;
        break;

      default:
        break;
    }

  IoSkipCurrentIrpStackLocation(Irp);
  return IoCallDriver(Adapter->NdisMiniportBlock.NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
NdisIPower(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp)
{
  PLOGICAL_ADAPTER Adapter = DeviceObject->DeviceExtension;

  PoStartNextPowerIrp(Irp);
  IoSkipCurrentIrpStackLocation(Irp);
  return PoCallDriver(Adapter->NdisMiniportBlock.NextDeviceObject, Irp);
}

NTSTATUS
NTAPI
NdisIAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
/*
 * FUNCTION: Create a device for an adapter found using PnP
 * ARGUMENTS:
 *     DriverObject         = Pointer to the miniport driver object
 *     PhysicalDeviceObject = Pointer to the PDO for our adapter
 */
{
  static const WCHAR ClassKeyName[] = {'C','l','a','s','s','\\'};
  static const WCHAR LinkageKeyName[] = {'\\','L','i','n','k','a','g','e',0};
  PNDIS_M_DRIVER_BLOCK Miniport;
  PNDIS_M_DRIVER_BLOCK *MiniportPtr;
  WCHAR *LinkageKeyBuffer;
  ULONG DriverKeyLength;
  RTL_QUERY_REGISTRY_TABLE QueryTable[2];
  UNICODE_STRING ExportName;
  PDEVICE_OBJECT DeviceObject;
  PLOGICAL_ADAPTER Adapter;
  NTSTATUS Status;

  /*
   * Gain the access to the miniport data structure first.
   */

  MiniportPtr = IoGetDriverObjectExtension(DriverObject, (PVOID)'NMID');
  if (MiniportPtr == NULL)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Can't get driver object extension.\n"));
      return NDIS_STATUS_FAILURE;
    }
  Miniport = *MiniportPtr;

  /*
   * Get name of the Linkage registry key for our adapter. It's located under
   * the driver key for our driver and so we have basicly two ways to do it.
   * Either we can use IoOpenDriverRegistryKey or compose it using information
   * gathered by IoGetDeviceProperty. I choosed the second because
   * IoOpenDriverRegistryKey wasn't implemented at the time of writing.
   */

  Status = IoGetDeviceProperty(PhysicalDeviceObject, DevicePropertyDriverKeyName,
                               0, NULL, &DriverKeyLength);
  if (Status != STATUS_BUFFER_TOO_SMALL && Status != STATUS_BUFFER_OVERFLOW && Status != STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Can't get miniport driver key length.\n"));
      return Status;
    }

  LinkageKeyBuffer = ExAllocatePool(PagedPool, DriverKeyLength +
                                    sizeof(ClassKeyName) + sizeof(LinkageKeyName));
  if (LinkageKeyBuffer == NULL)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Can't allocate memory for driver key name.\n"));
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = IoGetDeviceProperty(PhysicalDeviceObject, DevicePropertyDriverKeyName,
                               DriverKeyLength, LinkageKeyBuffer +
                               (sizeof(ClassKeyName) / sizeof(WCHAR)),
                               &DriverKeyLength);
  if (!NT_SUCCESS(Status))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Can't get miniport driver key.\n"));
      ExFreePool(LinkageKeyBuffer);
      return Status;
    }

  /* Compose the linkage key name. */
  RtlCopyMemory(LinkageKeyBuffer, ClassKeyName, sizeof(ClassKeyName));
  RtlCopyMemory(LinkageKeyBuffer + ((sizeof(ClassKeyName) + DriverKeyLength) /
                sizeof(WCHAR)) - 1, LinkageKeyName, sizeof(LinkageKeyName));

  NDIS_DbgPrint(DEBUG_MINIPORT, ("LinkageKey: %S.\n", LinkageKeyBuffer));

  /*
   * Now open the linkage key and read the "Export" and "RootDevice" values
   * which contains device name and root service respectively.
   */

  RtlZeroMemory(QueryTable, sizeof(QueryTable));
  RtlInitUnicodeString(&ExportName, NULL);
  QueryTable[0].Flags = RTL_QUERY_REGISTRY_REQUIRED | RTL_QUERY_REGISTRY_DIRECT;
  QueryTable[0].Name = L"Export";
  QueryTable[0].EntryContext = &ExportName;

  Status = RtlQueryRegistryValues(RTL_REGISTRY_CONTROL, LinkageKeyBuffer,
                                  QueryTable, NULL, NULL);
  ExFreePool(LinkageKeyBuffer);
  if (!NT_SUCCESS(Status))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Can't get miniport device name. (%x)\n", Status));
      return Status;
    }

  /*
   * Create the device object.
   */

  NDIS_DbgPrint(MAX_TRACE, ("creating device %wZ\n", &ExportName));

  Status = IoCreateDevice(Miniport->DriverObject, sizeof(LOGICAL_ADAPTER),
    &ExportName, FILE_DEVICE_PHYSICAL_NETCARD,
    0, FALSE, &DeviceObject);
  if (!NT_SUCCESS(Status))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Could not create device object.\n"));
      RtlFreeUnicodeString(&ExportName);
      return Status;
    }

  /*
   * Initialize the adapter structure.
   */

  Adapter = (PLOGICAL_ADAPTER)DeviceObject->DeviceExtension;
  KeInitializeSpinLock(&Adapter->NdisMiniportBlock.Lock);
  InitializeListHead(&Adapter->ProtocolListHead);

  Status = IoRegisterDeviceInterface(PhysicalDeviceObject,
                                     &GUID_DEVINTERFACE_NET,
                                     NULL,
                                     &Adapter->NdisMiniportBlock.SymbolicLinkName);

  if (!NT_SUCCESS(Status))
  {
      NDIS_DbgPrint(MIN_TRACE, ("Could not create device interface.\n"));
      IoDeleteDevice(DeviceObject);
      RtlFreeUnicodeString(&ExportName);
      return Status;
  }

  Adapter->NdisMiniportBlock.DriverHandle = Miniport;
  Adapter->NdisMiniportBlock.MiniportName = ExportName;
  Adapter->NdisMiniportBlock.DeviceObject = DeviceObject;
  Adapter->NdisMiniportBlock.PhysicalDeviceObject = PhysicalDeviceObject;
  Adapter->NdisMiniportBlock.NextDeviceObject =
    IoAttachDeviceToDeviceStack(Adapter->NdisMiniportBlock.DeviceObject,
                                PhysicalDeviceObject);

  Adapter->NdisMiniportBlock.OldPnPDeviceState = 0;
  Adapter->NdisMiniportBlock.PnPDeviceState = NdisPnPDeviceAdded;

  KeInitializeTimer(&Adapter->NdisMiniportBlock.WakeUpDpcTimer.Timer);
  KeInitializeDpc(&Adapter->NdisMiniportBlock.WakeUpDpcTimer.Dpc, MiniportHangDpc, Adapter);

  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NdisGenericIrpHandler(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(Irp);

    /* Use the characteristics to classify the device */
    if (DeviceObject->DeviceType == FILE_DEVICE_PHYSICAL_NETCARD)
    {
        if ((IrpSp->MajorFunction == IRP_MJ_CREATE) ||
            (IrpSp->MajorFunction == IRP_MJ_CLOSE))
        {
            return NdisICreateClose(DeviceObject, Irp);
        }
        else if (IrpSp->MajorFunction == IRP_MJ_PNP)
        {
            return NdisIDispatchPnp(DeviceObject, Irp);
        }
        else if (IrpSp->MajorFunction == IRP_MJ_SHUTDOWN)
        {
            return NdisIShutdown(DeviceObject, Irp);
        }
        else if (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL)
        {
            return NdisIDeviceIoControl(DeviceObject, Irp);
        }
        else if (IrpSp->MajorFunction == IRP_MJ_POWER)
        {
            return NdisIPower(DeviceObject, Irp);
        }
        NDIS_DbgPrint(MIN_TRACE, ("Unexpected IRP MajorFunction 0x%x\n", IrpSp->MajorFunction));
        ASSERT(FALSE);
    }
    else if (DeviceObject->DeviceType == FILE_DEVICE_NETWORK)
    {
        PNDIS_M_DEVICE_BLOCK DeviceBlock = DeviceObject->DeviceExtension;

        ASSERT(DeviceBlock->DeviceObject == DeviceObject);

        if (DeviceBlock->MajorFunction[IrpSp->MajorFunction] != NULL)
        {
            return DeviceBlock->MajorFunction[IrpSp->MajorFunction](DeviceObject, Irp);
        }
    }
    else
    {
        ASSERT(FALSE);
    }

    Irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_INVALID_DEVICE_REQUEST;
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterMiniport(
    IN  NDIS_HANDLE                     NdisWrapperHandle,
    IN  PNDIS_MINIPORT_CHARACTERISTICS  MiniportCharacteristics,
    IN  UINT                            CharacteristicsLength)
/*
 * FUNCTION: Registers a miniport's MiniportXxx entry points with the NDIS library
 * ARGUMENTS:
 *     NdisWrapperHandle       = Pointer to handle returned by NdisMInitializeWrapper
 *     MiniportCharacteristics = Pointer to a buffer with miniport characteristics
 *     CharacteristicsLength   = Number of bytes in characteristics buffer
 * RETURNS:
 *     Status of operation
 */
{
  UINT MinSize;
  PNDIS_M_DRIVER_BLOCK Miniport = GET_MINIPORT_DRIVER(NdisWrapperHandle);
  PNDIS_M_DRIVER_BLOCK *MiniportPtr;
  NTSTATUS Status;
  ULONG i;

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  switch (MiniportCharacteristics->MajorNdisVersion)
    {
      case 0x03:
        MinSize = sizeof(NDIS30_MINIPORT_CHARACTERISTICS);
        break;

      case 0x04:
        MinSize = sizeof(NDIS40_MINIPORT_CHARACTERISTICS);
        break;

      case 0x05:
        switch (MiniportCharacteristics->MinorNdisVersion)
        {
            case 0x00:
                MinSize = sizeof(NDIS50_MINIPORT_CHARACTERISTICS);
                break;

            case 0x01:
                MinSize = sizeof(NDIS51_MINIPORT_CHARACTERISTICS);
                break;

            default:
                NDIS_DbgPrint(MIN_TRACE, ("Bad 5.x minor characteristics version.\n"));
                return NDIS_STATUS_BAD_VERSION;
        }
        break;

      default:
        NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics version.\n"));
        return NDIS_STATUS_BAD_VERSION;
    }

   NDIS_DbgPrint(MID_TRACE, ("Initializing an NDIS %u.%u miniport\n",
                              MiniportCharacteristics->MajorNdisVersion,
                              MiniportCharacteristics->MinorNdisVersion));

  if (CharacteristicsLength < MinSize)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics length.\n"));
        return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

  /* Check if mandatory MiniportXxx functions are specified */
  if ((!MiniportCharacteristics->HaltHandler) ||
       (!MiniportCharacteristics->InitializeHandler)||
       (!MiniportCharacteristics->ResetHandler))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics.\n"));
      return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

  if (MiniportCharacteristics->MajorNdisVersion < 0x05)
  {
      if ((!MiniportCharacteristics->QueryInformationHandler) ||
          (!MiniportCharacteristics->SetInformationHandler))
      {
           NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics. (Set/Query)\n"));
           return NDIS_STATUS_BAD_CHARACTERISTICS;
      }
  }
  else
  {
      if (((!MiniportCharacteristics->QueryInformationHandler) ||
           (!MiniportCharacteristics->SetInformationHandler)) &&
           (!MiniportCharacteristics->CoRequestHandler))
      {
           NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics. (Set/Query)\n"));
           return NDIS_STATUS_BAD_CHARACTERISTICS;
      }
  }

  if (MiniportCharacteristics->MajorNdisVersion == 0x03)
    {
      if (!MiniportCharacteristics->SendHandler)
        {
          NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics. (NDIS 3.0)\n"));
          return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    }
  else if (MiniportCharacteristics->MajorNdisVersion == 0x04)
    {
      /* NDIS 4.0 */
      if ((!MiniportCharacteristics->SendHandler) &&
          (!MiniportCharacteristics->SendPacketsHandler))
        {
          NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics. (NDIS 4.0)\n"));
          return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    }
  else if (MiniportCharacteristics->MajorNdisVersion == 0x05)
    {
      /* TODO: Add more checks here */

      if ((!MiniportCharacteristics->SendHandler) &&
          (!MiniportCharacteristics->SendPacketsHandler) &&
          (!MiniportCharacteristics->CoSendPacketsHandler))
        {
          NDIS_DbgPrint(MIN_TRACE, ("Bad miniport characteristics. (NDIS 5.0)\n"));
          return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    }

  RtlCopyMemory(&Miniport->MiniportCharacteristics, MiniportCharacteristics, MinSize);

  /*
   * NOTE: This is VERY unoptimal! Should we store the NDIS_M_DRIVER_BLOCK
   * structure in the driver extension or what?
   */

  Status = IoAllocateDriverObjectExtension(Miniport->DriverObject, (PVOID)'NMID',
                                           sizeof(PNDIS_M_DRIVER_BLOCK), (PVOID*)&MiniportPtr);
  if (!NT_SUCCESS(Status))
    {
      NDIS_DbgPrint(MIN_TRACE, ("Can't allocate driver object extension.\n"));
      return NDIS_STATUS_RESOURCES;
    }

  *MiniportPtr = Miniport;

  /* We have to register for all of these so handler registered in NdisMRegisterDevice work */
  for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
  {
       Miniport->DriverObject->MajorFunction[i] = NdisGenericIrpHandler;
  }

  Miniport->DriverObject->DriverExtension->AddDevice = NdisIAddDevice;

  return NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
#undef NdisMResetComplete
VOID
EXPORT
NdisMResetComplete(
    IN NDIS_HANDLE MiniportAdapterHandle,
    IN NDIS_STATUS Status,
    IN BOOLEAN     AddressingReset)
{
  MiniResetComplete(MiniportAdapterHandle, Status, AddressingReset);
}

/*
 * @implemented
 */
#undef NdisMSendComplete
VOID
EXPORT
NdisMSendComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status)
/*
 * FUNCTION: Forwards a message to the initiating protocol saying
 *           that a packet was handled
 * ARGUMENTS:
 *     NdisAdapterHandle = Handle input to MiniportInitialize
 *     Packet            = Pointer to NDIS packet that was sent
 *     Status            = Status of send operation
 */
{
  MiniSendComplete(MiniportAdapterHandle, Packet, Status);
}

/*
 * @implemented
 */
#undef NdisMSendResourcesAvailable
VOID
EXPORT
NdisMSendResourcesAvailable(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
  MiniSendResourcesAvailable(MiniportAdapterHandle);
}

/*
 * @implemented
 */
#undef NdisMTransferDataComplete
VOID
EXPORT
NdisMTransferDataComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred)
{
  MiniTransferDataComplete(MiniportAdapterHandle, Packet, Status, BytesTransferred);
}

/*
 * @implemented
 */
#undef NdisMSetAttributes
VOID
EXPORT
NdisMSetAttributes(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  BOOLEAN             BusMaster,
    IN  NDIS_INTERFACE_TYPE AdapterType)
/*
 * FUNCTION: Informs the NDIS library of significant features of the caller's NIC
 * ARGUMENTS:
 *     MiniportAdapterHandle  = Handle input to MiniportInitialize
 *     MiniportAdapterContext = Pointer to context information
 *     BusMaster              = Specifies TRUE if the caller's NIC is a busmaster DMA device
 *     AdapterType            = Specifies the I/O bus interface of the caller's NIC
 */
{
  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
  NdisMSetAttributesEx(MiniportAdapterHandle, MiniportAdapterContext, 0,
                       BusMaster ? NDIS_ATTRIBUTE_BUS_MASTER : 0,
                       AdapterType);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMSetAttributesEx(
    IN  NDIS_HANDLE         MiniportAdapterHandle,
    IN  NDIS_HANDLE         MiniportAdapterContext,
    IN  UINT                CheckForHangTimeInSeconds   OPTIONAL,
    IN  ULONG               AttributeFlags,
    IN  NDIS_INTERFACE_TYPE	AdapterType)
/*
 * FUNCTION: Informs the NDIS library of significant features of the caller's NIC
 * ARGUMENTS:
 *     MiniportAdapterHandle     = Handle input to MiniportInitialize
 *     MiniportAdapterContext    = Pointer to context information
 *     CheckForHangTimeInSeconds = Specifies interval in seconds at which
 *                                 MiniportCheckForHang should be called
 *     AttributeFlags            = Bitmask that indicates specific attributes
 *     AdapterType               = Specifies the I/O bus interface of the caller's NIC
 */
{
  PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  Adapter->NdisMiniportBlock.MiniportAdapterContext = MiniportAdapterContext;
  Adapter->NdisMiniportBlock.Flags = AttributeFlags;
  Adapter->NdisMiniportBlock.AdapterType = AdapterType;
  if (CheckForHangTimeInSeconds > 0)
      Adapter->NdisMiniportBlock.CheckForHangSeconds = CheckForHangTimeInSeconds;
  if (AttributeFlags & NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER)
    NDIS_DbgPrint(MIN_TRACE, ("Intermediate drivers not supported yet.\n"));

  NDIS_DbgPrint(MID_TRACE, ("Miniport attribute flags: 0x%x\n", AttributeFlags));

  if (Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.AdapterShutdownHandler)
  {
      NDIS_DbgPrint(MAX_TRACE, ("Miniport set AdapterShutdownHandler in MiniportCharacteristics\n"));
      NdisMRegisterAdapterShutdownHandler(Adapter,
                      Adapter->NdisMiniportBlock.MiniportAdapterContext,
                      Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.AdapterShutdownHandler);
  }
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMSleep(
    IN  ULONG   MicrosecondsToSleep)
/*
 * FUNCTION: delay the thread's execution for MicrosecondsToSleep
 * ARGUMENTS:
 *     MicrosecondsToSleep: duh...
 * NOTES:
 *     - Because this is a blocking call, current IRQL must be < DISPATCH_LEVEL
 */
{
  KTIMER Timer;
  LARGE_INTEGER DueTime;

  PAGED_CODE();

  DueTime.QuadPart = (-1) * 10 * MicrosecondsToSleep;

  KeInitializeTimer(&Timer);
  KeSetTimer(&Timer, DueTime, 0);
  KeWaitForSingleObject(&Timer, Executive, KernelMode, FALSE, 0);
}

/*
 * @implemented
 */
BOOLEAN
EXPORT
NdisMSynchronizeWithInterrupt(
    IN  PNDIS_MINIPORT_INTERRUPT    Interrupt,
    IN  PVOID                       SynchronizeFunction,
    IN  PVOID                       SynchronizeContext)
{
  return(KeSynchronizeExecution(Interrupt->InterruptObject,
				(PKSYNCHRONIZE_ROUTINE)SynchronizeFunction,
				SynchronizeContext));
}

/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMWriteLogData(
    IN  NDIS_HANDLE LogHandle,
    IN  PVOID       LogBuffer,
    IN  UINT        LogBufferSize)
{
    PUCHAR Buffer = LogBuffer;
    UINT i, j, idx;

    UNIMPLEMENTED;
    for (i = 0; i < LogBufferSize; i += 16)
    {
        DbgPrint("%08x |", i);
        for (j = 0; j < 16; j++)
        {
            idx = i + j;
            if (idx < LogBufferSize)
                DbgPrint(" %02x", Buffer[idx]);
            else
                DbgPrint("   ");
        }
        DbgPrint(" | ");
        for (j = 0; j < 16; j++)
        {
            idx = i + j;
            if (idx == LogBufferSize)
                break;
            if (Buffer[idx] >= ' ') /* FIXME: not portable! replace by if (isprint(Buffer[idx])) ? */
                DbgPrint("%c", Buffer[idx]);
            else
                DbgPrint(".");
        }
        DbgPrint("\n");
    }

    return NDIS_STATUS_FAILURE;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisTerminateWrapper(
    IN  NDIS_HANDLE NdisWrapperHandle,
    IN  PVOID       SystemSpecific)
/*
 * FUNCTION: Releases resources allocated by a call to NdisInitializeWrapper
 * ARGUMENTS:
 *     NdisWrapperHandle = Handle returned by NdisInitializeWrapper (NDIS_M_DRIVER_BLOCK)
 *     SystemSpecific    = Always NULL
 */
{
  PNDIS_M_DRIVER_BLOCK Miniport = GET_MINIPORT_DRIVER(NdisWrapperHandle);

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  ExFreePool(Miniport->RegistryPath->Buffer);
  ExFreePool(Miniport->RegistryPath);
  ExInterlockedRemoveEntryList(&Miniport->ListEntry, &MiniportListLock);
  ExFreePool(Miniport);
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PLOGICAL_ADAPTER Adapter = (PLOGICAL_ADAPTER)MiniportAdapterHandle;
    UNICODE_STRING AdapterName;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    AdapterName.Length = 0;
    AdapterName.MaximumLength = Adapter->NdisMiniportBlock.MiniportName.MaximumLength;
    AdapterName.Buffer = ExAllocatePool(PagedPool, AdapterName.MaximumLength);
    if (!AdapterName.Buffer) {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
        return NDIS_STATUS_RESOURCES;
    }

    RtlCopyUnicodeString(&AdapterName, &Adapter->NdisMiniportBlock.MiniportName);

    *AdapterInstanceName = AdapterName;

    return NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisDeregisterAdapterShutdownHandler(
    IN  NDIS_HANDLE NdisAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    NdisMDeregisterAdapterShutdownHandler(NdisAdapterHandle);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisRegisterAdapterShutdownHandler(
    IN  NDIS_HANDLE                 NdisAdapterHandle,
    IN  PVOID                       ShutdownContext,
    IN  ADAPTER_SHUTDOWN_HANDLER    ShutdownHandler)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    NdisMRegisterAdapterShutdownHandler(NdisAdapterHandle,
                                        ShutdownContext,
                                        ShutdownHandler);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMGetDeviceProperty(
    IN      NDIS_HANDLE         MiniportAdapterHandle,
    IN OUT  PDEVICE_OBJECT      *PhysicalDeviceObject           OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *FunctionalDeviceObject         OPTIONAL,
    IN OUT  PDEVICE_OBJECT      *NextDeviceObject               OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResources             OPTIONAL,
    IN OUT  PCM_RESOURCE_LIST   *AllocatedResourcesTranslated   OPTIONAL)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Called\n"));

    if (PhysicalDeviceObject != NULL)
        *PhysicalDeviceObject = Adapter->NdisMiniportBlock.PhysicalDeviceObject;

    if (FunctionalDeviceObject != NULL)
        *FunctionalDeviceObject = Adapter->NdisMiniportBlock.DeviceObject;

    if (NextDeviceObject != NULL)
        *NextDeviceObject = Adapter->NdisMiniportBlock.NextDeviceObject;

    if (AllocatedResources != NULL)
        *AllocatedResources = Adapter->NdisMiniportBlock.AllocatedResources;

    if (AllocatedResourcesTranslated != NULL)
        *AllocatedResourcesTranslated = Adapter->NdisMiniportBlock.AllocatedResourcesTranslated;
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMRegisterUnloadHandler(
    IN  NDIS_HANDLE     NdisWrapperHandle,
    IN  PDRIVER_UNLOAD  UnloadHandler)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PNDIS_M_DRIVER_BLOCK DriverBlock = NdisWrapperHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Miniport registered unload handler\n"));

    DriverBlock->DriverObject->DriverUnload = UnloadHandler;
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMRegisterDevice(
    IN  NDIS_HANDLE         NdisWrapperHandle,
    IN  PNDIS_STRING        DeviceName,
    IN  PNDIS_STRING        SymbolicName,
    IN  PDRIVER_DISPATCH    MajorFunctions[],
    OUT PDEVICE_OBJECT      *pDeviceObject,
    OUT NDIS_HANDLE         *NdisDeviceHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PNDIS_M_DRIVER_BLOCK DriverBlock = NdisWrapperHandle;
    PNDIS_M_DEVICE_BLOCK DeviceBlock;
    PDEVICE_OBJECT DeviceObject;
    NDIS_STATUS Status;
    UINT i;

    NDIS_DbgPrint(MAX_TRACE, ("Called\n"));

    Status = IoCreateDevice(DriverBlock->DriverObject,
                            sizeof(NDIS_M_DEVICE_BLOCK),
                            DeviceName,
                            FILE_DEVICE_NETWORK,
                            0,
                            FALSE,
                            &DeviceObject);

    if (!NT_SUCCESS(Status))
    {
        NDIS_DbgPrint(MIN_TRACE, ("IoCreateDevice failed (%x)\n", Status));
        return Status;
    }

    Status = IoCreateSymbolicLink(SymbolicName, DeviceName);

    if (!NT_SUCCESS(Status))
    {
        NDIS_DbgPrint(MIN_TRACE, ("IoCreateSymbolicLink failed (%x)\n", Status));
        IoDeleteDevice(DeviceObject);
        return Status;
    }

    DeviceBlock = DeviceObject->DeviceExtension;

    if (!DeviceBlock)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources\n"));
        IoDeleteDevice(DeviceObject);
        IoDeleteSymbolicLink(SymbolicName);
        return NDIS_STATUS_RESOURCES;
    }

    for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
         DeviceBlock->MajorFunction[i] = MajorFunctions[i];

    DeviceBlock->DeviceObject = DeviceObject;
    DeviceBlock->SymbolicName = SymbolicName;

    *pDeviceObject = DeviceObject;
    *NdisDeviceHandle = DeviceBlock;

    return NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMDeregisterDevice(
    IN  NDIS_HANDLE NdisDeviceHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PNDIS_M_DEVICE_BLOCK DeviceBlock = NdisDeviceHandle;

    IoDeleteDevice(DeviceBlock->DeviceObject);

    IoDeleteSymbolicLink(DeviceBlock->SymbolicName);

    return NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisQueryAdapterInstanceName(
    OUT PNDIS_STRING    AdapterInstanceName,
    IN  NDIS_HANDLE     NdisBindingHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PADAPTER_BINDING AdapterBinding = NdisBindingHandle;
    PLOGICAL_ADAPTER Adapter = AdapterBinding->Adapter;

    return NdisMQueryAdapterInstanceName(AdapterInstanceName,
                                         Adapter);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisCompletePnPEvent(
    IN  NDIS_STATUS     Status,
    IN  NDIS_HANDLE     NdisBindingHandle,
    IN  PNET_PNP_EVENT  NetPnPEvent)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
  PIRP Irp = (PIRP)NetPnPEvent->NdisReserved[0];
  PLIST_ENTRY CurrentEntry = (PLIST_ENTRY)NetPnPEvent->NdisReserved[1];
  PADAPTER_BINDING AdapterBinding = NdisBindingHandle;
  PLOGICAL_ADAPTER Adapter = AdapterBinding->Adapter;
  NDIS_STATUS NdisStatus;

  if (Status != NDIS_STATUS_SUCCESS)
  {
      if (NetPnPEvent->Buffer) ExFreePool(NetPnPEvent->Buffer);
      ExFreePool(NetPnPEvent);
      Irp->IoStatus.Status = Status;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return;
  }

  while (CurrentEntry != &Adapter->ProtocolListHead)
  {
     AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);

     NdisStatus = (*AdapterBinding->ProtocolBinding->Chars.PnPEventHandler)(
      AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
      NetPnPEvent);

     if (NdisStatus == NDIS_STATUS_PENDING)
     {
         NetPnPEvent->NdisReserved[1] = (ULONG_PTR)CurrentEntry->Flink;
         return;
     }
     else if (NdisStatus != NDIS_STATUS_SUCCESS)
     {
         if (NetPnPEvent->Buffer) ExFreePool(NetPnPEvent->Buffer);
         ExFreePool(NetPnPEvent);
         Irp->IoStatus.Status = NdisStatus;
         IoCompleteRequest(Irp, IO_NO_INCREMENT);
         return;
     }

     CurrentEntry = CurrentEntry->Flink;
  }

  if (NetPnPEvent->Buffer) ExFreePool(NetPnPEvent->Buffer);
  ExFreePool(NetPnPEvent);

  Irp->IoStatus.Status = NDIS_STATUS_SUCCESS;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisCancelSendPackets(
    IN NDIS_HANDLE  NdisBindingHandle,
    IN PVOID  CancelId)
{
    PADAPTER_BINDING AdapterBinding = NdisBindingHandle;
    PLOGICAL_ADAPTER Adapter = AdapterBinding->Adapter;

    NDIS_DbgPrint(MAX_TRACE, ("Called for ID %x.\n", CancelId));

    if (Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.CancelSendPacketsHandler)
    {
        (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.CancelSendPacketsHandler)(
          Adapter->NdisMiniportBlock.MiniportAdapterContext,
          CancelId);
    }
}


/*
 * @implemented
 */
NDIS_HANDLE
EXPORT
NdisIMGetBindingContext(
    IN  NDIS_HANDLE NdisBindingHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PADAPTER_BINDING AdapterBinding = NdisBindingHandle;
    PLOGICAL_ADAPTER Adapter = AdapterBinding->Adapter;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return Adapter->NdisMiniportBlock.DeviceContext;
}


/*
 * @implemented
 */
NDIS_HANDLE
EXPORT
NdisIMGetDeviceContext(
    IN  NDIS_HANDLE MiniportAdapterHandle)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return Adapter->NdisMiniportBlock.DeviceContext;
}

/* EOF */
