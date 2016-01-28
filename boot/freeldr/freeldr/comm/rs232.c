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
#include <cportlib/cportlib.h>

#if DBG

/* STATIC VARIABLES ***********************************************************/

#define DEFAULT_BAUD_RATE   19200

#if defined(_M_IX86) || defined(_M_AMD64)
static const ULONG BaseArray[] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};
#elif defined(_M_PPC)
static const ULONG BaseArray[] = {0, 0x800003F8};
#elif defined(_M_MIPS)
static const ULONG BaseArray[] = {0, 0x80006000, 0x80007000};
#elif defined(_M_ARM)
static const ULONG BaseArray[] = {0, 0xF1012000};
#else
#error Unknown architecture
#endif

#define MAX_COM_PORTS   (sizeof(BaseArray) / sizeof(BaseArray[0]) - 1)

/* The COM port must only be initialized once! */
static ULONG Rs232ComPort = 0;
static CPPORT Rs232ComPortInfo;

/* FUNCTIONS ******************************************************************/

BOOLEAN Rs232PortInitialize(IN ULONG ComPort,
                            IN ULONG BaudRate)
{
    NTSTATUS Status;
    PUCHAR Address;

    /*
     * Check whether it's the first time we initialize a COM port.
     * If not, check whether the specified one was already initialized.
     */
    if ((Rs232ComPort != 0) && (Rs232ComPort == ComPort))
        return TRUE;

    if (BaudRate == 0)
        BaudRate = DEFAULT_BAUD_RATE;

    if (ComPort == 0)
    {
        /*
         * Start enumerating COM ports from the last one to the first one,
         * and break when we find a valid port.
         * If we reach the first element of the list, the invalid COM port,
         * then it means that no valid port was found.
         */
        for (ComPort = MAX_COM_PORTS; ComPort > 0; ComPort--)
        {
            if (CpDoesPortExist(UlongToPtr(BaseArray[ComPort])))
            {
                Address = UlongToPtr(BaseArray[ComPort]);
                break;
            }
        }
        if (ComPort == 0)
            return FALSE;
    }
    else if (ComPort <= MAX_COM_PORTS)
    {
        if (CpDoesPortExist(UlongToPtr(BaseArray[ComPort])))
            Address = UlongToPtr(BaseArray[ComPort]);
        else
            return FALSE;
    }
    else
    {
        return FALSE;
    }

    Status = CpInitialize(&Rs232ComPortInfo, Address, BaudRate);
    if (!NT_SUCCESS(Status)) return FALSE;

    Rs232ComPort = ComPort;

    return TRUE;
}

BOOLEAN Rs232PortGetByte(PUCHAR ByteReceived)
{
    if (Rs232ComPort == 0) return FALSE;
    return (CpGetByte(&Rs232ComPortInfo, ByteReceived, TRUE, FALSE) == CP_GET_SUCCESS);
}

/*
BOOLEAN Rs232PortPollByte(PUCHAR ByteReceived)
{
    if (Rs232ComPort == 0) return FALSE;
    return (CpGetByte(&Rs232ComPortInfo, ByteReceived, FALSE, FALSE) == CP_GET_SUCCESS);
}
*/

VOID Rs232PortPutByte(UCHAR ByteToSend)
{
    if (Rs232ComPort == 0) return;
    CpPutByte(&Rs232ComPortInfo, ByteToSend);
}

#endif /* DBG */

BOOLEAN Rs232PortInUse(PUCHAR Base)
{
#if DBG
    return ( ((Rs232ComPort != 0) && (Rs232ComPortInfo.Address == Base)) ? TRUE : FALSE );
#else
    return FALSE;
#endif
}

#endif /* not _M_ARM */
