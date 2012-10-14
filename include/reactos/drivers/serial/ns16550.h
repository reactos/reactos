/*
 * PROJECT:         ReactOS ComPort Library
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/drivers/serial/ns16550.h
 * PURPOSE:         Header for National Semiconductor 16550 UART
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* INCLUDES *******************************************************************/

#pragma once

/* Note: These definitions are the internal definitions used by Microsoft serial
   driver (see src/kernel/serial/serial.h in WDK source code). Linux uses its own, as
   do most other OS.
*/

#if !defined(SERIAL_REGISTER_STRIDE)
#define SERIAL_REGISTER_STRIDE 1
#endif

#define RECEIVE_BUFFER_REGISTER    ((ULONG)((0x00)*SERIAL_REGISTER_STRIDE))
#define TRANSMIT_HOLDING_REGISTER  ((ULONG)((0x00)*SERIAL_REGISTER_STRIDE))
#define INTERRUPT_ENABLE_REGISTER  ((ULONG)((0x01)*SERIAL_REGISTER_STRIDE))
#define INTERRUPT_IDENT_REGISTER   ((ULONG)((0x02)*SERIAL_REGISTER_STRIDE))
#define FIFO_CONTROL_REGISTER      ((ULONG)((0x02)*SERIAL_REGISTER_STRIDE))
#define LINE_CONTROL_REGISTER      ((ULONG)((0x03)*SERIAL_REGISTER_STRIDE))
#define MODEM_CONTROL_REGISTER     ((ULONG)((0x04)*SERIAL_REGISTER_STRIDE))
#define LINE_STATUS_REGISTER       ((ULONG)((0x05)*SERIAL_REGISTER_STRIDE))
#define MODEM_STATUS_REGISTER      ((ULONG)((0x06)*SERIAL_REGISTER_STRIDE))
#define DIVISOR_LATCH_LSB          ((ULONG)((0x00)*SERIAL_REGISTER_STRIDE))
#define DIVISOR_LATCH_MSB          ((ULONG)((0x01)*SERIAL_REGISTER_STRIDE))
#define SERIAL_REGISTER_SPAN       ((ULONG)(7*SERIAL_REGISTER_STRIDE))
#define SERIAL_STATUS_LENGTH       ((ULONG)(1*SERIAL_REGISTER_STRIDE))

#define SERIAL_DATA_LENGTH_5 0x00
#define SERIAL_DATA_LENGTH_6 0x01
#define SERIAL_DATA_LENGTH_7 0x02
#define SERIAL_DATA_LENGTH_8 0x03

#define SERIAL_IER_RDA   0x01
#define SERIAL_IER_THR   0x02
#define SERIAL_IER_RLS   0x04
#define SERIAL_IER_MS    0x08

#define SERIAL_IIR_RLS      0x06
#define SERIAL_IIR_RDA      0x04
#define SERIAL_IIR_CTI      0x0c
#define SERIAL_IIR_THR      0x02
#define SERIAL_IIR_MS       0x00
#define SERIAL_IIR_FIFOS_ENABLED 0xc0
#define SERIAL_IIR_NO_INTERRUPT_PENDING 0x01
#define SERIAL_IIR_MUST_BE_ZERO 0x30

#define SERIAL_FCR_ENABLE     ((UCHAR)0x01)
#define SERIAL_FCR_RCVR_RESET ((UCHAR)0x02)
#define SERIAL_FCR_TXMT_RESET ((UCHAR)0x04)

#define SERIAL_1_BYTE_HIGH_WATER   ((UCHAR)0x00)
#define SERIAL_4_BYTE_HIGH_WATER   ((UCHAR)0x40)
#define SERIAL_8_BYTE_HIGH_WATER   ((UCHAR)0x80)
#define SERIAL_14_BYTE_HIGH_WATER  ((UCHAR)0xc0)

#define SERIAL_LCR_DLAB     0x80
#define SERIAL_LCR_BREAK    0x40

#define SERIAL_5_DATA       ((UCHAR)0x00)
#define SERIAL_6_DATA       ((UCHAR)0x01)
#define SERIAL_7_DATA       ((UCHAR)0x02)
#define SERIAL_8_DATA       ((UCHAR)0x03)
#define SERIAL_DATA_MASK    ((UCHAR)0x03)

#define SERIAL_1_STOP       ((UCHAR)0x00)
#define SERIAL_1_5_STOP     ((UCHAR)0x04) // Only valid for 5 data bits
#define SERIAL_2_STOP       ((UCHAR)0x04) // Not valid for 5 data bits
#define SERIAL_STOP_MASK    ((UCHAR)0x04)

#define SERIAL_NONE_PARITY  ((UCHAR)0x00)
#define SERIAL_ODD_PARITY   ((UCHAR)0x08)
#define SERIAL_EVEN_PARITY  ((UCHAR)0x18)
#define SERIAL_MARK_PARITY  ((UCHAR)0x28)
#define SERIAL_SPACE_PARITY ((UCHAR)0x38)
#define SERIAL_PARITY_MASK  ((UCHAR)0x38)

#define SERIAL_MCR_DTR            0x01
#define SERIAL_MCR_RTS            0x02
#define SERIAL_MCR_OUT1           0x04
#define SERIAL_MCR_OUT2           0x08
#define SERIAL_MCR_LOOP           0x10
#define SERIAL_MCR_TL16C550CAFE   0x20

#define SERIAL_LSR_DR       0x01
#define SERIAL_LSR_OE       0x02
#define SERIAL_LSR_PE       0x04
#define SERIAL_LSR_FE       0x08
#define SERIAL_LSR_BI       0x10
#define SERIAL_LSR_THRE     0x20
#define SERIAL_LSR_TEMT     0x40
#define SERIAL_LSR_FIFOERR  0x80

#define SERIAL_MSR_DCTS     0x01
#define SERIAL_MSR_DDSR     0x02
#define SERIAL_MSR_TERI     0x04
#define SERIAL_MSR_DDCD     0x08
#define SERIAL_MSR_CTS      0x10
#define SERIAL_MSR_DSR      0x20
#define SERIAL_MSR_RI       0x40
#define SERIAL_MSR_DCD      0x80
