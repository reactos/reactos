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


LIST_ENTRY InterfaceListHead;
KSPIN_LOCK InterfaceListLock;
LIST_ENTRY NetTableListHead;
KSPIN_LOCK NetTableListLock;
UINT MaxLLHeaderSize; /* Largest maximum header size */
UINT MinLLFrameSize;  /* Largest minimum frame size */
BOOLEAN IPInitialized = FALSE;
NPAGED_LOOKASIDE_LIST IPPacketList;
/* Work around calling timer at Dpc level */

IP_PROTOCOL_HANDLER ProtocolTable[IP_PROTOCOL_TABLE_SIZE];


VOID FreePacket(
    PVOID Object)
/*
 * FUNCTION: Frees an IP packet object
 * ARGUMENTS:
 *     Object = Pointer to an IP packet structure
 */
{
    TcpipFreeToNPagedLookasideList(&IPPacketList, Object);
}


VOID DontFreePacket(
    PVOID Object)
/*
 * FUNCTION: Do nothing for when the IPPacket struct is part of another
 * ARGUMENTS:
 *     Object = Pointer to an IP packet structure
 */
{
}


VOID FreeADE(
    PVOID Object)
/*
 * FUNCTION: Frees an address entry object
 * ARGUMENTS:
 *     Object = Pointer to an address entry structure
 */
{
    exFreePool(Object);
}


VOID FreeNTE(
    PVOID Object)
/*
 * FUNCTION: Frees a net table entry object
 * ARGUMENTS:
 *     Object = Pointer to an net table entry structure
 */
{
    exFreePool(Object);
}


VOID FreeIF(
    PVOID Object)
/*
 * FUNCTION: Frees an interface object
 * ARGUMENTS:
 *     Object = Pointer to an interface structure
 */
{
    exFreePool(Object);
}


PADDRESS_ENTRY CreateADE(
    PIP_INTERFACE IF,    
    PIP_ADDRESS Address,
    UCHAR Type,
    PNET_TABLE_ENTRY NTE)
/*
 * FUNCTION: Creates an address entry and binds it to an interface
 * ARGUMENTS:
 *     IF      = Pointer to interface
 *     Address = Pointer to referenced interface address
 *     Type    = Type of address (ADE_*)
 *     NTE     = Pointer to net table entry
 * RETURNS:
 *     Pointer to ADE, NULL if there was not enough free resources
 * NOTES:
 *     The interface lock must be held when called. The address entry
 *     retains a reference to the provided address and NTE. The caller
 *     is responsible for referencing the these before calling.
 *     As long as you have referenced an ADE you can safely use the
 *     address and NTE as the ADE references both
 */
{
    PADDRESS_ENTRY ADE;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  Address (0x%X)  Type (0x%X)  NTE (0x%X).\n",
        IF, Address, Type, NTE));

    TI_DbgPrint(DEBUG_IP, ("Address (%s)  NTE (%s).\n",
        A2S(Address), A2S(NTE->Address)));

    /* Allocate space for an ADE and set it up */
    ADE = exAllocatePool(NonPagedPool, sizeof(ADDRESS_ENTRY));
    if (!ADE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    INIT_TAG(ADE, TAG('A','D','E',' '));
    ADE->Free     = FreeADE;
    ADE->NTE      = NTE;
    ADE->Type     = Type;
    RtlCopyMemory(&ADE->Address,Address,sizeof(ADE->Address));

    /* Add ADE to the list on the interface */
    InsertTailList(&IF->ADEListHead, &ADE->ListEntry);

    return ADE;
}


VOID DestroyADE(
    PIP_INTERFACE IF,
    PADDRESS_ENTRY ADE)
/*
 * FUNCTION: Destroys an address entry
 * ARGUMENTS:
 *     IF  = Pointer to interface
 *     ADE = Pointer to address entry
 * NOTES:
 *     The interface lock must be held when called
 */
{
    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  ADE (0x%X).\n", IF, ADE));

    TI_DbgPrint(DEBUG_IP, ("ADE (%s).\n", ADE->Address));

    /* Unlink the address entry from the list */
    RemoveEntryList(&ADE->ListEntry);

    /* And free the ADE */
    FreeADE(ADE);
}


VOID DestroyADEs(
    PIP_INTERFACE IF)
/*
 * FUNCTION: Destroys all address entries on an interface
 * ARGUMENTS:
 *     IF  = Pointer to interface
 * NOTES:
 *     The interface lock must be held when called
 */
{
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PADDRESS_ENTRY Current;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X).\n", IF));

    /* Search the list and remove every ADE we find */
    CurrentEntry = IF->ADEListHead.Flink;
    while (CurrentEntry != &IF->ADEListHead) {
        NextEntry = CurrentEntry->Flink;
  	    Current = CONTAINING_RECORD(CurrentEntry, ADDRESS_ENTRY, ListEntry);
        /* Destroy the ADE */
        DestroyADE(IF, Current);
        CurrentEntry = NextEntry;
    }
}


PIP_PACKET IPCreatePacket(ULONG Type)
/*
 * FUNCTION: Creates an IP packet object
 * ARGUMENTS:
 *     Type = Type of IP packet
 * RETURNS:
 *     Pointer to the created IP packet. NULL if there was not enough free resources.
 */
{
  PIP_PACKET IPPacket;

  IPPacket = TcpipAllocateFromNPagedLookasideList(&IPPacketList);
  if (!IPPacket)
    return NULL;

    /* FIXME: Is this needed? */
  RtlZeroMemory(IPPacket, sizeof(IP_PACKET));

  INIT_TAG(IPPacket, TAG('I','P','K','T'));

  IPPacket->Free       = FreePacket;
  IPPacket->Type       = Type;
  IPPacket->HeaderSize = 20;

  return IPPacket;
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
    /* FIXME: Is this needed? */
    RtlZeroMemory(IPPacket, sizeof(IP_PACKET));
    
    INIT_TAG(IPPacket, TAG('I','P','K','T'));
    
    IPPacket->Free     = DontFreePacket;
    IPPacket->Type     = Type;
    
    return IPPacket;
}


PNET_TABLE_ENTRY IPCreateNTE(
    PIP_INTERFACE IF,
    PIP_ADDRESS Address,
    UINT PrefixLength)
/*
 * FUNCTION: Creates a net table entry and binds it to an interface
 * ARGUMENTS:
 *     IF           = Pointer to interface
 *     Address      = Pointer to interface address
 *     PrefixLength = Length of prefix
 * RETURNS:
 *     Pointer to NTE, NULL if there was not enough free resources
 * NOTES:
 *     The interface lock must be held when called.
 *     The net table entry retains a reference to the interface and
 *     the provided address. The caller is responsible for providing
 *     these references
 */
{
    PNET_TABLE_ENTRY NTE;
    PADDRESS_ENTRY ADE;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  Address (0x%X)  PrefixLength (%d).\n", IF, Address, PrefixLength));

    TI_DbgPrint(DEBUG_IP, ("Address (%s).\n", A2S(Address)));

    /* Allocate room for an NTE */
    NTE = exAllocatePool(NonPagedPool, sizeof(NET_TABLE_ENTRY));
    if (!NTE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    INIT_TAG(NTE, TAG('N','T','E',' '));
    INIT_TAG(Address, TAG('A','D','R','S'));

    NTE->Free = FreeNTE;

    NTE->Interface = IF;

    NTE->Address = Address;

    /* Create an address entry and add it to the list */
    ADE = CreateADE(IF, NTE->Address, ADE_UNICAST, NTE);
    if (!ADE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        exFreePool(NTE);
        return NULL;
    }

    /* Create a prefix list entry for unicast address */
    NTE->PLE = CreatePLE(IF, NTE->Address, PrefixLength);
    if (!NTE->PLE) {
        DestroyADE(IF, ADE);
        exFreePool(NTE);
        return NULL;
    }

    /* Add NTE to the list on the interface */
    InsertTailList(&IF->NTEListHead, &NTE->IFListEntry);

    /* Add NTE to the global net table list */
    TcpipInterlockedInsertTailList(&NetTableListHead, &NTE->NTListEntry, &NetTableListLock);

    return NTE;
}


VOID DestroyNTE(
    PIP_INTERFACE IF,
    PNET_TABLE_ENTRY NTE)
/*
 * FUNCTION: Destroys a net table entry
 * ARGUMENTS:
 *     IF  = Pointer to interface
 *     NTE = Pointer to net table entry
 * NOTES:
 *     The net table list lock must be held when called
 *     The interface lock must be held when called
 */
{
    KIRQL OldIrql;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  NTE (0x%X).\n", IF, NTE));

    TI_DbgPrint(DEBUG_IP, ("NTE (%s).\n", NTE->Address));

    /* Invalidate the prefix list entry for this NTE */
    TcpipAcquireSpinLock(&PrefixListLock, &OldIrql);
    DestroyPLE(NTE->PLE);
    TcpipReleaseSpinLock(&PrefixListLock, OldIrql);

    /* Remove NTE from the interface list */
    RemoveEntryList(&NTE->IFListEntry);
    /* Remove NTE from the net table list */

/* TODO: DEBUG: removed by RobD to prevent failure when testing under bochs 6 sept 2002.

    RemoveEntryList(&NTE->NTListEntry);

 */

    /* And free the NTE */
    exFreePool(NTE);
}


VOID DestroyNTEs(
    PIP_INTERFACE IF)
/*
 * FUNCTION: Destroys all net table entries on an interface
 * ARGUMENTS:
 *     IF  = Pointer to interface
 * NOTES:
 *     The net table list lock must be held when called
 *     The interface lock may be held when called
 */
{
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PNET_TABLE_ENTRY Current;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X).\n", IF));

    /* Search the list and remove every NTE we find */
    CurrentEntry = IF->NTEListHead.Flink;
    while (CurrentEntry != &IF->NTEListHead) {
        NextEntry = CurrentEntry->Flink;
	      Current = CONTAINING_RECORD(CurrentEntry, NET_TABLE_ENTRY, IFListEntry);
        /* Destroy the NTE */
        DestroyNTE(IF, Current);
        CurrentEntry = NextEntry;
    }
}


PNET_TABLE_ENTRY IPLocateNTEOnInterface(
    PIP_INTERFACE IF,
    PIP_ADDRESS Address,
    PUINT AddressType)
/*
 * FUNCTION: Locates an NTE on an interface
 * ARGUMENTS:
 *     IF          = Pointer to interface
 *     Address     = Pointer to IP address
 *     AddressType = Address of type of IP address
 * NOTES:
 *     If found, the NTE is referenced for the caller. The caller is
 *     responsible for dereferencing after use
 * RETURNS:
 *     Pointer to net table entry, NULL if none was found
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PADDRESS_ENTRY Current;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  Address (%s)  AddressType (0x%X).\n",
        IF, A2S(Address), AddressType));

    if( !IF ) return NULL;

    TcpipAcquireSpinLock(&IF->Lock, &OldIrql);

    /* Search the list and return the NTE if found */
    CurrentEntry = IF->ADEListHead.Flink;

    if (CurrentEntry == &IF->ADEListHead) {
        TI_DbgPrint(DEBUG_IP, ("NTE list is empty!!!\n"));
    }

    while (CurrentEntry != &IF->ADEListHead) {
	      Current = CONTAINING_RECORD(CurrentEntry, ADDRESS_ENTRY, ListEntry);
        if (AddrIsEqual(Address, &Current->Address)) {
            *AddressType = Current->Type;
            TcpipReleaseSpinLock(&IF->Lock, OldIrql);
            return Current->NTE;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    TcpipReleaseSpinLock(&IF->Lock, OldIrql);

    return NULL;
}


PNET_TABLE_ENTRY IPLocateNTE(
    PIP_ADDRESS Address,
    PUINT AddressType)
/*
 * FUNCTION: Locates an NTE for the network Address is on 
 * ARGUMENTS:
 *     Address     = Pointer to an address to find associated NTE of
 *     AddressType = Address of address type
 * NOTES:
 *     If found the NTE is referenced for the caller. The caller is
 *     responsible for dereferencing after use
 * RETURNS:
 *     Pointer to NTE if the address was found, NULL if not.
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PNET_TABLE_ENTRY Current;
    PNET_TABLE_ENTRY NTE;

//    TI_DbgPrint(DEBUG_IP, ("Called. Address (0x%X)  AddressType (0x%X).\n",
//        Address, AddressType));

//    TI_DbgPrint(DEBUG_IP, ("Address (%s).\n", A2S(Address)));

    TcpipAcquireSpinLock(&NetTableListLock, &OldIrql);

    /* Search the list and return the NTE if found */
    CurrentEntry = NetTableListHead.Flink;
    while (CurrentEntry != &NetTableListHead) {
	      Current = CONTAINING_RECORD(CurrentEntry, NET_TABLE_ENTRY, NTListEntry);
        NTE = IPLocateNTEOnInterface(Current->Interface, Address, AddressType);
        if (NTE) {
            TcpipReleaseSpinLock(&NetTableListLock, OldIrql);
            return NTE;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    TcpipReleaseSpinLock(&NetTableListLock, OldIrql);

    return NULL;
}


PADDRESS_ENTRY IPLocateADE(
    PIP_ADDRESS Address,
    UINT AddressType)
/*
 * FUNCTION: Locates an ADE for the address
 * ARGUMENTS:
 *     Address     = Pointer to an address to find associated ADE of
 *     AddressType = Type of address
 * RETURNS:
 *     Pointer to ADE if the address was found, NULL if not.
 * NOTES:
 *     If found the ADE is referenced for the caller. The caller is
 *     responsible for dereferencing after use
 */
{
    KIRQL OldIrql;
    IF_LIST_ITER(CurrentIF);
    ADE_LIST_ITER(CurrentADE);

//    TI_DbgPrint(DEBUG_IP, ("Called. Address (0x%X)  AddressType (0x%X).\n",
//        Address, AddressType));

//    TI_DbgPrint(DEBUG_IP, ("Address (%s).\n", A2S(Address)));

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    /* Search the interface list */
    ForEachInterface(CurrentIF) {
        /* Search the address entry list and return the ADE if found */
	ForEachADE(CurrentIF->ADEListHead,CurrentADE) {
            if ((AddrIsEqual(Address, &CurrentADE->Address)) && 
                (CurrentADE->Type == AddressType)) {
                TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);
                return CurrentADE;
            }
        } EndFor(CurrentADE);
    } EndFor(CurrentIF);

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return NULL;
}


PADDRESS_ENTRY IPGetDefaultADE(
    UINT AddressType)
/*
 * FUNCTION: Returns a default address entry
 * ARGUMENTS:
 *     AddressType = Type of address
 * RETURNS:
 *     Pointer to ADE if found, NULL if not.
 * NOTES:
 *     Loopback interface is only considered if it is the only interface.
 *     If found, the address entry is referenced
 */
{
    KIRQL OldIrql;
    ADE_LIST_ITER(CurrentADE);
    IF_LIST_ITER(CurrentIF);
    BOOLEAN LoopbackIsRegistered = FALSE;

    TI_DbgPrint(DEBUG_IP, ("Called. AddressType (0x%X).\n", AddressType));

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql);

    /* Search the interface list */
    ForEachInterface(CurrentIF) {
        if (CurrentIF != Loopback) {
            /* Search the address entry list and return the first appropriate ADE found */
	    TI_DbgPrint(DEBUG_IP,("Checking interface %x\n", CurrentIF));
	    ForEachADE(CurrentIF->ADEListHead,CurrentADE) {
                if (CurrentADE->Type == AddressType) {
                    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);
                    return CurrentADE;
                }
	    } EndFor(CurrentADE);
        } else
            LoopbackIsRegistered = TRUE;
    } EndFor(CurrentIF);

    /* No address was found. Use loopback interface if available */
    if (LoopbackIsRegistered) {
	ForEachADE(CurrentIF->ADEListHead,CurrentADE) {
            if (CurrentADE->Type == AddressType) {
                TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);
                return CurrentADE;
            }
        } EndFor(CurrentADE);
    }

    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql);

    return NULL;
}

void STDCALL IPTimeout( PVOID Context ) {
    /* Check if datagram fragments have taken too long to assemble */
    IPDatagramReassemblyTimeout();
    
    /* Clean possible outdated cached neighbor addresses */
    NBTimeout();
    
    /* Call upper layer timeout routines */
    TCPTimeout();
}


VOID IPDispatchProtocol(
    PNET_TABLE_ENTRY NTE,
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

    switch (IPPacket->Type) {
    case IP_ADDRESS_V4:
        Protocol = ((PIPv4_HEADER)(IPPacket->Header))->Protocol;
        break;
    case IP_ADDRESS_V6:
        /* FIXME: IPv6 adresses not supported */
        TI_DbgPrint(MIN_TRACE, ("IPv6 datagram discarded.\n"));
        return;
    default:
        Protocol = 0;
    }

    /* Call the appropriate protocol handler */
    (*ProtocolTable[Protocol])(NTE, IPPacket);
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

#ifdef DBG
    if (BindInfo->Address) {
        PUCHAR A = BindInfo->Address;
        TI_DbgPrint(DEBUG_IP, ("Interface address (%02X %02X %02X %02X %02X %02X).\n",
            A[0], A[1], A[2], A[3], A[4], A[5]));
    }
#endif

    IF = exAllocatePool(NonPagedPool, sizeof(IP_INTERFACE));
    if (!IF) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    INIT_TAG(IF, TAG('F','A','C','E'));

    IF->Free       = FreeIF;
    IF->Context    = BindInfo->Context;
    IF->HeaderSize = BindInfo->HeaderSize;
	  if (IF->HeaderSize > MaxLLHeaderSize)
	  	MaxLLHeaderSize = IF->HeaderSize;

    IF->MinFrameSize = BindInfo->MinFrameSize;
	  if (IF->MinFrameSize > MinLLFrameSize)
  		MinLLFrameSize = IF->MinFrameSize;

    IF->MTU           = BindInfo->MTU;
    IF->Address       = BindInfo->Address;
    IF->AddressLength = BindInfo->AddressLength;
    IF->Transmit      = BindInfo->Transmit;

    InitializeListHead(&IF->ADEListHead);
    InitializeListHead(&IF->NTEListHead);

    TcpipInitializeSpinLock(&IF->Lock);

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
    KIRQL OldIrql1;
    KIRQL OldIrql2;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X).\n", IF));

#ifdef __NTDRIVER__
    RemoveTDIInterfaceEntity( IF );
#endif

    TcpipAcquireSpinLock(&NetTableListLock, &OldIrql1);
    TcpipAcquireSpinLock(&IF->Lock, &OldIrql2);
    DestroyADEs(IF);
    DestroyNTEs(IF);
    TcpipReleaseSpinLock(&IF->Lock, OldIrql2);
    TcpipReleaseSpinLock(&NetTableListLock, OldIrql1);

    exFreePool(IF);
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
    PLIST_ENTRY CurrentEntry;
    PNET_TABLE_ENTRY Current;
    PROUTE_CACHE_NODE RCN;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(MID_TRACE, ("Called. IF (0x%X).\n", IF));

    TcpipAcquireSpinLock(&IF->Lock, &OldIrql);

    /* Add routes to all NTEs on this interface */
    CurrentEntry = IF->NTEListHead.Flink;
    while (CurrentEntry != &IF->NTEListHead) {
	    Current = CONTAINING_RECORD(CurrentEntry, NET_TABLE_ENTRY, IFListEntry);

        /* Add a permanent neighbor for this NTE */
        NCE = NBAddNeighbor(IF, Current->Address, IF->Address,
            IF->AddressLength, NUD_PERMANENT);
        if (!NCE) {
            TI_DbgPrint(MIN_TRACE, ("Could not create NCE.\n"));
            TcpipReleaseSpinLock(&IF->Lock, OldIrql);
            return FALSE;
        }

        /* NCE is already referenced */
        if (!RouterAddRoute(Current->Address, &Current->PLE->Prefix, NCE, 1)) {
            TI_DbgPrint(MIN_TRACE, ("Could not add route due to insufficient resources.\n"));
        }

        RCN = RouteAddRouteToDestination(Current->Address, Current, IF, NCE);
        if (!RCN) {
            TI_DbgPrint(MIN_TRACE, ("Could not create RCN.\n"));
            TcpipReleaseSpinLock(&IF->Lock, OldIrql);
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    /* Add interface to the global interface list */
    ASSERT(&IF->ListEntry);
    TcpipInterlockedInsertTailList(&InterfaceListHead, 
				   &IF->ListEntry, 
				   &InterfaceListLock);

    /* Allow TCP to hang some configuration on this interface */
    IF->TCPContext = TCPPrepareInterface( IF );

    TcpipReleaseSpinLock(&IF->Lock, OldIrql);

    return TRUE;
}


VOID IPUnregisterInterface(
    PIP_INTERFACE IF)
/*
 * FUNCTION: Unregisters an IP interface with IP layer
 * ARGUMENTS:
 *     IF = Pointer to interface to unregister
 */
{
    KIRQL OldIrql1;
    KIRQL OldIrql2;
    KIRQL OldIrql3;
    PLIST_ENTRY CurrentEntry;
    PNET_TABLE_ENTRY Current;
    PNEIGHBOR_CACHE_ENTRY NCE;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X).\n", IF));

    TcpipAcquireSpinLock(&NetTableListLock, &OldIrql1);
    TcpipAcquireSpinLock(&IF->Lock, &OldIrql2);

    /* Remove routes to all NTEs on this interface */
    CurrentEntry = IF->NTEListHead.Flink;
    while (CurrentEntry != &IF->NTEListHead) {
        Current = CONTAINING_RECORD(CurrentEntry, NET_TABLE_ENTRY, IFListEntry);

        /* Remove NTE from global net table list */
        RemoveEntryList(&Current->NTListEntry);

        /* Remove all references from route cache to NTE */
        RouteInvalidateNTE(Current);

        /* Remove permanent NCE, but first we have to find it */
        NCE = NBLocateNeighbor(Current->Address);
        if (NCE)
            NBRemoveNeighbor(NCE);

        CurrentEntry = CurrentEntry->Flink;
    }

    TcpipAcquireSpinLock(&InterfaceListLock, &OldIrql3);
    /* Ouch...three spinlocks acquired! Fortunately
       we don't unregister interfaces very often */
    RemoveEntryList(&IF->ListEntry);
    TcpipReleaseSpinLock(&InterfaceListLock, OldIrql3);

    TcpipReleaseSpinLock(&IF->Lock, OldIrql2);
    TcpipReleaseSpinLock(&NetTableListLock, OldIrql1);
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
#ifdef DBG
    if (ProtocolNumber >= IP_PROTOCOL_TABLE_SIZE)
        TI_DbgPrint(MIN_TRACE, ("Protocol number is out of range (%d).\n", ProtocolNumber));
#endif

    ProtocolTable[ProtocolNumber] = Handler;
}


VOID DefaultProtocolHandler(
    PNET_TABLE_ENTRY NTE,
    PIP_PACKET IPPacket)
/*
 * FUNCTION: Default handler for Internet protocols
 * ARGUMENTS:
 *     NTE      = Pointer to net table entry which the packet was received on
 *     IPPacket = Pointer to an IP packet that was received
 */
{
    TI_DbgPrint(MID_TRACE, ("Packet of unknown Internet protocol discarded.\n"));
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

    MaxLLHeaderSize = 0;
    MinLLFrameSize  = 0;

    /* Initialize lookaside lists */
    ExInitializeNPagedLookasideList(
      &IPDRList,                      /* Lookaside list */
	    NULL,                           /* Allocate routine */
	    NULL,                           /* Free routine */
	    0,                              /* Flags */
	    sizeof(IPDATAGRAM_REASSEMBLY),  /* Size of each entry */
	    TAG('I','P','D','R'),           /* Tag */
	    0);                             /* Depth */

    ExInitializeNPagedLookasideList(
      &IPPacketList,                  /* Lookaside list */
	    NULL,                           /* Allocate routine */
	    NULL,                           /* Free routine */
	    0,                              /* Flags */
	    sizeof(IP_PACKET),              /* Size of each entry */
	    TAG('I','P','P','K'),           /* Tag */
	    0);                             /* Depth */

    ExInitializeNPagedLookasideList(
      &IPFragmentList,                /* Lookaside list */
	    NULL,                           /* Allocate routine */
	    NULL,                           /* Free routine */
	    0,                              /* Flags */
	    sizeof(IP_FRAGMENT),            /* Size of each entry */
	    TAG('I','P','F','G'),           /* Tag */
	    0);                             /* Depth */

    ExInitializeNPagedLookasideList(
      &IPHoleList,                    /* Lookaside list */
	    NULL,                           /* Allocate routine */
	    NULL,                           /* Free routine */
	    0,                              /* Flags */
	    sizeof(IPDATAGRAM_HOLE),        /* Size of each entry */
	    TAG('I','P','H','L'),           /* Tag */
	    0);                             /* Depth */

    /* Start routing subsystem */
    RouterStartup();

    /* Start route cache subsystem */
    RouteStartup();

    /* Start neighbor cache subsystem */
    NBStartup();

    /* Fill the protocol dispatch table with pointers
       to the default protocol handler */
    for (i = 0; i < IP_PROTOCOL_TABLE_SIZE; i++)
        IPRegisterProtocol(i, DefaultProtocolHandler);

    /* Register network level protocol receive handlers */
    IPRegisterProtocol(IPPROTO_ICMP, ICMPReceive);

    /* Initialize NTE list and protecting lock */
    InitializeListHead(&NetTableListHead);
    TcpipInitializeSpinLock(&NetTableListLock);

    /* Initialize reassembly list and protecting lock */
    InitializeListHead(&ReassemblyListHead);
    TcpipInitializeSpinLock(&ReassemblyListLock);

    InitPLE();

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

    /* Shutdown route cache subsystem */
    RouteShutdown();

    /* Shutdown routing subsystem */
    RouterShutdown();

    IPFreeReassemblyList();

    /* Clear prefix list */
    DestroyPLEs();

    /* Destroy lookaside lists */
    ExDeleteNPagedLookasideList(&IPHoleList);
    ExDeleteNPagedLookasideList(&IPDRList);
    ExDeleteNPagedLookasideList(&IPPacketList);
    ExDeleteNPagedLookasideList(&IPFragmentList);

    IPInitialized = FALSE;

    return STATUS_SUCCESS;
}

/* EOF */
