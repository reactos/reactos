/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         Serial port driver
 * FILE:            drivers/bus/serial/legacy.c
 * PURPOSE:         Legacy serial port enumeration
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 *                  Mark Junker (mjscod@gmx.de)
 */

#include "serial.h"

UART_TYPE
SerialDetectUartType(
	IN PUCHAR BaseAddress)
{
	UCHAR Lcr, TestLcr;
	UCHAR OldScr, Scr5A, ScrA5;
	BOOLEAN FifoEnabled;
	UCHAR NewFifoStatus;

	Lcr = READ_PORT_UCHAR(SER_LCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_LCR(BaseAddress), Lcr ^ 0xFF);
	TestLcr = READ_PORT_UCHAR(SER_LCR(BaseAddress)) ^ 0xFF;
	WRITE_PORT_UCHAR(SER_LCR(BaseAddress), Lcr);

	/* Accessing the LCR must work for a usable serial port */
	if (TestLcr != Lcr)
		return UartUnknown;

	/* Ensure that all following accesses are done as required */
	READ_PORT_UCHAR(SER_RBR(BaseAddress));
	READ_PORT_UCHAR(SER_IER(BaseAddress));
	READ_PORT_UCHAR(SER_IIR(BaseAddress));
	READ_PORT_UCHAR(SER_LCR(BaseAddress));
	READ_PORT_UCHAR(SER_MCR(BaseAddress));
	READ_PORT_UCHAR(SER_LSR(BaseAddress));
	READ_PORT_UCHAR(SER_MSR(BaseAddress));
	READ_PORT_UCHAR(SER_SCR(BaseAddress));

	/* Test scratch pad */
	OldScr = READ_PORT_UCHAR(SER_SCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_SCR(BaseAddress), 0x5A);
	Scr5A = READ_PORT_UCHAR(SER_SCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_SCR(BaseAddress), 0xA5);
	ScrA5 = READ_PORT_UCHAR(SER_SCR(BaseAddress));
	WRITE_PORT_UCHAR(SER_SCR(BaseAddress), OldScr);

	/* When non-functional, we have a 8250 */
	if (Scr5A != 0x5A || ScrA5 != 0xA5)
		return Uart8250;

	/* Test FIFO type */
	FifoEnabled = (READ_PORT_UCHAR(SER_IIR(BaseAddress)) & 0x80) != 0;
	WRITE_PORT_UCHAR(SER_FCR(BaseAddress), SR_FCR_ENABLE_FIFO);
	NewFifoStatus = READ_PORT_UCHAR(SER_IIR(BaseAddress)) & 0xC0;
	if (!FifoEnabled)
		WRITE_PORT_UCHAR(SER_FCR(BaseAddress), 0);
	switch (NewFifoStatus)
	{
		case 0x00:
			return Uart16450;
		case 0x40:
		case 0x80:
			/* Not sure about this but the documentation says that 0x40
			 * indicates an unusable FIFO but my tests only worked
			 * with 0x80 */
			return Uart16550;
	}

	/* FIFO is only functional for 16550A+ */
	return Uart16550A;
}
