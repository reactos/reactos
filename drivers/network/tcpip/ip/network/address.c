/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        tcpip/address.c
 * PURPOSE:     Routines for handling addresses
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 */

#include "precomp.h"

extern int sprintf( char *out, const char *fmt, ... );

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
	sprintf(p, "%d.%d.%d.%d",
		(INT)((ip >> 24) & 0xFF),
		(INT)((ip >> 16) & 0xFF),
		(INT)((ip >> 8) & 0xFF),
		(INT)(ip & 0xFF));
	break;

    case IP_ADDRESS_V6:
	/* FIXME: IPv6 is not supported */
	strcpy(p, "(IPv6 address not supported)");
	break;
    }
    return p;
}

ULONG IPv4NToHl( ULONG Address ) {
    return
	((Address & 0xff) << 24) |
	((Address & 0xff00) << 8) |
	((Address >> 8) & 0xff00) |
	((Address >> 24) & 0xff);
}

UINT AddrCountPrefixBits( PIP_ADDRESS Netmask ) {
    UINT Prefix = 0;
    if( Netmask->Type == IP_ADDRESS_V4 ) {
	ULONG BitTest = 0x80000000;

	/* The mask has been read in network order.  Put it in host order
	 * in order to scan it. */

	ULONG TestMask = IPv4NToHl(Netmask->Address.IPv4Address);

	while ((BitTest & TestMask) != 0) {
	    Prefix++;
	    BitTest >>= 1;
	}
	return Prefix;
    } else {
	TI_DbgPrint(DEBUG_DATALINK, ("Don't know address type %d\n",
				     Netmask->Type));
	return 0;
    }
}

VOID AddrWidenAddress( PIP_ADDRESS Network, PIP_ADDRESS Source,
		       PIP_ADDRESS Netmask ) {
    if( Netmask->Type == IP_ADDRESS_V4 ) {
        Network->Type = Netmask->Type;
	Network->Address.IPv4Address =
	    Source->Address.IPv4Address & Netmask->Address.IPv4Address;
    } else {
	TI_DbgPrint(DEBUG_DATALINK, ("Don't know address type %d\n",
				     Netmask->Type));
	*Network = *Source;
    }
}

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
    ExFreePoolWithTag(Object, IP_ADDRESS_TAG);
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
            return (Address->Address.IPv4Address == 0 ||
                    Address->Address.IPv4Address == 0xFFFFFFFF);

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
    PIP_ADDRESS Address,
    PUSHORT Port)
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
                PTDI_ADDRESS_IP ValidAddr = (PTDI_ADDRESS_IP)CurAddr->Address;
                *Port = ValidAddr->sin_port;
		Address->Type = CurAddr->AddressType;
		ValidAddr = (PTDI_ADDRESS_IP)CurAddr->Address;
		AddrInitIPv4(Address, ValidAddr->in_addr);
		return STATUS_SUCCESS;
	    }
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
    PTRANSPORT_ADDRESS TaAddress,
    PIP_ADDRESS Address,
    PUSHORT Port)
{
  PTDI_ADDRESS_IP ValidAddr;
  PTA_ADDRESS TdiAddress = &TaAddress->Address[0];

  if (TdiAddress->AddressType != TDI_ADDRESS_TYPE_IP) {
      TI_DbgPrint
	  (MID_TRACE,("AddressType %x, Not valid\n", TdiAddress->AddressType));
    return STATUS_INVALID_ADDRESS;
  }
  if (TdiAddress->AddressLength < TDI_ADDRESS_LENGTH_IP) {
      TI_DbgPrint
	  (MID_TRACE,("AddressLength %x, Not valid (expected %x)\n",
		      TdiAddress->AddressLength, TDI_ADDRESS_LENGTH_IP));
      return STATUS_INVALID_ADDRESS;
  }


  ValidAddr = (PTDI_ADDRESS_IP)TdiAddress->Address;

  AddrInitIPv4(Address, ValidAddr->in_addr);
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
    if (Address1->Type != Address2->Type) {
        DbgPrint("AddrIsEqual: Unequal Address Types\n");
        return FALSE;
    }

    switch (Address1->Type) {
        case IP_ADDRESS_V4:
            return (Address1->Address.IPv4Address == Address2->Address.IPv4Address);

        case IP_ADDRESS_V6:
            return (RtlCompareMemory(&Address1->Address, &Address2->Address,
                sizeof(IPv6_RAW_ADDRESS)) == sizeof(IPv6_RAW_ADDRESS));
            break;

        default:
            DbgPrint("AddrIsEqual: Bad address type\n");
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


unsigned long NTAPI inet_addr(const char *AddrString)
/*
 * Convert an ansi string dotted-quad address to a ulong
 * NOTES:
 *     - this isn't quite like the real inet_addr() - * it doesn't
 *       handle "10.1" and similar - but it's good enough.
 *     - Returns in *host* byte order, unlike real inet_addr()
 */
{
	ULONG Octets[4] = {0,0,0,0};
	ULONG i = 0;

	if(!AddrString)
		return -1;

	while(*AddrString)
		{
			CHAR c = *AddrString;
			AddrString++;

			if(c == '.')
				{
					i++;
					continue;
				}

			if(c < '0' || c > '9')
				return -1;

			Octets[i] *= 10;
			Octets[i] += (c - '0');

			if(Octets[i] > 255)
				return -1;
		}

	return (Octets[3] << 24) + (Octets[2] << 16) + (Octets[1] << 8) + Octets[0];
}

/* EOF */
