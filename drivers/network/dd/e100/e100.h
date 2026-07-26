/*
 * PROJECT:     Intel PRO/100 Ethernet Controller Driver
 * LICENSE:     BSD-2-Clause (https://spdx.org/licenses/BSD-2-Clause)
 * PURPOSE:     Main header file
 * COPYRIGHT:   Copyright 2026 Dmitry Borisov <di.sean@protonmail.com>
 */

#pragma once

#if !DBG
#define NO_KERNEL_LIST_ENTRY_CHECKS
#endif
#include <ndis.h>
#include <section_attribs.h>

#define E100_TAG   '001e'

#define E100_TRANSMIT_BLOCKS_MIN     8
#define E100_TRANSMIT_BLOCKS_MAX     128
#define E100_TRANSMIT_BLOCKS_DEFAULT 128
#define E100_RECEIVE_BUFFERS_MIN     8
#define E100_RECEIVE_BUFFERS_MAX     128
#define E100_RECEIVE_BUFFERS_DEFAULT 64
#define E100_TBD_PER_TCB             29
#define E100_TRANSMIT_BUFFERS        4

#define E100_INTERRUPT_PROCESSING_LIMIT   8
#define E100_RECEIVE_PROCESSING_LIMIT     4
#define E100_RECEIVE_ARRAY_SIZE           16

#define E100_MULTICAST_LIST_SIZE          32
#define E100_MAXIMUM_FRAME_SIZE           1514
#define E100_TRANSMIT_BLOCK_SIZE          E100_MAXIMUM_FRAME_SIZE
#define E100_RECEIVE_BLOCK_SIZE           1536
#define E100_ETHERNET_HEADER_SIZE         14
#define E100_MAXIMUM_VLAN_ID              0xFFF

/* Make RFD unaligned to align the user data after the 14-byte ethernet header */
#define E100_RECEIVE_BUFFER_SHIFT       0xA
#define E100_RECEIVE_BUFFER_DATA_ALIGN  8

#define E100_PACKET_FILTERS ( \
    NDIS_PACKET_TYPE_DIRECTED | \
    NDIS_PACKET_TYPE_MULTICAST | \
    NDIS_PACKET_TYPE_BROADCAST | \
    NDIS_PACKET_TYPE_PROMISCUOUS | \
    NDIS_PACKET_TYPE_ALL_MULTICAST)

#include "e100hw.h"

/* We make the TCB size to be a multiple of 128 bytes for performance */
#define E100_TCB_ALIGNMENT   128
C_ASSERT(sizeof(FXP_CB_TRANSMIT) % E100_TCB_ALIGNMENT == 0);

/* Pageable read-only data */
#define E100_PAGED_DATA    DATA_SEG("PAGECONS")
#if defined(_MSC_VER)
#pragma section("PAGECONS", read)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
/* Strict memory model, does not reorder Write-Write operations */
#define EE_WRITE_BARRIER()    KeMemoryBarrierWithoutFence()
#else
#define EE_WRITE_BARRIER()    KeMemoryBarrier()
#endif

#define E100_LIST_ENTRY_FROM_PACKET(Packet) \
    ((PLIST_ENTRY)(&(Packet)->MiniportReservedEx[0]))

#define E100_PACKET_FROM_LIST_ENTRY(ListEntry) \
    (CONTAINING_RECORD(ListEntry, NDIS_PACKET, MiniportReservedEx))

#define E100_RX_CONTEXT_FROM_PACKET(Packet) \
    ((PE100_RX_CONTEXT*)&(Packet)->MiniportReservedEx[0])

#define MII_FLAG_FAILED     0x80000000
#define MII_SUCCESS(Status) (((Status) & MII_FLAG_FAILED) == 0)

#define ETH_IS_LOCALLY_ADMINISTERED(Address) \
    ((BOOLEAN)(((PUCHAR)(Address))[0] & ((UCHAR)0x02)))

#define ETH_IS_EMPTY(Address) \
    ((BOOLEAN)((((PUCHAR)(Address))[0] | ((PUCHAR)(Address))[1] | ((PUCHAR)(Address))[2] | \
                ((PUCHAR)(Address))[3] | ((PUCHAR)(Address))[4] | ((PUCHAR)(Address))[5]) == 0))

#define E100_MEDIA_AUTO        0x01
#define E100_MEDIA_FD          0x02
#define E100_MEDIA_100T        0x04
#define E100_MEDIA_PAUSE_AUTO  0x08
#define E100_MEDIA_PAUSE_TX    0x10
#define E100_MEDIA_PAUSE_RX    0x20
#define E100_MEDIA_NONE        (0xFF & ~(E100_MEDIA_FD | E100_MEDIA_100T))

#define E100_PHY_TYPE_UNKNOWN    0
#define E100_PHY_TYPE_503        1
#define E100_PHY_TYPE_DP83840    2

#define E100_FLAG_IS_ICH              0x00000001 // ICH controller
#define E100_FLAG_NO_UCODE            0x00000002 // Microcode is not applicable
#define E100_FLAG_RX_LOCKUP_BUG       0x00000004 // Has receiver lock-up errata
#define E100_FLAG_CU_RESUME_BUG       0x00000008 // Has FXP_SCB_COMMAND_CU_RESUME errata
#define E100_FLAG_IRQ_SHARED          0x00000010 // Shared IRQ
#define E100_FLAG_HAS_WOL             0x00000020 // Wake on LAN capabilities
#define E100_FLAG_WMI_ENANLE          0x00000040 // Enable PCI WMI
#define E100_FLAG_SAVE_BAD_PACKETS    0x00000080 // Save bad packets: bad size, CRC, etc
#define E100_FLAG_EXT_TXCB            0x00000100 // Enable extended TxCB mode
#define E100_FLAG_LONG_PKT            0x00000200 // Enable long packet reception
#define E100_FLAG_82559_RXCSUM        0x00000400 // 82559 compatible RX checksum
#define E100_FLAG_EXT_RFA             0x00000800 // Extended RFDs for checksum offload
#define E100_FLAG_HAS_FLOW_CONTROL    0x00001000 // Has flow control
#define E100_FLAG_VLAN_TAGGING        0x00002000 // 802.1Q VLAN tagging
#define E100_FLAG_PACKET_PRIORITY     0x00004000 // 802.1Q packet priority

typedef struct _E100_TX_CONTEXT E100_TX_CONTEXT, *PE100_TX_CONTEXT;

typedef struct _E100_COALESCE_BUFFER
{
    SINGLE_LIST_ENTRY ListEntry;

    PVOID VirtualAddress;
    ULONG PhysicalAddress;

    ULONG PhysicalAddressOriginal;
    PVOID VirtualAddressOriginal;
} E100_COALESCE_BUFFER, *PE100_COALESCE_BUFFER;

typedef struct _E100_TX_CONTEXT
{
    PE100_TX_CONTEXT Next;
    PFXP_CB_TRANSMIT Tcb;
    PNDIS_PACKET Packet;
    ULONG TcbPhys;

    PE100_COALESCE_BUFFER Buffer;
} E100_TX_CONTEXT, *PE100_TX_CONTEXT;

typedef struct _E100_RX_CONTEXT
{
    LIST_ENTRY ListEntry;

    ULONG Flags;
#define E100_RX_FLAG_82559_CRC  0x00000001 // TODO not used yet
#define E100_RX_FLAG_RECLAIM    0x80000000

    PNDIS_PACKET Packet;
    PNDIS_BUFFER RfdMdl;
    PNDIS_BUFFER ReceiveBufferMdl;

    PFXP_RFD Rfd;
    ULONG RfdPhys;

    ULONG PhysicalAddressOriginal;
    PVOID VirtualAddressOriginal;
} E100_RX_CONTEXT, *PE100_RX_CONTEXT;

typedef struct _E100_CONTROL_BLOCK
{
    DECLSPEC_CACHEALIGN FXP_COUNTERS Counters;
    DECLSPEC_CACHEALIGN FXP_CB_CONFIGURE Configuration;
    union
    {
        DECLSPEC_CACHEALIGN FXP_CB_INDIVIDUAL_ADDRESS_SETUP IaSetup;
        DECLSPEC_CACHEALIGN FXP_CB_MULTICAST_SETUP MulticastSetup;
        DECLSPEC_CACHEALIGN FXP_CB_LOAD_MICROCODE MicrocodeSetup;
    };
} E100_CONTROL_BLOCK, *PE100_CONTROL_BLOCK;

typedef struct _E100_STATISTICS
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
    ULONG64 ReceiveErrors;
    ULONG64 ReceiveOverrunErrors;
    ULONG64 ReceiveNoBuffers;
    ULONG64 ReceiveCrcErrors;
    ULONG64 ReceiveAlignmentErrors;
} E100_STATISTICS, *PE100_STATISTICS;

typedef struct _E100_ADAPTER
{
    PUCHAR IoBase;
    ULONG Flags;
    BOOLEAN AdapterActive;
    BOOLEAN CommandUnitActive;
    BOOLEAN HardError;
    UCHAR CurrentMedia;
    USHORT TransmitCommand;
    UCHAR TransmitThreshold;
    UCHAR DefaultMedia;
    UCHAR PhyType;
    UCHAR RevisionID;
    ULONG VlanId;

    DECLSPEC_CACHEALIGN NDIS_SPIN_LOCK SendLock;
    PE100_TX_CONTEXT TxContext;
    PE100_TX_CONTEXT TxFirst;
    PE100_TX_CONTEXT TxLast;
    ULONG TxPending;
    ULONG TcbCount;
    LIST_ENTRY SendQueueList;

    DECLSPEC_CACHEALIGN NDIS_SPIN_LOCK ReceiveLock;
    PE100_RX_CONTEXT RxContext;
    LIST_ENTRY RxContextList;

    UCHAR InterruptStatus;
    UCHAR ReceiverUnitIdleTicks;
    E100_STATISTICS Statistics;
    NDIS_MINIPORT_INTERRUPT Interrupt;
    PE100_CONTROL_BLOCK ControlBlock;
    ULONG ControlBlockPa;
    ULONG PacketFilter;
    ULONG RfdSize;
    PFXP_CB_TRANSMIT HeadTcb;
    PVOID TcbOriginalVa;
    ULONG TcbOriginalPhys;
    ULONG HeadTcbPa;
    ULONG PhyCapabilities;
    ULONG PhyAddress;
    ULONG RfdCount;
    ULONG RfdToAllocate;
    ULONG TbdCount;
    ULONG MicrocodeInterruptDelay;
    ULONG MicrocodeMaxFramesPerIntr;
    ULONG MicrocodeMinSizeMask;
    UCHAR PermanentMacAddress[ETH_LENGTH_OF_ADDRESS];
    UCHAR CurrentMacAddress[ETH_LENGTH_OF_ADDRESS];
    ULONG MulticastListSize;
    struct
    {
        UCHAR MacAddress[ETH_LENGTH_OF_ADDRESS];
    } MulticastList[E100_MULTICAST_LIST_SIZE];
    ULONG WakeUpFlags;
    NDIS_DEVICE_POWER_STATE PowerState;
    NDIS_DEVICE_POWER_STATE PrevPowerState;
    SINGLE_LIST_ENTRY SendBufferList;
    SCATTER_GATHER_LIST LocalSgList;
    E100_COALESCE_BUFFER CoalesceBuffer[E100_TRANSMIT_BUFFERS];
    ULONG EepromAddressBusWidth;
    _Interlocked_ volatile LONG ResetLock;
    ULONG InterruptVector;
    ULONG InterruptLevel;
    ULONG InterruptFlags;
    USHORT DeviceID;
    ULONG ControlBlockOriginalPhys;
    PVOID ControlBlockOriginal;
    NDIS_HANDLE BufferPool;
    NDIS_HANDLE PacketPool;
    NDIS_HANDLE AdapterHandle;
    NDIS_HANDLE WrapperConfigurationHandle;
    PVOID AdapterOriginal;
    ULONG AdapterSize;
} E100_ADAPTER, *PE100_ADAPTER;

typedef struct _E100_UCODE_INFO
{
    const ULONG *Microcode;
    ULONG Length;
    USHORT RevisionID;
    USHORT IntDelayOffset;
    USHORT BundleMaxOffset;
    USHORT MinSizeMaskOffset;
} E100_UCODE_INFO, *PE100_UCODE_INFO;

CODE_SEG("INIT")
DRIVER_INITIALIZE DriverEntry;

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

BOOLEAN
NTAPI
MiniportCheckForHang(
    _In_ NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PPNDIS_PACKET PacketArray,
    _In_ UINT NumberOfPackets);

VOID
NTAPI
MiniportCancelSendPackets(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PVOID CancelId);

VOID
NTAPI
MiniportReturnPacket(
    _In_ NDIS_HANDLE MiniportAdapterContext,
    _In_ PNDIS_PACKET Packet);

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

VOID
NTAPI
MiniportIsr(
    _Out_ PBOOLEAN InterruptRecognized,
    _Out_ PBOOLEAN QueueMiniportHandleInterrupt,
    _In_ NDIS_HANDLE MiniportAdapterContext);

VOID
NTAPI
MiniportHandleInterrupt(
    _In_ NDIS_HANDLE MiniportAdapterContext);

CODE_SEG("PAGE")
VOID
EeFreeAdapter(
    _In_ __drv_freesMem(Mem) PE100_ADAPTER Adapter);

CODE_SEG("PAGE")
BOOLEAN
EeReadEeprom(
    _In_ PE100_ADAPTER Adapter);

CODE_SEG("PAGE")
NDIS_STATUS
EeFindMiiPhy(
    _In_ PE100_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
EePhyInit(
    _In_ PE100_ADAPTER Adapter);

CODE_SEG("PAGE")
NDIS_STATUS
EeSetupAdapter(
    _In_ PE100_ADAPTER Adapter);

CODE_SEG("PAGE")
VOID
EeStartAdapter(
    _In_ PE100_ADAPTER Adapter);

VOID
EeSoftReset(
    _In_ PE100_ADAPTER Adapter);

NDIS_STATUS
EeUpdateMulticastList(
    _In_ PE100_ADAPTER Adapter);

NDIS_STATUS
EeApplyPacketFilter(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG PacketFilter);

UCHAR
EePhyGetSpeedAndDuplex(
    _In_ PE100_ADAPTER Adapter);

BOOLEAN
EeScbWaitForCommandClear(
    _In_ PE100_ADAPTER Adapter);

VOID
EeSendPackets(
    _In_ PE100_ADAPTER Adapter,
    _In_ PPNDIS_PACKET PacketArray,
    _In_ UINT NumberOfPackets);

FORCEINLINE
VOID
E100_RELEASE_TX_CONTEXT(
    _In_ PE100_ADAPTER Adapter,
    _In_ PE100_TX_CONTEXT TxContext,
    _In_ PFXP_CB_TRANSMIT Tcb)
{
    /* Reset the IPCB checksum offload bits */
    Tcb->Tbd[0].Address = 0;

    if (TxContext->Buffer)
    {
        PushEntryList(&Adapter->SendBufferList, &TxContext->Buffer->ListEntry);
    }

    ASSERT(Adapter->TxPending > 0);
    --Adapter->TxPending;
}

FORCEINLINE
UCHAR
CSR_READ_8(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG Register)
{
    UCHAR Value;

    NdisReadRegisterUchar((PUCHAR)(Adapter->IoBase + Register), &Value);
    return Value;
}

FORCEINLINE
USHORT
CSR_READ_16(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG Register)
{
    USHORT Value;

    NdisReadRegisterUlong((PUSHORT)(Adapter->IoBase + Register), &Value);
    return Value;
}

FORCEINLINE
ULONG
CSR_READ_32(
    _In_ PE100_ADAPTER Adapter,
    _In_ ULONG Register)
{
    ULONG Value;

    NdisReadRegisterUlong((PULONG)(Adapter->IoBase + Register), &Value);
    return Value;
}

#define CSR_WRITE_8(Adapter, Register, Value)  \
    NdisWriteRegisterUchar((PUCHAR)((Adapter)->IoBase + (Register)), (Value));

#define CSR_WRITE_16(Adapter, Register, Value)  \
    NdisWriteRegisterUshort((PUSHORT)((Adapter)->IoBase + (Register)), (Value));

#define CSR_WRITE_32(Adapter, Register, Value)  \
    NdisWriteRegisterUlong((PULONG)((Adapter)->IoBase + (Register)), (Value));
