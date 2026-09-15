/*
 * PROJECT:     QLogic ISP SCSI Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Hardware definitions
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

/*
 * Register definitions are derived from the Matthew Jacob's
 * multiplatform driver for ISP chipsets.
 * Copyright (C) 2000-2007 by Matthew Jacob <mjacob@NetBSD.org>
 */

#pragma once

#define QL_PCI_VEN_QLOGIC       0x1077

#define QL_PCI_DEV_ISP1040      0x1020
#define QL_PCI_DEV_ISP1240      0x1240
#define QL_PCI_DEV_ISP1080      0x1080
#define QL_PCI_DEV_ISP1280      0x1280
#define QL_PCI_DEV_ISP10160     0x1016
#define QL_PCI_DEV_ISP12160     0x1216

#define QL_MAX_BUSES     2
#define QL_MAX_TARGETS   16

#ifndef ISP_FIRMWARE_DISABLE
#define QL_MAX_LUN       32
#else
#define QL_MAX_LUN       8
#endif

#define QL_MAX_MAILBOX   8

#define QL_CODE_ORG   0x1000

#define QL_FW_DMA_BLOCK_SIZE     4096

/* All IOCB Queue entries are this size */
#define QL_QENTRY_LEN    64

/*
 * Hardware registers
 */
#define QL_REG_ID_LOW                  0x00
#define QL_REG_ID_HIGH                 0x02
#define QL_REG_CFG0                    0x04
#define QL_REG_CFG1                    0x06
#define QL_REG_INT_CTRL                0x08
#define QL_REG_INT_STATUS              0x0A
#define QL_REG_SEMAPHORE               0x0C
#define QL_REG_NVRAM                   0x0E
#define QL_REG_FLASH_BIOS_DATA         0x10
#define QL_REG_FLASH_BIOS_ADDRESS      0x12
#define QL_REG_MAILBOX0                0x70
#define QL_REG_MAILBOX1                0x72
#define QL_REG_MAILBOX2                0x74
#define QL_REG_MAILBOX3                0x76
#define QL_REG_MAILBOX4                0x78
#define QL_REG_MAILBOX5                0x7A
#define QL_REG_MAILBOX6                0x7C
#define QL_REG_MAILBOX7                0x7E

/*
 * RISC bank registers
 */
#define QL_REG_RISC_PSR                0xA0
#define QL_REG_HOST_CMD                0xC0
#define QL_REG_GPIO_DATA               0xCC
#define QL_REG_GPIO_ENABLE             0xCE

/*
 * Bus Interface Block Register Definitions
 *
 * QL_REG_CFG0
 */
#define BIU_CONF0_HW_MASK        0x000F // Hardware revision mask

#define BIU_CHIP_TYPE_1020       0x0001
#define BIU_CHIP_TYPE_1020A      0x0002
#define BIU_CHIP_TYPE_1040       0x0003
#define BIU_CHIP_TYPE_1040A      0x0004
#define BIU_CHIP_TYPE_1040B      0x0005
#define BIU_CHIP_TYPE_1040C      0x0006

/*
 * Bus Interface Block Register Definitions
 *
 * QL_REG_CFG1
 */
#define BIU_BURST_ENABLE         0x0004 // Global enable Bus bursts
#define BIU_PCI_CONF1_SXP        0x0008 // SXP register select
#define BIU_PCI_CONF1_FIFO_16    0x0010 // 16 bytes FIFO threshold
#define BIU_PCI_CONF1_FIFO_32    0x0020 // 32 bytes FIFO threshold
#define BIU_PCI_CONF1_FIFO_64    0x0030 // 64 bytes FIFO threshold
#define BIU_PCI_CONF1_FIFO_128   0x0040 // 128 bytes FIFO threshold

#define BIU_PCI1080_CONF1_SXP0   0x0100 // SXP bank #1 select
#define BIU_PCI1080_CONF1_SXP1   0x0200 // SXP bank #2 select
#define BIU_PCI1080_CONF1_DMA    0x0300 // DMA bank select

/*
 * Bus control register
 *
 * QL_REG_INT_CTRL
 */
#define QL_IFACE_SOFT_RESET      0x0001

/*
 * Bus status register
 *
 * QL_REG_INT_STATUS
 */
#define QL_IFACE_ISR_PEND        0x0002
#define QL_IFACE_RISC_INTR       0x0004

/*
 * Mailbox semaphore register
 *
 * QL_REG_SEMAPHORE
 */
#define QL_SEMAPHORE_LOCK        0x0001
#define QL_SEMAPHORE_STATUS      0x0002

/*
 * NVRAM semaphore register
 *
 * QL_REG_NVRAM
 */
#define QL_NVRAM_CLOCK       0x0001
#define QL_NVRAM_SELECT      0x0002
#define QL_NVRAM_DATAOUT     0x0004
#define QL_NVRAM_DATAIN      0x0008

#define QL_NVRAM_DATAOUT_SHIFT  2
#define QL_NVRAM_DATAIN_SHIFT   3

#define QL_NVRAM_CMD_READ     6
#define QL_NVRAM_CMD_LENGTH   3

/*
 * Processor status register
 *
 * QL_REG_RISC_PSR
 */
#define QL_RISC_PSR_PCI_ULTRA    0x0080

/*
 * Host Command and Control register
 *
 * QL_REG_HOST_CMD
 */
#define QL_HC_RESET_RISC         0x1000
#define QL_HC_PAUSE_RISC         0x2000
#define QL_HC_RELEASE_RISC       0x3000
#define QL_HC_SET_HOST_INT       0x5000
#define QL_HC_CLEAR_HOST_INT     0x6000
#define QL_HC_CLEAR_RISC_INT     0x7000
#define QL_HC_DISABLE_BIOS       0x9000

/*
 * Mailbox I/O interface registers
 */
#define QL_REG_MBOX_STATUS       QL_REG_MAILBOX0
#define QL_REG_MBOX_HNDL_LOW     QL_REG_MAILBOX1
#define QL_REG_MBOX_HNDL_HIGH    QL_REG_MAILBOX2
#define QL_REG_MBOX_RQST         QL_REG_MAILBOX4
#define QL_REG_MBOX_RESP         QL_REG_MAILBOX5

/*
 * Device Flags
 */
#define DPARM_PPR        0x0020
#define DPARM_ASYNC      0x0040
#define DPARM_NARROW     0x0080
#define DPARM_RENEG      0x0100
#define DPARM_QFRZ       0x0200
#define DPARM_ARQ        0x0400
#define DPARM_TQING      0x0800
#define DPARM_SYNC       0x1000
#define DPARM_WIDE       0x2000
#define DPARM_PARITY     0x4000
#define DPARM_DISC       0x8000
#define DPARM_DEFAULT    (0xFF00 & ~DPARM_QFRZ)
#define DPARM_SAFE_DFLT  (DPARM_DEFAULT & ~(DPARM_WIDE|DPARM_SYNC|DPARM_TQING))

/*
 * Request header flags definitions
 */
#define RQSFLAG_CONTINUATION    0x01
#define RQSFLAG_FULL            0x02
#define RQSFLAG_BADHEADER       0x04
#define RQSFLAG_BADPACKET       0x08
#define RQSFLAG_BADCOUNT        0x10
#define RQSFLAG_BADORDER        0x20
#define RQSFLAG_MASK            0x3F

/*
 * Request header entry_type definitions
 */
#define RQSTYPE_REQUEST         0x01
#define RQSTYPE_DATASEG         0x02
#define RQSTYPE_RESPONSE        0x03
#define RQSTYPE_MARKER          0x04
#define RQSTYPE_CMDONLY         0x05
#define RQSTYPE_ATIO            0x06 // Target Mode
#define RQSTYPE_CTIO            0x07 // Target Mode
#define RQSTYPE_REQUEST_A64     0x09
#define RQSTYPE_A64_CONT        0x0A
#define RQSTYPE_ENABLE_LUN      0x0B // Target Mode
#define RQSTYPE_MODIFY_LUN      0x0C // Target Mode
#define RQSTYPE_NOTIFY          0x0D // Target Mode
#define RQSTYPE_NOTIFY_ACK      0x0E // Target Mode
#define RQSTYPE_CTIO_A64        0x0F // Target Mode

/*
 * Request flags values
 */
#define REQFLAG_NODISCON           0x0001
#define REQFLAG_HTAG               0x0002
#define REQFLAG_OTAG               0x0004
#define REQFLAG_STAG               0x0008
#define REQFLAG_TARGET_RTN         0x0010

#define REQFLAG_NODATA             0x0000
#define REQFLAG_DATA_IN            0x0020
#define REQFLAG_DATA_OUT           0x0040
#define REQFLAG_DATA_BIDIRECTIONAL 0x0060

#define REQFLAG_DISARQ             0x0100
#define REQFLAG_FRC_ASYNC          0x0200
#define REQFLAG_FRC_SYNC           0x0400
#define REQFLAG_FRC_WIDE           0x0800
#define REQFLAG_NOPARITY           0x1000
#define REQFLAG_STOPQ              0x2000
#define REQFLAG_XTRASNS            0x4000
#define REQFLAG_PRIORITY           0x8000

/*
 * Request completion status cdes
 */
#define RQCS_COMPLETE                    0x0000
#define RQCS_INCOMPLETE                  0x0001
#define RQCS_DMA_ERROR                   0x0002
#define RQCS_TRANSPORT_ERROR             0x0003
#define RQCS_RESET_OCCURRED              0x0004
#define RQCS_ABORTED                     0x0005
#define RQCS_TIMEOUT                     0x0006
#define RQCS_DATA_OVERRUN                0x0007
#define RQCS_COMMAND_OVERRUN             0x0008
#define RQCS_STATUS_OVERRUN              0x0009
#define RQCS_BAD_MESSAGE                 0x000A
#define RQCS_NO_MESSAGE_OUT              0x000B
#define RQCS_EXT_ID_FAILED               0x000C
#define RQCS_IDE_MSG_FAILED              0x000D
#define RQCS_ABORT_MSG_FAILED            0x000E
#define RQCS_REJECT_MSG_FAILED           0x000F
#define RQCS_NOP_MSG_FAILED              0x0010
#define RQCS_PARITY_ERROR_MSG_FAILED     0x0011
#define RQCS_DEVICE_RESET_MSG_FAILED     0x0012
#define RQCS_ID_MSG_FAILED               0x0013
#define RQCS_UNEXP_BUS_FREE              0x0014
#define RQCS_DATA_UNDERRUN               0x0015
#define RQCS_XACT_ERR1                   0x0018
#define RQCS_XACT_ERR2                   0x0019
#define RQCS_XACT_ERR3                   0x001A
#define RQCS_BAD_ENTRY                   0x001B
#define RQCS_PHASE_SKIPPED               0x001D
#define RQCS_ARQS_FAILED                 0x001E
#define RQCS_WIDE_FAILED                 0x001F
#define RQCS_QUEUE_FULL                  0x001C
#define RQCS_SYNCXFER_FAILED             0x0020
#define RQCS_LVD_BUSERR                  0x0021

/*
 * Request State Flags
 */
#define RQSF_GOT_BUS            0x0100
#define RQSF_GOT_TARGET         0x0200
#define RQSF_SENT_CDB           0x0400
#define RQSF_XFRD_DATA          0x0800
#define RQSF_GOT_STATUS         0x1000
#define RQSF_GOT_SENSE          0x2000
#define RQSF_XFER_COMPLETE      0x4000

/*
 * Request Status Flags
 */
#define RQSTF_DISCONNECT        0x0001
#define RQSTF_SYNCHRONOUS       0x0002
#define RQSTF_PARITY_ERROR      0x0004
#define RQSTF_BUS_RESET         0x0008
#define RQSTF_DEVICE_RESET      0x0010
#define RQSTF_ABORTED           0x0020
#define RQSTF_TIMEOUT           0x0040
#define RQSTF_NEGOTIATION       0x0080

#define MBOX_GET_IN_BITS(Cmd)    ((Cmd >> 0) & 0xFF)
#define MBOX_GET_OUT_BITS(Cmd)   ((Cmd >> 8) & 0xFF)
#define MBOX_GET_OPCODE(Cmd)     ((Cmd >> 16) & 0xFFFF)

#define MBC(OpCode, InBits, OutBits)  ((InBits) | ((OutBits) << 8) | ((OpCode) << 16))

/*
 * Mailbox Command Opcodes
 */
#define QL_MBOX_CMD_NO_OP                        MBC(0x0000, 0x01, 0x01)
#define QL_MBOX_CMD_LOAD_RAM                     MBC(0x0001, 0x1F, 0x01)
#define QL_MBOX_CMD_EXEC_FIRMWARE                MBC(0x0002, 0x03, 0x01)
#define QL_MBOX_CMD_DUMP_RAM                     MBC(0x0003, 0x1F, 0x01)
#define QL_MBOX_CMD_WRITE_RAM_WORD               MBC(0x0004, 0x07, 0x07)
#define QL_MBOX_CMD_READ_RAM_WORD                MBC(0x0005, 0x03, 0x07)
#define QL_MBOX_CMD_MAILBOX_REG_TEST             MBC(0x0006, 0xFF, 0xFF)
#define QL_MBOX_CMD_VERIFY_CHECKSUM              MBC(0x0007, 0x03, 0x07)
#define QL_MBOX_CMD_ABOUT_FIRMWARE               MBC(0x0008, 0x01, 0x4F)
#define QL_MBOX_CMD_LOAD_RAM_A64                 MBC(0x0009, 0xDF, 0x01)
#define QL_MBOX_CMD_DUMP_RAM_A64                 MBC(0x000A, 0xDF, 0x01)
#define QL_MBOX_CMD_INIT_REQ_QUEUE               MBC(0x0010, 0xDF, 0x01)
#define QL_MBOX_CMD_INIT_RES_QUEUE               MBC(0x0011, 0xEF, 0x01)
#define QL_MBOX_CMD_EXECUTE_IOCB                 MBC(0x0012, 0x0F, 0x0F)
#define QL_MBOX_CMD_ABORT_COMMAND                MBC(0x0015, 0x0F, 0x0F)
#define QL_MBOX_CMD_ABORT_DEVICE                 MBC(0x0016, 0x03, 0x03)
#define QL_MBOX_CMD_ABORT_TARGET                 MBC(0x0017, 0x07, 0x07)
#define QL_MBOX_CMD_BUS_RESET                    MBC(0x0018, 0x07, 0x07)
#define QL_MBOX_CMD_START_QUEUE                  MBC(0x001A, 0x03, 0x07)
#define QL_MBOX_CMD_GET_FIRMWARE_STATUS          MBC(0x001F, 0x01, 0x07)
#define QL_MBOX_CMD_GET_RETRY_COUNT              MBC(0x0022, 0x01, 0xC7)
#define QL_MBOX_CMD_GET_ACT_NEG_STATE            MBC(0x0025, 0x01, 0x07)
#define QL_MBOX_CMD_GET_TARGET_PARAMS            MBC(0x0028, 0x03, 0x4F)
#define QL_MBOX_CMD_SET_INIT_SCSI_ID             MBC(0x0030, 0x03, 0x03)
#define QL_MBOX_CMD_SET_SELECT_TIMEOUT           MBC(0x0031, 0x07, 0x07)
#define QL_MBOX_CMD_SET_RETRY_COUNT              MBC(0x0032, 0xC7, 0xC7)
#define QL_MBOX_CMD_SET_TAG_AGE_LIMIT            MBC(0x0033, 0x07, 0x07)
#define QL_MBOX_CMD_SET_CLOCK_RATE               MBC(0x0034, 0x03, 0x03)
#define QL_MBOX_CMD_SET_ACT_NEG_STATE            MBC(0x0035, 0x07, 0x07)
#define QL_MBOX_CMD_SET_ASYNC_DATA_SETUP_TIME    MBC(0x0036, 0x07, 0x07)
#define QL_MBOX_CMD_SET_PCI_PARAMETERS           MBC(0x0037, 0x07, 0x07)
#define QL_MBOX_CMD_SET_TARGET_PARAMS            MBC(0x0038, 0x4F, 0x4F)
#define QL_MBOX_CMD_SET_DEV_QUEUE_PARAMS         MBC(0x0039, 0x0F, 0x0F)
#define QL_MBOX_CMD_RETURN_BIOS_BLOCK_ADDR       MBC(0x0040, 0x01, 0x03)
#define QL_MBOX_CMD_WRITE_FOUR_RAM_WORDS         MBC(0x0041, 0x3F, 0x01)
#define QL_MBOX_CMD_EXEC_BIOS_IOCB               MBC(0x0042, 0x03, 0x07)
#define QL_MBOX_CMD_SET_SYSTEM_PARAMETER         MBC(0x0045, 0x03, 0x03)
#define QL_MBOX_CMD_SET_FW_FEATURES              MBC(0x004A, 0x03, 0x03)
#define QL_MBOX_CMD_INIT_REQ_QUEUE_A64           MBC(0x0052, 0xDF, 0xFF)
#define QL_MBOX_CMD_INIT_RES_QUEUE_A64           MBC(0x0053, 0xEF, 0xFF)
#define QL_MBOX_CMD_EXECUTE_IOCB_A64             MBC(0x0054, 0xCF, 0x01)
#define QL_MBOX_CMD_SET_DATA_OVERRUN_RECOVERY    MBC(0x005A, 0x03, 0x03)

/*
 * Mailbox Command Complete Status Codes
 */
#define QL_MBOX_STATUS_COMMAND_COMPLETE       0x4000
#define QL_MBOX_STATUS_INVALID_COMMAND        0x4001
#define QL_MBOX_STATUS_HOST_INTERFACE_ERROR   0x4002
#define QL_MBOX_STATUS_TEST_FAILED            0x4003
#define QL_MBOX_STATUS_COMMAND_ERROR          0x4005
#define QL_MBOX_STATUS_COMMAND_PARAM_ERROR    0x4006

#define QL_MBOX_COMPLETED(Status)   ((Status) & 0x4000)

/*
 * Mailbox asynchronous event status codes
 */
#define QL_MBOX_ASYNC_MIN_CODE                0x8000
#define QL_MBOX_ASYNC_BUS_RESET               0x8001
#define QL_MBOX_ASYNC_SYSTEM_ERROR            0x8002
#define QL_MBOX_ASYNC_RQS_XFER_ERR            0x8003
#define QL_MBOX_ASYNC_RSP_XFER_ERR            0x8004
#define QL_MBOX_ASYNC_ATIO_XFER_ERR           0x8005
#define QL_MBOX_ASYNC_TIMEOUT_RESET           0x8006
#define QL_MBOX_ASYNC_DEVICE_RESET            0x8007
#define QL_MBOX_ASYNC_EXTMSG_UNDERRUN         0x800A
#define QL_MBOX_ASYNC_SCAM_INT                0x800B
#define QL_MBOX_ASYNC_HUNG_SCSI               0x800C
#define QL_MBOX_ASYNC_KILLED_BUS              0x800D
#define QL_MBOX_ASYNC_BUS_TRANSIT             0x800E
#define QL_MBOX_ASYNC_CMD_CMPLT               0x8020

/*
 * Firmware features flags (QL_MBOX_CMD_SET_FW_FEATURES)
 */
#define QL_FW_FEATURE_FAST_POST   0x1
#define QL_FW_FEATURE_LVD_NOTIFY  0x2
#define QL_FW_FEATURE_RIO_32BIT   0x4
#define QL_FW_FEATURE_RIO_16BIT   0x8

#define QL_IOCB_MAX_REQ_SEG_32    4
#define QL_IOCB_MAX_REQ_SEG_64    2

#define QL_IOCB_MAX_CONT_SEG_32   7
#define QL_IOCB_MAX_CONT_SEG_64   5

/*
 * Limits of the number of DMA segments
 */
#define QL_SG_LIST_MAX_SEG_32     (254 * QL_IOCB_MAX_CONT_SEG_32)
#define QL_SG_LIST_MAX_SEG_64     (254 * QL_IOCB_MAX_CONT_SEG_64)

#include <pshpack1.h>

typedef struct QL_IOCB_DATA_SEGMENT_32
{
    ULONG Address;
    ULONG Length;
} QL_IOCB_DATA_SEGMENT_32, *PQL_IOCB_DATA_SEGMENT_32;

typedef struct _QL_IOCB_DATA_SEGMENT_64
{
    ULONG AddressLow;
    ULONG AddressHigh;
    ULONG Length;
} QL_IOCB_DATA_SEGMENT_64, *PQL_IOCB_DATA_SEGMENT_64;

typedef struct _QL_IOCB_HEADER
{
    UCHAR EntryType;
    UCHAR EntryCount;
    UCHAR SequenceNumber;
    UCHAR Flags;
} QL_IOCB_HEADER, *PQL_IOCB_HEADER;

typedef struct _QL_IOCB_REQUEST
{
    QL_IOCB_HEADER Header; // RQSTYPE_REQUEST, RQSTYPE_REQUEST_A64
    ULONG Handle;
    UCHAR Lun;
    UCHAR Target;
    USHORT CdbLength;
    USHORT Flags;
    USHORT Reserved;
    USHORT Timeout;
    USHORT SegmentCount;
    UCHAR Cdb[12];
    union
    {
        struct
        {
            QL_IOCB_DATA_SEGMENT_32 DataSegment[QL_IOCB_MAX_REQ_SEG_32];
        } x32;
        struct
        {
            ULONG TotalCount;
            ULONG Reserved2;
            QL_IOCB_DATA_SEGMENT_64 DataSegment[QL_IOCB_MAX_REQ_SEG_64];
        } x64;
    };
} QL_IOCB_REQUEST, *PQL_IOCB_REQUEST;

C_ASSERT(sizeof(QL_IOCB_REQUEST) == QL_QENTRY_LEN);

typedef struct _QL_IOCB_CONT_32
{
    QL_IOCB_HEADER Header; // RQSTYPE_DATASEG
    ULONG Reserved;
    QL_IOCB_DATA_SEGMENT_32 DataSegment[QL_IOCB_MAX_CONT_SEG_32];
} QL_IOCB_CONT_32, *PQL_IOCB_CONT_32;

C_ASSERT(sizeof(QL_IOCB_CONT_32) == QL_QENTRY_LEN);

typedef struct _QL_IOCB_CONT_64
{
    QL_IOCB_HEADER Header; // RQSTYPE_A64_CONT
    QL_IOCB_DATA_SEGMENT_64 DataSegment[QL_IOCB_MAX_CONT_SEG_64];
} QL_IOCB_CONT_64, *PQL_IOCB_CONT_64;

C_ASSERT(sizeof(QL_IOCB_CONT_64) == QL_QENTRY_LEN);

typedef struct _QL_IOCB_STATUS
{
    QL_IOCB_HEADER Header; // RQSTYPE_RESPONSE
    ULONG Handle;
    USHORT ScsiStatus;
    USHORT CompletionStatus;
    USHORT StateFlags;
    USHORT StatusFlags;
    USHORT Time;
    USHORT SenseLength;
    ULONG ResidualLength;
    UCHAR Response[8];
    UCHAR SenseData[32];
} QL_IOCB_STATUS, *PQL_IOCB_STATUS;

C_ASSERT(sizeof(QL_IOCB_STATUS) == QL_QENTRY_LEN);

typedef struct _QL_IOCB_MARKER
{
    QL_IOCB_HEADER Header; // RQSTYPE_MARKER
    ULONG Handle;
    UCHAR Lun;
    UCHAR Target;
    USHORT Modifier;
#define QL_IOCB_MODIFIER_SYNC_DEVICE    0
#define QL_IOCB_MODIFIER_SYNC_TARGET    1
#define QL_IOCB_MODIFIER_SYNC_ALL       2
    UCHAR Reserved[52];
} QL_IOCB_MARKER, *PQL_IOCB_MARKER;

C_ASSERT(sizeof(QL_IOCB_MARKER) == QL_QENTRY_LEN);

/*
 * NVRAM definitions.
 * For USHORT entities data is stored in Little Endian order.
 */

typedef struct _QL_NVRAM_HEADER
{
    UCHAR Id[4];
    UCHAR Version;
} QL_NVRAM_HEADER, *PQL_NVRAM_HEADER;

typedef struct _QL_NVRAM_TARGET_DATA
{
    UCHAR Parameters;
    UCHAR ExecutionThrottle;
    UCHAR SyncPeriod;
    UCHAR Flags;
    UCHAR PprOptions;
    UCHAR Reserved;
} QL_NVRAM_TARGET_DATA, *PQL_NVRAM_TARGET_DATA;

typedef struct _QL_NVRAM_BUS_DATA
{
    UCHAR Config1;
    UCHAR ResetDelay;
    UCHAR RetryCount;
    UCHAR RetryDelay;
    UCHAR Config2;
    UCHAR Reserved;
    USHORT SelectionTimeout;
    USHORT MaxQueueDepth;
    UCHAR Reserved1[6];
    QL_NVRAM_TARGET_DATA Target[QL_MAX_TARGETS];
} QL_NVRAM_BUS_DATA, *PQL_NVRAM_BUS_DATA;

typedef struct _QL_NVRAM_ISP1040
{
    QL_NVRAM_HEADER Header;
    UCHAR IspConfig1;
    UCHAR ResetDelay;
    UCHAR RetryCount;
    UCHAR RetryDelay;
    UCHAR IspConfig2;
    UCHAR TagAgeLimit;
    UCHAR Flags;
    USHORT SelectionTimeout;
    USHORT MaxQueueDepth;
    UCHAR BoardType;
    UCHAR Reserved;
    USHORT SystemVendorID;
    USHORT SystemDeviceID;
    USHORT IspParameter;
    USHORT FwFeatures;
    UCHAR Reserved1[2];
    QL_NVRAM_TARGET_DATA Target[QL_MAX_TARGETS];
    UCHAR Reserved2[3];
    UCHAR Checksum;
} QL_NVRAM_ISP1040, *PQL_NVRAM_ISP1040;

C_ASSERT(sizeof(QL_NVRAM_ISP1040) == 128);

typedef struct _QL_NVRAM_ISP1080
{
    QL_NVRAM_HEADER Header;
    UCHAR Flags1;
    USHORT Flags2;
    UCHAR Reserved[8];
    UCHAR IspConfig;
    UCHAR TerminationControl;
    USHORT IspParameter;
    USHORT FwFeatures;
    USHORT Reserved1;
    QL_NVRAM_BUS_DATA Bus[QL_MAX_BUSES];
    UCHAR Reserved2[2];
    USHORT SystemVendorID;
    USHORT SystemDeviceID;
    UCHAR Reserved3;
    UCHAR Checksum;
} QL_NVRAM_ISP1080, *PQL_NVRAM_ISP1080;

C_ASSERT(sizeof(QL_NVRAM_ISP1080) == 256);

#include <poppack.h>
