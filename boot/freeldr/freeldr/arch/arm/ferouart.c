/*
 * PROJECT:         ReactOS Boot Loader
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            boot/freeldr/arch/arm/ferouart.c
 * PURPOSE:         Implements code for Feroceon boards using the 16550 UART
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <freeldr.h>

/* GLOBALS ********************************************************************/

//
// UART Registers
//
#define UART0_RBR   (ArmBoardBlock->UartRegisterBase + 0x00)
#define UART0_THR   UART0_RBR
#define UART0_IER   (ArmBoardBlock->UartRegisterBase + 0x04)
#define UART0_FCR   (ArmBoardBlock->UartRegisterBase + 0x08)
#define UART0_LCR   (ArmBoardBlock->UartRegisterBase + 0x0C)
#define UART0_MCR   (ArmBoardBlock->UartRegisterBase + 0x10)
#define UART0_LSR   (ArmBoardBlock->UartRegisterBase + 0x14)
#define UART0_MSR   (ArmBoardBlock->UartRegisterBase + 0x18)
#define UART0_SCR   (ArmBoardBlock->UartRegisterBase + 0x1C)

//
// When we enable the divisor latch
//
#define UART0_DLL   UART0_RBR
#define UART0_DLM   UART0_IER

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

/* FUNCTIONS ******************************************************************/

VOID
ArmFeroSerialInit(IN ULONG Baudrate)
{
    ULONG BaudClock;
    
    //
    // Calculate baudrate clock divider to set the baud rate
    //
    BaudClock = (ArmBoardBlock->ClockRate / 16) / Baudrate;
    
    //
    // Disable interrupts
    //
    WRITE_REGISTER_UCHAR(UART0_IER, 0);
    
    //
    // Set the baud rate to 115200 bps
    //
    WRITE_REGISTER_UCHAR(UART0_LCR, LCR_DIVL_EN);
    WRITE_REGISTER_UCHAR(UART0_DLL, BaudClock);
    WRITE_REGISTER_UCHAR(UART0_DLM, (BaudClock >> 8) & 0xFF);
    
    //
    // Set 8 bits for data, 1 stop bit, no parity
    //
    WRITE_REGISTER_UCHAR(UART0_LCR, LCR_WLS_8 | LCR_1_STB | LCR_NO_PAR);
    
    //
    // Clear and enable FIFO
    //
    WRITE_REGISTER_UCHAR(UART0_FCR, FCR_FIFO_EN | FCR_RXSR | FCR_TXSR);
}

VOID
ArmFeroPutChar(IN INT Char)
{
    //
    // Properly support new-lines
    //
    if (Char == '\n') ArmFeroPutChar('\r');
    
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
ArmFeroGetCh(VOID)
{
    //
    // Wait for ready
    //
    while ((READ_REGISTER_UCHAR(UART0_LSR) & LSR_DR) == 0);
    
    //
    // Read the character
    //
    return READ_REGISTER_UCHAR(UART0_RBR);
}

BOOLEAN
ArmFeroKbHit(VOID)
{
    //
    // Return if something is ready
    //
    return ((READ_REGISTER_UCHAR(UART0_LSR) & LSR_DR) != 0);
}
