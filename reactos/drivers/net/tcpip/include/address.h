/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/address.h
 * PURPOSE:     Address manipulation prototypes
 */
#ifndef __ADDRESS_H
#define __ADDRESS_H


/*
 * Initialize an IPv4 style address
 * VOID AddrInitIPv4(
 *     PIP_ADDRESS IPAddress,
 *     IPv4_RAW_ADDRESS RawAddress)
 */
#define AddrInitIPv4(IPAddress, RawAddress)           \
{                                                     \
    (IPAddress)->RefCount            = 1;             \
    (IPAddress)->Type                = IP_ADDRESS_V4; \
    (IPAddress)->Address.IPv4Address = (RawAddress);  \
}


BOOLEAN AddrIsUnspecified(
    PIP_ADDRESS Address);

NTSTATUS AddrGetAddress(
    PTRANSPORT_ADDRESS AddrList,
    PIP_ADDRESS *Address,
    PUSHORT Port,
    PIP_ADDRESS *Cache);

BOOLEAN AddrIsEqual(
    PIP_ADDRESS Address1,
    PIP_ADDRESS Address2);

INT AddrCompare(
    PIP_ADDRESS Address1,
    PIP_ADDRESS Address2);

BOOLEAN AddrIsEqualIPv4(
    PIP_ADDRESS Address1,
    IPv4_RAW_ADDRESS Address2);

PIP_ADDRESS AddrBuildIPv4(
    IPv4_RAW_ADDRESS Address);

PADDRESS_ENTRY AddrLocateADEv4(
    IPv4_RAW_ADDRESS Address);

PADDRESS_FILE AddrSearchFirst(
    PIP_ADDRESS Address,
    USHORT Port,
    USHORT Protocol,
    PAF_SEARCH SearchContext);

PADDRESS_FILE AddrSearchNext(
    PAF_SEARCH SearchContext);

#endif /* __ADDRESS_H */

/* EOF */
