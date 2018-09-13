/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    d3dzport.h

Abstract:

    This include file defines the interface between the DECstation 5000
    DZ port driver and its client class drivers.

Author:

    David N. Cutler (davec) 15-Aug-1990

Environment:

    Kernel mode

Revision History:

--*/

#ifndef _DZPORT
#define _DZPORT

//
// Define DZ port internal I/O control functions.
//

#define IOCTL_MN_DZPORT_CONNECT 0          // connect line
#define IOCTL_MN_DZPORT_DISCONNECT 4       // disconnect line

//
// Define client procedure types for interrupt routines.
//

typedef
VOID
(*PDZPORT_INPUT_SERVICE) (
    IN PVOID ClientContext,
    IN UCHAR InputByte
    );

typedef
BOOLEAN
(*PDZPORT_OUTPUT_SERVICE) (
    IN PVOID ClientContext
    );

//
// Define port procedure types for service routines.
//

typedef
VOID
(*PDZPORT_DISABLE_LINE) (
    IN PVOID PortContext
    );

typedef
VOID
(*PDZPORT_ENABLE_LINE) (
    IN PVOID PortContext
    );

typedef
VOID
(*PDZPORT_ENABLE_TRANSMIT) (
    IN PVOID PortContext
    );

typedef
VOID
(*PDZPORT_OUTPUT_BYTE) (
    IN PVOID PortContext,
    IN UCHAR OutputByte
    );

typedef
BOOLEAN
(*PDZPORT_SET_LINE_PARAMETERS) (
    IN PVOID PortContext,
    IN ULONG BaudRate,
    IN ULONG CharacterLength,
    IN ULONG StopBits,
    IN ULONG Polarity,
    IN BOOLEAN ParityEnable
    );

//
// Define I/O request message formats.
//

typedef struct _DZPORT_ACCEPT {
    PVOID PortContext;
    PDZPORT_ENABLE_LINE EnableLine;
    PDZPORT_DISABLE_LINE DisableLine;
    PDZPORT_ENABLE_TRANSMIT EnableTransmit;
    PDZPORT_OUTPUT_BYTE OutputByte;
    PDZPORT_SET_LINE_PARAMETERS SetLineParameters;
    PKINTERRUPT Interrupt;
} DZPORT_ACCEPT, *PDZPORT_ACCEPT;

typedef struct _DZPORT_CONNECT {
    PVOID ClientContext;
    PDZPORT_INPUT_SERVICE InputService;
    PDZPORT_OUTPUT_SERVICE OutputService;
    ULONG LineNumber;
    ULONG BaudRate;
    ULONG CharacterLength;
    ULONG StopBits;
    ULONG Polarity;
    BOOLEAN ParityEnable;
} DZPORT_CONNECT, *PDZPORT_CONNECT;

typedef struct _DZPORT_DISCONNECT {
    ULONG LineNumber;
} DZPORT_DISCONNECT, *PDZPORT_DISCONNECT;

#endif // _DZPORT
