/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    par1284.h

Abstract:

    This file defines the interface for the 1284 export driver.
    The 1284 export driver will export a 1284 communications interface
    to parallel class drivers.

Author:

    Norbert P. Kusters  9-May-1994

Revision History:

--*/

#ifndef _PAR1284_
#define _PAR1284_

//
// Define the current known 1284 protocols for the parallel port.
//

#define P1284_PROTOCOL_ISA  0   // Centronics with Nibble for reverse.
#define P1284_PROTOCOL_BYTE 1   // Centronics with Byte for reverse.
#define P1284_PROTOCOL_EPP  2   // EPP protocol.
#define P1284_PROTOCOL_ECP  3   // ECP protocol.
#define P1284_NUM_PROTOCOLS 4

//
// Define the interface to the export driver.
//

NTSTATUS
P1284Initialize(
    IN  PUCHAR                      Controller,
    IN  PHYSICAL_ADDRESS            OriginalController,
    IN  BOOLEAN                     UsePICode,
    IN  PPARALLEL_ECP_INFORMATION   EcpInfo,
    OUT PVOID*                      P1284Extension
    );

VOID
P1284Cleanup(
    IN  PVOID   P1284Extension
    );

NTSTATUS
P1284Write(
    IN  PVOID   P1284Extension,
    IN  PVOID   Buffer,
    IN  ULONG   BufferSize,
    OUT PULONG  BytesTransfered
    );

NTSTATUS
P1284Read(
    IN  PVOID   P1284Extension,
    IN  PVOID   Buffer,
    IN  ULONG   BufferSize,
    OUT PULONG  BytesTransfered
    );

NTSTATUS
P1284NegotiateProtocol(
    IN  PVOID   P1284Extension,
    OUT PULONG  NegotiatedProtocol
    );

NTSTATUS
P1284SetProtocol(
    IN  PVOID   P1284Extension,
    IN  ULONG   ProtocolNumber,
    IN  BOOLEAN Negotiate
    );

NTSTATUS
P1284QueryDeviceId(
    IN  PVOID   P1284Extension,
    OUT PUCHAR  DeviceIdBuffer,
    IN  ULONG   BufferSize,
    OUT PULONG  DeviceIdSize
    );

#endif // _PAR1284_
