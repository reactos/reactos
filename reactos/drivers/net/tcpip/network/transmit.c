/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/transmit.c
 * PURPOSE:     Internet Protocol transmit routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <transmit.h>
#include <routines.h>
#include <checksum.h>
#include <pool.h>
#include <arp.h>
#include <lan.h>


BOOLEAN PrepareNextFragment(
    PIPFRAGMENT_CONTEXT IFC)
/*
 * FUNCTION: Prepares the next fragment of an IP datagram for transmission
 * ARGUMENTS:
 *     IFC = Pointer to IP fragment context
 * RETURNS:
 *     TRUE if a fragment was prepared for transmission, FALSE if
 *     there are no more fragments to send
 */
{
    UINT MaxData;
    UINT DataSize;
    PIPv4_HEADER Header;
    BOOLEAN MoreFragments;
    USHORT FragOfs;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    if (IFC->BytesLeft != 0) {

        TI_DbgPrint(MAX_TRACE, ("Preparing 1 fragment.\n"));

        MaxData  = IFC->PathMTU - IFC->HeaderSize;
        /* Make fragment a multiplum of 64bit */
        MaxData -= MaxData % 8;
        if (IFC->BytesLeft > MaxData) {
            DataSize      = MaxData;
            MoreFragments = TRUE;
        } else {
            DataSize      = IFC->BytesLeft;
            MoreFragments = FALSE;
        }

        RtlCopyMemory(IFC->Data, IFC->DatagramData, DataSize);

        FragOfs = (USHORT)IFC->Position; // Swap?
        if (MoreFragments)
            FragOfs |= IPv4_MF_MASK;
        else
            FragOfs &= ~IPv4_MF_MASK;

        Header = IFC->Header;
        Header->FlagsFragOfs = FragOfs;

        /* FIXME: Handle options */

        /* Calculate checksum of IP header */
        Header->Checksum = 0;
        Header->Checksum = (USHORT)IPv4Checksum(Header, IFC->HeaderSize, 0);

        /* Update pointers */
        (ULONG_PTR)IFC->DatagramData += DataSize;
        IFC->Position  += DataSize;
        IFC->BytesLeft -= DataSize;

        return TRUE;
    } else {
        TI_DbgPrint(MAX_TRACE, ("No more fragments.\n"));
        return FALSE;
    }
}


NTSTATUS SendFragments(
    PIP_PACKET IPPacket,
    PNEIGHBOR_CACHE_ENTRY NCE,
    UINT PathMTU)
/*
 * FUNCTION: Fragments and sends the first fragment of an IP datagram
 * ARGUMENTS:
 *     IPPacket  = Pointer to an IP packet
 *     NCE       = Pointer to NCE for first hop to destination
 *     PathMTU   = Size of Maximum Transmission Unit of path
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     IP datagram is larger than PathMTU when this is called
 */
{
    PIPFRAGMENT_CONTEXT IFC;
    NDIS_STATUS NdisStatus;
    PVOID Data;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    IFC = PoolAllocateBuffer(sizeof(IPFRAGMENT_CONTEXT));
    if (!IFC)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* We allocate a buffer for a PathMTU sized packet and reuse
       it for all fragments */
    Data = ExAllocatePool(NonPagedPool, MaxLLHeaderSize + PathMTU);
    if (!IFC->Header) {
        PoolFreeBuffer(IFC);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate NDIS packet */
    NdisAllocatePacket(&NdisStatus, &IFC->NdisPacket, GlobalPacketPool);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        ExFreePool(Data);
        PoolFreeBuffer(IFC);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate NDIS buffer */
    NdisAllocateBuffer(&NdisStatus, &IFC->NdisBuffer,
        GlobalBufferPool, Data, MaxLLHeaderSize + PathMTU);
    if (NdisStatus != NDIS_STATUS_SUCCESS) {
        NdisFreePacket(IFC->NdisPacket);
        ExFreePool(Data);
        PoolFreeBuffer(IFC);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Link NDIS buffer into packet */
    NdisChainBufferAtFront(IFC->NdisPacket, IFC->NdisBuffer);

    IFC->Header       = (PVOID)((ULONG_PTR)Data + MaxLLHeaderSize);
    IFC->Datagram     = IPPacket->NdisPacket;
    IFC->DatagramData = IPPacket->Header;
    IFC->HeaderSize   = IPPacket->HeaderSize;
    IFC->PathMTU      = PathMTU;
    IFC->NCE          = NCE;
    IFC->Position     = 0;
    IFC->BytesLeft    = IPPacket->TotalSize - IPPacket->HeaderSize;
    IFC->Data         = (PVOID)((ULONG_PTR)IFC->Header + IPPacket->HeaderSize);

    PC(IFC->NdisPacket)->DLComplete = IPSendComplete;
    /* Set upper layer completion function to NULL to indicate that
       this packet is an IP datagram fragment and thus we should
       check for more fragments to send. If this is NULL the
       Context field is a pointer to an IPFRAGMENT_CONTEXT structure */
    PC(IFC->NdisPacket)->Complete = NULL;
    PC(IFC->NdisPacket)->Context  = IFC;

    /* Copy IP datagram header to fragment buffer */
    RtlCopyMemory(IFC->Header, IPPacket->Header, IPPacket->HeaderSize);

    /* Prepare next fragment for transmission and send it */

    PrepareNextFragment(IFC);

    IPSendFragment(IFC->NdisPacket, NCE);

    return STATUS_SUCCESS;
}


VOID IPSendComplete(
    PVOID Context,
    PNDIS_PACKET NdisPacket,
    NDIS_STATUS NdisStatus)
/*
 * FUNCTION: IP datagram fragment send completion handler
 * ARGUMENTS:
 *     Context    = Pointer to context information (IP_INTERFACE)
 *     Packet     = Pointer to NDIS packet that was sent
 *     NdisStatus = NDIS status of operation
 * NOTES:
 *    This routine is called when an IP datagram fragment has been sent
 */
{
    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* FIXME: Stop sending fragments and cleanup datagram buffers if
       there was an error */

    if (PC(NdisPacket)->Complete)
        /* This datagram was only one fragment long so call completion handler now */
        (*PC(NdisPacket)->Complete)(PC(NdisPacket)->Context, NdisPacket, NdisStatus);
    else {
        /* This was one of many fragments of an IP datagram. Prepare
           next fragment and send it or if there are no more fragments,
           call upper layer completion routine */

        PIPFRAGMENT_CONTEXT IFC = (PIPFRAGMENT_CONTEXT)PC(NdisPacket)->Context;

        if (PrepareNextFragment(IFC)) {
            /* A fragment was prepared for transmission, so send it */
            IPSendFragment(IFC->NdisPacket, IFC->NCE);
        } else {
            TI_DbgPrint(MAX_TRACE, ("Calling completion handler.\n"));

            /* There are no more fragments to transmit, so call completion handler */
            NdisPacket = IFC->Datagram;
            FreeNdisPacket(IFC->NdisPacket);
            PoolFreeBuffer(IFC);
            (*PC(NdisPacket)->Complete)(PC(NdisPacket)->Context, NdisPacket, NdisStatus);
        }
    }
}


NTSTATUS IPSendFragment(
    PNDIS_PACKET NdisPacket,
    PNEIGHBOR_CACHE_ENTRY NCE)
/*
 * FUNCTION: Sends an IP datagram fragment to a neighbor
 * ARGUMENTS:
 *     NdisPacket = Pointer to an NDIS packet containing fragment
 *     NCE        = Pointer to NCE for first hop to destination
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     Lowest level IP send routine
 */
{
    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    TI_DbgPrint(MAX_TRACE, ("NCE->State = %d.\n", NCE->State));

    switch (NCE->State) {
    case NUD_PERMANENT:
        /* Neighbor is always valid */
        break;

    case NUD_REACHABLE:
        /* Neighbor is reachable */
        
        /* FIXME: Set reachable timer */

        break;

    case NUD_STALE:
        /* Enter delay state and send packet */

        /* FIXME: Enter delay state */

        break;

    case NUD_DELAY:
    case NUD_PROBE:
        /* In these states we send the packet and hope the neighbor
           hasn't changed hardware address */
        break;

    case NUD_INCOMPLETE:
        TI_DbgPrint(MAX_TRACE, ("Queueing packet.\n"));

        /* We don't know the hardware address of the first hop to
           the destination. Queue the packet on the NCE and return */
        NBQueuePacket(NCE, NdisPacket);

        return STATUS_SUCCESS;
    default:
        /* Should not happen */
        TI_DbgPrint(MIN_TRACE, ("Unknown NCE state.\n"));

        return STATUS_SUCCESS;
    }

    PC(NdisPacket)->DLComplete = IPSendComplete;
    (*NCE->Interface->Transmit)(NCE->Interface->Context, NdisPacket,
        MaxLLHeaderSize, NCE->LinkAddress, LAN_PROTO_IPv4);

    return STATUS_SUCCESS;
}


NTSTATUS IPSendDatagram(
    PIP_PACKET IPPacket,
    PROUTE_CACHE_NODE RCN)
/*
 * FUNCTION: Sends an IP datagram to a remote address
 * ARGUMENTS:
 *     IPPacket = Pointer to an IP packet
 *     RCN      = Pointer to route cache node
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     This is the highest level IP send routine. It possibly breaks the packet
 *     into two or more fragments before passing it on to the next lower level
 *     send routine (IPSendFragment)
 */
{
    PNEIGHBOR_CACHE_ENTRY NCE;
    UINT PathMTU;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    NCE = RCN->NCE;

#if DBG
    if (!NCE) {
        TI_DbgPrint(MIN_TRACE, ("No NCE to use.\n"));
        FreeNdisPacket(IPPacket->NdisPacket);
        return STATUS_SUCCESS;
    }
#endif

    /* Fetch path MTU now, because it may change */
    PathMTU = RCN->PathMTU;
    if (IPPacket->TotalSize > PathMTU) {
        return SendFragments(IPPacket, NCE, PathMTU);
    } else {
        /* Calculate checksum of IP header */
        ((PIPv4_HEADER)IPPacket->Header)->Checksum = 0;

        ((PIPv4_HEADER)IPPacket->Header)->Checksum = (USHORT)
            IPv4Checksum(IPPacket->Header, IPPacket->HeaderSize, 0);

        TI_DbgPrint(MAX_TRACE, ("Sending packet (length is %d).\n", WN2H(((PIPv4_HEADER)IPPacket->Header)->TotalLength)));

        return IPSendFragment(IPPacket->NdisPacket, NCE);
    }
}

/* EOF */
