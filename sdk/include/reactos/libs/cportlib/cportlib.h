/*
 * PROJECT:     ReactOS ComPort Library
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Header for the ComPort Library
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#pragma once

#include <ntdef.h>

//
// Return error codes.
//
#define CP_GET_SUCCESS  0
#define CP_GET_NODATA   1
#define CP_GET_ERROR    2

//
// COM port flags.
//
#define CPPORT_FLAG_MODEM_CONTROL   0x02

typedef struct _CPPORT
{
    PUCHAR Address;
    ULONG  BaudRate;
    USHORT Flags;
} CPPORT, *PCPPORT;

BOOLEAN
NTAPI
CpDoesPortExist(
    _In_ PUCHAR Address);

VOID
NTAPI
CpEnableFifo(
    _In_ PUCHAR Address,
    _In_ BOOLEAN Enable);

VOID
NTAPI
CpSetBaud(
    _Inout_ PCPPORT Port,
    _In_ ULONG BaudRate);

NTSTATUS
NTAPI
CpInitialize(
    _Inout_ PCPPORT Port,
    _In_ PUCHAR Address,
    _In_ ULONG BaudRate);

USHORT
NTAPI
CpGetByte(
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR Byte,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN Poll);

VOID
NTAPI
CpPutByte(
    _Inout_ PCPPORT Port,
    _In_ UCHAR Byte);

/* EOF */
