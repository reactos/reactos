/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/address.h
 * PURPOSE:     Address manipulation prototypes
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Ryan Wilson <ryan@verosnt.com>
 *
 * IPv6 link-local fe80::/64 and modified EUI-64 from RFC 4291 Sections
 * 2.5.6 and Appendix A (https://www.rfc-editor.org/rfc/rfc4291).
 * Solicited-node multicast from RFC 4291 Section 2.7.1.
 */

#pragma once

/*
 * Initialize an IPv4 style address
 * VOID AddrInitIPv4(
 *     PIP_ADDRESS IPAddress,
 *     IPv4_RAW_ADDRESS RawAddress)
 */
#define AddrInitIPv4(IPAddress, RawAddress)           \
{                                                     \
    (IPAddress)->Type                = IP_ADDRESS_V4; \
    (IPAddress)->Address.IPv4Address = (RawAddress);  \
}

/*
 * Initialize an IPv6 style address from eight network-order hextets
 * (RFC 8200 address representation as USHORT[8]).
 */
#define AddrInitIPv6(IPAddress, H0, H1, H2, H3, H4, H5, H6, H7) \
{                                                               \
    (IPAddress)->Type = IP_ADDRESS_V6;                          \
    (IPAddress)->Address.IPv6Address[0] = (H0);                 \
    (IPAddress)->Address.IPv6Address[1] = (H1);                 \
    (IPAddress)->Address.IPv6Address[2] = (H2);                 \
    (IPAddress)->Address.IPv6Address[3] = (H3);                 \
    (IPAddress)->Address.IPv6Address[4] = (H4);                 \
    (IPAddress)->Address.IPv6Address[5] = (H5);                 \
    (IPAddress)->Address.IPv6Address[6] = (H6);                 \
    (IPAddress)->Address.IPv6Address[7] = (H7);                 \
}

/* ::1 loopback (RFC 4291 Section 2.5.3), network byte order hextets. */
#define AddrInitIPv6Loopback(IPAddress) \
    AddrInitIPv6((IPAddress), 0, 0, 0, 0, 0, 0, 0, WH2N(0x0001))

/* Unspecified :: (RFC 4291 Section 2.5.2). */
#define AddrInitIPv6Unspecified(IPAddress) \
    AddrInitIPv6((IPAddress), 0, 0, 0, 0, 0, 0, 0, 0)

BOOLEAN AddrIsIPv6Loopback(
    PIP_ADDRESS Address);

BOOLEAN AddrIsIPv6LinkLocal(
    PIP_ADDRESS Address);

BOOLEAN AddrFormLinkLocalEui64(
    PIP_ADDRESS Address,
    PUCHAR MacAddress,
    UINT MacLength);

VOID AddrSolicitedNodeMulticast(
    PIP_ADDRESS Multicast,
    PIP_ADDRESS Unicast);

#if DBG

PCHAR A2S(
    PIP_ADDRESS Address);

#endif /* DBG */

VOID IPAddressFree(
    PVOID Object);

BOOLEAN AddrIsUnspecified(
    PIP_ADDRESS Address);

NTSTATUS AddrGetAddress(
    PTRANSPORT_ADDRESS AddrList,
    PIP_ADDRESS Address,
    PUSHORT Port);

NTSTATUS AddrBuildAddress(
    PTRANSPORT_ADDRESS TdiAddress,
    PIP_ADDRESS Address,
    PUSHORT Port);

BOOLEAN AddrIsEqual(
    PIP_ADDRESS Address1,
    PIP_ADDRESS Address2);

INT AddrCompare(
    PIP_ADDRESS Address1,
    PIP_ADDRESS Address2);

BOOLEAN AddrIsEqualIPv4(
    PIP_ADDRESS Address1,
    IPv4_RAW_ADDRESS Address2);

PIP_INTERFACE AddrLocateInterface(
    PIP_ADDRESS MatchAddress);

PADDRESS_FILE AddrSearchFirst(
    PIP_ADDRESS Address,
    USHORT Port,
    USHORT Protocol,
    PAF_SEARCH SearchContext);

PADDRESS_FILE AddrSearchNext(
    PAF_SEARCH SearchContext);

unsigned long NTAPI inet_addr(const char*);

ULONG IPv4NToHl( ULONG Address );

UINT AddrCountPrefixBits( PIP_ADDRESS Netmask );

VOID AddrWidenAddress( PIP_ADDRESS Network, PIP_ADDRESS Source,
		       PIP_ADDRESS Netmask );

/* EOF */
