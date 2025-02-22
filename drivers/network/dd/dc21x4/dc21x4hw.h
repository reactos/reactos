/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Hardware specific definitions
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

typedef enum _DC_CHIP_TYPE
{
    DC21040,
    DC21041,
    DC21140,
    DC21143,
    DC21145,
} DC_CHIP_TYPE;

/*
 * PCI Vendor and Device IDs
 */
#define DC_DEV_DECCHIP_21040                      0x00021011
#define DC_DEV_DECCHIP_21041                      0x00141011
#define DC_DEV_DECCHIP_21140                      0x00091011
#define DC_DEV_INTEL_21143                        0x00191011
#define DC_DEV_INTEL_21145                        0x00398086

#define DC_DESCRIPTOR_ALIGNMENT                   4
#define DC_SETUP_FRAME_ALIGNMENT                  4
#define DC_RECEIVE_BUFFER_ALIGNMENT               4
#define DC_RECEIVE_BUFFER_SIZE_MULTIPLE           4

#define DC_IO_LENGTH   128

#define DC_SETUP_FRAME_SIZE        192

/* Multicast perfect filter */
#define DC_SETUP_FRAME_PERFECT_FILTER_ADDRESSES   16

/* -1 for physical address and -1 for broadcast address */
#define DC_SETUP_FRAME_ADDRESSES       (16 - 2)

/* Computed hash of FF:FF:FF:FF:FF:FF */
#define DC_SETUP_FRAME_BROADCAST_HASH  0xFF

#if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#define DC_SETUP_FRAME_ENTRY(Value)   (Value)
#else
#define DC_SETUP_FRAME_ENTRY(Value)   ((Value) << 16)
#endif

#include <pshpack1.h>

/*
 * Transmit Buffer Descriptor
 */
typedef struct _DC_TBD
{
    ULONG Status;
#define DC_TBD_STATUS_DEFFERED                      0x00000001
#define DC_TBD_STATUS_UNDERFLOW                     0x00000002
#define DC_TBD_STATUS_LINK_FAIL                     0x00000004
#define DC_TBD_STATUS_COLLISIONS_MASK               0x00000078
#define DC_TBD_STATUS_HEARTBEAT_FAIL                0x00000080
#define DC_TBD_STATUS_RETRY_ERROR                   0x00000100
#define DC_TBD_STATUS_LATE_COLLISION                0x00000200
#define DC_TBD_STATUS_NO_CARRIER                    0x00000400
#define DC_TBD_STATUS_CARRIER_LOST                  0x00000800
#define DC_TBD_STATUS_JABBER_TIMEOUT                0x00004000
#define DC_TBD_STATUS_ERROR_SUMMARY                 0x00008000
#define DC_TBD_STATUS_OWNED                         0x80000000

#define DC_TBD_STATUS_SETUP_FRAME                   0x7FFFFFFF

#define DC_TBD_STATUS_COLLISIONS_SHIFT              3

    ULONG Control;
#define DC_TBD_CONTROL_LENGTH_MASK_1                0x000007FF
#define DC_TBD_CONTROL_LENGTH_MASK_2                0x003FF800
#define DC_TBD_CONTROL_NO_PAD                       0x00800000
#define DC_TBD_CONTROL_CHAINED                      0x01000000
#define DC_TBD_CONTROL_END_OF_RING                  0x02000000
#define DC_TBD_CONTROL_NO_CRC                       0x04000000
#define DC_TBD_CONTROL_SETUP_FRAME                  0x08000000
#define DC_TBD_CONTROL_FIRST_FRAGMENT               0x20000000
#define DC_TBD_CONTROL_LAST_FRAGMENT                0x40000000
#define DC_TBD_CONTROL_REQUEST_INTERRUPT            0x80000000
#define DC_TBD_CONTROL_PERFECT_FILTER               0x00000000
#define DC_TBD_CONTROL_HASH_PERFECT_FILTER          0x00400000
#define DC_TBD_CONTROL_INVERSE_FILTER               0x10000000
#define DC_TBD_CONTROL_IMPERFECT_FILTER             0x10400000

#define DC_TBD_CONTROL_LENGTH_2_SHIFT               11

    ULONG Address1;
    ULONG Address2;
} DC_TBD, *PDC_TBD;

C_ASSERT(sizeof(DC_TBD) == 16);

/*
 * Receive Buffer Descriptor
 */
typedef struct _DC_RBD
{
    ULONG Status;
#define DC_RBD_STATUS_OVERRUN                       0x00000001
#define DC_RBD_STATUS_CRC_ERROR                     0x00000002
#define DC_RBD_STATUS_DRIBBLE                       0x00000004
#define DC_RBD_STATUS_MII_ERROR                     0x00000008
#define DC_RBD_STATUS_WDT_EXPIRED                   0x00000010
#define DC_RBD_STATUS_FRAME_TYPE                    0x00000020
#define DC_RBD_STATUS_COLLISION_SEEN                0x00000040
#define DC_RBD_STATUS_TOO_LONG                      0x00000080
#define DC_RBD_STATUS_LAST_DESCRIPTOR               0x00000100
#define DC_RBD_STATUS_FIRST_DESCRIPTOR              0x00000200
#define DC_RBD_STATUS_MULTICAST                     0x00000400
#define DC_RBD_STATUS_RUNT                          0x00000800
#define DC_RBD_STATUS_DATA_TYPE_MASK                0x00003000
#define DC_RBD_STATUS_LENGTH_ERROR                  0x00004000
#define DC_RBD_STATUS_ERROR_SUMMARY                 0x00008000
#define DC_RBD_STATUS_FRAME_LENGTH_MASK             0x3FFF0000
#define DC_RBD_STATUS_FILTERING_FAIL                0x40000000
#define DC_RBD_STATUS_OWNED                         0x80000000

#define DC_RBD_STATUS_FRAME_LENGTH_SHIFT            16

    ULONG Control;
#define DC_RBD_CONTROL_BUFFER_LENGTH_MASK_1         0x000007FF
#define DC_RBD_CONTROL_BUFFER_LENGTH_MASK_2         0x003FF800
#define DC_RBD_CONTROL_CHAINED                      0x01000000
#define DC_RBD_CONTROL_END_OF_RING                  0x02000000

    ULONG Address1;
    ULONG Address2;
} DC_RBD, *PDC_RBD;

C_ASSERT(sizeof(DC_RBD) == 16);

#define DC_PATTERN_FILTERS   4

/*
 * Wake-Up Filter Register Block
 */
typedef union _DC_PATTERN_FILTER_BLOCK
{
    struct
    {
        ULONG Mask[DC_PATTERN_FILTERS];

        UCHAR Command[DC_PATTERN_FILTERS];
#define DC_PATTERN_FILTER_CMD_ENABLE          0x01
#define DC_PATTERN_FILTER_CMD_INVERSE_MODE    0x02
#define DC_PATTERN_FILTER_CMD_ADD_PREV        0x04
#define DC_PATTERN_FILTER_CMD_MULTICAST       0x08

        UCHAR Offset[DC_PATTERN_FILTERS];
        USHORT Crc[DC_PATTERN_FILTERS];
    };
    ULONG AsULONG[8];
} DC_PATTERN_FILTER_BLOCK, *PDC_PATTERN_FILTER_BLOCK;

C_ASSERT(sizeof(DC_PATTERN_FILTER_BLOCK) == 32);

#include <poppack.h>

/*
 * NIC Control and Status Registers
 */
typedef enum _DC_CSR
{
    DcCsr0_BusMode = 0x00,
    DcCsr1_TxPoll = 0x08,
    DcCsr1_WakeUpFilter = 0x08,
    DcCsr2_RxPoll = 0x10,
    DcCsr2_WakeUpControl = 0x10,
    DcCsr3_RxRingAddress = 0x18,
    DcCsr4_TxRingAddress = 0x20,
    DcCsr5_Status = 0x28,
    DcCsr6_OpMode = 0x30,
    DcCsr7_IrqMask = 0x38,
    DcCsr8_RxCounters = 0x40,
    DcCsr9_SerialInterface = 0x48,
    DcCsr10_BootRom = 0x50,
    DcCsr11_FullDuplex = 0x58,
    DcCsr11_Timer = 0x58,
    DcCsr12_Gpio = 0x60,
    DcCsr12_SiaStatus = 0x60,
    DcCsr13_SiaConnectivity = 0x68,
    DcCsr14_SiaTxRx = 0x70,
    DcCsr15_SiaGeneral = 0x78,
} DC_CSR;

/*
 * CSR0 Bus Mode
 */
#define DC_BUS_MODE_SOFT_RESET                      0x00000001
#define DC_BUS_MODE_BUS_ARB                         0x00000002
#define DC_BUS_MODE_DESC_SKIP_LENGTH_MASK           0x0000007C
#define DC_BUS_MODE_BUFFERS_BIG_ENDIAN              0x00000080
#define DC_BUS_MODE_BURST_LENGTH_MASK               0x00003F00
#define DC_BUS_MODE_CACHE_ALIGNMENT_MASK            0x0000C000
#define DC_BUS_MODE_DIAGNOSTIC_ADDRESS_SPACE        0x00010000
#define DC_BUS_MODE_TX_POLL_MASK                    0x000E0000
#define DC_BUS_MODE_DESC_BIG_ENDIAN                 0x00100000
#define DC_BUS_MODE_READ_MULTIPLE                   0x00200000
#define DC_BUS_MODE_READ_LINE                       0x00800000
#define DC_BUS_MODE_WRITE_INVALIDATE                0x01000000
#define DC_BUS_MODE_ON_NOW_UNLOCK                   0x04000000

#define DC_BUS_MODE_BURST_LENGTH_NO_LIMIT           0x00000000
#define DC_BUS_MODE_BURST_LENGTH_1                  0x00000100
#define DC_BUS_MODE_BURST_LENGTH_2                  0x00000200
#define DC_BUS_MODE_BURST_LENGTH_4                  0x00000400
#define DC_BUS_MODE_BURST_LENGTH_8                  0x00000800
#define DC_BUS_MODE_BURST_LENGTH_16                 0x00001000
#define DC_BUS_MODE_BURST_LENGTH_32                 0x00002000

#define DC_BUS_MODE_CACHE_ALIGNMENT_NONE            0x00000000
#define DC_BUS_MODE_CACHE_ALIGNMENT_8               0x00004000
#define DC_BUS_MODE_CACHE_ALIGNMENT_16              0x00008000
#define DC_BUS_MODE_CACHE_ALIGNMENT_32              0x0000C000

#define DC_BUS_MODE_TX_POLL_DISABLED                0x00000000
#define DC_BUS_MODE_TX_POLL_1                       0x00020000
#define DC_BUS_MODE_TX_POLL_2                       0x00040000
#define DC_BUS_MODE_TX_POLL_3                       0x00060000
#define DC_BUS_MODE_TX_POLL_4                       0x00080000
#define DC_BUS_MODE_TX_POLL_5                       0x000A0000
#define DC_BUS_MODE_TX_POLL_6                       0x000C0000
#define DC_BUS_MODE_TX_POLL_7                       0x000E0000

#define DC_BUS_MODE_DESC_SKIP_LENGTH_0              0x00000000
#define DC_BUS_MODE_DESC_SKIP_LENGTH_1              0x00000004
#define DC_BUS_MODE_DESC_SKIP_LENGTH_2              0x00000008
#define DC_BUS_MODE_DESC_SKIP_LENGTH_4              0x00000010
#define DC_BUS_MODE_DESC_SKIP_LENGTH_8              0x00000020
#define DC_BUS_MODE_DESC_SKIP_LENGTH_16             0x00000040
#define DC_BUS_MODE_DESC_SKIP_LENGTH_32             0x00000080

/*
 * CSR1 Transmit Poll Demand
 */
#define DC_TX_POLL_DOORBELL                         0x00000001

/*
 * CSR2 Receive Poll Demand
 */
#define DC_RX_POLL_DOORBELL                         0x00000001

/*
 * CSR2 Wake Up Control
 */
#define DC_WAKE_UP_CONTROL_LINK_CHANGE              0x00000001
#define DC_WAKE_UP_CONTROL_MAGIC_PACKET             0x00000002
#define DC_WAKE_UP_CONTROL_PATTERN_MATCH            0x00000004
#define DC_WAKE_UP_STATUS_LINK_CHANGE               0x00000010
#define DC_WAKE_UP_STATUS_MAGIC_PACKET              0x00000020
#define DC_WAKE_UP_STATUS_PATTERN_MATCH             0x00000040
#define DC_WAKE_UP_CONTROL_GLOBAL_UNICAST           0x00000200
#define DC_WAKE_UP_CONTROL_VLAN_ENABLE              0x00000800
#define DC_WAKE_UP_CONTROL_VLAN_TYPE_MASK           0xFFFF0000

/*
 * CSR5 Status, CSR7 Irq Mask
 */
#define DC_IRQ_TX_OK                                0x00000001
#define DC_IRQ_TX_STOPPED                           0x00000002
#define DC_IRQ_TX_NO_BUFFER                         0x00000004
#define DC_IRQ_TX_JABBER_TIMEOUT                    0x00000008
#define DC_IRQ_LINK_PASS                            0x00000010
#define DC_IRQ_TX_UNDERFLOW                         0x00000020
#define DC_IRQ_RX_OK                                0x00000040
#define DC_IRQ_RX_NO_BUFFER                         0x00000080
#define DC_IRQ_RX_STOPPED                           0x00000100
#define DC_IRQ_RX_WDT_TIMEOUT                       0x00000200
#define DC_IRQ_AUI                                  0x00000400
#define DC_IRQ_TX_EARLY                             0x00000400
#define DC_IRQ_FD_FRAME_RECEIVED                    0x00000800
#define DC_IRQ_TIMER_TIMEOUT                        0x00000800
#define DC_IRQ_LINK_FAIL                            0x00001000
#define DC_IRQ_SYSTEM_ERROR                         0x00002000
#define DC_IRQ_RX_EARLY                             0x00004000
#define DC_IRQ_ABNORMAL_SUMMARY                     0x00008000
#define DC_IRQ_NORMAL_SUMMARY                       0x00010000
#define DC_STATUS_RX_STATE_MASK                     0x000E0000
#define DC_STATUS_TX_STATE_MASK                     0x00700000
#define DC_STATUS_SYSTEM_ERROR_MASK                 0x03800000
#define DC_IRQ_GPIO_PORT                            0x04000000
#define DC_IRQ_LINK_CHANGED                         0x08000000
#define DC_IRQ_HPNA_PHY                             0x10000000

#define DC_STATUS_TX_STATE_STOPPED                  0x00000000
#define DC_STATUS_TX_STATE_FETCH                    0x00100000
#define DC_STATUS_TX_STATE_WAIT_FOR_END             0x00200000
#define DC_STATUS_TX_STATE_READ                     0x00300000
#define DC_STATUS_TX_STATE_RESERVED                 0x00400000
#define DC_STATUS_TX_STATE_SETUP_PACKET             0x00500000
#define DC_STATUS_TX_STATE_SUSPENDED                0x00600000
#define DC_STATUS_TX_STATE_CLOSE                    0x00700000

#define DC_STATUS_RX_STATE_STOPPED                  0x00000000
#define DC_STATUS_RX_STATE_FETCH                    0x00020000
#define DC_STATUS_RX_STATE_CHECK_END                0x00040000
#define DC_STATUS_RX_STATE_WAIT_FOR_RCV             0x00060000
#define DC_STATUS_RX_STATE_SUSPENDED                0x00080000
#define DC_STATUS_RX_STATE_CLOSE_DESC               0x000A0000
#define DC_STATUS_RX_STATE_FLUSH                    0x000C0000
#define DC_STATUS_RX_STATE_DEQUEUE                  0x000E0000

#define DC_STATUS_SYSTEM_ERROR_PARITY               0x00000000
#define DC_STATUS_SYSTEM_ERROR_MASTER_ABORT         0x00800000
#define DC_STATUS_SYSTEM_ERROR_TARGET_ABORT         0x01000000

/*
 * CSR6 Operation Mode
 */
#define DC_OPMODE_RX_HASH_PERFECT_FILT              0x00000001
#define DC_OPMODE_RX_ENABLE                         0x00000002
#define DC_OPMODE_RX_HASH_ONLY_FILT                 0x00000004
#define DC_OPMODE_RX_RUNTS                          0x00000008
#define DC_OPMODE_RX_INVERSE_FILT                   0x00000010
#define DC_OPMODE_BACKOFF_COUNTER                   0x00000020
#define DC_OPMODE_RX_PROMISCUOUS                    0x00000040
#define DC_OPMODE_RX_ALL_MULTICAST                  0x00000080
#define DC_OPMODE_FKD                               0x00000100
#define DC_OPMODE_FULL_DUPLEX                       0x00000200
#define DC_OPMODE_LOOPBACK_MASK                     0x00000C00
#define DC_OPMODE_FORCE_COLLISIONS                  0x00001000
#define DC_OPMODE_TX_ENABLE                         0x00002000
#define DC_OPMODE_TX_THRESHOLD_CTRL_MASK            0x0000C000
#define DC_OPMODE_TX_BACK_PRESSURE                  0x00010000
#define DC_OPMODE_TX_CAPTURE_EFFECT                 0x00020000
#define DC_OPMODE_PORT_SELECT                       0x00040000
#define DC_OPMODE_PORT_HEARTBEAT_DISABLE            0x00080000
#define DC_OPMODE_STORE_AND_FORWARD                 0x00200000
#define DC_OPMODE_PORT_XMIT_10                      0x00400000
#define DC_OPMODE_PORT_PCS                          0x00800000
#define DC_OPMODE_PORT_SCRAMBLER                    0x01000000
#define DC_OPMODE_PORT_ALWAYS                       0x02000000
#define DC_OPMODE_ADDR_LSB_IGNORE                   0x04000000
#define DC_OPMODE_RX_RECEIVE_ALL                    0x40000000
#define DC_OPMODE_TX_SPECIAL_CAPTURE_EFFECT         0x80000000

#define DC_OPMODE_LOOPBACK_NORMAL                   0x00000000
#define DC_OPMODE_LOOPBACK_INTERNAL                 0x00000400
#define DC_OPMODE_LOOPBACK_EXTERNAL                 0x00000800

#define DC_OPMODE_TX_THRESHOLD_LEVEL                0x00004000
#define DC_OPMODE_TX_THRESHOLD_MAX                  0x0000C000

#define DC_OPMODE_MEDIA_MASK ( \
    DC_OPMODE_TX_THRESHOLD_CTRL_MASK | \
    DC_OPMODE_LOOPBACK_MASK | \
    DC_OPMODE_FULL_DUPLEX | \
    DC_OPMODE_PORT_SELECT | \
    DC_OPMODE_PORT_HEARTBEAT_DISABLE | \
    DC_OPMODE_PORT_XMIT_10 | \
    DC_OPMODE_PORT_PCS | \
    DC_OPMODE_PORT_SCRAMBLER)

/*
 * CSR8 Receive Counters
 */
#define DC_COUNTER_RX_NO_BUFFER_MASK                0x0001FFFF
#define DC_COUNTER_RX_OVERFLOW_MASK                 0x1FFE0000

#define DC_COUNTER_RX_OVERFLOW_SHIFT                17

/*
 * CSR9 Serial Interface
 */
#define DC_SERIAL_EE_CS                             0x00000001
#define DC_SERIAL_EE_SK                             0x00000002
#define DC_SERIAL_EE_DI                             0x00000004
#define DC_SERIAL_EE_DO                             0x00000008
#define DC_SERIAL_EE_REG                            0x00000400
#define DC_SERIAL_EE_SR                             0x00000800
#define DC_SERIAL_EE_WR                             0x00002000
#define DC_SERIAL_EE_RD                             0x00004000
#define DC_SERIAL_EE_MOD                            0x00008000
#define DC_SERIAL_MII_MDC                           0x00010000
#define DC_SERIAL_MII_MDO                           0x00020000
#define DC_SERIAL_MII_MII                           0x00040000
#define DC_SERIAL_MII_MDI                           0x00080000
#define DC_SERIAL_EAR_DN                            0x80000000
#define DC_SERIAL_EAR_DT                            0x000000FF
#define DC_SERIAL_SPI_CS                            0x00100000
#define DC_SERIAL_SPI_SK                            0x00200000
#define DC_SERIAL_SPI_DI                            0x00400000
#define DC_SERIAL_SPI_DO                            0x00800000

#define DC_SERIAL_EE_DI_SHIFT                       2
#define DC_SERIAL_EE_DO_SHIFT                       3
#define DC_SERIAL_MII_MDO_SHIFT                     17
#define DC_SERIAL_MII_MDI_SHIFT                     19
#define DC_SERIAL_SPI_DI_SHIFT                      22
#define DC_SERIAL_SPI_DO_SHIFT                      23

/*
 * CSR11 Timer
 */
#define DC_TIMER_VALUE_MASK                         0x0000FFFF
#define DC_TIMER_CONTINUOUS                         0x00010000
#define DC_TIMER_RX_NUMBER_MASK                     0x000E0000
#define DC_TIMER_RX_TIMER_MASK                      0x00F00000
#define DC_TIMER_TX_NUMBER_MASK                     0x07000000
#define DC_TIMER_TX_TIMER_MASK                      0x78000000
#define DC_TIMER_CYCLE_SIZE                         0x80000000

#define DC_TIMER_RX_NUMBER_SHIFT                    17
#define DC_TIMER_RX_TIMER_SHIFT                     20
#define DC_TIMER_TX_NUMBER_SHIFT                    24
#define DC_TIMER_TX_TIMER_SHIFT                     27

/*
 * CSR12 SIA Status
 */
#define DC_SIA_STATUS_MII_RECEIVE_ACTIVITY          0x00000001
#define DC_SIA_STATUS_NETWORK_CONNECTION_ERROR      0x00000002
#define DC_SIA_STATUS_100T_LINK_FAIL                0x00000002
#define DC_SIA_STATUS_10T_LINK_FAIL                 0x00000004
#define DC_SIA_STATUS_SELECTED_PORT_ACTIVITY        0x00000100
#define DC_SIA_STATUS_AUI_ACTIVITY                  0x00000100
#define DC_SIA_STATUS_HPNA_ACTIVITY                 0x00000100
#define DC_SIA_STATUS_NONSEL_PORT_ACTIVITY          0x00000200
#define DC_SIA_STATUS_10T_ACTIVITY                  0x00000200
#define DC_SIA_STATUS_NSN                           0x00000400
#define DC_SIA_STATUS_TX_REMOTE_FAULT               0x00000800
#define DC_SIA_STATUS_ANS_MASK                      0x00007000
#define DC_SIA_STATUS_LP_AUTONED_SUPPORTED          0x00008000
#define DC_SIA_STATUS_LP_CODE_WORD_MASK             0xFFFF0000

#define DC_SIA_STATUS_ANS_AUTONEG_DISABLED          0x00000000
#define DC_SIA_STATUS_ANS_TX_DISABLE                0x00001000
#define DC_SIA_STATUS_ANS_ABILITY_DETECT            0x00002000
#define DC_SIA_STATUS_ANS_ACK_DETECT                0x00003000
#define DC_SIA_STATUS_ANS_ACK_COMPLETE              0x00004000
#define DC_SIA_STATUS_ANS_AUTONEG_COMPLETE          0x00005000
#define DC_SIA_STATUS_ANS_LINK_CHECK                0x00006000

#define DC_SIA_STATUS_LP_CODE_WORD_SHIFT            16

#define DC_GPIO_CONTROL    0x100

/*
 * CSR13 SIA Connectivity
 */
#define DC_SIA_CONN_RESET                           0x00000000
#define DC_SIA_CONN_HPNA                            0x00000008

/*
 * CSR14 SIA Transmit and Receive
 */
#define DC_SIA_TXRX_ENCODER                         0x00000001
#define DC_SIA_TXRX_LOOPBACK                        0x00000002
#define DC_SIA_TXRX_DRIVER                          0x00000004
#define DC_SIA_TXRX_LINK_PULSE                      0x00000008
#define DC_SIA_TXRX_COMPENSATION                    0x00000030
#define DC_SIA_TXRX_ADV_10T_HD                      0x00000040
#define DC_SIA_TXRX_AUTONEG                         0x00000080
#define DC_SIA_TXRX_RX_SQUELCH                      0x00000100
#define DC_SIA_TXRX_COLLISION_SQUELCH               0x00000200
#define DC_SIA_TXRX_COLLISION_DETECT                0x00000400
#define DC_SIA_TXRX_HEARTBEAT                       0x00000800
#define DC_SIA_TXRX_LINK_TEST                       0x00001000
#define DC_SIA_TXRX_AUTOPOLARITY                    0x00002000
#define DC_SIA_TXRX_SET_POLARITY_PLUS               0x00004000
#define DC_SIA_TXRX_10T_AUTOSENSE                   0x00008000
#define DC_SIA_TXRX_ADV_100TX_HD                    0x00010000
#define DC_SIA_TXRX_ADV_100TX_FD                    0x00020000
#define DC_SIA_TXRX_ADV_100T4                       0x00040000

/*
 * CSR15 SIA and GPIO
 */
#define DC_SIA_GENERAL_JABBER_DISABLE               0x00000001
#define DC_SIA_GENERAL_HOST_UNJAB                   0x00000002
#define DC_SIA_GENERAL_JABBER_CLOCK                 0x00000004
#define DC_SIA_GENERAL_AUI_BNC_MODE                 0x00000008
#define DC_SIA_GENERAL_RX_WDT_DISABLE               0x00000010
#define DC_SIA_GENERAL_RX_WDT_RELEASE               0x00000020
#define DC_SIA_GENERAL_LINK_EXTEND                  0x00000800
#define DC_SIA_GENERAL_RX_MAGIC_PACKET              0x00004000
#define DC_SIA_GENERAL_HCKR                         0x00008000
#define DC_SIA_GENERAL_GPIO_MASK                    0x000F0000
#define DC_SIA_GENERAL_LGS3                         0x00100000
#define DC_SIA_GENERAL_LGS2                         0x00200000
#define DC_SIA_GENERAL_LGS1                         0x00400000
#define DC_SIA_GENERAL_LGS0                         0x00800000
#define DC_SIA_GENERAL_GEI0                         0x01000000
#define DC_SIA_GENERAL_GEI1                         0x02000000
#define DC_SIA_GENERAL_RECEIVE_MATCH                0x04000000
#define DC_SIA_GENERAL_CONTROL_WRITE                0x08000000
#define DC_SIA_GENERAL_GI0                          0x10000000
#define DC_SIA_GENERAL_GI1                          0x20000000
#define DC_SIA_GENERAL_IRQ_RX_MATCH                 0x40000000

#define DC_RBD_STATUS_INVALID \
    (DC_RBD_STATUS_OVERRUN | \
     DC_RBD_STATUS_CRC_ERROR | \
     DC_RBD_STATUS_WDT_EXPIRED | \
     DC_RBD_STATUS_COLLISION_SEEN | \
     DC_RBD_STATUS_TOO_LONG | \
     DC_RBD_STATUS_RUNT | \
     DC_RBD_STATUS_LENGTH_ERROR)

#define DC_GENERIC_IRQ_MASK \
    (DC_IRQ_TX_OK | DC_IRQ_TX_STOPPED | DC_IRQ_TX_JABBER_TIMEOUT | \
     DC_IRQ_RX_OK | DC_IRQ_TX_UNDERFLOW | \
     DC_IRQ_RX_STOPPED | \
     DC_IRQ_SYSTEM_ERROR | DC_IRQ_ABNORMAL_SUMMARY | DC_IRQ_NORMAL_SUMMARY)

/* Errata: The programming guide incorrectly stated that CSR13 must be set to 0x30480009 */
#define DC_HPNA_ANALOG_CTRL                    0x708A0000

/*
 * PCI Configuration Registers
 */
#define DC_PCI_DEVICE_CONFIG      0x40
#define     DC_PCI_DEVICE_CONFIG_SNOOZE        0x40000000
#define     DC_PCI_DEVICE_CONFIG_SLEEP         0x80000000

/*
 * SPI Interface
 */
#define DC_SPI_BYTE_WRITE_OPERATION    2
#define DC_SPI_BYTE_READ_OPERATION     3
#define DC_SPI_CLEAR_WRITE_ENABLE      4
#define DC_SPI_SET_WRITE_ENABLE        6

/*
 * HomePNA PHY Registers
 */
#define HPNA_CONTROL_LOW         0x00
#define HPNA_CONTROL_HIGH        0x01
#define HPNA_NOISE               0x10
#define HPNA_NOISE_FLOOR         0x12
#define HPNA_NOISE_CEILING       0x13
#define HPNA_NOISE_ATTACK        0x14

/*
 * MDIO Protocol (IEEE 802.3)
 */
#define MDIO_START       0x01
#define MDIO_WRITE       0x01
#define MDIO_READ        0x02
#define MDIO_TA          0x02
#define MDIO_PREAMBLE    0xFFFFFFFF

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

#define MII_ADV_100 \
    (MII_ADV_100T_HD | MII_ADV_100T_FD | MII_ADV_100T4)
