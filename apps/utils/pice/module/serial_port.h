/*++

Copyright (c) 1998-2001 Klaus P. Gerlicher

Module Name:

    serial_port.h

Abstract:

    HEADER for serial.c

    serial port HW defines

Environment:

    LINUX 2.2.X
    Kernel mode only

Author: 

    Klaus P. Gerlicher

Revision History:

    15-Nov-2000:    general cleanup of source files

Copyright notice:

  This file may be distributed under the terms of the GNU Public License.

--*/
#define COM1            1
#define COM2            2
#define COM1BASE        0x3F8   /*  Base port address for COM1  */
#define COM2BASE        0x2F8   /*  Base port address for COM2  */

// FIX these
#define COM3BASE        0x3F8   /*  Base port address for COM3  */
#define COM4BASE        0x2F8   /*  Base port address for COM4   */

/*
    The 8250 UART has 10 registers accessible through 7 port addresses.
    Here are their addresses relative to COM1BASE and COM2BASE. Note
    that the baud rate registers, (DLL) and (DLH) are active only when
    the Divisor-Latch Access-Bit (DLAB) is on. The (DLAB) is bit 7 of
    the (LCR).

	o TXR Output data to the serial port.
	o RXR Input data from the serial port.
	o LCR Initialize the serial port.
	o IER Controls interrupt generation.
	o IIR Identifies interrupts.
	o MCR Send contorl signals to the modem.
	o LSR Monitor the status of the serial port.
	o MSR Receive status of the modem.
	o DLL Low byte of baud rate divisor.
	o DHH High byte of baud rate divisor.
*/
#define TXR             0       /*  Transmit register (WRITE) */
#define RXR             0       /*  Receive register  (READ)  */
#define IER             1       /*  Interrupt Enable          */
#define IIR             2       /*  Interrupt ID              */
#define FCR             2       /*  FIFO control              */
#define LCR             3       /*  Line control              */
#define MCR             4       /*  Modem control             */
#define LSR             5       /*  Line Status               */
#define MSR             6       /*  Modem Status              */
#define DLL             0       /*  Divisor Latch Low         */
#define DLH             1       /*  Divisor latch High        */


/*-------------------------------------------------------------------*
  Bit values held in the Line Control Register (LCR).
	bit		meaning
	---		-------
	0-1		00=5 bits, 01=6 bits, 10=7 bits, 11=8 bits.
	2		Stop bits.
	3		0=parity off, 1=parity on.
	4		0=parity odd, 1=parity even.
	5		Sticky parity.
	6		Set break.
	7		Toggle port addresses.
 *-------------------------------------------------------------------*/
#define NO_PARITY       0x00
#define EVEN_PARITY     0x18
#define ODD_PARITY      0x08



/*-------------------------------------------------------------------*
  Bit values held in the Line Status Register (LSR).
	bit		meaning
	---		-------
	0		Data ready.
	1		Overrun error - Data register overwritten.
	2		Parity error - bad transmission.
	3		Framing error - No stop bit was found.
	4		Break detect - End to transmission requested.
	5		Transmitter holding register is empty.
	6		Transmitter shift register is empty.
	7               Time out - off line.
 *-------------------------------------------------------------------*/
#define RCVRDY          0x01
#define OVRERR          0x02
#define PRTYERR         0x04
#define FRMERR          0x08
#define BRKERR          0x10
#define XMTRDY          0x20
#define XMTRSR          0x40
#define TIMEOUT		0x80

/*-------------------------------------------------------------------*
  Bit values held in the Modem Output Control Register (MCR).
	bit     	meaning
	---		-------
	0		Data Terminal Ready. Computer ready to go.
	1		Request To Send. Computer wants to send data.
	2		auxillary output #1.
	3		auxillary output #2.(Note: This bit must be
			set to allow the communications card to send
			interrupts to the system)
	4		UART ouput looped back as input.
	5-7		not used.
 *------------------------------------------------------------------*/
#define DTR             0x01
#define RTS             0x02


/*------------------------------------------------------------------*
  Bit values held in the Modem Input Status Register (MSR).
	bit		meaning
	---		-------
	0		delta Clear To Send.
	1		delta Data Set Ready.
	2		delta Ring Indicator.
	3		delta Data Carrier Detect.
	4		Clear To Send.
	5		Data Set Ready.
	6		Ring Indicator.
	7		Data Carrier Detect.
 *------------------------------------------------------------------*/
#define CTS             0x10
#define DSR             0x20


/*------------------------------------------------------------------*
  Bit values held in the Interrupt Enable Register (IER).
	bit		meaning
	---		-------
	0		Interrupt when data received.
	1		Interrupt when transmitter holding reg. empty.
	2		Interrupt when data reception error.
	3		Interrupt when change in modem status register.
	4-7		Not used.
 *------------------------------------------------------------------*/
#define RX_INT          0x01


/*------------------------------------------------------------------*
  Bit values held in the Interrupt Identification Register (IIR).
	bit		meaning
	---		-------
	0		Interrupt pending
	1-2             Interrupt ID code
			00=Change in modem status register,
			01=Transmitter holding register empty,
			10=Data received,
			11=reception error, or break encountered.
	3-7		Not used.
 *------------------------------------------------------------------*/
#define RX_ID           0x04
#define RX_MASK         0x07
