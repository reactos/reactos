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
#include "efilter.h"

#ifdef DBG
#include <buffer.h>
#endif /* DBG */

#undef NdisMSendComplete
VOID
EXPORT
NdisMSendComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status);

/* Root of the scm database */
#define SERVICES_ROOT L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

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
#ifdef DBG
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
#ifdef DBG
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
  /* KIRQL OldIrql; */
  PLIST_ENTRY CurrentEntry;
  PADAPTER_BINDING AdapterBinding;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called. Adapter (0x%X)  HeaderBuffer (0x%X)  "
      "HeaderBufferSize (0x%X)  LookaheadBuffer (0x%X)  LookaheadBufferSize (0x%X).\n",
      Adapter, HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize));

  MiniDisplayPacket2(HeaderBuffer, HeaderBufferSize, LookaheadBuffer, LookaheadBufferSize);

  /*
   * XXX Think about this.  This is probably broken.  Spinlocks are
   * taken out for now until i comprehend the Right Way to do this.
   *
   * This used to acquire the MiniportBlock spinlock and hold it until
   * just before the call to ReceiveHandler.  It would then release and
   * subsequently re-acquire the lock.
   *
   * I don't see how this does any good, as it would seem he's just
   * trying to protect the packet list.  If somebody else dequeues
   * a packet, we are in fact in bad shape, but we don't want to
   * necessarily call the receive handler at elevated irql either.
   *
   * therefore: We *are* going to call the receive handler at high irql
   * (due to holding the lock) for now, and eventually we have to
   * figure out another way to protect this packet list.
   *
   * UPDATE: this is busted; this results in a recursive lock acquisition.
   */
  //NDIS_DbgPrint(MAX_TRACE, ("acquiring miniport block lock\n"));
  //KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql);
    {
      CurrentEntry = Adapter->ProtocolListHead.Flink;
      NDIS_DbgPrint(DEBUG_MINIPORT, ("CurrentEntry = %x\n", CurrentEntry));

      if (CurrentEntry == &Adapter->ProtocolListHead)
        {
          NDIS_DbgPrint(DEBUG_MINIPORT, ("WARNING: No upper protocol layer.\n"));
        }

      while (CurrentEntry != &Adapter->ProtocolListHead)
        {
          AdapterBinding = CONTAINING_RECORD(CurrentEntry, ADAPTER_BINDING, AdapterListEntry);
	  NDIS_DbgPrint(DEBUG_MINIPORT, ("AdapterBinding = %x\n", AdapterBinding));

          /* see above */
          /* KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql); */

#ifdef DBG
          if(!AdapterBinding)
            {
              NDIS_DbgPrint(MIN_TRACE, ("AdapterBinding was null\n"));
              break;
            }

          if(!AdapterBinding->ProtocolBinding)
            {
              NDIS_DbgPrint(MIN_TRACE, ("AdapterBinding->ProtocolBinding was null\n"));
              break;
            }

          if(!AdapterBinding->ProtocolBinding->Chars.ReceiveHandler)
            {
              NDIS_DbgPrint(MIN_TRACE, ("AdapterBinding->ProtocolBinding->Chars.ReceiveHandler was null\n"));
              break;
            }
#endif

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

          /* see above */
          /* KeAcquireSpinLock(&Adapter->NdisMiniportBlock.Lock, &OldIrql); */

          CurrentEntry = CurrentEntry->Flink;
        }
    }
  //KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql);

  NDIS_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}


VOID NTAPI
MiniIndicateReceivePacket(
    IN  NDIS_HANDLE    Miniport,
    IN  PPNDIS_PACKET  PacketArray,
    IN  UINT           NumberOfPackets)
/*
 * FUNCTION: receives miniport packet array indications
 * ARGUMENTS:
 *     Miniport: Miniport handle for the adapter
 *     PacketArray: pointer to a list of packet pointers to indicate
 *     NumberOfPackets: number of packets to indicate
 * NOTES:
 *     - This currently is a big temporary hack.  In the future this should
 *       call ProtocolReceivePacket() on each bound protocol if it exists.
 *       For now it just mimics NdisMEthIndicateReceive.
 */
{
  UINT i;

  for(i = 0; i < NumberOfPackets; i++)
    {
      PCHAR PacketBuffer = 0;
      UINT PacketLength = 0;
      PNDIS_BUFFER NdisBuffer = 0;

#define PACKET_TAG (('k' << 24) + ('P' << 16) + ('D' << 8) + 'N')

      NdisAllocateMemoryWithTag((PVOID)&PacketBuffer, 1518, PACKET_TAG);
      if(!PacketBuffer)
        {
          NDIS_DbgPrint(MIN_TRACE, ("insufficient resources\n"));
          return;
        }

      NdisQueryPacket(PacketArray[i], NULL, NULL, &NdisBuffer, NULL);

      while(NdisBuffer)
        {
          PNDIS_BUFFER CurrentBuffer;
          PVOID BufferVa;
          UINT BufferLen;

          NdisQueryBuffer(NdisBuffer, &BufferVa, &BufferLen);
          memcpy(PacketBuffer + PacketLength, BufferVa, BufferLen);
          PacketLength += BufferLen;

          CurrentBuffer = NdisBuffer;
          NdisGetNextBuffer(CurrentBuffer, &NdisBuffer);
        }

      NDIS_DbgPrint(MID_TRACE, ("indicating a %d-byte packet\n", PacketLength));

      MiniIndicateData(Miniport, NULL, PacketBuffer, 14, PacketBuffer+14, PacketLength-14, PacketLength-14);

      NdisFreeMemory(PacketBuffer, 0, 0);
    }
}


VOID NTAPI
MiniResetComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status,
    IN  BOOLEAN     AddressingReset)
{
    UNIMPLEMENTED
}



VOID NTAPI
MiniRequestComplete(
    IN PNDIS_MINIPORT_BLOCK Adapter,
    IN PNDIS_REQUEST Request,
    IN NDIS_STATUS Status)
{
    PNDIS_REQUEST_MAC_BLOCK MacBlock = (PNDIS_REQUEST_MAC_BLOCK)Request->MacReserved;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    if( MacBlock->Binding->RequestCompleteHandler ) {
        (*MacBlock->Binding->RequestCompleteHandler)(
            MacBlock->Binding->ProtocolBindingContext,
            Request,
            Status);
    }
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
    PADAPTER_BINDING AdapterBinding;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    AdapterBinding = (PADAPTER_BINDING)Packet->Reserved[0];

    (*AdapterBinding->ProtocolBinding->Chars.SendCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Packet,
        Status);
}


VOID NTAPI
MiniSendResourcesAvailable(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
/*
    UNIMPLEMENTED
*/
}


VOID NTAPI
MiniTransferDataComplete(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  PNDIS_PACKET    Packet,
    IN  NDIS_STATUS     Status,
    IN  UINT            BytesTransferred)
{
    PADAPTER_BINDING AdapterBinding;

    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

    AdapterBinding = (PADAPTER_BINDING)Packet->Reserved[0];

    (*AdapterBinding->ProtocolBinding->Chars.SendCompleteHandler)(
        AdapterBinding->NdisOpenBlock.ProtocolBindingContext,
        Packet,
        Status);
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

#ifdef DBG
  if(!Adapter)
    {
      NDIS_DbgPrint(MID_TRACE, ("Adapter object was null\n"));
      return FALSE;
    }

  if(!Packet)
    {
      NDIS_DbgPrint(MID_TRACE, ("Packet was null\n"));
      return FALSE;
    }
#endif

  NdisQueryPacket(Packet, NULL, NULL, &NdisBuffer, NULL);

  if (!NdisBuffer)
    {
      NDIS_DbgPrint(MID_TRACE, ("Packet contains no buffers.\n"));
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
        NDIS_DbgPrint(MID_TRACE, ("Buffer is too small.\n"));
        return FALSE;
    }

  Start1 = (PUCHAR)&Adapter->Address;
  NDIS_DbgPrint(MAX_TRACE, ("packet address: %x:%x:%x:%x:%x:%x adapter address: %x:%x:%x:%x:%x:%x\n",
      *((char *)Start1), *(((char *)Start1)+1), *(((char *)Start1)+2), *(((char *)Start1)+3), *(((char *)Start1)+4), *(((char *)Start1)+5),
      *((char *)Start2), *(((char *)Start2)+1), *(((char *)Start2)+2), *(((char *)Start2)+3), *(((char *)Start2)+4), *(((char *)Start2)+5))
  );

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
      NDIS_DbgPrint(DEBUG_MINIPORT, ("No registered miniports for protocol to bind to\n"));
      return NULL;
    }

  KeAcquireSpinLock(&AdapterListLock, &OldIrql);
    {
      CurrentEntry = AdapterListHead.Flink;
	
      while (CurrentEntry != &AdapterListHead)
        {
	  Adapter = CONTAINING_RECORD(CurrentEntry, LOGICAL_ADAPTER, ListEntry);

	  ASSERT(Adapter);

	  NDIS_DbgPrint(DEBUG_MINIPORT, ("Examining adapter 0x%lx\n", Adapter));
	  NDIS_DbgPrint(DEBUG_MINIPORT, ("AdapterName = %wZ\n", AdapterName));
	  NDIS_DbgPrint(DEBUG_MINIPORT, ("DeviceName = %wZ\n", &Adapter->NdisMiniportBlock.MiniportName));
	    
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
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Leaving (adapter not found).\n"));
    }

  return Adapter;
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
 * NOTES:
 *     If the specified buffer is too small, a new buffer is allocated,
 *     and the query is attempted again
 * RETURNS:
 *     Status of operation
 * TODO:
 *     Is there any way to use the buffer provided by the protocol?
 */
{
  NDIS_STATUS NdisStatus;
  ULONG BytesNeeded;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  /* call the miniport's queryinfo handler */
  NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.QueryInformationHandler)(
      Adapter->NdisMiniportBlock.MiniportAdapterContext,
      Oid,
      Buffer,
      Size,
      BytesWritten,
      &BytesNeeded);

  /* FIXME: Wait in pending case! */

  /* XXX is status_pending part of success macro? */
  if ((NT_SUCCESS(NdisStatus)) || (NdisStatus == NDIS_STATUS_PENDING))
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Miniport returned status (0x%X).\n", NdisStatus));
      return NdisStatus;
    }

  return NdisStatus;
}


NDIS_STATUS
FASTCALL
MiniQueueWorkItem(
    PLOGICAL_ADAPTER     Adapter,
    NDIS_WORK_ITEM_TYPE  WorkItemType,
    PVOID                WorkItemContext)
/*
 * FUNCTION: Queues a work item for execution at a later time
 * ARGUMENTS:
 *     Adapter         = Pointer to the logical adapter object to queue work item on
 *     WorkItemType    = Type of work item to queue
 *     WorkItemContext = Pointer to context information for work item
 * NOTES:
 *     Adapter lock must be held when called
 * RETURNS:
 *     Status of operation
 */
{
    PNDIS_MINIPORT_WORK_ITEM Item;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
    
    ASSERT(Adapter);
    ASSERT(KeGetCurrentIrql() >= DISPATCH_LEVEL);
    
    Item = ExAllocatePool(NonPagedPool, sizeof(NDIS_MINIPORT_WORK_ITEM));
    if (Item == NULL)
    {
        NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NDIS_STATUS_RESOURCES;
    }
    
    Item->WorkItemType    = WorkItemType;
    Item->WorkItemContext = WorkItemContext;
    
    /* safe due to adapter lock held */
    Item->Link.Next = NULL;
    if (!Adapter->WorkQueueHead)
    {
        Adapter->WorkQueueHead = Item;
        Adapter->WorkQueueTail = Item;
    }
    else
    {
        Adapter->WorkQueueTail->Link.Next = (PSINGLE_LIST_ENTRY)Item;
        Adapter->WorkQueueTail = Item;
    }
    
    KeInsertQueueDpc(&Adapter->NdisMiniportBlock.DeferredDpc, NULL, NULL);
    
    return NDIS_STATUS_SUCCESS;
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
    PNDIS_MINIPORT_WORK_ITEM Item;
    
    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));
    
    Item = Adapter->WorkQueueHead;
    
    if (Item)
    {
        /* safe due to adapter lock held */
        Adapter->WorkQueueHead = (PNDIS_MINIPORT_WORK_ITEM)Item->Link.Next;
        
        if (Item == Adapter->WorkQueueTail)
            Adapter->WorkQueueTail = NULL;
        
        *WorkItemType    = Item->WorkItemType;
        *WorkItemContext = Item->WorkItemContext;
        
        ExFreePool(Item);
        
        return NDIS_STATUS_SUCCESS;
    }
    
    return NDIS_STATUS_FAILURE;
}


NDIS_STATUS
MiniDoRequest(
    PNDIS_MINIPORT_BLOCK Adapter,
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
    NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));
    
    Adapter->MediaRequest = NdisRequest;
    
    switch (NdisRequest->RequestType)
    {
    case NdisRequestQueryInformation:
        return (*Adapter->DriverHandle->MiniportCharacteristics.QueryInformationHandler)(
            Adapter->MiniportAdapterContext,
            NdisRequest->DATA.QUERY_INFORMATION.Oid,
            NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
            NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
            (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
            (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded);
        break;
        
    case NdisRequestSetInformation:
        return (*Adapter->DriverHandle->MiniportCharacteristics.SetInformationHandler)(
            Adapter->MiniportAdapterContext,
            NdisRequest->DATA.SET_INFORMATION.Oid,
            NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
            NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
            (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
            (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded);
        break;
        
    default:
        return NDIS_STATUS_FAILURE;
    }
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
    PNDIS_MINIPORT_BLOCK MiniportBlock =
	(PNDIS_MINIPORT_BLOCK)MiniportAdapterHandle;
    ASSERT(MiniportBlock);
    if( MiniportBlock->QueryCompleteHandler )
	(MiniportBlock->QueryCompleteHandler)(MiniportAdapterHandle, Status);
}


VOID NTAPI MiniportDpc(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2)
/*
 * FUNCTION: Deferred routine to handle serialization
 * ARGUMENTS:
 *     Dpc             = Pointer to DPC object
 *     DeferredContext = Pointer to context information (LOGICAL_ADAPTER)
 *     SystemArgument1 = Unused
 *     SystemArgument2 = Unused
 */
{
  NDIS_STATUS NdisStatus;
  PVOID WorkItemContext;
  NDIS_WORK_ITEM_TYPE WorkItemType;
  PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(DeferredContext);

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  NdisStatus = 
      MiniDequeueWorkItem
      (Adapter, &WorkItemType, &WorkItemContext);

  if (NdisStatus == NDIS_STATUS_SUCCESS)
    {
      switch (WorkItemType)
        {
          case NdisWorkItemSend:
            /*
             * called by ProSend when protocols want to send packets to the miniport
             */
#ifdef DBG
            MiniDisplayPacket((PNDIS_PACKET)WorkItemContext);
#endif
            if(Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)
              {
                NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's SendPackets handler\n"));

                /*
                 * XXX assumes single-packet - prolly OK since we'll call something
                 * different on multi-packet sends
                 */
                (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendPacketsHandler)(
                    Adapter->NdisMiniportBlock.MiniportAdapterContext, (PPNDIS_PACKET)&WorkItemContext, 1);
		NdisStatus =
		    NDIS_GET_PACKET_STATUS((PNDIS_PACKET)WorkItemContext);

                NDIS_DbgPrint(MAX_TRACE, ("back from miniport's SendPackets handler\n"));
              }
            else
              {
                NDIS_DbgPrint(MAX_TRACE, ("Calling miniport's Send handler\n"));

                NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.SendHandler)(
                    Adapter->NdisMiniportBlock.MiniportAdapterContext, (PNDIS_PACKET)WorkItemContext, 0);

                NDIS_DbgPrint(MAX_TRACE, ("back from miniport's Send handler\n"));
              }
	    if( NdisStatus != NDIS_STATUS_PENDING ) {
		NdisMSendComplete
		    ( Adapter, (PNDIS_PACKET)WorkItemContext, NdisStatus );
		Adapter->MiniportBusy = FALSE;
	    }
            break;

          case NdisWorkItemSendLoopback:
            /*
             * called by ProSend when protocols want to send loopback packets
             */
            /* XXX atm ProIndicatePacket sends a packet up via the loopback adapter only */
            NdisStatus = ProIndicatePacket(Adapter, (PNDIS_PACKET)WorkItemContext);
            MiniSendComplete((NDIS_HANDLE)Adapter, (PNDIS_PACKET)WorkItemContext, NdisStatus);
            break;

          case NdisWorkItemReturnPackets:
            break;

          case NdisWorkItemResetRequested:
            break;

          case NdisWorkItemResetInProgress:
            break;

          case NdisWorkItemMiniportCallback:
            break;

          case NdisWorkItemRequest:
            NdisStatus = MiniDoRequest(&Adapter->NdisMiniportBlock, (PNDIS_REQUEST)WorkItemContext);

            if (NdisStatus == NDIS_STATUS_PENDING)
              break;

            switch (((PNDIS_REQUEST)WorkItemContext)->RequestType)
              {
                case NdisRequestQueryInformation:
		  NdisMQueryInformationComplete((NDIS_HANDLE)Adapter, NdisStatus);
                  MiniRequestComplete( &Adapter->NdisMiniportBlock, (PNDIS_REQUEST)WorkItemContext, NdisStatus );
                  break;

                case NdisRequestSetInformation:
                  NdisMSetInformationComplete((NDIS_HANDLE)Adapter, NdisStatus);
                  MiniRequestComplete( &Adapter->NdisMiniportBlock, (PNDIS_REQUEST)WorkItemContext, NdisStatus );
                  break;

                default:
                  NDIS_DbgPrint(MIN_TRACE, ("Unknown NDIS request type.\n"));
                  break;
              }
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
    UNIMPLEMENTED
}


VOID
NTAPI
MiniStatusComplete(
    IN NDIS_HANDLE  MiniportAdapterHandle)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMCloseLog(
    IN  NDIS_HANDLE LogHandle)
{
    UNIMPLEMENTED
}


/*
 * @unimplemented
 */
NDIS_STATUS
EXPORT
NdisMCreateLog(
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  UINT            Size,
    OUT PNDIS_HANDLE    LogHandle)
{
    UNIMPLEMENTED

  return NDIS_STATUS_FAILURE;
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

  if(Adapter->BugcheckContext->ShutdownHandler)
    KeDeregisterBugCheckCallback(Adapter->BugcheckContext->CallbackRecord);
}


/*
 * @unimplemented
 */
VOID
EXPORT
NdisMFlushLog(
    IN  NDIS_HANDLE LogHandle)
{
    UNIMPLEMENTED
}

/*
 * @unimplemented
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
    UNIMPLEMENTED
}

/*
 * @unimplemented
 */
#undef NdisMIndicateStatusComplete
VOID
EXPORT
NdisMIndicateStatusComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle)
{
    UNIMPLEMENTED
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
  __asm__ ("int $3\n");
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
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return;
    }

  RegistryPath->Length = ((PUNICODE_STRING)SystemSpecific2)->Length;
  RegistryPath->MaximumLength = RegistryPath->Length + sizeof(WCHAR);	/* room for 0-term */

  RegistryBuffer = ExAllocatePool(PagedPool, RegistryPath->MaximumLength);
  if(!RegistryBuffer)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
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
 *     - FIXME - memory leak below
 */
{
  PLOGICAL_ADAPTER            Adapter = (PLOGICAL_ADAPTER)MiniportHandle;
  PMINIPORT_BUGCHECK_CONTEXT  BugcheckContext = Adapter->BugcheckContext;

  NDIS_DbgPrint(DEBUG_MINIPORT, ("Called.\n"));

  if(BugcheckContext)
    return;

  BugcheckContext = ExAllocatePool(NonPagedPool, sizeof(MINIPORT_BUGCHECK_CONTEXT));
  if(!BugcheckContext)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return;
    }

  BugcheckContext->ShutdownHandler = ShutdownHandler;
  BugcheckContext->DriverContext = ShutdownContext;

  /* not sure if this needs to be initialized or not... oh well, it's a leak. */
  BugcheckContext->CallbackRecord = ExAllocatePool(NonPagedPool, sizeof(KBUGCHECK_CALLBACK_RECORD));

  KeRegisterBugCheckCallback(BugcheckContext->CallbackRecord, NdisIBugcheckCallback,
      BugcheckContext, sizeof(BugcheckContext), (PUCHAR)"Ndis Miniport");
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

#ifdef DBG
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

  NDIS_DbgPrint(DEBUG_MINIPORT, ("CurLookaheadLength (0x%X).\n", Adapter->NdisMiniportBlock.CurrentLookahead));

  if (Adapter->NdisMiniportBlock.MaximumLookahead != 0)
    {
      Adapter->LookaheadLength = Adapter->NdisMiniportBlock.MaximumLookahead + Adapter->MediumHeaderSize;
      Adapter->LookaheadBuffer = ExAllocatePool(NonPagedPool, Adapter->LookaheadLength);

      if (!Adapter->LookaheadBuffer)
        return NDIS_STATUS_RESOURCES;
    }

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
/* FIXME - KIRQL OldIrql; */

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
      return Status;
    }

  NDIS_DbgPrint(MAX_TRACE, ("opened device reg key\n"));

  WrapperContext.DeviceObject = Adapter->NdisMiniportBlock.DeviceObject;

  /*
   * Store the adapter resources used by HW routines such as
   * NdisMQueryAdapterResources.
   */

  if (Stack->Parameters.StartDevice.AllocatedResources != NULL &&
      Stack->Parameters.StartDevice.AllocatedResourcesTranslated != NULL)
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
	  ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
          return STATUS_INSUFFICIENT_RESOURCES;
        }

      Adapter->NdisMiniportBlock.AllocatedResourcesTranslated =
        ExAllocatePool(PagedPool, ResourceListSize);
      if (Adapter->NdisMiniportBlock.AllocatedResourcesTranslated == NULL)
        {
          ExFreePool(Adapter->NdisMiniportBlock.AllocatedResources);
          Adapter->NdisMiniportBlock.AllocatedResources = NULL;
	  ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
          return STATUS_INSUFFICIENT_RESOURCES;
        }

      RtlCopyMemory(Adapter->NdisMiniportBlock.AllocatedResources,
                    Stack->Parameters.StartDevice.AllocatedResources,
                    ResourceListSize);

      RtlCopyMemory(Adapter->NdisMiniportBlock.AllocatedResourcesTranslated,
                    Stack->Parameters.StartDevice.AllocatedResourcesTranslated,
                    ResourceListSize);
    }

  /*
   * Store the Bus Type, Bus Number and Slot information. It's used by
   * the hardware routines then.
   */

  NdisOpenConfiguration(&NdisStatus, &ConfigHandle, (NDIS_HANDLE)&WrapperContext);

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

  NdisCloseConfiguration(ConfigHandle);

  /*
   * Call MiniportInitialize.
   */

  NDIS_DbgPrint(MID_TRACE, ("calling MiniportInitialize\n"));
  NdisStatus = (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.InitializeHandler)(
    &OpenErrorStatus, &SelectedMediumIndex, &MediaArray[0],
    MEDIA_ARRAY_SIZE, Adapter, (NDIS_HANDLE)&WrapperContext);

  ZwClose(WrapperContext.RegistryHandle);

  if (NdisStatus != NDIS_STATUS_SUCCESS ||
      SelectedMediumIndex >= MEDIA_ARRAY_SIZE)
    {
      NDIS_DbgPrint(MIN_TRACE, ("MiniportInitialize() failed for an adapter.\n"));
      ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
      return (NTSTATUS)NdisStatus;
    }

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
            Success = EthCreateFilter(32, /* FIXME: Query this from miniport. */
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
/* FIXME - KeReleaseSpinLock(&Adapter->NdisMiniportBlock.Lock, OldIrql); */
	ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
        return STATUS_UNSUCCESSFUL;
    }

  if (!Success || NdisStatus != NDIS_STATUS_SUCCESS)
    {
      NDIS_DbgPrint(MAX_TRACE, ("couldn't create filter (%x)\n", NdisStatus));
      if (Adapter->LookaheadBuffer)
        {
          ExFreePool(Adapter->LookaheadBuffer);
          Adapter->LookaheadBuffer = NULL;
        }
      ExInterlockedRemoveEntryList( &Adapter->ListEntry, &AdapterListLock );
      return (NTSTATUS)NdisStatus;
    }

  Adapter->NdisMiniportBlock.OldPnPDeviceState = Adapter->NdisMiniportBlock.PnPDeviceState;
  Adapter->NdisMiniportBlock.PnPDeviceState = NdisPnPDeviceStarted;

  /* Put adapter in adapter list for this miniport */
  ExInterlockedInsertTailList(&Adapter->NdisMiniportBlock.DriverHandle->DeviceList, &Adapter->MiniportListEntry, &Adapter->NdisMiniportBlock.DriverHandle->Lock);

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
  KIRQL OldIrql;

  /* Remove adapter from adapter list for this miniport */
  KeAcquireSpinLock(&Adapter->NdisMiniportBlock.DriverHandle->Lock, &OldIrql);
  RemoveEntryList(&Adapter->MiniportListEntry);
  KeReleaseSpinLock(&Adapter->NdisMiniportBlock.DriverHandle->Lock, OldIrql);

  /* Remove adapter from global adapter list */
  KeAcquireSpinLock(&AdapterListLock, &OldIrql);
  RemoveEntryList(&Adapter->ListEntry);
  KeReleaseSpinLock(&AdapterListLock, OldIrql);

  (*Adapter->NdisMiniportBlock.DriverHandle->MiniportCharacteristics.HaltHandler)(Adapter);

  if (Adapter->LookaheadBuffer)
    {
      ExFreePool(Adapter->LookaheadBuffer);
      Adapter->LookaheadBuffer = NULL;
    }
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

  Adapter->NdisMiniportBlock.OldPnPDeviceState = Adapter->NdisMiniportBlock.PnPDeviceState;
  Adapter->NdisMiniportBlock.PnPDeviceState = NdisPnPDeviceStopped;

  return STATUS_SUCCESS;
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
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

      case IRP_MN_STOP_DEVICE:
        Status = NdisIForwardIrpAndWait(Adapter, Irp);
        if (NT_SUCCESS(Status) && NT_SUCCESS(Irp->IoStatus.Status))
          {
            Status = NdisIPnPStopDevice(DeviceObject, Irp);
          }
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

      case IRP_MN_QUERY_DEVICE_RELATIONS:
        Status = STATUS_NOT_SUPPORTED;
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

      default:
        IoSkipCurrentIrpStackLocation(Irp);
        Status = IoCallDriver(Adapter->NdisMiniportBlock.NextDeviceObject, Irp);
        break;
    }

  return Status;
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

  MiniportPtr = IoGetDriverObjectExtension(DriverObject, (PVOID)TAG('D','I','M','N'));
  if (MiniportPtr == NULL)
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Can't get driver object extension.\n"));
      return STATUS_UNSUCCESSFUL;
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
  if (Status != STATUS_BUFFER_TOO_SMALL)
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Can't get miniport driver key length.\n"));
      return Status;
    }

  LinkageKeyBuffer = ExAllocatePool(PagedPool, DriverKeyLength +
                                    sizeof(ClassKeyName) + sizeof(LinkageKeyName));
  if (LinkageKeyBuffer == NULL)
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Can't allocate memory for driver key name.\n"));
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  Status = IoGetDeviceProperty(PhysicalDeviceObject, DevicePropertyDriverKeyName,
                               DriverKeyLength, LinkageKeyBuffer +
                               (sizeof(ClassKeyName) / sizeof(WCHAR)),
                               &DriverKeyLength);
  if (!NT_SUCCESS(Status))
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Can't get miniport driver key.\n"));
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
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Can't get miniport device name. (%x)\n", Status));
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
  Adapter->NdisMiniportBlock.DriverHandle = Miniport;

  Adapter->NdisMiniportBlock.MiniportName = ExportName;

  Adapter->NdisMiniportBlock.DeviceObject = DeviceObject;
  Adapter->NdisMiniportBlock.PhysicalDeviceObject = PhysicalDeviceObject;
  Adapter->NdisMiniportBlock.NextDeviceObject =
    IoAttachDeviceToDeviceStack(Adapter->NdisMiniportBlock.DeviceObject,
                                PhysicalDeviceObject);

  Adapter->NdisMiniportBlock.OldPnPDeviceState = 0;
  Adapter->NdisMiniportBlock.PnPDeviceState = NdisPnPDeviceAdded;

  KeInitializeDpc(&Adapter->NdisMiniportBlock.DeferredDpc, MiniportDpc, (PVOID)Adapter);

  DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

  return STATUS_SUCCESS;
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
        MinSize = sizeof(NDIS50_MINIPORT_CHARACTERISTICS);
        break;

      default:
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics version.\n"));
        return NDIS_STATUS_BAD_VERSION;
    }

  if (CharacteristicsLength < MinSize)
    {
        NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
        return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

  /* Check if mandatory MiniportXxx functions are specified */
  if ((!MiniportCharacteristics->HaltHandler) ||
      (!MiniportCharacteristics->InitializeHandler)||
      (!MiniportCharacteristics->QueryInformationHandler) ||
      (!MiniportCharacteristics->ResetHandler) ||
      (!MiniportCharacteristics->SetInformationHandler))
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
      return NDIS_STATUS_BAD_CHARACTERISTICS;
    }

  if (MiniportCharacteristics->MajorNdisVersion == 0x03)
    {
      if (!MiniportCharacteristics->SendHandler)
        {
          NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
          return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    }
  else if (MiniportCharacteristics->MajorNdisVersion >= 0x04)
    {
      /* NDIS 4.0+ */
      if ((!MiniportCharacteristics->SendHandler) &&
          (!MiniportCharacteristics->SendPacketsHandler))
        {
          NDIS_DbgPrint(DEBUG_MINIPORT, ("Bad miniport characteristics.\n"));
          return NDIS_STATUS_BAD_CHARACTERISTICS;
        }
    }

  /* TODO: verify NDIS5 and NDIS5.1 */

  RtlCopyMemory(&Miniport->MiniportCharacteristics, MiniportCharacteristics, MinSize);

  /*
   * NOTE: This is VERY unoptimal! Should we store the NDIS_M_DRIVER_BLOCK
   * structure in the driver extension or what?
   */

  Status = IoAllocateDriverObjectExtension(Miniport->DriverObject, (PVOID)TAG('D','I','M','N'),
                                           sizeof(PNDIS_M_DRIVER_BLOCK), (PVOID*)&MiniportPtr);
  if (!NT_SUCCESS(Status))
    {
      NDIS_DbgPrint(DEBUG_MINIPORT, ("Can't allocate driver object extension.\n"));
      return NDIS_STATUS_RESOURCES;
    }

  *MiniportPtr = Miniport;

  Miniport->DriverObject->MajorFunction[IRP_MJ_PNP] = NdisIDispatchPnp;
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
#undef NdisMSetInformationComplete
VOID
EXPORT
NdisMSetInformationComplete(
    IN  NDIS_HANDLE MiniportAdapterHandle,
    IN  NDIS_STATUS Status)
{
  (*((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->SetCompleteHandler)(MiniportAdapterHandle, Status);
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
  /* TODO: Take CheckForHandTimeInSeconds into account! */

  PLOGICAL_ADAPTER Adapter = GET_LOGICAL_ADAPTER(MiniportAdapterHandle);

  NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

  Adapter->NdisMiniportBlock.MiniportAdapterContext = MiniportAdapterContext;
  Adapter->NdisMiniportBlock.Flags = AttributeFlags;
  Adapter->NdisMiniportBlock.AdapterType = AdapterType;
  if (AttributeFlags & NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER)
    NDIS_DbgPrint(MAX_TRACE, ("Intermediate drivers not supported yet.\n"));
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
    UNIMPLEMENTED

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
  ExFreePool(Miniport);
}

/* EOF */

