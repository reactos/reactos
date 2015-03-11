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

#include <freeldr.h>
#include <cportlib/cportlib.h>

/* MACROS *******************************************************************/

#if DBG

#define DEFAULT_BAUD_RATE   19200


/* STATIC VARIABLES *********************************************************/

static ULONG BaseArray[] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};

/* The COM port must only be initialized once! */
static CPPORT Rs232ComPort;
static BOOLEAN PortInitialized = FALSE;


/* FUNCTIONS *********************************************************/

BOOLEAN Rs232PortInitialize(IN ULONG ComPort,
                            IN ULONG BaudRate)
{
    NTSTATUS Status;
    PUCHAR Address;

    if (PortInitialized == FALSE)
    {
        if (BaudRate == 0)
        {
            BaudRate = DEFAULT_BAUD_RATE;
        }

        if (ComPort == 0)
        {
            if (CpDoesPortExist(UlongToPtr(BaseArray[2])))
            {
                Address = UlongToPtr(BaseArray[2]);
            }
            else if (CpDoesPortExist(UlongToPtr(BaseArray[1])))
            {
                Address = UlongToPtr(BaseArray[1]);
            }
            else
            {
                return FALSE;
            }
        }
        else if (ComPort <= 4) // 4 == MAX_COM_PORTS
        {
            if (CpDoesPortExist(UlongToPtr(BaseArray[ComPort])))
            {
                Address = UlongToPtr(BaseArray[ComPort]);
            }
            else
            {
                return FALSE;
            }
        }
        else
        {
            return FALSE;
        }

        Status = CpInitialize(&Rs232ComPort, Address, BaudRate);
        if (!NT_SUCCESS(Status)) return FALSE;

        PortInitialized = TRUE;
    }

    return TRUE;
}

BOOLEAN Rs232PortGetByte(PUCHAR ByteReceived)
{
    if (PortInitialized == FALSE)
        return FALSE;

    return (CpGetByte(&Rs232ComPort, ByteReceived, TRUE, FALSE) == CP_GET_SUCCESS);
}

/*
BOOLEAN Rs232PortPollByte(PUCHAR ByteReceived)
{
    if (PortInitialized == FALSE)
        return FALSE;

    return (CpGetByte(&Rs232ComPort, ByteReceived, FALSE, FALSE) == CP_GET_SUCCESS);
}
*/

VOID Rs232PortPutByte(UCHAR ByteToSend)
{
    if (PortInitialized == FALSE)
        return;

    CpPutByte(&Rs232ComPort, ByteToSend);
}

#endif /* DBG */

BOOLEAN Rs232PortInUse(PUCHAR Base)
{
#if DBG
    return ( (PortInitialized && (Rs232ComPort.Address == Base)) ? TRUE : FALSE );
#else
    return FALSE;
#endif
}

#endif /* not _M_ARM */
