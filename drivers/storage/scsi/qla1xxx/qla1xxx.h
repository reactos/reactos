/*
 * PROJECT:     QLogic ISP SCSI Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Driver header file
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#include <ntddk.h>
#include <storport.h>

#include "isp.h"

#define ISP_REQUEST_QUEUE_SIZE      64
#define ISP_RESPONSE_QUEUE_SIZE     64

#define ISP_QENTRY_FREE(Prod, Cons, QueueSize) \
    ((Cons) - (Prod) - 1) & ((QueueSize) - 1)

#define ISP_NEXT_QENTRY(Index, QueueSize) \
    (((Index) + 1) & ((QueueSize) - 1))

#define ISP_QUEUE_ENTRY(QueueBase, Index) \
    ((PVOID)((ULONG_PTR)(QueueBase) + ((Index) * QL_QENTRY_LEN)))

#define ISP_QUEUE_MEM_BLOCK_SIZE \
    ((ISP_REQUEST_QUEUE_SIZE + ISP_RESPONSE_QUEUE_SIZE) * QL_QENTRY_LEN)

typedef enum _ISP_FAMILY
{
    QL_ISP_FAMILY_1040,
    QL_ISP_FAMILY_1080,
    QL_ISP_FAMILY_12160
} ISP_FAMILY;

typedef enum _ISP_TYPE
{
    QL_ISP_TYPE_1020,
    QL_ISP_TYPE_1020A,
    QL_ISP_TYPE_1040,
    QL_ISP_TYPE_1040A,
    QL_ISP_TYPE_1040B,
    QL_ISP_TYPE_1040C,
    QL_ISP_TYPE_1240,
    QL_ISP_TYPE_1080,
    QL_ISP_TYPE_1280,
    QL_ISP_TYPE_10160,
    QL_ISP_TYPE_12160
} ISP_TYPE;

typedef struct _ISP_BUS_DATA
{
    UCHAR InitiatorId;
    UCHAR ResetDelay;
    UCHAR RetryCount;
    UCHAR RetryDelay;
    UCHAR AsyncDataSetupTime;
    UCHAR ReqAckActiveNegation;
    UCHAR DataLineActiveNegation;
    UCHAR TagAgeLimit;
    USHORT SelectionTimeout;
    USHORT MaxQueueDepth;
} ISP_BUS_DATA, *PISP_BUS_DATA;

typedef struct _ISP_TARGET_DATA
{
    UCHAR ExecutionThrottle;
    UCHAR SyncPeriod;
    UCHAR SyncOffset;
    UCHAR PprOptions;
    USHORT Parameters;
} ISP_TARGET_DATA, *PISP_TARGET_DATA;

typedef struct _ISP_HW_EXTENSION
{
    /* Frequently used fields */
    PUCHAR IoBase; ///< Base I/O address
    PUCHAR RequestQueueBase; ///< Virtual address of the request ring
    PUCHAR ResponseQueueBase; ///< Virtual address of the response ring
    ULONG Flags; ///< Read-only flags
#define ISP_FLAG_1040_ULTRAMODE    0x00000001
#define ISP_FLAG_DDMA_BURST_ENABLE 0x00000002
#define ISP_FLAG_CDMA_BURST_ENABLE 0x00000004
#define ISP_FLAG_IN_RESET          0x00000008
#define ISP_FLAG_64BIT_ADDRESS     0x80000000
    BOOLEAN MarkerNeeded[QL_MAX_BUSES]; ///< Send a marker IOCB
    UCHAR ScsiBusCount; ///< Number of SCSI buses on this HBA
    UCHAR InterruptFlags; ///< DIRQL flags
#define ISP_INT_FLAG_ADAPTER_ACTIVE  0x01
#define ISP_INT_FLAG_MAILBOX_ACTIVE  0x02
#define ISP_INT_FLAG_NEED_RESET_ASIC 0x04
#define ISP_INT_FLAG_NEED_RESET_BUS1 0x08
#define ISP_INT_FLAG_NEED_RESET_BUS2 0x10
    ULONG RequestCons; ///< Index of last ISP pickup
    ULONG RequestProd; ///< Index of next request
    ULONG ResponseCons; ///< Index of next response
    PSCSI_REQUEST_BLOCK ActiveSrb[ISP_REQUEST_QUEUE_SIZE]; ///< Active I/O requests on the chip

    UCHAR InBitmap; ///< Mailbox command output registers
    UCHAR OutBitmap; ///< Mailbox command input registers
    USHORT Mailbox[QL_MAX_MAILBOX]; ///< Mailbox command or result
    STOR_DPC RecoveryDpc; ///< Error recovery DPC
    ISP_FAMILY IspFamily; ///< ISP chip family
    ISP_TYPE IspType; ///< ISP chip type
    UCHAR IspClock; ///< ISP clock rate, in MHz
    UCHAR TerminationControl;
    UCHAR IspConfig;
    USHORT IspParameter;
    USHORT FwFeatures;
    ULONG SystemIoBusNumber;
    ULONG SlotNumber;
    ISP_TARGET_DATA Target[QL_MAX_BUSES][QL_MAX_TARGETS]; ///< Target settings
    ISP_BUS_DATA BusData[QL_MAX_BUSES]; ///< Bus settings
    ULONG64 RequestQueuePa; ///< Physical address of the request ring
    ULONG64 ResponseQueuePa; ///< Physical address of the response ring
    USHORT Nvram[128]; ///< NVRAM image
} ISP_HW_EXTENSION, *PISP_HW_EXTENSION;

typedef struct _ISP_SRB_EXTENSION
{
    QL_IOCB_REQUEST Packet;
    PSTOR_SCATTER_GATHER_LIST Sgl;
} ISP_SRB_EXTENSION, *PISP_SRB_EXTENSION;

#if DBG

#ifndef __RELFILE__
#define __RELFILE__ __FILE__
#endif

#define TRACE(fmt, ...) \
    StorPortDebugPrint(0, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define INFO(fmt, ...) \
    StorPortDebugPrint(1, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define WARN(fmt, ...) \
    StorPortDebugPrint(2, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#define ERR(fmt, ...) \
    StorPortDebugPrint(3, "(%s:%d) %s " fmt, __RELFILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)

#else

#if defined(_MSC_VER)
#define TRACE  __noop
#define INFO   __noop
#define WARN   __noop
#define ERR    __noop
#else
#define TRACE(...) do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define INFO(...)  do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define WARN(...)  do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#define ERR(...)   do { if(0) { DbgPrint(__VA_ARGS__); } } while(0)
#endif

#endif

FORCEINLINE
VOID
MBX_INIT(
    _In_ PISP_HW_EXTENSION HwExt,
    _In_ ULONG Command)
{
    HwExt->InBitmap = MBOX_GET_IN_BITS(Command);
    HwExt->OutBitmap = MBOX_GET_OUT_BITS(Command);
    HwExt->Mailbox[0] = MBOX_GET_OPCODE(Command);
}

VOID
IspReadEeprom(
    _In_ PISP_HW_EXTENSION HwExt);

#define QL_READ(HwExt, Register) \
    StorPortReadRegisterUshort(HwExt, (PUSHORT)(HwExt->IoBase + Register))

#define QL_WRITE(HwExt, Register, Value) \
    StorPortWriteRegisterUshort(HwExt, (PUSHORT)(HwExt->IoBase + Register), Value)

#define QL_WAIT(Microseconds) \
    StorPortStallExecution(Microseconds)
