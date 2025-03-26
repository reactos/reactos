/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/receive.c
 * PURPOSE:     Internet Protocol receive routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:       The IP datagram reassembly algorithm is taken from
 *              from RFC 815
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

LIST_ENTRY ReassemblyListHead;
KSPIN_LOCK ReassemblyListLock;
NPAGED_LOOKASIDE_LIST IPDRList;
NPAGED_LOOKASIDE_LIST IPFragmentList;
NPAGED_LOOKASIDE_LIST IPHoleList;

PIPDATAGRAM_HOLE CreateHoleDescriptor(
  ULONG First,
  ULONG Last)
/*
 * FUNCTION: Returns a pointer to a IP datagram hole descriptor
 * ARGUMENTS:
 *     First = Offset of first octet of the hole
 *     Last  = Offset of last octet of the hole
 * RETURNS:
 *     Pointer to descriptor, NULL if there was not enough free
 *     resources
 */
{
	PIPDATAGRAM_HOLE Hole;

	TI_DbgPrint(DEBUG_IP, ("Called. First (%d)  Last (%d).\n", First, Last));

	Hole = ExAllocateFromNPagedLookasideList(&IPHoleList);
	if (!Hole) {
	    TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
	    return NULL;
	}

	Hole->First = First;
	Hole->Last  = Last;

	TI_DbgPrint(DEBUG_IP, ("Returning hole descriptor at (0x%X).\n", Hole));

	return Hole;
}


VOID FreeIPDR(
  PIPDATAGRAM_REASSEMBLY IPDR)
/*
 * FUNCTION: Frees an IP datagram reassembly structure
 * ARGUMENTS:
 *     IPDR = Pointer to IP datagram reassembly structure
 */
{
  PLIST_ENTRY CurrentEntry;
  PLIST_ENTRY NextEntry;
  PIPDATAGRAM_HOLE CurrentH;
  PIP_FRAGMENT CurrentF;

  TI_DbgPrint(DEBUG_IP, ("Freeing IP datagram reassembly descriptor (0x%X).\n", IPDR));

  /* Free all descriptors */
  CurrentEntry = IPDR->HoleListHead.Flink;
  while (CurrentEntry != &IPDR->HoleListHead) {
    NextEntry = CurrentEntry->Flink;
	  CurrentH = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_HOLE, ListEntry);
    /* Unlink it from the list */
    RemoveEntryList(CurrentEntry);

    TI_DbgPrint(DEBUG_IP, ("Freeing hole descriptor at (0x%X).\n", CurrentH));

    /* And free the hole descriptor */
    ExFreeToNPagedLookasideList(&IPHoleList, CurrentH);

    CurrentEntry = NextEntry;
  }

  /* Free all fragments */
  CurrentEntry = IPDR->FragmentListHead.Flink;
  while (CurrentEntry != &IPDR->FragmentListHead) {
    NextEntry = CurrentEntry->Flink;
	  CurrentF = CONTAINING_RECORD(CurrentEntry, IP_FRAGMENT, ListEntry);
    /* Unlink it from the list */
    RemoveEntryList(CurrentEntry);

    TI_DbgPrint(DEBUG_IP, ("Freeing fragment packet at (0x%X).\n", CurrentF->Packet));

    /* Free the fragment data buffer */
    if (CurrentF->ReturnPacket)
    {
        NdisReturnPackets(&CurrentF->Packet, 1);
    }
    else
    {
        FreeNdisPacket(CurrentF->Packet);
    }

    TI_DbgPrint(DEBUG_IP, ("Freeing fragment at (0x%X).\n", CurrentF));

    /* And free the fragment descriptor */
    ExFreeToNPagedLookasideList(&IPFragmentList, CurrentF);
    CurrentEntry = NextEntry;
  }

  if (IPDR->IPv4Header)
  {
      TI_DbgPrint(DEBUG_IP, ("Freeing IPDR header at (0x%X).\n", IPDR->IPv4Header));
      ExFreePoolWithTag(IPDR->IPv4Header, PACKET_BUFFER_TAG);
  }

  TI_DbgPrint(DEBUG_IP, ("Freeing IPDR data at (0x%X).\n", IPDR));

  ExFreeToNPagedLookasideList(&IPDRList, IPDR);
}


VOID RemoveIPDR(
  PIPDATAGRAM_REASSEMBLY IPDR)
/*
 * FUNCTION: Removes an IP datagram reassembly structure from the global list
 * ARGUMENTS:
 *     IPDR = Pointer to IP datagram reassembly structure
 */
{
  KIRQL OldIrql;

  TI_DbgPrint(DEBUG_IP, ("Removing IPDR at (0x%X).\n", IPDR));

  TcpipAcquireSpinLock(&ReassemblyListLock, &OldIrql);
  RemoveEntryList(&IPDR->ListEntry);
  TcpipReleaseSpinLock(&ReassemblyListLock, OldIrql);
}


PIPDATAGRAM_REASSEMBLY GetReassemblyInfo(
  PIP_PACKET IPPacket)
/*
 * FUNCTION: Returns a pointer to an IP datagram reassembly structure
 * ARGUMENTS:
 *     IPPacket = Pointer to IP packet
 * NOTES:
 *     A datagram is identified by four paramters, which are
 *     Source and destination address, protocol number and
 *     identification number
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry;
  PIPDATAGRAM_REASSEMBLY Current;
  PIPv4_HEADER Header = (PIPv4_HEADER)IPPacket->Header;

  TI_DbgPrint(DEBUG_IP, ("Searching for IPDR for IP packet at (0x%X).\n", IPPacket));

  TcpipAcquireSpinLock(&ReassemblyListLock, &OldIrql);

  /* FIXME: Assume IPv4 */

  CurrentEntry = ReassemblyListHead.Flink;
  while (CurrentEntry != &ReassemblyListHead) {
	  Current = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_REASSEMBLY, ListEntry);
    if (AddrIsEqual(&IPPacket->SrcAddr, &Current->SrcAddr) &&
      (Header->Id == Current->Id) &&
      (Header->Protocol == Current->Protocol) &&
      (AddrIsEqual(&IPPacket->DstAddr, &Current->DstAddr))) {
      TcpipReleaseSpinLock(&ReassemblyListLock, OldIrql);

      return Current;
    }
    CurrentEntry = CurrentEntry->Flink;
  }

  TcpipReleaseSpinLock(&ReassemblyListLock, OldIrql);

  return NULL;
}


BOOLEAN
ReassembleDatagram(
  PIP_PACKET             IPPacket,
  PIPDATAGRAM_REASSEMBLY IPDR)
/*
 * FUNCTION: Reassembles an IP datagram
 * ARGUMENTS:
 *     IPDR = Pointer to IP datagram reassembly structure
 * NOTES:
 *     This routine concatenates fragments into a complete IP datagram.
 *     The lock is held when this routine is called
 * RETURNS:
 *     Pointer to IP packet, NULL if there was not enough free resources
 * NOTES:
 *     At this point, header is expected to point to the IP header
 */
{
  PLIST_ENTRY CurrentEntry;
  PIP_FRAGMENT Fragment;
  PCHAR Data;

  PAGED_CODE();

  TI_DbgPrint(DEBUG_IP, ("Reassembling datagram from IPDR at (0x%X).\n", IPDR));
  TI_DbgPrint(DEBUG_IP, ("IPDR->HeaderSize = %d\n", IPDR->HeaderSize));
  TI_DbgPrint(DEBUG_IP, ("IPDR->DataSize = %d\n", IPDR->DataSize));

  IPPacket->TotalSize  = IPDR->HeaderSize + IPDR->DataSize;
  IPPacket->HeaderSize = IPDR->HeaderSize;

  RtlCopyMemory(&IPPacket->SrcAddr, &IPDR->SrcAddr, sizeof(IP_ADDRESS));
  RtlCopyMemory(&IPPacket->DstAddr, &IPDR->DstAddr, sizeof(IP_ADDRESS));

  /* Allocate space for full IP datagram */
  IPPacket->Header = ExAllocatePoolWithTag(NonPagedPool, IPPacket->TotalSize, PACKET_BUFFER_TAG);
  if (!IPPacket->Header) {
    TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
    (*IPPacket->Free)(IPPacket);
    return FALSE;
  }
  IPPacket->MappedHeader = FALSE;

  /* Copy the header into the buffer */
  RtlCopyMemory(IPPacket->Header, IPDR->IPv4Header, IPDR->HeaderSize);

  Data = (PVOID)((ULONG_PTR)IPPacket->Header + IPDR->HeaderSize);
  IPPacket->Data = Data;

  /* Copy data from all fragments into buffer */
  CurrentEntry = IPDR->FragmentListHead.Flink;
  while (CurrentEntry != &IPDR->FragmentListHead) {
    Fragment = CONTAINING_RECORD(CurrentEntry, IP_FRAGMENT, ListEntry);

    /* Copy fragment data into datagram buffer */
    CopyPacketToBuffer(Data + Fragment->Offset,
                       Fragment->Packet,
                       Fragment->PacketOffset,
                       Fragment->Size);

    CurrentEntry = CurrentEntry->Flink;
  }

  return TRUE;
}


static inline VOID Cleanup(
  PKSPIN_LOCK Lock,
  KIRQL OldIrql,
  PIPDATAGRAM_REASSEMBLY IPDR)
/*
 * FUNCTION: Performs cleaning operations on errors
 * ARGUMENTS:
 *     Lock     = Pointer to spin lock to be released
 *     OldIrql  = Value of IRQL when spin lock was acquired
 *     IPDR     = Pointer to IP datagram reassembly structure to free
 *     Buffer   = Optional pointer to a buffer to free
 */
{
  TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));

  TcpipReleaseSpinLock(Lock, OldIrql);
  RemoveIPDR(IPDR);
  FreeIPDR(IPDR);
}


VOID ProcessFragment(
  PIP_INTERFACE IF,
  PIP_PACKET IPPacket)
/*
 * FUNCTION: Processes an IP datagram or fragment
 * ARGUMENTS:
 *     IF       = Pointer to IP interface packet was receive on
 *     IPPacket = Pointer to IP packet
 * NOTES:
 *     This routine reassembles fragments and, if a whole datagram can
 *     be assembled, passes the datagram on to the IP protocol dispatcher
 */
{
  KIRQL OldIrql;
  PIPDATAGRAM_REASSEMBLY IPDR;
  PLIST_ENTRY CurrentEntry;
  PIPDATAGRAM_HOLE Hole, NewHole;
  USHORT FragFirst;
  USHORT FragLast;
  BOOLEAN MoreFragments;
  PIPv4_HEADER IPv4Header;
  IP_PACKET Datagram;
  PIP_FRAGMENT Fragment;
  BOOLEAN Success;

  /* FIXME: Assume IPv4 */

  IPv4Header = (PIPv4_HEADER)IPPacket->Header;

  /* Check if we already have an reassembly structure for this datagram */
  IPDR = GetReassemblyInfo(IPPacket);
  if (IPDR) {
    TI_DbgPrint(DEBUG_IP, ("Continueing assembly.\n"));
    /* We have a reassembly structure */
    TcpipAcquireSpinLock(&IPDR->Lock, &OldIrql);

    /* Reset the timeout since we received a fragment */
    IPDR->TimeoutCount = 0;
  } else {
    TI_DbgPrint(DEBUG_IP, ("Starting new assembly.\n"));

    /* We don't have a reassembly structure, create one */
    IPDR = ExAllocateFromNPagedLookasideList(&IPDRList);
    if (!IPDR)
      /* We don't have the resources to process this packet, discard it */
      return;

    /* Create a descriptor spanning from zero to infinity.
       Actually, we use a value slightly greater than the
       maximum number of octets an IP datagram can contain */
    Hole = CreateHoleDescriptor(0, 65536);
    if (!Hole) {
      /* We don't have the resources to process this packet, discard it */
      ExFreeToNPagedLookasideList(&IPDRList, IPDR);
      return;
    }
    AddrInitIPv4(&IPDR->SrcAddr, IPv4Header->SrcAddr);
    AddrInitIPv4(&IPDR->DstAddr, IPv4Header->DstAddr);
    IPDR->Id         = IPv4Header->Id;
    IPDR->Protocol   = IPv4Header->Protocol;
    IPDR->TimeoutCount = 0;
    InitializeListHead(&IPDR->FragmentListHead);
    InitializeListHead(&IPDR->HoleListHead);
    InsertTailList(&IPDR->HoleListHead, &Hole->ListEntry);

    TcpipInitializeSpinLock(&IPDR->Lock);

    TcpipAcquireSpinLock(&IPDR->Lock, &OldIrql);

    /* Update the reassembly list */
    TcpipInterlockedInsertTailList(
	&ReassemblyListHead,
	&IPDR->ListEntry,
	&ReassemblyListLock);
  }

  FragFirst     = (WN2H(IPv4Header->FlagsFragOfs) & IPv4_FRAGOFS_MASK) << 3;
  FragLast      = FragFirst + WN2H(IPv4Header->TotalLength);
  MoreFragments = (WN2H(IPv4Header->FlagsFragOfs) & IPv4_MF_MASK) > 0;

  CurrentEntry = IPDR->HoleListHead.Flink;
  for (;;) {
    if (CurrentEntry == &IPDR->HoleListHead)
        break;

    Hole = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_HOLE, ListEntry);

    TI_DbgPrint(DEBUG_IP, ("Comparing Fragment (%d,%d) to Hole (%d,%d).\n",
      FragFirst, FragLast, Hole->First, Hole->Last));

    if ((FragFirst > Hole->Last) || (FragLast < Hole->First)) {
      TI_DbgPrint(MID_TRACE, ("No overlap.\n"));
      /* The fragment does not overlap with the hole, try next
         descriptor in the list */

      CurrentEntry = CurrentEntry->Flink;
      continue;
    }

    /* The fragment overlap with the hole, unlink the descriptor */
    RemoveEntryList(CurrentEntry);

    if (FragFirst > Hole->First) {
      NewHole = CreateHoleDescriptor(Hole->First, FragFirst - 1);
      if (!NewHole) {
        /* We don't have the resources to process this packet, discard it */
        ExFreeToNPagedLookasideList(&IPHoleList, Hole);
        Cleanup(&IPDR->Lock, OldIrql, IPDR);
        return;
      }

      /* Put the new descriptor in the list */
      InsertTailList(&IPDR->HoleListHead, &NewHole->ListEntry);
    }

    if ((FragLast < Hole->Last) && MoreFragments) {
      NewHole = CreateHoleDescriptor(FragLast + 1, Hole->Last);
      if (!NewHole) {
        /* We don't have the resources to process this packet, discard it */
        ExFreeToNPagedLookasideList(&IPHoleList, Hole);
        Cleanup(&IPDR->Lock, OldIrql, IPDR);
        return;
      }

      /* Put the new hole descriptor in the list */
      InsertTailList(&IPDR->HoleListHead, &NewHole->ListEntry);
    }

    ExFreeToNPagedLookasideList(&IPHoleList, Hole);

    /* If this is the first fragment, save the IP header */
    if (FragFirst == 0) {
        IPDR->IPv4Header = ExAllocatePoolWithTag(NonPagedPool,
                                                 IPPacket->HeaderSize,
                                                 PACKET_BUFFER_TAG);
        if (!IPDR->IPv4Header)
        {
            Cleanup(&IPDR->Lock, OldIrql, IPDR);
            return;
        }

        RtlCopyMemory(IPDR->IPv4Header, IPPacket->Header, IPPacket->HeaderSize);
        IPDR->HeaderSize = IPPacket->HeaderSize;

        TI_DbgPrint(DEBUG_IP, ("First fragment found. Header buffer is at (0x%X). "
                               "Header size is (%d).\n", &IPDR->IPv4Header, IPPacket->HeaderSize));

    }

    /* Create a buffer, copy the data into it and put it
       in the fragment list */

    Fragment = ExAllocateFromNPagedLookasideList(&IPFragmentList);
    if (!Fragment) {
      /* We don't have the resources to process this packet, discard it */
      Cleanup(&IPDR->Lock, OldIrql, IPDR);
      return;
    }

    TI_DbgPrint(DEBUG_IP, ("Fragment descriptor allocated at (0x%X).\n", Fragment));

    Fragment->Size = IPPacket->TotalSize - IPPacket->HeaderSize;
    Fragment->Packet = IPPacket->NdisPacket;
    Fragment->ReturnPacket = IPPacket->ReturnPacket;
    Fragment->PacketOffset = IPPacket->Position + IPPacket->HeaderSize;
    Fragment->Offset = FragFirst;

    /* Disassociate the NDIS packet so it isn't freed upon return from IPReceive() */
    IPPacket->NdisPacket = NULL;

    /* If this is the last fragment, compute and save the datagram data size */
    if (!MoreFragments)
      IPDR->DataSize = FragFirst + Fragment->Size;

    /* Put the fragment in the list */
    InsertTailList(&IPDR->FragmentListHead, &Fragment->ListEntry);
    break;
  }

  TI_DbgPrint(DEBUG_IP, ("Done searching for hole descriptor.\n"));

  if (IsListEmpty(&IPDR->HoleListHead)) {
    /* Hole list is empty which means a complete datagram can be assembled.
       Assemble the datagram and pass it to an upper layer protocol */

    TI_DbgPrint(DEBUG_IP, ("Complete datagram received.\n"));

    RemoveIPDR(IPDR);
    TcpipReleaseSpinLock(&IPDR->Lock, OldIrql);

    /* FIXME: Assumes IPv4 */
    IPInitializePacket(&Datagram, IP_ADDRESS_V4);

    Success = ReassembleDatagram(&Datagram, IPDR);

    FreeIPDR(IPDR);

    if (!Success)
      /* Not enough free resources, discard the packet */
      return;

    DISPLAY_IP_PACKET(&Datagram);

    /* Give the packet to the protocol dispatcher */
    IPDispatchProtocol(IF, &Datagram);

    /* We're done with this datagram */
    TI_DbgPrint(MAX_TRACE, ("Freeing datagram at (0x%X).\n", Datagram));
    Datagram.Free(&Datagram);
  } else
    TcpipReleaseSpinLock(&IPDR->Lock, OldIrql);
}


VOID IPFreeReassemblyList(
  VOID)
/*
 * FUNCTION: Frees all IP datagram reassembly structures in the list
 */
{
  KIRQL OldIrql;
  PLIST_ENTRY CurrentEntry, NextEntry;
  PIPDATAGRAM_REASSEMBLY Current;

  TcpipAcquireSpinLock(&ReassemblyListLock, &OldIrql);

  CurrentEntry = ReassemblyListHead.Flink;
  while (CurrentEntry != &ReassemblyListHead) {
    NextEntry = CurrentEntry->Flink;
    Current = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_REASSEMBLY, ListEntry);

    /* Unlink it from the list */
    RemoveEntryList(CurrentEntry);

    /* And free the descriptor */
    FreeIPDR(Current);

    CurrentEntry = NextEntry;
  }

  TcpipReleaseSpinLock(&ReassemblyListLock, OldIrql);
}


VOID IPDatagramReassemblyTimeout(
  VOID)
/*
 * FUNCTION: IP datagram reassembly timeout handler
 * NOTES:
 *     This routine is called by IPTimeout to free any resources used
 *     to hold IP fragments that have taken too long to reassemble
 */
{
    PLIST_ENTRY CurrentEntry, NextEntry;
    PIPDATAGRAM_REASSEMBLY CurrentIPDR;

    TcpipAcquireSpinLockAtDpcLevel(&ReassemblyListLock);

    CurrentEntry = ReassemblyListHead.Flink;
    while (CurrentEntry != &ReassemblyListHead)
    {
       NextEntry = CurrentEntry->Flink;
       CurrentIPDR = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_REASSEMBLY, ListEntry);

       TcpipAcquireSpinLockAtDpcLevel(&CurrentIPDR->Lock);

       if (++CurrentIPDR->TimeoutCount == MAX_TIMEOUT_COUNT)
       {
           TcpipReleaseSpinLockFromDpcLevel(&CurrentIPDR->Lock);
           RemoveEntryList(CurrentEntry);
           FreeIPDR(CurrentIPDR);
       }
       else
       {
           ASSERT(CurrentIPDR->TimeoutCount < MAX_TIMEOUT_COUNT);
           TcpipReleaseSpinLockFromDpcLevel(&CurrentIPDR->Lock);
       }

       CurrentEntry = NextEntry;
    }

    TcpipReleaseSpinLockFromDpcLevel(&ReassemblyListLock);
}

VOID IPv4Receive(PIP_INTERFACE IF, PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives an IPv4 datagram (or fragment)
 * ARGUMENTS:
 *     Context  = Pointer to context information (IP_INTERFACE)
 *     IPPacket = Pointer to IP packet
 */
{
    UCHAR FirstByte;
    ULONG BytesCopied;

    TI_DbgPrint(DEBUG_IP, ("Received IPv4 datagram.\n"));

    /* Read in the first IP header byte for size information */
    BytesCopied = CopyPacketToBuffer((PCHAR)&FirstByte,
                                     IPPacket->NdisPacket,
                                     IPPacket->Position,
                                     sizeof(UCHAR));
    if (BytesCopied != sizeof(UCHAR))
    {
        TI_DbgPrint(MIN_TRACE, ("Failed to copy in first byte\n"));
        /* Discard packet */
        return;
    }

    IPPacket->HeaderSize = (FirstByte & 0x0F) << 2;
    TI_DbgPrint(DEBUG_IP, ("IPPacket->HeaderSize = %d\n", IPPacket->HeaderSize));

    if (IPPacket->HeaderSize > IPv4_MAX_HEADER_SIZE) {
        TI_DbgPrint(MIN_TRACE, ("Datagram received with incorrect header size (%d).\n",
	      IPPacket->HeaderSize));
        /* Discard packet */
        return;
    }

    /* This is freed by IPPacket->Free() */
    IPPacket->Header = ExAllocatePoolWithTag(NonPagedPool,
                                             IPPacket->HeaderSize,
                                             PACKET_BUFFER_TAG);
    if (!IPPacket->Header)
    {
        TI_DbgPrint(MIN_TRACE, ("No resources to allocate header\n"));
        /* Discard packet */
        return;
    }

    IPPacket->MappedHeader = FALSE;

    BytesCopied = CopyPacketToBuffer((PCHAR)IPPacket->Header,
                                     IPPacket->NdisPacket,
                                     IPPacket->Position,
                                     IPPacket->HeaderSize);
    if (BytesCopied != IPPacket->HeaderSize)
    {
        TI_DbgPrint(MIN_TRACE, ("Failed to copy in header\n"));
        /* Discard packet */
        return;
    }

    /* Checksum IPv4 header */
    if (!IPv4CorrectChecksum(IPPacket->Header, IPPacket->HeaderSize)) {
        TI_DbgPrint(MIN_TRACE, ("Datagram received with bad checksum. Checksum field (0x%X)\n",
	      WN2H(((PIPv4_HEADER)IPPacket->Header)->Checksum)));
        /* Discard packet */
        return;
    }

    IPPacket->TotalSize = WN2H(((PIPv4_HEADER)IPPacket->Header)->TotalLength);

    AddrInitIPv4(&IPPacket->SrcAddr, ((PIPv4_HEADER)IPPacket->Header)->SrcAddr);
    AddrInitIPv4(&IPPacket->DstAddr, ((PIPv4_HEADER)IPPacket->Header)->DstAddr);

    TI_DbgPrint(MID_TRACE,("IPPacket->Position = %d\n",
                           IPPacket->Position));

    /* FIXME: Possibly forward packets with multicast addresses */

    /* FIXME: Should we allow packets to be received on the wrong interface? */
    /* XXX Find out if this packet is destined for us */
    ProcessFragment(IF, IPPacket);
}


VOID IPReceive( PIP_INTERFACE IF, PIP_PACKET IPPacket )
/*
 * FUNCTION: Receives an IP datagram (or fragment)
 * ARGUMENTS:
 *     IF       = Interface
 *     IPPacket = Pointer to IP packet
 */
{
    UCHAR FirstByte;
    UINT Version, BytesCopied;

    /* Read in the first IP header byte for version information */
    BytesCopied = CopyPacketToBuffer((PCHAR)&FirstByte,
                                     IPPacket->NdisPacket,
                                     IPPacket->Position,
                                     sizeof(UCHAR));
    if (BytesCopied != sizeof(UCHAR))
    {
        TI_DbgPrint(MIN_TRACE, ("Failed to copy in first byte\n"));
        IPPacket->Free(IPPacket);
        return;
    }

    /* Check that IP header has a supported version */
    Version = (FirstByte >> 4);

    switch (Version) {
        case 4:
            IPPacket->Type = IP_ADDRESS_V4;
            IPv4Receive(IF, IPPacket);
            break;
        case 6:
            IPPacket->Type = IP_ADDRESS_V6;
            TI_DbgPrint(MAX_TRACE, ("Datagram of type IPv6 discarded.\n"));
            break;
        default:
            TI_DbgPrint(MIN_TRACE, ("Datagram has an unsupported IP version %d.\n", Version));
            break;
    }

    IPPacket->Free(IPPacket);
}

/* EOF */
