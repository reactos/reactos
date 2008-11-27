/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/neighbor.c
 * PURPOSE:     Neighbor address cache
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

NEIGHBOR_CACHE_TABLE NeighborCache[NB_HASHMASK + 1];

VOID NBCompleteSend( PVOID Context,
		     PNDIS_PACKET NdisPacket,
		     NDIS_STATUS Status ) {
    PNEIGHBOR_PACKET Packet = (PNEIGHBOR_PACKET)Context;
    TI_DbgPrint(MID_TRACE, ("Called\n"));
    ASSERT_KM_POINTER(Packet);
    ASSERT_KM_POINTER(Packet->Complete);
    Packet->Complete( Packet->Context, Packet->Packet, Status );
    TI_DbgPrint(MID_TRACE, ("Completed\n"));
    PoolFreeBuffer( Packet );
    TI_DbgPrint(MID_TRACE, ("Freed\n"));
}

VOID NBSendPackets( PNEIGHBOR_CACHE_ENTRY NCE ) {
    PLIST_ENTRY PacketEntry;
    PNEIGHBOR_PACKET Packet;
    UINT HashValue;

    if(!(NCE->State & NUD_CONNECTED))
       return;

    HashValue  = *(PULONG)(&NCE->Address.Address);
    HashValue ^= HashValue >> 16;
    HashValue ^= HashValue >> 8;
    HashValue ^= HashValue >> 4;
    HashValue &= NB_HASHMASK;

    /* Send any waiting packets */
    PacketEntry = ExInterlockedRemoveHeadList(&NCE->PacketQueue,
                                              &NeighborCache[HashValue].Lock);
    if( PacketEntry != NULL ) {
	Packet = CONTAINING_RECORD( PacketEntry, NEIGHBOR_PACKET, Next );

	TI_DbgPrint
	    (MID_TRACE,
	     ("PacketEntry: %x, NdisPacket %x\n",
	      PacketEntry, Packet->Packet));

	PC(Packet->Packet)->DLComplete = NBCompleteSend;
	PC(Packet->Packet)->Context  = Packet;

	NCE->Interface->Transmit
	    ( NCE->Interface->Context,
	      Packet->Packet,
	      MaxLLHeaderSize,
	      NCE->LinkAddress,
	      LAN_PROTO_IPv4 );
    }
}

/* Must be called with table lock acquired */
VOID NBFlushPacketQueue( PNEIGHBOR_CACHE_ENTRY NCE,
			 BOOLEAN CallComplete,
			 NTSTATUS ErrorCode ) {
    PLIST_ENTRY PacketEntry;
    PNEIGHBOR_PACKET Packet;

    while( !IsListEmpty(&NCE->PacketQueue) ) {
        PacketEntry = RemoveHeadList(&NCE->PacketQueue);
	Packet = CONTAINING_RECORD
	    ( PacketEntry, NEIGHBOR_PACKET, Next );

        ASSERT_KM_POINTER(Packet);

	TI_DbgPrint
	    (MID_TRACE,
	     ("PacketEntry: %x, NdisPacket %x\n",
	      PacketEntry, Packet->Packet));

	if( CallComplete )
        {
            ASSERT_KM_POINTER(Packet->Complete);
	    Packet->Complete( Packet->Context,
			      Packet->Packet,
			      ErrorCode );
        }

	PoolFreeBuffer( Packet );
    }
}

VOID NCETimeout(
  PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Neighbor cache entry timeout handler
 * NOTES:
 *   The neighbor cache lock must be held
 */
{
    TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X).\n", NCE));
    TI_DbgPrint(DEBUG_NCACHE, ("NCE->State is (0x%X).\n", NCE->State));

    switch (NCE->State)
    {
    case NUD_INCOMPLETE:
        /* Retransmission timer expired */
        if (NCE->EventCount++ > MAX_MULTICAST_SOLICIT)
	{
            /* We have retransmitted too many times */

            /* Calling IPSendComplete with cache lock held is not
	       a great thing to do. We don't get here very often
	       so maybe it's not that big a problem */

            /* Flush packet queue */
	    NBFlushPacketQueue( NCE, TRUE, NDIS_STATUS_REQUEST_ABORTED );
            NCE->EventCount = 0;
	}
        else
	{
            /* Retransmit request */
            NBSendSolicit(NCE);
	}
        break;

    case NUD_DELAY:
        /* FIXME: Delayed state */
        TI_DbgPrint(DEBUG_NCACHE, ("NCE delay state.\n"));
        break;

    case NUD_PROBE:
        /* FIXME: Probe state */
        TI_DbgPrint(DEBUG_NCACHE, ("NCE probe state.\n"));
        break;

    default:
        /* Should not happen since the event timer is not used in the other states */
        TI_DbgPrint(MIN_TRACE, ("Invalid NCE state (%d).\n", NCE->State));
        break;
    }
}


VOID NBTimeout(VOID)
/*
 * FUNCTION: Neighbor address cache timeout handler
 * NOTES:
 *     This routine is called by IPTimeout to remove outdated cache
 *     entries.
 */
{
    UINT i;
    KIRQL OldIrql;
    PNEIGHBOR_CACHE_ENTRY NCE;

    for (i = 0; i <= NB_HASHMASK; i++) {
        TcpipAcquireSpinLock(&NeighborCache[i].Lock, &OldIrql);

        for (NCE = NeighborCache[i].Cache;
            NCE != NULL; NCE = NCE->Next) {
            /* Check if event timer is running */
            if (NCE->EventTimer > 0)  {
                NCE->EventTimer--;
                if (NCE->EventTimer == 0) {
                    /* Call timeout handler for NCE */
                    NCETimeout(NCE);
                }
            }
        }

        TcpipReleaseSpinLock(&NeighborCache[i].Lock, OldIrql);
    }
}

VOID NBStartup(VOID)
/*
 * FUNCTION: Starts the neighbor cache
 */
{
    UINT i;

    TI_DbgPrint(DEBUG_NCACHE, ("Called.\n"));

    for (i = 0; i <= NB_HASHMASK; i++) {
	NeighborCache[i].Cache = NULL;
	TcpipInitializeSpinLock(&NeighborCache[i].Lock);
    }
}

VOID NBShutdown(VOID)
/*
 * FUNCTION: Shuts down the neighbor cache
 */
{
  PNEIGHBOR_CACHE_ENTRY NextNCE;
  PNEIGHBOR_CACHE_ENTRY CurNCE;
  KIRQL OldIrql;
  UINT i;

  TI_DbgPrint(DEBUG_NCACHE, ("Called.\n"));

  /* Remove possible entries from the cache */
  for (i = 0; i <= NB_HASHMASK; i++)
    {
      TcpipAcquireSpinLock(&NeighborCache[i].Lock, &OldIrql);

      CurNCE = NeighborCache[i].Cache;
      while (CurNCE) {
          NextNCE = CurNCE->Next;

          /* Flush wait queue */
	  NBFlushPacketQueue( CurNCE, FALSE, STATUS_SUCCESS );

	  CurNCE = NextNCE;
      }

    NeighborCache[i].Cache = NULL;

    TcpipReleaseSpinLock(&NeighborCache[i].Lock, OldIrql);
  }

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));
}

VOID NBSendSolicit(PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Sends a neighbor solicitation message
 * ARGUMENTS:
 *   NCE = Pointer to NCE of neighbor to solicit
 * NOTES:
 *   May be called with lock held on NCE's table
 */
{
    TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X).\n", NCE));

    if (NCE->State == NUD_INCOMPLETE)
    {
	/* This is the first solicitation of this neighbor. Broadcast
	   a request for the neighbor */

	TI_DbgPrint(MID_TRACE,("NCE: %x\n", NCE));

	ARPTransmit(&NCE->Address, NCE->Interface);
    } else {
	/* FIXME: Unicast solicitation since we have a cached address */
	TI_DbgPrint(MIN_TRACE, ("Uninplemented unicast solicitation.\n"));
    }
}

PNEIGHBOR_CACHE_ENTRY NBAddNeighbor(
  PIP_INTERFACE Interface,
  PIP_ADDRESS Address,
  PVOID LinkAddress,
  UINT LinkAddressLength,
  UCHAR State)
/*
 * FUNCTION: Adds a neighbor to the neighbor cache
 * ARGUMENTS:
 *   Interface         = Pointer to interface
 *   Address           = Pointer to IP address
 *   LinkAddress       = Pointer to link address (may be NULL)
 *   LinkAddressLength = Length of link address
 *   State             = State of NCE
 * RETURNS:
 *   Pointer to NCE, NULL there is not enough free resources
 * NOTES:
 *   The NCE if referenced for the caller if created. The NCE retains
 *   a reference to the IP address if it is created, the caller is
 *   responsible for providing this reference
 */
{
  PNEIGHBOR_CACHE_ENTRY NCE;
  ULONG HashValue;
  KIRQL OldIrql;

  TI_DbgPrint
      (DEBUG_NCACHE,
       ("Called. Interface (0x%X)  Address (0x%X)  "
	"LinkAddress (0x%X)  LinkAddressLength (%d)  State (0x%X)\n",
	Interface, Address, LinkAddress, LinkAddressLength, State));

  NCE = ExAllocatePool
      (NonPagedPool, sizeof(NEIGHBOR_CACHE_ENTRY) + LinkAddressLength);
  if (NCE == NULL)
    {
      TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
      return NULL;
    }

  INIT_TAG(NCE, TAG('N','C','E',' '));

  NCE->Interface = Interface;
  NCE->Address = *Address;
  NCE->LinkAddressLength = LinkAddressLength;
  NCE->LinkAddress = (PVOID)&NCE[1];
  if( LinkAddress )
      RtlCopyMemory(NCE->LinkAddress, LinkAddress, LinkAddressLength);
  else
      memset(NCE->LinkAddress, 0xff, LinkAddressLength);
  NCE->State = State;
  NCE->EventTimer = 0; /* Not in use */
  InitializeListHead( &NCE->PacketQueue );

  TI_DbgPrint(MID_TRACE,("NCE: %x\n", NCE));

  HashValue = *(PULONG)&Address->Address;
  HashValue ^= HashValue >> 16;
  HashValue ^= HashValue >> 8;
  HashValue ^= HashValue >> 4;
  HashValue &= NB_HASHMASK;

  NCE->Table = &NeighborCache[HashValue];

  TcpipAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

  NCE->Next = NeighborCache[HashValue].Cache;
  NeighborCache[HashValue].Cache = NCE;

  TcpipReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);

  return NCE;
}

VOID NBUpdateNeighbor(
  PNEIGHBOR_CACHE_ENTRY NCE,
  PVOID LinkAddress,
  UCHAR State)
/*
 * FUNCTION: Update link address information in NCE
 * ARGUMENTS:
 *   NCE         = Pointer to NCE to update
 *   LinkAddress = Pointer to link address
 *   State       = State of NCE
 * NOTES:
 *   The link address and state is updated. Any waiting packets are sent
 */
{
    KIRQL OldIrql;
    UINT HashValue;

    TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X)  LinkAddress (0x%X)  State (0x%X).\n", NCE, LinkAddress, State));

    HashValue  = *(PULONG)(&NCE->Address.Address);
    HashValue ^= HashValue >> 16;
    HashValue ^= HashValue >> 8;
    HashValue ^= HashValue >> 4;
    HashValue &= NB_HASHMASK;

    TcpipAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

    RtlCopyMemory(NCE->LinkAddress, LinkAddress, NCE->LinkAddressLength);
    NCE->State = State;

    TcpipReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);

    if( NCE->State & NUD_CONNECTED )
	NBSendPackets( NCE );
}

PNEIGHBOR_CACHE_ENTRY NBLocateNeighbor(
  PIP_ADDRESS Address)
/*
 * FUNCTION: Locates a neighbor in the neighbor cache
 * ARGUMENTS:
 *   Address = Pointer to IP address
 * RETURNS:
 *   Pointer to NCE, NULL if not found
 * NOTES:
 *   If the NCE is found, it is referenced. The caller is
 *   responsible for dereferencing it again after use
 */
{
  PNEIGHBOR_CACHE_ENTRY NCE;
  UINT HashValue;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. Address (0x%X).\n", Address));

  HashValue = *(PULONG)&Address->Address;
  HashValue ^= HashValue >> 16;
  HashValue ^= HashValue >> 8;
  HashValue ^= HashValue >> 4;
  HashValue &= NB_HASHMASK;

  TcpipAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

  NCE = NeighborCache[HashValue].Cache;

  while ((NCE) && (!AddrIsEqual(Address, &NCE->Address)))
    {
      NCE = NCE->Next;
    }

  TcpipReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);

  TI_DbgPrint(MAX_TRACE, ("Leaving.\n"));

  return NCE;
}

PNEIGHBOR_CACHE_ENTRY NBFindOrCreateNeighbor(
  PIP_INTERFACE Interface,
  PIP_ADDRESS Address)
/*
 * FUNCTION: Tries to find a neighbor and if unsuccesful, creates a new NCE
 * ARGUMENTS:
 *   Interface = Pointer to interface to use (in case NCE is not found)
 *   Address   = Pointer to IP address
 * RETURNS:
 *   Pointer to NCE, NULL if there is not enough free resources
 * NOTES:
 *   The NCE is referenced if found or created. The caller is
 *   responsible for dereferencing it again after use
 */
{
  PNEIGHBOR_CACHE_ENTRY NCE;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. Interface (0x%X)  Address (0x%X).\n", Interface, Address));

  NCE = NBLocateNeighbor(Address);
  if (NCE == NULL)
    {
        TI_DbgPrint(MID_TRACE,("BCAST: %s\n", A2S(&Interface->Broadcast)));
        if( AddrIsEqual(Address, &Interface->Broadcast) ) {
            TI_DbgPrint(MID_TRACE,("Packet targeted at broadcast addr\n"));
            NCE = NBAddNeighbor(Interface, Address, NULL,
                                Interface->AddressLength, NUD_CONNECTED);
            if (!NCE) return NULL;
            NCE->EventTimer = 0;
            NCE->EventCount = 0;
        } else {
            NCE = NBAddNeighbor(Interface, Address, NULL,
                                Interface->AddressLength, NUD_INCOMPLETE);
            if (!NCE) return NULL;
            NCE->EventTimer = 1;
            NCE->EventCount = 0;
        }
    }

  return NCE;
}

BOOLEAN NBQueuePacket(
  PNEIGHBOR_CACHE_ENTRY NCE,
  PNDIS_PACKET NdisPacket,
  PNEIGHBOR_PACKET_COMPLETE PacketComplete,
  PVOID PacketContext)
/*
 * FUNCTION: Queues a packet on an NCE for later transmission
 * ARGUMENTS:
 *   NCE        = Pointer to NCE to queue packet on
 *   NdisPacket = Pointer to NDIS packet to queue
 * RETURNS:
 *   TRUE if the packet was successfully queued, FALSE if not
 */
{
  KIRQL OldIrql;
  PNEIGHBOR_PACKET Packet;
  UINT HashValue;

  TI_DbgPrint
      (DEBUG_NCACHE,
       ("Called. NCE (0x%X)  NdisPacket (0x%X).\n", NCE, NdisPacket));

  Packet = PoolAllocateBuffer( sizeof(NEIGHBOR_PACKET) );
  if( !Packet ) return FALSE;

  /* FIXME: Should we limit the number of queued packets? */

  HashValue  = *(PULONG)(&NCE->Address.Address);
  HashValue ^= HashValue >> 16;
  HashValue ^= HashValue >> 8;
  HashValue ^= HashValue >> 4;
  HashValue &= NB_HASHMASK;

  TcpipAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

  Packet->Complete = PacketComplete;
  Packet->Context = PacketContext;
  Packet->Packet = NdisPacket;
  InsertTailList( &NCE->PacketQueue, &Packet->Next );

  TcpipReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);

  if( NCE->State & NUD_CONNECTED )
      NBSendPackets( NCE );

  return TRUE;
}


VOID NBRemoveNeighbor(
  PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Removes a neighbor from the neighbor cache
 * ARGUMENTS:
 *   NCE = Pointer to NCE to remove from cache
 * NOTES:
 *   The NCE must be in a safe state
 */
{
  PNEIGHBOR_CACHE_ENTRY *PrevNCE;
  PNEIGHBOR_CACHE_ENTRY CurNCE;
  ULONG HashValue;
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_NCACHE, ("Called. NCE (0x%X).\n", NCE));

  HashValue  = *(PULONG)(&NCE->Address.Address);
  HashValue ^= HashValue >> 16;
  HashValue ^= HashValue >> 8;
  HashValue ^= HashValue >> 4;
  HashValue &= NB_HASHMASK;

  TcpipAcquireSpinLock(&NeighborCache[HashValue].Lock, &OldIrql);

  /* Search the list and remove the NCE from the list if found */
  for (PrevNCE = &NeighborCache[HashValue].Cache;
    (CurNCE = *PrevNCE) != NULL;
    PrevNCE = &CurNCE->Next)
    {
      if (CurNCE == NCE)
        {
          /* Found it, now unlink it from the list */
          *PrevNCE = CurNCE->Next;

	  NBFlushPacketQueue( CurNCE, TRUE, NDIS_STATUS_REQUEST_ABORTED );
          ExFreePool(CurNCE);

	  break;
        }
    }

  TcpipReleaseSpinLock(&NeighborCache[HashValue].Lock, OldIrql);
}

ULONG NBCopyNeighbors
(PIP_INTERFACE Interface,
 PIPARP_ENTRY ArpTable)
{
  PNEIGHBOR_CACHE_ENTRY CurNCE;
  KIRQL OldIrql;
  UINT Size = 0, i;

  for (i = 0; i <= NB_HASHMASK; i++) {
      TcpipAcquireSpinLock(&NeighborCache[i].Lock, &OldIrql);
      for( CurNCE = NeighborCache[i].Cache;
	   CurNCE;
	   CurNCE = CurNCE->Next ) {
	  if( CurNCE->Interface == Interface ) {
	      if( ArpTable ) {
		  ArpTable[Size].Index = Interface->Index;
		  ArpTable[Size].AddrSize = CurNCE->LinkAddressLength;
		  RtlCopyMemory
		      (ArpTable[Size].PhysAddr, 
		       CurNCE->LinkAddress,
		       CurNCE->LinkAddressLength);
		  ArpTable[Size].LogAddr = CurNCE->Address.Address.IPv4Address;
		  if( CurNCE->State & NUD_PERMANENT )
		      ArpTable[Size].Type = ARP_ENTRY_STATIC;
		  else if( CurNCE->State & NUD_CONNECTED )
		      ArpTable[Size].Type = ARP_ENTRY_DYNAMIC;
		  else if( !(CurNCE->State & NUD_VALID) )
		      ArpTable[Size].Type = ARP_ENTRY_INVALID;
		  else
		      ArpTable[Size].Type = ARP_ENTRY_OTHER;
	      }
	      Size++;
	  }
      }
      TcpipReleaseSpinLock(&NeighborCache[i].Lock, OldIrql);
  }
  
  return Size;
}
