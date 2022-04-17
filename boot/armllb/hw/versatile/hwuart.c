/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/armllb/hw/versatile/hwuart.c
 * PURPOSE:         LLB UART Initialization Routines for Versatile
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include "precomp.h"

//
// UART Registers
//
#define UART_PL01x_DR            (LlbHwVersaUartBase + 0x00)
#define UART_PL01x_RSR           (LlbHwVersaUartBase + 0x04)
#define UART_PL01x_ECR           (LlbHwVersaUartBase + 0x04)
#define UART_PL01x_FR            (LlbHwVersaUartBase + 0x18)
#define UART_PL011_IBRD          (LlbHwVersaUartBase + 0x24)
#define UART_PL011_FBRD          (LlbHwVersaUartBase + 0x28)
#define UART_PL011_LCRH          (LlbHwVersaUartBase + 0x2C)
#define UART_PL011_CR            (LlbHwVersaUartBase + 0x30)
#define UART_PL011_IMSC          (LlbHwVersaUartBase + 0x38)

//
// LCR Values
//
#define UART_PL011_LCRH_WLEN_8   0x60
#define UART_PL011_LCRH_FEN      0x10

//
// FCR Values
//
#define UART_PL011_CR_UARTEN     0x01
#define UART_PL011_CR_TXE        0x100
#define UART_PL011_CR_RXE        0x200

//
// LSR Values
//
#define UART_PL01x_FR_RXFE       0x10
#define UART_PL01x_FR_TXFF       0x20

static const ULONG LlbHwVersaUartBase = 0x101F1000;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
LlbHwVersaUartInitialize(VOID)
{
    ULONG Divider, Remainder, Fraction, ClockRate, Baudrate;

    /* Query peripheral rate, hardcore baudrate */
    ClockRate = LlbHwGetPClk();
    Baudrate = 115200;

    /* Calculate baudrate clock divider and remainder */
    Divider = ClockRate / (16 * Baudrate);
    Remainder = ClockRate % (16 * Baudrate);

    /* Calculate the fractional part */
    Fraction = (8 * Remainder / Baudrate) >> 1;
    Fraction += (8 * Remainder / Baudrate) & 1;

    /* Disable interrupts */
    WRITE_REGISTER_ULONG(UART_PL011_CR, 0);

    /* Set the baud rate to 115200 bps */
    WRITE_REGISTER_ULONG(UART_PL011_IBRD, Divider);
    WRITE_REGISTER_ULONG(UART_PL011_FBRD, Fraction);

    /* Set 8 bits for data, 1 stop bit, no parity, FIFO enabled */
    WRITE_REGISTER_ULONG(UART_PL011_LCRH,
                         UART_PL011_LCRH_WLEN_8 | UART_PL011_LCRH_FEN);

    /* Clear and enable FIFO */
    WRITE_REGISTER_ULONG(UART_PL011_CR,
                         UART_PL011_CR_UARTEN |
                         UART_PL011_CR_TXE |
                         UART_PL011_CR_RXE);
}

VOID
NTAPI
LlbHwUartSendChar(IN CHAR Char)
{
    /* Send the character */
    WRITE_REGISTER_ULONG(UART_PL01x_DR, Char);
}

BOOLEAN
NTAPI
LlbHwUartTxReady(VOID)
{
    /* TX output buffer is ready? */
    return ((READ_REGISTER_ULONG(UART_PL01x_FR) & UART_PL01x_FR_TXFF) == 0);
}

ULONG
NTAPI
LlbHwGetUartBase(IN ULONG Port)
{
    if (Port == 0)
    {
        return 0x101F1000;
    }
    else if (Port == 1)
    {
        return 0x101F2000;
    }

    return 0;
}

/* EOF */
