/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/versuart.c
 * PURPOSE:         Implements code for Versatile boards using the PL011 UART
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

//
// UART Registers
//
#define UART_PL01x_DR            (ArmBoardBlock->UartRegisterBase + 0x00)
#define UART_PL01x_RSR           (ArmBoardBlock->UartRegisterBase + 0x04)
#define UART_PL01x_ECR           (ArmBoardBlock->UartRegisterBase + 0x04)
#define UART_PL01x_FR            (ArmBoardBlock->UartRegisterBase + 0x18)
#define UART_PL011_IBRD          (ArmBoardBlock->UartRegisterBase + 0x24)
#define UART_PL011_FBRD          (ArmBoardBlock->UartRegisterBase + 0x28)
#define UART_PL011_LCRH          (ArmBoardBlock->UartRegisterBase + 0x2C)
#define UART_PL011_CR            (ArmBoardBlock->UartRegisterBase + 0x30)
#define UART_PL011_IMSC          (ArmBoardBlock->UartRegisterBase + 0x38)

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

/* FUNCTIONS ******************************************************************/

VOID
ArmVersaSerialInit(IN ULONG Baudrate)
{
    ULONG Divider, Remainder, Fraction;
    
    //
    // Calculate baudrate clock divider and remainder
    //
    Divider   = ArmBoardBlock->ClockRate / (16 * Baudrate);
    Remainder = ArmBoardBlock->ClockRate % (16 * Baudrate);
    
    //
    // Calculate the fractional part
    //
    Fraction  = (8 * Remainder / Baudrate) >> 1;
    Fraction += (8 * Remainder / Baudrate) & 1;
    
    //
    // Disable interrupts
    //
    WRITE_REGISTER_ULONG(UART_PL011_CR, 0);
    
    //
    // Set the baud rate to 115200 bps
    //
    WRITE_REGISTER_ULONG(UART_PL011_IBRD, Divider);
    WRITE_REGISTER_ULONG(UART_PL011_FBRD, Fraction);
    
    //
    // Set 8 bits for data, 1 stop bit, no parity, FIFO enabled
    //
    WRITE_REGISTER_ULONG(UART_PL011_LCRH,
                         UART_PL011_LCRH_WLEN_8 | UART_PL011_LCRH_FEN);
    
    //
    // Clear and enable FIFO
    //
    WRITE_REGISTER_ULONG(UART_PL011_CR,
                         UART_PL011_CR_UARTEN |
                         UART_PL011_CR_TXE |
                         UART_PL011_CR_RXE);
}

VOID
ArmVersaPutChar(IN INT Char)
{
    //
    // Properly support new-lines
    //
    if (Char == '\n') ArmVersaPutChar('\r');
    
    //
    // Wait for ready
    //
    while ((READ_REGISTER_ULONG(UART_PL01x_FR) & UART_PL01x_FR_TXFF) != 0);
    
    //
    // Send the character
    //
    WRITE_REGISTER_ULONG(UART_PL01x_DR, Char);
}

INT
ArmVersaGetCh(VOID)
{
    //
    // Wait for ready
    //
    while ((READ_REGISTER_ULONG(UART_PL01x_FR) & UART_PL01x_FR_RXFE) != 0);
    
    //
    // Read the character
    //
    return READ_REGISTER_ULONG(UART_PL01x_DR);
}

BOOLEAN
ArmVersaKbHit(VOID)
{
    //
    // Return if something is ready
    //
    return ((READ_REGISTER_ULONG(UART_PL01x_FR) & UART_PL01x_FR_RXFE) == 0);
}
