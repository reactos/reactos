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

#define DEFAULT_BAUD_RATE    19200

#define   SER_RBR(x)   ((x)+0)
#define   SER_THR(x)   ((x)+0)
#define   SER_DLL(x)   ((x)+0)
#define   SER_IER(x)   ((x)+1)
#define   SER_DLM(x)   ((x)+1)
#define   SER_IIR(x)   ((x)+2)
#define   SER_LCR(x)   ((x)+3)
#define     SR_LCR_CS5 0x00
#define     SR_LCR_CS6 0x01
#define     SR_LCR_CS7 0x02
#define     SR_LCR_CS8 0x03
#define     SR_LCR_ST1 0x00
#define     SR_LCR_ST2 0x04
#define     SR_LCR_PNO 0x00
#define     SR_LCR_POD 0x08
#define     SR_LCR_PEV 0x18
#define     SR_LCR_PMK 0x28
#define     SR_LCR_PSP 0x38
#define     SR_LCR_BRK 0x40
#define     SR_LCR_DLAB 0x80
#define   SER_MCR(x)   ((x)+4)
#define     SR_MCR_DTR 0x01
#define     SR_MCR_RTS 0x02
#define   SER_LSR(x)   ((x)+5)
#define     SR_LSR_DR  0x01
#define     SR_LSR_TBE 0x20
#define   SER_MSR(x)   ((x)+6)
#define     SR_MSR_CTS 0x10
#define     SR_MSR_DSR 0x20
#define   SER_SCR(x)   ((x)+7)

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

    return (CpGetByte(&Rs232ComPort, ByteReceived, FALSE, FALSE) == CP_GET_SUCCESS);
}

/*
BOOLEAN Rs232PortPollByte(PUCHAR ByteReceived)
{
    if (PortInitialized == FALSE)
        return FALSE;

    while ((READ_PORT_UCHAR (SER_LSR(Rs232PortBase)) & SR_LSR_DR) == 0)
        ;

    *ByteReceived = READ_PORT_UCHAR (SER_RBR(Rs232PortBase));

    return TRUE;
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
