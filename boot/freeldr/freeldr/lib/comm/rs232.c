/*
 *  FreeLoader
 *  Copyright (C) 2001  Brian Palmer  <brianp@sginet.com>
 *  Copyright (C) 2001  Eric Kohl
 *  Copyright (C) 2001  Emanuele Aliberti
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef _M_ARM

/* INCLUDES *******************************************************************/

#include <freeldr.h>

#if DBG

#include <cportlib/cportlib.h>
#include <cportlib/uartinfo.h>

/* STATIC VARIABLES ***********************************************************/

/* The COM port must only be initialized once! */
static PUCHAR Rs232ComPort = NULL;
static CPPORT Rs232ComPortInfo;

#ifdef _M_ARM64
/* PL011 data and flag registers */
#define PL011_DR        0x00
#define PL011_FR        0x18
#define PL011_FR_RXFE   0x10
#define PL011_FR_TXFF   0x20

static BOOLEAN Rs232IsPl011 = FALSE;
#endif

/* FUNCTIONS ******************************************************************/

BOOLEAN
Rs232PortInitialize(
    _In_ PUCHAR PortAddress,
    _In_ ULONG BaudRate)
{
    NTSTATUS Status;

    /* Check whether it's the first time we initialize a COM port.
     * If not, check whether the specified one was already initialized. */
    if (Rs232ComPort && (Rs232ComPort == PortAddress))
        return TRUE;

    if (BaudRate == 0)
        BaudRate = DEFAULT_BAUD_RATE;

    if (!PortAddress)
    {
#ifdef _M_ARM64
        USHORT Subtype;
        ULONG64 Address;

        /* The base table only has the QEMU PL011, the firmware names its debug port in DBG2 */
        if (!UefiGetDebugSerialPort(&Subtype, &Address))
            return FALSE;

        PortAddress = (PUCHAR)(ULONG_PTR)Address;
        switch (Subtype)
        {
            case DBG2_SERIAL_PL011:
            case DBG2_SERIAL_SBSA_32BIT:
            case DBG2_SERIAL_SBSA:
                /* The firmware set the UART up, only its data path is used */
                Rs232IsPl011 = TRUE;
                Rs232ComPortInfo.Address = PortAddress;
                Rs232ComPort = PortAddress;
                return TRUE;

            case DBG2_SERIAL_16550:
            case DBG2_SERIAL_16550_SUBSET:
                if (!CpDoesPortExist(PortAddress))
                    return FALSE;
                break;

            default:
                return FALSE;
        }
#else
        /*
         * Enumerate COM ports from the last to the first one, and stop
         * when we find a valid port. If we reach the first list element
         * (the undefined COM port), no valid port was found.
         */
        ULONG ComPort;
        for (ComPort = MAX_COM_PORTS; ComPort > 0; ComPort--)
        {
            PortAddress = UlongToPtr(BaseArray[ComPort]);
            if (CpDoesPortExist(PortAddress))
                break;
        }
        if (ComPort == 0 || PortAddress == NULL)
            return FALSE;
#endif
    }
    else
    {
        if (!CpDoesPortExist(PortAddress))
            return FALSE;
    }

    Status = CpInitialize(&Rs232ComPortInfo, PortAddress, BaudRate);
    if (!NT_SUCCESS(Status))
        return FALSE;

    Rs232ComPort = Rs232ComPortInfo.Address;
    return TRUE;
}

BOOLEAN Rs232PortGetByte(PUCHAR ByteReceived)
{
    if (!Rs232ComPort) return FALSE;
#ifdef _M_ARM64
    if (Rs232IsPl011)
    {
        while (READ_REGISTER_ULONG((PULONG)(Rs232ComPort + PL011_FR)) & PL011_FR_RXFE)
            NOTHING;
        *ByteReceived = (UCHAR)READ_REGISTER_ULONG((PULONG)(Rs232ComPort + PL011_DR));
        return TRUE;
    }
#endif
    return (CpGetByte(&Rs232ComPortInfo, ByteReceived, TRUE, FALSE) == CP_GET_SUCCESS);
}

/*
BOOLEAN Rs232PortPollByte(PUCHAR ByteReceived)
{
    if (!Rs232ComPort) return FALSE;
    return (CpGetByte(&Rs232ComPortInfo, ByteReceived, FALSE, FALSE) == CP_GET_SUCCESS);
}
*/

VOID Rs232PortPutByte(UCHAR ByteToSend)
{
    if (!Rs232ComPort) return;
#ifdef _M_ARM64
    if (Rs232IsPl011)
    {
        while (READ_REGISTER_ULONG((PULONG)(Rs232ComPort + PL011_FR)) & PL011_FR_TXFF)
            NOTHING;
        WRITE_REGISTER_ULONG((PULONG)(Rs232ComPort + PL011_DR), ByteToSend);
        return;
    }
#endif
    CpPutByte(&Rs232ComPortInfo, ByteToSend);
}

#endif /* DBG */

BOOLEAN Rs232PortInUse(PUCHAR Base)
{
#if DBG
    return (Rs232ComPort && (Rs232ComPortInfo.Address == Base));
#else
    return FALSE;
#endif
}

#endif /* !_M_ARM */
