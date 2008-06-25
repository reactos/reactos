/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/arm/arm_kprintf.c
 * PURPOSE:         Early serial printf-style kernel debugging (ARM bringup)
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

//
// UART Registers
//
#define UART_BASE (void*)0xe00f1000 /* HACK: freeldr mapped it here */

#define UART_PL01x_DR            (UART_BASE + 0x00)
#define UART_PL01x_RSR           (UART_BASE + 0x04)
#define UART_PL01x_ECR           (UART_BASE + 0x04)
#define UART_PL01x_FR            (UART_BASE + 0x18)
#define UART_PL011_IBRD          (UART_BASE + 0x24)
#define UART_PL011_FBRD          (UART_BASE + 0x28)
#define UART_PL011_LCRH          (UART_BASE + 0x2C)
#define UART_PL011_CR            (UART_BASE + 0x30)
#define UART_PL011_IMSC          (UART_BASE + 0x38)

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

#define READ_REGISTER_ULONG(r) (*(volatile ULONG * const)(r))
#define WRITE_REGISTER_ULONG(r, v) (*(volatile ULONG *)(r) = (v))

/* FUNCTIONS ******************************************************************/

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

void arm_kprintf(const char *fmt, ...) {
	char buf[1024], *s;
	va_list args;

	va_start(args, fmt);
	_vsnprintf(buf,sizeof(buf),fmt,args);
	va_end(args);
	for (s = buf; *s; s++)
		ArmVersaPutChar(*s);
}
