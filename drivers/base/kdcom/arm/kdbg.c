/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            drivers/base/kdcom/arm/kdbg.c
 * PURPOSE:         Serial Port Kernel Debugging Transport Library
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#define NOEXTAPI
#include <ntddk.h>
#define NDEBUG
#include <halfuncs.h>
#include <stdio.h>
#include <debug.h>
#include "arc/arc.h"
#include "windbgkd.h"
#include <kddll.h>
#include <ioaccess.h>
#include <arm/peripherals/pl011.h>

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
KdPortInitialize(IN PKD_PORT_INFORMATION PortInformation,
                 IN ULONG Unknown1,
                 IN ULONG Unknown2)
{
    //
    // Call the extended version
    //
    return KdPortInitializeEx(PortInformation, Unknown1, Unknown2);
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

BOOLEAN
NTAPI
KdPortGetByte(OUT PUCHAR ByteReceived)
{
    //
    // Call the extended version
    //
    return KdPortGetByteEx(&DefaultPort, ByteReceived); 
}

BOOLEAN
NTAPI
KdPortPollByteEx(IN PKD_PORT_INFORMATION PortInformation,
                 OUT PUCHAR ByteReceived)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

BOOLEAN
NTAPI
KdPortPollByte(OUT PUCHAR ByteReceived)
{
    //
    // Call the extended version
    //
    return KdPortPollByteEx(&DefaultPort, ByteReceived);
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

VOID
NTAPI
KdPortPutByte(IN UCHAR ByteToSend)
{
    //
    // Call the extended version
    //
    KdPortPutByteEx(&DefaultPort, ByteToSend);
}

VOID
NTAPI
KdPortRestore(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

VOID
NTAPI
KdPortSave(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
}

BOOLEAN
NTAPI
KdPortDisableInterrupts(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

BOOLEAN
NTAPI
KdPortEnableInterrupts(VOID)
{
    UNIMPLEMENTED;
    while (TRUE);
    return TRUE;
}

/* WINDOWS FUNCTIONS **********************************************************/

NTSTATUS
NTAPI
KdDebuggerInitialize0(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KdDebuggerInitialize1(IN PLOADER_PARAMETER_BLOCK LoaderBlock OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
KdSave(IN BOOLEAN SleepTransition)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
KdRestore(IN BOOLEAN SleepTransition)
{
    UNIMPLEMENTED;
    while (TRUE);
    return STATUS_SUCCESS;
}

VOID
NTAPI
KdSendPacket(IN ULONG PacketType,
             IN PSTRING MessageHeader,
             IN PSTRING MessageData,
             IN OUT PKD_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
    return;
}

KDSTATUS
NTAPI
KdReceivePacket(IN ULONG PacketType,
                OUT PSTRING MessageHeader,
                OUT PSTRING MessageData,
                OUT PULONG DataLength,
                IN OUT PKD_CONTEXT Context)
{
    UNIMPLEMENTED;
    while (TRUE);
    return 0;
}

/* EOF */
