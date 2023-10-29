/*
 * ReactOS AMD PCNet Driver
 *
 * Copyright (C) 2003 Vizzini <vizzini@plasmic.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * PURPOSE:
 *     PCNet hardware configuration constants
 * REVISIONS:
 *     01-Sept-2003 vizzini - Created
 * NOTES:
 *     - This file represents a clean re-implementation from the AMD
 *       PCNet II chip documentation (Am79C790A, pub# 19436).
 */

#pragma once

/* when in 32-bit mode, most registers require the top 16 bits be 0. */
#define MASK16(__x__) ((__x__) & 0x0000ffff)

#define NUMBER_OF_PORTS 0x20    /* number of i/o ports the board requires */

/* offsets of important registers */
#define RDP     0x10    /* same address in 16-bit and 32-bit IO mode */

#define RAP16   0x12
#define RESET16 0x14
#define BDP16   0x16

#define RAP32   0x14
#define RESET32 0x18
#define BDP32   0x1c

/* NOTE:  vmware doesn't support 32-bit i/o programming so we use 16-bit */
#define RAP RAP16
#define BDP BDP16

/* pci id of the device */
#define PCI_ID 0x20001022
#define VEN_ID 0x1022
#define DEV_ID 0x2000

/* software style constants */
#define SW_STYLE_0 0
#define SW_STYLE_1 1
#define SW_STYLE_2 2
#define SW_STYLE_3 3

/* control and status registers */
#define CSR0   0x0      /* controller status register */
#define CSR1   0x1      /* init block address 0 */
#define CSR2   0x2      /* init block address 1 */
#define CSR3   0x3      /* interrupt masks and deferral control */
#define CSR4   0x4      /* test and features control */
#define CSR5   0x5      /* extended control and interrupt */
#define CSR6   0x6      /* rx/tx descriptor table length */
#define CSR8   0x8      /* logical address filter 0 */
#define CSR9   0x9      /* logical address filter 1 */
#define CSR10  0xa      /* logical address filter 2 */
#define CSR11  0xb      /* logical address filter 3 */
#define CSR12  0xc      /* physical address register 0 */
#define CSR13  0xd      /* physical address register 1 */
#define CSR14  0xe      /* physical address register 2 */
#define CSR15  0xf      /* Mode */
#define CSR16  0x10     /* initialization block address lower */
#define CSR17  0x11     /* initialization block address upper */
#define CSR18  0x12     /* current receive buffer address lower */
#define CSR19  0x13     /* current receive buffer address upper */
#define CSR20  0x14     /* current transmit buffer address lower */
#define CSR21  0x15     /* current transmit buffer address upper */
#define CSR22  0x16     /* next receive buffer address lower */
#define CSR23  0x17     /* next receive buffer address upper */
#define CSR24  0x18     /* base address of receive descriptor ring lower */
#define CSR25  0x19     /* base address of receive descriptor ring upper */
#define CSR26  0x1a     /* next receive descriptor address lower */
#define CSR27  0x1b     /* next receive descriptor address upper */
#define CSR28  0x1c     /* current receive descriptor address lower */
#define CSR29  0x1d     /* current receive descriptor address upper */
#define CSR30  0x1e     /* base address of transmit descriptor ring lower */
#define CSR31  0x1f     /* base address of transmit descriptor ring upper */
#define CSR32  0x20     /* next transmit descriptor address lower */
#define CSR33  0x21     /* next transmit descriptor address upper */
#define CSR34  0x22     /* current transmit descriptor address lower */
#define CSR35  0x23     /* current transmit descriptor address upper */
#define CSR36  0x24     /* next next receive descriptor address lower */
#define CSR37  0x25     /* next next receive descriptor address upper */
#define CSR38  0x26     /* next next transmit descriptor address lower */
#define CSR39  0x27     /* next next transmit descriptor address upper */
#define CSR40  0x28     /* current receive byte count */
#define CSR41  0x29     /* current receive status */
#define CSR42  0x2a     /* current transmit byte count */
#define CSR43  0x2b     /* current transmit status */
#define CSR44  0x2c     /* next receive byte count */
#define CSR45  0x2d     /* next receive status */
#define CSR46  0x2e     /* poll time counter */
#define CSR47  0x2f     /* polling interval */
#define CSR58  0x3a     /* software style */
#define CSR60  0x3c     /* previous transmit descriptor address lower */
#define CSR61  0x3d     /* previous transmit descriptor address upper */
#define CSR62  0x3e     /* previous transmit byte count */
#define CSR63  0x3f     /* previous transmit status */
#define CSR64  0x40     /* next transmit buffer address lower */
#define CSR65  0x41     /* next transmit buffer address upper */
#define CSR66  0x42     /* next transmit byte count */
#define CSR67  0x43     /* next transmit status */
#define CSR72  0x48     /* receive descriptor ring counter */
#define CSR74  0x4a     /* transmit descriptor ring counter */
#define CSR76  0x4c     /* receive descriptor ring length */
#define CSR78  0x4e     /* transmit descriptor ring length */
#define CSR80  0x50     /* dma transfer counter and fifo watermark control */
#define CSR82  0x52     /* bus activity timer */
#define CSR84  0x54     /* dma address register lower */
#define CSR85  0x55     /* dma address register upper */
#define CSR86  0x56     /* buffer byte counter */
#define CSR88  0x58     /* chip id register lower */
#define CSR89  0x59     /* chip id register upper */
#define CSR94  0x5e     /* transmit time domain reflectometry count */
#define CSR100 0x64     /* bus timeout */
#define CSR112 0x70     /* missed frame count */
#define CSR114 0x72     /* receive collision count */
#define CSR122 0x7a     /* advanced feature control */
#define CSR124 0x7c     /* test register control */

/* bus configuration registers */
#define BCR2   0x2      /* miscellaneous configuration */
#define BCR4   0x4      /* link status led */
#define BCR5   0x5      /* led1 status */
#define BCR6   0x6      /* led2 status */
#define BCR7   0x7      /* led3 status */
#define BCR9   0x9      /* full-duplex control */
#define BCR16  0x10     /* i/o base address lower */
#define BCR17  0x11     /* i/o base address upper */
#define BCR18  0x12     /* burst and bus control register */
#define BCR19  0x13     /* eeprom control and status */
#define BCR20  0x14     /* software style */
#define BCR21  0x15     /* interrupt control */
#define BCR22  0x16     /* pci latency register */

/* CSR0 bits */
#define CSR0_INIT  0x1          /* read initialization block */
#define CSR0_STRT  0x2          /* start the chip */
#define CSR0_STOP  0x4          /* stop the chip */
#define CSR0_TDMD  0x8          /* transmit demand */
#define CSR0_TXON  0x10         /* transmit on */
#define CSR0_RXON  0x20         /* receive on */
#define CSR0_IENA  0x40         /* interrupt enabled */
#define CSR0_INTR  0x80         /* interrupting */
#define CSR0_IDON  0x100        /* initialization done */
#define CSR0_TINT  0x200        /* transmit interrupt */
#define CSR0_RINT  0x400        /* receive interrupt */
#define CSR0_MERR  0x800        /* memory error */
#define CSR0_MISS  0x1000       /* missed frame */
#define CSR0_CERR  0x2000       /* collision error */
#define CSR0_BABL  0x4000       /* babble */
#define CSR0_ERR   0x8000       /* error */

/* CSR3 bits */
#define CSR3_BSWP    0x4        /* byte swap */
#define CSR3_EMBA    0x8        /* enable modified backoff algorithm */
#define CSR3_DXMT2PD 0x10       /* disable transmit two-part deferral */
#define CSR3_LAPPEN  0x20       /* lookahead packet processing enable */
#define CSR3_DXSUFLO 0x40       /* disable transmit stop on underflow */
#define CSR3_IDONM   0x100      /* initialization done mask */
#define CSR3_TINTM   0x200      /* transmit interrupt mask */
#define CSR3_RINTM   0x400      /* receive interrupt mask */
#define CSR3_MERRM   0x800      /* memory error interrupt mask */
#define CSR3_MISSM   0x1000     /* missed frame interrupt mask */
#define CSR3_BABLM   0x4000     /* babble interrupt mask */

/* CSR4 bits */
#define CSR4_JABM      0x1      /* jabber interrupt mask */
#define CSR4_JAB       0x2      /* interrupt on jabber error */
#define CSR4_TXSTRTM   0x4      /* transmit start interrupt mask */
#define CSR4_TXSTRT    0x8      /* interrupt on transmit start */
#define CSR4_RCVCCOM   0x10     /* receive collision counter overflow mask */
#define CSR4_RCVCCO    0X20     /* interrupt on receive collision counter overflow */
#define CSR4_UINT      0x40     /* user interrupt */
#define CSR4_UINTCMD   0x80     /* user interrupt command */
#define CSR4_MFCOM     0x100    /* missed frame counter overflow mask */
#define CSR4_MFCO      0x200    /* interrupt on missed frame counter overflow */
#define CSR4_ASTRP_RCV 0x400    /* auto pad strip on receive */
#define CSR4_APAD_XMT  0x800    /* auto pad on transmit */
#define CSR4_DPOLL     0x1000   /* disable transmit polling */
#define CSR4_TIMER     0x2000   /* enable bus activity timer */
#define CSR4_DMAPLUS   0x4000   /* set to 1 for pci */
#define CSR4_EN124     0x8000   /* enable CSR124 access */

/* CSR5 bits */
#define CSR5_SPND      0x1      /* suspend */
#define CSR5_MPMODE    0x2      /* magic packet mode */
#define CSR5_MPEN      0x4      /* magic packet enable */
#define CSR5_MPINTE    0x8      /* magic packet interrupt enable */
#define CSR5_MPINT     0x10     /* magic packet interrupt */
#define CSR5_MPPLBA    0x20     /* magic packet physical logical broadcast accept */
#define CSR5_EXDINTE   0x40     /* excessive deferral interrupt enable */
#define CSR5_EXDINT    0x80     /* excessive deferral interrupt */
#define CSR5_SLPINTE   0x100    /* sleep interrupt enable */
#define CSR5_SLPINT    0x200    /* sleep interrupt */
#define CSR5_SINE      0x400    /* system interrupt enable */
#define CSR5_SINT      0x800    /* system interrupt */
#define CSR5_LTINTEN   0x4000   /* last transmit interrupt enable */
#define CSR5_TOKINTD   0x8000   /* transmit ok interrupt disable */

/* CSR15 bits */
#define CSR15_DRX      0x1      /* disable receiver */
#define CSR15_DTX      0x2      /* disable transmitter */
#define CSR15_LOOP     0x4      /* loopback enable */
#define CSR15_DXMTFCS  0x8      /* disable transmit fcs */
#define CSR15_FCOLL    0x10     /* force collision */
#define CSR15_DRTY     0x20     /* disable retry */
#define CSR15_INTL     0x40     /* internal loopback */
#define CSR15_PORTSEL0 0x80     /* port selection bit 0 */
#define CSR15_PORTSEL1 0x100    /* port selection bit 1 */
#define CSR15_LRT      0x200    /* low receive threshold - same as TSEL */
#define CSR15_TSEL     0x200    /* transmit mode select - same as LRT */
#define CSR15_MENDECL  0x400    /* mendec loopback mode */
#define CSR15_DAPC     0x800    /* disable automatic parity correction */
#define CSR15_DLNKTST  0x1000   /* disable link status */
#define CSR15_DRCVPA   0x2000   /* disable receive physical address */
#define CSR15_DRCVBC   0x4000   /* disable receive broadcast */
#define CSR15_PROM     0x8000   /* promiscuous mode */

/* CSR58 bits */
#define CSR58_SSIZE32  0x100    /* 32-bit software size */
#define CSR58_CSRPCNET 0x200    /* csr pcnet-isa configuration */
#define CSR58_APERREN  0x400    /* advanced parity error handling enable */

/* CSR124 bits */
#define CSR124_RPA     0x4      /* runt packet accept */

/* BCR2 bits */
#define BCR2_ASEL      0x2      /* auto-select media */
#define BCR2_AWAKE     0x4      /* select sleep mode */
#define BCR2_EADISEL   0x8      /* eadi select */
#define BCR2_DXCVRPOL  0x10     /* dxcvr polarity */
#define BCR2_DXCVRCTL  0x20     /* dxcvr control */
#define BCR2_INTLEVEL  0x80     /* interrupt level/edge */
#define BCR2_APROMWE   0x100    /* address prom write enable */
#define BCR2_LEDPE     0x1000   /* LED programming enable */
#define BCR2_TMAULOOP  0x4000   /* t-mau transmit on loopback */

/* BCR4 bits */
#define BCR4_COLE      0x1      /* collision status enable */
#define BCR4_JABE      0x2      /* jabber status enable */
#define BCR4_RCVE      0x4      /* receive status enable */
#define BCR4_RXPOLE    0x8      /* receive polarity status enable */
#define BCR4_XMTE      0x10     /* transmit status enable */
#define BCR4_RCVME     0x20     /* receive match status enable */
#define BCR4_LNKSTE    0x40     /* link status enable */
#define BCR4_PSE       0x80     /* pulse stretcher enable */
#define BCR4_FDLSE     0x100    /* full-duplex link status enable */
#define BCR4_MPSE      0x200    /* magic packet status enable */
#define BCR4_E100      0x1000   /* link speed */
#define BCR4_LEDDIS    0x2000   /* led disable */
#define BCR4_LEDPOL    0x4000   /* led polarity */
#define BCR4_LEDOUT    0x8000   /* led output pin value */

/* BCR5 bits */
#define BCR5_COLE      0x1      /* collision status enable */
#define BCR5_JABE      0x2      /* jabber status enable */
#define BCR5_RCVE      0x4      /* receive status enable */
#define BCR5_RXPOLE    0x8      /* receive polarity status enable */
#define BCR5_XMTE      0x10     /* transmit status enable */
#define BCR5_RCVME     0x20     /* receive match status enable */
#define BCR5_LNKSTE    0x40     /* link status enable */
#define BCR5_PSE       0x80     /* pulse stretcher enable */
#define BCR5_FDLSE     0x100    /* full-duplex link status enable */
#define BCR5_MPSE      0x200    /* magic packet status enable */
#define BCR5_E100      0x1000   /* link speed */
#define BCR5_LEDDIS    0x2000   /* led disable */
#define BCR5_LEDPOL    0x4000   /* led polarity */
#define BCR5_LEDOUT    0x8000   /* led output pin value */

/* BCR6 bits */
#define BCR6_COLE      0x1      /* collision status enable */
#define BCR6_JABE      0x2      /* jabber status enable */
#define BCR6_RCVE      0x4      /* receive status enable */
#define BCR6_RXPOLE    0x8      /* receive polarity status enable */
#define BCR6_XMTE      0x10     /* transmit status enable */
#define BCR6_RCVME     0x20     /* receive match status enable */
#define BCR6_LNKSTE    0x40     /* link status enable */
#define BCR6_PSE       0x80     /* pulse stretcher enable */
#define BCR6_FDLSE     0x100    /* full-duplex link status enable */
#define BCR6_MPSE      0x200    /* magic packet status enable */
#define BCR6_E100      0x1000   /* link speed */
#define BCR6_LEDDIS    0x2000   /* led disable */
#define BCR6_LEDPOL    0x4000   /* led polarity */
#define BCR6_LEDOUT    0x8000   /* led output pin value */

/* BCR7 bits */
#define BCR7_COLE      0x1      /* collision status enable */
#define BCR7_JABE      0x2      /* jabber status enable */
#define BCR7_RCVE      0x4      /* receive status enable */
#define BCR7_RXPOLE    0x8      /* receive polarity status enable */
#define BCR7_XMTE      0x10     /* transmit status enable */
#define BCR7_RCVME     0x20     /* receive match status enable */
#define BCR7_LNKSTE    0x40     /* link status enable */
#define BCR7_PSE       0x80     /* pulse stretcher enable */
#define BCR7_FDLSE     0x100    /* full-duplex link status enable */
#define BCR7_MPSE      0x200    /* magic packet status enable */
#define BCR7_E100      0x1000   /* link speed */
#define BCR7_LEDDIS    0x2000   /* led disable */
#define BCR7_LEDPOL    0x4000   /* led polarity */
#define BCR7_LEDOUT    0x8000   /* led output pin value */

/* BCR9 bits */
#define BCR9_FDEN      0x1      /* full-duplex enable */
#define BCR9_AUIFD     0x2      /* aui full-duplex */
#define BCR9_FDRPAD    0x4      /* full-duplex runt packet accept disable */

/* BCR18 bits */
#define BCR18_BWRITE   0x20     /* burst write enable */
#define BCR18_BREADE   0x40     /* burst read enable */
#define BCR18_DWIO     0x80     /* dword i/o enable */
#define BCR18_EXTREQ   0x100    /* extended request */
#define BCR18_MEMCMD   0x200    /* memory command */

/* BCR19 bits */
#define BCR19_EDI      0x1      /* eeprom data in - same as EDO */
#define BCR19_ED0      0x1      /* eeprom data out - same as EDI */
#define BCR19_ESK      0x2      /* eeprom serial clock */
#define BCR19_ECS      0x4      /* eeprom chip select */
#define BCR19_EEN      0x8      /* eeprom port enable */
#define BCR19_EEDET    0x2000   /* eeprom detect */
#define BCR19_PREAD    0x4000   /* eeprom read */
#define BCR19_PVALID   0x8000   /* eeprom valid */

/* BCR20 bits */
#define BCR20_SSIZE32  0x100    /* 32-bit software size */
#define BCR20_CSRPCNET 0x200    /* csr pcnet-isa configuration */
#define BCR20_APERREN  0x400    /* advanced parity error handling enable */

/* initialization block for 32-bit software style */
typedef struct _INITIALIZATION_BLOCK
{
  USHORT MODE;          /* card mode (csr15) */
  UCHAR  RLEN;          /* encoded number of receive descriptor ring entries */
  UCHAR  TLEN;          /* encoded number of transmit descriptor ring entries */
  UCHAR  PADR[6];       /* physical address */
  USHORT RES;           /* reserved */
  UCHAR  LADR[8];       /* logical address */
  ULONG  RDRA;          /* receive descriptor ring address */
  ULONG  TDRA;          /* transmit descriptor ring address */
} INITIALIZATION_BLOCK, *PINITIALIZATION_BLOCK;

/* receive descriptor, software stle 2 (32-bit) */
typedef struct _RECEIVE_DESCRIPTOR
{
  ULONG  RBADR;         /* receive buffer address */
  USHORT BCNT;          /* two's compliment buffer byte count - NOTE: always OR with 0xf000 */
  USHORT FLAGS;         /* flags - always and with 0xfff0 */
  USHORT MCNT;          /* message byte count ; always AND with 0x0fff */
  UCHAR  RPC;           /* runt packet count */
  UCHAR  RCC;           /* receive collision count */
  ULONG  RES;           /* reserved */
} RECEIVE_DESCRIPTOR, *PRECEIVE_DESCRIPTOR;

/* receive descriptor flags */
#define RD_BAM         0x10     /* broadcast address match */
#define RD_LAFM        0x20     /* logical address filter match */
#define RD_PAM         0x40     /* physical address match */
#define RD_BPE         0x80     /* bus parity error */
#define RD_ENP         0x100    /* end of packet */
#define RD_STP         0x200    /* start of packet */
#define RD_BUFF        0x400    /* buffer error */
#define RD_CRC         0x800    /* crc error */
#define RD_OFLO        0x1000   /* overflow error */
#define RD_FRAM        0x2000   /* framing error */
#define RD_ERR         0x4000   /* an error bit is set */
#define RD_OWN         0x8000   /* buffer ownership (0=host, 1=nic) */

/* transmit descriptor, software style 2 */
typedef struct _TRANSMIT_DESCRIPTOR
{
  ULONG  TBADR;         /* transmit buffer address */
  USHORT BCNT;          /* two's compliment buffer byte count - OR with 0xf000 */
  USHORT FLAGS;         /* flags */
  USHORT TRC;           /* transmit retry count (AND with 0x000f */
  USHORT FLAGS2;        /* more flags */
  ULONG  RES;           /* reserved */
} TRANSMIT_DESCRIPTOR, *PTRANSMIT_DESCRIPTOR;

/* transmit descriptor flags */
#define TD1_BPE         0x80    /* bus parity error */
#define TD1_ENP         0x100   /* end of packet */
#define TD1_STP         0x200   /* start of packet */
#define TD1_DEF         0x400   /* frame transmission deferred */
#define TD1_ONE         0x800   /* exactly one retry was needed for transmission */
#define TD1_MORE        0x1000  /* more than 1 transmission retry required - same as LTINT */
#define TD1_LTINT       0x1000  /* suppress transmit success interrupt - same as MORE */
#define TD1_ADD_FCS     0x2000  /* force fcs generation - same as NO_FCS */
#define TD1_NO_FCS      0x2000  /* prevent fcs generation - same as ADD_FCS */
#define TD1_ERR         0x4000  /* an error bit is set */
#define TD1_OWN         0x8000  /* buffer ownership */

/* transmit descriptor flags2 flags */
#define TD2_RTRY        0x400   /* retry error */
#define TD2_LCAR        0x800   /* loss of carrier */
#define TD2_LCOL        0x1000  /* late collision */
#define TD2_EXDEF       0x2000  /* excessive deferral */
#define TD2_UFLO        0x4000  /* buffer underflow */
#define TD2_BUFF        0x8000  /* buffer error */
