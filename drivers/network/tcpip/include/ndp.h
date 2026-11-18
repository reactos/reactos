/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/tcpip/include/ndp.h
 * PURPOSE:     Neighbor Discovery Protocol (NDP) definitions
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * ICMPv6 Neighbor Solicitation/Advertisement types, option types, and
 * message layouts from RFC 4861 Sections 4.3, 4.4, and 4.6
 * (https://www.rfc-editor.org/rfc/rfc4861). Solicited-node multicast
 * from RFC 4291 Section 2.7.1 (https://www.rfc-editor.org/rfc/rfc4291).
 * Ethernet multicast mapping from RFC 2464 Section 7
 * (https://www.rfc-editor.org/rfc/rfc2464).
 */

#pragma once

/* ICMPv6 Neighbor Discovery message types (RFC 4861 Section 4.1 / 4.3 / 4.4) */
#define ICMPV6_TYPE_NEIGHBOR_SOLICIT    135
#define ICMPV6_TYPE_NEIGHBOR_ADVERT     136

/* Neighbor Discovery option types (RFC 4861 Section 4.6) */
#define ND_OPT_SOURCE_LINKADDR          1
#define ND_OPT_TARGET_LINKADDR          2

/* Neighbor Advertisement flags in the 32-bit flags/reserved word (RFC 4861 4.4) */
#define ND_NA_FLAG_ROUTER               0x80000000UL
#define ND_NA_FLAG_SOLICITED            0x40000000UL
#define ND_NA_FLAG_OVERRIDE             0x20000000UL

#include <pshpack1.h>
typedef struct ND_NEIGHBOR_SOLICIT {
    UINT8 Type;
    UINT8 Code;
    UINT16 Checksum;
    UINT32 Reserved;
    IPv6_RAW_ADDRESS Target;
} ND_NEIGHBOR_SOLICIT, *PND_NEIGHBOR_SOLICIT;

typedef struct ND_NEIGHBOR_ADVERT {
    UINT8 Type;
    UINT8 Code;
    UINT16 Checksum;
    UINT32 FlagsReserved;
    IPv6_RAW_ADDRESS Target;
} ND_NEIGHBOR_ADVERT, *PND_NEIGHBOR_ADVERT;

typedef struct ND_OPTION_LINKADDR {
    UINT8 Type;
    UINT8 Length; /* Units of 8 octets (RFC 4861 Section 4.6) */
    UCHAR Address[6];
} ND_OPTION_LINKADDR, *PND_OPTION_LINKADDR;
#include <poppack.h>

BOOLEAN NDPSendSolicit(
    PIP_INTERFACE Interface,
    PIP_ADDRESS Target);

VOID NDPReceiveSolicit(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket);

VOID NDPReceiveAdvert(
    PIP_INTERFACE Interface,
    PIP_PACKET IPPacket);

/* EOF */
