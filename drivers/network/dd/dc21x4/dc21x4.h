/*
 * PROJECT:     ReactOS DC21x4 Driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Main header file
 * COPYRIGHT:   Copyright 2023 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#if !DBG
#define NO_KERNEL_LIST_ENTRY_CHECKS
#endif
#include <ndis.h>
#include <section_attribs.h>

#include "dc21x4hw.h"
#include "eeprom.h"
#include "media.h"
#include "util.h"

#define DC21X4_TAG   '4x12'

#define DC_TRANSMIT_DESCRIPTORS    64
#define DC_TRANSMIT_BLOCKS         48
#define DC_TRANSMIT_BUFFERS        4
#define DC_LOOPBACK_FRAMES         4

#define DC_RECEIVE_BUFFERS_DEFAULT     64
#define DC_RECEIVE_BUFFERS_MIN         8
#define DC_RECEIVE_BUFFERS_EXTRA       16

#define DC_RECEIVE_ARRAY_SIZE      16

#define DC_MULTICAST_LIST_SIZE     36

#define DC_MAXIMUM_FRAME_SIZE      1514
#define DC_TRANSMIT_BLOCK_SIZE     1536
#define DC_RECEIVE_BLOCK_SIZE      1536
#define DC_ETHERNET_HEADER_SIZE    14

#define DC_TX_UNDERRUN_LIMIT    5
#define DC_INTERRUPT_PROCESSING_LIMIT    8

#define DC_FRAGMENTATION_THRESHOLD    32

#define DC_PACKET_FILTERS ( \
    NDIS_PACKET_TYPE_DIRECTED | \
    NDIS_PACKET_TYPE_MULTICAST | \
    NDIS_PACKET_TYPE_BROADCAST | \
    NDIS_PACKET_TYPE_PROMISCUOUS | \
    NDIS_PACKET_TYPE_ALL_MULTICAST)

#define DC_LOOPBACK_FRAME_SIZE    64

/* Transmit descriptors reserved for internal use */
#define DC_TBD_RESERVE     (2 + DC_LOOPBACK_FRAMES) /* (+2 for setup frame) */
#define DC_TCB_RESERVE     (1 + DC_LOOPBACK_FRAMES) /* (+1 for setup frame) */

#define DC_EVENT_SETUP_FRAME_COMPLETED    0x00000001

typedef struct _DC21X4_ADAPTER       DC21X4_ADAPTER, *PDC21X4_ADAPTER;
typedef struct _DC_TCB               DC_TCB, *PDC_TCB;
typedef struct _DC_RCB               DC_RCB, *PDC_RCB;
typedef struct _DC_COALESCE_BUFFER   DC_COALESCE_BUFFER, *PDC_COALESCE_BUFFER;

typedef VOID
(MEDIA_HANDLE_LINK_STATE_CHANGE)(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG InterruptStatus);
typedef MEDIA_HANDLE_LINK_STATE_CHANGE *PMEDIA_HANDLE_LINK_STATE_CHANGE;

typedef struct _DC_TX_BUFFER_DATA
{
    PVOID VirtualAddress;
    NDIS_PHYSICAL_ADDRESS PhysicalAddress;
} DC_TX_BUFFER_DATA, *PDC_TX_BUFFER_DATA;

typedef struct _DC_STATISTICS
{
    ULONG64 TransmitOk;
    ULONG64 TransmitDeferred;
    ULONG64 TransmitHeartbeatErrors;
    ULONG64 TransmitOneRetry;
    ULONG64 TransmitMoreCollisions;
    ULONG64 TransmitErrors;
    ULONG64 TransmitExcessiveCollisions;
    ULONG64 TransmitUnderrunErrors;
    ULONG64 TransmitLostCarrierSense;
    ULONG64 TransmitLateCollisions;
    ULONG64 ReceiveOk;
    ULONG64 ReceiveBroadcast;
    ULONG64 ReceiveMulticast;
    ULONG64 ReceiveUnicast;
    ULONG64 ReceiveErrors;
    ULONG64 ReceiveOverrunErrors;
    ULONG64 ReceiveNoBuffers;
    ULONG64 ReceiveCrcErrors;
    ULONG64 ReceiveAlignmentErrors;
} DC_STATISTICS, *PDC_STATISTICS;

typedef struct _DC21X4_ADAPTER
{
    PUCHAR IoBase;
    ULONG InterruptMask;
    ULONG CurrentInterruptMask;

    ULONG Features;
#define DC_NEED_RX_OVERFLOW_WORKAROUND              0x80000000
#define DC_SIA_GPIO                                 0x00000001
#define DC_SIA_ANALOG_CONTROL                       0x00000002
#define DC_HAS_POWER_MANAGEMENT                     0x00000004
#define DC_HAS_POWER_SAVING                         0x00000008
#define DC_HAS_MII                                  0x00000010
#define DC_PERFECT_FILTERING_ONLY                   0x00000020
#define DC_ENABLE_PCI_COMMANDS                      0x00000040
#define DC_MII_AUTOSENSE                            0x00000080
#define DC_HAS_TIMER                                0x00000100

    ULONG Flags;
#define DC_ACTIVE                                   0x80000000
#define DC_IO_MAPPED                                0x00000001
#define DC_IRQ_SHARED                               0x00000002
#define DC_FIRST_SETUP                              0x00000004
#define DC_AUTOSENSE                                0x00000008

    ULONG InterruptStatus;
    PMEDIA_HANDLE_LINK_STATE_CHANGE HandleLinkStateChange;

    DECLSPEC_CACHEALIGN NDIS_SPIN_LOCK SendLock;
    PDC_TCB TailTcb;
    PDC_TCB LastTcb;
    PDC_TCB CurrentTcb;
    PDC_TBD CurrentTbd;
    PDC_TBD HeadTbd;
    PDC_TBD TailTbd;
    LIST_ENTRY SendQueueList;
    ULONG TcbSlots;
    ULONG TbdSlots;
    ULONG TcbCompleted;
    ULONG LastTcbCompleted;
    PDC_TCB HeadTcb;
    SINGLE_LIST_ENTRY SendBufferList;
    SCATTER_GATHER_LIST LocalSgList;

    DECLSPEC_CACHEALIGN NDIS_SPIN_LOCK ReceiveLock;
    PDC_RCB* RcbArray;
    PDC_RBD CurrentRbd;
    PDC_RBD HeadRbd;
    PDC_RBD TailRbd;
    SINGLE_LIST_ENTRY FreeRcbList;
    ULONG RcbFree;

    ULONG TransmitUnderruns;
    ULONG PacketFilter;

    DC_STATISTICS Statistics;

    NDIS_HANDLE AdapterHandle;
    NDIS_HANDLE WrapperConfigurationHandle;

    DECLSPEC_CACHEALIGN NDIS_SPIN_LOCK ModeLock;
    ULONG ModeFlags;
#define DC_MODE_AUTONEG_MASK                 0x0000000F
#define DC_MODE_PORT_AUTOSENSE               0x00000010
#define DC_MODE_TEST_PACKET                  0x00000020
#define DC_MODE_AUI_FAILED                   0x00000040
#define DC_MODE_BNC_FAILED                   0x00000080

#define DC_MODE_AUTONEG_NONE                 0x00000000
#define DC_MODE_AUTONEG_WAIT_INTERRUPT       0x00000001
#define DC_MODE_AUTONEG_LINK_STATUS_CHECK    0x00000002

    ULONG OpMode;
    ULONG MediaNumber;
    ULONG MediaBitmap;
    BOOLEAN LinkUp;
    ULONG PhyAddress;
    ULONG SiaSetting;
    ULONG LastReceiveActivity;
    volatile LONG MediaTestStatus;
    NDIS_MINIPORT_TIMER MediaMonitorTimer;
    DC_MII_MEDIA MiiMedia;
    DC_MEDIA Media[MEDIA_LIST_MAX];

    ULONG AnalogControl;
    ULONG SymAdvertising;
    ULONG MiiAdvertising;
    ULONG MiiControl;
    DC_CHIP_TYPE ChipType;
    ULONG LinkStateChangeMask;

    ULONG WakeUpFlags;
    NDIS_DEVICE_POWER_STATE PowerState;
    NDIS_DEVICE_POWER_STATE PrevPowerState;

    ULONG HpnaInitBitmap;
    UCHAR HpnaRegister[32];

    UCHAR PermanentMacAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR CurrentMacAddress[ETH_LENGTH_OF_ADDRESS];

    ULONG MulticastMaxEntries;
    _Field_range_(0, MulticastMaxEntries)
    ULONG MulticastCount;
    struct
    {
        UCHAR MacAddress[ETH_LENGTH_OF_ADDRESS];
    } MulticastList[DC_MULTICAST_LIST_SIZE];

    ULONG LinkSpeedMbps;
    ULONG BusMode;
    ULONG DefaultMedia;
    ULONG RcbCount;
    BOOLEAN OidPending;
    BOOLEAN ProgramHashPerfectFilter;
    PULONG SetupFrame;
    PULONG SetupFrameSaved;
    ULONG SetupFramePhys;
    ULONG LoopbackFrameSlots;
    ULONG LoopbackFrameNumber;
    ULONG LoopbackFramePhys[DC_LOOPBACK_FRAMES];
    ULONG BusNumber;
    UCHAR DeviceNumber;
    UCHAR RevisionId;
    UCHAR ControllerIndex;
    UCHAR ResetStreamLength;
    USHORT ResetStream[SROM_MAX_STREAM_REGS];
    USHORT DeviceId;
    SINGLE_LIST_ENTRY AllocRcbList;
    SINGLE_LIST_ENTRY UsedRcbList;
    NDIS_MINIPORT_INTERRUPT Interrupt;
    ULONG InterruptVector;
    ULONG InterruptLevel;
    ULONG InterruptFlags;
    ULONG AdapterSize;
    NDIS_WORK_ITEM PowerWorkItem;
    NDIS_WORK_ITEM ResetWorkItem;
    NDIS_WORK_ITEM TxRecoveryWorkItem;
    _Interlocked_ volatile LONG ResetLock;
    NDIS_PHYSICAL_ADDRESS IoBaseAddress;
    PDC_SROM_ENTRY SRomEntry;
    PVOID AdapterOriginal;
    PVOID TbdOriginal;
    PVOID RbdOriginal;
    ULONG TbdPhys;
    ULONG RbdPhys;
    NDIS_HANDLE BufferPool;
    NDIS_HANDLE PacketPool;
    NDIS_PHYSICAL_ADDRESS TbdPhysOriginal;
    NDIS_PHYSICAL_ADDRESS RbdPhysOriginal;
    PVOID LoopbackFrame[DC_LOOPBACK_FRAMES];
    PDC_COALESCE_BUFFER CoalesceBuffer;
    DC_TX_BUFFER_DATA SendBufferData[DC_TRANSMIT_BUFFERS];
} DC21X4_ADAPTER, *PDC21X4_ADAPTER;

#include "sendrcv.h"

extern LIST_ENTRY SRompAdapterList;

FORCEINLINE
ULONG
DC_READ(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ DC_CSR Register)
{
    ULONG Value;

    NdisRawReadPortUlong((PULONG)(Adapter->IoBase + Register), &Value);
    return Value;
}

#define DC_WRITE(Adapter, Register, Value)  \
    NdisRawWritePortUlong((PULONG)((Adapter)->IoBase + (Register)), (Value));

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

CODE_SEG("PAGE")
NDIS_STATUS
NTAPI
DcInitialize(
    _Out_ PNDIS_STATUS OpenErrorStatus,
    _Out_ PUINT SelectedMediumIndex,
    _In_ PNDIS_MEDIUM MediumArray,
    _In_ UINT MediumArraySize,
    _In_ NDIS_HANDLE MiniportAdapterHandle,
    _In_ NDIS_HANDLE WrapperConfigurationContext);

VOID
NTAPI
DcSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PPNDIS_PACKET PacketArray,
    _In_ UINT NumberOfPackets);

VOID
NTAPI
DcCancelSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PVOID CancelId);

VOID
DcProcessPendingPackets(
    _In_ PDC21X4_ADAPTER Adapter);

VOID
NTAPI
DcReturnPacket(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet);

NDIS_STATUS
NTAPI
DcQueryInformation(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesWritten,
    _Out_ PULONG BytesNeeded);

NDIS_STATUS
NTAPI
DcSetInformation(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ NDIS_OID Oid,
    _In_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _Out_ PULONG BytesRead,
    _Out_ PULONG BytesNeeded);

VOID
NTAPI
DcIsr(
    _Out_ PBOOLEAN InterruptRecognized,
    _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
    _In_ NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
DcHandleInterrupt(
    _In_ NDIS_HANDLE MiniportAdapterContext);

CODE_SEG("PAGE")
VOID
NTAPI
DcPowerWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context);

NDIS_STATUS
DcSetPower(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ NDIS_DEVICE_POWER_STATE PowerState);

NDIS_STATUS
DcAddWakeUpPattern(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern);

NDIS_STATUS
DcRemoveWakeUpPattern(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ PNDIS_PM_PACKET_PATTERN PmPattern);

CODE_SEG("PAGE")
VOID
DcFreeAdapter(
    _In_ __drv_freesMem(Mem) PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
NTAPI
DcResetWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context);

DECLSPEC_NOINLINE
VOID
DcStopAdapter(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN WaitForPackets);

CODE_SEG("PAGE")
VOID
DcStartAdapter(
    _In_ PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
NDIS_STATUS
DcSetupAdapter(
    _In_ PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
NDIS_STATUS
DcReadEeprom(
    _In_ PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
DcFreeEeprom(
    _In_ PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
DcInitTxRing(
    _In_ PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
DcInitRxRing(
    _In_ PDC21X4_ADAPTER Adapter);

ULONG
DcEthernetCrc(
    _In_reads_bytes_(Size) const VOID* Buffer,
    _In_ ULONG Size);

VOID
DcDisableHw(
    _In_ PDC21X4_ADAPTER Adapter);

VOID
DcStopTxRxProcess(
    _In_ PDC21X4_ADAPTER Adapter);

VOID
DcWriteGpio(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG Value);

VOID
DcWriteSia(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG Csr13,
    _In_ ULONG Csr14,
    _In_ ULONG Csr15);

VOID
DcTestPacket(
    _In_ PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
DcSetupFrameInitialize(
    _In_ PDC21X4_ADAPTER Adapter);

BOOLEAN
DcSetupFrameDownload(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN WaitForCompletion);

NDIS_STATUS
DcApplyPacketFilter(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG PacketFilter);

NDIS_STATUS
DcUpdateMulticastList(
    _In_ PDC21X4_ADAPTER Adapter);

VOID
DcPowerSave(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN Enable);

CODE_SEG("PAGE")
BOOLEAN
DcFindMiiPhy(
    _In_ PDC21X4_ADAPTER Adapter);

BOOLEAN
MiiWrite(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _In_ ULONG Data);

BOOLEAN
MiiRead(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG PhyAddress,
    _In_ ULONG RegAddress,
    _Out_ PULONG Data);

CODE_SEG("PAGE")
VOID
HpnaPhyInit(
    _In_ PDC21X4_ADAPTER Adapter);

VOID
NTAPI
DcTransmitTimeoutRecoveryWorker(
    _In_ PNDIS_WORK_ITEM WorkItem,
    _In_opt_ PVOID Context);

NDIS_TIMER_FUNCTION MediaMonitor21040Dpc;
NDIS_TIMER_FUNCTION MediaMonitor21041Dpc;
NDIS_TIMER_FUNCTION MediaMonitor21140Dpc;
NDIS_TIMER_FUNCTION MediaMonitor21143Dpc;

MEDIA_HANDLE_LINK_STATE_CHANGE MediaLinkStateChange21040;
MEDIA_HANDLE_LINK_STATE_CHANGE MediaLinkStateChange21041;
MEDIA_HANDLE_LINK_STATE_CHANGE MediaLinkStateChange21143;

CODE_SEG("PAGE")
VOID
MediaInitMediaList(
    _In_ PDC21X4_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
MediaInitDefaultMedia(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ ULONG MediaNumber);

VOID
MediaIndicateConnect(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN LinkUp);

VOID
MediaSelectMiiPort(
    _In_ PDC21X4_ADAPTER Adapter,
    _In_ BOOLEAN ResetPhy);

VOID
MediaMiiSelect(
    _In_ PDC21X4_ADAPTER Adapter);

BOOLEAN
MediaMiiCheckLink(
    _In_ PDC21X4_ADAPTER Adapter);

VOID
MediaSiaSelect(
    _In_ PDC21X4_ADAPTER Adapter);

VOID
MediaGprSelect(
    _In_ PDC21X4_ADAPTER Adapter);
