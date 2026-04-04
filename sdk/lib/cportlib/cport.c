/*
 * PROJECT:     ReactOS ComPort Library
 * LICENSE:     BSD - See COPYING.ARM in the top level directory
 * PURPOSE:     Provides a serial port library for KDCOM, INIT, and FREELDR
 * COPYRIGHT:   Copyright 2010 ReactOS Portable Systems Group
 *              Copyright 2012-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 *
 * NOTE: This code is used by Headless Support (Ntoskrnl.exe and Osloader.exe)
 * and Kdcom.dll in Windows. It may be that WinDBG depends on some of these quirks.
 *
 * FIXMEs:
 * - Get x64 KDCOM, KDBG, FREELDR, and other current code to use this.
 */

/* INCLUDES *******************************************************************/

#include <ntstatus.h>
#include <cportlib/cportlib.h>

#include "ns16550.c"

/* GLOBALS ********************************************************************/

// /* Wait timeout value */
// #define TIMEOUT_COUNT   (1024 * 200)

// UCHAR RingIndicator;

/* FUNCTIONS ******************************************************************/

BOOLEAN
NTAPI
CpDoesPortExist(
    _In_ PUCHAR Address)
{
    return Uart16550DoesPortExist(Address);
}

VOID
NTAPI
CpEnableFifo(
    _In_ PUCHAR Address,
    _In_ BOOLEAN Enable)
{
    Uart16550EnableFifo(Address, Enable);
}

VOID
NTAPI
CpSetBaud(
    _Inout_ PCPPORT Port,
    _In_ ULONG BaudRate)
{
    Uart16550SetBaud(Port, BaudRate);
}

NTSTATUS
NTAPI
CpInitialize(
    _Inout_ PCPPORT Port,
    _In_ PUCHAR Address,
    _In_ ULONG BaudRate)
{
    /* Validity checks */
    if (Port == NULL || Address == NULL || BaudRate == 0)
        return STATUS_INVALID_PARAMETER;

#if 0
    if (!CpDoesPortExist(Address))
        return STATUS_NOT_FOUND;

    /* Initialize port data */
    Port->Address  = Address;
    Port->BaudRate = 0;
    Port->Flags    = 0;
#endif
    return Uart16550Initialize(Port, Address, BaudRate);
}

USHORT
NTAPI
CpGetByte(
    _Inout_ PCPPORT Port,
    _Out_ PUCHAR Byte,
    _In_ BOOLEAN Wait,
    _In_ BOOLEAN Poll)
{
    return Uart16550GetByte(Port, Byte, Wait, Poll);
}

VOID
NTAPI
CpPutByte(
    _Inout_ PCPPORT Port,
    _In_ UCHAR Byte)
{
    Uart16550PutByte(Port, Byte);
}

/* EOF */
