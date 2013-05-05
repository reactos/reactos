/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/base/kdcom/arm/kdbg.c
 * PURPOSE:         Serial Port Kernel Debugging Transport Library
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#define NOEXTAPI
#include <ntifs.h>
#include <halfuncs.h>
#include <stdio.h>
#include <arc/arc.h>
#include <windbgkd.h>
#include <kddll.h>
#include <ioaccess.h>
#include <arm/peripherals/pl011.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

typedef struct _KD_PORT_INFORMATION
{
    ULONG ComPort;
    ULONG BaudRate;
    ULONG BaseAddress;
} KD_PORT_INFORMATION, *PKD_PORT_INFORMATION;

KD_PORT_INFORMATION DefaultPort = {0, 0, 0};

//
// We need to build this in the configuration root and use KeFindConfigurationEntry
// to recover it later.
//
#define HACK 24000000

/* REACTOS FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
KdPortInitializeEx(IN PKD_PORT_INFORMATION PortInformation,
                   IN ULONG Unknown1,
                   IN ULONG Unknown2)
{
    ULONG Divider, Remainder, Fraction;
    ULONG Baudrate = PortInformation->BaudRate;
    
    //
    // Calculate baudrate clock divider and remainder
    //
    Divider   = HACK / (16 * Baudrate);
    Remainder = HACK % (16 * Baudrate);
    
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
    // Set the baud rate
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
    
    //
    // Done
    //
    return TRUE;
}

BOOLEAN
NTAPI
KdPortGetByteEx(IN PKD_PORT_INFORMATION PortInformation,
                OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VOID
NTAPI
KdPortPutByteEx(IN PKD_PORT_INFORMATION PortInformation,
                IN UCHAR ByteToSend)
{
    //
    // Wait for ready
    //
    while ((READ_REGISTER_ULONG(UART_PL01x_FR) & UART_PL01x_FR_TXFF) != 0);
    
    //
    // Send the character
    //
    WRITE_REGISTER_ULONG(UART_PL01x_DR, ByteToSend);
}

/* EOF */
