/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Intel 8251A-based UART header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

#define SER_8251A_REG_DATA                     0x30
#define SER_8251A_REG_STATUS                   0x32

#define SR_8251A_STATUS_TxRDY                  0x01 /* Transmitter ready */
#define SR_8251A_STATUS_RxRDY                  0x02 /* Receiver ready */
#define SR_8251A_STATUS_TxEMPTY                0x04 /* Transmitter empty */
#define SR_8251A_STATUS_PE                     0x08 /* Parity error */
#define SR_8251A_STATUS_OE                     0x10 /* Overrun error */
#define SR_8251A_STATUS_FE                     0x20 /* Framing error */
#define SR_8251A_STATUS_SYNDET                 0x40 /* Sync detect / Break detect */
#define SR_8251A_STATUS_DSR                    0x80 /* Data set ready */

/* Parity generate/check */
#define SR_8251A_MODE_PEN                      0x10 /* Parity enable */
#define SR_8251A_MODE_EP                       0x20 /* Even parity generation/check */
#define SR_8251A_MODE_ESD                      0x40 /* External sync detect */
#define SR_8251A_MODE_SCS                      0x80 /* Single character sync */
/* Character length */
#define SR_8251A_MODE_LENGTH_5                 0x00
#define SR_8251A_MODE_LENGTH_6                 0x04
#define SR_8251A_MODE_LENGTH_7                 0x08
#define SR_8251A_MODE_LENGTH_8                 0x0C
/* Baud rate factor */
#define SR_8251A_MODE_SYNC                     0x00
#define SR_8251A_MODE_CLOCKx1                  0x01
#define SR_8251A_MODE_CLOCKx16                 0x02
#define SR_8251A_MODE_CLOCKx64                 0x03
/* Number of stop bits */
#define SR_8251A_MODE_1_STOP                   0x40
#define SR_8251A_MODE_1_5_STOP                 0x80
#define SR_8251A_MODE_2_STOP                   0xC0
/* Command bits */
#define SR_8251A_COMMMAND_TxEN                 0x01 /* Transmit enable */
#define SR_8251A_COMMMAND_DTR                  0x02 /* Data terminal ready */
#define SR_8251A_COMMMAND_RxEN                 0x04 /* Receive enable */
#define SR_8251A_COMMMAND_SBRK                 0x08 /* Send break character */
#define SR_8251A_COMMMAND_ER                   0x10 /* Error reset */
#define SR_8251A_COMMMAND_RTS                  0x20 /* Request to send */
#define SR_8251A_COMMMAND_IR                   0x40 /* Internal reset */
#define SR_8251A_COMMMAND_EH                   0x80 /* Enter hunt mode */

#define SER_8251F_REG_RBR                      0x130 /* Receive Buffer */
#define SER_8251F_REG_LSR                      0x132 /* Line Status */
#define SER_8251F_REG_MSR                      0x134 /* Modem Status */
#define SER_8251F_REG_IIR                      0x136 /* Interrupt ID */
#define SER_8251F_REG_FCR                      0x138 /* FIFO Control */
#define SER_8251F_REG_DLR                      0x13A /* Divisor Latch */

#define SR_8251F_LSR_TxEMPTY                   0x01 /* Transmitter empty */
#define SR_8251F_LSR_TxRDY                     0x02 /* Transmitter ready */
#define SR_8251F_LSR_RxRDY                     0x04 /* Receiver ready */
#define SR_8251F_LSR_OE                        0x10 /* Overrun error */
#define SR_8251F_LSR_PE                        0x20 /* Parity error */
#define SR_8251F_LSR_BI                        0x80 /* Break detect */

#define SR_8251F_MSR_CTS_CHANGED               0x01 /* Change in clear to send */
#define SR_8251F_MSR_DSR_CHANGED               0x02 /* Change in data set ready */
#define SR_8251F_MSR_RI_CHANGED                0x04 /* Trailing edge ring indicator */
#define SR_8251F_MSR_DCD_CHANGED               0x08 /* Change in carrier detect */
#define SR_8251F_MSR_CTS                       0x10 /* Clear to send */
#define SR_8251F_MSR_DSR                       0x20 /* Data set ready */
#define SR_8251F_MSR_RI                        0x40 /* Ring indicator */
#define SR_8251F_MSR_DCD                       0x80 /* Data carrier detect */

#define SR_8251F_IIR_MS                        0x00 /* Modem status change */
#define SR_8251F_IIR_THR                       0x02 /* Transmitter holding register empty */
#define SR_8251F_IIR_RDA                       0x04 /* Received data acailable */
#define SR_8251F_IIR_RLS                       0x06 /* Receiver line status change */
#define SR_8251F_IIR_CTI                       0x0C /* Character timeout */
#define SR_8251F_IIR_ID_MASK                   0x0F
#define SR_8251F_IIR_SELF                      0x01 /* No interrupt pending */
#define SR_8251F_IIR_MUST_BE_ZERO              0x20
#define SR_8251F_IIR_FIFO_DET                  0x40 /* Toggles for each read */

#define SR_8251F_FCR_DISABLE                   0x00 /* Disable FIFO */
#define SR_8251F_FCR_ENABLE                    0x01 /* Enable FIFO */
#define SR_8251F_FCR_RCVR_RESET                0x02 /* Clear receive FIFO */
#define SR_8251F_FCR_TXMT_RESET                0x04 /* Clear transmit FIFO */
/* Receive FIFO interrupt trigger level */
#define SR_8251F_FCR_1_BYTE_HIGH_WATER         0x00
#define SR_8251F_FCR_4_BYTE_HIGH_WATER         0x40
#define SR_8251F_FCR_8_BYTE_HIGH_WATER         0x80
#define SR_8251F_FCR_14_BYTE_HIGH_WATER        0xC0

#define SR_8251F_DLR_BAUD_115200               0x01
#define SR_8251F_DLR_BAUD_57600                0x02
#define SR_8251F_DLR_BAUD_38400                0x03
#define SR_8251F_DLR_BAUD_28800                0x04
#define SR_8251F_DLR_BAUD_19200                0x06
#define SR_8251F_DLR_BAUD_14400                0x08
#define SR_8251F_DLR_BAUD_9600                 0x0C
#define SR_8251F_DLR_MODE_VFAST                0x80
#define SR_8251F_DLR_MODE_LEGACY               0x00
