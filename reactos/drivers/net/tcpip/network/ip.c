/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        network/ip.c
 * PURPOSE:     Internet Protocol module
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <ip.h>
#include <loopback.h>
#include <neighbor.h>
#include <receive.h>
#include <address.h>
#include <route.h>
#include <icmp.h>
#include <pool.h>


KTIMER IPTimer;
KDPC IPTimeoutDpc;
LIST_ENTRY InterfaceListHead;
KSPIN_LOCK InterfaceListLock;
LIST_ENTRY NetTableListHead;
KSPIN_LOCK NetTableListLock;
LIST_ENTRY PrefixListHead;
KSPIN_LOCK PrefixListLock;
UINT MaxLLHeaderSize; /* Largest maximum header size */
UINT MinLLFrameSize;  /* Largest minimum frame size */
BOOLEAN IPInitialized = FALSE;

IP_PROTOCOL_HANDLER ProtocolTable[IP_PROTOCOL_TABLE_SIZE];


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
    ADE = PoolAllocateBuffer(sizeof(ADDRESS_ENTRY));
    if (!ADE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    ADE->RefCount = 1;
    ADE->NTE      = NTE;
    ADE->Type     = Type;
    ADE->Address  = Address;

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

    /* Dereference the address */
    DereferenceObject(ADE->Address);

    /* Dereference the NTE */
    DereferenceObject(ADE->NTE);

#ifdef DBG
    ADE->RefCount--;

    if (ADE->RefCount != 0) {
        TI_DbgPrint(MIN_TRACE, ("Address entry at (0x%X) has (%d) references (should be 0).\n", ADE, ADE->RefCount));
    }
#endif

    /* And free the ADE */
    PoolFreeBuffer(ADE);
    TI_DbgPrint(MIN_TRACE, ("Check.\n"));
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


PPREFIX_LIST_ENTRY CreatePLE(
    PIP_INTERFACE IF,
    PIP_ADDRESS Prefix,
    UINT Length)
/*
 * FUNCTION: Creates a prefix list entry and binds it to an interface
 * ARGUMENTS:
 *     IF     = Pointer to interface
 *     Prefix = Pointer to prefix
 *     Length = Length of prefix
 * RETURNS:
 *     Pointer to PLE, NULL if there was not enough free resources
 * NOTES:
 *     The prefix list entry retains a reference to the interface and
 *     the provided address.  The caller is responsible for providing
 *     these references
 */
{
    PPREFIX_LIST_ENTRY PLE;

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  Prefix (0x%X)  Length (%d).\n", IF, Prefix, Length));

    TI_DbgPrint(DEBUG_IP, ("Prefix (%s).\n", A2S(Prefix)));

    /* Allocate space for an PLE and set it up */
    PLE = PoolAllocateBuffer(sizeof(PREFIX_LIST_ENTRY));
    if (!PLE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    PLE->RefCount     = 1;
    PLE->Interface    = IF;
    PLE->Prefix       = Prefix;
    PLE->PrefixLength = Length;

    /* Add PLE to the global prefix list */
    ExInterlockedInsertTailList(&PrefixListHead, &PLE->ListEntry, &PrefixListLock);

    return PLE;
}


VOID DestroyPLE(
    PPREFIX_LIST_ENTRY PLE)
/*
 * FUNCTION: Destroys an prefix list entry
 * ARGUMENTS:
 *     PLE = Pointer to prefix list entry
 * NOTES:
 *     The prefix list lock must be held when called
 */
{
    TI_DbgPrint(DEBUG_IP, ("Called. PLE (0x%X).\n", PLE));

    TI_DbgPrint(DEBUG_IP, ("PLE (%s).\n", PLE->Prefix));

    /* Unlink the prefix list entry from the list */
    RemoveEntryList(&PLE->ListEntry);

    /* Dereference the address */
    DereferenceObject(PLE->Prefix);

    /* Dereference the interface */
    DereferenceObject(PLE->Interface);

#ifdef DBG
    PLE->RefCount--;

    if (PLE->RefCount != 0) {
        TI_DbgPrint(MIN_TRACE, ("Prefix list entry at (0x%X) has (%d) references (should be 0).\n", PLE, PLE->RefCount));
    }
#endif

    /* And free the PLE */
    PoolFreeBuffer(PLE);
}


VOID DestroyPLEs(
    VOID)
/*
 * FUNCTION: Destroys all prefix list entries
 */
{
    KIRQL OldIrql;
    PLIST_ENTRY CurrentEntry;
    PLIST_ENTRY NextEntry;
    PPREFIX_LIST_ENTRY Current;

    TI_DbgPrint(DEBUG_IP, ("Called.\n"));

    KeAcquireSpinLock(&PrefixListLock, &OldIrql);

    /* Search the list and remove every PLE we find */
    CurrentEntry = PrefixListHead.Flink;
    while (CurrentEntry != &PrefixListHead) {
        NextEntry = CurrentEntry->Flink;
	    Current = CONTAINING_RECORD(CurrentEntry, PREFIX_LIST_ENTRY, ListEntry);
        /* Destroy the PLE */
        DestroyPLE(Current);
        CurrentEntry = NextEntry;
    }
    KeReleaseSpinLock(&PrefixListLock, OldIrql);
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
    NTE = PoolAllocateBuffer(sizeof(NET_TABLE_ENTRY));
    if (!NTE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    NTE->Interface = IF;

    /* One reference is for beeing alive and one reference is for the ADE */
    NTE->RefCount = 2;

    NTE->Address = Address;
    /* One reference is for NTE, one reference is given to the
       address entry, and one reference is given to the prefix
       list entry */
    ReferenceObject(Address);
    ReferenceObject(Address);
    ReferenceObject(Address);

    /* Create an address entry and add it to the list */
    ADE = CreateADE(IF, NTE->Address, ADE_UNICAST, NTE);
    if (!ADE) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        PoolFreeBuffer(NTE);
        return NULL;
    }

    /* Create a prefix list entry for unicast address */
    NTE->PLE = CreatePLE(IF, NTE->Address, PrefixLength);
    if (!NTE->PLE) {
        DestroyADE(IF, ADE);
        PoolFreeBuffer(NTE);
        return NULL;
    }

    /* Reference the interface for the prefix list entry */
    ReferenceObject(IF);

    /* Add NTE to the list on the interface */
    InsertTailList(&IF->NTEListHead, &NTE->IFListEntry);

    /* Add NTE to the global net table list */
    ExInterlockedInsertTailList(&NetTableListHead, &NTE->NTListEntry, &NetTableListLock);

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
    KeAcquireSpinLock(&PrefixListLock, &OldIrql);
    DestroyPLE(NTE->PLE);
    KeReleaseSpinLock(&PrefixListLock, OldIrql);

    /* Remove NTE from the interface list */
    RemoveEntryList(&NTE->IFListEntry);
    /* Remove NTE from the net table list */
    RemoveEntryList(&NTE->NTListEntry);
    /* Dereference the objects that are referenced */
    DereferenceObject(NTE->Address);
    DereferenceObject(NTE->Interface);
#ifdef DBG
    NTE->RefCount--;

    if (NTE->RefCount != 0) {
        TI_DbgPrint(MIN_TRACE, ("Net table entry at (0x%X) has (%d) references (should be 0).\n", NTE, NTE->RefCount));
    }
#endif
    /* And free the NTE */
    PoolFreeBuffer(NTE);
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

//    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X)  Address (0x%X)  AddressType (0x%X).\n",
//        IF, Address, AddressType));

//    TI_DbgPrint(DEBUG_IP, ("Address (%s)  AddressType (0x%X).\n", A2S(Address)));

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    /* Search the list and return the NTE if found */
    CurrentEntry = IF->ADEListHead.Flink;
    while (CurrentEntry != &IF->ADEListHead) {
	    Current = CONTAINING_RECORD(CurrentEntry, ADDRESS_ENTRY, ListEntry);
        if (AddrIsEqual(Address, Current->Address)) {
            ReferenceObject(Current->NTE);
            *AddressType = Current->Type;
            KeReleaseSpinLock(&IF->Lock, OldIrql);
            return Current->NTE;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&IF->Lock, OldIrql);

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

    KeAcquireSpinLock(&NetTableListLock, &OldIrql);

    /* Search the list and return the NTE if found */
    CurrentEntry = NetTableListHead.Flink;
    while (CurrentEntry != &NetTableListHead) {
	    Current = CONTAINING_RECORD(CurrentEntry, NET_TABLE_ENTRY, NTListEntry);
        NTE = IPLocateNTEOnInterface(Current->Interface, Address, AddressType);
        if (NTE) {
            ReferenceObject(NTE);
            KeReleaseSpinLock(&NetTableListLock, OldIrql);
            return NTE;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&NetTableListLock, OldIrql);

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
    PLIST_ENTRY CurrentIFEntry;
    PLIST_ENTRY CurrentADEEntry;
    PIP_INTERFACE CurrentIF;
    PADDRESS_ENTRY CurrentADE;

//    TI_DbgPrint(DEBUG_IP, ("Called. Address (0x%X)  AddressType (0x%X).\n",
//        Address, AddressType));

//    TI_DbgPrint(DEBUG_IP, ("Address (%s).\n", A2S(Address)));

    KeAcquireSpinLock(&InterfaceListLock, &OldIrql);

    /* Search the interface list */
    CurrentIFEntry = InterfaceListHead.Flink;
    while (CurrentIFEntry != &InterfaceListHead) {
	    CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);

        /* Search the address entry list and return the ADE if found */
        CurrentADEEntry = CurrentIF->ADEListHead.Flink;
        while (CurrentADEEntry != &CurrentIF->ADEListHead) {
	        CurrentADE = CONTAINING_RECORD(CurrentADEEntry, ADDRESS_ENTRY, ListEntry);
            if ((AddrIsEqual(Address, CurrentADE->Address)) && 
                (CurrentADE->Type == AddressType)) {
                ReferenceObject(CurrentADE);
                KeReleaseSpinLock(&InterfaceListLock, OldIrql);
                return CurrentADE;
            }
            CurrentADEEntry = CurrentADEEntry->Flink;
        }
        CurrentIFEntry = CurrentIFEntry->Flink;
    }

    KeReleaseSpinLock(&InterfaceListLock, OldIrql);

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
    PLIST_ENTRY CurrentIFEntry;
    PLIST_ENTRY CurrentADEEntry;
    PIP_INTERFACE CurrentIF;
    PADDRESS_ENTRY CurrentADE;
    BOOLEAN LoopbackIsRegistered = FALSE;

    TI_DbgPrint(DEBUG_IP, ("Called. AddressType (0x%X).\n", AddressType));

    KeAcquireSpinLock(&InterfaceListLock, &OldIrql);

    /* Search the interface list */
    CurrentIFEntry = InterfaceListHead.Flink;
    while (CurrentIFEntry != &InterfaceListHead) {
	    CurrentIF = CONTAINING_RECORD(CurrentIFEntry, IP_INTERFACE, ListEntry);

        if (CurrentIF != Loopback) {
            /* Search the address entry list and return the first appropriate ADE found */
            CurrentADEEntry = CurrentIF->ADEListHead.Flink;
            while (CurrentADEEntry != &CurrentIF->ADEListHead) {
	            CurrentADE = CONTAINING_RECORD(CurrentADEEntry, ADDRESS_ENTRY, ListEntry);
                if (CurrentADE->Type == AddressType)
                    ReferenceObject(CurrentADE);
                    KeReleaseSpinLock(&InterfaceListLock, OldIrql);
                    return CurrentADE;
                }
                CurrentADEEntry = CurrentADEEntry->Flink;
        } else
            LoopbackIsRegistered = TRUE;
        CurrentIFEntry = CurrentIFEntry->Flink;
    }

    /* No address was found. Use loopback interface if available */
    if (LoopbackIsRegistered) {
        CurrentADEEntry = Loopback->ADEListHead.Flink;
        while (CurrentADEEntry != &Loopback->ADEListHead) {
	        CurrentADE = CONTAINING_RECORD(CurrentADEEntry, ADDRESS_ENTRY, ListEntry);
            if (CurrentADE->Type == AddressType) {
                ReferenceObject(CurrentADE);
                KeReleaseSpinLock(&InterfaceListLock, OldIrql);
                return CurrentADE;
            }
            CurrentADEEntry = CurrentADEEntry->Flink;
        }
    }

    KeReleaseSpinLock(&InterfaceListLock, OldIrql);

    return NULL;
}


VOID IPTimeout(
    PKDPC Dpc,
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
 *     This routine is dispatched once in a while to do maintainance jobs
 */
{
    /* Check if datagram fragments have taken too long to assemble */
    IPDatagramReassemblyTimeout();

    /* Clean possible outdated cached neighbor addresses */
    NBTimeout();
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

    IF = PoolAllocateBuffer(sizeof(IP_INTERFACE));
    if (!IF) {
        TI_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        return NULL;
    }

    IF->RefCount   = 1;
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

    KeInitializeSpinLock(&IF->Lock);

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

    KeAcquireSpinLock(&NetTableListLock, &OldIrql1);
    KeAcquireSpinLock(&IF->Lock, &OldIrql2);
    DestroyADEs(IF);
    DestroyNTEs(IF);
    KeReleaseSpinLock(&IF->Lock, OldIrql2);
    KeReleaseSpinLock(&NetTableListLock, OldIrql1);

#ifdef DBG
    IF->RefCount--;

    if (IF->RefCount != 0) {
        TI_DbgPrint(MIN_TRACE, ("Interface at (0x%X) has (%d) references (should be 0).\n", IF, IF->RefCount));
    }
#endif
    PoolFreeBuffer(IF);
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

    TI_DbgPrint(DEBUG_IP, ("Called. IF (0x%X).\n", IF));

    KeAcquireSpinLock(&IF->Lock, &OldIrql);

    /* Add routes to all NTEs on this interface */
    CurrentEntry = IF->NTEListHead.Flink;
    while (CurrentEntry != &IF->NTEListHead) {
	    Current = CONTAINING_RECORD(CurrentEntry, NET_TABLE_ENTRY, IFListEntry);

        /* Add a permanent neighbor for this NTE */
        ReferenceObject(Current->Address);
        NCE = NBAddNeighbor(IF, Current->Address, IF->Address,
            IF->AddressLength, NUD_PERMANENT);
        if (!NCE) {
            TI_DbgPrint(MIN_TRACE, ("Could not create NCE.\n"));
            DereferenceObject(Current->Address);
            KeReleaseSpinLock(&IF->Lock, OldIrql);
            return FALSE;
        }
#if 1
        /* Reference objects for forward information base */
        ReferenceObject(Current->Address);
        ReferenceObject(Current->PLE->Prefix);
        ReferenceObject(Current);
        /* NCE is already referenced */
        if (!RouterAddRoute(Current->Address, Current->PLE->Prefix, Current, NCE, 1)) {
            TI_DbgPrint(MIN_TRACE, ("Could not add route due to insufficient resources.\n"));
            DereferenceObject(Current->Address);
            DereferenceObject(Current->PLE->Prefix);
            DereferenceObject(Current);
            DereferenceObject(NCE);
        }
#else
        RCN = RouteAddRouteToDestination(Current->Address, Current, IF, NCE);
        if (!RCN) {
            TI_DbgPrint(MIN_TRACE, ("Could not create RCN.\n"));
            DereferenceObject(Current->Address);
            KeReleaseSpinLock(&IF->Lock, OldIrql);
            return FALSE;
        }
        /* Don't need this any more since the route cache references the NCE */
        DereferenceObject(NCE);
#endif
        CurrentEntry = CurrentEntry->Flink;
    }

    /* Add interface to the global interface list */
    ExInterlockedInsertTailList(&InterfaceListHead, &IF->ListEntry, &InterfaceListLock);

    KeReleaseSpinLock(&IF->Lock, OldIrql);

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

    KeAcquireSpinLock(&NetTableListLock, &OldIrql1);
    KeAcquireSpinLock(&IF->Lock, &OldIrql2);

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
        if (NCE) {
            DereferenceObject(NCE);
            NBRemoveNeighbor(NCE);
        }

        CurrentEntry = CurrentEntry->Flink;
    }

    KeAcquireSpinLock(&InterfaceListLock, &OldIrql3);
    /* Ouch...three spinlocks acquired! Fortunately
       we don't unregister interfaces very often */
    RemoveEntryList(&IF->ListEntry);
    KeReleaseSpinLock(&InterfaceListLock, OldIrql3);

    KeReleaseSpinLock(&IF->Lock, OldIrql2);
    KeReleaseSpinLock(&NetTableListLock, OldIrql1);
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


NTSTATUS IPStartup(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath)
/*
 * FUNCTION: Initializes the IP subsystem
 * ARGUMENTS:
 *     DriverObject = Pointer to a driver object for this driver
 *     RegistryPath = Our registry node for configuration parameters
 * RETURNS:
 *     Status of operation
 */
{
    UINT i;
    LARGE_INTEGER DueTime;

    TI_DbgPrint(MAX_TRACE, ("Called.\n"));

	MaxLLHeaderSize = 0;
    MinLLFrameSize  = 0;

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
    KeInitializeSpinLock(&NetTableListLock);

    /* Initialize reassembly list and protecting lock */
    InitializeListHead(&ReassemblyListHead);
    KeInitializeSpinLock(&ReassemblyListLock);

    /* Initialize the prefix list and protecting lock */
    InitializeListHead(&PrefixListHead);
    KeInitializeSpinLock(&PrefixListLock);

    /* Initialize our periodic timer and its associated DPC object. When the
       timer expires, the IPTimeout deferred procedure call (DPC) is queued */
    KeInitializeDpc(&IPTimeoutDpc, IPTimeout, NULL);
    KeInitializeTimer(&IPTimer);

    /* Start the periodic timer with an initial and periodic
       relative expiration time of IP_TIMEOUT milliseconds */
    DueTime.QuadPart = -(LONGLONG)IP_TIMEOUT * 10000;
    KeSetTimerEx(&IPTimer, DueTime, IP_TIMEOUT, &IPTimeoutDpc);

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

    /* Cancel timer */
    KeCancelTimer(&IPTimer);

    /* Shutdown neighbor cache subsystem */
    NBShutdown();

    /* Shutdown route cache subsystem */
    RouteShutdown();

    /* Shutdown routing subsystem */
    RouterShutdown();

    IPFreeReassemblyList();

    /* Clear prefix list */
    DestroyPLEs();

    IPInitialized = FALSE;

    return STATUS_SUCCESS;
}

/* EOF */
