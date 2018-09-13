/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    jazznvr.h

Abstract:

    This module contains definitions for the Jazz non-volatile ram structures.

Author:

    David M. Robinson (davidro) 11-Nov-1991

Revision History:

--*/

#ifndef _JAZZNVR_
#define _JAZZNVR_

//
// Define the private configuration packet structure, which contains a
// configuration component as well as pointers to the component's parent,
// peer, child, and configuration data.
//

typedef struct _CONFIGURATION_PACKET {
    CONFIGURATION_COMPONENT Component;
    struct _CONFIGURATION_PACKET *Parent;
    struct _CONFIGURATION_PACKET *Peer;
    struct _CONFIGURATION_PACKET *Child;
    PVOID ConfigurationData;
} CONFIGURATION_PACKET, *PCONFIGURATION_PACKET;

//
// The compressed configuration packet structure used to store configuration
// data in NVRAM.
//

typedef struct _COMPRESSED_CONFIGURATION_PACKET {
    UCHAR Parent;
    UCHAR Class;
    UCHAR Type;
    UCHAR Flags;
    ULONG Key;
    USHORT Version;
    USHORT ConfigurationDataLength;
    USHORT Identifier;
    USHORT ConfigurationData;
} COMPRESSED_CONFIGURATION_PACKET, *PCOMPRESSED_CONFIGURATION_PACKET;

//
// Defines for Identifier index.
//

#define NO_CONFIGURATION_IDENTIFIER 0xFFFF

//
// Defines for the volatile and non-volatile configuration tables.
//

#define NUMBER_OF_ENTRIES 32
#define LENGTH_OF_IDENTIFIER 504
#define LENGTH_OF_DATA 2048
#define LENGTH_OF_ENVIRONMENT 1024

#define MAXIMUM_ENVIRONMENT_VALUE 128

//
// The volatile configuration table structure.
//

typedef struct _CONFIGURATION {
    CONFIGURATION_PACKET Packet[NUMBER_OF_ENTRIES];
    UCHAR Identifier[LENGTH_OF_IDENTIFIER];
    UCHAR Data[LENGTH_OF_DATA];
} CONFIGURATION, *PCONFIGURATION;

//
// The non-volatile configuration table structure.
//

typedef struct _NV_CONFIGURATION {
    COMPRESSED_CONFIGURATION_PACKET Packet[NUMBER_OF_ENTRIES];
    UCHAR Identifier[LENGTH_OF_IDENTIFIER];
    UCHAR Data[LENGTH_OF_DATA];
    UCHAR Checksum1[4];
    UCHAR Environment[LENGTH_OF_ENVIRONMENT];
    UCHAR Checksum2[4];
} NV_CONFIGURATION, *PNV_CONFIGURATION;

//
// Non-volatile ram layout.
//

#if defined(MIPS)

#define NVRAM_CONFIGURATION NVRAM_VIRTUAL_BASE
#define NVRAM_SYSTEM_ID NVRAM_VIRTUAL_BASE + 0x00002000

#endif

#endif // _JAZZNVR_
