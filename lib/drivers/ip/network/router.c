/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/router.c
 * PURPOSE:     IP routing subsystem
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:
 *   This file holds authoritative routing information.
 *   Information queries on the route table should be handled here.
 *   This information should always override the route cache info.
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"


LIST_ENTRY FIBListHead;
KSPIN_LOCK FIBLock;

void RouterDumpRoutes() {
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PFIB_ENTRY Current;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(DEBUG_ROUTER,("Dumping Routes\n"));

    CurrentEntry = FIBListHead.Flink;
    while (CurrentEntry != &FIBListHead) {
        NextEntry = CurrentEntry->Flink;
	Current = CONTAINING_RECORD(CurrentEntry, FIB_ENTRY, ListEntry);

        NCE   = Current->Router;

	TI_DbgPrint(DEBUG_ROUTER,("Examining FIBE %x\n", Current));
	TI_DbgPrint(DEBUG_ROUTER,("... NetworkAddress %s\n", A2S(&Current->NetworkAddress)));
	TI_DbgPrint(DEBUG_ROUTER,("... NCE->Address . %s\n", A2S(&NCE->Address)));

	CurrentEntry = NextEntry;
    }

    TI_DbgPrint(DEBUG_ROUTER,("Dumping Routes ... Done\n"));
}

VOID FreeFIB(
    PVOID Object)
/*
 * FUNCTION: Frees an forward information base object
 * ARGUMENTS:
 *     Object = Pointer to an forward information base structure
 */
{
    PoolFreeBuffer(Object);
}


VOID DestroyFIBE(
    PFIB_ENTRY FIBE)
/*
 * FUNCTION: Destroys an forward information base entry
 * ARGUMENTS:
 *     FIBE = Pointer to FIB entry
 * NOTES:
 *     The forward information base lock must be held when called
 */
{
    TI_DbgPrint(DEBUG_ROUTER, ("Called. FIBE (0x%X).\n", FIBE));

    /* Unlink the FIB entry from the list */
    RemoveEntryList(&FIBE->ListEntry);

    /* And free the FIB entry */
    FreeFIB(FIBE);
}


VOID DestroyFIBEs(
    VOID)
/*
 * FUNCTION: Destroys all forward information base entries
 * NOTES:
 *     The forward information base lock must be held when called
 */
{
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PFIB_ENTRY Current;

    /* Search the list and remove every FIB entry we find */
    CurrentEntry = FIBListHead.Flink;
    while (CurrentEntry != &FIBListHead) {
        NextEntry = CurrentEntry->Flink;
	Current = CONTAINING_RECORD(CurrentEntry, FIB_ENTRY, ListEntry);
        /* Destroy the FIB entry */
        DestroyFIBE(Current);
        CurrentEntry = NextEntry;
    }
}


UINT CountFIBs() {
    UINT FibCount = 0;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;

    /* Search the list and remove every FIB entry we find */
    CurrentEntry = FIBListHead.Flink;
    while (CurrentEntry != &FIBListHead) {
        NextEntry = CurrentEntry->Flink;
        CurrentEntry = NextEntry;
	FibCount++;
    }

    return FibCount;
}


UINT CopyFIBs( PFIB_ENTRY Target ) {
    UINT FibCount = 0;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PFIB_ENTRY Current;

    /* Search the list and remove every FIB entry we find */
    CurrentEntry = FIBListHead.Flink;
    while (CurrentEntry != &FIBListHead) {
        NextEntry = CurrentEntry->Flink;
	Current = CONTAINING_RECORD(CurrentEntry, FIB_ENTRY, ListEntry);
	Target[FibCount] = *Current;
        CurrentEntry = NextEntry;
	FibCount++;
    }

    return FibCount;
}


UINT CommonPrefixLength(
    PIP_ADDRESS Address1,
    PIP_ADDRESS Address2)
/*
 * FUNCTION: Computes the length of the longest prefix common to two addresses
 * ARGUMENTS:
 *     Address1 = Pointer to first address
 *     Address2 = Pointer to second address
 * NOTES:
 *     The two addresses must be of the same type
 * RETURNS:
 *     Length of longest common prefix
 */
{
    PUCHAR Addr1, Addr2;
    UINT Size;
    UINT i, j;
    UINT Bitmask;

    TI_DbgPrint(DEBUG_ROUTER, ("Called. Address1 (0x%X)  Address2 (0x%X).\n", Address1, Address2));

    /*TI_DbgPrint(DEBUG_ROUTER, ("Target  (%s) \n", A2S(Address1)));*/
    /*TI_DbgPrint(DEBUG_ROUTER, ("Adapter (%s).\n", A2S(Address2)));*/

    if (Address1->Type == IP_ADDRESS_V4)
        Size = sizeof(IPv4_RAW_ADDRESS);
    else
        Size = sizeof(IPv6_RAW_ADDRESS);

    Addr1 = (PUCHAR)&Address1->Address.IPv4Address;
    Addr2 = (PUCHAR)&Address2->Address.IPv4Address;

    /* Find first non-matching byte */
    for (i = 0; i < Size && Addr1[i] == Addr2[i]; i++);
    if( i == Size ) return 8 * i;

    /* Find first non-matching bit */
    Bitmask = 0x80;
    for (j = 0; (Addr1[i] & Bitmask) == (Addr2[i] & Bitmask); j++)
        Bitmask >>= 1;

    TI_DbgPrint(DEBUG_ROUTER, ("Returning %d\n", 8 * i + j));

    return 8 * i + j;
}


PFIB_ENTRY RouterAddRoute(
    PIP_ADDRESS NetworkAddress,
    PIP_ADDRESS Netmask,
    PNEIGHBOR_CACHE_ENTRY Router,
    UINT Metric)
/*
 * FUNCTION: Adds a route to the Forward Information Base (FIB)
 * ARGUMENTS:
 *     NetworkAddress = Pointer to address of network
 *     Netmask        = Pointer to netmask of network
 *     Router         = Pointer to NCE of router to use
 *     Metric         = Cost of this route
 * RETURNS:
 *     Pointer to FIB entry if the route was added, NULL if not
 * NOTES:
 *     The FIB entry references the NetworkAddress, Netmask and
 *     the NCE of the router. The caller is responsible for providing
 *     these references
 */
{
    PFIB_ENTRY FIBE;

    TI_DbgPrint(DEBUG_ROUTER, ("Called. NetworkAddress (0x%X)  Netmask (0x%X) "
        "Router (0x%X)  Metric (%d).\n", NetworkAddress, Netmask, Router, Metric));

    TI_DbgPrint(DEBUG_ROUTER, ("NetworkAddress (%s)  Netmask (%s)  Router (%s).\n",
			       A2S(NetworkAddress),
			       A2S(Netmask),
			       A2S(&Router->Address)));

    FIBE = PoolAllocateBuffer(sizeof(FIB_ENTRY));
    if (!FIBE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    INIT_TAG(Router, TAG('R','O','U','T'));

    RtlCopyMemory( &FIBE->NetworkAddress, NetworkAddress,
		   sizeof(FIBE->NetworkAddress) );
    RtlCopyMemory( &FIBE->Netmask, Netmask,
		   sizeof(FIBE->Netmask) );
    FIBE->Router         = Router;
    FIBE->Metric         = Metric;

    /* Add FIB to the forward information base */
    TcpipInterlockedInsertTailList(&FIBListHead, &FIBE->ListEntry, &FIBLock);

    return FIBE;
}


PNEIGHBOR_CACHE_ENTRY RouterGetRoute(PIP_ADDRESS Destination)
/*
 * FUNCTION: Finds a router to use to get to Destination
 * ARGUMENTS:
 *     Destination = Pointer to destination address (NULL means don't care)
 * RETURNS:
 *     Pointer to NCE for router, NULL if none was found
 * NOTES:
 *     If found the NCE is referenced
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PFIB_ENTRY Current;
    UCHAR State, BestState = 0;
    UINT Length, BestLength = 0, MaskLength;
    PNEIGHBOR_CACHE_ENTRY NCE, BestNCE = NULL;

    TI_DbgPrint(DEBUG_ROUTER, ("Called. Destination (0x%X)\n", Destination));

    TI_DbgPrint(DEBUG_ROUTER, ("Destination (%s)\n", A2S(Destination)));

    TcpipAcquireSpinLock(&FIBLock, &OldIrql);

    CurrentEntry = FIBListHead.Flink;
    while (CurrentEntry != &FIBListHead) {
        NextEntry = CurrentEntry->Flink;
	    Current = CONTAINING_RECORD(CurrentEntry, FIB_ENTRY, ListEntry);

        NCE   = Current->Router;
        State = NCE->State;

	Length = CommonPrefixLength(Destination, &Current->NetworkAddress);
	MaskLength = AddrCountPrefixBits(&Current->Netmask);

	TI_DbgPrint(DEBUG_ROUTER,("This-Route: %s (Sharing %d bits)\n",
				  A2S(&NCE->Address), Length));

	if(Length >= MaskLength && (Length > BestLength || !BestLength)) {
	    /* This seems to be a better router */
	    BestNCE    = NCE;
	    BestLength = Length;
	    BestState  = State;
	    TI_DbgPrint(DEBUG_ROUTER,("Route selected\n"));
	}

        CurrentEntry = NextEntry;
    }

    TcpipReleaseSpinLock(&FIBLock, OldIrql);

    if( BestNCE ) {
	TI_DbgPrint(DEBUG_ROUTER,("Routing to %s\n", A2S(&BestNCE->Address)));
    } else {
	TI_DbgPrint(DEBUG_ROUTER,("Packet won't be routed\n"));
    }

    return BestNCE;
}

PNEIGHBOR_CACHE_ENTRY RouteGetRouteToDestination(PIP_ADDRESS Destination)
/*
 * FUNCTION: Locates an RCN describing a route to a destination address
 * ARGUMENTS:
 *     Destination = Pointer to destination address to find route to
 *     RCN         = Address of pointer to an RCN
 * RETURNS:
 *     Status of operation
 * NOTES:
 *     The RCN is referenced for the caller. The caller is responsible
 *     for dereferencing it after use
 */
{
    PNEIGHBOR_CACHE_ENTRY NCE = NULL;
    PIP_INTERFACE Interface;

    TI_DbgPrint(DEBUG_RCACHE, ("Called. Destination (0x%X)\n", Destination));

    TI_DbgPrint(DEBUG_RCACHE, ("Destination (%s)\n", A2S(Destination)));

#if 0
    TI_DbgPrint(MIN_TRACE, ("Displaying tree (before).\n"));
    PrintTree(RouteCache);
#endif

    /* Check if the destination is on-link */
    Interface = FindOnLinkInterface(Destination);
    if (Interface) {
	/* The destination address is on-link. Check our neighbor cache */
	NCE = NBFindOrCreateNeighbor(Interface, Destination);
    } else {
	/* Destination is not on any subnets we're on. Find a router to use */
	NCE = RouterGetRoute(Destination);
    }

    if( NCE )
	TI_DbgPrint(DEBUG_ROUTER,("Interface->MTU: %d\n", NCE->Interface->MTU));

    return NCE;
}

NTSTATUS RouterRemoveRoute(PIP_ADDRESS Target, PIP_ADDRESS Router)
/*
 * FUNCTION: Removes a route from the Forward Information Base (FIB)
 * ARGUMENTS:
 *     Target: The machine or network targeted by the route
 *     Router: The router used to pass the packet to the destination
 *
 * Searches the FIB and removes a route matching the indicated parameters.
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PFIB_ENTRY Current;
    BOOLEAN Found = FALSE;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(DEBUG_ROUTER, ("Called\n"));
    TI_DbgPrint(DEBUG_ROUTER, ("Deleting Route From: %s\n", A2S(Router)));
    TI_DbgPrint(DEBUG_ROUTER, ("                 To: %s\n", A2S(Target)));

    TcpipAcquireSpinLock(&FIBLock, &OldIrql);

    RouterDumpRoutes();

    CurrentEntry = FIBListHead.Flink;
    while (CurrentEntry != &FIBListHead) {
        NextEntry = CurrentEntry->Flink;
	Current = CONTAINING_RECORD(CurrentEntry, FIB_ENTRY, ListEntry);

        NCE   = Current->Router;

	if( AddrIsEqual( &Current->NetworkAddress, Target ) &&
	    AddrIsEqual( &NCE->Address, Router ) ) {
	    Found = TRUE;
	    break;
	}

	Current = NULL;
        CurrentEntry = NextEntry;
    }

    if( Found ) {
	TI_DbgPrint(DEBUG_ROUTER, ("Deleting route\n"));
	DestroyFIBE( Current );
    }

    RouterDumpRoutes();

    TcpipReleaseSpinLock(&FIBLock, OldIrql);

    TI_DbgPrint(DEBUG_ROUTER, ("Leaving\n"));

    return Found ? STATUS_NO_SUCH_FILE : STATUS_SUCCESS;
}


PFIB_ENTRY RouterCreateRoute(
    PIP_ADDRESS NetworkAddress,
    PIP_ADDRESS Netmask,
    PIP_ADDRESS RouterAddress,
    PIP_INTERFACE Interface,
    UINT Metric)
/*
 * FUNCTION: Creates a route with IPv4 addresses as parameters
 * ARGUMENTS:
 *     NetworkAddress = Address of network
 *     Netmask        = Netmask of network
 *     RouterAddress  = Address of router to use
 *     NTE            = Pointer to NTE to use
 *     Metric         = Cost of this route
 * RETURNS:
 *     Pointer to FIB entry if the route was created, NULL if not.
 *     The FIB entry references the NTE. The caller is responsible
 *     for providing this reference
 */
{
    KIRQL OldIrql;
    PFIB_ENTRY FIBE;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PFIB_ENTRY Current;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TcpipAcquireSpinLock(&FIBLock, &OldIrql);

    CurrentEntry = FIBListHead.Flink;
    while (CurrentEntry != &FIBListHead) {
        NextEntry = CurrentEntry->Flink;
	Current = CONTAINING_RECORD(CurrentEntry, FIB_ENTRY, ListEntry);

        NCE   = Current->Router;

	if( AddrIsEqual(NetworkAddress, &Current->NetworkAddress) &&
	    AddrIsEqual(Netmask, &Current->Netmask) ) {
	    TI_DbgPrint(DEBUG_ROUTER,("Attempting to add duplicate route to %s\n", A2S(NetworkAddress)));
	    TcpipReleaseSpinLock(&FIBLock, OldIrql);
	    return NULL;
	}

	CurrentEntry = NextEntry;
    }

    TcpipReleaseSpinLock(&FIBLock, OldIrql);

    /* The NCE references RouterAddress. The NCE is referenced for us */
    NCE = NBFindOrCreateNeighbor(Interface, RouterAddress);

    if (!NCE) {
        /* Not enough free resources */
        return NULL;
    }

    FIBE = RouterAddRoute(NetworkAddress, Netmask, NCE, Metric);
    if (!FIBE) {
        /* Not enough free resources */
        NBRemoveNeighbor(NCE);
    }

    return FIBE;
}


NTSTATUS RouterStartup(
    VOID)
/*
 * FUNCTION: Initializes the routing subsystem
 * RETURNS:
 *     Status of operation
 */
{
    TI_DbgPrint(DEBUG_ROUTER, ("Called.\n"));

    /* Initialize the Forward Information Base */
    InitializeListHead(&FIBListHead);
    TcpipInitializeSpinLock(&FIBLock);

    return STATUS_SUCCESS;
}


NTSTATUS RouterShutdown(
    VOID)
/*
 * FUNCTION: Shuts down the routing subsystem
 * RETURNS:
 *     Status of operation
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_ROUTER, ("Called.\n"));

    /* Clear Forward Information Base */
    TcpipAcquireSpinLock(&FIBLock, &OldIrql);
    DestroyFIBEs();
    TcpipReleaseSpinLock(&FIBLock, OldIrql);

    return STATUS_SUCCESS;
}

/* EOF */
