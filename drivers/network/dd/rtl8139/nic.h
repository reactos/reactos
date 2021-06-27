/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Realtek 8139 driver
 * FILE:        rtl8139.h
 * PURPOSE:     RTL8139 driver definitions
 */

#ifndef _RTL8139_PCH_
#define _RTL8139_PCH_

#include <ndis.h>

#include "rtlhw.h"

#define ADAPTER_TAG 'Altr'
#define RESOURCE_LIST_TAG 'Rltr'

#define MAX_RESET_ATTEMPTS 25
#define RECEIVE_BUFFER_SIZE      (32768)
// The full receive window requires 16 bytes of padding
// plus 2048 for receive overflows (because of WRAP)
#define FULL_RECEIVE_BUFFER_SIZE (32768 + 16 + 2048)
#define RECV_CRC_LENGTH 4

#define MINIMUM_FRAME_SIZE 60
#define MAXIMUM_FRAME_SIZE 1514

#define DRIVER_VERSION 1

// 1/2 packet early RX, 512 byte FIFO threshold, 32K RX buffer, unlimited DMA bursts, WRAP
#define RC_VAL (0x800B780)

// 2048 byte DMA bursts
#define TC_VAL (0x700)

typedef struct _RTL_ADAPTER {
    NDIS_HANDLE MiniportAdapterHandle;
    NDIS_SPIN_LOCK Lock;

    ULONG IoRangeStart;
    ULONG IoRangeLength;

    ULONG InterruptVector;
    ULONG InterruptLevel;
    BOOLEAN InterruptShared;
    ULONG InterruptFlags;

    PUCHAR IoBase;
    NDIS_MINIPORT_INTERRUPT Interrupt;
    BOOLEAN InterruptRegistered;

    UCHAR PermanentMacAddress[IEEE_802_ADDR_LENGTH];
    UCHAR CurrentMacAddress[IEEE_802_ADDR_LENGTH];
    struct {
        UCHAR MacAddress[IEEE_802_ADDR_LENGTH];
    } MulticastList[MAXIMUM_MULTICAST_ADDRESSES];

    ULONG ReceiveBufferLength;
    PUCHAR ReceiveBuffer;
    NDIS_PHYSICAL_ADDRESS ReceiveBufferPa;
    USHORT ReceiveOffset;

    ULONG LinkSpeedMbps;
    ULONG MediaState;
    BOOLEAN LinkChange;

    ULONG PacketFilter;

    USHORT InterruptMask;
    USHORT InterruptPending;

    UCHAR DirtyTxDesc;
    UCHAR CurrentTxDesc;
    BOOLEAN TxFull;
    PUCHAR RuntTxBuffers;
    NDIS_PHYSICAL_ADDRESS RuntTxBuffersPa;

    ULONG ReceiveOk;
    ULONG TransmitOk;
    ULONG ReceiveError;
    ULONG TransmitError;
    ULONG ReceiveNoBufferSpace;
    ULONG ReceiveCrcError;
    ULONG ReceiveAlignmentError;
    ULONG TransmitOneCollision;
    ULONG TransmitMoreCollisions;

} RTL_ADAPTER, *PRTL_ADAPTER;

NDIS_STATUS
NTAPI
NICPowerOn (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICSoftReset (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICRegisterReceiveBuffer (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICRemoveReceiveBuffer (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICEnableTxRx (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICGetPermanentMacAddress (
    IN PRTL_ADAPTER Adapter,
    OUT PUCHAR MacAddress
    );

NDIS_STATUS
NTAPI
NICApplyPacketFilter (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICApplyInterruptMask (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICDisableInterrupts (
    IN PRTL_ADAPTER Adapter
    );

USHORT
NTAPI
NICInterruptRecognized (
    IN PRTL_ADAPTER Adapter,
    OUT PBOOLEAN InterruptRecognized
    );

VOID
NTAPI
NICAcknowledgeInterrupts (
    IN PRTL_ADAPTER Adapter
    );

VOID
NTAPI
NICUpdateLinkStatus (
    IN PRTL_ADAPTER Adapter
    );

NDIS_STATUS
NTAPI
NICTransmitPacket (
    IN PRTL_ADAPTER Adapter,
    IN UCHAR TxDesc,
    IN ULONG PhysicalAddress,
    IN ULONG Length
    );

NDIS_STATUS
NTAPI
MiniportSetInformation (
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesRead,
    OUT PULONG BytesNeeded
    );

NDIS_STATUS
NTAPI
MiniportQueryInformation (
    IN NDIS_HANDLE MiniportAdapterContext,
    IN NDIS_OID Oid,
    IN PVOID InformationBuffer,
    IN ULONG InformationBufferLength,
    OUT PULONG BytesWritten,
    OUT PULONG BytesNeeded
    );

VOID
NTAPI
MiniportISR (
    OUT PBOOLEAN InterruptRecognized,
    OUT PBOOLEAN QueueMiniportHandleInterrupt,
    IN NDIS_HANDLE MiniportAdapterContext
    );

VOID
NTAPI
MiniportHandleInterrupt (
    IN NDIS_HANDLE MiniportAdapterContext
    );

#endif /* _RTL8139_PCH_ */
