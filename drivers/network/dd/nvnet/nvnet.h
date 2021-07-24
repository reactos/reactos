/*
 * PROJECT:     ReactOS nVidia nForce Ethernet Controller Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Common header file
 * COPYRIGHT:   Copyright 2021-2022 Dmitry Borisov <di.sean@protonmail.com>
 */

#ifndef _NVNET_PCH_
#define _NVNET_PCH_

#if !DBG
#define NO_KERNEL_LIST_ENTRY_CHECKS
#endif
#include <ndis.h>

#include <section_attribs.h>

#include "eth.h"
#include "nic.h"
#include "phyreg.h"

#define NVNET_TAG 'ENVN'

#if defined(SARCH_XBOX)
/* Reduce memory requirements on OG Xbox */
#define NVNET_TRANSMIT_BLOCKS        8
#define NVNET_TRANSMIT_DESCRIPTORS   32
#define NVNET_TRANSMIT_BUFFERS       1
#define NVNET_RECEIVE_DESCRIPTORS    16
#else
#define NVNET_TRANSMIT_BLOCKS        64
#define NVNET_TRANSMIT_DESCRIPTORS   512
#define NVNET_TRANSMIT_BUFFERS       16
#define NVNET_RECEIVE_DESCRIPTORS    512
#endif

#define NVNET_ALIGNMENT    64

#define NVNET_RECEIVE_BUFFER_SIZE    2048

#define NVNET_RECEIVE_PROCESSING_LIMIT   64

#define NVNET_IM_THRESHOLD    4
#define NVNET_IM_MAX_IDLE     40

#if defined(SARCH_XBOX)
#define NVNET_TRANSMIT_HANG_THRESHOLD    3
#else
#define NVNET_TRANSMIT_HANG_THRESHOLD    5
#endif

#if defined(SARCH_XBOX)
#define NVNET_FRAGMENTATION_THRESHOLD    8
#else
#define NVNET_FRAGMENTATION_THRESHOLD    32
#endif

#define NVNET_MEDIA_DETECTION_INTERVAL   5000

#define NVNET_MAXIMUM_FRAME_SIZE        1514
#define NVNET_MAXIMUM_FRAME_SIZE_JUMBO  9014
#define NVNET_MAXIMUM_VLAN_ID           0xFFF

#define NVNET_MULTICAST_LIST_SIZE    32

#define NVNET_MINIMUM_LSO_SEGMENT_COUNT    2
#define NVNET_MAXIMUM_LSO_FRAME_SIZE       0xFC00

#define NVNET_PACKET_FILTERS ( \
    NDIS_PACKET_TYPE_DIRECTED | \
    NDIS_PACKET_TYPE_MULTICAST | \
    NDIS_PACKET_TYPE_BROADCAST | \
    NDIS_PACKET_TYPE_PROMISCUOUS | \
    NDIS_PACKET_TYPE_ALL_MULTICAST)

#define PACKET_ENTRY(Packet) ((PLIST_ENTRY)(&(Packet)->MiniportReserved[0]))

typedef enum _NVNET_OPTIMIZATION_MODE
{
    NV_OPTIMIZATION_MODE_DYNAMIC = 0,
    NV_OPTIMIZATION_MODE_CPU,
    NV_OPTIMIZATION_MODE_THROUGHPUT
} NVNET_OPTIMIZATION_MODE;

typedef enum _NVNET_FLOW_CONTROL_MODE
{
    NV_FLOW_CONTROL_DISABLE = 0,
    NV_FLOW_CONTROL_AUTO,
    NV_FLOW_CONTROL_RX,
    NV_FLOW_CONTROL_TX,
    NV_FLOW_CONTROL_RX_TX
} NVNET_FLOW_CONTROL_MODE;

typedef union _NVNET_OFFLOAD
{
    struct {
        ULONG SendIpOptions:1;
        ULONG SendTcpOptions:1;
        ULONG SendTcpChecksum:1;
        ULONG SendUdpChecksum:1;
        ULONG SendIpChecksum:1;

        ULONG ReceiveIpOptions:1;
        ULONG ReceiveTcpOptions:1;
        ULONG ReceiveTcpChecksum:1;
        ULONG ReceiveUdpChecksum:1;
        ULONG ReceiveIpChecksum:1;

        ULONG SendIpV6Options:1;
        ULONG SendTcpV6Options:1;
        ULONG SendTcpV6Checksum:1;
        ULONG SendUdpV6Checksum:1;

        ULONG ReceiveIpV6Options:1;
        ULONG ReceiveTcpV6Options:1;
        ULONG ReceiveTcpV6Checksum:1;
        ULONG ReceiveUdpV6Checksum:1;
    };
    ULONG Value;
} NVNET_OFFLOAD, *PNVNET_OFFLOAD;

typedef struct _NVNET_STATISTICS
{
    ULONG64 HwTxCnt;
    ULONG64 HwTxZeroReXmt;
    ULONG64 HwTxOneReXmt;
    ULONG64 HwTxManyReXmt;
    ULONG64 HwTxLateCol;
    ULONG64 HwTxUnderflow;
    ULONG64 HwTxLossCarrier;
    ULONG64 HwTxExcessDef;
    ULONG64 HwTxRetryErr;
    ULONG64 HwRxFrameErr;
    ULONG64 HwRxExtraByte;
    ULONG64 HwRxLateCol;
    ULONG64 HwRxRunt;
    ULONG64 HwRxFrameTooLong;
    ULONG64 HwRxOverflow;
    ULONG64 HwRxFCSErr;
    ULONG64 HwRxFrameAlignErr;
    ULONG64 HwRxLenErr;
    ULONG64 HwTxDef;
    ULONG64 HwTxFrame;
    ULONG64 HwRxCnt;
    ULONG64 HwTxPause;
    ULONG64 HwRxPause;
    ULONG64 HwRxDropFrame;
    ULONG64 HwRxUnicast;
    ULONG64 HwRxMulticast;
    ULONG64 HwRxBroadcast;
    ULONG64 HwTxUnicast;
    ULONG64 HwTxMulticast;
    ULONG64 HwTxBroadcast;

    ULONG64 TransmitOk;
    ULONG64 ReceiveOk;
    ULONG64 TransmitErrors;
    ULONG64 ReceiveErrors;
    ULONG64 ReceiveNoBuffers;
    ULONG64 ReceiveCrcErrors;
    ULONG64 ReceiveAlignmentErrors;
    ULONG64 TransmitDeferred;
    ULONG64 TransmitExcessiveCollisions;
    ULONG64 ReceiveOverrunErrors;
    ULONG64 TransmitUnderrunErrors;
    ULONG64 TransmitZeroRetry;
    ULONG64 TransmitOneRetry;
    ULONG64 TransmitLostCarrierSense;
    ULONG64 TransmitLateCollisions;

    ULONG ReceiveIrqNoBuffers;
} NVNET_STATISTICS, *PNVNET_STATISTICS;

typedef struct _NVNET_WAKE_FRAME
{
    union
    {
        UCHAR AsUCHAR[16];
        ULONG AsULONG[4];
    } PatternMask;
    UCHAR WakeUpPattern[128];
} NVNET_WAKE_FRAME, *PNVNET_WAKE_FRAME;

typedef struct _NVNET_TX_BUFFER_DATA
{
    PVOID VirtualAddress;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;
} NVNET_TX_BUFFER_DATA, *PNVNET_TX_BUFFER_DATA;

typedef struct _NVNET_TX_BUFFER
{
    SINGLE_LIST_ENTRY Link;
    PVOID VirtualAddress;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;
} NVNET_TX_BUFFER, *PNVNET_TX_BUFFER;

typedef union _NVNET_TBD
{
    PNVNET_DESCRIPTOR_32 x32;
    PNVNET_DESCRIPTOR_64 x64;
    PVOID Memory;
} NVNET_TBD;

typedef struct _NVNET_TCB
{
    NVNET_TBD Tbd;
    NVNET_TBD DeferredTbd;
    PNDIS_PACKET Packet;
    PNVNET_TX_BUFFER Buffer;
    ULONG Slots;
    ULONG Flags;
#define NV_TCB_LARGE_SEND     0x00000001
#define NV_TCB_CHECKSUM_IP    0x00000002
#define NV_TCB_CHECKSUM_TCP   0x00000004
#define NV_TCB_CHECKSUM_UDP   0x00000008
#define NV_TCB_COALESCE       0x00000010

    ULONG Mss;
} NVNET_TCB, *PNVNET_TCB;

typedef union _NV_RBD
{
    PNVNET_DESCRIPTOR_32 x32;
    PNVNET_DESCRIPTOR_64 x64;
    PVOID Memory;
} NV_RBD;

typedef struct _NVNET_RBD
{
    NV_RBD NvRbd;
    PNDIS_PACKET Packet;
    PNDIS_BUFFER Buffer;
} NVNET_RBD, *PNVNET_RBD;

typedef struct _NVNET_SEND
{
    NDIS_SPIN_LOCK Lock;
    PNVNET_TCB HeadTcb;
    PNVNET_TCB TailTcb;
    PNVNET_TCB LastTcb;
    PNVNET_TCB CurrentTcb;
    PNVNET_TCB DeferredTcb;
    NVNET_TBD HeadTbd;
    NVNET_TBD TailTbd;
    NVNET_TBD CurrentTbd;
    ULONG TcbSlots;
    ULONG TbdSlots;
    ULONG StuckCount;
    ULONG PacketsCount;
    SINGLE_LIST_ENTRY BufferList;
} NVNET_SEND, *PNVNET_SEND;

typedef struct _NVNET_RECEIVE
{
    NDIS_SPIN_LOCK Lock;
    NV_RBD NvRbd;
} NVNET_RECEIVE, *PNVNET_RECEIVE;

typedef struct _NVNET_ADAPTER      NVNET_ADAPTER, *PNVNET_ADAPTER;

typedef VOID
(NVNET_TRANSMIT_PACKET)(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNVNET_TCB Tcb,
    _In_ PSCATTER_GATHER_LIST SgList);
typedef NVNET_TRANSMIT_PACKET *PNVNET_TRANSMIT_PACKET;

typedef ULONG
(NVNET_PROCESS_TRANSMIT)(
    _In_ PNVNET_ADAPTER Adapter,
    _Inout_ PLIST_ENTRY SendReadyList);
typedef NVNET_PROCESS_TRANSMIT *PNVNET_PROCESS_TRANSMIT;

typedef ULONG
(NVNET_PROCESS_RECEIVE)(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG TotalRxProcessed);
typedef NVNET_PROCESS_RECEIVE *PNVNET_PROCESS_RECEIVE;

typedef struct _NVNET_ADAPTER
{
    volatile PUCHAR IoBase;
    NDIS_HANDLE AdapterHandle;
    ULONG Features;
    ULONG Flags;
#define NV_ACTIVE                  0x80000000
#define NV_SEND_CHECKSUM           0x00000002
#define NV_SEND_LARGE_SEND         0x00000004
#define NV_SEND_ERRATA_PRESENT     0x00000008

#define NV_MAC_IN_USE              0x00000010
#define NV_GIGABIT_PHY             0x00000020
#define NV_UNIT_SEMAPHORE_ACQUIRED 0x00000040
#define NV_USE_SOFT_MAC_ADDRESS    0x00000100
#define NV_FORCE_SPEED_AND_DUPLEX  0x00000200
#define NV_FORCE_FULL_DUPLEX       0x00000400
#define NV_USER_SPEED_100          0x00000800
#define NV_PACKET_PRIORITY         0x00001000
#define NV_VLAN_TAGGING            0x00002000

    ULONG TxRxControl;
    ULONG InterruptMask;
    ULONG InterruptStatus;
    ULONG InterruptIdleCount;
    ULONG AdapterStatus;
    NVNET_OPTIMIZATION_MODE OptimizationMode;
    NVNET_OFFLOAD Offload;
    ULONG IpHeaderOffset;
    ULONG PacketFilter;
    NVNET_SEND Send;

    NVNET_RECEIVE Receive;
    PUCHAR ReceiveBuffer;
    ULONG CurrentRx;

    PNVNET_TRANSMIT_PACKET TransmitPacket;
    PNVNET_PROCESS_TRANSMIT ProcessTransmit;

    NVNET_STATISTICS Statistics;
    NDIS_SPIN_LOCK Lock;
    ULONG MaximumFrameSize;
    ULONG ReceiveBufferSize;
    ULONG VlanId;
    ULONG WakeFlags;
    ULONG PhyAddress;
    ULONG PhyModel;
    ULONG PhyRevision;
    ULONG PhyOui;
    ULONG PowerStatePending;

    ULONG VlanControl;
    ULONG PauseFlags;
    ULONG LinkSpeed;
    BOOLEAN Connected;
    BOOLEAN FullDuplex;

    ULONG OriginalMacAddress[2];
    UCHAR PermanentMacAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR CurrentMacAddress[ETH_LENGTH_OF_ADDRESS];

    _Field_range_(0, NVNET_MULTICAST_LIST_SIZE)
    ULONG MulticastListSize;
    struct
    {
        UCHAR MacAddress[ETH_LENGTH_OF_ADDRESS];
    } MulticastList[NVNET_MULTICAST_LIST_SIZE];

    ULONG WakeFrameBitmap;
    PNVNET_WAKE_FRAME WakeFrames[NV_WAKEUPPATTERNS_V2];

    NDIS_HANDLE WrapperConfigurationHandle;

    NDIS_WORK_ITEM PowerWorkItem;
    NDIS_WORK_ITEM ResetWorkItem;

    _Interlocked_
    volatile LONG ResetLock;

    NDIS_PHYSICAL_ADDRESS IoAddress;
    ULONG IoLength;

    NVNET_FLOW_CONTROL_MODE FlowControlMode;
    NDIS_MINIPORT_TIMER MediaDetectionTimer;

    USHORT DeviceId;
    UCHAR RevisionId;

    BOOLEAN InterruptShared;
    NDIS_MINIPORT_INTERRUPT Interrupt;
    ULONG InterruptVector;
    ULONG InterruptLevel;
    ULONG InterruptFlags;

    NDIS_PHYSICAL_ADDRESS TbdPhys;
    NDIS_PHYSICAL_ADDRESS RbdPhys;
    NDIS_PHYSICAL_ADDRESS ReceiveBufferPhys;

    PVOID SendBuffer;
    PVOID TbdOriginal;
    PVOID RbdOriginal;
    PVOID AdapterOriginal;
    NDIS_PHYSICAL_ADDRESS TbdPhysOriginal;
    NDIS_PHYSICAL_ADDRESS RbdPhysOriginal;
    NVNET_TX_BUFFER_DATA SendBufferAllocationData[NVNET_TRANSMIT_BUFFERS];
} NVNET_ADAPTER, *PNVNET_ADAPTER;

#define NvNetLogError(Adapter, ErrorCode) \
    NdisWriteErrorLogEntry((Adapter)->AdapterHandle, ErrorCode, 1, __LINE__)

NVNET_TRANSMIT_PACKET NvNetTransmitPacket32;
NVNET_TRANSMIT_PACKET NvNetTransmitPacket64;
NVNET_PROCESS_TRANSMIT ProcessTransmitDescriptorsLegacy;
NVNET_PROCESS_TRANSMIT ProcessTransmitDescriptors32;
NVNET_PROCESS_TRANSMIT ProcessTransmitDescriptors64;

CODE_SEG("PAGE")
NDIS_STATUS
NTAPI
MiniportInitialize(
    _Out_ PNDIS_STATUS OpenErrorStatus,
    _Out_ PUINT SelectedMediumIndex,
    _In_ PNDIS_MEDIUM MediumArray,
    _In_ UINT MediumArraySize,
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _In_ NDIS_HANDLE WrapperConfigurationContext);

CODE_SEG("PAGE")
VOID
NvNetFreeAdapter(
    _In_ PNVNET_ADAPTER Adapter);

CODE_SEG("PAGE")
NDIS_STATUS
NvNetRecognizeHardware(
    _Inout_ PNVNET_ADAPTER Adapter);

CODE_SEG("PAGE")
NDIS_STATUS
NvNetGetPermanentMacAddress(
    _Inout_ PNVNET_ADAPTER Adapter,
    _Out_writes_bytes_all_(ETH_LENGTH_OF_ADDRESS) PUCHAR MacAddress);

CODE_SEG("PAGE")
VOID
NvNetSetupMacAddress(
    _In_ PNVNET_ADAPTER Adapter,
    _In_reads_bytes_(ETH_LENGTH_OF_ADDRESS) PUCHAR MacAddress);

CODE_SEG("PAGE")
NDIS_STATUS
NvNetInitNIC(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ BOOLEAN InitPhy);

CODE_SEG("PAGE")
NDIS_STATUS
NvNetFindPhyDevice(
    _In_ PNVNET_ADAPTER Adapter);

CODE_SEG("PAGE")
NDIS_STATUS
NvNetPhyInit(
    _In_ PNVNET_ADAPTER Adapter);

VOID
SidebandUnitReleaseSemaphore(
    _In_ PNVNET_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
NvNetStartAdapter(
    _In_ PNVNET_ADAPTER Adapter);

DECLSPEC_NOINLINE
VOID
NvNetPauseProcessing(
    _In_ PNVNET_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
NvNetStopAdapter(
    _In_ PNVNET_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
NvNetFlushTransmitQueue(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NDIS_STATUS CompleteStatus);

KSYNCHRONIZE_ROUTINE NvNetInitPhaseSynchronized;
NDIS_TIMER_FUNCTION NvNetMediaDetectionDpc;

BOOLEAN
MiiWrite(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _In_ ULONG Data);

BOOLEAN
MiiRead(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _Out_ PULONG Data);

BOOLEAN
NvNetUpdateLinkSpeed(
    _In_ PNVNET_ADAPTER Adapter);

VOID
NvNetResetReceiverAndTransmitter(
    _In_ PNVNET_ADAPTER Adapter);

VOID
NvNetStartReceiver(
    _In_ PNVNET_ADAPTER Adapter);

VOID
NvNetStartTransmitter(
    _In_ PNVNET_ADAPTER Adapter);

VOID
NvNetStopReceiver(
    _In_ PNVNET_ADAPTER Adapter);

VOID
NvNetStopTransmitter(
    _In_ PNVNET_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
NvNetIdleTransmitter(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ BOOLEAN ClearPhyControl);

VOID
NvNetUpdatePauseFrame(
    _Inout_ PNVNET_ADAPTER Adapter,
    _In_ ULONG PauseFlags);

VOID
NvNetToggleClockPowerGating(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ BOOLEAN Gate);

VOID
NvNetSetPowerState(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NDIS_DEVICE_POWER_STATE NewPowerState,
    _In_ ULONG WakeFlags);

CODE_SEG("PAGE")
VOID
NvNetBackoffSetSlotTime(
    _In_ PNVNET_ADAPTER Adapter);

VOID
NvNetBackoffReseed(
    _In_ PNVNET_ADAPTER Adapter);

VOID
NvNetBackoffReseedEx(
    _In_ PNVNET_ADAPTER Adapter);

NDIS_STATUS
NTAPI
MiniportSend(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet,
    _In_ UINT Flags);

VOID
NTAPI
MiniportISR(
    _Out_ PBOOLEAN InterruptRecognized,
    _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
    _In_ NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportHandleInterrupt(
    _In_ NDIS_HANDLE MiniportAdapterContext);

NDIS_STATUS
NTAPI
MiniportQueryInformation(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesWritten,
    _Out_ PULONG BytesNeeded);

NDIS_STATUS
NTAPI
MiniportSetInformation(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesRead,
    _Out_ PULONG BytesNeeded);

#define NV_IMPLICIT_ENTRIES(Length) \
    (((Length - (NV_MAXIMUM_SG_SIZE + 1)) >> NV_TX2_TSO_MAX_SHIFT) + 1)

FORCEINLINE
VOID
NV_RELEASE_TCB(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNVNET_TCB Tcb)
{
    if (Tcb->Flags & NV_TCB_COALESCE)
    {
        PushEntryList(&Adapter->Send.BufferList, &Tcb->Buffer->Link);
    }

    Tcb->Packet = NULL;

    ++Adapter->Send.TcbSlots;

    Adapter->Send.TbdSlots += Tcb->Slots;

    Adapter->Send.StuckCount = 0;
}

FORCEINLINE
PNVNET_TCB
NV_NEXT_TCB(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ PNVNET_TCB Tcb)
{
    if (Tcb++ == Adapter->Send.TailTcb)
        return Adapter->Send.HeadTcb;
    else
        return Tcb;
}

FORCEINLINE
NVNET_TBD
NV_NEXT_TBD_32(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NVNET_TBD Tbd)
{
    if (Tbd.x32++ == Adapter->Send.TailTbd.x32)
        return Adapter->Send.HeadTbd;
    else
        return Tbd;
}

FORCEINLINE
NVNET_TBD
NV_NEXT_TBD_64(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NVNET_TBD Tbd)
{
    if (Tbd.x64++ == Adapter->Send.TailTbd.x64)
        return Adapter->Send.HeadTbd;
    else
        return Tbd;
}

FORCEINLINE
VOID
NV_WRITE(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NVNET_REGISTER Register,
    _In_ ULONG Value)
{
    NdisWriteRegisterUlong((PULONG)(Adapter->IoBase + Register), Value);
}

FORCEINLINE
ULONG
NV_READ(
    _In_ PNVNET_ADAPTER Adapter,
    _In_ NVNET_REGISTER Register)
{
    ULONG Value;

    NdisReadRegisterUlong((PULONG)(Adapter->IoBase + Register), &Value);
    return Value;
}

#define NvNetDisableInterrupts(Adapter) \
    NV_WRITE(Adapter, NvRegIrqMask, 0);

#define NvNetApplyInterruptMask(Adapter) \
    NV_WRITE(Adapter, NvRegIrqMask, (Adapter)->InterruptMask);

#endif /* _NVNET_PCH_ */
