/*
 * PROJECT:     NEC PC-98 series onboard hardware
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     UART header file
 * COPYRIGHT:   Copyright 2020 Dmitry Borisov (di.sean@protonmail.com)
 */

#pragma once

/* COM1 (Intel 8251A-based UART) **********************************************/

/*
 * UART registers and definitions
 */
#define SER1_IO_i_DATA               0x030
#define SER1_IO_i_STATUS             0x032
    #define SER1_STATUS_TxRDY             0x01 /* Transmitter ready */
    #define SER1_STATUS_RxRDY             0x02 /* Receiver ready */
    #define SER1_STATUS_TxEMPTY           0x04 /* Transmitter empty */
    #define SER1_STATUS_PE                0x08 /* Parity error */
    #define SER1_STATUS_OE                0x10 /* Overrun error */
    #define SER1_STATUS_FE                0x20 /* Framing error */
    #define SER1_STATUS_SYNDET            0x40 /* Sync detect / Break detect */
    #define SER1_STATUS_DSR               0x80 /* Data set ready */
#define SER1_IO_i_RECEIVER_BUFFER    0x130
#define SER1_IO_i_LINE_STATUS        0x132
    #define SER1_LSR_TxEMPTY              0x01 /* Transmitter empty */
    #define SER1_LSR_TxRDY                0x02 /* Transmitter ready */
    #define SER1_LSR_RxRDY                0x04 /* Receiver ready */
    #define SER1_LSR_OE                   0x10 /* Overrun error */
    #define SER1_LSR_PE                   0x20 /* Parity error */
    #define SER1_LSR_BI                   0x80 /* Break detect */
#define SER1_IO_i_MODEM_STATUS       0x134
    #define SER_MSR_CTS_CHANGED           0x01 /* Change in clear to send */
    #define SER_MSR_DSR_CHANGED           0x02 /* Change in data set ready */
    #define SER_MSR_RI_CHANGED            0x04 /* Trailing edge ring indicator */
    #define SER_MSR_DCD_CHANGED           0x08 /* Change in carrier detect */
    #define SER_MSR_CTS                   0x10 /* Clear to send */
    #define SER_MSR_DSR                   0x20 /* Data set ready */
    #define SER_MSR_RI                    0x40 /* Ring indicator */
    #define SER_MSR_DCD                   0x80 /* Data carrier detect */
#define SER1_IO_i_INTERRUPT_ID       0x136
    #define SER_IIR_MS                    0x00 /* Modem status change */
    #define SER_IIR_THR                   0x02 /* Transmitter holding register empty */
    #define SER_IIR_RDA                   0x04 /* Received data acailable */
    #define SER_IIR_RLS                   0x06 /* Receiver line status change */
    #define SER_IIR_CTI                   0x0C /* Character timeout */
    #define SER_IIR_ID_MASK               0x0F
    #define SER_IIR_SELF                  0x01 /* No interrupt pending */
    #define SER1_IIR_MUST_BE_ZERO         0x20
    #define SER1_IIR_FIFOS_ENABLED        0x40 /* Toggles for each read */
#define SER1_IO_i_FIFO_CONTROL       0x138
#define SER1_IO_i_DIVISOR_LATCH      0x13A

#define SER1_IO_o_DATA               0x030
#define SER1_IO_o_MODE_COMMAND       0x032
    /* Parity generate/check */
    #define SER1_MODE_PEN                 0x10 /* Parity enable */
    #define SER1_MODE_EP                  0x20 /* Even parity generation/check */
    #define SER1_MODE_ESD                 0x40 /* External sync detect */
    #define SER1_MODE_SCS                 0x80 /* Single character sync */
    /* Character length */
    #define SER1_MODE_LENGTH_5            0x00
    #define SER1_MODE_LENGTH_6            0x04
    #define SER1_MODE_LENGTH_7            0x08
    #define SER1_MODE_LENGTH_8            0x0C
    /* Baud rate factor */
    #define SER1_MODE_SYNC                0x00
    #define SER1_MODE_CLOCKx1             0x01
    #define SER1_MODE_CLOCKx16            0x02
    #define SER1_MODE_CLOCKx64            0x03
    /* Number of stop bits */
    #define SER1_MODE_1_STOP              0x40
    #define SER1_MODE_1_5_STOP            0x80
    #define SER1_MODE_2_STOP              0xC0
    /* Command bits */
    #define SER1_COMMMAND_TxEN            0x01 /* Transmit enable */
    #define SER1_COMMMAND_DTR             0x02 /* Data terminal ready */
    #define SER1_COMMMAND_RxEN            0x04 /* Receive enable */
    #define SER1_COMMMAND_SBRK            0x08 /* Send break character */
    #define SER1_COMMMAND_ER              0x10 /* Error reset */
    #define SER1_COMMMAND_RTS             0x20 /* Request to send */
    #define SER1_COMMMAND_IR              0x40 /* Internal reset */
    #define SER1_COMMMAND_EH              0x80 /* Enter hunt mode */
#define SER1_IO_o_TRANSMITTER_BUFFER 0x130
#define SER1_IO_o_FIFO_CONTROL       0x138
    #define SER_FCR_DISABLE               0x00 /* Disable FIFO */
    #define SER_FCR_ENABLE                0x01 /* Enable FIFO */
    #define SER_FCR_RCVR_RESET            0x02 /* Clear receive FIFO */
    #define SER_FCR_TXMT_RESET            0x04 /* Clear transmit FIFO */
    /* Receive FIFO interrupt trigger level */
    #define SER_FCR_1_BYTE_HIGH_WATER     0x00
    #define SER_FCR_4_BYTE_HIGH_WATER     0x40
    #define SER_FCR_8_BYTE_HIGH_WATER     0x80
    #define SER_FCR_14_BYTE_HIGH_WATER    0xC0
#define SER1_IO_o_DIVISOR_LATCH      0x13A
    #define SER1_DLR_BAUD_115200          0x01
    #define SER1_DLR_BAUD_57600           0x02
    #define SER1_DLR_BAUD_38400           0x03
    #define SER1_DLR_BAUD_28800           0x04
    #define SER1_DLR_BAUD_19200           0x06
    #define SER1_DLR_BAUD_14400           0x08
    #define SER1_DLR_BAUD_9600            0x0C
    #define SER1_DLR_MODE_VFAST           0x80
    #define SER1_DLR_MODE_LEGACY          0x00

/* COM2 (National Semiconductor 16550 UART) ***********************************/

/*
 * UART registers and definitions
 */
#define SER2_IO_i_RECEIVER_BUFFER    0x238 /* If DLAB = 0 */
#define SER2_IO_i_DIVISOR_LATCH_LSB  0x238 /* If DLAB = 1 */
#define SER2_IO_i_INTERRUPT_EN       0x239 /* If DLAB = 0 */
#define SER2_IO_i_DIVISOR_LATCH_MSB  0x239 /* If DLAB = 1 */
#define SER2_IO_i_INTERRUPT_ID       0x23A
    /* Bits 0-3 same as for COM1 */
    #define SER2_IIR_MUST_BE_ZERO         0x30
    #define SER2_IIR_NO_FIFO              0x00
    #define SER2_IIR_HAS_FIFO             0x40
    #define SER2_IIR_FIFOS_ENABLED        0xC0
#define SER2_IO_i_LINE_CONTROL       0x23B
#define SER2_IO_i_MODEM_CONTROL      0x23C
#define SER2_IO_i_LINE_STATUS        0x23D
    #define SER2_LSR_DR                   0x01 /* Data ready */
    #define SER2_LSR_OE                   0x02 /* Overrun error */
    #define SER2_LSR_PE                   0x04 /* Parity error */
    #define SER2_LSR_FE                   0x80 /* Framing error */
    #define SER2_LSR_BI                   0x80 /* Break interrupt */
    #define SER2_LSR_THR_EMPTY            0x20 /* Transmit holding register empty */
    #define SER2_LSR_TSR_EMPTY            0x40 /* Transmitter FIFO empty */
    #define SER2_LSR_ERROR_IN_FIFO        0x80 /* FIFO error */
#define SER2_IO_i_MODEM_STATUS       0x23E
    /* Bits 0-7 same as for COM1 */
#define SER2_IO_i_SCRATCH            0x23F

#define SER2_IO_o_TRANSMITTER_BUFFER 0x238 /* If DLAB = 0 */
#define SER2_IO_o_DIVISOR_LATCH_LSB  0x238 /* If DLAB = 1 */
    #define SER2_DLR_BAUD_115200          0x0001
    #define SER2_DLR_BAUD_57600           0x0002
    #define SER2_DLR_BAUD_38400           0x0003
    #define SER2_DLR_BAUD_19200           0x0006
    #define SER2_DLR_BAUD_9600            0x000C
    #define SER2_DLR_BAUD_4800            0x0018
    #define SER2_DLR_BAUD_2400            0x0030
    #define SER2_DLR_BAUD_1200            0x0060
    #define SER2_DLR_BAUD_600             0x00C0
    #define SER2_DLR_BAUD_300             0x0180
#define SER2_IO_o_DIVISOR_LATCH_MSB  0x239 /* If DLAB = 1 */
#define SER2_IO_o_INTERRUPT_EN       0x239 /* If DLAB = 0 */
    #define SER2_IER_DATA_RECEIVED        0x01 /* Received data available */
    #define SER2_IER_THR_EMPTY            0x02 /* Transmitter holding register empty */
    #define SER2_IER_LSR_CHANGE           0x04 /* Receiver line register status change */
    #define SER2_IER_MSR_CHANGE           0x08 /* Modem status register change */
#define SER2_IO_o_FIFO_CONTROL       0x23A
    /* Bits 0-2, 6-7 same as for COM1 */
    #define SER2_FCR_DMA_SELECT           0x04
#define SER2_IO_o_LINE_CONTROL       0x23B
    /* Character length */
    #define SER2_LCR_LENGTH_5             0x00
    #define SER2_LCR_LENGTH_6             0x01
    #define SER2_LCR_LENGTH_7             0x02
    #define SER2_LCR_LENGTH_8             0x03
    /* Number of stop bits */
    #define SER2_LCR_ST1                  0x00 /* 1 */
    #define SER2_LCR_ST2                  0x04 /* 1.5 - 2 */
    /* Parity generate/check */
    #define SER2_LCR_NO_PARITY            0x00
    #define SER2_LCR_ODD_PARITY           0x08
    #define SER2_LCR_EVEN_PARITY          0x18
    #define SER2_LCR_MARK_PARITY          0x28
    #define SER2_LCR_SPACE_PARITY         0x38
    #define SER2_LCR_BREAK                0x40
    #define SER2_LCR_DLAB                 0x80 /* Divisor latch access bit */
#define SER2_IO_o_MODEM_CONTROL      0x23C
    #define SER2_MCR_DTR_STATE            0x01 /* Data terminal ready */
    #define SER2_MCR_RTS_STATE            0x02 /* Request to send */
    #define SER2_MCR_OUT_1                0x04
    #define SER2_MCR_OUT_2                0x08
    #define SER2_MCR_LOOPBACK             0x10
#define SER2_IO_o_LINE_STATUS        0x23D
#define SER2_IO_o_SCRATCH            0x23F

#define SER2_CLOCK_RATE    115200
