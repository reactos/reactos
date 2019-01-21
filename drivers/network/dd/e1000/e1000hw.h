/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Hardware specific definitions
 * COPYRIGHT:   2018 Mark Jansen (mark.jansen@reactos.org)
 *              2019 Victor Perevertkin (victor.perevertkin@reactos.org)
 */

#pragma once

#define IEEE_802_ADDR_LENGTH 6

#define HW_VENDOR_INTEL     0x8086

#define MAX_RESET_ATTEMPTS  10

#define MAX_PHY_REG_ADDRESS         0x1F
#define MAX_PHY_READ_ATTEMPTS       1800

#define MAX_EEPROM_READ_ATTEMPTS    10000


#define MAXIMUM_MULTICAST_ADDRESSES 16


/* Ethernet frame header */
typedef struct _ETH_HEADER {
    UCHAR Destination[IEEE_802_ADDR_LENGTH];
    UCHAR Source[IEEE_802_ADDR_LENGTH];
    USHORT PayloadType;
} ETH_HEADER, *PETH_HEADER;


C_ASSERT(sizeof(ETH_HEADER) == 14);


typedef enum _E1000_RCVBUF_SIZE
{
    E1000_RCVBUF_2048 = 0,
    E1000_RCVBUF_1024 = 1,
    E1000_RCVBUF_512 = 2,
    E1000_RCVBUF_256 = 3,

    E1000_RCVBUF_INDEXMASK = 3,
    E1000_RCVBUF_RESERVED = 4 | 0,

    E1000_RCVBUF_16384 = 4 | 1,
    E1000_RCVBUF_8192 =  4 | 2,
    E1000_RCVBUF_4096 =  4 | 3,
} E1000_RCVBUF_SIZE;



#include <pshpack1.h>


/* 3.2.3 Receive Descriptor Format */

#define E1000_RDESC_STATUS_PIF          (1 << 7)    /* Passed in-exact filter */
#define E1000_RDESC_STATUS_IXSM         (1 << 2)    /* Ignore Checksum Indication */
#define E1000_RDESC_STATUS_EOP          (1 << 1)    /* End of Packet */
#define E1000_RDESC_STATUS_DD           (1 << 0)    /* Descriptor Done */

typedef struct _E1000_RECEIVE_DESCRIPTOR
{
    UINT64 Address;

    USHORT Length;
    USHORT Checksum;
    UCHAR Status;
    UCHAR Errors;
    USHORT Special;

} E1000_RECEIVE_DESCRIPTOR, *PE1000_RECEIVE_DESCRIPTOR;


/* 3.3.3 Legacy Transmit Descriptor Format */

#define E1000_TDESC_CMD_IDE             (1 << 7)    /* Interrupt Delay Enable */
#define E1000_TDESC_CMD_RS              (1 << 3)    /* Report Status */
#define E1000_TDESC_CMD_IFCS            (1 << 1)    /* Insert FCS */
#define E1000_TDESC_CMD_EOP             (1 << 0)    /* End Of Packet */

#define E1000_TDESC_STATUS_DD           (1 << 0)    /* Descriptor Done */

typedef struct _E1000_TRANSMIT_DESCRIPTOR
{
    UINT64 Address;

    USHORT Length;
    UCHAR ChecksumOffset;
    UCHAR Command;
    UCHAR Status;
    UCHAR ChecksumStartField;
    USHORT Special;

} E1000_TRANSMIT_DESCRIPTOR, *PE1000_TRANSMIT_DESCRIPTOR;

#include <poppack.h>


C_ASSERT(sizeof(E1000_RECEIVE_DESCRIPTOR) == 16);
C_ASSERT(sizeof(E1000_TRANSMIT_DESCRIPTOR) == 16);


/* Valid Range: 80-256 for 82542 and 82543 gigabit ethernet controllers
   Valid Range: 80-4096 for 82544 and newer */
#define NUM_TRANSMIT_DESCRIPTORS        128
#define NUM_RECEIVE_DESCRIPTORS         128



/* Registers */
#define E1000_REG_CTRL              0x0000      /* Device Control Register, R/W */
#define E1000_REG_STATUS            0x0008      /* Device Status Register, R */
#define E1000_REG_EERD              0x0014      /* EEPROM Read Register, R/W */
#define E1000_REG_MDIC              0x0020      /* MDI Control Register, R/W */
#define E1000_REG_VET               0x0038      /* VLAN Ether Type, R/W */
#define E1000_REG_ICR               0x00C0      /* Interrupt Cause Read, R/clr */
#define E1000_REG_ITR               0x00C4      /* Interrupt Throttling Register, R/W */

#define E1000_REG_IMS               0x00D0      /* Interrupt Mask Set/Read Register, R/W */
#define E1000_REG_IMC               0x00D8      /* Interrupt Mask Clear, W */

#define E1000_REG_RCTL              0x0100      /* Receive Control Register, R/W */
#define E1000_REG_TCTL              0x0400      /* Transmit Control Register, R/W */
#define E1000_REG_TIPG              0x0410      /* Transmit IPG Register, R/W */

#define E1000_REG_RDBAL             0x2800      /* Receive Descriptor Base Address Low, R/W */
#define E1000_REG_RDBAH             0x2804      /* Receive Descriptor Base Address High, R/W */
#define E1000_REG_RDLEN             0x2808      /* Receive Descriptor Length, R/W */
#define E1000_REG_RDH               0x2810      /* Receive Descriptor Head, R/W */
#define E1000_REG_RDT               0x2818      /* Receive Descriptor Tail, R/W */
#define E1000_REG_RDTR              0x2820      /* Receive Delay Timer, R/W */
#define E1000_REG_RADV              0x282C      /* Receive Absolute Delay Timer, R/W */

#define E1000_REG_TDBAL             0x3800      /* Transmit Descriptor Base Address Low, R/W */
#define E1000_REG_TDBAH             0x3804      /* Transmit Descriptor Base Address High, R/W */
#define E1000_REG_TDLEN             0x3808      /* Transmit Descriptor Length, R/W */
#define E1000_REG_TDH               0x3810      /* Transmit Descriptor Head, R/W */
#define E1000_REG_TDT               0x3818      /* Transmit Descriptor Tail, R/W */
#define E1000_REG_TIDV              0x3820      /* Transmit Interrupt Delay Value, R/W */
#define E1000_REG_TADV              0x382C      /* Transmit Absolute Delay Timer, R/W */


#define E1000_REG_RAL               0x5400      /* Receive Address Low, R/W */
#define E1000_REG_RAH               0x5404      /* Receive Address High, R/W */


/* E1000_REG_CTRL */
#define E1000_CTRL_LRST             (1 << 3)    /* Link Reset */
#define E1000_CTRL_ASDE             (1 << 5)    /* Auto-Speed Detection Enable */
#define E1000_CTRL_SLU              (1 << 6)    /* Set Link Up */
#define E1000_CTRL_RST              (1 << 26)   /* Device Reset, Self clearing */
#define E1000_CTRL_VME              (1 << 30)   /* VLAN Mode Enable */


/* E1000_REG_STATUS */
#define E1000_STATUS_FD             (1 << 0)    /* Full Duplex Indication */
#define E1000_STATUS_LU             (1 << 1)    /* Link Up Indication */
#define E1000_STATUS_SPEEDSHIFT     6           /* Link speed setting */
#define E1000_STATUS_SPEEDMASK      (3 << E1000_STATUS_SPEEDSHIFT)


/* E1000_REG_EERD */
#define E1000_EERD_START            (1 << 0)    /* Start Read*/
#define E1000_EERD_DONE             (1 << 4)    /* Read Done */
#define E1000_EERD_ADDR_SHIFT       8
#define E1000_EERD_DATA_SHIFT       16


/* E1000_REG_MDIC */
#define E1000_MDIC_REGADD_SHIFT     16          /* PHY Register Address */
#define E1000_MDIC_PHYADD_SHIFT     21          /* PHY Address (1=Gigabit, 2=PCIe) */
#define E1000_MDIC_PHYADD_GIGABIT   1
#define E1000_MDIC_OP_READ          (2 << 26)   /* Opcode */
#define E1000_MDIC_R                (1 << 28)   /* Ready Bit */
#define E1000_MDIC_E                (1 << 30)   /* Error */


/* E1000_REG_IMS */
#define E1000_IMS_TXDW              (1 << 0)    /* Transmit Descriptor Written Back */
#define E1000_IMS_TXQE              (1 << 1)    /* Transmit Queue Empty */
#define E1000_IMS_LSC               (1 << 2)    /* Sets mask for Link Status Change */
#define E1000_IMS_RXDMT0            (1 << 4)    /* Receive Descriptor Minimum Threshold Reached */
#define E1000_IMS_RXT0              (1 << 7)    /* Receiver Timer Interrupt */
#define E1000_IMS_TXD_LOW           (1 << 15)   /* Transmit Descriptor Low Threshold hit */
#define E1000_IMS_SRPD              (1 << 16)   /* Small Receive Packet Detection */


/* E1000_REG_ITR */
#define MAX_INTS_PER_SEC        2000
#define DEFAULT_ITR             1000000000/(MAX_INTS_PER_SEC * 256)


/* E1000_REG_RCTL */
#define E1000_RCTL_EN               (1 << 1)    /* Receiver Enable */
#define E1000_RCTL_SBP              (1 << 2)    /* Store Bad Packets */
#define E1000_RCTL_UPE              (1 << 3)    /* Unicast Promiscuous Enabled */
#define E1000_RCTL_MPE              (1 << 4)    /* Multicast Promiscuous Enabled */
#define E1000_RCTL_BAM              (1 << 15)   /* Broadcast Accept Mode */
#define E1000_RCTL_BSIZE_SHIFT      16
#define E1000_RCTL_PMCF             (1 << 23)   /* Pass MAC Control Frames */
#define E1000_RCTL_BSEX             (1 << 25)   /* Buffer Size Extension */
#define E1000_RCTL_SECRC            (1 << 26)   /* Strip Ethernet CRC from incoming packet */

#define E1000_RCTL_FILTER_BITS      (E1000_RCTL_SBP | E1000_RCTL_UPE | E1000_RCTL_MPE | E1000_RCTL_BAM | E1000_RCTL_PMCF)


/* E1000_REG_TCTL */
#define E1000_TCTL_EN               (1 << 1)    /* Transmit Enable */
#define E1000_TCTL_PSP              (1 << 3)    /* Pad Short Packets */

/* E1000_REG_TIPG */
#define E1000_TIPG_IPGT_DEF         (10 << 0)   /* IPG Transmit Time */
#define E1000_TIPG_IPGR1_DEF        (10 << 10)  /* IPG Receive Time 1 */
#define E1000_TIPG_IPGR2_DEF        (10 << 20)  /* IPG Receive Time 2 */


/* E1000_REG_RAH */
#define E1000_RAH_AV                (1 << 31)   /* Address Valid */




/* NVM */
#define E1000_NVM_REG_CHECKSUM      0x03f
#define NVM_MAGIC_SUM               0xBABA



/* PHY (Read with MDIC) */

#define E1000_PHY_STATUS            0x01
#define E1000_PHY_SPECIFIC_STATUS   0x11


/* E1000_PHY_STATUS */
#define E1000_PS_LINK_STATUS        (1 << 2)



/* E1000_PHY_SPECIFIC_STATUS */
#define E1000_PSS_SPEED_AND_DUPLEX  (1 << 11)   /* Speed and Duplex Resolved */
#define E1000_PSS_SPEEDSHIFT        14
#define E1000_PSS_SPEEDMASK         (3 << E1000_PSS_SPEEDSHIFT)
