/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/omapuart.c
 * PURPOSE:         Implements code for TI OMAP3 boards using the 16550 UART
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

//
// UART Registers
//
#define UART0_RHR   (ArmBoardBlock->UartRegisterBase + 0x00)
#define UART0_THR   UART0_RHR
#define UART0_IER   (ArmBoardBlock->UartRegisterBase + 0x04)
#define UART0_FCR   (ArmBoardBlock->UartRegisterBase + 0x08)
#define UART0_LCR   (ArmBoardBlock->UartRegisterBase + 0x0C)
#define UART0_MCR   (ArmBoardBlock->UartRegisterBase + 0x10)
#define UART0_LSR   (ArmBoardBlock->UartRegisterBase + 0x14)
#define UART0_MDR1  (ArmBoardBlock->UartRegisterBase + 0x20)

//
// When we enable the divisor latch
//
#define UART0_DLL   UART0_RHR
#define UART0_DLH   UART0_IER

//
// FCR Values
//
#define FCR_FIFO_EN 0x01
#define FCR_RXSR    0x02
#define FCR_TXSR    0x04

//
// LCR Values
//
#define LCR_WLS_8   0x03
#define LCR_1_STB   0x00
#define LCR_DIVL_EN 0x80
#define LCR_NO_PAR  0x00

//
// LSR Values
//
#define LSR_DR      0x01
#define LSR_THRE    0x20

//
// MCR Values
//
#define MCR_DTR     0x01
#define MCR_RTS     0x02

//
// MDR1 Modes
//
#define MDR1_UART16X            1
#define MDR1_SIR                2
#define MDR1_UART16X_AUTO_BAUD  3
#define MDR1_UART13X            4
#define MDR1_MIR                5
#define MDR1_FIR                6
#define MDR1_CIR                7
#define MDR1_DISABLE            8

/* FUNCTIONS ******************************************************************/

VOID
ArmOmap3SerialInit(IN ULONG Baudrate)
{
    ULONG BaudClock;
    
    //
    // Calculate baudrate clock divider to set the baud rate
    //
    BaudClock = (ArmBoardBlock->ClockRate / 16) / Baudrate;
    
    //
    // Disable serial port
    //
    WRITE_REGISTER_UCHAR(UART0_MDR1, MDR1_DISABLE);
    
    //
    // Disable interrupts
    //
    WRITE_REGISTER_UCHAR(UART0_IER, 0);
    
    //
    // Set the baud rate to 115200 bps
    //
    WRITE_REGISTER_UCHAR(UART0_LCR, LCR_DIVL_EN);
    WRITE_REGISTER_UCHAR(UART0_DLL, BaudClock);
    WRITE_REGISTER_UCHAR(UART0_DLH, (BaudClock >> 8) & 0xFF);
    
    //
    // Setup loopback
    //
    WRITE_REGISTER_UCHAR(UART0_MCR, MCR_DTR | MCR_RTS);
    
    //
    // Set 8 bits for data, 1 stop bit, no parity
    //
    WRITE_REGISTER_UCHAR(UART0_LCR, LCR_WLS_8 | LCR_1_STB | LCR_NO_PAR);
    
    //
    // Clear and enable FIFO
    //
    WRITE_REGISTER_UCHAR(UART0_FCR, FCR_FIFO_EN | FCR_RXSR | FCR_TXSR);
    
    //
    // Enable serial port
    //
    WRITE_REGISTER_UCHAR(UART0_MDR1, MDR1_UART16X);
}

VOID
ArmOmap3PutChar(IN INT Char)
{
    //
    // Properly support new-lines
    //
    if (Char == '\n') ArmOmap3PutChar('\r');
    
    //
    // Wait for ready
    //
    while ((READ_REGISTER_UCHAR(UART0_LSR) & LSR_THRE) == 0);
    
    //
    // Send the character
    //
    WRITE_REGISTER_UCHAR(UART0_THR, Char);
}

INT
ArmOmap3GetCh(VOID)
{
    //
    // Wait for ready
    //
    while ((READ_REGISTER_UCHAR(UART0_LSR) & LSR_DR) == 0);
    
    //
    // Read the character
    //
    return READ_REGISTER_UCHAR(UART0_RHR);
}

BOOLEAN
ArmOmap3KbHit(VOID)
{
    //
    // Return if something is ready
    //
    return ((READ_REGISTER_UCHAR(UART0_LSR) & LSR_DR) != 0);
}
