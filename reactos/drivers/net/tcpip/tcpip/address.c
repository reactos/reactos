/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/address.c
 * PURPOSE:     Routines for handling addresses
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */
#include <tcpip.h>
#include <address.h>
#include <pool.h>
#include <ip.h>

#ifdef DBG

CHAR A2SStr[128];

PCHAR A2S(
    PIP_ADDRESS Address)
/*
 * FUNCTION: Convert an IP address to a string (for debugging)
 * ARGUMENTS:
 *     Address = Pointer to an IP address structure
 * RETURNS:
 *     Pointer to buffer with string representation of IP address
 */
{
    ULONG ip;
    CHAR b[10];
    PCHAR p;

    p = A2SStr;

    if (!Address) {
        TI_DbgPrint(MIN_TRACE, ("NULL address given.\n"));
        strcpy(p, "(NULL)");
        return p;
    }

    switch (Address->Type) {
        case IP_ADDRESS_V4:
            ip = DN2H(Address->Address.IPv4Address);
            sprintf(p, "%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
            break;

        case IP_ADDRESS_V6:
            /* FIXME: IPv6 is not supported */
            strcpy(p, "(IPv6 address not supported)");
            break;
    }
    return p;
}

#endif /* DBG */


VOID IPAddressFree(
    PVOID Object)
/*
 * FUNCTION: Frees an IP_ADDRESS object
 * ARGUMENTS:
 *     Object = Pointer to an IP address structure
 * RETURNS:
 *     Nothing
 */
{
    ExFreePool(Object);
}


BOOLEAN AddrIsUnspecified(
    PIP_ADDRESS Address)
/*
 * FUNCTION: Return wether IP address is an unspecified address
 * ARGUMENTS:
 *     Address = Pointer to an IP address structure
 * RETURNS:
 *     TRUE if the IP address is an unspecified address, FALSE if not
 */
{
    switch (Address->Type) {
        case IP_ADDRESS_V4:
            return (Address->Address.IPv4Address == 0);

        case IP_ADDRESS_V6:
        /* FIXME: IPv6 is not supported */
        default:
            return FALSE;
    }
}


/*
 * FUNCTION: Extract IP address from TDI address structure
 * ARGUMENTS:
 *     AddrList = Pointer to transport address list to extract from
 *     Address  = Address of a pointer to where an IP address is stored
 *     Port     = Pointer to where port number is stored
 *     Cache    = Address of pointer to a cached address (updated on return)
 * RETURNS:
 *     Status of operation
 */
NTSTATUS AddrGetAddress(
    PTRANSPORT_ADDRESS AddrList,
    PIP_ADDRESS *Address,
    PUSHORT Port,
    PIP_ADDRESS *Cache)
{
    PTA_ADDRESS CurAddr;
    INT i;

    /* We can only use IP addresses. Search the list until we find one */
    CurAddr = AddrList->Address;

    for (i = 0; i < AddrList->TAAddressCount; i++) {
        switch (CurAddr->AddressType) {
        case TDI_ADDRESS_TYPE_IP:
            if (CurAddr->AddressLength >= TDI_ADDRESS_LENGTH_IP) {
                /* This is an IPv4 address */
                PIP_ADDRESS IPAddress;
                PTDI_ADDRESS_IP ValidAddr = (PTDI_ADDRESS_IP)CurAddr->Address;

                *Port = ValidAddr->sin_port;

                if ((Cache) && (*Cache)) {
                    if (((*Cache)->Type == IP_ADDRESS_V4) &&
                        ((*Cache)->Address.IPv4Address == ValidAddr->in_addr)) {
                        *Address = *Cache;
                        return STATUS_SUCCESS;
                    } else {
                        /* Release the cached address as we cannot use it this time */
                        DereferenceObject(*Cache);
                        *Cache = NULL;
                    }
                }

                IPAddress = ExAllocatePool(NonPagedPool, sizeof(IP_ADDRESS));
                if (IPAddress) {
                    AddrInitIPv4(IPAddress, ValidAddr->in_addr);
                    *Address = IPAddress;

                    /* Update address cache */
                    if (Cache) {
                      *Cache = IPAddress;
                      ReferenceObject(*Cache);
                    }
                    return STATUS_SUCCESS;
                } else
                    return STATUS_INSUFFICIENT_RESOURCES;
            } else
                return STATUS_INVALID_ADDRESS;
        default:
            /* This is an unsupported address type.
               Skip it and go to the next in the list */
            CurAddr = (PTA_ADDRESS)((ULONG_PTR)CurAddr->Address + CurAddr->AddressLength);
        }
    }

    return STATUS_INVALID_ADDRESS;
}


/*
 * FUNCTION: Extract IP address from TDI address structure
 * ARGUMENTS:
 *     TdiAddress = Pointer to transport address list to extract from
 *     Address    = Address of a pointer to where an IP address is stored
 *     Port       = Pointer to where port number is stored
 * RETURNS:
 *     Status of operation
 */
NTSTATUS AddrBuildAddress(
    PTA_ADDRESS TdiAddress,
    PIP_ADDRESS *Address,
    PUSHORT Port)
{
  PTDI_ADDRESS_IP ValidAddr;
  PIP_ADDRESS IPAddress;

  if (TdiAddress->AddressType != TDI_ADDRESS_TYPE_IP)
    return STATUS_INVALID_ADDRESS;

  if (TdiAddress->AddressLength >= TDI_ADDRESS_LENGTH_IP)
    return STATUS_INVALID_ADDRESS;

  ValidAddr = (PTDI_ADDRESS_IP)TdiAddress->Address;

  IPAddress = ExAllocatePool(NonPagedPool, sizeof(IP_ADDRESS));
  if (!IPAddress)
    return STATUS_INSUFFICIENT_RESOURCES;

  AddrInitIPv4(IPAddress, ValidAddr->in_addr);
  *Address = IPAddress;
  *Port = ValidAddr->sin_port;

  return STATUS_SUCCESS;
}


/*
 * FUNCTION: Returns wether two addresses are equal
 * ARGUMENTS:
 *     Address1 = Pointer to first address
 *     Address2 = Pointer to last address
 * RETURNS:
 *     TRUE if Address1 = Address2, FALSE if not
 */
BOOLEAN AddrIsEqual(
    PIP_ADDRESS Address1,
    PIP_ADDRESS Address2)
{
    if (Address1->Type != Address2->Type)
        return FALSE;

    switch (Address1->Type) {
        case IP_ADDRESS_V4:
            return (Address1->Address.IPv4Address == Address2->Address.IPv4Address);

        case IP_ADDRESS_V6:
            return (RtlCompareMemory(&Address1->Address, &Address2->Address,
                sizeof(IPv6_RAW_ADDRESS)) == sizeof(IPv6_RAW_ADDRESS));
            break;
    }

    return FALSE;
}


/*
 * FUNCTION: Returns wether Address1 is less than Address2
 * ARGUMENTS:
 *     Address1 = Pointer to first address
 *     Address2 = Pointer to last address
 * RETURNS:
 *     -1 if Address1 < Address2, 1 if Address1 > Address2,
 *     or 0 if they are equal
 */
INT AddrCompare(
    PIP_ADDRESS Address1,
    PIP_ADDRESS Address2)
{
    switch (Address1->Type) {
        case IP_ADDRESS_V4: {
            ULONG Addr1, Addr2;
            if (Address2->Type == IP_ADDRESS_V4) {
                Addr1 = DN2H(Address1->Address.IPv4Address);
                Addr2 = DN2H(Address2->Address.IPv4Address);
                if (Addr1 < Addr2)
                    return -1;
                else
                    if (Addr1 == Addr2)
                        return 0;
                    else
                        return 1;
            } else
                /* FIXME: Support IPv6 */
                return -1;

        case IP_ADDRESS_V6:
            /* FIXME: Support IPv6 */
        break;
        }
    }

    return FALSE;
}


/*
 * FUNCTION: Returns wether two addresses are equal with IPv4 as input
 * ARGUMENTS:
 *     Address1 = Pointer to first address
 *     Address2 = Pointer to last address
 * RETURNS:
 *     TRUE if Address1 = Address2, FALSE if not
 */
BOOLEAN AddrIsEqualIPv4(
    PIP_ADDRESS Address1,
    IPv4_RAW_ADDRESS Address2)
{
    if (Address1->Type == IP_ADDRESS_V4)
        return (Address1->Address.IPv4Address == Address2);

    return FALSE;
}


/*
 * FUNCTION: Build an IPv4 style address
 * ARGUMENTS:
 *     Address = Raw IPv4 address
 * RETURNS:
 *     Pointer to IP address structure, NULL if there was not enough free
 *     non-paged memory
 */
PIP_ADDRESS AddrBuildIPv4(
    IPv4_RAW_ADDRESS Address)
{
    PIP_ADDRESS IPAddress;

    IPAddress = ExAllocatePool(NonPagedPool, sizeof(IP_ADDRESS));
    if (IPAddress) {
        IPAddress->RefCount            = 1;
        IPAddress->Type                = IP_ADDRESS_V4;
        IPAddress->Address.IPv4Address = Address;
        IPAddress->Free                = IPAddressFree;
    }

    return IPAddress;
}


/*
 * FUNCTION: Locates and returns an address entry using IPv4 adress as argument
 * ARGUMENTS:
 *     Address = Raw IPv4 address
 * RETURNS:
 *     Pointer to address entry if found, NULL if not found
 * NOTES:
 *     Only unicast addresses are considered.
 *     If found, the address is referenced
 */
PADDRESS_ENTRY AddrLocateADEv4(
    IPv4_RAW_ADDRESS Address)
{
    IP_ADDRESS Addr;

    AddrInitIPv4(&Addr, Address);

    return IPLocateADE(&Addr, ADE_UNICAST);
}


/*
 * FUNCTION: Searches through address file entries to find the first match
 * ARGUMENTS:
 *     Address       = IP address
 *     Port          = Port number
 *     Protocol      = Protocol number
 *     SearchContext = Pointer to search context
 * RETURNS:
 *     Pointer to address file, NULL if none was found
 */
PADDRESS_FILE AddrSearchFirst(
    PIP_ADDRESS Address,
    USHORT Port,
    USHORT Protocol,
    PAF_SEARCH SearchContext)
{
    SearchContext->Address  = Address;
    SearchContext->Port     = Port;
    SearchContext->Next     = AddressFileListHead.Flink;
    SearchContext->Protocol = Protocol;

    return AddrSearchNext(SearchContext);
}


/*
 * FUNCTION: Searches through address file entries to find next match
 * ARGUMENTS:
 *     SearchContext = Pointer to search context
 * RETURNS:
 *     Pointer to address file, NULL if none was found
 */
PADDRESS_FILE AddrSearchNext(
    PAF_SEARCH SearchContext)
{
    PLIST_ENTRY CurrentEntry;
    PIP_ADDRESS IPAddress;
    KIRQL OldIrql;
    PADDRESS_FILE Current = NULL;
    BOOLEAN Found = FALSE;

    if (IsListEmpty(SearchContext->Next))
        return NULL;

    CurrentEntry = SearchContext->Next;

    KeAcquireSpinLock(&AddressFileListLock, &OldIrql);

    while (CurrentEntry != &AddressFileListHead) {
        Current = CONTAINING_RECORD(CurrentEntry, ADDRESS_FILE, ListEntry);

        IPAddress = Current->ADE->Address;

        TI_DbgPrint(DEBUG_ADDRFILE, ("Comparing: ((%d, %d, %s), (%d, %d, %s)).\n",
            Current->Port,
            Current->Protocol,
            A2S(IPAddress),
            SearchContext->Port,
            SearchContext->Protocol,
            A2S(SearchContext->Address)));

        /* See if this address matches the search criteria */
        if (((Current->Port    == SearchContext->Port) &&
            (Current->Protocol == SearchContext->Protocol) &&
            (AddrIsEqual(IPAddress, SearchContext->Address))) ||
            (AddrIsUnspecified(IPAddress))) {
            /* We've found a match */
            Found = TRUE;
            break;
        }
        CurrentEntry = CurrentEntry->Flink;
    }

    KeReleaseSpinLock(&AddressFileListLock, OldIrql);

    if (Found) {
        SearchContext->Next = CurrentEntry->Flink;
        return Current;
    } else
        return NULL;
}

/* EOF */
