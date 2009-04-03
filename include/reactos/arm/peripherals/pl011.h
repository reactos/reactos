/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/arm/peripherals/pl011.h
 * PURPOSE:         PL011 Registers and Constants
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* GLOBALS ********************************************************************/

//
// UART Registers
//
#define UART_BASE                (PVOID)0xE00F1000 /* HACK: freeldr mapped it here */

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
typedef union _PL011_LCR_REGISTER
{
    ULONG Todo;
} PL011_LCR_REGISTER, *PPL011_LCR_REGISTER;

#define UART_PL011_LCRH_WLEN_8   0x60
#define UART_PL011_LCRH_FEN      0x10

//
// FCR Values
//
typedef union _PL011_FCR_REGISTER
{
    ULONG Todo;
} PL011_FCR_REGISTER, *PPL011_FCR_REGISTER;

#define UART_PL011_CR_UARTEN     0x01
#define UART_PL011_CR_TXE        0x100
#define UART_PL011_CR_RXE        0x200

//
// LSR Values
//
typedef union _PL011_LSR_REGISTER
{
    ULONG Todo;
} PL011_LSR_REGISTER, *PPL011_LSR_REGISTER;

#define UART_PL01x_FR_RXFE       0x10
#define UART_PL01x_FR_TXFF       0x20
