/*
 * PROJECT:     VerOS NT Kernel
 * LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
 * FILE:        drivers/network/tcpip/include/dhcp6.h
 * PURPOSE:     DHCPv6 client sketch (RFC 8415)
 * COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
 * PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>
 *
 * Message types and UDP ports from RFC 8415 Sections 5.1, 7.2, 7.3, and 18
 * (https://www.rfc-editor.org/rfc/rfc8415). All-DHCP-Agents multicast
 * FF02::1:2 is Section 7.1. The peer that answers is
 * virtual-machines/veros-ntfs/Send-Ipv6Peer.ps1 -Mode Dhcp6.
 */

#pragma once

/* DHCPv6 UDP ports (RFC 8415 Section 7.2) */
#define DHCP6_CLIENT_PORT               546
#define DHCP6_SERVER_PORT               547

/* Selected message types (RFC 8415 Section 7.3) */
#define DHCP6_MSG_SOLICIT               1
#define DHCP6_MSG_ADVERTISE             2
#define DHCP6_MSG_REQUEST               3
#define DHCP6_MSG_REPLY                 7
#define DHCP6_MSG_INFORMATION_REQUEST   11

NTSTATUS DHCP6Startup(VOID);
NTSTATUS DHCP6Shutdown(VOID);

NTSTATUS DHCP6StartOnInterface(
    PIP_INTERFACE Interface);

VOID DHCP6StopOnInterface(
    PIP_INTERFACE Interface);

VOID DHCP6Receive(
    PUCHAR Payload,
    UINT PayloadLength);

BOOLEAN DHCP6SourceForDestination(
    PIP_INTERFACE Interface,
    PIP_ADDRESS Destination,
    PIP_ADDRESS Source);

/* EOF */
