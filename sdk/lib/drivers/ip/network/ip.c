/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/ip.c
 * PURPOSE:     Internet Protocol module
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

#define __LWIP_INET_H__
#include "lwip/netifapi.h"


LIST_ENTRY InterfaceListHead;
KSPIN_LOCK InterfaceListLock;
LIST_ENTRY NetTableListHead;
KSPIN_LOCK NetTableListLock;
BOOLEAN IPInitialized = FALSE;
BOOLEAN IpWorkItemQueued = FALSE;
/* Work around calling timer at Dpc level */

IP_PROTOCOL_HANDLER ProtocolTable[IP_PROTOCOL_TABLE_SIZE];

ULONG IpTimerExpirations;

VOID
TCPRegisterInterface(PIP_INTERFACE IF);

VOID
TCPUnregisterInterface(PIP_INTERFACE IF);

VOID DeinitializePacket(
    PVOID Object)
/*
 * FUNCTION: Frees buffers attached to the packet
 * ARGUMENTS:
 *     Object = Pointer to an IP packet structure
 */
{
    PIP_PACKET IPPacket = Object;

    TI_DbgPrint(MAX_TRACE, ("Freeing object: 0x%p\n", Object));

    /* Detect double free */
    ASSERT(IPPacket->Type != 0xFF);
    IPPacket->Type = 0xFF;

    /* Check if there's a packet to free */
    if (IPPacket->NdisPacket != NULL)
    {
        if (IPPacket->ReturnPacket)
        {
            /* Return the packet to the miniport driver */
            TI_DbgPrint(MAX_TRACE, ("Returning packet 0x%p\n",
                                    IPPacket->NdisPacket));
            NdisReturnPackets(&IPPacket->NdisPacket, 1);
        }
        else
        {
            /* Free it the conventional way */
            TI_DbgPrint(MAX_TRACE, ("Freeing packet 0x%p\n",
                                    IPPacket->NdisPacket));
            FreeNdisPacket(IPPacket->NdisPacket);
        }
    }

    /* Check if we have a pool-allocated header */
    if (!IPPacket->MappedHeader && IPPacket->Header)
    {
        /* Free it */
        TI_DbgPrint(MAX_TRACE, ("Freeing header: 0x%p\n",
                                IPPacket->Header));
        ExFreePoolWithTag(IPPacket->Header,
                          PACKET_BUFFER_TAG);
    }
}

VOID FreeIF(
    PVOID Object)
/*
 * FUNCTION: Frees an interface object
 * ARGUMENTS:
 *     Object = Pointer to an interface structure
 */
{
    ExFreePoolWithTag(Object, IP_INTERFACE_TAG);
}

PIP_PACKET IPInitializePacket(
    PIP_PACKET IPPacket,
    ULONG Type)
/*
 * FUNCTION: Creates an IP packet object
 * ARGUMENTS:
 *     Type = Type of IP packet
 * RETURNS:
 *     Pointer to the created IP packet. NULL if there was not enough free resources.
 */
{
    RtlZeroMemory(IPPacket, sizeof(IP_PACKET));

    IPPacket->Free     = DeinitializePacket;
    IPPacket->Type     = Type;

    return IPPacket;
}


VOID NTAPI IPTimeoutDpcFn(PKDPC Dpc,
                          PVOID DeferredContext,
                          PVOID SystemArgument1,
                          PVOID SystemArgument2)
/*
 * FUNCTION: Timeout DPC
 * ARGUMENTS:
 *     Dpc             = Pointer to our DPC object
 *     DeferredContext = Pointer to context information (unused)
 *     SystemArgument1 = Unused
 *     SystemArgument2 = Unused
 * NOTES:
 *     This routine is dispatched once in a while to do maintenance jobs
 */
{
    IpTimerExpirations++;

    if ((IpTimerExpirations % 10) == 0)
    {
        LogActiveObjects();
    }

    /* Check if datagram fragments have taken too long to assemble */
    IPDatagramReassemblyTimeout();

    /* Clean possible outdated cached neighbor addresses */
    NBTimeout();
}


VOID IPDispatchProtocol(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: IP protocol dispatcher
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 * NOTES:
 *     This routine examines the IP header and passes the packet on to the
 *     right upper level protocol receive handler
 */
{
    UINT Protocol;
    IP_ADDRESS SrcAddress;

    switch (IPPacket->Type) {
    case IP_ADDRESS_V4:
        Protocol = ((PIPv4_HEADER)(IPPacket->Header))->Protocol;
        AddrInitIPv4(&SrcAddress, ((PIPv4_HEADER)(IPPacket->Header))->SrcAddr);
        break;
    case IP_ADDRESS_V6:
        /* FIXME: IPv6 adresses not supported */
        TI_DbgPrint(MIN_TRACE, ("IPv6 datagram discarded.\n"));
        return;
    default:
        TI_DbgPrint(MIN_TRACE, ("Unrecognized datagram discarded.\n"));
        return;
    }

    NBResetNeighborTimeout(&SrcAddress);

    if (Protocol < IP_PROTOCOL_TABLE_SIZE)
    {
       /* Call the appropriate protocol handler */
       (*ProtocolTable[Protocol])(Interface, IPPacket);
    }
}


PIP_INTERFACE IPCreateInterface(
    PLLIP_BIND_INFO BindInfo)
/*
 * FUNCTION: Creates an IP interface
 * ARGUMENTS:
 *     BindInfo = Pointer to link layer to IP binding information
 * RETURNS:
 *     Pointer to IP_INTERFACE structure, NULL if there was
 *     not enough free resources
 */
{
    PIP_INTERFACE IF;

    TI_DbgPrint(DEBUG_IP, ("Called. BindInfo (0x%X).\n", BindInfo));

#if DBG
    if (BindInfo->Address) {
        PUCHAR A = BindInfo->Address;
        TI_DbgPrint(DEBUG_IP, ("Interface address (%02X %02X %02X %02X %02X %02X).\n",
            A[0], A[1], A[2], A[3], A[4], A[5]));
    }
#endif

    IF = ExAllocatePoolWithTag(NonPagedPool, sizeof(IP_INTERFACE),
                               IP_INTERFACE_TAG);
    if (!IF) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

	RtlZeroMemory(IF, sizeof(IP_INTERFACE));

    IF->Free       = FreeIF;
    IF->Context    = BindInfo->Context;
    IF->HeaderSize = BindInfo->HeaderSize;
    IF->MinFrameSize = BindInfo->MinFrameSize;
    IF->Address       = BindInfo->Address;
    IF->AddressLength = BindInfo->AddressLength;
    IF->Transmit      = BindInfo->Transmit;

	IF->Unicast.Type = IP_ADDRESS_V4;
	IF->PointToPoint.Type = IP_ADDRESS_V4;
	IF->Netmask.Type = IP_ADDRESS_V4;
	IF->Broadcast.Type = IP_ADDRESS_V4;

    TcpipInitializeSpinLock(&IF->Lock);

    IF->TCPContext = ExAllocatePool
	( NonPagedPool, sizeof(struct netif));
    if (!IF->TCPContext) {
        ExFreePoolWithTag(IF, IP_INTERFACE_TAG);
        return NULL;
    }

    TCPRegisterInterface(IF);

#ifdef __NTDRIVER__
    InsertTDIInterfaceEntity( IF );
#endif

    return IF;
}


VOID IPDestroyInterface(
    PIP_INTERFACE IF)
/*
 * FUNCTION: Destroys an IP interface
 * ARGUMENTS:
 *     IF = Pointer to interface to destroy
 */
{
    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X).\n", IF));

#ifdef __NTDRIVER__
    RemoveTDIInterfaceEntity( IF );
#endif

    TCPUnregisterInterface(IF);

    ExFreePool(IF->TCPContext);
    ExFreePoolWithTag(IF, IP_INTERFACE_TAG);
}

VOID IPAddInterfaceRoute( PIP_INTERFACE IF ) {
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_ADDRESS NetworkAddress;

    /* Add a permanent neighbor for this NTE */
    NCE = NBAddNeighbor(IF, &IF->Unicast,
			IF->Address, IF->AddressLength,
			NUD_PERMANENT, 0);
    if (!NCE) {
	TI_DbgPrint(MIN_TRACE, ("Could not create NCE.\n"));
        return;
    }

    AddrWidenAddress( &NetworkAddress, &IF->Unicast, &IF->Netmask );

    if (!RouterAddRoute(&NetworkAddress, &IF->Netmask, NCE, 1)) {
	TI_DbgPrint(MIN_TRACE, ("Could not add route due to insufficient resources.\n"));
    }

    /* Send a gratuitous ARP packet to update the route caches of
     * other computers */
    if (IF != Loopback)
       ARPTransmit(NULL, NULL, IF);

    TCPUpdateInterfaceIPInformation(IF);
}

BOOLEAN IPRegisterInterface(
    PIP_INTERFACE IF)
/*
 * FUNCTION: Registers an IP interface with IP layer
 * ARGUMENTS:
 *     IF = Pointer to interface to register
 * RETURNS;
 *     TRUE if interface was successfully registered, FALSE if not
 */
{
    KIRQL OldIrql;
    UINT ChosenIndex = 0;
    BOOLEAN IndexHasBeenChosen;
    IF_LIST_ITER(Interface);

    TI_DbgPrint(MID_TRACE, ("Called. IF (0x%X).\n", IF));

    TcpipAcquireSpinLock(&IF->Lock, &OldIrql);

    /* Choose an index */
    do {
        IndexHasBeenChosen = TRUE;
        ForEachInterface(Interface) {
            if( Interface->Index == ChosenIndex ) {
                ChosenIndex++;
                IndexHasBeenChosen = FALSE;
            }
        } EndFor(Interface);
    } while( !IndexHasBeenChosen );

    IF->Index = ChosenIndex;

    /* Add interface to the global interface list */
    TcpipInterlockedInsertTailList(&InterfaceListHead,
				   &IF->ListEntry,
				   &InterfaceListLock);

    TcpipReleaseSpinLock(&IF->Lock, OldIrql);

    return TRUE;
}

VOID IPRemoveInterfaceRoute( PIP_INTERFACE IF ) {
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_ADDRESS GeneralRoute;

    NCE = NBLocateNeighbor(&IF->Unicast, IF);
    if (NCE)
    {
       TI_DbgPrint(DEBUG_IP,("Removing interface Addr %s\n", A2S(&IF->Unicast)));
       TI_DbgPrint(DEBUG_IP,("                   Mask %s\n", A2S(&IF->Netmask)));

       AddrWidenAddress(&GeneralRoute,&IF->Unicast,&IF->Netmask);

       RouterRemoveRoute(&GeneralRoute, &IF->Unicast);

       NBRemoveNeighbor(NCE);
    }
}

VOID IPUnregisterInterface(
    PIP_INTERFACE IF)
/*
 * FUNCTION: Unregisters an IP interface with IP layer
 * ARGUMENTS:
 *     IF = Pointer to interface to unregister
 */
{
    KIRQL OldIrql3;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X).\n", IF));

    IPRemoveInterfaceRoute( IF );

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql3);
    RemoveEntryList(&IF->ListEntry);
    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql3);
}


VOID DefaultProtocolHandler(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Default handler for Internet protocols
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 */
{
    TI_DbgPrint(MID_TRACE, ("[IF %x] Packet of unknown Internet protocol "
			    "discarded.\n", Interface));

    Interface->Stats.InDiscardedUnknownProto++;
}


VOID IPRegisterProtocol(
    UINT ProtocolNumber,
    IP_PROTOCOL_HANDLER Handler)
/*
 * FUNCTION: Registers a handler for an IP protocol number
 * ARGUMENTS:
 *     ProtocolNumber = Internet Protocol number for which to register handler
 *     Handler        = Pointer to handler to be called when a packet is received
 * NOTES:
 *     To unregister a protocol handler, call this function with Handler = NULL
 */
{
    if (ProtocolNumber >= IP_PROTOCOL_TABLE_SIZE) {
        TI_DbgPrint(MIN_TRACE, ("Protocol number is out of range (%d).\n", ProtocolNumber));
        return;
    }

    ProtocolTable[ProtocolNumber] = Handler ? Handler : DefaultProtocolHandler;
}


NTSTATUS IPStartup(PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Initializes the IP subsystem
 * ARGUMENTS:
 *     RegistryPath = Our registry node for configuration parameters
 * RETURNS:
 *     Status of operation
 */
{
    UINT i;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* Initialize lookaside lists */
    ExInitializeNPagedLookasideList(
      &IPDRList,                      /* Lookaside list */
	    NULL,                           /* Allocate routine */
	    NULL,                           /* Free routine */
	    0,                              /* Flags */
	    sizeof(IPDATAGRAM_REASSEMBLY),  /* Size of each entry */
	    DATAGRAM_REASSEMBLY_TAG,        /* Tag */
	    0);                             /* Depth */

    ExInitializeNPagedLookasideList(
      &IPFragmentList,                /* Lookaside list */
	    NULL,                           /* Allocate routine */
	    NULL,                           /* Free routine */
	    0,                              /* Flags */
	    sizeof(IP_FRAGMENT),            /* Size of each entry */
	    DATAGRAM_FRAGMENT_TAG,          /* Tag */
	    0);                             /* Depth */

    ExInitializeNPagedLookasideList(
      &IPHoleList,                    /* Lookaside list */
	    NULL,                           /* Allocate routine */
	    NULL,                           /* Free routine */
	    0,                              /* Flags */
	    sizeof(IPDATAGRAM_HOLE),        /* Size of each entry */
	    DATAGRAM_HOLE_TAG,              /* Tag */
	    0);                             /* Depth */

    /* Start routing subsystem */
    RouterStartup();

    /* Start neighbor cache subsystem */
    NBStartup();

    /* Fill the protocol dispatch table with pointers
       to the default protocol handler */
    for (i = 0; i < IP_PROTOCOL_TABLE_SIZE; i++)
        IPRegisterProtocol(i, DefaultProtocolHandler);

    /* Initialize NTE list and protecting lock */
    InitializeListHead(&NetTableListHead);
    TcpipInitializeSpinLock(&NetTableListLock);

    /* Initialize reassembly list and protecting lock */
    InitializeListHead(&ReassemblyListHead);
    TcpipInitializeSpinLock(&ReassemblyListLock);

    IPInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS IPShutdown(
    VOID)
/*
 * FUNCTION: Shuts down the IP subsystem
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

    if (!IPInitialized)
        return STATUS_SUCCESS;

    /* Shutdown neighbor cache subsystem */
    NBShutdown();

    /* Shutdown routing subsystem */
    RouterShutdown();

    IPFreeReassemblyList();

    /* Destroy lookaside lists */
    ExDeleteNPagedLookasideList(&IPHoleList);
    ExDeleteNPagedLookasideList(&IPDRList);
    ExDeleteNPagedLookasideList(&IPFragmentList);

    IPInitialized = FALSE;

    return STATUS_SUCCESS;
}

/* EOF */
