/*
 * COPYRIGHT:       GPL, see COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            drivers/base/kddll/kdcom.h
 * PURPOSE:         COM port definitions for the kernel debugger.
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@ewactos.org)
 */

#pragma once

#define COM_DAT 0x00
#define COM_IEN 0x01 /* interrupt enable register */
#define COM_FCR 0x02 /* FIFO Control Register */
#define COM_LCR 0x03 /* line control registers */
#define COM_MCR 0x04 /* modem control reg */
#define COM_LSR 0x05 /* line status register */
#define COM_MSR 0x06 /* modem status register */
#define COM_SCR 0x07 /* scratch register */
#define COM_DLL 0x00 /* divisor latch least sig */
#define COM_DLM 0x01 /* divisor latch most sig */

#define IEN_ERDA   0x01
#define IEN_ETHRE  0x02
#define IEN_ERLSI  0x04
#define IEN_EMS    0x08
#define IEN_ALL    0x0F
#define FCR_ENABLE_FIFO 0x01
#define FCR_CLEAR_RCVR  0x02
#define FCR_CLEAR_XMIT  0x04
#define LCR_CS5 0x00
#define LCR_CS6 0x01
#define LCR_CS7 0x02
#define LCR_CS8 0x03
#define LCR_ST1 0x00
#define LCR_ST2 0x04
#define LCR_PNO 0x00
#define LCR_POD 0x08
#define LCR_PEV 0x18
#define LCR_PMK 0x28
#define LCR_PSP 0x38
#define LCR_BRK 0x40
#define LCR_DLAB 0x80
#define MCR_DTR  0x01
#define MCR_RTS  0x02
#define MCR_OUT1 0x04 /* general purpose output */
#define MCR_OUT2 0x08
#define MCR_LOOP 0x10 /* loopback testing mode */
#define MCR_ALL (MCR_DTR | MCR_RTS | MCR_OUT1 | MCR_OUT2 | MCR_LOOP)
#define LSR_DR   0x01
#define LSR_TBE  0x20
#define MSR_CTS  0x10 /* (complemented) state of clear to send (CTS). */
#define MSR_DSR  0x20 /* (complemented) state of data set ready (DSR). */
#define MSR_RI   0x40 /* (complemented) state of ring indicator (RI). */
#define MSR_DCD  0x80 /* (complemented) state of data carrier detect (DCD). */
