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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "freeldr.h"
#include "portio.h"


/* MACROS *******************************************************************/

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

static ULONG Rs232ComPort = 0;
static ULONG Rs232BaudRate = 0;
static PUCHAR Rs232PortBase = (PUCHAR)0;

/* The com port must only be initialized once! */
static BOOLEAN PortInitialized = FALSE;

/* STATIC FUNCTIONS *********************************************************/

static BOOL Rs232DoesComPortExist(PUCHAR BaseAddress)
{
        BOOLEAN found;
        BYTE mcr;
        BYTE msr;

        found = FALSE;

        /* save Modem Control Register (MCR) */
        mcr = READ_PORT_UCHAR (SER_MCR(BaseAddress));

        /* enable loop mode (set Bit 4 of the MCR) */
        WRITE_PORT_UCHAR (SER_MCR(BaseAddress), 0x10);

        /* clear all modem output bits */
        WRITE_PORT_UCHAR (SER_MCR(BaseAddress), 0x10);

        /* read the Modem Status Register */
        msr = READ_PORT_UCHAR (SER_MSR(BaseAddress));

        /*
         * the upper nibble of the MSR (modem output bits) must be
         * equal to the lower nibble of the MCR (modem input bits)
         */
        if ((msr & 0xF0) == 0x00)
        {
                /* set all modem output bits */
                WRITE_PORT_UCHAR (SER_MCR(BaseAddress), 0x1F);

                /* read the Modem Status Register */
                msr = READ_PORT_UCHAR (SER_MSR(BaseAddress));

                /*
                 * the upper nibble of the MSR (modem output bits) must be
                 * equal to the lower nibble of the MCR (modem input bits)
                 */
                if ((msr & 0xF0) == 0xF0)
                        found = TRUE;
        }

        /* restore MCR */
        WRITE_PORT_UCHAR (SER_MCR(BaseAddress), mcr);

        return (found);
}

/* FUNCTIONS *********************************************************/

BOOL Rs232PortInitialize(ULONG ComPort, ULONG BaudRate)
{
        ULONG BaseArray[5] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};
        char buffer[80];
        ULONG divisor;
        BYTE lcr;

        if (PortInitialized == FALSE)
        {
                if (BaudRate != 0)
                {
                        Rs232BaudRate = BaudRate;
                }
                else
                {
                        Rs232BaudRate = DEFAULT_BAUD_RATE;
                }

                if (ComPort == 0)
                {
                        if (Rs232DoesComPortExist ((PUCHAR)BaseArray[2]))
                        {
                                Rs232PortBase = (PUCHAR)BaseArray[2];
                                Rs232ComPort = 2;
/*#ifndef NDEBUG
                                sprintf (buffer,
                                         "\nSerial port COM%ld found at 0x%lx\n",
                                         ComPort,
                                         (ULONG)PortBase);
                                HalDisplayString (buffer);
#endif*/ /* NDEBUG */
                        }
                        else if (Rs232DoesComPortExist ((PUCHAR)BaseArray[1]))
                        {
                                Rs232PortBase = (PUCHAR)BaseArray[1];
                                Rs232ComPort = 1;
/*#ifndef NDEBUG
                                sprintf (buffer,
                                         "\nSerial port COM%ld found at 0x%lx\n",
                                         ComPort,
                                         (ULONG)PortBase);
                                HalDisplayString (buffer);
#endif*/ /* NDEBUG */
                        }
                        else
                        {
                                /*sprintf (buffer,
                                         "\nKernel Debugger: No COM port found!!!\n\n");
                                HalDisplayString (buffer);*/
                                return FALSE;
                        }
                }
                else
                {
                        if (Rs232DoesComPortExist ((PUCHAR)BaseArray[ComPort]))
                        {
                                Rs232PortBase = (PUCHAR)BaseArray[ComPort];
                                Rs232ComPort = ComPort;
/*#ifndef NDEBUG
                                sprintf (buffer,
                                         "\nSerial port COM%ld found at 0x%lx\n",
                                         ComPort,
                                         (ULONG)PortBase);
                                HalDisplayString (buffer);
#endif*/ /* NDEBUG */
                        }
                        else
                        {
                                /*sprintf (buffer,
                                         "\nKernel Debugger: No serial port found!!!\n\n");
                                HalDisplayString (buffer);*/
                                return FALSE;
                        }
                }

                PortInitialized = TRUE;
        }

        /*
         * set baud rate and data format (8N1)
         */

        /*  turn on DTR and RTS  */
        WRITE_PORT_UCHAR (SER_MCR(Rs232PortBase), SR_MCR_DTR | SR_MCR_RTS);

        /* set DLAB */
        lcr = READ_PORT_UCHAR (SER_LCR(Rs232PortBase)) | SR_LCR_DLAB;
        WRITE_PORT_UCHAR (SER_LCR(Rs232PortBase), lcr);

        /* set baud rate */
        divisor = 115200 / BaudRate;
        WRITE_PORT_UCHAR (SER_DLL(Rs232PortBase), divisor & 0xff);
        WRITE_PORT_UCHAR (SER_DLM(Rs232PortBase), (divisor >> 8) & 0xff);

        /* reset DLAB and set 8N1 format */
        WRITE_PORT_UCHAR (SER_LCR(Rs232PortBase),
                          SR_LCR_CS8 | SR_LCR_ST1 | SR_LCR_PNO);

        /* read junk out of the RBR */
        lcr = READ_PORT_UCHAR (SER_RBR(Rs232PortBase));

        /*
         * set global info
         */
        //KdComPortInUse = (ULONG)PortBase;

        /*
         * print message to blue screen
         */
        /*sprintf (buffer,
                 "\nKernel Debugger: COM%ld (Port 0x%lx) BaudRate %ld\n\n",
                 ComPort,
                 (ULONG)PortBase,
                 BaudRate);

        HalDisplayString (buffer);*/

        return TRUE;
}

BOOL Rs232PortGetByte(PUCHAR ByteRecieved)
{
	if (PortInitialized == FALSE)
		return FALSE;

	if ((READ_PORT_UCHAR (SER_LSR(Rs232PortBase)) & SR_LSR_DR))
	{
		*ByteRecieved = READ_PORT_UCHAR (SER_RBR(Rs232PortBase));
		return TRUE;
	}

	return FALSE;
}

BOOL Rs232PortPollByte(PUCHAR ByteRecieved)
{
	if (PortInitialized == FALSE)
		return FALSE;

	while ((READ_PORT_UCHAR (SER_LSR(Rs232PortBase)) & SR_LSR_DR) == 0)
		;

	*ByteRecieved = READ_PORT_UCHAR (SER_RBR(Rs232PortBase));

	return TRUE;
}

VOID Rs232PortPutByte(UCHAR ByteToSend)
{
	if (PortInitialized == FALSE)
		return;

	while ((READ_PORT_UCHAR (SER_LSR(Rs232PortBase)) & SR_LSR_TBE) == 0)
		;

	WRITE_PORT_UCHAR (SER_THR(Rs232PortBase), ByteToSend);
}
