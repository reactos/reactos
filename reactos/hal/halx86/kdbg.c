/* $Id: kdbg.c,v 1.4 2002/05/08 17:05:31 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/hal/x86/kdbg.c
 * PURPOSE:         Serial i/o functions for the kernel debugger.
 * PROGRAMMER:      Emanuele Aliberti
 *                  Eric Kohl
 * UPDATE HISTORY:
 *                  Created 05/09/99
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <internal/debug.h>


#define DEFAULT_BAUD_RATE    19200


/* MACROS *******************************************************************/

#define   SER_RBR(x)   ((x)+0)
#define   SER_THR(x)   ((x)+0)
#define   SER_DLL(x)   ((x)+0)
#define   SER_IER(x)   ((x)+1)
#define     SR_IER_ERDA   0x01
#define     SR_IER_ETHRE  0x02
#define     SR_IER_ERLSI  0x04
#define     SR_IER_EMS    0x08
#define     SR_IER_ALL    0x0F
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
#define     SR_MCR_OUT1 0x04
#define     SR_MCR_OUT2 0x08
#define     SR_MCR_LOOP 0x10
#define   SER_LSR(x)   ((x)+5)
#define     SR_LSR_DR  0x01
#define     SR_LSR_TBE 0x20
#define   SER_MSR(x)   ((x)+6)
#define     SR_MSR_CTS 0x10
#define     SR_MSR_DSR 0x20
#define   SER_SCR(x)   ((x)+7)


/* GLOBAL VARIABLES *********************************************************/

ULONG EXPORTED KdComPortInUse = 0;


/* STATIC VARIABLES *********************************************************/

static ULONG ComPort = 0;
static ULONG BaudRate = 0;
static PUCHAR PortBase = (PUCHAR)0;

/* The com port must only be initialized once! */
static BOOLEAN PortInitialized = FALSE;


/* STATIC FUNCTIONS *********************************************************/

static BOOLEAN
KdpDoesComPortExist (PUCHAR BaseAddress)
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


/* FUNCTIONS ****************************************************************/

/* HAL.KdPortInitialize */
BOOLEAN
STDCALL
KdPortInitialize (
	PKD_PORT_INFORMATION	PortInformation,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
        ULONG BaseArray[5] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};
        char buffer[80];
        ULONG divisor;
        BYTE lcr;

        if (PortInitialized == FALSE)
        {
                if (PortInformation->BaudRate != 0)
                {
                        BaudRate = PortInformation->BaudRate;
                }
                else
                {
                        BaudRate = DEFAULT_BAUD_RATE;
                }

                if (PortInformation->ComPort == 0)
                {
                        if (KdpDoesComPortExist ((PUCHAR)BaseArray[2]))
                        {
                                PortBase = (PUCHAR)BaseArray[2];
                                ComPort = 2;
                                PortInformation->BaseAddress = (ULONG)PortBase;
                                PortInformation->ComPort = ComPort;
#ifndef NDEBUG
                                sprintf (buffer,
                                         "\nSerial port COM%ld found at 0x%lx\n",
                                         ComPort,
                                         (ULONG)PortBase);
                                HalDisplayString (buffer);
#endif /* NDEBUG */
                        }
                        else if (KdpDoesComPortExist ((PUCHAR)BaseArray[1]))
                        {
                                PortBase = (PUCHAR)BaseArray[1];
                                ComPort = 1;
                                PortInformation->BaseAddress = (ULONG)PortBase;
                                PortInformation->ComPort = ComPort;
#ifndef NDEBUG
                                sprintf (buffer,
                                         "\nSerial port COM%ld found at 0x%lx\n",
                                         ComPort,
                                         (ULONG)PortBase);
                                HalDisplayString (buffer);
#endif /* NDEBUG */
                        }
                        else
                        {
                                sprintf (buffer,
                                         "\nKernel Debugger: No COM port found!!!\n\n");
                                HalDisplayString (buffer);
                                return FALSE;
                        }
                }
                else
                {
                        if (KdpDoesComPortExist ((PUCHAR)BaseArray[PortInformation->ComPort]))
                        {
                                PortBase = (PUCHAR)BaseArray[PortInformation->ComPort];
                                ComPort = PortInformation->ComPort;
                                PortInformation->BaseAddress = (ULONG)PortBase;
#ifndef NDEBUG
                                sprintf (buffer,
                                         "\nSerial port COM%ld found at 0x%lx\n",
                                         ComPort,
                                         (ULONG)PortBase);
                                HalDisplayString (buffer);
#endif /* NDEBUG */
                        }
                        else
                        {
                                sprintf (buffer,
                                         "\nKernel Debugger: No serial port found!!!\n\n");
                                HalDisplayString (buffer);
                                return FALSE;
                        }
                }

                PortInitialized = TRUE;
        }

        /*
         * set baud rate and data format (8N1)
         */

        /*  turn on DTR and RTS  */
        WRITE_PORT_UCHAR (SER_MCR(PortBase), SR_MCR_DTR | SR_MCR_RTS);

        /* set DLAB */
        lcr = READ_PORT_UCHAR (SER_LCR(PortBase)) | SR_LCR_DLAB;
        WRITE_PORT_UCHAR (SER_LCR(PortBase), lcr);

        /* set baud rate */
        divisor = 115200 / BaudRate;
        WRITE_PORT_UCHAR (SER_DLL(PortBase), divisor & 0xff);
        WRITE_PORT_UCHAR (SER_DLM(PortBase), (divisor >> 8) & 0xff);

        /* reset DLAB and set 8N1 format */
        WRITE_PORT_UCHAR (SER_LCR(PortBase),
                          SR_LCR_CS8 | SR_LCR_ST1 | SR_LCR_PNO);

        /* read junk out of the RBR */
        lcr = READ_PORT_UCHAR (SER_RBR(PortBase));

        /*
         * set global info
         */
        KdComPortInUse = (ULONG)PortBase;

        /*
         * print message to blue screen
         */
        sprintf (buffer,
                 "\nKernel Debugger: COM%ld (Port 0x%lx) BaudRate %ld\n\n",
                 ComPort,
                 (ULONG)PortBase,
                 BaudRate);

        HalDisplayString (buffer);

        return TRUE;
}


/* HAL.KdPortInitializeEx */
BOOLEAN
STDCALL
KdPortInitializeEx (
	PKD_PORT_INFORMATION	PortInformation,
	DWORD	Unknown1,
	DWORD	Unknown2
	)
{
        ULONG BaseArray[5] = {0, 0x3F8, 0x2F8, 0x3E8, 0x2E8};
		PUCHAR ComPortBase;
        char buffer[80];
        ULONG divisor;
        BYTE lcr;

		if (PortInformation->BaudRate == 0)
		{
				PortInformation->BaudRate = DEFAULT_BAUD_RATE;
		}

		if (PortInformation->ComPort == 0)
		{
				return FALSE;
		}
		else
		{
				if (KdpDoesComPortExist ((PUCHAR)BaseArray[PortInformation->ComPort]))
				{
						ComPortBase = (PUCHAR)BaseArray[PortInformation->ComPort];
						PortInformation->BaseAddress = (ULONG)ComPortBase;
#ifndef NDEBUG
						sprintf (buffer,
								 "\nSerial port COM%ld found at 0x%lx\n",
								 PortInformation->ComPort,
								 (ULONG)ComPortBase];
						HalDisplayString (buffer);
#endif /* NDEBUG */
				}
				else
				{
						sprintf (buffer,
								 "\nKernel Debugger: Serial port not found!!!\n\n");
						HalDisplayString (buffer);
						return FALSE;
				}
		}

        /*
         * set baud rate and data format (8N1)
         */

        /*  turn on DTR and RTS  */
        WRITE_PORT_UCHAR (SER_MCR(ComPortBase), SR_MCR_DTR | SR_MCR_RTS);

        /* set DLAB */
        lcr = READ_PORT_UCHAR (SER_LCR(ComPortBase)) | SR_LCR_DLAB;
        WRITE_PORT_UCHAR (SER_LCR(ComPortBase), lcr);

        /* set baud rate */
        divisor = 115200 / PortInformation->BaudRate;
        WRITE_PORT_UCHAR (SER_DLL(ComPortBase), divisor & 0xff);
        WRITE_PORT_UCHAR (SER_DLM(ComPortBase), (divisor >> 8) & 0xff);

        /* reset DLAB and set 8N1 format */
        WRITE_PORT_UCHAR (SER_LCR(ComPortBase),
                          SR_LCR_CS8 | SR_LCR_ST1 | SR_LCR_PNO);

        /* read junk out of the RBR */
        lcr = READ_PORT_UCHAR (SER_RBR(ComPortBase));

#ifndef NDEBUG

        /*
         * print message to blue screen
         */
        sprintf (buffer,
                 "\nKernel Debugger: COM%ld (Port 0x%lx) BaudRate %ld\n\n",
                 PortInformation->ComPort,
                 (ULONG)ComPortBase,
                 PortInformation->BaudRate);

        HalDisplayString (buffer);

#endif /* NDEBUG */

        return TRUE;
}


/* HAL.KdPortGetByte */
BOOLEAN
STDCALL
KdPortGetByte (
	PUCHAR	ByteRecieved
	)
{
	if (PortInitialized == FALSE)
		return FALSE;

	if ((READ_PORT_UCHAR (SER_LSR(PortBase)) & SR_LSR_DR))
	{
		*ByteRecieved = READ_PORT_UCHAR (SER_RBR(PortBase));
		return TRUE;
	}

	return FALSE;
}


/* HAL.KdPortGetByteEx */
BOOLEAN
STDCALL
KdPortGetByteEx (
	PKD_PORT_INFORMATION	PortInformation,
	PUCHAR	ByteRecieved
	)
{
	PUCHAR ComPortBase = (PUCHAR)PortInformation->BaseAddress;

	if ((READ_PORT_UCHAR (SER_LSR(ComPortBase)) & SR_LSR_DR))
	{
		*ByteRecieved = READ_PORT_UCHAR (SER_RBR(ComPortBase));
		return TRUE;
	}

	return FALSE;
}


/* HAL.KdPortPollByte */
BOOLEAN
STDCALL
KdPortPollByte (
	PUCHAR	ByteRecieved
	)
{
	if (PortInitialized == FALSE)
		return FALSE;

	while ((READ_PORT_UCHAR (SER_LSR(PortBase)) & SR_LSR_DR) == 0)
		;

	*ByteRecieved = READ_PORT_UCHAR (SER_RBR(PortBase));

	return TRUE;
}


/* HAL.KdPortPollByteEx */
BOOLEAN
STDCALL
KdPortPollByteEx (
	PKD_PORT_INFORMATION	PortInformation,
	PUCHAR	ByteRecieved
	)
{
	PUCHAR ComPortBase = (PUCHAR)PortInformation->BaseAddress;

	while ((READ_PORT_UCHAR (SER_LSR(ComPortBase)) & SR_LSR_DR) == 0)
		;

	*ByteRecieved = READ_PORT_UCHAR (SER_RBR(ComPortBase));

	return TRUE;
}




/* HAL.KdPortPutByte */
VOID
STDCALL
KdPortPutByte (
	UCHAR ByteToSend
	)
{
	if (PortInitialized == FALSE)
		return;

	while ((READ_PORT_UCHAR (SER_LSR(PortBase)) & SR_LSR_TBE) == 0)
		;

	WRITE_PORT_UCHAR (SER_THR(PortBase), ByteToSend);
}

/* HAL.KdPortPutByteEx */
VOID
STDCALL
KdPortPutByteEx (
	PKD_PORT_INFORMATION	PortInformation,
	UCHAR ByteToSend
	)
{
	PUCHAR ComPortBase = (PUCHAR)PortInformation->BaseAddress;

	while ((READ_PORT_UCHAR (SER_LSR(ComPortBase)) & SR_LSR_TBE) == 0)
		;

	WRITE_PORT_UCHAR (SER_THR(ComPortBase), ByteToSend);
}


/* HAL.KdPortRestore */
VOID
STDCALL
KdPortRestore (
	VOID
	)
{
}


/* HAL.KdPortSave */
VOID
STDCALL
KdPortSave (
	VOID
	)
{
}


/* HAL.KdPortDisableInterrupts */
BOOLEAN
STDCALL
KdPortDisableInterrupts()
{
  UCHAR ch;

	if (PortInitialized == FALSE)
		return FALSE;

	ch = READ_PORT_UCHAR (SER_MCR (PortBase));
  ch &= (~(SR_MCR_OUT1 | SR_MCR_OUT2));
	WRITE_PORT_UCHAR (SER_MCR (PortBase), ch);

	ch = READ_PORT_UCHAR (SER_IER (PortBase));
  ch &= (~SR_IER_ALL);
	WRITE_PORT_UCHAR (SER_IER (PortBase), ch);

	return TRUE;
}


/* HAL.KdPortEnableInterrupts */
BOOLEAN
STDCALL
KdPortEnableInterrupts()
{
  UCHAR ch;

	if (PortInitialized == FALSE)
		return FALSE;

	ch = READ_PORT_UCHAR (SER_IER (PortBase));
  ch &= (~SR_IER_ALL);
  ch |= SR_IER_ERDA;
	WRITE_PORT_UCHAR (SER_IER (PortBase), ch);

	ch = READ_PORT_UCHAR (SER_MCR (PortBase));
  ch &= (~SR_MCR_LOOP);
  ch |= (SR_MCR_OUT1 | SR_MCR_OUT2);
	WRITE_PORT_UCHAR (SER_MCR (PortBase), ch);

	return TRUE;
}

/* EOF */
