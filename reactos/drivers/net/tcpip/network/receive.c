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
#include <tcpip.h>
#include <receive.h>
#include <routines.h>
#include <checksum.h>
#include <transmit.h>
#include <address.h>
#include <pool.h>
#include <route.h>


LIST_ENTRY ReassemblyListHead;
KSPIN_LOCK ReassemblyListLock;


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

    Hole = PoolAllocateBuffer(sizeof(IPDATAGRAM_HOLE));
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
        PoolFreeBuffer(CurrentH);

        CurrentEntry = NextEntry;
    }

    /* Free all fragments */
    CurrentEntry = IPDR->FragmentListHead.Flink;
    while (CurrentEntry != &IPDR->FragmentListHead) {
        NextEntry = CurrentEntry->Flink;
	    CurrentF = CONTAINING_RECORD(CurrentEntry, IP_FRAGMENT, ListEntry);
        /* Unlink it from the list */
        RemoveEntryList(CurrentEntry);

        TI_DbgPrint(DEBUG_IP, ("Freeing fragment data at (0x%X).\n", CurrentF->Data));

        /* Free the fragment data buffer */
        ExFreePool(CurrentF->Data);

        TI_DbgPrint(DEBUG_IP, ("Freeing fragment at (0x%X).\n", CurrentF));

        /* And free the fragment descriptor */
        PoolFreeBuffer(CurrentF);
        CurrentEntry = NextEntry;
    }

    /* Free resources for the header, if it exists */
    if (IPDR->IPv4Header) {
        TI_DbgPrint(DEBUG_IP, ("Freeing IPv4 header data at (0x%X).\n", IPDR->IPv4Header));
        ExFreePool(IPDR->IPv4Header);
    }

    TI_DbgPrint(DEBUG_IP, ("Freeing IPDR data at (0x%X).\n", IPDR));

    PoolFreeBuffer(IPDR);
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

    KeAcquireSpinLock(&ReassemblyListLock, &OldIrql);
    RemoveEntryList(&IPDR->ListEntry);
    KeReleaseSpinLock(&ReassemblyListLock, OldIrql);
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

    KeAcquireSpinLock(&ReassemblyListLock, &OldIrql);

    /* FIXME: Assume IPv4 */

    CurrentEntry = ReassemblyListHead.Flink;
    while (CurrentEntry != &ReassemblyListHead) {
	    Current = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_REASSEMBLY, ListEntry);
        if (AddrIsEqual(&IPPacket->SrcAddr, &Current->SrcAddr) &&
            (Header->Id == Current->Id) &&
            (Header->Protocol == Current->Protocol) &&
            (AddrIsEqual(&IPPacket->DstAddr, &Current->DstAddr))) {
            KeReleaseSpinLock(&ReassemblyListLock, OldIrql);

            return Current;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&ReassemblyListLock, OldIrql);

    return NULL;
}


PIP_PACKET ReassembleDatagram(
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
 */
{
    PIP_PACKET IPPacket;
    PLIST_ENTRY CurrentEntry;
    PIP_FRAGMENT Current;
    PVOID Data;

    TI_DbgPrint(DEBUG_IP, ("Reassembling datagram from IPDR at (0x%X).\n", IPDR));

    IPPacket = PoolAllocateBuffer(sizeof(IP_PACKET));
    if (!IPPacket) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

	/* FIXME: Assume IPv4 */

	IPPacket->Type       = IP_ADDRESS_V4;
    IPPacket->RefCount   = 1;
    IPPacket->TotalSize  = IPDR->HeaderSize + IPDR->DataSize;
    IPPacket->ContigSize = IPPacket->TotalSize;
    IPPacket->HeaderSize = IPDR->HeaderSize;
    IPPacket->Position   = IPDR->HeaderSize;

    RtlCopyMemory(&IPPacket->SrcAddr, &IPDR->SrcAddr, sizeof(IP_ADDRESS));
    RtlCopyMemory(&IPPacket->DstAddr, &IPDR->DstAddr, sizeof(IP_ADDRESS));

    /* Allocate space for full IP datagram */
    IPPacket->Header = ExAllocatePool(NonPagedPool, IPPacket->TotalSize);
    if (!IPPacket->Header) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        PoolFreeBuffer(IPPacket);
        return NULL;
    }

    /* Copy the header into the buffer */
    RtlCopyMemory(IPPacket->Header, IPDR->IPv4Header, IPDR->HeaderSize);

	Data = (PVOID)((ULONG_PTR)IPPacket->Header + IPDR->HeaderSize);
    IPPacket->Data = Data;

    /* Copy data from all fragments into buffer */
    CurrentEntry = IPDR->FragmentListHead.Flink;
    while (CurrentEntry != &IPDR->FragmentListHead) {
	    Current = CONTAINING_RECORD(CurrentEntry, IP_FRAGMENT, ListEntry);

        TI_DbgPrint(DEBUG_IP, ("Copying (%d) bytes of fragment data from (0x%X) to offset (%d).\n",
            Current->Size, Data, Current->Offset));
        /* Copy fragment data to the destination buffer at the correct offset */
        RtlCopyMemory((PVOID)((ULONG_PTR)Data + Current->Offset),
                      Current->Data,
                      Current->Size);

        CurrentEntry = CurrentEntry->Flink;
    }

    return IPPacket;
}


__inline VOID Cleanup(
    PKSPIN_LOCK Lock,
    KIRQL OldIrql,
    PIPDATAGRAM_REASSEMBLY IPDR,
    PVOID Buffer OPTIONAL)
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

    KeReleaseSpinLock(Lock, OldIrql);
    RemoveIPDR(IPDR);
    FreeIPDR(IPDR);
    if (Buffer)
        PoolFreeBuffer(Buffer);
}


VOID ProcessFragment(
    PIP_INTERFACE IF,
    PIP_PACKET IPPacket,
    PNET_TABLE_ENTRY NTE)
/*
 * FUNCTION: Processes an IP datagram or fragment
 * ARGUMENTS:
 *     IF       = Pointer to IP interface packet was receive on
 *     IPPacket = Pointer to IP packet
 *     NTE      = Pointer to NTE packet was received on
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
    PIP_PACKET Datagram;
    PIP_FRAGMENT Fragment;

    /* FIXME: Assume IPv4 */

    IPv4Header = (PIPv4_HEADER)IPPacket->Header;

    /* Check if we already have an reassembly structure for this datagram */
    IPDR = GetReassemblyInfo(IPPacket);
    if (IPDR) {
        TI_DbgPrint(DEBUG_IP, ("Continueing assembly.\n"));
        /* We have a reassembly structure */
        KeAcquireSpinLock(&IPDR->Lock, &OldIrql);
        CurrentEntry = IPDR->HoleListHead.Flink;
        Hole = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_HOLE, ListEntry);
    } else {
        TI_DbgPrint(DEBUG_IP, ("Starting new assembly.\n"));

        /* We don't have a reassembly structure, create one */
        IPDR = PoolAllocateBuffer(sizeof(IPDATAGRAM_REASSEMBLY));
        if (!IPDR) {
            /* We don't have the resources to process this packet, discard it */
            TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return;
        }

        /* Create a descriptor spanning from zero to infinity.
           Actually, we use a value slightly greater than the
           maximum number of octets an IP datagram can contain */
        Hole = CreateHoleDescriptor(0, 65536);
        if (!Hole) {
            /* We don't have the resources to process this packet, discard it */
            TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            PoolFreeBuffer(IPDR);
            return;
        }
        AddrInitIPv4(&IPDR->SrcAddr, IPv4Header->SrcAddr);
        AddrInitIPv4(&IPDR->DstAddr, IPv4Header->DstAddr);
        IPDR->Id         = IPv4Header->Id;
        IPDR->Protocol   = IPv4Header->Protocol;
        IPDR->IPv4Header = NULL;
        InitializeListHead(&IPDR->FragmentListHead);
        InitializeListHead(&IPDR->HoleListHead);
        InsertTailList(&IPDR->HoleListHead, &Hole->ListEntry);
        CurrentEntry = IPDR->HoleListHead.Flink;

        KeInitializeSpinLock(&IPDR->Lock);

        KeAcquireSpinLock(&IPDR->Lock, &OldIrql);

        /* Update the reassembly list */
        ExInterlockedInsertTailList(&ReassemblyListHead,
                                    &IPDR->ListEntry,
                                    &ReassemblyListLock);
    }

    FragFirst     = (WN2H(IPv4Header->FlagsFragOfs) & IPv4_FRAGOFS_MASK) << 3;
    FragLast      = FragFirst + WN2H(IPv4Header->TotalLength);
    MoreFragments = (WN2H(IPv4Header->FlagsFragOfs) & IPv4_MF_MASK) > 0;

    for (;;) {
        if (CurrentEntry == &IPDR->HoleListHead)
            /* No more entries */
            break;

        TI_DbgPrint(DEBUG_IP, ("Comparing Fragment (%d,%d) to Hole (%d,%d).\n",
            FragFirst, FragLast, Hole->First, Hole->Last));

        if ((FragFirst > Hole->Last) || (FragLast < Hole->First)) {
            TI_DbgPrint(MID_TRACE, ("No overlap.\n"));
            /* The fragment does not overlap with the hole, try next
               descriptor in the list */

            CurrentEntry = CurrentEntry->Flink;
            if (CurrentEntry != &IPDR->HoleListHead)
                Hole = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_HOLE, ListEntry);
            continue;
        }

        /* The fragment overlap with the hole, unlink the descriptor */
        RemoveEntryList(CurrentEntry);

        if (FragFirst > Hole->First) {
            NewHole = CreateHoleDescriptor(Hole->First, FragLast - 1);
            if (!NewHole) {
                /* We don't have the resources to process this packet, discard it */
                Cleanup(&IPDR->Lock, OldIrql, IPDR, Hole);
                return;
            }

            /* Put the new descriptor in the list */
            InsertTailList(&IPDR->HoleListHead, &NewHole->ListEntry);
        }

        if ((FragLast < Hole->Last) && (MoreFragments)) {
            /* We can reuse the descriptor for the new hole */
			Hole->First = FragLast + 1;

			/* Put the new hole descriptor in the list */
            InsertTailList(&IPDR->HoleListHead, &Hole->ListEntry);
        } else
            PoolFreeBuffer(Hole);

        /* If this is the first fragment, save the IP header */
        if (FragFirst == 0) {
            IPDR->IPv4Header = ExAllocatePool(NonPagedPool, IPPacket->HeaderSize);
            if (!IPDR->IPv4Header) {
                /* We don't have the resources to process this packet, discard it */
                Cleanup(&IPDR->Lock, OldIrql, IPDR, NULL);
                return;
            }

            TI_DbgPrint(DEBUG_IP, ("First fragment found. Header buffer is at (0x%X). "
                "Header size is (%d).\n", IPDR->IPv4Header, IPPacket->HeaderSize));

            RtlCopyMemory(IPDR->IPv4Header, IPPacket->Header, IPPacket->HeaderSize);
            IPDR->HeaderSize = IPPacket->HeaderSize;
        }

        /* Create a buffer, copy the data into it and put it
           in the fragment list */

        Fragment = PoolAllocateBuffer(sizeof(IP_FRAGMENT));
        if (!Fragment) {
            /* We don't have the resources to process this packet, discard it */
            Cleanup(&IPDR->Lock, OldIrql, IPDR, NULL);
            return;
        }

        TI_DbgPrint(DEBUG_IP, ("Fragment descriptor allocated at (0x%X).\n", Fragment));

        Fragment->Size = IPPacket->TotalSize - IPPacket->HeaderSize;
        Fragment->Data = ExAllocatePool(NonPagedPool, Fragment->Size);
        if (!Fragment->Data) {
            /* We don't have the resources to process this packet, discard it */
            Cleanup(&IPDR->Lock, OldIrql, IPDR, Fragment);
            return;
        }

        TI_DbgPrint(DEBUG_IP, ("Fragment data buffer allocated at (0x%X)  Size (%d).\n",
            Fragment->Data, Fragment->Size));

        /* Copy datagram data into fragment buffer */
        CopyPacketToBuffer(Fragment->Data,
                           IPPacket->NdisPacket,
                           IPPacket->Position,
                           Fragment->Size);
        Fragment->Offset = FragFirst;

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

        Datagram = ReassembleDatagram(IPDR);
		KeReleaseSpinLock(&IPDR->Lock, OldIrql);

        RemoveIPDR(IPDR);
        FreeIPDR(IPDR);

        if (!Datagram) {
            /* Not enough free resources, discard the packet */
            TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
            return;
        }

        DISPLAY_IP_PACKET(Datagram);

        /* Give the packet to the protocol dispatcher */
        IPDispatchProtocol(NTE, Datagram);

        /* We're done with this datagram */
        ExFreePool(Datagram->Header);
        PoolFreeBuffer(Datagram);
    } else
        KeReleaseSpinLock(&IPDR->Lock, OldIrql);
}


VOID IPFreeReassemblyList(
    VOID)
/*
 * FUNCTION: Frees all IP datagram reassembly structures in the list
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PIPDATAGRAM_REASSEMBLY Current;

    KeAcquireSpinLock(&ReassemblyListLock, &OldIrql);

    CurrentEntry = ReassemblyListHead.Flink;
    while (CurrentEntry != &ReassemblyListHead) {
	    Current = CONTAINING_RECORD(CurrentEntry, IPDATAGRAM_REASSEMBLY, ListEntry);
        /* Unlink it from the list */
        RemoveEntryList(CurrentEntry);
           
        /* And free the descriptor */
        FreeIPDR(Current);

        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&ReassemblyListLock, OldIrql);
}


VOID IPDatagramReassemblyTimeout(
    VOID)
/*
 * FUNCTION: IP datagram reassembly timeout handler
 * NOTES:
 *     This routine is called by IPTimeout to free any resources used
 *     to hold IP fragments that are being reassembled to form a
 *     complete IP datagram
 */
{
}


VOID IPv4Receive(
    PVOID Context,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives an IPv4 datagram (or fragment)
 * ARGUMENTS:
 *     Context  = Pointer to context information (IP_INTERFACE)
 *     IPPacket = Pointer to IP packet
 */
{
//    PNEIGHBOR_CACHE_ENTRY NCE;
    PNET_TABLE_ENTRY NTE;
    UINT AddressType;

    //TI_DbgPrint(DEBUG_IP, ("Received IPv4 datagram.\n"));

    IPPacket->HeaderSize = (((PIPv4_HEADER)IPPacket->Header)->VerIHL & 0x0F) << 2;

    if (IPPacket->HeaderSize > IPv4_MAX_HEADER_SIZE) {
        TI_DbgPrint(MIN_TRACE, ("Datagram received with incorrect header size (%d).\n",
            IPPacket->HeaderSize));
        /* Discard packet */
        return;
    }

    /* Checksum IPv4 header */
    if (!CorrectChecksum(IPPacket->Header, IPPacket->HeaderSize)) {
        TI_DbgPrint(MIN_TRACE, ("Datagram received with bad checksum. Checksum field (0x%X)\n",
            WN2H(((PIPv4_HEADER)IPPacket->Header)->Checksum)));
        /* Discard packet */
        return;
    }

//    TI_DbgPrint(DEBUG_IP, ("TotalSize (datalink) is (%d).\n", IPPacket->TotalSize));

    IPPacket->TotalSize = WN2H(((PIPv4_HEADER)IPPacket->Header)->TotalLength);

//    TI_DbgPrint(DEBUG_IP, ("TotalSize (IPv4) is (%d).\n", IPPacket->TotalSize));

	AddrInitIPv4(&IPPacket->SrcAddr, ((PIPv4_HEADER)IPPacket->Header)->SrcAddr);
	AddrInitIPv4(&IPPacket->DstAddr, ((PIPv4_HEADER)IPPacket->Header)->DstAddr);

    IPPacket->Position = IPPacket->HeaderSize;
    IPPacket->Data     = (PVOID)((ULONG_PTR)IPPacket->Header + IPPacket->HeaderSize);

    /* FIXME: Possibly forward packets with multicast addresses */

    /* FIXME: Should we allow packets to be received on the wrong interface? */
#if 0
    NTE = IPLocateNTE(&IPPacket->DstAddr, &AddressType);
#else
    NTE = IPLocateNTEOnInterface((PIP_INTERFACE)Context, &IPPacket->DstAddr, &AddressType);
#endif
    if (NTE) {
        /* This packet is destined for us */
        ProcessFragment((PIP_INTERFACE)Context, IPPacket, NTE);
        /* Done with this NTE */
        DereferenceObject(NTE);
    } else {
        /* This packet is not destined for us. If we are a router,
           try to find a route and forward the packet */

        /* FIXME: Check if acting as a router */
#if 0
        NCE = RouteFindRouter(&IPPacket->DstAddr, NULL);
        if (NCE) {
            /* FIXME: Possibly fragment datagram */
            /* Forward the packet */
            IPSendFragment(IPPacket, NCE);
        } else {
            TI_DbgPrint(MIN_TRACE, ("No route to destination (0x%X).\n",
                IPPacket->DstAddr.Address.IPv4Address));

            /* FIXME: Send ICMP error code */
        }
#endif
    }
}


VOID IPReceive(
    PVOID Context,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Receives an IP datagram (or fragment)
 * ARGUMENTS:
 *     Context  = Pointer to context information (IP_INTERFACE)
 *     IPPacket = Pointer to IP packet
 */
{
    UINT Version;

    /* Check that IP header has a supported version */
    Version = (((PIPv4_HEADER)IPPacket->Header)->VerIHL >> 4);

    switch (Version) {
    case 4:
        IPPacket->Type = IP_ADDRESS_V4;
        IPv4Receive(Context, IPPacket);
        break;
    case 6:
        IPPacket->Type = IP_ADDRESS_V6;
        TI_DbgPrint(MAX_TRACE, ("Datagram of type IPv6 discarded.\n"));
        return;
    default:
		TI_DbgPrint(MIN_TRACE, ("Datagram has an unsupported IP version %d.\n", Version));
        return;
    }
}

/* EOF */
