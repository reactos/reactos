/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Ethernet definitions
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <pshpack1.h>
typedef struct _ETH_HEADER
{
    UCHAR Destination[ETH_LENGTH_OF_ADDRESS];
    UCHAR Source[ETH_LENGTH_OF_ADDRESS];
    USHORT PayloadType;
} ETH_HEADER, *PETH_HEADER;
#include <poppack.h>

#define ETH_IS_LOCALLY_ADMINISTERED(Address) \
    (BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x02))

#define ETH_IS_EMPTY(Address) \
    (BOOLEAN)((((PUCHAR)(Address))[0] | ((PUCHAR)(Address))[1] | ((PUCHAR)(Address))[2] | \
               ((PUCHAR)(Address))[3] | ((PUCHAR)(Address))[5] | ((PUCHAR)(Address))[5]) == 0)

typedef struct IPv4_HEADER
{
    UCHAR VersionLength;
    UCHAR Tos;
    USHORT TotalLength;
    USHORT Id;
    USHORT Offset;
    UCHAR Ttl;
    UCHAR Protocol;
    USHORT Checksum;
    ULONG Source;
    ULONG Destination;
} IPv4_HEADER, *PIPv4_HEADER;

typedef struct TCPv4_HEADER
{
    USHORT SourcePort;
    USHORT DestinationPort;
    ULONG SequenceNumber;
    ULONG AckNumber;
    UCHAR DataOffset;
    UCHAR Flags;
    USHORT Window;
    USHORT Checksum;
    USHORT Urgent;
} TCPv4_HEADER, *PTCPv4_HEADER;

#define IP_HEADER_LENGTH(Header) \
    (((Header)->VersionLength & 0x0F) << 2)

#define TCP_HEADER_LENGTH(Header) \
    ((Header->DataOffset & 0xF0) >> 2)
