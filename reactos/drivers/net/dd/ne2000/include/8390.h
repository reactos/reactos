/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Novell Eagle 2000 driver
 * FILE:        include/8390.h
 * PURPOSE:     National Semiconductor 8390 NIC definitions
 */
#ifndef __8390_H
#define __8390_H

/* Page 0 register layout (PS1 = 0, PS0 = 0) */
#define PG0_CR      0x00    /* Command Register (R/W) */
#define PG0_CLDA0   0x01    /* Current Local DMA Address 0 (R) */
#define PG0_PSTART  0x01    /* Page Start Register (W) */
#define PG0_CLDA1   0x02    /* Current Local DMA Address 1 (R) */
#define PG0_PSTOP   0x02    /* Page Stop Register (W) */
#define PG0_BNRY    0x03    /* Boundary Pointer (R/W) */
#define PG0_TSR     0x04    /* Transmit Status Register (R) */
#define PG0_TPSR    0x04    /* Transmit Page Start Register (W) */
#define PG0_NCR     0x05    /* Number of Collisions Register (R) */
#define PG0_TBCR0   0x05    /* Transmit Byte Count Register 0 (W) */
#define PG0_FIFO    0x06    /* FIFO (R) */
#define PG0_TBCR1   0x06    /* Transmit Byte Count Register 1 (W) */
#define PG0_ISR     0x07    /* Interrupt Status Register (R/W) */
#define PG0_CRDA0   0x08    /* Current Remote DMA Address 0 (R) */
#define PG0_RSAR0   0x08    /* Remote Start Address Register 0 (W) */
#define PG0_CRDA1   0x09    /* Current Remote DMA Address 1 (R) */
#define PG0_RSAR1   0x09    /* Remote Start Address Register 1 (W) */
#define PG0_RBCR0   0x0A    /* Remote Byte Count Register 0 (W) */
#define PG0_RBCR1   0x0B    /* Remote Byte Count Register 1 (W) */
#define PG0_RSR     0x0C    /* Receive Status Register (R) */
#define PG0_RCR     0x0C    /* Receive Configuration Register (W) */
#define PG0_CNTR0   0x0D    /* Tally Counter 0 (Frame Alignment Errors) (R) */
#define PG0_TCR     0x0D    /* Transmit Configuration Register (W) */
#define PG0_CNTR1   0x0E    /* Tally Counter 1 (CRC Errors) (R) */
#define PG0_DCR     0x0E    /* Data Configuration Register (W) */
#define PG0_CNTR2   0x0F    /* Tally Counter 2 (Missed Packet Errors) (R) */
#define PG0_IMR     0x0F    /* Interrupt Mask Register (W) */

/* Page 1 register layout (PS1 = 0, PS0 = 1) */
#define PG1_CR      0x00    /* Command Register (R/W) */
#define PG1_PAR     0x01    /* Physical Address Registers (6 registers) (R/W) */
#define PG1_CURR    0x07    /* Current Page Register (R/W) */
#define PG1_MAR     0x08    /* Multicast Address Registers (8 registers) (R/W) */

/* Page 2 register layout (PS1 = 1, PS0 = 0) */
#define PG2_CR      0x00    /* Command Register (R/W) */
#define PG2_PSTART  0x01    /* Page Start Register (R) */
#define PG2_CLDA0   0x01    /* Current Local DMA Address 0 (W) */
#define PG2_PSTOP   0x02    /* Page Stop Register (R) */
#define PG2_CLDA1   0x02    /* Current Local DMA Address 1 (W) */
#define PG2_RNPP    0x03    /* Remote Next Packet Pointer (R/W) */
#define PG2_TPSR    0x04    /* Transmit Page Start Address (R) */
#define PG2_LNPP    0x05    /* Local Next Packet Pointer (R/W) */
#define PG2_AC1     0x06    /* Address Counter (Upper) (R/W) */
#define PG2_AC0     0x07    /* Address Counter (Lower) (R/W) */
#define PG2_RCR     0x0C    /* Receive Configuration Register (R) */
#define PG2_TCR     0x0D    /* Transmit Configuration Register (R) */
#define PG2_DCR     0x0E    /* Data Configuration Register (R) */
#define PG2_IMR     0x0F    /* Interrupt Mask Register (R) */

/* Bits in PGX_CR - Command Register */
#define CR_STP      0x01    /* Stop chip */
#define CR_STA      0x02    /* Start chip */
#define CR_TXP      0x04    /* Transmit a frame */
#define CR_RD0      0x08    /* Remote read */
#define CR_RD1      0x10    /* Remote write */
#define CR_RD2      0x20    /* Abort/complete remote DMA */
#define CR_PAGE0    0x00    /* Select page 0 of chip registers */
#define CR_PAGE1    0x40    /* Select page 1 of chip registers */
#define CR_PAGE2    0x80    /* Select page 2 of chip registers */

/* Bits in PG0_ISR - Interrupt Status Register */
#define ISR_PRX     0x01    /* Packet received, no errors */
#define ISR_PTX     0x02    /* Packet transmitted, no errors */
#define ISR_RXE     0x04    /* Receive error */
#define ISR_TXE     0x08    /* Transmit error */
#define ISR_OVW     0x10    /* Overwrite warning */
#define ISR_CNT     0x20    /* Counter overflow */
#define ISR_RDC     0x40    /* Remote DMA complete */
#define ISR_RST     0x80    /* Reset status */

/* Bits in PG0_TSR - Transmit Status Register */
#define TSR_PTX     0x01h   /* Packet transmitted without error */
#define TSR_COL     0x04h   /* Collided at least once */
#define TSR_ABT     0x08h   /* Collided 16 times and was dropped */
#define TSR_CRS     0x10h   /* Carrier sense lost */
#define TSR_FU      0x20h   /* Transmit FIFO Underrun */
#define TSR_CDH     0x40h   /* Collision detect heartbeat */
#define TSR_OWC     0x80h   /* Out of window collision */

/* Bits for PG0_RCR - Receive Configuration Register */
#define RCR_SEP     0x01    /* Save error packets */
#define RCR_AR      0x02    /* Accept runt packets */
#define RCR_AB      0x04    /* Accept broadcasts */
#define RCR_AM      0x08    /* Accept multicast */
#define RCR_PRO     0x10    /* Promiscuous physical addresses */
#define RCR_MON     0x20    /* Monitor mode */

/* Bits in PG0_RSR - Receive Status Register */
#define RSR_PRX     0x01    /* Received packet intact */
#define RSR_CRC     0x02    /* CRC error */
#define RSR_FAE     0x04    /* Frame alignment error */
#define RSR_FO      0x08    /* FIFO overrun */
#define RSR_MPA     0x10    /* Missed packet */
#define RSR_PHY     0x20    /* Physical/multicast address */
#define RSR_DIS     0x40    /* Receiver disabled (monitor mode) */
#define RSR_DFR     0x80    /* Deferring */

/* Bits in PG0_TCR - Transmit Configuration Register */
#define TCR_CRC  0x01       /* Inhibit CRC, do not append CRC */
#define TCR_LOOP 0x02       /* Set loopback mode */
#define TCR_LB01 0x06       /* Encoded loopback control */
#define TCR_ATD  0x08       /* Auto transmit disable */
#define TCR_OFST 0x10       /* Collision offset enable */

/* Bits in PG0_DCR - Data Configuration Register */
#define DCR_WTS     0x01    /* Word transfer mode selection */
#define DCR_BOS     0x02    /* Byte order selection */
#define DCR_LAS     0x04    /* Long address selection */
#define DCR_LS      0x08    /* Loopback select (when 0) */
#define DCR_ARM     0x10    /* Autoinitialize remote */
#define DCR_FT00    0x00    /* Burst length selection (1 word/2 bytes) */
#define DCR_FT01    0x20    /* burst length selection (2 words/4 bytes) */
#define DCR_FT10    0x40    /* Burst length selection (4 words/8 bytes) */
#define DCR_FT11    0x60    /* Burst length selection (6 words/12 bytes) */

/* Bits in PG0_IMR - Interrupt Mask Register */
#define IMR_PRXE    0x01    /* Packet received interrupt enable */
#define IMR_PTXE    0x02    /* Packet transmitted interrupt enable */
#define IMR_RXEE    0x04    /* Receive error interrupt enable */
#define IMR_TXEE    0x08    /* Transmit error interrupt enable */
#define IMR_OVWE    0x10    /* Overwrite warning interrupt enable */
#define IMR_CNTE    0x20    /* Counter overflow interrupt enable */
#define IMR_RDCE    0x40    /* Remote DMA complete interrupt enable */
#define IMR_ALLE    0x7F    /* All interrupts enable */


/* NIC prepended structure to a received packet */
typedef struct _PACKET_HEADER {
    UCHAR Status;           /* See RSR_* constants */
    UCHAR NextPacket;       /* Pointer to next packet in chain */
    USHORT PacketLength;    /* Length of packet including this header */
} PACKET_HEADER, PPACKET_HEADER;


#define NICDisableInterrupts(Adapter) \
    NdisRawWritePortUchar((Adapter)->IOBase + PG0_IMR, 0x00);

#define NICEnableInterrupts(Adapter) \
    NdisRawWritePortUchar((Adapter)->IOBase + PG0_IMR, (Adapter)->InterruptMask);

VOID MiniportHandleInterrupt(
    IN  NDIS_HANDLE MiniportAdapterContext);

#endif /* __8390_H */

/* EOF */
