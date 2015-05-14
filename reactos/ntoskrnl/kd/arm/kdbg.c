/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/base/kdcom/arm/kdbg.c
 * PURPOSE:         Serial Port Kernel Debugging Transport Library
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#include <arm/peripherals/pl011.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

CPPORT DefaultPort = {0, 0, 0};

//
// We need to build this in the configuration root and use KeFindConfigurationEntry
// to recover it later.
//
#define HACK 24000000

/* REACTOS FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
KdDebuggerInitialize1(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    return STATUS_NOT_IMPLEMENTED;
}

BOOLEAN
NTAPI
KdPortInitializeEx(IN PCPPORT PortInformation,
                   IN ULONG ComPortNumber)
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
    WRITE_REGISTER_ULONG((PULONG)UART_PL011_CR, 0);

    //
    // Set the baud rate
    //
    WRITE_REGISTER_ULONG((PULONG)UART_PL011_IBRD, Divider);
    WRITE_REGISTER_ULONG((PULONG)UART_PL011_FBRD, Fraction);

    //
    // Set 8 bits for data, 1 stop bit, no parity, FIFO enabled
    //
    WRITE_REGISTER_ULONG((PULONG)UART_PL011_LCRH,
                         UART_PL011_LCRH_WLEN_8 | UART_PL011_LCRH_FEN);

    //
    // Clear and enable FIFO
    //
    WRITE_REGISTER_ULONG((PULONG)UART_PL011_CR,
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
KdPortGetByteEx(IN PCPPORT PortInformation,
                OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    while (TRUE);
    return FALSE;
}

VOID
NTAPI
KdPortPutByteEx(IN PCPPORT PortInformation,
                IN UCHAR ByteToSend)
{
    //
    // Wait for ready
    //
    while ((READ_REGISTER_ULONG((PULONG)UART_PL01x_FR) & UART_PL01x_FR_TXFF) != 0);

    //
    // Send the character
    //
    WRITE_REGISTER_ULONG((PULONG)UART_PL01x_DR, ByteToSend);
}

/* EOF */
