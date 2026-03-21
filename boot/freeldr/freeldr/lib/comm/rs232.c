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
