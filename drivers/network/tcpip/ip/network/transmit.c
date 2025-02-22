/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/transmit.c
 * PURPOSE:     Internet Protocol transmit routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

BOOLEAN PrepareNextFragment(PIPFRAGMENT_CONTEXT IFC);
NTSTATUS IPSendFragment(PNDIS_PACKET NdisPacket,
			PNEIGHBOR_CACHE_ENTRY NCE,
			PIPFRAGMENT_CONTEXT IFC);

VOID IPSendComplete
(PVOID Context, PNDIS_PACKET NdisPacket, NDIS_STATUS NdisStatus)
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
    PIPFRAGMENT_CONTEXT IFC = (PIPFRAGMENT_CONTEXT)Context;

    TI_DbgPrint
	(MAX_TRACE,
	 ("Called. Context (0x%X)  NdisPacket (0x%X)  NdisStatus (0x%X)\n",
	  Context, NdisPacket, NdisStatus));

	IFC->Status = NdisStatus;
	KeSetEvent(&IFC->Event, 0, FALSE);
}

NTSTATUS IPSendFragment(
    PNDIS_PACKET NdisPacket,
    PNEIGHBOR_CACHE_ENTRY NCE,
    PIPFRAGMENT_CONTEXT IFC)
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
    TI_DbgPrint(MAX_TRACE, ("Called. NdisPacket (0x%X)  NCE (0x%X).\n", NdisPacket, NCE));

    TI_DbgPrint(MAX_TRACE, ("NCE->State = %d.\n", NCE->State));
    return NBQueuePacket(NCE, NdisPacket, IPSendComplete, IFC);
}

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

    TI_DbgPrint(MAX_TRACE, ("Called. IFC (0x%X)\n", IFC));

    if (IFC->BytesLeft > 0) {

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

	TI_DbgPrint(MID_TRACE,("Copying data from %x to %x (%d)\n",
			       IFC->DatagramData, IFC->Data, DataSize));

        RtlCopyMemory(IFC->Data, IFC->DatagramData, DataSize); // SAFE

        /* Fragment offset is in 8 byte blocks */
        FragOfs = (USHORT)(IFC->Position / 8);

        if (MoreFragments)
            FragOfs |= IPv4_MF_MASK;
        else
            FragOfs &= ~IPv4_MF_MASK;

        Header = IFC->Header;
        Header->FlagsFragOfs = WH2N(FragOfs);
        Header->TotalLength = WH2N((USHORT)(DataSize + IFC->HeaderSize));

        /* FIXME: Handle options */

        /* Calculate checksum of IP header */
        Header->Checksum = 0;
        Header->Checksum = (USHORT)IPv4Checksum(Header, IFC->HeaderSize, 0);
	TI_DbgPrint(MID_TRACE,("IP Check: %x\n", Header->Checksum));

        /* Update pointers */
        IFC->DatagramData = (PVOID)((ULONG_PTR)IFC->DatagramData + DataSize);
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
    UINT BufferSize = PathMTU, InSize;
    PCHAR InData;

    TI_DbgPrint(MAX_TRACE, ("Called. IPPacket (0x%X)  NCE (0x%X)  PathMTU (%d).\n",
        IPPacket, NCE, PathMTU));

    /* Make a smaller buffer if we will only send one fragment */
    GetDataPtr( IPPacket->NdisPacket, IPPacket->Position, &InData, &InSize );
    if( InSize < BufferSize ) BufferSize = InSize;

    TI_DbgPrint(MAX_TRACE, ("Fragment buffer is %d bytes\n", BufferSize));

    IFC = ExAllocatePoolWithTag(NonPagedPool, sizeof(IPFRAGMENT_CONTEXT), IFC_TAG);
    if (IFC == NULL)
    {
        IPPacket->Free(IPPacket);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate NDIS packet */
    NdisStatus = AllocatePacketWithBuffer
	( &IFC->NdisPacket, NULL, BufferSize );

    if( !NT_SUCCESS(NdisStatus) ) {
        IPPacket->Free(IPPacket);
        ExFreePoolWithTag( IFC, IFC_TAG );
        return NdisStatus;
    }

    GetDataPtr( IFC->NdisPacket, 0, (PCHAR *)&Data, &InSize );

    IFC->Header       = ((PCHAR)Data);
    IFC->Datagram     = IPPacket->NdisPacket;
    IFC->DatagramData = ((PCHAR)IPPacket->Header) + IPPacket->HeaderSize;
    IFC->HeaderSize   = IPPacket->HeaderSize;
    IFC->PathMTU      = PathMTU;
    IFC->NCE          = NCE;
    IFC->Position     = 0;
    IFC->BytesLeft    = IPPacket->TotalSize - IPPacket->HeaderSize;
    IFC->Data         = (PVOID)((ULONG_PTR)IFC->Header + IPPacket->HeaderSize);
    KeInitializeEvent(&IFC->Event, NotificationEvent, FALSE);

    TI_DbgPrint(MID_TRACE,("Copying header from %x to %x (%d)\n",
			   IPPacket->Header, IFC->Header,
			   IPPacket->HeaderSize));

    RtlCopyMemory( IFC->Header, IPPacket->Header, IPPacket->HeaderSize );

    while (PrepareNextFragment(IFC))
    {
        NdisStatus = IPSendFragment(IFC->NdisPacket, NCE, IFC);
        if (NT_SUCCESS(NdisStatus))
        {
            KeWaitForSingleObject(&IFC->Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            NdisStatus = IFC->Status;
        }

        if (!NT_SUCCESS(NdisStatus))
            break;
    }

    FreeNdisPacket(IFC->NdisPacket);
    ExFreePoolWithTag(IFC, IFC_TAG);
    IPPacket->Free(IPPacket);

    return NdisStatus;
}

NTSTATUS IPSendDatagram(PIP_PACKET IPPacket, PNEIGHBOR_CACHE_ENTRY NCE)
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
    TI_DbgPrint(MAX_TRACE, ("Called. IPPacket (0x%X)  NCE (0x%X)\n", IPPacket, NCE));

    DISPLAY_IP_PACKET(IPPacket);

    /* Fetch path MTU now, because it may change */
    TI_DbgPrint(MID_TRACE,("PathMTU: %d\n", NCE->Interface->MTU));

    return SendFragments(IPPacket, NCE, NCE->Interface->MTU);
}

/* EOF */
