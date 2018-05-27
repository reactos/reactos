/*
 * PROJECT:     ReactOS Intel PRO/1000 Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Intel PRO/1000 driver definitions
 * COPYRIGHT:   Copyright 2018 Mark Jansen (mark.jansen@reactos.org)
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





/* Registers */
#define E1000_REG_CTRL              0x0000      /* Device Control Register, R/W */
#define E1000_REG_STATUS            0x0008      /* Device Status Register, R */
#define E1000_REG_EERD              0x0014      /* EEPROM Read Register, R/W */
#define E1000_REG_MDIC              0x0020      /* MDI Control Register, R/W */
#define E1000_REG_VET               0x0038      /* VLAN Ether Type, R/W */
#define E1000_REG_ICR               0x00C0      /* Interrupt Cause Read, R/clr */

#define E1000_REG_IMS               0x00D0      /* Interrupt Mask Set/Read Register, R/W */
#define E1000_REG_IMC               0x00D8      /* Interrupt Mask Clear, W */
#define E1000_REG_RCTL              0x0100      /* Receive Control, R/W */

#define E1000_REG_RAL               0x5400      /* Receive Address Low, R/W */
#define E1000_REG_RAH               0x5404      /* Receive Address High, R/W */


/* E1000_REG_CTRL */
#define E1000_CTRL_RST              (1 << 26)   /* Device Reset, Self clearing */


/* E1000_REG_STATUS */
#define E1000_STATUS_LU             (1 << 0)    /* Link Up Indication */
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
#define E1000_IMS_LSC               (1 << 2)    /* Sets mask for Link Status Change */


/* E1000_REG_RCTL */
#define E1000_RCTL_EN               (1 << 1)    /* Receiver Enable */
#define E1000_RCTL_SBP              (1 << 2)    /* Store Bad Packets */
#define E1000_RCTL_UPE              (1 << 3)    /* Unicast Promiscuous Enabled */
#define E1000_RCTL_MPE              (1 << 4)    /* Multicast Promiscuous Enabled */
#define E1000_RCTL_BAM              (1 << 15)   /* Broadcast Accept Mode */
#define E1000_RCTL_PMCF             (1 << 23)   /* Pass MAC Control Frames */

#define E1000_RCTL_FILTER_BITS      (E1000_RCTL_SBP | E1000_RCTL_UPE | E1000_RCTL_MPE | E1000_RCTL_BAM | E1000_RCTL_PMCF)

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

