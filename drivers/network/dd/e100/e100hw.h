/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Hardware specific definitions
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Hardware definitions were taken from the FreeBSD fxp driver.
 * Copyright (c) 1995, David Greenman
 * Copyright (c) 2001 Jonathan Lemon <jlemon@freebsd.org>
 */

#pragma once

/*
 * PCI Vendor and Device IDs
 */
#define PCI_VEN_INTEL        0x8086

#define PCI_DEV_8255x        0x1229
#define PCI_DEV_82559ER      0x1209
#define PCI_DEV_82559_CB     0x1029
#define PCI_DEV_82559        0x1030
#define PCI_DEV_82551QM      0x1059

#define PCI_DEV_ICH2         0x2449

#define PCI_DEV_ICH3_1       0x1031
#define PCI_DEV_ICH3_2       0x1032
#define PCI_DEV_ICH3_3       0x1033
#define PCI_DEV_ICH3_4       0x1034
#define PCI_DEV_ICH3_5       0x1035
#define PCI_DEV_ICH3_6       0x1036
#define PCI_DEV_ICH3_7       0x1037
#define PCI_DEV_ICH3_8       0x1038

#define PCI_DEV_ICH4_1       0x1039
#define PCI_DEV_ICH4_2       0x103A
#define PCI_DEV_ICH4_3       0x103B
#define PCI_DEV_ICH4_4       0x103C
#define PCI_DEV_ICH4_5       0x103D
#define PCI_DEV_ICH4_6       0x103E

#define PCI_DEV_ICH5_1       0x1050
#define PCI_DEV_ICH5_2       0x1051
#define PCI_DEV_ICH5_3       0x1052
#define PCI_DEV_ICH5_4       0x1053
#define PCI_DEV_ICH5_5       0x1054
#define PCI_DEV_ICH5_6       0x1055
#define PCI_DEV_ICH5_7       0x1056
#define PCI_DEV_ICH5_8       0x1057

#define PCI_DEV_C_ICH_1      0x2459
#define PCI_DEV_C_ICH_2      0x245D

#define PCI_DEV_ICH6_1       0x1064
#define PCI_DEV_ICH6_2       0x1065
#define PCI_DEV_ICH6_3       0x1066
#define PCI_DEV_ICH6_4       0x1067
#define PCI_DEV_ICH6_5       0x1068
#define PCI_DEV_ICH6_6       0x1069
#define PCI_DEV_ICH6_7       0x106A
#define PCI_DEV_ICH6_8       0x106B
#define PCI_DEV_ICH6_9       0x266C

#define PCI_DEV_ICH7_1       0x1091
#define PCI_DEV_ICH7_2       0x1092
#define PCI_DEV_ICH7_3       0x1093
#define PCI_DEV_ICH7_4       0x1094
#define PCI_DEV_ICH7_5       0x1095
#define PCI_DEV_ICH7_6       0x27DC
#define PCI_DEV_82552        0x10FE

#define FXP_PCI_IO_BAR_LENGTH     0x20
#define FXP_PCI_MMIO_BAR_LENGTH   0x1000

/*
 * Chip revision values
 */
#define FXP_REV_82557        1  // Catch all 82557 chip type
#define FXP_REV_82558_A4     4  // 82558 A4 stepping
#define FXP_REV_82558_B0     5  // 82558 B0 stepping
#define FXP_REV_82559_A0     8  // 82559 A0 stepping
#define FXP_REV_82559S_A     9  // 82559S A stepping
#define FXP_REV_82550        12
#define FXP_REV_82550_C      13 // 82550 C stepping
#define FXP_REV_82551_E      14 // 82551
#define FXP_REV_82551_F      15 // 82551
#define FXP_REV_82551_10     16 // 82551

/*
 * Control/status registers
 */
#define FXP_CSR_SCB_RUSCUS      0x00 // SCB Status (1 byte)
#define FXP_CSR_SCB_STATACK     0x01 // SCB STAT/ACK (1 byte)
#define FXP_CSR_SCB_COMMAND     0x02 // SCB Command (1 byte)
#define FXP_CSR_SCB_INTRCNTL    0x03 // SCB Interrupt Control (1 byte)
#define FXP_CSR_SCB_GENERAL     0x04 // SCB General Pointer (4 bytes)
#define FXP_CSR_PORT            0x08 // PORT Interface (4 bytes)
#define FXP_CSR_FLASHCONTROL    0x0C // FLASH Control (2 bytes)
#define FXP_CSR_EEPROMCONTROL   0x0E // EEPROM Control (2 bytes)
#define FXP_CSR_MDICONTROL      0x10 // MDI Сontrol (4 bytes)
#define FXP_CSR_FC_THRESH       0x19 // Flow Сontrol Threshold (1 byte)
#define FXP_CSR_FC_STATUS       0x1A // Flow Сontrol Status (1 byte)
#define FXP_CSR_PMDR            0x1B // Power management driver (1 byte)
#define FXP_CSR_GENCONTROL      0x1C // General control (1 byte)
#define FXP_CSR_GENSTATUS       0x1D // General status (1 byte)

/*
 * SCB Status Byte
 * FXP_CSR_SCB_RUSCUS
 */
#define FXP_SCB_RUS_IDLE             0
#define FXP_SCB_RUS_SUSPENDED        1
#define FXP_SCB_RUS_NORESOURCES      2
#define FXP_SCB_RUS_READY            4
#define FXP_SCB_RUS_SUSP_NORBDS      9
#define FXP_SCB_RUS_NORES_NORBDS     10
#define FXP_SCB_RUS_READY_NORBDS     12

#define FXP_SCB_CUS_IDLE             0
#define FXP_SCB_CUS_SUSPENDED        1
#define FXP_SCB_CUS_ACTIVE           2

#define FXP_SCB_RUS(Value)  (((Value) >> 2) & 0x0F)
#define FXP_SCB_CUS(Value)  (((Value) >> 6) & 0x03)

/*
 * SCB STAT/ACK Byte
 * FXP_CSR_SCB_STATACK
 */
#define FXP_SCB_STATACK_FCP     0x01
#define FXP_SCB_STATACK_ER      0x02
#define FXP_SCB_STATACK_SWI     0x04
#define FXP_SCB_STATACK_MDI     0x08
#define FXP_SCB_STATACK_RNR     0x10
#define FXP_SCB_STATACK_CNA     0x20
#define FXP_SCB_STATACK_FR      0x40
#define FXP_SCB_STATACK_CXTNO   0x80

/*
 * SCB Command Byte
 * FXP_CSR_SCB_COMMAND
 */
#define FXP_SCB_COMMAND_RU_NOP          0x00
#define FXP_SCB_COMMAND_RU_START        0x01
#define FXP_SCB_COMMAND_RU_RESUME       0x02
#define FXP_SCB_COMMAND_RU_ABORT        0x04
#define FXP_SCB_COMMAND_RU_LOADHDS      0x05
#define FXP_SCB_COMMAND_RU_BASE         0x06
#define FXP_SCB_COMMAND_RU_RBDRESUME    0x07

#define FXP_SCB_COMMAND_CU_NOP          0x00
#define FXP_SCB_COMMAND_CU_START        0x10
#define FXP_SCB_COMMAND_CU_RESUME       0x20
#define FXP_SCB_COMMAND_CU_DUMP_ADR     0x40
#define FXP_SCB_COMMAND_CU_DUMP         0x50
#define FXP_SCB_COMMAND_CU_BASE         0x60
#define FXP_SCB_COMMAND_CU_DUMPRESET    0x70

/*
 * SCB Interrupt Control Byte
 * FXP_CSR_SCB_INTRCNTL
 */
#define FXP_SCB_INTR_DISABLE    0x01
#define FXP_SCB_INTR_SWI        0x02
#define FXP_SCB_INTMASK_FCP     0x04
#define FXP_SCB_INTMASK_ER      0x08
#define FXP_SCB_INTMASK_RNR     0x10
#define FXP_SCB_INTMASK_CNA     0x20
#define FXP_SCB_INTMASK_FR      0x40
#define FXP_SCB_INTMASK_CXTNO   0x80

/*
 * Port Interface
 * FXP_CSR_PORT
 */
#define FXP_PORT_SOFTWARE_RESET    0
#define FXP_PORT_SELFTEST          1
#define FXP_PORT_SELECTIVE_RESET   2
#define FXP_PORT_DUMP              3

/*
 * EEPROM Control Register
 * FXP_CSR_EEPROMCONTROL
 */
#define FXP_EEPROM_EESK        0x01
#define FXP_EEPROM_EECS        0x02
#define FXP_EEPROM_EEDI        0x04
#define FXP_EEPROM_EEDO        0x08

#define FXP_EEPROM_EEDI_SHIFT  2
#define FXP_EEPROM_EEDO_SHIFT  3

#define FXP_EEPROM_OPC_ERASE   0x4
#define FXP_EEPROM_OPC_WRITE   0x5
#define FXP_EEPROM_OPC_READ    0x6

#define FXP_EEPROM_OPC_LENGTH  3

/*
 * MDI Control Register
 * FXP_CSR_MDICONTROL
 */
#define FXP_MDI_DATA_MASK        0x0000FFFF
#define FXP_MDI_READY            0x10000000
#define FXP_MDI_IE               0x20000000

#define FXP_MDI_REG_ADDR_SHIFT   16
#define FXP_MDI_PHY_ADDR_SHIFT   21
#define FXP_MDI_OPCODE_SHIFT     26

#define FXP_MDIO_WRITE       0x01
#define FXP_MDIO_READ        0x02

#define FXP_MIIPHY_DELAYMAX   10000
#define FXP_MIIPHY_DELAY      20

/*
 * General Status Register
 * FXP_CSR_GENSTATUS
 */
#define FXP_GENSTATUS_LINK_UP    0x01
#define FXP_GENSTATUS_LINK_100   0x02
#define FXP_GENSTATUS_LINK_FDX   0x04

/*
 * EEPROM map
 */
#define FXP_EEPROM_MAP_IA0       0x00 // Station address
#define FXP_EEPROM_MAP_IA1       0x01
#define FXP_EEPROM_MAP_IA2       0x02
#define FXP_EEPROM_MAP_COMPAT    0x03 // Compatibility
#define FXP_EEPROM_MAP_CNTR      0x05 // Controller/connector type
#define FXP_EEPROM_MAP_PRI_PHY   0x06 // Primary PHY record
#define FXP_EEPROM_MAP_SEC_PHY   0x07 // Secondary PHY record
#define FXP_EEPROM_MAP_PWA0      0x08 // Printed wire assembly num.
#define FXP_EEPROM_MAP_PWA1      0x09 // Printed wire assembly num.
#define FXP_EEPROM_MAP_ID        0x0A // EEPROM ID
#define FXP_EEPROM_MAP_SUBSYS    0x0B // Subsystem ID
#define FXP_EEPROM_MAP_SUBVEN    0x0C // Subsystem vendor ID
#define FXP_EEPROM_MAP_CKSUM64   0x3F // 64-word EEPROM checksum
#define FXP_EEPROM_MAP_CKSUM256  0xFF // 256-word EEPROM checksum

#define FXP_EEPROM_MAX_WORDS  256

/*
 * PHY device types
 */
#define FXP_PHY_DEVICE_MASK   0x3F00
#define FXP_PHY_SERIAL_ONLY   0x8000
#define FXP_PHY_NONE          0
#define FXP_PHY_82553A        1
#define FXP_PHY_82553C        2
#define FXP_PHY_82503         3
#define FXP_PHY_DP83840       4
#define FXP_PHY_80C240        5
#define FXP_PHY_80C24         6
#define FXP_PHY_82555         7
#define FXP_PHY_DP83840A      10
#define FXP_PHY_82555B        11

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define htole16(x)   (x)
#define htobe16(x)   _byteswap_ushort(x)
#define htole32(x)   (x)
#define letoh16(x)   (x)
#define betoh16(x)   _byteswap_ushort(x)
#define letoh32(x)   (x)

#define FXP_BITFIELD2(a, b)                     a, b
#define FXP_BITFIELD3(a, b, c)                  a, b, c
#define FXP_BITFIELD4(a, b, c, d)               a, b, c, d
#define FXP_BITFIELD5(a, b, c, d, e)            a, b, c, d, e
#define FXP_BITFIELD6(a, b, c, d, e, f)         a, b, c, d, e, f
#define FXP_BITFIELD7(a, b, c, d, e, f, g)      a, b, c, d, e, f, g
#define FXP_BITFIELD8(a, b, c, d, e, f, g, h)   a, b, c, d, e, f, g, h
#else
#define htole16(x)   _byteswap_ushort(x)
#define htobe16(x)   (x)
#define htole32(x)   _byteswap_ulong(x)
#define letoh16(x)   _byteswap_ushort(x)
#define betoh16(x)   (x)
#define letoh32(x)   _byteswap_ulong(x)

#define FXP_BITFIELD2(a, b)                     b, a
#define FXP_BITFIELD3(a, b, c)                  c, b, a
#define FXP_BITFIELD4(a, b, c, d)               d, c, b, a
#define FXP_BITFIELD5(a, b, c, d, e)            e, d, c, b, a
#define FXP_BITFIELD6(a, b, c, d, e, f)         f, e, d, c, b, a
#define FXP_BITFIELD7(a, b, c, d, e, f, g)      g, f, e, d, c, b, a
#define FXP_BITFIELD8(a, b, c, d, e, f, g, h)   h, g, f, e, d, c, b, a
#endif

/* Write to unaligned memory */
FORCEINLINE
VOID
le32enc(
    _Out_ PULONG Data,
    _In_ ULONG Value)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    *Data = Value;
#else
    PUCHAR P = (PUCHAR)Data;

    P[0] = (UCHAR)(Value >> 0);
    P[1] = (UCHAR)(Value >> 8);
    P[2] = (UCHAR)(Value >> 16);
    P[3] = (UCHAR)(Value >> 24);
#endif
}

FORCEINLINE
USHORT
FXP_IPCB_PACK_8021Q_INFO(
    _In_ ULONG UserPriority,
    _In_ ULONG CanonicalFormatId,
    _In_ ULONG VlanId)
{
    USHORT Control;

    Control = 0;
    Control |= (VlanId & 0xFFF);
    Control |= (CanonicalFormatId & 1) << 12;
    Control |= (UserPriority & 7) << 13;

    return htobe16(Control);
}

FORCEINLINE
VOID
FXP_IPCB_UNPACK_8021Q_INFO(
    _In_ USHORT Control,
    _Out_ PULONG UserPriority,
    _Out_ PULONG CanonicalFormatId,
    _Out_ PULONG VlanId)
{
    Control = betoh16(Control);

    *VlanId = Control & 0xFFF;
    *CanonicalFormatId = (Control >> 12) & 1;
    *UserPriority = (Control >> 13) & 7;
}

/*
 * Command Block Header
 */
typedef struct _FXP_CB_HEADER
{
    USHORT Status;
#define FXP_CB_STATUS_OK          0x2000
#define FXP_CB_STATUS_C           0x8000

#define FXP_RFD_STATUS_RCOL       0x0001 // Receive collision
#define FXP_RFD_STATUS_IAMATCH    0x0002 // 0 = matches station address
#define FXP_RFD_STATUS_NOAMATCH   0x0004 // 1 = does not match anything
#define FXP_RFD_STATUS_PARSE      0x0008 // Packet parse ok (82550/1 only)
#define FXP_RFD_STATUS_S4         0x0010 // Receive error from PHY
#define FXP_RFD_STATUS_TL         0x0020 // Type/length
#define FXP_RFD_STATUS_FTS        0x0080 // Frame too short
#define FXP_RFD_STATUS_OVERRUN    0x0100 // DMA overrun
#define FXP_RFD_STATUS_RNR        0x0200 // No resources
#define FXP_RFD_STATUS_ALIGN      0x0400 // Alignment error
#define FXP_RFD_STATUS_CRC        0x0800 // CRC error
#define FXP_RFD_STATUS_VLAN       0x1000 // VLAN tagged frame
#define FXP_RFD_STATUS_OK         0x2000 // Packet received okay
#define FXP_RFD_STATUS_C          0x8000 // Packet reception complete

    USHORT Command;
#define FXP_CB_COMMAND_NOP        0x0
#define FXP_CB_COMMAND_IAS        0x1
#define FXP_CB_COMMAND_CONFIG     0x2
#define FXP_CB_COMMAND_MCAS       0x3
#define FXP_CB_COMMAND_XMIT       0x4
#define FXP_CB_COMMAND_UCODE      0x5
#define FXP_CB_COMMAND_DUMP       0x6
#define FXP_CB_COMMAND_DIAG       0x7
#define FXP_CB_COMMAND_LOADFILT   0x8
#define FXP_CB_COMMAND_IPCBXMIT   0x9

#define FXP_CB_COMMAND_SF         0x0008 // Simple/flexible mode
#define FXP_CB_COMMAND_I          0x2000 // Generate interrupt on completion
#define FXP_CB_COMMAND_S          0x4000 // Suspend on completion
#define FXP_CB_COMMAND_EL         0x8000 // End of list

#define FXP_RFD_CONTROL_SF        0x0008 // Simple/flexible memory mode
#define FXP_RFD_CONTROL_H         0x0010 // Header RFD
#define FXP_RFD_CONTROL_S         0x4000 // Suspend after reception
#define FXP_RFD_CONTROL_EL        0x8000 // End of list

    ULONG LinkAddress;
} FXP_CB_HEADER, *PFXP_CB_HEADER;

C_ASSERT(sizeof(FXP_CB_HEADER) == 8);

/*
 * NOP Command
 * FXP_CB_COMMAND_NOP
 */
typedef struct _FXP_CB_NOP
{
    FXP_CB_HEADER Header;
} FXP_CB_NOP, *PFXP_CB_NOP;

C_ASSERT(sizeof(FXP_CB_NOP) == 8);

/*
 * Individual Address Setup Command
 * FXP_CB_COMMAND_IAS
 */
typedef struct _FXP_CB_INDIVIDUAL_ADDRESS_SETUP
{
    FXP_CB_HEADER Header;
    UCHAR MacAddress[ETH_LENGTH_OF_ADDRESS];
} FXP_CB_INDIVIDUAL_ADDRESS_SETUP, *PFXP_CB_INDIVIDUAL_ADDRESS_SETUP;

/*
 * Configure Command
 * FXP_CB_COMMAND_CONFIG
 */
typedef struct _FXP_CB_CONFIGURE
{
    FXP_CB_HEADER Header;
    // 0
    UCHAR FXP_BITFIELD2(ByteCount:6,
                        :2);
    // 1
    UCHAR FXP_BITFIELD3(RxFifoLimit:4,
                        TxFifoLimit:3,
                        :1);
    // 2
    UCHAR AdaptiveInterframeSpacing;
    // 3
    UCHAR FXP_BITFIELD5(MwiEnable:1,
                        TypeEnable:1,
                        ReadAlignEnable:1,
                        TermWriteOnCl:1,
                        :4);
    // 4
    UCHAR FXP_BITFIELD2(RxDmaMaxByteCount:7,
                        :1);
    // 5
    UCHAR FXP_BITFIELD2(TxDmaMaxByteCount:7,
                        Dmbc:1);
    // 6
    UCHAR FXP_BITFIELD8(LateScb:1,
                        DirectDmaDisable:1,
                        TnoInterruptOrTcoStat:1,
                        CiInterrupt:1,
                        ExtTxCbDisable:1,
                        ExtStatsDisable:1,
                        PassOverrunRx:1,
                        SaveBadFrames:1);
    // 7
    UCHAR FXP_BITFIELD6(DiscardShortReceive:1,
                        UnderunRetries:2,
                        :2,
                        ExtRfa:1,
                        TwoFramesInFifo:1,
                        DynamicTbd:1);
    // 8
    UCHAR FXP_BITFIELD3(MiiMode:1,
                        :6,
                        CsmaDisable:1);
    // 9
    UCHAR FXP_BITFIELD6(TcpUdpChecksum:1,
                        :3,
                        VlanTco:1,
                        LinkStatusChangeWakeEnable:1,
                        ArpWakeEnable:1,
                        MulticastMatchWakeEnable:1);
    // 10
    UCHAR FXP_BITFIELD6(:1,
                        Byte10_1:1,
                        Byte10_2:1,
                        Nsai:1,
                        PreambleLength:2,
                        Loopback:2);
    // 11
    UCHAR FXP_BITFIELD2(LinearPriority:3,
                        :5);
    // 12
    UCHAR FXP_BITFIELD3(LinearPriorityMode:1,
                        :3,
                        InterframeSpacing:4);
    // 13
    UCHAR IpAddressLow;
    // 14
    UCHAR IpAddressHigh;
    // 15
    UCHAR FXP_BITFIELD8(PromiscuousMode:1,
                        BroadcastDisable:1,
                        WaitAfterWin:1,
                        Byte15_3:1,
                        IgnoreUl:1,
                        SelectCrc16:1,
                        Byte15_6:1,
                        CrsCdt:1);
    // 16
    UCHAR FcDelayLsb;
    // 17
    UCHAR FcDelayMsb;
    // 18
    UCHAR FXP_BITFIELD6(StrippingEnable:1,
                        PaddingEnable:1,
                        RxCrcTransfer:1,
                        LongRxOk:1,
                        PriorityFcThreshold:3,
                        Byte18_7:1);
    // 19
    UCHAR FXP_BITFIELD8(IaMatchWakeEnable:1,
                        MagicPacketWakeDisable:1,
                        TxFullDuplexFlowControlDisable:1,
                        RxFullDuplexRestopFlowControl:1,
                        RxFullDuplexRestartFlowControl:1,
                        RejectFc:1,
                        ForceFullDuplex:1,
                        AutomaticFullDuplex:1);
    // 20
    UCHAR FXP_BITFIELD8(Byte20_0:1,
                        Byte20_1:1,
                        Byte20_2:1,
                        Byte20_3:1,
                        Byte20_4:1,
                        PriorityFcLocation:1,
                        MultipleIa:1,
                        :1);
    // 21
    UCHAR FXP_BITFIELD5(Byte21_0:1,
                        :1,
                        Byte21_1:1,
                        MulticastAll:1,
                        :4);
    // 22
    UCHAR FXP_BITFIELD3(GamlaRx:1,
                        VlanTagStrippingEnable:1,
                        :6);
    // 23-31
    UCHAR Reserved[9];
} FXP_CB_CONFIGURE, *PFXP_CB_CONFIGURE;

C_ASSERT(sizeof(FXP_CB_CONFIGURE) == 8 + 32);

/*
 * Multicast Setup Command
 * FXP_CB_COMMAND_MCAS
 */
typedef struct _FXP_CB_MULTICAST_SETUP
{
    FXP_CB_HEADER Header;
    USHORT Count;
    UCHAR MacAddress[E100_MULTICAST_LIST_SIZE][ETH_LENGTH_OF_ADDRESS];
} FXP_CB_MULTICAST_SETUP, *PFXP_CB_MULTICAST_SETUP;

/*
 * Load Microcode Command
 * FXP_CB_COMMAND_UCODE
 */
typedef struct _FXP_CB_LOAD_MICROCODE
{
    FXP_CB_HEADER Header;
    ULONG Data[192];
} FXP_CB_LOAD_MICROCODE, *PFXP_CB_LOAD_MICROCODE;

/*
 * Transmit Buffer Descriptor
 */
typedef struct _FXP_TBD
{
    ULONG Address;
    ULONG Size;
} FXP_TBD, *PFXP_TBD;

C_ASSERT(sizeof(FXP_TBD) == 8);

/*
 * IP Command Block
 */
typedef struct _FXP_IPCB
{
    USHORT IpScheduleLow;
    UCHAR IpSchedule;
#define FXP_IPCB_IP_CHECKSUM_ENABLE        0x10
#define FXP_IPCB_TCPUDP_CHECKSUM_ENABLE    0x20
#define FXP_IPCB_TCP_PACKET                0x40
#define FXP_IPCB_LARGESEND_ENABLE          0x80

    UCHAR IpActivationHigh;
#define FXP_IPCB_HARDWAREPARSING_ENABLE    0x01
#define FXP_IPCB_INSERTVLAN_ENABLE         0x02

    USHORT VlanId;
    UCHAR IpHeaderOffset;
    UCHAR TcpHeaderOffset;
} FXP_IPCB, *PFXP_IPCB;

C_ASSERT(sizeof(FXP_IPCB) == 8);

/*
 * Transmit Command
 * FXP_CB_COMMAND_XMIT
 * FXP_CB_COMMAND_IPCBXMIT
 */
typedef struct _FXP_CB_TRANSMIT
{
    /*
     * Transmit Control Block (TCB)
     */
    FXP_CB_HEADER Header;
    union
    {
        struct
        {
            ULONG TbdArrayAddress;
            USHORT ByteCount;
            UCHAR TxThreshold;
            UCHAR TbdNumber;
        };
        FXP_TBD TbdTso;
    };

    /*
     * Flexible Mode TBD array
     */
    union
    {
        /*
         * When the Extended TxCB mode is selected,
         * the device reads the TCB structure and two TBDs from host memory.
         * This means that we need at least two TBD entries in the list for this mode.
         *
         * When we use the FXP_CB_COMMAND_IPCBXMIT command, the first TBD
         * in the array must be sacrificed for the TCP/IP
         * checksum offload control bits (IPCB).
         *
         * We add the +1 because the 82550 may read an extra TBD
         * after the last valid TBD in Large Send mode.
         */
        FXP_IPCB Ipcb;
        FXP_TBD Tbd[E100_TBD_PER_TCB + 1];
    };
} FXP_CB_TRANSMIT, *PFXP_CB_TRANSMIT;

C_ASSERT(E100_TBD_PER_TCB >= 2);

/*
 * Statistical Counters
 * FXP_SCB_COMMAND_CU_DUMPRESET
 */
typedef struct _FXP_COUNTERS
{
    ULONG TxOk;
    ULONG TxMaxCol;
    ULONG TxLateCol;
    ULONG TxUnderruns;
    ULONG TxLossCarrier;
    ULONG TxDeffered;
    ULONG TxSingleCol;
    ULONG TxMiltipleCol;
    ULONG TxTotalCol;
    ULONG RxOk;
    ULONG RxCrcErr;
    ULONG RxAlignErr;
    ULONG RxResourceErr;
    ULONG RxOverrunErr;
    ULONG RxCdtErr;
    ULONG RxShortFramesErr;
    union
    {
        ULONG TxFcPause;
        ULONG CompletionStatus557;
    };
    ULONG RxFcPause;
    ULONG RxFcUnsupported;
    union
    {
        struct
        {
            USHORT TxTco;
            USHORT RxTco;
        };
        ULONG CompletionStatus558;
    };
    ULONG CompletionStatus559;
} FXP_COUNTERS, *PFXP_COUNTERS;

C_ASSERT(sizeof(FXP_COUNTERS) == 84);

#define FXP_STATS_DUMP_COMPLETE   0xA005
#define FXP_STATS_DR_COMPLETE     0xA007

#include <pshpack1.h>

/*
 * Receive Frame Descriptor (RFD)
 */
typedef struct _FXP_RFD
{
    FXP_CB_HEADER Header;
    ULONG RbdAddress;
    USHORT ActualSize;
#define FXP_RFD_FRAME_LENGTH_MASK   0x3FFF

    USHORT Size;

    /*
     * The following fields are only available when using
     * extended receive mode on an 82550/82551 chipset.
     */
    struct
    {
        USHORT VlanId;
        UCHAR ParserStatus;
        UCHAR Reserved;
        USHORT SecurityStatus;
        UCHAR ChecksumStatus;
        UCHAR ZerocopyStatus;
        UCHAR Pad[8];
    } Ex;

    /* The data portion of the received frame starts here (we use the simplified mode) */
} FXP_RFD, *PFXP_RFD;

C_ASSERT(RTL_SIZEOF_THROUGH_FIELD(FXP_RFD, Size) == 16);
C_ASSERT(sizeof(FXP_RFD) == 32);

#include <poppack.h>

#define MII_MAX_PHY_ADDRESSES    32

/*
 * PHY register definitions (IEEE 802.3)
 */
#define MII_CONTROL              0x00
#define     MII_CR_COLLISION_TEST   0x0080
#define     MII_CR_FULL_DUPLEX      0x0100
#define     MII_CR_AUTONEG_RESTART  0x0200
#define     MII_CR_ISOLATE          0x0400
#define     MII_CR_POWER_DOWN       0x0800
#define     MII_CR_AUTONEG          0x1000
#define     MII_CR_SPEED_SELECTION  0x2000
#define     MII_CR_LOOPBACK         0x4000
#define     MII_CR_RESET            0x8000
#define MII_STATUS               0x01
#define     MII_SR_LINK_STATUS      0x0004
#define     MII_SR_AUTONEG_COMPLETE 0x0020
#define MII_PHY_ID1              0x02
#define MII_PHY_ID2              0x03
#define MII_AUTONEG_ADVERTISE    0x04
#define     MII_ADV_CSMA            0x0001
#define     MII_ADV_10T_HD          0x0020
#define     MII_ADV_10T_FD          0x0040
#define     MII_ADV_100T_HD         0x0080
#define     MII_ADV_100T_FD         0x0100
#define     MII_ADV_100T4           0x0200
#define     MII_ADV_PAUSE_SYM       0x0400
#define     MII_ADV_PAUSE_ASYM      0x0800
#define MII_AUTONEG_LINK_PARTNER 0x05
#define     MII_LP_10T_HD           0x0020
#define     MII_LP_10T_FD           0x0040
#define     MII_LP_100T_HD          0x0080
#define     MII_LP_100T_FD          0x0100
#define     MII_LP_100T4            0x0200
#define     MII_LP_PAUSE_SYM        0x0400
#define     MII_LP_PAUSE_ASYM       0x0800
#define MII_AUTONEG_EXPANSION    0x06
#define     MII_EXP_LP_AUTONEG      0x0001
#define MII_MASTER_SLAVE_CONTROL 0x09
#define     MII_MS_CR_1000T_HD      0x0100
#define     MII_MS_CR_1000T_FD      0x0200
#define MII_MASTER_SLAVE_STATUS  0x0A
#define     MII_MS_SR_1000T_FD      0x0800

/*
 * National Semiconductor DP83840 MII PHY definitions
 */
#define PHY_DP_REG_PCS  0x17

#define PHY_DP_PCS_LED4_MODE     0x0002
#define PHY_DP_PCS_F_CONNECT     0x0020
#define PHY_DP_PCS_BIT_8         0x0100
#define PHY_DP_PCS_BIT_10        0x0400

#define PHY_IS_DP83840(PhyId)    (((PhyId) & 0xFFF0FFFF) == 0x5C002000)
